// GPS: muestra satelites y posicion. Ponlo AFUERA con vista al cielo; el primer fix puede tardar varios minutos.
// Si no sale nada: revisa que TX del GPS vaya al pin GPS_RX y que el baud sea 9600.
#include <TinyGPSPlus.h>
#define GPS_RX 13
#define GPS_TX 17
TinyGPSPlus gps; unsigned long t0;
void setup() { Serial.begin(115200); Serial2.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX); }
void loop() {
  while (Serial2.available()) gps.encode(Serial2.read());
  if (millis() - t0 > 1000) {
    t0 = millis();
    Serial.printf("chars=%lu sats=%d fix=%d lat=%.6f lon=%.6f\n", gps.charsProcessed(), (int)gps.satellites.value(),
                  gps.location.isValid(), gps.location.lat(), gps.location.lng());
  }
}
