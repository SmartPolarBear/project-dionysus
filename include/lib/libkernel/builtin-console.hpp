#pragma once

#include "lib/libkernel/console.hpp"

namespace KernelLibrary
{
class BuiltinConsole : IConsole
{
public:
    template <typename... TArgs>
    error_code write(char *fmt, const TArgs &... args);
};
} // namespace KernelLibrary
