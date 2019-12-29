#pragma once
#ifndef ITEM_POCKET_H
#define ITEM_POCKET_H

#include <list>

#include "enums.h"
#include "enum_traits.h"
#include "optional.h"
#include "type_id.h"
#include "ret_val.h"
#include "translations.h"
#include "units.h"
#include "visitable.h"

class Character;
class item;
class item_location;
class player;
class pocket_data;

struct iteminfo;
struct itype;
struct tripoint;

using itype_id = std::string;

class item_pocket
{
    public:
        enum pocket_type {
            // this is to aid the transition from the previous way item contents were handled.
            // this will have the rules that previous contents would have
            LEGACY_CONTAINER,
            CONTAINER,
            MAGAZINE,
            LAST
        };
        enum contain_code {
            SUCCESS,
            // legacy containers can't technically contain anything
            ERR_LEGACY_CONTAINER,
            // trying to put a liquid into a non-watertight container
            ERR_LIQUID,
            // trying to put a gas in a non-gastight container
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
            ERR_FLAG
        };

        item_pocket() = default;
        item_pocket( const pocket_data *data ) : data( data ) {}

        bool stacks_with( const item_pocket &rhs ) const;

        bool is_type( pocket_type ptype ) const;
        bool empty() const;

        std::list<item> all_items();
        std::list<item> all_items() const;
        std::list<item *> all_items_ptr( pocket_type pk_type );
        std::list<const item *> all_items_ptr( pocket_type pk_type ) const;

        item &back();
        const item &back() const;
        item &front();
        const item &front() const;
        size_t size() const;
        void pop_back();

        ret_val<contain_code> can_contain( const item &it ) const;

        // combined volume of contained items
        units::volume contains_volume() const;
        units::volume remaining_volume() const;
        units::volume volume_capacity() const;
        // combined weight of contained items
        units::mass contains_weight() const;
        units::mass remaining_weight() const;

        units::volume item_size_modifier() const;
        units::mass item_weight_modifier() const;

        item *magazine_current();
        void casings_handle( const std::function<bool( item & )> &func );
        bool use_amount( const itype_id &it, int &quantity, std::list<item> &used );
        bool will_explode_in_a_fire() const;
        bool detonate( const tripoint &p, std::vector<item> &drops );
        bool process( const itype &type, player *carrier, const tripoint &pos, bool activate,
                      float insulation, temperature_flag flag );
        bool legacy_unload( player *guy, bool &changed );
        void remove_all_ammo( Character &guy );
        void remove_all_mods( Character &guy );

        // removes and returns the item from the pocket.
        cata::optional<item> remove_item( const item &it );
        cata::optional<item> remove_item( const item_location &it );
        bool spill_contents( const tripoint &pos );
        void clear_items();
        bool has_item( const item &it ) const;
        item *get_item_with( const std::function<bool( const item & )> &filter );
        void remove_items_if( const std::function<bool( item & )> &filter );
        void has_rotten_away( const tripoint &pnt );

        // tries to put an item in the pocket. returns false if failure
        ret_val<contain_code> insert_item( const item &it );
        void add( const item &it );

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

        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );

        bool operator==( const item_pocket &rhs ) const;

        bool was_loaded;
    private:
        const pocket_data *data = nullptr;
        // the items inside the pocket
        std::list<item> contents;
};

class pocket_data
{
    public:
        bool was_loaded;

        item_pocket::pocket_type type = item_pocket::pocket_type::LEGACY_CONTAINER;
        // max volume of stuff the pocket can hold
        units::volume max_contains_volume = 0_ml;
        // min volume of item that can be contained, otherwise it spills
        units::volume min_item_volume = 0_ml;
        // max weight of stuff the pocket can hold
        units::mass max_contains_weight = 0_gram;
        // multiplier for spoilage rate of contained items
        float spoil_multiplier = 1.0f;
        // items' weight in this pocket are modified by this number
        float weight_multiplier = 1.0f;
        // base time it takes to pull an item out of the pocket
        int moves = 100;
        // protects contents from exploding in a fire
        bool fire_protection = false;
        // can hold liquids
        bool watertight = false;
        // can hold gas
        bool gastight = false;
        // the pocket will spill its contents if placed in another container
        bool open_container = false;
        // allows only items with the appropriate flags to be stored inside
        // empty means no restriction
        std::vector<std::string> flag_restriction;
        // container's size and encumbrance does not change based on contents.
        bool rigid = false;

        bool operator==( const pocket_data &rhs ) const;

        void load( const JsonObject &jo );
};

template<>
struct enum_traits<item_pocket::pocket_type> {
    static constexpr auto last = item_pocket::pocket_type::LAST;
};

template<>
struct ret_val<item_pocket::contain_code>::default_success
    : public std::integral_constant<item_pocket::contain_code,
      item_pocket::contain_code::SUCCESS> {};

#endif
