#include <Arduino.h>

struct Config {
    char station_uid[64];
    char station_name[64];
    char wifi_ssid[64];
    char wifi_password[64];
    char mqtt_server[64];
    char mqtt_username[64];
    char mqtt_password[64];
    char mqtt_topic[64];
    //---------------//
    char mqtt_hostV2_server[64];
    char mqtt_hostV2_username[64];
    char mqtt_hostV2_password[64];
    int mqtt_hostV2_port;
    //----------------/
    //char iotGatewayHost[64];
    int mqtt_port;
    int interval;
};

bool parseConfigString(const char* configString, Config& config) {
    const char* delimiter = "\t";
    char* configCopy = strdup(configString); // Make a copy of the input string
    if (configCopy == nullptr) return false; // Error: memory allocation failed

    char* token;
    char* saveptr;
    token = strtok_r(configCopy, delimiter, &saveptr);
    if (token == nullptr) {
        free(configCopy); // Free allocated memory before returning
        return false; // Error: empty string
    }

    while (token != nullptr) {
        char* colon = strchr(token, ':');
        if (colon != nullptr) {
            *colon = '\0'; // Null terminate to separate variable name and value
            const char* varName = token;
            const char* varValue = colon + 1; // Move past the colon
            // Now you have variable name and value, you can handle them accordingly
            // Here, we'll store the values in the Config struct based on the variable name

            if (strcmp(varName, "STATION_UID") == 0) {
                strcpy(config.station_uid, varValue);
            } else if (strcmp(varName, "STATION_NAME") == 0) {
                strcpy(config.station_name, varValue);
            } else if (strcmp(varName, "WIFI_SSID") == 0) {
                strcpy(config.wifi_ssid, varValue);
            } else if (strcmp(varName, "WIFI_PASSWORD") == 0) {
                strcpy(config.wifi_password, varValue);
            } else if (strcmp(varName, "MQTT_SERVER") == 0) {
                strcpy(config.mqtt_server, varValue);
            } else if (strcmp(varName, "MQTT_USERNAME") == 0) {
                strcpy(config.mqtt_username, varValue);
            } else if (strcmp(varName, "MQTT_PASSWORD") == 0) {
                strcpy(config.mqtt_password, varValue);
            } else if (strcmp(varName, "MQTT_TOPIC") == 0) {
                strcpy(config.mqtt_topic, varValue);
            } else if (strcmp(varName, "MQTT_PORT") == 0) {
                config.mqtt_port = atoi(varValue);
            } else if (strcmp(varName, "INTERVAL") == 0) {
                config.interval = atoi(varValue);
            } else if (strcmp(varName, "MQTT_HOSTV2_SERVER") == 0) {
                strcpy(config.mqtt_hostV2_server, varValue);
            } else if (strcmp(varName, "MQTT_HOSTV2_USERNAME") == 0) {
                strcpy(config.mqtt_hostV2_username, varValue);
            } else if (strcmp(varName, "MQTT_HOSTV2_PASSWORD") == 0) {
                strcpy(config.mqtt_hostV2_password, varValue);
            } else if (strcmp(varName, "MQTT_HOSTV2_PORT") == 0) {
                config.mqtt_hostV2_port = atoi(varValue);
            } // Add other variable assignments as needed
        } else {
            // Handle case where token doesn't contain a colon
        }
        token = strtok_r(NULL, delimiter, &saveptr);
    }

    free(configCopy); // Free allocated memory
    return true;
}

void setup() {
    Serial.begin(9600); // Initialize serial communication
    while (!Serial); // Wait for Serial Monitor to open
    delay(1000); // Delay for stability

    const char* configString = "STATION_UID:537\tSTATION_NAME:weather-2\tWIFI_SSID:Gabriel\tWIFI_PASSWORD:2014072276\tMQTT_SERVER:broker.gpicm-ufrj.tec.br\tMQTT_USERNAME:telemetria\tMQTT_PASSWORD:kancvx8thz9FCN5jyq\tMQTT_TOPIC:/prefeituras/macae/estacoes/weather-2\tMQTT_PORT:1883\tINTERVAL:60000\tMQTT_HOSTV2_SERVER:192.168.0.223\tMQTT_HOSTV2_USERNAME:\tMQTT_HOSTV2_PASSWORD:\tMQTT_HOSTV2_PORT:1883";
    Config config;
    if (parseConfigString(configString, config)) {
        // Print parsed values for verification
        Serial.println("Parsed Config:");
        Serial.print("station_uid: "); Serial.println(config.station_uid);
        Serial.print("station_name: "); Serial.println(config.station_name);
        // Print other values as needed
    } else {
        Serial.println("Error: Failed to parse config string.");
    }
}

void lo
