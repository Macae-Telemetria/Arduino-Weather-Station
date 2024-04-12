
#pragma once
#define FIRMWARE_VERSION 1.8
#define UPDATE_URL 
#include <Arduino.h>


class OTA {
public:
  static void onUpdate(unsigned long timeNow, unsigned long interval);
  static void update();
  static void setUpdateUrl(String url);

private:
  static bool checkForUpdateCondition();
  static void performFirmwareUpdate();

  static String url_update;
  static unsigned long lastCheckUpdateTime;
};


