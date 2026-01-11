# Guía de Prueba de Motores - RC Car

## 🎯 Objetivo

Este documento explica cómo funciona la prueba de motores para el carro RC con dirección Ackermann usando drivers MX1508.

## 🔌 Conexiones Hardware

### Motor Trasero (Tracción)
- **IN1** → GPIO 7
- **IN2** → GPIO 8
- **Función:** Controla avance y retroceso del vehículo

### Motor Delantero (Dirección)
- **IN1** → GPIO 9
- **IN2** → GPIO 10
- **Función:** Controla dirección (izquierda/derecha)

### Alimentación
- **VM** → 5V (alimentación del motor)
- **GND** → GND común con ESP32

## 📋 Estructura de la Prueba

La prueba se divide en **3 fases**:

### Fase 1: Motor de Tracción (Trasero)

Prueba el motor que controla el movimiento adelante/atrás.

**Secuencia:**
1. **Adelante 30%** - 2 segundos
2. **Adelante 50%** - 2 segundos
3. **Adelante 70%** - 2 segundos
4. **Atrás 30%** - 2 segundos
5. **Atrás 50%** - 2 segundos

**Qué verificar:**
- ✅ El motor gira en la dirección correcta
- ✅ La velocidad aumenta con el porcentaje
- ✅ El motor se detiene entre pruebas
- ✅ El cambio de dirección funciona correctamente

### Fase 2: Motor de Dirección (Delantero)

Prueba el motor que controla la dirección del vehículo.

**Secuencia:**
1. **Centro** - Posición neutral
2. **Izquierda 50%** - Giro medio a la izquierda
3. **Centro** - Regresa al centro
4. **Derecha 50%** - Giro medio a la derecha
5. **Centro** - Regresa al centro
6. **Izquierda completa** - Giro máximo izquierda
7. **Centro** - Regresa al centro
8. **Derecha completa** - Giro máximo derecha
9. **Centro** - Posición final

**Qué verificar:**
- ✅ Las ruedas giran hacia la izquierda cuando se indica
- ✅ Las ruedas giran hacia la derecha cuando se indica
- ✅ Las ruedas regresan al centro correctamente
- ✅ El rango de movimiento es adecuado

### Fase 3: Movimiento Combinado

Prueba ambos motores trabajando juntos.

**Secuencia:**
1. **Adelante recto (60%)** - Avanza sin girar
2. **Giro a la izquierda** - Avanza girando a la izquierda
3. **Giro a la derecha** - Avanza girando a la derecha
4. **Retroceso + Izquierda** - Retrocede girando a la izquierda
5. **Retroceso + Derecha** - Retrocede girando a la derecha

**Qué verificar:**
- ✅ El carro avanza en línea recta
- ✅ El carro gira correctamente mientras avanza
- ✅ El carro puede retroceder con dirección
- ✅ Los movimientos son suaves y coordinados

## 🖥️ Salida del Monitor Serial

```
╔════════════════════════════════════════╗
║    RC Car Motor Test - MX1508          ║
╚════════════════════════════════════════╝

Initializing motors...
Motors initialized successfully
Drive motor: IN1=GPIO7, IN2=GPIO8
Steering motor: IN1=GPIO9, IN2=GPIO10

╔════════════════════════════════════════╗
║  TEST 1: Drive Motor (Rear)            ║
╚════════════════════════════════════════╝

→ Forward 30%
→ Forward 50%
→ Forward 70%
← Backward 30%
← Backward 50%
✓ Drive motor test complete

╔════════════════════════════════════════╗
║  TEST 2: Steering Motor (Front)        ║
╚════════════════════════════════════════╝

⊙ Center position
← Left 50%
⊙ Center
→ Right 50%
⊙ Center
← Full left
⊙ Center
→ Full right
⊙ Center
✓ Steering motor test complete

╔════════════════════════════════════════╗
║  TEST 3: Combined Movement             ║
╚════════════════════════════════════════╝

↑ Forward straight (60%)
↖ Forward + Left turn
↗ Forward + Right turn
↙ Backward + Left
↘ Backward + Right
✓ Combined movement test complete

╔════════════════════════════════════════╗
║  All Tests Completed Successfully!     ║
╚════════════════════════════════════════╝

Motors are now stopped.
You can now integrate with sensors for autonomous driving.
```

## 🔧 Cómo Funciona el Control PWM

### Control de Velocidad

El driver MX1508 usa PWM (Modulación por Ancho de Pulso) para controlar la velocidad:

- **0%** → Motor detenido
- **50%** → Media velocidad
- **100%** → Velocidad máxima

### Tabla de Verdad MX1508

| IN1 | IN2 | Resultado |
|-----|-----|-----------|
| LOW | LOW | Parado |
| PWM | LOW | Adelante (velocidad variable) |
| LOW | PWM | Atrás (velocidad variable) |
| HIGH | HIGH | Freno |

### Ejemplo de Código

```c
// Adelante al 50%
motor_drive_forward(50);
// Internamente: IN1=PWM(50%), IN2=0%

// Atrás al 30%
motor_drive_backward(30);
// Internamente: IN1=0%, IN2=PWM(30%)

// Girar a la izquierda
motor_steering_set_angle(-80);
// Internamente: IN1=0%, IN2=PWM(80%)
```

## ⚠️ Solución de Problemas

### Motor no se mueve

1. **Verificar conexiones:**
   - Revisa que los cables estén en los GPIOs correctos
   - Verifica la alimentación del MX1508 (VM)
   - Comprueba GND común

2. **Verificar alimentación:**
   - Alimentación de 5V
   - Conexión VM del MX1508

3. **Verificar en el monitor:**
   - ¿Aparece "Motors initialized successfully"?
   - ¿Hay mensajes de error?

### Motor gira al revés

**Solución:** Intercambia los cables IN1 e IN2 del motor afectado.

```c
// Si el motor va al revés, cambia:
.in1_pin = GPIO_NUM_7,  // Era IN1
.in2_pin = GPIO_NUM_8,  // Era IN2

// Por:
.in1_pin = GPIO_NUM_8,  // Ahora es IN2
.in2_pin = GPIO_NUM_7,  // Ahora es IN1
```

### Motor se mueve muy lento

1. **Aumentar el porcentaje de velocidad** en el código
2. **Verificar voltaje de alimentación** - debe ser 5V para mejor rendimiento
3. **Revisar conexiones** - resistencia alta puede reducir potencia

### Dirección no regresa al centro

El motor de dirección puede necesitar **calibración manual**:

```c
// Ajustar el valor de "centro"
// En motor_control.c, línea ~220:
// Cambiar de 0 a un valor entre -10 y +10
motor_steering_set_angle(5);  // Prueba diferentes valores
```

## 🚀 Próximos Pasos

Una vez que la prueba funcione correctamente:

1. **Integrar con sensores VL53L0X** para detección de obstáculos
2. **Implementar navegación autónoma** básica
3. **Agregar control PID** para movimiento suave
4. **Crear modos de conducción** (lento, normal, rápido)

## 📝 Notas Importantes

- ⚠️ **Siempre** prueba primero con velocidades bajas (30-50%)
- ⚠️ **Asegúrate** de que el carro esté elevado o tenga espacio libre
- ⚠️ **Ten a mano** el botón de reset del ESP32 por seguridad
- ✅ **Verifica** que la batería esté bien conectada antes de flashear

---

**Duración total de la prueba:** ~2 minutos  
**Requiere supervisión:** Sí  
**Nivel de dificultad:** Principiante
