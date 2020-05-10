#pragma once

#include "sys/error.h"
#include "drivers/debug/kdebug.h"

namespace KernelLibrary
{
struct ConsoleCapabilities
{
    /* data */
};

// base class
class IConsole
{
public:
    virtual ~IConsole() {}

    template <typename... TArgs>
    error_code write(char *fmt, const TArgs &... args)
    {
        KDEBUG_NOT_IMPLEMENTED;
    }
};
} // namespace KernelLibrary
