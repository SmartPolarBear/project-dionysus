#pragma once

#include <compare>

namespace task::ipc
{

class endpoint;

// TODO: the following should be architecture-dependent
using message_register_type = uint64_t;
using buffer_register_type = uint64_t;

namespace _internals
{
template<typename T>
concept SingleMessageItem= (sizeof(T) == sizeof(uint64_t)) &&
	ktl::is_standard_layout_v<T> &&
	(
		requires(T t)
		{
			{ t.raw() }->ktl::convertible_to<message_register_type>;
		}
			||
				requires(T t)
				{
					{ t.raw() }->ktl::convertible_to<buffer_register_type>;
				}
	);

}

class message_tag final
{
 public:
	static inline constexpr size_t MESSAGE_TAG_LABEL_LEN = sizeof(message_register_type) * 8 - 16; // 64-16


	[[nodiscard]] message_tag() : raw_(0)
	{
	}

	~message_tag() = default;

	message_tag(message_tag&& another) : raw_(std::exchange(another.raw_, 0))
	{
	}

	message_tag(const message_tag& another) : raw_(another.raw_)
	{
	}

	message_tag& operator=(const message_tag& another)
	{
		if (this == &another)return *this;
		raw_ = another.raw_;
		return *this;
	}

	[[nodiscard]]explicit message_tag(message_register_type r) : raw_(r)
	{
	}

	[[nodiscard]] auto untyped_count() const
	{
		return u_;
	}

	[[nodiscard]] auto typed_count() const
	{
		return t_;
	}

	[[nodiscard]] auto label() const
	{
		return label_;
	}

	void set_label(size_t label)
	{
		label_ = label;
	}

	[[nodiscard]] auto flags() const
	{
		return flags_;
	}

	void set_flags(uint32_t flags)
	{
		flags_ = flags;
	}

	[[nodiscard]] bool check_flags(uint32_t flags) const
	{
		return flags_ & flags;
	}

	[[nodiscard]] message_register_type raw() const
	{
		return raw_;
	}

 private:

	union
	{
		struct
		{
			uint64_t u_: 6;
			uint64_t t_: 6;
			uint64_t flags_: 4;
			uint64_t label_: MESSAGE_TAG_LABEL_LEN;
		}__attribute__((packed));
		message_register_type raw_;
	}__attribute__((packed));

}__attribute__((packed));

static_assert(_internals::SingleMessageItem<message_tag>);

class message_acceptor final
{
 public:
	static constexpr size_t ACCEPTOR_RECEIVER_LEN = sizeof(buffer_register_type) * 8 - 4;

	enum receive_window_values : uint64_t
	{
		NONE = 0,
		COMPLETE = UINT64_MAX
	};

	[[nodiscard]] bool allow_string() const
	{
		return accept_strings_;
	}

	[[nodiscard]] bool allow_map_or_grant() const
	{
		return accept_map_grant_;
	}

	[[nodiscard]] auto receive_window() const
	{
		return receive_window_;
	}

	[[nodiscard]] buffer_register_type raw() const
	{
		return raw_;
	}

 private:
	union
	{
		struct
		{
			uint64_t accept_map_grant_: 1;
			uint64_t accept_strings_: 1;
			[[maybe_unused]] uint64_t res1_: 2;
			uint64_t receive_window_: ACCEPTOR_RECEIVER_LEN;
		};
		buffer_register_type raw_;
	};
}__attribute__((packed));

static_assert(_internals::SingleMessageItem<message_acceptor>);

class message final
{

}__attribute__((packed));

}