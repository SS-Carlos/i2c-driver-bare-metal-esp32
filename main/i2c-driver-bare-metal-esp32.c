/*
 * Driver BMP280 para ESP32
 * Copyright (c) 2026 Carlos Solano
 *
 * SPDX-License-Identifier: MIT
 *
 * Datasheet: BST-BMP280-DS001.pdf
 */

#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"

// We define the output pins (depend of each microcontroller)
#define I2C_SDA GPIO_NUM_21
#define I2C_SCL GPIO_NUM_22

// I2C direction when SD0 is LOW (GND) or 0x77 if is HIGH (Vcc)
#define BMP_ADDR 0x76

// Registers
#define BMP280_CHIP_ID_REG 0xD0
#define BMP280_CTRL_MEAS   0xF4
#define BMP280_PRESS_MSB   0xF7

i2c_master_bus_handle_t bus_handle;
i2c_master_dev_handle_t bmp280;

void bmp280_read_register(uint8_t reg, uint8_t *data, uint8_t len)
{
  i2c_master_transmit_receive(bmp280, &reg, 1, data, len, -1);
}

void bmp280_write_register(uint8_t reg, uint8_t value)
{
  uint8_t data[2] = {reg, value};
  i2c_master_transmit(bmp280, data, sizeof(data), -1);
}

// ---------- Calibration struct ----------

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
} bmp280_calibration_data;

static bmp280_calibration_data bmp280_calibration_d;

static esp_err_t bmp280_read_calibration(void)
{
  uint8_t buff[24];
  bmp280_read_register(0x88, buff, 24);

  bmp280_calibration_d.dig_T1 = (buff[1]<<8) | buff[0];
  bmp280_calibration_d.dig_T2 = (buff[3]<<8) | buff[2];
  bmp280_calibration_d.dig_T3 = (buff[5]<<8) | buff[4];
    
  bmp280_calibration_d.dig_P1 = (buff[7]<<8) | buff[6];
  bmp280_calibration_d.dig_P2 = (buff[9]<<8) | buff[8];
  bmp280_calibration_d.dig_P3 = (buff[11]<<8) | buff[10];
  bmp280_calibration_d.dig_P4 = (buff[13]<<8) | buff[12];
  bmp280_calibration_d.dig_P5 = (buff[15]<<8) | buff[14];
  bmp280_calibration_d.dig_P6 = (buff[17]<<8) | buff[16];
  bmp280_calibration_d.dig_P7 = (buff[19]<<8) | buff[18];
  bmp280_calibration_d.dig_P8 = (buff[21]<<8) | buff[20];
  bmp280_calibration_d.dig_P9 = (buff[23]<<8) | buff[22];

  return ESP_OK;
}
// ----------------------------------------

// ---------- Compensation formula ----------
typedef int32_t BMP280_S32_T;
typedef uint32_t BMP280_U32_T;
typedef int64_t BMP280_S64_T;

// Returns temperature in DegC
BMP280_S32_T t_fine;
BMP280_S32_T bmp280_compensate_T_int32 (BMP280_S32_T adc_T)
{
  // local variables for compensation
  int32_t dig_T1 = bmp280_calibration_d.dig_T1;
  int32_t dig_T2 = bmp280_calibration_d.dig_T2;
  int32_t dig_T3 = bmp280_calibration_d.dig_T3;

  BMP280_S32_T var_1, var_2, T;
  var_1 = ((((adc_T>>3) - ((BMP280_S32_T)dig_T1<<1))) * ((BMP280_S32_T)dig_T2)) >> 11;
  var_2 = (((((adc_T>>4) - ((BMP280_S32_T)dig_T1)) * ((adc_T>>4) - ((BMP280_S32_T)dig_T1))) >> 12) * ((BMP280_S32_T)dig_T3)) >> 14;
  t_fine = var_1 + var_2;
  T = (t_fine * 5 + 128) >> 8;

  return T;
}

// returns pressure in Pa as unsigned 32 bit integer in Q24.8 format
BMP280_U32_T bmp280_compensate_P_int64 (BMP280_U32_T adc_P)
{
  // local variables for compensation
  int32_t dig_P1 = bmp280_calibration_d.dig_P1;
  int32_t dig_P2 = bmp280_calibration_d.dig_P2;
  int32_t dig_P3 = bmp280_calibration_d.dig_P3;
  int32_t dig_P4 = bmp280_calibration_d.dig_P4;
  int32_t dig_P5 = bmp280_calibration_d.dig_P5;
  int32_t dig_P6 = bmp280_calibration_d.dig_P6;
  int32_t dig_P7 = bmp280_calibration_d.dig_P7;
  int32_t dig_P8 = bmp280_calibration_d.dig_P8;
  int32_t dig_P9 = bmp280_calibration_d.dig_P9;

  BMP280_S64_T var_1, var_2, p;
  var_1 = ((BMP280_S64_T)t_fine) - 128000;
  var_2 = var_1 * var_1 * (BMP280_S64_T)dig_P6;
  var_2 = var_2 + ((var_1*(BMP280_S64_T)dig_P5)<<17);
  var_2 = var_2 + (((BMP280_S64_T)dig_P4)<<35);
  var_1 = ((var_1 * var_1 * (BMP280_S64_T)dig_P3)>>8) + ((var_1 * (BMP280_S64_T)dig_P2)<<12);
  var_1 = (((((BMP280_S64_T)1)<<47)+var_1)) * ((BMP280_S64_T)dig_P1)>>33;

  if (var_1 == 0)
  {
    return 0; // avoid exception caused by division by zero
  }

  p = 1048576 - adc_P;
  p = (((p<<31)-var_2)*3125)/var_1;

  var_1 = (((BMP280_S64_T)dig_P9) * (p>>13) * (p>>13)) >> 25;
  var_2 = (((BMP280_S64_T)dig_P8) * p) >> 19;

  p = ((p + var_1 + var_2) >> 8) + (((BMP280_S64_T)dig_P7)<<4);

  return (BMP280_U32_T)p;
}

// ----------------------------------------

// main function
void app_main(void)
{
  i2c_master_bus_config_t bus_cfg = 
  {
      .i2c_port = I2C_NUM_0,
      .sda_io_num = I2C_SDA,
      .scl_io_num = I2C_SCL,
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .glitch_ignore_cnt = 7,
  };

  i2c_new_master_bus(&bus_cfg, &bus_handle);

  i2c_device_config_t dev_cfg =
  {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = BMP_ADDR,
      .scl_speed_hz = 100000,
  };

  i2c_master_bus_add_device(bus_handle, &dev_cfg, &bmp280);

  //create instances
  uint8_t chip_id;

  bmp280_read_register(BMP280_CHIP_ID_REG, &chip_id, 1);
  bmp280_read_calibration();
   
  printf("Chip ID: 0x%X\n", chip_id);

  if (chip_id != 0x58) {
    printf("BMP Not found");
    return;
  }
  printf("BMP Found");

  bmp280_write_register(BMP280_CTRL_MEAS, 0x27);

  while(1)
  {
    uint8_t raw[6];

    bmp280_read_register(BMP280_PRESS_MSB, raw, 6);
    
    int32_t adc_P = ((int32_t)raw[0] << 12) | 
                    ((int32_t)raw[1] << 4) | 
                    ((int32_t)raw[2] >> 4);

    int32_t adc_T = ((int32_t)raw[3] << 12) | 
                    ((int32_t)raw[4] << 4) | 
                    ((int32_t)raw[5] >> 4);
    
    int32_t temp = bmp280_compensate_T_int32(adc_T);
    int32_t pres = bmp280_compensate_P_int64(adc_P);

    
    printf("Temperature: %ld.%02ld C\n", temp / 100, temp % 100);
    printf("Pressure in hPa: %ld\n", pres / 256);

    // printf("Temp raw=%lu Press raw=%lu\n", adc_T, adc_P);

    vTaskDelay(pdMS_TO_TICKS(1000));
  }

}
