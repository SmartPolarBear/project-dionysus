#pragma once

#ifdef USE_SCHEDULER_CLASS
#error "USE_SCHEDULER_CLASS can't have been defined"
#endif

#if defined(_SCHEDULER_FCFS)

#define USE_SCHEDULER_CLASS fcfs_scheduler_class
#define SCHEDULER_STATE_BASE fcfs_scheduler_state_base

#elif defined(_SCHEDULER_ULE)

#define USE_SCHEDULER_CLASS ule_scheduler_class
#define SCHEDULER_STATE_BASE ule_scheduler_state_base

#else
#error "scheduler class isn't defined or is wrongly defined."
#endif



