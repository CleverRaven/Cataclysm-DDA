

#include "wcwidth.c"

//copied from SDL2_ttf code
Uint32 UTF8_getch(const char **src, size_t *srclen)
{
    const Uint8 *p = *(const Uint8**)src;
    size_t left = 0;
    SDL_bool overlong = SDL_FALSE;
    SDL_bool underflow = SDL_FALSE;
    Uint32 ch = UNKNOWN_UNICODE;

    if (*srclen == 0) {
        return UNKNOWN_UNICODE;
    }
    if (p[0] >= 0xFC) {
        if ((p[0] & 0xFE) == 0xFC) {
            if (p[0] == 0xFC) {
                overlong = SDL_TRUE;
            }
            ch = (Uint32) (p[0] & 0x01);
            left = 5;
        }
    } else if (p[0] >= 0xF8) {
        if ((p[0] & 0xFC) == 0xF8) {
            if (p[0] == 0xF8) {
                overlong = SDL_TRUE;
            }
            ch = (Uint32) (p[0] & 0x03);
            left = 4;
        }
    } else if (p[0] >= 0xF0) {
        if ((p[0] & 0xF8) == 0xF0) {
            if (p[0] == 0xF0) {
                overlong = SDL_TRUE;
            }
            ch = (Uint32) (p[0] & 0x07);
            left = 3;
        }
    } else if (p[0] >= 0xE0) {
        if ((p[0] & 0xF0) == 0xE0) {
            if (p[0] == 0xE0) {
                overlong = SDL_TRUE;
            }
            ch = (Uint32) (p[0] & 0x0F);
            left = 2;
        }
    } else if (p[0] >= 0xC0) {
        if ((p[0] & 0xE0) == 0xC0) {
            if ((p[0] & 0xDE) == 0xC0) {
                overlong = SDL_TRUE;
            }
            ch = (Uint32) (p[0] & 0x1F);
            left = 1;
        }
    } else {
        if ((p[0] & 0x80) == 0x00) {
            ch = (Uint32) p[0];
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
        underflow = SDL_TRUE;
    }
    if (overlong || underflow ||
        (ch >= 0xD800 && ch <= 0xDFFF) ||
        (ch == 0xFFFE || ch == 0xFFFF) || ch > 0x10FFFF) {
        ch = UNKNOWN_UNICODE;
    }
    return ch;
}


int cursorx_to_position(const char* line, int cursorx)
{
	int i=0, c=0;
	while(c<cursorx)
	{
		const char* utf8str = line+i;
		size_t len = ANY_LENGTH;
		Uint32 ch = UTF8_getch(&utf8str, &len);
		int cw = mk_wcwidth((wchar_t)ch);
		len = ANY_LENGTH-len;
        if(len<=0) len=1;
		i+=len;
		if(cw<=0) cw=1;
		c+=cw;
	}
	return i;
}

//erase character by unicode char width
//fill the character with spaces
//return: bytes affected
void erease_utf8_by_cw( char* t, int cw, int len)
{
    static char buf[8000]; //LOL
    int c=0,i=0;
    while(c<cw)
    {
		const char* utf8str = t+i;
		size_t len = ANY_LENGTH;
		Uint32 ch = UTF8_getch(&utf8str, &len);
		int cw = mk_wcwidth((wchar_t)ch);
		len = ANY_LENGTH-len;
        if(len<=0) len=1;
		i+=len;
		if(cw<=0) cw=1;
		c+=cw;
    }
    if(cw==c && len==i)
    {
        memset(t, ' ', len);
    }
    else
    {
        strcpy(buf, t+i);
        memset(t, ' ', len+c-cw);
        strcpy(t+len+c-cw, buf);
    }
}

