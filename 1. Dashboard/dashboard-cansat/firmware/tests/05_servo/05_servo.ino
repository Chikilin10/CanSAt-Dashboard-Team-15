// Calibracion del servo: escribe un angulo (0-180) en el Monitor Serial y presiona Enter.
// Anota el angulo con el seguro PUESTO (BLOQUEADO) y con el seguro LIBERADO; van en cansat.ino.
// Alimenta el servo desde el regulador/buck, NO desde el pin 3V3 del ESP32.
#include <ESP32Servo.h>
#define PIN_SERVO 33
Servo servo;
void setup() { Serial.begin(115200); servo.attach(PIN_SERVO); servo.write(0); Serial.println("Escribe un angulo:"); }
void loop() {
  if (Serial.available()) {
    int ang = constrain(Serial.parseInt(), 0, 180);
    servo.write(ang); Serial.printf("Angulo: %d\n", ang);
    while (Serial.available()) Serial.read();
  }
}
