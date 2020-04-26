

#include "sys/types.h"

// extern "C" [[deprecated("Deprecated for safety reasons")]] char *itoa(char *buf, size_t n, int base)
// {
//     size_t tmp = 0;
//     int i = 0;

//     tmp = n;
//     do
//     {
//         tmp = n % base;
//         buf[i++] = (tmp < 10) ? (tmp + '0') : (tmp + 'a' - 10);
//     } while (n /= base);
//     buf[i--] = 0;

//     for (int j = 0; j < i; j++, i--)
//     {
//         tmp = buf[j];
//         buf[j] = buf[i];
//         buf[i] = tmp;
//     }

//     return buf;
// }

size_t itoa_ex(char *buf, unsigned long long n, int base)
{
    size_t tmp = 0;
    int i = 0;

    tmp = n;
    do
    {
        tmp = n % base;
        buf[i++] = (tmp < 10) ? (tmp + '0') : (tmp + 'a' - 10);
    } while (n /= base);

    size_t len = i;

    buf[i--] = 0;

    for (int j = 0; j < i; j++, i--)
    {
        tmp = buf[j];
        buf[j] = buf[i];
        buf[i] = tmp;
    }

    return len;
}
