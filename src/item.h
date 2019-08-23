#pragma once
#ifndef ITEM_H
#define ITEM_H

#include <stdint.h>
#include <climits>
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include "calendar.h"
#include "cata_utility.h"
#include "craft_command.h"
#include "debug.h"
#include "enums.h"
#include "flat_set.h"
#include "io_tags.h"
#include "item_location.h"
#include "requirements.h"
#include "safe_reference.h"
#include "string_id.h"
#include "type_id.h"
#include "units.h"
#include "visitable.h"
#include "gun_mode.h"
#include "point.h"

class item;
class material_type;
struct mtype;
class faction;

namespace cata
{
template<typename T>
class optional;
} // namespace cata
class nc_color;
class JsonIn;
class JsonOut;
class iteminfo_query;
template<typename T>
class ret_val;
class gun_type_type;
class gunmod_location;
class Character;
class player;
class recipe;
struct itype;
struct islot_comestible;

using bodytype_id = std::string;
struct islot_armor;
struct use_function;
class item_category;

enum art_effect_passive : int;
enum phase_id : int;
enum body_part : int;
enum m_size : int;
enum class side : int;
class body_part_set;

using itype_id = std::string;
struct fire_data;
struct damage_instance;
struct damage_unit;
class map;

enum damage_type : int;
enum clothing_mod_type : int;

const std::string &rad_badge_color( int rad );

struct light_emission {
    unsigned short luminance;
    short width;
    short direction;
};
extern light_emission nolight;

/**
 *  Value and metadata for one property of an item
 *
 *  Contains the value of one property of an item, as well as various metadata items required to
 *  output that value.  This is used primarily for user output of information about an item, for
 *  example in the various inventory menus.  See @ref item::info() for the main example of how a
 *  class desiring to provide user output might obtain a class of this type.
 *
 *  As an example, if the item being queried was a piece of clothing, then several properties might
 *  be returned.  All would have sType "ARMOR".  There would be one for the coverage stat with
 *  sName "Coverage: ", another for the warmth stat with sName "Warmth: ", etc.
 */
struct iteminfo {
    public:
        /** Category of item that owns this iteminfo.  See @ref item_category. */
        std::string sType;

        /** Main text of this property's name */
        std::string sName;

        /** Formatting text to be placed between the name and value of this item. */
        std::string sFmt;

        /** Numerical value of this property. Set to -999 if no compare value is present */
        std::string sValue;

        /** Internal double floating point version of value, for numerical comparisons */
        double dValue;

        /** Flag indicating type of sValue.  True if integer, false if single decimal */
        bool is_int;

        /** Flag indicating whether a newline should be printed after printing this item */
        bool bNewLine;

        /** Reverses behavior of red/green text coloring; smaller values are green if true */
        bool bLowerIsBetter;

        /** Whether to print sName.  If false, use for comparisons but don't print for user. */
        bool bDrawName;

        /** Whether to print a sign on positive values */
        bool bShowPlus;

        enum flags {
            no_flags = 0,
            is_decimal = 1 << 0, ///< Print as decimal rather than integer
            no_newline = 1 << 1, ///< Do not follow with a newline
            lower_is_better = 1 << 2, ///< Lower values are better for this stat
            no_name = 1 << 3, ///< Do not print the name
            show_plus = 1 << 4, ///< Use a + sign for positive values
        };

        /**
         *  @param Type The item type of the item this iteminfo belongs to.
         *  @param Name The name of the property this iteminfo describes.
         *  @param Fmt Formatting text desired between item name and value
         *  @param Flags Additional flags to customize this entry
         *  @param Value Numerical value of this property, -999 for none.
         */
        iteminfo( const std::string &Type, const std::string &Name, const std::string &Fmt = "",
                  flags Flags = no_flags, double Value = -999 );
        iteminfo( const std::string &Type, const std::string &Name, double Value );
};

inline iteminfo::flags operator|( iteminfo::flags l, iteminfo::flags r )
{
    using I = std::underlying_type<iteminfo::flags>::type;
    return static_cast<iteminfo::flags>( static_cast<I>( l ) | r );
}

inline iteminfo::flags &operator|=( iteminfo::flags &l, iteminfo::flags r )
{
    return l = l | r;
}

inline bool is_crafting_component( const item &component );

class item : public visitable<item>
{
    public:
        item();

        item( item && ) = default;
        item( const item & ) = default;
        item &operator=( item && ) = default;
        item &operator=( const item & ) = default;

        explicit item( const itype_id &id, time_point turn = calendar::turn, int qty = -1 );
        explicit item( const itype *type, time_point turn = calendar::turn, int qty = -1 );

        /** Suppress randomization and always start with default quantity of charges */
        struct default_charges_tag {};
        item( const itype_id &id, time_point turn, default_charges_tag );
        item( const itype *type, time_point turn, default_charges_tag );

        /** Default (or randomized) charges except if counted by charges then only one charge */
        struct solitary_tag {};
        item( const itype_id &id, time_point turn, solitary_tag );
        item( const itype *type, time_point turn, solitary_tag );

        /** For constructing in-progress crafts */
        item( const recipe *rec, int qty, std::list<item> items, std::vector<item_comp> selections );

        /** Return a pointer-like type that's automatically invalidated if this
         * item is destroyed or assigned-to */
        safe_reference<item> get_safe_reference();

        /**
         * Filter converting this instance to another type preserving all other aspects
         * @param new_type the type id to convert to
         * @return same instance to allow method chaining
         */
        item &convert( const itype_id &new_type );

        /**
         * Filter converting this instance to the inactive type
         * If the item is either inactive or cannot be deactivated is a no-op
         * @param ch character currently possessing or acting upon the item (if any)
         * @param alert whether to display any messages
         * @return same instance to allow method chaining
         */
        item &deactivate( const Character *ch = nullptr, bool alert = true );

        /** Filter converting instance to active state */
        item &activate();

        /**
         * Add or remove energy from a battery.
         * If adding the specified energy quantity would go over the battery's capacity fill
         * the battery and ignore the remainder.
         * If adding the specified energy quantity would reduce the battery's charge level
         * below 0 do nothing and return how far below 0 it would have gone.
         * @param qty energy quantity to add (can be negative)
         * @return 0 valued energy quantity on success
         */
        units::energy set_energy( const units::energy &qty );

        /**
         * Filter setting the ammo for this instance
         * Any existing ammo is removed. If necessary a magazine is also added.
         * @param ammo specific type of ammo (must be compatible with item ammo type)
         * @param qty maximum ammo (capped by item capacity) or negative to fill to capacity
         * @return same instance to allow method chaining
         */
        item &ammo_set( const itype_id &ammo, int qty = -1 );

        /**
         * Filter removing all ammo from this instance
         * If the item is neither a tool, gun nor magazine is a no-op
         * For items reloading using magazines any empty magazine remains present.
         */
        item &ammo_unset();

        /**
         * Filter setting damage constrained by @ref min_damage and @ref max_damage
         * @note this method does not invoke the @ref on_damage callback
         * @return same instance to allow method chaining
         */
        item &set_damage( int qty );

        /**
         * Splits a count-by-charges item always leaving source item with minimum of 1 charge
         * @param qty number of required charges to split from source
         * @return new instance containing exactly qty charges or null item if splitting failed
         */
        item split( int qty );

        /**
         * Make a corpse of the given monster type.
         * The monster type id must be valid (see @ref MonsterGenerator::get_all_mtypes).
         *
         * The turn parameter sets the birthday of the corpse, in other words: the turn when the
         * monster died. Because corpses are removed from the map when they reach a certain age,
         * one has to be careful when placing corpses with a birthday of 0. They might be
         * removed immediately when the map is loaded without been seen by the player.
         *
         * The name parameter can be used to give the corpse item a name. This is
         * used instead of the monster type name ("corpse of X" instead of "corpse of bear").
         *
         * With the default parameters it makes a human corpse, created at the current turn.
         */
        /*@{*/
        static item make_corpse( const mtype_id &mt = string_id<mtype>::NULL_ID(),
                                 time_point turn = calendar::turn, const std::string &name = "" );
        /*@}*/
        /**
         * @return The monster type associated with this item (@ref corpse). It is usually the
         * type that this item is made of (e.g. corpse, meat or blood of the monster).
         * May return a null-pointer.
         */
        const mtype *get_mtype() const;
        /**
         * Sets the monster type associated with this item (@ref corpse). You must not pass a
         * null pointer.
         * TODO: change this to take a reference instead.
         */
        void set_mtype( const mtype *m );
        /**
         * Whether this is a corpse item. Corpses always have valid monster type (@ref corpse)
         * associated (@ref get_mtype return a non-null pointer) and have been created
         * with @ref make_corpse.
         */
        bool is_corpse() const;
        /**
         * Whether this is a corpse that can be revived.
         */
        bool can_revive() const;
        /**
         * Whether this corpse should revive now. Note that this function includes some randomness,
         * the return value can differ on successive calls.
         * @param pos The location of the item (see REVIVE_SPECIAL flag).
         */
        bool ready_to_revive( const tripoint &pos ) const;

        bool is_money() const;

        /**
         * Returns the default color of the item (e.g. @ref itype::color).
         */
        nc_color color() const;
        /**
         * Returns the color of the item depending on usefulness for the player character,
         * e.g. differently if it its an unread book or a spoiling food item etc.
         * This should only be used for displaying data, it should not affect game play.
         */
        nc_color color_in_inventory() const;
        /**
         * Return the (translated) item name.
         * @param quantity used for translation to the proper plural form of the name, e.g.
         * returns "rock" for quantity 1 and "rocks" for quantity > 0.
         * @param with_prefix determines whether to include more item properties, such as
         * the extent of damage and burning (was created to sort by name without prefix
         * in additional inventory)
         */
        std::string tname( unsigned int quantity = 1, bool with_prefix = true,
                           unsigned int truncate = 0 ) const;
        std::string display_money( unsigned int quantity, unsigned int amount ) const;
        /**
         * Returns the item name and the charges or contained charges (if the item can have
         * charges at all). Calls @ref tname with given quantity and with_prefix being true.
         */
        std::string display_name( unsigned int quantity = 1 ) const;
        /**
         * Return all the information about the item and its type.
         *
         * This includes the different
         * properties of the @ref itype (if they are visible to the player). The returned string
         * is already translated and can be *very* long.
         * @param showtext If true, shows the item description, otherwise only the properties item type.
         */
        std::string info( bool showtext = false ) const;

        /**
         * Return all the information about the item and its type, and dump to vector.
         *
         * This includes the different
         * properties of the @ref itype (if they are visible to the player). The returned string
         * is already translated and can be *very* long.
         * @param showtext If true, shows the item description, otherwise only the properties item type.
         * @param dump The properties (encapsulated into @ref iteminfo) are added to this vector,
         * the vector can be used to compare them to properties of another item.
         */
        std::string info( bool showtext, std::vector<iteminfo> &iteminfo ) const;

        /**
        * Return all the information about the item and its type, and dump to vector.
        *
        * This includes the different
        * properties of the @ref itype (if they are visible to the player). The returned string
        * is already translated and can be *very* long.
        * @param showtext If true, shows the item description, otherwise only the properties item type.
        * @param dump The properties (encapsulated into @ref iteminfo) are added to this vector,
        * the vector can be used to compare them to properties of another item.
        * @param batch The batch crafting number to multiply data by
        */
        std::string info( bool showtext, std::vector<iteminfo> &iteminfo, int batch ) const;

        /**
        * Return all the information about the item and its type, and dump to vector.
        *
        * This includes the different
        * properties of the @ref itype (if they are visible to the player). The returned string
        * is already translated and can be *very* long.
        * @param parts controls which parts of the iteminfo to return.
        * @param dump The properties (encapsulated into @ref iteminfo) are added to this vector,
        * the vector can be used to compare them to properties of another item.
        * @param batch The batch crafting number to multiply data by
        */
        std::string info( std::vector<iteminfo> &info, const iteminfo_query *parts = nullptr,
                          int batch = 1 ) const;

        /**
         * Calculate all burning calculations, but don't actually apply them to item.
         * DO apply them to @ref fire_data argument, though.
         * @return Amount of "burn" that would be applied to the item.
         */
        float simulate_burn( fire_data &frd ) const;
        /** Burns the item. Returns true if the item was destroyed. */
        bool burn( fire_data &frd );

        // Returns the category of this item.
        const item_category &get_category() const;

        class reload_option
        {
            public:
                reload_option() = default;

                reload_option( const reload_option & );
                reload_option &operator=( const reload_option & );

                reload_option( const player *who, const item *target, const item *parent,
                               const item_location &ammo );

                const player *who = nullptr;
                const item *target = nullptr;
                item_location ammo;

                int qty() const {
                    return qty_;
                }
                void qty( int val );

                int moves() const;

                explicit operator bool() const {
                    return who && target && ammo && qty_ > 0;
                }

            private:
                int qty_ = 0;
                int max_qty = INT_MAX;
                const item *parent = nullptr;
        };

        /**
         * Reload item using ammo from location returning true if successful
         * @param u Player doing the reloading
         * @param loc Location of ammo to be reloaded
         * @param qty caps reloading to this (or fewer) units
         */
        bool reload( player &u, item_location loc, int qty );

        template<typename Archive>
        void io( Archive & );
        using archive_type_tag = io::object_archive_tag;

        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );

        const std::string &symbol() const;
        /**
         * Returns the monetary value of an item.
         * If `practical` is false, returns pre-cataclysm market value,
         * otherwise returns approximate post-cataclysm value.
         */
        int price( bool practical ) const;

        /**
         * Whether two items should stack when displayed in a inventory menu.
         * This is different from stacks_with, when two previously non-stackable
         * items are now stackable and mergeable because, for example, they
         * reaches the same temperature. This is necessary to avoid misleading
         * stacks like "3 items-count-by-charge (5)".
         */
        bool display_stacked_with( const item &rhs, bool check_components = false ) const;
        bool stacks_with( const item &rhs, bool check_components = false ) const;
        /**
         * Merge charges of the other item into this item.
         * @return true if the items have been merged, otherwise false.
         * Merging is only done for items counted by charges (@ref count_by_charges) and
         * items that stack together (@ref stacks_with).
         */
        bool merge_charges( const item &rhs );

        units::mass weight( bool include_contents = true, bool integral = false ) const;

        /**
         * Total volume of an item accounting for all contained/integrated items
         * NOTE: Result is rounded up to next nearest milliliter when working with stackable (@ref count_by_charges) items that have fractional volume per charge.
         * If trying to determine how many of an item can fit in a given space, @ref charges_per_volume should be used instead.
         * @param integral if true return effective volume if this item was integrated into another
         */
        units::volume volume( bool integral = false ) const;

        /**
         * Simplified, faster volume check for when processing time is important and exact volume is not.
         * NOTE: Result is rounded up to next nearest milliliter when working with stackable (@ref count_by_charges) items that have fractional volume per charge.
         * If trying to determine how many of an item can fit in a given space, @ref charges_per_volume should be used instead.
         */
        units::volume base_volume() const;

        /** Volume check for corpses, helper for base_volume(). */
        units::volume corpse_volume( const mtype *corpse ) const;

        /** Required strength to be able to successfully lift the item unaided by equipment */
        int lift_strength() const;

        /**
         * @name Melee
         *
         * The functions here assume the item is used in melee, even if's a gun or not a weapon at
         * all. Because the functions apply to all types of items, several of the is_* functions here
         * may return true for the same item. This only indicates that it can be used in various ways.
         */
        /*@{*/
        /**
         * Base number of moves (@ref Creature::moves) that a single melee attack with this items
         * takes. The actual time depends heavily on the attacker, see melee.cpp.
         */
        int attack_time() const;

        /** Damage of given type caused when this item is used as melee weapon */
        int damage_melee( damage_type dt ) const;

        /** All damage types this item deals when used in melee (no skill modifiers etc. applied). */
        damage_instance base_damage_melee() const;
        /** All damage types this item deals when thrown (no skill modifiers etc. applied). */
        damage_instance base_damage_thrown() const;

        /**
         * Whether the character needs both hands to wield this item.
         */
        bool is_two_handed( const Character &guy ) const;

        /** Is this item an effective melee weapon for the given damage type? */
        bool is_melee( damage_type dt ) const;

        /**
         *  Is this item an effective melee weapon for any damage type?
         *  @see item::is_gun()
         *  @note an item can be both a gun and melee weapon concurrently
         */
        bool is_melee() const;

        /**
         * The most relevant skill used with this melee weapon. Can be "null" if this is not a weapon.
         * Note this function returns null if the item is a gun for which you can use gun_skill() instead.
         */
        skill_id melee_skill() const;
        /*@}*/

        /** Max range weapon usable for melee attack accounting for player/NPC abilities */
        int reach_range( const player &p ) const;

        /**
         * Sets time until activation for an item that will self-activate in the future.
         **/
        void set_countdown( int num_turns );

        /**
         * Consumes specified charges (or fewer) from this and any contained items
         * @param what specific type of charge required, e.g. 'battery'
         * @param qty maximum charges to consume. On return set to number of charges not found (or zero)
         * @param used filled with duplicates of each item that provided consumed charges
         * @param pos position at which the charges are being consumed
         * @param filter Must return true for use to occur.
         * @return true if this item should be deleted (count-by-charges items with no remaining charges)
         */
        bool use_charges( const itype_id &what, int &qty, std::list<item> &used, const tripoint &pos,
                          const std::function<bool( const item & )> &filter = return_true<item> );

        /**
         * Invokes item type's @ref itype::drop_action.
         * This function can change the item.
         * @param pos Where is the item being placed. Note: the item isn't there yet.
         * @return true if the item was destroyed during placement.
         */
        bool on_drop( const tripoint &pos );

        /**
         * Invokes item type's @ref itype::drop_action.
         * This function can change the item.
         * @param pos Where is the item being placed. Note: the item isn't there yet.
         * @param map A map object associated with that position.
         * @return true if the item was destroyed during placement.
         */
        bool on_drop( const tripoint &pos, map &map );

        /**
         * Consume a specific amount of items of a specific type.
         * This includes this item, and any of its contents (recursively).
         * @see item::use_charges - this is similar for items, not charges.
         * @param it Type of consumable item.
         * @param quantity How much to consumed.
         * @param used On success all consumed items will be stored here.
         * @param filter Must return true for use to occur.
         */
        bool use_amount( const itype_id &it, int &quantity, std::list<item> &used,
                         const std::function<bool( const item & )> &filter = return_true<item> );

        /** Permits filthy components, should only be used as a helper in creating filters */
        bool allow_crafting_component() const;

        /**
         * @name Containers
         *
         * Containers come in two flavors:
         * - suitable for liquids (@ref is_watertight_container),
         * - and the remaining one (they are for currently only for flavor).
         */
        /*@{*/
        /** Whether this is container. Note that container does not necessarily means it's
         * suitable for liquids. */
        bool is_container() const;
        /** Whether this is a container which can be used to store liquids. */
        bool is_watertight_container() const;
        /** Whether this item has no contents at all. */
        bool is_container_empty() const;
        /** Whether removing this item's contents will permanently alter it. */
        bool is_non_resealable_container() const;
        /**
         * Whether this item has no more free capacity for its current content.
         * @param allow_bucket Allow filling non-sealable containers
         */
        bool is_container_full( bool allow_bucket = false ) const;
        /**
         * Fill item with liquid up to its capacity. This works for guns and tools that accept
         * liquid ammo.
         * @param liquid Liquid to fill the container with.
         * @param amount Amount to fill item with, capped by remaining capacity
         */
        void fill_with( item &liquid, int amount = INFINITE_CHARGES );
        /**
         * How much more of this liquid (in charges) can be put in this container.
         * If this is not a container (or not suitable for the liquid), it returns 0.
         * Note that mixing different types of liquid is not possible.
         * Also note that this works for guns and tools that accept liquid ammo.
         * @param liquid Liquid to check capacity for
         * @param allow_bucket Allow filling non-sealable containers
         * @param err Message to print if no more material will fit
         */
        int get_remaining_capacity_for_liquid( const item &liquid, bool allow_bucket = false,
                                               std::string *err = nullptr ) const;
        int get_remaining_capacity_for_liquid( const item &liquid, const Character &p,
                                               std::string *err = nullptr ) const;
        /**
         * It returns the total capacity (volume) of the container for liquids.
         */
        units::volume get_container_capacity() const;
        /**
         * It returns the maximum volume of any contents, including liquids,
         * ammo, magazines, weapons, etc.
         */
        units::volume get_total_capacity() const;
        /**
         * Puts the given item into this one, no checks are performed.
         */
        void put_in( const item &payload );

        /** Stores a newly constructed item at the end of this item's contents */
        template<typename ... Args>
        item &emplace_back( Args &&... args ) {
            contents.emplace_back( std::forward<Args>( args )... );
            if( contents.back().is_null() ) {
                debugmsg( "Tried to emplace null item" );
            }
            return contents.back();
        }

        /**
         * Returns this item into its default container. If it does not have a default container,
         * returns this. It's intended to be used like \code newitem = newitem.in_its_container();\endcode
         */
        item in_its_container() const;
        item in_container( const itype_id &container_type ) const;
        /*@}*/

        /*@{*/
        /**
         * Funnel related functions. See weather.cpp for their usage.
         */
        bool is_funnel_container( units::volume &bigger_than ) const;
        void add_rain_to_container( bool acid, int charges = 1 );
        /*@}*/

        int get_quality( const quality_id &id ) const;
        bool count_by_charges() const;

        /**
         * If count_by_charges(), returns charges, otherwise 1
         */
        int count() const;
        bool craft_has_charges();

        /**
         * Modify the charges of this item, only use for items counted by charges!
         * The item must have enough charges for this (>= quantity) and be counted
         * by charges.
         * @param mod How many charges should be removed.
         */
        void mod_charges( int mod );

        /**
         * Accumulate rot of the item since last rot calculation.
         * This function should not be called directly. since it does not have all the needed checks or temperature calculations.
         * If you need to calc rot of item call process_temperature_rot instead.
         * @param time Time point to which rot is calculated
         * @param temp Temperature at which the rot is calculated
         */
        void calc_rot( time_point time, int temp );

        /**
         * This is part of a workaround so that items don't rot away to nothing if the smoking rack
         * is outside the reality bubble.
         * @param smoking_duration
         */
        void calc_rot_while_processing( time_duration processing_duration );

        /**
         * Update temperature for things like food
         * Update rot for things that perish
         * All items that rot also have temperature
         * @param temp Temperature at which item is current exposed
         * @param insulation Amount of insulation item has from surroundings
         * @param pos The current position
         * @param carrier The current carrier
         * @param flag to specify special temperature situations
         */
        void process_temperature_rot( float insulation, const tripoint &pos, player *carrier,
                                      temperature_flag flag = temperature_flag::TEMP_NORMAL );

        /** Set the item to HOT */
        void heat_up();

        /** Set the item to COLD */
        void cold_up();

        /** Sets the item temperature and item energy from new temperature (K)*/
        void set_item_temperature( float new_temperature );

        /** Sets the item to new temperature and energy based new specific energy (J/g)*/
        void set_item_specific_energy( float specific_energy );

        /** reset the last_temp_check used when crafting new items and the like */
        void reset_temp_check();

        /** whether an item is perishable (can rot) */
        bool goes_bad() const;

        /** Get the shelf life of the item*/
        time_duration get_shelf_life() const;

        /** Get @ref rot value relative to shelf life (or 0 if item does not spoil) */
        double get_relative_rot() const;

        /** Set current item @ref rot relative to shelf life (no-op if item does not spoil) */
        void set_relative_rot( double val );

        void set_rot( time_duration val );

        /**
         * Get time left to rot, ignoring fridge.
         * Returns time to rot if item is able to, max int - N otherwise,
         * where N is
         * 3 for food,
         * 2 for medication,
         * 1 for other comestibles,
         * 0 otherwise.
         */
        int spoilage_sort_order();

        /** an item is fresh if it is capable of rotting but still has a long shelf life remaining */
        bool is_fresh() const {
            return goes_bad() && get_relative_rot() < 0.1;
        }

        /** an item is about to become rotten when shelf life has nearly elapsed */
        bool is_going_bad() const {
            return get_relative_rot() > 0.9;
        }

        /** returns true if item is now rotten after all shelf life has elapsed */
        bool rotten() const {
            return get_relative_rot() > 1.0;
        }

        /** at twice regular shelf life perishable foods rot away completely. Corpses last longer */
        bool has_rotten_away() const {
            return is_food() && get_relative_rot() > 2.0;
        }

        /** remove frozen tag and if it takes freezerburn, applies mushy/rotten */
        void apply_freezerburn();

        time_duration get_rot() const {
            return rot;
        }
        void mod_rot( const time_duration &val ) {
            rot += val;
        }

        /** Time for this item to be fully fermented. */
        time_duration brewing_time() const;
        /** The results of fermenting this item. */
        const std::vector<itype_id> &brewing_results() const;

        /**
         * Detonates the item and adds remains (if any) to drops.
         * Returns true if the item actually detonated,
         * potentially destroying other items and invalidating iterators.
         * Should NOT be called on an item on the map, but on a local copy.
         */
        bool detonate( const tripoint &p, std::vector<item> &drops );

        bool will_explode_in_fire() const;

        /**
         * @name Material(s) of the item
         *
         * Each item is made of one or more materials (@ref material_type). Materials have
         * properties that affect properties of the item (e.g. resistance against certain
         * damage types).
         *
         * Additionally, items have a phase property (@ref phase_id). This is independent of
         * the material types (there can be solid items made of X and liquid items made of the same
         * material).
         *
         * Corpses inherit the material of the monster type.
         */
        /*@{*/
        /**
         * Get a material reference to a random material that this item is made of.
         * This might return the null-material, you may check this with @ref material_type::ident.
         * Note that this may also return a different material each time it's invoked (if the
         * item is made from several materials).
         */
        const material_type &get_random_material() const;
        /**
         * Get the basic (main) material of this item. May return the null-material.
         */
        const material_type &get_base_material() const;
        /**
         * The ids of all the materials this is made of.
         * This may return an empty vector.
         * The returned vector does not contain the null id.
         */
        const std::vector<material_id> &made_of() const;
        /**
        * The ids of all the qualities this contains.
        */
        const std::map<quality_id, int> &quality_of() const;
        /**
         * Same as @ref made_of(), but returns the @ref material_type directly.
         */
        std::vector<const material_type *> made_of_types() const;
        /**
         * Check we are made of at least one of a set (e.g. true if at least
         * one item of the passed in set matches any material).
         * @param mat_idents Set of material ids.
         */
        bool made_of_any( const std::set<material_id> &mat_idents ) const;
        /**
         * Check we are made of only the materials (e.g. false if we have
         * one material not in the set or no materials at all).
         * @param mat_idents Set of material ids.
         */
        bool only_made_of( const std::set<material_id> &mat_idents ) const;
        /**
         * Check we are made of this material (e.g. matches at least one
         * in our set.)
         */
        bool made_of( const material_id &mat_ident ) const;
        /**
         * If contents nonempty, return true if item phase is same, else false
         */
        bool contents_made_of( phase_id phase ) const;
        /**
         * Are we solid, liquid, gas, plasma?
         */
        bool made_of( phase_id phase ) const;
        bool made_of_from_type( phase_id phase ) const;
        /**
         * Returns a list of components used to craft this item or the default
         * components if it wasn't player-crafted.
         */
        std::vector<item_comp> get_uncraft_components() const;
        /**
         * Whether the items is conductive.
         */
        bool conductive() const;
        /**
         * Whether the items is flammable. (Make sure to keep this in sync with
         * fire code in fields.cpp)
         * @param threshold Item is flammable if it provides more fuel than threshold.
         */
        bool flammable( int threshold = 0 ) const;
        /**
        * Whether the item can be repaired beyond normal health.
        */
        bool reinforceable() const;
        /*@}*/

        /**
         * Resistance against different damage types (@ref damage_type).
         * Larger values means more resistance are thereby better, but there is no absolute value to
         * compare them to. The values can be interpreted as chance (@ref one_in) of damaging the item
         * when exposed to the type of damage.
         * @param to_self If this is true, it returns item's own resistance, not one it gives to wearer.
         * @param base_env_resist Will override the base environmental
         * resistance (to allow hypothetical calculations for gas masks).
         */
        /*@{*/
        int bash_resist( bool to_self = false ) const;
        int cut_resist( bool to_self = false )  const;
        int stab_resist( bool to_self = false ) const;
        int acid_resist( bool to_self = false, int base_env_resist = 0 ) const;
        int fire_resist( bool to_self = false, int base_env_resist = 0 ) const;
        /*@}*/

        /**
         * Assuming that specified du hit the armor, reduce du based on the item's resistance to the
         * damage type. This will never reduce du.amount below 0.
         */
        void mitigate_damage( damage_unit &du ) const;
        /**
         * Resistance provided by this item against damage type given by an enum.
         */
        int damage_resist( damage_type dt, bool to_self = false ) const;

        /**
         * Returns resistance to being damaged by attack against the item itself.
         * Calculated from item's materials.
         * @param worst If this is true, the worst resistance is used. Otherwise the best one.
         */
        int chip_resistance( bool worst = false ) const;

        /** How much damage has the item sustained? */
        int damage() const;

        /**
         * Scale item damage to the given number of levels. This function is
         * here mostly for back-compatibility. It should not be used when
         * doing continuous math with the damage value: use damage() instead.
         *
         * For example, for max = 4, min_damage = -1000, max_damage = 4000
         *   damage       level
         *   -1000 ~   -1    -1
         *              0     0
         *       1 ~ 1333     1
         *    1334 ~ 2666     2
         *    2667 ~ 3999     3
         *           4000     4
         *
         * @param max Maximum number of levels
         */
        int damage_level( int max ) const;

        /** Minimum amount of damage to an item (state of maximum repair) */
        int min_damage() const;

        /** Maximum amount of damage to an item (state before destroyed) */
        int max_damage() const;

        /**
         * Relative item health.
         * Returns 1 for undamaged ||items, values in the range (0, 1) for damaged items
         * and values above 1 for reinforced ++items.
         */
        float get_relative_health() const;

        /**
         * Apply damage to const itemrained by @ref min_damage and @ref max_damage
         * @param qty maximum amount by which to adjust damage (negative permissible)
         * @param dt type of damage which may be passed to @ref on_damage callback
         * @return whether item should be destroyed
         */
        bool mod_damage( int qty, damage_type dt );
        /// same as other mod_damage, but uses @ref DT_NULL as damage type.
        bool mod_damage( int qty );

        /**
         * Increment item damage by @ref itype::damage_scale constrained by @ref max_damage
         * @param dt type of damage which may be passed to @ref on_damage callback
         * @return whether item should be destroyed
         */
        bool inc_damage( damage_type dt );
        /// same as other inc_damage, but uses @ref DT_NULL as damage type.
        bool inc_damage();

        /** Provide color for UI display dependent upon current item damage level */
        nc_color damage_color() const;

        /** Provide prefix symbol for UI display dependent upon current item damage level */
        std::string damage_symbol() const;

        /**
         * Provides a prefix for the durability state of the item. with ITEM_HEALTH_BAR enabled,
         * returns a symbol with color tag already applied. Otherwise, returns an adjective.
         * if include_intact is true, this provides a string for the corner case of a player
         * with ITEM_HEALTH_BAR disabled, but we need still a string for some reason.
         */
        std::string durability_indicator( bool include_intact = false ) const;

        /** If possible to repair this item what tools could potentially be used for this purpose? */
        const std::set<itype_id> &repaired_with() const;

        /**
         * Check whether the item has been marked (by calling mark_as_used_by_player)
         * as used by this specific player.
         */
        bool already_used_by_player( const player &p ) const;
        /**
         * Marks the item as being used by this specific player, it remains unmarked
         * for other players. The player is identified by its id.
         */
        void mark_as_used_by_player( const player &p );
        /** Marks the item as filthy, so characters with squeamish trait can't wear it.
        */
        bool is_filthy() const;
        /**
         * This is called once each turn. It's usually only useful for active items,
         * but can be called for inactive items without problems.
         * It is recursive, and calls process on any contained items.
         * @param carrier The player / npc that carries the item. This can be null when
         * the item is not carried by anyone (laying on ground)!
         * @param pos The location of the item on the map, same system as
         * @ref player::pos used. If the item is carried, it should be the
         * location of the carrier.
         * @param activate Whether the item should be activated (true), or
         * processed as an active item.
         * @return true if the item has been destroyed by the processing. The caller
         * should than delete the item wherever it was stored.
         * Returns false if the item is not destroyed.
         */
        bool process( player *carrier, const tripoint &pos, bool activate, float insulation = 1,
                      temperature_flag flag = temperature_flag::TEMP_NORMAL );

        /**
         * Gets the point (vehicle tile) the cable is connected to.
         * Returns nothing if not connected to anything.
         */
        cata::optional<tripoint> get_cable_target( player *p, const tripoint &pos ) const;
        /**
         * Helper to bring a cable back to its initial state.
         */
        void reset_cable( player *p );

        /**
         * Whether the item should be processed (by calling @ref process).
         */
        bool needs_processing() const;
        /**
         * The rate at which an item should be processed, in number of turns between updates.
         */
        int processing_speed() const;
        /**
         * Process and apply artifact effects. This should be called exactly once each turn, it may
         * modify character stats (like speed, strength, ...), so call it after those have been reset.
         * @param carrier The character carrying the artifact, can be null.
         * @param pos The location of the artifact (should be the player location if carried).
         */
        void process_artifact( player *carrier, const tripoint &pos );

        bool destroyed_at_zero_charges() const;
        // Most of the is_whatever() functions call the same function in our itype
        bool is_null() const; // True if type is NULL, or points to the null item (id == 0)
        bool is_comestible() const;
        bool is_food() const;                // Ignoring the ability to eat batteries, etc.
        bool is_food_container() const;      // Ignoring the ability to eat batteries, etc.
        bool is_med_container() const;
        bool is_ammo_container() const; // does this item contain ammo? (excludes magazines)
        bool is_medication() const;            // Is it a medication that only pretends to be food?
        bool is_bionic() const;
        bool is_magazine() const;
        bool is_battery() const;
        bool is_ammo_belt() const;
        bool is_bandolier() const;
        bool is_holster() const;
        bool is_ammo() const;
        // is this armor for a pet creature?  if on_pet is true, returns false if a pet isn't
        // wearing it
        bool is_pet_armor( bool on_pet = false ) const;
        bool is_armor() const;
        bool is_book() const;
        bool is_map() const;
        bool is_salvageable() const;
        bool is_craft() const;

        bool is_deployable() const;
        bool is_tool() const;
        bool is_transformable() const;
        bool is_artifact() const;
        bool is_bucket() const;
        bool is_bucket_nonempty() const;

        bool is_brewable() const;
        bool is_engine() const;
        bool is_wheel() const;
        bool is_fuel() const;
        bool is_toolmod() const;

        bool is_faulty() const;
        bool is_irremovable() const;

        bool is_unarmed_weapon() const; //Returns true if the item should be considered unarmed

        bool has_temperature() const;

        /** Returns true if the item is A: is SOLID and if it B: is of type LIQUID */
        bool is_frozen_liquid() const;

        float get_specific_heat_liquid() const;
        float get_specific_heat_solid() const;
        float get_latent_heat() const;
        float get_freeze_point() const; // Farenheit

        /** What faults can potentially occur with this item? */
        std::set<fault_id> faults_potential() const;

        /** Returns the total area of this wheel or 0 if it isn't one. */
        int wheel_area() const;

        /** Returns energy of one charge of this item as fuel for an engine. */
        float fuel_energy() const;
        /** Returns the string of the id of the terrain that pumps this fuel, if any. */
        std::string fuel_pump_terrain() const;
        bool has_explosion_data() const;
        struct fuel_explosion get_explosion_data();

        /**
         * Can this item have given item/itype as content?
         *
         * For example, airtight for gas, acidproof for acid etc.
         */
        /*@{*/
        bool can_contain( const item &it ) const;
        bool can_contain( const itype &tp ) const;
        /*@}*/

        /**
         * Is it ever possible to reload this item?
         * Only the base item is considered with any mods ignored
         * @see player::can_reload()
         */
        bool is_reloadable() const;
        /** Returns true if this item can be reloaded with specified ammo type, ignoring capacity. */
        bool can_reload_with( const itype_id &ammo ) const;
        /** Returns true if this item can be reloaded with specified ammo type at this moment. */
        bool is_reloadable_with( const itype_id &ammo ) const;
        /** Returns true if not empty if it's liquid, it's not currently frozen in resealable container */
        bool can_unload_liquid() const;

        bool is_dangerous() const; // Is it an active grenade or something similar that will hurt us?

        /** Is item derived from a zombie? */
        bool is_tainted() const;

        /**
         * Is this item flexible enough to be worn on body parts like antlers?
         */
        bool is_soft() const;

        /**
         * Does the item provide the artifact effect when it is wielded?
         */
        bool has_effect_when_wielded( art_effect_passive effect ) const;
        /**
         * Does the item provide the artifact effect when it is worn?
         */
        bool has_effect_when_worn( art_effect_passive effect ) const;
        /**
         * Does the item provide the artifact effect when it is carried?
         */
        bool has_effect_when_carried( art_effect_passive effect ) const;

        /**
         * Set the snippet text (description) of this specific item, using the snippet library.
         * @see snippet_library.
         */
        void set_snippet( const std::string &snippet_id );

        bool operator<( const item &other ) const;
        /** List of all @ref components in printable form, empty if this item has
         * no components */
        std::string components_to_string() const;

        /** return the unique identifier of the items underlying type */
        itype_id typeId() const;

        /**
         * Return a contained item (if any and only one).
         */
        const item &get_contained() const;
        /**
         * Unloads the item's contents.
         * @param c Character who receives the contents.
         *          If c is the player, liquids will be handled, otherwise they will be spilled.
         * @return If the item is now empty.
         */
        bool spill_contents( Character &c );
        /**
         * Unloads the item's contents.
         * @param pos Position to dump the contents on.
         * @return If the item is now empty.
         */
        bool spill_contents( const tripoint &pos );

        /** Checks if item is a holster and currently capable of storing obj
         *  @param obj object that we want to holster
         *  @param ignore only check item is compatible and ignore any existing contents
         */
        bool can_holster( const item &obj, bool ignore = false ) const;

        /**
         * Callback when a character starts wearing the item. The item is already in the worn
         * items vector and is called from there.
         */
        void on_wear( Character &p );
        /**
         * Callback when a character takes off an item. The item is still in the worn items
         * vector but will be removed immediately after the function returns
         */
        void on_takeoff( Character &p );
        /**
         * Callback when a player starts wielding the item. The item is already in the weapon
         * slot and is called from there.
         * @param p player that has started wielding item
         * @param mv number of moves *already* spent wielding the weapon
         */
        void on_wield( player &p, int mv = 0 );
        /**
         * Callback when a player starts carrying the item. The item is already in the inventory
         * and is called from there. This is not called when the item is added to the inventory
         * from worn vector or weapon slot. The item is considered already carried.
         */
        void on_pickup( Character &p );
        /**
         * Callback when contents of the item are affected in any way other than just processing.
         */
        void on_contents_changed();

        /**
         * Callback immediately **before** an item is damaged
         * @param qty maximum damage that will be applied (constrained by @ref max_damage)
         * @param dt type of damage (or DT_NULL)
         */
        void on_damage( int qty, damage_type dt );

        /**
         * Name of the item type (not the item), with proper plural.
         * This is only special when the item itself has a special name ("name" entry in
         * @ref item_tags) or is a named corpse.
         * It's effectively the same as calling @ref nname with the item type id. Use this when
         * the actual item is not meant, for example "The shovel" instead of "Your shovel".
         * Or "The jacket is too small", when it applies to all jackets, not just the one the
         * character tried to wear).
         */
        std::string type_name( unsigned int quantity = 1 ) const;

        /**
         * Number of (charges of) this item that fit into the given volume.
         * May return 0 if not even one charge fits into the volume. Only depends on the *type*
         * of this item not on its current charge count.
         *
         * For items not counted by charges, this returns vol / this->volume().
         */
        int charges_per_volume( const units::volume &vol ) const;

        /**
         * @name Item variables
         *
         * Item variables can be used to store any value in the item. The storage is persistent,
         * it remains through saving & loading, it is copied when the item is moved etc.
         * Each item variable is referred to by its name, so make sure you use a name that is not
         * already used somewhere.
         * You can directly store integer, floating point and string values. Data of other types
         * must be converted to one of those to be stored.
         * The set_var functions override the existing value.
         * The get_var function return the value (if the variable exists), or the default value
         * otherwise.  The type of the default value determines which get_var function is used.
         * All numeric values are returned as doubles and may be cast to the desired type.
         * <code>
         * int v = itm.get_var("v", 0); // v will be an int
         * double d = itm.get_var("v", 0.0); // d will be a double
         * std::string s = itm.get_var("v", ""); // s will be a std::string
         * // no default means empty string as default:
         * auto n = itm.get_var("v"); // v will be a std::string
         * </code>
         */
        /*@{*/
        void set_var( const std::string &name, int value );
        // Acceptable to use long as part of overload set
        // NOLINTNEXTLINE(cata-no-long)
        void set_var( const std::string &name, long value );
        void set_var( const std::string &name, double value );
        double get_var( const std::string &name, double default_value ) const;
        void set_var( const std::string &name, const tripoint &value );
        tripoint get_var( const std::string &name, const tripoint &default_value ) const;
        void set_var( const std::string &name, const std::string &value );
        std::string get_var( const std::string &name, const std::string &default_value ) const;
        /** Get the variable, if it does not exists, returns an empty string. */
        std::string get_var( const std::string &name ) const;
        /** Whether the variable is defined at all. */
        bool has_var( const std::string &name ) const;
        /** Erase the value of the given variable. */
        void erase_var( const std::string &name );
        /** Removes all item variables. */
        void clear_vars();
        /*@}*/

        /**
         * @name Item flags
         *
         * If you use any new flags, add a comment to doc/JSON_FLAGS.md and make sure your new
         * flag does not conflict with any existing flag.
         *
         * Item flags are taken from the item type (@ref itype::item_tags), but also from the
         * item itself (@ref item_tags). The item has the flag if it appears in either set.
         *
         * Gun mods that are attached to guns also contribute their flags to the gun item.
         */
        /*@{*/
        bool has_flag( const std::string &flag ) const;
        bool has_any_flag( const std::vector<std::string> &flags ) const;

        /** Idempotent filter setting an item specific flag. */
        item &set_flag( const std::string &flag );

        /** Idempotent filter removing an item specific flag */
        item &unset_flag( const std::string &flag );

        /** Removes all item specific flags. */
        void unset_flags();
        /*@}*/

        /**Does this item have the specified fault*/
        bool has_fault( fault_id fault ) const;

        /**
         * @name Item properties
         *
         * Properties are specific to an item type so unlike flags the meaning of a property
         * may not be the same for two different item types. Each item type can have multiple
         * properties however duplicate property names are not permitted.
         *
         */
        /*@{*/
        bool has_property( const std::string &prop ) const;
        /**
          * Get typed property for item.
          * Return same type as the passed default value, or string where no default provided
          */
        std::string get_property_string( const std::string &prop, const std::string &def = "" ) const;
        int64_t get_property_int64_t( const std::string &prop, int64_t def = 0 ) const;
        /*@}*/

        /**
         * @name Light emitting items
         *
         * Items can emit light either through the definition of their type
         * (@ref itype::light_emission) or through an item specific light data (@ref light).
         */
        /*@{*/
        /**
         * Directional light emission of the item.
         * @param luminance The amount of light (see lightmap.cpp)
         * @param width If greater 0, the light is emitted in an arc, this is the angle of it.
         * @param direction The direction of the light arc. In degrees.
         */
        bool getlight( float &luminance, int &width, int &direction ) const;
        /**
         * How much light (see lightmap.cpp) the item emits (it's assumed to be circular).
         */
        int getlight_emit() const;
        /**
         * Whether the item emits any light at all.
         */
        bool is_emissive() const;
        /*@}*/

        /**
         * @name Seed data.
         */
        /*@{*/
        /**
         * Whether this is actually a seed, the seed functions won't be of much use for non-seeds.
         */
        bool is_seed() const;
        /**
         * Time it takes to grow from one stage to another. There are 4 plant stages:
         * seed, seedling, mature and harvest. Non-seed items return 0.
         */
        time_duration get_plant_epoch() const;
        /**
         * The name of the plant as it appears in the various informational menus. This should be
         * translated. Returns an empty string for non-seed items.
         */
        std::string get_plant_name() const;
        /*@}*/

        /**
         * @name Armor related functions.
         *
         * The functions here refer to values from @ref islot_armor. They only apply to armor items,
         * those items can be worn. The functions are safe to call for any item, for non-armor they
         * return a default value.
         */
        /*@{*/
        /**
         * Whether this item (when worn) covers the given body part.
         */
        bool covers( body_part bp ) const;
        /**
         * Bitset of all covered body parts.
         *
         * If the bit is set, the body part is covered by this
         * item (when worn). The index of the bit should be a body part, for example:
         * @code if( some_armor.get_covered_body_parts().test( bp_head ) ) { ... } @endcode
         * For testing only a single body part, use @ref covers instead. This function allows you
         * to get the whole covering data in one call.
         */
        body_part_set get_covered_body_parts() const;
        /**
        * Bitset of all covered body parts, from a specific side.
        *
        * If the bit is set, the body part is covered by this
        * item (when worn). The index of the bit should be a body part, for example:
        * @code if( some_armor.get_covered_body_parts().test( bp_head ) ) { ... } @endcode
        * For testing only a single body part, use @ref covers instead. This function allows you
        * to get the whole covering data in one call.
        *
        * @param s Specifies the side. Will be ignored for non-sided items.
        */
        body_part_set get_covered_body_parts( side s ) const;
        /**
          * Returns true if item is armor and can be worn on different sides of the body
          */
        bool is_sided() const;
        /**
         *  Returns side item currently worn on. Returns BOTH if item is not sided or no side currently set
         */
        side get_side() const;
        /**
          * Change the side on which the item is worn. Returns false if the item is not sided
          */
        bool set_side( side s );

        /**
         * Swap the side on which the item is worn. Returns false if the item is not sided
         */
        bool swap_side();
        /**
         * Returns the warmth value that this item has when worn. See player class for temperature
         * related code, or @ref player::warmth. Returned values should be positive. A value
         * of 0 indicates no warmth from this item at all (this is also the default for non-armor).
         */
        int get_warmth() const;
        /**
         * Returns the @ref islot_armor::thickness value, or 0 for non-armor. Thickness is are
         * relative value that affects the items resistance against bash / cutting damage.
         */
        int get_thickness() const;
        /**
         * Returns clothing layer for item.
         */
        layer_level get_layer() const;
        /**
         * Returns the relative coverage that this item has when worn.
         * Values range from 0 (not covering anything, or no armor at all) to
         * 100 (covering the whole body part). Items that cover more are more likely to absorb
         * damage from attacks.
         */
        int get_coverage() const;
        /**
         * Returns the encumbrance value that this item has when worn by given
         * player, when containing a particular volume of contents.
         * Returns 0 if this can not be worn at all.
         */
        int get_encumber_when_containing(
            const Character &, const units::volume &contents_volume ) const;
        /**
         * Returns the encumbrance value that this item has when worn by given
         * player.
         * Returns 0 if this is can not be worn at all.
         */
        int get_encumber( const Character & ) const;
        /**
         * Returns the storage amount (@ref islot_armor::storage) that this item provides when worn.
         * For non-armor it returns 0. The storage amount increases the volume capacity of the
         * character that wears the item.
         */
        units::volume get_storage() const;
        /**
         * Returns the weight capacity modifier (@ref islot_armor::weight_capacity_modifier) that this item provides when worn.
         * For non-armor it returns 1. The modifier is multiplied with the weight capacity of the character that wears the item.
         */
        float get_weight_capacity_modifier() const;
        /**
         * Returns the weight capacity bonus (@ref islot_armor::weight_capacity_modifier) that this item provides when worn.
         * For non-armor it returns 0. The bonus is added to the total weight capacity of the character that wears the item.
         */
        units::mass get_weight_capacity_bonus() const;
        /**
         * Returns the resistance to environmental effects (@ref islot_armor::env_resist) that this
         * item provides when worn. See @ref player::get_env_resist. Higher values are better.
         * For non-armor it returns 0.
         *
         * @param override_base_resist Pass this to artifically increase the
         * base resistance, so that the function can take care of other
         * modifications to resistance for you. Note that this parameter will
         * never decrease base resistnace.
         */
        int get_env_resist( int override_base_resist = 0 ) const;
        /**
         * Returns the base resistance to environmental effects if an item (for example a gas mask)
         * requires a gas filter to operate and this filter is installed. Used in iuse::gasmask to
         * change protection of a gas mask if it has (or don't has) filters. For other applications
         * use get_env_resist() above.
         */
        int get_base_env_resist_w_filter() const;
        /**
         * Whether this is a power armor item. Not necessarily the main armor, it could be a helmet
         * or similar.
         */
        bool is_power_armor() const;
        /**
         * If this is an armor item, return its armor data. You should probably not use this function,
         * use the various functions above (like @ref get_storage) to access armor data directly.
         */
        const islot_armor *find_armor_data() const;
        /**
         * Returns true whether this item can be worn only when @param it is worn.
         */
        bool is_worn_only_with( const item &it ) const;

        /**
         * @name Pet armor related functions.
         *
         * The functions here refer to values from @ref islot_pet_armor. They only apply to pet
         * armor items, those items can be worn by pets. The functions are safe to call for any
         * item, for non-pet armor they return a default value.
         */
        units::volume get_pet_armor_max_vol() const;
        units::volume get_pet_armor_min_vol() const;
        bodytype_id get_pet_armor_bodytype() const;
        /*@}*/

        /**
         * @name Books
         *
         * Book specific functions, apply to items that are books.
         */
        /*@{*/
        /**
         * How many chapters the book has (if any). Will be 0 if the item is not a book, or if it
         * has no chapters at all.
         * Each reading will "consume" a chapter, if the book has no unread chapters, it's less fun.
         */
        int get_chapters() const;
        /**
         * Get the number of unread chapters. If the item is no book or has no chapters, it returns 0.
         * This is a per-character setting, different characters may have different number of
         * unread chapters.
         */
        int get_remaining_chapters( const player &u ) const;
        /**
         * Mark one chapter of the book as read by the given player. May do nothing if the book has
         * no unread chapters. This is a per-character setting, see @ref get_remaining_chapters.
         */
        void mark_chapter_as_read( const player &u );
        /**
         * Enumerates recipes available from this book and the skill level required to use them.
         */
        std::vector<std::pair<const recipe *, int>> get_available_recipes( const player &u ) const;
        /*@}*/

        /**
         * @name Martial art techniques
         *
         * See martialarts.h for further info.
         */
        /*@{*/
        /**
         * Whether the item supports a specific martial art technique (either through its type, or
         * through its individual @ref techniques).
         */
        bool has_technique( const matec_id &tech ) const;
        /**
         * Returns all the martial art techniques that this items supports.
         */
        std::set<matec_id> get_techniques() const;
        /**
         * Add the given technique to the item specific @ref techniques. Note that other items of
         * the same type are not affected by this.
         */
        void add_technique( const matec_id &tech );
        /*@}*/

        /** Returns all toolmods currently attached to this item (always empty if item not a tool) */
        std::vector<item *> toolmods();
        std::vector<const item *> toolmods() const;

        /**
         * @name Gun and gunmod functions
         *
         * Gun and gun mod functions. Anything stated to apply to guns, applies to auxiliary gunmods
         * as well (they are some kind of gun). Non-guns are items that are neither gun nor
         * auxiliary gunmod.
         */
        /*@{*/
        bool is_gunmod() const;

        /**
         *  Can this item be used to perform a ranged attack?
         *  @see item::is_melee()
         *  @note an item can be both a gun and melee weapon concurrently
         */
        bool is_gun() const;

        /** Quantity of energy currently loaded in tool or battery */
        units::energy energy_remaining() const;

        /** Quantity of ammunition currently loaded in tool, gun or auxiliary gunmod */
        int ammo_remaining() const;
        /** Maximum quantity of ammunition loadable for tool, gun or auxiliary gunmod */
        int ammo_capacity() const;
        /** @param potential_capacity whether to try a default magazine if necessary */
        int ammo_capacity( bool potential_capacity ) const;
        /** Quantity of ammunition consumed per usage of tool or with each shot of gun */
        int ammo_required() const;

        /**
         * Check if sufficient ammo is loaded for given number of uses.
         *
         * Check if there is enough ammo loaded in a tool for the given number of uses
         * or given number of gun shots.  Using this function for this check is preferred
         * because we expect to add support for items consuming multiple ammo types in
         * the future.  Users of this function will not need to be refactored when this
         * happens.
         *
         * @param[in] qty Number of uses
         * @returns true if ammo sufficient for number of uses is loaded, false otherwise
         */
        bool ammo_sufficient( int qty = 1 ) const;

        /**
         * Consume ammo (if available) and return the amount of ammo that was consumed
         * @param qty maximum amount of ammo that should be consumed
         * @param pos current location of item, used for ejecting magazines and similar effects
         * @return amount of ammo consumed which will be between 0 and qty
         */
        int ammo_consume( int qty, const tripoint &pos );

        /** Specific ammo data, returns nullptr if item is neither ammo nor loaded with any */
        const itype *ammo_data() const;
        /** Specific ammo type, returns "null" if item is neither ammo nor loaded with any */
        itype_id ammo_current() const;
        /** Set of ammo types (@ref ammunition_type) used by item
         *  @param conversion whether to include the effect of any flags or mods which convert the type
         *  @return empty set if item does not use a specific ammo type (and is consequently not reloadable) */
        const std::set<ammotype> &ammo_types( bool conversion = true ) const;

        /** Ammo type of an ammo item
         *  @return ammotype of ammo item or a null id if the item is not ammo */
        ammotype ammo_type() const;

        /** Get default ammo used by item or "NULL" if item does not have a default ammo type
         *  @param conversion whether to include the effect of any flags or mods which convert the type
         *  @return NULL if item does not use a specific ammo type (and is consequently not reloadable) */
        itype_id ammo_default( bool conversion = true ) const;

        /** Get default ammo for the first ammotype common to an item and its current magazine or "NULL" if none exists
         * @param conversion whether to include the effect of any flags or mods which convert the type
         * @return itype_id of default ammo for the first ammotype common to an item and its current magazine or "NULL" if none exists */
        itype_id common_ammo_default( bool conversion = true ) const;

        /** Get ammo effects for item optionally inclusive of any resulting from the loaded ammo */
        std::set<std::string> ammo_effects( bool with_ammo = true ) const;

        /* Get the name to be used when sorting this item by ammo type */
        std::string ammo_sort_name() const;

        /** How many spent casings are contained within this item? */
        int casings_count() const;

        /** Apply predicate to each contained spent casing removing it if predicate returns true */
        void casings_handle( const std::function<bool( item & )> &func );

        /** Does item have an integral magazine (as opposed to allowing detachable magazines) */
        bool magazine_integral() const;

        /** Get the default magazine type (if any) for the current effective ammo type
         *  @param conversion whether to include the effect of any flags or mods which convert item's ammo type
         *  @return magazine type or "null" if item has integral magazine or no magazines for current ammo type */
        itype_id magazine_default( bool conversion = true ) const;

        /** Get compatible magazines (if any) for this item
         *  @param conversion whether to include the effect of any flags or mods which convert item's ammo type
         *  @return magazine compatibility which is always empty if item has integral magazine
         *  @see item::magazine_integral
         */
        std::set<itype_id> magazine_compatible( bool conversion = true ) const;

        /** Currently loaded magazine (if any)
         *  @return current magazine or nullptr if either no magazine loaded or item has integral magazine
         *  @see item::magazine_integral
         */
        item *magazine_current();
        const item *magazine_current() const;

        /** Normalizes an item to use the new magazine system. Idempotent if item already converted.
         *  @return items that were created as a result of the conversion (excess ammo or magazines) */
        std::vector<item> magazine_convert();

        /** Returns all gunmods currently attached to this item (always empty if item not a gun) */
        std::vector<item *> gunmods();
        std::vector<const item *> gunmods() const;

        /** Get first attached gunmod matching type or nullptr if no such mod or item is not a gun */
        item *gunmod_find( const itype_id &mod );
        const item *gunmod_find( const itype_id &mod ) const;

        /*
         * Checks if mod can be applied to this item considering any current state (jammed, loaded etc.)
         * @param msg message describing reason for any incompatibility
         */
        ret_val<bool> is_gunmod_compatible( const item &mod ) const;

        /** Get all possible modes for this gun inclusive of any attached gunmods */
        std::map<gun_mode_id, gun_mode> gun_all_modes() const;

        /** Check if gun supports a specific mode returning an invalid/empty mode if not */
        gun_mode gun_get_mode( const gun_mode_id &mode ) const;

        /** Get the current mode for this gun (or an invalid mode if item is not a gun) */
        gun_mode gun_current_mode() const;

        /** Get id of mode a gun is currently set to, e.g. DEFAULT, AUTO, BURST */
        gun_mode_id gun_get_mode_id() const;

        /** Try to set the mode for a gun, returning false if no such mode is possible */
        bool gun_set_mode( const gun_mode_id &mode );

        /** Switch to the next available firing mode */
        void gun_cycle_mode();

        /** Get lowest dispersion of either integral or any attached sights */
        int sight_dispersion() const;

        struct sound_data {
            /** Volume of the sound. Can be 0 if the gun is silent (or not a gun at all). */
            int volume;
            /** Sound description, can be used with @ref sounds::sound, it is already translated. */
            std::string sound;
        };
        /**
         * Returns the sound of the gun being fired.
         * @param burst Whether the gun was fired in burst mode (the sound string is usually different).
         */
        sound_data gun_noise( bool burst = false ) const;
        /** Whether this is a (nearly) silent gun (a tiny bit of sound is allowed). Non-guns are always silent. */
        bool is_silent() const;

        /**
         * The weapons range in map squares. If the item has an active gunmod, it returns the range
         * of that gunmod, the guns range is returned only when the item has no active gunmod.
         * This function applies to guns and auxiliary gunmods. For other items, 0 is returned.
         * It includes the range given by the ammo.
         * @param u The player that uses the weapon, their strength might affect this.
         * It's optional and can be null.
         */
        int gun_range( const player *p ) const;
        /**
         * Summed range value of a gun, including values from mods. Returns 0 on non-gun items.
         */
        int gun_range( bool with_ammo = true ) const;

        /**
         *  Get effective recoil considering handling, loaded ammo and effects of attached gunmods
         *  @param p player stats such as STR can alter effective recoil
         *  @param bipod whether any bipods should be considered
         *  @return effective recoil (per shot) or zero if gun uses ammo and none is loaded
         */
        int gun_recoil( const player &p, bool bipod = false ) const;

        /**
         * Summed ranged damage, armor piercing, and multipliers for both, of a gun, including values from mods.
         * Returns empty instance on non-gun items.
         */
        damage_instance gun_damage( bool with_ammo = true ) const;
        /**
         * Summed dispersion of a gun, including values from mods. Returns 0 on non-gun items.
         */
        int gun_dispersion( bool with_ammo = true, bool with_scaling = true ) const;
        /**
         * The skill used to operate the gun. Can be "null" if this is not a gun.
         */
        skill_id gun_skill() const;

        /** Get the type of a ranged weapon (e.g. "rifle", "crossbow"), or empty string if non-gun */
        gun_type_type gun_type() const;

        /** Get mod locations, including those added by other mods */
        std::map<gunmod_location, int> get_mod_locations() const;
        /**
         * Number of mods that can still be installed into the given mod location,
         * for non-guns it always returns 0.
         */
        int get_free_mod_locations( const gunmod_location &location ) const;
        /**
         * Does it require gunsmithing tools to repair.
         */
        bool is_firearm() const;
        /**
         * Returns the reload time of the gun. Returns 0 if not a gun.
         */
        int get_reload_time() const;
        /*@}*/

        /**
         * @name Vehicle parts
         *
         *@{*/

        /** for combustion engines the displacement (cc) */
        int engine_displacement() const;
        /*@}*/

        /**
         * @name Bionics / CBMs
         * Functions specific to CBMs
         */
        /*@{*/
        /**
         * Whether the CBM is an upgrade to another bionic module
         */
        bool is_upgrade() const;
        /*@}*/

        /**
         * Returns the pointer to use_function with name use_name assigned to the type of
         * this item or any of its contents. Checks contents recursively.
         * Returns nullptr if not found.
         */
        const use_function *get_use( const std::string &use_name ) const;
        /**
         * Checks this item and its contents (recursively) for types that have
         * use_function with type use_name. Returns the first item that does have
         * such type or nullptr if none found.
         */
        item *get_usable_item( const std::string &use_name );

        /**
         * How many units (ammo or charges) are remaining?
         * @param ch character responsible for invoking the item
         * @param limit stop searching after this many units found
         * @note also checks availability of UPS charges if applicable
         */
        int units_remaining( const Character &ch, int limit = INT_MAX ) const;

        /**
         * Check if item has sufficient units (ammo or charges) remaining
         * @param ch Character to check (used if ammo is UPS charges)
         * @param qty units required, if unspecified use item default
         */
        bool units_sufficient( const Character &ch, int qty = -1 ) const;
        /**
         * Returns name of deceased being if it had any or empty string if not
         **/
        std::string get_corpse_name();
        /**
         * Returns the translated item name for the item with given id.
         * The name is in the proper plural form as specified by the
         * quantity parameter. This is roughly equivalent to creating an item instance and calling
         * @ref tname, however this function does not include strings like "(fresh)".
         */
        static std::string nname( const itype_id &id, unsigned int quantity = 1 );
        /**
         * Returns the item type of the given identifier. Never returns null.
         */
        static const itype *find_type( const itype_id &type );
        /**
         * Whether the item is counted by charges, this is a static wrapper
         * around @ref count_by_charges, that does not need an items instance.
         */
        static bool count_by_charges( const itype_id &id );
        /**
         * Check whether the type id refers to a known type.
         * This should be used either before instantiating an item when it's possible
         * that the item type is unknown and the caller can do something about it (e.g. the
         * uninstall-bionics function checks this to see if there is a CBM item type and has
         * logic to handle the case when that item type does not exist).
         * Or one can use this to check that type ids from json refer to valid items types (e.g.
         * the items that make up the vehicle parts must be defined somewhere, or the result of
         * crafting recipes must be valid type ids).
         */
        static bool type_is_defined( const itype_id &id );

        /**
        * Returns true if item has "item_label" itemvar
        */
        bool has_label() const;
        /**
        * Returns label from "item_label" itemvar and quantity
        */
        std::string label( unsigned int quantity = 0 ) const;

        bool has_infinite_charges() const;

        /** Puts the skill in context of the item */
        skill_id contextualize_skill( const skill_id &id ) const;

        /* remove a monster from this item, optionally spawning the monster */
        int release_monster( const tripoint &target, bool spawn = true );
        /* add the monster at target to this item, despawning it */
        int contain_monster( const tripoint &target );

        time_duration age() const;
        void set_age( const time_duration &age );
        time_point birthday() const;
        void set_birthday( const time_point &bday );
        void handle_pickup_ownership( Character &c );
        int get_gun_ups_drain() const;
        inline void set_old_owner( const faction *temp_owner ) {
            old_owner = temp_owner;
        }
        inline void remove_old_owner() {
            old_owner = nullptr;
        }
        inline void set_owner( faction *new_owner ) {
            owner = new_owner;
        }
        inline void remove_owner() {
            owner = nullptr;
        }
        inline const faction *get_owner() const {
            if( owner ) {
                return owner;
            }
            return nullptr;
        }
        inline const faction *get_old_owner() const {
            if( old_owner ) {
                return old_owner;
            }
            return nullptr;
        }
        inline bool has_owner() const {
            return owner;
        }
        int get_min_str() const;

        const cata::optional<islot_comestible> &get_comestible() const;

        /**
         * Get the stored recipe for in progress crafts.
         * Causes a debugmsg if called on a non-craft and returns the null recipe.
         * @return the recipe in progress
         */
        const recipe &get_making() const;

        /**
         * Get the failure point stored in this item.
         * returns INT_MAX if the failure point is unset.
         * Causes a debugmsg and returns INT_MAX if called on a non-craft.
         * @return an integer >= 0 representing a percent to 5 decimal places.
         *         67.32 percent would be represented as 6732000
         */
        int get_next_failure_point() const;

        /**
         * Calculates and sets the next failure point for an in progress craft.
         * Causes a debugmsg if called on non-craft.
         * @param crafter the crafting player
         */
        void set_next_failure_point( const player &crafter );

        /**
         * Handle failure during crafting.
         * Destroy components, lose progress, and set a new failure point.
         * @param crafter the crafting player.
         */
        void handle_craft_failure( player &crafter );

        /**
         * Returns requirement data representing what is needed to resume work on an in progress craft.
         * Causes a debugmsg and returns empty requirement data if called on a non-craft
         * @return what is needed to continue craft, may be empty requirement data
         */
        requirement_data get_continue_reqs() const;

        void set_tools_to_continue( bool value );
        bool has_tools_to_continue() const;
        void set_cached_tool_selections( const std::vector<comp_selection<tool_comp>> &selections );
        const std::vector<comp_selection<tool_comp>> &get_cached_tool_selections() const;

    private:
        /**
         * Calculate the thermal energy and temperature change of the item
         * @param temp Temperature of surroundings
         * @param insulation Amount of insulation item has
         * @param time time point which the item is processed to
         */
        void calc_temp( int temp, float insulation, const time_point &time );

        /**
         * Get the thermal energy of the item in Joules.
         */
        float get_item_thermal_energy();

        /** Calculates item specific energy (J/g) from temperature (K)*/
        float get_specific_energy_from_temperature( float new_temperature );

        /** Helper for checking reloadability. **/
        bool is_reloadable_helper( const itype_id &ammo, bool now ) const;

    public:
        enum class sizing {
            human_sized_human_char = 0,
            big_sized_human_char,
            small_sized_human_char,
            big_sized_big_char,
            human_sized_big_char,
            small_sized_big_char,
            small_sized_small_char,
            human_sized_small_char,
            big_sized_small_char,
            not_wearable
        };

        sizing get_sizing( const Character &, bool ) const;

    protected:
        // Sub-functions of @ref process, they handle the processing for different
        // processing types, just to make the process function cleaner.
        // The interface is the same as for @ref process.
        bool process_corpse( player *carrier, const tripoint &pos );
        bool process_wet( player *carrier, const tripoint &pos );
        bool process_litcig( player *carrier, const tripoint &pos );
        bool process_extinguish( player *carrier, const tripoint &pos );
        // Place conditions that should remove fake smoke item in this sub-function
        bool process_fake_smoke( player *carrier, const tripoint &pos );
        bool process_fake_mill( player *carrier, const tripoint &pos );
        bool process_cable( player *p, const tripoint &pos );
        bool process_blackpowder_fouling( player *carrier );
        bool process_tool( player *carrier, const tripoint &pos );

    public:
        static const int INFINITE_CHARGES;

        const itype *type;
        std::list<item> contents;
        std::list<item> components;
        /** What faults (if any) currently apply to this item */
        std::set<fault_id> faults;
        cata::flat_set<std::string> item_tags; // generic item specific flags

    private:
        safe_reference_anchor anchor;
        const itype *curammo = nullptr;
        std::map<std::string, std::string> item_vars;
        const mtype *corpse = nullptr;
        std::string corpse_name;       // Name of the late lamented
        std::set<matec_id> techniques; // item specific techniques

        // Only for in-progress crafts
        const recipe *making = nullptr;
        int next_failure_point = -1;
        std::vector<item_comp> comps_used;
        // If the crafter has insufficient tools to continue to the next 5% progress step
        bool tools_to_continue = false;
        std::vector<comp_selection<tool_comp>> cached_tool_selections;

    public:
        int charges;
        units::energy energy;      // Amount of energy currently stored in a battery

        int recipe_charges = 1;    // The number of charges a recipe creates.
        int burnt = 0;             // How badly we're burnt
        int poison = 0;            // How badly poisoned is it?
        int frequency = 0;         // Radio frequency
        int note = 0;              // Associated dynamic text snippet.
        int irradiation = 0;       // Tracks radiation dosage.
        int item_counter = 0;      // generic counter to be used with item flags
        int specific_energy = -10; // Specific energy (0.00001 J/g). Negative value for unprocessed.
        int temperature = 0;       // Temperature of the item (in 0.00001 K).
        int mission_id = -1;       // Refers to a mission in game's master list
        int player_id = -1;        // Only give a mission to the right player!

    private:
        /**
         * Accumulated rot, expressed as time the item has been in standard temperature.
         * It is compared to shelf life (@ref islot_comestible::spoils) to decide if
         * the item is rotten.
         */
        time_duration rot = 0_turns;
        /** Time when the rot calculation was last performed. */
        time_point last_rot_check = calendar::turn_zero;
        /** the last time the temperature was updated for this item */
        time_point last_temp_check = calendar::turn_zero;
        /// The time the item was created.
        time_point bday;
        /**
         * Current phase state, inherits a default at room temperature from
         * itype and can be changed through item processing.  This is a static
         * cast to avoid importing the entire enums.h header here, zero is
         * PNULL.
         */
        phase_id current_phase = static_cast<phase_id>( 0 );
        // The faction that owns this item.
        const faction *owner = nullptr;
        // The faction that previously owned this item
        const faction *old_owner = nullptr;
        int damage_ = 0;
        light_emission light = nolight;

    public:
        char invlet = 0;      // Inventory letter
        bool active = false; // If true, it has active effects to be processed
        bool is_favorite = false;

        void set_favorite( bool favorite );
        bool has_clothing_mod() const;
        float get_clothing_mod_val( clothing_mod_type type ) const;
        void update_clothing_mod_val();
};

bool item_compare_by_charges( const item &left, const item &right );
bool item_ptr_compare_by_charges( const item *left, const item *right );

/**
 *  Hint value used in a hack to decide text color.
 *
 *  This is assigned as a result of some legacy logic in @ref draw_item_info().  This
 *  will eventually be rewritten to eliminate the need for this hack.
 */
enum hint_rating {
    /** Item should display as gray */
    HINT_CANT = 0,
    /** Item should display as red */
    HINT_IFFY = 1,
    /** Item should display as green */
    HINT_GOOD = -999
};

/**
 * Returns a reference to a null item (see @ref item::is_null). The reference is always valid
 * and stays valid until the program ends.
 */
item &null_item_reference();

/**
 * Default filter for crafting component searches
 */
inline bool is_crafting_component( const item &component )
{
    return ( component.allow_crafting_component() || component.count_by_charges() ) &&
           !component.is_filthy();
}

#endif
