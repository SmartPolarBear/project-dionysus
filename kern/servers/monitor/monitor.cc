#include "server_syscalls.hpp"
#include "debug_output.hpp"

#include "boot/multiboot2.h"

#include <cstdlib>
#include <cstring>

uintptr_t g_framebuffer_addr = 0;
multiboot_tag_framebuffer* g_tag_framebuffer = nullptr;

char str[16] = { 0 };

extern "C" int main(int argc, char** argv)
{
	if (argc < 2 || argv[1] == nullptr)
	{
		return -ERROR_INVALID;
	}

	g_tag_framebuffer = reinterpret_cast<decltype(g_tag_framebuffer)>(argv[1]);
	g_framebuffer_addr = g_tag_framebuffer->common.framebuffer_addr;

	return 0;
}