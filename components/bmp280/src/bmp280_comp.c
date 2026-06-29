/*
 * BMP280 Driver for ESP32 — Compensation formulas implementation
 * Copyright (c) 2026 Carlos Solano
 *
 * SPDX-License-Identifier: MIT
 *
 * Datasheet: BST-BMP280-DS001.pdf — section 4.2.3
 */

#include "bmp280_comp.h"

/* ── t_fine — shared variable between temperature and pressure ───────────
 *
 * Updated by bmp280_compensate_temp() and read by bmp280_compensate_press().
 * Declared static — invisible outside this file.
 * This is intentional: nobody should read or write t_fine directly.
 */

// Returns temperature in DegC
static BMP280_S32_T t_fine;
BMP280_S32_T bmp280_compensate_temp (BMP280_S32_T adc_T, const bmp280_calib_t *calib)
{
  // local variables for compensation
  BMP280_S32_T dig_T1 = calib->dig_T1;
  BMP280_S32_T dig_T2 = calib->dig_T2;
  BMP280_S32_T dig_T3 = calib->dig_T3;

  BMP280_S32_T var_1, var_2, T;
  var_1 = ((((adc_T >> 3) - (dig_T1 << 1))) * dig_T2) >> 11;
  var_2 = (((((adc_T >> 4) - dig_T1) * ((adc_T >> 4) - dig_T1)) >> 12) * dig_T3) >> 14;
  t_fine = var_1 + var_2;
  T = (t_fine * 5 + 128) >> 8;

  return T;
}

/* ── Pressure compensation (datasheet section 4.2.3) ────────────────────
 *
 * Uses int64_t (BMP280_S64_T) to avoid overflow during intermediate
 * calculations. Do NOT change these types — the math requires 64 bits.
 */

// returns pressure in Pa as unsigned 32 bit integer in Q24.8 format
BMP280_U32_T bmp280_compensate_press (BMP280_S32_T adc_P, const bmp280_calib_t *calib)
{
  // local variables for compensation
  BMP280_S64_T dig_P1 = calib->dig_P1;
  BMP280_S64_T dig_P2 = calib->dig_P2;
  BMP280_S64_T dig_P3 = calib->dig_P3;
  BMP280_S64_T dig_P4 = calib->dig_P4;
  BMP280_S64_T dig_P5 = calib->dig_P5;
  BMP280_S64_T dig_P6 = calib->dig_P6;
  BMP280_S64_T dig_P7 = calib->dig_P7;
  BMP280_S64_T dig_P8 = calib->dig_P8;
  BMP280_S64_T dig_P9 = calib->dig_P9;

  BMP280_S64_T var_1, var_2, p;

  var_1 = ((BMP280_S64_T)t_fine) - 128000;
  var_2 = var_1 * var_1 * dig_P6;
  var_2 = var_2 + ((var_1 * dig_P5) << 17);
  var_2 = var_2 + (dig_P4 << 35);
  var_1 = ((var_1 * var_1 * dig_P3) >> 8) + ((var_1 * dig_P2) << 12);
  var_1 = (((((BMP280_S64_T)1) << 47) + var_1)) * dig_P1 >> 33;

  if (var_1 == 0)
  {
    return 0; // avoid exception caused by division by zero
  }

  p = 1048576 - adc_P;
  p = (((p << 31) - var_2) * 3125) / var_1;

  var_1 = (dig_P9 * (p >> 13) * (p >> 13)) >> 25;
  var_2 = (dig_P8 * p) >> 19;

  p = ((p + var_1 + var_2) >> 8) + (dig_P7 << 4);

  return (BMP280_U32_T)p;
}
