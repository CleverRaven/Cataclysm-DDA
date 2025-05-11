#pragma once
#ifndef CATA_SRC_VISITABLE_H
#define CATA_SRC_VISITABLE_H

#include <climits>
#include <functional>
#include <list>
#include <utility>
#include <vector>

#include "cata_utility.h"
#include "type_id.h"

class Character;
class item;

enum class VisitResponse : int {
    ABORT, // Stop processing after this node
    NEXT,  // Descend vertically to any child nodes and then horizontally to next sibling
    SKIP   // Skip any child nodes and move directly to the next sibling
};

/**
 * Read-only interface for the "container of items".
 * Provides API for the traversal and querying of the items hierarchy.
 */
class read_only_visitable
{
    public:
        /**
         * Traverses this object and any child items contained using a visitor pattern
         * @note Implement for each class that will implement the visitable interface.
         *    Other `visit_items` variants rely on this function.
         *
         * @param func visitor function called for each node which controls whether traversal continues.
         * The first argument is the node and the second is the parent node (if any)
         * The visitor function should return VisitResponse::Next to recursively process child items,
         * VisitResponse::Skip to ignore children of the current node or VisitResponse::Abort to skip all remaining nodes
         * @return This method itself only ever returns VisitResponse::Next or VisitResponse::Abort.
         */
        virtual VisitResponse visit_items(
            const std::function<VisitResponse( item *, item * )> &func ) const = 0;

        /**
         * Determine the immediate parent container (if any) for an item.
         * @param it item to search for which must be contained (at any depth) by this object
         * @return parent container or nullptr if the item is not within a container
         */
        item *find_parent( const item &it ) const;

        /**
         * Returns vector of parent containers (if any) starting with the innermost
         * @param it item to search for which must be contained (at any depth) by this object
         */
        std::vector<item *> parents( const item &it ) const;

        /** Returns true if this visitable instance contains the item */
        bool has_item( const item &it ) const;

        /** Returns true if any item (including those within a container) matches the filter */
        bool has_item_with( const std::function<bool( const item & )> &filter ) const;

        /** Returns true if instance has amount (or more) items of at least quality level */
        virtual bool has_quality( const quality_id &qual, int level = 1, int qty = 1 ) const;

        /** Return maximum tool quality level provided by instance or INT_MIN if not found */
        virtual int max_quality( const quality_id &qual ) const;

        std::pair<int, int> kcal_range( const itype_id &id,
                                        const std::function<bool( const item & )> &filter, Character &player_character ) const;
        /**
         * Count maximum available charges from this instance and any contained items
         * @param what ID of item to count charges of
         * @param limit stop searching after this many charges have been found
         * @param filter only count charges of items that match the filter
         * @param visitor is called when UPS charge is used (parameter is the charge itself)
         */
        virtual int charges_of( const itype_id &what, int limit = INT_MAX,
                                const std::function<bool( const item & )> &filter = return_true<item>,
                                const std::function<void( int )> &visitor = nullptr, bool in_tools = false ) const;

        /**
         * Count items matching id including both this instance and any contained items
         * @param what ID of items to count. "any" will count all items (usually used with a filter)
         * @param pseudo whether pseudo-items (from map/vehicle tiles, bionics etc) are considered
         * @param limit stop searching after this many matches
         * @param filter only count items that match the filter
         * @note items must be empty to be considered a match
         */
        virtual int amount_of( const itype_id &what, bool pseudo = true,
                               int limit = INT_MAX,
                               const std::function<bool( const item & )> &filter = return_true<item> ) const;

        /** Check instance provides at least qty of an item (@see amount_of) */
        bool has_amount( const itype_id &what, int qty, bool pseudo = true,
                         const std::function<bool( const item & )> &filter = return_true<item> ) const;

        /** Returns all items (including those within a container) matching the filter */
        std::vector<item *> items_with( const std::function<bool( const item & )> &filter );
        std::vector<const item *> items_with( const std::function<bool( const item & )> &filter ) const;

        virtual ~read_only_visitable() = default;

        bool has_tools( const itype_id &it, int quantity,
                        const std::function<bool( const item & )> &filter = return_true<item> ) const;
        bool has_components( const itype_id &it, int quantity,
                             const std::function<bool( const item & )> &filter = return_true<item> ) const;

        virtual bool has_charges( const itype_id &it, int quantity,
                                  const std::function<bool( const item & )> &filter = return_true<item> ) const;

};

/**
 * Read-write interface for the "container of items".
 * Provides API for the traversal and querying of the items hierarchy.
 * Also provides means to modify the hierarchy.
 */
class visitable : public read_only_visitable
{
    public:
        /**
           * Removes items contained by this instance which match the filter
           *
           * @note: Implement for each class that will implement the visitable interface
           * @note if this instance itself is an item it will not be considered by the filter
           *
           * @param filter a UnaryPredicate which should return true if the item is to be removed
           * @param count maximum number of items to if unspecified unlimited. A count of zero is a no-op
           * @return any items removed (items counted by charges are not guaranteed to be stacked)
           */
        virtual std::list<item> remove_items_with( const std::function<bool( const item & )> &filter,
                int count = INT_MAX ) = 0;

        /** Removes and returns the item which must be contained by this instance */
        item remove_item( item &it );
};

#endif // CATA_SRC_VISITABLE_H
