#pragma once

namespace kbl
{

//FIXME: this result in the constructor not called.
template<typename T>
class singleton
{
 public:
	static T& instance()
	{
		static T inst;
		return inst;
	}

	singleton(const singleton&) = delete;
	singleton& operator=(const singleton&) = delete;

 protected:
	singleton() = default;
};

}