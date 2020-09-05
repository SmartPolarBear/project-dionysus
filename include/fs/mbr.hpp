#pragma once
#include "system/types.h"
#include <cstring>

namespace file_system
{
	[[maybe_unused]]constexpr uintptr_t MBR_BOOTLOADER_OFFSET = 0x000;
	[[maybe_unused]]constexpr uintptr_t MBR_UNIQUE_ID_OFFSET = 0x1B8;
	[[maybe_unused]]constexpr uintptr_t MBR_RESERVED_OFFSET = 0x1BC;
	[[maybe_unused]]constexpr uintptr_t MBR_FIRST_PARTITION_ENTRY_OFFSET = 0x1BE;
	[[maybe_unused]]constexpr uintptr_t MBR_SECOND_PARTITION_ENTRY_OFFSET = 0x1CE;
	[[maybe_unused]]constexpr uintptr_t MBR_THIRD_PARTITION_ENTRY_OFFSET = 0x1DE;
	[[maybe_unused]]constexpr uintptr_t MBR_FOURTH_PARTITION_ENTRY_OFFSET = 0x1EE;
	[[maybe_unused]]constexpr uintptr_t MBR_VALID_SIGNATURE_OFFSET = 0x1FE;

	struct chs_struct
	{
		uint32_t head: 8;
		uint32_t sector: 6;
		uint32_t cylinder: 10;
	}__attribute__((__packed__));

	struct mbr_partition_table_entry
	{
		uint8_t bootable;
		chs_struct start_chs;
		uint8_t sys_id;
		chs_struct end_chs;
		uint32_t start_lba;
		uint32_t sector_count;
	}__attribute__((__packed__));

}



