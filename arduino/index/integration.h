// Titulo: Integração HTTP & MQQT
// Data: 30/07/2023
//.........................................................................................................................
#include "pch.h"


#pragma once
#include <WiFi.h> // #include <WiFiClientSecure.h>
#include <NTPClient.h>


/**** WIFI Client Initialisation *****/
WiFiClient wifiClient;

/**** Secure WiFi Connectivity Initialisation *****/
// WiFiClientSecure secureWifiClient;

/**** NTP Client Initialisation  *****/
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

/**** MQTT Client Initialisation Using WiFi Connection *****/


unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];

/***** WIFI ****/

int setupWifi(const char *contextName, char* ssid, char*password)
{
  OnDebug(Serial.printf("%s: Estabelecendo conexão inicial", contextName);)
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  // secureWifiClient.setCACert(root_ca);    // enable this line and the the "certificate" code for secure connection
  while (WiFi.status() != WL_CONNECTED) {
    for(int i=0; i<5;i++){
    digitalWrite(LED2,i%2);
    delay(200);}
      Serial.print(".");
  }
  
  OnDebug(Serial.printf("\n%s: Contectado com sucesso \n", contextName);)
  OnDebug(Serial.printf("%s: IP address = %s \n", contextName, WiFi.localIP());)
  return 1;
}


/* Ntp Client */
int connectNtp(const char *contextName)
{
  OnDebug(Serial.printf("%s: Estabelecendo conexão inicial\n", contextName);)

  timeClient.begin();

  while(!timeClient.update()) {
    Serial.print(".");
    delay(1000);
  }

  OnDebug(Serial.printf("%s: Conectado com sucesso. \n", contextName);)
  return 1;
}


