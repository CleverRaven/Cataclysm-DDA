#ifndef MAPSHARING_H
#define MAPSHARING_H

#ifdef __linux__
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#endif // __linux__
#include <stdlib.h>

#include <string>
#include <set>
#include <map>
#include <fstream>

namespace MAP_SHARING
{
extern bool sharing;
extern std::string username;

extern bool competitive;
extern bool worldmenu;

void setSharing( bool mode );
void setUsername( std::string name );
bool isSharing();
std::string getUsername();

void setCompetitive( bool mode );
bool isCompetitive();

void setWorldmenu( bool mode );
bool isWorldmenu();

extern std::set<std::string> admins;
bool isAdmin();

void setAdmins( std::set<std::string> names );
void addAdmin( std::string name );

extern std::set<std::string> debuggers;
bool isDebugger();

void setDebuggers( std::set<std::string> names );
void addDebugger( std::string name );

void setDefaults();
}

int getLock( char const *lockName );
void releaseLock( int fd, char const *lockName );
extern std::map<std::string, int> lockFiles;
void fopen_exclusive( std::ofstream &fout, const char *filename,
                      std::ios_base::openmode mode = std::ios_base::out );
//std::ofstream fopen_exclusive(const char* filename);
void fclose_exclusive( std::ofstream &fout, const char *filename );

#endif
