#ifndef TPVTIMER_HPP
#define TPVTIMER_HPP

#include "common/common_libraries.hpp"
#include "common/tpv_mode.hpp"
#include "common/tpv_status.hpp"
#include "mqtt_controller.hpp"
#include "include/time_sync.hpp"
#include "laser.hpp"
#include <time.h>

class TPVTimer {
public:
    void set_mode(byte* payload, unsigned int length);
    void start(byte* payload, unsigned int length);
    void set_mqtt_controller(MQTTController *mqtt_controller);
    void set_mqtt_client(PubSubClient *mqtt_client);
    void set_laser(Laser *laser);
    void set_time_sync(TimeSync *time_sync);
    // unsigned long get_sync_time();

    void wait_for_laser();

    void acceleration();
    void skidpad();
    void autocross();
    void endurance();

    TPVMode get_mode() { return _mode; }
    TPVStatus get_status() { return _status; }
    
private:
    TPVMode _mode = TPVMode::NOT_SELECTED;
    TPVStatus _status = TPVStatus::CONNECTED;
    MQTTController *_mqtt_controller = nullptr;
    Laser *_laser = nullptr;
    TimeSync *_time_sync = nullptr;
    PubSubClient *_mqtt_client = nullptr;

    std::map<TPVMode, std::function<void()>> _tpvHandlers = {
    {TPVMode::ACCELERATION, [this](){acceleration();}},
    {TPVMode::SKIDPAD, [this](){skidpad();}},
    {TPVMode::AUTOCROSS, [this](){autocross();}},
    {TPVMode::ENDURANCE, [this](){endurance();}}
    };
};

#endif