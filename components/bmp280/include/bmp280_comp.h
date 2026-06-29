/*
 * BMP280 Driver for ESP32 — Compensation formulas header
 * Copyright (c) 2026 Carlos Solano
 *
 * SPDX-License-Identifier: MIT
 *
 * Datasheet: BST-BMP280-DS001.pdf — section 4.2.3
 */

#ifndef BMP280_COMP_H
#define BMP280_COMP_H

#include "bmp280_types.h"

/* ---- temperature compensation -----
 *
 * Receives raw ADC temeperature value (20-bit unsigned).
 * return temeperature in celsius degrees (T * 100).
 *
 * This function must be called before compensating pressure.
 *
 * @param adc_T  raw temeperature value from registers
 * @param calib  pointer to calibration coeficients struct
 * @return       compensated temeperature in degrees celsius
 *
 *
 */

BMP280_S32_T bmp280_compensate_temp(BMP280_S32_T adc_T,
                                    const bmp280_calib_t *calib);

/* ---- pressure compensation -----
 * 
 * Receives raw ADC pressure value (20-bit unsigned)
 * Returns pressure in Q24.8 format divide by 256 to get Pa
 * then divide by 100 to get hPa.
 *
 * To use this function the compensated temperature function
 * must be called first.
 *
 * @param adc_P  raw pressure values from registers
 * @param calib  pointer to calibration coeficients struct
 * @return       compensated pressure in Q24.8 format
 *
 */

BMP280_U32_T bmp280_compensate_press(BMP280_S32_T adc_P,
                                     const bmp280_calib_t *calib);


#endif
