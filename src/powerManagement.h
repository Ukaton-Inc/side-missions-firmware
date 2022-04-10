#pragma once
#ifndef _POWER_MANAGEMENT_
#define _POWER_MANAGEMENT_

namespace powerManagemnet
{
    extern bool enabled;
    extern unsigned long delayUntilSleep;
    void setup();
    void loop();
    void sleep();
} // namespace powerManagemnet

#endif // _POWER_MANAGEMENT_