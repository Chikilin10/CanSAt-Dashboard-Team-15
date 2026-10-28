/*
 * CanSat - firmware de vuelo (ESP32 + LoRa + BMP280 + MPU6050 + GPS + servo)
 *
 * Librerias (Gestor de librerias de Arduino IDE):
 *   LoRa (Sandeep Mistry), Adafruit BMP280, Adafruit MPU6050, Adafruit Unified Sensor,
 *   TinyGPSPlus, ESP32Servo        (Preferences y esp_system ya vienen con el ESP32)
 *
 * COMO SE MIDE LA ALTURA
 *   La altura es BAROMETRICA y RELATIVA: 0 m = el lugar donde se enciende la placa.
 *   Al arrancar promedia 20 lecturas de presion (p0) y de ahi calcula  alt = f(presion, p0).
 *   => ENCENDER SIEMPRE EN EL SUELO, en la plataforma, antes de subir al dron.
 *
 * CUANDO SE SUELTA LA CARGA SECUNDARIA (solo depende de la altura)
 *   1. "Armado": la altura debe haber superado ALT_ARMADO (260 m). Mientras sube con el dron, pasa por 210 m
 *      pero NO dispara porque aun no esta armado.
 *   2. Ya armado, en cuanto la altura baje a ALT_DESPLIEGUE (210 m) o menos -> servo.
 *   No hay temporizadores. Respaldo manual: el comando DEPLOY desde el dashboard.
 *
 * SI LA PLACA SE REINICIA EN VUELO (ej. caida de voltaje por el servo)
 *   Se guardan en memoria flash: p0, fase y si ya estaba armado/desplegado. Al reiniciar, si la causa NO fue un
 *   encendido normal y la altura calculada con el p0 guardado es > 40 m, retoma el vuelo donde iba
 *   en lugar de recalibrar (que pondria "0 m" en pleno aire y nunca desplegaria).
 *
 * Formato de cada paquete (ver docs/protocol.md):
 *   T,packet,ms,pressure,temp,ax,ay,az,gx,gy,gz,volt,rotor,sec_dist,sec_bearing,apk,lat,lon,sats
 */
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Preferences.h>
#include <esp_system.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_MPU6050.h>
#include <TinyGPSPlus.h>
#include <ESP32Servo.h>

// ======================= PINES (verifica con el pinout de TU placa) =======================
#define LORA_SCK   5
#define LORA_MISO  19
#define LORA_MOSI  27
#define LORA_SS    18
#define LORA_RST   14
#define LORA_DIO0  26
#define I2C_SDA    21
#define I2C_SCL    22
#define GPS_RX     13   // pin del ESP32 donde llega el TX del GPS
#define GPS_TX     17
#define PIN_SERVO  33
#define PIN_VBAT   34   // ADC (solo entrada) a traves del divisor 100k / 33k. NUNCA conectar los 9V directo.

// ======================= RADIO (igual en tierra y en el CanSat) =======================
#define LORA_FREQ  915E6
#define LORA_SF    7
#define LORA_BW    250E3
#define LORA_SYNC  0x42

// ======================= MISION =======================
const int   ANGULO_BLOQUEADO = 0;       // servo sujetando el seguro   (calibrar con la pieza real: sketch 05_servo)
const int   ANGULO_LIBERADO  = 90;      // servo liberando el seguro   (calibrar con la pieza real)
const float ALT_DESPLIEGUE   = 210.0;   // m. El reglamento pide 200 m; se dispara un poco antes porque el servo tarda
                                        //    ~0.3 s y en caida libre se bajan ~10 m en ese tiempo (AJUSTAR con pruebas).
const float ALT_ARMADO       = 260.0;   // m. Debe superarse antes de poder desplegar.
const float ALT_DESPEGUE     = 20.0;    // m. Arriba de esto se considera "en vuelo".
const float ALT_REINICIO     = 40.0;    // m. Para decidir si un reinicio ocurrio en vuelo.
const float VBAT_DIVISOR     = 4.03;    // (R1+R2)/R2 con R1=100k, R2=33k. Ajustar comparando con un multimetro.
const unsigned long TX_PERIODO_MS = 500; // 2 paquetes por segundo

enum Fase { EN_TIERRA, ASCENSO, DESPLEGADO };
Fase fase = EN_TIERRA;

Adafruit_BMP280 bmp;
Adafruit_MPU6050 mpu;
TinyGPSPlus gps;
Servo servo;
Preferences prefs;

float p0 = 1013.25;                    // presion de referencia ("0 m"), hPa
float presion = 0, temp = 0, altura = 0, altMax = 0;
float ax = 0, ay = 0, az = 0, gx = 0, gy = 0, gz = 0;
float apk = 1.0;                       // pico de aceleracion (g) desde el ultimo paquete
bool  rotor = false, armado = false;
unsigned long paqueteId = 0, tDespliegue = 0, tIMU = 0, tBaro = 0, tTx = 0;
volatile bool txTerminada = false;

// ----------------------------------------------------------------------------------------
float altitudDesde(float p, float ref) { return 44330.0f * (1.0f - powf(p / ref, 0.1903f)); }

// Guardar en flash SOLO en cambios de estado (la escritura tarda unos ms; no se hace en cada ciclo).
void guardarVuelo() {
  prefs.putFloat("p0", p0);
  prefs.putFloat("altMax", altMax);
  prefs.putUChar("fase", (uint8_t)fase);
}
void limpiarVuelo() { prefs.putUChar("fase", EN_TIERRA); }

void desplegar() {
  if (rotor) return;
  if (!servo.attached()) servo.attach(PIN_SERVO);
  servo.write(ANGULO_LIBERADO);
  rotor = true;
  fase = DESPLEGADO;
  tDespliegue = millis();
  guardarVuelo();
}

void bloquear() {                      // pruebas en mesa: vuelve al estado inicial
  if (!servo.attached()) servo.attach(PIN_SERVO);
  servo.write(ANGULO_BLOQUEADO);
  rotor = false; armado = false; fase = EN_TIERRA; altMax = 0;
  limpiarVuelo();
}

void calibrarPresion() {
  float suma = 0;
  for (int i = 0; i < 20; i++) { suma += bmp.readPressure() / 100.0; delay(20); }
  p0 = suma / 20.0;
  altura = 0;
}

float leerVoltaje() { return analogReadMilliVolts(PIN_VBAT) / 1000.0 * VBAT_DIVISOR; }

// ----------------------------------------------------------------------------------------
// IMU a 200 Hz: guarda el valor reciente y el PICO de aceleracion (el impacto dura milisegundos;
// a 2 Hz de radio se perderia si solo mandaramos la ultima lectura).
void leerIMU() {
  sensors_event_t a, g, t;
  mpu.getEvent(&a, &g, &t);
  ax = a.acceleration.x; ay = a.acceleration.y; az = a.acceleration.z;
  gx = g.gyro.x * RAD_TO_DEG; gy = g.gyro.y * RAD_TO_DEG; gz = g.gyro.z * RAD_TO_DEG;
  float mag = sqrtf(ax*ax + ay*ay + az*az) / 9.80665f;
  if (mag > apk) apk = mag;
}

// Barometro a 20 Hz + decision de despliegue (SOLO por altura)
void leerBaroYDecidir() {
  presion = bmp.readPressure() / 100.0;
  temp = bmp.readTemperature();
  altura += 0.3f * (altitudDesde(presion, p0) - altura);   // filtro suave

  if (fase == EN_TIERRA && altura > ALT_DESPEGUE) { fase = ASCENSO; guardarVuelo(); }

  if (fase == ASCENSO && altura > altMax) {
    altMax = altura;
    if (!armado && altMax >= ALT_ARMADO) { armado = true; guardarVuelo(); }
  }

  if (armado && !rotor && altura <= ALT_DESPLIEGUE) desplegar();
}

// ----------------------------------------------------------------------------------------
void revisarComandos() {
  if (!LoRa.parsePacket()) return;
  String cmd = "";
  while (LoRa.available()) cmd += (char)LoRa.read();
  cmd.trim();
  if (cmd == "DEPLOY" && fase != EN_TIERRA)    desplegar();            // nunca en la plataforma
  else if (cmd == "LOCK")                      { bloquear(); calibrarPresion(); }
  else if (cmd == "TARE" && fase == EN_TIERRA) calibrarPresion();
}

void enviarTelemetria() {
  char buf[200];
  char lat[16] = "", lon[16] = "";
  if (gps.location.isValid()) {
    snprintf(lat, sizeof lat, "%.6f", gps.location.lat());
    snprintf(lon, sizeof lon, "%.6f", gps.location.lng());
  }
  int n = snprintf(buf, sizeof buf,
    "T,%lu,%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.1f,%.1f,%.1f,%.2f,%d,,,%.2f,%s,%s,%d",
    paqueteId++, millis(), presion, temp, ax, ay, az, gx, gy, gz,
    leerVoltaje(), rotor ? 1 : 0,          // sec_dist y sec_bearing vacios = sin dato (ver protocol.md)
    apk, lat, lon, (int)gps.satellites.value());
  apk = 1.0;                               // reinicia el pico para el siguiente paquete

  LoRa.beginPacket();
  LoRa.write((uint8_t*)buf, n);
  LoRa.endPacket(true);                    // true = asincrono: no bloquea el muestreo de la IMU
}

// ----------------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);
  Serial2.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  analogReadResolution(12);
  prefs.begin("cansat", false);

  if (!bmp.begin(0x76) && !bmp.begin(0x77)) Serial.println("ERROR: BMP280 no encontrado");
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL, Adafruit_BMP280::SAMPLING_X2,
                  Adafruit_BMP280::SAMPLING_X16, Adafruit_BMP280::FILTER_X4,
                  Adafruit_BMP280::STANDBY_MS_1);
  if (!mpu.begin()) Serial.println("ERROR: MPU6050 no encontrado");
  mpu.setAccelerometerRange(MPU6050_RANGE_16_G);   // para no saturar en el impacto
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_260_HZ);

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(LORA_FREQ)) Serial.println("ERROR: LoRa no inicio");
  LoRa.setSpreadingFactor(LORA_SF);
  LoRa.setSignalBandwidth(LORA_BW);
  LoRa.setSyncWord(LORA_SYNC);
  LoRa.enableCrc();                        // descarta paquetes corruptos
  LoRa.onTxDone([]() { txTerminada = true; });

  // --- ¿Es un reinicio en pleno vuelo? ---
  delay(300);
  uint8_t faseGuardada = prefs.getUChar("fase", EN_TIERRA);
  float   p0Guardado   = prefs.getFloat("p0", 0);
  bool enVuelo = false;
  if (faseGuardada != EN_TIERRA && p0Guardado > 300 && esp_reset_reason() != ESP_RST_POWERON) {
    float altAhora = altitudDesde(bmp.readPressure() / 100.0, p0Guardado);
    if (altAhora > ALT_REINICIO) { enVuelo = true; altura = altAhora; }
  }

  servo.attach(PIN_SERVO);
  if (enVuelo) {
    p0 = p0Guardado;
    fase = (Fase)faseGuardada;
    altMax = prefs.getFloat("altMax", 0);
    armado = altMax >= ALT_ARMADO;
    rotor = (fase == DESPLEGADO);
    servo.write(rotor ? ANGULO_LIBERADO : ANGULO_BLOQUEADO);   // conserva el estado del servo
    tDespliegue = millis();
    Serial.println("REINICIO EN VUELO: se conserva la referencia de altura");
  } else {
    servo.write(ANGULO_BLOQUEADO);         // arranque normal: siempre bloqueado
    limpiarVuelo();
    delay(700);
    calibrarPresion();                     // 0 m = aqui
  }
  LoRa.receive();
}

void loop() {
  while (Serial2.available()) gps.encode(Serial2.read());   // el GPS se lee SIEMPRE, sin delay()

  unsigned long ahora = millis();
  if (ahora - tIMU >= 5)    { tIMU = ahora;  leerIMU(); }           // 200 Hz
  if (ahora - tBaro >= 50)  { tBaro = ahora; leerBaroYDecidir(); }  // 20 Hz

  if (txTerminada) { txTerminada = false; LoRa.receive(); }         // despues de transmitir, a escuchar
  if (ahora - tTx >= TX_PERIODO_MS && !LoRa.isTransmitting()) {
    tTx = ahora;
    enviarTelemetria();
  }
  revisarComandos();

  // Suelta el servo 2 s despues de desplegar (evita vibracion y consumo)
  if (rotor && servo.attached() && ahora - tDespliegue > 2000) servo.detach();
}
