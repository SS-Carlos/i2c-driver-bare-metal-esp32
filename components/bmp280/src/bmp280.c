/*
 * BMP280 Driver for ESP32 — Driver implementation
 * Copyright (c) 2026 Carlos Solano
 *
 * SPDX-License-Identifier: MIT
 *
 * Datasheet: BST-BMP280-DS001.pdf
 */

#include "bmp280.h"
#include "bmp280_comp.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"

static const char *TAG = "BMP280";

// Private module instances
static i2c_master_dev_handle_t dev_handle;
static bmp280_calib_t          calib;

// Private functions

/* ── bmp280_read_register ────────────────────────────────────────────────
 *
 * Writes the register address then reads len bytes from the sensor.
 * Uses i2c_master_transmit_receive() — single transaction on the bus.
 *
 * @param reg   Register address to read from
 * @param data  Buffer to store the received bytes
 * @param len   Number of bytes to read
 * @return      ESP_OK on success
 */

static esp_err_t bmp280_read_register(uint8_t reg, 
                                      uint8_t *data, 
                                      uint8_t len) 
{
  return i2c_master_transmit_receive(dev_handle, 
                                     &reg, 1, 
                                     data, len, 
                                     pdMS_TO_TICKS(100));
}

/* ── bmp280_write_register ───────────────────────────────────────────────
 *
 * Writes a single byte value to the given register.
 *
 * @param reg    Register address to write to
 * @param value  Byte value to write
 * @return       ESP_OK on success
 */

static esp_err_t bmp280_write_register(uint8_t reg, uint8_t value) 
{
  uint8_t data[2] = {reg, value};
  return i2c_master_transmit(dev_handle, data, sizeof(data), pdMS_TO_TICKS(100));
}

/* ── bmp280_read_calibration ─────────────────────────────────────────────
 *
 * Reads 24 calibration bytes from registers 0x88-0x9F.
 * All coefficients are stored in little-endian format (LSB first).
 * dig_T1 and dig_P1 are unsigned — all others are signed.
 *
 * Datasheet table 17 — Compensation parameter storage registers.
 */

static esp_err_t bmp280_read_calibration(void)
{
  uint8_t buff[24];
  
  ESP_RETURN_ON_ERROR(
    bmp280_read_register(BMP280_REG_CALIB_START, buff, 24),
    TAG, "Failed to read calibration registers"
  );

  calib.dig_T1 = (uint16_t)((buff[1]<<8) | buff[0]);
  calib.dig_T2 = (int16_t)((buff[3]<<8) | buff[2]);
  calib.dig_T3 = (int16_t)((buff[5]<<8) | buff[4]);
    
  calib.dig_P1 = (uint16_t)((buff[7]<<8) | buff[6]);
  calib.dig_P2 = (int16_t)((buff[9]<<8) | buff[8]);
  calib.dig_P3 = (int16_t)((buff[11]<<8) | buff[10]);
  calib.dig_P4 = (int16_t)((buff[13]<<8) | buff[12]);
  calib.dig_P5 = (int16_t)((buff[15]<<8) | buff[14]);
  calib.dig_P6 = (int16_t)((buff[17]<<8) | buff[16]);
  calib.dig_P7 = (int16_t)((buff[19]<<8) | buff[18]);
  calib.dig_P8 = (int16_t)((buff[21]<<8) | buff[20]);
  calib.dig_P9 = (int16_t)((buff[23]<<8) | buff[22]);

  ESP_LOGI(TAG, "calibration coefficients loadded succesfully");

  return ESP_OK;
}

/*
 * ----- PUBLIC API -----
 */

esp_err_t bmp280_check_id(void) 
{
  uint8_t chip_id;

  ESP_RETURN_ON_ERROR(
    bmp280_read_register(BMP280_REG_CHIP_ID, &chip_id, 1),
    TAG, "Failed to read chip ID"
  );

  if (chip_id != BMP280_CHIP_ID) {
    ESP_LOGE(TAG, "Wrong chip ID: got 0x%02X expected 0x%02X", 
             chip_id, BMP280_REG_CHIP_ID);
    return ESP_ERR_NOT_FOUND;
  }

  ESP_LOGI(TAG, "BMP280 detected - chip ID: 0x%02X", chip_id);
  return ESP_OK;
}

esp_err_t bmp280_reset(void)
{
  ESP_RETURN_ON_ERROR(
    bmp280_write_register(BMP280_REG_RESET, BMP280_RESET_VALUE),
    TAG, "failed to read sensor"
  );

  // wait for sensor complete reset
  vTaskDelay(pdMS_TO_TICKS(10));

  ESP_LOGI(TAG, "BMP280 reset complete");
  return ESP_OK;
}

esp_err_t bmp280_init(i2c_master_bus_handle_t bus_handle, const bmp280_config_t *config)
{
  // Register device in the i2c bus
  i2c_device_config_t dev_cfg =
  {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = config->i2c_addr,
      .scl_speed_hz = 100000,
  };

  ESP_RETURN_ON_ERROR(
    i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle),
    TAG, "Failed to add device to I2C bus"
  );

  // Verify chip ID
  ESP_RETURN_ON_ERROR(
    bmp280_check_id(),
    TAG, "Check ID failed"
  );

  // Load calibration coefficients
  ESP_RETURN_ON_ERROR(
    bmp280_read_calibration(),
    TAG, "Failed to read calibration"
  );

  // Confure oversampling and operation mode
  // ctrl_meas register (0xF4) layout:
  // bits 7-5: osrs_t (temperature oversampling)
  // bits 4-2: osrs_p (pressure oversampling)
  // bits 1-0: mode
  uint8_t ctrl_meas = (uint8_t)(
    (config->osrs_t << 5) | 
    (config->osrs_p << 2) | 
    (config->mode)
  );

  ESP_RETURN_ON_ERROR(
    bmp280_write_register(BMP280_REG_CTRL_MEAS, ctrl_meas), 
    TAG, "Failed to configure sensor"
  );

  ESP_LOGI(TAG, "BMP280 initialized - addr=0x%02X mode=0x%02X",
           config->i2c_addr, ctrl_meas
  );

  return ESP_OK;
}

esp_err_t bmp280_read(bmp280_data_t *data)
{
  /* Read 6 bytes starting at 0xF7:
   * buf[0-2] → raw pressure    (0xF7, 0xF8, 0xF9)
   * buf[3-5] → raw temperature (0xFA, 0xFB, 0xFC)
   */

  uint8_t raw[6];

  ESP_RETURN_ON_ERROR(
    bmp280_read_register(BMP280_REG_PRESS_MSB, raw, 6),
    TAG, "Failed to read sensor data"
  );


  int32_t adc_P = ((int32_t)raw[0] << 12) | 
                  ((int32_t)raw[1] << 4) | 
                  ((int32_t)raw[2] >> 4);
  int32_t adc_T = ((int32_t)raw[3] << 12) | 
                  ((int32_t)raw[4] << 4) | 
                  ((int32_t)raw[5] >> 4);

  // temperature must be compensated first
  BMP280_S32_T temp  = bmp280_compensate_temp(adc_T, &calib);
  BMP280_S32_T press = bmp280_compensate_press(adc_P, &calib);

  /* Convert to human-readable units:
   * temperature: raw × 100 → divide by 100 for °C
   * pressure:    Q24.8 format → divide by 256 for Pa → by 100 for hPa
   */

  data->temperature = (float)temp / 100.0f;
  data->pressure    = (float)(press / 256) / 100.0f;

  return ESP_OK;
}
