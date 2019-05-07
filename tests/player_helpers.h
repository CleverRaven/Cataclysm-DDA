#pragma once
#ifndef PLAYER_HELPERS_H
#define PLAYER_HELPERS_H

#include <string>

class player;

int get_remaining_charges( const std::string &tool_id );
bool player_has_item_of_type( const std::string & );
void clear_player();
void process_activity( player &dummy );

#endif
