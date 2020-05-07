#ifndef CATA_SRC_RANGED_H
#define CATA_SRC_RANGED_H

#include <vector>

#include "memory_fast.h"
#include "type_id.h"
#include "units.h"

class JsonIn;
class JsonOut;
class item;
class player;
class spell;
class turret_data;
class vehicle;
struct itype;
struct tripoint;
struct vehicle_part;
template<typename T> struct enum_traits;

/**
 * Specifies weapon source for aiming across turns and
 * (de-)serialization of targeting_data
 */
enum weapon_source_enum {
    /** Invalid weapon source */
    WEAPON_SOURCE_INVALID,
    /** Firing wielded weapon */
    WEAPON_SOURCE_WIELDED,
    /** Firing fake gun provided by a bionic */
    WEAPON_SOURCE_BIONIC,
    /** Firing fake gun provided by a mutation */
    WEAPON_SOURCE_MUTATION,
    NUM_WEAPON_SOURCES
};

template <>
struct enum_traits<weapon_source_enum> {
    static constexpr weapon_source_enum last = NUM_WEAPON_SOURCES;
};

/** Stores data for aiming the player's weapon across turns */
struct targeting_data {
    weapon_source_enum weapon_source;

    /** Cached fake weapon provided by bionic/mutation */
    shared_ptr_fast<item> cached_fake_weapon;

    /** Bionic power cost per shot */
    units::energy bp_cost_per_shot;

    bool is_valid() const;

    /** Use wielded gun */
    static targeting_data use_wielded();

    /** Use fake gun provided by a bionic */
    static targeting_data use_bionic( const item &fake_gun, const units::energy &cost_per_shot );

    /** Use fake gun provided by a mutation */
    static targeting_data use_mutation( const item &fake_gun );

    // Since only avatar uses targeting_data,
    // (de-)serialization is implemented in savegame_json.cpp
    // near avatar (de-)serialization
    void serialize( JsonOut &json ) const;
    void deserialize( JsonIn &jsin );
};

namespace target_handler
{
// Trajectory to target. Empty if selection was aborted or player ran out of moves
using trajectory = std::vector<tripoint>;

/**
 * Firing ranged weapon. This mode allows spending moves on aiming.
 * 'reload_requested' is set to 'true' if user aborted aiming to reload the gun/change ammo
 */
trajectory mode_fire( player &pc, item &weapon, bool &reload_requested );

/** Throwing item */
trajectory mode_throw( player &pc, item &relevant, bool blind_throwing );

/** Reach attacking */
trajectory mode_reach( player &pc, item &weapon );

/** Manually firing vehicle turret */
trajectory mode_turret_manual( player &pc, turret_data &turret );

/** Selecting target for turrets (when using vehicle controls) */
trajectory mode_turrets( player &pc, vehicle &veh, const std::vector<vehicle_part *> &turrets );

/** Casting a spell */
trajectory mode_spell( player &pc, spell &casting, bool no_fail, bool no_mana );
trajectory mode_spell( player &pc, spell_id sp, bool no_fail, bool no_mana );
} // namespace target_handler

int range_with_even_chance_of_good_hit( int dispersion );

#endif // CATA_SRC_RANGED_H
