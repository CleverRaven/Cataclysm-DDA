#pragma once
#ifndef CATA_SRC_MMAP_FILE_H
#define CATA_SRC_MMAP_FILE_H

#include <memory>
#include <string>

#include <filesystem>

class mmap_file
{
    public:
        static std::shared_ptr<mmap_file> map_file( const std::string &file_path );
        static std::shared_ptr<mmap_file> map_file( const std::filesystem::path &file_path );

        ~mmap_file();

        uint8_t *base = nullptr;
        size_t len = 0;

    private:
        mmap_file();

        // Opaque type to platform specific mmap implementation which can clean up the view when destructed.
        struct handle;
        std::shared_ptr<handle> mmap_handle;
};

#endif // CATA_SRC_MMAP_FILE_H
