#pragma once
#ifndef _DEBUG_
#define _DEBUG_

namespace debug
{
    void setup();

    bool getEnabled();
    void setEnabled(bool isEnabled);
} // namespace debug

#endif // _DEBUG_