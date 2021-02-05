#pragma once
#ifndef CATA_SRC_FILESYSTEM_H
#define CATA_SRC_FILESYSTEM_H

#include <string>
#include <vector>

/**
 * Create directory if it does not exist.
 * @return true if directory exists or was successfully created.
 */
bool assure_dir_exist( const std::string &path );
/**
 * Check if directory exists.
 * @return false if directory does not exist or if unable to check.
 */
bool dir_exist( const std::string &path );
/**
 * Check if file exists.
 * @return false if file does not exist or if unable to check.
 */
bool file_exist( const std::string &path );
/**
 * Remove a file. Does not remove directories.
 * @return true on success.
 */
bool remove_file( const std::string &path );
/**
 * Remove an empty directory.
 * @return true on success, false on failure (e.g. directory is not empty).
 */
bool remove_directory( const std::string &path );
/**
 * Rename a file, overwriting the target. Does not overwrite directories.
 * @return true on success, false on failure.
 */
bool rename_file( const std::string &old_path, const std::string &new_path );
/**
 * Check if can write to the given directory (write permission, disk space).
 * @return false if cannot write or if unable to check.
 */
bool can_write_to_dir( const std::string &dir_path );
/**
 * Copy file, overwriting the target. Does not overwrite directories.
 * @return true on success, false on failure.
 */
bool copy_file( const std::string &source_path, const std::string &dest_path );
/** Get process id string. Used for temporary file paths. */
std::string get_pid_string();

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

/**
 *  Replace invalid characters in a string with a default character; can be used to ensure that a file name is compliant with most file systems.
 *  @param file_name Name of the file to check.
 *  @return A string with all invalid characters replaced with the replacement character, if any change was made.
 *  @note  The default replacement character is space (0x20) and the invalid characters are "\\/:?\"<>|".
 */
std::string ensure_valid_file_name( const std::string &file_name );

#endif // CATA_SRC_FILESYSTEM_H
