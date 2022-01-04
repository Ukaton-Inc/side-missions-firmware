#pragma once
#ifndef _WEBSOCKET_
#define _WEBSOCKET_

#include "wifiServer.h"

namespace webSocket
{
    extern AsyncWebSocket server;
    bool isConnectedToClient();

    void setup();
    void loop();
} // namespace webSocket

#endif // _WEBSOCKET_