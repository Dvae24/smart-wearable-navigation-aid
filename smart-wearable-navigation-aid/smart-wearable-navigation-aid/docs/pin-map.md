# Pin map & wiring — AIDEEPEN ESP32-S3-CAM (HW-679)

> **Status: proposed, not yet validated on hardware.** Every peripheral line below must pass its
> `firmware/0X_*_test` sketch on breadboard before soldering. Camera pins are confirmed working.

![ESP32-S3-CAM header pinout](images/esp32-s3-cam-pinout.png)

## Peripheral wiring

| Peripheral | Module pin | ESP32-S3-CAM | Notes |
|---|---|---|---|
| HC-SR04 | VCC | 5V | 5 V module |
| | TRIG | GPIO14 | 3.3 V trigger is enough |
| | ECHO | GPIO38 **via 1 kΩ / 2 kΩ divider** | ECHO outputs 5 V — never connect it direct |
| | GND | GND | |
| VL53L0X (GY-530) | VIN | 3.3V | |
| | SDA | GPIO41 | own I²C bus (`Wire`), camera keeps GPIO4/5 |
| | SCL | GPIO40 | |
| | GND | GND | |
| DFPlayer Mini | VCC | 5V | |
| | RX | GPIO47 **via 1 kΩ** | resistor cuts the hiss/noise |
| | DAC_R | earbud tip | single wired earbud (one ear stays open) |
| | GND | GND + earbud sleeve | |
| Neo-6M (GY-GPS6MV2) | VCC | 5V | module has its own regulator |
| | TX | GPIO39 | ESP → GPS line not needed |
| | GND | GND | |
| Vibration motor | — | GPIO19 → 1 kΩ → NPN base | S8050 / 2N2222, 1N4148 flyback diode across motor |
| User button | — | GPIO0 ↔ GND | internal pull-up; press = nearest-landmark distance |
| Buzzer (optional) | — | GPIO20 → transistor | disabled by default (`ENABLE_BUZZER 0`) |
| Status LED | on-board | GPIO2 | 1 Hz heartbeat |

Power: power bank → 5V/GND of the board. Put a **100–470 µF capacitor across 5V/GND** near the board — camera
init draws a current spike that can brown-out-reset the ESP32 on weak power banks.

## Why these pins (the short version)

- **Only header-exposed GPIOs are free:** 14, 47, 38, 39, 40, 41, 0, 19, 20 (+ RX/TX 43/44 for Serial Monitor).
  Everything else is used by the camera or the PSRAM.
- **39 / 40 / 41 are shared with the on-board micro SD slot.** We don't use the SD slot (the hazard log goes to
  internal flash via LittleFS), so **leave the SD slot empty**.
- **19 / 20 are the native USB D-/D+.** Fine as GPIO because we program through the UART
  (CH340) programmer board with *USB CDC On Boot = Disabled*.
- **GPIO0 is the BOOT strap.** Using it as an input button is safe — just don't hold it while pressing reset.
- **UART pins:** `HardwareSerial.begin()` falls back to *default* UART pins only when *both* RX and TX are `-1`.
  The S3 defaults collide with camera pins, so always pass at least one real pin (we do).

## Pin budget

| Used by | GPIOs |
|---|---|
| Camera (confirmed) | 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16, 17, 18 |
| Peripherals | 14, 38, 39, 40, 41, 47, 19, 0 |
| Spare | 20 (buzzer, optional) |
| Serial Monitor | 43 (TX), 44 (RX) |
