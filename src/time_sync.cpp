/**
 * @file time_sync.cpp
 * @author Raúl Arcos Herrera
 * @brief This file contains the implementation of the Time Sync for G24 Telemetry.
 */

#include "../include/time_sync.hpp"

void TimeSync::begin() {
    _start_time = millis();
}

void TimeSync::trigger(byte* payload, unsigned int length) {
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    
    unsigned long receivedTime = strtoull(message, NULL, 10);

    Serial.println("Received Time: " + String(receivedTime));
    long external_diff = receivedTime - _recieved_timestamp;
    long internal_diff = millis() - _last_time;

    Serial.println("External Diff: " + String(external_diff));
    Serial.println("Internal Diff: " + String(internal_diff));

    if (abs(external_diff - internal_diff) < 5) {
        Serial.println("Time Synced");
        _synced_time = receivedTime;
        _restart_time = millis();
    }
    _last_time = millis();
    _recieved_timestamp = receivedTime;
}

unsigned long TimeSync::get_synced_time() {
    unsigned long currentMillis = millis();
    return _synced_time + (currentMillis - _restart_time);
    
}