#pragma once
#ifndef _OSC_
#define _OSC_

#include "wifiServer.h"
#include <AsyncUDP.h>

namespace osc
{
    void listen(uint16_t port = 9998);
    void loop();
    bool hasListener();
} // namespace osc

#endif // _OSC_