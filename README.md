# RC Car Control System - ESP32-S3

Sistema de control para carro RC con Ackermann steering (tracción trasera, dirección delantera) usando ESP32-S3 y drivers MX1508.

## 🎯 Características

- ✅ **Control de motores PWM** con MX1508
- ✅ **Ackermann steering** (motor trasero + dirección delantera)
- ✅ **Sensor de distancia VL53L0X** para detección de obstáculos
- ✅ **Arquitectura basada en componentes** modular y reutilizable
- ⏳ **Control web** (en desarrollo futuro)

## 📁 Estructura del Proyecto

```
sensor/
├── components/
│   ├── motor_control/          # Control PWM de motores MX1508
│   │   ├── include/
│   │   │   └── motor_control.h
│   │   ├── src/
│   │   │   └── motor_control.c
│   │   └── CMakeLists.txt
│   ├── vl53l0x/               # Driver para sensor VL53L0X
│   │   ├── include/
│   │   ├── src/
│   │   └── CMakeLists.txt
│   └── obstacle_detection/    # Sistema de detección de obstáculos
│       ├── include/
│       ├── src/
│       └── CMakeLists.txt
├── main/
│   ├── main.c                 # Aplicación principal (test de motores)
│   └── CMakeLists.txt
├── examples/
│   ├── motor_test/            # Ejemplo de prueba de motores
│   └── vl53l0x_test/          # Ejemplo de sensor VL53L0X
└── README.md
```

## 🔧 Hardware

### ESP32-S3
- **Placa:** ESP32-S3 DevKit
- **Flash:** 2MB mínimo
- **RAM:** 512KB

### Motores y Drivers
- **Driver:** MX1508 (x2)
- **Motor tracción:** Conectado a GPIO 7, 8
- **Motor dirección:** Conectado a GPIO 9, 10
- **Alimentación motores:** 5V (separada del ESP32)
- **PWM:** 1000 Hz

### Sensor de Distancia
- **Modelo:** VL53L0X
- **Interfaz:** I2C
- **Pines:** SDA=GPIO6, SCL=GPIO5
- **Rango:** 30-1000mm

## 🚀 Inicio Rápido

### 1. Requisitos

- ESP-IDF v5.5.1
- Python 3.11
- Cable USB para programación

### 2. Configuración del Entorno

```bash
# Configurar ESP-IDF
$env:IDF_PATH = 'C:\Users\joseg\esp\v5.5.1\esp-idf'
```

### 3. Compilar y Flashear

```bash
cd sensor
idf.py build
idf.py -p COM4 flash monitor
```

## 📖 Componentes

### Motor Control

Control PWM para motores DC con MX1508.

**Características:**
- Control de velocidad -100% a +100%
- Control de dirección -100° a +100°
- Funciones de alto nivel (adelante, atrás, girar)
- Thread-safe con mutexes

**Ejemplo de uso:**

```c
#include "motor_control.h"

// Configurar motores
motor_config_t drive_config = {
    .in1_pin = GPIO_NUM_7,
    .in2_pin = GPIO_NUM_8,
    .pwm_freq_hz = 1000,
    .timer = LEDC_TIMER_0,
    .channel_a = LEDC_CHANNEL_0,
    .channel_b = LEDC_CHANNEL_1
};

motor_config_t steering_config = {
    .in1_pin = GPIO_NUM_9,
    .in2_pin = GPIO_NUM_10,
    .pwm_freq_hz = 1000,
    .timer = LEDC_TIMER_1,
    .channel_a = LEDC_CHANNEL_2,
    .channel_b = LEDC_CHANNEL_3
};

// Inicializar
motor_control_init(&drive_config, &steering_config);

// Controlar
motor_drive_forward(50);        // Adelante 50%
motor_steering_set_angle(-30);  // Girar izquierda 30°
motor_turn_left(60);            // Giro completo izquierda
motor_stop_all();               // Detener todo
```

### VL53L0X Driver

Driver para sensor de distancia láser VL53L0X.

**Modos disponibles:**
- `VL53L0X_MODE_DEFAULT` - Balance velocidad/precisión
- `VL53L0X_MODE_HIGH_SPEED` - Lecturas rápidas
- `VL53L0X_MODE_HIGH_ACCURACY` - Máxima precisión
- `VL53L0X_MODE_LONG_RANGE` - Máximo alcance

**Ejemplo:**

```c
#include "vl53l0x_driver.h"

vl53l0x_config_t config = {
    .scl_pin = GPIO_NUM_5,
    .sda_pin = GPIO_NUM_6,
    .i2c_freq_hz = 400000,
    .mode = VL53L0X_MODE_HIGH_ACCURACY
};

vl53l0x_handle_t sensor;
vl53l0x_init(&config, &sensor);

vl53l0x_measurement_t measurement;
vl53l0x_read_single(sensor, &measurement);
printf("Distance: %d mm\n", measurement.distance_mm);
```

## 🎮 Aplicación Principal

El `main.c` actual ejecuta un test completo de los motores:

1. **Test de tracción** - Adelante/atrás a diferentes velocidades
2. **Test de dirección** - Izquierda/derecha/centro
3. **Test combinado** - Movimientos complejos

## 📝 Documentación Adicional

- [`MOTOR_TEST_GUIDE.md`](MOTOR_TEST_GUIDE.md) - Guía del test de motores
- [`HIGH_PRECISION_MODE.md`](HIGH_PRECISION_MODE.md) - Modo de alta precisión VL53L0X
- [`LIBRARY_DEPENDENCIES.md`](LIBRARY_DEPENDENCIES.md) - Dependencias de librerías

## ⚠️ Notas Importantes

### Alimentación de Motores

**CRÍTICO:** Los motores MX1508 deben tener alimentación separada:

```
ESP32 (USB 5V) ──────┐
                     │
MX1508 (5V) ──┼── GND común
                     │
Motores ─────────────┘
```

- **NO** alimentar motores desde el ESP32
- **SÍ** conectar GND común entre ESP32 y MX1508
- **SÍ** usar alimentación de 5V para motores

### Frecuencia PWM

- Usar **1000 Hz** para evitar resets del watchdog
- No usar frecuencias >3000 Hz

## 🔮 Desarrollo Futuro

- [ ] Control web WiFi
- [ ] Modo autónomo con sensores
- [ ] Telemetría en tiempo real
- [ ] Grabación de rutas
- [ ] Múltiples perfiles de velocidad

## 📄 Licencia

Proyecto educativo - ESP32-S3 RC Car Control System

---

**Versión:** 1.0  
**Hardware:** ESP32-S3 + MX1508 + VL53L0X  
**Firmware:** ESP-IDF v5.5.1
