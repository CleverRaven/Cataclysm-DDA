#ifndef MAPSHARING_H_INCLUDED
#define MAPSHARING_H_INCLUDED

#ifdef __linux__
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#endif // __linux__

#include <string>

namespace MAP_SHARING {
extern bool sharing;
extern std::string username;

void setSharing(bool mode);
void setUsername(std::string name);
bool isSharing();
std::string getUsername();

int getLock( char const *lockName );
void releaseLock( int fd, char const *lockName );
}

#endif // MAPSHARING_H_INCLUDED
