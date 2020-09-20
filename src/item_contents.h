#pragma once
#ifndef CATA_SRC_ITEM_CONTENTS_H
#define CATA_SRC_ITEM_CONTENTS_H

#include <cstddef>
#include <functional>
#include <list>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "enums.h"
#include "item_pocket.h"
#include "iteminfo_query.h"
#include "optional.h"
#include "ret_val.h"
#include "type_id.h"
#include "units_fwd.h"
#include "visitable.h"

class Character;
class JsonIn;
class JsonOut;
class item;
class item;
class item_location;
class iteminfo_query;
class player;
class pocket_data;
struct iteminfo;
struct tripoint;
struct tripoint;

class item_contents
{
    public:
        item_contents() = default;
        // used for loading itype
        item_contents( const std::vector<pocket_data> &pockets );

        /**
          * returns an item_location and pointer to the best pocket that can contain the item @it
          * only checks CONTAINER pocket type
          */
        std::pair<item_location, item_pocket *> best_pocket( const item &it, item_location &parent,
                bool nested );
        /**
         * returns whether an item can be physically stored within these item contents.
         * Fails if all pockets are MOD, CORPSE, SOFTWARE, or MIGRATION type, as they are not
         * physical pockets.
         */
        ret_val<bool> can_contain( const item &it ) const;
        ret_val<bool> can_contain_rigid( const item &it ) const;
        bool can_contain_liquid( bool held_or_ground ) const;
        // does not ignore mods
        bool empty_real() const;
        bool empty() const;
        // ignores all pockets except CONTAINER pockets to check if this contents is empty.
        bool empty_container() const;
        // checks if CONTAINER pockets are all full
        bool full( bool allow_bucket ) const;
        // are any CONTAINER pockets bigger on the inside than the container's volume?
        bool bigger_on_the_inside( const units::volume &container_volume ) const;
        // number of pockets
        size_t size() const;

        /** returns a list of pointers to all top-level items */
        std::list<item *> all_items_top( item_pocket::pocket_type pk_type );
        /** returns a list of pointers to all top-level items */
        std::list<const item *> all_items_top( item_pocket::pocket_type pk_type ) const;

        // returns a list of pointers to all top level items that pass is_standard_type
        std::list<const item *> all_standard_items_top() const;

        /** returns a list of pointers to all top-level items that are not mods */
        std::list<item *> all_items_top();
        /** returns a list of pointers to all top-level items that are not mods */
        std::list<const item *> all_items_top() const;

        // returns a list of pointers to all items inside recursively
        std::list<item *> all_items_ptr( item_pocket::pocket_type pk_type );
        // returns a list of pointers to all items inside recursively
        std::list<const item *> all_items_ptr( item_pocket::pocket_type pk_type ) const;
        // returns a list of pointers to all items inside recursively
        // includes mods.  used for item_location::unpack()
        std::list<const item *> all_items_ptr() const;

        /** gets all gunmods in the item */
        std::vector<item *> gunmods();
        /** gets all gunmods in the item */
        std::vector<const item *> gunmods() const;

        std::vector<const item *> mods() const;

        std::vector<const item *> softwares() const;

        void update_modified_pockets( const cata::optional<const pocket_data *> &mag_or_mag_well,
                                      std::vector<const pocket_data *> container_pockets );
        // all magazines compatible with any pockets.
        // this only checks MAGAZINE_WELL
        std::set<itype_id> magazine_compatible() const;
        // returns the default magazine; assumes only one MAGAZINE_WELL. returns NULL_ID if not a magazine well or no compatible magazines.
        itype_id magazine_default() const;
        /**
         * This function is to aid migration to using nested containers.
         * The call sites of this function need to be updated to search the
         * pockets of the item, or not assume there is only one pocket or item.
         */
        item &legacy_front();
        const item &legacy_front() const;

        units::volume item_size_modifier() const;
        units::mass item_weight_modifier() const;

        // gets the total weight capacity of all pockets
        units::mass total_container_weight_capacity() const;

        /**
          * gets the total volume available to be used.
          * does not guarantee that an item of that size can be inserted.
          */
        units::volume total_container_capacity() const;

        // Gets the total volume of every is_standard_type container
        units::volume total_standard_capacity() const;

        units::volume remaining_container_capacity() const;
        units::volume total_contained_volume() const;
        units::volume get_contents_volume_with_tweaks( const std::map<const item *, int> &without ) const;
        units::volume get_nested_content_volume_recursive( const std::map<const item *, int> &without )
        const;

        // gets all pockets contained in this item
        ret_val<std::vector<const item_pocket *>> get_all_contained_pockets() const;
        ret_val<std::vector<item_pocket *>> get_all_contained_pockets();

        // gets the number of charges of liquid that can fit into the rest of the space
        int remaining_capacity_for_liquid( const item &liquid ) const;

        /** If contents should contribute to encumbrance, returns a value
         * between 0 and 1 indicating the position between minimum and maximum
         * contribution it's currently making.  Otherwise, return 0 */
        float relative_encumbrance() const;
        /** True if every pocket is rigid or we have no pockets */
        bool all_pockets_rigid() const;

        /** returns the best quality of the id that's contained in the item in CONTAINER pockets */
        int best_quality( const quality_id &id ) const;

        // what will the move cost be of taking @it out of this container?
        // should only be used from item_location if possible, to account for
        // player inventory handling penalties from traits
        int obtain_cost( const item &it ) const;
        // what will the move cost be of storing @it into this container? (CONTAINER pocket type)
        int insert_cost( const item &it ) const;

        /**
         * Attempts to insert an item into these item contents, stipulating that it must go into a
         * pocket of the given type.  Fails if the contents have no pocket with that type.
         *
         * With CONTAINER, MAGAZINE, or MAGAZINE_WELL pocket types, items must fit the pocket's
         * volume, length, weight, ammo type, and all other physical restrictions.  This is
         * synonymous with the success of item_contents::can_contain with that item.
         *
         * For the MOD, CORPSE, SOFTWARE, and MIGRATION pocket types, if contents have such a
         * pocket, items will be successfully inserted without regard to volume, length, or any
         * other restrictions, since these pockets are not considered to be normal "containers".
         */
        ret_val<bool> insert_item( const item &it, item_pocket::pocket_type pk_type );
        void force_insert_item( const item &it, item_pocket::pocket_type pk_type );
        // fills the contents to the brim of this item
        void fill_with( const item &contained );
        bool can_unload_liquid() const;

        /**
         * returns the number of items stacks in contents
         * each item that is not count_by_charges,
         * plus whole stacks of items that are
         */
        size_t num_item_stacks() const;

        /**
         * Open a menu for the player to set pocket favorite settings for the pockets in this item_contents
         */
        void favorite_settings_menu( const std::string &item_name );

        item_pocket *contained_where( const item &contained );
        void on_pickup( Character &guy );
        bool spill_contents( const tripoint &pos );
        // spill items that don't fit in the container
        void overflow( const tripoint &pos );
        void clear_items();
        void update_open_pockets();

        /**
         * Sets the items contained to their defaults.
         */
        void set_item_defaults();

        // returns true if any pocket was sealed
        bool seal_all_pockets();

        enum class sealed_summary {
            unsealed,
            part_sealed,
            all_sealed,
        };
        sealed_summary get_sealed_summary() const;

        // heats up the contents if they have temperature
        void heat_up();
        // returns amount of ammo consumed
        int ammo_consume( int qty, const tripoint &pos );
        item *magazine_current();
        std::set<ammotype> ammo_types() const;
        int ammo_capacity( const ammotype &ammo ) const;
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
        // returns true if any of the pockets will spill if placed into a pocket
        bool will_spill() const;
        bool spill_open_pockets( Character &guy, const item *avoid = nullptr );
        void casings_handle( const std::function<bool( item & )> &func );

        // gets the item contained IFF one item is contained (CONTAINER pocket), otherwise a null item reference
        item &only_item();
        const item &only_item() const;
        item *get_item_with( const std::function<bool( const item & )> &filter );
        void remove_items_if( const std::function<bool( item & )> &filter );

        // whether the contents has a pocket with the associated type
        bool has_pocket_type( item_pocket::pocket_type pk_type ) const;
        bool has_any_with( const std::function<bool( const item & )> &filter,
                           item_pocket::pocket_type pk_type ) const;

        /**
         * Is part of the recursive call of item::process. see that function for additional comments
         * NOTE: this destroys the items that get processed
         */
        void process( player *carrier, const tripoint &pos, float insulation = 1,
                      temperature_flag flag = temperature_flag::NORMAL, float spoil_multiplier_parent = 1.0f );

        bool item_has_uses_recursive() const;
        bool stacks_with( const item_contents &rhs ) const;
        bool same_contents( const item_contents &rhs ) const;
        // can this item be used as a funnel?
        bool is_funnel_container( units::volume &bigger_than ) const;
        /**
         * @relates visitable
         * NOTE: upon expansion, this may need to be filtered by type enum depending on accessibility
         */
        VisitResponse visit_contents( const std::function<VisitResponse( item *, item * )> &func,
                                      item *parent = nullptr );
        void remove_internal( const std::function<bool( item & )> &filter,
                              int &count, std::list<item> &res );

        void info( std::vector<iteminfo> &info, const iteminfo_query *parts ) const;

        // reads the items in the MOD pocket first
        void read_mods( const item_contents &read_input );
        void combine( const item_contents &read_input, bool convert = false );

        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );
    private:
        // finds the pocket the item will fit in, given the pocket type.
        // this will be where the algorithm picks the best pocket in the contents
        // returns nullptr if none is found
        ret_val<item_pocket *> find_pocket_for( const item &it,
                                                item_pocket::pocket_type pk_type = item_pocket::pocket_type::CONTAINER );

        ret_val<const item_pocket *> find_pocket_for( const item &it,
                item_pocket::pocket_type pk_type = item_pocket::pocket_type::CONTAINER ) const;

        //called by all_items_ptr to recursively get all items without duplicating items in nested pockets
        std::list<const item *> all_items_top_recursive( item_pocket::pocket_type pk_type ) const;
        //called by all_items_ptr to recursively get all items without duplicating items in nested pockets
        std::list<item *> all_items_top_recursive( item_pocket::pocket_type pk_type );

        std::list<item_pocket> contents;

        struct item_contents_helper;

        friend struct item_contents_helper;
};

#endif // CATA_SRC_ITEM_CONTENTS_H
