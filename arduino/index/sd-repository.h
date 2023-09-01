#pragma once

#include "FS.h"
#include "SD.h"
#include "SPI.h"

#include <ArduinoJson.h>

const int chipSelectPin = 32;
const int mosiPin = 23;
const int misoPin = 27;
const int clockPin = 25;


void loadConfiguration(fs::FS &fs, const char *filename, Config &config)
{

  Serial.printf("Carregando Arquivo de configuração... %s\n", filename);

  StaticJsonDocument<512> doc;
  SPI.begin(clockPin, misoPin, mosiPin);

  if (SD.begin(chipSelectPin, SPI)) {
   
    File file = fs.open(filename);
    if (!file) Serial.println("Não foi possivel encontrar arquivo de configuração");

    DeserializationError error = deserializeJson(doc, file);
    if (error){
      Serial.println(F("** Formato do arquivo de configuração inválido **"));
      Serial.println(error.c_str());
    }
    file.close();
  } else {
    Serial.println(" ** Não foi possivel estabelecer conexão com leitor de Cartão SD. **\n");
  }
 
  strlcpy(config.station_uid, doc["STATION_UID"] | "01", sizeof(config.station_uid));
  strlcpy(config.station_name, doc["STATION_NAME"] | "est001", sizeof(config.station_name));
  strlcpy(config.wifi_ssid, doc["WIFI_SSID"] | "wifi", sizeof(config.wifi_ssid));
  strlcpy(config.wifi_password, doc["WIFI_PASSWORD"] | "wifi_password", sizeof(config.wifi_password));
  strlcpy(config.mqtt_server, doc["MQTT_SERVER"] | "localhost:3000", sizeof(config.mqtt_server));
  strlcpy(config.mqtt_username, doc["MQTT_USERNAME"] | "telemetria", sizeof(config.mqtt_username));
  strlcpy(config.mqtt_password, doc["MQTT_PASSWORD"] | "telemetria_password", sizeof(config.mqtt_password));
  strlcpy(config.mqtt_topic, doc["MQTT_TOPIC"] | "/fake_topic", sizeof(config.mqtt_topic));
  config.mqtt_port = doc["MQTT_PORT"] | 1883;
  config.interval = doc["INTERVAL"] | 60000;

  Serial.print("STATION_UID: ");
  Serial.println(config.station_uid);

  Serial.print("STATION_NAME: ");
  Serial.println(config.station_name);

  Serial.print("WIFI_SSID: ");
  Serial.println(config.wifi_ssid);

  Serial.print("WIFI_PASSWORD: ");
  Serial.println(config.wifi_password);

  Serial.print("MQTT_SERVER: ");
  Serial.println(config.mqtt_server);

  Serial.print("MQTT_USERNAME: ");
  Serial.println(config.mqtt_username);

  Serial.print("MQTT_PASSWORD: ");
  Serial.println(config.mqtt_password);

  Serial.print("MQRR_TOPIC: ");
  Serial.println(config.mqtt_topic);

  Serial.print("MQRR_PORT: ");
  Serial.println(config.mqtt_port);

  Serial.print("READ_INTERVAL: ");
  Serial.println(config.interval);

  Serial.println();
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

void storeMeasurement(String directory, String fileName,const char *payload){
  String path = directory + "/" + fileName + ".txt";
  if (!SD.exists(directory)) {
    if (SD.mkdir(directory)) {
      Serial.println(" - Diretorio criado com sucesso!");
    } else {
      Serial.println(" - Falha ao criar diretorio de metricas.");
    }
  }
  Serial.println(" - Atualizando arquivo.");
  appendFile(SD, path.c_str(), payload);
}

/*   
if (!SD.exists(path.c_str())){
    Serial.println(" - Criando novo arquivo.");
    appendFile(SD, path.c_str(), header);
  } 
*/

String readFileContent(File file) {
  String content = "";
  while (file.available()) {
    content += (char)file.read();
  }
  return content;
}


int removeFile(const char * filePath) {
  Serial.println('Deltando aquivo: ' + filePath);
  if (SD.remove(filePath)) {
    Serial.println("' deleted.");
    return 1;
  } else {
    Serial.print("Error deleting '");
    return 0;
  }
}

void loopThroughFiles(const char* dirName, int max, std::function<void(char*, char*)> callback ) {
  File root = SD.open(dirName);

  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }

  while (true) {
    File entry = root.openNextFile();
    if (!entry || max == 0) break;

    if (!entry.isDirectory()) {
      Serial.print("File: ");
      Serial.println(entry.name());
      
      String fileContent = readFileContent(entry);
      callback((char *)entry.name(), (char *) fileContent.c_str());
      max -=1;
    }
    entry.close();
  }
  root.close();
}
