#ifndef FILE_FINDER_H
#define FILE_FINDER_H

#include <string>
#include <vector>

//--------------------------------------------------------------------------------------------------
//! Returns a vector of files or directories matching pattern at root_path.
//!
//! Seaches through the directory tree breadth-first. Directories are searched in lexigraphical
//! order. Matching files within in each directory are also ordered lexigraphically.
//!
//! @param pattern The substring to match.
//! @param root_path The path relative to the current working directory to search; "" means ".".
//! @param recursive_search Whether to recursively search sub directories.
//! @param match_extension If true, match pattern at the end of file names.
//--------------------------------------------------------------------------------------------------
std::vector<std::string> get_files_from_path(std::string const &pattern,
        std::string const &root_path = "", bool recursive_search = false,
        bool match_extension = false);

//--------------------------------------------------------------------------------------------------
//! Returns a vector of directories containing files matching any of patterns.
//!
//! @param patterns A vector or patterns to match.
//! @see get_files_from_path
//--------------------------------------------------------------------------------------------------
std::vector<std::string> get_directories_with(std::vector<std::string> const &patterns,
        std::string const &root_path = "", bool recursive_search = false);

#endif
