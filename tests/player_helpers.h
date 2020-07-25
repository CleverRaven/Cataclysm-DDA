#pragma once
#ifndef CATA_TESTS_PLAYER_HELPERS_H
#define CATA_TESTS_PLAYER_HELPERS_H

#include <string>
#include <vector>

#include "type_id.h"

class npc;
class player;
struct point;

int get_remaining_charges( const std::string &tool_id );
bool player_has_item_of_type( const std::string & );
void clear_character( player &, bool debug_storage = true );
void clear_avatar();
void process_activity( player &dummy );

npc &spawn_npc( const point &, const std::string &npc_class );
void give_and_activate_bionic( player &, bionic_id const & );

void arm_character( player &shooter, const std::string &gun_type,
                    const std::vector<std::string> &mods = {},
                    const std::string &ammo_type = "" );

#endif // CATA_TESTS_PLAYER_HELPERS_H
