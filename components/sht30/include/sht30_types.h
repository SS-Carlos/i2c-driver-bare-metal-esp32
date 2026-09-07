/* SHT30 Driver for ESP32
 * Copyright (c) 2026 Carlos Solano
 *
 * SPDX-License-Identifier: MIT
 *
 * Datasheet: SHT3x_DIS.pdf
 */

#ifndef SHT30_TYPES_H
#define SHT30_TYPES_H

#include <stdint.h>

// I2C Directions
#define SHT30_I2C_ADDR_PRIMARY            0x44  // ADDR connected to logic low
#define SHT30_I2C_ADDR_SECONDARY          0x45  // ADDR connected to logic high

// Measurement commands for single shot
#define SHT30_CMD_SS_CLK_STRETCH_HIGH     0x2C06
#define SHT30_CMD_SS_CLK_STRETCH_MEDIUM   0x2C0D
#define SHT30_CMD_SS_CLK_STRETCH_LOW      0x2C10

#define SHT30_CMD_SS_HIGH                 0x2400
#define SHT30_CMD_SS_MEDIUM               0x240B
#define SHT30_CMD_SS_LOW                  0x2416

// Measurement commands for periodic mode
#define SHT30_CMD_PERIODIC_0_5MPS_HIGH    0x2032
#define SHT30_CMD_PERIODIC_0_5MPS_MEDIUM  0x2024
#define SHT30_CMD_PERIODIC_0_5MPS_LOW     0x202F

#define SHT30_CMD_PERIODIC_1MPS_HIGH      0x2130
#define SHT30_CMD_PERIODIC_1MPS_MEDIUM    0x2126
#define SHT30_CMD_PERIODIC_1MPS_LOW       0x212D

#define SHT30_CMD_PERIODIC_2MPS_HIGH      0x2236
#define SHT30_CMD_PERIODIC_2MPS_MEDIUM    0x2220
#define SHT30_CMD_PERIODIC_2MPS_LOW       0x222B

#define SHT30_CMD_PERIODIC_4MPS_HIGH      0x2334
#define SHT30_CMD_PERIODIC_4MPS_MEDIUM    0x2322
#define SHT30_CMD_PERIODIC_4MPS_LOW       0x2329

#define SHT30_CMD_PERIODIC_10MPS_HIGH     0x2737
#define SHT30_CMD_PERIODIC_10MPS_MEDIUM   0x2721
#define SHT30_CMD_PERIODIC_10MPS_LOW      0x272A

#define SHT30_CMD_PERIODIC_READOUT        0xE000
#define SHT30_CMD_PERIODIC_ART            0x2B32
#define SHT30_CMD_PERIODIC_STOP_DATA_ACQ  0x3093

// Reset commands
#define SHT30_CMD_SOFT_RESET              0x30A2
#define SHT30_CMD_GENERAL_CALL_RESET      0x0006

// Integrated Heater
#define SHT30_CMD_HEATER_ENABLE           0x306D
#define SHT30_CMD_HEATER_DISABLED         0x3066

// Status register
#define SHT30_CMD_READ_STATUS             0xF32D
#define SHT30_CMD_CLEAR_STATUS_REGISTER   0x3041

// Measurement duration in ms — datasheet section 2.1
// Used for vTaskDelay after sending measurement command
#define SHT30_MEAS_DURATION_HIGH          15  // ms
#define SHT30_MEAS_DURATION_MEDIUM        6   // ms
#define SHT30_MEAS_DURATION_LOW           4   // ms


// SHT30 Configuration struct
typedef struct {
  uint8_t  i2c_addr;
  uint16_t cmd_measure;
  uint32_t meas_delay_ms;
} sht30_config_t;

// SHT30 Sensor Data
typedef struct {
  float temperature;
  float humidity;
} sht30_data_t;

#endif // SHT30_TYPES_H
