#pragma once
#define FIRMWARE_VERSION "2.1.2"
#define UPDATE_URL 
#include <Arduino.h>

class OTA {
public:
  static bool update(const String& url);
};
