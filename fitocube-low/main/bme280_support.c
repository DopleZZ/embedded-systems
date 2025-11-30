#include "bme280_support.h"

#include "esp_check.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BME280_REG_CHIP_ID      0xD0
#define BME280_REG_RESET        0xE0
#define BME280_REG_CTRL_HUM     0xF2
#define BME280_REG_STATUS       0xF3
#define BME280_REG_CTRL_MEAS    0xF4
#define BME280_REG_CONFIG       0xF5
#define BME280_REG_PRESS_MSB    0xF7

#define BME280_RESET_VALUE      0xB6

static const char *TAG_BME = "bme280";

typedef struct
{
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;
    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;
    uint8_t dig_H1;
    int16_t dig_H2;
    uint8_t dig_H3;
    int16_t dig_H4;
    int16_t dig_H5;
    int8_t dig_H6;
} bme280_calib_data_t;

static i2c_port_t s_port;
static uint8_t s_address;
static bme280_calib_data_t s_calib;
static bool s_ready = false;
static int32_t s_t_fine = 0;

static esp_err_t bme_i2c_reg_write(uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = {reg, value};
    return i2c_master_write_to_device(s_port, s_address, buf, sizeof(buf), pdMS_TO_TICKS(100));
}

static esp_err_t bme_i2c_reg_read(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(s_port, s_address, &reg, 1, data, len, pdMS_TO_TICKS(100));
}

static esp_err_t load_calibration(void)
{
    uint8_t calib1[26] = {0};
    ESP_RETURN_ON_ERROR(bme_i2c_reg_read(0x88, calib1, sizeof(calib1)), TAG_BME, "Failed read calib block 1");

    s_calib.dig_T1 = (uint16_t)((calib1[1] << 8) | calib1[0]);
    s_calib.dig_T2 = (int16_t)((calib1[3] << 8) | calib1[2]);
    s_calib.dig_T3 = (int16_t)((calib1[5] << 8) | calib1[4]);
    s_calib.dig_P1 = (uint16_t)((calib1[7] << 8) | calib1[6]);
    s_calib.dig_P2 = (int16_t)((calib1[9] << 8) | calib1[8]);
    s_calib.dig_P3 = (int16_t)((calib1[11] << 8) | calib1[10]);
    s_calib.dig_P4 = (int16_t)((calib1[13] << 8) | calib1[12]);
    s_calib.dig_P5 = (int16_t)((calib1[15] << 8) | calib1[14]);
    s_calib.dig_P6 = (int16_t)((calib1[17] << 8) | calib1[16]);
    s_calib.dig_P7 = (int16_t)((calib1[19] << 8) | calib1[18]);
    s_calib.dig_P8 = (int16_t)((calib1[21] << 8) | calib1[20]);
    s_calib.dig_P9 = (int16_t)((calib1[23] << 8) | calib1[22]);

    uint8_t hum1 = 0;
    ESP_RETURN_ON_ERROR(bme_i2c_reg_read(0xA1, &hum1, 1), TAG_BME, "Failed read hum calib 1");
    s_calib.dig_H1 = hum1;

    uint8_t calib2[7] = {0};
    ESP_RETURN_ON_ERROR(bme_i2c_reg_read(0xE1, calib2, sizeof(calib2)), TAG_BME, "Failed read hum calib 2");
    s_calib.dig_H2 = (int16_t)((calib2[1] << 8) | calib2[0]);
    s_calib.dig_H3 = calib2[2];
    s_calib.dig_H4 = (int16_t)((calib2[3] << 4) | (calib2[4] & 0x0F));
    s_calib.dig_H5 = (int16_t)((calib2[5] << 4) | (calib2[4] >> 4));
    s_calib.dig_H6 = (int8_t)calib2[6];

    return ESP_OK;
}

static float compensate_temp(int32_t adc_T)
{
    double var1 = ((double)adc_T / 16384.0 - ((double)s_calib.dig_T1) / 1024.0) * s_calib.dig_T2;
    double var2 = ((((double)adc_T / 131072.0 - ((double)s_calib.dig_T1) / 8192.0) *
                    ((double)adc_T / 131072.0 - ((double)s_calib.dig_T1) / 8192.0)) * s_calib.dig_T3);
    s_t_fine = (int32_t)(var1 + var2);
    return (float)((var1 + var2) / 5120.0);
}

static float compensate_hum(int32_t adc_H)
{
    double var_h = s_t_fine - 76800.0;
    var_h = (adc_H - (s_calib.dig_H4 * 64.0 + s_calib.dig_H5 / 16384.0 * var_h)) *
            (s_calib.dig_H2 / 65536.0 * (1.0 + s_calib.dig_H6 / 67108864.0 * var_h *
                                         (1.0 + s_calib.dig_H3 / 67108864.0 * var_h)));
    var_h = var_h * (1.0 - s_calib.dig_H1 * var_h / 524288.0);
    if (var_h > 100.0)
    {
        var_h = 100.0;
    }
    else if (var_h < 0.0)
    {
        var_h = 0.0;
    }
    return (float)var_h;
}

static float compensate_press(int32_t adc_P)
{
    double var1 = (double)s_t_fine / 2.0 - 64000.0;
    double var2 = var1 * var1 * s_calib.dig_P6 / 32768.0;
    var2 = var2 + var1 * s_calib.dig_P5 * 2.0;
    var2 = var2 / 4.0 + ((double)s_calib.dig_P4) * 65536.0;
    var1 = (s_calib.dig_P3 * var1 * var1 / 524288.0 + s_calib.dig_P2 * var1) / 524288.0;
    var1 = (1.0 + var1 / 32768.0) * s_calib.dig_P1;
    if (var1 == 0.0)
    {
        return 0.0f;
    }
    double pressure = 1048576.0 - (double)adc_P;
    pressure = (pressure - var2 / 4096.0) * 6250.0 / var1;
    var1 = s_calib.dig_P9 * pressure * pressure / 2147483648.0;
    var2 = pressure * s_calib.dig_P8 / 32768.0;
    pressure = pressure + (var1 + var2 + s_calib.dig_P7) / 16.0;
    return (float)pressure;
}

esp_err_t bme280_support_init(i2c_port_t port,
                              uint8_t address,
                              gpio_num_t sda,
                              gpio_num_t scl,
                              uint32_t frequency_hz)
{
    s_port = port;
    s_address = address;

    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = scl,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = frequency_hz,
    };

    ESP_RETURN_ON_ERROR(i2c_param_config(port, &i2c_conf), TAG_BME, "i2c_param_config failed");
    esp_err_t err = i2c_driver_install(port, i2c_conf.mode, 0, 0, 0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        return err;
    }

    uint8_t chip_id = 0;
    ESP_RETURN_ON_ERROR(bme_i2c_reg_read(BME280_REG_CHIP_ID, &chip_id, 1), TAG_BME, "chip id read failed");
    if (chip_id != 0x60)
    {
        ESP_LOGW(TAG_BME, "Unexpected chip id 0x%02X", chip_id);
        return ESP_FAIL;
    }

    ESP_RETURN_ON_ERROR(load_calibration(), TAG_BME, "calibration load failed");

    ESP_RETURN_ON_ERROR(bme_i2c_reg_write(BME280_REG_RESET, BME280_RESET_VALUE), TAG_BME, "reset failed");
    esp_rom_delay_us(2000);

    ESP_RETURN_ON_ERROR(bme_i2c_reg_write(BME280_REG_CTRL_HUM, 0x01), TAG_BME, "ctrl hum failed");
    ESP_RETURN_ON_ERROR(bme_i2c_reg_write(BME280_REG_CONFIG, 0xA0), TAG_BME, "config failed");
    ESP_RETURN_ON_ERROR(bme_i2c_reg_write(BME280_REG_CTRL_MEAS, 0x27), TAG_BME, "ctrl meas failed");

    s_ready = true;
    ESP_LOGI(TAG_BME, "BME280 initialized (addr 0x%02X)", s_address);
    return ESP_OK;
}

bool bme280_support_read(bme280_reading_t *out)
{
    if (!s_ready || out == NULL)
    {
        return false;
    }

    uint8_t buffer[8] = {0};
    if (bme_i2c_reg_read(BME280_REG_PRESS_MSB, buffer, sizeof(buffer)) != ESP_OK)
    {
        ESP_LOGW(TAG_BME, "Failed to read measurement block");
        return false;
    }

    int32_t adc_press = ((int32_t)buffer[0] << 12) | ((int32_t)buffer[1] << 4) | (buffer[2] >> 4);
    int32_t adc_temp = ((int32_t)buffer[3] << 12) | ((int32_t)buffer[4] << 4) | (buffer[5] >> 4);
    int32_t adc_hum = ((int32_t)buffer[6] << 8) | buffer[7];

    out->temperature_c = compensate_temp(adc_temp);
    out->pressure_pa = compensate_press(adc_press);
    out->humidity_percent = compensate_hum(adc_hum);
    return true;
}
