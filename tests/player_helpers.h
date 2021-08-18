#pragma once
#ifndef CATA_TESTS_PLAYER_HELPERS_H
#define CATA_TESTS_PLAYER_HELPERS_H

#include <iosfwd>

#include "npc.h"

class item;
class player;
struct point;

int get_remaining_charges( const std::string &tool_id );
bool player_has_item_of_type( const std::string & );
void clear_character( player & );
void clear_avatar();
void process_activity( player &dummy );

npc &spawn_npc( const point &, const std::string &npc_class );
void give_and_activate_bionic( player &, bionic_id const & );

item tool_with_ammo( const std::string &tool, int qty );

void arm_shooter( npc &shooter, const std::string &gun_type,
                  const std::vector<std::string> &mods = {},
                  const std::string &ammo_type = "" );

#endif // CATA_TESTS_PLAYER_HELPERS_H
