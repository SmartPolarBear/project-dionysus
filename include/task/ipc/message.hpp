#pragma once

#include "../../../build/external/third_party_root/include/c++/v1/compare"

namespace task::ipc
{

class endpoint;

// TODO: the following should be architecture-dependent
using message_register_type = uint64_t;
using buffer_register_type = uint64_t;

static inline constexpr size_t REGS_PER_MESSAGE = 64;

namespace _internals
{
template<typename T>
concept UntypedMessageItem=sizeof(T) <= sizeof(message_register_type) &&
	ktl::convertible_to<T, message_register_type> &&
	ktl::is_trivially_copy_assignable_v<T>;

template<typename T>
concept SingleMessageItem= (sizeof(T) == sizeof(message_register_type) ||
	sizeof(T) == sizeof(buffer_register_type)) &&
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

template<typename T>
concept DoubleMessageItem=(sizeof(T) == sizeof(message_register_type[2]) ||
	sizeof(T) == sizeof(buffer_register_type[2])) &&
	ktl::is_standard_layout_v<T>;

template<typename T>
concept Message=sizeof(T) == sizeof(message_register_type[REGS_PER_MESSAGE]) &&
	ktl::is_standard_layout_v<T>;

}

/// \brief A message tag describe the message items to be sent
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

/// \brief A message acceptor constraint how to receive the messages
class message_acceptor final
{
 public:
	static constexpr size_t ACCEPTOR_RECEIVER_LEN = sizeof(buffer_register_type) * 8 - 4;

	enum receive_window_values : uint64_t
	{
		NONE = 0,
		COMPLETE = UINT64_MAX
	};

	[[nodiscard]] message_acceptor() : raw_(0)
	{
	}

	~message_acceptor() = default;

	message_acceptor(message_acceptor&& another) : raw_(std::exchange(another.raw_, 0))
	{
	}

	message_acceptor(const message_acceptor& another) : raw_(another.raw_)
	{
	}

	message_acceptor& operator=(const message_acceptor& another)
	{
		if (this == &another)return *this;
		raw_ = another.raw_;
		return *this;
	}

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

enum class message_item_types : uint8_t
{
	STRING, MAP, GRANT
};

/// \brief string item is a block of memory less than 4MB
class string_item final
{
 public:
	static constexpr size_t STRING_ITEM_STR_LEN_BITS = sizeof(message_register_type) * 8 - 10;

	[[nodiscard]] message_item_types type() const
	{
		return (message_item_types)type_;
	}
 private:
	union
	{
		struct
		{
			uint64_t type_: 4;
			uint64_t res0_: 6;
			uint64_t string_len_: STRING_ITEM_STR_LEN_BITS;
			uint64_t string_ptr;
		}__attribute__((packed));
		uint64_t raws_[2];
	};
}__attribute__((packed));

static_assert(_internals::DoubleMessageItem<string_item>);

/// \brief map item share the page with threads
class map_item final
{
 public:
 private:
	[[maybe_unused]]uint64_t plchdl[2];
}__attribute__((packed));

static_assert(_internals::DoubleMessageItem<map_item>);

/// \brief grant item map the page with receiver and unmap for sender
class grant_item final
{
 public:
 private:
	[[maybe_unused]]uint64_t plchdl[2];
}__attribute__((packed));

static_assert(_internals::DoubleMessageItem<grant_item>);

class message final
{
 public:
	void put(_internals::UntypedMessageItem auto& item, size_t index)
	{

	}

	void put(_internals::SingleMessageItem auto& item, size_t index)
	{

	}

	void put(_internals::DoubleMessageItem auto& item, size_t index)
	{

	}

	void append(_internals::UntypedMessageItem auto& item)
	{

	}

	void append(_internals::SingleMessageItem auto& item)
	{

	}

	void append(_internals::DoubleMessageItem auto& item)
	{

	}

	template<_internals::UntypedMessageItem T>
	T at(size_t index)
	{

	}

	template<_internals::SingleMessageItem T>
	T at(size_t index)
	{

	}

	template<_internals::DoubleMessageItem T>
	T at(size_t index)
	{

	}

	void clear()
	{

	}

 private:
	union
	{
		uint64_t raws_[REGS_PER_MESSAGE];

		message_register_type regs_[REGS_PER_MESSAGE];

		message_tag tag_;
	}__attribute__((packed));
}__attribute__((packed));

static_assert(_internals::Message<message>);

}