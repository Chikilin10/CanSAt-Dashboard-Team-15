// Busca dispositivos I2C. Esperado: BMP280 en 0x76 (o 0x77) y MPU6050 en 0x68 (o 0x69).
#include <Wire.h>
void setup() {
  Serial.begin(115200); Wire.begin(21, 22); delay(500);
  for (byte a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) { Serial.print("Encontrado: 0x"); Serial.println(a, HEX); }
  }
  Serial.println("Listo.");
}
void loop() {}
