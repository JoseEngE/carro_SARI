/**
 * @file motor_control.c
 * @brief Motor Control Implementation for RC Car
 */

#include "motor_control.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "MOTOR_CTRL";

#define PWM_RESOLUTION    LEDC_TIMER_8_BIT
#define PWM_MAX_DUTY      ((1 << 8) - 1)  // 255 for 8-bit

typedef struct {
    motor_config_t config;
    bool initialized;
} motor_state_t;

static motor_state_t drive_motor;
static motor_state_t steering_motor;
static SemaphoreHandle_t motor_mutex = NULL;

/**
 * @brief Configure PWM for a motor channel
 */
static esp_err_t configure_pwm_channel(const motor_config_t* config, ledc_channel_t channel, gpio_num_t gpio) {
    ledc_channel_config_t ledc_channel = {
        .channel    = channel,
        .duty       = 0,
        .gpio_num   = gpio,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .hpoint     = 0,
        .timer_sel  = config->timer
    };
    return ledc_channel_config(&ledc_channel);
}

/**
 * @brief Initialize a motor
 */
static esp_err_t init_motor(motor_state_t* motor, const motor_config_t* config) {
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Copy configuration
    memcpy(&motor->config, config, sizeof(motor_config_t));
    
    // Configure timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = config->timer,
        .duty_resolution  = PWM_RESOLUTION,
        .freq_hz          = config->pwm_freq_hz,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    
    esp_err_t ret = ledc_timer_config(&ledc_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC timer: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Configure channels
    ret = configure_pwm_channel(config, config->channel_a, config->in1_pin);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure channel A: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = configure_pwm_channel(config, config->channel_b, config->in2_pin);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure channel B: %s", esp_err_to_name(ret));
        return ret;
    }
    
    motor->initialized = true;
    return ESP_OK;
}

/**
 * @brief Set motor PWM duty cycles
 */
static esp_err_t set_motor_duty(motor_state_t* motor, uint32_t duty_a, uint32_t duty_b) {
    if (!motor->initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    esp_err_t ret = ledc_set_duty(LEDC_LOW_SPEED_MODE, motor->config.channel_a, duty_a);
    if (ret == ESP_OK) {
        ret = ledc_update_duty(LEDC_LOW_SPEED_MODE, motor->config.channel_a);
    }
    
    if (ret == ESP_OK) {
        ret = ledc_set_duty(LEDC_LOW_SPEED_MODE, motor->config.channel_b, duty_b);
    }
    
    if (ret == ESP_OK) {
        ret = ledc_update_duty(LEDC_LOW_SPEED_MODE, motor->config.channel_b);
    }
    
    return ret;
}

esp_err_t motor_control_init(const motor_config_t* drive_config, 
                              const motor_config_t* steering_config) {
    if (!drive_config || !steering_config) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Create mutex
    motor_mutex = xSemaphoreCreateMutex();
    if (!motor_mutex) {
        return ESP_ERR_NO_MEM;
    }
    
    // Initialize drive motor
    esp_err_t ret = init_motor(&drive_motor, drive_config);
    if (ret != ESP_OK) {
        vSemaphoreDelete(motor_mutex);
        return ret;
    }
    
    // Initialize steering motor
    ret = init_motor(&steering_motor, steering_config);
    if (ret != ESP_OK) {
        vSemaphoreDelete(motor_mutex);
        return ret;
    }
    
    ESP_LOGI(TAG, "Motor control initialized");
    ESP_LOGI(TAG, "Drive motor: IN1=GPIO%d, IN2=GPIO%d", drive_config->in1_pin, drive_config->in2_pin);
    ESP_LOGI(TAG, "Steering motor: IN1=GPIO%d, IN2=GPIO%d", steering_config->in1_pin, steering_config->in2_pin);
    
    return ESP_OK;
}

esp_err_t motor_drive_set_speed(int8_t speed) {
    if (!motor_mutex) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Clamp speed
    if (speed > 100) speed = 100;
    if (speed < -100) speed = -100;
    
    xSemaphoreTake(motor_mutex, portMAX_DELAY);
    
    uint32_t duty_a = 0, duty_b = 0;
    
    if (speed > 0) {
        // Forward
        duty_a = (speed * PWM_MAX_DUTY) / 100;
        duty_b = 0;
    } else if (speed < 0) {
        // Backward
        duty_a = 0;
        duty_b = ((-speed) * PWM_MAX_DUTY) / 100;
    }
    // else: speed == 0, both duties remain 0 (stop)
    
    esp_err_t ret = set_motor_duty(&drive_motor, duty_a, duty_b);
    
    xSemaphoreGive(motor_mutex);
    
    return ret;
}

esp_err_t motor_drive_forward(uint8_t speed) {
    if (speed > 100) speed = 100;
    return motor_drive_set_speed(speed);
}

esp_err_t motor_drive_backward(uint8_t speed) {
    if (speed > 100) speed = 100;
    return motor_drive_set_speed(-speed);
}

esp_err_t motor_drive_stop(void) {
    return motor_drive_set_speed(0);
}

esp_err_t motor_steering_set_angle(int8_t angle) {
    if (!motor_mutex) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Clamp angle
    if (angle > 100) angle = 100;
    if (angle < -100) angle = -100;
    
    xSemaphoreTake(motor_mutex, portMAX_DELAY);
    
    uint32_t duty_a = 0, duty_b = 0;
    
    if (angle < 0) {
        // Turn left
        duty_a = 0;
        duty_b = ((-angle) * PWM_MAX_DUTY) / 100;
    } else if (angle > 0) {
        // Turn right
        duty_a = (angle * PWM_MAX_DUTY) / 100;
        duty_b = 0;
    }
    // else: angle == 0, both duties remain 0 (center)
    
    esp_err_t ret = set_motor_duty(&steering_motor, duty_a, duty_b);
    
    xSemaphoreGive(motor_mutex);
    
    return ret;
}

esp_err_t motor_steering_set_position(steering_position_t position) {
    int8_t angle = 0;
    
    switch (position) {
        case STEER_LEFT:
            angle = -80;  // 80% left
            break;
        case STEER_RIGHT:
            angle = 80;   // 80% right
            break;
        case STEER_CENTER:
        default:
            angle = 0;
            break;
    }
    
    return motor_steering_set_angle(angle);
}

esp_err_t motor_steering_center(void) {
    return motor_steering_set_angle(0);
}

esp_err_t motor_move_forward(uint8_t speed, int8_t steering_angle) {
    esp_err_t ret = motor_drive_forward(speed);
    if (ret == ESP_OK) {
        ret = motor_steering_set_angle(steering_angle);
    }
    return ret;
}

esp_err_t motor_move_backward(uint8_t speed, int8_t steering_angle) {
    esp_err_t ret = motor_drive_backward(speed);
    if (ret == ESP_OK) {
        ret = motor_steering_set_angle(steering_angle);
    }
    return ret;
}

esp_err_t motor_turn_left(uint8_t speed) {
    return motor_move_forward(speed, -80);
}

esp_err_t motor_turn_right(uint8_t speed) {
    return motor_move_forward(speed, 80);
}

esp_err_t motor_stop_all(void) {
    esp_err_t ret = motor_drive_stop();
    if (ret == ESP_OK) {
        ret = motor_steering_center();
    }
    return ret;
}

esp_err_t motor_control_deinit(void) {
    if (motor_mutex) {
        motor_stop_all();
        vSemaphoreDelete(motor_mutex);
        motor_mutex = NULL;
    }
    
    drive_motor.initialized = false;
    steering_motor.initialized = false;
    
    return ESP_OK;
}
