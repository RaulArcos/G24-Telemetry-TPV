/**
 * @file laser.cpp
 * @author Raúl Arcos Herrera
 * @brief This file contains the implementation of the Laser for G24 Telemetry.
 */

#include "../include/laser.hpp"

int Laser::_pin;
unsigned long Laser::_lastLaserTime;
bool Laser::_laserActivated;

Laser::Laser(int pin){
    _pin = pin;
    _lastLaserTime = 0;
    _laserActivated = false;
    pinMode(_pin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(_pin), this->on_laser_activated, FALLING);
}

bool Laser::laser_is_activated(){
    return _laserActivated;
}

void Laser::on_laser_activated(){
    if ((millis() - _lastLaserTime) > 2500) {
        _laserActivated = true;
        _lastLaserTime = millis();
    }
}

void Laser::reset_laser(){
    _laserActivated = false;
}