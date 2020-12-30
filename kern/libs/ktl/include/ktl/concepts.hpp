#pragma once

#include "ktl/type_traits.hpp"
#include <concepts>

namespace ktl
{
	template<class T>
	concept Pointer = std::is_pointer<T>::value;

	template<typename TList, typename TChild>
	concept ListOfTWithBound=
	requires(TList l){ l[0]; (l[0])->TChild; };

}