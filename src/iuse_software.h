#pragma once
#ifndef IUSE_SOFTWARE_H
#define IUSE_SOFTWARE_H

#include <string>
#include <map>
bool play_videogame( const std::string &function_name,
                     std::map<std::string, std::string> &game_data,
                     int &score );

#endif
