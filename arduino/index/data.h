#pragma once
struct Config {
  char station_uid[64];
  char station_name[64];
  char wifi_ssid[64];
  char wifi_password[64];
  char mqtt_server[64];
  char mqtt_username[64];
  char mqtt_password[64];
  char mqtt_topic[64];
  int mqtt_port;
  int interval;
  int backup_time;
};

struct HealthCheck {
  const char *softwareVersion;
  int timestamp;
  bool isWifiConnected;
  bool isMqttConnected;
  int wifiDbmLevel;
  int timeRemaining;
};

struct Metrics {
  float wind_speed = 0;
  float wind_gust = 0;
  float rain_acc = 0;
  float humidity = 0;
  float temperature = 0;
  float pressure = 0;
  int wind_dir = -1;
  long timestamp;
}; 

extern Metrics Data;

inline Config config;
inline const char *configFileName = "/config.txt";

extern char hcJsonOutput[240];
extern char hcCsvOutput[240];

extern char metricsjsonOutput[240];
extern char metricsCsvOutput[240];
const char *parseHealthCheckData(HealthCheck hc, int type = 1);
void parseData();