#pragma once
#define FIRMWARE_VERSION "2.1.212"
#define UPDATE_URL 
#include <Arduino.h>

class OTA {
public:
  static bool update(const String& url);
};
