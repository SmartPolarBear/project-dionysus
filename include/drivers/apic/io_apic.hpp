#pragma once

#include "drivers/acpi/cpu.h"
#include "system/types.h"

namespace apic
{

enum destination_mode
{
	DTM_PHYSICAL = 0,
	DTM_LOGICAL = 1,
};

enum trigger_mode
{
	TRG_EDGE = 0,
	TRG_LEVEL = 1,
};


}

namespace apic::io_apic
{

/*
Field	            Bits	    Description
Vector	            0 - 7	    The Interrupt vector that will be raised on the specified CPU(s).
Delivery Mode	    8 - 10	    How the interrupt will be sent to the CPU(s). It can be 000 (Fixed), 001 (Lowest Priority), 010 (SMI), 100 (NMI), 101 (INIT) and 111 (ExtINT). Most of the cases you want Fixed mode, or Lowest Priority if you don't want to suspend a high priority task on some important Processor/Core/Thread.
Destination Mode	11	        Specify how the Destination field shall be interpreted. 0: Physical Destination, 1: Logical Destination
Delivery Status	    12	        If 0, the IRQ is just relaxed and waiting for something to happen (or it has fired and already processed by Local APIC(s)). If 1, it means that the IRQ has been sent to the Local APICs but it's still waiting to be delivered.
Pin Polarity	    13	        0: Active high, 1: Active low. For ISA IRQs assume Active High unless otherwise specified in Interrupt Source Override descriptors of the MADT or in the MP Tables.
Remote IRR	        14
Trigger Mode	    15	        0: Edge, 1: Level. For ISA IRQs assume Edge unless otherwise specified in Interrupt Source Override descriptors of the MADT or in the MP Tables.
Mask	            16	        Just like in the old PIC, you can temporary disable this IRQ by setting this bit, and reenable it by clearing the bit.
Destination	        56 - 63	    This field is interpreted according to the Destination Format bit. If Physical destination is choosen, then this field is limited to bits 56 - 59 (only 16 CPUs addressable). You put here the APIC ID of the CPU that you want to receive the interrupt.
*/

union redirection_entry
{
	struct
	{
		uint64_t vector: 8;
		uint64_t delievery_mode: 3;
		uint64_t destination_mode: 1;
		uint64_t delivery_status: 1;
		uint64_t polarity: 1;
		uint64_t remote_irr: 1;
		uint64_t trigger_mode: 1;
		uint64_t mask: 1;
		uint64_t reserved: 39;
		uint64_t destination_id: 8;
	} __attribute__((packed));
	struct
	{
		uint32_t raw_low;
		uint32_t raw_high;
	} __attribute__((packed));
};



PANIC void init_ioapic(void);
void enable_trap(uint32_t trapnum, uint32_t cpu_rounted);

} // namespace io_apic
