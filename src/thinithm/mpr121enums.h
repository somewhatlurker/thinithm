/*
 * enums for MPR121
 * 
 * Copyright 2019 somewhatlurker, MIT license
 */

#pragma once
#include <Arduino.h>

enum mpr121FilterFFI : byte {
  MPR_FFI_6 = 0,
  MPR_FFI_10 = 1,
  MPR_FFI_18 = 2,
  MPR_FFI_34 = 3,
};
enum mpr121FilterCDT : byte {
  MPR_CDT_0_5 = 1,
  MPR_CDT_1 = 2,
  MPR_CDT_2 = 3,
  MPR_CDT_4 = 4,
  MPR_CDT_8 = 5,
  MPR_CDT_16 = 6,
  MPR_CDT_32 = 7,
};
enum mpr121FilterSFI : byte {
  MPR_SFI_4 = 0,
  MPR_SFI_6 = 1,
  MPR_SFI_10 = 2,
  MPR_SFI_18 = 3,
};
enum mpr121FilterESI : byte {
  MPR_ESI_1 = 0,
  MPR_ESI_2 = 1,
  MPR_ESI_4 = 2,
  MPR_ESI_8 = 3,
  MPR_ESI_16 = 4,
  MPR_ESI_32 = 5,
  MPR_ESI_64 = 6,
  MPR_ESI_128 = 7,
};

enum mpr121ElectrodeConfigCL : byte {
  MPR_CL_TRACKING_ENABLED = 0,
  MPR_CL_TRACKING_DISABLED = 1,
  MPR_CL_TRACKING_ENABLED_LOAD5 = 2,
  MPR_CL_TRACKING_ENABLED_LOAD10 = 3,
};
enum mpr121ElectrodeConfigProx : byte {
  MPR_ELEPROX_DISABLED = 0,
  MPR_ELEPROX_0_TO_1 = 1,
  MPR_ELEPROX_0_TO_3 = 2,
  MPR_ELEPROX_0_TO_11 = 2,
};

enum mpr121AutoConfigRetry : byte {
  MPR_AUTOCONFIG_RETRY_DISABLED = 0,
  MPR_AUTOCONFIG_RETRY_2 = 1,
  MPR_AUTOCONFIG_RETRY_4 = 2,
  MPR_AUTOCONFIG_RETRY_8 = 2,
};
enum mpr121AutoConfigBVA : byte {
  MPR_AUTOCONFIG_BVA_DISABLED = 0,
  MPR_AUTOCONFIG_BVA_CLEAR = 1,
  MPR_AUTOCONFIG_BVA_SET_CLEAR3 = 2,
  MPR_AUTOCONFIG_BVA_SET_ALL = 2,
};

enum mpr121Register : byte {
  MPRREG_ELE0_TO_ELE7_TOUCH_STATUS = 0x00,
  MPRREG_ELE8_TO_ELEPROX_TOUCH_STATUS = 0x01,
  MPRREG_ELE0_TO_ELE7_OOR_STATUS = 0x02,
  MPRREG_ELE8_TO_ELEPROX_OOR_STATUS = 0x03,
  
  MPRREG_ELE0_FILTERED_DATA_LSB = 0x04,
  MPRREG_ELE0_FILTERED_DATA_MSB = 0x05,
  MPRREG_ELE1_FILTERED_DATA_LSB = 0x06,
  MPRREG_ELE1_FILTERED_DATA_MSB = 0x07,
  MPRREG_ELE2_FILTERED_DATA_LSB = 0x08,
  MPRREG_ELE2_FILTERED_DATA_MSB = 0x09,
  MPRREG_ELE3_FILTERED_DATA_LSB = 0x0a,
  MPRREG_ELE3_FILTERED_DATA_MSB = 0x0b,
  MPRREG_ELE4_FILTERED_DATA_LSB = 0x0c,
  MPRREG_ELE4_FILTERED_DATA_MSB = 0x0d,
  MPRREG_ELE5_FILTERED_DATA_LSB = 0x0e,
  MPRREG_ELE5_FILTERED_DATA_MSB = 0x0f,
  MPRREG_ELE6_FILTERED_DATA_LSB = 0x10,
  MPRREG_ELE6_FILTERED_DATA_MSB = 0x11,
  MPRREG_ELE7_FILTERED_DATA_LSB = 0x12,
  MPRREG_ELE7_FILTERED_DATA_MSB = 0x13,
  MPRREG_ELE8_FILTERED_DATA_LSB = 0x14,
  MPRREG_ELE8_FILTERED_DATA_MSB = 0x15,
  MPRREG_ELE9_FILTERED_DATA_LSB = 0x16,
  MPRREG_ELE9_FILTERED_DATA_MSB = 0x17,
  MPRREG_ELE10_FILTERED_DATA_LSB = 0x18,
  MPRREG_ELE10_FILTERED_DATA_MSB = 0x19,
  MPRREG_ELE11_FILTERED_DATA_LSB = 0x1a,
  MPRREG_ELE11_FILTERED_DATA_MSB = 0x1b,
  MPRREG_ELEPROX_FILTERED_DATA_LSB = 0x1c,
  MPRREG_ELEPROX_FILTERED_DATA_MSB = 0x1d,

  MPRREG_ELE0_BASELINE = 0x1e,
  MPRREG_ELE1_BASELINE = 0x1f,
  MPRREG_ELE2_BASELINE = 0x20,
  MPRREG_ELE3_BASELINE = 0x21,
  MPRREG_ELE4_BASELINE = 0x22,
  MPRREG_ELE5_BASELINE = 0x23,
  MPRREG_ELE6_BASELINE = 0x24,
  MPRREG_ELE7_BASELINE = 0x25,
  MPRREG_ELE8_BASELINE = 0x26,
  MPRREG_ELE9_BASELINE = 0x27,
  MPRREG_ELE10_BASELINE = 0x28,
  MPRREG_ELE11_BASELINE = 0x29,
  MPRREG_ELEPROX_BASELINE = 0x2a,
  
  MPRREG_MHD_RISING = 0x2b,
  MPRREG_NHD_AMOUNT_RISING = 0x2c,
  MPRREG_NCL_RISING = 0x2d,
  MPRREG_FDL_RISING = 0x2e,
  MPRREG_MHD_FALLING = 0x2f,
  MPRREG_NHD_AMOUNT_FALLING = 0x30,
  MPRREG_NCL_FALLING = 0x31,
  MPRREG_FDL_FALLING = 0x32,
  MPRREG_NHD_AMOUNT_TOUCHED = 0x33,
  MPRREG_NCL_TOUCHED = 0x34,
  MPRREG_FDL_TOUCHED = 0x35,

  MPRREG_ELEPROX_MHD_RISING = 0x36,
  MPRREG_ELEPROX_NHD_AMOUNT_RISING = 0x37,
  MPRREG_ELEPROX_NCL_RISING = 0x38,
  MPRREG_ELEPROX_FDL_RISING = 0x39,
  MPRREG_ELEPROX_MHD_FALLING = 0x3a,
  MPRREG_ELEPROX_NHD_AMOUNT_FALLING = 0x3b,
  MPRREG_ELEPROX_NCL_FALLING = 0x3c,
  MPRREG_ELEPROX_FDL_FALLING = 0x3d,
  MPRREG_ELEPROX_NHD_AMOUNT_TOUCHED = 0x3e,
  MPRREG_ELEPROX_NCL_TOUCHED = 0x3f,
  MPRREG_ELEPROX_FDL_TOUCHED = 0x40,
  
  MPRREG_ELE0_TOUCH_THRESHOLD = 0x41,
  MPRREG_ELE0_RELEASE_THRESHOLD = 0x42,
  MPRREG_ELE1_TOUCH_THRESHOLD = 0x43,
  MPRREG_ELE1_RELEASE_THRESHOLD = 0x44,
  MPRREG_ELE2_TOUCH_THRESHOLD = 0x45,
  MPRREG_ELE2_RELEASE_THRESHOLD = 0x46,
  MPRREG_ELE3_TOUCH_THRESHOLD = 0x47,
  MPRREG_ELE3_RELEASE_THRESHOLD = 0x48,
  MPRREG_ELE4_TOUCH_THRESHOLD = 0x49,
  MPRREG_ELE4_RELEASE_THRESHOLD = 0x4a,
  MPRREG_ELE5_TOUCH_THRESHOLD = 0x4b,
  MPRREG_ELE5_RELEASE_THRESHOLD = 0x4c,
  MPRREG_ELE6_TOUCH_THRESHOLD = 0x4d,
  MPRREG_ELE6_RELEASE_THRESHOLD = 0x4e,
  MPRREG_ELE7_TOUCH_THRESHOLD = 0x4f,
  MPRREG_ELE7_RELEASE_THRESHOLD = 0x50,
  MPRREG_ELE8_TOUCH_THRESHOLD = 0x51,
  MPRREG_ELE8_RELEASE_THRESHOLD = 0x52,
  MPRREG_ELE9_TOUCH_THRESHOLD = 0x53,
  MPRREG_ELE9_RELEASE_THRESHOLD = 0x54,
  MPRREG_ELE10_TOUCH_THRESHOLD = 0x55,
  MPRREG_ELE10_RELEASE_THRESHOLD = 0x56,
  MPRREG_ELE11_TOUCH_THRESHOLD = 0x57,
  MPRREG_ELE11_RELEASE_THRESHOLD = 0x58,
  MPRREG_ELEPROX_TOUCH_THRESHOLD = 0x59,
  MPRREG_ELEPROX_RELEASE_THRESHOLD = 0x5a,
  
  MPRREG_DEBOUNCE = 0x5b,
  
  MPRREG_FILTER_GLOBAL_CDC_CONFIG = 0x5c,
  MPRREG_AFE_CONFIG = 0x5c, // alternate name
  MPRREG_FILTER_GLOBAL_CDT_CONFIG = 0x5d,
  MPRREG_FILTER_CONFIG = 0x5d, // alternate name
  
  MPRREG_ELECTRODE_CONFIG = 0x5e,
  
  MPRREG_ELE0_CDC = 0x5f,
  MPRREG_ELE1_CDC = 0x60,
  MPRREG_ELE2_CDC = 0x61,
  MPRREG_ELE3_CDC = 0x62,
  MPRREG_ELE4_CDC = 0x63,
  MPRREG_ELE5_CDC = 0x64,
  MPRREG_ELE6_CDC = 0x65,
  MPRREG_ELE7_CDC = 0x66,
  MPRREG_ELE8_CDC = 0x67,
  MPRREG_ELE9_CDC = 0x68,
  MPRREG_ELE10_CDC = 0x69,
  MPRREG_ELE11_CDC = 0x6a,
  MPRREG_ELEPROX_CDC = 0x6b,
  
  MPRREG_ELE0_ELE1_CDT = 0x6c,
  MPRREG_ELE2_ELE3_CDT = 0x6d,
  MPRREG_ELE4_ELE5_CDT = 0x6e,
  MPRREG_ELE6_ELE7_CDT = 0x6f,
  MPRREG_ELE8_ELE9_CDT = 0x70,
  MPRREG_ELE10_ELE11_CDT = 0x71,
  MPRREG_ELEPROX_CDT = 0x72,
  
  MPRREG_GPIO_CONTROL_0 = 0x73,
  MPRREG_GPIO_CONTROL_1 = 0x74,
  MPRREG_GPIO_DATA = 0x75,
  MPRREG_GPIO_DIRECTION = 0x76,
  MPRREG_GPIO_ENABLE = 0x77,
  MPRREG_GPIO_DATA_SET = 0x78,
  MPRREG_GPIO_DATA_CLEAR = 0x79,
  MPRREG_GPIO_DATA_TOGGLE = 0x7a,
  
  MPRREG_AUTOCONFIG_CONTROL_0 = 0x7b,
  MPRREG_AUTOCONFIG_CONTROL_1 = 0x7c,
  MPRREG_AUTOCONFIG_USL = 0x7d,
  MPRREG_AUTOCONFIG_LSL = 0x7e,
  MPRREG_AUTOCONFIG_TL = 0x7f,
  
  MPRREG_SOFT_RESET = 0x80,
};