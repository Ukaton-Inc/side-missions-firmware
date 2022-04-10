#pragma once
#ifndef _POWER_MANAGEMENT_
#define _POWER_MANAGEMENT_

namespace powerManagement
{
    extern bool enabled;
    extern unsigned long delayUntilSleep;
    void setup();
    void loop();
    void sleep();
} // namespace powerManagement

#endif // _POWER_MANAGEMENT_