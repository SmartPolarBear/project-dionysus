#pragma once
#include "kbl/lock/lockable.hpp"

namespace lock
{
template<typename T>
concept Mutex= Lockable < T> &&
requires(T t)
{
	t.holding();
};

}