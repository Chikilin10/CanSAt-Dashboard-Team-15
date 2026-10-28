// Receptor LoRa de prueba: imprime el mensaje y el RSSI. Lo importante para el requisito de 250 m:
// aleja las placas y anota hasta que distancia siguen llegando paquetes (RSSI mejor que -110 dBm = buen margen).
#include <SPI.h>
#include <LoRa.h>
void setup() {
  Serial.begin(115200);
  SPI.begin(5, 19, 27, 18); LoRa.setPins(18, 14, 26);
  if (!LoRa.begin(915E6)) { Serial.println("LoRa fallo"); while (1); }
  LoRa.setSpreadingFactor(7); LoRa.setSignalBandwidth(250E3); LoRa.setSyncWord(0x42); LoRa.enableCrc();
}
void loop() {
  if (LoRa.parsePacket()) {
    String m = ""; while (LoRa.available()) m += (char)LoRa.read();
    Serial.printf("%s   RSSI=%d  SNR=%.1f\n", m.c_str(), LoRa.packetRssi(), LoRa.packetSnr());
  }
}
