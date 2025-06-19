#pragma once
#ifndef CATA_SRC_ZZIP_H
#define CATA_SRC_ZZIP_H

#include <stdint.h>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "flexbuffer_json.h"

class mmap_file;
struct std_fs_path_hash;
/**
 * A 'zstandard zip'. A basic implementation of an archive consisting of independently decompressable entries.
 * The archive is implemented as concatenated zstd frames with a flexbuffer footer describing the contents.
 * The first frame contains a length and checksum for the footer, to help detect corruption.
 * Limited metadata about each file is encoded as skippable zstd frames before each compressed frame.
 * The root of a flexbuffer is the end of the buffer, so putting it at the end of the file means we don't have
 * to store any metadata about where it is or how long it is.
 *
 * +-------------------+-------------------+
 * | footer len & hash | 12345, 0x12345678 |
 * +-------------------+-------------------+
 * |     file name     | `cows/steak.json` |
 * +-------------------+-------------------+
 * |  compressed hash  |   `0xDEADBEEF`    |
 * +-------------------+-------------------+
 * |  compressed file  | zstd binary data  |
 * +-------------------+-------------------+
 * |       <repeated file entries>         |
 * +-------------------+-------------------+
 * |    empty bytes    |                   |
 * +-------------------+-------------------+
 * | flexbuffer footer | flexbuffer data   |
 * +-------------------+-------------------+
 *
 * The zstd cli can decompress files consistenting of concatenated frames, so with some clever scripting
 * one can recover the content without needing to run the game or use the zzip api.
 *
 * zzips are append-only. When new entries are written or old entries replaced, the new data is appended after
 * the last compressed frame, and a new footer written to the end of the file. If a zzip is corrupted, data can
 * be recovered by linearly scanning the zzip and recovering each file entry from the filename, hash, and compressed
 * frame. The hash is used to verify the integrity of the compressed frame to check for truncation.
 */
class zzip
{
        zzip( std::filesystem::path path, std::shared_ptr<mmap_file> file, JsonObject footer );

    public:
        ~zzip() noexcept;

        /**
         * Create a zzip at the given path, using the given dictionary to de/compress files in the zzip.
         * The same dictionary must be used every time the zzip is loaded.
         * A dictionary can either be arbitrary reference data or a dictionary created with the zstd cli.
         * See https://github.com/facebook/zstd/blob/dev/programs/README.md#dictionary-builder-in-command-line-interface
         * for more details.
         */
        static std::shared_ptr<zzip> load( std::filesystem::path const &path,
                                           std::filesystem::path const &dictionary = {} );

        /**
         * Writes the given file contents under the given file path into the zzip.
         * Returns true on success, false on any error.
         */
        bool add_file( std::filesystem::path const &zzip_relative_path, std::string_view content );

        /**
         * Directly copies a compressed entry from one zzip to another. Both zzips
         * must have been opened using the same dictionary. The relative path is
         * preserved.
         */
        bool copy_files( std::vector<std::filesystem::path> const &zzip_relative_paths,
                         std::shared_ptr<zzip> const &from );

        /**
         * Returns true if the zzip contains the given path. Paths are checked through exact string
         * matches. Normalizing paths, and using generic u8 paths, is recommended.
         */
        bool has_file( std::filesystem::path const &zzip_relative_path ) const;

        /**
         * Returns the exact decompressed size of the given file, if it exists in the zzip.
         * Returns 0 otherwise.
         */
        size_t get_file_size( std::filesystem::path const &zzip_relative_path ) const;

        /**
         * Returns the end of the live content in the zzip, i.e. space usage excluding padding
         * and footer.
         */
        size_t get_content_size() const;

        /**
         * Returns the actual file size of the entire zzip including stale entries and metadata.
         */
        size_t get_zzip_size() const;

        /**
         * Returns the compressed file size of the entry including metadata.
         */
        size_t get_entry_size( std::filesystem::path const &zzip_relative_path ) const;

        /**
         * Returns the path to this zzip.
         */
        std::filesystem::path const &get_path() const;

        /**
         * Returns a vector of filesystem paths of the contained entries.
         */
        std::vector<std::filesystem::path> get_entries() const;

        /**
         * Extracts the given file and returns it in a fresh std::vector<std::byte>.
         * Returns an empty vector if it does not exist.
         */
        std::vector<std::byte> get_file( std::filesystem::path const &zzip_relative_path ) const;

        /**
         * Extracts the given file into the given destination.
         * Returns 0 if the file does not exist.
         * Returns 0 if the destination does not have enough space for the file.
         * Returns the size of the decompressed file on success.
         */
        size_t get_file_to( std::filesystem::path const &zzip_relative_path,
                            std::byte *dest,
                            size_t dest_len ) const;

        /**
         * Removes the files from the zzip.
         * Under the hood, this just removes the file entry from the footer. The last
         * written version still exists in the archive. In the event of footer corruption,
         * the deleted files will be recovered on a scan of the file.
         */
        bool delete_files( std::unordered_set<std::filesystem::path, std_fs_path_hash> const
                           &zzip_relative_paths );

        /**
         * Possibly shrink the zzip by eliminating orphaned data or padding bytes.
         * If bloat_factor passed, only shrinks the zzip if the underlying file is
         * larger than optimal than the given ratio.
         * If bloat_factor is 0, forces a compaction even with no expected savings.
         * Returns true if compaction occurred.
         */
        bool compact( double bloat_factor = 1.0 );

        /**
         * Create a zzip from a folder of existing files.
         * The files in the zzip are indexed based on their relative path inside the folder.
         * See zzip::load for documentation about dictionaries.
         */
        static std::shared_ptr<zzip> create_from_folder( std::filesystem::path const &path,
                std::filesystem::path const &folder,
                std::filesystem::path const &dictionary = {} );

        static std::shared_ptr<zzip> create_from_folder_with_files( std::filesystem::path const &path,
                std::filesystem::path const &folder,
                const std::vector<std::filesystem::path> &files,
                uint64_t total_file_size,
                std::filesystem::path const &dictionary = {} );
        /**
         * Extract the given zzip's contents into the given folder.
         * The files in the zzip are written into the folder under their relative paths.
         * See zzip::load for documentation about dictionaries.
         */
        static bool extract_to_folder( std::filesystem::path const &path,
                                       std::filesystem::path const &folder,
                                       std::filesystem::path const &dictionary = {} );

        struct compressed_entry;
    private:
        JsonObject copy_footer() const;
        size_t ensure_capacity_for( size_t bytes );
        size_t write_file_at( std::string_view filename, std::string_view content, size_t offset );

        bool update_footer( JsonObject const &original_footer, size_t content_end,
                            const std::vector<compressed_entry> &entries, bool shrink_to_fit = false );

        bool rewrite_footer();

        std::filesystem::path path_;
        std::shared_ptr<mmap_file> file_;
        JsonObject footer_;

        void *file_base_plus( size_t offset ) const;
        size_t file_capacity_at( size_t offset ) const;

        struct context;
        std::unique_ptr<context> ctx_;
};

#endif // CATA_SRC_ZZIP_H
