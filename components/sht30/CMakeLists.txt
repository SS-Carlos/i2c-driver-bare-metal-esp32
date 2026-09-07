# SHT30 Driver Component
# Copyright (c) 2026 Carlos Solano
# SPDX-License-Identifier: MIT

idf_component_register(
    SRCS
        "src/sht30.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        esp_driver_i2c
        driver
        freertos
        esp_common
)
