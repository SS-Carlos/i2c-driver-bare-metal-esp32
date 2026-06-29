/*
 * BMP280 Driver for ESP32 — Public API header
 * Copyright (c) 2026 Carlos Solano
 *
 * SPDX-License-Identifier: MIT
 *
 * Datasheet: BST-BMP280-DS001.pdf
 */

#ifndef BMP280_H
#define BMP280_H

#include "bmp280_types.h"
#include "esp_err.h"
#include "driver/i2c_master.h"

/* Default configuration
 * 
 * Values for general use:
 *  - Primary I2C address (SDO to GND)
 *  - x1 oversampling for temperature and pressure
 *  - Normal mode -> continuous measurements
 *  
 * Usage:
 *  bmp280_config_t config = BMP280_DEFAULT_CONFIG;
 *  bmp280_init(bus_handle, &config);
 */

#define BMP280_DEFAULT_CONFIG {       \
  .i2c_addr = BMP280_I2C_ADDR_PRIMARY,\
  .osrs_t = BMP280_OSRS_X1,           \
  .osrs_p = BMP280_OSRS_X1,           \
  .mode = BMP280_MODE_NORMAL          \
}

/* ── bmp280_init ─────────────────────────────────────────────────────────
 *
 * Initializes the BMP280 sensor on the I2C bus.
 *
 * Performs in order:
 *   1. Registers the device on the I2C bus
 *   2. Verifies chip ID (must be 0x58)
 *   3. Reads calibration coefficients from registers 0x88-0x9F
 *   4. Writes oversampling and operating mode to ctrl_meas (0xF4)
 *
 * Must be called once before any bmp280_read() call.
 *
 * @param bus_handle  Initialized I2C master bus handle from app_main
 * @param config      Pointer to sensor configuration struct
 * @return            ESP_OK on success
 *                    ESP_ERR_NOT_FOUND if chip ID is wrong
 *                    ESP_FAIL if I2C communication fails
 */

esp_err_t bmp280_init(i2c_master_bus_handle_t bus_handle, const bmp280_config_t *config);

/* ── bmp280_read ─────────────────────────────────────────────────────────
 *
 * Reads and compensates temperature and pressure from the sensor.
 *
 * Reads 6 raw bytes from registers 0xF7-0xFC:
 *   0xF7, 0xF8, 0xF9 → raw pressure  (20 bits)
 *   0xFA, 0xFB, 0xFC → raw temperature (20 bits)
 *
 * Temperature is always compensated first — required to update
 * t_fine before pressure compensation.
 *
 * @param data  Pointer to struct where results are stored:
 *                data->temperature in degrees Celsius
 *                data->pressure    in hPa
 * @return      ESP_OK on success
 *              ESP_FAIL if I2C read fails
 */

esp_err_t bmp280_read(bmp280_data_t *data);

/* ── bmp280_reset ────────────────────────────────────────────────────────
 *
 * Performs a soft reset by writing 0xB6 to register 0xE0.
 * Equivalent to a power-on reset — all registers return to default.
 * Waits 10ms after reset for the sensor to be ready.
 *
 * @return  ESP_OK on success
 *          ESP_FAIL if I2C write fails
 */
esp_err_t bmp280_reset(void);

/* ── bmp280_check_id ─────────────────────────────────────────────────────
 *
 * Reads chip ID register (0xD0) and validates it against BMP280_CHIP_ID.
 * Called internally by bmp280_init() — exposed here for diagnostics.
 *
 * @return  ESP_OK if chip ID is 0x58
 *          ESP_ERR_NOT_FOUND if chip ID is wrong or sensor not responding
 */
esp_err_t bmp280_check_id(void);

#endif // BMP280_H
