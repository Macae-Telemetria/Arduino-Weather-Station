#pragma once
#include<string>

class BLE {
  public:
  static void Init(const char* boardName,  const std::string& currentConfig);
  static void SetConfigCallback(int(*jsCallback)(const std::string& json));
};


