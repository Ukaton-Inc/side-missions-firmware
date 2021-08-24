#pragma once
#ifndef _HID_
#define _HID_

#include <Arduino.h>
#include <BleGamepad.h>

namespace hid
{
    extern BleGamepad bleGamepad;
    void setup();
    void loop();
} // namespace hid

#endif // _HID_