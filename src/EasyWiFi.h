#ifndef EASYWIFI_H
#define EASYWIFI_H

#if defined(ESP32)
#include "Arduino.h"
#include "RunLoop.h"
#include "WiFiManager.h"
#include "ConfigServer.h"
#include "CustomPageHandler.h"
#include "Storage.h"
#include "WebPages.h"
#include "StatusLED.h"
#include "ErrorHandler.h"
#include "Macros.h"
#else
#error "This code is only intended to be compiled for ESP32 platforms."
#endif

#endif // EASYWIFI_H
