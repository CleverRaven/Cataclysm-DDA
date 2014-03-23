#include "mapsharing.h"

void MAP_SHARING::setSharing(bool mode) {MAP_SHARING::sharing = mode;}
void MAP_SHARING::setUsername(std::string name) {MAP_SHARING::username = name;}

bool MAP_SHARING::isSharing() {return MAP_SHARING::sharing;}
std::string MAP_SHARING::getUsername() {return MAP_SHARING::username;}

