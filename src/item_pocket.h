#pragma once
#ifndef CATA_SRC_ITEM_POCKET_H
#define CATA_SRC_ITEM_POCKET_H

#include <cstddef>
#include <functional>
#include <iosfwd>
#include <list>
#include <map>
#include <new>
#include <set>
#include <type_traits>
#include <vector>

#include "enums.h"
#include "flat_set.h"
#include "optional.h"
#include "ret_val.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"
#include "visitable.h"

class Character;
class JsonIn;
class JsonObject;
class JsonOut;
class item;
class item_location;
class player;
class pocket_data;
struct iteminfo;
struct itype;
struct tripoint;
template <typename E> struct enum_traits;

class item_pocket
{
    public:
        enum class pocket_type : int {
            CONTAINER,
            MAGAZINE,
            MAGAZINE_WELL, //holds magazines
            MOD, // the gunmods or toolmods
            CORPSE, // the "corpse" pocket - bionics embedded in a corpse
            SOFTWARE, // software put into usb or some such
            MIGRATION, // this allows items to load contents that are too big, in order to spill them later.
            LAST
        };
        enum class contain_code : int {
            SUCCESS,
            // only mods can go into the pocket for mods
            ERR_MOD,
            // trying to put a liquid into a non-watertight container
            ERR_LIQUID,
            // trying to put a gas in a non-airtight container
            ERR_GAS,
            // trying to put an item that wouldn't fit if the container were empty
            ERR_TOO_BIG,
            // trying to put an item that wouldn't fit if the container were empty
            ERR_TOO_HEAVY,
            // trying to put an item that wouldn't fit if the container were empty
            ERR_TOO_SMALL,
            // pocket doesn't have sufficient space left
            ERR_NO_SPACE,
            // pocket doesn't have sufficient weight left
            ERR_CANNOT_SUPPORT,
            // requires a flag
            ERR_FLAG,
            // requires item be a specific ammotype
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

                void info( std::vector<iteminfo> &info ) const;

                void serialize( JsonOut &json ) const;
                void deserialize( JsonIn &jsin );
            private:
                int priority_rating = 0;
                cata::flat_set<itype_id> item_whitelist;
                cata::flat_set<itype_id> item_blacklist;
                cata::flat_set<item_category_id> category_whitelist;
                cata::flat_set<item_category_id> category_blacklist;
        };

        item_pocket() = default;
        explicit item_pocket( const pocket_data *data ) : data( data ) {}

        bool stacks_with( const item_pocket &rhs ) const;
        bool is_funnel_container( units::volume &bigger_than ) const;
        bool has_any_with( const std::function<bool( const item & )> &filter ) const;

        bool is_valid() const;
        bool is_type( pocket_type ptype ) const;
        bool empty() const;
        bool full( bool allow_bucket ) const;

        // Convenience accessors for pocket data attributes with the same name
        bool rigid() const;
        bool watertight() const;
        bool airtight() const;
        // is this speedloader compatible with this pocket (if any speedloaders are whitelisted)
        bool allows_speedloader( const itype_id &speedloader_id ) const;

        // is this pocket one of the standard types?
        // exceptions are MOD, CORPSE, SOFTWARE, MIGRATION, etc.
        bool is_standard_type() const;

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

        ret_val<contain_code> can_contain( const item &it ) const;
        bool can_contain_liquid( bool held_or_ground ) const;
        bool contains_phase( phase_id phase ) const;

        units::length max_containable_length() const;

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
        bool process( const itype &type, player *carrier, const tripoint &pos,
                      float insulation, temperature_flag flag );
        void remove_all_ammo( Character &guy );
        void remove_all_mods( Character &guy );

        void set_item_defaults();

        // removes and returns the item from the pocket.
        cata::optional<item> remove_item( const item &it );
        cata::optional<item> remove_item( const item_location &it );
        // spills any contents that can't fit into the pocket, largest items first
        void overflow( const tripoint &pos );
        bool spill_contents( const tripoint &pos );
        void on_pickup( Character &guy );
        void on_contents_changed();
        void handle_liquid_or_spill( Character &guy, const item *avoid = nullptr );
        void clear_items();
        bool has_item( const item &it ) const;
        item *get_item_with( const std::function<bool( const item & )> &filter );
        void remove_items_if( const std::function<bool( item & )> &filter );
        /**
         * Is part of the recursive call of item::process. see that function for additional comments
         * NOTE: this destroys the items that get processed
         */
        void process( player *carrier, const tripoint &pos, float insulation = 1,
                      temperature_flag flag = temperature_flag::NORMAL, float spoil_multiplier_parent = 1.0f );
        pocket_type saved_type() const {
            return _saved_type;
        }

        bool saved_sealed() const {
            return _saved_sealed;
        }

        // tries to put an item in the pocket. returns false if failure
        ret_val<contain_code> insert_item( const item &it );
        /**
          * adds an item to the pocket with no checks
          * may create a new pocket
          */
        void add( const item &it, item **ret = nullptr );
        bool can_unload_liquid() const;

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
        void favorite_info( std::vector<iteminfo> &info );

        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );

        // true if pocket state is the same as if freshly created from the pocket type
        bool is_default_state() const;

        bool same_contents( const item_pocket &rhs ) const;
        /** stacks like items inside the pocket */
        void restack();
        /** same as above, except returns the stack where input item was placed */
        item *restack( /*const*/ item *it );
        bool has_item_stacks_with( const item &it ) const;

        /**
         * Whether another pocket is a better fit for an item.
         *
         * This assumes that both pockets are able to and allowed to contain the item.
         */
        bool better_pocket( const item_pocket &rhs, const item &it ) const;

        bool operator==( const item_pocket &rhs ) const;

        favorite_settings settings;
    private:
        // the type of pocket, saved to json
        pocket_type _saved_type = pocket_type::LAST;
        bool _saved_sealed = false;
        const pocket_data *data = nullptr;
        // the items inside the pocket
        std::list<item> contents;
        bool _sealed = false;
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
    void deserialize( JsonIn &jsin );
};

class pocket_data
{
    public:
        using FlagsSetType = std::set<flag_id>;

        bool was_loaded = false;

        pocket_data() = default;
        // this constructor is used for special types of pockets, not loading
        explicit pocket_data( item_pocket::pocket_type pk ) : type( pk ) {
            rigid = true;
        }

        item_pocket::pocket_type type = item_pocket::pocket_type::CONTAINER;
        // max volume of stuff the pocket can hold
        units::volume volume_capacity = 0_ml;
        // max volume of item that can be contained, otherwise it spills
        cata::optional<units::volume> max_item_volume = cata::nullopt;
        // min volume of item that can be contained, otherwise it spills
        units::volume min_item_volume = 0_ml;
        // max weight of stuff the pocket can hold
        units::mass max_contains_weight = 0_gram;
        // longest item that can fit into the pocket
        // if not defined in json, calculated to be cbrt( volume ) * sqrt( 2 )
        units::length max_item_length = 0_mm;
        // if true, this pocket can can contain one and only one item
        bool holster = false;
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
        cata::flat_set<itype_id> allowed_speedloaders;
        // the first in the json array for item_id_restriction when loaded
        itype_id default_magazine = itype_id::NULL_ID();
        // container's size and encumbrance does not change based on contents.
        bool rigid = false;

        bool operator==( const pocket_data &rhs ) const;

        units::volume max_contains_volume() const;

        std::string check_definition() const;

        void load( const JsonObject &jo );
        void deserialize( JsonIn &jsin );
    private:

        FlagsSetType flag_restrictions;
};

template<>
struct enum_traits<item_pocket::pocket_type> {
    static constexpr item_pocket::pocket_type last = item_pocket::pocket_type::LAST;
};

template<>
struct ret_val<item_pocket::contain_code>::default_success
    : public std::integral_constant<item_pocket::contain_code,
      item_pocket::contain_code::SUCCESS> {};

#endif // CATA_SRC_ITEM_POCKET_H
