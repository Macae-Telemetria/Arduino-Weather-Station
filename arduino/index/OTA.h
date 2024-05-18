#pragma once
#define FIRMWARE_VERSION "2.0.1"
#define UPDATE_URL 
#include <Arduino.h>

class OTA {
public:
  static bool update(const String& url);
};
