# Modo de Alta Precisión - VL53L0X

## 🎯 Descripción

El sensor VL53L0X soporta múltiples modos de operación. El proyecto está configurado por defecto en **modo de alta precisión**, optimizado para mediciones exactas a corta distancia.

## 📊 Modos de Operación Disponibles

### 1. Modo Por Defecto (Default)
- **Timing Budget:** 30 ms
- **Precisión:** ±3%
- **Rango:** 30-1200 mm
- **Frecuencia:** ~33 Hz
- **Uso:** Aplicaciones generales

### 2. Modo Alta Precisión (High Accuracy) ⭐ **ACTIVO**
- **Timing Budget:** 200 ms
- **Precisión:** ±1%
- **Rango:** 30-1200 mm (óptimo: 30-500 mm)
- **Frecuencia:** ~4 Hz
- **Uso:** Mediciones precisas, micromouse, navegación

### 3. Modo Alta Velocidad (High Speed)
- **Timing Budget:** 20 ms
- **Precisión:** ±3%
- **Rango:** 30-1200 mm
- **Frecuencia:** ~50 Hz
- **Uso:** Detección rápida de movimiento

### 4. Modo Largo Alcance (Long Range)
- **Timing Budget:** 33 ms
- **Precisión:** ±3%
- **Rango:** 30-2000 mm
- **Frecuencia:** ~30 Hz
- **Uso:** Detección de objetos distantes

## ⚙️ Configuración del Modo Alta Precisión

El modo se configura en la inicialización del sensor:

```c
vl53l0x_config_t config = {
    .scl_pin = GPIO_NUM_5,
    .sda_pin = GPIO_NUM_6,
    .i2c_freq_hz = 400000,
    .mode = VL53L0X_MODE_HIGH_ACCURACY,  // ← Modo alta precisión
    .i2c_address = 0x29
};
```

### Parámetros Internos

El modo de alta precisión configura automáticamente:

| Parámetro | Valor | Efecto |
|-----------|-------|--------|
| **Signal Rate Limit** | 0.25 MCPS | Filtra señales débiles |
| **Sigma Limit** | 18 mm | Límite de desviación estándar |
| **Timing Budget** | 200 ms | Tiempo de medición |

## 📈 Comparación de Rendimiento

| Característica | Default | Alta Precisión | Alta Velocidad | Largo Alcance |
|----------------|---------|----------------|----------------|---------------|
| **Precisión** | ±3% | **±1%** | ±3% | ±3% |
| **Timing Budget** | 30 ms | **200 ms** | 20 ms | 33 ms |
| **Frecuencia** | 33 Hz | **4 Hz** | 50 Hz | 30 Hz |
| **Rango óptimo** | 0-1200mm | **30-500mm** | 0-1200mm | 0-2000mm |
| **Estabilidad** | Buena | **Excelente** | Buena | Buena |

## 🎯 Ventajas para Micromouse

El modo de alta precisión es ideal para micromouse porque:

1. **Mayor precisión** - ±1% vs ±3% en otros modos
2. **Mejor estabilidad** - Menos variación entre lecturas
3. **Filtrado robusto** - Rechaza mediciones erróneas
4. **Rango óptimo** - Perfecto para distancias cortas (paredes del laberinto)

## 🔄 Cambiar de Modo

Para cambiar el modo de operación en tiempo de ejecución:

```c
// Cambiar a modo alta velocidad
vl53l0x_set_mode(sensor, VL53L0X_MODE_HIGH_SPEED);

// Cambiar a modo largo alcance
vl53l0x_set_mode(sensor, VL53L0X_MODE_LONG_RANGE);

// Volver a alta precisión
vl53l0x_set_mode(sensor, VL53L0X_MODE_HIGH_ACCURACY);
```

## 📊 Resultados Esperados

Con un objeto a 100 mm de distancia en modo alta precisión:

```
Mediciones típicas:
100, 101, 100, 99, 100, 101, 100, 100, 99, 100

Promedio: 100.1 mm
Desviación estándar: < 1 mm
Error: 0.1%
```

## ⚙️ Configuración en app_config.h

El modo se puede configurar globalmente:

```c
// Modo del sensor
#define SENSOR_MODE  VL53L0X_MODE_HIGH_ACCURACY
```

## 💡 Recomendaciones de Uso

### ✅ Usar Alta Precisión cuando:
- Necesitas mediciones exactas (±1%)
- Trabajas con distancias cortas (< 500mm)
- La frecuencia de 4 Hz es suficiente
- Navegación precisa en micromouse

### ⚠️ Considerar otros modos cuando:
- Necesitas > 10 Hz de frecuencia → **Alta Velocidad**
- Trabajas con distancias > 1.2m → **Largo Alcance**
- Consumo de energía es crítico → **Default**

## 📝 Notas Técnicas

### Signal Rate (0.25 MCPS)
- **MCPS:** Mega Counts Per Second
- **Efecto:** Rechaza señales débiles que causan lecturas erróneas
- **Rango típico:** 0.1 - 0.5 MCPS

### Sigma (18 mm)
- **Definición:** Desviación estándar máxima aceptable
- **Efecto:** Filtra mediciones con alta variabilidad
- **Valores típicos:** 15-60 mm (menor = más estricto)

### Timing Budget (200 ms)
- **Definición:** Tiempo asignado para cada medición
- **Efecto:** Más tiempo = más muestras = mejor precisión
- **Rango:** 20-200 ms

---

**Modo actual del proyecto:** Alta Precisión (VL53L0X_MODE_HIGH_ACCURACY)  
**Configurado en:** `main/main.c` línea 18
