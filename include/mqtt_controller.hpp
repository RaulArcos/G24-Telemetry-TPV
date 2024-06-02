#ifndef MQTTCONTROLLER_HPP
#define MQTTCONTROLLER_HPP

#include "common/common_libraries.hpp"
#include <PubSubClient.h>
#include <WiFiClient.h>

class MQTTController {
public:
    MQTTController();
    void connect();
    static void callback(char* topic, byte* payload, unsigned int length);
    void publish_telemetry(const char* topic, const char* message);
    void publish_status(const char* status, const char* mode);
    PubSubClient* get_client();

private:
    const char* _mqtt_server = "broker.hivemq.com";
    const int _mqtt_port = 1883;
    WiFiClient _espClient;
    PubSubClient _client = PubSubClient(_espClient);

    static const char* status_topic;
    static const char* mode_topic;

    static void handle_mode_message(byte* payload, unsigned int length);
};

#endif