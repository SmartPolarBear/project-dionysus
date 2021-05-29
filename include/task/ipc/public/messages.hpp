#pragma once

#include <compare>
#include <cstring>

#include "memory/fpage.hpp"

#if defined(_DIONYSUS_KERNEL_)
#include "ktl/concepts.hpp"
#include "ktl/span.hpp"
#elif defined(_DIONYSUS_USER_)
#include <concepts>
#include <span>
#endif

namespace task::ipc
{
class endpoint;

// TODO: the following should be architecture-dependent
using message_register_type = uint64_t;
using buffer_register_type = uint64_t;

static inline constexpr size_t REGS_PER_MESSAGE = 64;

#if defined(_DIONYSUS_KERNEL_)
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
	ktl::is_standard_layout_v<T> &&
	(
		requires(T t)
		{
			{ t.raw() }->ktl::convertible_to<ktl::span<message_register_type>>;
		}
			||
				requires(T t)
				{
					{ t.raw() }->ktl::convertible_to<ktl::span<buffer_register_type>>;
				}
	);

template<typename T>
concept Message=sizeof(T) == sizeof(message_register_type[REGS_PER_MESSAGE]) &&
	ktl::is_standard_layout_v<T>;

}

using message_span = ktl::span<message_register_type>;

#elif defined(_DIONYSUS_USER_)
namespace _internals
{
template<typename T>
concept UntypedMessageItem=sizeof(T) <= sizeof(message_register_type) &&
	std::convertible_to<T, message_register_type> &&
	std::is_trivially_copy_assignable_v<T>;

template<typename T>
concept SingleMessageItem= (sizeof(T) == sizeof(message_register_type) ||
	sizeof(T) == sizeof(buffer_register_type)) &&
	std::is_standard_layout_v<T> &&
	(
		requires(T t)
		{
			{ t.raw() }->std::convertible_to<message_register_type>;
		}
			||
				requires(T t)
				{
					{ t.raw() }->std::convertible_to<buffer_register_type>;
				}
	);

template<typename T>
concept DoubleMessageItem=(sizeof(T) == sizeof(message_register_type[2]) ||
	sizeof(T) == sizeof(buffer_register_type[2])) &&
	std::is_standard_layout_v<T> &&
	(
		requires(T t)
		{
			{ t.raw() }->std::convertible_to<std::span<message_register_type>>;
		}
			||
				requires(T t)
				{
					{ t.raw() }->std::convertible_to<std::span<buffer_register_type>>;
				}
	);

template<typename T>
concept Message=sizeof(T) == sizeof(message_register_type[REGS_PER_MESSAGE]) &&
	std::is_standard_layout_v<T>;

}

using message_span=std::span<message_register_type>;
#endif

/// \brief A message tag describe the message items to be sent
class message_tag final
{
 public:
	friend class message;

	static inline constexpr size_t MESSAGE_TAG_LABEL_LEN = sizeof(message_register_type) * 8 - 16; // 64-16

	static inline constexpr message_register_type EMPTY{ 0 };

	[[nodiscard]] constexpr message_tag() : raw_(0)
	{
	}

	constexpr ~message_tag() = default;

	constexpr message_tag(message_tag&& another) : raw_(std::exchange(another.raw_, 0))
	{
	}

	constexpr message_tag(const message_tag& another) : raw_(another.raw_)
	{
	}

	message_tag& operator=(const message_tag& another)
	{
		if (this == &another)return *this;
		raw_ = another.raw_;
		return *this;
	}

	[[nodiscard]] constexpr explicit message_tag(message_register_type r) : raw_(r)
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

	[[nodiscard]]size_t next_typed_pos() const
	{
		return 1 + u_ + t_;
	}

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

/// \brief A message acceptor constraint how to receive_locked the messages
class message_acceptor final
{
 public:
	static constexpr size_t ACCEPTOR_RECEIVER_LEN = sizeof(buffer_register_type) * 8 - 4;

	friend class message;

	enum receive_window_values : uint64_t
	{
		NONE = 0,
		COMPLETE = UINT64_MAX
	};

	[[nodiscard]] message_acceptor() : raw_(0)
	{
	}

	~message_acceptor() = default;

	constexpr message_acceptor(message_acceptor&& another) : raw_(std::exchange(another.raw_, 0))
	{
	}

	constexpr message_acceptor(const message_acceptor& another) : raw_(another.raw_)
	{
	}

	constexpr message_acceptor(buffer_register_type raw) : raw_(raw)
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

	[[nodiscard]] fpage get_send_region(fpage original) const
	{

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
	/*STRING, */MAP = 1, GRANT
};

// We will not support string
//class string_item final
//{
// public:
//	friend class message;
//
//	static constexpr size_t STRING_ITEM_STR_LEN_BITS = sizeof(message_register_type) * 8 - 10;
//
// public:
//
//	constexpr string_item() : type_{ static_cast<uint64_t>(message_item_types::STRING) }
//	{
//	}
//
//	[[nodiscard]] message_item_types type() const
//	{
//		return (message_item_types)type_;
//	}
//
//	message_span raw() const
//	{
//		return message_span{ raws_, 2 };
//	}
//
// private:
//	union
//	{
//		struct
//		{
//			uint64_t type_: 4;
//			uint64_t res0_: 6;
//			uint64_t string_len_: STRING_ITEM_STR_LEN_BITS;
//			uint64_t string_ptr;
//		}__attribute__((packed));
//		mutable uint64_t raws_[2]{ 0, 0 };
//	};
//}__attribute__((packed));
//
//static_assert(_internals::DoubleMessageItem<string_item>);

///// \brief map item share the page with threads
class map_item final
{
 public:
	constexpr map_item() : type_{ static_cast<uint64_t>(message_item_types::MAP) }
	{
	}

	[[nodiscard]]message_span raw() const
	{
		return message_span{ raws_, 2 };
	}

	[[nodiscard]]uint64_t base() const
	{
		return send_base_;
	}

	[[nodiscard]]fpage page() const
	{
		return send_fpage_;
	}

 private:
	union
	{
		struct
		{
			uint64_t type_: 4;
			uint64_t res0_: 6;
			uint64_t send_base_: 54;

			fpage send_fpage_;
		}__attribute__((packed));
		mutable uint64_t raws_[2]{ 0, 0 };
	};
}__attribute__((packed));

static_assert(_internals::DoubleMessageItem<map_item>);

///// \brief grant item map the page with receiver and unmap for sender
class grant_item final
{
 public:
	constexpr grant_item() : type_{ static_cast<uint64_t>(message_item_types::GRANT) }
	{
	}

	message_span raw() const
	{
		return message_span{ raws_, 2 };
	}

	[[nodiscard]]uint64_t base() const
	{
		return send_base_;
	}

	[[nodiscard]]fpage page() const
	{
		return send_fpage_;
	}
 private:
	union
	{
		struct
		{
			uint64_t type_: 4;
			uint64_t res0_: 6;
			uint64_t send_base_: 54;

			fpage send_fpage_;
		}__attribute__((packed));
		mutable uint64_t raws_[2]{ 0, 0 };
	};
}__attribute__((packed));

static_assert(_internals::DoubleMessageItem<grant_item>);

/// \brief IPC message.
class message final
{
 public:
	constexpr message()
	{
	}
	~message() = default;

	constexpr message(const message& another)
	{
		memmove(raws_, another.raws_, sizeof(raws_));
	}

	constexpr message(message&& another)
	{
		memmove(raws_, another.raws_, sizeof(raws_));
		memset(another.raws_, 0, sizeof(another.raws_));
	}

	void append(_internals::UntypedMessageItem auto& item)
	{
		if (tag_.typed_count() != 0)
		{
			// move all the typed item to next
			for (size_t i = tag_.next_typed_pos();
			     i > 1 + tag_.untyped_count();
			     i--)
			{
				regs_[i] = regs_[i - 1];
			}
		}
		regs_[++tag_.u_] = static_cast<message_register_type>(item);
	}

	void append(_internals::SingleMessageItem auto& item)
	{
		regs_[tag_.next_typed_pos()] = static_cast<uint64_t>(item.raw());
		tag_.t_++;
	}

	void append(_internals::DoubleMessageItem auto& item)
	{
		auto raw = item.raw();

		regs_[tag_.next_typed_pos()] = static_cast<uint64_t>(raw[0]);
		tag_.t_++;

		regs_[tag_.next_typed_pos()] = static_cast<uint64_t>(raw[1]);
		tag_.t_++;
	}

	void put(_internals::UntypedMessageItem auto& item, size_t index)
	{
		regs_[tag_.u_ + index + 1] = static_cast<uint64_t >(item);
	}

	void put(_internals::SingleMessageItem auto& item, size_t index)
	{
		regs_[tag_.u_ + index + 1] = static_cast<uint64_t >(item.raw());
	}

	void put(_internals::DoubleMessageItem auto& item, size_t index)
	{
		auto raw = item.raw();
		regs_[tag_.u_ + index + 1] = static_cast<uint64_t >(raw[0]);
		regs_[tag_.u_ + index + 2] = static_cast<uint64_t >(raw[1]);
	}

	template<_internals::UntypedMessageItem T>
	T at(size_t index)
	{
		return static_cast<T>(regs_[tag_.u_ + index]);
	}

	template<_internals::SingleMessageItem T>
	T at(size_t index)
	{
		return static_cast<T>(regs_[tag_.u_ + index]);
	}

	template<_internals::DoubleMessageItem T>
	T at(size_t index)
	{
		T ret{};
		ret.raw_[0] = static_cast<T>(regs_[tag_.u_ + index]);
		ret.raw_[1] = static_cast<T>(regs_[tag_.u_ + index + 1]);
		return ret;
	}

	void clear()
	{
		tag_ = message_tag{ message_tag::EMPTY };
	}

	[[nodiscard]] message_tag get_tag() const
	{
		return tag_;
	}

	void set_tag(const message_tag& tag)
	{
		tag_ = tag;
	}

	[[nodiscard]] message_span get_items_span() const
	{
		message_span
			ret{ const_cast<message_register_type*>(  raws_ + 1), tag_.untyped_count() + tag_.typed_count() };
		return ret;
	}

	/// \brief get span for specified tag
	/// \param tag
	/// \return
	[[nodiscard]] message_span get_items_span(const message_tag& tag) const
	{
		message_span
			ret{ const_cast<message_register_type*>(  raws_ + 1), tag.untyped_count() + tag.typed_count() };
		return ret;
	}

 private:
	union
	{
		uint64_t raws_[REGS_PER_MESSAGE]{ 0 };

		message_register_type regs_[REGS_PER_MESSAGE];

		message_tag tag_;
	}__attribute__((packed));
}__attribute__((packed));

static_assert(_internals::Message<message>);

}