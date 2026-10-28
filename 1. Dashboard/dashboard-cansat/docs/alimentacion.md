# Alimentación con una sola batería de 9V

La unica bateria permitida es una **9V cuadrada**. El ESP32, el GPS, los sensores y el servo **no** trabajan a 9V,
y el ADC del ESP32 solo soporta **3.3 V**: conectar los 9V directo a cualquier pin lo daña.
Por eso hacen falta dos cosas: un **regulador (buck) 9V → 5V** y un **divisor de voltaje** para medir la bateria.

## Esquema

```
 9V (+) ──[ interruptor ]──┬────────────── IN+  ┌──────────────┐  OUT+ 5.0V ──┬── ESP32  pin 5V / VIN
                           │                    │ BUCK a 5 V   │              ├── Servo (cable rojo)
                           │                    │ (>=1 A)      │              └── C 1000 µF  (junto al servo,
                           │                    └──────────────┘                   entre 5V y GND)
                           │  Divisor para leer la bateria
                           ├── R1 100 kΩ ──┬── R2 33 kΩ ── GND
                           │               └── GPIO34 del ESP32  (+ 100 nF a GND)
 9V (−) ───────────────────┴── GND COMUN: bateria, buck, ESP32, servo, GPS, sensores

 ESP32 pin 3V3 ──┬── BMP280  VCC
                 ├── MPU6050 VCC
                 └── GPS     VCC   (si tu modulo GPS indica 3.3–5 V, tambien puede ir a 3V3)
```

El divisor entrega  `V_adc = V_bat × 33 / (100 + 33)`:  9.6 V (bateria nueva) → 2.38 V, seguro para el ESP32.
En el firmware `VBAT_DIVISOR = 4.03`. Compara contra un multimetro (sketch `07_bateria`) y ajusta ese numero.

## Primera conexion, en este orden (no te saltes pasos)

1. **Solo el buck**, sin nada conectado a su salida. Conecta la bateria y con un multimetro ajusta el tornillo/trimmer
   hasta leer **5.0 V** en la salida. Desconecta la bateria.
2. Mide el **nodo del divisor** (entre R1 y R2) con la bateria conectada: debe dar ~2.3–2.4 V. Solo si es asi, conectalo al GPIO34.
3. Conecta el **ESP32** al buck (pin 5V). Verifica que enciende. **No alimentes por USB y por el buck al mismo tiempo**
   a menos que tu placa tenga diodo de proteccion; para programar, desconecta la bateria.
4. Conecta sensores y GPS (3.3 V).
5. Al final conecta el **servo**, con el capacitor de 1000 µF pegado a el.

## Por que se reinicia el ESP32 cuando se mueve el servo

El servo arranca con picos de corriente (cientos de mA). Una 9V alcalina tiene resistencia interna alta: el voltaje
se hunde un instante, el buck baja de 5V y el ESP32 se reinicia. Para reducirlo:

- Capacitor de **1000 µF** en la alimentacion del servo (y otro de 100 µF cerca del ESP32).
- Buck que entregue **al menos 1 A** (MP1584, LM2596 o similar).
- Idealmente una **9V de litio** (ej. Energizer Ultimate Lithium): aguanta mucho mejor los picos y pesa menos.
- El firmware ya **suelta el servo 2 s despues de moverlo** y, si la placa se reinicia en vuelo, conserva su referencia de altura.

## Opcional pero recomendable

- Diodo Schottky (1N5819) en serie con el + de la bateria: protege si alguien la conecta al reves (cae ~0.3 V).
- Interruptor accesible desde fuera de la carcasa.
- Mide el consumo total con el multimetro en serie: con LoRa transmitiendo + GPS + ESP32 debe ser ~150–200 mA.
  Una 9V alcalina (~500 mAh) alcanza de sobra para los minutos que dura el vuelo, pero no la dejes encendida horas.
