#pragma once

#include "system/types.h"

#include "kerror.h"

#include "debug/thread_annotations.hpp"

#define DEBUG
//#define RELEASE

namespace kdebug
{
	extern bool panicked;

	void kdebug_get_caller_pcs(size_t buflen, uintptr_t* pcs);

// use uint32_t for the bool value to make va_args happy.
	[[noreturn]] void kdebug_panic(const char* fmt, uint32_t topleft, ...);

	void kdebug_warning(const char* fmt, ...);

	void kdebug_log(const char* fmt, ...);

// panic with line number and file name
// to make __FILE__ and __LINE__ macros works right, this must be a macro as well.

#define KDEBUG_RICHPANIC(msg, title, topleft, add_fmt, args...) \
    kdebug::kdebug_panic("%s:\nIn file: %s, line: %d\nIn scope: %s\nMessage:\n%s\n" add_fmt, topleft,  title, __FILE__, __LINE__, __PRETTY_FUNCTION__, msg, ##args)

#define KDEBUG_RICHPANIC_CODE(code, topleft, add_fmt, args...) \
    kdebug::kdebug_panic("%s:\nIn file: %s, line: %d\nIn scope: %s\nMessage:\n%s\n" add_fmt, topleft, kdebug::error_title(code), __FILE__, __LINE__, __PRETTY_FUNCTION__, kdebug::error_message(code), ##args)

#define KDEBUG_GERNERALPANIC_CODE(code) \
    KDEBUG_GENERALPANIC(kdebug::error_message(code))

#define KDEBUG_GENERALPANIC(msg) \
    KDEBUG_RICHPANIC(msg, "KDEBUG_GENERALPANIC", true, "")

// pnaic that not move the cursor
#define KDEBUG_FOLLOWPANIC(msg) \
    KDEBUG_RICHPANIC(msg, "KDEBUG_GENERALPANIC", false, "")

// panic for not implemented functions
#define KDEBUG_NOT_IMPLEMENTED \
    KDEBUG_RICHPANIC("The function is not implemented", "KDEBUG_NOT_IMPLEMENTED", true, "");

	// panic the kernel if the condition isn't equal to 1
#define KDEBUG_ASSERT(cond)                                                 \
    do                                                                      \
    {                                                                       \
        if (!(cond))                                                        \
        {                                                                   \
            KDEBUG_RICHPANIC("Assertion failed",                            \
                             "ASSERT_PANIC",                                \
                             true,                                          \
                             "The expression \" %s \" is expected to be 1", \
                             #cond);                                        \
        }                                                                   \
    } while (0)

} // namespace kdebug
