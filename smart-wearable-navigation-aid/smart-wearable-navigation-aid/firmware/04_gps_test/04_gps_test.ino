// 04 — Neo-6M GPS test + landmark coordinate capture
// Wiring: VCC->5V (GY-GPS6MV2 has a regulator), GND->GND, TX->GPIO39
// First fix outdoors can take 1–5 min (cold start). Keep the antenna facing up.
// Stand at a landmark, wait for >= 6 satellites, then type "a" to average
// 60 fixes — paste the printed line into firmware/navigation_aid/config.h
#include <TinyGPSPlus.h>
HardwareSerial GPSSerial(2);
TinyGPSPlus gps;
uint32_t tPrint = 0;

void setup() {
  Serial.begin(115200); delay(500);
  GPSSerial.begin(9600, SERIAL_8N1, 39, -1);
  Serial.println("Waiting for fix... type 'a' to average 60 fixes");
}

void average() {
  double la = 0, lo = 0; int n = 0; uint32_t t = millis();
  while (n < 60 && millis() - t < 90000) {
    while (GPSSerial.available()) gps.encode(GPSSerial.read());
    if (gps.location.isUpdated() && gps.location.isValid()) { la += gps.location.lat(); lo += gps.location.lng(); n++; }
  }
  if (n) Serial.printf("  { \"NAME\", %.6f, %.6f, TRK_LANDMARK_X },   // %d fixes\n", la / n, lo / n, n);
  else   Serial.println("No valid fixes collected");
}

void loop() {
  while (GPSSerial.available()) gps.encode(GPSSerial.read());
  if (Serial.available() && Serial.read() == 'a') average();
  if (millis() - tPrint > 1000) {
    tPrint = millis();
    Serial.printf("chars=%lu sats=%u hdop=%.1f fix=%s lat=%.6f lon=%.6f\n",
                  (unsigned long)gps.charsProcessed(), (unsigned)gps.satellites.value(), gps.hdop.hdop(),
                  gps.location.isValid() ? "yes" : "no", gps.location.lat(), gps.location.lng());
    if (gps.charsProcessed() < 10 && millis() > 5000) Serial.println("  no NMEA data — check TX->GPIO39 and baud");
  }
}
