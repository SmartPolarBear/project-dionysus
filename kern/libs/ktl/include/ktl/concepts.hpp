#pragma once

#include "ktl/type_traits.hpp"
#include <concepts>

namespace ktl
{
template<class T>
concept is_pointer = std::is_pointer_v<T>;

template<typename TFrom, typename TTo>
concept convertible_to=std::convertible_to<TFrom, TTo>;

template<typename T>
concept is_trivially_copyable=std::is_trivially_copyable_v<T>;

}