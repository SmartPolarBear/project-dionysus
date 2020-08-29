#pragma once

#include "system/types.h"
#include "drivers/pci/pci_device.hpp"

namespace ahci
{
	constexpr size_t ATA_DEFAULT_SECTOR_SIZE = 512;

	constexpr size_t ATA_IDENT_SERIAL_OFFSET = 10;
	constexpr size_t ATA_IDENT_SERIAL_LEN_WORD = 10;

	constexpr size_t ATA_IDENT_MODEL_OFFSET = 27;
	constexpr size_t ATA_IDENT_MODEL_LEN_WORD = 20;

	constexpr size_t ATA_IDENT_CMD_SUP_OFFSET = 82;
	constexpr size_t ATA_IDENT_CMD_SUP_LEN_WORD = 3;

	constexpr size_t ATA_IDENT_LBA48_TOTAL_SECTORS_OFFSET = 100;
	constexpr size_t ATA_IDENT_LBA48_TOTAL_SECTORS_LEN_WORD = 4;

	constexpr size_t ATA_IDENT_LBA28_TOTAL_SECTORS_OFFSET = 60;
	constexpr size_t ATA_IDENT_LBA28_TOTAL_SECTORS_LEN_WORD = 2;

	constexpr size_t ATA_IDENT_LOGICAL_SECTOR_SIZE_OFFSET = 117;
	constexpr size_t ATA_IDENT_LOGICAL_SECTOR_SIZE_LEN_WORD = 2;

	constexpr size_t ATA_IDENT_PSECTOR_LSECTOR_SIZE_OFFSET = 106;
	constexpr size_t ATA_IDENT_PSECTOR_LSECTOR_SIZE_LEN_WORD = 1;

	struct ata_psector_lsector_size_word
	{
		uint64_t lsec_per_psec_power_of_2: 4;
		uint64_t reserved0: 8;
		uint64_t lsec_longer_than_256: 1;
		uint64_t multiple_lsec_per_psec: 1;
		uint64_t always1: 1;
		uint64_t always0: 1;
	}__attribute__((__packed__));
	static_assert(sizeof(ata_psector_lsector_size_word) == sizeof(uint16_t));

	struct ata_ident_cmd_fea_supported
	{
		// Word offset 82
		uint64_t smart: 1;
		uint64_t security: 1;
		uint64_t obsoleted0: 1;
		uint64_t pm: 1; // always 1
		uint64_t packed: 1; // always 0 for ata
		uint64_t volatile_wcache: 1;
		uint64_t read_look_ahead: 1;
		uint64_t obsoleted1: 2;
		uint64_t dev_reset: 1; // always 0
		uint64_t obsoleted2: 2;
		uint64_t wbuffer_sup: 1;
		uint64_t rbuffer_sup: 1;
		uint64_t nop_sup: 1;
		uint64_t obsoleted3: 1;

		// Word offset 83
		uint64_t download_microcode_sup: 1;
		uint64_t obsoleted4: 1;
		uint64_t reserved_for_cfa: 1;
		uint64_t apm: 1;
		uint64_t obsolete5: 1;
		uint64_t puis: 1;
		uint64_t powerup_set_features_required: 1;
		uint64_t obsoleted6: 1;
		uint64_t obsoleted7: 2;
		uint64_t lba48: 1;
		uint64_t obsoleted8: 1;
		uint64_t flush_cache: 1; //always 1
		uint64_t flush_cache_ext_sup: 1;
		uint64_t always1: 1;
		uint64_t always0: 1;

		// Word offset 84
		uint64_t smart_err_log: 1;
		uint64_t smart_self_test: 1;
		uint64_t reserved0: 1;
		uint64_t obsoleted9: 1;
		uint64_t streaming: 1;
		uint64_t gpl: 1;
		uint64_t write_dma_fua_and_write_multiple_fua: 1;
		uint64_t obsoleted10: 1;
		uint64_t ww_name: 1; // always 1
		uint64_t obsoleted11: 2;
		uint64_t obsoleted12: 2;
		uint64_t idle_immediate_with_unload: 1;
		uint64_t always1_1: 1;
		uint64_t always0_1: 1;

	}__attribute__((__packed__));
	static_assert(sizeof(ata_ident_cmd_fea_supported) == sizeof(uint16_t) * 3);

	error_code ata_identify_device(ahci_port *port);
	error_code atapi_identify_device(ahci_port *port);
}