#include "system/types.h"

namespace task
{
class scheduler_state_base
{
 public:
	virtual bool nice_available() = 0;
	virtual int32_t get_nice() = 0;
	virtual void set_nice(int32_t nice) = 0;
};
}