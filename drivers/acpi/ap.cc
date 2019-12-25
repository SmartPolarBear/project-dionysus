#include "arch/amd64/x86.h"

#include "drivers/acpi/acpi.h"
#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/apic_timer/timer.h"
#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"

#include "sys/bootmm.h"
#include "sys/memlayout.h"
#include "sys/types.h"
#include "sys/vm.h"

#include "lib/libc/string.h"

// generated by the linker for ap_boot binary
extern uint8_t _binary___build_ap_boot_start[];
extern uint8_t _binary___build_ap_boot_end[];
extern uint8_t _binary___build_ap_boot_size[];

// in boot.S
extern "C" void entry32mp(void);

constexpr uintptr_t AP_CODE_LOAD_ADDR = 0x7000;

[[clang::optnone]] void ap::init_ap(void) {
    uint8_t *code = reinterpret_cast<decltype(code)>(P2V(AP_CODE_LOAD_ADDR));
    memmove(code, _binary___build_ap_boot_start, (size_t)_binary___build_ap_boot_size);

    auto current_cpuid = local_apic::get_cpunum();

    for (const auto &core : cpus)
    {
        if (core.present && core.id != current_cpuid)
        {
            char *stack = vm::bootmm_alloc();
            if (stack == nullptr)
            {
                KDEBUG_GENERALPANIC("Can't allocate enough memory for AP boot.\n");
            }

            *(uint32_t *)(code - 4) = 0x8000; // just enough stack to get us to entry64mp
            *(uint32_t *)(code - 8) = V2P(uintptr_t(entry32mp));
            *(uint64_t *)(code - 16) = (uint64_t)(stack + PAGE_SIZE);

            local_apic::start_ap(core.apicid, V2P((uintptr_t)code));

            while (core.started == 0u)
                if (core.started == 0u)
                    ;
            ;
        }
    }
}

extern "C" [[clang::optnone]] void ap_enter(void) {
    // install the kernel vm
    vm::switch_kernelvm();
    // install trap vectors
    trap::initialize_trap_vectors();
    // initialize segmentation
    vm::segment::init_segmentation();
    // initialize local APIC
    local_apic::init_lapic();
    // initialize apic timer
    timer::init_apic_timer();

    xchg(&cpu->started, 1u);
    // console::printf("Init CPU apicid=%d, id=%d, started=%d\n", cpu->apicid, cpu->id, cpu->started);
}
