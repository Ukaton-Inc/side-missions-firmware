#pragma once
#ifndef _UDP_
#define _UDP_

#include "wifiServer.h"
#include <AsyncUDP.h>

namespace udp
{
    extern AsyncUDP udp;

    void listen(uint16_t port = 9999);
    void loop();
} // namespace udp

#endif // _UDP_