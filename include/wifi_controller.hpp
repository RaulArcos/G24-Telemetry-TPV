#ifndef WIFICONTROLLER_HPP
#define WIFICONTROLLER_HPP

#include "common/common_libraries.hpp"
#include <WiFi.h>

class WifiController {
public:
    WifiController(const char* ssid, const char* password): _ssid(ssid), _password(password) {}
    bool connect();
private:
    const char* _ssid;
    const char* _password;
    unsigned long _timeout = 5000;
};

#endif