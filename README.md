# BMP280 Driver for ESP32

Bare-metal driver for the Bosch BMP280 temperature and pressure sensor,
implemented in C using ESP-IDF v6.x without third-party sensor libraries.

The driver is structured as a reusable ESP-IDF component with a clean
separation between I2C communication, sensor logic, and compensation formulas.

## Features

- I2C communication using the ESP-IDF `i2c_master` driver
- Full compensation formulas from the official Bosch datasheet (section 4.2.3)
- Modular architecture split into independent components
- Error handling using `ESP_RETURN_ON_ERROR` throughout
- Structured logging with `ESP_LOG` at appropriate levels
- No third-party sensor libraries — registers and formulas implemented from scratch

## Architecture

```
components/bmp280/
├── include/
│   ├── bmp280_types.h   — types, structs, register definitions and constants
│   ├── bmp280_comp.h    — compensation formula declarations (internal API)
│   └── bmp280.h         — public API
└── src/
    ├── bmp280.c         — I2C communication, initialization and data reading
    └── bmp280_comp.c    — temperature and pressure compensation formulas
```

## Hardware

- ESP32-D0WDQ6 (revision v1.0, dual core 240MHz)
- BMP280 sensor module (I2C, SDO to GND → address 0x76)
- SDA → GPIO 21 / SCL → GPIO 22

## Usage

```c
#include "bmp280.h"

// Initialize I2C bus
i2c_master_bus_handle_t bus_handle;
i2c_master_bus_config_t bus_cfg = {
    .i2c_port                     = I2C_NUM_0,
    .sda_io_num                   = GPIO_NUM_21,
    .scl_io_num                   = GPIO_NUM_22,
    .clk_source                   = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt            = 7,
    .flags.enable_internal_pullup = true,
};
ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));

// Initialize BMP280 with default config
bmp280_config_t config = BMP280_DEFAULT_CONFIG;
ESP_ERROR_CHECK(bmp280_init(bus_handle, &config));

// Read sensor data
bmp280_data_t data;
ESP_ERROR_CHECK(bmp280_read(&data));

ESP_LOGI(TAG, "Temperature: %.2f C  Pressure: %.2f hPa",
         data.temperature, data.pressure);
```

## API Reference

| Function | Description |
|---|---|
| `bmp280_init(bus, config)` | Initialize sensor, verify chip ID, load calibration |
| `bmp280_read(data)` | Read and compensate temperature and pressure |
| `bmp280_reset()` | Soft reset via register 0xE0 |
| `bmp280_check_id()` | Verify chip ID (0x58) — also called by init |

## References

- [BMP280 Datasheet — BST-BMP280-DS001](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp280-ds001.pdf)
- [ESP-IDF v6 Documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/)
- [ESP-IDF i2c_master API](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/i2c.html)

## License

MIT © Carlos Solano 2026
