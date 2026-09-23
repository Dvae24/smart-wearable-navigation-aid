# Testing protocol

Build order is strict: **validate each sensor alone → integrate → package → test with respondents.**
Never package before firmware is stable. Data templates are in `data/templates/`.

## Phase 1 — Individual sensor validation (breadboard)

| Test | Sketch | Procedure | Pass criterion |
|---|---|---|---|
| Camera | `00_camera_test` | cover/uncover lens | PID 0x2145, 320×240 frames, brightness changes |
| HC-SR04 accuracy | `01_ultrasonic_test` | wall at 30/50/80/100/150/200 cm, 10 readings each | mean abs error ≤ 3 cm up to 200 cm |
| VL53L0X drop | `02_tof_test` | sweep over a 15 cm step edge at wrist height | reading jumps > 150 mm or goes out of range |
| Audio | `03_audio_test` | play tracks 1–13 | all tracks audible in the earbud, no hiss |
| GPS | `04_gps_test` | outdoors, open sky | fix within 5 min, ≥ 6 satellites |
| Haptics + button | `05_haptic_button_test` | press button | three patterns distinguishable by touch |

## Phase 2 — Integrated system (controlled environment, campus)

| Test | Trials | Metric | Template |
|---|---|---|---|
| Obstacle detection | 30 (10 each at L/C/R) | detection rate, direction-suggestion accuracy | `obstacle_direction_trials.csv` |
| Drop detection | 20 drop + 20 flat-floor controls | detection rate, **false-alarm rate** | `drop_detection_trials.csv` |
| Response time | 20 per event type | latency (trigger → alert), target ≤ 1 s | `response_time.csv` |
| Stairs cue (experimental) | 20 stair + 20 non-stair scenes | precision / recall | `drop_detection_trials.csv` (scenario col) |

Report **accuracy, precision, recall, false-alarm rate** — never only "it worked". Record lighting (indoor /
outdoor / dim) because the camera and ToF both depend on it.

Use `tools/serial_logger.py` to capture every run; the firmware prints every alert with its track number.

## Phase 3 — Stair model (if pursued)

- Dataset: ≥ 200 images (360 recommended), captured **from the wrist-mounted position**, varied angles,
  lighting, distances, with partial occlusions (feet, debris).
- Classes: `upstairs`, `downstairs` only.
- Evaluate on a **held-out test split**: mAP@0.5, precision, recall, confusion matrix. Training accuracy is
  not a result.

## Phase 4 — Respondent evaluation

- Blindfolded sighted pilot first (safety), then visually impaired respondents with a sighted spotter.
- Fixed course with marked obstacles, one step-down, one staircase.
- Usability survey (`usability_likert.csv`), 5-point Likert.
