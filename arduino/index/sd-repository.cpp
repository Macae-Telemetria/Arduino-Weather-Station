#include "sd-repository.h"

#include <sd_defines.h>
#include <sd_diskio.h>

#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "constants.h"
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
    OnDebug(Serial.printf("\n  - Cartão não encontrado. tentando novamente em %d segundos ...", 2);)
    SD_BLINK(2000);}

  OnDebug(Serial.printf("\n  - Leitor de Cartão iniciado com sucesso!.\n");)
}

// Adicionar novo diretorio
void createDirectory(const char * directory){
  OnDebug(Serial.printf("\n  - Tentando Criando novo diretorio: %s.", directory);)
  if (!SD.exists(directory)) {
    if (SD.mkdir(directory)) {
      OnDebug(Serial.printf("\n- Diretorio criado com sucesso!");)
    } else {
      OnDebug(Serial.printf("\n- Falha ao criar diretorio.");)
    }
    return;
  }
  OnDebug(Serial.printf("\n - Diretorio já existe.");)
}

// Carrega arquivo de configuração inicial
void loadConfiguration(fs::FS &fs, const char *filename, Config &config, std::string& configJson) {
  OnDebug(Serial.printf("\n - Carregando variáveis de ambiente");)

  SPI.begin(clockPin, misoPin, mosiPin);

  int attemptCount = 0;
  bool success = false;
  while (success == false) {

    OnDebug(Serial.printf("\n - Iniciando leitura do arquivo de configuração %s (tentativa: %d)", filename, attemptCount + 1);)

    if (SD.begin(chipSelectPin, SPI)){
      File file = fs.open(filename);
      StaticJsonDocument<512> doc;

      if (file){
        DeserializationError error = deserializeJson(doc, file);
        if (!error){
          strlcpy(config.station_uid, doc["STATION_UID"] | "", sizeof(config.station_uid));
          strlcpy(config.station_name, doc["STATION_NAME"] | "", sizeof(config.station_name));
          strlcpy(config.wifi_ssid, doc["WIFI_SSID"] | "", sizeof(config.wifi_ssid));
          strlcpy(config.wifi_password, doc["WIFI_PASSWORD"] | "", sizeof(config.wifi_password));
          strlcpy(config.mqtt_server, doc["MQTT_SERVER"] | "", sizeof(config.mqtt_server));
          strlcpy(config.mqtt_username, doc["MQTT_USERNAME"] | "", sizeof(config.mqtt_username));
          strlcpy(config.mqtt_password, doc["MQTT_PASSWORD"] | "", sizeof(config.mqtt_password));
          strlcpy(config.mqtt_topic, doc["MQTT_TOPIC"] | "", sizeof(config.mqtt_topic));
          config.mqtt_port = doc["MQTT_PORT"] | 1883;
          config.interval = doc["INTERVAL"] | 60000;
          config.backup_time = doc["BACKUP_TIME"] | 0;
          file.close();
          success = true;
          serializeJson(doc, configJson);
          continue;
        }
        OnDebug(Serial.printf("\n - [ ERROR ] Formato inválido (JSON)\n");)
        OnDebug(Serial.println(error.c_str());)
      }
      OnDebug(Serial.printf("\n - [ ERROR ] Arquivo de configuração não encontrado\n");)
    } else {
      OnDebug(Serial.printf("\n - [ ERROR ] Cartão SD não encontrado.\n");)
    }

    OnDebug(Serial.printf("\n - Proxima tentativa de re-leitura em %d segundos ... \n\n\n", (RETRY_INTERVAL / 1000));)
    attemptCount++;
    SD_BLINK(RETRY_INTERVAL);

  }

  OnDebug(Serial.printf("\n - Variáveis de ambiente carregadas com sucesso!");)
  OnDebug(Serial.printf("\n - %s", configJson.c_str());)
  OnDebug(Serial.println();)
  return;
}

void readFile(fs::FS &fs, const String& path, String &data) {
  File file = fs.open(path, FILE_READ);
  if (!file) {
    OnDebug(Serial.println("Failed to open file for reading");)
    return;
  }
  data = "";
  while (file.available()) {
      data += (char)file.read();
  }
  file.close();
}


void createFile(fs::FS &fs, const char * path, const char * message){
    OnDebug(Serial.printf("Salvando json no cartao SD: %s\n.", path); )

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        OnDebug(Serial.println(" - Falha ao encontrar cartão SD.");)
        return;
    }
    if(file.print(message)){
        OnDebug(Serial.println(" - sucesso.");)
    } else {
        OnDebug(Serial.println("- Falha ao salvar.");)
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    OnDebug(Serial.printf(" - Salvando dados no cartao SD: %s\n", path);)

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        OnDebug(Serial.println(" - Falha ao encontrar cartão SD");)
        return;
    }
    if(file.print(message)){
        OnDebug(Serial.println(" - Nova linha salva com sucesso.");)
    } else {
        OnDebug(Serial.println(" - Falha ao salvar nova linha");)
    }
    file.close();
}


void storeMeasurement(String directory, String fileName, const char *payload){
  String path = directory + "/" + fileName + ".txt";
  if (!SD.exists(directory)) {
    if (SD.mkdir(directory)) {
      OnDebug(Serial.println(" - Diretorio criado com sucesso!");)
    } else {
      OnDebug(Serial.println(" - Falha ao criar diretorio de metricas.");)
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


String insertStringBeforeExtension(const String& originalFilename, const String& insertString) {
  int dotPosition = originalFilename.lastIndexOf('.');
  
  if (dotPosition != -1) {
    String filename = originalFilename.substring(0, dotPosition);
    String extension = originalFilename.substring(dotPosition);
    return filename + insertString + extension;
  } else {
    // If no dot is found, return an empty string to indicate an error
    return "";
  }
}

