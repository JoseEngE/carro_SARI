/**
 * @file app_config.h
 * @brief Application Configuration for Micromouse/Autonomous Car
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "driver/gpio.h"
#include "vl53l0x_driver.h"

// ============================================================================
// HARDWARE CONFIGURATION
// ============================================================================

// Front sensor (high accuracy for precise navigation)
#define GPIO_SCL_FRONT    GPIO_NUM_5
#define GPIO_SDA_FRONT    GPIO_NUM_6

// Left sensor (for wall following)
#define GPIO_SCL_LEFT     GPIO_NUM_7
#define GPIO_SDA_LEFT     GPIO_NUM_8

// Right sensor (for wall following)
#define GPIO_SCL_RIGHT    GPIO_NUM_9
#define GPIO_SDA_RIGHT    GPIO_NUM_10

// ============================================================================
// NAVIGATION PARAMETERS
// ============================================================================

// Distance thresholds (in millimeters)
#define SAFE_DISTANCE_MM       200  // Safe operating distance
#define WARNING_DISTANCE_MM    100  // Warning threshold
#define CRITICAL_DISTANCE_MM    50  // Critical - must stop
#define WALL_FOLLOW_DISTANCE    80  // Target distance for wall following

// Sensor modes
#define FRONT_SENSOR_MODE   VL53L0X_MODE_HIGH_ACCURACY  // Front needs precision
#define SIDE_SENSOR_MODE    VL53L0X_MODE_DEFAULT        // Sides can be faster

// ============================================================================
// SYSTEM CONFIGURATION
// ============================================================================

#define I2C_FREQUENCY_HZ    400000  // 400 kHz I2C

#endif // APP_CONFIG_H
