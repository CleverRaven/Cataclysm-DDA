#include "mapsharing.h"

void MAP_SHARING::setSharing(bool mode) {MAP_SHARING::sharing = mode;}
void MAP_SHARING::setUsername(std::string name) {MAP_SHARING::username = name;}

bool MAP_SHARING::isSharing() {return MAP_SHARING::sharing;}
std::string MAP_SHARING::getUsername() {return MAP_SHARING::username;}

int MAP_SHARING::getLock( char const *lockName ) {
    #ifdef __linux__ // make non-linux operating systems happy
    mode_t m = umask( 0 );
    int fd = open( lockName, O_RDWR|O_CREAT, 0666 );
    umask( m );
    if( fd >= 0 && flock( fd, LOCK_EX | LOCK_NB ) < 0 )
    {
        close( fd );
        fd = -1;
    }
    return fd;
    #endif // __linux__
    return -1;
}

void MAP_SHARING::releaseLock( int fd, char const *lockName ) {
    #ifdef __linux__ // same as above
    if( fd < 0 )
        return;
    remove( lockName );
    close( fd );
    #endif // __linux__
}
