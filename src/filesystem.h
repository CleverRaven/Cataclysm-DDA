#pragma once
#ifndef CATA_SRC_FILESYSTEM_H
#define CATA_SRC_FILESYSTEM_H

#include <fstream>
#include <string> // IWYU pragma: keep
#include <vector>

#include <ghc/fs_std_fwd.hpp>

#include "catacharset.h"
#include "compatibility.h"

bool assure_dir_exist( const std::string &path );
bool dir_exist( const std::string &path );
bool file_exist( const std::string &path );
// Remove a file, does not remove folders,
// returns true on success
bool remove_file( const std::string &path );
bool remove_directory( const std::string &path );
// Rename a file, overriding the target!
bool rename_file( const std::string &old_path, const std::string &new_path );

std::string read_entire_file( const std::string &path );

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

bool copy_file( const std::string &source_path, const std::string &dest_path );

/**
 *  Replace invalid characters in a string with a default character; can be used to ensure that a file name is compliant with most file systems.
 *  @param file_name Name of the file to check.
 *  @return A string with all invalid characters replaced with the replacement character, if any change was made.
 *  @note  The default replacement character is space (0x20) and the invalid characters are "\\/:?\"<>|".
 */
std::string ensure_valid_file_name( const std::string &file_name );

namespace cata
// A slightly modified version of `ghc::filesystem` fstreams to handle native file encoding on windows.
// See `third-party/ghc/filesystem.hpp` for the original version.
{
namespace _details
{
#if defined(_WIN32) && !defined(__GLIBCXX__)
std::wstring path_to_native( const fs::path &p );
#else
std::string path_to_native( const fs::path &p );
#endif
} // namespace _details

template<class charT, class traits = std::char_traits<charT>>
class basic_ifstream : public std::basic_ifstream<charT, traits>
{
    public:
        basic_ifstream() = default;
        explicit basic_ifstream( const fs::path &p, std::ios_base::openmode mode = std::ios_base::in )
            : std::basic_ifstream<charT, traits>( _details::path_to_native( p ).c_str(), mode ) {
        }
        void open( const fs::path &p, std::ios_base::openmode mode = std::ios_base::in ) {
            std::basic_ifstream<charT, traits>::open( _details::path_to_native( p ).c_str(), mode );
        }
        basic_ifstream( const basic_ifstream & ) = delete;
        const basic_ifstream &operator=( const basic_ifstream & ) = delete;
        // NOLINTNEXTLINE(performance-noexcept-move-constructor)
        basic_ifstream( basic_ifstream && ) noexcept( basic_ifstream_is_noexcept ) = default;
        // NOLINTNEXTLINE(performance-noexcept-move-constructor)
        basic_ifstream &operator=( basic_ifstream && ) noexcept( basic_ifstream_is_noexcept ) = default;
        ~basic_ifstream() override = default;
};

template<class charT, class traits = std::char_traits<charT>>
class basic_ofstream : public std::basic_ofstream<charT, traits>
{
    public:
        basic_ofstream() = default;
        explicit basic_ofstream( const fs::path &p, std::ios_base::openmode mode = std::ios_base::out )
            : std::basic_ofstream<charT, traits>( _details::path_to_native( p ).c_str(), mode ) {
        }
        void open( const fs::path &p, std::ios_base::openmode mode = std::ios_base::out ) {
            std::basic_ofstream<charT, traits>::open( _details::path_to_native( p ).c_str(), mode );
        }
        basic_ofstream( const basic_ofstream & ) = delete;
        const basic_ofstream &operator=( const basic_ofstream & ) = delete;
        // NOLINTNEXTLINE(performance-noexcept-move-constructor)
        basic_ofstream( basic_ofstream && ) noexcept( basic_ofstream_is_noexcept ) = default;
        // NOLINTNEXTLINE(performance-noexcept-move-constructor)
        basic_ofstream &operator=( basic_ofstream && ) noexcept( basic_ofstream_is_noexcept ) = default;
        ~basic_ofstream() override = default;
};

using ifstream = basic_ifstream<char>;
using ofstream = basic_ofstream<char>;
} // namespace cata

#endif // CATA_SRC_FILESYSTEM_H
