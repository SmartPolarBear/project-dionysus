#include "drivers/ahci/ata_string.hpp"

using std::make_pair;

ahci::ATAString::char_pair ahci::ATAString::get_char_pair() const
{
	uint8_t first = this->data >> 8u;
	uint8_t second = this->data & 0x00FFu;
	return make_pair(first, second);
}

