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

bool is_special_dir(dirent const &entry)
{
    return !strncmp(entry.d_name, ".",  sizeof(entry.d_name) - 1) ||
           !strncmp(entry.d_name, "..", sizeof(entry.d_name) - 1);
}

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

std::vector<std::string> file_finder::get_files_from_path(std::string const extension,
        std::string const root_path, bool const recursive_search, bool const match_extension)
{
    std::vector<std::string> files;
    std::deque<std::string>  directories {!root_path.empty() ? root_path : "."};
 
    while (!directories.empty()) {
        auto const path = std::move(directories.front());
        directories.pop_front();

        // old end positions
        auto const n_files = files.size();
        auto const n_dirs  = directories.size();

        for_each_dir_entry(path, [&](dirent const &entry) {
            if (is_special_dir(entry)) {
                return;
            }

            if (recursive_search && is_directory(entry)) {
                directories.emplace_back(path + "/" + entry.d_name);
                return;
            }

            if (!matches_extension(entry, extension, match_extension)) {
                return;
            }
            
            files.emplace_back(path + "/" + entry.d_name);   
            if (files.back().back() == '~') {
                files.pop_back();
            }
        });

        // Keep files and directories to recurse ordered consistently
        // by sorting from the old end to the new end.
        std::sort(std::begin(files) + n_files, std::end(files));
        std::sort(std::begin(directories) + n_dirs, std::end(directories));
    }

    return files;
}

std::vector<std::string> file_finder::get_directories_with( std::vector<std::string> extensions,
        std::string root_path, bool recursive_search )
{
    std::vector<std::string> found;
    const int num_extensions = extensions.size();
    // Test for empty root path.
    if( root_path.empty() ) {
        root_path = ".";
    }
    std::stack<std::string> directories, tempstack;
    directories.push( root_path );
    std::string path = "";

    while( !directories.empty() ) {

        path = directories.top();
        directories.pop();
        DIR *root = opendir( path.c_str() );

        if( root ) {
            struct dirent *root_file;
            struct stat _buff;
            DIR *subdir;
            bool foundit = false;
            while( (root_file = readdir(root)) ) {
                // Check to see if it is a folder!
                if( stat(root_file->d_name, &_buff) != 0x4 ) {
                    // Ignore '.' and '..' folder names, which are current and parent folder
                    // relative paths.
                    if( strcmp(root_file->d_name, ".") != 0 &&
                        strcmp(root_file->d_name, "..") != 0 ) {
                        std::string subpath = path + "/" + root_file->d_name;

                        if( recursive_search ) {
                            subdir = opendir( subpath.c_str() );
                            if( subdir ) {
                                tempstack.push( subpath );
                                closedir( subdir );
                            }
                        }
                    }
                }
                // check to see if it is a file with the appropriate extension
                std::string tmp = root_file->d_name;
                for( int i = 0; i < num_extensions && foundit == false; i++ ) {
                    if( tmp.find(extensions[i].c_str()) != std::string::npos ) {
                        found.push_back( path );
                        foundit = true;
                    }
                }
            }
            closedir( root );
        }
        // Directories are added to tempstack in A->Z order, which makes them pop from Z->A.
        // This Makes sure that directories are searched in the proper order and that
        // the final output is in the proper order.
        while( !tempstack.empty() ) {
            directories.push( tempstack.top() );
            tempstack.pop();
        }
    }
    return found;
}
