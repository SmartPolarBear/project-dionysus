#include "include/lapic.hpp"

#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic.h"
#include "drivers/apic/apic_resgiters.hpp"
#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "debug/kdebug.h"
#include "drivers/apic/timer.h"

#include "system/memlayout.h"
#include "system/mmu.h"

#include "arch/amd64/cpu/x86.h"

#include <gsl/util>

using namespace apic;
using namespace local_apic;
using namespace _internals;
