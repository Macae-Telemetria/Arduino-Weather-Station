
#pragma once
#define FIRMWARE_VERSION 1.8
#define UPDATE_URL 
#include <Arduino.h>


class OTA {
public:
  static void update(const String& url);

};


