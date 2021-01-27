#pragma once

namespace kbl
{

template<typename TIter>
class reversed_iterator
{
 public:
	using value_type = typename TIter::value_type;
	using iterator_type = TIter;
	using dummy_type = int;

 public:
	constexpr reversed_iterator() = default;

	constexpr reversed_iterator(const reversed_iterator& another)
		: iter_(another.iter_)
	{
	}

	constexpr reversed_iterator& operator=(const reversed_iterator& another)
	{
		if (this == &another)return *this;

		this->iter_ = another.iter_;
		return *this;
	}

	constexpr reversed_iterator(reversed_iterator&& another)
		: iter_(another.iter_)
	{
	}

	constexpr explicit reversed_iterator(const TIter& iter)
		: iter_(iter)
	{
	}

	template<typename ...Args>
	constexpr explicit reversed_iterator(Args&& ...args)
		:iter_(std::forward<Args>(args)...)
	{
	}

	iterator_type& get_iterator()
	{
		return iter_;
	}

	value_type& operator*()
	{
		return *operator->();
	}

	value_type* operator->()
	{
		return iter_.operator->();
	}

	bool operator==(const reversed_iterator& other) const
	{
		return iter_ == other.iter_;
	}

	reversed_iterator& operator++()
	{
		--iter_;
		return *this;
	}

	reversed_iterator& operator--()
	{
		++iter_;
		return *this;
	}

	reversed_iterator operator++(dummy_type) noexcept
	{
		auto t = *this;
		--iter_;
		return t;
	}

	reversed_iterator operator--(dummy_type) noexcept
	{
		auto t = *this;
		++iter_;
		return t;
	}

 private:
	iterator_type iter_{};
};

}