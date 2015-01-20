#include "filesystem.h"
#include "debug.h"

#include <string.h> // for strnlen
#include <cstring>
#include <cstddef>
#include <string>
#include <vector>
#include <deque>
#include <algorithm>
#include <memory>

// FILE I/O
#include <sys/stat.h>

#ifdef _MSC_VER
#   include "wdirent.h"
#   include <direct.h>
#else
#   include <dirent.h>
#   include <unistd.h>
#endif

#if defined(_WIN32) || defined (__WIN32__)
#   include "platform_win.h"
#endif

//--------------------------------------------------------------------------------------------------
// HACK: mingw only issue as of 14/01/2015
// TODO: move elsewhere
//--------------------------------------------------------------------------------------------------
#ifdef __MINGW32__
size_t strnlen(const char *const start, size_t const maxlen)
{
    auto const end = reinterpret_cast<const char*>(memchr(start, '\0', maxlen));
    return (end) ? static_cast<size_t>(end - start) : maxlen;
}
#endif

namespace {

#if (defined _WIN32 || defined __WIN32__)
bool do_mkdir(std::string const& path, int const mode)
{
    (void)mode; //not used on windows
    return mkdir(path.c_str()) == 0;
}
#else
bool do_mkdir(std::string const& path, int const mode)
{
    return mkdir(path.c_str(), mode) == 0;
}
#endif

} //anonymous namespace

bool assure_dir_exist(std::string const &path)
{
    std::unique_ptr<DIR, decltype(&closedir)> dir {opendir(path.c_str()), closedir};
    return dir || do_mkdir(path, 0777);
}

bool file_exist(const std::string &path)
{
    struct stat buffer;
    return ( stat( path.c_str(), &buffer ) == 0 );
}

#if (defined _WIN32 || defined __WIN32__)
bool remove_file(const std::string &path)
{
    return DeleteFile(path.c_str()) != 0;
}
#else
bool remove_file(const std::string &path)
{
    return unlink(path.c_str()) == 0;
}
#endif

#if (defined _WIN32 || defined __WIN32__)
bool rename_file(const std::string &old_path, const std::string &new_path)
{
    // Windows rename function does not override existing targets, so we
    // have to remove the target to make it compatible with the linux rename
    if (file_exist(new_path)) {
        if(!remove_file(new_path)) {
            return false;
        }
    }

    return rename(old_path.c_str(), new_path.c_str()) == 0;
}
#else
bool rename_file(const std::string &old_path, const std::string &new_path)
{
    return rename(old_path.c_str(), new_path.c_str()) == 0;
}
#endif

namespace {

//TODO move elsewhere.
template <typename T, size_t N>
inline size_t sizeof_array(T const (&)[N]) noexcept {
    return N;
}

//--------------------------------------------------------------------------------------------------
// For non-empty path, call function for each file at path.
//--------------------------------------------------------------------------------------------------
template <typename Function>
void for_each_dir_entry(std::string const &path, Function function)
{
    using dir_ptr = std::unique_ptr<DIR, decltype(&closedir)>;

    if (path.empty()) {
        return;
    }

    dir_ptr root {opendir(path.c_str()), closedir};
    if (!root) {
        auto const e_str = strerror(errno);
        DebugLog(D_WARNING, D_MAIN) << "opendir [" << path << "] failed with \"" << e_str << "\".";
        return;
    }

    while (auto const entry = readdir(root.get())) {
        function(*entry);
    }
}

//--------------------------------------------------------------------------------------------------
bool is_directory_stat(std::string const &full_path)
{
    struct stat result;
    if (stat(full_path.c_str(), &result) != 0) {
        auto const e_str = strerror(errno);
        DebugLog(D_WARNING, D_MAIN) << "stat [" << full_path << "] failed with \"" << e_str << "\".";
        return false;
    }

    return S_ISDIR(result.st_mode);
}

//--------------------------------------------------------------------------------------------------
// Returns true if entry is a directory, false otherwise.
//--------------------------------------------------------------------------------------------------
#if defined (__MINGW32__)
bool is_directory(dirent const &entry, std::string const &full_path)
{
    // no dirent::d_type
    (void)entry; //not used for mingw
    return is_directory_stat(full_path);
}
#else
bool is_directory(dirent const &entry, std::string const &full_path)
{
    if (entry.d_type == DT_DIR) {
        return true;
    } else if (entry.d_type != DT_UNKNOWN) {
        return false;
    }

    return is_directory_stat(full_path);
}
#endif

//--------------------------------------------------------------------------------------------------
// Returns true if the name of entry matches "." or "..".
//--------------------------------------------------------------------------------------------------
bool is_special_dir(dirent const &entry)
{
    return !strncmp(entry.d_name, ".",  sizeof(entry.d_name) - 1) ||
           !strncmp(entry.d_name, "..", sizeof(entry.d_name) - 1);
}

//--------------------------------------------------------------------------------------------------
// If at_end is true, returns whether entry's name ends in match.
// Otherwise, returns whether entry's name contains match.
//--------------------------------------------------------------------------------------------------
bool name_contains(dirent const &entry, std::string const &match, bool const at_end)
{
    auto const len_fname = strnlen(entry.d_name, sizeof_array(entry.d_name));
    auto const len_match = match.length();

    if (len_match > len_fname) {
        return false;
    }

    auto const offset = at_end ? (len_fname - len_match) : 0;
    return strstr(entry.d_name + offset, match.c_str()) != 0;
}

//--------------------------------------------------------------------------------------------------
// Return every file at root_path matching predicate.
//
// If root_path is empty, search the current working directory.
// If recurse is true, search breadth-first into the directory hierarchy.
//
// Results are ordered depth-first with directories searched in lexically order. Furthermore,
// regular files at each level are also ordered lexically by file name.
//
// Files ending in ~ are excluded.
//--------------------------------------------------------------------------------------------------
template <typename Predicate>
std::vector<std::string> find_file_if_bfs(std::string const &root_path, bool const recurse,
    Predicate predicate)
{
    std::deque<std::string>  directories {!root_path.empty() ? root_path : "."};
    std::vector<std::string> results;

    while (!directories.empty()) {
        auto const path = std::move(directories.front());
        directories.pop_front();

        auto const n_dirs    = static_cast<std::ptrdiff_t>(directories.size());
        auto const n_results = static_cast<std::ptrdiff_t>(results.size());
        
        for_each_dir_entry(path, [&](dirent const &entry) {
            // exclude special directories.
            if (is_special_dir(entry)) {
                return;
            }

            auto const full_path = path + "/" + entry.d_name;
            
            // don't add files ending in '~'.
            if (full_path.back() == '~') {
                return;
            }

            // add sub directories to recurse if requested
            auto const is_dir = is_directory(entry, full_path);
            if (recurse && is_dir) {
                directories.emplace_back(full_path);
            }

            // check the file
            if (!predicate(entry, is_dir)) {
                return;
            }

            results.emplace_back(full_path);
        });

        // Keep files and directories to recurse ordered consistently
        // by sorting from the old end to the new end.
        std::sort(std::begin(directories) + n_dirs,    std::end(directories));
        std::sort(std::begin(results)     + n_results, std::end(results));
    }

    return results;
}

} //anonymous namespace

//--------------------------------------------------------------------------------------------------
std::vector<std::string> get_files_from_path(std::string const &pattern,
    std::string const &root_path, bool const recurse, bool const match_extension)
{
    return find_file_if_bfs(root_path, recurse, [&](dirent const &entry, bool) {
        return name_contains(entry, pattern, match_extension);
    });
}

//--------------------------------------------------------------------------------------------------
std::vector<std::string> get_directories_with(std::vector<std::string> const &patterns,
    std::string const &root_path, bool const recurse)
{
    if (patterns.empty()) {
        return std::vector<std::string>();
    }

    auto const ext_beg = std::begin(patterns);
    auto const ext_end = std::end(patterns);

    auto files = find_file_if_bfs(root_path, recurse, [&](dirent const &entry, bool) {
        return std::any_of(ext_beg, ext_end, [&](std::string const& ext) {
            return name_contains(entry, ext, true);
        });
    });

    //chop off the file names
    for (auto &file : files) {
        file.erase(file.rfind('/'), std::string::npos);
    }

    //remove resulting duplicates
    files.erase(std::unique(std::begin(files), std::end(files)), std::end(files));

    return files;
}
