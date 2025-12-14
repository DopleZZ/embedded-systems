#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/ledc.h"
#include "esp_adc_cal.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "mqtt_client.h"
#include "nvs_flash.h"

#include "bme280_support.h"

static const char *TAG = "soil_sensor";

#ifndef CONFIG_SOIL_SENSOR_PUMP_PWM_FREQ_HZ
#define CONFIG_SOIL_SENSOR_PUMP_PWM_FREQ_HZ 20000
#endif

#ifndef CONFIG_SOIL_SENSOR_PUMP_POWER_PERCENT
#define CONFIG_SOIL_SENSOR_PUMP_POWER_PERCENT 60
#endif

#if CONFIG_SOIL_SENSOR_ADC_CH0
#define SOIL_SENSOR_ADC_CHANNEL ADC1_CHANNEL_0  // GPIO36
#define SOIL_SENSOR_GPIO        36
#elif CONFIG_SOIL_SENSOR_ADC_CH1
#define SOIL_SENSOR_ADC_CHANNEL ADC1_CHANNEL_1  // GPIO37
#define SOIL_SENSOR_GPIO        37
#elif CONFIG_SOIL_SENSOR_ADC_CH2
#define SOIL_SENSOR_ADC_CHANNEL ADC1_CHANNEL_2  // GPIO38
#define SOIL_SENSOR_GPIO        38
#elif CONFIG_SOIL_SENSOR_ADC_CH3
#define SOIL_SENSOR_ADC_CHANNEL ADC1_CHANNEL_3  // GPIO39
#define SOIL_SENSOR_GPIO        39
#elif CONFIG_SOIL_SENSOR_ADC_CH4
#define SOIL_SENSOR_ADC_CHANNEL ADC1_CHANNEL_4  // GPIO32
#define SOIL_SENSOR_GPIO        32
#elif CONFIG_SOIL_SENSOR_ADC_CH5
#define SOIL_SENSOR_ADC_CHANNEL ADC1_CHANNEL_5  // GPIO33
#define SOIL_SENSOR_GPIO        33
#elif CONFIG_SOIL_SENSOR_ADC_CH6
#define SOIL_SENSOR_ADC_CHANNEL ADC1_CHANNEL_6  // GPIO34
#define SOIL_SENSOR_GPIO        34
#elif CONFIG_SOIL_SENSOR_ADC_CH7
#define SOIL_SENSOR_ADC_CHANNEL ADC1_CHANNEL_7  // GPIO35
#define SOIL_SENSOR_GPIO        35
#else
#warning "Конфигурация ADC не выбрана, используем ADC1_CH6 (GPIO34) по умолчанию"
#define SOIL_SENSOR_ADC_CHANNEL ADC1_CHANNEL_6
#define SOIL_SENSOR_GPIO        34
#endif

#if CONFIG_SOIL_SENSOR_BME280_ENABLED
#define BME280_I2C_PORT I2C_NUM_0
#define BME280_I2C_FREQ_HZ 100000
#endif

static const TickType_t kSampleDelayTicks = pdMS_TO_TICKS(CONFIG_SOIL_SENSOR_SAMPLE_PERIOD_MS);
static const TickType_t kWifiConnectTimeoutTicks = pdMS_TO_TICKS(10000);
static const char *kCmdGetInfo = "get_info";
static EventGroupHandle_t wifi_event_group;
static const int WIFI_CONNECTED_BIT = BIT0;

static int64_t last_publish_timestamp_ms = 0;
static esp_adc_cal_characteristics_t adc_chars;
static esp_mqtt_client_handle_t mqtt_client = nullptr;
static bool mqtt_connected = false;
static char device_uid[32];

#if CONFIG_SOIL_SENSOR_PUMP_ENABLED
static const char *kCmdWater = "water";
static const char *kCmdWaterStop = "water_stop";
static bool pump_running = false;
static int64_t pump_stop_at_ms = 0;
static portMUX_TYPE pump_mux = portMUX_INITIALIZER_UNLOCKED;
static bool pump_pwm_ready = false;
static uint32_t pump_on_duty = 0;
static uint32_t pump_target_duty = 0;
static int64_t pump_boost_until_ms = 0;

static const ledc_mode_t kPumpPwmMode = LEDC_LOW_SPEED_MODE;
static const ledc_timer_t kPumpPwmTimer = LEDC_TIMER_0;
static const ledc_channel_t kPumpPwmChannel = LEDC_CHANNEL_0;
static const ledc_timer_bit_t kPumpPwmResolution = LEDC_TIMER_10_BIT;

static void pump_early_safe_state(void)
{
    gpio_reset_pin((gpio_num_t)CONFIG_SOIL_SENSOR_PUMP_IN1);
    gpio_reset_pin((gpio_num_t)CONFIG_SOIL_SENSOR_PUMP_IN2);

    gpio_set_direction((gpio_num_t)CONFIG_SOIL_SENSOR_PUMP_IN1, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)CONFIG_SOIL_SENSOR_PUMP_IN2, GPIO_MODE_OUTPUT);

    gpio_set_level((gpio_num_t)CONFIG_SOIL_SENSOR_PUMP_IN1, 0);
    gpio_set_level((gpio_num_t)CONFIG_SOIL_SENSOR_PUMP_IN2, 0);

    gpio_pulldown_en((gpio_num_t)CONFIG_SOIL_SENSOR_PUMP_IN1);
    gpio_pulldown_en((gpio_num_t)CONFIG_SOIL_SENSOR_PUMP_IN2);
    gpio_pullup_dis((gpio_num_t)CONFIG_SOIL_SENSOR_PUMP_IN1);
    gpio_pullup_dis((gpio_num_t)CONFIG_SOIL_SENSOR_PUMP_IN2);
}

static void pump_set_off(void)
{
    if (pump_pwm_ready)
    {
        ledc_set_duty(kPumpPwmMode, kPumpPwmChannel, 0);
        ledc_update_duty(kPumpPwmMode, kPumpPwmChannel);
    }
    else
    {
        gpio_set_level((gpio_num_t)CONFIG_SOIL_SENSOR_PUMP_IN1, 0);
    }
    gpio_set_level((gpio_num_t)CONFIG_SOIL_SENSOR_PUMP_IN2, 0);
}

static void pump_stop(void)
{
    portENTER_CRITICAL(&pump_mux);
    pump_running = false;
    pump_stop_at_ms = 0;
    pump_boost_until_ms = 0;
    portEXIT_CRITICAL(&pump_mux);
    pump_set_off();
    ESP_LOGI(TAG, "Помпа выключена");
}

static void pump_start(uint32_t duration_ms)
{
    if (duration_ms == 0)
    {
        pump_stop();
        return;
    }

    uint32_t max_ms = (uint32_t)CONFIG_SOIL_SENSOR_PUMP_MAX_DURATION_MS;
    if (max_ms > 0 && duration_ms > max_ms)
    {
        duration_ms = max_ms;
    }

    if (pump_pwm_ready)
    {
        gpio_set_level((gpio_num_t)CONFIG_SOIL_SENSOR_PUMP_IN2, 0);
        uint32_t max_duty = (1U << (uint32_t)kPumpPwmResolution) - 1U;
        ledc_set_duty(kPumpPwmMode, kPumpPwmChannel, max_duty);
        ledc_update_duty(kPumpPwmMode, kPumpPwmChannel);
    }
    else
    {
        gpio_set_level((gpio_num_t)CONFIG_SOIL_SENSOR_PUMP_IN1, 1);
        gpio_set_level((gpio_num_t)CONFIG_SOIL_SENSOR_PUMP_IN2, 0);
    }

    int64_t now_ms = esp_timer_get_time() / 1000;
    portENTER_CRITICAL(&pump_mux);
    pump_running = true;
    pump_stop_at_ms = now_ms + (int64_t)duration_ms;
    if (pump_pwm_ready)
    {
        pump_target_duty = pump_on_duty;
        pump_boost_until_ms = now_ms + 200;  // небольшой "пинок" для старта мотора
    }
    portEXIT_CRITICAL(&pump_mux);
    ESP_LOGI(TAG, "Помпа включена на %u ms", (unsigned)duration_ms);
}

static void pump_task(void *param)
{
    (void)param;
    const TickType_t delay = pdMS_TO_TICKS(50);

    while (true)
    {
        bool should_stop = false;
        bool should_drop_boost = false;
        int64_t now_ms = esp_timer_get_time() / 1000;

        portENTER_CRITICAL(&pump_mux);
        if (pump_running && pump_stop_at_ms > 0 && now_ms >= pump_stop_at_ms)
        {
            should_stop = true;
        }
        if (pump_running && pump_pwm_ready && pump_boost_until_ms > 0 && now_ms >= pump_boost_until_ms)
        {
            should_drop_boost = true;
            pump_boost_until_ms = 0;
        }
        portEXIT_CRITICAL(&pump_mux);

        if (should_stop)
        {
            pump_stop();
        }
        else if (should_drop_boost)
        {
            ledc_set_duty(kPumpPwmMode, kPumpPwmChannel, pump_target_duty);
            ledc_update_duty(kPumpPwmMode, kPumpPwmChannel);
        }

        vTaskDelay(delay);
    }
}

static void pump_init(void)
{
    gpio_config_t conf = {};
    conf.mode = GPIO_MODE_OUTPUT;
    conf.pin_bit_mask = (1ULL << CONFIG_SOIL_SENSOR_PUMP_IN1) | (1ULL << CONFIG_SOIL_SENSOR_PUMP_IN2);
    conf.intr_type = GPIO_INTR_DISABLE;
    conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    conf.pull_up_en = GPIO_PULLUP_DISABLE;

    esp_err_t err = gpio_config(&conf);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Не удалось настроить GPIO для помпы (%s)", esp_err_to_name(err));
        return;
    }

    pump_set_off();

    ledc_timer_config_t timer_cfg = {};
    timer_cfg.speed_mode = kPumpPwmMode;
    timer_cfg.timer_num = kPumpPwmTimer;
    timer_cfg.duty_resolution = kPumpPwmResolution;
    timer_cfg.freq_hz = CONFIG_SOIL_SENSOR_PUMP_PWM_FREQ_HZ;
    timer_cfg.clk_cfg = LEDC_AUTO_CLK;

    esp_err_t ledc_err = ledc_timer_config(&timer_cfg);
    if (ledc_err != ESP_OK)
    {
        ESP_LOGW(TAG, "PWM для помпы недоступен (%s), используем on/off", esp_err_to_name(ledc_err));
        pump_pwm_ready = false;
    }
    else
    {
        ledc_channel_config_t channel_cfg = {};
        channel_cfg.speed_mode = kPumpPwmMode;
        channel_cfg.channel = kPumpPwmChannel;
        channel_cfg.timer_sel = kPumpPwmTimer;
        channel_cfg.intr_type = LEDC_INTR_DISABLE;
        channel_cfg.gpio_num = CONFIG_SOIL_SENSOR_PUMP_IN1;
        channel_cfg.duty = 0;
        channel_cfg.hpoint = 0;

        ledc_err = ledc_channel_config(&channel_cfg);
        if (ledc_err != ESP_OK)
        {
            ESP_LOGW(TAG, "PWM канал для помпы недоступен (%s), используем on/off", esp_err_to_name(ledc_err));
            pump_pwm_ready = false;
        }
        else
        {
            pump_pwm_ready = true;
        }
    }

    uint32_t max_duty = (1U << (uint32_t)kPumpPwmResolution) - 1U;
    uint32_t pct = (uint32_t)CONFIG_SOIL_SENSOR_PUMP_POWER_PERCENT;
    if (pct < 1U)
    {
        pct = 1U;
    }
    if (pct > 100U)
    {
        pct = 100U;
    }
    pump_on_duty = (max_duty * pct) / 100U;
    if (pump_on_duty == 0U)
    {
        pump_on_duty = 1U;
    }

    xTaskCreate(pump_task, "pump_task", 2048, nullptr, tskIDLE_PRIORITY + 1, nullptr);
}
#endif

static void allow_immediate_publish(void)
{
    int64_t now_ms = esp_timer_get_time() / 1000;
    last_publish_timestamp_ms = now_ms - CONFIG_SOIL_SENSOR_PUBLISH_INTERVAL_MS;
}

typedef struct
{
    int raw;
    float percent;
} soil_moisture_t;

typedef struct
{
    soil_moisture_t soil;
    float air_temperature_c;
    float air_humidity_percent;
    int64_t timestamp_ms;
} measurement_report_t;

static void init_device_uid(void)
{
    uint8_t mac[6] = {0};
    esp_efuse_mac_get_default(mac);
    snprintf(device_uid,
             sizeof(device_uid),
             "esp32-%02X%02X%02X%02X%02X%02X",
             mac[0],
             mac[1],
             mac[2],
             mac[3],
             mac[4],
             mac[5]);
    ESP_LOGI(TAG, "Device UID: %s", device_uid);
}

static void init_adc(void)
{
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(SOIL_SENSOR_ADC_CHANNEL, ADC_ATTEN_DB_11);
    esp_adc_cal_characterize(
        ADC_UNIT_1,
        ADC_ATTEN_DB_11,
        ADC_WIDTH_BIT_12,
        CONFIG_SOIL_SENSOR_VREF_MV,
        &adc_chars);
}

static float compute_soil_percent(int raw)
{
    float dry = (float)CONFIG_SOIL_SENSOR_SOIL_DRY_RAW;
    float wet = (float)CONFIG_SOIL_SENSOR_SOIL_WET_RAW;
    if (wet <= dry)
    {
        return 0.0f;
    }
    float percent = ((float)raw - dry) / (wet - dry) * 100.0f;
    if (percent < 0.0f)
    {
        percent = 0.0f;
    }
    if (percent > 100.0f)
    {
        percent = 100.0f;
    }
    return percent;
}

static void format_iso8601(int64_t timestamp_ms, char *out, size_t len)
{
    time_t seconds = (time_t)(timestamp_ms / 1000);
    int32_t ms = (int32_t)(timestamp_ms % 1000);
    struct tm tm_snapshot;
    gmtime_r(&seconds, &tm_snapshot);
    snprintf(out,
             len,
             "%04d-%02d-%02dT%02d:%02d:%02d.%03ldZ",
             tm_snapshot.tm_year + 1900,
             tm_snapshot.tm_mon + 1,
             tm_snapshot.tm_mday,
             tm_snapshot.tm_hour,
             tm_snapshot.tm_min,
             tm_snapshot.tm_sec,
             (long)ms);
}

static const char *float_or_null(char *buf, size_t len, float value)
{
    if (isnan(value))
    {
        return "null";
    }
    snprintf(buf, len, "%.2f", value);
    return buf;
}

static measurement_report_t collect_measurements(void)
{
    measurement_report_t report = {};
    report.timestamp_ms = esp_timer_get_time() / 1000;

    int raw = adc1_get_raw(SOIL_SENSOR_ADC_CHANNEL);
    report.soil.raw = raw;
    report.soil.percent = compute_soil_percent(raw);

#if CONFIG_SOIL_SENSOR_BME280_ENABLED
    bme280_reading_t env = {0.0f, 0.0f, 0.0f};
    if (bme280_support_read(&env))
    {
        report.air_temperature_c = env.temperature_c;
        report.air_humidity_percent = env.humidity_percent;
    }
    else
    {
        report.air_temperature_c = NAN;
        report.air_humidity_percent = NAN;
    }
#else
    report.air_temperature_c = NAN;
    report.air_humidity_percent = NAN;
#endif

    return report;
}

static const char *derive_mood(const measurement_report_t *report)
{
    float soil_percent = report->soil.percent;
    if (!isnan(soil_percent))
    {
        if (soil_percent < 5.0f)
        {
            return "DRY";
        }
        if (soil_percent < 25.0f)
        {
            return "THIRSTY";
        }
        if (soil_percent > 70.0f)
        {
            return "HAPPY";
        }
    }

    float temp = report->air_temperature_c;
    if (!isnan(temp))
    {
        if (temp < 15.0f)
        {
            return "COLD";
        }
        if (temp > 30.0f)
        {
            return "HOT";
        }
    }

    return "NORMAL";
}

static void publish_measurement(const measurement_report_t *report)
{
    if (!mqtt_client || !mqtt_connected)
    {
        return;
    }

    char timestamp_iso[32];
    format_iso8601(report->timestamp_ms, timestamp_iso, sizeof(timestamp_iso));

    char temp_buf[16];
    char hum_buf[16];
    char soil_percent_buf[16];
    const char *temp_str = float_or_null(temp_buf, sizeof(temp_buf), report->air_temperature_c);
    const char *hum_str = float_or_null(hum_buf, sizeof(hum_buf), report->air_humidity_percent);
    const char *soil_percent_str = float_or_null(soil_percent_buf, sizeof(soil_percent_buf), report->soil.percent);

    const char *mood = derive_mood(report);

    char payload[384];
    int written = snprintf(payload,
                          sizeof(payload),
                          "{\"deviceUid\":\"%s\"," \
                          "\"measurements\":{\"airTemperatureC\":%s,\"airHumidityPercent\":%s,"
                          "\"soilMoisturePercent\":%s,\"soilMoistureRaw\":%d,\"timestamp\":\"%s\"},"
                          "\"mood\":\"%s\",\"friendVisible\":true}",
                          device_uid,
                          temp_str,
                          hum_str,
                          soil_percent_str,
                          report->soil.raw,
                          timestamp_iso,
                          mood);
    if (written <= 0 || written >= (int)sizeof(payload))
    {
        ESP_LOGW(TAG, "Payload truncated, skipping publish");
        return;
    }

    int msg_id = esp_mqtt_client_publish(
        mqtt_client,
        CONFIG_SOIL_SENSOR_MQTT_DATA_TOPIC,
        payload,
        0,
        1,
        0);
    if (msg_id < 0)
    {
        ESP_LOGW(TAG, "Не удалось опубликовать данные датчика");
    }
}

static void soil_sensor_task(void *param)
{
    (void)param;

    while (true)
    {
        measurement_report_t report = collect_measurements();
        ESP_LOGI(TAG,
                 "Soil raw=%d (%.1f%%) Temp=%.2fC Hum=%.2f%%",
                 report.soil.raw,
                 report.soil.percent,
                 report.air_temperature_c,
                 report.air_humidity_percent);

        if (mqtt_connected &&
            (report.timestamp_ms - last_publish_timestamp_ms) >= CONFIG_SOIL_SENSOR_PUBLISH_INTERVAL_MS)
        {
            publish_measurement(&report);
            last_publish_timestamp_ms = report.timestamp_ms;
        }

        vTaskDelay(kSampleDelayTicks);
    }
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)arg;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        wifi_event_sta_disconnected_t *disconnect = static_cast<wifi_event_sta_disconnected_t *>(event_data);
        int reason = disconnect ? disconnect->reason : -1;
        ESP_LOGW(TAG, "Wi-Fi отключён (reason=%d), повторное подключение...", reason);
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        ESP_LOGI(TAG, "Wi-Fi подключён");
    }
}

static bool wifi_start(void)
{
    if (strlen(CONFIG_SOIL_SENSOR_WIFI_SSID) == 0)
    {
        ESP_LOGW(TAG, "SSID не задан, Wi-Fi не будет запущен");
        return false;
    }

    wifi_event_group = xEventGroupCreate();
    if (!wifi_event_group)
    {
        ESP_LOGE(TAG, "Не удалось создать EventGroup для Wi-Fi");
        return false;
    }

    esp_netif_t *netif = esp_netif_create_default_wifi_sta();
    if (!netif)
    {
        ESP_LOGE(TAG, "Не удалось создать сетевой интерфейс Wi-Fi");
        return false;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    static esp_event_handler_instance_t wifi_event_instance_any_id;
    static esp_event_handler_instance_t ip_event_instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        nullptr,
                                                        &wifi_event_instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        nullptr,
                                                        &ip_event_instance_got_ip));

    wifi_config_t wifi_config = {};
    snprintf((char *)wifi_config.sta.ssid, sizeof(wifi_config.sta.ssid), "%s", CONFIG_SOIL_SENSOR_WIFI_SSID);
    snprintf((char *)wifi_config.sta.password, sizeof(wifi_config.sta.password), "%s", CONFIG_SOIL_SENSOR_WIFI_PASSWORD);

    if (strlen(CONFIG_SOIL_SENSOR_WIFI_PASSWORD) == 0)
    {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
    }
    else
    {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    }

    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                           WIFI_CONNECTED_BIT,
                                           pdFALSE,
                                           pdTRUE,
                                           kWifiConnectTimeoutTicks);
    if ((bits & WIFI_CONNECTED_BIT) == 0)
    {
        ESP_LOGE(TAG, "Не удалось подключиться к Wi-Fi (таймаут)");
        return false;
    }

    return true;
}

static void mqtt_handle_command(const esp_mqtt_event_handle_t event)
{
    if (!event->topic || !event->data)
    {
        return;
    }

    size_t topic_len = strlen(CONFIG_SOIL_SENSOR_MQTT_CMD_TOPIC);
    if (event->topic_len != (int)topic_len ||
        strncmp(event->topic, CONFIG_SOIL_SENSOR_MQTT_CMD_TOPIC, topic_len) != 0)
    {
        return;
    }

    if ((int)strlen(kCmdGetInfo) == event->data_len &&
        strncmp(event->data, kCmdGetInfo, event->data_len) == 0)
    {
        ESP_LOGI(TAG, "Получена команда get_info через MQTT");
        measurement_report_t report = collect_measurements();
        publish_measurement(&report);
        last_publish_timestamp_ms = report.timestamp_ms;
    }

#if CONFIG_SOIL_SENSOR_PUMP_ENABLED
    if (event->data_len <= 0)
    {
        return;
    }

    char cmd_buf[160];
    if (event->data_len >= (int)sizeof(cmd_buf))
    {
        ESP_LOGW(TAG, "Слишком длинная команда MQTT, пропускаем (len=%d)", event->data_len);
        return;
    }

    memcpy(cmd_buf, event->data, event->data_len);
    cmd_buf[event->data_len] = '\0';

    if (strncmp(cmd_buf, kCmdWaterStop, strlen(kCmdWaterStop)) == 0 &&
        cmd_buf[strlen(kCmdWaterStop)] == ';')
    {
        const char *uid = cmd_buf + strlen(kCmdWaterStop) + 1;
        if (strcmp(uid, device_uid) == 0)
        {
            pump_stop();
        }
        return;
    }

    if (strncmp(cmd_buf, kCmdWater, strlen(kCmdWater)) == 0 &&
        cmd_buf[strlen(kCmdWater)] == ';')
    {
        const char *uid_start = cmd_buf + strlen(kCmdWater) + 1;
        const char *sep = strchr(uid_start, ';');
        if (!sep)
        {
            return;
        }

        size_t uid_len = (size_t)(sep - uid_start);
        if (uid_len == 0 || uid_len >= sizeof(device_uid))
        {
            return;
        }

        char uid[sizeof(device_uid)];
        memcpy(uid, uid_start, uid_len);
        uid[uid_len] = '\0';
        if (strcmp(uid, device_uid) != 0)
        {
            return;
        }

        const char *duration_str = sep + 1;
        char *endptr = nullptr;
        unsigned long long duration_ms = strtoull(duration_str, &endptr, 10);
        if (endptr == duration_str || (endptr && *endptr != '\0'))
        {
            return;
        }
        if (duration_ms > UINT32_MAX)
        {
            duration_ms = UINT32_MAX;
        }

        pump_start((uint32_t)duration_ms);
        return;
    }
#endif
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    (void)handler_args;
    (void)base;

    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch (event_id)
    {
    case MQTT_EVENT_CONNECTED:
        mqtt_connected = true;
        ESP_LOGI(TAG, "MQTT подключен");
        esp_mqtt_client_subscribe(event->client, CONFIG_SOIL_SENSOR_MQTT_CMD_TOPIC, 1);
        allow_immediate_publish();
        break;
    case MQTT_EVENT_DISCONNECTED:
        mqtt_connected = false;
        ESP_LOGW(TAG, "MQTT отключен");
        break;
    case MQTT_EVENT_DATA:
        mqtt_handle_command(event);
        break;
    default:
        break;
    }
}

static const char *sanitize_mqtt_uri(void)
{
    static char sanitized[128];
    const char *uri = CONFIG_SOIL_SENSOR_MQTT_BROKER_URI;
    const char *needle = strstr(uri, "://:");
    if (needle)
    {
        size_t prefix_len = needle - uri + 3;  // include "://"
        if (prefix_len >= sizeof(sanitized))
        {
            return uri;
        }
        memcpy(sanitized, uri, prefix_len);
        sanitized[prefix_len] = '\0';
        strncat(sanitized, needle + 4, sizeof(sanitized) - prefix_len - 1);
        return sanitized;
    }
    return uri;
}

static void mqtt_start(void)
{
    if (strlen(CONFIG_SOIL_SENSOR_MQTT_BROKER_URI) == 0)
    {
        ESP_LOGW(TAG, "URI брокера MQTT не задан, MQTT отключен");
        return;
    }

    const char *uri = sanitize_mqtt_uri();
    ESP_LOGI(TAG, "MQTT URI = '%s'", uri);

    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = uri;

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (!mqtt_client)
    {
        ESP_LOGE(TAG, "Не удалось инициализировать MQTT клиент");
        return;
    }

    esp_mqtt_client_register_event(mqtt_client,
                                   MQTT_EVENT_ANY,
                                   mqtt_event_handler,
                                   nullptr);
    esp_err_t err = esp_mqtt_client_start(mqtt_client);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Не удалось запустить MQTT клиент (%s)", esp_err_to_name(err));
    }
}

extern "C" void app_main(void)
{
#if CONFIG_SOIL_SENSOR_PUMP_ENABLED
    pump_early_safe_state();
#endif
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    init_device_uid();
    init_adc();
#if CONFIG_SOIL_SENSOR_PUMP_ENABLED
    pump_init();
#endif
#if CONFIG_SOIL_SENSOR_BME280_ENABLED
    ESP_ERROR_CHECK_WITHOUT_ABORT(bme280_support_init(BME280_I2C_PORT,
                                                      CONFIG_SOIL_SENSOR_BME280_I2C_ADDRESS,
                                                      (gpio_num_t)CONFIG_SOIL_SENSOR_BME280_SDA,
                                                      (gpio_num_t)CONFIG_SOIL_SENSOR_BME280_SCL,
                                                      BME280_I2C_FREQ_HZ));
#endif

    xTaskCreate(soil_sensor_task, "soil_sensor", 4096, nullptr, tskIDLE_PRIORITY + 1, nullptr);

    bool wifi_ok = wifi_start();
    if (wifi_ok)
    {
        allow_immediate_publish();
        mqtt_start();
    }
    else
    {
        ESP_LOGW(TAG, "MQTT не будет запущен без Wi-Fi");
    }
}
