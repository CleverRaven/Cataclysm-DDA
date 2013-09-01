#ifndef _COMPRESS_H_
#define _COMPRESS_H_

#ifdef COMPRESS
#include <zlib.h>
#endif

#include <string>

std::string compress_string(std::string data);

std::string decompress_string(std::string data);

#endif
