#ifndef UTF8_H_
#define UTF8_H_

unsigned int utf8_strlen(const char * str,
                         unsigned int str_sz);

unsigned int utf8_offset(const char * str,
                         unsigned int str_sz,
                         unsigned int n);

#endif
