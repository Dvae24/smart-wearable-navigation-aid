# Stair dataset (Roboflow)

- Project: public, **CC BY 4.0**, object detection (bounding boxes)
- Classes: `upstairs`, `downstairs` (the `obstacle` class was dropped — the HC-SR04 already handles it)

## Collection rules

1. Capture **from the wrist-mounted position**, not phone/eye height — the angle is very different.
   Best: capture frames with the actual ESP32-S3-CAM mounted on the wrist.
2. Minimum **200** images, **360** recommended, roughly balanced between the two classes.
3. Vary: distance (1–4 m), angle (straight / diagonal), lighting (daylight, indoor, dim), stair material.
4. Include hard cases on purpose: partial occlusion (feet, bags, debris), people on stairs, handrails.
5. Add ~10–15 % **negative** images (hallways, ramps, tiled floors) with no labels.
6. Split **70 / 20 / 10** (train / valid / test) and never tune on the test split.

## Export path

Roboflow → export → Edge Impulse (or TFLite) → int8 quantised, 96×96 grayscale input to fit the ESP32-S3.
Evaluate with mAP@0.5, precision, recall, and a confusion matrix on the test split.
