/**
 * @file vl53l0x_device_ext.h
 * @brief Extended VL53L0X device structure for ESP32
 */

#ifndef VL53L0X_DEVICE_EXT_H
#define VL53L0X_DEVICE_EXT_H

#include "vl53l0x_device.h"
#include "driver/i2c_master.h"

// Add I2C device handle to VL53L0X_Dev_t
// This is done by redefining the structure with the additional field
#define VL53L0X_Dev_t VL53L0X_Dev_t_Extended

typedef struct {
    VL53L0X_DevData_t Data;
    uint8_t   I2cDevAddr;
    uint8_t   comms_type;
    uint16_t  comms_speed_khz;
    i2c_master_dev_handle_t i2c_dev_handle;  // Added for ESP32
} VL53L0X_Dev_t_Extended;

#endif // VL53L0X_DEVICE_EXT_H
