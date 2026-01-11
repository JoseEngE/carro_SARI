/**
 * @file obstacle_detection.c
 * @brief Obstacle Detection Implementation
 */

#include "obstacle_detection.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "OBSTACLE_DET";

typedef struct {
    vl53l0x_handle_t sensor;
    obstacle_zone_config_t config;
    uint16_t last_distance;
    obstacle_event_t last_event;
} zone_state_t;

static zone_state_t zones[ZONE_MAX];
static size_t num_active_zones = 0;
static obstacle_callback_t global_callback = NULL;
static void* global_user_data = NULL;
static bool is_running = false;

static void sensor_callback(const vl53l0x_measurement_t* measurement, void* user_data) {
    obstacle_zone_t zone = (obstacle_zone_t)(uintptr_t)user_data;
    
    if (zone >= num_active_zones) return;
    
    zone_state_t* state = &zones[zone];
    state->last_distance = measurement->distance_mm;
    
    obstacle_event_t event = OBSTACLE_EVENT_CLEAR;
    
    if (!measurement->is_valid) {
        event = OBSTACLE_EVENT_ERROR;
    } else if (measurement->distance_mm <= state->config.critical_distance_mm) {
        event = OBSTACLE_EVENT_CRITICAL;
    } else if (measurement->distance_mm <= state->config.warning_distance_mm) {
        event = OBSTACLE_EVENT_WARNING;
    }
    
    if (event != state->last_event && global_callback) {
        global_callback(zone, measurement->distance_mm, event, global_user_data);
        state->last_event = event;
    }
}

esp_err_t obstacle_detection_init(const obstacle_zone_config_t* zone_configs, size_t num_zones) {
    if (!zone_configs || num_zones == 0 || num_zones > ZONE_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memset(zones, 0, sizeof(zones));
    num_active_zones = num_zones;
    
    for (size_t i = 0; i < num_zones; i++) {
        memcpy(&zones[i].config, &zone_configs[i], sizeof(obstacle_zone_config_t));
        
        if (!zone_configs[i].enabled) continue;
        
        vl53l0x_config_t sensor_config = {
            .scl_pin = zone_configs[i].scl_pin,
            .sda_pin = zone_configs[i].sda_pin,
            .i2c_freq_hz = 400000,
            .mode = zone_configs[i].mode,
            .i2c_address = 0x29 + i  // Different address per sensor
        };
        
        esp_err_t ret = vl53l0x_init(&sensor_config, &zones[i].sensor);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to init sensor for zone %d", zone_configs[i].zone);
            return ret;
        }
        
        ESP_LOGI(TAG, "Initialized zone %s", obstacle_detection_get_zone_name(zone_configs[i].zone));
    }
    
    return ESP_OK;
}

esp_err_t obstacle_detection_start(obstacle_callback_t callback, void* user_data) {
    if (!callback) {
        return ESP_ERR_INVALID_ARG;
    }
    
    global_callback = callback;
    global_user_data = user_data;
    is_running = true;
    
    for (size_t i = 0; i < num_active_zones; i++) {
        if (!zones[i].config.enabled) continue;
        
        vl53l0x_start_continuous(zones[i].sensor, sensor_callback, (void*)(uintptr_t)i);
    }
    
    ESP_LOGI(TAG, "Obstacle detection started");
    return ESP_OK;
}

esp_err_t obstacle_detection_stop(void) {
    is_running = false;
    
    for (size_t i = 0; i < num_active_zones; i++) {
        if (zones[i].sensor) {
            vl53l0x_stop_continuous(zones[i].sensor);
        }
    }
    
    return ESP_OK;
}

esp_err_t obstacle_detection_get_distance(obstacle_zone_t zone, uint16_t* distance_mm) {
    if (zone >= num_active_zones || !distance_mm) {
        return ESP_ERR_INVALID_ARG;
    }
    
    *distance_mm = zones[zone].last_distance;
    return ESP_OK;
}

bool obstacle_detection_is_path_clear(void) {
    for (size_t i = 0; i < num_active_zones; i++) {
        if (!zones[i].config.enabled) continue;
        if (zones[i].last_distance <= zones[i].config.warning_distance_mm) {
            return false;
        }
    }
    return true;
}

bool obstacle_detection_is_zone_clear(obstacle_zone_t zone) {
    if (zone >= num_active_zones) return false;
    return zones[zone].last_distance > zones[zone].config.warning_distance_mm;
}

const char* obstacle_detection_get_zone_name(obstacle_zone_t zone) {
    switch (zone) {
        case ZONE_FRONT: return "Front";
        case ZONE_FRONT_LEFT: return "Front-Left";
        case ZONE_FRONT_RIGHT: return "Front-Right";
        case ZONE_LEFT: return "Left";
        case ZONE_RIGHT: return "Right";
        case ZONE_REAR: return "Rear";
        default: return "Unknown";
    }
}

esp_err_t obstacle_detection_deinit(void) {
    obstacle_detection_stop();
    
    for (size_t i = 0; i < num_active_zones; i++) {
        if (zones[i].sensor) {
            vl53l0x_deinit(zones[i].sensor);
        }
    }
    
    num_active_zones = 0;
    return ESP_OK;
}
