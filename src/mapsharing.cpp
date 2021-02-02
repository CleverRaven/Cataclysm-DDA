#include "mapsharing.h"

#include <cstdlib>
#include <stdexcept>

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
