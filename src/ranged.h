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

enum target_mode : int {
    TARGET_MODE_FIRE,
    TARGET_MODE_THROW,
    TARGET_MODE_TURRET,
    TARGET_MODE_TURRET_MANUAL,
    TARGET_MODE_REACH,
    TARGET_MODE_THROW_BLIND,
    TARGET_MODE_SPELL
};

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

class target_handler
{
        // TODO: alias return type of target_ui
    public:
        /**
         *  Prompts for target and returns trajectory to it.
         *  TODO: pass arguments via constructor(s) and add methods for getting names and button labels,
         *        switching ammo & firing modes, drawing - stuff like that
         *  @param pc The player doing the targeting
         *  @param mode targeting mode, which affects UI display among other things.
         *  @param relevant active item, if any (for instance, a weapon to be aimed).
         *  @param range the maximum distance to which we're allowed to draw a target.
         *  @param ammo effective ammo data (derived from @param relevant if unspecified).
         *  @param turret turret being fired (relevant for TARGET_MODE_TURRET_MANUAL)
         *  @param veh vehicle that turrets belong to (relevant for TARGET_MODE_TURRET)
         *  @param vturrets vehicle turrets being aimed (relevant for TARGET_MODE_TURRET)
         */
        std::vector<tripoint> target_ui( player &pc, target_mode mode,
                                         item *relevant, int range,
                                         const itype *ammo = nullptr,
                                         turret_data *turret = nullptr,
                                         vehicle *veh = nullptr,
                                         const std::vector<vehicle_part *> &vturrets = std::vector<vehicle_part *>()
                                       );
        // magic version of target_ui
        std::vector<tripoint> target_ui( spell_id sp, bool no_fail = false,
                                         bool no_mana = false );
        std::vector<tripoint> target_ui( spell &casting, bool no_fail = false,
                                         bool no_mana = false );
};

int range_with_even_chance_of_good_hit( int dispersion );

#endif // CATA_SRC_RANGED_H
