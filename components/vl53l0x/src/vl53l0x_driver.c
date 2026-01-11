/**
 * @file vl53l0x_driver.c
 * @brief VL53L0X Driver Implementation
 */

#include "vl53l0x_driver.h"
#include "vl53l0x_api.h"
#include "vl53l0x_platform.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/i2c_master.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "VL53L0X_DRV";

// I2C bus handle (one per I2C bus)
static i2c_master_bus_handle_t i2c_bus_handle = NULL;
static bool i2c_bus_initialized = false;

// Current device handle for platform layer access
i2c_master_dev_handle_t g_current_vl53l0x_dev_handle = NULL;

/**
 * @brief Internal handle structure
 */
struct vl53l0x_handle_s {
    VL53L0X_Dev_t device;
    i2c_master_dev_handle_t i2c_dev_handle;  // I2C device handle
    vl53l0x_config_t config;
    vl53l0x_measurement_cb_t callback;
    void* user_data;
    TaskHandle_t task_handle;
    SemaphoreHandle_t mutex;
    bool is_continuous;
    bool is_initialized;
};

/**
 * @brief Configure sensor mode
 */
static esp_err_t configure_mode(vl53l0x_handle_t handle, vl53l0x_mode_t mode) {
    VL53L0X_Error status = VL53L0X_ERROR_NONE;
    
    switch (mode) {
        case VL53L0X_MODE_HIGH_ACCURACY:
            status = VL53L0X_SetLimitCheckValue(&handle->device,
                    VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE,
                    (FixPoint1616_t)(0.25 * 65536));
            if (status == VL53L0X_ERROR_NONE) {
                status = VL53L0X_SetLimitCheckValue(&handle->device,
                        VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE,
                        (FixPoint1616_t)(18 * 65536));
            }
            if (status == VL53L0X_ERROR_NONE) {
                status = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(&handle->device, 200000);
            }
            break;
            
        case VL53L0X_MODE_HIGH_SPEED:
            status = VL53L0X_SetLimitCheckValue(&handle->device,
                    VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE,
                    (FixPoint1616_t)(0.25 * 65536));
            if (status == VL53L0X_ERROR_NONE) {
                status = VL53L0X_SetLimitCheckValue(&handle->device,
                        VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE,
                        (FixPoint1616_t)(32 * 65536));
            }
            if (status == VL53L0X_ERROR_NONE) {
                status = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(&handle->device, 20000);
            }
            break;
            
        case VL53L0X_MODE_LONG_RANGE:
            status = VL53L0X_SetLimitCheckValue(&handle->device,
                    VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE,
                    (FixPoint1616_t)(0.1 * 65536));
            if (status == VL53L0X_ERROR_NONE) {
                status = VL53L0X_SetLimitCheckValue(&handle->device,
                        VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE,
                        (FixPoint1616_t)(60 * 65536));
            }
            if (status == VL53L0X_ERROR_NONE) {
                status = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(&handle->device, 33000);
            }
            if (status == VL53L0X_ERROR_NONE) {
                status = VL53L0X_SetVcselPulsePeriod(&handle->device,
                        VL53L0X_VCSEL_PERIOD_PRE_RANGE, 18);
            }
            if (status == VL53L0X_ERROR_NONE) {
                status = VL53L0X_SetVcselPulsePeriod(&handle->device,
                        VL53L0X_VCSEL_PERIOD_FINAL_RANGE, 14);
            }
            break;
            
        case VL53L0X_MODE_DEFAULT:
        default:
            // Default mode - no special configuration
            break;
    }
    
    return (status == VL53L0X_ERROR_NONE) ? ESP_OK : ESP_FAIL;
}

/**
 * @brief Continuous measurement task
 */
static void continuous_task(void* arg) {
    vl53l0x_handle_t handle = (vl53l0x_handle_t)arg;
    VL53L0X_RangingMeasurementData_t measurement_data;
    vl53l0x_measurement_t measurement;
    
    while (handle->is_continuous) {
        VL53L0X_Error status = VL53L0X_PerformSingleRangingMeasurement(&handle->device, &measurement_data);
        
        if (status == VL53L0X_ERROR_NONE) {
            measurement.distance_mm = measurement_data.RangeMilliMeter;
            measurement.range_status = measurement_data.RangeStatus;
            measurement.signal_rate_mcps = measurement_data.SignalRateRtnMegaCps / 65536.0f;
            measurement.ambient_rate_mcps = measurement_data.AmbientRateRtnMegaCps / 65536.0f;
            measurement.is_valid = (measurement_data.RangeStatus == 0);
            
            if (handle->callback) {
                handle->callback(&measurement, handle->user_data);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(50)); // Adjust based on mode
    }
    
    vTaskDelete(NULL);
}

esp_err_t vl53l0x_init(const vl53l0x_config_t* config, vl53l0x_handle_t* handle) {
    if (!config || !handle) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Allocate handle
    *handle = (vl53l0x_handle_t)calloc(1, sizeof(struct vl53l0x_handle_s));
    if (!*handle) {
        return ESP_ERR_NO_MEM;
    }
    
    // Copy configuration
    memcpy(&(*handle)->config, config, sizeof(vl53l0x_config_t));
    
    // Create mutex
    (*handle)->mutex = xSemaphoreCreateMutex();
    if (!(*handle)->mutex) {
        free(*handle);
        return ESP_ERR_NO_MEM;
    }
    
    // Initialize I2C bus if not already done
    if (!i2c_bus_initialized) {
        i2c_master_bus_config_t bus_config = {
            .i2c_port = I2C_NUM_0,
            .sda_io_num = config->sda_pin,
            .scl_io_num = config->scl_pin,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .flags.enable_internal_pullup = true,
        };
        
        esp_err_t ret = i2c_new_master_bus(&bus_config, &i2c_bus_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize I2C bus: %s", esp_err_to_name(ret));
            vSemaphoreDelete((*handle)->mutex);
            free(*handle);
            return ret;
        }
        i2c_bus_initialized = true;
        ESP_LOGI(TAG, "I2C bus initialized (SDA: GPIO%d, SCL: GPIO%d)", config->sda_pin, config->scl_pin);
    }
    
    // Add device to I2C bus
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = config->i2c_address,
        .scl_speed_hz = config->i2c_freq_hz,
    };
    
    // Store I2C device handle in driver structure (not in ST structure)
    esp_err_t ret = i2c_master_bus_add_device(i2c_bus_handle, &dev_config, &(*handle)->i2c_dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add I2C device: %s", esp_err_to_name(ret));
        vSemaphoreDelete((*handle)->mutex);
        free(*handle);
        return ret;
    }
    
    // Configure device structure
    (*handle)->device.I2cDevAddr = config->i2c_address;
    (*handle)->device.comms_type = 1;
    (*handle)->device.comms_speed_khz = config->i2c_freq_hz / 1000;
    
    // Set global handle for platform layer
    g_current_vl53l0x_dev_handle = (*handle)->i2c_dev_handle;
    
    // Initialize sensor
    VL53L0X_Error status = VL53L0X_DataInit(&(*handle)->device);
    if (status != VL53L0X_ERROR_NONE) {
        ESP_LOGE(TAG, "DataInit failed: %d", status);
        vSemaphoreDelete((*handle)->mutex);
        free(*handle);
        return ESP_FAIL;
    }
    
    // Calibration
    uint32_t refSpadCount;
    uint8_t isApertureSpads, VhvSettings, PhaseCal;
    
    status = VL53L0X_StaticInit(&(*handle)->device);
    if (status == VL53L0X_ERROR_NONE) {
        status = VL53L0X_PerformRefCalibration(&(*handle)->device, &VhvSettings, &PhaseCal);
    }
    if (status == VL53L0X_ERROR_NONE) {
        status = VL53L0X_PerformRefSpadManagement(&(*handle)->device, &refSpadCount, &isApertureSpads);
    }
    if (status == VL53L0X_ERROR_NONE) {
        status = VL53L0X_SetDeviceMode(&(*handle)->device, VL53L0X_DEVICEMODE_SINGLE_RANGING);
    }
    
    // Configure mode
    if (status == VL53L0X_ERROR_NONE) {
        configure_mode(*handle, config->mode);
    }
    
    if (status != VL53L0X_ERROR_NONE) {
        ESP_LOGE(TAG, "Initialization failed: %d", status);
        vSemaphoreDelete((*handle)->mutex);
        free(*handle);
        return ESP_FAIL;
    }
    
    (*handle)->is_initialized = true;
    ESP_LOGI(TAG, "VL53L0X initialized successfully (mode: %s)", vl53l0x_get_mode_name(config->mode));
    
    return ESP_OK;
}

esp_err_t vl53l0x_read_single(vl53l0x_handle_t handle, vl53l0x_measurement_t* measurement) {
    if (!handle || !handle->is_initialized || !measurement) {
        return ESP_ERR_INVALID_ARG;
    }
    
    xSemaphoreTake(handle->mutex, portMAX_DELAY);
    
    VL53L0X_RangingMeasurementData_t data;
    VL53L0X_Error status = VL53L0X_PerformSingleRangingMeasurement(&handle->device, &data);
    
    if (status == VL53L0X_ERROR_NONE) {
        measurement->distance_mm = data.RangeMilliMeter;
        measurement->range_status = data.RangeStatus;
        measurement->signal_rate_mcps = data.SignalRateRtnMegaCps / 65536.0f;
        measurement->ambient_rate_mcps = data.AmbientRateRtnMegaCps / 65536.0f;
        measurement->is_valid = (data.RangeStatus == 0);
    }
    
    xSemaphoreGive(handle->mutex);
    
    return (status == VL53L0X_ERROR_NONE) ? ESP_OK : ESP_FAIL;
}

esp_err_t vl53l0x_start_continuous(vl53l0x_handle_t handle, vl53l0x_measurement_cb_t callback, void* user_data) {
    if (!handle || !handle->is_initialized || !callback) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (handle->is_continuous) {
        return ESP_ERR_INVALID_STATE;
    }
    
    handle->callback = callback;
    handle->user_data = user_data;
    handle->is_continuous = true;
    
    BaseType_t ret = xTaskCreate(continuous_task, "vl53l0x_cont", 4096, handle, 5, &handle->task_handle);
    
    return (ret == pdPASS) ? ESP_OK : ESP_FAIL;
}

esp_err_t vl53l0x_stop_continuous(vl53l0x_handle_t handle) {
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }
    
    handle->is_continuous = false;
    vTaskDelay(pdMS_TO_TICKS(100)); // Wait for task to finish
    
    return ESP_OK;
}

esp_err_t vl53l0x_set_mode(vl53l0x_handle_t handle, vl53l0x_mode_t mode) {
    if (!handle || !handle->is_initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    xSemaphoreTake(handle->mutex, portMAX_DELAY);
    esp_err_t ret = configure_mode(handle, mode);
    handle->config.mode = mode;
    xSemaphoreGive(handle->mutex);
    
    return ret;
}

esp_err_t vl53l0x_get_distance(vl53l0x_handle_t handle, uint16_t* distance_mm) {
    vl53l0x_measurement_t measurement;
    esp_err_t ret = vl53l0x_read_single(handle, &measurement);
    
    if (ret == ESP_OK && measurement.is_valid) {
        *distance_mm = measurement.distance_mm;
    }
    
    return ret;
}

esp_err_t vl53l0x_is_ready(vl53l0x_handle_t handle, bool* is_ready) {
    if (!handle || !is_ready) {
        return ESP_ERR_INVALID_ARG;
    }
    
    *is_ready = handle->is_initialized;
    return ESP_OK;
}

esp_err_t vl53l0x_deinit(vl53l0x_handle_t handle) {
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (handle->is_continuous) {
        vl53l0x_stop_continuous(handle);
    }
    
    if (handle->mutex) {
        vSemaphoreDelete(handle->mutex);
    }
    
    free(handle);
    
    return ESP_OK;
}

const char* vl53l0x_get_mode_name(vl53l0x_mode_t mode) {
    switch (mode) {
        case VL53L0X_MODE_HIGH_ACCURACY: return "High Accuracy";
        case VL53L0X_MODE_HIGH_SPEED: return "High Speed";
        case VL53L0X_MODE_LONG_RANGE: return "Long Range";
        case VL53L0X_MODE_DEFAULT:
        default: return "Default";
    }
}
