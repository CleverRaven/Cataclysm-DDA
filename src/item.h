#ifndef ITEM_H
#define ITEM_H

#include <climits>
#include <string>
#include <vector>
#include <list>
#include <bitset>
#include <unordered_set>
#include <set>
#include "enums.h"
#include "json.h"
#include "color.h"
#include "bodypart.h"
#include "string_id.h"
#include "line.h"
#include "item_location.h"

class game;
class Character;
class player;
class npc;
struct itype;
struct mtype;
using mtype_id = string_id<mtype>;
struct islot_armor;
struct use_function;
class material_type;
class item_category;
using ammotype = std::string;
using itype_id = std::string;
class ma_technique;
using matec_id = string_id<ma_technique>;
class Skill;
using skill_id = string_id<Skill>;

enum damage_type : int;

std::string const& rad_badge_color(int rad);

struct light_emission {
    unsigned short luminance;
    short width;
    short direction;
};
extern light_emission nolight;

namespace io {
struct object_archive_tag;
}

struct iteminfo {
    public:
        std::string sType; //Itemtype
        std::string sName; //Main item text
        std::string sFmt; //Text between main item and value
        std::string sValue; //Set to "-999" if no compare value is present
        double dValue; //Stores double value of sValue for value comparisons
        bool is_int; //Sets if sValue should be treated as int or single decimal double
        std::string sPlus; //number +
        bool bNewLine; //New line at the end
        bool bLowerIsBetter; //Lower values are better (red <-> green)
        bool bDrawName; //If false then compares sName, but don't print sName.

        // Inputs are: ItemType, main text, text between main text and value, value,
        // if the value should be an int instead of a double, text after number,
        // if there should be a newline after this item, if lower values are better
        iteminfo(std::string Type, std::string Name, std::string Fmt = "", double Value = -999,
                 bool _is_int = true, std::string Plus = "", bool NewLine = true,
                 bool LowerIsBetter = false, bool DrawName = true);
};

enum layer_level {
    UNDERWEAR = 0,
    REGULAR_LAYER,
    WAIST_LAYER,
    OUTER_LAYER,
    BELTED_LAYER,
    MAX_CLOTHING_LAYER
};

enum class VisitResponse {
    ABORT, // Stop processing after this node
    NEXT,  // Descend vertically to any child nodes and then horizontally to next sibling
    SKIP   // Skip any child nodes and move directly to the next sibling
};

class item_category
{
    public:
        // id (like itype::id) - used when loading from json
        std::string id;
        // display name (localized)
        std::string name;
        // categories are sorted by this value,
        // lower values means the category is shown first
        int sort_rank;

        item_category();
        item_category(const std::string &id, const std::string &name, int sort_rank);
        // Comparators operato on the sort_rank, name, id
        // (in that order).
        bool operator<(const item_category &rhs) const;
        bool operator==(const item_category &rhs) const;
        bool operator!=(const item_category &rhs) const;
};

class item : public JsonSerializer, public JsonDeserializer
{
public:
 item();
 item(const std::string new_type, int turn, bool rand = true);

        /**
         * Make this a corpse of the given monster type.
         * The monster type id must be valid (see @ref MonsterGenerator::get_mtype).
         *
         * The turn parameter sets the birthday of the corpse, in other words: the turn when the
         * monster died. Because corpses are removed from the map when they reach a certain age,
         * one has to be careful when placing corpses with a birthday of 0. They might be
         * removed immediately when the map is loaded without been seen by the player.
         *
         * The name parameter can be used to give the corpse item a name. This is
         * used instead of the monster type name ("corpse of X" instead of "corpse of bear").
         *
         * Without any parameters it makes a human corpse, created at the current turn.
         */
        /*@{*/
        void make_corpse( const mtype_id& mt, unsigned int turn );
        void make_corpse( const mtype_id& mt, unsigned int turn, const std::string &name );
        void make_corpse();
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
        void set_mtype( const mtype *corpse );
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

 item(JsonObject &jo);
        item(item &&) = default;
        item(const item &) = default;
        item &operator=(item &&) = default;
        item &operator=(const item &) = default;
 virtual ~item();
 void make( const std::string new_type, bool scrub = false );

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
    std::string tname( unsigned int quantity = 1, bool with_prefix = true ) const;
    /**
     * Returns the item name and the charges or contained charges (if the item can have
     * charges at at all). Calls @ref tname with given quantity and with_prefix being true.
     */
    std::string display_name( unsigned int quantity = 1) const;
    /**
     * Return all the information about the item and its type. This includes the different
     * properties of the @ref itype (if they are visible to the player). The returned string
     * is already translated and can be *very* long.
     * @param showtext If true, shows the item description, otherwise only the properties item type.
     * @param dump The properties (encapsulated into @ref iteminfo) are added to this vector,
     * the vector can be used to compare them to properties of another item (@ref game::compare).
     */
    std::string info( bool showtext = false) const;
    std::string info( bool showtext, std::vector<iteminfo> &dump ) const;

    bool burn(int amount = 1); // Returns true if destroyed

 // Returns the category of this item.
 const item_category &get_category() const;

    /**
     * Select suitable ammo with which to reload the item
     * @param u player inventory to search for suitable ammo.
     * @param interactive if true prompt to select ammo otherwise select first suitable ammo
     */
    item_location pick_reload_ammo( player &u, bool interactive ) const;

    /** Reload item using ammo from inventory position returning true if sucessful */
    bool reload( player &u, int pos );

    /** Reload item using ammo from location returning true if sucessful */
    bool reload( player &u, item_location loc );

    skill_id skill() const;

    template<typename Archive>
    void io( Archive& );
    using archive_type_tag = io::object_archive_tag;

    using JsonSerializer::serialize;
    // give the option not to save recursively, but recurse by default
    void serialize(JsonOut &jsout) const override { serialize(jsout, true); }
    virtual void serialize(JsonOut &jsout, bool save_contents) const;
    using JsonDeserializer::deserialize;
    // easy deserialization from JsonObject
    virtual void deserialize(JsonObject &jo);
    void deserialize(JsonIn &jsin) override {
        JsonObject jo = jsin.get_object();
        deserialize(jo);
    }

    // Legacy function, don't use.
    void load_info( const std::string &data );
 char symbol() const;
 int price() const;

    /**
     * Return the butcher factor (BUTCHER tool quality).
     * If the item can not be used for butchering it returns INT_MIN.
     */
    int butcher_factor() const;

        bool stacks_with( const item &rhs ) const;
        /**
         * Merge charges of the other item into this item.
         * @return true if the items have been merged, otherwise false.
         * Merging is only done for items counted by charges (@ref count_by_charges) and
         * items that stack together (@ref stacks_with).
         */
        bool merge_charges( const item &rhs );

 int weight() const;

 int precise_unit_volume() const;
 int volume(bool unit_value=false, bool precise_value=false) const;

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
    /**
     * Damage of type @ref DT_BASH that is caused by using this item as melee weapon.
     */
    int damage_bash() const;
    /**
     * Damage of type @ref DT_CUT that is caused by using this item as melee weapon.
     */
    int damage_cut() const;
    /**
     * Whether the character needs both hands to wield this item.
     */
    bool is_two_handed( const player &u ) const;
    /** The weapon is considered a suitable melee weapon. */
    bool is_weap() const;
    /** The item is considered a bashing weapon (inflicts a considerable bash damage). */
    bool is_bashing_weapon() const;
    /** The item is considered a cutting weapon (inflicts a considerable cutting damage). */
    bool is_cutting_weapon() const;
    /**
     * The most relevant skill used with this melee weapon. Can be "null" if this is not a weapon.
     * Note this function returns null if the item is a gun for which you can use gun_skill() instead.
     */
    skill_id weap_skill() const;
    /*@}*/

 /**
  * Count the amount of items of type 'it' including this item,
  * and any of its contents (recursively).
  * @param it The type id, only items with the same id are counted.
  * @param used_as_tool If false all items with the PSEUDO flag are ignore
  * (not counted).
  */
 int amount_of(const itype_id &it, bool used_as_tool) const;
 /**
  * Count all the charges of items of the type 'it' including this item,
  * and any of its contents (recursively).
  * @param it The type id, only items with the same id are counted.
  */
 long charges_of(const itype_id &it) const;
 /**
  * Consume a specific amount of charges from items of a specific type.
  * This includes this item, and any of its contents (recursively).
  * @param it The type id, only items of this type are considered.
  * @param quantity The number of charges that should be consumed.
  * It will be changed for each used charge. After calling this function it
  * may be at 0 which means all requested charges have been consumed.
  * @param used All used charges are put into this list, the caller may need it.
  * @return Whether this item should be deleted (in which case it returns true).
  * Some items (those that are counted by charges) must be destroyed when
  * their charges reach 0.
  * This is usually does not apply to tools.
  * Also if this function is called on a container and the function erase charges
  * from its contents the container should not be deleted - it returns false in
  * that case.
  * The caller *must* check the return value and remove the item from wherever
  * it is stored when the function returns true.
  * Note that the item itself has no way of knowing where it is stored and can
  * therefor not delete itself.
  */
 bool use_charges(const itype_id &it, long &quantity, std::list<item> &used);
 /**
  * Consume a specific amount of items of a specific type.
  * This includes this item, and any of its contents (recursively).
  * @see item::use_charges - this is similar for items, not charges.
  * @param it Type of consumable item.
  * @param quantity How much to consumed.
  * @param used On success all consumed items will be stored here.
  */
 bool use_amount(const itype_id &it, long &quantity, std::list<item> &used);

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
    /** Whether this is sealable container which can be resealed after removing part of it's content */
    bool is_sealable_container() const;
    /** Whether this item has no contents at all. */
    bool is_container_empty() const;
    /** Whether this item has no more free capacity for its current content. */
    bool is_container_full() const;
    /**
     * Fill item with liquid up to its capacity. This works for guns and tools that accept
     * liquid ammo.
     * @param liquid Liquid to fill the container with.
     * @param err Contains error message if function returns false.
     * @return Returns false in case of error. Nothing has been added in that case.
     */
    bool fill_with( item &liquid, std::string &err );
    /**
     * How much more of this liquid (in charges) can be put in this container.
     * If this is not a container (or not suitable for the liquid), it returns 0.
     * Note that mixing different types of liquid is not possible.
     * Also note that this works for guns and tools that accept liquid ammo.
     */
    long get_remaining_capacity_for_liquid(const item &liquid) const;
    /**
     * Puts the given item into this one, no checks are performed.
     */
    void put_in( item payload );
    /**
     * Returns this item into its default container. If it does not have a default container,
     * returns this. It's intended to be used like \code newitem = newitem.in_its_container();\endcode
     */
    item in_its_container(); // TODO: make this const
    /*@}*/

    /*@{*/
    /**
     * Funnel related functions. See weather.cpp for their usage.
     */
    bool is_funnel_container(int &bigger_than) const;
    void add_rain_to_container(bool acid, int charges = 1);
    /*@}*/

 bool has_quality(std::string quality_id) const;
 bool has_quality(std::string quality_id, int quality_value) const;
 bool count_by_charges() const;
 bool craft_has_charges();
 long num_charges();

    /**
     * Reduce the charges of this item, only use for items counted by charges!
     * The item must have enough charges for this (>= quantity) and be counted
     * by charges.
     * @param quantity How many charges should be removed.
     * @return true if all charges would have been removed and the item must be destroyed.
     * The charges member is not changed in that case (for usage in `player::i_rem`
     * which returns the removed item).
     * False if there are charges remaining, the charges have been reduced in that case.
     */
    bool reduce_charges( long quantity );
    /**
     * Returns true if the item is considered rotten.
     */
    bool rotten() const;
    /**
     * Accumulate rot of the item since last rot calculation.
     * This function works for non-rotting stuff, too - it increases the value
     * of rot.
     * @param p The absolute, global location (in map square coordinates) of the item to
     * check for temperature.
     */
    void calc_rot( const tripoint &p );
    /**
     * Returns whether the item has completely rotten away.
     */
    bool has_rotten_away() const;
    /**
     * Whether the item is nearly rotten (implies that it spoils).
     */
    bool is_going_bad() const;
    /**
     * Get @ref rot value relative to it_comest::spoils, if the item does not spoil,
     * it returns 0. If the item is rotten the returned value is > 1.
     */
    float get_relative_rot();
    /**
     * Set the @ref rot to the given relative rot (relative to it_comest::spoils).
     */
    void set_relative_rot(float rel_rot);
    /**
     * Whether the item will spoil at all.
     */
    bool goes_bad() const;
private:
    /**
     * Accumulated rot of the item. This is compared to it_comest::spoils
     * to decide weather the item is rotten or not.
     */
    int rot;
    /**
     * The turn when the rot calculation has been done the last time.
     */
    int last_rot_check;
public:
    int get_rot() const
    {
        return rot;
    }
    /**
     * The turn when this item has been put into a fridge.
     * 0 if this item is not in a fridge.
     */
    int fridge;

 int brewing_time() const;
 void detonate( const tripoint &p ) const;

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
     * This might return the null-material, you may check this with @ref material_type::is_null.
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
     */
    const std::vector<std::string> &made_of() const;
    /**
     * Same as @ref made_of(), but returns the @ref material_type directly.
     */
    std::vector<material_type*> made_of_types() const;
    /**
     * Check we are made of at least one of a set (e.g. true if even
     * one item of the passed in set matches any material).
     * @param mat_idents Set of material ids.
     */
    bool made_of_any( const std::vector<std::string> &mat_idents ) const;
    /**
     * Check we are made of only the materials (e.g. false if we have
     * one material not in the set).
     * @param mat_idents Set of material ids.
     */
    bool only_made_of( const std::vector<std::string> &mat_idents ) const;
    /**
     * Check we are made of this material (e.g. matches at least one
     * in our set.)
     */
    bool made_of( const std::string &mat_ident ) const;
    /**
     * Are we solid, liquid, gas, plasma?
     */
    bool made_of( phase_id phase ) const;
    /**
     * Whether the items is conductive.
     */
    bool conductive() const;
    /**
     * Whether the items is flammable. (Make sure to keep this in sync with
     * fire code in fields.cpp)
     */
    bool flammable() const;
    /*@}*/

    /**
     * Resistance against different damage types (@ref damage_type).
     * Larger values means more resistance are thereby better, but there is no absolute value to
     * compare them to. The values can be interpreted as chance (@ref one_in) of damaging the item
     * when exposed to the type of damage.
     * @param to_self If this is true, it returns item's own resistance, not one it gives to wearer.
     */
    /*@{*/
    int bash_resist( bool to_self = false ) const;
    int cut_resist ( bool to_self = false )  const;
    int stab_resist( bool to_self = false ) const;
    int acid_resist( bool to_self = false ) const;
    int fire_resist( bool to_self = false ) const;
    /*@}*/

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

    /**
     * Check whether the item has been marked (by calling mark_as_used_by_player)
     * as used by this specific player.
     */
    bool already_used_by_player(const player &p) const;
    /**
     * Marks the item as being used by this specific player, it remains unmarked
     * for other players. The player is identified by its id.
     */
    void mark_as_used_by_player(const player &p);
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
    bool process(player *carrier, const tripoint &pos, bool activate);
protected:
    // Sub-functions of @ref process, they handle the processing for different
    // processing types, just to make the process function cleaner.
    // The interface is the same as for @ref process.
    bool process_food(player *carrier, const tripoint &pos);
    bool process_corpse(player *carrier, const tripoint &pos);
    bool process_wet(player *carrier, const tripoint &pos);
    bool process_litcig(player *carrier, const tripoint &pos);
    bool process_cable(player *carrier, const tripoint &pos);
    bool process_tool(player *carrier, const tripoint &pos);
    bool process_charger_gun(player *carrier, const tripoint &pos);
public:
    /**
     * Helper to bring a cable back to its initial state.
     */
    void reset_cable(player* carrier);

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
     * @return True if the item should be destroyed (it has run out of charges or similar), false
     * if the item should be kept. Artifacts usually return false as they never get destroyed.
     * @param carrier The character carrying the artifact, can be null.
     * @param pos The location of the artifact (should be the player location if carried).
     */
    bool process_artifact( player *carrier, const tripoint &pos );

 bool destroyed_at_zero_charges() const;
// Most of the is_whatever() functions call the same function in our itype
 bool is_null() const; // True if type is NULL, or points to the null item (id == 0)
 bool is_food(player const*u) const;// Some non-food items are food to certain players
 bool is_food_container(player const*u) const;  // Ditto
 bool is_food() const;                // Ignoring the ability to eat batteries, etc.
 bool is_food_container() const;      // Ignoring the ability to eat batteries, etc.
 bool is_ammo_container() const;
 bool is_bionic() const;
 bool is_ammo() const;
 bool is_armor() const;
 bool is_book() const;
 bool is_salvageable() const;
 bool is_disassemblable() const;

 bool is_tool() const;
 bool is_tool_reversible() const;
 bool is_software() const;
 bool is_var_veh_part() const;
 bool is_artifact() const;

        bool is_dangerous() const; // Is it an active grenade or something similar that will hurt us?

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

 bool operator<(const item& other) const;
    /** List of all @ref components in printable form, empty if this item has
     * no components */
    std::string components_to_string() const;

 itype_id typeId() const;
 const itype* type;
 std::vector<item> contents;

        /** Traverses this item and any child items contained using a visitor pattern
         * @pram func visitor function called for each node which controls whether traversal continues.
         * Typically a lambda making use of captured state it should return VisitResponse::Next to
         * recursively process child items, VisitResponse::Skip to ignore children of the current node
         * or VisitResponse::Abort to skip further processing of any nodes.
         * @return This method itself only ever returns VisitResponse::Next or VisitResponse::Abort.
         */
        VisitResponse visit( const std::function<VisitResponse(item&)>& func );
        VisitResponse visit( const std::function<VisitResponse(const item&)>& func ) const;

        /** Checks if item is a holster and currently capable of storing obj */
        bool can_holster ( const item& obj ) const;
        /**
         * Returns @ref curammo, the ammo that is currently load in this item.
         * May return a null pointer.
         * If non-null, the returned itype is quaranted to have an ammo slot:
         * @code itm.get_curammo()->ammo->damage @endcode will work.
         */
        const itype* get_curammo() const;
        /**
         * Returns the item type id of the currently loaded ammo.
         * Returns "null" if the item is not loaded.
         */
        itype_id get_curammo_id() const;
        /**
         * Whether the item is currently loaded (which implies it has some non-null pointer
         * as @ref curammo).
         */
        bool has_curammo() const;
        /**
         * Sets the current ammo to nullptr. Note that it does not touch the charges or anything else.
         */
        void unset_curammo();
        /**
         * Set the current ammo from an item type id (not an ammo type id!). The type must have
         * an ammo slot (@ref itype::ammo). If the type id is "null", the curammo is unset as by calling
         * @ref unset_curammo.
         */
        void set_curammo( const itype_id &type );
        /**
         * Shortcut to set the current ammo to the type of the given item. This is the same as
         * calling @ref set_curammo with item type id of the ammo item:
         * \code set_curammo(ammo.typeId()) \endcode
         */
        void set_curammo( const item &ammo );
        /**
         * Callback when a player starts wearing the item. The item is already in the worn
         * items vector and is called from there.
         */
        void on_wear( player &p );
        /**
         * Callback when a player takes off an item. The item is still in the worn items
         * vector but will be removed immediately after the function returns
         */
        void on_takeoff (player &p);
        /**
         * Callback when a player starts wielding the item. The item is already in the weapon
         * slot and is called from there.
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
         * Liquids use a different (and type specific) scale for the charges vs volume.
         * This functions converts them. You can assume that
         * @code liquid_charges( liquid_units( x ) ) == x @endcode holds true.
         * For items that are not liquids or otherwise don't use this system, both functions
         * simply return their input (conversion factor is 1).
         * One "unit" takes up one container storage capacity, e.g.
         * A container with @ref islot_container::contains == 2 can store
         * @code liquid.liquid_charges( 2 ) @endcode charges of the given liquid.
         * For water this would be 2, for most strong alcohols it's 14, etc.
         */
        /*@{*/
        long liquid_charges( long units ) const;
        long liquid_units( long charges ) const;
        /*@}*/

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
         * long l = itm.get_var("v", 0l); // l will be a long
         * double d = itm.get_var("v", 0.0); // d will be a double
         * std::string s = itm.get_var("v", ""); // s will be a std::string
         * // no default means empty string as default:
         * auto n = itm.get_var("v"); // v will be a std::string
         * </code>
         */
        /*@{*/
        void set_var( const std::string &name, int value );
        void set_var( const std::string &name, long value );
        void set_var( const std::string &name, double value );
        double get_var( const std::string &name, double default_value ) const;
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
        bool has_flag( const std::string& flag ) const;
        /** Removes all item specific flags. */
        void unset_flags();
        /*@}*/

        /**
         * @name Item properties
         *
         * Properties are specific to an item type so unlike flags the meaning of a property
         * may not be the same for two different item types. Each item type can have mutliple
         * properties however duplicate property names are not permitted.
         *
         */
        /*@{*/
        bool has_property (const std::string& prop) const;
        /**
          * Get typed property for item.
          * Return same type as the passed default value, or string where no default provided
          */
        std::string get_property_string( const std::string &prop, const std::string& def = "" ) const;
        long get_property_long( const std::string& prop, long def = 0 ) const;
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
         * Time (in turns) it takes to grow from one stage to another. There are 4 plant stages:
         * seed, seedling, mature and harvest. Non-seed items return 0.
         */
        int get_plant_epoch() const;
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
         * Bitset of all covered body parts. If the bit is set, the body part is covered by this
         * item (when worn). The index of the bit should be a body part, for example:
         * @code if( some_armor.get_covered_body_parts().test( bp_head ) ) { ... } @endcode
         * For testing only a single body part, use @ref covers instead. This function allows you
         * to get the whole covering data in one call.
         */
        std::bitset<num_bp> get_covered_body_parts() const;
        /**
          * Returns true if item is armor and can be worn on different sides of the body
          */
        bool is_sided() const;
        /**
         *  Returns side item currently worn on. Returns BOTH if item is not sided or no side currently set
         */
        int get_side() const;
        /**
          * Change the side on which the item is worn. Returns false if the item is not sided
          */
        bool set_side (side s);
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
         * Returns clothing layer for item which will always be 0 for non-wearable items.
         */
        int get_layer() const;
        /**
         * Returns the relative coverage that this item has when worn.
         * Values range from 0 (not covering anything, or no armor at all) to
         * 100 (covering the whole body part). Items that cover more are more likely to absorb
         * damage from attacks.
         */
        int get_coverage() const;
        /**
         * Returns the encumbrance value that this item has when worn.
         * Returns 0 if this is can not be worn at all.
         */
        int get_encumber() const;
        /**
         * Returns the storage amount (@ref islot_armor::storage) that this item provides when worn.
         * For non-armor it returns 0. The storage amount increases the volume capacity of the
         * character that wears the item.
         */
        int get_storage() const;
        /**
         * Returns the resistance to environmental effects (@ref islot_armor::env_resist) that this
         * item provides when worn. See @ref player::get_env_resist. Higher values are better.
         * For non-armor it returns 0.
         */
        int get_env_resist() const;
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
        bool has_technique( const matec_id & tech ) const;
        /**
         * Returns all the martial art techniques that this items supports.
         */
        std::set<matec_id> get_techniques() const;
        /**
         * Add the given technique to the item specific @ref techniques. Note that other items of
         * the same type are not affected by this.
         */
        void add_technique( const matec_id & tech );
        /*@}*/

        /**
         * @name Charger gun
         *
         * These functions are used on charger guns. Those items are activated, load over time
         * (using the wielders UPS), and fire like a normal gun using pseudo ammo.
         * Each function returns false when called on items that are not charger guns.
         * Nothing is done in that case, so it's save to call them even when it's unknown whether
         * the item is a charger gun.
         * You must all @ref update_charger_gun_ammo before using properties of it as they depend
         * on the charges of the gun.
         */
        /*@{*/
        /**
         * Deactivate the gun.
         */
        bool deactivate_charger_gun();
        /**
         * Activate the gun, it will now load charges over time.
         * The item must be in the possessions of a player (given as parameter).
         * The function will show a message regarding the loading status. If the player does not
         * have a power source, it will not start loading and a different message is displayed.
         * Can be called on npcs (no messages than).
         */
        bool activate_charger_gun( player &u );
        /**
         * Update the charges ammo settings. This must be called right before firing the gun because
         * the properties of the ammo depend on the loading of the gun.
         * E.g. a gun with many charges provides more ammo effects.
         */
        bool update_charger_gun_ammo();
        /** Whether this is a charger gun. */
        bool is_charger_gun() const;
        /*@}*/

        /**
         * @name Gun and gunmod functions
         *
         * Gun and gun mod functions. Anything stated to apply to guns, applies to auxiliary gunmods
         * as well (they are some kind of gun). Non-guns are items that are neither gun nor
         * auxiliary gunmod.
         */
        /*@{*/
        bool is_gunmod() const;
        bool is_gun() const;
        /**
         * How much moves (@ref Creature::moves) it takes to reload this item.
         * This also applies to tools.
         */
        int reload_time( const player &u ) const;
        /** Quantity of ammunition currently loaded in tool, gun or axuiliary gunmod */
        long ammo_remaining() const;
        /** Maximum quantity of ammunition loadable for tool, gun or axuiliary gunmod */
        long ammo_capacity() const;
        /** Quantity of ammunition consumed per usage of tool or with each shot of gun */
        long ammo_required() const;
        /** If sufficient ammo available consume it, otherwise do nothing and return false */
        bool ammo_consume( int qty );
        /** Specific ammo data, returns nullptr if item is neither ammo nor loaded with any */
        const itype * ammo_data() const;
        /**
         * The id of the ammo type (@ref ammunition_type) that can be used by this item.
         * Will return "NULL" if the item does not use a specific ammo type. Items without
         * ammo type can not be reloaded.
         */
        ammotype ammo_type() const;
        /**
         * Number of charges this gun can hold. Includes effects from installed gunmods.
         * This does use the auxiliary gunmod (if any).
         */
        // TODO: make long? Because it relates to charges.
        int clip_size() const;
        /**
         * Burst size (see ranged.cpp), includes effects from installed gunmods.
         */
        int burst_size() const;
        /** Aim speed. See ranged.cpp */
        int aim_speed( int aim_threshold ) const;
        /** We use the current aim level to decide which sight to use. */
        int sight_dispersion( int aim_threshold ) const;
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
         * Auxiliary gun mod: a gunmod that can be fired instead of the actual gun.
         * Example: underslug shotgun.
         */
        bool is_auxiliary_gunmod() const;
        /**
         * Same as @code get_gun_mode() == "MODE_AUX" @endcode
         */
        bool is_in_auxiliary_mode() const;
        /**
         * Same as @code set_gun_mode("MODE_AUX") @endcode
         */
        void set_auxiliary_mode();
        /**
         * Get the gun mode, e.g. BURST, or MODE_AUX, or something else.
         */
        std::string get_gun_mode() const;
        /**
         * Set the gun mode (see @ref get_gun_mode).
         */
        void set_gun_mode( const std::string &mode );
        /**
         * If this item is a gun with several firing mods (including auxiliary gunmods), switch
         * to the next mode. Otherwise, make nothing at all.
         */
        void next_mode();
        /**
         * The weapons range in map squares. If the item has an active gunmod, it returns the range
         * of that gunmod, the guns range is returned only when the item has no active gunmod.
         * This function applies to guns and auxiliary gunmods. For other items, 0 is returned.
         * It includes the range given by the ammo.
         * @param u The player that uses the weapon, their strength might affect this.
         * It's optional and can be null.
         */
        int gun_range( const player *u ) const;
        /**
         * Summed range value of a gun, including values from mods. Returns 0 on non-gun items.
         */
        int gun_range( bool with_ammo = true ) const;
        /**
         * Summed recoils value of a gun, including values from mods. Returns 0 on non-gun items.
         */
        int gun_recoil( bool with_ammo = true ) const;
        /**
         * Summed ranged damage of a gun, including values from mods. Returns 0 on non-gun items.
         */
        int gun_damage( bool with_ammo = true ) const;
        /**
         * Summed ranged armor-piercing of a gun, including values from mods. Returns 0 on non-gun items.
         */
        int gun_pierce( bool with_ammo = true ) const;
        /**
         * Summed dispersion of a gun, including values from mods. Returns 0 on non-gun items.
         */
        int gun_dispersion( bool with_ammo = true ) const;
        /**
         * The skill used to operate the gun. Can be "null" if this is not a gun.
         * Note that this function is not like @ref skill, it returns "null" for any non-gun (books)
         * for which skill() would return a skill.
         */
        skill_id gun_skill() const;
        /**
         * Returns the appropriate size for a spare magazine used with this gun. If this is not a gun,
         * it returns 0.
         */
        int spare_mag_size() const;
        /**
         * Returns the currently active auxiliary (@ref is_auxiliary_gunmod) gun mod item.
         * May return null if there is no such gun mod or if the gun is not in the
         * auxiliary mode (@ref is_in_auxiliary_mode).
         */
        item* active_gunmod();
        item const* active_gunmod() const;
        /**
         * Returns the index of a gunmod item of the given type. The actual gunmod item is in
         * the @ref contents vector, the returned index point into that vector.
         * Returns -1 if this is not a gun, or if it has no such gunmod.
         */
        int has_gunmod( const itype_id& type ) const;
        /**
         * Number of mods that can still be installed into the given mod location,
         * for non-guns it always returns 0.
         */
        int get_free_mod_locations( const std::string& location ) const;
        /**
         * Does it require gunsmithing tools to repair.
         */
        bool is_firearm() const;
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
         * Recursively check the contents of this item and remove those items
         * that match the filter. Note that this function does *not* match
         * the filter against *this* item, only against the contents.
         * @return The removed items, the list may be empty if no items matches.
         */
        template<typename T>
        std::list<item> remove_items_with( T filter )
        {
            std::list<item> result;
            for( auto it = contents.begin(); it != contents.end(); ) {
                if( filter( *it ) ) {
                    result.push_back( std::move( *it ) );
                    it = contents.erase( it );
                } else {
                    result.splice( result.begin(), it->remove_items_with( filter ) );
                    ++it;
                }
            }
            return result;
        }
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
        static itype *find_type( const itype_id &id );
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
    private:
        /** Reset all members to default, making this a null item. */
        void init();
        /** Helper for liquid and container related stuff. */
        enum LIQUID_FILL_ERROR : int;
        LIQUID_FILL_ERROR has_valid_capacity_for_liquid(const item &liquid) const;
        std::string name;
        const itype* curammo;
        std::map<std::string, std::string> item_vars;
        const mtype* corpse;
        std::set<matec_id> techniques; // item specific techniques
        light_emission light;
public:
 char invlet;             // Inventory letter
 long charges;
 bool active;             // If true, it has active effects to be processed
 signed char damage;      // How much damage it's sustained; generally, max is 5
 int burnt;               // How badly we're burnt
 int bday;                // The turn on which it was created
 union{
   int poison;          // How badly poisoned is it?
   int bigness;         // engine power, wheel size
   int frequency;       // Radio frequency
   int note;            // Associated dynamic text snippet.
   int irridation;      // Tracks radiation dosage.
 };
 std::set<std::string> item_tags; // generic item specific flags
 unsigned item_counter; // generic counter to be used with item flags
 int mission_id; // Refers to a mission in game's master list
 int player_id; // Only give a mission to the right player!
 typedef std::vector<item> t_item_vector;
 t_item_vector components;

 int quiver_store_arrow(item &arrow);
 int max_charges_from_flag(std::string flagName);
 int get_gun_ups_drain() const;
};

bool item_compare_by_charges( const item& left, const item& right);
bool item_ptr_compare_by_charges( const item *left, const item *right);

std::ostream &operator<<(std::ostream &, const item &);
std::ostream &operator<<(std::ostream &, const item *);

class map_item_stack
{
    private:
        class item_group {
            public:
                tripoint pos;
                int count;

                //only expected to be used for things like lists and vectors
                item_group() {
                    pos = tripoint( 0, 0, 0 );
                    count = 0;
                }

                item_group( const tripoint &p, const int arg_count ) {
                    pos = p;
                    count = arg_count;
                }

                ~item_group() {};
        };
    public:
        item const *example; //an example item for showing stats, etc.
        std::vector<item_group> vIG;
        int totalcount;

        //only expected to be used for things like lists and vectors
        map_item_stack() {
            vIG.push_back( item_group() );
            totalcount = 0;
        }

        map_item_stack( item const *it, const tripoint &pos ) {
            example = it;
            vIG.push_back( item_group( pos, ( it->count_by_charges() ) ? it->charges : 1 ) );
            totalcount = ( it->count_by_charges() ) ? it->charges : 1;
        }

        ~map_item_stack() {};

        // This adds to an existing item group if the last current
        // item group is the same position and otherwise creates and
        // adds to a new item group. Note that it does not search
        // through all older item groups for a match.
        void add_at_pos( item const *it, const tripoint &pos ) {
            int amount = ( it->count_by_charges() ) ? it->charges : 1;

            if( !vIG.size() || vIG[vIG.size() - 1].pos != pos ) {
                vIG.push_back( item_group( pos, amount ) );
            } else {
                vIG[vIG.size() - 1].count += amount;
            }

            totalcount += amount;
        }

        static bool map_item_stack_sort( const map_item_stack &lhs, const map_item_stack &rhs ) {
            if( lhs.example->get_category().sort_rank == rhs.example->get_category().sort_rank ) {
                return square_dist( tripoint( 0, 0, 0 ), lhs.vIG[0].pos) <
                    square_dist( tripoint( 0, 0, 0 ), rhs.vIG[0].pos );
            }

            return lhs.example->get_category().sort_rank < rhs.example->get_category().sort_rank;
        }
};

// Commonly used convenience functions that match an item to one of the 3 common types of locators:
// type_id (itype_id, a typedef of string), position (int) or pointer (item *).
// The item's position is optional, if not passed in we expect the item to fail position match.
bool item_matches_locator(const item &it, const itype_id &id, int item_pos = INT_MIN);
bool item_matches_locator(const item &it, int locator_pos, int item_pos = INT_MIN);
bool item_matches_locator(const item &it, const item *other, int);

//the assigned numbers are a result of legacy stuff in draw_item_info(),
//it would be better long-term to rewrite stuff so that we don't need that hack
enum hint_rating {
    HINT_CANT = 0, //meant to display as gray
    HINT_IFFY = 1, //meant to display as red
    HINT_GOOD = -999 // meant to display as green
};

#endif

