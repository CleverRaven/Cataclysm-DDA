#pragma once
#ifndef CATA_SRC_MMAP_FILE_H
#define CATA_SRC_MMAP_FILE_H

#include <memory>
#include <string>

#include <filesystem>

// Consider https://github.com/decodeless/mappedfile instead of this.

class mmap_file
{
    public:
        /**
         * Returns a readonly memory mapping of the entire file at the given path.
         * Returns nullptr if the file does not exist.
         */
        static std::shared_ptr<const mmap_file> map_file( const std::filesystem::path &file_path );

        /**
         * Returns a writeable memory mapping of the entire file at the given path.
         * Returns nullptr if the file cannot be opened or created or another error occurs.
         * If the file does not exist, but no errors occur, then this returns a non-null mmap_file
         * but base() is nullptr and len() is 0.
         */
        static std::unique_ptr<mmap_file> map_writeable_file( const std::filesystem::path &file_path );

        /**
         * Sets the underlying file size to the set size, and remaps the file to that range.
         * Returns false on failure.
         * Do not assume base() or len() are unchanged across any call to this function regardless
         * of return value.
         */
        bool resize_file( size_t desired_size );

        ~mmap_file();

        /**
         * The starting address of the memory mapping.
         */
        void *base();
        void const *base() const;

        /**
         * The valid accessible length of the memory mapping.
         */
        size_t len() const;

        /**
         * Synchronously write modified parts of the file to disk.
         * Use sparingly, this is slow.
         */
        void flush();
        /**
         * Synchronously write modified parts of the file to disk
         * within the range specified by [offset, len].
         * Use sparingly, this is slow.
         */
        void flush( size_t offset, size_t len );

    private:
        mmap_file();

        static std::unique_ptr<mmap_file> map_file_generic(
            const std::filesystem::path &file_path,
            bool writeable );

        // Opaque type to platform specific mmap implementation.
        struct impl;
        std::shared_ptr<impl> pimpl;
};

#endif // CATA_SRC_MMAP_FILE_H
