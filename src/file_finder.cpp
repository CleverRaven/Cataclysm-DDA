#include "file_finder.h"
#include <cstring>  // for strcmp
#include <stack>    // for stack (obviously)
#include <queue>
#include <algorithm>
#include <memory>

// FILE I/O
#include <sys/stat.h>
#ifdef _MSC_VER
#include "wdirent.h"
#include <direct.h>
#else
#include <dirent.h>
#endif

namespace {

using dir_ptr = std::unique_ptr<DIR, decltype(&closedir)>;


} //anonymous namespace

/**
    Searches the supplied root directory for the extension provided. Can recursively search
    downward through the directory tree to get all files. It will then return a vector of
    file paths that fulfill the search requirements.
    USAGE:
    Extension should be preceded by the '.' character to denote it as an extension,
    if it is not then it will return all files and folder names that contain the extension string.
    root_path can point to any folder relative to the execution directory.
        Searching from the execution directory -- Supply "." as the root_path
        Searching from the parent directory of the execution directory -- Supply ".." as the root_path
        Searching child directories within the execution directory --
            Supply "<directory path>" or "./<directory path>", both will provide correct results.
    recursive_search will:
        if true, continue searching all child directories from the supplied root_path
        if false, search only the supplied root_path, and stop when the directory contents
        have been worked through.
*/

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
template <typename Function>
void for_each_dir_entry(std::string const &path, Function function)
{
    dir_ptr root {opendir(!path.empty() ? path.c_str() : "."), closedir};
    if (!root) {
        auto const e = errno;
        return;
    }
    
    while (auto const entry = readdir(root.get())) {
        if (!entry) {
            return;
        }
    
        function(*entry);
    }
}

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
bool is_directory(dirent const &entry)
{
    if (entry.d_type == DT_DIR) {
        return true;
    } else if (entry.d_type != DT_UNKNOWN) {
        return false;
    }

    struct stat result;
    if (stat(entry.d_name, &result) != 0) {
        auto const e = errno;
        return false;
    }

    return S_ISDIR(result.st_mode);
}

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
bool is_special_dir(dirent const &entry)
{
    return !strncmp(entry.d_name, ".",  sizeof(entry.d_name) - 1) ||
           !strncmp(entry.d_name, "..", sizeof(entry.d_name) - 1);
}

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
bool matches_extension(dirent const &entry, std::string const &extension,
    bool const match_extension)
{
    auto const len_fname = entry.d_namlen;
    auto const len_ext   = extension.length();

    if (len_ext > len_fname) {
        return false;
    }

    auto const start = entry.d_name + (match_extension ? len_fname - len_ext : 0);
    return strstr(start, extension.c_str()) != 0;
}

template <typename Predicate>
std::vector<std::string> find_file_if_bfs(std::string const &root_path, bool const recurse,
    Predicate predicate)
{
    std::deque<std::string>  directories {!root_path.empty() ? root_path : "."};
    std::vector<std::string> results;

    while (!directories.empty()) {
        auto const path = std::move(directories.front());
        directories.pop_front();

        auto const n_dirs    = directories.size();
        auto const n_results = results.size();
        
        for_each_dir_entry(path, [&](dirent const &entry) {
            // exclude special directories.
            if (is_special_dir(entry)) {
                return;
            }

            // add sub directories to recurse if requested
            auto const is_dir = is_directory(entry);
            if (recurse && is_dir) {
                directories.emplace_back(path + "/" + entry.d_name);
            }

            // check the file
            if (!predicate(entry, is_dir)) {
                return;
            }

            // don't add files ending in '~'.
            results.emplace_back(path + "/" + entry.d_name);   
            if (results.back().back() == '~') {
                results.pop_back();
            }
        });

        // Keep files and directories to recurse ordered consistently
        // by sorting from the old end to the new end.
        std::sort(std::begin(directories) + n_dirs,    std::end(directories));
        std::sort(std::begin(results)     + n_results, std::end(results));
    }

    return results;
}

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
std::vector<std::string> get_files_from_path(std::string const &extension,
    std::string const &root_path, bool const recurse, bool const match_extension)
{
    return find_file_if_bfs(root_path, recurse, [&](dirent const &entry, bool const is_dir) {
        return matches_extension(entry, extension, match_extension);
    });
}

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------

std::vector<std::string> get_directories_with(std::vector<std::string> const &extensions,
    std::string const &root_path, bool const recurse)
{
    auto files = find_file_if_bfs(root_path, recurse, [&](dirent const &entry, bool const is_dir) {
        for (auto const &ext : extensions) {
            if (matches_extension(entry, ext, true)) {
                return true;
            }
        }

        return false;
    });

    for (auto &file : files) {
        file.erase(file.rfind('/'), std::string::npos);
    }

    return files;
}
