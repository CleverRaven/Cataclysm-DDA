#include "file_wrapper.h"

#include <sys/stat.h>
#ifdef _MSC_VER
#include "wdirent.h"
#include <direct.h>
#else
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#endif
#if (defined _WIN32 || defined __WIN32__)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <tchar.h>
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
