// MPU6050: quieto sobre la mesa debe dar az ~ +9.8 m/s2 y giroscopio ~ 0. En caida libre (suelta sobre un cojin) todo ~ 0.
#include <Wire.h>
#include <Adafruit_MPU6050.h>
Adafruit_MPU6050 mpu;
void setup() {
  Serial.begin(115200); Wire.begin(21, 22);
  if (!mpu.begin()) { Serial.println("MPU6050 no encontrado"); while (1); }
  mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
}
void loop() {
  sensors_event_t a, g, t; mpu.getEvent(&a, &g, &t);
  Serial.printf("a=%.2f %.2f %.2f m/s2   g=%.1f %.1f %.1f deg/s\n", a.acceleration.x, a.acceleration.y, a.acceleration.z,
                g.gyro.x * RAD_TO_DEG, g.gyro.y * RAD_TO_DEG, g.gyro.z * RAD_TO_DEG);
  delay(200);
}
