/**
 * @file obstacle_detection.h
 * @brief Obstacle Detection System for Micromouse/Autonomous Vehicles
 * 
 * High-level API for managing multiple VL53L0X sensors
 * and detecting obstacles in different zones
 */

#ifndef OBSTACLE_DETECTION_H
#define OBSTACLE_DETECTION_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "vl53l0x_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Vehicle zones for obstacle detection
 */
typedef enum {
    ZONE_FRONT,          /*!< Front zone */
    ZONE_FRONT_LEFT,     /*!< Front-left zone */
    ZONE_FRONT_RIGHT,    /*!< Front-right zone */
    ZONE_LEFT,           /*!< Left zone */
    ZONE_RIGHT,          /*!< Right zone */
    ZONE_REAR,           /*!< Rear zone */
    ZONE_MAX             /*!< Maximum number of zones */
} obstacle_zone_t;

/**
 * @brief Zone configuration structure
 */
typedef struct {
    obstacle_zone_t zone;        /*!< Zone identifier */
    gpio_num_t scl_pin;          /*!< I2C SCL pin for this zone */
    gpio_num_t sda_pin;          /*!< I2C SDA pin for this zone */
    uint16_t warning_distance_mm; /*!< Warning distance threshold */
    uint16_t critical_distance_mm;/*!< Critical distance threshold */
    vl53l0x_mode_t mode;         /*!< Sensor mode for this zone */
    bool enabled;                /*!< Enable/disable this zone */
} obstacle_zone_config_t;

/**
 * @brief Obstacle event type
 */
typedef enum {
    OBSTACLE_EVENT_CLEAR,        /*!< Path is clear */
    OBSTACLE_EVENT_WARNING,      /*!< Obstacle in warning range */
    OBSTACLE_EVENT_CRITICAL,     /*!< Obstacle in critical range */
    OBSTACLE_EVENT_ERROR         /*!< Sensor error */
} obstacle_event_t;

/**
 * @brief Obstacle callback function
 * 
 * @param zone Zone where obstacle was detected
 * @param distance_mm Distance to obstacle in mm
 * @param event Event type
 * @param user_data User data
 */
typedef void (*obstacle_callback_t)(obstacle_zone_t zone, uint16_t distance_mm, 
                                     obstacle_event_t event, void* user_data);

/**
 * @brief Initialize obstacle detection system
 * 
 * @param zones Array of zone configurations
 * @param num_zones Number of zones
 * @return ESP_OK on success
 */
esp_err_t obstacle_detection_init(const obstacle_zone_config_t* zones, size_t num_zones);

/**
 * @brief Start obstacle detection
 * 
 * @param callback Callback function for obstacle events
 * @param user_data User data to pass to callback
 * @return ESP_OK on success
 */
esp_err_t obstacle_detection_start(obstacle_callback_t callback, void* user_data);

/**
 * @brief Stop obstacle detection
 * 
 * @return ESP_OK on success
 */
esp_err_t obstacle_detection_stop(void);

/**
 * @brief Get distance for specific zone
 * 
 * @param zone Zone to query
 * @param distance_mm Pointer to store distance
 * @return ESP_OK on success
 */
esp_err_t obstacle_detection_get_distance(obstacle_zone_t zone, uint16_t* distance_mm);

/**
 * @brief Check if path is clear (all zones)
 * 
 * @return true if all zones are clear
 */
bool obstacle_detection_is_path_clear(void);

/**
 * @brief Check if specific zone is clear
 * 
 * @param zone Zone to check
 * @return true if zone is clear
 */
bool obstacle_detection_is_zone_clear(obstacle_zone_t zone);

/**
 * @brief Get zone name string
 * 
 * @param zone Zone identifier
 * @return Zone name string
 */
const char* obstacle_detection_get_zone_name(obstacle_zone_t zone);

/**
 * @brief Deinitialize obstacle detection system
 * 
 * @return ESP_OK on success
 */
esp_err_t obstacle_detection_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // OBSTACLE_DETECTION_H
