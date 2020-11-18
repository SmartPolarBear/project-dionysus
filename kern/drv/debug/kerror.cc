
#include "arch/amd64/x86.h"

#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"
#include "drivers/debug/kerror.h"

#include "system/error.hpp"
#include "system/memlayout.h"
#include "system/types.h"

#include <cstring>

#pragma clang diagnostic push

// we must use array designators here whatsoever. And clang supports this.
#pragma clang diagnostic ignored "-Wc99-designator"

const char* err_msg[] = {
	[ERROR_SUCCESS]=     "the action is completed successfully",
	[ERROR_UNKOWN]=      "failedbut reason can't be figured out",
	[ERROR_INVALID]=  "invalid data",
	[ERROR_NOT_IMPL]=    "not implemented",
	[ERROR_LOCK_STATUS]= "lock is not at a right status",
	[ERROR_UNSUPPORTED]= "unsupported features",

	[ERROR_MEMORY_ALLOC]=          "insufficient memory",
	[ERROR_REWRITE]=               "rewrite the data that shouldn't be done so",
	[ERROR_VMA_NOT_FOUND]=         "can't find a VMA",
	[ERROR_PAGE_NOT_PRESENT]=      "page isn't persent",
	[ERROR_HARDWARE_NOT_COMPATIBLE]="the hardware isn't compatible with the kernel",
	[ERROR_TOO_MANY_PROC]=         "too many processes",
	[ERROR_CANNOT_WAKEUP]=           "process's state isn't valid for waking up",
	[ERROR_HAS_KILLED]=              "process to be killed has been killed",
	[ERROR_BUSY]=              "device is busy",
	[ERROR_DEV_TIMEOUT]=             "device operation time out",
	[ERROR_IO]=              "IO error"
};

const char* err_title[] = {
	[ERROR_SUCCESS]="ERROR_SUCCESS",
	[ERROR_UNKOWN]="ERROR_UNKOWN",
	[ERROR_INVALID]="ERROR_INVALID",
	[ERROR_NOT_IMPL]="ERROR_NOT_IMPL",
	[ERROR_LOCK_STATUS]="ERROR_LOCK_STATUS",
	[ERROR_UNSUPPORTED]="ERROR_UNSUPPORTED",
	[ERROR_MEMORY_ALLOC]="ERROR_MEMORY_ALLOC",
	[ERROR_REWRITE]="ERROR_REWRITE",
	[ERROR_VMA_NOT_FOUND]="ERROR_VMA_NOT_FOUND",
	[ERROR_PAGE_NOT_PRESENT]="ERROR_PAGE_NOT_PRESENT",
	[ERROR_HARDWARE_NOT_COMPATIBLE]="ERROR_HARDWARE_NOT_COMPATIBLE",
	[ERROR_TOO_MANY_PROC]="ERROR_TOO_MANY_PROC",
	[ERROR_CANNOT_WAKEUP]="ERROR_CANNOT_WAKEUP",
	[ERROR_HAS_KILLED]="ERROR_HAS_KILLED",
	[ERROR_BUSY]="ERROR_BUSY",
	[ERROR_DEV_TIMEOUT]="ERROR_DEV_TIMEOUT",
	[ERROR_IO]="ERROR_IO",
};

const char* kdebug::error_message(error_code code)
{
	if (code < 0)code = -code;
	return err_msg[static_cast<size_t>(code)];
}

const char* kdebug::error_title(error_code code)
{
	if (code < 0)code = -code;
	return err_title[static_cast<size_t>(code)];
}

#pragma clang diagnostic pop
