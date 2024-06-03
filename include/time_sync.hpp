#ifndef TIME_SYNC_HPP
#define TIME_SYNC_HPP

#include "common/common_libraries.hpp"
#include <NTPClient.h>
#include <WiFiUdp.h>

class TimeSync{
    public:
        TimeSync(): _timeClient(_ntpUDP, "pool.ntp.org") {}
        void begin();
        void sync_time();
        unsigned long get_synced_time();
        bool is_time_synced();

    private:
        unsigned long _start_time;
        WiFiUDP _ntpUDP;
        NTPClient _timeClient;
};

#endif