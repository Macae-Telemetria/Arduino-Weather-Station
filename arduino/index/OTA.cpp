
#include "OTA.h"
#include <HTTPClient.h>
#include <Update.h>



void OTA::update(const String& url) {
  Serial.println("Starting firmware update...");
  
  HTTPClient http;
  if (http.begin(url)) {
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
              Serial.println("Reiniciando");
              delay(1500);
              ESP.restart();
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


