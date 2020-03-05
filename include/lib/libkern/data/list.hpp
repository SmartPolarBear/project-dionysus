#pragma once

#include "sys/types.h"
#include "lib/libkern/data/list.h"

// a list implementation with lock
template <typename TElement> class list
{
  private:
    using value_type = TElement;
    
    struct node
    {
        TElement element;
        list_head list;
    }

    
};