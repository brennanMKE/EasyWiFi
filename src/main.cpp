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
    
    // Initialize with custom device name (or use default "EasyWiFi")
    // This name will be used for:
    // - AP SSID: MyDevice-Setup-XXXXXX
    // - mDNS hostname: mydevice-xxxxxx.local
    // - Web page titles and headers
    // 
    // Examples:
    //   runloop.setup("SmartLED");     // For LED controller
    //   runloop.setup("TempSensor");   // For temperature sensor
    //   runloop.setup("Garden IoT");   // For garden controller
    //   runloop.setup();               // Use default "EasyWiFi"
    //
    // See Docs/Customization.md for more details
    runloop.setup("Lanterns");  // Change to your device name
}

void loop() {
    runloop.loop();
}