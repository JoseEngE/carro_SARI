/**
 * @file main.c
 * @brief RC Car Web Control - Main Application
 * 
 * Web-based control for RC car with mobile interface
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "motor_control.h"
#include "web_control.h"
#include "servo_control.h"

static const char *TAG = "MAIN";

/**
 * @brief Motor control callback - called when web commands are received
 */
float map_range(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void motor_callback(int8_t throttle, int8_t steering)
{
    ESP_LOGI(TAG, "Motor command: throttle=%d, steering=%d", throttle, steering);
    
    // Apply throttle (forward/backward)
    if (throttle > 5) {
        motor_drive_forward(throttle);
    } else if (throttle < -5) {
        motor_drive_backward(-throttle);
    } else {
        motor_drive_stop();
    }
    
    // Apply steering (servo)
    // Map -100 to 100 range to 75-41 degrees
    float angle = map_range((float)steering, -100.0f, 100.0f, 75.0f, 41.0f);
    servo_set_angle(angle);
}

void app_main(void) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║    RC Car Web Control System           ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    
    // Configure drive motor (rear)
    motor_config_t drive_config = {
        .in1_pin = GPIO_NUM_7,
        .in2_pin = GPIO_NUM_8,
        .pwm_freq_hz = 1000,
        .timer = LEDC_TIMER_0,
        .channel_a = LEDC_CHANNEL_0,
        .channel_b = LEDC_CHANNEL_1
    };
    
    // Initialize drive motor
    ESP_LOGI(TAG, "Initializing drive motor...");
    esp_err_t ret = motor_control_init(&drive_config, NULL); // Pass NULL for steering config
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize drive motor!");
        return;
    }
    ESP_LOGI(TAG, "✓ Drive motor initialized");

    // Initialize Servo
    // Using GPIO 9 (original steering pin) and parameters from servomotor example
    ESP_LOGI(TAG, "Initializing servo...");
    servo_config_t servo_cfg = {
        .gpio_num = GPIO_NUM_9,
        .timer_number = LEDC_TIMER_1, // Different timer than drive motor (Timer 0)
        .channel_number = LEDC_CHANNEL_2,
        .min_pulse_width_us = 500,
        .max_pulse_width_us = 2400,
        .frequency = 50,
        .min_angle = 41.0f,
        .max_angle = 75.0f,
        .initial_angle = 58.0f
    };
    ret = servo_init(&servo_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize servo!");
        return;
    }
    ESP_LOGI(TAG, "✓ Servo initialized");
    
    // Initialize web control
    ESP_LOGI(TAG, "Initializing web control...");
    web_control_config_t web_config = WEB_CONTROL_DEFAULT_CONFIG();
    ret = web_control_init(&web_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize web control!");
        return;
    }
    ESP_LOGI(TAG, "✓ Web control initialized");
    
    // Set motor callback
    web_control_set_motor_callback(motor_callback);
    
    // Start web server
    ESP_LOGI(TAG, "Starting web server...");
    ret = web_control_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start web server!");
        return;
    }
    
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  Web Control Ready!                    ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "📱 Connect your phone to WiFi:");
    ESP_LOGI(TAG, "   SSID: RC_Car_Control");
    ESP_LOGI(TAG, "   Password: rccar123");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "🌐 Open browser and go to:");
    ESP_LOGI(TAG, "   http://192.168.4.1");
    ESP_LOGI(TAG, "");
    
    // Telemetry task - send data to web interface
    while (1) {
        if (web_control_is_connected()) {
            // Send telemetry (battery, speed, signal)
            // For now, using dummy values
            web_control_send_telemetry(87, 0.0, 100);
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Update every 100ms
    }
}
