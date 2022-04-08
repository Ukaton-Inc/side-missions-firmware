#pragma once
#ifndef _TYPE_
#define _TYPE_

#include <Arduino.h>

namespace type
{
    enum class Type : uint8_t
    {
        MOTION_MODULE,
        LEFT_INSOLE,
        RIGHT_INSOLE,
        COUNT
    };
    
    void setup();
    Type getType();
    bool isInsole();
    bool isRightInsole();
    bool isTypeValid(Type newType);
    void setType(Type newType);
} // namespace type

#endif // _TYPE_