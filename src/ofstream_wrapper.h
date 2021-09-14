#pragma once
#ifndef CATA_SRC_OFSTREAM_WRAPPER_H
#define CATA_SRC_OFSTREAM_WRAPPER_H

#include "filesystem.h"

/**
 * Wrapper around cata::ofstream that handles error checking and throws on errors.
 *
 * Use like a normal ofstream: the stream is opened in the constructor and
 * closed via @ref close. Both functions check for success and throw std::exception
 * upon any error (e.g. when opening failed or when the stream is in an error state when
 * being closed).
 * Use @ref stream (or the implicit conversion) to access the output stream and to write
 * to it.
 *
 * @note: The stream is closed in the destructor, but no exception is throw from it. To
 * ensure all errors get reported correctly, you should always call `close` explicitly.
 *
 * @note: This uses exclusive I/O.
 */
class ofstream_wrapper
{
    private:
        cata::ofstream file_stream;
        fs::path path;
        fs::path temp_path;

        void open( std::ios::openmode mode );

    public:
        ofstream_wrapper( const fs::path &path, std::ios::openmode mode );
        ~ofstream_wrapper();

        std::ostream &stream() {
            return file_stream;
        }
        explicit operator std::ostream &() {
            return file_stream;
        }

        void close();
};

#endif // CATA_SRC_OFSTREAM_WRAPPER_H
