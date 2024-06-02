#include "include/mqtt_controller.hpp"
#include "include/wifi_controller.hpp"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

WifiController wifiController("CIELO_S", "NAAEE5022041515");
MQTTController mqttController;
PubSubClient* mqttClient;

void setup() {
    Serial.begin(115200);
    Serial.println("G24::WifiController - Attempting Wifi Conection...");
    
    //Connect to WiFi
    if(wifiController.connect()){
        Serial.println("G24::WifiController - Connected to WiFi!");
    }else{
        Serial.println("G24::WifiController - Failed to connect to WiFi! Timeout!");
    }

    //Connect to MQTT
    mqttController.connect();
    Serial.println("G24::MQTTController - Connected to MQTT!");
    mqttClient = mqttController.get_client();
}

void loop() {
    if (!mqttClient->connected()) {
        mqttController.connect();
    }
    mqttClient->loop();
    mqttController.publish_status("connected", "acceleration");

    delay(100);
}