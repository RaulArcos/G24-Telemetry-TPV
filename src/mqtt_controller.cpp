/**
 * @file mqtt_controller.cpp
 * @author Raúl Arcos Herrera
 * @brief This file contains the implementation of the MQTT Controller for AWS - G24 Telemetry.
 */

#include "../include/mqtt_controller.hpp"
#include <ArduinoJson.h>

MQTTController::MQTTController() {
    _client.setServer(_mqtt_server, _mqtt_port);
}

void MQTTController::set_callback(std::function<void(char*, byte*, unsigned int)> func){
    _client.setCallback(func);
}

void MQTTController::connect() {
    while (!_client.connected()) {
        Serial.print("Attempting MQTT connection...");
        String clientId = "ESP32-";
        clientId += String(random(0xffff), HEX);
        
        if (_client.connect(clientId.c_str())) {
            Serial.println("Connected");

            _client.subscribe(mode_topic);
            _client.subscribe(start_topic);
            _client.subscribe("G24/tpv/test");
        } else {
            Serial.print("Failed, rc=");
            Serial.print(_client.state());
            Serial.println(" Waiting 1 seconds");
            delay(1000);
        }
    }
}

void MQTTController::publish_status(const char* status, const char* mode) {
    StaticJsonDocument<200> doc;
    doc["status"] = status;
    doc["mode"] = mode;

    char buffer[256];
    serializeJson(doc, buffer);
    _client.publish(status_topic, buffer);
}

void MQTTController::publish_time(const char* mode, int lap, const char* time) {
    StaticJsonDocument<200> doc;
    doc["mode"] = mode;
    doc["lap"] = lap;
    doc["time"] = time;

    char buffer[256];
    serializeJson(doc, buffer);
    _client.publish(time_topic, buffer);
}

void MQTTController::publish_timestamp(unsigned long timestamp) {
    StaticJsonDocument<200> doc;
    doc["timestamp"] = timestamp;

    char buffer[256];
    serializeJson(doc, buffer);
    _client.publish("G24/tpv/test2", buffer);
}

PubSubClient* MQTTController::get_client() {
    return &_client;
}

const char* MQTTController::toString(TPVMode mode) {
    switch (mode) {
        case TPVMode::NOT_SELECTED:
            return "not-selected";
        case TPVMode::ACCELERATION:
            return "acceleration";
        case TPVMode::SKIDPAD:
            return "skidpad";
        case TPVMode::AUTOCROSS:
            return "autocross";
        case TPVMode::ENDURANCE:
            return "endurance";
        default:
            return "not-selected";
    }
}

const char* MQTTController::toString(TPVStatus status) {
    switch (status) {
        case TPVStatus::CONNECTED:
            return "connected";
        case TPVStatus::READY:
            return "ready";
        case TPVStatus::WAITING_CAR:
            return "waiting-car";
        case TPVStatus::READING:
            return "reading";
        default:
            return "connected";
    }
}