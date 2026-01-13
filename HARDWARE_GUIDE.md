# Guía de Conexión y Especificaciones Técnicas - VL53L0X

## 📐 Especificaciones del Sensor VL53L0X

### Características Principales

| Característica | Especificación |
|----------------|----------------|
| **Tecnología** | Time-of-Flight (ToF) |
| **Rango de medición** | 30 mm - 2000 mm |
| **Precisión** | ±3% (hasta 1m) |
| **Velocidad de medición** | Hasta 50 Hz |
| **Interfaz** | I2C (hasta 400 kHz) |
| **Voltaje de operación** | 2.6V - 3.5V |
| **Corriente de operación** | ~19 mA (medición activa) |
| **Ángulo de visión (FoV)** | 25° |
| **Longitud de onda** | 940 nm (infrarrojo) |

### Modos de Operación

#### 1. Modo Por Defecto
- **Timing Budget**: ~30 ms
- **Rango**: 30 mm - 1200 mm
- **Uso**: Aplicaciones generales

#### 2. Modo Alta Precisión
- **Timing Budget**: ~200 ms
- **Rango**: 30 mm - 1200 mm
- **Precisión**: ±1% (hasta 1m)
- **Uso**: Mediciones de alta exactitud

#### 3. Modo Alta Velocidad
- **Timing Budget**: ~20 ms
- **Rango**: 30 mm - 1200 mm
- **Velocidad**: Hasta 50 Hz
- **Uso**: Detección rápida de movimiento

#### 4. Modo Largo Alcance
- **Timing Budget**: ~33 ms
- **Rango**: 30 mm - 2000 mm
- **Uso**: Detección de objetos distantes

## 🔌 Diagrama de Conexión

### Conexión: Módulo CJVL53L0XV2 ↔ ESP32

El módulo **CJVL53L0XV2** (color morado) tiene el siguiente pinout.

> [!WARNING]
> **Voltaje VCC**: Conecta VCC a **3.3V** del ESP32. Aunque el módulo puede tolerar 5V, usar 3.3V asegura que las señales I2C (SDA/SCL) sean de 3.3V, protegiendo al ESP32.

```
ESP32 (3.3V System)      CJVL53L0XV2 (Sensor)
┌─────────────┐         ┌──────────────┐
│             │         │  ○ VCC       │◄─── 3.3V (ESP32)
│   GND     ──┼─────────┼──○ GND       │◄─── GND
│             │         │              │
│   GPIO 22 ──┼─────────┼──○ SCL       │
│             │         │              │
│   GPIO 21 ──┼─────────┼──○ SDA       │
│             │         │              │
│             │         │  ○ GPIO1     │ (Sin conectar)
│             │         │              │
│   GPIO X  ──┼─────────┼──○ XSHUT     │ (Opcional, para reset)
└─────────────┘         └──────────────┘
```

## 🔋 Configuración de Energía (SM5308 + Motores)

Al usar el módulo Power Bank **SM5308** para alimentar tanto el ESP32 como los motores desde la misma salida de 5V, debes tener mucho cuidado con el ruido eléctrico.

### Topología de Energía (Compartida)

```
[ Batería ] ── [ SM5308 Power Bank ] ──┬──► +5V ──► [ Driver Motores ] ──► [ Motores ]
                                       │
                                       └──► +5V ──► [ ESP32-S3 ] (¡RIESGO DE RUIDO!)
```

### ⚠️ Problema Crítico: Ruido y Brownouts
Los motores generan picos de voltaje que viajan por la línea de +5V y pueden reiniciar el ESP32.

**Solución OBLIGATORIA:**
1.  **Capacitor de Filtrado**: Conecta un capacitor electrolítico de **470µF a 1000µF (10V+)** directamente en los pines `5V` y `GND` del ESP32.
2.  **Cables**: Usa cables cortos y gruesos para la alimentación.

> [!NOTE]
> El módulo SM5308 puede apagarse automáticamente si el consumo es muy bajo (menos de ~50mA). Si el ESP32 se apaga solo cuando los motores están detenidos, es posible que el power bank esté entrando en modo de ahorro.

## ⚙️ Configuración I2C

### Dirección I2C

- **Dirección por defecto**: `0x29` (7-bit)
- **Dirección alternativa**: Configurable por software
- La dirección puede cambiarse temporalmente usando `VL53L0X_SetDeviceAddress()`

### Velocidad del Bus

| Modo | Frecuencia | Configuración en código |
|------|------------|------------------------|
| Standard | 100 kHz | `I2C_MASTER_FREQ_HZ = 100000` |
| Fast | 400 kHz | `I2C_MASTER_FREQ_HZ = 400000` (recomendado) |

### Pull-up Resistors

- **Valor recomendado**: 4.7kΩ - 10kΩ
- La mayoría de módulos VL53L0X ya incluyen resistencias pull-up
- Si usas el chip directamente, debes agregar resistencias externas

## 🛠️ Configuración de Hardware

### Opción 1: Módulo Breakout (Recomendado)

Módulos comunes:
- **Adafruit VL53L0X**
- **Pololu VL53L0X**
- **Módulos genéricos GY-VL53L0XV2**

Ventajas:
- ✅ Regulador de voltaje incluido (puede usar 5V)
- ✅ Pull-ups incluidos
- ✅ Fácil de conectar
- ✅ Protección incorporada

### Opción 2: Chip Directo

Si usas el chip VL53L0X directamente:

```
                    VL53L0X
                 ┌──────────┐
    3.3V ────────┤ AVDD     │
                 │          │
    3.3V ────────┤ VDD      │
                 │          │
    GND  ────────┤ GND      │
                 │          │
    SDA  ────┬───┤ SDA      │
             │   │          │
         4.7kΩ  │          │
             │   │          │
    3.3V ────┘   │          │
                 │          │
    SCL  ────┬───┤ SCL      │
             │   │          │
         4.7kΩ  │          │
             │   │          │
    3.3V ────┘   │          │
                 │          │
    3.3V ────────┤ XSHUT    │
                 └──────────┘
```

## 🔍 Verificación de Conexión

### Usando i2cdetect (Linux/ESP-IDF)

Si tienes acceso a herramientas I2C:

```bash
i2cdetect -y 0
```

Deberías ver el dispositivo en la dirección `0x29`:

```
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:          -- -- -- -- -- -- -- -- -- -- -- -- -- 
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
20: -- -- -- -- -- -- -- -- -- 29 -- -- -- -- -- -- 
30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
```

### Código de Prueba Simple

```c
#include "driver/i2c.h"

esp_err_t test_vl53l0x_connection(void)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (0x29 << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    if (ret == ESP_OK) {
        printf("✓ VL53L0X detectado en 0x29\n");
    } else {
        printf("❌ VL53L0X no detectado\n");
    }
    
    return ret;
}
```

## 📊 Consideraciones de Rendimiento

### Factores que Afectan la Medición

| Factor | Impacto | Solución |
|--------|---------|----------|
| **Superficie oscura** | Reduce alcance | Usar modo largo alcance |
| **Superficie brillante/reflectante** | Puede saturar sensor | Reducir timing budget |
| **Luz solar directa** | Interferencia | Evitar o usar filtro |
| **Ángulo de incidencia** | Reduce precisión | Mantener perpendicular |
| **Objetos transparentes** | No detectables | Usar sensor alternativo |

### Optimización del Consumo de Energía

```c
// Modo de bajo consumo
VL53L0X_SetDeviceMode(pDevice, VL53L0X_DEVICEMODE_SINGLE_RANGING);

// Apagar entre mediciones
VL53L0X_StopMeasurement(pDevice);

// Usar XSHUT para apagado completo
gpio_set_level(XSHUT_PIN, 0);  // Apagar
vTaskDelay(pdMS_TO_TICKS(100));
gpio_set_level(XSHUT_PIN, 1);  // Encender
```

## 🔧 Solución de Problemas de Hardware

### Problema: Sensor no detectado

**Verificar:**
1. ✓ Conexiones físicas (SDA, SCL, VCC, GND)
2. ✓ Voltaje de alimentación (debe ser 3.3V)
3. ✓ Resistencias pull-up presentes
4. ✓ Cables no demasiado largos (< 30cm recomendado)

### Problema: Mediciones erráticas

**Verificar:**
1. ✓ Alimentación estable (usar capacitor de desacople)
2. ✓ Cables I2C blindados si hay ruido
3. ✓ Frecuencia I2C no demasiado alta
4. ✓ Objeto dentro del rango de medición

### Problema: Alcance limitado

**Soluciones:**
1. Usar modo largo alcance
2. Aumentar timing budget
3. Mejorar reflectividad del objeto
4. Reducir luz ambiente

## 📝 Notas Importantes

> [!WARNING]
> **No conectar VCC a 5V directamente** - El VL53L0X opera a 3.3V. Usar módulo con regulador o alimentar con 3.3V.

> [!IMPORTANT]
> **Capacitor de desacople** - Agregar un capacitor de 100nF cerca del pin VCC para estabilidad.

> [!TIP]
> **Múltiples sensores** - Para usar varios VL53L0X en el mismo bus I2C, usa el pin XSHUT para cambiar direcciones individualmente.

## 🔗 Referencias

- [Datasheet VL53L0X](https://www.st.com/resource/en/datasheet/vl53l0x.pdf)
- [API Documentation](https://www.st.com/content/st_com/en/products/embedded-software/proximity-sensors-software/stsw-img005.html)
- [ESP-IDF I2C Driver](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2c.html)

## ⚡ Brownout Prevention (ESP32 Reset Fix)

If your ESP32 resets when the steering motor is activated (especially if stalled), it is likely due to a voltage drop ("brownout").

### Solutions:
1.  **Add a Capacitor**: Place a large electrolytic capacitor (470µF to 1000µF, 10V or higher) across the power input pins (VIN/5V and GND) of the ESP32. This acts as a local energy reservoir.
2.  **Separate Power**: Use a dedicated 5V Buck Converter (Step-down) for the ESP32, separate from the motor power source, but sharing a common Ground.
3.  **Check Wiring**: Ensure wires from the battery to the motor driver are thick enough (at least 22AWG) to handle the surge current without significant voltage drop.
