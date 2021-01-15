//
// Created by bear on 6/28/20.
//

#include "syscall_client.hpp"
#include "dionysus.hpp"


#include "system/syscall.h"
#include "system/error.hpp"

#include <cstring>
#include <cstdlib>

void write_format(const char* fmt, ...)
{

	va_list ap;
	va_start(ap, fmt);

	write_format_a(fmt, ap);

	va_end(ap);
}

void write_format_a(const char* fmt, va_list ap)
{
	// TODO: lock
	// buffer for converting ints with itoa

	constexpr size_t MAXNUMBER_LEN = 64;
	char nbuf[MAXNUMBER_LEN] = {};

	if (fmt == nullptr)
	{
		put_str("write_format: Invalid null format strings.\n");
	}

	auto char_data = [](char d) -> decltype(d & 0xFF)
	{
	  return d & 0xFF;
	};

	size_t i = 0, c = 0;
	auto next_char = [&i, fmt, char_data]() -> char
	{
	  return char_data(fmt[++i]);
	};

	char ch = 0;
	const char* s = nullptr;

	for (i = 0; (c = char_data(fmt[i])) != 0; i++)
	{

		if (c != '%')
		{
			put_char(c);
			continue;
		}

		c = next_char();

		if (c == 0)
		{
			break;
		}

		// reset buffer for itoa()
		memset(nbuf, 0, sizeof(nbuf));

		switch (c)
		{
		case 'c':
		{ // this va_arg uses int
			// otherwise, a warning will be given, saying
			// warning: second argument to 'va_arg' is of promotable type 'char'; this va_arg has undefined behavior because arguments will be promoted to 'int
			ch = va_arg(ap, int);
			put_char(char_data(ch));
			break;
		}
		case 'f':
		{
			//FIXME: va_arg(ap, double) always return wrong value
			put_str("%f flags is disabled because va_arg(ap, double) always return wrong value");

			size_t len = ftoa_ex(va_arg(ap, double), nbuf, 10);
			for (size_t i = 0; i < len; i++)
			{
				put_char(nbuf[i]);
			}
			break;
		}
		case 'd':
		{
			size_t len = itoa_ex(nbuf, va_arg(ap, int), 10);
			for (size_t i = 0; i < len; i++)
			{
				put_char(nbuf[i]);
			}
			break;
		}
		case 'l':
		{
			char nextchars[2] = { 0 };
			nextchars[0] = next_char();
			nextchars[1] = next_char();

			if (nextchars[0] == 'l' && nextchars[1] == 'd')
			{
				size_t len = itoa_ex(nbuf, va_arg(ap, unsigned long long), 10);
				for (size_t i = 0; i < len; i++)
				{
					put_char(nbuf[i]);
				}
			}
			else
			{
				// Print unknown % sequence to draw attention.
				put_char('%');
				put_char('l');
				put_char(nextchars[0]);
				put_char(nextchars[1]);
			}
			break;
		}
		case 'x':
		{
			size_t len = itoa_ex(nbuf, va_arg(ap, int), 16);
			for (size_t i = 0; i < len; i++)
			{
				put_char(nbuf[i]);
			}
			break;
		}
		case 'p':
		{
			static_assert(sizeof(size_t*) == sizeof(size_t));

			size_t len = itoa_ex(nbuf, va_arg(ap, int), 16);
			for (size_t i = 0; i < len; i++)
			{
				put_char(nbuf[i]);
			}
			break;
		}
		case 's':
		{
			if ((s = va_arg(ap, char *)) == 0)
			{
				s = "(null)";
			}
			for (; *s; s++)
			{
				put_char(*s);
			}
			break;
		}
		case '%':
		{
			put_char('%');
			break;
		}
		default:
		{
			// Print unknown % sequence to draw attention.
			put_char('%');
			put_char(c);
			break;
		}
		}
	}
}