#include "include/pmm.h"
#include "include/buddy_pmm.h"

#include "memory/pmm.hpp"

#include "arch/amd64/cpu/x86.h"

#include "system/error.hpp"
#include "system/kmem.hpp"
#include "system/memlayout.h"
#include "system/mmu.h"
#include "system/multiboot.h"
#include "system/pmm.h"
#include "system/vmm.h"
#include "system/segmentation.hpp"

#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "debug/kdebug.h"

#include "../../libs/basic_io/include/builtin_text_io.hpp"

#include "kbl/lock/lock_guard.hpp"

#include <algorithm>
#include <utility>

#include <gsl/util>

memory::physical_memory_manager::physical_memory_manager()
	: provider_()
{

}
