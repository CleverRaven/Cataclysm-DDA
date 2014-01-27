#include "file_wrapper.h"

#include <sys/stat.h>
#ifdef _MSC_VER
#include "wdirent.h"
#include <direct.h>
#else
#include <dirent.h>
#endif

bool assure_dir_exist( std::string path ) {
    DIR *dir = opendir( path.c_str() );
    if( dir != NULL ) {
        closedir( dir );
        return true;
    }
#if (defined _WIN32 || defined __WIN32__)
    return ( mkdir( path.c_str() ) == 0 );
#else
    return ( mkdir( path.c_str(), 0777 ) == 0 );
#endif
}

