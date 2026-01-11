/**
 * @file vl53l0x_driver.h
 * @brief VL53L0X Distance Sensor Driver for ESP32
 * 
 * High-level API for VL53L0X Time-of-Flight distance sensor
 * Optimized for micromouse and autonomous vehicle applications
 */

#ifndef VL53L0X_DRIVER_H
#define VL53L0X_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief VL53L0X operation modes
 */
typedef enum {
    VL53L0X_MODE_DEFAULT,        /*!< Default mode (~30ms, ±3%) */
    VL53L0X_MODE_HIGH_ACCURACY,  /*!< High accuracy mode (~200ms, ±1%) */
    VL53L0X_MODE_HIGH_SPEED,     /*!< High speed mode (~20ms, ±3%) */
    VL53L0X_MODE_LONG_RANGE      /*!< Long range mode (~33ms, up to 2m) */
} vl53l0x_mode_t;

/**
 * @brief VL53L0X configuration structure
 */
typedef struct {
    gpio_num_t scl_pin;          /*!< I2C SCL pin */
    gpio_num_t sda_pin;          /*!< I2C SDA pin */
    uint32_t i2c_freq_hz;        /*!< I2C frequency in Hz (typically 400000) */
    vl53l0x_mode_t mode;         /*!< Operation mode */
    uint8_t i2c_address;         /*!< I2C address (default 0x29) */
} vl53l0x_config_t;

/**
 * @brief VL53L0X handle (opaque)
 */
typedef struct vl53l0x_handle_s* vl53l0x_handle_t;

/**
 * @brief Measurement data structure
 */
typedef struct {
    uint16_t distance_mm;        /*!< Distance in millimeters */
    uint8_t range_status;        /*!< Range status (0 = valid) */
    float signal_rate_mcps;      /*!< Signal rate in MCPS */
    float ambient_rate_mcps;     /*!< Ambient rate in MCPS */
    bool is_valid;               /*!< True if measurement is valid */
} vl53l0x_measurement_t;

/**
 * @brief Callback function for continuous measurements
 * 
 * @param measurement Pointer to measurement data
 * @param user_data User data passed during callback registration
 */
typedef void (*vl53l0x_measurement_cb_t)(const vl53l0x_measurement_t* measurement, void* user_data);

/**
 * @brief Initialize VL53L0X sensor
 * 
 * @param config Pointer to configuration structure
 * @param handle Pointer to store sensor handle
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t vl53l0x_init(const vl53l0x_config_t* config, vl53l0x_handle_t* handle);

/**
 * @brief Perform single distance measurement
 * 
 * @param handle Sensor handle
 * @param measurement Pointer to store measurement data
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t vl53l0x_read_single(vl53l0x_handle_t handle, vl53l0x_measurement_t* measurement);

/**
 * @brief Start continuous measurements with callback
 * 
 * @param handle Sensor handle
 * @param callback Callback function for measurements
 * @param user_data User data to pass to callback
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t vl53l0x_start_continuous(vl53l0x_handle_t handle, 
                                    vl53l0x_measurement_cb_t callback, 
                                    void* user_data);

/**
 * @brief Stop continuous measurements
 * 
 * @param handle Sensor handle
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t vl53l0x_stop_continuous(vl53l0x_handle_t handle);

/**
 * @brief Change operation mode
 * 
 * @param handle Sensor handle
 * @param mode New operation mode
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t vl53l0x_set_mode(vl53l0x_handle_t handle, vl53l0x_mode_t mode);

/**
 * @brief Get current distance (quick read)
 * 
 * @param handle Sensor handle
 * @param distance_mm Pointer to store distance in mm
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t vl53l0x_get_distance(vl53l0x_handle_t handle, uint16_t* distance_mm);

/**
 * @brief Check if sensor is ready
 * 
 * @param handle Sensor handle
 * @param is_ready Pointer to store ready status
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t vl53l0x_is_ready(vl53l0x_handle_t handle, bool* is_ready);

/**
 * @brief Deinitialize sensor and free resources
 * 
 * @param handle Sensor handle
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t vl53l0x_deinit(vl53l0x_handle_t handle);

/**
 * @brief Get mode name string
 * 
 * @param mode Operation mode
 * @return Mode name string
 */
const char* vl53l0x_get_mode_name(vl53l0x_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif // VL53L0X_DRIVER_H
