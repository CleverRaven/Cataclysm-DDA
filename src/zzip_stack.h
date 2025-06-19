#pragma once
#ifndef CATA_SRC_ZZIP_STACK_H
#define CATA_SRC_ZZIP_STACK_H

#include <stdint.h>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "std_hash_fs_path.h"

class zzip;

/**
 * A zzip_stack uses multiple zzips to handle frequent updates to only a small portion of the stored data.
 * Fresh data is always written at the top of the stack. When the top is compacted, some of the contents
 * may get written to a zzip lower in the stack. This may trigger that zzip to compact, which repeats until
 * reaching the bottom of the stack.
 * The top of the stack experiences the most compactions, so keeping only fresh data in it minimizes the
 * size of the freshly compacted zzip (and thus minimizes how much data has to be written each compaction).
 * The basic idea is to keep track of how many compactions a file goes through. If the file has been through
 * a compaction once, and during the next compaction was not updated, then it gets evicted to a lower zzip.
 *
 * The interface is otherwise identical to a zzip, since the only difference is the internal organization.
 */
class zzip_stack
{
        zzip_stack( std::filesystem::path const &path,
                    std::filesystem::path const &dictionary_path ) noexcept;
    public:
        ~zzip_stack() noexcept;

        /**
         * Create a zzip_stack at the given path, which will be a folder containing the zzips.
         * See the documentation in zzip.h for more information about the dictionary parameter.
         */
        static std::shared_ptr<zzip_stack> load( std::filesystem::path const &path,
                std::filesystem::path const &dictionary = {} );

        /**
         * Writes the given file contents under the given file path into the zzip.
         * Returns true on success, false on any error.
         */
        bool add_file( std::filesystem::path const &zzip_relative_path, std::string_view content );

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
         * Create a zzip_stack from a folder of existing files.
         * The files in the zzip are indexed based on their relative path inside the folder.
         * See zzip::load for documentation about dictionaries.
         */
        static std::shared_ptr<zzip_stack> create_from_folder( std::filesystem::path const &path,
                std::filesystem::path const &folder,
                std::filesystem::path const &dictionary = {} );

        static std::shared_ptr<zzip_stack> create_from_folder_with_files( std::filesystem::path const &path,
                std::filesystem::path const &folder,
                std::vector<std::filesystem::path> const &files,
                uint64_t total_file_size,
                std::filesystem::path const &dictionary = {} );
        /**
         * Extract the given zzip_stack's contents into the given folder.
         * The files in the zzip_stack are written into the folder under their relative paths.
         * See zzip::load for documentation about dictionaries.
         */
        static bool extract_to_folder( std::filesystem::path const &path,
                                       std::filesystem::path const &folder,
                                       std::filesystem::path const &dictionary = {} );

    private:
        std::shared_ptr<zzip> &hot() const;
        std::shared_ptr<zzip> &warm() const;
        std::shared_ptr<zzip> &cold() const;

        std::filesystem::path path_;
        std::filesystem::path dictionary_;

        enum class file_temp : int {
            unknown,
            cold,
            warm,
            hot,
        };
        // We use mutable members for lazy loading under const methods.
        mutable std::shared_ptr<zzip> hot_;
        mutable std::shared_ptr<zzip> warm_;
        mutable std::shared_ptr<zzip> cold_;
        mutable std::unordered_map<std::filesystem::path, file_temp, std_fs_path_hash> path_temp_map_;
        file_temp temp_of_file( std::filesystem::path const &zzip_relative_path ) const;

        zzip *zzip_of_temp( file_temp temp ) const;
        std::shared_ptr<zzip> load_temp( file_temp temp ) const;
};

#endif // CATA_SRC_ZZIP_STACK_H
