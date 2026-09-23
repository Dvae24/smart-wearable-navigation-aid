// 02 — VL53L0X downward drop-detection test
// Wiring: VIN->3.3V (or 5V if your GY-530 board has a regulator), GND->GND,
//         SDA->GPIO41, SCL->GPIO40. Do NOT insert a micro SD card in the
//         ESP32-S3-CAM slot — GPIO39/40/41 are shared with it.
// Pass: stable floor reading at wrist height; reading jumps by >150 mm
//       (or goes out of range) when the sensor passes over a step edge.
#include <Wire.h>
#include <Adafruit_VL53L0X.h>
Adafruit_VL53L0X tof;

void setup() {
  Serial.begin(115200); delay(500);
  Wire.begin(41, 40);
  if (!tof.begin(VL53L0X_I2C_ADDR, false, &Wire)) {
    Serial.println("VL53L0X not found — check wiring / run an I2C scan");
    while (true) delay(1000);
  }
  Serial.println("millis,range_mm,status");
}

void loop() {
  VL53L0X_RangingMeasurementData_t m;
  tof.rangingTest(&m, false);
  if (m.RangeStatus == 4) Serial.printf("%lu,,OUT_OF_RANGE\n", (unsigned long)millis());
  else                    Serial.printf("%lu,%u,OK\n", (unsigned long)millis(), m.RangeMilliMeter);
  delay(50);
}
