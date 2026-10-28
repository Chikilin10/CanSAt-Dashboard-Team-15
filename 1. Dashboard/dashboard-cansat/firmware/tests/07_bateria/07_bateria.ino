// Lectura de la bateria de 9V con divisor (100k arriba, 33k abajo => divisor 4.03). Compara con un multimetro y ajusta VBAT_DIVISOR.
// NUNCA conectes los 9V directo al pin: el ESP32 solo soporta 3.3 V.
#define PIN_VBAT 34
#define VBAT_DIVISOR 4.03
void setup() { Serial.begin(115200); analogReadResolution(12); }
void loop() { Serial.printf("Bateria: %.2f V\n", analogReadMilliVolts(PIN_VBAT) / 1000.0 * VBAT_DIVISOR); delay(500); }
