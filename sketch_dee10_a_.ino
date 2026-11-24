#include "esp_camera.h"

#define BUZZER_PIN 13

// Sensor Thresholds
const int BLACKOUT_LIMIT = 20;     // Obstructed lens / darkness
const int BLINDING_LIMIT = 235;    // Laser / Flashlight
const float VARIANCE_LIMIT = 10.0; // Uniform color / Blurring / Paint

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  
  // AI-Thinker Camera Pin Map (Explicit Numbers)
  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 21;
  config.pin_d4 = 36;
  config.pin_d5 = 39;
  config.pin_d6 = 34;
  config.pin_d7 = 35;
  config.pin_xclk = 0;
  config.pin_pclk = 22;
  config.pin_vsync = 25;
  config.pin_href = 23;
  config.pin_sccb_sda = 26; // Serial camera control data (I2C SDA)
  config.pin_sccb_scl = 27; // Serial camera control clock (I2C SCL)
  config.pin_pwdn = 32;
  config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  
  config.pixel_format = PIXFORMAT_GRAYSCALE;
  config.frame_size = FRAMESIZE_QQVGA; 
  config.jpeg_quality = 12;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera initialization failed with error 0x%x\n", err);
    return;
  }
  Serial.println("System Ready! Optical Tamper Detection Active.");
}

void loop() {
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Frame capture failed");
    return;
  }

  // Calculate Average Brightness
  long totalBrightness = 0;
  size_t totalPixels = fb->len;
  for (size_t i = 0; i < totalPixels; i++) {
    totalBrightness += fb->buf[i];
  }
  float avgBrightness = (float)totalBrightness / totalPixels;

  // Calculate Texture Variance
  float varianceSum = 0;
  for (size_t i = 0; i < totalPixels; i++) {
    varianceSum += pow(fb->buf[i] - avgBrightness, 2);
  }
  float textureVariance = sqrt(varianceSum / totalPixels);

  Serial.printf("Brightness: %.2f | Texture Variance: %.2f --> ", avgBrightness, textureVariance);

  if (avgBrightness < BLACKOUT_LIMIT) {
    Serial.println("[ALERT] Lens Obstructed / Blackout!");
    digitalWrite(BUZZER_PIN, HIGH);
  } else if (avgBrightness > BLINDING_LIMIT) {
    Serial.println("[ALERT] Camera Blinded by Intense Light!");
    digitalWrite(BUZZER_PIN, HIGH);
  } else if (textureVariance < VARIANCE_LIMIT) {
    Serial.println("[ALERT] Visual Occlusion / Blurring Detected!");
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    Serial.println("STATUS: Normal");
    digitalWrite(BUZZER_PIN, LOW);
  }

  esp_camera_fb_return(fb);
  delay(1500);
}