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

apic_svr_register& apic_svr_register::clear()
{
	data.raw = 0;
	return *this;
}
apic_svr_register& apic_svr_register::load()
{
	data = read_lapic<svr_reg>(SVR_ADDR);
	return *this;
}

void apic_svr_register::apply()
{
	write_lapic(SVR_ADDR, data);
}
apic_svr_register& apic_svr_register::vector(uint8_t vector)
{
	data.vector = vector;
	return *this;
}
apic_svr_register& apic_svr_register::enable(bool enable)
{
	data.apic_software_enable = enable;
	return *this;
}
apic_svr_register& apic_svr_register::broadcast_suppression(bool sup)
{
	data.eoi_broadcast_suppression = sup;
	return *this;
}

apic_lvt_timer_register& apic_lvt_timer_register::load()
{
	data = read_lapic<lvt_timer_reg>(LVT_TIMER_ADDR);
	return *this;
}
apic_lvt_timer_register& apic_lvt_timer_register::clear()
{
	memset(&data, 0, sizeof(data));
	return *this;
}
void apic_lvt_timer_register::apply()
{
	write_lapic(LVT_TIMER_ADDR, data);
}
apic_lvt_timer_register& apic_lvt_timer_register::vector(uint8_t vec)
{
	data.vector = vec;
	return *this;
}
apic_lvt_timer_register& apic_lvt_timer_register::delivery_status(delivery_statuses dls)
{
	data.delivery_status = dls;
	return *this;
}
apic_lvt_timer_register& apic_lvt_timer_register::masked(bool masked)
{
	data.mask = masked;
	return *this;
}
apic_lvt_timer_register apic_lvt_timer_register::mode(apic_timer_modes mode)
{
	data.timer_mode = mode;
	return *this;
}
