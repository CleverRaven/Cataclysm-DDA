#include "catacharset.h"
#include <string.h>
#include "debug.h"
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

std::string utf32_to_utf8(unsigned ch) {
    char out[5];
    char* buf = out;
    static const unsigned char utf8FirstByte[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
    int utf8Bytes;
    if (ch < 0x80) {
        utf8Bytes = 1;
    } else if (ch < 0x800) {
        utf8Bytes = 2;
    } else if (ch < 0x10000) {
        utf8Bytes = 3;
    } else if (ch <= 0x10FFFF) {
        utf8Bytes = 4;
    } else {
        utf8Bytes = 3;
        ch = UNKNOWN_UNICODE;
    }

    buf += utf8Bytes;
    switch (utf8Bytes) {
        case 4: 
            *--buf = (ch|0x80)&0xBF;
            ch >>= 6;
        case 3:
            *--buf = (ch|0x80)&0xBF;
            ch >>= 6;
        case 2: 
            *--buf = (ch|0x80)&0xBF;
            ch >>= 6;
        case 1: 
            *--buf = ch|utf8FirstByte[utf8Bytes];
    }
    out[utf8Bytes] = '\0';
    return out;
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
            w += mk_wcwidth(ch);
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
        int cw = mk_wcwidth(ch);
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
//returns length modified
int erease_utf8_by_cw( char* t, int cw, int clen, int maxlen)
{
    static char buf[8000]; //LOL
    int c=0,i=0;
    while(c<cw)
    {
        const char* utf8str = t+i;
        int len = ANY_LENGTH;
        unsigned ch = UTF8_getch(&utf8str, &len);
        int cw = mk_wcwidth(ch);
        len = ANY_LENGTH-len;

        if(len<=0) len=1;
        i+=len;
        if(cw<=0) cw=1;
        c+=cw;
    }
    if(cw==c && clen==i)
    {
        memset(t, ' ', clen);
        return 0;
    }
    else
    {
        int filled = clen+c-cw;
        memcpy(buf, t+i, maxlen-i);
        memset(t, ' ', filled);
        memcpy(t+filled, buf, maxlen-filled);
        return filled-i;
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
        int tw = mk_wcwidth(tc);
        erease_utf8_by_cw(buf+pos, tw, tw, len-pos-1);
    }

    if(size>0) 
    {
        int end = cursorx_to_position(buf, start+size-1, &pos);
        if(end!=pos)
        {
            const char* ts = buf+pos;
            int l = ANY_LENGTH;
            unsigned tc = UTF8_getch(&ts, &l);
            int tw = mk_wcwidth(tc);
            erease_utf8_by_cw(buf+pos, tw, tw, len-pos-1);
            end = pos+tw-1;
        }
        buf[end+1] = '\0';
    }
    
    return std::string(buf+start);
}

static const char base64_encoding_table[] = 
{'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
'w', 'x', 'y', 'z', '0', '1', '2', '3',
'4', '5', '6', '7', '8', '9', '+', '-'};

static char base64_decoding_table[256];
static const  int mod_table[] = {0, 2, 1};

static void build_base64_decoding_table() {
    static bool built = false;
    if(!built) {
        for (int i = 0; i < 64; i++) {
            base64_decoding_table[(unsigned char) base64_encoding_table[i]] = i;
        }
        built = true;
    }
}

std::string base64_encode(std::string str) {
    //assume it is already encoded
    if(str.length()>0 && str[0]=='#') {
        return str;
    }

    int input_length = str.length();
    int output_length = 4 * ((input_length + 2) / 3);

    char *encoded_data = new char[output_length+1];
    const unsigned char* data = (const unsigned char*)str.c_str();

    for (int i = 0, j = 0; i < input_length;) {

        unsigned octet_a = i < input_length ? data[i++] : 0;
        unsigned octet_b = i < input_length ? data[i++] : 0;
        unsigned octet_c = i < input_length ? data[i++] : 0;

        unsigned triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = base64_encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = base64_encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = base64_encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = base64_encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[output_length - 1 - i] = '=';

    encoded_data[output_length] = '\0';
    std::string ret = "#";
    ret += encoded_data;
    delete[] encoded_data;

    //DebugLog()<<"base64 encoded: \n"<<str<<"\n"<<ret<<"\n";

    return ret;
}


std::string base64_decode(std::string str) {
    // do not decode if it is not base64
    if(str.length()==0 || str[0]!='#') {
        return str;
    }

    build_base64_decoding_table();
    
    std::string instr = str.substr(1);

    int input_length = instr.length();

    if (input_length % 4 != 0) {
        return str;
    }

    int output_length = input_length / 4 * 3;
    const char* data = (const char*)instr.c_str();

    if (data[input_length - 1] == '=') output_length--;
    if (data[input_length - 2] == '=') output_length--;

    unsigned char *decoded_data = new unsigned char[output_length+1];

    for (int i = 0, j = 0; i < input_length;) {

        unsigned sextet_a = data[i] == '=' ? 0 & i++ : base64_decoding_table[(unsigned char)data[i++]];
        unsigned sextet_b = data[i] == '=' ? 0 & i++ : base64_decoding_table[(unsigned char)data[i++]];
        unsigned sextet_c = data[i] == '=' ? 0 & i++ : base64_decoding_table[(unsigned char)data[i++]];
        unsigned sextet_d = data[i] == '=' ? 0 & i++ : base64_decoding_table[(unsigned char)data[i++]];

        unsigned triple = (sextet_a << 3 * 6)
        + (sextet_b << 2 * 6)
        + (sextet_c << 1 * 6)
        + (sextet_d << 0 * 6);

        if (j < output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
    }

    decoded_data[output_length] = 0;

    std::string ret = (char*)decoded_data;
    delete[] decoded_data;

    //DebugLog()<<"base64 decoded: \n"<<str<<"\n"<<ret<<"\n";

    return ret;
}


