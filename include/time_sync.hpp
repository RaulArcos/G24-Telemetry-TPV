#ifndef TIME_SYNC_HPP
#define TIME_SYNC_HPP

#include "common/common_libraries.hpp"
#include <NTPClient.h>
#include <WiFiUdp.h>

class TimeSync{
    public:
        TimeSync() = default;
        void begin();
        void trigger(byte* payload, unsigned int length);
        unsigned long get_synced_time();
    private:
        unsigned long _start_time;
        unsigned long _last_time;
        unsigned long _recieved_timestamp;

        unsigned long _synced_time;
        unsigned long _restart_time;
};

#endif