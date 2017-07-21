#ifndef UTF8_H_
#define UTF8_H_

#include <stdlib.h>

/**
 * Maximum number of bytes necessary to encode *any* code point in UTF-8.
 */
#define U8_MAX_BYTES 6

unsigned int u8_strlen_b(const char * str,
                         size_t str_sz);

unsigned int u8_strlen(const char * str);

unsigned int u8_byte_offset(const char * str,
                            unsigned int n);

/**
 * Attempts to read a UTF-8 encoded character from fd. The supplied buffer
 * should be at least as large as U8_MAX_BYTES. Returns the number of bytes
 * written into buffer, or 0 if an invalid UTF-8 character was read.
 */
size_t u8_getc(int fd,
               char * buffer);

#endif
