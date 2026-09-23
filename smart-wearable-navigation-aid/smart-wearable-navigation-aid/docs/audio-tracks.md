# Audio tracks (DFPlayer Mini SD card)

Format the DFPlayer's micro SD card as **FAT32** and create a folder named `mp3`. Name the files
exactly as below — `playMp3Folder(n)` plays `/mp3/000n.mp3`, so copy order does not matter.

Keep every clip **short (≤ 1 s)**: the whole alert must fit a 1–2 s response window. Single words beat sentences.

| File | Says | Used when |
|---|---|---|
| 0001.mp3 | "Step down" | drop / path discontinuity (VL53L0X) |
| 0002.mp3 | "Obstacle" | forward obstacle ≤ 80 cm (HC-SR04) |
| 0003.mp3 | "Go left" | follows 0002 when camera says left is freer |
| 0004.mp3 | "Go right" | follows 0002 when camera says right is freer |
| 0005.mp3 | "Stairs ahead" | experimental ascending-stairs cue (camera) |
| 0006.mp3 | "Ready" | boot complete |
| 0007.mp3 | "No GPS signal" | button pressed without GPS fix |
| 0008.mp3 | "Cathedral" | landmark 1 name |
| 0009.mp3 | "St. Cruz" | landmark 2 name |
| 0010.mp3 | "Less than 100 meters" | landmark distance bucket |
| 0011.mp3 | "100 to 500 meters" | landmark distance bucket |
| 0012.mp3 | "More than 500 meters" | landmark distance bucket |
| 0013.mp3 | "Calibrating" | floor baseline calibration at boot |

Tip: generate the clips with any free text-to-speech tool (English or Cebuano/Filipino — whichever the
respondents prefer), export as MP3 mono 44.1 kHz, and trim silence at both ends.
