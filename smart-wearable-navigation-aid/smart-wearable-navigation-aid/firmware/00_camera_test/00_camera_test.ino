// 00 — Camera bring-up test (GC2145 on ESP32-S3-CAM)
// Pass: prints "frame 320x240" every second with changing brightness when
// you cover/uncover the lens. Board: ESP32S3 Dev Module, OPI PSRAM, CDC off.
#include "esp_camera.h"

bool initCamera() {
  camera_config_t c = {};
  c.pin_pwdn = -1; c.pin_reset = -1; c.pin_xclk = 15;
  c.pin_sccb_sda = 4; c.pin_sccb_scl = 5;
  c.pin_d7 = 16; c.pin_d6 = 17; c.pin_d5 = 18; c.pin_d4 = 12;
  c.pin_d3 = 10; c.pin_d2 = 8;  c.pin_d1 = 9;  c.pin_d0 = 11;
  c.pin_vsync = 6; c.pin_href = 7; c.pin_pclk = 13;
  c.xclk_freq_hz = 20000000;
  c.ledc_timer = LEDC_TIMER_0; c.ledc_channel = LEDC_CHANNEL_0;
  c.pixel_format = PIXFORMAT_RGB565;
  c.frame_size = FRAMESIZE_QVGA;
  c.fb_count = 1;
  c.fb_location = CAMERA_FB_IN_PSRAM;
  c.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  return esp_camera_init(&c) == ESP_OK;
}

void setup() {
  Serial.begin(115200); delay(500);
  Serial.printf("PSRAM: %s (%u bytes free)\n", psramFound() ? "found" : "NOT FOUND", ESP.getFreePsram());
  if (!initCamera()) { Serial.println("Camera init FAILED"); while (true) delay(1000); }
  sensor_t* s = esp_camera_sensor_get();
  Serial.printf("Camera OK, PID=0x%04X (GC2145 = 0x2145)\n", s->id.PID);
}

void loop() {
  uint32_t t0 = millis();
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) { Serial.println("capture failed"); delay(1000); return; }
  uint32_t sum = 0, n = 0;
  for (size_t i = 0; i + 1 < fb->len; i += 64) {           // sample pixels
    uint16_t px = ((uint16_t)fb->buf[i] << 8) | fb->buf[i + 1];
    sum += ((px >> 5) & 0x3F) << 2; n++;
  }
  Serial.printf("frame %ux%u  len=%u  mean_green=%lu  grab=%lums\n",
                (unsigned)fb->width, (unsigned)fb->height, (unsigned)fb->len, (unsigned long)(sum / n), (unsigned long)(millis() - t0));
  esp_camera_fb_return(fb);
  delay(1000);
}
