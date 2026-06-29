/*
 * BMP280 Driver for ESP32
 * Copyright (c) 2026 Carlos Solano
 *
 * SPDX-License-Identifier: MIT
 *
 * Datasheet: BST-BMP280-DS001.pdf
 */

#ifndef BMP280_TYPES_H
#define BMP280_TYPES_H

#include <stdint.h>

// Datasheet types
typedef int32_t  BMP280_S32_T;
typedef uint32_t BMP280_U32_T;
typedef int64_t  BMP280_S64_T;

// I2C directions
#define BMP280_I2C_ADDR_PRIMARY   0x76
#define BMP280_I2C_ADDR_SECUNDARY 0X77

// Registers
#define BMP280_REG_CALIB_START       0x88
#define BMP280_REG_CHIP_ID           0xD0
#define BMP280_REG_RESET             0xE0
#define BMP280_REG_STATUS            0xF3
#define BMP280_REG_CTRL_MEAS         0xF4
#define BMP280_REG_CONFIG            0xF5
#define BMP280_REG_PRESS_MSB         0xF7

// Known values
#define BMP280_CHIP_ID               0x58
#define BMP280_RESET_VALUE           0xB6

// Operation modes
#define BMP280_MODE_SLEEP            0X00
#define BMP280_MODE_FORCED           0x01
#define BMP280_MODE_NORMAL           0x03

// Oversampling
#define BMP280_OSRS_SKIP             0x00
#define BMP280_OSRS_X1               0x01
#define BMP280_OSRS_X2               0x02
#define BMP280_OSRS_X4               0x03
#define BMP280_OSRS_X8               0x04
#define BMP280_OSRS_X16              0x05

// Calibration struct
typedef struct {
  uint16_t dig_T1;
  int16_t  dig_T2;
  int16_t  dig_T3;

  uint16_t dig_P1;
  int16_t  dig_P2;
  int16_t  dig_P3;
  int16_t  dig_P4;
  int16_t  dig_P5;
  int16_t  dig_P6;
  int16_t  dig_P7;
  int16_t  dig_P8;
  int16_t  dig_P9;
} bmp280_calib_t;

// Sensor data struct
typedef struct {
  float temperature; //Celsius
  float pressure;    //hPa
} bmp280_data_t;

// Configuration struct
typedef struct {
  uint8_t i2c_addr;
  uint8_t osrs_t;
  uint8_t osrs_p;
  uint8_t mode;
} bmp280_config_t;

#endif // !BMP280_TYPES_H
