#include "server_syscalls.hpp"
#include "debug_output.hpp"

#include "boot/multiboot2.h"

#include <cstdlib>
#include <cstring>

uintptr_t g_framebuffer_addr = 0;
multiboot_tag_framebuffer* g_tag_framebuffer = nullptr;
volatile uint16_t* g_framebuffer = nullptr;

[[maybe_unused]]static inline constexpr uint16_t make_cga_color_attrib(uint8_t foreground, uint8_t background)
{
	return (background << 4) | (foreground & 0x0F);
}

[[maybe_unused]]static inline constexpr uint16_t make_cga_char(char content, uint16_t attr)
{
	uint16_t ret = content | (attr << 8);
	return ret;
}

extern "C" int main(int argc, char** argv)
{
	if (argc < 2 || argv[1] == nullptr)
	{
		return -ERROR_INVALID;
	}

	g_tag_framebuffer = reinterpret_cast<decltype(g_tag_framebuffer)>(argv[1]);
	g_framebuffer_addr = g_tag_framebuffer->common.framebuffer_addr;

	g_framebuffer = (volatile uint16_t*)g_framebuffer_addr;

//	for (size_t i = 0;
//		 i < g_tag_framebuffer->common.framebuffer_width * g_tag_framebuffer->common.framebuffer_height;
//		 i++)
//	{
//		g_framebuffer[i] = make_cga_char('a' + (i % 25), make_cga_color_attrib(3, 2));
//	}

	return 0;
}