#include "drivers/ahci/ata_string.hpp"

using std::make_pair;

ahci::ATAStrWord::char_pair ahci::ATAStrWord::get_char_pair() const
{
	uint8_t first = this->data >> 8u;
	uint8_t second = this->data & 0x00FFu;
	return make_pair(first, second);
}

