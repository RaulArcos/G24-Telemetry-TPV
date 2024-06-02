#ifndef LASER_HPP
#define LASER_HPP

#include "common/common_libraries.hpp"

class Laser{
public:
    Laser(int pin);
    static void on_laser_activated();
    static bool laser_is_activated();
    static void reset_laser();
private:
    static int _pin;
    static unsigned long _lastLaserTime;
    static bool _laserActivated;
};

#endif