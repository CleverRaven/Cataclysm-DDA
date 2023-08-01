#pragma once
#ifndef CATA_SRC_MAP_TGZ_ARCHIVER_H
#define CATA_SRC_MAP_TGZ_ARCHIVER_H
#include <array>
#include <chrono>
#include <string>
#include <utility>

#include <ghc/fs_std_fwd.hpp>

#include "filesystem.h"
#include "zlib.h"

class tgz_archiver
{
    private:
        static constexpr std::size_t tar_block_size = 512;
        using tar_block_t = std::array<char, tar_block_size>;
        std::string _gen_tar_header( fs::path const &file_name, fs::path const &prefix,
                                     fs::path const &real_path, std::streamsize size );

        gzFile fd = nullptr;
        std::string const output;
        fs::file_time_type const _fsnow;
        std::chrono::system_clock::time_point const _sysnow;

    public:
        explicit tgz_archiver( std::string ofile )
            : output( std::move( ofile ) ), _fsnow( fs::file_time_type::clock::now() ),
              _sysnow( std::chrono::system_clock::now() ) {};
        ~tgz_archiver();

        tgz_archiver( tgz_archiver const & ) = delete;
        tgz_archiver( tgz_archiver && ) = delete;
        tgz_archiver &operator=( tgz_archiver const & ) = delete;
        tgz_archiver &operator=( tgz_archiver && ) = delete;

        bool add_file( fs::path const &real_path, fs::path const &archived_path );
        void finalize();
};

#endif // CATA_SRC_MAP_TGZ_ARCHIVER_H
