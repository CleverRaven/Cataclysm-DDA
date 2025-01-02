#pragma once
#ifndef CATA_SRC_MMAP_FILE_H
#define CATA_SRC_MMAP_FILE_H

#include <memory>
#include <string>

#include <ghc/fs_std_fwd.hpp>

// Consider https://github.com/decodeless/mappedfile instead of this.

class mmap_file
{
    public:
        static std::shared_ptr<const mmap_file> map_file( const fs::path &file_path );

        static std::unique_ptr<mmap_file> map_writeable_file( const fs::path &file_path );

        bool resize_file( size_t desired_size );

        ~mmap_file();

        void *base();
        void const *base() const;

        size_t len() const;

    private:
        mmap_file();

        static std::unique_ptr<mmap_file> map_file_generic( const fs::path &file_path, bool writeable );

        // Opaque type to platform specific mmap implementation.
        struct impl;
        std::shared_ptr<impl> pimpl;
};

#endif // CATA_SRC_MMAP_FILE_H
