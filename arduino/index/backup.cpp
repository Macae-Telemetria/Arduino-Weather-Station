#include "backup.h"
#include "constants.h"
#include "integration.h"
#include "data.h"
#include "SD.h"

//----------------------------------------------//

namespace TLM{
//Define in integration.cpp
int streamFile(File& file, const String& url);
}

namespace BK {
static const String dirNome="/metricas";

  void deleteFile(const String& fileName) {
    if (SD.remove(dirNome+String("/")+fileName)) {
      OnDebug(Serial.print("File ");)
      OnDebug(Serial.print(fileName);)
      OnDebug(Serial.println(" deleted successfully.");)
    }
  }

  bool execute(){
    String url = "http://146.190.171.0:3001/iotgateway/backup";
    File file = SD.open("/failures/falhas.csv", FILE_READ);
    
    OnDebug(Serial.print("Arquivo encontrado: ");
    Serial.println(file.name());)
    int resutlado = TLM::streamFile(file, url);
    OnDebug(Serial.println("");)
    SD.remove("/failures/falhas.csv");
    return 1;
    }

/*
  bool execute(){
    OnDebug(Serial.println("Iniciando backup de arquivos csv.");)
   
    
    File dir=SD.open("/metricas");
    if(!dir){
      OnDebug(Serial.println("Não foi possivel abrir pasta de metricas.");)
      return 0;
    }

    OnDebug(Serial.println("Pasta de metricas aberta com sucesso!");)

    int filesCount = 0;
    int filesUploaded = 0;

    String url = "http://146.190.171.0:3001/iotgateway/backup";
    File entry;
    while(entry = dir.openNextFile()){        
      OnDebug(Serial.print("Arquivo encontrado: ");
      Serial.println(entry.name());)
      int resutlado = TLM::streamFile(entry, url);
      filesCount++;
      OnDebug(Serial.println("");)
      delay(2000);
    } 

    OnDebug(Serial.println("Arquivos enviados com sucesso!");
    Serial.print("Total de arquivos:");
    Serial.println(filesCount);)
    return 1;
  }*/

  

}



