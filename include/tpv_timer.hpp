#ifndef TPVTIMER_HPP
#define TPVTIMER_HPP

#include "common/common_libraries.hpp"
#include "common/tpv_mode.hpp"
#include "common/tpv_status.hpp"

class TPVTimer {
public:
    TPVTimer();
private:
    TPVMode _mode = TPVMode::NOT_SELECTED;
    TPVStatus _status = TPVStatus::CONNECTED;

};

#endif