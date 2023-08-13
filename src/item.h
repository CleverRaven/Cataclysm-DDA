#pragma once
#ifndef CATA_SRC_ITEM_H
#define CATA_SRC_ITEM_H

#include <algorithm>
#include <climits>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <list>
#include <map>
#include <new>
#include <optional>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

#include "calendar.h"
#include "cata_utility.h"
#include "compatibility.h"
#include "enums.h"
#include "gun_mode.h"
#include "io_tags.h"
#include "item_components.h"
#include "item_contents.h"
#include "item_location.h"
#include "item_pocket.h"
#include "material.h"
#include "requirements.h"
#include "safe_reference.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"
#include "visitable.h"
#include "rng.h"

class Character;
class Creature;
class JsonObject;
class JsonOut;
class book_proficiency_bonuses;
class enchantment;
class enchant_cache;
class faction;
class gun_type_type;
class gunmod_location;
class item;
class iteminfo_query;
class monster;
class nc_color;
class recipe;
class relic;
struct part_material;
struct armor_portion_data;
struct itype_variant_data;
struct islot_comestible;
struct itype;
struct item_comp;
template<typename CompType>
struct comp_selection;
struct tool_comp;
struct mtype;
struct tripoint;
template<typename T>
class ret_val;
template <typename T> struct enum_traits;
class vehicle;

namespace enchant_vals
{
enum class mod : int;
} // namespace enchant_vals

using bodytype_id = std::string;
using faction_id = string_id<faction>;
class item_category;
struct islot_armor;
struct use_function;

enum art_effect_passive : int;
enum class side : int;
class body_part_set;
class map;
struct damage_instance;
struct damage_unit;
struct fire_data;
enum class link_state : int;

enum clothing_mod_type : int;

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

        /** Same as dValue, adjusted for the minimum unit (for numerical comparisons) */
        double dUnitAdjustedVal;

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

        /** Flag indicating decimal with three points of precision.  */
        bool three_decimal;

        enum flags {
            no_flags = 0,
            is_decimal = 1 << 0, ///< Print as decimal rather than integer
            is_three_decimal = 1 << 1, ///< Print as decimal with three points of precision
            no_newline = 1 << 2, ///< Do not follow with a newline
            lower_is_better = 1 << 3, ///< Lower values are better for this stat
            no_name = 1 << 4, ///< Do not print the name
            show_plus = 1 << 5, ///< Use a + sign for positive values
        };

        /**
         *  @param Type The item type of the item this iteminfo belongs to.
         *  @param Name The name of the property this iteminfo describes.
         *  @param Fmt Formatting text desired between item name and value
         *  @param Flags Additional flags to customize this entry
         *  @param Value Numerical value of this property, -999 for none.
         */
        iteminfo( const std::string &Type, const std::string &Name, const std::string &Fmt = "",
                  flags Flags = no_flags, double Value = -999, double UnitVal = 0 );
        iteminfo( const std::string &Type, const std::string &Name, flags Flags );
        iteminfo( const std::string &Type, const std::string &Name, double Value, double UnitVal = 0 );
};

template<>
struct enum_traits<iteminfo::flags> {
    static constexpr bool is_flag_enum = true;
};

iteminfo vol_to_info( const std::string &type, const std::string &left,
                      const units::volume &vol, int decimal_places = 2, bool lower_is_better = true );
iteminfo weight_to_info( const std::string &type, const std::string &left,
                         const units::mass &weight, int decimal_places = 2, bool lower_is_better = true );

inline bool is_crafting_component( const item &component );

class item : public visitable
{
    public:
        using FlagsSetType = std::set<flag_id>;

        item();

        item( item && ) noexcept( map_is_noexcept );
        item( const item & );
        item &operator=( item && ) noexcept( list_is_noexcept );
        item &operator=( const item & );

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
        item( const recipe *rec, int qty, item_components items, std::vector<item_comp> selections );

        /** For constructing in-progress disassemblies */
        item( const recipe *rec, int qty, item &component );

        // Legacy constructor for constructing from string rather than itype_id
        // TODO: remove this and migrate code using it.
        template<typename... Args>
        explicit item( const std::string &itype, Args &&... args ) :
            item( itype_id( itype ), std::forward<Args>( args )... )
        {}

        ~item() override;

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
         * Invoke use function on a thrown item that had "ACT_ON_RANGED_HIT" flag.
         * The function is called on the spot where the item landed.
         * @param pos position
         * @return true if the item was destroyed (exploded)
         */
        bool activate_thrown( const tripoint &pos );

        /**
         * Add or remove energy from a battery.
         * If adding the specified energy quantity would go over the battery's capacity fill
         * the battery and ignore the remainder.
         * If adding the specified energy quantity would reduce the battery's charge level
         * below 0 do nothing and return how far below 0 it would have gone.
         * @param qty energy quantity to add (can be negative)
         * @return 0 valued energy quantity on success
         */
        units::energy mod_energy( const units::energy &qty );

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
        * Sets item damage constrained by [@ref degradation and @ref max_damage]
        */
        void set_damage( int qty );

        /**
        * Sets item's degradation constrained by [0 and @ref max_damage]
        * If item damage is lower it is raised up to @ref degradation
        */
        void set_degradation( int qty );

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
                                 time_point turn = calendar::turn, const std::string &name = "", int upgrade_time = -1 );
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
        bool ready_to_revive( map &here, const tripoint &pos ) const;

        bool is_money() const;
        bool is_software() const;
        bool is_software_storage() const;

        bool is_ebook_storage() const;

        /**
         * A heuristic on whether it's a good idea to use this as a melee weapon.
         * Used for nicer messages only.
         */
        bool is_maybe_melee_weapon() const;

        /**
         * Returns whether this weapon does any damage type suitable for diamond coating.
         */
        bool has_edged_damage() const;

        /**
         * Returns a symbol for indicating the current dirt or fouling level for a gun.
         */
        std::string dirt_symbol() const;

        /**
         * Returns a symbol indicating the current degradation of the item.
         */
        std::string degradation_symbol() const;

        /**
         * Returns the default color of the item (e.g. @ref itype::color).
         */
        nc_color color() const;
        /**
         * Returns the color of the item depending on usefulness for the player character,
         * e.g. differently if it its an unread book or a spoiling food item etc.
         * This should only be used for displaying data, it should not affect game play.
         */
        nc_color color_in_inventory( const Character *ch = nullptr ) const;
        /**
         * Return the (translated) item name.
         * @param quantity used for translation to the proper plural form of the name, e.g.
         * returns "rock" for quantity 1 and "rocks" for quantity > 0.
         * @param with_prefix determines whether to include more item properties, such as
         * the extent of damage and burning (was created to sort by name without prefix
         * in additional inventory)
         * @param with_contents determines whether to add a suffix with the full name of the contents
         * of this item (if with_contents = false and item is not empty, "n items" will be added)
         */
        std::string tname( unsigned int quantity = 1, bool with_prefix = true,
                           unsigned int truncate = 0, bool with_contents_full = true,
                           bool with_collapsed = true, bool with_contents_abbrev = true ) const;
        std::string display_money( unsigned int quantity, unsigned int total,
                                   const std::optional<unsigned int> &selected = std::nullopt ) const;
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
         * @param iteminfo The properties (encapsulated into @ref iteminfo) are added to this vector,
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
        * @param iteminfo The properties (encapsulated into @ref iteminfo) are added to this vector,
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
        * @param info The properties (encapsulated into @ref iteminfo) are added to this vector,
        * the vector can be used to compare them to properties of another item.
        * @param batch The batch crafting number to multiply data by
        */
        std::string info( std::vector<iteminfo> &info, const iteminfo_query *parts = nullptr,
                          int batch = 1 ) const;
        /* type specific helper functions for info() that should probably be in itype() */
        void basic_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                         bool debug ) const;
        void debug_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                         bool debug ) const;
        void med_info( const item *med_item, std::vector<iteminfo> &info, const iteminfo_query *parts,
                       int batch, bool debug ) const;
        void food_info( const item *food_item, std::vector<iteminfo> &info, const iteminfo_query *parts,
                        int batch, bool debug ) const;
        void rot_info( const item *food_item, std::vector<iteminfo> &info, const iteminfo_query *parts,
                       int batch, bool debug ) const;
        void magazine_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                            bool debug ) const;
        void ammo_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                        bool debug ) const;
        void gun_info( const item *mod, std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                       bool debug ) const;
        void gunmod_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                          bool debug ) const;
        void armor_protection_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                                    bool debug, const sub_bodypart_id &sbp = sub_bodypart_id() ) const;
        void armor_material_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                                  bool debug, const sub_bodypart_id &sbp = sub_bodypart_id() ) const;
        void armor_attribute_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                                   bool debug, const sub_bodypart_id &sbp = sub_bodypart_id() ) const;
        void pet_armor_protection_info( std::vector<iteminfo> &info, const iteminfo_query *parts ) const;
        void armor_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                         bool debug ) const;
        void animal_armor_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                                bool debug ) const;
        void armor_fit_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                             bool debug ) const;
        void book_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                        bool debug ) const;
        void battery_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                           bool debug ) const;
        void tool_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                        bool debug ) const;
        void actions_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                           bool debug ) const;
        void component_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                             bool debug ) const;
        void enchantment_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                               bool debug ) const;
        void repair_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                          bool debug ) const;
        void disassembly_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                               bool debug ) const;
        void qualities_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                             bool debug ) const;
        void bionic_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                          bool debug ) const;
        void melee_combat_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                                bool debug ) const;
        void contents_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                            bool debug ) const;
        void properties_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                              bool debug ) const;
        void ascii_art_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                             bool debug ) const;
        void final_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                         bool debug ) const;

        /**
         * Calculate all burning calculations, but don't actually apply them to item.
         * DO apply them to @ref fire_data argument, though.
         * @return Amount of "burn" that would be applied to the item.
         */
        float simulate_burn( fire_data &frd ) const;
        /** Burns the item. Returns true if the item was destroyed. */
        bool burn( fire_data &frd );

        // Returns the category of this item, regardless of contents.
        const item_category &get_category_shallow() const;
        // Returns the category of item inside this item.
        // "can of meat" would be food, instead of container.
        // If there are multiple items/stacks or none then it defaults to category of this item.
        const item_category &get_category_of_contents() const;

        class reload_option
        {
            public:
                reload_option() = default;

                reload_option( const reload_option & );
                reload_option &operator=( const reload_option & );

                reload_option( const Character *who, const item_location &target, const item_location &ammo );

                const Character *who = nullptr;
                item_location target;
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
        };

        /**
         * Reload item using ammo from location returning true if successful
         * @param u Player doing the reloading
         * @param ammo Location of ammo to be reloaded
         * @param qty caps reloading to this (or fewer) units
         */
        bool reload( Character &u, item_location ammo, int qty );
        // is this speedloader compatible with this item?
        bool allows_speedloader( const itype_id &speedloader_id ) const;

        template<typename Archive>
        void io( Archive & );
        using archive_type_tag = io::object_archive_tag;

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &data );

        const std::string &symbol() const;
        /**
         * Returns the monetary value of an item.
         * If `practical` is false, returns pre-Cataclysm market value,
         * otherwise returns approximate post-cataclysm value.
         */
        int price( bool practical ) const;

        /**
         * Returns the monetary value of an item by itself.
         * If `practical` is false, returns pre-Cataclysm market value,
         * otherwise returns approximate post-cataclysm value.
         */
        int price_no_contents( bool practical, std::optional<int> price_override = std::nullopt ) const;

        /**
         * Whether two items should stack when displayed in a inventory menu.
         * This is different from stacks_with, when two previously non-stackable
         * items are now stackable and mergeable because, for example, they
         * reaches the same temperature. This is necessary to avoid misleading
         * stacks like "3 items-count-by-charge (5)".
         */
        bool display_stacked_with( const item &rhs, bool check_components = false ) const;
        bool stacks_with( const item &rhs, bool check_components = false,
                          bool combine_liquid = false, int depth = 0, int maxdepth = 2 ) const;

        /**
         * Whether the two items have same contents.
         * Checks the contents and the contents of the contents.
         */
        bool same_contents( const item &rhs ) const;

        /**
         * Whether item is the same as `rhs` for RLE compression purposes.
         * Essentially a stricter version of `stacks_with`.
         * @return true if items are same, i.e. *this is "equal" to `item(rhs)`
         * @note false negative results are OK
         * @note must be transitive
         */
        bool same_for_rle( const item &rhs ) const;
        /** combines two items together if possible. returns false if it fails. */
        bool combine( const item &rhs );
        bool can_combine( const item &rhs ) const;
        /**
         * Merge charges of the other item into this item.
         * @return true if the items have been merged, otherwise false.
         * Merging is only done for items counted by charges (@ref count_by_charges) and
         * items that stack together (@ref stacks_with).
         */
        bool merge_charges( const item &rhs );

        /**
        * Total weight of an item accounting for all contained/integrated items
        * @param include_contents if true include weight of contained items
        * @param integral if true return effective weight if this item was integrated into another
        */
        units::mass weight( bool include_contents = true, bool integral = false ) const;

        /**
         * Total volume of an item accounting for all contained/integrated items
         * NOTE: Result is rounded up to next nearest milliliter when working with stackable (@ref count_by_charges) items that have fractional volume per charge.
         * If trying to determine how many of an item can fit in a given space, @ref charges_per_volume should be used instead.
         * @param integral if true return effective volume if this item was integrated into another
         * @param ignore_contents if true return effective volume for the item alone, ignoring its contents
         * @param charges_in_vol if specified, get the volume for this many charges instead of current charges
         */
        units::volume volume( bool integral = false, bool ignore_contents = false,
                              int charges_in_vol = -1 ) const;

        units::length length() const;
        units::length barrel_length() const;

        /**
         * Simplified, faster volume check for when processing time is important and exact volume is not.
         * NOTE: Result is rounded up to next nearest milliliter when working with stackable (@ref count_by_charges) items that have fractional volume per charge.
         * If trying to determine how many of an item can fit in a given space, @ref charges_per_volume should be used instead.
         */
        units::volume base_volume() const;

        /** Volume check for corpses, helper for base_volume(). */
        units::volume corpse_volume( const mtype *corpse ) const;

        /**
         * Volume to subtract when the item is collapsed or folded.
         * @result positive value when the item increases in volume when wielded and 0 if no change.
         */
        units::volume collapsed_volume_delta() const;

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
        int attack_time( const Character &you ) const;

        /** Damage of given type caused when this item is used as melee weapon */
        int damage_melee( const damage_type_id &dt ) const;

        /** All damage types this item deals when used in melee (no skill modifiers etc. applied). */
        damage_instance base_damage_melee() const;
        /** All damage types this item deals when thrown (no skill modifiers etc. applied). */
        damage_instance base_damage_thrown() const;

        /**
        * Calculate the item's effective damage per second past armor when wielded by a
         * character against a monster.
         */
        double effective_dps( const Character &guy, Creature &mon ) const;
        /**
         * calculate effective dps against a stock set of monsters.  by default, assume g->u
         * is wielding
        * for_display - include monsters intended for display purposes
         * for_calc - include monsters intended for evaluation purposes
         * for_display and for_calc are inclusive
               */
        std::map<std::string, double> dps( bool for_display, bool for_calc, const Character &guy ) const;
        std::map<std::string, double> dps( bool for_display, bool for_calc ) const;
        /** return the average dps of the weapon against evaluation monsters */
        double average_dps( const Character &guy ) const;

        /**
         * Whether the character needs both hands to wield this item.
         */
        bool is_two_handed( const Character &guy ) const;

        /** Is this item an effective melee weapon for the given damage type? */
        bool is_melee( const damage_type_id &dt ) const;

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

        /*
         * Max range of melee attack this weapon can be used for.
         * Accounts for character's abilities and installed gun mods.
         * Guaranteed to be at least 1
         */
        int reach_range( const Character &guy ) const;

        /*
         * Max range of melee attack this weapon can be used for in its current state.
         * Accounts for character's abilities and installed gun mods.
         * Guaranteed to be at least 1
         */
        int current_reach_range( const Character &guy ) const;

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
                          const std::function<bool( const item & )> &filter = return_true<item>,
                          Character *carrier = nullptr, bool in_tools = false );

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

        // seal the item's pockets. used for crafting and spawning items.
        bool seal();

        bool all_pockets_sealed() const;
        bool any_pockets_sealed() const;
        /** Whether this is container. Note that container does not necessarily means it's
         * suitable for liquids. */
        bool is_container() const;
        /** Whether it is a container, and if it is has some restrictions */
        bool is_container_with_restriction() const;
        /** Whether it is a container with only one pocket, and if it is has some restrictions */
        bool is_single_container_with_restriction() const;
        // whether the contents has a pocket with the associated type
        bool has_pocket_type( item_pocket::pocket_type pk_type ) const;
        bool has_any_with( const std::function<bool( const item & )> &filter,
                           item_pocket::pocket_type pk_type ) const;

        /** True if every pocket is rigid or we have no pockets */
        bool all_pockets_rigid() const;

        // gets all pockets contained in this item
        std::vector<const item_pocket *> get_all_contained_pockets() const;
        std::vector<item_pocket *> get_all_contained_pockets();
        std::vector<const item_pocket *> get_all_standard_pockets() const;
        std::vector<item_pocket *> get_all_standard_pockets();
        std::vector<item_pocket *> get_all_ablative_pockets();
        std::vector<const item_pocket *> get_all_ablative_pockets() const;
        /**
         * Updates the pockets of this item to be correct based on the mods that are installed.
         * Pockets which are modified that contain an item will be spilled
         * NOTE: This assumes that there is always one and only one pocket where ammo goes (mag or mag well)
         */
        void update_modified_pockets();
        // for pocket update stuff, which pocket is @contained in?
        // returns a nullptr if the item is not contained, and prints a debug message
        item_pocket *contained_where( const item &contained );
        const item_pocket *contained_where( const item &contained ) const;
        /** Whether this is a container which can be used to store liquids. */
        bool is_watertight_container() const;
        /** Whether this item has no contents at all. */
        bool is_container_empty() const;
        /**
         * Whether this item has no more free capacity for its current content.
         * @param allow_bucket Allow filling non-sealable containers
         */
        bool is_container_full( bool allow_bucket = false ) const;

        /**
         * Whether the magazine pockets of this item have room for additional items
         */
        bool is_magazine_full() const;

        /**
         * Fill item with an item up to @amount number of items. This works for any item with container pockets.
         * @param contained item to fill the container with.
         * @param amount Amount to fill item with, capped by remaining capacity
         * @returns amount of contained that was put into it
         */
        int fill_with( const item &contained, int amount = INFINITE_CHARGES,
                       bool unseal_pockets = false,
                       bool allow_sealed = false,
                       bool ignore_settings = false,
                       bool into_bottom = false );

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

        units::volume total_contained_volume() const;

        /**
         * It returns the maximum volume of any contents, including liquids,
         * ammo, magazines, weapons, etc.
         */
        units::volume get_total_capacity( bool unrestricted_pockets_only = false ) const;
        units::mass get_total_weight_capacity( bool unrestricted_pockets_only = false ) const;

        units::volume get_remaining_capacity( bool unrestricted_pockets_only = false ) const;
        units::mass get_remaining_weight_capacity( bool unrestricted_pockets_only = false ) const;

        units::volume get_total_contained_volume( bool unrestricted_pockets_only = false ) const;
        units::mass get_total_contained_weight( bool unrestricted_pockets_only = false ) const;

        int get_used_holsters() const;
        int get_total_holsters() const;
        units::volume get_total_holster_volume() const;
        units::volume get_used_holster_volume() const;

        units::mass get_total_holster_weight() const;
        units::mass get_used_holster_weight() const;

        // recursive function that checks pockets for remaining free space
        units::volume check_for_free_space() const;
        units::volume get_selected_stack_volume( const std::map<const item *, int> &without ) const;
        bool has_unrestricted_pockets() const;
        units::volume get_contents_volume_with_tweaks( const std::map<const item *, int> &without ) const;
        units::volume get_nested_content_volume_recursive( const std::map<const item *, int> &without )
        const;

        // returns the abstract 'size' of the pocket
        // used for attaching to molle items
        int get_pocket_size() const;

        /**
         * Returns true if the item can be attached with PALS straps
         */
        bool can_attach_as_pocket() const;

        // what will the move cost be of taking @it out of this container?
        // should only be used from item_location if possible, to account for
        // player inventory handling penalties from traits
        int obtain_cost( const item &it ) const;
        // what will the move cost be of storing @it into this container? (CONTAINER pocket type)
        int insert_cost( const item &it ) const;

        /**
         * Puts the given item into this one.
         */
        ret_val<void> put_in( const item &payload, item_pocket::pocket_type pk_type,
                              bool unseal_pockets = false );
        void force_insert_item( const item &it, item_pocket::pocket_type pk_type );

        /**
         * Returns this item into its default container. If it does not have a default container,
         * returns this. It's intended to be used like \code newitem = newitem.in_its_container();\endcode
         * qty <= 0 means the current quantity of the item will be used. Any quantity exceeding the capacity
         * of the container will be ignored.
         */
        item in_its_container( int qty = 0 ) const;
        item in_container( const itype_id &container_type, int qty = 0, bool sealed = true ) const;
        void add_automatic_whitelist();
        void clear_automatic_whitelist();

        /**
        * True if item and its contents have any uses.
        * @param contents_only Set to true to ignore the item itself and check only its contents.
        */
        bool item_has_uses_recursive( bool contents_only = false ) const;

        /*@{*/
        /**
         * Funnel related functions. See weather.cpp for their usage.
         */
        bool is_funnel_container( units::volume &bigger_than ) const;
        void add_rain_to_container( int charges = 1 );
        /*@}*/

        /**
         * Returns true if this item itself is of at least quality level
         * This version is non-recursive and does not check the item contents.
         */
        bool has_quality_nonrecursive( const quality_id &qual, int level = 1 ) const;

        /**
         * Return the level of a given quality the tool may have, or INT_MIN if it
         * does not have that quality, or lacks enough charges to have that quality.
         * This version is non-recursive and does not check the item contents.
         * @param strict_boiling True if containers must be empty to have BOIL quality
         */
        int get_quality_nonrecursive( const quality_id &id, bool strict_boiling = true ) const;

        /**
         * Return the level of a given quality the tool may have, or INT_MIN if it
         * does not have that quality, or lacks enough charges to have that quality.
         * This version is recursive and will check the item contents as well.
         * @param strict_boiling True if containers must be empty to have BOIL quality
         */
        int get_quality( const quality_id &id, bool strict_boiling = true ) const;

        /**
         * Return true if this item's type is counted by charges
         * (true for stackable, ammo, or comestible)
         */
        bool count_by_charges() const;

        /**
         * If count_by_charges(), returns charges, otherwise 1
         */
        int count() const;
        bool craft_has_charges() const;

        /**
         * Modify the charges of this item, only use for items counted by charges!
         * The item must have enough charges for this (>= quantity) and be counted
         * by charges.
         * @param mod How many charges should be removed.
         */
        void mod_charges( int mod );

        /**
         * Returns rate of rot (rot/h) at the given temperature
         */
        float calc_hourly_rotpoints_at_temp( const units::temperature &temp ) const;

        /**
         * Accumulate rot of the item since last rot calculation.
         * This function should not be called directly. since it does not have all the needed checks or temperature calculations.
         * If you need to calc rot of item call process_temperature_rot instead.
         * @param time Time point to which rot is calculated
         * @param temp Temperature at which the rot is calculated
         */
        void calc_rot( units::temperature temp, float spoil_modifier, const time_duration &time_delta );

        /**
         * This is part of a workaround so that items don't rot away to nothing if the smoking rack
         * is outside the reality bubble.
         * @param processing_duration
         */
        void calc_rot_while_processing( time_duration processing_duration );

        /**
         * Update temperature for things like food
         * Update rot for things that perish
         * All items that rot also have temperature
         * @param insulation Amount of insulation item has from surroundings
         * @param pos The current position
         * @param carrier The current carrier
         * @param flag to specify special temperature situations
         * @return true if the item is fully rotten and is ready to be removed
         */
        bool process_temperature_rot( float insulation, const tripoint &pos, map &here, Character *carrier,
                                      temperature_flag flag = temperature_flag::NORMAL, float spoil_modifier = 1.0f );

        /** Set the item to HOT and resets last_temp_check */
        void heat_up();

        /** Set the item to COLD and resets last_temp_check*/
        void cold_up();

        /** Sets the item temperature and item energy from new temperature and resets last_temp_check */
        void set_item_temperature( units::temperature new_temperature );

        /** Sets the item to new temperature and energy based new specific energy (J/g) and resets last_temp_check*/
        void set_item_specific_energy( const units::specific_energy &specific_energy );

        /**
         * Get the thermal energy of the item in Joules.
         */
        float get_item_thermal_energy() const;

        /** reset the last_temp_check used when crafting new items and the like */
        void reset_temp_check();

        int get_comestible_fun() const;

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
         * Set item @ref rot to a random value. If the item is a container
         * (such as MRE) - processes its contents recursively.
         */
        void randomize_rot();

        /**
         * Get minimum time for this item or any of its contents to rot, ignoring
         * fridge. If this item is a container, its spoil multiplier is taken into
         * account, but the spoil multiplier of the parent container of this item,
         * if any, is not.
         *
         * If this item does not rot and none of its contents rot either, the function
         * returns INT_MAX - N,
         *
         * where N is
         * 3 for food,
         * 2 for medication,
         * 1 for other comestibles,
         * 0 otherwise.
         */
        int spoilage_sort_order() const;

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

        /**
         * Whether the item has enough rot that it should get removed.
         * Regular shelf life perishable foods rot away completely at 2x shelf life. Corpses last 10 days
         * @return true if the item has enough rot to be removed, false otherwise.
         */
        bool has_rotten_away() const;

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
        const std::map<itype_id, int> &brewing_results() const;

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
         * This is the material with the highest "portion" value.
         */
        const material_type &get_base_material() const;
        /**
         * The ids of all the materials this is made of.
         * This may return an empty map.
         * The returned map does not contain the null id.
         */
        const std::map<material_id, int> &made_of() const;
        /**
         * The ids of the materials a specific portion is made of. The specific
         * portion is the part of the armour covering the specified body part.
         * This may return an empty vector.
         * The returned vector does not contain the null id.
         */
        std::vector<const part_material *> armor_made_of( const bodypart_id &bp ) const;
        std::vector<const part_material *> armor_made_of( const sub_bodypart_id &bp ) const;
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
         * @return The portion of this item made up by the material
         */
        int made_of( const material_id &mat_ident ) const;
        /**
        * Check we can repair with this material (e.g. matches at least one
        * in our set.)
        */
        bool can_repair_with( const material_id &mat_ident ) const;
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
        * Helper function to retrieve any applicable resistance bonuses from item mods.
        * @param dmg_type Damage type
        * @return Amount of additional modded resistance
        */
        float get_clothing_mod_val_for_damage_type( const damage_type_id &dmg_type ) const;

        /**
        * Helper function to check whether a damage_type can damage items for forward-declared
        * damage_type.
        */
        bool damage_type_can_damage_items( const damage_type_id &dmg_type ) const;



        /**
         * Resistance against different damage types (@ref damage_type).
         * Larger values means more resistance are thereby better, but there is no absolute value to
         * compare them to. The values can be interpreted as chance (@ref one_in) of damaging the item
         * when exposed to the type of damage.
         * @param dmg_type The type of incoming damage
         * @param to_self If this is true, it returns item's own resistance, not one it gives to wearer.
         * @param bodypart_target The bodypart_id or sub_bodypart_id of the target bodypart.
         * @param resist_value A supplied roll against coverage for physical attacks. A value of -1 causes rolls
         *                     against each item material individually. For environmental type damage such as
         *                     fire and acid, this value can be used to allow hypothetical calculations for
         *                     environmental protection items such as gas masks.
         */
        /*@{*/

        template<typename bodypart_target = bodypart_id>
        float resist( const damage_type_id &dmg_type, bool to_self = false,
                      const bodypart_target &bp = bodypart_target(),
                      int resist_value = 0 ) const;

    private:
        float _resist( const damage_type_id &dmg_type, bool to_self = false, int resist_value = 0,
                       bool bp_null = true,
                       const std::vector<const part_material *> &armor_mats = {},
                       float avg_thickness = 1.0f ) const;
        /**
         * Handles the special rules regarding environmental type damage such as fire and acid.
         * Currently does not damage items as damage to items from fire is handled elsewhere, and
         * acid is currently not intended to cause item damage.
         * @param dmg_type The type of incoming damage
         * @param to_self If this is true, it returns item's own resistance, not one it gives to wearer.
         * @param bodypart_target The bodypart_id or sub_bodypart_id of the target bodypart.
         * @param base_env_resist Will override the base environmental
         * resistance (to allow hypothetical calculations for gas masks).
         */
        float _environmental_resist( const damage_type_id &dmg_type, bool to_self = false,
                                     int base_env_resist = 0,
                                     bool bp_null = true,
                                     const std::vector<const part_material *> &armor_mats = {} ) const;
        /*@}*/
    public:

        /**
         * Breathability is the ability of a fabric to allow moisture vapor to be transmitted through the material.
         * range 0 - 100 (no breathability - full breathability)
         * takes as a value the body part to check
         * @return armor data consolidated breathability
         */
        int breathability( const bodypart_id &bp ) const;
        int breathability() const;

        /**
         * Assuming that specified du hit the armor, reduce du based on the item's resistance to the
         * damage type. This will never reduce du.amount below 0.
         * roll is normally set to 0 which means all materials protect for legacy
         * giving a roll between 0-99 will influence the covered materials with a fixed roll
         * giving a roll of -1 (< 0) will mean each material is rolled individually
         */
        void mitigate_damage( damage_unit &du, const bodypart_id &bp = bodypart_id(), int roll = 0 ) const;
        void mitigate_damage( damage_unit &du, const sub_bodypart_id &bp, int roll = 0 ) const;

        /**
         * Returns resistance to being damaged by attack against the item itself.
         * Calculated from item's materials.
         * @param worst If this is true, the worst resistance is used. Otherwise the best one.
         */
        int chip_resistance( bool worst = false, const bodypart_id &bp = bodypart_id() ) const;

        /** How much damage has the item sustained? */
        int damage() const;

        /** How much degradation has the item accumulated? */
        int degradation() const;

        // Sets a random degradation within [0, damage), used when spawning the item
        void rand_degradation();

        // @see itype::damage_level()
        int damage_level() const;

        // modifies melee weapon damage to account for item's damage
        float damage_adjusted_melee_weapon_damage( float value ) const;
        // modifies gun damage to account for item's damage
        float damage_adjusted_gun_damage( float value ) const;
        // modifies armor resist to account for item's damage
        float damage_adjusted_armor_resist( float value ) const;

        // @return 0 if item is count_by_charges() or 4000 ( value of itype::damage_max_ )
        int max_damage() const;

        /**
        * Returns how many damage levels can be repaired on this item
        * Example: item with  100 damage returns 1 (  100 -> 0 )
        * Example: item with 2100 damage returns 3 ( 2100 -> 1100 -> 100 -> 0 )
        */
        int repairable_levels() const;

        // @return 1 for undamaged items or remaining hp divided by max hp in range [0, 1)
        float get_relative_health() const;

        /**
         * Apply damage to const itemrained by @ref min_damage and @ref max_damage
         * @param qty maximum amount by which to adjust damage (negative permissible)
         * @return whether item should be destroyed
         */
        bool mod_damage( int qty );

        /**
         * Same as mod_damage( itype::damage_scale ), advances item to next damage level
         * @return whether item should be destroyed
         */
        bool inc_damage();

        enum class armor_status {
            UNDAMAGED,
            DAMAGED,
            DESTROYED,
            TRANSFORMED
        };

        /**
         * Damage related logic for armor items, wraps mod_damage with needed logic
         * This version is for items with durability
         * @return the state of the armor
         */
        armor_status damage_armor_durability( damage_unit &du, const bodypart_id &bp );

        /**
         * Damage related logic for armor items that warp and transform instead of degrading.
         * Items such as ablative plates are considered with this.
         * @return the state of the armor
         */
        armor_status damage_armor_transforms( damage_unit &du ) const;

        // @return colorize()-ed damage indicator as string, e.g. "<color_green>++</color>"
        std::string damage_indicator() const;

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
        bool already_used_by_player( const Character &p ) const;
        /**
         * Marks the item as being used by this specific player, it remains unmarked
         * for other players. The player is identified by its id.
         */
        void mark_as_used_by_player( const Character &p );
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
         * @param spoil_multiplier_parent is the spoilage multiplier passed down from any parent item
         * @return true if the item has been destroyed by the processing. The caller
         * should than delete the item wherever it was stored.
         * Returns false if the item is not destroyed.
         */
        bool process( map &here, Character *carrier, const tripoint &pos, float insulation = 1,
                      temperature_flag flag = temperature_flag::NORMAL, float spoil_multiplier_parent = 1.0f,
                      bool recursive = true );

        bool leak( map &here, Character *carrier, const tripoint &pos, item_pocket *pocke = nullptr );

        struct link_data {
            /// State of the link's source, the end usually represented by the cable item. @ref link_state.
            link_state s_state = link_state::no_link;
            /// State of the link's target, the end represented by t_abs_pos, @ref link_state.
            link_state t_state = link_state::no_link;
            /// The last turn process_link was called on this cable. Used for time the cable spends outside the bubble.
            time_point last_processed = calendar::turn;
            /// Absolute position of the linked target vehicle/appliance.
            tripoint_abs_ms t_abs_pos = tripoint_abs_ms( tripoint_min );
            /// Reality bubble position of the link's source cable item.
            tripoint s_bub_pos = tripoint_min; // NOLINT(cata-serialize)
            /// A safe reference to the link's target vehicle. Will recreate itself whenever the vehicle enters the bubble.
            safe_reference<vehicle> t_veh_safe; // NOLINT(cata-serialize)
            /// The linked part's mount offset on the target vehicle.
            point t_mount = point_zero;
            /// The current slack of the cable.
            int length = 0;
            /// The maximum length of the cable. Set during initialization.
            int max_length = 2;
            /// The cable's power capacity in watts, affects battery charge rate. Set during initialization.
            int charge_rate = 0;
            /// (this) out of 1.0 chance to successfully add 1 charge every charge interval.
            float efficiency = 0.0f;
            /// The turn interval between charges. Set during initialization.
            int charge_interval = 0;

            bool has_state( link_state state ) const {
                return s_state == state || t_state == state;
            }
            bool has_states( link_state s_state_, link_state t_state_ ) const {
                return s_state == s_state_ && t_state == t_state_;
            }
            bool has_no_links() const {
                return s_state == link_state::no_link && t_state == link_state::no_link;
            }

            void serialize( JsonOut &jsout ) const;
            void deserialize( const JsonObject &data );
        };
        /**
         * @brief Sets max_length and efficiency of a link, taking cable extensions into account.
         * @brief max_length is set to the sum of all cable lengths.
         * @brief efficiency is set to the product of all efficiencies multiplied together.
         * @param assign_t_state If true, set the t_state based on the parts at the connection point. Defaults to false.
         */
        void set_link_traits( bool assign_t_state = false );

        /**
         * @return The link's current length.
         * @return `-1` if the item has link_data but needs reeling.
         * @return `-2` if the item has no active link.
         */
        int link_length() const;

        /**
         * @return The item's maximum possible link length, including extensions. Item doesn't need an active link.
         * @return `-2` if the item has no active link or extensions.
         */
        int max_link_length() const;

        /**
         * Value used for sorting linked items in inventory lists.
         */
        int link_sort_key() const;

        /**
         * Brings a cable item back to its initial state.
         * @param p Set to character that's holding the linked item, nullptr if none.
         * @param vpart_index The index of the vehicle part the cable is attached to, so it can have `linked_flag` removed.
         * @param * At -1, the default, this function will look up the index itself. At -2, skip modifying the part's flags entirely.
         * @param loose_message If there should be a notification that the link was disconnected.
         * @param cable_position Position of the linked item, used to determine if the player can see the link becoming loose.
         * @return True if the cable should be deleted.
         */
        bool reset_link( Character *p = nullptr, int vpart_index = -1,
                         bool loose_message = false, tripoint cable_position = tripoint_zero );

        /**
        * @brief Exchange power between an item's batteries and the vehicle/appliance it's linked to.
        * @brief A positive link.charge_rate will charge the item at the expense of the vehicle,
        * while a negative link.charge_rate will charge the vehicle at the expense of the item.
        *
        * @param linked_veh The vehicle the item is connected to.
        * @param turns_elapsed The number of turns the link has spent outside the reality bubble. Default 1.
        * @return The amount of power given or taken to be displayed; ignores turns_elapsed and inefficiency.
        */
        int charge_linked_batteries( vehicle &linked_veh, int turns_elapsed = 1 );

        /**
         * Whether the item should be processed (by calling @ref process).
         */
        bool needs_processing() const;
        /**
         * The rate at which an item should be processed, in number of turns between updates.
         */
        int processing_speed() const;
        static constexpr int NO_PROCESSING = 10000;
        /**
         * Process and apply artifact effects. This should be called exactly once each turn, it may
         * modify character stats (like speed, strength, ...), so call it after those have been reset.
         * @param carrier The character carrying the artifact, can be null.
         * @param pos The location of the artifact (should be the player location if carried).
         */
        void process_relic( Character *carrier, const tripoint &pos );

        void overwrite_relic( const relic &nrelic );

        // Most of the is_whatever() functions call the same function in our itype
        bool is_null() const; // True if type is NULL, or points to the null item (id == 0)
        bool is_comestible() const;
        bool is_food() const;                // Ignoring the ability to eat batteries, etc.
        bool is_food_container() const;      // Ignoring the ability to eat batteries, etc.
        bool is_ammo_container() const; // does this item contain ammo? (excludes magazines)
        bool is_medication() const;            // Is it a medication that only pretends to be food?
        bool is_medical_tool() const;
        bool is_bionic() const;
        bool is_magazine() const;
        bool is_battery() const;
        bool is_vehicle_battery() const;
        bool is_ammo_belt() const;
        bool is_holster() const;
        bool is_ammo() const;
        // is this armor for a pet creature?  if on_pet is true, returns false if a pet isn't
        // wearing it
        bool is_pet_armor( bool on_pet = false ) const;
        bool is_armor() const;
        bool is_book() const;
        bool is_map() const;
        bool is_salvageable() const;
        bool is_disassemblable() const;
        bool is_craft() const;

        bool is_deployable() const;
        bool is_tool() const;
        bool is_transformable() const;
        bool is_relic() const;
        bool is_same_relic( item const &rhs ) const;
        bool is_bucket_nonempty() const;

        bool is_brewable() const;
        bool is_engine() const;
        bool is_wheel() const;
        bool is_fuel() const;
        bool is_toolmod() const;

        bool is_irremovable() const;

        /** Returns true if the item is broken and can't be activated or used in crafting */
        bool is_broken() const;

        /** Returns true if the item is broken or will be broken on activation */
        bool is_broken_on_active() const;

        bool has_temperature() const;

        /** Returns true if the item is A: is SOLID and if it B: is of type LIQUID */
        bool is_frozen_liquid() const;

        /** Returns empty string if the book teach no skill */
        std::string get_book_skill() const;

        //** Returns specific heat of the item (J/g K) */
        float get_specific_heat_liquid() const;
        float get_specific_heat_solid() const;

        /** Returns latent heat of the item (J/g) */
        float get_latent_heat() const;

        units::temperature get_freeze_point() const;

        void set_last_temp_check( const time_point &pt );

        /** How resistant clothes made of this material are to wind (0-100) */
        int wind_resist() const;

        /** What faults can potentially occur with this item? */
        std::set<fault_id> faults_potential() const;

        /** Returns the total area of this wheel or 0 if it isn't one. */
        int wheel_area() const;

        /** Returns energy of single unit of this item as fuel for an engine. */
        units::energy fuel_energy() const;
        /** Returns the string of the id of the terrain that pumps this fuel, if any. */
        std::string fuel_pump_terrain() const;
        bool has_explosion_data() const;
        fuel_explosion_data get_explosion_data() const;

        /**
         * returns whether any of the pockets is compatible with the specified item.
         * Does not check if the item actually fits volume/weight wise
         * Only checks CONTAINER, MAGAZINE and MAGAZINE WELL pockets
         * @param it the item being put in
         */
        ret_val<void> is_compatible( const item &it ) const;

        /**
         * Can the pocket contain the specified item?
         * @param it the item being put in
         * @param nested whether or not the current call is nested (used recursively).
         * @param ignore_pkt_settings whether to ignore pocket autoinsert settings
         * @param remaining_parent_volume the ammount of space in the parent pocket,
         * @param allow_nested whether nested pockets should be checked
         * needed to make sure we dont try to nest items which can't fit in the nested pockets
         */
        /*@{*/
        ret_val<void> can_contain( const item &it, bool nested = false,
                                   bool ignore_rigidity = false,
                                   bool ignore_pkt_settings = true,
                                   const item_location &parent_it = item_location(),
                                   units::volume remaining_parent_volume = 10000000_ml,
                                   bool allow_nested = true ) const;
        ret_val<void> can_contain( const item &it, int &copies_remaining, bool nested = false,
                                   bool ignore_rigidity = false,
                                   bool ignore_pkt_settings = true,
                                   const item_location &parent_it = item_location(),
                                   units::volume remaining_parent_volume = 10000000_ml,
                                   bool allow_nested = true ) const;
        bool can_contain( const itype &tp ) const;
        bool can_contain_partial( const item &it ) const;
        ret_val<void> can_contain_directly( const item &it ) const;
        bool can_contain_partial_directly( const item &it ) const;
        /*@}*/
        std::pair<item_location, item_pocket *> best_pocket( const item &it, item_location &this_loc,
                const item *avoid = nullptr, bool allow_sealed = false, bool ignore_settings = false,
                bool nested = false, bool ignore_rigidity = false );

        units::length max_containable_length( bool unrestricted_pockets_only = false ) const;
        units::length min_containable_length() const;
        units::volume max_containable_volume() const;

        /**
         * Is it ever possible to reload this item?
         * ALso checks for reloading installed gunmods
         * @see player::can_reload()
         */
        bool is_reloadable() const;

        /**
         * returns whether the item can be reloaded with the specified item.
         * @param ammo item to be loaded in
         * @param now whether the currently contained ammo/magazine should be taken into account
         */
        bool can_reload_with( const item &ammo, bool now ) const;

        /**
          * Returns true if any of the contents are not frozen or not empty if it's liquid
          */
        bool can_unload_liquid() const;

        /**
         * Returns true if none of the contents are solid
         */
        bool contains_no_solids() const;

        bool is_dangerous() const; // Is it an active grenade or something similar that will hurt us?

        /** Is item derived from a zombie? */
        bool is_tainted() const;

        /**
         * Is this item flexible enough to be worn on body parts like antlers?
         */
        bool is_soft() const;

        // is any bit of the armor rigid
        bool is_rigid() const;
        // is any bit of the armor comfortable
        bool is_comfortable() const;
        template <typename T>
        bool is_bp_rigid( const T &bp ) const;
        // check if rigid and that it only cares about the layer it is on
        template <typename T>
        bool is_bp_rigid_selective( const T &bp ) const;
        template <typename T>
        bool is_bp_comfortable( const T &bp ) const;

        /**
         * Set the snippet text (description) of this specific item, using the snippet library.
         * @see snippet_library.
         */
        void set_snippet( const snippet_id &id );

        bool operator<( const item &other ) const;
        /** List of all @ref components in printable form, empty if this item has
         * no components */
        std::string components_to_string() const;

        /** Creates a hash from the itype_ids of this item's @ref components. */
        uint64_t make_component_hash() const;

        /** return the unique identifier of the items underlying type */
        itype_id typeId() const;

        /**
          * if the item will spill if placed into a container
          */
        bool will_spill() const;
        bool will_spill_if_unsealed() const;
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
        bool spill_open_pockets( Character &guy, const item *avoid = nullptr );
        // spill items that don't fit in the container
        void overflow( const tripoint &pos, const item_location &loc = item_location::nowhere );

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
         * Calculate (but do not deduct) the number of moves required to wield this weapon
         */
        int on_wield_cost( const Character &you ) const;

        /**
         * Callback when a player starts wielding the item. The item is already in the weapon
         * slot and is called from there.
         * @param p player that has started wielding item
         * @param mv number of moves *already* spent wielding the weapon
         */
        void on_wield( Character &you );
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

        bool use_relic( Character &guy, const tripoint &pos );
        bool has_relic_recharge() const;
        bool has_relic_activation() const;
        std::vector<trait_id> mutations_from_wearing( const Character &guy, bool removing = false ) const;

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
        int charges_per_weight( const units::mass &m ) const;

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
        void set_var( const std::string &name, long long value );
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

        struct extended_photo_def {
            int quality = 0;
            std::string name;
            std::string description;

            void deserialize( const JsonObject &obj );
            void serialize( JsonOut &jsout ) const;
        };
        bool read_extended_photos( std::vector<extended_photo_def> &extended_photos,
                                   const std::string &var_name, bool insert_at_begin ) const;
        void write_extended_photos( const std::vector<extended_photo_def> &, const std::string & );

        /**
         * @name Item flags
         *
         * If you use any new flags, add them to `flags.json`,
         * add a comment to doc/JSON_FLAGS.md and make sure your new
         * flag does not conflict with any existing flag.
         *
         * Item flags are taken from the item type (@ref itype::item_tags), but also from the
         * item itself (@ref item_tags). The item has the flag if it appears in either set.
         *
         * Gun mods that are attached to guns also contribute their flags to the gun item.
         */
        /*@{*/
        bool has_flag( const flag_id &flag ) const;

        template<typename Container, typename T = std::decay_t<decltype( *std::declval<const Container &>().begin() )>>
        bool has_any_flag( const Container &flags ) const {
            return std::any_of( flags.begin(), flags.end(), [&]( const T & flag ) {
                return has_flag( flag );
            } );
        }

        /**
         * Checks whether item itself has given flag (doesn't check item type or gunmods).
         * Essentially get_flags().count(f).
         * Works faster than `has_flag`
        */
        bool has_own_flag( const flag_id &f ) const;

        /** returns read-only set of flags of this item (not including flags from item type or gunmods) */
        const FlagsSetType &get_flags() const;

        /** Idempotent filter setting an item specific flag. */
        item &set_flag( const flag_id &flag );

        /** Idempotent filter removing an item specific flag */
        item &unset_flag( const flag_id &flag );

        /** Idempotent filter recursively setting an item specific flag on this item and its components. */
        item &set_flag_recursive( const flag_id &flag );

        /** Removes all item specific flags. */
        void unset_flags();
        /*@}*/

        /**Does this item have the specified fault*/
        bool has_fault( const fault_id &fault ) const;

        /** Does this item part have a fault with this flag */
        bool has_fault_flag( const std::string &searched_flag ) const;

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
        bool getlight( float &luminance, units::angle &width, units::angle &direction ) const;
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
        bool covers( const bodypart_id &bp ) const;
        /**
         * Whether this item (when worn) covers the given sub body part.
         */
        bool covers( const sub_bodypart_id &bp ) const;
        // do both items overlap a bodypart at all? returns the side that conflicts via rhs
        std::optional<side> covers_overlaps( const item &rhs ) const;
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
         * returns a vector of all the sub_body_parts of this item
         */
        std::vector<sub_bodypart_id> get_covered_sub_body_parts() const;

        /**
         * returns a vector of all the sub_body_parts of this item accounting for a specific side
         */
        std::vector<sub_bodypart_id> get_covered_sub_body_parts( side s ) const;

        /**
         * returns true if the item has armor and if it has sub location coverage
         */
        bool has_sublocations() const;

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
         * Returns if the armor has ablative pockets
         */
        bool is_ablative() const;
        /**
         * Returns if the armor has pockets with additional encumbrance
         */
        bool has_additional_encumbrance() const;
        /**
         * Returns if the armor has pockets with a chance to be ripped off
         */
        bool has_ripoff_pockets() const;
        /**
         * Returns if the armor has pockets with a chance to make noise when moving
         */
        bool has_noisy_pockets() const;
        /**
         * Returns the warmth value that this item has when worn. See player class for temperature
         * related code, or @ref player::warmth. Returned values should be positive. A value
         * of 0 indicates no warmth from this item at all (this is also the default for non-armor).
         */
        int get_warmth() const;
        /** Returns the warmth on the body part of the item on a specific bp. */
        int get_warmth( const bodypart_id &bp ) const;
        /**
         * Returns the @ref islot_armor::thickness value, or 0 for non-armor. Thickness is are
         * relative value that affects the items resistance against bash / cutting / bullet damage.
         */
        float get_thickness() const;
        /**
         * Returns the average thickness value for the specified bodypart, or 0 for non-armor. Thickness is a
         * relative value that affects the items resistance against bash / cutting / bullet damage.
         */
        float get_thickness( const bodypart_id &bp ) const;
        /**
        * Returns the average thickness value for the specified sub-bodypart, or 0 for non-armor. Thickness
        * is a relative value that affects the items resistance against bash / cutting / bullet damage.
        */
        float get_thickness( const sub_bodypart_id &bp ) const;
        /**
         * Returns clothing layer for item.
         */
        std::vector<layer_level> get_layer() const;

        /**
         * Returns clothing layer for body part.
         */
        std::vector<layer_level> get_layer( bodypart_id bp ) const;

        /**
         * Returns clothing layer for sub bodypart .
         */
        std::vector<layer_level> get_layer( sub_bodypart_id sbp ) const;

        /**
         * Returns true if an item has a given layer level on a specific part.
         * matches to any layer within the vector input.
         */
        bool has_layer( const std::vector<layer_level> &ll, const bodypart_id &bp ) const;

        /**
         * Returns true if an item has a given layer level on a specific subpart.
         */
        bool has_layer( const std::vector<layer_level> &ll, const sub_bodypart_id &sbp ) const;

        /**
         * Returns true if an item has any of the given layer levels.
         */
        bool has_layer( const std::vector<layer_level> &ll ) const;

        /**
         * Returns highest layer of an item
         */
        layer_level get_highest_layer( const sub_bodypart_id &sbp ) const;

        enum class cover_type {
            COVER_DEFAULT,
            COVER_MELEE,
            COVER_RANGED,
            COVER_VITALS
        };
        static cover_type get_cover_type( const damage_type_id &type );

        /*
         * Returns the average coverage of each piece of data this item
         */
        int get_avg_coverage( const cover_type &type = cover_type::COVER_DEFAULT ) const;
        /**
         * Returns the highest coverage that any piece of data that this item has that covers the bodypart.
         * Values range from 0 (not covering anything) to 100 (covering the whole body part).
         * Items that cover more are more likely to absorb damage from attacks.
         */
        int get_coverage( const bodypart_id &bodypart,
                          const cover_type &type = cover_type::COVER_DEFAULT ) const;

        int get_coverage( const sub_bodypart_id &bodypart,
                          const cover_type &type = cover_type::COVER_DEFAULT ) const;

        enum class encumber_flags : int {
            none = 0,
            assume_full = 1,
            assume_empty = 2
        };

        const armor_portion_data *portion_for_bodypart( const bodypart_id &bodypart ) const;

        const armor_portion_data *portion_for_bodypart( const sub_bodypart_id &bodypart ) const;

        /**
         * Returns the average encumbrance value that this item across all portions
         * Returns 0 if this is can not be worn at all.
         */
        int get_avg_encumber( const Character &, encumber_flags = encumber_flags::none ) const;

        /**
         * Returns the encumbrance value that this item has when worn by given
         * player.
         * Returns 0 if this is can not be worn at all.
         */
        int get_encumber( const Character &, const bodypart_id &bodypart,
                          encumber_flags = encumber_flags::none ) const;

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
         * @param override_base_resist Pass this to artificially increase the
         * base resistance, so that the function can take care of other
         * modifications to resistance for you. Note that this parameter will
         * never decrease base resistance.
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
        * Returns true wether this item is worn or not
        */
        bool is_worn_by_player() const;

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
         * translates the vector of proficiency bonuses into the container. returns an empty object if it's not a book
         */
        book_proficiency_bonuses get_book_proficiency_bonuses() const;
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
        int get_remaining_chapters( const Character &u ) const;
        /**
         * Mark one chapter of the book as read by the given player. May do nothing if the book has
         * no unread chapters. This is a per-character setting, see @ref get_remaining_chapters.
         */
        void mark_chapter_as_read( const Character &u );
        /**
         * Returns recipes stored on the item (laptops, smartphones, sd cards etc)
         * Filters out !is_valid() recipes
         */
        std::set<recipe_id> get_saved_recipes() const;
        /**
        * Sets recipes stored on the item (laptops, smartphones, sd cards etc)
        */
        void set_saved_recipes( const std::set<recipe_id> &recipes );
        /**
         * Enumerates recipes available from this book and the skill level required to use them.
         */
        std::vector<std::pair<const recipe *, int>> get_available_recipes( const Character &u ) const;
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

        /**
         * Does this item have a gun variant associated with it
         * If check_option, the return of this is dependent on the SHOW_GUN_VARIANTS option
         */
        bool has_itype_variant( bool check_option = true ) const;

        /**
         * The gun variant associated with this item
         */
        const itype_variant_data &itype_variant() const;

        /**
         * Set the gun variant of this item
         */
        void set_itype_variant( const std::string &variant );

        void clear_itype_variant();

        /**
         * Quantity of shots in the gun. Looks at both ammo and available energy.
         * @param carrier is used for UPS and bionic power
         */
        int shots_remaining( const Character *carrier ) const;

        /**
         * Energy available from battery/UPS/bionics
         * @param carrier is used for UPS and bionic power.
         */
        units::energy energy_remaining( const Character *carrier = nullptr ) const;


        /**
         * Quantity of ammunition currently loaded in tool, gun or auxiliary gunmod.
         * @param carrier is used for UPS and bionic power for tools
         * @param include_linked Add cable-linked vehicles' ammo to the ammo count
         */
        int ammo_remaining( const Character *carrier = nullptr, bool include_linked = false ) const;
        int ammo_remaining( bool include_linked ) const;

        /**
         * ammo capacity for a specific ammo
         * @param ammo The ammo type to get the capacity for
         * @param include_linked If linked up, return linked electricity grid's capacity
         */
        int ammo_capacity( const ammotype &ammo, bool include_linked = false ) const;

        /**
         * how much more ammo can fit into this item
         * if this item is not loaded, gives remaining capacity of its default ammo
         */
        int remaining_ammo_capacity() const;

        /** Quantity of ammunition consumed per usage of tool or with each shot of gun */
        int ammo_required() const;
        // gets the first ammo in all magazine pockets
        // does not support multiple magazine pockets!
        item &first_ammo();
        // gets the first ammo in all magazine pockets
        // does not support multiple magazine pockets!
        const item &first_ammo() const;
        // spills liquid and other contents from the container. contents may remain
        // in the container if the player cancels spilling. removing liquid from
        // a magazine requires unload logic.
        void handle_liquid_or_spill( Character &guy, const item *avoid = nullptr );

        /**
         * Check if sufficient ammo is loaded for given number of uses.
         * Check if there is enough ammo loaded in a tool for the given number of uses
         * or given number of gun shots.
         * If carrier is provides then UPS and bionic may be also used as ammo
         * Using this function for this check is preferred
         * because we expect to add support for items consuming multiple ammo types in
         * the future.  Users of this function will not need to be refactored when this
         * happens.
         *
         * @param carrier who holds the item. Needed for UPS/bionic
         * @param qty Number of uses
         * @returns true if ammo sufficient for number of uses is loaded, false otherwise
         */
        bool ammo_sufficient( const Character *carrier, int qty = 1 ) const;
        bool ammo_sufficient( const Character *carrier, const std::string &method, int qty = 1 ) const;

        /**
         * Consume ammo (if available) and return the amount of ammo that was consumed
         * Consume order: loaded items, UPS, bionic
         * @param qty maximum amount of ammo that should be consumed
         * @param pos current location of item, used for ejecting magazines and similar effects
         * @param carrier holder of the item, used for getting UPS and bionic power
         * @param fuel_efficiency if this is a generator of some kind the efficiency at which it consumes fuel
         * @return amount of ammo consumed which will be between 0 and qty
         */
        int ammo_consume( int qty, const tripoint &pos, Character *carrier );

        /**
         * Consume energy (if available) and return the amount of energy that was consumed
         * Consume order: battery, UPS, bionic
         * When consuming energy from batteries the consumption will round up by adding 1 kJ. More energy may be consumed than required.
         * @param qty amount of energy that should be consumed
         * @param pos current location of item, used for ejecting magazines and similar effects
         * @param carrier holder of the item, used for getting UPS and bionic power
         * @return amount of energy consumed which will be between 0 kJ and qty+1 kJ
         */
        units::energy energy_consume( units::energy qty, const tripoint &pos, Character *carrier,
                                      float fuel_efficiency = -1.0 );

        /**
         * Consume ammo to activate item qty times (if available) and return the amount of ammo that was consumed
         * Consume order: loaded items, UPS, bionic
         * @param qty number of times to consume item activation charges
         * @param pos current location of item, used for ejecting magazines and similar effects
         * @param carrier holder of the item, used for getting UPS and bionic power
         * @return amount of ammo consumed which will be between 0 and qty
         */
        int activation_consume( int qty, const tripoint &pos, Character *carrier );

        /** Specific ammo data, returns nullptr if item is neither ammo nor loaded with any */
        const itype *ammo_data() const;
        /** Specific ammo type, returns "null" if item is neither ammo nor loaded with any */
        itype_id ammo_current() const;
        /** Get currently loaded ammo, if any.
         * @return item reference or null item if not loaded. */
        const item &loaded_ammo() const;
        /** Ammo type of an ammo item
         *  @return ammotype of ammo item or a null id if the item is not ammo */
        ammotype ammo_type() const;

        /** Ammo types (@ref ammunition_type) the item magazine pocket can contain.
         *  @param conversion whether to include the effect of any flags or mods which convert the type
         *  @return empty set if item does not have a magazine for a specific ammo type */
        std::set<ammotype> ammo_types( bool conversion = true ) const;
        /** Default ammo for the the item magazine pocket, if item has ammo_types().
         *  @param conversion whether to include the effect of any flags or mods which convert the type
         *  @return itype_id::NULL_ID() if item does have a magazine for a specific ammo type */
        itype_id ammo_default( bool conversion = true ) const;
        // format a string with all the ammo that this mag can use
        std::string print_ammo( ammotype at, const item *gun = nullptr ) const;

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

        /** Can item load ammo like a magazine (has magazine pocket) */
        bool magazine_integral() const;

        /** Does item have magazine well */
        bool uses_magazine() const;

        /** Get the default magazine type (if any) for the current effective ammo type
         *  @param conversion whether to include the effect of any flags or mods which convert item's ammo type
         *  @return magazine type or "null" if item has integral magazine or no magazines for current ammo type */
        itype_id magazine_default( bool conversion = false ) const;

        /** Get compatible magazines (if any) for this item
         *  @return magazine compatibility which is always empty if item has integral magazine
         *  @see item::magazine_integral
         */
        std::set<itype_id> magazine_compatible() const;

        /** Currently loaded magazine (if any)
         *  @return current magazine or nullptr if either no magazine loaded or item has integral magazine
         *  @see item::magazine_integral
         */
        item *magazine_current();
        const item *magazine_current() const;

        /** Returns all gunmods currently attached to this item (always empty if item not a gun) */
        std::vector<item *> gunmods();
        std::vector<const item *> gunmods() const;

        std::vector<const item *> mods() const;

        std::vector<const item *> softwares() const;

        std::vector<const item *> ebooks() const;

        std::vector<const item *> cables() const;

        /** Get first attached gunmod matching type or nullptr if no such mod or item is not a gun */
        item *gunmod_find( const itype_id &mod );
        const item *gunmod_find( const itype_id &mod ) const;
        /** Get first attached gunmod with flag or nullptr if no such mod or item is not a gun */
        item *gunmod_find_by_flag( const flag_id &flag );

        /*
         * Checks if mod can be applied to this item considering any current state (jammed, loaded etc.)
         * @param msg message describing reason for any incompatibility
         */
        ret_val<void> is_gunmod_compatible( const item &mod ) const;

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

        /** Get lowest actual and effective dispersion of either integral or any attached sights for specific character */
        std::pair<int, int> sight_dispersion( const Character &character ) const;

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
         * @param p The player that uses the weapon, their strength might affect this.
         * It's optional and can be null.
         */
        int gun_range( const Character *p ) const;
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
        int gun_recoil( const Character &p, bool bipod = false ) const;

        /**
         * Summed ranged damage, armor piercing, and multipliers for both, of a gun, including values from mods.
         * Returns empty instance on non-gun items.
         */
        damage_instance gun_damage( bool with_ammo = true, bool shot = false ) const;
        damage_instance gun_damage( itype_id ammo ) const;
        /**
         * The minimum force required to cycle the gun, can be overridden by mods
         */
        int min_cycle_recoil() const;
        /**
         * Summed dispersion of a gun, including values from mods. Returns 0 on non-gun items.
         */
        int gun_dispersion( bool with_ammo = true, bool with_scaling = true ) const;
        /**
        * Summed shot spread from mods. Returns 0 on non-gun items.
        */
        float gun_shot_spread_multiplier() const;
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
        const item *get_usable_item( const std::string &use_name ) const;
        item *get_usable_item( const std::string &use_name );

        /**
         * Returns name of deceased being if it had any or empty string if not
         **/
        std::string get_corpse_name() const;
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

        /* Remove a monster from this item and spawn it.
         * See @game::place_critter for meaning of @p target and @p pos.
         * @return Whether the monster has been spawned (may fail if no space available).
         */
        bool release_monster( const tripoint &target, int radius = 0 );
        /* add the monster at target to this item, despawning it */
        int contain_monster( const tripoint &target );

        time_duration age() const;
        void set_age( const time_duration &age );
        time_point birthday() const;
        void set_birthday( const time_point &bday );
        void handle_pickup_ownership( Character &c );

        /**
         * Get gun energy drain. Includes modifiers from gunmods.
         * @return energy drained per shot
         */
        units::energy get_gun_energy_drain() const;
        units::energy get_gun_ups_drain() const;
        units::energy get_gun_bionic_drain() const;

        void validate_ownership() const;
        inline void set_old_owner( const faction_id &temp_owner ) {
            old_owner = temp_owner;
        }
        inline void remove_old_owner() const {
            old_owner = faction_id::NULL_ID();
        }
        void set_owner( const faction_id &new_owner );
        void set_owner( const Character &c );
        inline void remove_owner() const {
            owner = faction_id::NULL_ID();
        }
        faction_id get_owner() const;
        faction_id get_old_owner() const;
        bool is_owned_by( const Character &c, bool available_to_take = false ) const;
        bool is_old_owner( const Character &c, bool available_to_take = false ) const;
        std::string get_old_owner_name() const;
        std::string get_owner_name() const;
        int get_min_str() const;

        const cata::value_ptr<islot_comestible> &get_comestible() const;

        /**
         * Get the stored recipe for in progress crafts.
         * Causes a debugmsg if called on a non-craft and returns the null recipe.
         * @return the recipe in progress
         */
        const recipe &get_making() const;
        int get_making_batch_size() const;

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
        void set_next_failure_point( const Character &crafter );

        /**
         * Handle failure during crafting.
         * Destroy components, lose progress, and set a new failure point.
         * @param crafter the crafting player.
         * @return whether the craft being worked on should be entirely destroyed
         */
        bool handle_craft_failure( Character &crafter );

        /**
         * Returns requirement data representing what is needed to resume work on an in progress craft.
         * Causes a debugmsg and returns empty requirement data if called on a non-craft
         * @return what is needed to continue craft, may be empty requirement data
         */
        requirement_data get_continue_reqs() const;

        /**
         * @brief Inherit applicable flags from the given parent item.
         *
         * @param parent Item to inherit from
         */
        void inherit_flags( const item &parent, const recipe &making );

        /**
         * @brief Inherit applicable flags from the given list of parent items.
         *
         * @param parents Items to inherit from
         */
        void inherit_flags( const item_components &parents, const recipe &making );

        void set_tools_to_continue( bool value );
        bool has_tools_to_continue() const;
        void set_cached_tool_selections( const std::vector<comp_selection<tool_comp>> &selections );
        const std::vector<comp_selection<tool_comp>> &get_cached_tool_selections() const;

        std::vector<enchant_cache> get_proc_enchantments() const;
        std::vector<enchantment> get_defined_enchantments() const;
        double calculate_by_enchantment( const Character &owner, double modify, enchant_vals::mod value,
                                         bool round_value = false ) const;
        // calculates the enchantment value as if this item were wielded.
        double calculate_by_enchantment_wield( const Character &owner, double modify,
                                               enchant_vals::mod value,
                                               bool round_value = false ) const;

        /**
         * Compute the number of moves needed to disassemble this item and its components
         * @param guy The character performing the disassembly
         * @return The number of moves to recursively disassemble this item
         */
        int get_recursive_disassemble_moves( const Character &guy ) const;

        // inherited from visitable
        VisitResponse visit_items( const std::function<VisitResponse( item *, item * )> &func ) const
        override;
        /**
         * @relates visitable
         * NOTE: upon expansion, this may need to be filtered by type enum depending on accessibility
         */
        VisitResponse visit_contents( const std::function<VisitResponse( item *, item * )> &func,
                                      item *parent = nullptr );
        void remove_internal( const std::function<bool( item & )> &filter,
                              int &count, std::list<item> &res );
        std::list<item> remove_items_with( const std::function<bool( const item & )> &filter,
                                           int count = INT_MAX ) override;

        /** returns a list of pointers to all top-level items that are not mods */
        std::list<const item *> all_items_top() const;
        /** returns a list of pointers to all top-level items that are not mods */
        std::list<item *> all_items_top();
        /** returns a list of pointers to all top-level items */
        std::list<const item *> all_items_top( item_pocket::pocket_type pk_type ) const;
        /** returns a list of pointers to all top-level items
         *  if unloading is true it ignores items in pockets that are flagged to not unload
         */
        std::list<item *> all_items_top( item_pocket::pocket_type pk_type, bool unloading = false );

        item const *this_or_single_content() const;
        bool contents_only_one_type() const;

        /**
         * returns a list of pointers to all items inside recursively
         * includes mods.  used for item_location::unpack()
         */
        std::list<const item *> all_items_ptr() const;
        /** returns a list of pointers to all items inside recursively */
        std::list<const item *> all_items_ptr( item_pocket::pocket_type pk_type ) const;
        /** returns a list of pointers to all items inside recursively */
        std::list<item *> all_items_ptr( item_pocket::pocket_type pk_type );

        /** returns a list of pointers to all visible or remembered top-level items */
        std::list<item *> all_known_contents();
        std::list<const item *> all_known_contents() const;

        std::list<item *> all_ablative_armor();
        std::list<const item *> all_ablative_armor() const;

        void clear_items();
        bool empty() const;
        // ignores all pockets except CONTAINER pockets to check if this contents is empty.
        bool empty_container() const;

        // gets the item contained IFF one item is contained (CONTAINER pocket), otherwise a null item reference
        item &only_item();
        const item &only_item() const;
        item *get_item_with( const std::function<bool( const item & )> &filter );

        /**
         * returns the number of items stacks in contents
         * each item that is not count_by_charges,
         * plus whole stacks of items that are
         */
        size_t num_item_stacks() const;
        /**
         * This function is to aid migration to using nested containers.
         * The call sites of this function need to be updated to search the
         * pockets of the item, or not assume there is only one pocket or item.
         */
        item &legacy_front();
        const item &legacy_front() const;

        /**
         * Open a menu for the player to set pocket favorite settings for the pockets in this item_contents
         */
        void favorite_settings_menu();

        void combine( const item_contents &read_input, bool convert = false );

        bool is_collapsed() const;

    private:
        /** migrates an item into this item. */
        void migrate_content_item( const item &contained );

        bool use_amount_internal( const itype_id &it, int &quantity, std::list<item> &used,
                                  const std::function<bool( const item & )> &filter = return_true<item> );
        const use_function *get_use_internal( const std::string &use_name ) const;
        template<typename Item>
        static Item *get_usable_item_helper( Item &self, const std::string &use_name );
        bool process_internal( map &here, Character *carrier, const tripoint &pos, float insulation = 1,
                               temperature_flag flag = temperature_flag::NORMAL, float spoil_modifier = 1.0f );
        void iterate_covered_body_parts_internal( side s,
                const std::function<void( const bodypart_str_id & )> &cb ) const;
        void iterate_covered_sub_body_parts_internal( side s,
                const std::function<void( const sub_bodypart_str_id & )> &cb ) const;
        /**
         * Calculate the thermal energy and temperature change of the item
         * @param temp Temperature of surroundings
         * @param insulation Amount of insulation item has
         * @param time_delta time duration from previous temperature calculation
         */
        void calc_temp( const units::temperature &temp, float insulation, const time_duration &time_delta );

        /** Calculates item specific energy (J/g) from temperature*/
        units::specific_energy get_specific_energy_from_temperature( const units::temperature
                &new_temperature )
        const;

        /** Update flags associated with temperature */
        void set_temp_flags( units::temperature new_temperature, float freeze_percentage );

        std::list<item *> all_items_top_recursive( item_pocket::pocket_type pk_type );
        std::list<const item *> all_items_top_recursive( item_pocket::pocket_type pk_type ) const;

        /** Returns true if protection info was printed as well */
        bool armor_full_protection_info( std::vector<iteminfo> &info, const iteminfo_query *parts ) const;

        void update_inherited_flags();

    public:
        enum class sizing : int {
            human_sized_human_char = 0,
            big_sized_human_char,
            small_sized_human_char,
            big_sized_big_char,
            human_sized_big_char,
            small_sized_big_char,
            small_sized_small_char,
            human_sized_small_char,
            big_sized_small_char,
            ignore
        };

        sizing get_sizing( const Character & ) const;

    protected:
        // Sub-functions of @ref process, they handle the processing for different
        // processing types, just to make the process function cleaner.
        // The interface is the same as for @ref process.
        bool process_corpse( map &here, Character *carrier, const tripoint &pos );
        bool process_wet( Character *carrier, const tripoint &pos );
        bool process_litcig( map &here, Character *carrier, const tripoint &pos );
        bool process_extinguish( map &here, Character *carrier, const tripoint &pos );
        // Place conditions that should remove fake smoke item in this sub-function
        bool process_fake_smoke( map &here, Character *carrier, const tripoint &pos );
        bool process_fake_mill( map &here, Character *carrier, const tripoint &pos );
        bool process_link( map &here, Character *carrier, const tripoint &pos );
        bool process_linked_item( Character *carrier, const tripoint &pos, link_state required_state );
        bool process_blackpowder_fouling( Character *carrier );
        bool process_tool( Character *carrier, const tripoint &pos );

    public:
        static const int INFINITE_CHARGES;

        const itype *type;
        item_components components;
        /** What faults (if any) currently apply to this item */
        std::set<fault_id> faults;

    private:
        item_contents contents;
        /** `true` if item has any of the flags that require processing in item::process_internal.
         * This flag is reset to `true` if item tags are changed.
         */
        bool requires_tags_processing = true;
        FlagsSetType item_tags; // generic item specific flags
        FlagsSetType inherited_tags_cache;
        safe_reference_anchor anchor;
        std::map<std::string, std::string> item_vars;
        const mtype *corpse = nullptr;
        std::string corpse_name;       // Name of the late lamented
        std::set<matec_id> techniques; // item specific techniques

        // Select a random variant from the possibilities
        // Intended to be called when no explicit variant is set
        void select_itype_variant();

        bool can_have_itype_variant() const;

        // Does this have a variant with this id?
        bool possible_itype_variant( const std::string &test ) const;

        // If the item has a gun variant, this points to it
        const itype_variant_data *_itype_variant = nullptr;

        /**
         * Data for items that represent in-progress crafts.
         */
        class craft_data
        {
            public:
                const recipe *making = nullptr;
                int next_failure_point = -1;
                std::vector<item_comp> comps_used;
                // If the crafter has insufficient tools to continue to the next 5% progress step
                bool tools_to_continue = false;
                int batch_size = -1;
                std::vector<comp_selection<tool_comp>> cached_tool_selections;
                std::optional<units::mass> cached_weight; // NOLINT(cata-serialize)
                std::optional<units::volume> cached_volume; // NOLINT(cata-serialize)

                // if this is an in progress disassembly as opposed to craft
                bool disassembly = false;
                void serialize( JsonOut &jsout ) const;
                void deserialize( const JsonObject &obj );
        };

        cata::value_ptr<craft_data> craft_data_;
    public:
        // any relic data specific to this item
        cata::value_ptr<relic> relic_data;
        cata::value_ptr<link_data> link;
        int charges = 0;
        units::energy energy = 0_mJ; // Amount of energy currently stored in a battery

        int recipe_charges = 1;    // The number of charges a recipe creates.
        int burnt = 0;             // How badly we're burnt
        int poison = 0;            // How badly poisoned is it?
        int frequency = 0;         // Radio frequency
        snippet_id snip_id = snippet_id::NULL_ID(); // Associated dynamic text snippet id.
        int irradiation = 0;       // Tracks radiation dosage.
        int item_counter = 0;      // generic counter to be used with item flags

        // Time point at which countdown_action is triggered
        time_point countdown_point = calendar::turn_max;

        units::specific_energy specific_energy = units::from_joule_per_gram(
                    -10 ); // Specific energy J/g. Negative value for unprocessed.
        units::temperature temperature = units::from_kelvin( 0 );       // Temperature of the item .
        int mission_id = -1;       // Refers to a mission in game's master list
        int player_id = -1;        // Only give a mission to the right player!
        bool ethereal = false;
        int wetness = 0;           // Turns until this item is completely dry.

        int seed = rng( 0, INT_MAX );  // A random seed for layering and other options

        harvest_drop_type_id dropped_from =
            harvest_drop_type_id::NULL_ID(); // The drop type this item spawned from

        // Set when the item / its content changes. Used for worn item with
        // encumbrance depending on their content.
        // This not part serialized or compared on purpose!
        bool encumbrance_update_ = false;

        item_contents &get_contents() {
            return contents;
        };

        const item_contents &get_contents() const {
            return contents;
        };

    private:
        /**
         * Accumulated rot, expressed as time the item has been in standard temperature.
         * It is compared to shelf life (@ref islot_comestible::spoils) to decide if
         * the item is rotten.
         */
        time_duration rot = 0_turns;
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
        mutable faction_id owner = faction_id::NULL_ID();
        // The faction that previously owned this item
        mutable faction_id old_owner = faction_id::NULL_ID();
        int damage_ = 0;
        int degradation_ = 0;
        light_emission light = nolight;
        mutable std::optional<float> cached_relative_encumbrance;

        // additional encumbrance this specific item has
        units::volume additional_encumbrance = 0_ml;

    public:
        char invlet = 0;      // Inventory letter
        bool active = false; // If true, it has active effects to be processed
        bool is_favorite = false;

        void set_favorite( bool favorite );
        bool has_clothing_mod() const;
        float get_clothing_mod_val( clothing_mod_type type ) const;
        void update_clothing_mod_val();
};

extern template float item::resist<bodypart_id>( const damage_type_id &dmg_type, bool to_self,
        const bodypart_id &bp,
        int resist_value ) const;
extern template float item::resist<sub_bodypart_id>( const damage_type_id &dmg_type,
        bool to_self,
        const sub_bodypart_id &bp,
        int resist_value ) const;

template<>
struct enum_traits<item::encumber_flags> {
    static constexpr bool is_flag_enum = true;
};

bool item_compare_by_charges( const item &left, const item &right );
bool item_ptr_compare_by_charges( const item *left, const item *right );

/**
 * Hint value used for item examination screen and filtering items by action.
 * Represents whether an item permits given action (reload, wear, read, etc.).
 */
enum class hint_rating {
    /** Item permits this action */
    good,
    /** Item permits this action, but circumstances don't */
    iffy,
    /** Item does not permit this action */
    cant
};

// Weight per level of LIFT/JACK tool quality
constexpr units::mass TOOL_LIFT_FACTOR = 500_kilogram;

inline units::mass lifting_quality_to_mass( int quality_level )
{
    return TOOL_LIFT_FACTOR * quality_level;
}

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

#endif // CATA_SRC_ITEM_H
