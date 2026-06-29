/*
 * Driver BMP280 para ESP32
 * Copyright (c) 2026 Carlos Solano
 *
 * SPDX-License-Identifier: MIT
 *
 * Datasheet: BST-BMP280-DS001.pdf
 */

#include "bmp280.h"
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
  bmp280_config_t config = BMP280_DEFAULT_CONFIG;
  ESP_ERROR_CHECK(bmp280_init(bus_handle, &config));

  // Read data
  bmp280_data_t data;


  while(1)
  {
    esp_err_t ret = bmp280_read(&data);
    
    if (ret == ESP_OK)
    {
       ESP_LOGI(TAG, "Temperature: %.2f C  Pressure: %.2f hPa",
                data.temperature,data.pressure);
    } else {
      ESP_LOGE(TAG, "Failed to read sensor: %s",
               esp_err_to_name(ret));
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
