#pragma once
#ifndef CATA_SRC_ITEM_CONTENTS_H
#define CATA_SRC_ITEM_CONTENTS_H

#include <cstddef>
#include <functional>
#include <iosfwd>
#include <list>
#include <map>
#include <optional>
#include <set>
#include <utility>
#include <vector>

#include "enums.h"
#include "item_pocket.h"
#include "ret_val.h"
#include "type_id.h"
#include "units_fwd.h"
#include "visitable.h"

class Character;
class JsonOut;
class item;
class item_location;
class iteminfo_query;
struct iteminfo;
struct tripoint;

class item_contents
{
    public:
        item_contents() = default;
        // used for loading itype
        explicit item_contents( const std::vector<pocket_data> &pockets );

        /**
          * returns an item_location and pointer to the best pocket that can contain the item @it
          * checks all items contained in every pocket
          * only checks CONTAINER pocket type
          */
        std::pair<item_location, item_pocket *> best_pocket( const item &it, item_location &this_loc,
                const item *avoid = nullptr, bool allow_sealed = false, bool ignore_settings = false,
                bool nested = false, bool ignore_rigidity = false );

        units::length max_containable_length( bool unrestricted_pockets_only = false ) const;
        units::length min_containable_length() const;
        units::volume max_containable_volume( bool unrestricted_pockets_only = false ) const;

        std::set<flag_id> magazine_flag_restrictions() const;
        /**
         * returns whether any of the pockets contained is compatible with the specified item.
         * Does not check if the item actually fits volume/weight wise
         * Ignores mod, migration, corpse pockets
         * @param it the item being put in
         */
        ret_val<void> is_compatible( const item &it ) const;

        /**
         * returns whether an item can be physically stored within these item contents.
         * Fails if all pockets are MOD, CORPSE, SOFTWARE, or MIGRATION type, as they are not
         * physical pockets.
         * @param it the item being put in
         * @param ignore_pkt_settings whether to ignore pocket autoinsert settings
         * @param remaining_parent_volume if we are nesting things without concern for rigidity we need to be careful about overfilling pockets
         * this tracks the remaining volume of any parent pockets
         */
        ret_val<void> can_contain( const item &it, bool ignore_pkt_settings = true,
                                   bool is_pick_up_inv = false,
                                   units::volume remaining_parent_volume = 10000000_ml ) const;
        ret_val<void> can_contain( const item &it, int &copies_remaining, bool ignore_pkt_settings = true,
                                   bool is_pick_up_inv = false,
                                   units::volume remaining_parent_volume = 10000000_ml ) const;
        ret_val<void> can_contain_rigid( const item &it, bool ignore_pkt_settings = true,
                                         bool is_pick_up_inv = false ) const;
        ret_val<void> can_contain_rigid( const item &it, int &copies_remaining,
                                         bool ignore_pkt_settings = true,
                                         bool is_pick_up_inv = false ) const;
        bool can_contain_liquid( bool held_or_ground ) const;

        bool contains_no_solids() const;

        /**
         * returns whether any of the pockets can be reloaded with the specified item.
         * @param ammo item to be loaded in
         * @param now whether the currently contained ammo/magazine should be taken into account
         */
        bool can_reload_with( const item &ammo, bool now ) const;

        // Returns true if contents are empty (ignoring item mods, since they aren't contents)
        bool empty() const;
        // Returns true if contents are empty of everything including mods
        bool empty_with_no_mods() const;
        // ignores all pockets except CONTAINER pockets to check if this contents is empty.
        bool empty_container() const;
        // checks if CONTAINER pockets are all full
        bool full( bool allow_bucket ) const;
        // Checks if MAGAZINE pockets are all full
        bool is_magazine_full() const;
        // are any CONTAINER pockets bigger on the inside than the container's volume?
        bool bigger_on_the_inside( const units::volume &container_volume ) const;
        // number of pockets
        size_t size() const;

        /** returns a list of pointers to all top-level items from pockets that match the predicate */
        std::list<item *> all_items_top( const std::function<bool( item_pocket & )> &filter );
        /** returns a list of pointers to all top-level items from pockets that match the predicate */
        std::list<const item *> all_items_top( const std::function<bool( const item_pocket & )> &filter )
        const;
        /** returns a list of pointers to all top-level items */
        /** if unloading is true it ignores items in pockets that are flagged to not unload */
        std::list<item *> all_items_top( pocket_type pk_type, bool unloading = false );
        /** returns a list of pointers to all top-level items */
        std::list<const item *> all_items_top( pocket_type pk_type ) const;

        /** returns a list of pointers to all top-level items that are not mods */
        std::list<item *> all_items_top();
        /** returns a list of pointers to all top-level items that are not mods */
        std::list<const item *> all_items_top() const;

        /** returns a list of pointers to all visible or remembered content items that are not mods */
        std::list<item *> all_known_contents();
        std::list<const item *> all_known_contents() const;

        // returns all the ablative armor in pockets
        std::list<item *> all_ablative_armor();
        std::list<const item *> all_ablative_armor() const;

        /** gets all gunmods in the item */
        std::vector<item *> gunmods();
        /** gets all gunmods in the item */
        std::vector<const item *> gunmods() const;
        // checks the pockets if this speedloader is compatible
        bool allows_speedloader( const itype_id &speedloader_id ) const;

        std::vector<const item *> mods() const;

        std::vector<const item *> softwares() const;

        std::vector<item *> ebooks();
        std::vector<const item *> ebooks() const;

        std::vector<item *> cables();
        std::vector<const item *> cables() const;

        void update_modified_pockets( const std::optional<const pocket_data *> &mag_or_mag_well,
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
        units::length item_length_modifier() const;

        // gets the total weight capacity of all pockets
        units::mass total_container_weight_capacity( bool unrestricted_pockets_only = false ) const;

        /**
          * gets the total volume available to be used.
          * does not guarantee that an item of that size can be inserted.
          */
        units::volume total_container_capacity( bool unrestricted_pockets_only = false ) const;

        // Gets the total volume of every is_standard_type container
        units::volume total_standard_capacity( bool unrestricted_pockets_only = false ) const;

        units::volume remaining_container_capacity( bool unrestricted_pockets_only = false ) const;
        units::volume total_contained_volume( bool unrestricted_pockets_only = false ) const;
        units::mass remaining_container_capacity_weight( bool unrestricted_pockets_only = false ) const;
        units::mass total_contained_weight( bool unrestricted_pockets_only = false ) const;
        units::volume get_contents_volume_with_tweaks( const std::map<const item *, int> &without ) const;
        units::volume get_nested_content_volume_recursive( const std::map<const item *, int> &without )
        const;

        // get all holsters
        int get_used_holsters() const;
        int get_total_holsters() const;
        units::volume get_total_holster_volume() const;
        units::volume get_used_holster_volume() const;
        units::mass get_total_holster_weight() const;
        units::mass get_used_holster_weight() const;

        // gets all CONTAINER pockets contained in this item
        std::vector<const item_pocket *> get_all_contained_pockets() const;
        std::vector<item_pocket *> get_all_contained_pockets();
        std::vector<const item_pocket *> get_all_standard_pockets() const;
        std::vector<item_pocket *> get_all_standard_pockets();
        std::vector<const item_pocket *> get_all_ablative_pockets() const;
        std::vector<item_pocket *> get_all_ablative_pockets();
        std::vector<const item_pocket *>
        get_pockets( std::function<bool( item_pocket const & )> const &filter ) const;
        std::vector<item_pocket *>
        get_pockets( std::function<bool( item_pocket const & )> const &filter );

        // called when adding an item as pockets
        // to a molle item
        void add_pocket( const item &pocket );

        // called when removing a molle pocket
        // needs the index of the pocket in both
        // related vectors
        // returns the item that was attached
        item remove_pocket( int index );

        // retrieves the pocket in contents corresponding to the added pocket item
        const item_pocket *get_added_pocket( int index ) const;

        std::vector<const item *> get_added_pockets() const;

        bool has_additional_pockets() const;

        int get_additional_pocket_encumbrance( float mod ) const;
        int get_additional_space_used() const;
        units::mass get_additional_weight() const;
        units::volume get_additional_volume() const;

        // Gets all CONTAINER/MAGAZINE/MAGAZINE WELL pockets in this item
        std::vector<const item_pocket *> get_all_reloadable_pockets() const;

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
         * If ignore_contents is true, will disregard other pocket contents for these checks.
         *
         * For the MOD, CORPSE, SOFTWARE, CABLE, and MIGRATION pocket types, if contents have such a
         * pocket, items will be successfully inserted without regard to volume, length, or any
         * other restrictions, since these pockets are not considered to be normal "containers".
         */
        ret_val<item *> insert_item( const item &it, pocket_type pk_type,
                                     bool ignore_contents = false, bool unseal_pockets = false );
        void force_insert_item( const item &it, pocket_type pk_type );
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
        void favorite_settings_menu( item *i );

        item_pocket *contained_where( const item &contained );
        void on_pickup( Character &guy, item *avoid = nullptr );
        bool spill_contents( const tripoint &pos );
        // spill items that don't fit in the container
        void overflow( const tripoint &pos, const item_location &loc );
        void clear_items();
        // clears all items from magazine type pockets
        void clear_magazines();
        void clear_pockets_if( const std::function<bool( item_pocket const & )> &filter );
        void update_open_pockets();

        /**
         * Sets the items contained to their defaults.
         */
        void set_item_defaults();

        // returns true if any pocket was sealed
        bool seal_all_pockets();
        bool all_pockets_sealed() const;
        bool any_pockets_sealed() const;
        // heats up the contents if they have temperature
        void heat_up();
        // returns amount of ammo consumed
        int ammo_consume( int qty, const tripoint &pos, float fuel_efficiency = -1.0 );
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
        bool will_spill_if_unsealed() const;
        bool spill_open_pockets( Character &guy, const item *avoid = nullptr );
        void casings_handle( const std::function<bool( item & )> &func );

        // gets the item contained IFF one item is contained (CONTAINER pocket), otherwise a null item reference
        item &only_item();
        item &first_item();
        const item &only_item() const;
        const item &first_item() const;
        item *get_item_with( const std::function<bool( const item & )> &filter );
        const item *get_item_with( const std::function<bool( const item & )> &filter ) const;
        void remove_items_if( const std::function<bool( item & )> &filter );

        // whether the contents has a pocket with the associated type
        bool has_pocket_type( pocket_type pk_type ) const;
        bool has_unrestricted_pockets() const;
        bool has_any_with( const std::function<bool( const item & )> &filter,
                           pocket_type pk_type ) const;

        /**
         * Is part of the recursive call of item::process. see that function for additional comments
         * NOTE: this destroys the items that get processed
         */
        void process( map &here, Character *carrier, const tripoint &pos, float insulation = 1,
                      temperature_flag flag = temperature_flag::NORMAL, float spoil_multiplier_parent = 1.0f );

        void leak( map &here, Character *carrier, const tripoint &pos, item_pocket *pocke = nullptr );

        bool item_has_uses_recursive() const;
        bool stacks_with( const item_contents &rhs, int depth = 0, int maxdepth = 2 ) const;
        bool same_contents( const item_contents &rhs ) const;
        // can this item be used as a funnel?
        bool is_funnel_container( units::volume &bigger_than ) const;
        // the container has restrictions
        bool is_restricted_container() const;
        bool is_single_restricted_container() const;
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
        void combine( const item_contents &read_input, bool convert = false, bool into_bottom = false,
                      bool restack_charges = true, bool ignore_contents = false );

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &data );
    private:
        // finds the pocket the item will fit in, given the pocket type.
        // this will be where the algorithm picks the best pocket in the contents
        // returns nullptr if none is found
        ret_val<item_pocket *> find_pocket_for( const item &it,
                                                pocket_type pk_type = pocket_type::CONTAINER );

        ret_val<const item_pocket *> find_pocket_for( const item &it,
                pocket_type pk_type = pocket_type::CONTAINER ) const;

        std::list<item_pocket> contents;

        // pockets that have been custom added
        std::vector<item> additional_pockets;
        // TODO make this work with non torso items
        units::volume additional_pockets_volume = 0_ml; // NOLINT(cata-serialize)

        // an abstraction for how many 'spaces' of this item have been used attaching additional pockets
        int additional_pockets_space_used = 0; // NOLINT(cata-serialize)

        struct item_contents_helper;

        friend struct item_contents_helper;
};

void pocket_management_menu( const std::string &title, const std::vector<item *> &to_organize );

#endif // CATA_SRC_ITEM_CONTENTS_H
