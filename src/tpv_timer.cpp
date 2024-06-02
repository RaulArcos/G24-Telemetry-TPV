/**
 * @file tpv_timer.cpp
 * @author Raúl Arcos Herrera
 * @brief This file contains the implementation of the TPV Timer for G24 Telemetry.
 */

#include "../include/tpv_timer.hpp"

void TPVTimer::set_mode(byte* payload, unsigned int length){
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    int mode = doc["mode"];
    _mode = static_cast<TPVMode>(mode);
    _status = TPVStatus::READY;
    _mqtt_controller->publish_status(_mqtt_controller->toString(_status), _mqtt_controller->toString(_mode));
}

void TPVTimer::start(byte* payload, unsigned int length){
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    bool start = doc["start"];
    if(start){
        auto it = _tpvHandlers.find(_mode);
        if (it != _tpvHandlers.end()) {
            it->second(); 
        }
    }else{
        _status = TPVStatus::STOP;
    }
    
}
void TPVTimer::set_mqtt_controller(MQTTController *mqtt_controller){
    _mqtt_controller = mqtt_controller;
}

void TPVTimer::set_mqtt_client(PubSubClient *mqtt_client){
    _mqtt_client = mqtt_client;
}

void TPVTimer::set_laser(Laser *laser){
    _laser = laser;
}

void TPVTimer::wait_for_laser(){
    while(!_laser->laser_is_activated()){
        _mqtt_client->loop();
        _mqtt_controller->publish_status(_mqtt_controller->toString(_status), _mqtt_controller->toString(_mode));
        if (_status == TPVStatus::STOP){
            break;
        }
        delay(5);
    }
    _laser->reset_laser();
}

void TPVTimer::acceleration(){
}

void TPVTimer::skidpad(){
}

void TPVTimer::autocross(){
}

void TPVTimer::endurance(){
    _status = TPVStatus::WAITING_CAR;
    _mqtt_controller->publish_status(_mqtt_controller->toString(_status), _mqtt_controller->toString(_mode));
    wait_for_laser();
    int lap = 0;
    while(_status != TPVStatus::STOP){
        _mqtt_client->loop();
        unsigned long start_time = millis();
        _status = TPVStatus::READING;
        _mqtt_controller->publish_status(_mqtt_controller->toString(_status), _mqtt_controller->toString(_mode));
        wait_for_laser();
        unsigned long end_time = millis();
        unsigned long lap_time = end_time - start_time;
        float seconds = lap_time / 1000.0;
        lap++;
        if(_status != TPVStatus::STOP){
            _mqtt_controller->publish_time(_mqtt_controller->toString(_mode), lap, String(seconds).c_str());
        }
    }
    _status = TPVStatus::READY;
}

