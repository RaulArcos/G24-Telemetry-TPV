/**
 * @file mqtt_controller.cpp
 * @author
 * @brief This file contains the implementation of the MQTT Controller for AWS - G24 Telemetry.
 */

#include "../include/mqtt_controller.hpp"
#include <ArduinoJson.h>

const char* MQTTController::status_topic = "G24/tpv/status";
const char* MQTTController::mode_topic = "G24/tpv/set_mode";

MQTTController::MQTTController() {
    _client.setServer(_mqtt_server, _mqtt_port);
    _client.setCallback(MQTTController::callback);
}

void MQTTController::connect() {
    while (!_client.connected()) {
        Serial.print("Attempting MQTT connection...");
        String clientId = "ESP32-";
        clientId += String(random(0xffff), HEX);
        
        if (_client.connect(clientId.c_str())) {
            Serial.println("Connected");

            _client.subscribe(mode_topic);
        } else {
            Serial.print("Failed, rc=");
            Serial.print(_client.state());
            Serial.println(" Waiting 5 seconds");
            delay(5000);
        }
    }
}

void MQTTController::publish_telemetry(const char* topic, const char* message) {
    _client.publish(topic, message);
}

void MQTTController::publish_status(const char* status, const char* mode) {
    StaticJsonDocument<200> doc;
    doc["status"] = status;
    doc["mode"] = mode;

    char buffer[256];
    serializeJson(doc, buffer);
    _client.publish(status_topic, buffer);
}

void MQTTController::callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();

    if (strcmp(topic, mode_topic) == 0) {
        handle_mode_message(payload, length);
    }
}

void MQTTController::handle_mode_message(byte* payload, unsigned int length) {
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
    }

    int mode = doc["mode"];
    Serial.print("Mode received: ");
    Serial.println(mode);

    // Handle the mode change logic here
}

PubSubClient* MQTTController::get_client() {
    return &_client;
}