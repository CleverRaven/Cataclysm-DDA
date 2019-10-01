#include "mapsharing.h"

#include "cata_utility.h"
#include "filesystem.h"
#include "platform_win.h"

#include <cstdlib>
#include <stdexcept>

#if defined(__linux__)
#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#endif // __linux__

bool MAP_SHARING::sharing;
bool MAP_SHARING::competitive;
bool MAP_SHARING::worldmenu;
std::string MAP_SHARING::username;
std::set<std::string> MAP_SHARING::admins;
std::set<std::string> MAP_SHARING::debuggers;

void MAP_SHARING::setSharing( bool mode )
{
    MAP_SHARING::sharing = mode;
}
void MAP_SHARING::setUsername( const std::string &name )
{
    MAP_SHARING::username = name;
}

bool MAP_SHARING::isSharing()
{
    return MAP_SHARING::sharing;
}
std::string MAP_SHARING::getUsername()
{
    return MAP_SHARING::username;
}

void MAP_SHARING::setCompetitive( bool mode )
{
    MAP_SHARING::competitive = mode;
}
bool MAP_SHARING::isCompetitive()
{
    return MAP_SHARING::competitive;
}

void MAP_SHARING::setWorldmenu( bool mode )
{
    MAP_SHARING::worldmenu = mode;
}
bool MAP_SHARING::isWorldmenu()
{
    return MAP_SHARING::worldmenu;
}

bool MAP_SHARING::isAdmin()
{
    return admins.find( getUsername() ) != admins.end();
}

void MAP_SHARING::setAdmins( const std::set<std::string> &names )
{
    MAP_SHARING::admins = names;
}

void MAP_SHARING::addAdmin( const std::string &name )
{
    MAP_SHARING::admins.insert( name );
    MAP_SHARING::debuggers.insert( name );
}

bool MAP_SHARING::isDebugger()
{
    return debuggers.find( getUsername() ) != debuggers.end();
}

void MAP_SHARING::setDebuggers( const std::set<std::string> &names )
{
    MAP_SHARING::debuggers = names;
}

void MAP_SHARING::addDebugger( const std::string &name )
{
    MAP_SHARING::debuggers.insert( name );
}

void MAP_SHARING::setDefaults()
{
    MAP_SHARING::setSharing( false );
    MAP_SHARING::setCompetitive( false );
    MAP_SHARING::setWorldmenu( true );
    MAP_SHARING::setUsername( "" );
    if( MAP_SHARING::getUsername().empty() && getenv( "USER" ) ) {
        MAP_SHARING::setUsername( getenv( "USER" ) );
    }
    MAP_SHARING::addAdmin( "admin" );
}

void ofstream_wrapper::open( const std::ios::openmode mode )
{
    // Create a *unique* temporary path. No other running program should
    // use this path. If the file exists, it must be of a *former* program
    // instance and can savely be deleted.
#if defined(__linux__)
    temp_path = path + "." + std::to_string( getpid() ) + ".temp";

#elif defined(_WIN32)
    temp_path = path + "." + std::to_string( GetCurrentProcessId() ) + ".temp";

#else
    // @todo exclusive I/O for other systems
    temp_path = path + ".temp";

#endif

    if( file_exist( temp_path ) ) {
        remove_file( temp_path );
    }

    file_stream.open( temp_path, mode );
    if( !file_stream.is_open() ) {
        throw std::runtime_error( "opening file failed" );
    }
}

void ofstream_wrapper::close()
{
    if( !file_stream.is_open() ) {
        return;
    }

    if( file_stream.fail() ) {
        // Remove the incomplete or otherwise faulty file (if possible).
        // Failures from it are ignored as we can't really do anything about them.
        remove_file( temp_path );
        throw std::runtime_error( "writing to file failed" );
    }
    file_stream.close();
    if( !rename_file( temp_path, path ) ) {
        // Leave the temp path, so the user can move it if possible.
        throw std::runtime_error( "moving temporary file \"" + temp_path + "\" failed" );
    }
}
