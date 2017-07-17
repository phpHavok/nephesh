#include "utf8.h"

unsigned int utf8_strlen(const char * str,
                         unsigned int str_sz)
{
    unsigned int len = 0;
    for (unsigned int i = 0; i < str_sz; ++i) {
        len++;
        if (0xFCU == (0xFCU & str[i])) {
            i += 5;
        } else if (0xF8U == (0xF8U & str[i])) {
            i += 4;
        } else if (0xF0U == (0xF0U & str[i])) {
            i += 3;
        } else if (0xE0U == (0xE0U & str[i])) {
            i += 2;
        } else if (0xC0U == (0xC0U & str[i])) {
            i += 1;
        }
    }
    return len;
}

unsigned int utf8_offset(const char * str,
                         unsigned int str_sz,
                         unsigned int n)
{
    unsigned int offset = 0;
    for (unsigned int i = 0; i < n && offset < str_sz; ++i) {
        offset++;
        if (offset > str_sz - 1) {
            break;
        }
        while (0x80U == (0xC0U & str[offset])) {
            offset++;
        }
    }
    return offset;
}
