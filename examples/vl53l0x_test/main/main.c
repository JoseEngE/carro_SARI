/**
 * @file main.c
 * @brief VL53L0X Distance Sensor Test Example
 * 
 * Simple example demonstrating VL53L0X sensor usage
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "vl53l0x_driver.h"

static const char *TAG = "MAIN";

void app_main(void) {
    ESP_LOGI(TAG, "VL53L0X Component Test");
    
    // Configure sensor
    vl53l0x_config_t config = {
        .scl_pin = GPIO_NUM_5,
        .sda_pin = GPIO_NUM_6,
        .i2c_freq_hz = 400000,
        .mode = VL53L0X_MODE_HIGH_ACCURACY,
        .i2c_address = 0x29
    };
    
    // Initialize sensor
    vl53l0x_handle_t sensor;
    esp_err_t ret = vl53l0x_init(&config, &sensor);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize sensor");
        return;
    }
    
    ESP_LOGI(TAG, "Sensor initialized successfully");
    
    // Read distance loop
    while (1) {
        vl53l0x_measurement_t measurement;
        ret = vl53l0x_read_single(sensor, &measurement);
        
        if (ret == ESP_OK && measurement.is_valid) {
            ESP_LOGI(TAG, "Distance: %d mm", measurement.distance_mm);
        } else {
            ESP_LOGW(TAG, "Measurement failed or invalid");
        }
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
