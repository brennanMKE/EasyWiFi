# Customizing EasyWiFi for Your Project

**Version**: 0.1.0  
**Updated**: December 14, 2024

---

## Overview

EasyWiFi is designed to be easily customized for different projects. The most common customization is changing the device name to match your project's branding.

---

## Device Name Customization

### What Gets Customized

When you set a custom device name, it affects:

1. **Access Point SSID**: `YourDevice-Setup-XXXXXX` (instead of `EasyWiFi-Setup-XXXXXX`)
2. **mDNS Hostname**: `yourdevice-xxxxxx.local` (instead of `easywifi-xxxxxx.local`)
3. **Web Page Titles**: Shows your device name in all web pages
4. **Log Messages**: Logs show your device name

The `XXXXXX` is the last 3 bytes of the device's MAC address for uniqueness.

---

## How to Set Custom Device Name

### In main.cpp

```cpp
#include <Arduino.h>
#include <EasyWiFi.h>

RunLoop runLoop;

void setup() {
    Serial.begin(115200);
    delay(500);
    
    // Set your custom device name here
    runLoop.setup("MyAwesomeDevice");  // Change this!
}

void loop() {
    runLoop.loop();
}
```

### Examples

#### Example 1: Smart LED Controller
```cpp
runLoop.setup("SmartLED");
```

**Result:**
- AP SSID: `SmartLED-Setup-8774BC`
- mDNS: `smartled-8774bc.local`
- Web page title: "SmartLED Configuration"

#### Example 2: Temperature Sensor
```cpp
runLoop.setup("TempSensor");
```

**Result:**
- AP SSID: `TempSensor-Setup-8774BC`
- mDNS: `tempsensor-8774bc.local`
- Web page title: "TempSensor Configuration"

#### Example 3: Garden Controller
```cpp
runLoop.setup("Garden IoT");
```

**Result:**
- AP SSID: `Garden IoT-Setup-8774BC`
- mDNS: `garden-iot-8774bc.local` (spaces converted to hyphens)
- Web page title: "Garden IoT Configuration"

#### Example 4: Use Default
```cpp
runLoop.setup();  // No parameter = uses "EasyWiFi"
// or
runLoop.setup("EasyWiFi");  // Explicit default
```

---

## Device Name Guidelines

### Best Practices

✅ **Good device names:**
- Short and descriptive: "SmartPlug", "TempSensor", "LED Strip"
- Project-specific: "GarageMonitor", "PlantWatering"
- Brand-specific: "Acme IoT", "MyCompany Device"

❌ **Avoid:**
- Too long: "SuperAwesomeAutomatedGardenWateringSystemPro" (truncated)
- Special characters that might not work in hostnames: `@`, `#`, `$`
- Leading/trailing spaces (they'll be trimmed)

### Length Limits

- **SSID**: Maximum 32 characters total
  - Your name + "-Setup-" + MAC (6 chars) must fit
  - Example: "MyDevice" (8) + "-Setup-" (7) + "8774BC" (6) = 21 chars ✅
  - Example: "MyReallyLongDeviceName" (22) + "-Setup-8774BC" (13) = 35 chars ❌ (truncated)

- **mDNS Hostname**: Maximum 63 characters
  - Usually not an issue unless your device name is extremely long

### Special Characters

The library handles special characters automatically:

| In Device Name | In SSID | In mDNS Hostname |
|----------------|---------|------------------|
| Spaces | Preserved | Converted to `-` |
| Uppercase | Preserved | Converted to lowercase |
| `_` | Preserved | Preserved |
| `-` | Preserved | Preserved |

**Example:**
```cpp
runLoop.setup("My Cool Device");
```
- SSID: `My Cool Device-Setup-8774BC` (spaces preserved)
- mDNS: `my-cool-device-8774bc.local` (lowercase, spaces to hyphens)

---

## Advanced Customization

### Changing AP Password

The AP password is set in `lib/EasyWiFi/Macros.h`:

```cpp
#define AP_PASSWORD ""  // Empty = open network

// Change to:
#define AP_PASSWORD "setup123"  // Your password
```

**Note:** See `Docs/Security.md` for security recommendations.

### Changing Web Server Port

Default port is 80 (HTTP). To change:

```cpp
// In lib/EasyWiFi/Macros.h
#define WEB_SERVER_PORT 80  // Change to desired port
```

### Customizing LED Pin and Brightness

```cpp
// In lib/EasyWiFi/Macros.h
#define LED_PIN 8            // GPIO pin for status LED
#define LED_BRIGHTNESS 128   // 0-255 (0=off, 255=max)
```

### Customizing Timeouts

```cpp
// In lib/EasyWiFi/Macros.h
#define WIFI_CONNECTION_TIMEOUT 30000  // 30 seconds (in ms)
#define STATUS_LOG_INTERVAL 120000     // 2 minutes (in ms)
```

---

## Web Page Customization

The device name is automatically used in all web pages:

### Home Page Title
```html
<title>DeviceName Configuration</title>
<h1>DeviceName Setup</h1>
```

### Access Point Info
```
Connect to WiFi: DeviceName-Setup-8774BC
```

### mDNS Address
```
Access at: http://devicename-8774bc.local
```

**Note:** If you need more extensive web page customization (colors, layout, etc.), you'll need to edit `lib/EasyWiFi/WebPages.cpp`.

---

## Multiple Devices on Same Network

If you have multiple devices using EasyWiFi:

### Option 1: Different Device Names
```cpp
// Device 1
runLoop.setup("Kitchen Sensor");  // kitchen-sensor-8774bc.local

// Device 2
runLoop.setup("Living Room Sensor");  // living-room-sensor-ab12cd.local
```

Each device will have a unique mDNS hostname based on name + MAC.

### Option 2: Same Device Name
```cpp
// Device 1
runLoop.setup("Sensor");  // sensor-8774bc.local

// Device 2
runLoop.setup("Sensor");  // sensor-ab12cd.local
```

The MAC address suffix ensures uniqueness.

---

## Runtime vs Compile-Time Names

### Current Implementation (Compile-Time)
```cpp
void setup() {
    runLoop.setup("MyDevice");  // Set once at compile time
}
```

The device name is set at startup and cannot be changed without reflashing.

### Future: Runtime Configuration
If you need the device name to be configurable after deployment, you could:

1. Store the device name in NVS (non-volatile storage)
2. Load it during `setup()`
3. Provide a web UI to change it

**Example implementation idea:**
```cpp
void setup() {
    String storedName = storage.getDeviceName();
    if (storedName.length() == 0) {
        storedName = "EasyWiFi";  // Default
    }
    runLoop.setup(storedName);
}
```

This is not currently implemented but could be added if needed.

---

## Testing Your Customization

After setting a custom device name:

### 1. Build and Upload
```bash
pio run -t upload
```

### 2. Monitor Serial Output
```bash
pio device monitor
```

Look for:
```
[I][RUNLOOP] ========================================
[I][RUNLOOP] MyDevice Starting...
[I][RUNLOOP] ========================================
```

### 3. Check AP SSID
On your phone/computer, look for WiFi network:
```
MyDevice-Setup-8774BC
```

### 4. Test mDNS
After connecting to WiFi, access:
```
http://mydevice-8774bc.local
```

**Note:** mDNS requires Bonjour (macOS/iOS) or Avahi (Linux). Windows 10+ has built-in support.

---

## Troubleshooting

### SSID Not Showing Up with Custom Name

**Check:**
1. Did you call `runLoop.setup("YourName")`?
2. Is the name too long? Keep it under 20 characters to be safe
3. Look at serial output for the actual SSID being created

### mDNS Not Working

**Check:**
1. Device must be connected to WiFi (not just in AP mode)
2. Your computer must support mDNS
3. Try the IP address directly instead (shown in serial output)
4. Some networks block mDNS - try a different network

### Special Characters in Device Name

If you use special characters and things break:
- Stick to alphanumeric characters, spaces, hyphens, and underscores
- Avoid: `@`, `#`, `$`, `%`, `^`, `&`, `*`, `(`, `)`, `/`, `\`

---

## Examples from Real Projects

### Example 1: Product Line
```cpp
// Different products, same company
runLoop.setup("Acme Plug");    // Smart plug
runLoop.setup("Acme Bulb");    // Smart bulb
runLoop.setup("Acme Sensor");  // Sensor
```

### Example 2: Room-Based
```cpp
// Same device type, different rooms
runLoop.setup("Kitchen Light");
runLoop.setup("Bedroom Light");
runLoop.setup("Garage Light");
```

### Example 3: Versioned Devices
```cpp
// Include version in name
runLoop.setup("PlantBot v2");
runLoop.setup("TempSensor v3");
```

---

## Summary

### Quick Steps
1. Open `src/main.cpp`
2. Change `runLoop.setup("EasyWiFi")` to `runLoop.setup("YourDeviceName")`
3. Build and upload
4. Done!

### What Changes
- ✅ AP SSID: `YourDeviceName-Setup-XXXXXX`
- ✅ mDNS hostname: `yourdevicename-xxxxxx.local`
- ✅ Web page titles and headers
- ✅ Log messages

### What Doesn't Change
- ❌ MAC address (hardware-specific)
- ❌ Stored WiFi credentials
- ❌ IP address (assigned by your router)
- ❌ Web page layout and styling

---

## Need More Customization?

For more extensive customization beyond the device name:

- **Web Pages**: Edit `lib/EasyWiFi/WebPages.cpp`
- **Colors/Styling**: Edit the HTML/CSS in `lib/EasyWiFi/WebPages.cpp`
- **API Endpoints**: Edit `lib/EasyWiFi/ConfigServer.cpp`
- **Behavior**: Edit `lib/EasyWiFi/RunLoop.cpp`

---

**Happy Customizing!** 🎨

