#pragma once

void mqttSubCallback(char* topic, byte* payload, unsigned int length);

class MQTT
{
public:
MQTT();
~MQTT();
bool publish(const char *topic, const char *payload);
bool connectMqtt(const char *contextName, const char* mqtt_username, const char* mqtt_password, const char* mqtt_topic);
bool setupMqtt(const char *contextName, const char* mqtt_server, int mqtt_port, const char* mqtt_username, const char* mqtt_password, const char* mqtt_topic);
bool loopMqtt();
void setCallback(void (*callback)(char*, unsigned char*, unsigned int));

private:
class WiFiClient* m_WifiClient;
class PubSubClient* m_Client;
};