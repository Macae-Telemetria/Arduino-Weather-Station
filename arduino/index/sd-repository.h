#include <SD.h>
#include <sd_defines.h>
#include <sd_diskio.h>

#pragma once

#include "FS.h"
#include "SD.h"
#include "SPI.h"

#include <ArduinoJson.h>

const int chipSelectPin = 32;
const int mosiPin = 23;
const int misoPin = 27;
const int clockPin = 25;
const int RETRY_INTERVAL = 5000;

void SD_BLINK(int interval)
{
  for(int i=0; i<interval/500;i++){
    digitalWrite(LED2,i%2);
    delay(500+((i%2==0) && (i%3>0))*1000);
}
}

// Inicia leitura cartão SD
void initSdCard(){
  SPI.begin(clockPin, misoPin, mosiPin);
  while(!SD.begin(chipSelectPin, SPI)) {
    Serial.printf("\n  - Cartão não encontrado. tentando novamente em %d segundos ...", 2);
    SD_BLINK(2000);}

  Serial.printf("\n  - Leitor de Cartão iniciado com sucesso!.\n");
}

// Adicionar novo diretorio
void createDirectory(const char * directory){
  Serial.printf("\n  - Tentando Criando novo diretorio: %s.", directory);
  if (!SD.exists(directory)) {
    if (SD.mkdir(directory)) {
      Serial.printf("\n     - Diretorio criado com sucesso!");
    } else {
      Serial.printf("\n     - Falha ao criar diretorio.");
    }
    return;
  }
  Serial.printf("\n     - Diretorio já existe.");
}
void parseMQTTString(const char *mqttString, char *username, char *password, char *broker, int &port) {
  if (memcmp(mqttString, "mqtt://", 7) != 0) {
    printf("Invalid MQTT string format!\n");
    return;
  } 
  int size = strlen(mqttString)+1;
  char *ptr = new char[size-7];
  strlcpy(ptr,mqttString+7,size-7);
  strlcpy(username, strtok(ptr, ":"), sizeof(config.mqtt_username));
  strlcpy(password, strtok(NULL, "@"), sizeof(config.mqtt_password));
  strlcpy(broker, strtok(NULL, ":"), sizeof(config.mqtt_server));
  port =  atoi(strtok(NULL, ""));

}
// Carrega arquivo de configuração inicial
void loadConfiguration(fs::FS &fs, const char *filename, Config &config, std::string& configJson) {
  Serial.printf("\n - Carregando variáveis de ambiente");

  SPI.begin(clockPin, misoPin, mosiPin);

  int attemptCount = 0;
  bool success = false;
  while (success == false) {

    Serial.printf("\n - Iniciando leitura do arquivo de configuração %s (tentativa: %d)", filename, attemptCount + 1);
    if (SD.begin(chipSelectPin, SPI)){
      File file = fs.open(filename);
      StaticJsonDocument<512> doc;
      if (file){
        DeserializationError error = deserializeJson(doc, file);
        if (!error){
          strlcpy(config.station_uid, doc["UID"] | "0", sizeof(config.station_uid));
          strlcpy(config.station_name, doc["SLUG"] | "est000", sizeof(config.station_name));
          strlcpy(config.wifi_ssid, doc["WIFI_SSID"] | "", sizeof(config.wifi_ssid));
          strlcpy(config.wifi_password, doc["WIFI_PASSWORD"] | "", sizeof(config.wifi_password));
          config.interval = doc["INTERVAL"] | 60000;
          strlcpy(config.mqtt_topic, doc["MQTT_TOPIC"] | "unnamed", sizeof(config.mqtt_topic));
          parseMQTTString(doc["MQTT_HOST_V1"],config.mqtt_username,config.mqtt_password,config.mqtt_server,config.mqtt_port);
          parseMQTTString(doc["MQTT_HOST_V2"],config.mqtt_hostV2_username,config.mqtt_hostV2_password,config.mqtt_hostV2_server,config.mqtt_hostV2_port);

          file.close();
          success = true;
          //serializeJsonPretty(doc, configJson);
          serializeJson(doc, configJson);
          continue;
        }
        Serial.printf("\n - [ ERROR ] Formato inválido (JSON)\n");
        Serial.println(error.c_str());
      }
      Serial.printf("\n - [ ERROR ] Arquivo de configuração não encontrado\n");
    } else {
      Serial.printf("\n - [ ERROR ] Cartão SD não encontrado.\n");
    }

    Serial.printf("\n - Proxima tentativa de re-leitura em %d segundos ... \n\n\n", (RETRY_INTERVAL / 1000));
    attemptCount++;
    SD_BLINK(RETRY_INTERVAL);

  }

  Serial.printf("\n - Variáveis de ambiente carregadas com sucesso!");
  Serial.printf("\n - %s", configJson.c_str());
  Serial.println();
  return;
}

void createFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Salvando json no cartao SD: %s\n.", path); 

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println(" - Falha ao encontrar cartão SD.");
        return;
    }
    if(file.print(message)){
        Serial.println(" - sucesso.");
    } else {
        Serial.println("- Falha ao salvar.");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf(" - Salvando dados no cartao SD: %s\n", path); 

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println(" - Falha ao encontrar cartão SD");
        return;
    }
    if(file.print(message)){
        Serial.println(" - Nova linha salva com sucesso.");
    } else {
        Serial.println(" - Falha ao salvar nova linha");
    }
    file.close();
}


void storeMeasurement(String directory, String fileName, const char *payload){
  String path = directory + "/" + fileName + ".txt";
  if (!SD.exists(directory)) {
    if (SD.mkdir(directory)) {
      Serial.println(" - Diretorio criado com sucesso!");
    } else {
      Serial.println(" - Falha ao criar diretorio de metricas.");
    }
  }
  appendFile(SD, path.c_str(), payload);
}


// Adicion uma nova linha de metricas
void storeLog(const char *payload){
  String path = "/logs/boot.txt";
  File file = SD.open(path, FILE_APPEND);
  if (file) { file.print(payload); }
  file.close();
} 