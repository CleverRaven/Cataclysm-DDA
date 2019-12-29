#pragma once
#ifndef ITEM_CONTENTS_H
#define ITEM_CONTENTS_H

#include "enums.h"
#include "item_pocket.h"
#include "optional.h"
#include "ret_val.h"
#include "units.h"
#include "visitable.h"

class item;
class item_location;
class player;

struct iteminfo;
struct tripoint;

class item_contents
{
    public:
        item_contents() {
            // items should have a legacy pocket until everything is migrated
            add_legacy_pocket();
        }

        // used for loading itype
        item_contents( const std::vector<pocket_data> &pockets ) {
            for( const pocket_data &data : pockets ) {
                contents.push_back( item_pocket( &data ) );
            }
            add_legacy_pocket();
        }

        // for usage with loading to aid migration
        void add_legacy_pocket();

        bool stacks_with( const item_contents &rhs ) const;

        ret_val<bool> can_contain( const item &it ) const;
        bool empty() const;

        // all the items contained in each pocket combined into one list
        std::list<item> all_items();
        std::list<item> all_items() const;
        // all item pointers in a specific pocket type
        // used for inventory screen
        std::list<item *> all_items_ptr( item_pocket::pocket_type pk_type );
        std::list<const item *> all_items_ptr( item_pocket::pocket_type pk_type ) const;
        std::list<item *> all_items_ptr();
        std::list<const item *> all_items_ptr() const;

        // total size the parent item needs to be modified based on rigidity of pockets
        units::volume item_size_modifier() const;
        units::volume total_container_capacity() const;
        // total weight the parent item needs to be modified based on weight modifiers of pockets
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

        // tries to put an item in a pocket. returns false on failure
        // has similar code to can_contain in order to avoid running it twice
        ret_val<bool> insert_item( const item &it );
        // finds or makes a fake pocket and puts this item into it
        void insert_legacy( const item &it );
        // equivalent to contents.back() when item::contents was a std::list<item>
        std::list<item> &legacy_items();
        item &legacy_back();
        const item &legacy_back() const;
        item &legacy_front();
        const item &legacy_front() const;
        size_t legacy_size() const;
        // ignores legacy_pocket, so -1
        size_t size() const;
        void legacy_pop_back();
        size_t num_item_stacks() const;
        bool spill_contents( const tripoint &pos );
        void clear_items();
        bool has_item( const item &it ) const;
        item *get_item_with( const std::function<bool( const item & )> &filter );
        void remove_items_if( const std::function<bool( item & )> &filter );
        void has_rotten_away( const tripoint &pnt );

        int obtain_cost( const item &it ) const;

        void remove_internal( const std::function<bool( item & )> &filter,
                              int &count, std::list<item> &res );
        // @relates visitable
        // NOTE: upon expansion, this may need to be filtered by type enum depending on accessibility
        VisitResponse visit_contents( const std::function<VisitResponse( item *, item * )> &func,
                                      item *parent = nullptr );

        void info( std::vector<iteminfo> &info ) const;

        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );

        bool was_loaded;
    private:
        // gets the pocket described as legacy, or creates one
        item_pocket &legacy_pocket();
        const item_pocket &legacy_pocket() const;

        std::list<item_pocket> contents;
};

#endif
