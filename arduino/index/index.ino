// Autor: Lucas Fonseca e Gabriel Fonseca
// Titulo: GCIPM arduino
// Versão: 2.0.0 OTA;
//.........................................................................................................................

#include "constants.h"
#include "data.h"
#include "sd-repository.h"
#include "integration.h"
#include "sensores.h"
#include <stdio.h>
#include "esp_system.h"
#include "bt-integration.h"
#include <string>
#include <vector>
#include <rtc_wdt.h>
#include "mqtt.h"
#include "OTA.h"
#include <ArduinoJson.h>
// -- WATCH-DOG
#define WDT_TIMEOUT 600000

extern unsigned long lastPVLImpulseTime; 
extern unsigned int rainCounter;
extern unsigned long lastVVTImpulseTime;
extern float anemometerCounter;
extern int rps[20];
extern Sensors sensors;
long startTime;
int timeRemaining=0;
std::string jsonConfig;
String formatedDateString = "";
struct HealthCheck healthCheck = {FIRMWARE_VERSION, 0, false, false, 0, 0};
// -- MQTT
String sysReportMqqtTopic;
String softwareReleaseMqttTopic;
MQTT mqqtClient1;
MQTT mqqtClient2;

// -- Novo
int wifiDisconnectCount=0;

void logIt(const std::string &message, bool store = false){
  Serial.print(message.c_str());
  if(store == true){
    storeLog(message.c_str());
  }
}

void watchdogRTC() {
  rtc_wdt_protect_off();      //Disable RTC WDT write protection
  rtc_wdt_disable();
  rtc_wdt_set_stage(RTC_WDT_STAGE0, RTC_WDT_STAGE_ACTION_RESET_RTC);
  rtc_wdt_set_time(RTC_WDT_STAGE0, WDT_TIMEOUT); // timeout rtd_wdt 10000ms.
  rtc_wdt_enable();           //Start the RTC WDT timer
  rtc_wdt_protect_on();       //Enable RTC WDT write protection
}

void setup() {
  Serial.begin(115200);
  delay(3000);
  logIt("\n >> Sistema Integrado de meteorologia << \n");

  pinMode(LED1,OUTPUT);
  pinMode(LED2,OUTPUT);
  pinMode(LED3,OUTPUT);
  digitalWrite(LED1,HIGH);
  digitalWrite(LED2,LOW);
  digitalWrite(LED3,LOW);
  
  pinMode(PLV_PIN, INPUT_PULLDOWN);
  pinMode(ANEMOMETER_PIN, INPUT_PULLUP);
  pinMode(16, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PLV_PIN), pluviometerChange, RISING);
  attachInterrupt(digitalPinToInterrupt(ANEMOMETER_PIN), anemometerChange, FALLING);

  logIt("\nIniciando cartão SD");
  initSdCard();
  
  logIt("\nCriando diretorios padrões");
  createDirectory("/metricas");
  createDirectory("/logs");

  logIt("\n1. Estação iniciada;", true);
  loadConfiguration(SD, configFileName, config, jsonConfig);

  logIt("\n1.1 Iniciando bluetooth;", true);
  BLE::Init(config.station_name, bluetoothController);
  BLE::updateValue(CONFIGURATION_UUID, jsonConfig);

  logIt("\n1.2 Estabelecendo conexão com wifi ", true);
  setupWifi("  - Wifi", config.wifi_ssid, config.wifi_password);
  int nivelDbm = (WiFi.RSSI()) * -1;
  storeLog((String(nivelDbm) + ";").c_str());

  logIt("\n1.3 Estabelecendo conexão com NTP;", true);
  connectNtp("  - NTP");

  logIt("\n1.4 Estabelecendo conexão com MQTT;", true);
  mqqtClient1.setupMqtt("  - MQTT", config.mqtt_server, config.mqtt_port, config.mqtt_username, config.mqtt_password, config.mqtt_topic);
   softwareReleaseMqttTopic =String("software-release/") + String(config.station_name);

  mqqtClient2.setupMqtt("- MQTT2", config.mqtt_hostV2_server, config.mqtt_hostV2_port, config.mqtt_hostV2_username, config.mqtt_hostV2_password, softwareReleaseMqttTopic.c_str());
  mqqtClient2.setCallback(mqttSubCallback);
  mqqtClient2.setBufferSize(512);
  mqqtClient2.subscribe((String("sys/") + String(config.station_name)).c_str());
  sysReportMqqtTopic =(String("sys-report/")+String(config.station_name));

  logIt("\n\n1.5 Iniciando controllers;", true);
  setupSensors();

  int now = millis();
  lastVVTImpulseTime = now;
  lastPVLImpulseTime = now;

  // 2; Inicio
  Serial.printf("\n >> PRIMEIRA ITERAÇÃO\n");

  int timestamp = timeClient.getEpochTime();
  convertTimeToLocaleDate(timestamp);
  
  String dataHora = String(formatedDateString) + "T" + timeClient.getFormattedTime();
  storeLog(("\n" + dataHora + "\n").c_str());

  // -- WATCH-DOG
  watchdogRTC();

  for(int i=0; i<7; i++) {
    digitalWrite(LED1,i%2);
    delay(400);
  }
  
  mqqtClient2.publish((sysReportMqqtTopic+String("/handshake")).c_str(), "{\"version\":\""FIRMWARE_VERSION"\"}");
  startTime = millis();
}

void loop() {
  digitalWrite(LED3,HIGH);
  rtc_wdt_feed(); // -- WATCH-DOG

  timeClient.update();
  int timestamp = timeClient.getEpochTime();
  convertTimeToLocaleDate(timestamp);
  resetSensors();

  do {

    if (Serial.available() > 0) {
      char command = Serial.read();
      if (command == 'r' || command == 'R') {
        Serial.println("Restarting...");
        delay(100);
        ESP.restart();
      }
    }
  
    unsigned long now = millis();
    timeRemaining = startTime + config.interval - now;

    WindGustRead(now);

    if(ceil(timeRemaining % 5000) != 0) continue;

    // Health check
    healthCheck.timestamp = timestamp;
    healthCheck.isWifiConnected = WiFi.status() == WL_CONNECTED;
    healthCheck.wifiDbmLevel = !healthCheck.isWifiConnected ? 0 : (WiFi.RSSI()) * -1;
    healthCheck.isMqttConnected = mqqtClient1.loopMqtt();
    healthCheck.timeRemaining = timeRemaining;

    const char * hcCsv = parseHealthCheckData(healthCheck, 1);
  
    Serial.printf("\n\nColetando dados, metricas em %d segundos ...", (timeRemaining / 1000));
    Serial.printf("\n  - %s",hcCsv);

    // Garantindo conexão com mqqt broker;
    if (healthCheck.isWifiConnected && !healthCheck.isMqttConnected) {
      healthCheck.isMqttConnected = mqqtClient1.connectMqtt("\n  - MQTT", config.mqtt_username, config.mqtt_password, config.mqtt_topic);
    }

    if(healthCheck.isWifiConnected && !mqqtClient2.loopMqtt()) {
      mqqtClient2.connectMqtt("\n  - MQTT2", config.mqtt_hostV2_username, config.mqtt_hostV2_password, softwareReleaseMqttTopic.c_str());
    }
  
    // Atualizando BLE advertsting value
    BLE::updateValue(HEALTH_CHECK_UUID, ("HC: " + String(hcCsv)).c_str());

    // Garantindo Tempo ocioso para captação de metricas 60s
  } while (timeRemaining > 0);

  startTime = millis();
  // Computando dados
  
  Serial.printf("\n\n Computando dados ...\n");

  Data.timestamp = timestamp;
  Data.wind_dir = getWindDir();
  Data.rain_acc = rainCounter * VOLUME_PLUVIOMETRO;
  Data.wind_gust  = 3.052f /3.0f* ANEMOMETER_CIRC *findMax(rps,sizeof(rps)/sizeof(int));
  Data.wind_speed = 3.052 * (ANEMOMETER_CIRC * anemometerCounter) / (config.interval / 1000.0); // m/s
  
  DHTRead(Data.humidity, Data.temperature);
  BMPRead(Data.pressure);

  // Apresentação
  parseData();
  Serial.printf("\nResultado CSV:\n%s", metricsCsvOutput); 
  Serial.printf("\nResultado JSON:\n%s\n", metricsjsonOutput);

  // Armazenamento local
  Serial.println("\n Gravando em disco:");
  storeMeasurement("/metricas", formatedDateString, metricsCsvOutput);

  // Enviando Dados Remotamente
  Serial.println("\n Enviando Resultados:  ");
  bool measurementSent1 = mqqtClient1.publish(config.mqtt_topic, metricsjsonOutput);
  bool measurementSent2 = mqqtClient2.publish(config.mqtt_topic, metricsjsonOutput);
  if(!measurementSent1)
    storeMeasurement("/falhas", formatedDateString, metricsCsvOutput);
  // Update metrics advertsting value
  BLE::updateValue(HEALTH_CHECK_UUID, ("ME: " + String(metricsCsvOutput)).c_str());
  Serial.printf("\n >> PROXIMA ITERAÇÃO\n");
}

// callbacks
int bluetoothController(const char *uid, const std::string &content) {
  if (content.length() == 0) return 0;
  printf("Bluetooth message received: %s\n", uid);
  if (content == "@@RESTART") {
    logIt("Reiniciando Arduino a força;", true);
    delay(2000);
    ESP.restart();
    return 1;
  } else if(content == "@@BLE_SHUTDOWN") {
    logIt("Desligando o BLE permanentemente", true);
    delay(2000);
    BLE::stop();
    return 1;
  } else {
    logIt("Modificando configuração de ambiente via bluetooth", true);
    delay(2000);
    createFile(SD, "/config.txt", content.c_str());
    logIt("Reiniciando Arduino a força;", true);
    ESP.restart();
    return 1;
  }
  return 0;
}


void mqttSubCallback(char* topic, unsigned char* payload, unsigned int length) {
  Serial.println("executing OTA");

  char* jsonBuffer = new char[length + 1-9];
  memcpy(jsonBuffer, (char*)payload+8, length-9);
  jsonBuffer[length-9] = '\0';
  Serial.println(jsonBuffer);
  Serial.println(topic);
  DynamicJsonDocument doc(length + 1);
  DeserializationError error = deserializeJson(doc, jsonBuffer);
  
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    delete[] jsonBuffer;
    return;
  }

  if(strcmp(topic,softwareReleaseMqttTopic.c_str())==0) {
    // Extract the value of the "data" field (assuming it's a string)
    const char* url = doc["url"];
    const char* id = doc["id"];
    if(!id) return;

    Serial.print("URL: ");
    Serial.println(url);
    String urlStr(url);

    const size_t capacity = 128;
    char jsonString[capacity];
    snprintf(jsonString, sizeof(jsonString), "{\"id\":\"%s\",\"status\":1}", id);
    mqqtClient2.publish((sysReportMqqtTopic+String("/OTA")).c_str(), jsonString);
    bool result = OTA::update(urlStr);
    
    printf("%d",result);
    if(healthCheck.isWifiConnected && !mqqtClient2.loopMqtt()) {
      mqqtClient2.connectMqtt("\n  - MQTT2", config.mqtt_hostV2_username, config.mqtt_hostV2_password, softwareReleaseMqttTopic.c_str());
    }

    if(result == true ){
      snprintf(jsonString, sizeof(jsonString), "{\"id\":\"%s\",\"status\":2}", id);
      mqqtClient2.publish((sysReportMqqtTopic+String("/OTA")).c_str(), jsonString);
    } else {
      snprintf(jsonString, sizeof(jsonString), "{\"id\":\"%s\",\"status\":4}", id);
      mqqtClient2.publish((sysReportMqqtTopic+String("/OTA")).c_str(), jsonString);
    }

    Serial.println("Update successful!");
    Serial.println("Reiniciando");
    delay(1500);
    ESP.restart();
  }

  // Release dynamically allocated memory
  delete[] jsonBuffer;
}


void convertTimeToLocaleDate(long timestamp) {
  struct tm *ptm = gmtime((time_t *)&timestamp);
  int day = ptm->tm_mday;
  int month = ptm->tm_mon + 1;
  int year = ptm->tm_year + 1900;
  formatedDateString = String(day) + "-" + String(month) + "-" + String(year);
}

// Dever ser usado somente para debugar
void wifiWatcher(){
  if (!healthCheck.isWifiConnected ) {
    if(++wifiDisconnectCount > 60) {
      logIt("Reiniciando a forca, problemas no wifi identificado.");
      ESP.restart();
    };

  } else {
    wifiDisconnectCount = 0;
  }
}