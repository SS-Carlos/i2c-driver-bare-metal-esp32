/* SHT30 Driver for ESP32
 * Copyright (c) 2026 Carlos Solano
 *
 * SPDX-License-Identifier: MIT
 *
 * Datasheet: SHT3x_DIS.pdf
 */

#ifndef SHT30_H
#define SHT30_H

#include "sht30_types.h"
#include "esp_err.h"
#include "driver/i2c_master.h"

/* Default configuration
 *
 * Values for general use:
 *  - Primary I2C address (ADDR to GND)
 *  - Single shot mode - measurement on demand
 *  - High repeability - best accuracy, ~15ms measurement time
 *
 * Usage:
 *  sht30_config_t config = SHT30_DEFAULT_CONFIG;
 *  sht30_init(bus_handle, &config);
 */

// SHT30 Default Config
#define SHT30_DEFAULT_CONFIG {                \
  .i2c_addr      = SHT30_I2C_ADDR_PRIMARY,  \
  .cmd_measure   = SHT30_CMD_SS_HIGH,       \
  .meas_delay_ms = SHT30_MEAS_DURATION_HIGH \
}

/* ── sht30_init ────────────────────────────────────────────────────────
 *
 * Initializes the sht30 sensor on the I2C bus.
 *
 * Performs in order:
 *   1. Registers the device on the I2C bus
 *   2. Reads and validates status register
 *   3. Sends soft reset to ensure clean state
 *   4. Waits for sensor to be ready
 *
 * Must be called once before any sht30_read() call.
 *
 * @param bus_handle  Initialized I2C master bus handle from app_main
 * @param config      Pointer to sensor configuration struct
 * @return            ESP_OK on success
 *                    ESP_FAIL if I2C communication fails
 */

esp_err_t sht30_init(i2c_master_bus_handle_t bus_handle, const sht30_config_t *config);

/* ── sht30_read ────────────────────────────────────────────────────────
 *
 * Triggers a single shot measurement and reads temperature and humidity.
 *
 * Sequence (Datasheet section 4.3):
 *  1. Send 16-bit measurement command (e.g. 0x2400 - SS High repeability)
 *  2. Wait 15ms for measurement to complete
 *  3. Read 6 bytes: temp(2) + crc(1) + humidity(2) + crc(1)
 *  4. Verify CRC-8 for both temperature and humidity
 *  5. Apply conversion formulas from datasheet section 4.13
 *
 * Conversion formulas:
 *  temperature = -45 + 175 * (raw_temp / 65535) -> °C
 *  humidity    = 100 * (raw_hum / 65535)        -> %RH
 *
 * @param data  Pointer to struct where results are stored:
 *                data->temperature   in °C
 *                data->humidity      in %RH
 * @return      ESP_OK on success
 *              ESP_ERR_INVALID_CRC if CRC verification fails
 *              ESP_FAIL if I2C read fails
 */

esp_err_t sht30_read(sht30_data_t *data);

/* ── sht30_reset ───────────────────────────────────────────────────────
 *
 * Performs a soft reset by sending command 0x30A2.
 * All internal state machines return to default.
 * Waits 2ms after reset as required by datasheet section 4.9.
 *
 * @return  ESP_OK on success
 *          ESP_FAIL if I2C write fails
 */

esp_err_t sht30_reset(void);

/* ── sht30_check_status ────────────────────────────────────────────────────
 *
 * Reads the status register via command 0xF32D (Datasheet section 4.8).
 * Returns a 16-bit status word followed by CRC.
 * Called internally by sht30_init() - exposed here for diagnostics.
 * 
 * Status reguster bits of interest:
 *   Bit 15: alert pending
 *   Bit 11: heater status
 *   Bit  2: reset detected
 *
 * @return  ESP_OK if status register reads correctly
 *          ESP_FAIL if communication fails
 */

esp_err_t sht30_check_status(void);

#endif //SHT30_H
