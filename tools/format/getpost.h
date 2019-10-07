/*****************************************************************************
The MIT License

Copyright (c) 2007 Guy Rutenberg

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*****************************************************************************/

#ifndef __GETPOST_H__
#define __GETPOST_H__

#include <cstring>
#include <string>
#include <stdlib.h>
#include <map>
#include <new>

inline std::string urlDecode( std::string str )
{
    std::string temp;
    int i;
    char tmp[5], tmpchar;
    strcpy( tmp, "0x" );
    int size = str.size();
    for( i = 0; i < size; i++ ) {
        if( str[i] == '%' ) {
            if( i + 2 < size ) {
                tmp[2] = str[i + 1];
                tmp[3] = str[i + 2];
                tmp[4] = '\0';
                tmpchar = static_cast<char>( strtol( tmp, NULL, 0 ) );
                temp += tmpchar;
                i += 2;
                continue;
            } else {
                break;
            }
        } else if( str[i] == '+' ) {
            temp += ' ';
        } else {
            temp += str[i];
        }
    }
    return temp;
}

inline void initializeGet( std::map <std::string, std::string> &Get )
{
    std::string tmpkey, tmpvalue;
    std::string *tmpstr = &tmpkey;
    char *raw_get = getenv( "QUERY_STRING" );
    if( raw_get == NULL ) {
        Get.clear();
        return;
    }
    while( *raw_get != '\0' ) {
        if( *raw_get == '&' ) {
            if( tmpkey != "" ) {
                Get[urlDecode( tmpkey )] = urlDecode( tmpvalue );
            }
            tmpkey.clear();
            tmpvalue.clear();
            tmpstr = &tmpkey;
        } else if( *raw_get == '=' ) {
            tmpstr = &tmpvalue;
        } else {
            ( *tmpstr ) += ( *raw_get );
        }
        raw_get++;
    }
    //enter the last pair to the map
    if( tmpkey != "" ) {
        Get[urlDecode( tmpkey )] = urlDecode( tmpvalue );
        tmpkey.clear();
        tmpvalue.clear();
    }
}

inline void initializePost( std::map <std::string, std::string> &Post )
{
    std::string tmpkey, tmpvalue;
    std::string *tmpstr = &tmpkey;
    int content_length;
    char *ibuffer;
    char *buffer = NULL;
    char *strlength = getenv( "CONTENT_LENGTH" );
    if( strlength == NULL ) {
        Post.clear();
        return;
    }
    content_length = atoi( strlength );
    if( content_length == 0 ) {
        Post.clear();
        return;
    }

    try {
        buffer = new char[content_length * sizeof( char )];
    } catch( std::bad_alloc &xa ) {
        Post.clear();
        return;
    }
    if( fread( buffer, sizeof( char ), content_length,
               stdin ) != static_cast<unsigned int>( content_length ) ) {
        Post.clear();
        return;
    }
    *( buffer + content_length ) = '\0';
    ibuffer = buffer;
    while( *ibuffer != '\0' ) {
        if( *ibuffer == '&' ) {
            if( tmpkey != "" ) {
                Post[urlDecode( tmpkey )] = urlDecode( tmpvalue );
            }
            tmpkey.clear();
            tmpvalue.clear();
            tmpstr = &tmpkey;
        } else if( *ibuffer == '=' ) {
            tmpstr = &tmpvalue;
        } else {
            ( *tmpstr ) += ( *ibuffer );
        }
        ibuffer++;
    }
    //enter the last pair to the map
    if( tmpkey != "" ) {
        Post[urlDecode( tmpkey )] = urlDecode( tmpvalue );
        tmpkey.clear();
        tmpvalue.clear();
    }
}

#endif /*__GETPOST_H__*/
