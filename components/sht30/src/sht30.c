/* SHT30 Driver for ESP32
 * Copyright (c) 2026 Carlos Solano
 *
 * SPDX-License-Identifier: MIT
 *
 * Datasheet: SHT3x_DIS.pdf
 */

#include "sht30.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"

static const char *TAG = "SHT30";

// Private module instances
static i2c_master_dev_handle_t dev_handle;
static sht30_config_t          dev_config;

/* ═══════════════════════════════════════════════════════════════════════
 * PRIVATE FUNCTIONS
 * ═══════════════════════════════════════════════════════════════════════ */

/* sht30_send_command send a 16-bit command to the sensor
 * commands are always 2 bytes, MSB first (Big endian)
 * Example: 0x2C06 -> sends 0x2C then 0x06
 */
static esp_err_t sht30_send_command(uint16_t cmd)
{
  uint8_t buf[2] = {
    (cmd >> 8) & 0xFF, // MSB first
    (cmd)      & 0xFF  // LSB then
  };
  return i2c_master_transmit(dev_handle, buf, 2, pdMS_TO_TICKS(100));
}

/* sht30_read_measurement read raw data from sensor
 * Always reads 6 bytes: temp(2) -> crc(1) -> hum(2) -> crc(1)
 */
static esp_err_t sht30_read_measurement(uint8_t *buf)
{
  return i2c_master_receive(dev_handle, buf, 6, pdMS_TO_TICKS(100));
}

// crc-8 algorithm for sht30
static uint8_t sht30_crc(const uint8_t *data, size_t len)
{
  uint8_t crc = 0xFF; // initial value
  
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; bit++) { 
      if (crc & 0x80) {
        crc = (crc << 1) ^ 0x31;  // CRC-8 polynomial — datasheet section 4.12
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}

/* sht30_convert — applies datasheet conversion formulas (section 4.13)
 * Verifies CRC-8 for both temperature and humidity before converting.
 * Temperature: T = -45 + 175 * (raw / 65535)  → °C
 * Humidity:    RH = 100 * (raw / 65535)        → %RH
 *
 * @param buf   6-byte raw data buffer from sht30_read_measurement
 * @param data  pointer to struct where converted values are stored
 * @return      ESP_OK on success
 *              ESP_ERR_INVALID_CRC if CRC verification fails
 */
static esp_err_t sht30_convert(const uint8_t *buf, sht30_data_t *data)
{
  uint16_t raw_temp = ((uint16_t)buf[0] << 8) | buf[1];
  uint16_t raw_hum  = ((uint16_t)buf[3] << 8) | buf[4];

  // Verify temperature CRC — datasheet section 4.12
  uint8_t temp_crc = sht30_crc(&buf[0], 2);
  if (temp_crc != buf[2]) {
    // CRC mismatch — temperature data corrupted during I2C transfer
    return ESP_ERR_INVALID_CRC;
  }

  // Verify humidity CRC — datasheet section 4.12
  uint8_t hum_crc  = sht30_crc(&buf[3], 2);
  if (hum_crc != buf[5]) {
    // CRC mismatch — humidity data corrupted during I2C transfer
    return ESP_ERR_INVALID_CRC;
  }

  data->temperature = -45.0f + 175.0f * ((float)raw_temp / 65535.f);
  data->humidity    = 100.0f * ((float)raw_hum / 65535.0f);

  return ESP_OK;
}

/* ═══════════════════════════════════════════════════════════════════════
 * PUBLIC API
 * ═══════════════════════════════════════════════════════════════════════ */

esp_err_t sht30_init(i2c_master_bus_handle_t bus_handle,
                        const sht30_config_t *config)
{
  dev_config = *config;

  /* 1. Register device on the I2C bus */
  i2c_device_config_t dev_cfg = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address  = config->i2c_addr,
      .scl_speed_hz    = 400000,  // SHT30 Fast Mode (400kHz)
  };

  ESP_RETURN_ON_ERROR(
      i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle),
      TAG, "Failed to add device to I2C bus"
  );

  /* 2. Reset chip to clear commands */
  ESP_RETURN_ON_ERROR(
      sht30_reset(),
      TAG, "Failed to reset sensor"
  );

  vTaskDelay(pdMS_TO_TICKS(100));

  /* 3. Verify chip status */
  ESP_RETURN_ON_ERROR(
      sht30_check_status(),
      TAG, "Sensor have commands in queue"
  );

    ESP_LOGI(TAG, "SHT30 initalized - addr= 0x%02X", config->i2c_addr);

  return ESP_OK;
}


esp_err_t sht30_read(sht30_data_t *data)
{
  uint8_t buf[6];

  ESP_RETURN_ON_ERROR(
    sht30_send_command(dev_config.cmd_measure),
    TAG, "Failed to send measurement command"
  );

  vTaskDelay(pdMS_TO_TICKS(dev_config.meas_delay_ms));

  ESP_RETURN_ON_ERROR(
    sht30_read_measurement(buf),
    TAG, "Failed to read sensor data"
  );

  ESP_RETURN_ON_ERROR(
    sht30_convert(buf, data),
    TAG, "Failed to convert raw data"
  );

  return  ESP_OK;
}

/* sht30_reset — sends soft reset command (0x30A2)
 * All internal state machines return to default.
 * Waits 2ms after reset — datasheet section 4.9
 *
 * @return  ESP_OK on success
 *          ESP_FAIL if I2C write fails
 */
esp_err_t sht30_reset(void)
{
  ESP_RETURN_ON_ERROR(
    sht30_send_command(SHT30_CMD_SOFT_RESET),
    TAG, "Failed to reset sensor"
  );

  vTaskDelay(pdMS_TO_TICKS(2));

  return ESP_OK;
}

/* sht30_check_status — reads status register via command 0xF32D
 * Reads 3 bytes: status MSB + status LSB + CRC.
 * Verifies CRC to confirm sensor is communicating correctly.
 * Datasheet section 4.8
 *
 * @return  ESP_OK if status register reads correctly
 *          ESP_ERR_INVALID_CRC if CRC verification fails
 *          ESP_FAIL if I2C communication fails
 */
esp_err_t sht30_check_status(void)
{
  uint8_t buf[3];

  ESP_RETURN_ON_ERROR(
    sht30_send_command(SHT30_CMD_READ_STATUS),
    TAG, "Failed to read status"
  );

  vTaskDelay(pdMS_TO_TICKS(1));

  ESP_RETURN_ON_ERROR(
    i2c_master_receive(dev_handle, buf, 3, pdMS_TO_TICKS(100)), 
    TAG, "Failed to receive status data"
  );

  // Use — verifies status
  uint8_t status_crc = sht30_crc(&buf[0], 2);  // calc 2 bytes CRC
  if (status_crc != buf[2]) {
    // corrupted data — ERROR
    return ESP_ERR_INVALID_CRC;
  }

  ESP_LOGI(TAG, "SHT30 Status OK");

  return ESP_OK;
}
