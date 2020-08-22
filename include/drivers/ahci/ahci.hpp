#pragma once
#include "system/types.h"
#include "drivers/pci/pci_device.hpp"

namespace ahci
{
	constexpr size_t AHCI_PCI_CLASS = 0x1;
	constexpr size_t AHCI_PCI_SUBCLASS = 0x6;
	constexpr size_t AHCI_PCI_PROGIF = 0x1;

	struct ahci_abar
	{
		uint64_t res_type_indicator: 1;
		uint64_t type: 2;
		uint64_t prefetchable: 1;
		uint64_t reserved0: 9;
		uint64_t base_addr: 19;
	}__attribute__((__packed__));
	static_assert(sizeof(ahci_abar) == sizeof(uint32_t));

	struct ahci_version
	{
		uint64_t minor_ver: 16;
		uint64_t major_ver: 16;
	}__attribute__((__packed__));
	static_assert(sizeof(ahci_version) == sizeof(uint32_t));

	struct ahci_port_impl
	{
		uint32_t bits;
	}__attribute__((__packed__));
	static_assert(sizeof(ahci_port_impl) == sizeof(uint32_t));

	struct ahci_generic_host_control
	{
		uint32_t cap;
		uint32_t ghc;
		uint32_t is;
		ahci_port_impl pi;
		ahci_version vs;
		uint32_t ccc_ctl;
		uint32_t ccc_ports;
		uint32_t em_loc;
		uint32_t em_ctl;
		uint32_t cap2;
		uint32_t bohc;
	}__attribute__((__packed__));
	static_assert(sizeof(ahci_generic_host_control) == 44);

	enum pxssts_ipm
	{
		IPM_NOT_PRESENT = 0x0,
		IPM_ACTIVE = 0x1,
		IPM_PARTIAL_POWER_MANAGEMENT = 0x2,
		IPM_SLUBBER_POWER_MANAGEMENT = 0x6,
		IPM_DEVSLEEP_POWER_MANAGEMENT = 0x8
	};

	enum pxssts_spd
	{
		SPD_DEV_NOT_PRESENT_OR_NOT_ESTABLISHED = 0x0,
		SPD_G1_RATE = 0x1,
		SPD_G2_RATE = 0x2,
		SPD_G3_RATE = 0x3,
	};

	enum pxssts_det
	{
		DET_NO_DEV_OR_NO_COMM = 0x0,
		DET_DEV_BUT_NO_COMM = 0x1,
		DET_DEV_AND_COMM = 0x3,
		DET_PHY_OFFLINE =
		0x4 //Phy in offline mode as a result of the interface being disabled or running in a BIST loopback mode
	};

	struct ahci_port_pxcmd
	{
		uint64_t st: 1;
		uint64_t sud: 1;
		uint64_t pod: 1;
		uint64_t clo: 1;
		uint64_t fre: 1;
		uint64_t ccs: 5;
		uint64_t mpss: 1;
		uint64_t fr: 1;
		uint64_t cr: 1;
		uint64_t cps: 1;
		uint64_t pma: 1;
		uint64_t hpcp: 1;
		uint64_t mpsp: 1;
		uint64_t cpd: 1;
		uint64_t esp: 1;
		uint64_t fbscp: 1;
		uint64_t apste: 1;
		uint64_t atapi: 1;
		uint64_t dlae: 1;
		uint64_t alpe: 1;
		uint64_t asp: 1;
		uint64_t icc: 4;
	}__attribute__((__packed__));
	static_assert(sizeof(ahci_port_pxcmd) == sizeof(uint32_t));

	struct ahci_port_pxssts
	{
		uint64_t det: 4;
		uint64_t spd: 4;
		uint64_t ipm: 4;
		uint64_t reserved0: 20;
	}__attribute__((__packed__));
	static_assert(sizeof(ahci_port_pxssts) == sizeof(uint32_t));

	struct ahci_port_pxsig
	{
		uint64_t sec_count_reg: 8;
		uint64_t lba_low: 8;
		uint64_t lba_mid: 8;
		uint64_t lba_high: 8;
	}__attribute__((__packed__));
	static_assert(sizeof(ahci_port_pxssts) == sizeof(uint32_t));

	struct ahci_port
	{
		uint32_t clb;                    // 0x00
		uint32_t clbu;                    // 0x04
		uint32_t fb;                        // 0x08
		uint32_t fbu;                    // 0x0C
		uint32_t is;                        // 0x10
		uint32_t ie;                        // 0x14
		ahci_port_pxcmd cmd;                    // 0x18
		uint32_t reserved0;                    // 0x1C
		uint32_t tfd;                    // 0x20
		ahci_port_pxsig sig;                    // 0x24
		ahci_port_pxssts ssts;            // 0x28
		uint32_t sctl;                    // 0x2C
		uint32_t serr;                    // 0x30
		uint32_t sact;                    // 0x34
		uint32_t ci;                        // 0x38
		uint32_t reserved1[13];                // 0x3C
		uint32_t vs[4];
	}__attribute__((__packed__));
	static_assert(sizeof(ahci_port) == 0x7F + 1);

	struct ahci_command_dw0
	{
		uint64_t cfl: 5;
		uint64_t atapi: 1;
		uint64_t write: 1;
		uint64_t prefetchable: 1;
		uint64_t reset: 1;
		uint64_t bist: 1;
		uint64_t c: 1;
		uint64_t reserved0: 1;
		uint64_t pmp: 4;
		uint64_t prdtl: 16;
	}__attribute__((__packed__));
	static_assert(sizeof(ahci_command_dw0) == sizeof(uint32_t));

	constexpr size_t AHCI_COMMAND_LIST_MAX = 32;
	struct ahci_command_list
	{
		ahci_command_dw0 dw0;
		uint32_t prdbc;
		uint32_t ctba;
		uint32_t ctba_u0;
		uint32_t reserved[4];
	}__attribute__((__packed__));
	static_assert(sizeof(ahci_command_list) == sizeof(uint32_t) * 8);

	struct ahci_controller
	{
		pci_device* pci_dev;
		uint8_t* regs;

		list_head list;
	};

	error_code ahci_init();
}