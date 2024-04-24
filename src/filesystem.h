#pragma once
#ifndef CATA_SRC_FILESYSTEM_H
#define CATA_SRC_FILESYSTEM_H

#include <fstream>
#include <string> // IWYU pragma: keep
#include <vector>

#include <ghc/fs_std_fwd.hpp>

#include "catacharset.h"
#include "compatibility.h"
#include "path_info.h"

bool assure_dir_exist( const std::string &path );
bool dir_exist( const std::string &path );
bool file_exist( const std::string &path );
// Remove a file, does not remove folders,
// returns true on success
bool remove_file( const std::string &path );
bool remove_directory( const std::string &path );
// Rename a file, overriding the target!
bool rename_file( const std::string &old_path, const std::string &new_path );

std::string abs_path( const std::string &path );

std::string read_entire_file( const std::string &path );

// Overloads of the above that take fs::path directly.
bool assure_dir_exist( const fs::path &path );
bool assure_dir_exist( const cata_path &path );
bool dir_exist( const fs::path &path );
bool file_exist( const fs::path &path );
bool file_exist( const cata_path &path );

// Force 'path' to be a normalized directory
std::string as_norm_dir( const std::string &path );
std::string as_norm_dir( const fs::path &path );

// Remove a file, does not remove folders,
// returns true on success
bool remove_file( const fs::path &path );
bool remove_directory( const fs::path &path );
// Rename a file, overriding the target!
bool rename_file( const fs::path &old_path, const fs::path &new_path );

fs::path abs_path( const fs::path &path );

std::string read_entire_file( const fs::path &path );

namespace cata_files
{
const char *eol();
} // namespace cata_files

//--------------------------------------------------------------------------------------------------
/**
 * Returns a vector of files or directories matching pattern at @p root_path.
 *
 * Searches through the directory tree breadth-first. Directories are searched in lexical
 * order. Matching files within in each directory are also ordered lexically.
 *
 * @param pattern The sub-string to match.
 * @param root_path The path relative to the current working directory to search; empty means ".".
 * @param recursive_search Whether to recursively search sub directories.
 * @param match_extension If true, match pattern at the end of file names. Otherwise, match anywhere
 *                        in the file name.
 */
std::vector<std::string> get_files_from_path( const std::string &pattern,
        const std::string &root_path = "", bool recursive_search = false,
        bool match_extension = false );
std::vector<cata_path> get_files_from_path( const std::string &pattern,
        const cata_path &root_path, bool recursive_search = false,
        bool match_extension = false );

//--------------------------------------------------------------------------------------------------
/**
 * Returns a vector of directories which contain files matching any of @p patterns.
 *
 * @param patterns A vector or patterns to match.
 * @see get_files_from_path
 */
std::vector<std::string> get_directories_with( const std::vector<std::string> &patterns,
        const std::string &root_path = "", bool recursive_search = false );

std::vector<std::string> get_directories_with( const std::string &pattern,
        const std::string &root_path = "", bool recursive_search = false );

std::vector<cata_path> get_directories_with( const std::vector<std::string> &patterns,
        const cata_path &root_path = {}, bool recursive_search = false );

std::vector<cata_path> get_directories_with( const std::string &pattern,
        const cata_path &root_path = {}, bool recursive_search = false );

bool copy_file( const std::string &source_path, const std::string &dest_path );
bool copy_file( const cata_path &source_path, const cata_path &dest_path );

/**
 *  Replace invalid characters in a string with a default character; can be used to ensure that a file name is compliant with most file systems.
 *  @param file_name Name of the file to check.
 *  @return A string with all invalid characters replaced with the replacement character, if any change was made.
 *  @note  The default replacement character is space (0x20) and the invalid characters are "\\/:?\"<>|".
 */
std::string ensure_valid_file_name( const std::string &file_name );

#if defined(_WIN32)
// On Windows, it checks for some validity of the path. See .cpp
bool is_lexically_valid( const fs::path & );
#else
constexpr bool is_lexically_valid( const fs::path & )
{
    return true;
}
#endif

#endif // CATA_SRC_FILESYSTEM_H
