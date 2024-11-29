#pragma once
#ifndef CATA_SRC_IUSE_SOFTWARE_H
#define CATA_SRC_IUSE_SOFTWARE_H

#include <map>
#include <string>

bool play_videogame( const std::string &function_name,
                     std::map<std::string, std::string> &game_data,
                     int &score );

#endif // CATA_SRC_IUSE_SOFTWARE_H
