// Titulo: Integração HTTP & MQQT
// Data: 30/07/2023
//.........................................................................................................................

#pragma once
#include <WiFi.h> // #include <WiFiClientSecure.h>
#include <NTPClient.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
/**** WIFI Client Initialisation *****/
WiFiClient wifiClient;

/**** Secure WiFi Connectivity Initialisation *****/
// WiFiClientSecure secureWifiClient;

/**** NTP Client Initialisation  *****/
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
  Serial.printf("%s: Estabelecendo conexão inicial", contextName);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  // secureWifiClient.setCACert(root_ca);    // enable this line and the the "certificate" code for secure connection
  while (WiFi.status() != WL_CONNECTED) {
    for(int i=0; i<5;i++){
    digitalWrite(LED2,i%2);
    delay(200);}
      Serial.print(".");
  }
  
  Serial.printf("\n%s: Contectado com sucesso \n", contextName);
  Serial.printf("%s: IP address = %s \n", contextName, WiFi.localIP());
  return 1;
}

bool sendMeasurementToMqtt(char *topic, const char *payload)
{
  bool sent = (mqttClient.publish(topic, payload, true));
  if (sent){
    Serial.println("  - MQTT broker: Message publised [" + String(topic) + "]: ");
  } else {
    Serial.println("  - MQTT: Não foi possivel enviar");
  }
  return sent;
}

bool connectMqtt(const char *contextName, char* mqtt_username, char* mqtt_password, char* mqtt_topic) {
  if (!mqttClient.connected()) {
    Serial.printf("%s: Tentando nova conexão...", contextName);
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (mqttClient.connect(clientId.c_str(), mqtt_username, mqtt_password)){
      Serial.printf("%s: Reconectado", contextName);
      mqttClient.subscribe(mqtt_topic);
      return true;
    }
    Serial.print("failed, rc=");
    Serial.print(mqttClient.state());
    return false;
  }
  Serial.printf("%s: Conectado [ %s ]", contextName, mqtt_topic);
  return true;
}

bool setupMqtt(const char *contextName, char* mqtt_server, int mqtt_port, char* mqtt_username, char* mqtt_password, char* mqtt_topic){
  Serial.printf("%s: Estabelecendo conexão inicial\n", contextName);
  mqttClient.setServer(mqtt_server, mqtt_port);
  // mqttClient.setCallback(callback);
  return connectMqtt(contextName, mqtt_username, mqtt_password, mqtt_topic);
}

/* Ntp Client */
int connectNtp(const char *contextName)
{
  Serial.printf("%s: Estabelecendo conexão inicial\n", contextName);

  timeClient.begin();

  while(!timeClient.update()) {
    Serial.print(".");
    delay(1000);
  }

  Serial.printf("%s: Conectado com sucesso. \n", contextName);
  return 1;
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
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    Serial.println("File uploaded successfully.");
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
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
    delay(1000);
  }

  http.end();
  return 1;
}

int streamFile(File& file, const String& url) {
  HTTPClient http;
  String fileName = file.name();
  String slugStacao = config.station_name;

  // Iniciando o backup
  String iniciarBackupURl = url + "/start/" + slugStacao + "/" + fileName;
  if (!http.begin(wifiClient, iniciarBackupURl)) {
    Serial.println("Failed to begin connection.");
    return 0;
  }

  int httpResponseCode = http.sendRequest("GET");
  if (httpResponseCode != HTTP_CODE_OK) {
      Serial.println("Failed to upload chunk. HTTP response code: " + String(httpResponseCode));
      file.close();
      http.end();
      return 0;
  }
  String backupFilepath = http.getString();
  http.end();

  // Iniciar stream
  String streamBackupUrl = url + "/stream?f=" + backupFilepath;
  if (!http.begin(wifiClient, streamBackupUrl)) {
    Serial.println("Failed to begin connection.");
    file.close();
    http.end();
    return 0;
  }
  Serial.println("Conexão iniciado com sucesso. " + String(streamBackupUrl));

  const size_t chunkSize = 2048;
  uint8_t buffer[chunkSize];
  int bytesRead;
  while ((bytesRead = file.read(buffer, chunkSize)) > 0) {
    http.addHeader("Content-Type", "application/octet-stream");
    httpResponseCode = http.sendRequest("POST", buffer, bytesRead);
  
    if (httpResponseCode != HTTP_CODE_OK) {
      Serial.println("Failed to upload chunk. HTTP response code: " + String(httpResponseCode));
      file.close();
      http.end();
      return 0;
    }
  }

  Serial.println("Done!");
  file.close();
  http.end();

  // Finalizando o backup
  String finalizarBackupURl = url + "/finish?f=" + backupFilepath;
  if (!http.begin(wifiClient, finalizarBackupURl)) {
    Serial.println("Failed to begin connection.");
    return 0;
  }

  httpResponseCode = http.sendRequest("GET");
  if (httpResponseCode == HTTP_CODE_OK) {
    Serial.println("Backup realizado com sucesso!" + String(httpResponseCode));
    SD.remove( String("/metricas/") + fileName );
    return 1;
  }

  http.end();
  return 0;
}