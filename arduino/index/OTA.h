#pragma once
#define FIRMWARE_VERSION "2.0.4"
#define UPDATE_URL 
#include <Arduino.h>

class OTA {
public:
  static void update(const String& url);
};
