#pragma once
#ifndef PLAYER_HELPERS_H
#define PLAYER_HELPERS_H

#include <string>

class npc;
class player;
struct point;

int get_remaining_charges( const std::string &tool_id );
bool player_has_item_of_type( const std::string & );
void clear_character( player & );
void clear_avatar();
void process_activity( player &dummy );

npc &spawn_npc( const point &, const std::string &npc_class );

#endif
