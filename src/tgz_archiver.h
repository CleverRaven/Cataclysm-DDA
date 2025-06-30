#pragma once
#ifndef CATA_SRC_MAP_TGZ_ARCHIVER_H
#define CATA_SRC_MAP_TGZ_ARCHIVER_H
#include <array>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <iosfwd>
#include <string>
#include <utility>

#include "zlib.h"

class tgz_archiver
{
    private:
        static constexpr std::size_t tar_block_size = 512;
        using tar_block_t = std::array<char, tar_block_size>;
        std::string _gen_tar_header( std::filesystem::path const &file_name,
                                     std::filesystem::path const &prefix,
                                     std::filesystem::path const &real_path, std::streamsize size );

        gzFile fd = nullptr;
        std::string const output;
        std::filesystem::file_time_type const _fsnow;
        std::chrono::system_clock::time_point const _sysnow;

    public:
        explicit tgz_archiver( std::string ofile )
            : output( std::move( ofile ) ), _fsnow( std::filesystem::file_time_type::clock::now() ),
              _sysnow( std::chrono::system_clock::now() ) {};
        ~tgz_archiver();

        tgz_archiver( tgz_archiver const & ) = delete;
        tgz_archiver( tgz_archiver && ) = delete;
        tgz_archiver &operator=( tgz_archiver const & ) = delete;
        tgz_archiver &operator=( tgz_archiver && ) = delete;

        bool add_file( std::filesystem::path const &real_path, std::filesystem::path const &archived_path );
        void finalize();
};

#endif // CATA_SRC_MAP_TGZ_ARCHIVER_H
