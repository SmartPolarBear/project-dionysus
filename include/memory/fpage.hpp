#pragma once

namespace task::ipc
{

enum access_rights
{
	AR_X = 0b0001,
	AR_W = 0b0010,
	AR_R = 0b0100,
};

enum access_rights_shorthand
{
	AR_RX = AR_X | AR_X,
	AR_RW = AR_X | AR_W,
	AR_FULL = AR_X | AR_W | AR_X
};

class fpage
{
 public:
	constexpr fpage() = default;
	constexpr fpage(uint64_t raw) : raw_{ raw }
	{
	}

	constexpr fpage(uint64_t b, uint64_t s, uint64_t rights)
		: rights_{ rights }, s_{ s }, b_div_1024_{ b >> 10 }
	{
	}

	[[nodiscard]] bool check_rights(uint64_t rights) const
	{
		return (rights & rights_) == rights;
	}

	[[nodiscard]] uint64_t get_size() const
	{
		return 1ull << s_;
	}

	[[nodiscard]] uintptr_t get_base_address() const
	{
		return 1ull << b_div_1024_;
	}

 private:
	union
	{
		struct
		{
			uint64_t rights_: 4;
			uint64_t s_: 6;
			uint64_t b_div_1024_: 54;
		}__attribute__((packed));

		mutable uint64_t raw_{ 0 };
	};
}__attribute__((packed));

static inline constexpr fpage NULL_PAGE{ 0 };
static inline constexpr fpage COMPLETE{ 0, 1, AR_FULL };

}