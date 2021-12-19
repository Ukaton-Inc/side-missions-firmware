#pragma once
#ifndef _MOVE_TO_WAKE_
#define _MOVE_TO_WAKE_

namespace moveToWake
{
    extern bool enabled;
    extern unsigned long delayUntilSleep;
    void setup();
    void loop();
    void sleep();
} // namespace moveToWake

#endif // _MOVE_TO_WAKE_