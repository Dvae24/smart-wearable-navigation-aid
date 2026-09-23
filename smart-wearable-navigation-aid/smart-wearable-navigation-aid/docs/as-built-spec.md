# As-built technical specification

**This page is the single source of truth for the manuscript.** If the paper and this page disagree, the
paper is wrong (or this page needs updating first). Update it whenever the hardware changes.

| Item | As built |
|---|---|
| Form factor | **Wrist / forearm mount** (not a glove) |
| Device class | Standalone embedded wearable — embedded sensor-fusion system (not IoT) |
| Processor | ESP32-S3-N16R8 (16 MB flash, 8 MB OPI PSRAM) on AIDEEPEN ESP32-S3-CAM (HW-679 V0.0.1) |
| Camera | GC2145, 320×240 RGB565, ~4 FPS analysis |
| Forward sensing | HC-SR04 ultrasonic, forearm, facing forward, 2–300 cm |
| Downward sensing | VL53L0X **laser time-of-flight** (not "infrared sensor"), wrist underside, angled down |
| Location | Neo-6M GPS — hazard-location logging + distance to preset landmarks (Haversine) |
| Audio | DFPlayer Mini → **single wired earbud** (one ear open for environmental awareness) |
| Haptics | Coin vibration motor, proximity-coded pulses |
| Input | One push button (landmark distance query) |
| Power | 5 V power bank |
| Connectivity | None (no Wi-Fi/Bluetooth/cloud) |
| Firmware | Arduino (ESP32 core), `firmware/navigation_aid` |

## Capabilities — status

| Capability | Method | Status |
|---|---|---|
| Forward obstacle warning | HC-SR04 threshold | implemented |
| Drop / path discontinuity detection (incl. descending stairs, curbs) | VL53L0X vs. calibrated baseline | implemented |
| Left/right movement suggestion | camera zone edge density | implemented |
| Haptic feedback near objects | proximity-coded vibration | implemented |
| Ascending stair detection | camera stair-line heuristic | **experimental** |
| Stair classification model | Roboflow `upstairs`/`downstairs` → TFLite Micro / Edge Impulse | **planned** |
| Landmark distance | GPS + Haversine | implemented (coordinates to be captured) |
| Turn-by-turn navigation | needs magnetometer | **out of scope** |
| Bluetooth earphones | conflicts with wired single-ear design | **out of scope** |

## Terminology to use in the paper

- "path discontinuity detection" / "drop detection" — **not** "hole detection"
- "laser-based time-of-flight sensor" — **not** "infrared sensor"
- "computer vision" / "lightweight image processing" — **not** "AI" (unless discussing the trained model)
- "Level 1 complementary sensor fusion (threshold-based)" — **not** Kalman / probabilistic fusion

## Manuscript items to reconcile

- [ ] Replace "glove" with wrist/forearm mount throughout
- [ ] Scope says GPS is excluded — GPS is in the build (logging + landmark distance)
- [ ] VL53L0X described as generic infrared → laser time-of-flight
- [ ] Ascending stair detection presented as settled → mark as experimental
- [ ] Respondent counts are inconsistent across chapters → pick one figure
- [ ] Remove "Future research: GPS integration"

## Open decisions (need adviser sign-off)

- [ ] Descope turn-by-turn navigation (no magnetometer) → landmark *distance* only
- [ ] Keep wired single earbud instead of Bluetooth earphones
- [ ] Ascending stairs: heuristic now, trained model if time allows
