// Transmisor LoRa de prueba: manda un contador cada segundo. Carga 06_lora_rx en la otra placa.
// NUNCA transmitas sin la antena conectada.
#include <SPI.h>
#include <LoRa.h>
int n = 0;
void setup() {
  Serial.begin(115200);
  SPI.begin(5, 19, 27, 18); LoRa.setPins(18, 14, 26);
  if (!LoRa.begin(915E6)) { Serial.println("LoRa fallo"); while (1); }
  LoRa.setSpreadingFactor(7); LoRa.setSignalBandwidth(250E3); LoRa.setSyncWord(0x42); LoRa.enableCrc();
}
void loop() {
  LoRa.beginPacket(); LoRa.print("hola "); LoRa.print(n++); LoRa.endPacket();
  Serial.println(n); delay(1000);
}
