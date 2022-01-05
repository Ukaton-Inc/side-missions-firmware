#pragma once
#ifndef _NAME_
#define _NAME_

#include <Arduino.h>

namespace name
{
    const uint8_t MAX__NAME_LENGTH = 30;
    void setup();

    const std::string *getName();
    void setName(const char* newName, size_t length);
    void setName(const char* newName);
} // namespace name

#endif // _NAME_