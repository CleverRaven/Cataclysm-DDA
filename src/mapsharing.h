#ifndef MAPSHARING_H_INCLUDED
#define MAPSHARING_H_INCLUDED

#include <string>

namespace MAP_SHARING {
extern bool sharing;
extern std::string username;

void setSharing(bool mode);
void setUsername(std::string name);
bool isSharing();
std::string getUsername();
}

#endif // MAPSHARING_H_INCLUDED
