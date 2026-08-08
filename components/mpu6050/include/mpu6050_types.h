/* MPU6050 Driver for ESP32
 * Copyright (c) 2026 Carlos Solano
 *
 * SPDX-License-Identifier: MIT
 *
 * Datasheet: RM-MPU-6000A.pdf
 */

#ifndef MPU6050_TYPES_H
#define MPU6050_TYPES_H

#include <stdint.h>

// I2C Identification
#define MPU6050_WHO_AM_I_VALUE         0x98  // must be 0x68 (0x98 is for clone chips)
#define MPU6050_REG_WHO_AM_I           0x75
#define MPU6050_I2C_ADDR_PRIMARY       0x68  // AD0 to GND
#define MPU6050_I2C_ADDR_SECONDARY     0x69  // AD0 to VCC

// Configuration registers
#define MPU6050_REG_SMPLRT_DIV         0x19
#define MPU6050_REG_CONFIG             0x1A
#define MPU6050_REG_GYRO_CONFIG        0x1B
#define MPU6050_REG_ACCEL_CONFIG       0x1C
#define MPU6050_REG_PWR_MGMT_1         0x6B

// Data registers
#define MPU6050_REG_ACCEL_XOUT_H       0x3B

// Configuration parameters
// ACCEL_CONFIG register bits 4:3 
#define MPU6050_ACCEL_RANGE_2G         0x00
#define MPU6050_ACCEL_RANGE_4G         0x01
#define MPU6050_ACCEL_RANGE_8G         0x02
#define MPU6050_ACCEL_RANGE_16G        0x03

// GYRO_CONFIG register bits 4:3
#define MPU6050_GYRO_RANGE_250         0x00
#define MPU6050_GYRO_RANGE_500         0x01
#define MPU6050_GYRO_RANGE_1000        0x02
#define MPU6050_GYRO_RANGE_2000        0x03

// DLPF - register CONFIG bits 2:0
#define MPU6050_DLPF_260HZ             0x00
#define MPU6050_DLPF_184HZ             0x01
#define MPU6050_DLPF_94HZ              0x02
#define MPU6050_DLPF_44HZ              0x03
#define MPU6050_DLPF_21HZ              0x04
#define MPU6050_DLPF_10HZ              0x05
#define MPU6050_DLPF_5HZ               0x06

// Clock Source - PWR_MGMT_1 register bits 2:0
#define MPU6050_CLOCK_INTERNAL         0x00
#define MPU6050_CLOCK_PLL_XGYRO        0x01

// MPU6050 Configuration struct
typedef struct {
  uint8_t i2c_addr;
  uint8_t accel_range;
  uint8_t gyro_range;
  uint8_t dlpf_config;
  uint8_t clk_source;
} mpu6050_config_t;

// MPU6050 Sensor Data
typedef struct {
  float accel_x;
  float accel_y;
  float accel_z;
  float gyro_x;
  float gyro_y;
  float gyro_z;
  float temp;
} mpu6050_data_t;

#endif // !MPU6050_TYPES_H
