/**
 * @file mqtt_controller.cpp
 * @author Raúl Arcos Herrera
 * @brief This file contains the implementation of the MQTT Controller for AWS - G24 Telemetry.
 */

#include "../include/mqtt_controller.hpp"
#include <ArduinoJson.h>


MQTTController::MQTTController(){
    _espClient.setCACert(_awsCertificates.root_ca);
    _espClient.setCertificate(_awsCertificates.cert);
    _espClient.setPrivateKey(_awsCertificates.private_key);

    _client.setServer(_mqtt_server, _mqtt_port);
    _client.setCallback(MQTTController::callback);
}

void MQTTController::connect(){
    while (!_client.connected()) {
        Serial.print("Attempting MQTT connection...");
        String clientId = "ESP32-";
        clientId += String(random(0xffff), HEX);
        
        if (_client.connect(clientId.c_str())) {
        Serial.println("conectada");
    
        _client.publish("ei_out", "Hola mis niños del elektrokics");
        _client.subscribe("ei_in");
        } else {
            Serial.print("failed, rc=");
            Serial.print(_client.state());
            Serial.println(" Esperando 5 segundos");
            delay(5000);
        }
    }
}

void MQTTController::publish_telemetry(const char* topic, const char* message){
    _client.publish(topic, message);
}

void MQTTController::callback(char* topic, byte* payload, unsigned int length){
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();
}