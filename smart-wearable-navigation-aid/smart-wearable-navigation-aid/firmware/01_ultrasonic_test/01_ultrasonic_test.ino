// 01 — HC-SR04 forward obstacle test
// Wiring: VCC->5V, GND->GND, TRIG->GPIO14, ECHO->1k->GPIO38->2k->GND (divider!)
// Pass: readings within ~±2 cm of a tape measure at 30/50/100/150/200 cm.
// Output is CSV so you can paste it straight into data/templates/ultrasonic_accuracy.csv
#define TRIG 14
#define ECHO 38
#define MAX_CM 300

int readCm() {
  digitalWrite(TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  unsigned long us = pulseIn(ECHO, HIGH, (MAX_CM + 20) * 58UL);
  return us ? (int)(us / 58) : -1;   // -1 = nothing in range
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG, OUTPUT); pinMode(ECHO, INPUT);
  Serial.println("millis,distance_cm");
}

void loop() {
  Serial.printf("%lu,%d\n", (unsigned long)millis(), readCm());
  delay(100);   // >= 60 ms between pings to avoid echo overlap
}
