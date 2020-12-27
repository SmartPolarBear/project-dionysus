
#include "arch/amd64/cpu/x86.h"

#include "drivers/console/console.h"
#include "debug/kdebug.h"
#include "debug/kerror.h"

#include "system/error.hpp"
#include "system/memlayout.h"
#include "system/types.h"

#include <cstring>

#pragma clang diagnostic push

// we must use array designators here whatsoever. And clang supports this.
#pragma clang diagnostic ignored "-Wc99-designator"

const char* err_msg_arr[ERROR_CODE_COUNT] = {
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
	[ERROR_IO]=              "IO error",
	[ERROR_OBSOLETE]="out of date",           // Out of date
	[ERROR_OUT_OF_BOUND]="out of bound",
	[ERROR_INTERNAL]="internal error",
	[ERROR_IS_DIR]="operation can't be on a directory",
	[ERROR_NOT_DIR]="operation must be on a directory",
	[ERROR_DEV_NOT_FOUND]="device not found",
	[ERROR_NO_ENTRY]="no such entry",
	[ERROR_EOF]="end of file",
	[ERROR_INVALID_ACCESS]="invalid access",
	[ERROR_SHOULD_NOT_REACH_HERE]="should not reach here",
	[ERROR_TOO_MANY_CALLS]="too many calls",
	[ERROR_ACCESS]="access error",
	[ERROR_NOT_EXIST]="object not exist",
	[ERROR_ALREADY_EXIST]="object already exist"
};

const char* err_title_arr[ERROR_CODE_COUNT] = {
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
	[ERROR_OBSOLETE]="ERROR_OBSOLETE",           // Out of date
	[ERROR_OUT_OF_BOUND]="ERROR_OUT_OF_BOUND",
	[ERROR_INTERNAL]="ERROR_INTERNAL",
	[ERROR_IS_DIR]="ERROR_IS_DIR",
	[ERROR_NOT_DIR]="ERROR_NOT_DIR",
	[ERROR_DEV_NOT_FOUND]="ERROR_DEV_NOT_FOUND",
	[ERROR_NO_ENTRY]="ERROR_NO_ENTRY",
	[ERROR_EOF]="ERROR_EOF",
	[ERROR_INVALID_ACCESS]="ERROR_INVALID_ACCESS",
	[ERROR_SHOULD_NOT_REACH_HERE]="ERROR_SHOULD_NOT_REACH_HERE",
	[ERROR_TOO_MANY_CALLS]="ERROR_TOO_MANY_CALLS",
	[ERROR_ACCESS]="ERROR_ACCESS",
	[ERROR_NOT_EXIST]="ERROR_NOT_EXIST",
	[ERROR_ALREADY_EXIST]="ERROR_ALREADY_EXIST"
};

constexpr const char* NO_SUCH_ERROR_CODE_MSG = "no such error code.";
constexpr const char* NO_SUCH_ERROR_CODE_TITLE = "NO SUCH ERROR CODE";

const char* kdebug::error_message(error_code code)
{
	if (code < 0)code = -code;
	if (code >= ERROR_CODE_COUNT)return NO_SUCH_ERROR_CODE_MSG;
	return err_msg_arr[static_cast<size_t>(code)];
}

const char* kdebug::error_title(error_code code)
{
	if (code < 0)code = -code;
	if (code >= ERROR_CODE_COUNT)return NO_SUCH_ERROR_CODE_TITLE;
	return err_title_arr[static_cast<size_t>(code)];
}

#pragma clang diagnostic pop
