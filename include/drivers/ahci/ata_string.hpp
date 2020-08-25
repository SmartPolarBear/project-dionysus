#pragma once
#include "system/types.h"

#include  <utility>
namespace ahci
{
	class ATAString
	{

	 private:
		using char_pair = std::pair<uint8_t, uint8_t>;
		word_type data;
	 public:
		ATAString() : data(0)
		{
		}

		explicit ATAString(word_type _data) : data(_data)
		{
		}

		[[nodiscard]] char_pair get_char_pair() const;
	};
}