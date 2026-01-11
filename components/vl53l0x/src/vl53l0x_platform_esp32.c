/**
 * @file vl53l0x_platform_esp32.c
 * @brief Implementación de la capa de plataforma VL53L0X para ESP32
 * Usando la nueva API i2c_master.h de ESP-IDF v5.x
 */

#include "vl53l0x_platform.h"
#include "vl53l0x_api.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>

#define I2C_MASTER_TIMEOUT_MS 1000

// External global handle set by driver
extern i2c_master_dev_handle_t g_current_vl53l0x_dev_handle;

/**
 * @brief Escribe múltiples bytes en un registro del VL53L0X
 */
VL53L0X_Error VL53L0X_WriteMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata, uint32_t count)
{
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    
    // Get I2C device handle from global variable
    i2c_master_dev_handle_t dev_handle = g_current_vl53l0x_dev_handle;
    if (!dev_handle) {
        return VL53L0X_ERROR_CONTROL_INTERFACE;
    }
    
    // Crear buffer con el índice del registro seguido de los datos
    uint8_t *write_buf = malloc(count + 1);
    if (write_buf == NULL) {
        return VL53L0X_ERROR_CONTROL_INTERFACE;
    }
    
    write_buf[0] = index;
    memcpy(&write_buf[1], pdata, count);
    
    // Usar la nueva API i2c_master_transmit
    esp_err_t ret = i2c_master_transmit(dev_handle, write_buf, count + 1, I2C_MASTER_TIMEOUT_MS);
    
    free(write_buf);
    
    if (ret != ESP_OK) {
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    }
    
    return Status;
}

/**
 * @brief Lee múltiples bytes desde un registro del VL53L0X
 */
VL53L0X_Error VL53L0X_ReadMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata, uint32_t count)
{
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    
    // Get I2C device handle from global variable
    i2c_master_dev_handle_t dev_handle = g_current_vl53l0x_dev_handle;
    if (!dev_handle) {
        return VL53L0X_ERROR_CONTROL_INTERFACE;
    }
    
    // Usar la nueva API i2c_master_transmit_receive
    esp_err_t ret = i2c_master_transmit_receive(dev_handle, 
                                                 &index, 1,           // Escribir el índice del registro
                                                 pdata, count,        // Leer los datos
                                                 I2C_MASTER_TIMEOUT_MS);
    
    if (ret != ESP_OK) {
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    }
    
    return Status;
}

/**
 * @brief Escribe un solo byte en un registro
 */
VL53L0X_Error VL53L0X_WrByte(VL53L0X_DEV Dev, uint8_t index, uint8_t data)
{
    return VL53L0X_WriteMulti(Dev, index, &data, 1);
}

/**
 * @brief Escribe una palabra (16 bits) en un registro
 */
VL53L0X_Error VL53L0X_WrWord(VL53L0X_DEV Dev, uint8_t index, uint16_t data)
{
    uint8_t buffer[2];
    buffer[0] = (data >> 8) & 0xFF;
    buffer[1] = data & 0xFF;
    return VL53L0X_WriteMulti(Dev, index, buffer, 2);
}

/**
 * @brief Escribe un double word (32 bits) en un registro
 */
VL53L0X_Error VL53L0X_WrDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t data)
{
    uint8_t buffer[4];
    buffer[0] = (data >> 24) & 0xFF;
    buffer[1] = (data >> 16) & 0xFF;
    buffer[2] = (data >> 8) & 0xFF;
    buffer[3] = data & 0xFF;
    return VL53L0X_WriteMulti(Dev, index, buffer, 4);
}

/**
 * @brief Lee un solo byte desde un registro
 */
VL53L0X_Error VL53L0X_RdByte(VL53L0X_DEV Dev, uint8_t index, uint8_t *data)
{
    return VL53L0X_ReadMulti(Dev, index, data, 1);
}

/**
 * @brief Lee una palabra (16 bits) desde un registro
 */
VL53L0X_Error VL53L0X_RdWord(VL53L0X_DEV Dev, uint8_t index, uint16_t *data)
{
    uint8_t buffer[2];
    VL53L0X_Error Status = VL53L0X_ReadMulti(Dev, index, buffer, 2);
    *data = ((uint16_t)buffer[0] << 8) | buffer[1];
    return Status;
}

/**
 * @brief Lee un double word (32 bits) desde un registro
 */
VL53L0X_Error VL53L0X_RdDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t *data)
{
    uint8_t buffer[4];
    VL53L0X_Error Status = VL53L0X_ReadMulti(Dev, index, buffer, 4);
    *data = ((uint32_t)buffer[0] << 24) | ((uint32_t)buffer[1] << 16) | 
            ((uint32_t)buffer[2] << 8) | buffer[3];
    return Status;
}

/**
 * @brief Actualiza un byte (lee-modifica-escribe)
 */
VL53L0X_Error VL53L0X_UpdateByte(VL53L0X_DEV Dev, uint8_t index, uint8_t AndData, uint8_t OrData)
{
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    uint8_t data;

    Status = VL53L0X_RdByte(Dev, index, &data);
    if (Status == VL53L0X_ERROR_NONE) {
        data = (data & AndData) | OrData;
        Status = VL53L0X_WrByte(Dev, index, data);
    }

    return Status;
}

/**
 * @brief Función de delay en milisegundos
 */
VL53L0X_Error VL53L0X_PollingDelay(VL53L0X_DEV Dev)
{
    (void)Dev;
    vTaskDelay(pdMS_TO_TICKS(1));
    return VL53L0X_ERROR_NONE;
}
