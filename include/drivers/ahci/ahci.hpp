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

	struct ahci_port_pxtfd
	{
		uint64_t sts_err: 1;
		uint64_t sts_cs0: 2;
		uint64_t sts_drq: 1;
		uint64_t sts_cs1: 3;
		uint64_t sts_bsy: 1;
		uint64_t err: 8;
		uint64_t reserved: 16;
	}__attribute__((__packed__));
	static_assert(sizeof(ahci_port_pxssts) == sizeof(uint32_t));

	struct ahci_port_pxis
	{
		uint64_t dhrs: 1;
		uint64_t pss: 1;
		uint64_t dss: 1;
		uint64_t sdbs: 1;
		uint64_t ufs: 1;
		uint64_t dps: 1;
		uint64_t pcs: 1;
		uint64_t dmps: 1;
		uint64_t reserved0: 14;
		uint64_t prcs: 1;
		uint64_t imps: 1;
		uint64_t ofs: 1;
		uint64_t reserved1: 1;
		uint64_t infs: 1;
		uint64_t ifs: 1;
		uint64_t hbds: 1;
		uint64_t hbfs: 1;
		uint64_t tfes: 1;
		uint64_t cpds: 1;
	}__attribute__((__packed__));
	static_assert(sizeof(ahci_port_pxis) == sizeof(uint32_t));

	enum ahci_ata_cmd
	{
		ATA_CMD_IDENTIFY = 0xEC,
		ATA_CMD_PACKET = 0xA0,
		ATA_CMD_READ_DMA_EX = 0x25,
		ATA_CMD_WRITE_DMA_EX = 0x35,
		ATA_CMD_PACKET_IDENTIFY = 0xA1,
		ATAPI_CMD_READ_SECTORS = 0xA8,
	};

	enum ahci_ata_identity
	{
		ATA_IDENT_DEVICE_TYPE = 0x00,
		ATA_IDENT_SERIAL = 0x14,
		ATA_IDENT_MODEL = 0x36,
		ATA_IDENT_CAPS = 0x62,
		ATA_IDENT_MAX_LBA = 0x78,
		ATA_IDENT_CMD_SETS = 0xA4,
		ATA_IDENT_MAX_LBAEXT = 0xC8,
	};

	struct ahci_port
	{
		uint32_t clb;                    // 0x00
		uint32_t clbu;                    // 0x04
		uint32_t fb;                        // 0x08
		uint32_t fbu;                    // 0x0C
		ahci_port_pxis is;                        // 0x10
		uint32_t ie;                        // 0x14
		ahci_port_pxcmd cmd;                    // 0x18
		uint32_t reserved0;                    // 0x1C
		ahci_port_pxtfd tfd;                    // 0x20
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

	struct ahci_hba_mem
	{
		ahci_generic_host_control ghc;

		// 0x2C - 0x9F, Reserved
		uint8_t reserved[116];

		// 0xA0 - 0xFF, Vendor specific registers
		uint8_t vendor_specified[96];

		// 0x100 - 0x10FF, Port control registers
		ahci_port ports[0];    // 1 ~ 32
	}__attribute__((__packed__));

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
	struct ahci_command_list_entry
	{
		ahci_command_dw0 dw0;
		uint32_t prdbc;
		uint32_t ctba;
		uint32_t ctba_u0;
		uint32_t reserved[4];
	}__attribute__((__packed__));
	static_assert(sizeof(ahci_command_list_entry) == sizeof(uint32_t) * 8);

	constexpr size_t AHCI_FIS_TYPE_REG_H2D = 0x27;
	constexpr size_t AHCI_ATA_CMD_IDENTIFY = 0xEC;
	constexpr size_t AHCI_ATA_CMD_PACKET_IDENTIFY = 0xA1;


	struct ahci_fis_reg_h2d
	{
		// DWORD 0
		uint8_t fis_type;    // FIS_TYPE_REG_H2D

		uint8_t pmport: 4;    // Port multiplier
		uint8_t rsv0: 3;        // Reserved
		uint8_t c: 1;        // 1: Command, 0: Control

		uint8_t command;    // Command register
		uint8_t featurel;    // Feature register, 7:0

		// DWORD 1
		uint8_t lba0;        // LBA low register, 7:0
		uint8_t lba1;        // LBA mid register, 15:8
		uint8_t lba2;        // LBA high register, 23:16
		uint8_t device;        // Device register

		// DWORD 2
		uint8_t lba3;        // LBA register, 31:24
		uint8_t lba4;        // LBA register, 39:32
		uint8_t lba5;        // LBA register, 47:40
		uint8_t featureh;    // Feature register, 15:8

		// DWORD 3
		uint8_t countl;        // Count register, 7:0
		uint8_t counth;        // Count register, 15:8
		uint8_t icc;        // Isochronous command completion
		uint8_t control;    // Control register

		// DWORD 4
		uint8_t rsv1[4];    // Reserved
	}__attribute__((__packed__));

	struct ahci_fis_reg_d2h
	{
		// DWORD 0
		uint8_t fis_type;    // FIS_TYPE_REG_D2H

		uint8_t pmport: 4;    // Port multiplier
		uint8_t rsv0: 2;      // Reserved
		uint8_t i: 1;         // Interrupt bit
		uint8_t rsv1: 1;      // Reserved

		uint8_t status;      // Status register
		uint8_t error;       // Error register

		// DWORD 1
		uint8_t lba0;        // LBA low register, 7:0
		uint8_t lba1;        // LBA mid register, 15:8
		uint8_t lba2;        // LBA high register, 23:16
		uint8_t device;      // Device register

		// DWORD 2
		uint8_t lba3;        // LBA register, 31:24
		uint8_t lba4;        // LBA register, 39:32
		uint8_t lba5;        // LBA register, 47:40
		uint8_t rsv2;        // Reserved

		// DWORD 3
		uint8_t countl;      // Count register, 7:0
		uint8_t counth;      // Count register, 15:8
		uint8_t rsv3[2];     // Reserved

		// DWORD 4
		uint8_t rsv4[4];     // Reserved
	}__attribute__((__packed__));

	struct ahci_prd_dw3
	{
		uint64_t dbc: 22;
		uint64_t reserved0: 9;
		uint64_t interrupt_on_completion: 1;
	}__attribute__((__packed__));
	static_assert(sizeof(ahci_prd_dw3) == sizeof(uint32_t));

	struct ahci_prd
	{
		uint32_t dba;
		uint32_t dbau;
		uint32_t reserved0;
		ahci_prd_dw3 dw3;
	}__attribute__((__packed__));
	static_assert(sizeof(ahci_prd) == sizeof(uint32_t) * 4);

	constexpr size_t AHCI_PRD_MAX_SIZE = 8192;
	struct ahci_command_table
	{
		ahci_fis_reg_h2d fis_h2d;
		uint8_t acmd[16];
		uint8_t reserved0[48];
		// Any number of entries can follow
		ahci_prd prdt[0];
	}__attribute__((__packed__));
	static_assert(sizeof(ahci_command_list_entry) == sizeof(uint32_t) * 8);

	enum ahci_device_type
	{
		DEVICE_SATA,
		DEVICE_SATAPI
	};

	constexpr size_t AHCI_SPIN_WAIT_MAX = 0xBCAFFE;

	struct ahci_controller
	{
		pci_device* pci_dev;
		uint8_t* regs;

		ahci_device_type type;

		list_head list;
	};

	error_code ahci_init();
}