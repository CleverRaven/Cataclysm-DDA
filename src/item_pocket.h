#pragma once
#ifndef CATA_SRC_ITEM_POCKET_H
#define CATA_SRC_ITEM_POCKET_H

#include <cstddef>
#include <functional>
#include <iosfwd>
#include <list>
#include <map>
#include <new>
#include <optional>
#include <set>
#include <type_traits>
#include <vector>

#include "enums.h"
#include "flat_set.h"
#include "pocket_type.h"
#include "ret_val.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"
#include "visitable.h"

class Character;
class JsonObject;
class JsonOut;
class item;
class item_location;
class pocket_data;
struct iteminfo;
struct itype;
struct tripoint;
class map;

class item_pocket
{
    public:
        enum class contain_code : int {
            SUCCESS,
            // only mods can go into the pocket for mods
            ERR_MOD,
            // trying to put a liquid into a non-watertight container
            ERR_LIQUID,
            // trying to put a gas into a non-airtight container
            ERR_GAS,
            // trying to store an item that wouldn't fit even if the container were empty
            ERR_TOO_BIG,
            // trying to store an item that would be too heavy even if the container were empty
            ERR_TOO_HEAVY,
            // trying to store an item that's below the minimum size or length
            ERR_TOO_SMALL,
            // pocket doesn't have sufficient volume capacity left
            ERR_NO_SPACE,
            // pocket doesn't have sufficient weight capacity left
            ERR_CANNOT_SUPPORT,
            // requires a flag the item doesn't have
            ERR_FLAG,
            // requires the item to be a specific ammotype
            ERR_AMMO
        };

        class favorite_settings
        {
            public:
                void clear();

                void set_priority( int priority );
                int priority() const {
                    return priority_rating;
                }

                // have these settings been modified by the player?
                bool is_null() const;

                void whitelist_item( const itype_id &id );
                void blacklist_item( const itype_id &id );
                void clear_item( const itype_id &id );

                const cata::flat_set<itype_id> &get_item_whitelist() const;
                const cata::flat_set<itype_id> &get_item_blacklist() const;
                const cata::flat_set<item_category_id> &get_category_whitelist() const;
                const cata::flat_set<item_category_id> &get_category_blacklist() const;

                void whitelist_category( const item_category_id &id );
                void blacklist_category( const item_category_id &id );
                void clear_category( const item_category_id &id );

                /** Whether an item passes the current whitelist and blacklist filters. */
                bool accepts_item( const item &it ) const;

                /** Are pocket contents hidden?*/
                bool is_collapsed() const;
                /** Flag to show or hide the pocket contents in 'i'nventory screen. */
                void set_collapse( bool );

                // functions for setting if the pocket is completely disabled for normal pickup
                bool is_disabled() const;
                void set_disabled( bool );

                // functions for setting if the pocket will unload during normal actions
                bool is_unloadable() const;
                void set_unloadable( bool );

                const std::optional<std::string> &get_preset_name() const;
                void set_preset_name( const std::string & );

                void set_was_edited();
                bool was_edited() const;

                void info( std::vector<iteminfo> &info ) const;

                void serialize( JsonOut &json ) const;
                void deserialize( const JsonObject &data );
            private:
                std::optional<std::string> preset_name;
                int priority_rating = 0;
                cata::flat_set<itype_id> item_whitelist;
                cata::flat_set<itype_id> item_blacklist;
                cata::flat_set<item_category_id> category_whitelist;
                cata::flat_set<item_category_id> category_blacklist;
                bool collapsed = false;
                bool disabled = false;
                bool unload = true;
                bool player_edited = false;
        };

        item_pocket() = default;
        explicit item_pocket( const pocket_data *data ) : data( data ) {}

        bool stacks_with( const item_pocket &rhs, int depth = 0, int maxdepth = 2 ) const;
        bool is_funnel_container( units::volume &bigger_than ) const;
        bool is_restricted() const;
        bool has_any_with( const std::function<bool( const item & )> &filter ) const;

        bool is_valid() const;
        bool is_type( pocket_type ptype ) const;
        bool is_ablative() const;
        bool is_holster() const;
        // checks if the pocket is a holster and if it has something in it
        bool holster_full() const;
        bool empty() const;
        bool full( bool allow_bucket ) const;

        // Convenience accessors for pocket data attributes with the same name
        bool rigid() const;
        bool watertight() const;
        bool airtight() const;
        bool inherits_flags() const;
        // open pockets are always considered transparent
        bool transparent() const;
        // is this speedloader compatible with this pocket (if any speedloaders are whitelisted)
        bool allows_speedloader( const itype_id &speedloader_id ) const;

        // is this pocket one of the standard types?
        // exceptions are MOD, CORPSE, SOFTWARE, MIGRATION, etc.
        bool is_standard_type() const;

        bool is_forbidden() const;

        const translation &get_description() const;
        const translation &get_name() const;

        const pocket_data *get_pocket_data() const;

        std::list<item *> all_items_top();
        std::list<const item *> all_items_top() const;
        std::list<item *> all_items_ptr( pocket_type pk_type );
        std::list<const item *> all_items_ptr( pocket_type pk_type ) const;

        item &back();
        const item &back() const;
        item &front();
        const item &front() const;
        size_t size() const;
        void pop_back();

        /**
         * Is the pocket compatible with the specified item?
         * Does not check if the item actually fits volume/weight wise
         * @param it the item being put in
         */
        ret_val<contain_code> is_compatible( const item &it ) const;

        /**
         * Can the pocket contain the specified item?
         * @param it The item being put in
         * @param ignore_contents If true, only check for compatible phase, size, and weight, skipping the more CPU-intensive checks against other contents. Optional, default false.
         * @param copies_remaining An optional integer reference that will be set to the number of item copies that won't fit
         */
        ret_val<contain_code> can_contain( const item &it, bool ignore_contents = false ) const;
        ret_val<contain_code> can_contain( const item &it, int &copies_remaining,
                                           bool ignore_contents = false ) const;

        bool can_contain_liquid( bool held_or_ground ) const;
        bool contains_phase( phase_id phase ) const;

        /**
         * returns whether this pocket can be reloaded with the specified item.
         * @param ammo item to be loaded in
         * @param now whether the currently contained ammo/magazine should be taken into account
         */
        bool can_reload_with( const item &ammo, bool now ) const;

        units::length max_containable_length() const;
        units::length min_containable_length() const;

        // combined volume of contained items
        units::volume contains_volume() const;
        units::volume remaining_volume() const;
        // how many more of @it can this pocket hold?
        int remaining_capacity_for_item( const item &it ) const;
        units::volume volume_capacity() const;
        // the amount of space this pocket can hold before it starts expanding
        units::volume magazine_well() const;
        units::mass weight_capacity() const;
        // The largest volume of contents this pocket can have.  Different from
        // volume_capacity because that doesn't take into account ammo containers.
        units::volume max_contains_volume() const;
        // combined weight of contained items
        units::mass contains_weight() const;
        units::mass remaining_weight() const;
        // these avoid rounding errors and are preferred over
        // `it.charges_per_volume( pocket.remaining_volume() )` or
        // `it.charges_per_weight( pocket.remaining_weight() )`
        int charges_per_remaining_volume( const item &it ) const;
        int charges_per_remaining_weight( const item &it ) const;

        units::volume item_size_modifier() const;
        units::mass item_weight_modifier() const;
        units::length item_length_modifier() const;

        /** gets the spoilage multiplier depending on sealed data */
        float spoil_multiplier() const;

        int moves() const;

        int best_quality( const quality_id &id ) const;

        // heats up contents
        void heat_up();
        // returns a list of pointers of all gunmods in the pocket
        std::vector<item *> gunmods();
        // returns a list of pointers of all gunmods in the pocket
        std::vector<const item *> gunmods() const;
        cata::flat_set<itype_id> item_type_restrictions() const;
        item *magazine_current();
        // returns the default magazine if MAGAZINE_WELL, otherwise NULL_ID
        itype_id magazine_default() const;
        // returns amount of ammo consumed
        int ammo_consume( int qty );
        // returns all allowable ammotypes
        std::set<ammotype> ammo_types() const;
        int ammo_capacity( const ammotype &ammo ) const;
        int remaining_ammo_capacity( const ammotype &ammo ) const;
        void casings_handle( const std::function<bool( item & )> &func );
        bool use_amount( const itype_id &it, int &quantity, std::list<item> &used );
        bool will_explode_in_a_fire() const;
        bool item_has_uses_recursive() const;
        // will the items inside this pocket fall out of this pocket if it is placed into another item?
        bool will_spill() const;
        bool will_spill_if_unsealed() const;
        // seal the pocket. returns false if it fails (pocket does not seal)
        bool seal();
        // unseal the pocket.
        void unseal();
        /**
         * A pocket is sealable IFF it has "sealed_data". Sealable pockets are sealed by calling seal().
         * If a pocket is not sealable, it is never considered "sealed".
         */
        bool sealable() const;
        /**
         * A pocket is sealed when item spawns if pocket has "sealed_data".
         * A pocket can never be sealed unless it is sealable (having "sealed_data").
         * If a pocket is sealed, you cannot interact with it until the seal is broken with unseal().
         */
        bool sealed() const;

        std::string translated_sealed_prefix() const;
        bool detonate( const tripoint &p, std::vector<item> &drops );
        bool process( const itype &type, map &here, Character *carrier, const tripoint &pos,
                      float insulation, temperature_flag flag );
        void remove_all_ammo( Character &guy );
        void remove_all_mods( Character &guy );

        void set_item_defaults();

        // removes and returns the item from the pocket.
        std::optional<item> remove_item( const item &it );
        std::optional<item> remove_item( const item_location &it );
        // spills any contents that can't fit into the pocket, largest items first
        void overflow( const tripoint &pos, const item_location &loc );
        bool spill_contents( const tripoint &pos );
        void on_pickup( Character &guy, item *avoid = nullptr );
        void on_contents_changed();
        void handle_liquid_or_spill( Character &guy, const item *avoid = nullptr );
        void clear_items();
        bool has_item( const item &it ) const;
        item *get_item_with( const std::function<bool( const item & )> &filter );
        const item *get_item_with( const std::function<bool( const item & )> &filter ) const;
        void remove_items_if( const std::function<bool( item & )> &filter );
        /**
         * Is part of the recursive call of item::process. see that function for additional comments
         * NOTE: this destroys the items that get processed
         */
        void process( map &here, Character *carrier, const tripoint &pos, float insulation = 1,
                      temperature_flag flag = temperature_flag::NORMAL, float spoil_multiplier_parent = 1.0f );

        void leak( map &here, Character *carrier, const tripoint &pos, item_pocket *pocke = nullptr );

        pocket_type saved_type() const {
            return _saved_type;
        }

        bool saved_sealed() const {
            return _saved_sealed;
        }

        // tries to put an item in the pocket. returns false if failure
        ret_val<item *> insert_item( const item &it, bool into_bottom = false,
                                     bool restack_charges = true, bool ignore_contents = false );
        /**
          * adds an item to the pocket with no checks
          * may create a new pocket
          */
        void add( const item &it, item **ret = nullptr );
        void add( const item &it, int copies, std::vector<item *> &added );
        bool can_unload_liquid() const;

        int fill_with( const item &contained, Character &guy, int amount = 0,
                       bool allow_unseal = false, bool ignore_settings = false );

        /**
        * @brief Check contents of pocket to see if it contains a valid item/pocket to store the given item.
        * @param ret Used to cache and return a pocket if a valid one was found.
        * @param pocket The @a item_pocket pocket to recursively look through.
        * @param parent The parent item location, most likely provided by initial call to @a best_pocket.
        * @param it The item to try and store.
        * @param avoid Item to avoid trying to search for/inside of.
        *
        * @returns A non-nullptr if a suitable pocket is found.
        */
        std::pair<item_location, item_pocket *> best_pocket_in_contents(
            item_location &this_loc, const item &it, const item *avoid,
            bool allow_sealed, bool ignore_settings );

        // only available to help with migration from previous usage of std::list<item>
        std::list<item> &edit_contents();

        // cost of getting an item from this pocket
        // @TODO: make move cost vary based on other contained items
        int obtain_cost( const item &it ) const;

        // this is used for the visitable interface. returns true if no further visiting is required
        bool remove_internal( const std::function<bool( item & )> &filter,
                              int &count, std::list<item> &res );
        // @relates visitable
        VisitResponse visit_contents( const std::function<VisitResponse( item *, item * )> &func,
                                      item *parent = nullptr );

        void general_info( std::vector<iteminfo> &info, int pocket_number, bool disp_pocket_number ) const;
        void contents_info( std::vector<iteminfo> &info, int pocket_number, bool disp_pocket_number ) const;
        void favorite_info( std::vector<iteminfo> &info ) const;

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &data );

        // true if pocket state is the same as if freshly created from the pocket type
        bool is_default_state() const;

        bool same_contents( const item_pocket &rhs ) const;
        /** stacks like items inside the pocket */
        void restack();
        /** same as restack(), except returns the stack where input item was placed */
        item *restack( /*const*/ item *it );
        bool has_item_stacks_with( const item &it ) const;

        /**
         * Whether another pocket is a better fit for an item.
         *
         * This assumes that both pockets are able to and allowed to contain the item.
         */
        bool better_pocket( const item_pocket &rhs, const item &it, bool nested = false ) const;

        bool operator==( const item_pocket &rhs ) const;

        favorite_settings settings;

        // Pocket presets functions
        static void serialize_presets( JsonOut &json );
        static void deserialize_presets( const JsonArray &ja );
        static void load_presets();
        static void add_preset( const item_pocket::favorite_settings &preset );
        static void save_presets();
        static std::vector<item_pocket::favorite_settings>::iterator find_preset( const std::string &s );
        static bool has_preset( const std::string &s );
        static void delete_preset( std::vector<item_pocket::favorite_settings>::iterator iter );
        static std::vector<item_pocket::favorite_settings> pocket_presets;

        // Set wether rigid items are blocked in the pocket
        void set_no_rigid( const std::set<sub_bodypart_id> &is_no_rigid );

        // should the name of this pocket be used as a description
        bool name_as_description = false; // NOLINT(cata-serialize)
    private:
        // the type of pocket, saved to json
        pocket_type _saved_type = pocket_type::LAST; // NOLINT(cata-serialize)
        bool _saved_sealed = false; // NOLINT(cata-serialize)
        const pocket_data *data = nullptr; // NOLINT(cata-serialize)
        // the items inside the pocket
        std::list<item> contents;
        bool _sealed = false;
        // list of sub body parts that can't currently support rigid ablative armor
        std::set<sub_bodypart_id> no_rigid;

        ret_val<contain_code> _can_contain( const item &it, int &copies_remaining,
                                            bool ignore_contents ) const;
};

/**
 *  There are a few implicit things about this struct when applied to pocket_data:
 *  - When a pocket_data::open_container == true, if it's sealed this is false.
 *  - When a pocket_data::watertight == false, if it's sealed this is true.
 *     This is relevant for crafting and spawned items.
 *     Example: Plastic bag with liquid in it, you need to
  *     poke a hole into it to get the liquid, and it's no longer watertight
 */
struct sealable_data {
    // required for generic_factory
    bool was_loaded = false;
    /** multiplier for spoilage rate of contained items when sealed */
    float spoil_multiplier = 1.0f;

    void load( const JsonObject &jo );
    void deserialize( const JsonObject &data );
};

// the chance and volume this pocket makes when moving
struct pocket_noise {
    // required for generic_factory
    bool was_loaded = false;
    /** multiplier for spoilage rate of contained items when sealed */
    int volume = 0;
    int chance = 0;

    void load( const JsonObject &jo );
    void deserialize( const JsonObject &data );
};

class pocket_data
{
    public:
        using FlagsSetType = std::set<flag_id>;

        bool was_loaded = false;

        pocket_data() = default;
        // this constructor is used for special types of pockets, not loading
        explicit pocket_data( pocket_type pk ) : type( pk ) {
            rigid = true;
        }

        // fixed values for the weight and volume of undefined pockets
        static constexpr units::volume max_volume_for_container = 200000000_ml;
        static constexpr units::mass max_weight_for_container = 2000000_kilogram;

        pocket_type type = pocket_type::CONTAINER;
        // max volume of stuff the pocket can hold
        units::volume volume_capacity = max_volume_for_container;
        // max volume of item that can be contained, otherwise it spills
        std::optional<units::volume> max_item_volume = std::nullopt;
        // min volume of item that can be contained, otherwise it spills
        units::volume min_item_volume = 0_ml;
        // min length of item that can be contained used for exterior pockets
        units::length min_item_length = 0_mm;
        // max weight of stuff the pocket can hold
        units::mass max_contains_weight = max_weight_for_container;
        // longest item that can fit into the pocket
        // if not defined in json, calculated to be cbrt( volume ) * sqrt( 2 )
        units::length max_item_length = 0_mm;
        // if true, this pocket can can contain one and only one item
        bool holster = false;
        // if true, this pocket holds ablative armor
        bool ablative = false;
        // additional encumbrance when this pocket is in use
        int extra_encumbrance = 0;
        // how much this pocket contributes to enumbrance compared to an average item
        float volume_encumber_modifier = 1;
        // chance this pockets contents get ripped off when escaping a grab
        int ripoff = 0;
        // volume this pocket makes when moving
        pocket_noise activity_noise;
        // multiplier for spoilage rate of contained items
        float spoil_multiplier = 1.0f;
        // items' weight in this pocket are modified by this number
        float weight_multiplier = 1.0f;
        // items' volume in this pocket are modified by this number for calculation of the containing object
        float volume_multiplier = 1.0f;
        // the size that gets subtracted from the contents before it starts enlarging the item
        units::volume magazine_well = 0_ml;
        // base time it takes to pull an item out of the pocket
        int moves = 100;
        // protects contents from exploding in a fire
        bool fire_protection = false;
        // can hold liquids
        bool watertight = false;
        // can hold gas
        bool airtight = false;
        // the pocket will spill its contents if placed in another container
        bool open_container = false;
        // items in this pocket pass their flags to the parent item
        bool inherits_flags = false;
        // the contents of the pocket are visible
        bool transparent = false;

        // a description of the pocket
        translation description;

        // the name of the item the pocket belongs to
        // this can be used as a fallback description if needed
        translation name;
        // an optional name defined for this pocket
        // used to very briefly distinguish the purpose of the pocket (ex: Torso compartment)
        translation pocket_name;

        /** Data that is different for sealed pockets than unsealed pockets. This takes priority. */
        cata::value_ptr<sealable_data> sealed_data;
        // allows only items with at least one of the following flags to be stored inside
        // empty means no restriction
        const FlagsSetType &get_flag_restrictions() const;
        // flag_restrictions are not supposed to be modifiable, but sometimes there is a need to
        // add some, i.e. for tests.
        void add_flag_restriction( const flag_id &flag );
        // items stored are restricted to these ammo types:
        // the pocket can only contain one of them since the amount is also defined for each ammotype
        std::map<ammotype, int> ammo_restriction;
        // items stored are restricted to these item ids.
        // this takes precedence over the other two restrictions
        cata::flat_set<itype_id> item_id_restriction;
        // Restricts items by their material.
        cata::flat_set<material_id> material_restriction;
        cata::flat_set<itype_id> allowed_speedloaders;
        // the first in the json array for item_id_restriction when loaded
        itype_id default_magazine = itype_id::NULL_ID();
        // container's size and encumbrance does not change based on contents.
        bool rigid = false;
        // Parent item of this pocket has flag NO_UNLOAD
        bool _no_unload = false; // NOLINT(cata-serialize)
        // Parent item of this pocket  has flag NO_RELOAD
        bool _no_reload = false; // NOLINT(cata-serialize)
        // if true, the pocket cannot be used by the player
        bool forbidden = false;

        bool operator==( const pocket_data &rhs ) const;

        units::volume max_contains_volume() const;

        std::string check_definition() const;

        void load( const JsonObject &jo );
        void deserialize( const JsonObject &data );
    private:

        FlagsSetType flag_restrictions;
};

template<>
struct ret_val<item_pocket::contain_code>::default_success
    : public std::integral_constant<item_pocket::contain_code,
      item_pocket::contain_code::SUCCESS> {};

#endif // CATA_SRC_ITEM_POCKET_H
