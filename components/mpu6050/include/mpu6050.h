/* MPU6050 Driver for ESP32
 * Copyright (c) 2026 Carlos Solano
 *
 * SPDX-License-Identifier: MIT
 *
 * Datasheet: RM-MPU-6000A.pdf
 */

#ifndef MPU6050_H
#define MPU6050_H

#include "mpu6050_types.h"
#include "esp_err.h"
#include "driver/i2c_master.h"

/*
 * Values for general use:
 *  - Primary I2C address (AD0 to GND)
 *  - Accelerometer range: ±2g
 *  - Gyroscope range: ±250°/s
 *  - DLPF: 44Hz bandwidth
 *  - Clock: PLL with X axis gyroscope
 *
 * Usage:
 *  mpu6050_config_t config = MPU6050_DEFAULT_CONFIG;
 *  mpu6050_init(bus_handle, &config);
 */

// MPU6050 Default Config
#define MPU6050_DEFAULT_CONFIG {             \
  .i2c_addr    = MPU6050_I2C_ADDR_PRIMARY,  \
  .accel_range = MPU6050_ACCEL_RANGE_2G,    \
  .gyro_range  = MPU6050_GYRO_RANGE_250,    \
  .dlpf_config = MPU6050_DLPF_44HZ,         \
  .clk_source  = MPU6050_CLOCK_PLL_XGYRO    \
}

/* ── mpu6050_init ────────────────────────────────────────────────────────
 *
 * Initializes the MPU6050 sensor on the I2C bus.
 *
 * Performs in order:
 *   1. Registers the device on the I2C bus
 *   2. Verifies chip ID via WHO_AM_I register (must be 0x68)
 *   3. Wakes up the sensor — PWR_MGMT_1 (exits sleep mode)
 *   4. Configures sample rate divider — SMPLRT_DIV
 *   5. Configures digital low pass filter — CONFIG
 *   6. Configures gyroscope range — GYRO_CONFIG
 *   7. Configures accelerometer range — ACCEL_CONFIG
 *
 * Must be called once before any mpu6050_read() call.
 *
 * @param bus_handle  Initialized I2C master bus handle from app_main
 * @param config      Pointer to sensor configuration struct
 * @return            ESP_OK on success
 *                    ESP_ERR_NOT_FOUND if chip ID is wrong
 *                    ESP_FAIL if I2C communication fails
 */

esp_err_t mpu6050_init(i2c_master_bus_handle_t bus_handle, const mpu6050_config_t *config);

/* ── mpu6050_read ────────────────────────────────────────────────────────
 *
 * Reads accelerometer, gyroscope and temperature data from the sensor.
 *
 * Reads 14 consecutive bytes starting at register 0x3B:
 *   0x3B-0x40 → raw accelerometer X, Y, Z  (3 × int16_t, big endian)
 *   0x41-0x42 → raw temperature             (int16_t, big endian)
 *   0x43-0x48 → raw gyroscope X, Y, Z       (3 × int16_t, big endian)
 *
 * Raw values are scaled using sensitivity factors from the datasheet
 * based on the configured ranges in mpu6050_init().
 *
 * @param data  Pointer to struct where results are stored:
 *                data->accel_x/y/z  in g
 *                data->gyro_x/y/z   in °/s
 *                data->temp         in °C
 * @return      ESP_OK on success
 *              ESP_FAIL if I2C read fails
 */

esp_err_t mpu6050_read(mpu6050_data_t *data);

/* ── mpu6050_reset ───────────────────────────────────────────────────────
 *
 * Performs a soft reset by setting the DEVICE_RESET bit in PWR_MGMT_1.
 * All registers return to default values. Sensor enters sleep mode
 * after reset — mpu6050_init() must be called again to use the sensor.
 * Waits 100ms after reset for the sensor to be ready.
 *
 * @return  ESP_OK on success
 *          ESP_FAIL if I2C write fails
 */

esp_err_t mpu6050_reset(void);

/* ── mpu6050_check_id ────────────────────────────────────────────────────
 *
 * Reads WHO_AM_I register (0x75) and validates against MPU6050_WHO_AM_I_VALUE.
 * Called internally by mpu6050_init() — exposed here for diagnostics.
 *
 * @return  ESP_OK if WHO_AM_I returns 0x68
 *          ESP_ERR_NOT_FOUND if value is wrong or sensor not responding
 */

esp_err_t mpu6050_check_id(void);

#endif // MPU6050_H
