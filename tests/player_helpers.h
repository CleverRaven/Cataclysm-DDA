#pragma once
#ifndef CATA_TESTS_PLAYER_HELPERS_H
#define CATA_TESTS_PLAYER_HELPERS_H

#include <iosfwd>
#include <vector>

#include "npc.h"

class item;
class Character;
struct point;

int get_remaining_charges( const std::string &tool_id );
bool player_has_item_of_type( const std::string & );
bool character_has_item_with_var_val( const Character &they, const std::string &var,
                                      const std::string &val );
void clear_character( Character &, bool skip_nutrition = false );
void clear_avatar();
void process_activity( Character &dummy );

npc &spawn_npc( const point &, const std::string &npc_class );

void set_single_trait( Character &dummy, const std::string &trait_name );
void give_and_activate_bionic( Character &, bionic_id const & );

item tool_with_ammo( const std::string &tool, int qty );

void arm_shooter( Character &shooter, const std::string &gun_type,
                  const std::vector<std::string> &mods = {},
                  const std::string &ammo_type = "" );

void equip_shooter( npc &shooter, const std::vector<std::string> &apparel );

#endif // CATA_TESTS_PLAYER_HELPERS_H
