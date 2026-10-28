# Dashboard CanSat – Equipo 15

```
frontend/   React + Vite (dashboard)
backend/c/  calculo de datos derivados (C)
backend/bridge/  puente serial -> C -> WebSocket (Node)
backend/tools/   simulador de vuelo para probar sin hardware
firmware/   codigo de las dos placas ESP32 (Arduino IDE)
docs/protocol.md   formato de los datos, comandos y como se mide la altura (leer primero)
docs/alimentacion.md   bateria de 9V, regulador y divisor de voltaje
firmware/tests/   un sketch de prueba por componente
data/       registro CSV de cada vuelo (lo crea el puente)
```

## Probar todo SIN hardware
```bash
cd backend/c && make && make test        # compila el backend en C y corre sus pruebas
cd ../bridge && npm install
python3 ../tools/sim_flight.py | SOURCE=stdin node index.js
# en otra terminal:
cd frontend && VITE_DATA_SOURCE=ws npm run dev
```

## Con el hardware
1. Cargar `firmware/cansat/cansat.ino` en la placa del CanSat y `firmware/ground/ground.ino` en la de tierra.
2. Conectar la de tierra por USB y correr: `SERIAL_PATH=COM3 node index.js` (en `backend/bridge`).
3. En `frontend/.env.development` poner `VITE_DATA_SOURCE=ws` y correr `npm run dev`.

## Antes de cada lanzamiento
1. Encender el CanSat **en el suelo** (ahi queda el 0 m) y arrancar el puente.
2. Confirmar en el dashboard: paquetes llegando, `Señal` razonable, satelites del GPS, bateria > 8.5 V.
3. Datos del vuelo: `data/vuelo-*.csv` (crudo) y `data/vuelo-*.jsonl` (con calculos), para las graficas del RAM.
