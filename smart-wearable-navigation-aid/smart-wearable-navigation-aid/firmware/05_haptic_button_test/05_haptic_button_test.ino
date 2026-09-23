// 05 — Vibration motor (via NPN transistor) + user button test
// Motor: GPIO19 -> 1k -> NPN base (S8050/2N2222); emitter->GND;
//        collector->motor(-); motor(+)->3.3V; 1N4148 across the motor (cathode to +)
// Button: GPIO0 -> button -> GND (internal pull-up). Don't hold it while resetting.
// Pass: each button press cycles through the three haptic patterns.
#define VIB 19
#define BTN 0

void pulse(int n, int on, int off) {
  for (int i = 0; i < n; i++) { digitalWrite(VIB, HIGH); delay(on); digitalWrite(VIB, LOW); delay(off); }
}

void setup() {
  Serial.begin(115200);
  pinMode(VIB, OUTPUT); digitalWrite(VIB, LOW);
  pinMode(BTN, INPUT_PULLUP);
  Serial.println("Press the button to cycle patterns");
}

void loop() {
  static int mode = 0;
  if (digitalRead(BTN) == LOW) {
    delay(30);
    while (digitalRead(BTN) == LOW) delay(5);
    mode = (mode + 1) % 3;
    Serial.printf("pattern %d\n", mode);
    if (mode == 0) pulse(5, 80, 500);   // far obstacle (slow)
    if (mode == 1) pulse(10, 80, 80);   // near obstacle (fast)
    if (mode == 2) pulse(1, 1000, 0);   // drop (solid)
  }
}
