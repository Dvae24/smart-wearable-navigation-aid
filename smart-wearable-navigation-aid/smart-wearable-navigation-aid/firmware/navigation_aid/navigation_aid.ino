// =====================================================================
//  Smart Wearable Navigation Aid for the Visually Impaired
//  Integrated firmware — embedded sensor-fusion system (no network)
//
//  Board : AIDEEPEN ESP32-S3-CAM (ESP32-S3-N16R8, GC2145 camera)
//  IDE   : Arduino IDE, board "ESP32S3 Dev Module", PSRAM "OPI PSRAM",
//          Flash 16MB, USB CDC On Boot "Disabled", 115200 baud
//
//  Fusion model (Level 1, complementary, threshold-based):
//    HC-SR04  -> is something AHEAD?          (forward axis)
//    VL53L0X  -> did the FLOOR drop away?     (downward axis)
//    Camera   -> which SIDE is freer? stairs? (lateral / visual axis)
//    Neo-6M   -> WHERE did a hazard happen?   (logging + landmark distance)
//  Each sensor answers a different question; the decision logic combines
//  them with a fixed priority: drop > near obstacle > stairs cue.
//
//  Serial commands (115200): d = dump hazard log, c = clear log,
//                            r = recalibrate floor, s = status
// =====================================================================
#include <Arduino.h>
#include <Wire.h>
#include <LittleFS.h>
#include "esp_camera.h"
#include <Adafruit_VL53L0X.h>
#include <DFRobotDFPlayerMini.h>
#include <TinyGPSPlus.h>
#include "config.h"
#include "camera_zones.h"

// ---------------------------------------------------------------- objects
Adafruit_VL53L0X   tof;
DFRobotDFPlayerMini dfp;
TinyGPSPlus        gps;
HardwareSerial     DFSerial(1);
HardwareSerial     GPSSerial(2);

// ---------------------------------------------------------------- state
struct State {
  int      usCm        = US_MAX_CM + 1;  // forward distance
  int      tofMm       = -1;             // -1 = out of range
  int      floorMm     = 0;              // calibrated baseline
  uint8_t  dropHits    = 0;
  bool     dropActive  = false;
  ZoneResult zones     = {0, 0, 0, 0, 0};
  bool     camOk = false, tofOk = false, audioOk = false, fsOk = false;
} S;

uint32_t tUs = 0, tTof = 0, tCam = 0, tLed = 0;
uint32_t lastAlertAt = 0;
uint8_t  pendingTrack = 0; uint32_t pendingAt = 0;
uint32_t lastLogAt[4] = {0, 0, 0, 0};

// ================================================================ camera
bool initCamera() {
  camera_config_t c = {};
  c.pin_pwdn = CAM_PIN_PWDN;   c.pin_reset = CAM_PIN_RESET;
  c.pin_xclk = CAM_PIN_XCLK;   c.pin_sccb_sda = CAM_PIN_SIOD; c.pin_sccb_scl = CAM_PIN_SIOC;
  c.pin_d7 = CAM_PIN_Y9; c.pin_d6 = CAM_PIN_Y8; c.pin_d5 = CAM_PIN_Y7; c.pin_d4 = CAM_PIN_Y6;
  c.pin_d3 = CAM_PIN_Y5; c.pin_d2 = CAM_PIN_Y4; c.pin_d1 = CAM_PIN_Y3; c.pin_d0 = CAM_PIN_Y2;
  c.pin_vsync = CAM_PIN_VSYNC; c.pin_href = CAM_PIN_HREF; c.pin_pclk = CAM_PIN_PCLK;
  c.xclk_freq_hz = 20000000;
  c.ledc_timer = LEDC_TIMER_0; c.ledc_channel = LEDC_CHANNEL_0;
  c.pixel_format = PIXFORMAT_RGB565;       // GC2145 has no HW JPEG; RGB565 is fine
  c.frame_size   = FRAMESIZE_QVGA;         // 320x240
  c.fb_count     = 1;
  c.fb_location  = CAMERA_FB_IN_PSRAM;
  c.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;
  return esp_camera_init(&c) == ESP_OK;
}

void updateCamera() {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) return;
  if (fb->width == 320 && fb->height == 240) {
    rgb565ToGrayHalf(fb->buf, fb->width, fb->height);
    esp_camera_fb_return(fb);            // release early, analysis uses our copy
    analyzeZones(&S.zones);
  } else {
    esp_camera_fb_return(fb);
  }
}

// ================================================================ sensors
int readUltrasonicCm() {
  digitalWrite(PIN_US_TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(PIN_US_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(PIN_US_TRIG, LOW);
  unsigned long us = pulseIn(PIN_US_ECHO, HIGH, (US_MAX_CM + 20) * 58UL);
  if (us == 0) return US_MAX_CM + 1;       // timeout = nothing in range
  return (int)(us / 58);
}

int readTofMm() {
  VL53L0X_RangingMeasurementData_t m;
  tof.rangingTest(&m, false);
  if (m.RangeStatus == 4) return -1;       // phase fail / out of range
  return m.RangeMilliMeter;
}

void calibrateFloor() {
  Serial.println(F("[TOF] Calibrating floor baseline — hold arm in normal walking pose"));
  playNow(TRK_CALIBRATING);
  delay(1500);
  long sum = 0; int n = 0;
  for (int i = 0; i < TOF_CAL_SAMPLES; i++) {
    int mm = readTofMm();
    if (mm > 0 && mm < 2000) { sum += mm; n++; }
    delay(30);
  }
  S.floorMm = n >= TOF_CAL_SAMPLES / 2 ? (int)(sum / n) : 600;
  Serial.printf("[TOF] baseline = %d mm (%d/%d valid)%s\n", S.floorMm, n, TOF_CAL_SAMPLES,
                n >= TOF_CAL_SAMPLES / 2 ? "" : "  <-- FALLBACK used, check sensor angle");
}

void updateTof() {
  S.tofMm = readTofMm();
  bool drop = (S.tofMm < 0) ? TOF_OUT_OF_RANGE_IS_DROP
                            : (S.tofMm > S.floorMm + TOF_DROP_DELTA_MM);
  S.dropHits = drop ? min<int>(S.dropHits + 1, 255) : 0;
  S.dropActive = S.dropHits >= TOF_DROP_CONFIRM;
}

// ================================================================ feedback
void playNow(uint8_t track) {
#if ENABLE_AUDIO
  if (S.audioOk) dfp.playMp3Folder(track);   // plays /mp3/000N.mp3
#endif
  Serial.printf("[AUDIO] track %u\n", track);
}

// Plays one word now; an optional second word ~0.7 s later (e.g. "Obstacle" .. "Go left")
bool alert(uint8_t track, uint8_t followUp) {
  uint32_t now = millis();
  if (now - lastAlertAt < ALERT_COOLDOWN_MS) return false;
  lastAlertAt = now;
  playNow(track);
  if (followUp) { pendingTrack = followUp; pendingAt = now + 700; }
  return true;
}

// Haptics: proximity-coded pulses, like a car parking sensor.
// Closer obstacle -> shorter gap between pulses. Drop -> solid buzz.
struct Haptic { uint32_t nextToggle = 0; bool on = false; } H;

void updateHaptics() {
  uint32_t now = millis();
  if (S.dropActive) { digitalWrite(PIN_VIBRATION, HIGH); H.on = true; return; }
  if (S.usCm > US_WARN_CM) { digitalWrite(PIN_VIBRATION, LOW); H.on = false; return; }
  uint32_t gap = map(constrain(S.usCm, 20, US_WARN_CM), 20, US_WARN_CM, 60, 600);
  if (now >= H.nextToggle) {
    H.on = !H.on;
    digitalWrite(PIN_VIBRATION, H.on);
    H.nextToggle = now + (H.on ? 80 : gap);
  }
}

// ================================================================ GPS
double haversineM(double lat1, double lon1, double lat2, double lon2) {
  const double R = 6371000.0, d2r = PI / 180.0;
  double dLat = (lat2 - lat1) * d2r, dLon = (lon2 - lon1) * d2r;
  double a = sin(dLat / 2) * sin(dLat / 2) +
             cos(lat1 * d2r) * cos(lat2 * d2r) * sin(dLon / 2) * sin(dLon / 2);
  return 2 * R * atan2(sqrt(a), sqrt(1 - a));
}

bool gpsFix() { return gps.location.isValid() && gps.location.age() < 5000; }

void announceNearestLandmark() {
  if (!gpsFix()) { alert(TRK_NO_GPS, 0); Serial.println(F("[GPS] no fix")); return; }
  double best = 1e12; int bi = -1;
  for (size_t i = 0; i < LANDMARK_COUNT; i++) {
    if (LANDMARKS[i].lat == 0.0 && LANDMARKS[i].lon == 0.0) continue;  // placeholder
    double d = haversineM(gps.location.lat(), gps.location.lng(), LANDMARKS[i].lat, LANDMARKS[i].lon);
    if (d < best) { best = d; bi = (int)i; }
  }
  if (bi < 0) { Serial.println(F("[GPS] no landmark coordinates set in config.h")); return; }
  uint8_t bucket = best < 100 ? TRK_DIST_NEAR : best < 500 ? TRK_DIST_MID : TRK_DIST_FAR;
  Serial.printf("[GPS] nearest: %s, %.0f m\n", LANDMARKS[bi].name, best);
  lastAlertAt = 0;                       // user asked; don't swallow it
  alert(LANDMARKS[bi].track, bucket);
}

// ================================================================ logging
// CSV: millis,type,value,lat,lon,sats
void logHazard(uint8_t type, const char* name, int value) {
  uint32_t now = millis();
  if (!S.fsOk || now - lastLogAt[type] < GPS_LOG_MIN_GAP_MS) return;
  lastLogAt[type] = now;
  File f = LittleFS.open("/hazards.csv", FILE_APPEND);
  if (!f) return;
  if (gpsFix())
    f.printf("%lu,%s,%d,%.6f,%.6f,%u\n", (unsigned long)now, name, value,
             gps.location.lat(), gps.location.lng(), (unsigned)gps.satellites.value());
  else
    f.printf("%lu,%s,%d,,,0\n", (unsigned long)now, name, value);
  f.close();
}

// ================================================================ decision
void decide() {
  // 1) Drop / path discontinuity — highest priority (fall risk)
  if (S.dropActive) {
    if (alert(TRK_STEP_DOWN, 0)) logHazard(0, "DROP", S.tofMm);
    return;
  }
  // 2) Near obstacle — say it, then suggest the freer side from the camera
  if (S.usCm <= US_NEAR_CM) {
    uint8_t dir = 0;
#if ENABLE_CAMERA
    if (S.camOk) dir = S.zones.freerSide < 0 ? TRK_GO_LEFT
                      : S.zones.freerSide > 0 ? TRK_GO_RIGHT : 0;
#endif
    if (alert(TRK_OBSTACLE, dir)) logHazard(1, "OBSTACLE", S.usCm);
    return;
  }
  // 3) EXPERIMENTAL: ascending stairs cue (camera lines + something ahead)
#if ENABLE_CAMERA && ENABLE_STAIR_HEURISTIC
  if (S.camOk && S.zones.stairLines >= STAIR_MIN_LINES && S.usCm <= STAIR_MAX_CM) {
    if (alert(TRK_STAIRS, 0)) logHazard(2, "STAIRS_CUE", S.zones.stairLines);
  }
#endif
}

// ================================================================ serial
void handleSerial() {
  if (!Serial.available()) return;
  char c = Serial.read();
  if (c == 'd') {
    File f = LittleFS.open("/hazards.csv", FILE_READ);
    Serial.println(F("millis,type,value,lat,lon,sats"));
    if (f) { while (f.available()) Serial.write(f.read()); f.close(); }
    Serial.println(F("--- end ---"));
  } else if (c == 'c') {
    LittleFS.remove("/hazards.csv"); Serial.println(F("log cleared"));
  } else if (c == 'r') {
    if (S.tofOk) calibrateFloor();
  } else if (c == 's') {
    Serial.printf("US=%dcm TOF=%dmm floor=%dmm drop=%d | zones L%.2f C%.2f R%.2f side=%d lines=%d | "
                  "gps=%s sats=%u\n",
                  S.usCm, S.tofMm, S.floorMm, S.dropActive,
                  S.zones.left, S.zones.center, S.zones.right, S.zones.freerSide, S.zones.stairLines,
                  gpsFix() ? "fix" : "none", (unsigned)gps.satellites.value());
  }
}

// ================================================================ setup/loop
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println(F("\n=== Smart Wearable Navigation Aid ==="));

  pinMode(PIN_STATUS_LED, OUTPUT);
  pinMode(PIN_VIBRATION, OUTPUT); digitalWrite(PIN_VIBRATION, LOW);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
#if ENABLE_BUZZER
  pinMode(PIN_BUZZER, OUTPUT); digitalWrite(PIN_BUZZER, LOW);
#endif
#if ENABLE_ULTRASONIC
  pinMode(PIN_US_TRIG, OUTPUT); pinMode(PIN_US_ECHO, INPUT);
#endif

  // Camera first: its init draws a current spike. A weak power bank can
  // brown-out here — add a 100–470 uF cap across 5V/GND near the board.
#if ENABLE_CAMERA
  S.camOk = initCamera();
  Serial.printf("[CAM] %s\n", S.camOk ? "OK" : "FAILED");
  delay(200);
#endif

#if ENABLE_AUDIO
  DFSerial.begin(9600, SERIAL_8N1, PIN_DF_RX, PIN_DF_TX);
  delay(500);                                  // DFPlayer boot time
  S.audioOk = dfp.begin(DFSerial, /*isACK=*/false, /*doReset=*/true);
  if (S.audioOk) dfp.volume(AUDIO_VOLUME);
  Serial.printf("[AUDIO] %s\n", S.audioOk ? "OK (no-ACK mode)" : "FAILED");
#endif

#if ENABLE_TOF
  Wire.begin(PIN_TOF_SDA, PIN_TOF_SCL);
  S.tofOk = tof.begin(VL53L0X_I2C_ADDR, false, &Wire);
  Serial.printf("[TOF] %s\n", S.tofOk ? "OK" : "FAILED");
  if (S.tofOk) calibrateFloor();
#endif

#if ENABLE_GPS
  GPSSerial.begin(9600, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
#endif

  S.fsOk = LittleFS.begin(true);
  Serial.printf("[FS] %s\n", S.fsOk ? "OK" : "FAILED");

  playNow(TRK_READY);
  Serial.println(F("Ready. Commands: d=dump log, c=clear, r=recalibrate, s=status"));
}

void loop() {
  uint32_t now = millis();

#if ENABLE_ULTRASONIC
  if (now - tUs >= US_PERIOD_MS) { tUs = now; S.usCm = readUltrasonicCm(); }
#endif
#if ENABLE_TOF
  if (S.tofOk && now - tTof >= TOF_PERIOD_MS) { tTof = now; updateTof(); }
#endif
#if ENABLE_CAMERA
  if (S.camOk && now - tCam >= CAM_PERIOD_MS) { tCam = now; updateCamera(); }
#endif
#if ENABLE_GPS
  while (GPSSerial.available()) gps.encode(GPSSerial.read());
#endif

  // Button: short press -> nearest landmark distance (simple debounce)
  static bool lastBtn = HIGH; static uint32_t btnAt = 0;
  bool b = digitalRead(PIN_BUTTON);
  if (b != lastBtn && now - btnAt > 50) {
    btnAt = now; lastBtn = b;
    if (b == LOW) announceNearestLandmark();
  }

  decide();
  updateHaptics();

  if (pendingTrack && (int32_t)(now - pendingAt) >= 0) { playNow(pendingTrack); pendingTrack = 0; }

  if (now - tLed >= 1000) { tLed = now; digitalWrite(PIN_STATUS_LED, !digitalRead(PIN_STATUS_LED)); }
  handleSerial();
}
