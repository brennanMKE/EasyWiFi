#ifndef STATUSLED_H
#define STATUSLED_H

#if defined(ESP32)
#include "Arduino.h"
#include "Macros.h"
#else
#error "This code is only intended to be compiled for ESP32 platforms."
#endif

// LED blink patterns for different states
enum LEDPattern {
    LED_OFF,              // LED off
    LED_SOLID,            // LED solid on
    LED_SLOW_BLINK,       // Slow blink (AP mode)
    LED_FAST_BLINK,       // Fast blink (connecting)
    LED_DOUBLE_BLINK,     // Double blink (error)
    LED_HEARTBEAT         // Heartbeat pattern (connected)
};

class StatusLED {
public:
    StatusLED(int pin = -1);
    
    void begin();
    void loop();
    void setPattern(LEDPattern pattern);
    void setBrightness(uint8_t brightness);
    bool isEnabled();

private:
    int ledPin;
    bool enabled;
    LEDPattern currentPattern;
    uint8_t brightness;
    unsigned long lastUpdate;
    bool ledState;
    int blinkCount;
    
    void updateLED();
    void setLEDState(bool state);
};

#endif // STATUSLED_H
