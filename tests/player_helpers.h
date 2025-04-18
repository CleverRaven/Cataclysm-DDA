#pragma once
#ifndef CATA_TESTS_PLAYER_HELPERS_H
#define CATA_TESTS_PLAYER_HELPERS_H

#include <string>
#include <vector>

#include "coords_fwd.h"
#include "type_id.h"

class Character;
class item;
class npc;

int get_remaining_charges( const itype_id &tool_id );
bool player_has_item_of_type( const itype_id &id );
bool character_has_item_with_var_val( const Character &they, const std::string &var,
                                      const std::string &val );
void clear_character( Character &, bool skip_nutrition = false );
void clear_avatar();
void process_activity( Character &dummy, bool pass_time = false );

npc &spawn_npc( const point_bub_ms &, const std::string &npc_class );

void set_single_trait( Character &dummy, const std::string &trait_name );
void give_and_activate_bionic( Character &, bionic_id const & );

item tool_with_ammo( const itype_id &tool, int qty );

void arm_shooter( Character &shooter, const itype_id &gun_type,
                  const std::vector<itype_id> &mods = {},
                  const itype_id &ammo_type = itype_id::NULL_ID() );

void equip_shooter( npc &shooter, const std::vector<itype_id> &apparel );

#endif // CATA_TESTS_PLAYER_HELPERS_H
