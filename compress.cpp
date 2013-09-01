#include "compress.h"
#include "output.h"
#include "options.h"

#define CHUNK 16384

std::string compress_string(std::string data)
{
#ifdef COMPRESS
    if (!OPTIONS["COMPRESS_SAVES"])
    {
        return data;
    }

    std::string out_string;
    int ret;
    z_stream stream;
    unsigned char out[CHUNK + 1];

    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;

    ret = deflateInit(&stream, Z_DEFAULT_COMPRESSION);
    if(ret != Z_OK)
    {
        debugmsg("Compression initialization failed, %s", stream.msg);
        return data;
    }

    stream.avail_in = data.size();
    stream.next_in = (unsigned char*)data.c_str();

    do {
        stream.avail_out = CHUNK;
        stream.next_out = out;

        deflate(&stream, Z_FINISH);

        out_string.append((char*)out, (size_t)(CHUNK - stream.avail_out));
    } while (stream.avail_out == 0);

    deflateEnd(&stream);
    return out_string;
#else
    return data;
#endif
}

std::string decompress_string(std::string data)
{
#ifdef COMPRESS
    int ret;
    z_stream stream;
    unsigned char out[CHUNK];
    std::string result;

    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.avail_in = data.size();
    stream.next_in = (unsigned char*)data.c_str();

    ret = inflateInit(&stream);
    if (ret != Z_OK)
    {
        debugmsg("Decompression initialization failed, %s", stream.msg);
        return data;
    }

    do {
        stream.avail_out = CHUNK;
        stream.next_out = out;
        ret = inflate(&stream, Z_FINISH);
        if (ret == Z_DATA_ERROR) // The data wasn't actually compressed
        {
            inflateEnd(&stream);
            return data;
        }
        result.append((char*)out, (size_t)(CHUNK - stream.avail_out));
    } while (stream.avail_out == 0);

    return result;
#else
    return data;
#endif
}
