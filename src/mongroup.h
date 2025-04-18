#pragma once
#ifndef CATA_SRC_MONGROUP_H
#define CATA_SRC_MONGROUP_H

#include <map>
#include <set>
#include <string>
#include <vector>

#include "calendar.h"
#include "coordinates.h"
#include "enums.h"
#include "io_tags.h"
#include "mapgen.h"
#include "monster.h"
#include "point.h"
#include "type_id.h"

class JsonObject;
class JsonOut;
// from overmap.h
class overmap;
struct MonsterGroupEntry;
template <typename T> struct enum_traits;

using FreqDef = std::vector<MonsterGroupEntry>;
using FreqDef_iter = FreqDef::iterator;

struct MonsterGroupEntry {
    mtype_id mtype;
    mongroup_id group;
    int frequency;
    int cost_multiplier;
    int pack_minimum;
    int pack_maximum;
    spawn_data data;
    std::vector<std::string> conditions;
    time_duration starts;
    time_duration ends;
    holiday event;
    bool lasts_forever() const {
        return ends <= 0_turns;
    }
    bool is_group() const {
        return group != mongroup_id();
    }

    MonsterGroupEntry( const mtype_id &new_id, int new_freq, int new_cost, int new_pack_min,
                       int new_pack_max, const spawn_data &new_data, const time_duration &new_starts,
                       const time_duration &new_ends, holiday new_event )
        : mtype( new_id )
        , frequency( new_freq )
        , cost_multiplier( new_cost )
        , pack_minimum( new_pack_min )
        , pack_maximum( new_pack_max )
        , data( new_data )
        , starts( new_starts )
        , ends( new_ends )
        , event( new_event ) {
    }

    MonsterGroupEntry( const mongroup_id &new_id, int new_freq, int new_cost, int new_pack_min,
                       int new_pack_max, const spawn_data &new_data, const time_duration &new_starts,
                       const time_duration &new_ends, holiday new_event )
        : group( new_id )
        , frequency( new_freq )
        , cost_multiplier( new_cost )
        , pack_minimum( new_pack_min )
        , pack_maximum( new_pack_max )
        , data( new_data )
        , starts( new_starts )
        , ends( new_ends )
        , event( new_event ) {
    }
};

struct MonsterGroupResult {
    mtype_id id;
    int pack_size;
    spawn_data data;

    MonsterGroupResult() : id( mtype_id::NULL_ID() ), pack_size( 0 ) {
    }

    MonsterGroupResult( const mtype_id &new_id, int new_pack_size, const spawn_data &new_data )
        : id( new_id ), pack_size( new_pack_size ), data( new_data ) {
    }
};

/**
 * MonsterGroup is a theoretical distribution of monsters to spawn, as opposed to a mongroup which is an instance of group of monsters to be spawned together.
*/
struct MonsterGroup {
    mongroup_id id;
    mtype_id defaultMonster;
    FreqDef monsters;
    bool IsMonsterInGroup( const mtype_id &id ) const;
    bool is_animal = false;

    /*replaces this group after a period of time when exploring an unexplored portion of the map. Used for monster evolution. */
    bool replace_monster_group = false;
    /* The monster group to replace this if replace_monster_group == true. */
    mongroup_id new_monster_group;
    /* Time until replacement if replace_monster_group == true. */
    time_duration monster_group_time = 0_turns;

    /** Encapsulated by mongroup::is_safe(), but currently not used anywhere.*/
    bool is_safe = false;
    int freq_total = 0; // max number to roll for spawns (non-event)
    std::map<holiday, int> event_freq; // total freq for each event
    // Get the total frequency of entries that are valid for the specified event.
    // This includes entries that have an event of "none". By default, use the current holiday.
    int event_adjusted_freq_total( holiday event = holiday::num_holiday ) const;
};

/**
 * A mongroup is a group of monsters that are spawned together. There are two uses for mongroups:
 * 1. spawning new monsters.
 * 2. unload monsters out of sight of the player into hordes and spawn them back in when the player is close enough.
 */
struct mongroup {
    /** The type of the mongroup, which is only used to spawn new monsters and not for hordes. */
    mongroup_id type;
    /** The monsters vector will be ignored if the vector is empty.
     *  Otherwise it will keep track of the individual monsters that
     *  are contained in this horde, and the population property will
     *  be ignored instead.
     */
    std::vector<monster> monsters;
    // Note: position is not saved as such in the json
    // Instead, a vector of positions is saved for

    /** position of the mongroup in absolute submap coordinates*/
    tripoint_abs_sm abs_pos;

    /**
     * Number of monsters in the group. If the monsters vector is not empty, this will be ignored.
     */
    unsigned int population = 1;

    /** location the horde is interested in. Do not set this directly, use set_target instead. */
    point_abs_sm target;
    /** abs target for nemesis hordes. */
    point_abs_sm nemesis_target; //
    /** interest to target in percents. Used to determine movement speed(overmap::move_hordes), or reaction to signal sources(overmap::signal_hordes), etc.*/
    int interest = 0;

    /** If true, overmap::process_mongroups decreases the population by a fixed ratio every time the function is called. */
    bool dying = false;
    /** If false, this group is not a horde, and instead is likely an attempt to spawn new monsters through map::spawn_monsters_submap from overmap_buffer. */
    bool horde = false;

    enum class horde_behaviour : short {
        none,
        city, ///< Try to stick around cities and return to them whenever possible
        roam, ///< Roam around the map randomly
        nemesis, ///< Follow the avatar specifically
        last
    };
    horde_behaviour behaviour = horde_behaviour::none;

    mongroup( const mongroup_id &ptype, const tripoint_abs_sm &ppos,
              unsigned int ppop )
        : type( ptype )
        , abs_pos( ppos )
        , population( ppop )
        , target( abs_pos.xy() ) {
    }
    mongroup( const std::string &ptype, const tripoint_abs_sm &ppos,
              unsigned int ppop, point_abs_sm ptarget, int pint, bool pdie, bool phorde ) :
        type( ptype ), abs_pos( ppos ), population( ppop ), target( ptarget ),
        interest( pint ), dying( pdie ), horde( phorde ) { }
    mongroup() = default;
    bool is_safe() const;
    bool empty() const;
    void clear();
    tripoint_om_sm rel_pos() const {
        return project_remain<coords::om>( abs_pos ).remainder_tripoint;
    }
    void set_target( const point_abs_sm &p ) {
        target = p;
    }
    void set_nemesis_target( const point_abs_sm &p ) {
        nemesis_target = p;
    }
    void wander( const overmap & );

    /**
     * This function increases the interest of the group, but ensures that it does not exceed 100 percent.
     *
     */
    void inc_interest( int inc ) {
        interest += inc;
        if( interest > 100 ) {
            interest = 100;
        }
    }

    /**
     * This function decreases the interest of the group, but ensures that it does not fall below 15 percent.
     *
     */
    void dec_interest( int dec ) {
        interest -= dec;
        if( interest < 15 ) {
            interest = 15;
        }
    }
    /**
     * This function sets the interest of the group, but ensures that it does not fall below 15 percent or exceed 100 percent.
     *
     */
    void set_interest( int set ) {
        if( set < 15 ) {
            set = 15;
        }
        if( set > 100 ) {
            set = 100;
        }
        interest = set;
    }
    float avg_speed() const;

    template<typename Archive>
    void io( Archive & );
    using archive_type_tag = io::object_archive_tag;

    void deserialize( const JsonObject &jo );
    void deserialize_legacy( const JsonObject &jo );
    void serialize( JsonOut &json ) const;
};

template<>
struct enum_traits<mongroup::horde_behaviour> {
    static constexpr mongroup::horde_behaviour last = mongroup::horde_behaviour::last;
};

class MonsterGroupManager
{
    public:
        static void LoadMonsterGroup( const JsonObject &jo );
        static void LoadMonsterBlacklist( const JsonObject &jo );
        static void LoadMonsterWhitelist( const JsonObject &jo );
        static void FinalizeMonsterGroups();
        static std::vector<MonsterGroupResult> GetResultFromGroup( const mongroup_id &group,
                int *quantity = nullptr, bool *mon_found = nullptr, bool is_recursive = false,
                bool *returned_default = nullptr, bool use_pack_size = false );
        static bool IsMonsterInGroup( const mongroup_id &group, const mtype_id &monster );
        static bool isValidMonsterGroup( const mongroup_id &group );
        static const mongroup_id &Monster2Group( const mtype_id &monster );
        static std::vector<mtype_id> GetMonstersFromGroup( const mongroup_id &group, bool from_subgroups );
        static const MonsterGroup &GetMonsterGroup( const mongroup_id &group );
        /**
         * Gets a random monster, weighted by frequency.
         * Ignores cost multiplier.
         */
        static const mtype_id &GetRandomMonsterFromGroup( const mongroup_id &group );

        static void check_group_definitions();

        static void ClearMonsterGroups();

        static bool monster_is_blacklisted( const mtype_id &m );

        static bool is_animal( const mongroup_id &group );

        // Public getter for private monsterGroupMap, do not use if you don't know what you're doing.
        static std::map<mongroup_id, MonsterGroup> &Get_all_Groups();

    private:
        static std::map<mongroup_id, MonsterGroup> monsterGroupMap;
        using t_string_set = std::set<std::string>;
        static t_string_set monster_blacklist;
        static t_string_set monster_whitelist;
        static t_string_set monster_categories_blacklist;
        static t_string_set monster_categories_whitelist;
        static t_string_set monster_species_blacklist;
        static t_string_set monster_species_whitelist;

};

#endif // CATA_SRC_MONGROUP_H
