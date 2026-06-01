# BMP280 Driver for ESP32

Driver for the Bosch BMP280 temperature and pressure sensor,
implemented in C using ESP-IDF v6.x without third-party libraries.

## Features
- I2C communication using ESP-IDF i2c_master driver
- Full compensation formulas from official datasheet
- Temperature and pressure reading

## Hardware
- ESP32 (tested on ESP32-D0WDQ6)
- BMP280 sensor module

## Usage
```c
bmp280_init();
float temp, press;
bmp280_read(&temp, &press);
```

## References
- [BMP280 Datasheet](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp280-ds001.pdf)
- [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/)

## License
MIT © Carlos Solano 2025
