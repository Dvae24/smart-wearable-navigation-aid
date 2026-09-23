// 03 — DFPlayer Mini + single wired earbud test
// Wiring: VCC->5V, GND->GND, RX<-1k<-GPIO47, DAC_R->earbud tip, GND->earbud sleeve
// SD card (on the DFPlayer, FAT32): /mp3/0001.mp3 ... /mp3/0013.mp3 (see docs/audio-tracks.md)
// Type a track number (1-13) in Serial Monitor and press Enter. "v20" sets volume 20.
#include <DFRobotDFPlayerMini.h>
HardwareSerial DFSerial(1);
DFRobotDFPlayerMini dfp;

void setup() {
  Serial.begin(115200); delay(500);
  DFSerial.begin(9600, SERIAL_8N1, -1, 47);
  delay(500);
  if (!dfp.begin(DFSerial, false, true)) Serial.println("DFPlayer begin() returned false — check wiring/SD");
  dfp.volume(22);
  Serial.println("Enter track number 1-13, or vNN for volume");
}

void loop() {
  if (!Serial.available()) return;
  String s = Serial.readStringUntil('\n'); s.trim();
  if (s.startsWith("v")) { dfp.volume(s.substring(1).toInt()); Serial.println("volume set"); }
  else if (s.toInt() > 0) { dfp.playMp3Folder(s.toInt()); Serial.printf("playing %d\n", (int)s.toInt()); }
}
