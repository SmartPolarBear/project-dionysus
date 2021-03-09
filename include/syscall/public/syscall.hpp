#pragma once

namespace syscall
{

enum SYSCALL_NUMBER
{
	// starts from 1 for the sake of debugging
	SYS_hello = 1,
	SYS_exit,
	SYS_put_str,
	SYS_put_char,
	SYS_send,
	SYS_send_page,
	SYS_receive,
	SYS_receive_page,
	SYS_set_heap_size,
};

}