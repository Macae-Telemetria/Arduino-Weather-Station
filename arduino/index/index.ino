// Autor: Lucas Fonseca e Gabriel Fonseca
// Titulo: Sit arduino
// Versão: 1.8.0 HTTP send metrics;
#define FIRMWARE_VERSION 1.8

//.........................................................................................................................

#include "constants.h"
#include "data.h"
#include "sd-repository.h"
#include "integration.h"
#include "sensores.h"
#include "esp_system.h"
#include "bt-integration.h"
#include "backup.h"
//#include <PubSubClient.h>
#include <stdio.h>
#include <string>
#include <rtc_wdt.h>
#include <NTPClient.h>
namespace TLM{
extern NTPClient timeClient;
}
//extern PubSubClient mqttClient;
//#include "OTA.h"
// -- WATCH-DOG
#define WDT_TIMEOUT 350000   
// Pluviometro
extern unsigned long lastPVLImpulseTime;
extern unsigned int rainCounter;

// Anemometro (Velocidade do vento)
extern unsigned long lastVVTImpulseTime;
extern float anemometerCounter;
extern int rps[20];

#define STRINGIZE(x) #x
#define EXPAND_AND_STRINGIZE(x) STRINGIZE(x)
// globals
long startTime;
int timeRemaining=0;
std::string jsonConfig;
String formatedDateString = "";
struct HealthCheck healthCheck = {EXPAND_AND_STRINGIZE(FIRMWARE_VERSION), 0, false, false, 0, 0};

//-------------Do every ...--------------------//
bool doneSendingBackup = 0;
//--------------------------------------------//

void logIt(const String &message, bool store = false){
  Serial.print(message);
  if(store == true){
    storeLog(message.c_str());
  }
}


void setup() {
  delay(2000);
  logIt("\n >>SInmeteorologia<<\n");
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
  
  logIt("\ncdp");
  createDirectory("/metricas");
  createDirectory("/logs");

  logIt("\n1. on;", true);
  loadConfiguration(SD, configFileName, config, jsonConfig);

  logIt("\n1.1 Init bt;", true);
  BLE::Init(config.station_name, bluetoothController);
  BLE::updateValue(CONFIGURATION_UUID, jsonConfig);

  logIt("\n1.2 Estabelecendo conexão com wifi ", true);
  TLM::setupWifi("  - Wifi", config.wifi_ssid, config.wifi_password);
  int nivelDbm = (TLM::getWifiSginal()) * -1;
  storeLog((String(nivelDbm) + ";").c_str());

  logIt("\n1.3 Estabelecendo conexão com NTP;", true);
  TLM::connectNtp("  - NTP");

  logIt("\n1.4 Estabelecendo conexão com MQTT;", true);
  TLM::setupMqtt("  - MQTT", config.mqtt_server, config.mqtt_port, config.mqtt_username, config.mqtt_password, config.mqtt_topic);

  logIt("\n\n1.5 Iniciando controllers;", true);
  setupSensors();

  int now = millis();
  lastVVTImpulseTime = now;
  lastPVLImpulseTime = now;

  // 2; Inicio
  OnDebug(Serial.printf("\n >> PRIMEIRA ITERAÇÃO\n");)

  int timestamp = TLM::timeClient.getEpochTime();
  convertTimeToLocaleDate(timestamp);
  
  String dataHora = String(formatedDateString) + "T" + TLM::timeClient.getFormattedTime();
  storeLog(("\n" + dataHora + "\n").c_str());

  // -- WATCH-DOG -- //
    rtc_wdt_protect_off();      //Disable RTC WDT write protection
    rtc_wdt_disable();
    rtc_wdt_set_stage(RTC_WDT_STAGE0, RTC_WDT_STAGE_ACTION_RESET_RTC);
    rtc_wdt_set_time(RTC_WDT_STAGE0, WDT_TIMEOUT); // timeout rtd_wdt 10000ms.
    rtc_wdt_enable();           //Start the RTC WDT timer
    rtc_wdt_protect_on();       //Enable RTC WDT write protection
  // ----------------- //

  //Yellow Blink
  for(int i=0; i<7; i++) {
    digitalWrite(LED1,i%2);
    delay(400);
  }

  startTime = millis();

  String up;
  readFile(SD,"/updated",up);
  doneSendingBackup = (up.toInt()==(timestamp / 3600));
  
}

void loop() {
  digitalWrite(LED3,HIGH);

  // -- WATCH-DOG
  rtc_wdt_feed();

  // -- NTP
  TLM::timeClient.update();
  int timestamp = TLM::timeClient.getEpochTime();

  /* -- BACKUP AGENDADO -- */
  int hourNow = (timestamp / 3600) % 24;
  if (hourNow == ( (config.backup_time + 3)  % 24)) {
    if(!doneSendingBackup) {
      BK::execute();
      doneSendingBackup = true;
      createFile(SD, "/updated", String(timestamp / 3600).c_str());
    }
  }
  else { doneSendingBackup = false;}

  convertTimeToLocaleDate(timestamp);
  resetSensors();

  do {

  OnDebug(
    if (Serial.available() > 0) {
      char command = Serial.read();
      if (command == 'R' || command == 'r') { // 'R' or 'r' will trigger restart
        OnDebug(Serial.println("Restarting ESP32...");)
        delay(1000); // Give time for serial to transmit
        ESP.restart(); // Restart the ESP32
      }
    })


    unsigned long now = millis();
    timeRemaining = startTime + config.interval - now;
    //calculate
    WindGustRead(now);

    if((timeRemaining % 5000) != 0) continue;

    // Health check
    healthCheck.timestamp = timestamp;
    healthCheck.isWifiConnected = TLM::isWifiConnected();
    healthCheck.wifiDbmLevel = healthCheck.isWifiConnected *(TLM::getWifiSginal()) * -1;
    healthCheck.isMqttConnected = TLM::loopMqtt();
    healthCheck.timeRemaining = timeRemaining;

    const char * hcCsv = parseHealthCheckData(healthCheck, 1);

    OnDebug(Serial.printf("\n\nColetando dados, metricas em %d segundos ...", (timeRemaining / 1000));)
    OnDebug(Serial.printf("\n  - %s",hcCsv));

    // Garantindo conexão com mqqt broker;
    if (healthCheck.isWifiConnected && !healthCheck.isMqttConnected) {
      healthCheck.isMqttConnected = TLM::connectMqtt("\n  - MQTT", config.mqtt_username, config.mqtt_password, config.mqtt_topic);
    }

    // Atualizando BLE advertsting value
    BLE::updateValue(HEALTH_CHECK_UUID, ("HC: " + String(hcCsv)).c_str());

    // Garantindo Tempo ocioso para captação de metricas 60s
  } while (timeRemaining > 0);
  startTime = millis();
  // Computando dados
  
  OnDebug(Serial.printf("\n\n Computando dados ...\n");)

  Data.timestamp = timestamp;
  Data.wind_dir = getWindDir();
  Data.rain_acc = rainCounter * VOLUME_PLUVIOMETRO;
  Data.wind_gust  = 3.052f /3.0f* ANEMOMETER_CIRC *findMax(rps,sizeof(rps)/sizeof(int));
  Data.wind_speed = 3.052 * (ANEMOMETER_CIRC * anemometerCounter) / (config.interval / 1000.0); // m/s
  
  DHTRead(Data.humidity, Data.temperature);
  BMPRead(Data.pressure);

  // Apresentação
  parseData();
  //Serial.printf("\nResultado CSV:\n%s", metricsCsvOutput); 
  Serial.printf("\nResultado JSON:\n%s\n", metricsjsonOutput);



  // Enviando Dados Remotamente
  Serial.println("\n Enviando Resultados:  ");
  bool measurementSent = TLM::sendMeasurementToMqtt(config.mqtt_topic, metricsjsonOutput);
  if(!measurementSent)
    storeMeasurement("/failures", "falhas.csv", metricsCsvOutput);

  // Armazenamento local
  OnDebug(Serial.println("\n Gravando em disco:");)
  storeMeasurement("/metricas", formatedDateString, metricsCsvOutput);

  // Update metrics advertsting value
  BLE::updateValue(HEALTH_CHECK_UUID, ("ME: " + String(metricsCsvOutput)).c_str());
  OnDebug(Serial.printf("\n >> PROXIMA ITERAÇÃO\n");)
}

// callbacks
int bluetoothController(const char *uid, const std::string& content) {
  if (content.length() == 0) return 0;
  OnDebug(printf("Bluetooth message received: %s\n", uid);)
  if (content == "@@RESTART") {
    logIt("RAF;", true);
    delay(2000);
    ESP.restart();
    return 1;
  } else if(content == "@@BLE_SHUTDOWN") {
    logIt("BLEOFF ", true);
    delay(2000);
    BLE::stop();
    return 1;
  } else {
    logIt("Mcdavbt", true);
    delay(2000);
    createFile(SD, "/config.txt", content.c_str());
    logIt("Reiniciando Arduino a força;", true);
    ESP.restart();
    return 1;
  }
  return 0;
}

void convertTimeToLocaleDate(long timestamp) {
  struct tm *ptm = gmtime((time_t *)&timestamp);
  int day = ptm->tm_mday;
  int month = ptm->tm_mon + 1;
  int year = ptm->tm_year + 1900;
  formatedDateString = String(day) + "-" + String(month) + "-" + String(year);
}
