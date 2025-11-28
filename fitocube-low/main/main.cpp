#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "driver/i2c.h"
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
            return "dry";
        }
        if (soil_percent < 25.0f)
        {
            return "thirsty";
        }
        if (soil_percent > 70.0f)
        {
            return "happy";
        }
    }

    float temp = report->air_temperature_c;
    if (!isnan(temp))
    {
        if (temp < 15.0f)
        {
            return "cold";
        }
        if (temp > 30.0f)
        {
            return "hot";
        }
    }

    return "normal";
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
        ESP_LOGW(TAG, "Wi-Fi отключён, повторное подключение...");
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

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        nullptr,
                                                        nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        nullptr,
                                                        nullptr));

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
        mqtt_start();
        last_publish_timestamp_ms = esp_timer_get_time() / 1000;
    }
    else
    {
        ESP_LOGW(TAG, "MQTT не будет запущен без Wi-Fi");
    }
}
