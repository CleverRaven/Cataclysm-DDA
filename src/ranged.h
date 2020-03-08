#ifndef RANGED_H
#define RANGED_H

#include <vector>
#include "type_id.h"

class item;
class player;
class avatar;
class spell;
struct itype;
struct tripoint;

template<typename T> struct enum_traits;

class JsonIn;
class JsonOut;

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
enum weapon_source {
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
struct enum_traits<weapon_source> {
    static constexpr weapon_source last = weapon_source::NUM_WEAPON_SOURCES;
};

/** Stores data for aiming the player's weapon across turns */
struct targeting_data {
    weapon_source weapon_source;

    /** Cached fake weapon provided by bionic/mutation */
    std::shared_ptr<item> cached_fake_weapon;

    /** Cached range */
    int range;
    /** Relevant ammo */
    const itype *ammo;
    /** Bionic power cost to fire */
    int bp_cost;

    bool is_valid() const;

    /** Use wielded gun */
    static targeting_data use_wielded( const avatar &you );

    /** Use fake gun provided by a bionic */
    static targeting_data use_bionic( const item &fake_gun, int bp_cost );

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
         *  @param pc The player doing the targeting
         *  @param mode targeting mode, which affects UI display among other things.
         *  @param relevant active item, if any (for instance, a weapon to be aimed).
         *  @param range the maximum distance to which we're allowed to draw a target.
         *  @param ammo effective ammo data (derived from @param relevant if unspecified).
         */
        std::vector<tripoint> target_ui( player &pc, target_mode mode,
                                         item *relevant, int range,
                                         const itype *ammo = nullptr );
        // magic version of target_ui
        std::vector<tripoint> target_ui( spell_id sp, bool no_fail = false,
                                         bool no_mana = false );
        std::vector<tripoint> target_ui( spell &casting, bool no_fail = false,
                                         bool no_mana = false );
};

int range_with_even_chance_of_good_hit( int dispersion );

#endif // RANGED_H
