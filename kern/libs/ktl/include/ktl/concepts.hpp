#pragma once

#include "ktl/type_traits.hpp"
#include <concepts>

namespace ktl
{
template<class T>
concept Pointer = std::is_pointer_v<T>;

template<typename TFrom, typename TTo>
concept Convertible=std::is_convertible_v<TFrom, TTo>;

}