#pragma once
#ifndef CATA_SRC_TRAP_H
#define CATA_SRC_TRAP_H

#include <cstddef>
#include <functional>
#include <iosfwd>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "color.h"
#include "flat_set.h"
#include "magic.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"

class Character;
class Creature;
class JsonObject;
class item;
class map;
struct tripoint;

namespace trapfunc
{
// p is the point where the trap is (not where the creature is)
// creature is the creature that triggered the trap,
// item is the item that triggered the trap,
// creature and item can be nullptr.
bool none( const tripoint &, Creature *, item * );
bool bubble( const tripoint &p, Creature *c, item *i );
bool glass( const tripoint &p, Creature *c, item *i );
bool cot( const tripoint &p, Creature *c, item *i );
bool beartrap( const tripoint &p, Creature *c, item *i );
bool snare_light( const tripoint &p, Creature *c, item *i );
bool snare_heavy( const tripoint &p, Creature *c, item *i );
bool board( const tripoint &p, Creature *c, item *i );
bool caltrops( const tripoint &p, Creature *c, item *i );
bool caltrops_glass( const tripoint &p, Creature *c, item *i );
bool tripwire( const tripoint &p, Creature *c, item *i );
bool crossbow( const tripoint &p, Creature *c, item *i );
bool shotgun( const tripoint &p, Creature *c, item *i );
bool blade( const tripoint &p, Creature *c, item *i );
bool landmine( const tripoint &p, Creature *c, item *i );
bool telepad( const tripoint &p, Creature *c, item *i );
bool goo( const tripoint &p, Creature *c, item *i );
bool dissector( const tripoint &p, Creature *c, item *i );
bool sinkhole( const tripoint &p, Creature *c, item *i );
bool pit( const tripoint &p, Creature *c, item *i );
bool pit_spikes( const tripoint &p, Creature *c, item *i );
bool pit_glass( const tripoint &p, Creature *c, item *i );
bool lava( const tripoint &p, Creature *c, item *i );
bool portal( const tripoint &p, Creature *c, item *i );
bool ledge( const tripoint &p, Creature *c, item *i );
bool boobytrap( const tripoint &p, Creature *c, item *i );
bool temple_flood( const tripoint &p, Creature *c, item *i );
bool temple_toggle( const tripoint &p, Creature *c, item *i );
bool glow( const tripoint &p, Creature *c, item *i );
bool hum( const tripoint &p, Creature *c, item *i );
bool shadow( const tripoint &p, Creature *c, item *i );
bool map_regen( const tripoint &p, Creature *c, item *i );
bool drain( const tripoint &p, Creature *c, item *i );
bool snake( const tripoint &p, Creature *c, item *i );
bool cast_spell( const tripoint &p, Creature *critter, item * );
} // namespace trapfunc

struct vehicle_handle_trap_data {
    bool remove_trap = false;
    bool do_explosion = false;
    bool is_falling = false;
    int chance = 100;
    int damage = 0;
    int shrapnel = 0;
    int sound_volume = 0;
    translation sound;
    std::string sound_type;
    std::string sound_variant;
    // the double represents the count or chance to spawn.
    std::vector<std::pair<itype_id, double>> spawn_items;
    trap_str_id set_trap = trap_str_id::NULL_ID();
};

using trap_function = std::function<bool( const tripoint &, Creature *, item * )>;

/**
 * Some traps aren't actually traps in the usual sense of the word. We use traps to implement
 * funnels (the main map keeps a list of traps and we iterate over that list during rain
 * in order to fill the containers with water).
 * Use @ref is_benign to check for that kind of "non-dangerous" traps. Traps that are not benign
 * are considered dangerous.
 *
 * Some traps are always invisible. They are never revealed as such to the player.
 * They can still be triggered. Use @ref is_always_invisible to check for that.
 *
 * Traps names can be empty. This usually applies to always invisible traps.
 *
 * Some traps are always revealed to the player (e.g. funnels). Other traps can be spotted when
 * the player is close (also needs perception stat).
 *
 * Use @ref map::can_see_trap_at or @ref trap::can_see to check whether a creature knows about a
 * given trap. Monsters / NPCs should base their behavior on that information and not on a simple
 * check for any trap being there (e.g. don't use `map::tr_at(...).is_null()` - that would reveal
 * the existence of the trap).
 */
struct trap {
        trap_str_id id;
        std::vector<std::pair<trap_str_id, mod_id>> src;
        trap_id loadid;

        bool was_loaded = false;

        int sym = 0;
        nc_color color;
    private:
        /**
         * How easy it is to spot the trap. Smaller values means it's easier to spot.
         * 1 to ??, affects detection
         */
        // @TODO it can be negative (?)
        // @TODO Add checks for it having proper values
        // @TODO check usage in combination with is_always_invisible
        int visibility = 1;
        // 0 to ??, affects avoidance
        int avoidance = 0;
        /*
         * This is used when disarming the trap. A value of 0 means disarming will always work
         * (e.g. for funnels), a values of 99 means it can not be disarmed at all. Smaller values
         * makes it easier to disarm the trap.
         */
        int difficulty = 0;
        // 0 to ??, trap radius
        int trap_radius = 0;
        bool benign = false;
        bool always_invisible = false;
        update_mapgen_id map_regen;
        trap_function act;
        translation name_;

        std::optional<translation> memorial_male;
        std::optional<translation> memorial_female;

        std::optional<translation> trigger_message_u;
        std::optional<translation> trigger_message_npc;

        cata::flat_set<flag_id> _flags;

        /**
         * If an item with this weight or more is thrown onto the trap, it triggers.
         */
        units::mass trigger_weight = 500_gram;
        int funnel_radius_mm = 0;
        // For disassembly?
        std::vector<std::tuple<itype_id, int, int>> components;
    public:
        // data required for trapfunc::spell()
        fake_spell spell_data;
        int comfort = 0;
        int floor_bedding_warmth = 0;
        vehicle_handle_trap_data vehicle_data;
        std::string name() const;
        /**
         * There are special always invisible traps. See player::search_surroundings
         */
        bool is_always_invisible() const {
            return always_invisible;
        }

        bool operator==( const trap_id &id ) const {
            return loadid == id;
        }
        bool operator!=( const trap_id &id ) const {
            return loadid != id;
        }

        bool has_flag( const flag_id &flag ) const {
            return _flags.count( flag );
        }

        bool has_memorial_msg() const {
            return memorial_male && memorial_female;
        }

        std::string memorial_msg( bool male ) const {
            if( male ) {
                return memorial_male->translated();
            }
            return memorial_female->translated();
        }

        /**
         * Called when the player examines a tile. This is supposed to handled
         * all kind of interaction of the player with the trap, including removal.
         * It also handles visibility of the trap, and it does nothing when
         * called on the null trap.
         */
        // Implemented for historical reasons in iexamine.cpp
        void examine( const tripoint &examp ) const;

        update_mapgen_id map_regen_target() const;

        /**
         * Whether triggering the trap can be avoid (if greater than 0) and if so, this is
         * compared to dodge skill (with some adjustments). Smaller values means it's easier
         * to dodge.
         */
        int get_avoidance() const {
            return avoidance;
        }
        /**
         * If true, this is not really a trap and there won't be any safety queries before stepping
         * onto it (e.g. for funnels).
         */
        bool is_benign() const {
            return benign;
        }
        /**
         * @return True for traps that can simply be taken down without any skill check or similar.
         * This usually applies to traps like funnels, rollmat.
         */
        bool easy_take_down() const;

        bool is_trivial_to_spot() const;

        /**
         * Some traps are part of the terrain (e.g. pits) and can therefore not be disarmed
         * via the usual mechanics. They can be "disarmed" by changing the terrain they are part of.
         */
        bool can_not_be_disarmed() const;

        /**
         * Whether this kind of trap will be detected by ground sonar (e.g. via the bionic).
         */
        bool detected_by_ground_sonar() const;
        /** Player has not yet seen the trap and returns the variable chance, at this moment,
         of whether the trap is seen or not. */
        bool detect_trap( const tripoint &pos, const Character &p ) const;
        /**
         * Can player/npc p see this kind of trap, either by their memory (they known there is
         * the trap) or by the visibility of the trap (the trap is not hidden at all)?
         */
        bool can_see( const tripoint &pos, const Character &p ) const;

        bool has_trigger_msg() const {
            return trigger_message_u && trigger_message_npc;
        }
        /**
        * Prints a trap-specific trigger message when player steps on it.
        */
        std::string get_trigger_message_u() const {
            return trigger_message_u->translated();
        }
        /**
        * Prints a trap-specific trigger message when NPC or a monster steps on it.
        */
        std::string get_trigger_message_npc() const {
            return trigger_message_npc->translated();
        }
    private:
        /**
         * Trigger trap effects.
         * @param creature The creature that triggered the trap, it does not necessarily have to
         * be on the place of the trap (traps can be triggered from adjacent, e.g. when disarming
         * them). This can also be a null pointer if the trap has been triggered by some thrown
         * item (which must have the @ref trigger_weight).
         * @param pos The location of the trap in the main map.
         * @param item The item that triggered the trap
         */
        // Don't call from outside this class. Add a wrapper like the ones below instead.
        void trigger( const tripoint &pos, Creature *creature, item *item ) const;
    public:
        /*@{*/
        /**
         * This applies the effects of the trap to the world and
         * possibly to the triggering object (creature, item).
         *
         * The function assumes the
         * caller has already checked whether the trap should be activated
         * (e.g. the creature has had a chance to avoid the trap, but it failed).
         */
        void trigger( const tripoint &pos, Creature &creature ) const;
        void trigger( const tripoint &pos, item &item ) const;
        /*@}*/

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
         * This is used when defining area this trap occupies. A value of 0 means trap occupies exactly 1 tile.
         */
        int get_trap_radius() const {
            return trap_radius;
        }
        /**
         * Whether this is the null-traps, aka no trap at all.
         */
        bool is_null() const;
        /**
         * Loads this specific trap.
         */
        void load( const JsonObject &jo, const std::string &src );

        std::string debug_describe() const;

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
         * @throw JsonError if the json is invalid as usual.
         */
        static void load_trap( const JsonObject &jo, const std::string &src );
        /**
         * Releases the loaded trap objects in trapmap and traplist.
         */
        static void reset();
        /**
         * Stores the actual @ref loadid of the loaded traps in the global tr_* variables.
         * It also sets the trap ids of the terrain types that have built-in traps.
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

extern trap_id tr_null;
extern const trap_str_id tr_beartrap_buried;
extern const trap_str_id tr_shotgun_2;
extern const trap_str_id tr_shotgun_1;
extern const trap_str_id tr_blade;
extern const trap_str_id tr_landmine;
extern const trap_str_id tr_landmine_buried;
extern const trap_str_id tr_telepad;
extern const trap_str_id tr_goo;
extern const trap_str_id tr_dissector;
extern const trap_str_id tr_sinkhole;
extern const trap_str_id tr_pit;
extern const trap_str_id tr_lava;
extern const trap_str_id tr_portal;
extern const trap_str_id tr_ledge;
extern const trap_str_id tr_temple_flood;
extern const trap_str_id tr_temple_toggle;
extern const trap_str_id tr_glow;
extern const trap_str_id tr_hum;
extern const trap_str_id tr_shadow;
extern const trap_str_id tr_drain;
extern const trap_str_id tr_snake;

#endif // CATA_SRC_TRAP_H
