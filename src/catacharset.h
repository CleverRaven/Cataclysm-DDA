#ifndef CATACHARSET_H
#define CATACHARSET_H
#include <stdint.h>
#include <string>
#define ANY_LENGTH 5
#define UNKNOWN_UNICODE 0xFFFD

// get a unicode character from a utf8 string
unsigned UTF8_getch(const char **src, int *srclen);
// from wcwidth.c, return "cell" width of a unicode char
int mk_wcwidth(uint32_t ucs);
// convert cursorx value to byte position
int cursorx_to_position(const char* line, int cursorx, int* prevppos=NULL);
//erease for characters insertion
int erease_utf8_by_cw( char* t, int cw, int len, int maxlen);
int utf8_width(const char* s);
std::string utf8_substr(std::string s, int start, int size=-1);
std::string utf32_to_utf8(unsigned ch);

std::string base64_encode(std::string str);
std::string base64_decode(std::string str);
#endif
