#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_spiffs.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "mqtt_client.h"
#include "nvs_flash.h"

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

#ifndef CONFIG_SOIL_SENSOR_SAMPLE_PERIOD_MS
#define CONFIG_SOIL_SENSOR_SAMPLE_PERIOD_MS 1000
#endif

#ifndef CONFIG_SOIL_SENSOR_VREF_MV
#define CONFIG_SOIL_SENSOR_VREF_MV 1100
#endif

#ifndef CONFIG_SOIL_SENSOR_LOG_FILE_PATH
#define CONFIG_SOIL_SENSOR_LOG_FILE_PATH "/spiffs/soil_log.csv"
#endif

#ifndef CONFIG_SOIL_SENSOR_WIFI_SSID
#define CONFIG_SOIL_SENSOR_WIFI_SSID ""
#endif

#ifndef CONFIG_SOIL_SENSOR_WIFI_PASSWORD
#define CONFIG_SOIL_SENSOR_WIFI_PASSWORD ""
#endif

#ifndef CONFIG_SOIL_SENSOR_MQTT_BROKER_URI
#define CONFIG_SOIL_SENSOR_MQTT_BROKER_URI ""
#endif

#ifndef CONFIG_SOIL_SENSOR_MQTT_CMD_TOPIC
#define CONFIG_SOIL_SENSOR_MQTT_CMD_TOPIC "soil/cmd"
#endif

#ifndef CONFIG_SOIL_SENSOR_MQTT_DATA_TOPIC
#define CONFIG_SOIL_SENSOR_MQTT_DATA_TOPIC "soil/data"
#endif

static const TickType_t kSampleDelayTicks = pdMS_TO_TICKS(CONFIG_SOIL_SENSOR_SAMPLE_PERIOD_MS);
static const char *kCmdGetInfo = "get_info";
static EventGroupHandle_t wifi_event_group;
static const int WIFI_CONNECTED_BIT = BIT0;

static esp_adc_cal_characteristics_t adc_chars;
static FILE *log_file = nullptr;
static esp_mqtt_client_handle_t mqtt_client = nullptr;
static bool mqtt_connected = false;

typedef struct
{
    int raw;
    uint32_t voltage_mv;
    int64_t timestamp_ms;
} soil_measurement_t;

static void init_spiffs(void)
{
    const esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "storage",
        .max_files = 2,
        .format_if_mount_failed = true,
    };

    esp_err_t err = esp_vfs_spiffs_register(&conf);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "SPIFFS недоступен (%s)", esp_err_to_name(err));
        return;
    }

    log_file = fopen(CONFIG_SOIL_SENSOR_LOG_FILE_PATH, "a+");
    if (!log_file)
    {
        ESP_LOGW(TAG,
                 "Не удалось открыть %s: errno=%d",
                 CONFIG_SOIL_SENSOR_LOG_FILE_PATH,
                 errno);
        return;
    }

    if (fseek(log_file, 0, SEEK_END) == 0)
    {
        long size = ftell(log_file);
        if (size == 0)
        {
            fprintf(log_file, "timestamp_ms,raw,voltage_mv\n");
            fflush(log_file);
        }
    }
}

static void init_adc()
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

static soil_measurement_t capture_measurement(void)
{
    soil_measurement_t measurement = {0};
    measurement.raw = adc1_get_raw(SOIL_SENSOR_ADC_CHANNEL);
    measurement.voltage_mv = esp_adc_cal_raw_to_voltage(measurement.raw, &adc_chars);
    measurement.timestamp_ms = esp_timer_get_time() / 1000;

    ESP_LOGI(TAG,
             "GPIO%d raw=%d voltage=%dmV",
             SOIL_SENSOR_GPIO,
             measurement.raw,
             measurement.voltage_mv);

    return measurement;
}

static void log_measurement(const soil_measurement_t *measurement)
{
    if (!log_file)
    {
        return;
    }

    fprintf(log_file,
            "%lld,%d,%lu\n",
            (long long)measurement->timestamp_ms,
            measurement->raw,
            (unsigned long)measurement->voltage_mv);
    fflush(log_file);
}

static void publish_measurement(const soil_measurement_t *measurement)
{
    if (!mqtt_client || !mqtt_connected)
    {
        return;
    }

    char payload[160];
    snprintf(payload,
             sizeof(payload),
             "{\"timestamp_ms\":%lld,\"raw\":%d,\"voltage_mv\":%u}",
             (long long)measurement->timestamp_ms,
             measurement->raw,
             (unsigned)measurement->voltage_mv);

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

static void handle_get_info(void)
{
    soil_measurement_t measurement = capture_measurement();
    log_measurement(&measurement);
    publish_measurement(&measurement);
}

static void soil_sensor_task(void *param)
{
    (void)param;

    while (true)
    {
        soil_measurement_t measurement = capture_measurement();
        log_measurement(&measurement);
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

    EventBits_t bits = xEventGroupWaitBits(
        wifi_event_group,
        WIFI_CONNECTED_BIT,
        pdFALSE,
        pdFALSE,
        pdMS_TO_TICKS(15000));

    if ((bits & WIFI_CONNECTED_BIT) == 0)
    {
        ESP_LOGE(TAG, "Не удалось подключиться к Wi-Fi");
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
        handle_get_info();
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

static void mqtt_start(void)
{
    if (strlen(CONFIG_SOIL_SENSOR_MQTT_BROKER_URI) == 0)
    {
        ESP_LOGW(TAG, "URI брокера MQTT не задан, MQTT отключен");
        return;
    }

    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = CONFIG_SOIL_SENSOR_MQTT_BROKER_URI;

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

    init_spiffs();
    init_adc();
    xTaskCreate(soil_sensor_task, "soil_sensor", 4096, nullptr, tskIDLE_PRIORITY + 1, nullptr);

    bool wifi_ok = wifi_start();
    if (wifi_ok)
    {
        mqtt_start();
    }
    else
    {
        ESP_LOGW(TAG, "MQTT не будет запущен без Wi-Fi");
    }
}
