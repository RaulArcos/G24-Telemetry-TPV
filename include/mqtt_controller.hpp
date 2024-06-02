#ifndef MQTTCONTROLLER_HPP
#define MQTTCONTROLLER_HPP

#include "common/common_libraries.hpp"
#include "common/tpv_mode.hpp"
#include "common/tpv_status.hpp"
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <functional>

class MQTTController {
public:
    MQTTController();
    void connect();
    static void callback(char* topic, byte* payload, unsigned int length);
    void set_callback(std::function<void(char*, byte*, unsigned int)> func);
    void publish_status(const char* status, const char* mode);
    void publish_time(const char* mode, int lap, const char* time);
    PubSubClient* get_client();

    const char* toString(TPVMode mode);
    const char* toString(TPVStatus status);

    const char* status_topic = "G24/tpv/status";
    const char* mode_topic = "G24/tpv/set_mode";
    const char* start_topic = "G24/tpv/start";
    const char* time_topic = "G24/tpv/time";

private:
    const char* _mqtt_server = "broker.hivemq.com";
    const int _mqtt_port = 1883;
    WiFiClient _espClient;
    PubSubClient _client = PubSubClient(_espClient);
};

#endif