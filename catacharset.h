
#include <wchar.h>
#define ANY_LENGTH 5
#define UNKNOWN_UNICODE 0xFFFD

// get a unicode character from a utf8 string
Uint32 UTF8_getch(const char **src, size_t *srclen);
// from wcwidth.c, return "cell" width of a unicode char
int mk_wcwidth(wchar_t ucs);
// convert cursorx value to byte position
int cursorx_to_position(const char* line, int cursorx);
//erease for characters insertion
void erease_utf8_by_cw( char* t, int cw, int len);

