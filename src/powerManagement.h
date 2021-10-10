#pragma once
#ifndef _POWER_MANAGEMENT_
#define _POWER_MANAGEMENT_

namespace powerManagement
{
    constexpr unsigned long delayUntilSleep = 1000 * 60;
    void print_wakeup_reason();
    void setup();
    void loop();
    void sleep();
} // namespace powerManagement

#endif // _POWER_MANAGEMENT_