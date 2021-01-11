#include "task/thread/thread_dispatcher.hpp"

using namespace task;


task::thread_dispatcher::thread_dispatcher(ktl::shared_ptr<process_dispatcher> proc, uint32_t flags)
{

}

error_code_with_result<ktl::shared_ptr<thread_dispatcher>> thread_dispatcher::create(ktl::shared_ptr<
	process_dispatcher> parent,
	uint32_t flags,
	ktl::string_view name)
{

}

thread_dispatcher* thread_dispatcher::get_current()
{
}
[[noreturn]] void thread_dispatcher::exit_current()
{
}

error_code thread_dispatcher::initialize() TA_EXCL(lock)
{
}
error_code thread_dispatcher::start(const entry_status entry, bool initial)
{
}
error_code thread_dispatcher::mark_runnable(const entry_status entry, bool suspend)
{
}
void thread_dispatcher::kill()
{
}

error_code thread_dispatcher::suspend()
{
}
void thread_dispatcher::resume()
{
}

bool thread_dispatcher::is_dying_or_dead() const TA_EXCL(lock)
{
}
bool thread_dispatcher::has_started() const TA_EXCL(lock)
{
}

void thread_dispatcher::on_suspending()
{
}
void thread_dispatcher::on_resuming()
{
}
void thread_dispatcher::exiting_current()
{
}
