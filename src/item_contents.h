#pragma once
#ifndef CATA_SRC_ITEM_CONTENTS_H
#define CATA_SRC_ITEM_CONTENTS_H

#include <cstddef>
#include <functional>
#include <list>
#include <set>
#include <string>
#include <vector>

#include "ret_val.h"
#include "type_id.h"
#include "units.h"
#include "visitable.h"

class Character;
class JsonIn;
class JsonOut;
class item;
struct tripoint;

using itype_id = std::string;

class item_contents
{
    public:
        item_contents() = default;
        /** used to aid migration */
        item_contents( const std::list<item> &items ) : items( items ) {}

        bool empty() const;

        /** returns a list of pointers to all top-level items */
        std::list<item *> all_items_top();
        /** returns a list of pointers to all top-level items */
        std::list<const item *> all_items_top() const;

        // returns a list of pointers to all items inside recursively
        std::list<item *> all_items_ptr();
        // returns a list of pointers to all items inside recursively
        std::list<const item *> all_items_ptr() const;

        /** gets all gunmods in the item */
        std::vector<item *> gunmods();
        /** gets all gunmods in the item */
        std::vector<const item *> gunmods() const;

        /**
         * this is an artifact of the previous code using
         * front() everywhere for contents. this is to aid
         * migration to pockets. please do not use for new functions
         */
        item &front();
        const item &front() const;
        /**
         * this is an artifact of the previous code using
         * back() everywhere for contents. this is to aid
         * migration to pockets. please do not use for new functions
         */
        item &back();
        const item &back() const;

        units::volume item_size_modifier() const;
        units::mass item_weight_modifier() const;

        int best_quality( const quality_id &id ) const;

        ret_val<bool> insert_item( const item &it );

        /**
         * returns the number of items stacks in contents
         * each item that is not count_by_charges,
         * plus whole stacks of items that are
         */
        size_t num_item_stacks() const;

        bool spill_contents( const tripoint &pos );
        void clear_items();

        /**
         * Sets the items contained to their defaults.
         */
        void set_item_defaults();

        void handle_liquid_or_spill( Character &guy );
        void casings_handle( const std::function<bool( item & )> &func );

        item *get_item_with( const std::function<bool( const item & )> &filter );

        bool has_any_with( const std::function<bool( const item & )> &filter ) const;

        void migrate_item( item &obj, const std::set<itype_id> &migrations );
        bool item_has_uses_recursive() const;
        bool stacks_with( const item_contents &rhs ) const;
        /**
         * @relates visitable
         * NOTE: upon expansion, this may need to be filtered by type enum depending on accessibility
         */
        VisitResponse visit_contents( const std::function<VisitResponse( item *, item * )> &func,
                                      item *parent = nullptr );
        bool remove_internal( const std::function<bool( item & )> &filter,
                              int &count, std::list<item> &res );

        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );
    private:
        std::list<item> items;
};

#endif // CATA_SRC_ITEM_CONTENTS_H
