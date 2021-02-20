#include "object/object.hpp"

template<typename T>
object::object_counter_type object::object<T>::kobject_counter_{0};

template<typename T>
object::koid_counter_type object::object<T>::koid_counter_{0};