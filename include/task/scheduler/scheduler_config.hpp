#pragma once

#ifdef USE_SCHEDULER_CLASS
#error "USE_SCHEDULER_CLASS can't have been defined"
#endif

#if defined(_SCHEDULER_FCFS)

#define USE_SCHEDULER_CLASS fcfs_scheduler_class

#elif defined(_SCHEDULER_ULE)


#define USE_SCHEDULER_CLASS mlfq_scheduler_class

#else
#error "scheduler class isn't defined or is wrongly defined."
#endif



