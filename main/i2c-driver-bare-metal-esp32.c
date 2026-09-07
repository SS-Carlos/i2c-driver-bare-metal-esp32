/*
 * Driver BMP280 para ESP32
 * Copyright (c) 2026 Carlos Solano
 *
 * SPDX-License-Identifier: MIT
 *
 * Datasheet: BST-BMP280-DS001.pdf
 */

#include "bmp280.h"
#include "mpu6050.h"
#include "sht30.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "MAIN";

// define I2C pins
#define I2C_SDA GPIO_NUM_21
#define I2C_SCL GPIO_NUM_22

void app_main(void)
{
  i2c_master_bus_handle_t bus_handle;
  i2c_master_bus_config_t bus_cfg = 
  {
      .i2c_port = I2C_NUM_0,
      .sda_io_num = I2C_SDA,
      .scl_io_num = I2C_SCL,
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .glitch_ignore_cnt = 7,
      .flags.enable_internal_pullup = true,
  };

  ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));
  ESP_LOGI(TAG, "I2C busus initialized");
  
  // initialize bmp280
  bmp280_config_t bmp_config = BMP280_DEFAULT_CONFIG;
  ESP_ERROR_CHECK(bmp280_init(bus_handle, &bmp_config));

  vTaskDelay(pdMS_TO_TICKS(100));

  // initialize mpu6050
  mpu6050_config_t mpu_config = MPU6050_DEFAULT_CONFIG;
  ESP_ERROR_CHECK(mpu6050_init(bus_handle, &mpu_config));

  vTaskDelay(pdMS_TO_TICKS(100));

  sht30_config_t sht_config = SHT30_DEFAULT_CONFIG;
  ESP_ERROR_CHECK(sht30_init(bus_handle, &sht_config));

  // Read data
  bmp280_data_t  bmp_data;
  mpu6050_data_t mpu_data;
  sht30_data_t   sht_data;


  while(1)
  {
    // bmp280 - temperature and pressure
    esp_err_t ret = bmp280_read(&bmp_data);
    if (ret == ESP_OK)
    {
       ESP_LOGI(TAG, "Temperature: %.2f C  Pressure: %.2f hPa",
                bmp_data.temperature,bmp_data.pressure);
    } else {
      ESP_LOGE(TAG, "Failed to read sensor: %s",
               esp_err_to_name(ret));
    }

    // mpu6050 - accelerometer, gyroscope and temperature
    ret = mpu6050_read(&mpu_data);
    if (ret == ESP_OK) {
      ESP_LOGI(TAG, "MPU6050 - Accel: X=%.2f Y=%.2f Z=%.2f g",
               mpu_data.accel_x, mpu_data.accel_y, mpu_data.accel_z);
      ESP_LOGI(TAG, "MPU6050 - Gyro:  X=%.2f Y=%.2f Z=%.2f °/s",
               mpu_data.gyro_x, mpu_data.gyro_y, mpu_data.gyro_z);
      ESP_LOGI(TAG, "MPU6050 - Temp: %.2f C", mpu_data.temp);
    } else {
      ESP_LOGE(TAG, "MPU6050 read failed: %s", esp_err_to_name(ret));
    }

    ret = sht30_read(&sht_data);
    if (ret == ESP_OK) {
      ESP_LOGI(TAG, "SHT30 - Temp: %.2f C Humidity: %.2f %%", 
               sht_data.temperature, sht_data.humidity);
    } else {
      ESP_LOGE(TAG, "SHT30 Read failed: %s", esp_err_to_name(ret));
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
