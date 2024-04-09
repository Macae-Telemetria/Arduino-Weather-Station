bool next(String& fileContent, String& fileName) {
    entry = dir.openNextFile();
    if (!entry) {
      return 0;
    }

    fileName = entry.name();
    int count =0;
    int partitions=0;
    int resultado=0;

    while (entry.available()){
      char c = (char)entry.read();
      fileContent += c;
      count++;
      if(count >=1000 && (c=='\n')) {

        resultado = sendFilehttp(
          insertStringBeforeExtension(fileName, String("_part") + String(partitions)),
          fileContent, 
          "http://192.168.0.173:3001/iotgateway/bulk-upload/test-local-1");
        
        partitions++;
        count =0;
        fileContent = "";
      }
    }
    
    entry.close(); 
    if (resultado > 200 && resultado < 204) {
      // deleteFile(fileName);
    }
    return 1;
  }


  bool openDir(const char* dirName) { 
    dir = SD.open(dirName);
    Serial.println(dir.name());
    if (!dir) {
      Serial.println("Failed to open directory");
      return 0;
    }
    dirNome=dirName;
    return 1;
  }


  //-----------------------------------------------------//
//int sendFilehttp(const String& fileName,const String& inputData, const String& url);
//int sendMultiPartFile(File& file, const String& url);
//int streamFile(File& file, const String& url);
//-----------------------------------------------------//

//String insertStringBeforeExtension(const String& originalFilename, const String& insertString) ;

//----------------------------------------------//


//----------------------------------------------//
namespace BK {
  File dir;
  File entry;
  String dirNome;

  void deleteFile(const String& fileName) {
    if (SD.remove(dirNome+String("/")+fileName)) {
      Serial.print("File ");
      Serial.print(fileName);
      Serial.println(" deleted successfully.");
    }
  }

  bool openDir(const char* dirName) { 
    dir = SD.open(dirName);
    Serial.println(dir.name());
    if (!dir) {
      Serial.println("Failed to open directory");
      return 0;
    }
    dirNome=dirName;
    return 1;
  }

  bool execute(){
    Serial.println("Iniciando backup de arquivos csv.");

    if(!BK::openDir("/metricas")){
      Serial.println("Não foi possivel abrir pasta de metricas.");
      return 0;
    }

    Serial.println("Pasta de metricas aberta com sucesso!");

    int filesCount = 0;
    int filesUploaded = 0;

    String url = "http://192.168.0.173:3001/iotgateway/backup";
    while(entry = dir.openNextFile()){        
      OnDebug(Serial.print("Arquivo encontrado: ");
      Serial.println(entry.name());)
      int resutlado = streamFile(entry, url);
      filesCount++;
      OnDebug(Serial.println("");)
      delay(2000);
    } 

    OnDebug(Serial.println("Arquivos enviados com sucesso!");
    Serial.print("Total de arquivos:");
    Serial.println(filesCount);)
    return 1;
  }

  

}



/* BK */
  int sendhttp(const String& fileName,const String& inputData, const String& url) {
    
    Serial.println(url);
    HTTPClient http;

    if(!http.begin(wifiClient,url)){
      Serial.println("Falhou em iniciar conexão.");
      return 0;
    }

    Serial.print("FileName: ");Serial.println(fileName);

    // form data
    String boundary = "----ArduinoBoundary";
    String data = "";

    // Add file1 to the request
    data += "--" + boundary + "\r\n";
    data += "Content-Disposition: form-data; name=\"files\"; filename=\""+fileName+"\"\r\n";
    data += "Content-Type: text/csv\r\n\r\n";

    data += inputData;

    // End of request
    data += "\r\n--" + boundary + "--\r\n";
    http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary); 

    // Send end of file marker
    int httpResponseCode = http.POST(data);

    if (httpResponseCode > 0) {
      OnDebug(Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      Serial.println("File uploaded successfully.");)
    } else {
      OnDebug(Serial.print("Error code: ");
      Serial.println(httpResponseCode);)
    }
    // Close connection
    http.end();

    return httpResponseCode;
  }

  int sendMultiPartFile(File& file, const String& url) {
    HTTPClient http;
    String boundary = "----ArduinoBoundary";

    String fileName = file.name();
    const int chunkSize = 2048;
    uint8_t buffer[chunkSize]{0};

    if (!http.begin(wifiClient, url)) {
      Serial.println("Failed to begin connection.");
      return 0;
    }
    Serial.println("Conexão iniciado com sucesso. " + String(url));

    file.seek(0, fs::SeekEnd);
    int fileSize = file.position();
    if (fileSize >= 0) {
      Serial.print("File size: ");
      Serial.println(fileSize);
      file.seek(0, fs::SeekSet);
    } else {
      Serial.println("Error getting file size.");
      return 0;
    }

    int totalSent = 0;
    int chunkCount = fileSize / chunkSize + (fileSize % chunkSize ? 1 : 0);
    int chunkIndex = 0;

    http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary); 
    Serial.println("Enviando pacotes: "+ String(fileSize)+ "byte(s)");

    while (file.available()) {
      chunkIndex++;
      Serial.println("=> ["+ String(chunkIndex)+ "/" + String(chunkCount)+"]");

      memset(buffer, 0, chunkSize);
      size_t bytesRead = file.readBytes((char*)buffer, chunkSize);

      // form data
      String formData = "";
      formData += "--" + boundary + "\r\n";
      formData += "Content-Disposition: form-data; name=\"files\"; filename=\""+fileName+"\"\r\n";
      formData += "Transfer-Encoding: chunked\r\n\r\n";
      formData += "Content-Type: text/csv\r\n\r\n";
      formData += String((char*)buffer, bytesRead); // + "\r\n";
    
      // X-headers
      formData += "\r\n--" + boundary + "\r\n";
      formData += "Content-Disposition: form-data; name=\"test\"\r\n";
      formData += "Content-Type: text/plain\r\n\r\n";
      formData += "conteudo-teste";

      formData += "\r\n--" + boundary + "--\r\n";
      int httpResponseCode = http.POST(formData);
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      if(httpResponseCode >= 200){
        totalSent = bytesRead;
      }
    }

    http.end();
    return 1;
  }