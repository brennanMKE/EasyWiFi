# EasyWiFi - ESP32-C3 WiFi Configuration System

<div align="center">

![ESP32-C3](https://img.shields.io/badge/ESP32--C3-Supported-green)
![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-blue)
![License](https://img.shields.io/badge/License-MIT-yellow)
![Version](https://img.shields.io/badge/Version-0.1.0-orange)

**Professional WiFi configuration system for ESP32-C3 with web interface and REST API**

</div>

---

## 📋 Table of Contents

- [Features](#-features)
- [Quick Start](#-quick-start)
- [Hardware Requirements](#-hardware-requirements)
- [Installation](#-installation)
- [First Boot](#-first-boot)
- [Usage](#-usage)
- [API Documentation](#-api-documentation)
- [Configuration](#-configuration)
- [LED Status Indicators](#-led-status-indicators)
- [Troubleshooting](#-troubleshooting)
- [Architecture](#-architecture)
- [Development](#-development)
- [Contributing](#-contributing)
- [License](#-license)

---

## ✨ Features

### Core Functionality
- 🌐 **Automatic WiFi Connection** - Connects to stored networks on boot
- 📡 **Access Point Fallback** - Creates AP when no WiFi configured
- 🔍 **Network Scanning** - Discover available networks with signal strength
- 🖥️ **Web Configuration UI** - Modern, responsive web interface
- 💾 **Persistent Storage** - Credentials saved in NVS (survives reboots)
- 🔄 **Auto-Reconnect** - Monitors and reconnects on disconnect

### Advanced Features
- 🚀 **REST API** - Full programmatic access via JSON API
- 🌍 **Captive Portal** - Auto-redirect on AP connection
- 🔒 **mDNS Support** - Access via `easywifi-XXXXXX.local`
- 💡 **LED Status Indicators** - Visual feedback of device state
- 🐕 **Watchdog Timer** - Auto-recovery from crashes
- 📊 **Memory Monitoring** - Track heap usage and prevent leaks
- 🔌 **CORS Enabled** - Web apps can access API cross-origin

### User Experience
- 📱 **Mobile Responsive** - Works on all devices
- ⚡ **Real-time Validation** - Immediate feedback on forms
- 🎨 **Modern UI** - Clean, professional interface
- 🔐 **Password Visibility Toggle** - Show/hide password
- 🔄 **Auto-refresh Status** - Live connection monitoring
- ✅ **Success Feedback** - Clear status messages

---

## 🚀 Quick Start

### 1. Flash Firmware
```bash
pio run --target upload
```

### 2. Connect to AP
On first boot, device creates WiFi network:
```
SSID: EasyWiFi-Setup-XXXXXX
Password: (open network)
```

### 3. Configure
Open browser to: `http://192.168.4.1`

- Click "Scan for Networks"
- Select your WiFi network
- Enter password
- Click "Save & Connect"

### 4. Done!
Device connects and remembers your WiFi credentials.

---

## 🔧 Hardware Requirements

### Supported Boards
- ESP32-C3-DevKitM-1
- ESP32-C3-DevKitC-02
- Any ESP32-C3 based board

### Minimum Specifications
- **MCU:** ESP32-C3 (RISC-V, 160MHz)
- **RAM:** 320KB SRAM
- **Flash:** 4MB (minimum 2MB recommended)
- **WiFi:** 2.4GHz 802.11 b/g/n

### Optional
- **LED:** GPIO 8 (built-in on most dev boards)
- **Button:** Any GPIO for factory reset (future feature)

---

## 📦 Installation

### Prerequisites
- [PlatformIO Core](https://platformio.org/install) or [PlatformIO IDE](https://platformio.org/platformio-ide)
- USB cable for programming
- ESP32-C3 development board

### Method 1: PlatformIO CLI

```bash
# Clone repository
git clone <repository-url>
cd EasyWiFi

# Build firmware
pio run

# Upload to device
pio run --target upload

# Monitor serial output
pio device monitor
```

### Method 2: PlatformIO IDE (VS Code)

1. Open project folder in VS Code
2. PlatformIO will auto-detect `platformio.ini`
3. Click "Build" button (✓)
4. Click "Upload" button (→)
5. Click "Serial Monitor" button (🔌)

### Method 3: Pre-built Binary

```bash
# Flash pre-built binary
esptool.py --chip esp32c3 write_flash 0x0 firmware.bin
```

---

## 🎯 First Boot

### What Happens

1. **Power On** → LED blinks fast (initializing)
2. **No Credentials Found** → LED slow blinks (AP mode)
3. **Access Point Created** → `EasyWiFi-Setup-XXXXXX`
4. **Captive Portal Active** → Auto-redirects to config page

### Connect to Device

**Via AP:**
```
1. Connect to: EasyWiFi-Setup-XXXXXX
2. Open browser (may auto-redirect)
3. If no redirect, go to: http://192.168.4.1
```

**Via Serial Monitor:**
```
115200 baud
Check logs for AP SSID and IP address
```

---

## 💻 Usage

### Web Interface

#### Home Page
- Shows connection status
- Device information (IP, signal strength)
- Quick access to scan and configure

#### Scan Networks
- Lists all available WiFi networks
- Shows signal strength (bars and dBm)
- Indicates security type (locked/open)
- Click network to connect

#### Configure WiFi
- Enter SSID (auto-filled if scanned)
- Enter password (with show/hide toggle)
- Real-time validation
- Clear error messages

#### Status Monitoring
- Real-time connection progress
- Auto-refreshes every 2 seconds
- Shows success or failure
- Retry options on failure

### REST API

See [API.md](API.md) for complete documentation.

#### Quick Examples

**Get Status:**
```bash
curl http://192.168.4.1/api/status
```

**Scan Networks:**
```bash
curl http://192.168.4.1/api/scan
```

**Configure WiFi:**
```bash
curl -X POST http://192.168.4.1/api/configure \
  -d "ssid=MyNetwork" \
  -d "password=MyPassword123"
```

**Clear Credentials:**
```bash
curl -X DELETE http://192.168.4.1/api/credentials
```

---

## 📡 API Documentation

Full REST API documentation available in [API.md](API.md)

### Endpoints

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/api/status` | GET | Device and WiFi status |
| `/api/scan` | GET | Scan WiFi networks |
| `/api/configure` | POST | Save WiFi credentials |
| `/api/connect` | POST | Initiate connection |
| `/api/credentials` | DELETE | Clear credentials |
| `/api/reset` | GET | Factory reset + reboot |

### Language Examples
- ✅ curl (bash)
- ✅ JavaScript (Fetch API)
- ✅ Python (requests)
- ✅ Postman compatible

---

## ⚙️ Configuration

### Build Configuration

Edit `platformio.ini`:

```ini
[env:esp32c3]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino
monitor_speed = 115200
lib_deps = ArduinoJson
```

### WiFi Configuration

Edit `lib/EasyWiFi/Macros.h`:

```cpp
// Access Point Configuration
#define AP_SSID_PREFIX "EasyWiFi-Setup-"
#define AP_PASSWORD ""  // Empty = open network
#define AP_CHANNEL 1
#define AP_TIMEOUT 300000  // 5 minutes

// LED Configuration
#define LED_PIN 8
#define LED_ENABLED true
#define LED_BRIGHTNESS 128

// Connection Settings
#define WIFI_CONNECTION_TIMEOUT 10000
#define WIFI_CONNECTION_ATTEMPTS 3
#define MAX_STORED_NETWORKS 5
```

### Storage Configuration

Credentials stored in NVS namespace: `easywifi`

Maximum stored networks: 5 (oldest removed when full)

---

## 💡 LED Status Indicators

| Pattern | Meaning | Description |
|---------|---------|-------------|
| ❤️ Heartbeat | Connected | Double pulse every second - WiFi connected |
| 🔄 Slow Blink | AP Mode | 1 second on/off - Waiting for configuration |
| ⚡ Fast Blink | Connecting | 250ms on/off - Attempting WiFi connection |
| ⚠️ Double Blink | Error | Two quick blinks - System error occurred |
| ⬜ Off | Disabled | LED功能关闭 or no LED configured |

### Interpreting LED Patterns

**Normal Operation:**
```
Fast Blink → Slow Blink → Fast Blink → Heartbeat
(Boot)      (AP Mode)     (Connecting) (Connected)
```

**Configuration Workflow:**
```
Slow Blink → Fast Blink → Heartbeat
(AP Mode)     (Connecting) (Connected)
```

---

## 🔧 Troubleshooting

### Device Not Creating AP

**Symptoms:** No AP visible after power on

**Solutions:**
1. Wait 10 seconds for boot
2. Check serial monitor for errors
3. Verify LED is blinking (if available)
4. Try factory reset (power cycle 3 times quickly)

### Cannot Connect to AP

**Symptoms:** WiFi network visible but won't connect

**Solutions:**
1. Forget network on your device
2. Ensure you're in range (< 10 meters)
3. Check device is in AP mode (LED slow blinking)
4. Try different client device (phone/laptop)

### Captive Portal Not Working

**Symptoms:** No auto-redirect after connecting

**Solutions:**
1. Manually go to: `http://192.168.4.1`
2. Disable mobile data on phone
3. Check browser isn't cached
4. Try incognito/private mode

### WiFi Connection Fails

**Symptoms:** Credentials saved but won't connect

**Solutions:**
1. Verify SSID and password are correct
2. Check network uses 2.4GHz (not 5GHz)
3. Ensure WPA2/WPA3 encryption (not WEP)
4. Move closer to router
5. Check router isn't blocking device

### Web Interface Not Loading

**Symptoms:** Page won't load or times out

**Solutions:**
1. Verify connected to device AP
2. Check IP address: `http://192.168.4.1`
3. Clear browser cache
4. Try different browser
5. Check firewall settings

### Device Keeps Rebooting

**Symptoms:** Watchdog resets, constant reboots

**Solutions:**
1. Check serial monitor for crash logs
2. Verify power supply is adequate (5V 500mA minimum)
3. Update to latest firmware
4. Contact support with logs

### Low Memory Warnings

**Symptoms:** "Low memory" in serial logs

**Solutions:**
1. Check for memory leaks in serial monitor
2. Reduce number of stored credentials
3. Update firmware
4. This is usually normal during operation

---

## 🏗️ Architecture

### Component Overview

```
┌─────────────────────────────────────────┐
│           EasyWiFi System               │
├─────────────────────────────────────────┤
│                                         │
│  ┌──────────┐    ┌───────────────┐    │
│  │ RunLoop  │───▶│ WiFiManager   │    │
│  │          │    │ - Connect     │    │
│  │ - Setup  │    │ - Scan        │    │
│  │ - Loop   │    │ - AP Mode     │    │
│  │ - States │    │ - mDNS        │    │
│  └────┬─────┘    │ - Captive     │    │
│       │          └───────────────┘    │
│       │                                │
│       ├──▶ ┌────────────────┐         │
│       │    │ ConfigServer   │         │
│       │    │ - Web UI       │         │
│       │    │ - REST API     │         │
│       │    │ - CORS         │         │
│       │    └────────────────┘         │
│       │                                │
│       ├──▶ ┌────────────────┐         │
│       │    │ Storage        │         │
│       │    │ - NVS          │         │
│       │    │ - Credentials  │         │
│       │    └────────────────┘         │
│       │                                │
│       └──▶ ┌────────────────┐         │
│            │ StatusLED      │         │
│            │ - Patterns     │         │
│            │ - Brightness   │         │
│            └────────────────┘         │
│                                         │
└─────────────────────────────────────────┘
```

### State Machine

```
┌──────────────┐
│ INITIALIZING │
└──────┬───────┘
       ▼
┌──────────────────┐
│ LOADING_CREDS    │
└──────┬───────────┘
       ▼
    ┌──────┐
    │ Has  │
    │Creds?│
    └──┬───┘
  Yes  │  No
   ┌───┴───┐
   ▼       ▼
┌──────┐ ┌────────┐
│CONN  │ │AP_MODE │
│ECTING│ └────┬───┘
└──┬───┘      │
   │ Success  │
   ▼          │Configure
┌──────────┐  │
│ CONNECTED│◀─┘
└──────────┘
```

### File Structure

```
EasyWiFi/
├── src/
│   └── main.cpp              # Application entry point
├── lib/
│   └── EasyWiFi/
│       ├── EasyWiFi.h        # Main include
│       ├── RunLoop.h/cpp     # Main orchestration
│       ├── WiFiManager.h/cpp # WiFi management
│       ├── ConfigServer.h/cpp# Web server
│       ├── WebPages.h/cpp    # HTML templates
│       ├── Storage.h/cpp     # NVS storage
│       ├── StatusLED.h/cpp   # LED control
│       ├── Macros.h          # Constants
│       └── library.json      # Library manifest
├── platformio.ini            # Build configuration
├── README.md                 # This file
├── API.md                    # REST API docs
├── PRD.md                    # Product requirements
└── Task.md                   # Implementation tasks
```

---

## 👨‍💻 Development

### Building from Source

```bash
# Clone repository
git clone <repository-url>
cd EasyWiFi

# Install dependencies (auto via PlatformIO)
pio pkg install

# Build
pio run

# Upload
pio run --target upload

# Monitor
pio device monitor --baud 115200
```

### Development Build

Enable verbose logging in `platformio.ini`:

```ini
build_flags = 
    -DCORE_DEBUG_LEVEL=5
    -DCONFIG_LOG_MAXIMUM_LEVEL=5
```

### Testing

#### Manual Testing
1. Test AP mode creation
2. Test network scanning
3. Test WiFi configuration
4. Test reconnection after reboot
5. Test captive portal redirect
6. Test factory reset

#### API Testing
```bash
# Run API test script
bash test_api.sh
```

### Memory Usage

**Current Usage:**
- RAM: 12.7% (41,588 bytes / 327,680 bytes)
- Flash: 68.9% (903,254 bytes / 1,310,720 bytes)

**Optimization Tips:**
- Reduce HTML template sizes
- Minimize string concatenation
- Use F() macro for constants
- Optimize buffer sizes

---

## 🤝 Contributing

Contributions welcome! Please:

1. Fork the repository
2. Create feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit changes (`git commit -m 'Add AmazingFeature'`)
4. Push to branch (`git push origin feature/AmazingFeature`)
5. Open Pull Request

### Coding Standards
- Follow existing code style
- Add comments for complex logic
- Update documentation
- Test on hardware before PR

---

## 📄 License

This project is licensed under the MIT License.

```
MIT License

Copyright (c) 2025 EasyWiFi Project

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

## 🙏 Acknowledgments

- **GlowBoxen Project** - Reference implementation and patterns
- **ESP32 Community** - Documentation and support
- **PlatformIO** - Development platform
- **Arduino Framework** - Core libraries

---

## 📞 Support

- **Documentation:** Check README.md, API.md, and PRD.md
- **Issues:** Open GitHub issue with logs
- **Serial Logs:** Include output from serial monitor
- **Hardware:** Specify your ESP32-C3 board model

---

## 🗺️ Roadmap

### Future Features
- [ ] OTA firmware updates
- [ ] Bluetooth configuration (BLE)
- [ ] Static IP configuration
- [ ] DNS server configuration
- [ ] Multiple AP credentials priority
- [ ] Network statistics dashboard
- [ ] Scheduled connections
- [ ] Guest network mode
- [ ] MQTT integration
- [ ] HomeAssistant integration

---

<div align="center">

**Made with ❤️ for the ESP32 community**

⭐ Star this project if you find it useful!

</div>

