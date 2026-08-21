# Changelog

All notable changes to EasyWiFi are documented here. This project adheres to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html). While the version is
below 1.0.0, breaking changes bump the minor version.

## [0.3.0] - 2026-08-20

Toolchain compatibility release. v0.2.0 no longer compiles against current
Arduino-ESP32 cores; this release fixes that and pins the versions it was
verified against.

### Fixed
- Build failure on Arduino-ESP32 3.x: removed the `WiFi.getAutoConnect()` call
  from `WiFiManager::logWiFiDiagnostics()`. The API was removed upstream; the
  adjacent `getAutoReconnect()` diagnostic still reports the surviving setting.
- Build failure on ESP-IDF 5.x: `RunLoop::setup()` now passes an
  `esp_task_wdt_config_t` to `esp_task_wdt_init()`, falling back to
  `esp_task_wdt_reconfigure()` when the framework has already started the task
  watchdog. The pre-IDF-5 two-argument call is retained behind
  `ESP_IDF_VERSION_MAJOR` so older cores still build.

### Changed
- **Requires ArduinoJson 7.** The manifest dependency moved from `*` to
  `^7.4.3`, and `ConfigServer::handleAPIHealth()` now uses the ArduinoJson 7
  API (`JsonDocument` and `doc[key].to<JsonObject>()` in place of the
  deprecated `StaticJsonDocument<N>` and `createNestedObject()`). Consumers
  still on ArduinoJson 6 must stay on v0.2.0 or upgrade.
- `esp_task_wdt_init()` failures are now logged as a warning instead of being
  silently discarded.
- The packed tarball no longer ships development-only files (`build.sh`,
  `EasyWiFi.code-workspace`, `.gitignore`, `include/`, `test/`, `dist/`).

### Notes
- Verified against `espressif32@54.3.21` (Arduino core 3.2.1, ESP-IDF 5.4) on
  `esp32-c3-devkitm-1`. The Lantern example pins that platform version.

## [0.2.0] - 2026-06-08

### Changed
- Restructured the repository as a PlatformIO library so `library.json` and
  `EasyWiFi.h` resolve at the clone root, making a bare git URL or registry
  name usable as a single `lib_deps` entry. The demo app moved to
  `examples/Lantern/`.
- Prefixed all configuration macros with `EWIFI_` to avoid collisions with
  consumer code.

### Added
- `build.sh` task runner (`clean` / `build` / `publish`) and `BUILD.md`.
- ESPAsyncWebServer migration guide.

[0.3.0]: https://github.com/brennanMKE/EasyWiFi/compare/v0.2.0...v0.3.0
[0.2.0]: https://github.com/brennanMKE/EasyWiFi/releases/tag/v0.2.0
