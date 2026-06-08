#include <Arduino.h>
#include <EasyWiFi.h>
#include <esp_log.h>
#include "LanternsPages.h"

static const char *TAG = EWIFI_TAG_MAIN;

RunLoop runloop;
LanternsPages* lanternsPages = nullptr;

void setup() {
    Serial.begin(115200);
    delay(500);
    
    esp_log_level_set(TAG, ESP_LOG_VERBOSE);
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "EasyWiFi v0.2.0");
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
    
    // ========================================
    // Register Custom Pages
    // ========================================
    // Create and register your custom page handler
    // This keeps your device-specific code separate from EasyWiFi
    ESP_LOGI(TAG, "Registering custom pages for Lanterns functionality...");
    lanternsPages = new LanternsPages(runloop.getConfigServer());
    runloop.getConfigServer().registerCustomHandler(lanternsPages);
    
    ESP_LOGI(TAG, "Setup complete!");
    ESP_LOGI(TAG, "Custom pages available at:");
    ESP_LOGI(TAG, "  - http://lanterns-xxxxxx.local/lanterns");
    ESP_LOGI(TAG, "  - http://192.168.x.x/lanterns");
}

void loop() {
    runloop.loop();
}