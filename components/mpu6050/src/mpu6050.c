/*
 * MPU6050 Driver for ESP32
 * Copyright (c) 2026 Carlos Solano
 *
 * SPDX-License-Identifier: MIT
 *
 * Datasheet: RM-MPU-6000A.pdf
 */

#include "mpu6050.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"

static const char *TAG = "MPU6050";

// Private module instances
static i2c_master_dev_handle_t dev_handle;


// Private functions

/* ── mpu6050_read_register ────────────────────────────────────────────────
 *
 * Writes the register address then reads len bytes from the sensor.
 * Uses i2c_master_transmit_receive() — single transaction on the bus.
 *
 * @param reg   Register address to read from
 * @param data  Buffer to store the received bytes
 * @param len   Number of bytes to read
 * @return      ESP_OK on success
 */

static esp_err_t mpu6050_read_register(uint8_t reg, 
                                      uint8_t *data, 
                                      uint8_t len) 
{
  return i2c_master_transmit_receive(dev_handle, 
                                     &reg, 1, 
                                     data, len, 
                                     pdMS_TO_TICKS(100));
}

/* ── mpu6050_write_register ───────────────────────────────────────────────
 *
 * Writes a single byte value to the given register.
 *
 * @param reg    Register address to write to
 * @param value  Byte value to write
 * @return       ESP_OK on success
 */

static esp_err_t mpu6050_write_register(uint8_t reg, uint8_t value) 
{
  uint8_t data[2] = {reg, value};
  return i2c_master_transmit(dev_handle, data, sizeof(data), pdMS_TO_TICKS(100));
}


/*
 * ----- PUBLIC API -----
 */

esp_err_t mpu6050_check_id(void) 
{
  uint8_t chip_id;

  ESP_RETURN_ON_ERROR(
    mpu6050_read_register(MPU6050_REG_WHO_AM_I, &chip_id, 1),
    TAG, "Failed to read chip ID"
  );

  if (chip_id != 0x68 && chip_id != 0x98 && chip_id != 0x70) {
    ESP_LOGE(TAG, "Wrong chip ID: got 0x%02X", chip_id);
    return ESP_ERR_NOT_FOUND;
  }

  ESP_LOGI(TAG, "MPU6050 detected - chip ID: 0x%02X", chip_id);
  return ESP_OK;
}

esp_err_t mpu6050_reset(void)
{
  ESP_RETURN_ON_ERROR(
    mpu6050_write_register(MPU6050_REG_PWR_MGMT_1, 0x80),
    TAG, "failed to read sensor"
  );

  // wait for sensor complete reset
  vTaskDelay(pdMS_TO_TICKS(10));

  ESP_LOGI(TAG, "MPU6050 reset complete");
  return ESP_OK;
}

esp_err_t mpu6050_init(i2c_master_bus_handle_t bus_handle,
                        const mpu6050_config_t *config)
{
    /* 1. Register device on the I2C bus */
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = config->i2c_addr,
        .scl_speed_hz    = 400000,  // MPU6050 Fast Mode (400kHz)
    };

    ESP_RETURN_ON_ERROR(
        i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle),
        TAG, "Failed to add device to I2C bus"
    );

    /* Wait for sensor power-on reset to complete
     * MPU6050 needs ~100ms after power-on before responding correctly
     */
    vTaskDelay(pdMS_TO_TICKS(100));

    /* 2. Verify chip identity */
    ESP_RETURN_ON_ERROR(
        mpu6050_check_id(),
        TAG, "Chip ID check failed"
    );

    /* 3. Wake up sensor — exits sleep mode and configure clock source
     * Default after power-on: PWR_MGMT_1 = 0x40 (SLEEP bit set)
     * Write CLKSEL only (bits 2:0) with SLEEP=0
     * Datasheet recommends PLL with gyroscope for better stability
     */
    ESP_RETURN_ON_ERROR(
        mpu6050_write_register(MPU6050_REG_PWR_MGMT_1, config->clk_source),
        TAG, "Failed to wake up sensor"
    );

    /* Small delay after wake-up — sensor needs time to stabilize */
    vTaskDelay(pdMS_TO_TICKS(50));

    /* 4. Configure sample rate divider
     * Sample Rate = 1kHz / (1 + SMPLRT_DIV) when DLPF is active
     * 0x09 → 1000 / (1+9) = 100Hz
     */
    ESP_RETURN_ON_ERROR(
        mpu6050_write_register(MPU6050_REG_SMPLRT_DIV, 0x09),
        TAG, "Failed to configure sample rate"
    );

    /* 5. Configure digital low pass filter
     * bits 2:0 of CONFIG register
     */
    ESP_RETURN_ON_ERROR(
        mpu6050_write_register(MPU6050_REG_CONFIG, config->dlpf_config),
        TAG, "Failed to configure DLPF"
    );

    /* 6. Configure gyroscope range
     * bits 4:3 of GYRO_CONFIG register
     * datasheet section 4.4
     */
    uint8_t gyro_cfg = (uint8_t)(config->gyro_range << 3);
    ESP_RETURN_ON_ERROR(
        mpu6050_write_register(MPU6050_REG_GYRO_CONFIG, gyro_cfg),
        TAG, "Failed to configure gyroscope"
    );

    /* 7. Configure accelerometer range
     * bits 4:3 of ACCEL_CONFIG register
     * datasheet section 4.5
     */
    uint8_t accel_cfg = (uint8_t)(config->accel_range << 3);
    ESP_RETURN_ON_ERROR(
        mpu6050_write_register(MPU6050_REG_ACCEL_CONFIG, accel_cfg),
        TAG, "Failed to configure accelerometer"
    );

    ESP_LOGI(TAG, "MPU6050 initialized — addr=0x%02X gyro=%d accel=%d",
             config->i2c_addr, config->gyro_range, config->accel_range);

    return ESP_OK;
}

esp_err_t mpu6050_read(mpu6050_data_t *data)
{
  uint8_t buf[14];

  /* Read 14 bits starting at 0x3B - big endian, MSB first 
   * buf[0-5]  -> accelerometer X, Y, Z
   * buf[6-7]  -> temperature
   * buf[8-13] -> gyroscope X, Y, Z
   */

   ESP_RETURN_ON_ERROR(
    mpu6050_read_register(MPU6050_REG_ACCEL_XOUT_H, buf, 14),
    TAG, "Failed to read sensor data"
  );

  /* Assemble 16-bit signed values - big endian (MSB First)
   * opposite of BMP280 which was little endian
   */

  int16_t raw_ax = (int16_t)((buf[0]  << 8) | buf[1]);
  int16_t raw_ay = (int16_t)((buf[2]  << 8) | buf[3]);
  int16_t raw_az = (int16_t)((buf[4]  << 8) | buf[5]);
  int16_t raw_t  = (int16_t)((buf[6]  << 8) | buf[7]);
  int16_t raw_gx = (int16_t)((buf[8]  << 8) | buf[9]);
  int16_t raw_gy = (int16_t)((buf[10] << 8) | buf[11]);
  int16_t raw_gz = (int16_t)((buf[12] << 8) | buf[13]);

  /* Apply sensitivity scale factors - datasheet section 6.2
   * ACCEL_RANGE_2G -> 16384.0 LSB/g
   * GYRO_RANGE_250 -> 131.0   LSB/°/s
   * Temperature formula from datasheet: temp = raw/340 + 36.53
   */

  data->accel_x = (float)raw_ax / 16384.0f;
  data->accel_y = (float)raw_ay / 16384.0f;
  data->accel_z = (float)raw_az / 16384.0f;

  data->gyro_x  = (float)raw_gx / 131.0f;
  data->gyro_y  = (float)raw_gy / 131.0f;
  data->gyro_z  = (float)raw_gz / 131.0f;

  data->temp    = (float)raw_t/ 340.0f + 36.53f;

  return ESP_OK;
}
