#include "catacharset.h"
#include <string.h>
#include "wcwidth.c"

//copied from SDL2_ttf code
unsigned UTF8_getch(const char **src, int *srclen)
{
    const unsigned char *p = *(const unsigned char**)src;
    int left = 0;
    bool overlong = false;
    bool underflow = false;
    unsigned ch = UNKNOWN_UNICODE;

    if (*srclen == 0) {
        return UNKNOWN_UNICODE;
    }
    if (p[0] >= 0xFC) {
        if ((p[0] & 0xFE) == 0xFC) {
            if (p[0] == 0xFC && (p[1] & 0xFC) == 0x80) {
                overlong = true;
            }
            ch = (unsigned) (p[0] & 0x01);
            left = 5;
        }
    } else if (p[0] >= 0xF8) {
        if ((p[0] & 0xFC) == 0xF8) {
            if (p[0] == 0xF8 && (p[1] & 0xF8) == 0x80) {
                overlong = true;
            }
            ch = (unsigned) (p[0] & 0x03);
            left = 4;
        }
    } else if (p[0] >= 0xF0) {
        if ((p[0] & 0xF8) == 0xF0) {
            if (p[0] == 0xF0 && (p[1] & 0xF0) == 0x80) {
                overlong = true;
            }
            ch = (unsigned) (p[0] & 0x07);
            left = 3;
        }
    } else if (p[0] >= 0xE0) {
        if ((p[0] & 0xF0) == 0xE0) {
            if (p[0] == 0xE0 && (p[1] & 0xE0) == 0x80) {
                overlong = true;
            }
            ch = (unsigned) (p[0] & 0x0F);
            left = 2;
        }
    } else if (p[0] >= 0xC0) {
        if ((p[0] & 0xE0) == 0xC0) {
            if ((p[0] & 0xDE) == 0xC0) {
                overlong = true;
            }
            ch = (unsigned) (p[0] & 0x1F);
            left = 1;
        }
    } else {
        if ((p[0] & 0x80) == 0x00) {
            ch = (unsigned) p[0];
        }
    }
    ++*src;
    --*srclen;
    while (left > 0 && *srclen > 0) {
        ++p;
        if ((p[0] & 0xC0) != 0x80) {
            ch = UNKNOWN_UNICODE;
            break;
        }
        ch <<= 6;
        ch |= (p[0] & 0x3F);
        ++*src;
        --*srclen;
        --left;
    }
    if (left > 0) {
        underflow = true;
    }
    if (overlong || underflow ||
        (ch >= 0xD800 && ch <= 0xDFFF) ||
        (ch == 0xFFFE || ch == 0xFFFF) || ch > 0x10FFFF) {
        ch = UNKNOWN_UNICODE;
    }
    return ch;
}

//Calculate width of a unicode string
//Latin characters have a width of 1
//CJK characters have a width of 2, etc
int utf8_width(const char* s)
{
    int len = strlen(s);
    const char* ptr = s;
    int w = 0;
    while(len>0)
    {
        unsigned ch = UTF8_getch(&ptr, &len);
        if(ch!=UNKNOWN_UNICODE) {
            w += mk_wcwidth((wchar_t)ch);
        } else {
            continue;
        }
    }
    return w;
}

//Convert cursor position to byte offset
//returns the first character position in bytes behind the cursor position.
//If the cursor is not on the first half of the character, 
//prevpos (which points to the first byte of the cursor located char)
// should be a different value.
int cursorx_to_position(const char* line, int cursorx, int* prevpos)
{
	int dummy;
	int i=0, c=0, *p=prevpos?prevpos:&dummy;
	*p = 0;
	while(c<cursorx)
	{
		const char* utf8str = line+i;
		int len = ANY_LENGTH;
		unsigned ch = UTF8_getch(&utf8str, &len);
		int cw = mk_wcwidth((wchar_t)ch);
		len = ANY_LENGTH-len;
        if(len<=0) len=1;
		i+=len;
		if(cw<=0) cw=1;
		c+=cw;
		if(c<=cursorx) *p = i;
	}
	return i;
}

//Erase character by unicode char width.
//Fill the characters with spaces.
void erease_utf8_by_cw( char* t, int cw, int clen, int maxlen)
{
    static char buf[8000]; //LOL
    int c=0,i=0;
    while(c<cw)
    {
		const char* utf8str = t+i;
		int len = ANY_LENGTH;
		unsigned ch = UTF8_getch(&utf8str, &len);
		int cw = mk_wcwidth((wchar_t)ch);
		len = ANY_LENGTH-len;
        if(len<=0) len=1;
		i+=len;
		if(cw<=0) cw=1;
		c+=cw;
    }
    if(cw==c && clen==i)
    {
        memset(t, ' ', clen);
    }
    else
    {
        memcpy(buf, t+i, maxlen-i);
        memset(t, ' ', clen+c-cw);
        memcpy(t+clen+c-cw, buf, maxlen-clen-c+cw);
    }

}

//cut ut8 string by character size
//utf8_substr("正正正正", 1, 4) returns
// " 正 "
// Broken characters will be filled with space
std::string utf8_substr(std::string s, int start, int size)
{
    static char buf[8000];
    int len = strlen(s.c_str());
    int pos;
    strcpy(buf, s.c_str());
    int begin = cursorx_to_position(buf, start, &pos);
    if(begin!=pos)
    {
		const char* ts = buf+pos;
		int l = ANY_LENGTH;
		unsigned tc = UTF8_getch(&ts, &l);
		int tw = mk_wcwidth((wchar_t)tc);
		erease_utf8_by_cw(buf+pos, tw, tw, len-pos-1);
		begin = pos+tw-1;
    }

    if(size>0) 
    {
        int end = cursorx_to_position(buf, start+size-1, &pos);
        if(end!=pos)
        {
            const char* ts = buf+pos;
            int l = ANY_LENGTH;
            unsigned tc = UTF8_getch(&ts, &l);
            int tw = mk_wcwidth((wchar_t)tc);
            erease_utf8_by_cw(buf+pos, tw, tw, len-pos-1);
            end = pos+tw-1;
        }
        buf[end+1] = '\0';
    }
    
    return std::string(buf+start);
}


