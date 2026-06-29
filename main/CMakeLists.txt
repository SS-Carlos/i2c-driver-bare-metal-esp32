idf_component_register(
  SRCS "i2c-driver-bare-metal-esp32.c"
  INCLUDE_DIRS "."
  REQUIRES
    bmp280
    esp_driver_i2c
    freertos
)
