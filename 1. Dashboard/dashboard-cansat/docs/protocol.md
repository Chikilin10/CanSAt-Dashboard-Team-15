# Protocolo de datos del CanSat

```
Sensores ─► ESP32 CanSat ─LoRa 915 MHz─► ESP32 tierra ─USB─► bridge (Node) ─► backend C ─► WebSocket ─► dashboard
```

## 1. Radio (CanSat → tierra): una linea CSV por paquete, 2 Hz

```
T,packet,ms,pressure,temp,ax,ay,az,gx,gy,gz,volt,rotor,sec_dist,sec_bearing,apk,lat,lon,sats
```

La estacion en tierra agrega `,rssi` al final antes de pasarla a la PC (campo 19).
**Campo vacio = sin dato** (se convierte en `null`). Ejemplo:

```
T,42,21000,1002.31,24.80,0.12,-0.05,9.81,1.2,-0.4,0.3,7.38,0,,,1.04,25.651234,-100.289456,8,-62
```

| # | Campo | Unidad | Notas |
|---|-------|--------|-------|
| 1 | packet | – | contador; el dashboard lo usa para calcular perdida |
| 2 | ms | ms | `millis()` del CanSat; el backend lo convierte a `t` en segundos |
| 3 | pressure | hPa | BMP280 |
| 4 | temp | °C | BMP280 |
| 5-7 | ax, ay, az | m/s² | MPU6050 (±16 g) |
| 8-10 | gx, gy, gz | °/s | MPU6050 |
| 11 | volt | V | bateria (divisor de voltaje → ADC) |
| 12 | rotor | 0/1 | 1 = seguro liberado / rotor desplegado |
| 13-14 | sec_dist, sec_bearing | m, ° | carga secundaria (pendiente de definir; vacio por ahora) |
| 15 | apk | g | pico de aceleracion desde el paquete anterior (muestreo a 200 Hz) |
| 16-17 | lat, lon | ° | vacio si el GPS no tiene fix |
| 18 | sats | – | satelites |
| 19 | rssi | dBm | agregado por la estacion en tierra |

## 2. Backend en C → dashboard: una linea JSON por paquete

`raw` = datos del CanSat (convertidos a unidades finales). `derived` = calculado en C.

```json
{"raw":{"packet":42,"t":21.00,"pressure":1002.31,"temp":24.80,"ax":0.12,"ay":-0.05,"az":9.81,
        "gx":1.2,"gy":-0.4,"gz":0.3,"volt":7.38,"rotor_deployed":false,
        "sec_dist":null,"sec_bearing":null,"lat":25.651234,"lon":-100.289456,"sats":8,"rssi":-62},
 "derived":{"alt":142.3,"vel_v":29.8,"alt_max":142.3,"phase":"ascent","deploy_alt":null,
            "descent_rate":null,"descent_ok":null,"impact_g":null,"post_landing_s":null}}
```

### Campos derivados

| Campo | Como se calcula |
|-------|-----------------|
| `alt` | altitud barometrica relativa a la presion promedio de los primeros 10 paquetes (en la plataforma), con filtro exponencial |
| `vel_v` | pendiente de `alt` en una ventana de ~1 s (+ sube / − baja) |
| `alt_max` | maximo de `alt` |
| `phase` | `ground` → `ascent` → `freefall` → `autogyro` → `landed` |
| `deploy_alt` | `alt` en el instante en que `rotor` pasa a 1 |
| `descent_rate` | `-vel_v`, solo durante `autogyro` |
| `descent_ok` | `descent_rate` entre 8 y 12 m/s |
| `impact_g` | maximo `apk` por debajo de 25 m de altura, se muestra al aterrizar |
| `post_landing_s` | segundos desde el aterrizaje (fechado al inicio de la quietud) |

Aterrizaje = `alt < 8 m` y `|vel_v| < 1.5 m/s` sostenido 1 s. Todos los umbrales estan en `backend/c/include/cansat.h`.

## 3. Comandos (dashboard → CanSat)

Texto plano por el mismo WebSocket. El puente los repite 3 veces (LoRa es half-duplex).
Los botones estan en `frontend/src/components/layout/CommandBar.jsx`.

| Comando | Efecto |
|---------|--------|
| `DEPLOY` | suelta la carga secundaria (respaldo manual; solo si ya despego) |
| `TARE` | pone la altura en 0 m (solo en tierra). El puente tambien reinicia el calculo en C |
| `LOCK` | vuelve a bloquear el servo y reinicia el estado (pruebas en mesa). Tambien reinicia el calculo en C |
| `RESET` | reinicia solo el backend en C (calibracion y fases); no viaja por radio |

## 4. Como se mide la altura y cuando se suelta la carga

- **0 m = donde se enciende el CanSat.** El firmware promedia 20 lecturas de presion al arrancar (`p0`); el backend en C
  hace lo mismo con los primeros 10 paquetes recibidos. **Encender siempre en el suelo, antes de subir al dron, con el puente ya corriendo.**
- **Disparo solo por altura:** el sistema se *arma* al superar 260 m; ya armado, al bajar a 210 m o menos se activa el servo
  (210 en vez de 200 para compensar los ~0.3 s del servo a ~40 m/s; se ajusta con pruebas). No hay temporizadores.
- **Reinicio en vuelo:** el firmware guarda `p0`, fase y estado en flash. Si la placa se reinicia (no por encendido normal)
  y la altura calculada con el `p0` guardado es > 40 m, retoma el vuelo sin recalibrar. El backend en C tambien lo tolera.
