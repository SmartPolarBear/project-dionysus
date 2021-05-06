#include "system/types.h"

namespace task
{
class scheduler_state_base
{
 public:
	virtual void on_tick() = 0;
	virtual void on_sleep() = 0;
	virtual void on_wakeup() = 0;

	virtual bool nice_available() = 0;
	virtual int32_t get_nice() = 0;
	virtual void set_nice(int32_t nice) = 0;
};
}