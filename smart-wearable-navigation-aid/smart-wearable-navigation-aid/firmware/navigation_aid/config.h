// =====================================================================
//  config.h — single place for pins, thresholds, and feature flags
//  Board: AIDEEPEN ESP32-S3-CAM (HW-679 V0.0.1, ESP32-S3-N16R8, GC2145)
//  See docs/pin-map.md for wiring and the reasoning behind each pin.
// =====================================================================
#pragma once

// ---------- Feature flags (turn modules on as they pass bench tests) ----------
#define ENABLE_CAMERA           1   // lateral (left/right) zone analysis
#define ENABLE_STAIR_HEURISTIC  1   // EXPERIMENTAL ascending-stair edge cue (camera)
#define ENABLE_TOF              1   // VL53L0X downward drop detection
#define ENABLE_ULTRASONIC       1   // HC-SR04 forward obstacle
#define ENABLE_AUDIO            1   // DFPlayer Mini -> single wired earbud
#define ENABLE_GPS              1   // Neo-6M hazard logging + landmark distance
#define ENABLE_BUZZER           0   // optional; DFPlayer already covers audio

// ---------- Camera pins (confirmed working on this board) ----------
#define CAM_PIN_PWDN   -1
#define CAM_PIN_RESET  -1
#define CAM_PIN_XCLK   15
#define CAM_PIN_SIOD    4
#define CAM_PIN_SIOC    5
#define CAM_PIN_Y9     16
#define CAM_PIN_Y8     17
#define CAM_PIN_Y7     18
#define CAM_PIN_Y6     12
#define CAM_PIN_Y5     10
#define CAM_PIN_Y4      8
#define CAM_PIN_Y3      9
#define CAM_PIN_Y2     11
#define CAM_PIN_VSYNC   6
#define CAM_PIN_HREF    7
#define CAM_PIN_PCLK   13

// ---------- Peripheral pins (header-exposed GPIOs only) ----------
// HC-SR04 (5 V module). ECHO MUST go through a 1k/2k divider -> 3.3 V.
#define PIN_US_TRIG    14
#define PIN_US_ECHO    38
// VL53L0X on its own I2C bus (camera SCCB keeps GPIO4/5 to itself)
#define PIN_TOF_SDA    41
#define PIN_TOF_SCL    40
// UART note: never set BOTH rx and tx to -1 — the core then falls back to the
// S3 default UART pins, which collide with the camera.
// DFPlayer Mini (UART1). TX -> DFPlayer RX through a 1k resistor.
#define PIN_DF_TX      47
#define PIN_DF_RX      -1   // DFPlayer TX not needed (fire-and-forget)
// Neo-6M GPS (UART2). GPS TX -> ESP RX. ESP->GPS line not needed.
#define PIN_GPS_RX     39
#define PIN_GPS_TX     -1
// Vibration motor via NPN transistor (never drive the motor from the GPIO)
#define PIN_VIBRATION  19
// Optional buzzer via transistor
#define PIN_BUZZER     20
// User button (to GND). GPIO0 is the BOOT strap: don't hold it during reset.
#define PIN_BUTTON      0
// On-board status LED
#define PIN_STATUS_LED  2

// ---------- Timing ----------
#define US_PERIOD_MS          70    // HC-SR04 needs >= 60 ms between pings
#define TOF_PERIOD_MS         50
#define CAM_PERIOD_MS        250    // ~4 FPS is plenty for zone analysis
#define GPS_LOG_MIN_GAP_MS  5000    // don't log the same hazard every loop
#define ALERT_COOLDOWN_MS   1500    // 1–2 s audio response window

// ---------- Forward obstacle (HC-SR04) ----------
#define US_MAX_CM            300    // readings beyond this = "clear"
#define US_NEAR_CM            80    // strong alert
#define US_WARN_CM           150    // soft alert (haptic only)

// ---------- Drop detection (VL53L0X, angled down) ----------
#define TOF_CAL_SAMPLES       40    // baseline samples at boot (normal arm pose)
#define TOF_DROP_DELTA_MM    150    // reading > baseline + delta => drop
#define TOF_DROP_CONFIRM       3    // consecutive hits to confirm (debounce)
#define TOF_OUT_OF_RANGE_IS_DROP 1  // no return often means the floor fell away

// ---------- Camera zone analysis ----------
#define CAM_ROI_TOP_PCT        35   // ignore upper 35% of frame (sky/ceiling)
#define ZONE_EDGE_THRESH       40   // per-pixel gradient to count as an edge
#define ZONE_MIN_DIFF        0.03f  // min edge-density gap to prefer a side
// Stair heuristic: strong horizontal edge rows in the center zone
#define STAIR_ROW_PCT          45   // row = "line" if >= 45% of its center pixels are horizontal edges
#define STAIR_MIN_LINES         3
#define STAIR_MAX_CM          250   // only trust the cue when something is ahead

// ---------- Audio tracks on the SD card of the DFPlayer: /mp3/000N.mp3 ----------
enum Track : uint8_t {
  TRK_STEP_DOWN   = 1,
  TRK_OBSTACLE    = 2,
  TRK_GO_LEFT     = 3,
  TRK_GO_RIGHT    = 4,
  TRK_STAIRS      = 5,
  TRK_READY       = 6,
  TRK_NO_GPS      = 7,
  TRK_LANDMARK_1  = 8,
  TRK_LANDMARK_2  = 9,
  TRK_DIST_NEAR   = 10,  // "less than 100 meters"
  TRK_DIST_MID    = 11,  // "100 to 500 meters"
  TRK_DIST_FAR    = 12,  // "more than 500 meters"
  TRK_CALIBRATING = 13,
};
#define AUDIO_VOLUME  22   // 0..30

// ---------- Landmarks (Haversine distance) ----------
// PLACEHOLDERS — stand at each landmark with firmware/04_gps_test and
// paste the averaged fix here before any demo or data gathering.
struct Landmark { const char* name; double lat; double lon; uint8_t track; };
static const Landmark LANDMARKS[] = {
  { "Cathedral", 0.0, 0.0, TRK_LANDMARK_1 },
  { "St. Cruz",  0.0, 0.0, TRK_LANDMARK_2 },
};
static const size_t LANDMARK_COUNT = sizeof(LANDMARKS) / sizeof(LANDMARKS[0]);
