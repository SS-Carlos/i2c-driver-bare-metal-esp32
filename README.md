# I2C Bare-Metal Drivers for ESP32

Collection of bare-metal I2C sensor drivers for the ESP32, implemented
in C using ESP-IDF v6.x without third-party sensor libraries.

Each driver is structured as a reusable ESP-IDF component with a clean
separation between I2C communication, sensor logic, and data processing.

## Sensors

| Sensor | Manufacturer | Measurements | Address |
|--------|-------------|--------------|---------|
| BMP280 | Bosch | Temperature, Pressure | 0x76 / 0x77 |
| MPU6050 | InvenSense | Accelerometer, Gyroscope, Temperature | 0x68 / 0x69 |
| SHT30 | Sensirion | Temperature, Humidity | 0x44 / 0x45 |

## Features

- I2C communication using the ESP-IDF `i2c_master` driver
- All sensors sharing a single I2C bus simultaneously
- Modular architecture — each sensor is an independent ESP-IDF component
- Error handling using `ESP_RETURN_ON_ERROR` throughout
- Structured logging with `ESP_LOG` at appropriate levels
- CRC-8 verification on SHT30 data integrity
- Full compensation formulas from official datasheets
- No third-party sensor libraries — registers and formulas implemented from scratch

## Architecture

```
components/
├── bmp280/
│   ├── include/
│   │   ├── bmp280_types.h   — types, structs, register definitions
│   │   ├── bmp280_comp.h    — compensation formula declarations
│   │   └── bmp280.h         — public API
│   └── src/
│       ├── bmp280.c         — I2C communication and initialization
│       └── bmp280_comp.c    — temperature and pressure compensation
├── mpu6050/
│   ├── include/
│   │   ├── mpu6050_types.h  — types, structs, register definitions
│   │   └── mpu6050.h        — public API
│   └── src/
│       └── mpu6050.c        — I2C communication, initialization and scaling
└── sht30/
    ├── include/
    │   ├── sht30_types.h    — types, structs, command definitions
    │   └── sht30.h          — public API
    └── src/
        └── sht30.c          — command-based I2C communication and CRC-8
```

## Hardware

- ESP32-D0WDQ6 (revision v1.0, dual core 240MHz)
- BMP280 sensor module — I2C address 0x76 (SDO to GND)
- MPU6050 GY-521 module — I2C address 0x68 (AD0 to GND)
- SHT30 sensor module — I2C address 0x44 (ADDR to GND)
- SDA → GPIO 21 / SCL → GPIO 22 (shared bus)

## Usage

```c
#include "bmp280.h"
#include "mpu6050.h"
#include "sht30.h"

// Initialize shared I2C bus — one bus for all sensors
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

// Initialize sensors
bmp280_config_t  bmp_config = BMP280_DEFAULT_CONFIG;
mpu6050_config_t mpu_config = MPU6050_DEFAULT_CONFIG;
sht30_config_t   sht_config = SHT30_DEFAULT_CONFIG;

ESP_ERROR_CHECK(bmp280_init(bus_handle, &bmp_config));
ESP_ERROR_CHECK(mpu6050_init(bus_handle, &mpu_config));
ESP_ERROR_CHECK(sht30_init(bus_handle, &sht_config));

// Read sensor data
bmp280_data_t  bmp_data;
mpu6050_data_t mpu_data;
sht30_data_t   sht_data;

ESP_ERROR_CHECK(bmp280_read(&bmp_data));
ESP_ERROR_CHECK(mpu6050_read(&mpu_data));
ESP_ERROR_CHECK(sht30_read(&sht_data));
```

## API Reference

### BMP280
| Function | Description |
|---|---|
| `bmp280_init(bus, config)` | Initialize sensor, verify chip ID, load calibration |
| `bmp280_read(data)` | Read and compensate temperature and pressure |
| `bmp280_reset()` | Soft reset via register 0xE0 |
| `bmp280_check_id()` | Verify chip ID (0x58) |

### MPU6050
| Function | Description |
|---|---|
| `mpu6050_init(bus, config)` | Initialize sensor, wake up, configure ranges |
| `mpu6050_read(data)` | Read accelerometer, gyroscope and temperature |
| `mpu6050_reset()` | Soft reset via PWR_MGMT_1 bit 7 |
| `mpu6050_check_id()` | Verify WHO_AM_I register (0x68) |

### SHT30
| Function | Description |
|---|---|
| `sht30_init(bus, config)` | Initialize sensor, reset, verify status |
| `sht30_read(data)` | Send measurement command, read and verify CRC |
| `sht30_reset()` | Soft reset via command 0x30A2 |
| `sht30_check_status()` | Read and verify status register with CRC |

## Known Issue

I2C bus collisions detected with logic analyzer when reading three sensors
sequentially in a single loop. To be resolved in the next repository with
FreeRTOS tasks and mutex protection for shared bus access.

## References

- [BMP280 Datasheet — BST-BMP280-DS001](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp280-ds001.pdf)
- [MPU6050 Product Specification](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf)
- [MPU6050 Register Map](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf)
- [SHT30 Datasheet — SHT3x-DIS](https://sensirion.com/media/documents/213E6A3B/63A5A569/Datasheet_SHT3x_DIS.pdf)
- [ESP-IDF v6 Documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/)
- [ESP-IDF i2c_master API](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/i2c.html)

## License

MIT © Carlos Solano 2026
