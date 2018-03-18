#include "mapsharing.h"

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
void MAP_SHARING::setUsername( std::string name )
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
    if( admins.find( getUsername() ) != admins.end() ) {
        return true;
    }
    return false;
}

void MAP_SHARING::setAdmins( std::set<std::string> names )
{
    MAP_SHARING::admins = names;
}

void MAP_SHARING::addAdmin( std::string name )
{
    MAP_SHARING::admins.insert( name );
    MAP_SHARING::debuggers.insert( name );
}

bool MAP_SHARING::isDebugger()
{
    if( debuggers.find( getUsername() ) != debuggers.end() ) {
        return true;
    }
    return false;
}

void MAP_SHARING::setDebuggers( std::set<std::string> names )
{
    MAP_SHARING::debuggers = names;
}

void MAP_SHARING::addDebugger( std::string name )
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

#ifndef __linux__ // make non-Linux operating systems happy

int getLock( char const * )
{
    return 0;
}

void releaseLock( int, char const * )
{
    // Nothing to do.
}

#else

int getLock( char const *lockName )
{
    mode_t m = umask( 0 );
    int fd = open( lockName, O_RDWR | O_CREAT, 0666 );
    umask( m );
    if( fd >= 0 && flock( fd, LOCK_EX | LOCK_NB ) < 0 ) {
        close( fd );
        fd = -1;
    }
    return fd;
}

void releaseLock( int fd, char const *lockName )
{
    if( fd < 0 ) {
        return;
    }
    remove( lockName );
    close( fd );
}

#endif // __linux__

std::map<std::string, int> lockFiles;

void fopen_exclusive( std::ofstream &fout, const char *filename,
                      std::ios_base::openmode mode )  //TODO: put this in an ofstream_exclusive class?
{
    std::string lockfile = std::string( filename ) + ".lock";
    lockFiles[lockfile] = getLock( lockfile.c_str() );
    if( lockFiles[lockfile] != -1 ) {
        fout.open( filename, mode );
    }
}
/*
std::ofstream fopen_exclusive(const char* filename) {
    std::string lockfile = std::string(filename)+".lock";
    std::ofstream fout;
    lockFiles[lockfile] = getLock(lockfile);
    if(lockFiles[lockfile] != -1) {
        fout.open(filename, std::fstream::ios_base::out);
    }
    return fout;
} */

void fclose_exclusive( std::ofstream &fout, const char *filename )
{
    std::string lockFile = std::string( filename ) + ".lock";
    fout.close();
    releaseLock( lockFiles[lockFile], lockFile.c_str() );
    lockFiles[lockFile] = -1;
}
