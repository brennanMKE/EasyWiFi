#include <Arduino.h>
#include <EasyWiFi.h>
#include <esp_log.h>

static const char *TAG = TAG_MAIN;

RunLoop runloop;

void setup() {
    Serial.begin(115200);
    delay(500);
    
    esp_log_level_set(TAG, ESP_LOG_VERBOSE);
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "EasyWiFi v0.1.0");
    ESP_LOGI(TAG, "ESP32-C3 WiFi Configuration System");
    ESP_LOGI(TAG, "========================================");
    
    runloop.setup();
}

void loop() {
    runloop.loop();
}