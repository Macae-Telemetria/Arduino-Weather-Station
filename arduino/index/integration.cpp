// Titulo: Integração HTTP & MQQT
// Data: 30/07/2023
//.........................................................................................................................
#include "integration.h"
#include <WiFi.h> // #include <WiFiClientSecure.h>
#include <NTPClient.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
/**** WIFI Client Initialisation *****/
#include "constants.h"
#include "data.h"
#include "SD.h"
#include <WString.h>

/**** Secure WiFi Connectivity Initialisation *****/
// WiFiClientSecure secureWifiClient;

namespace TLM{
  /**** NTP Client Initialisation  *****/
  WiFiClient wifiClient;
  WiFiUDP ntpUDP;
  NTPClient timeClient(ntpUDP);

  /**** MQTT Client Initialisation Using WiFi Connection *****/
  PubSubClient mqttClient(wifiClient);

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
        OnDebug(Serial.print(".");)
    }
    
    OnDebug(Serial.printf("\n%s: Contectado com sucesso \n", contextName);)
    OnDebug(Serial.printf("%s: IP address = %s \n", contextName, WiFi.localIP());)
    return 1;
  }

  bool sendMeasurementToMqtt(char *topic, const char *payload)
  {
    bool sent = (mqttClient.publish(topic, payload, true));
    if (sent){
      OnDebug(Serial.println("  - MQTT broker: Message publised [" + String(topic) + "]: ");)
    } else {
      OnDebug(Serial.println("  - MQTT: Não foi possivel enviar");)
    }
    return sent;
  }

  bool connectMqtt(const char *contextName, char* mqtt_username, char* mqtt_password, char* mqtt_topic) {
    if (!mqttClient.connected()) {
      OnDebug(Serial.printf("%s: Tentando nova conexão...", contextName);)
      String clientId = "ESP8266Client-";
      clientId += String(random(0xffff), HEX);
      if (mqttClient.connect(clientId.c_str(), mqtt_username, mqtt_password)){
        OnDebug(Serial.printf("%s: Reconectado", contextName);)
        mqttClient.subscribe(mqtt_topic);
        return true;
      }
      OnDebug(Serial.print("failed, rc=");)
      OnDebug(Serial.print(mqttClient.state());)
      return false;
    }
    OnDebug(Serial.printf("%s: Conectado [ %s ]", contextName, mqtt_topic);)
    return true;
  }

  bool setupMqtt(const char *contextName, char* mqtt_server, int mqtt_port, char* mqtt_username, char* mqtt_password, char* mqtt_topic){
    OnDebug(Serial.printf("%s: Estabelecendo conexão inicial\n", contextName);)
    mqttClient.setServer(mqtt_server, mqtt_port);
    // mqttClient.setCallback(callback);
    return connectMqtt(contextName, mqtt_username, mqtt_password, mqtt_topic);
  }

  /* Ntp Client */
  int connectNtp(const char *contextName)
  {
    OnDebug(Serial.printf("%s: Estabelecendo conexão inicial\n", contextName);)

    timeClient.begin();

    while(!timeClient.update()) {
      OnDebug(Serial.print(".");)
      delay(1000);
    }

    OnDebug(Serial.printf("%s: Conectado com sucesso. \n", contextName);)
    return 1;
  }

  bool loopMqtt(){
    return mqttClient.loop();
  }
  bool isWifiConnected(){
    return WiFi.status() == WL_CONNECTED;
  }

  int getWifiSginal(){
    return WiFi.RSSI();
  }


 int streamFile(File& file, const String& url) {
    HTTPClient http;
    String fileName = file.name();
    String slugStacao = config.station_name;

    // Iniciando o backup
    String iniciarBackupURl = url + "/start/" + slugStacao + "/" + fileName;
    if (!http.begin(wifiClient, iniciarBackupURl)) {
      OnDebug(Serial.println("Failed to begin connection.");)
      return 0;
    }

    int httpResponseCode = http.sendRequest("GET");
    if (httpResponseCode != HTTP_CODE_OK) {
        OnDebug(Serial.println("Failed to upload chunk. HTTP response code: " + String(httpResponseCode));)
        file.close();
        http.end();
        return 0;
    }
    String backupFilepath = http.getString();
    http.end();

    // Iniciar stream
    String streamBackupUrl = url + "/stream?f=" + backupFilepath;
    if (!http.begin(wifiClient, streamBackupUrl)) {
      OnDebug(Serial.println("Failed to begin connection.");)
      file.close();
      http.end();
      return 0;
    }
    OnDebug(Serial.println("Conexão iniciado com sucesso. " + String(streamBackupUrl));)

    const size_t chunkSize = 2048;
    uint8_t buffer[chunkSize];
    int bytesRead;
    while ((bytesRead = file.read(buffer, chunkSize)) > 0) {
      http.addHeader("Content-Type", "application/octet-stream");
      httpResponseCode = http.sendRequest("POST", buffer, bytesRead);
    
      if (httpResponseCode != HTTP_CODE_OK) {
        OnDebug(Serial.println("Failed to upload chunk. HTTP response code: " + String(httpResponseCode));)
        file.close();
        http.end();
        return 0;
      }
    }

    OnDebug(Serial.println("Done!");)
    file.close();
    http.end();

    // Finalizando o backup
    String finalizarBackupURl = url + "/finish?f=" + backupFilepath;
    if (!http.begin(wifiClient, finalizarBackupURl)) {
      OnDebug(Serial.println("Failed to begin connection.");)
      return 0;
    }

    httpResponseCode = http.sendRequest("GET");
    if (httpResponseCode == HTTP_CODE_OK) {
      OnDebug(Serial.println("Backup realizado com sucesso!" + String(httpResponseCode));)
      SD.remove( String("/metricas/") + fileName );
      return 1;
    }

    http.end();
    return 0;
  }
}