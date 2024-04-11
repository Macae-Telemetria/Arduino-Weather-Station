#pragma once
class String;
namespace TLM
{
  int setupWifi(const char *contextName, char* ssid, char*password);
  bool sendMeasurementToMqtt(char *topic, const char *payload);
  bool connectMqtt(const char *contextName, char* mqtt_username, char* mqtt_password, char* mqtt_topic);
  bool setupMqtt(const char *contextName, char* mqtt_server, int mqtt_port, char* mqtt_username, char* mqtt_password, char* mqtt_topic);
  int connectNtp(const char *contextName);
  bool loopMqtt();
  bool isWifiConnected();
  int getWifiSginal();
}