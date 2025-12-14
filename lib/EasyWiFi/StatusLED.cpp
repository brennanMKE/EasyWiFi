#include "StatusLED.h"
#include <esp_log.h>

static const char *TAG = "STATUS_LED";

// Timing constants (milliseconds)
#define SLOW_BLINK_INTERVAL 1000
#define FAST_BLINK_INTERVAL 250
#define DOUBLE_BLINK_ON 100
#define DOUBLE_BLINK_OFF 100
#define DOUBLE_BLINK_PAUSE 800
#define HEARTBEAT_ON 100
#define HEARTBEAT_OFF_SHORT 100
#define HEARTBEAT_OFF_LONG 800

StatusLED::StatusLED(int pin) 
    : ledPin(pin), 
      enabled(false), 
      currentPattern(LED_OFF),
      brightness(255),
      lastUpdate(0),
      ledState(false),
      blinkCount(0) {
}

void StatusLED::begin() {
    if (ledPin < 0) {
        ESP_LOGI(TAG, "LED pin not configured, status LED disabled");
        enabled = false;
        return;
    }
    
    pinMode(ledPin, OUTPUT);
    setLEDState(false);
    enabled = true;
    
    ESP_LOGI(TAG, "Status LED initialized on pin %d", ledPin);
}

void StatusLED::loop() {
    if (!enabled) {
        return;
    }
    
    updateLED();
}

void StatusLED::setPattern(LEDPattern pattern) {
    if (currentPattern != pattern) {
        ESP_LOGV(TAG, "LED pattern changed: %d", pattern);
        currentPattern = pattern;
        lastUpdate = 0;
        blinkCount = 0;
        ledState = false;
    }
}

void StatusLED::setBrightness(uint8_t b) {
    brightness = b;
}

bool StatusLED::isEnabled() {
    return enabled;
}

void StatusLED::updateLED() {
    unsigned long now = millis();
    
    switch (currentPattern) {
        case LED_OFF:
            setLEDState(false);
            break;
            
        case LED_SOLID:
            setLEDState(true);
            break;
            
        case LED_SLOW_BLINK:
            if (now - lastUpdate >= SLOW_BLINK_INTERVAL) {
                ledState = !ledState;
                setLEDState(ledState);
                lastUpdate = now;
            }
            break;
            
        case LED_FAST_BLINK:
            if (now - lastUpdate >= FAST_BLINK_INTERVAL) {
                ledState = !ledState;
                setLEDState(ledState);
                lastUpdate = now;
            }
            break;
            
        case LED_DOUBLE_BLINK:
            // Pattern: ON-OFF-ON-OFF-PAUSE
            {
                unsigned long elapsed = now - lastUpdate;
                
                if (blinkCount == 0 && elapsed >= DOUBLE_BLINK_PAUSE) {
                    setLEDState(true);
                    lastUpdate = now;
                    blinkCount = 1;
                } else if (blinkCount == 1 && elapsed >= DOUBLE_BLINK_ON) {
                    setLEDState(false);
                    lastUpdate = now;
                    blinkCount = 2;
                } else if (blinkCount == 2 && elapsed >= DOUBLE_BLINK_OFF) {
                    setLEDState(true);
                    lastUpdate = now;
                    blinkCount = 3;
                } else if (blinkCount == 3 && elapsed >= DOUBLE_BLINK_ON) {
                    setLEDState(false);
                    lastUpdate = now;
                    blinkCount = 0;
                }
            }
            break;
            
        case LED_HEARTBEAT:
            // Pattern: ON-OFF-ON-OFF(long pause)
            {
                unsigned long elapsed = now - lastUpdate;
                
                if (blinkCount == 0 && elapsed >= HEARTBEAT_OFF_LONG) {
                    setLEDState(true);
                    lastUpdate = now;
                    blinkCount = 1;
                } else if (blinkCount == 1 && elapsed >= HEARTBEAT_ON) {
                    setLEDState(false);
                    lastUpdate = now;
                    blinkCount = 2;
                } else if (blinkCount == 2 && elapsed >= HEARTBEAT_OFF_SHORT) {
                    setLEDState(true);
                    lastUpdate = now;
                    blinkCount = 3;
                } else if (blinkCount == 3 && elapsed >= HEARTBEAT_ON) {
                    setLEDState(false);
                    lastUpdate = now;
                    blinkCount = 0;
                }
            }
            break;
    }
}

void StatusLED::setLEDState(bool state) {
    if (state) {
        analogWrite(ledPin, brightness);
    } else {
        analogWrite(ledPin, 0);
    }
    ledState = state;
}
