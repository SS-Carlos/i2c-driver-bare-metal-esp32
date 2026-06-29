# BMP280 Driver Component
# Copyright (c) 2026 Carlos Solano
# SPDX-License-Identifier: MIT

idf_component_register(
    SRCS
        "src/bmp280.c"
        "src/bmp280_comp.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        esp_driver_i2c
        driver
        freertos
        esp_common
)
