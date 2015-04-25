#ifndef TRAP_H
#define TRAP_H

#include "color.h"
#include "itype.h"
#include "json.h"
#include <string>

class Creature;
class item;

typedef int trap_id;
/** map trap ids to index into <B>traps</B> */
extern std::map<std::string, int> trapmap;

struct trap;

struct trapfunc {
    // creature is the creature that triggered the trap,
    // p is the point where the trap is (not where the creature is)
    // creature can be NULL.
    void none           ( Creature *, int, int ) { };
    void bubble         ( Creature *creature, int x, int y );
    void cot            ( Creature *creature, int x, int y );
    void beartrap       ( Creature *creature, int x, int y );
    void snare_light    ( Creature *creature, int x, int y );
    void snare_heavy    ( Creature *creature, int x, int y );
    void board          ( Creature *creature, int x, int y );
    void caltrops       ( Creature *creature, int x, int y );
    void tripwire       ( Creature *creature, int x, int y );
    void crossbow       ( Creature *creature, int x, int y );
    void shotgun        ( Creature *creature, int x, int y );
    void blade          ( Creature *creature, int x, int y );
    void landmine       ( Creature *creature, int x, int y );
    void telepad        ( Creature *creature, int x, int y );
    void goo            ( Creature *creature, int x, int y );
    void dissector      ( Creature *creature, int x, int y );
    void sinkhole       ( Creature *creature, int x, int y );
    void pit            ( Creature *creature, int x, int y );
    void pit_spikes     ( Creature *creature, int x, int y );
    void pit_glass      ( Creature *creature, int x, int y );
    void lava           ( Creature *creature, int x, int y );
    void portal         ( Creature *creature, int x, int y );
    void ledge          ( Creature *creature, int x, int y );
    void boobytrap      ( Creature *creature, int x, int y );
    void temple_flood   ( Creature *creature, int x, int y );
    void temple_toggle  ( Creature *creature, int x, int y );
    void glow           ( Creature *creature, int x, int y );
    void hum            ( Creature *creature, int x, int y );
    void shadow         ( Creature *creature, int x, int y );
    void drain          ( Creature *creature, int x, int y );
    void snake          ( Creature *creature, int x, int y );
};

typedef void (trapfunc::*trap_function)( Creature *, int x, int y );

struct trap {
        std::string id;
        int loadid;
        long sym;
        nc_color color;
        std::string name;
    private:
        int visibility; // 1 to ??, affects detection
        int avoidance;  // 0 to ??, affects avoidance
        int difficulty; // 0 to ??, difficulty of assembly & disassembly
        bool benign = false;
        trap_function act;
        /**
         * If an item with this weight or more is thrown onto the trap, it triggers.
         */
        int trigger_weight;
        int funnel_radius_mm;
        std::vector<itype_id> components; // For disassembly?
    public:
        /**
         * How easy it is to spot the trap. Smaller values means it's easier to spot.
         */
        int get_visibility() const
        {
            return visibility;
        }
        /**
         * Whether triggering the trap can be avoid (if greater than 0) and if so, this is
         * compared to dodge skill (with some adjustments). Smaller values means it's easier
         * to dodge.
         */
        int get_avoidance() const
        {
            return avoidance;
        }
        /**
         * This is used when disarming the trap. A value of 0 means disarming will always work
         * (e.g. for funnels), a values of 99 means it can not be disarmed at all. Smaller values
         * makes it easier to disarm the trap.
         */
        int get_difficulty() const
        {
            return difficulty;
        }
        /**
         * If true, this is not really a trap and there won't be any safety queries before stepping
         * onto it (e.g. for funnels).
         */
        bool is_benign() const
        {
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
         * Called when a trap at the given point in the main map has been disarmed.
         * It should spawn trap items (if any) and remove the trap from the map via
         * @ref map::remove_trap.
         */
        void on_disarmed( const tripoint &pos ) const;
        /**
         * Whether this kind of trap actually occupies a 3x3 area. Currently only blade traps
         * do so.
         */
        bool is_3x3_trap() const;
        /**
         * Whether this is the null-traps, aka no trap at all.
         */
        bool is_null() const;

        /*@{*/
        /**
         * @name Funnels
         *
         * Traps can act as funnels, for this they need a @ref funnel_radius_mm > 0.
         * Funnels are usual not hidden at all (@ref visibility == 0), are @ref benign and can
         * be picked up easily (@ref difficulty == 0).
         * The funnel filling is handled in weather.cpp. is_funnel is used the check whether the
         * funnel specific code should be run for this trap.
         */
        bool is_funnel() const;
        double funnel_turns_per_charge( double rain_depth_mm_per_hour ) const;
        /*@}*/

        /*@{*/
        /**
         * @name Initialization
         *
         * Those functions are used by the @ref DynamicDataLoader, see there.
         */
        /**
         * Loads the trap and adds it to the @ref trapmap, and the @ref traplist.
         * @throw std::string if the json is invalid as usual.
         */
        static void load( JsonObject &jo );
        /**
         * Releases the loaded trap objects in @ref trapmap and @ref traplist.
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
};

/** list of all trap types */
extern std::vector<trap *> traplist;

trap_function trap_function_from_string(std::string function_name);

extern trap_id
tr_null,
tr_bubblewrap,
tr_cot,
tr_brazier,
tr_funnel,
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
tr_light_snare,
tr_heavy_snare,
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
