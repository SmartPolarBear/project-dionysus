#include "arch/amd64/cpu/x86.h"

#include "drivers/acpi/acpi.h"
#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "debug/kdebug.h"

#include "system/memlayout.h"
#include "system/mmu.h"

using trap::TRAP_IRQ0;

enum controllers
{
    IO_PIC1 = 0x20,
    IO_PIC2 = 0xA0
};

constexpr size_t IRQ_SLAVE = 2;

static uint16_t irqmask = 0xFFFF & ~(1 << IRQ_SLAVE);

static inline void pic_setmask(uint16_t mask)
{
    irqmask = mask;
    outb(IO_PIC1 + 1, mask);
    outb(IO_PIC2 + 1, mask >> 8);
}

// for we never use 8259A pic, this is deprecated
[[deprecated("for we never use 8259A pic, this is deprecated")]][[maybe_unused]] void pic_enable(int irq)
{
    pic_setmask(irqmask & ~(1 << irq));
}

void pic8259A::init_pic(void)
{
    // mask all interrupts
    outb(IO_PIC1 + 1, 0xFF);
    outb(IO_PIC2 + 1, 0xFF);

    // Set up master (8259A-1)

    // ICW1:  0001g0hi
    //    g:  0 = edge triggering, 1 = level triggering
    //    h:  0 = cascaded PICs, 1 = master only
    //    i:  0 = no ICW4, 1 = ICW4 required
    outb(IO_PIC1, 0x11);

    // ICW2:  Vector offset
    outb(IO_PIC1 + 1, TRAP_IRQ0);

    // ICW3:  (master PIC) bit mask of IR lines connected to slaves
    //        (slave PIC) 3-bit # of slave's connection to master
    outb(IO_PIC1 + 1, 1 << IRQ_SLAVE);

    // ICW4:  000nbmap
    //    n:  1 = special fully nested mode
    //    b:  1 = buffered mode
    //    m:  0 = slave PIC, 1 = master PIC
    //      (ignored when b is 0, as the master/slave role
    //      can be hardwired).
    //    a:  1 = Automatic EOI mode
    //    p:  0 = MCS-80/85 mode, 1 = intel x86 mode
    outb(IO_PIC1 + 1, 0x3);

    // Set up slave (8259A-2)
    outb(IO_PIC2, 0x11);              // ICW1
    outb(IO_PIC2 + 1, TRAP_IRQ0 + 8); // ICW2
    outb(IO_PIC2 + 1, IRQ_SLAVE);     // ICW3
    // NB Automatic EOI mode doesn't tend to work on the slave.
    // Linux source code says it's "to be investigated".
    outb(IO_PIC2 + 1, 0x3); // ICW4

    // OCW3:  0ef01prs
    //   ef:  0x = NOP, 10 = clear specific mask, 11 = set specific mask
    //    p:  0 = no polling, 1 = polling mode
    //   rs:  0x = NOP, 10 = read IRR, 11 = read ISR
    outb(IO_PIC1, 0x68); // clear specific mask
    outb(IO_PIC1, 0x0a); // read IRR by default

    outb(IO_PIC2, 0x68); // OCW3
    outb(IO_PIC2, 0x0a); // OCW3

    if (irqmask != 0xFFFF)
    {
        pic_setmask(irqmask);
    }
}
