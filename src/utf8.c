#include "utf8.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

unsigned int u8_strlen_b(const char * str,
                         size_t str_sz)
{
    unsigned int u8_len = 0;
    for (unsigned int i = 0; i < str_sz; ++i) {
        u8_len++;
        unsigned int num_ones = 0;
        for (num_ones = 0; 0x80u == (0x80u & (str[i] << num_ones)); ++num_ones);
        if (num_ones > 0) {
            i += num_ones - 1;
        }
    }
    return u8_len;
}

unsigned int u8_strlen(const char * str)
{
    return u8_strlen_b(str, strlen(str));
}

unsigned int u8_byte_offset(const char * str,
                            unsigned int n)
{
    unsigned int offset = 0;
    size_t limit = strlen(str);
    for (unsigned int u8_char = 0; u8_char < n; ++u8_char) {
        unsigned int num_ones = 0;
        for (num_ones = 0; 0x80u == (0x80u & (str[offset] << num_ones)); ++num_ones);
        if (0 == num_ones) {
            offset++;
        } else {
            offset += num_ones;
        }
    }
    return offset;
}

size_t u8_getc(int fd,
               char * buffer)
{
    // Read one byte.
    if (1 != read(fd, buffer, 1)) {
        return 0;
    }
    // Determine how many total bytes this UTF-8 character consists of.
    unsigned int num_ones = 0;
    for (num_ones = 0; 0x80u == (0x80u & (buffer[0] << num_ones)); ++num_ones);
    if (0 == num_ones) { // ASCII
        return 1;
    } else if (1 == num_ones || num_ones > 6) { // Invalid UTF-8
        return 0;
    } else { // Multi-byte UTF-8
        size_t buffer_sz;
        for (buffer_sz = 1; buffer_sz < num_ones; ++buffer_sz) {
            if (1 != read(fd, buffer + buffer_sz, 1)) {
                return 0;
            }
        }
        return buffer_sz;
    }
}
