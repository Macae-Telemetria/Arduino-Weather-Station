
#include "OTA.h"
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>
//#include <ESP32httpUpdate.h>
String OTA::url_update;
unsigned long OTA::lastCheckUpdateTime = 0;

String bin_url;

void OTA::onUpdate(unsigned long timeNow, unsigned long interval) {
  if (timeNow - lastCheckUpdateTime >= interval) {
    update();
    lastCheckUpdateTime = timeNow;
  }
}

void OTA::update() {
  bool updateRequired = checkForUpdateCondition();
  if (updateRequired) {
    performFirmwareUpdate();
    ESP.restart();
  }
}

void OTA::setUpdateUrl(String url) {
  url_update = url;
}

bool OTA::checkForUpdateCondition() {
  HTTPClient http;
  String response;
    
  // Check for update condition
  Serial.println("Checking for firmware update...");
  if (http.begin(url_update)) {
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      response = http.getString();
      StaticJsonDocument<1024> doc;
      deserializeJson(doc, response);
      JsonObject obj = doc.as<JsonObject>();

      String version = obj["version"].as<String>();
      bin_url = obj["url"].as<String>();

      if (version.toDouble() > FIRMWARE_VERSION) {
        Serial.println("Firmware update available!");
        return true;
      } else {
        Serial.println("Firmware is up to date.");
      }
    } else {
      Serial.printf("HTTP request failed with error code %d\n", httpCode);
    }
    http.end();
  } else {
    Serial.println("Failed to connect to update server.");
  }
  
  return false;
}

void OTA::performFirmwareUpdate() {
  Serial.println("Starting firmware update...");
  
  HTTPClient http;
  if (http.begin(bin_url)) {
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      int contentLength = http.getSize();
      if (contentLength > 0) {
        Serial.printf("Downloading firmware binary (%d bytes)...\n", contentLength);
        if (Update.begin(contentLength)) {
          WiFiClient *stream = http.getStreamPtr();
          if (Update.writeStream(*stream)) {
            if (Update.end()) {
              Serial.println("Update successful!");
            } else {
              Serial.println("Update failed!");
            }
          } 
        } 
      } 
    } 
    http.end();
  } 
}


