#pragma once
#include "kbl/lock/spinlock.h"

namespace task
{
extern lock::spinlock master_thread_lock;
}