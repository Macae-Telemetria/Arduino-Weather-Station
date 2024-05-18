#include "OTA.h"
#include <HTTPClient.h>
#include <Update.h>
#include "pch.h"
bool OTA::update(const String& url) {
  Serial.println("Starting firmware update...");

  HTTPClient http;

  if (!http.begin(url)) {
    Serial.printf("Não foi possivel conectar.");
    return false;
  }

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("Servidor indisponivel.");
    http.end();
    return false;
  }

  int contentLength = http.getSize();
  if (contentLength <= 0) {
    http.end();
    return false;
  }

  Serial.printf("Downloading firmware binary (%d bytes)...\n", contentLength);

  if (Update.begin(contentLength)) {
    WiFiClient* stream = http.getStreamPtr();
    if (Update.writeStream(*stream)) {
      Serial.printf(".");
      if (Update.end()) {
        Serial.printf("Downloading concluido com sucesso.");
        http.end();
        return true;
      }
    }
  }

  Serial.printf("Falha no downloading.");
  http.end();
  return false;
}
