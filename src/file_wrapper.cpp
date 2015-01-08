#include "file_wrapper.h"
#include "platform.h"

#include <sys/stat.h>

#if defined(CATA_OS_WINDOWS)
#   include "wdirent.h"
#   include <direct.h>
#else
#   include <dirent.h>
#   include <unistd.h>
#   include <stdio.h>
#endif

bool assure_dir_exist( std::string path )
{
    DIR *dir = opendir( path.c_str() );
    if( dir != NULL ) {
        closedir( dir );
        return true;
    }
#if defined(CATA_OS_WINDOWS)
    return ( mkdir( path.c_str() ) == 0 );
#else
    return ( mkdir( path.c_str(), 0777 ) == 0 );
#endif
}

bool file_exist(const std::string &path)
{
    struct stat buffer;
    return ( stat( path.c_str(), &buffer ) == 0 );
}

bool remove_file(const std::string &path)
{
#if defined(CATA_OS_WINDOWS)
    return DeleteFile(path.c_str()) != 0;
#else
    return unlink(path.c_str()) == 0;
#endif
}

bool rename_file(const std::string &old_path, const std::string &new_path)
{
#if defined(CATA_OS_WINDOWS)
    // Windows rename function does not override existing targets, so we
    // have to remove the target to make it compatible with the linux rename
    if (file_exist(new_path)) {
        if(!remove_file(new_path)) {
            return false;
        }
    }
    return rename(old_path.c_str(), new_path.c_str()) == 0;
#else
    return rename(old_path.c_str(), new_path.c_str()) == 0;
#endif
}
