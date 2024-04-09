#pragma once
#include "SD.h"
#include "data.h"
//void SD_BLINK(int interval);

// Inicia leitura cartão SD
void initSdCard();

// Adicionar novo diretorio
void createDirectory(const char * directory);

// Carrega arquivo de configuração inicial
void loadConfiguration(fs::FS &fs, const char *filename, Config &config, std::string& configJson);

//void readFile(fs::FS &fs, const String& path, String &data);

void createFile(fs::FS &fs, const char * path, const char * message);

//void appendFile(fs::FS &fs, const char * path, const char * message);

void storeMeasurement(String directory, String fileName, const char *payload);

// Adicion uma nova linha de metricas
void storeLog(const char *payload);




