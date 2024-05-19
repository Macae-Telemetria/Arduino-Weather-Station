#include "mqtt.h"
#include <PubSubClient.h>
#include <WiFi.h>
#include "pch.h"
  MQTT::MQTT()
  {
    m_WifiClient = new WiFiClient();
    m_Client = new PubSubClient(*m_WifiClient);
  }
   MQTT::~MQTT()
  {
    delete m_WifiClient;
    delete m_Client;
  }
  bool MQTT::publish(const char *topic, const char *payload)
  {
    bool sent = (m_Client->publish(topic, payload, true));
    if (sent){
      Serial.print("  - MQTT broker: Message publised ["); Serial.print(topic); Serial.println( "]: ");Serial.println(payload);
    } else {
      Serial.println("  - MQTT: Não foi possivel enviar");
    }
    return sent;
  }

  bool MQTT::connectMqtt(const char *contextName, const char* mqtt_username, const char* mqtt_password, const char* mqtt_topic) {
  if (!m_Client->connected()) {
    OnDebug(Serial.printf("%s: Tentando nova conexão...", contextName);)
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    if (m_Client->connect(clientId.c_str(), mqtt_username, mqtt_password)){
      OnDebug(Serial.printf("%s: Reconectado", contextName);)
      subscribe(mqtt_topic);
      return true;
    }
    Serial.print("failed, rc=");
    Serial.print(m_Client->state());
    return false;
  }
  OnDebug(Serial.printf("%s: Conectado [ %s ]", contextName, mqtt_topic);)
  return true;
}
 

  bool MQTT::setupMqtt(const char *contextName, const char* mqtt_server, int mqtt_port, const char* mqtt_username, const char* mqtt_password, const char* mqtt_topic){
    OnDebug(Serial.printf("\n%s: Estabelecendo conexão inicial\n", contextName);)
    Serial.println(mqtt_server);
    Serial.println(mqtt_port);
    Serial.println(mqtt_username);
    Serial.println(mqtt_password);
    Serial.println(mqtt_topic);
    m_Client->setServer(mqtt_server, mqtt_port);
    return connectMqtt(contextName, mqtt_username, mqtt_password, mqtt_topic);
  }
  
  bool MQTT::loopMqtt(){
    return m_Client->loop();
  }

  void MQTT::setBufferSize(int size){
      m_Client->setBufferSize(size);
  }

  void MQTT::subscribe(const char* mqtt_topic){
  m_Client->subscribe(mqtt_topic);
  }

  void MQTT::setCallback(void (*callback)(char*, unsigned char*, unsigned int)){
    m_Client->setCallback(callback);
  }

  bool MQTT::beginPublish(const char* topic, unsigned int plength, bool retained){
    return m_Client->beginPublish(topic,plength,retained);
  }

  unsigned long long MQTT::write(const unsigned char* buffer, unsigned long long size){
    return m_Client->write(buffer,size);
  }
  
  int MQTT::endPublish(){
   return m_Client->endPublish();
  }
