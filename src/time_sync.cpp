/**
 * @file time_sync.cpp
 * @author Raúl Arcos Herrera
 * @brief This file contains the implementation of the Time Sync for G24 Telemetry.
 */

#include "../include/time_sync.hpp"

void TimeSync::begin() {
    _timeClient.begin();
    _timeClient.setTimeOffset(0);
    _start_time = millis();
}

void TimeSync::sync_time() {
    _timeClient.forceUpdate();
}

bool TimeSync::is_time_synced() {
    return _timeClient.update();
}

unsigned long TimeSync::get_synced_time() {
    unsigned long currentMillis = millis();
    return (currentMillis - _start_time);
    
}