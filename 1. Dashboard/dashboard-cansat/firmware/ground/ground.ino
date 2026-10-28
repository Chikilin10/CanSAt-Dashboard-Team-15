/*
 * Estacion en tierra: ESP32 + LoRa conectado por USB a la PC.
 *   LoRa -> PC : imprime cada paquete recibido + ",rssi" (una linea)
 *   PC -> LoRa : cada linea que llegue por serial (DEPLOY, LOCK, TARE) se transmite al CanSat
 */
#include <SPI.h>
#include <LoRa.h>

#define LORA_SCK   5
#define LORA_MISO  19
#define LORA_MOSI  27
#define LORA_SS    18
#define LORA_RST   14
#define LORA_DIO0  26
#define LORA_FREQ  915E6
#define LORA_SF    7
#define LORA_BW    250E3
#define LORA_SYNC  0x42

void setup() {
  Serial.begin(115200);
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(LORA_FREQ)) { Serial.println("ERROR: LoRa no inicio"); while (true) delay(1000); }
  LoRa.setSpreadingFactor(LORA_SF);
  LoRa.setSignalBandwidth(LORA_BW);
  LoRa.setSyncWord(LORA_SYNC);
  LoRa.enableCrc();
  LoRa.receive();
  Serial.println("LISTO: estacion en tierra");
}

void loop() {
  if (LoRa.parsePacket()) {
    String msg = "";
    while (LoRa.available()) msg += (char)LoRa.read();
    msg.trim();
    Serial.print(msg);
    Serial.print(",");
    Serial.println(LoRa.packetRssi());
  }

  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.length()) {
      LoRa.beginPacket();
      LoRa.print(cmd);
      LoRa.endPacket();      // bloqueante: son mensajes cortos
      LoRa.receive();
    }
  }
}
