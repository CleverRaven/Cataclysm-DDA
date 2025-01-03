#pragma once
#ifndef CATA_SRC_ZZIP_H
#define CATA_SRC_ZZIP_H

#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include <ghc/fs_std_fwd.hpp>

#include "flexbuffer_json.h"
#include "mmap_file.h"

// A 'zstandard zip'. A basic implementation of an archive consistenting of
// independently decompressable entries.
// The archive is implemented as concatenated zstd frames with a flexbuffer
// footer describing the contents.
class zzip
{
        zzip( fs::path path, std::shared_ptr<mmap_file> file, JsonObject footer );

    public:
        ~zzip() noexcept;

        static std::shared_ptr<zzip> load( const fs::path &path, const fs::path &dictionary = {} );

        bool has_file( const fs::path &zzip_relative_path ) const;
        size_t get_file_size( const fs::path &zzip_relative_path ) const;
        std::vector<std::byte> get_file( const fs::path &zzip_relative_path ) const;
        size_t get_file_to( const fs::path &zzip_relative_path, std::byte *dest, size_t dest_len ) const;

        bool add_file( const fs::path &zzip_relative_path, std::string_view content );

        void compact( double bloat_factor = 1.0 );

        static std::shared_ptr<zzip> create_from_folder( const fs::path &path, const fs::path &folder,
                const fs::path &dictionary = {} );
        static bool extract_to_folder( const fs::path &path, const fs::path &folder,
                                       const fs::path &dictionary = {} );

        struct compressed_entry;
    private:
        JsonObject copy_footer() const;
        size_t ensure_capacity_for( size_t bytes );
        size_t write_file_at( std::string_view filename, std::string_view content, size_t offset );

        bool update_footer( JsonObject const &original_footer, size_t content_end,
                            const std::vector<compressed_entry> &entries, bool shrink_to_fit = false );

        void recover_lost_footer();

        fs::path path_;
        std::shared_ptr<mmap_file> file_;
        JsonObject footer_;

        void *file_base_plus( size_t offset ) const;
        size_t file_capacity_at( size_t offset ) const;

        struct context;
        std::unique_ptr<context> ctx_;
};

#endif // CATA_SRC_ZZIP_H
