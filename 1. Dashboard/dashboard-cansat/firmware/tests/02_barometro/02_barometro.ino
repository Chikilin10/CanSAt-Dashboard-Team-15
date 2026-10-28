// BMP280: presion, temperatura y altura relativa. Sube/baja la placa con la mano: la altura debe cambiar ~1 m por metro.
#include <Wire.h>
#include <Adafruit_BMP280.h>
Adafruit_BMP280 bmp; float p0;
void setup() {
  Serial.begin(115200); Wire.begin(21, 22);
  if (!bmp.begin(0x76) && !bmp.begin(0x77)) { Serial.println("BMP280 no encontrado"); while (1); }
  delay(500); p0 = bmp.readPressure() / 100.0;
}
void loop() {
  float p = bmp.readPressure() / 100.0;
  Serial.printf("P=%.2f hPa  T=%.2f C  alt=%.2f m\n", p, bmp.readTemperature(), 44330.0 * (1.0 - pow(p / p0, 0.1903)));
  delay(200);
}
