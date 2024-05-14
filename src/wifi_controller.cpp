/**
 * @file wifi_controller.cpp
 * @author Raúl Arcos Herrera
 * @brief This file contains the implementation of the WiFi Controller for G24 Telemetry.
 */

#include "../include/wifi_controller.hpp"

bool WifiController::connect(){
    WiFi.begin(_ssid, _password);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED || millis() - start > _timeout){
        delay(1000);
    }
    if(WiFi.status() == WL_CONNECTED){
        return true;
    }
    return false;
}