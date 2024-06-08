#include "include/mqtt_controller.hpp"
#include "include/wifi_controller.hpp"
#include "include/tpv_timer.hpp"
#include "include/laser.hpp"
#include "include/time_sync.hpp"
#include <time.h>
#include <HTTPClient.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

WifiController wifiController("TP-Link_FE1C", "41432457");
MQTTController mqttController;
TPVTimer tpvTimer;
TimeSync timeSync;
Laser laser(19);

PubSubClient* mqttClient;

void mqtt_callback(char* topic, byte* payload, unsigned int length){
    // Serial.print("Message arrived [");
    // Serial.print(topic);
    // Serial.print("] ");
    // for (int i = 0; i < length; i++) {
    //     Serial.print((char)payload[i]);
    // }
    // Serial.println();

    if (strcmp(topic, mqttController.mode_topic) == 0) {
        tpvTimer.set_mode(payload, length);
    }else if(strcmp(topic, mqttController.start_topic) == 0){
        tpvTimer.start(payload, length);
    }else if(strcmp(topic, "G24/tpv/test") == 0){
        timeSync.trigger(payload, length);
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("G24::WifiController - Attempting Wifi Conection...");
    
    laser.begin();

    //Connect to WiFi
    if(wifiController.connect()){
        Serial.println("G24::WifiController - Connected to WiFi!");
    }else{
        Serial.println("G24::WifiController - Failed to connect to WiFi! Timeout!");
    }

    //Connect to MQTT
    mqttController.set_callback(mqtt_callback);
    mqttController.connect();
    Serial.println("G24::MQTTController - Connected to MQTT!");
    mqttClient = mqttController.get_client();
    tpvTimer.set_mqtt_controller(&mqttController);
    tpvTimer.set_laser(&laser);
    tpvTimer.set_mqtt_client(mqttClient);
    tpvTimer.set_time_sync(&timeSync);
}

void loop() {
    if (!mqttClient->connected()) {
        mqttController.connect();
    }
    mqttClient->loop();
    mqttController.publish_status(mqttController.toString(tpvTimer.get_status()), mqttController.toString(tpvTimer.get_mode()));
    
    size_t freeHeap = esp_get_free_heap_size();
    Serial.print("Free Heap Memory: ");
    Serial.print(freeHeap);
    Serial.println(" bytes");
    
    delay(100);
}