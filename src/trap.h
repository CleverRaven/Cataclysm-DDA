#pragma once
#ifndef TRAP_H
#define TRAP_H

#include <functional>
#include <vector>

#include "color.h"
#include "int_id.h"
#include "string_id.h"
#include "units.h"

class Creature;
class item;
class player;
class map;
struct trap;
struct tripoint;
class JsonObject;
using trap_id = int_id<trap>;
using trap_str_id = string_id<trap>;

namespace trapfunc
{
// creature is the creature that triggered the trap,
// p is the point where the trap is (not where the creature is)
// creature can be NULL.
void none( Creature *, const tripoint & );
void bubble( Creature *creature, const tripoint &p );
void cot( Creature *creature, const tripoint &p );
void beartrap( Creature *creature, const tripoint &p );
void snare_light( Creature *creature, const tripoint &p );
void snare_heavy( Creature *creature, const tripoint &p );
void board( Creature *creature, const tripoint &p );
void caltrops( Creature *creature, const tripoint &p );
void tripwire( Creature *creature, const tripoint &p );
void crossbow( Creature *creature, const tripoint &p );
void shotgun( Creature *creature, const tripoint &p );
void blade( Creature *creature, const tripoint &p );
void landmine( Creature *creature, const tripoint &p );
void telepad( Creature *creature, const tripoint &p );
void goo( Creature *creature, const tripoint &p );
void dissector( Creature *creature, const tripoint &p );
void sinkhole( Creature *creature, const tripoint &p );
void pit( Creature *creature, const tripoint &p );
void pit_spikes( Creature *creature, const tripoint &p );
void pit_glass( Creature *creature, const tripoint &p );
void lava( Creature *creature, const tripoint &p );
void portal( Creature *creature, const tripoint &p );
void ledge( Creature *creature, const tripoint &p );
void boobytrap( Creature *creature, const tripoint &p );
void temple_flood( Creature *creature, const tripoint &p );
void temple_toggle( Creature *creature, const tripoint &p );
void glow( Creature *creature, const tripoint &p );
void hum( Creature *creature, const tripoint &p );
void shadow( Creature *creature, const tripoint &p );
void drain( Creature *creature, const tripoint &p );
void snake( Creature *creature, const tripoint &p );
}

using trap_function = std::function<void( Creature *, const tripoint & )>;

struct trap {
        using itype_id = std::string;
        trap_str_id id;
        trap_id loadid;

        bool was_loaded = false;

        long sym;
        nc_color color;
    private:
        int visibility; // 1 to ??, affects detection
        int avoidance;  // 0 to ??, affects avoidance
        int difficulty; // 0 to ??, difficulty of assembly & disassembly
        bool benign = false;
        trap_function act;
        std::string name_;
        /**
         * If an item with this weight or more is thrown onto the trap, it triggers.
         */
        units::mass trigger_weight = units::mass( -1, units::mass::unit_type{} );
        int funnel_radius_mm;
        std::vector<itype_id> components; // For disassembly?
    public:
        std::string name() const;
        /**
         * How easy it is to spot the trap. Smaller values means it's easier to spot.
         */
        int get_visibility() const {
            return visibility;
        }
        /**
         * Whether triggering the trap can be avoid (if greater than 0) and if so, this is
         * compared to dodge skill (with some adjustments). Smaller values means it's easier
         * to dodge.
         */
        int get_avoidance() const {
            return avoidance;
        }
        /**
         * This is used when disarming the trap. A value of 0 means disarming will always work
         * (e.g. for funnels), a values of 99 means it can not be disarmed at all. Smaller values
         * makes it easier to disarm the trap.
         */
        int get_difficulty() const {
            return difficulty;
        }
        /**
         * If true, this is not really a trap and there won't be any safety queries before stepping
         * onto it (e.g. for funnels).
         */
        bool is_benign() const {
            return benign;
        }
        /** Player has not yet seen the trap and returns the variable chance, at this moment,
         of whether the trap is seen or not. */
        bool detect_trap( const tripoint &pos, const player &p ) const;
        /**
         * Can player/npc p see this kind of trap, either by their memory (they known there is
         * the trap) or by the visibility of the trap (the trap is not hidden at all)?
         */
        bool can_see( const tripoint &pos, const player &p ) const;
        /**
         * Trigger trap effects.
         * @param creature The creature that triggered the trap, it does not necessarily have to
         * be on the place of the trap (traps can be triggered from adjacent, e.g. when disarming
         * them). This can also be a null pointer if the trap has been triggered by some thrown
         * item (which must have the @ref trigger_weight).
         * @param pos The location of the trap in the main map.
         */
        void trigger( const tripoint &pos, Creature *creature ) const;
        /**
         * If the given item is throw onto the trap, does it trigger the trap?
         */
        bool triggered_by_item( const item &itm ) const;
        /**
         * Called when a trap at the given point in the map has been disarmed.
         * It should spawn trap items (if any) and remove the trap from the map via
         * @ref map::remove_trap.
         */
        void on_disarmed( map &m, const tripoint &p ) const;
        /**
         * Whether this kind of trap actually occupies a 3x3 area. Currently only blade traps
         * do so.
         */
        bool is_3x3_trap() const;
        /**
         * Whether this is the null-traps, aka no trap at all.
         */
        bool is_null() const;
        /**
         * Loads this specific trap.
         */
        void load( JsonObject &jo, const std::string &src );

        /*@{*/
        /**
         * @name Funnels
         *
         * Traps can act as funnels, for this they need a @ref trap::funnel_radius_mm > 0.
         * Funnels are usual not hidden at all (@ref trap::visibility == 0), are @ref trap::benign and can
         * be picked up easily (@ref trap::difficulty == 0).
         * The funnel filling is handled in weather.cpp. is_funnel is used the check whether the
         * funnel specific code should be run for this trap.
         */
        bool is_funnel() const;
        double funnel_turns_per_charge( double rain_depth_mm_per_hour ) const;
        /**
         * Returns all trap objects that are actually funnels (is_funnel returns true for all
         * of them).
         */
        static const std::vector<const trap *> &get_funnels();
        /*@}*/

        /*@{*/
        /**
         * @name Initialization
         *
         * Those functions are used by the @ref DynamicDataLoader, see there.
         */
        /**
         * Loads the trap and adds it to the trapmap, and the traplist.
         * @throw std::string if the json is invalid as usual.
         */
        static void load_trap( JsonObject &jo, const std::string &src );
        /**
         * Releases the loaded trap objects in trapmap and traplist.
         */
        static void reset();
        /**
         * Stores the actual @ref loadid of the loaded traps in the global tr_* variables.
         * It also sets the trap ids of the terrain types that have build-in traps.
         * Must be called after all traps have been loaded.
         */
        static void finalize();
        /**
         * Checks internal consistency (reference to other things like item ids etc.)
         */
        static void check_consistency();
        /*@}*/
        static size_t count();
};

const trap_function &trap_function_from_string( const std::string &function_name );

extern trap_id
tr_null,
tr_bubblewrap,
tr_cot,
tr_funnel,
tr_metal_funnel,
tr_makeshift_funnel,
tr_leather_funnel,
tr_rollmat,
tr_fur_rollmat,
tr_beartrap,
tr_beartrap_buried,
tr_nailboard,
tr_caltrops,
tr_tripwire,
tr_crossbow,
tr_shotgun_2,
tr_shotgun_1,
tr_engine,
tr_blade,
tr_landmine,
tr_landmine_buried,
tr_telepad,
tr_goo,
tr_dissector,
tr_sinkhole,
tr_pit,
tr_spike_pit,
tr_glass_pit,
tr_lava,
tr_portal,
tr_ledge,
tr_boobytrap,
tr_temple_flood,
tr_temple_toggle,
tr_glow,
tr_hum,
tr_shadow,
tr_drain,
tr_snake;

#endif
