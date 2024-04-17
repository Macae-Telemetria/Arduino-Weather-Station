// Autor: Lucas Fonseca e Gabriel Fonseca
// Titulo: Sit arduino
// Versão: 1.7.5 adjustments;
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

#define WDT_TIMEOUT 150000 // -- WATCH-DOG

extern unsigned long lastPVLImpulseTime; // Pluviometro
extern unsigned int rainCounter;
extern unsigned long lastVVTImpulseTime; // Anemometro (Velocidade do vento)
extern float anemometerCounter;
extern int rps[20];

// Sensors
extern Sensors sensors;


#define STRINGIZE(x) #x
#define EXPAND_AND_STRINGIZE(x) STRINGIZE(x)
// globals
long startTime;
int timeRemaining=0;
std::string jsonConfig;
String formatedDateString = "";
struct HealthCheck healthCheck = {EXPAND_AND_STRINGIZE(FIRMWARE_VERSION), 0, false, false, 0, 0};
MQTT client1;
MQTT client2;

void logIt(const std::string &message, bool store = false){
  Serial.print(message.c_str());
  if(store == true){
    storeLog(message.c_str());
  }
}

void caubeque(char* topic, byte* payload, unsigned int length);

void watchdogRTC()
{
    rtc_wdt_protect_off();      //Disable RTC WDT write protection
    rtc_wdt_disable();
    rtc_wdt_set_stage(RTC_WDT_STAGE0, RTC_WDT_STAGE_ACTION_RESET_RTC);
    rtc_wdt_set_time(RTC_WDT_STAGE0, WDT_TIMEOUT); // timeout rtd_wdt 10000ms.
    rtc_wdt_enable();           //Start the RTC WDT timer
    rtc_wdt_protect_on();       //Enable RTC WDT write protection
}

void setup() {
  delay(3000);
  logIt("\n >> Sistema Integrado de meteorologia << \n");
  pinMode(LED1,OUTPUT);
  pinMode(LED2,OUTPUT);
  pinMode(LED3,OUTPUT);
  digitalWrite(LED1,HIGH);
  digitalWrite(LED2,LOW);
  digitalWrite(LED3,LOW);
  Serial.begin(115200);

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
  client1.setupMqtt("  - MQTT", config.mqtt_server, config.mqtt_port, config.mqtt_username, config.mqtt_password, config.mqtt_topic);
  client2.setupMqtt("- MQTT",config.mqtt_hostV2_server, config.mqtt_hostV2_port, config.mqtt_hostV2_username, config.mqtt_hostV2_password, config.station_name);
  client2.setCallback(caubeque);

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

  //Yellow Blink
  for(int i=0; i<7; i++) {
    digitalWrite(LED1,i%2);
    delay(400);
  }

  startTime = millis();
  //config.IOT_GATEWAY_HOST = http://146.190.171.
  //config.iotGatewayHost = http://146.190.171.



}

void loop() {
  digitalWrite(LED3,HIGH);

  // -- WATCH-DOG
  rtc_wdt_feed();
  // -- WATCH-DOG


  timeClient.update();
  int timestamp = timeClient.getEpochTime();

  convertTimeToLocaleDate(timestamp);

  resetSensors();

  do {
    unsigned long now = millis();
    timeRemaining = startTime + config.interval - now;
    //calculate
    client2.loopMqtt();
    WindGustRead(now);
    if(ceil(timeRemaining % 5000) != 0) continue;

    // Health check
    healthCheck.timestamp = timestamp;
    healthCheck.isWifiConnected = WiFi.status() == WL_CONNECTED;
    healthCheck.wifiDbmLevel = !healthCheck.isWifiConnected ? 0 : (WiFi.RSSI()) * -1;
    healthCheck.isMqttConnected = client1.loopMqtt();
    healthCheck.timeRemaining = timeRemaining;

    const char * hcCsv = parseHealthCheckData(healthCheck, 1);

    Serial.printf("\n\nColetando dados, metricas em %d segundos ...", (timeRemaining / 1000));
    Serial.printf("\n  - %s",hcCsv);

    // Garantindo conexão com mqqt broker;
    if (healthCheck.isWifiConnected && !healthCheck.isMqttConnected) {
      healthCheck.isMqttConnected = client1.connectMqtt("\n  - MQTT", config.mqtt_username, config.mqtt_password, config.mqtt_topic);
    }
    if(!client2.loopMqtt())
      client2.connectMqtt("\n  - MQTT", config.mqtt_hostV2_username, config.mqtt_hostV2_password, config.station_name);

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
  bool measurementSent = client1.sendMeasurementToMqtt(config.mqtt_topic, metricsjsonOutput);
  client2.sendMeasurementToMqtt((String(config.station_name)+String("/measurement")).c_str(), metricsjsonOutput);
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

void caubeque(char* topic, byte* payload, unsigned int length) {
  Serial.print("\nPayload: ");
  for (int i = 0; i < length; i++) {
  Serial.write((char)payload[i]);
  }
  Serial.write('\n');
  if(strncmp((char*)payload, "restart", strlen("restart")) == 0)
  {
    logIt("Reiniciando Arduino a força;", true);
    delay(1200);
    ESP.restart();
  }

  char prefix[] = "http://";
  if (strncmp((char*)payload, prefix, strlen(prefix)) == 0) {
    Serial.println("Received OTA update trigger");
    OTA::update(String((const char*)payload,length));
  }

  client2.sendMeasurementToMqtt((String(config.station_name)+String("/response")).c_str(),healthCheck.softwareVersion );
}


void convertTimeToLocaleDate(long timestamp) {
  struct tm *ptm = gmtime((time_t *)&timestamp);
  int day = ptm->tm_mday;
  int month = ptm->tm_mon + 1;
  int year = ptm->tm_year + 1900;
  formatedDateString = String(day) + "-" + String(month) + "-" + String(year);
}