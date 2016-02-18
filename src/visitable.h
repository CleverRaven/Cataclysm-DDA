#ifndef VISITABLE_H
#define VISITABLE_H

#include <vector>
#include <functional>

#include "enums.h"

class item;

enum class VisitResponse {
    ABORT, // Stop processing after this node
    NEXT,  // Descend vertically to any child nodes and then horizontally to next sibling
    SKIP   // Skip any child nodes and move directly to the next sibling
};

template <typename T>
class visitable {
    public:
    /**
     * Traverses this object and any child items contained using a visitor pattern
     *
     * @param func visitor function called for each node which controls whether traversal continues.
     * The first argument is the node, the second is the parent node (if any) and the third is the item location
     * on the map which may be nullptr for items found within a character inventory.
     *
     * The visitor function should return VisitResponse::Next to recursively process child items,
     * VisitResponse::Skip to ignore children of the current node or VisitResponse::Abort to skip all remaining nodes
     *
     * @return This method itself only ever returns VisitResponse::Next or VisitResponse::Abort.
     */
    VisitResponse visit_items_with_loc(
        const std::function<VisitResponse(item *, item *, const tripoint *)>& func );
    VisitResponse visit_items_with_loc_const(
        const std::function<VisitResponse(const item *, const item *, const tripoint *)>& func ) const;

    /** Lightweight version which provides only the current node */
    VisitResponse visit_items( const std::function<VisitResponse(item *)>& func );
    VisitResponse visit_items_const( const std::function<VisitResponse(const item *)>& func ) const;

    /**
     * Determine the immediate parent container (if any) for an item.
     * @param it item to search for which must be contained (at any depth) by this object
     * @return parent container or nullptr if the item is not within a container
     */
    item * find_parent( const item& it );
    const item * find_parent( const item& it ) const;

    /**
     * Returns vector of parent containers (if any) starting with the innermost
     * @param it item to search for which must be contained (at any depth) by this object
     */
    std::vector<item *> parents( const item& it );
    std::vector<const item *> parents( const item& it ) const;

    /** Returns true if this visitable instance contains the item */
    bool has_item( const item& it ) const;

    /** Returns true if any item (including those within a container) matches the filter */
    bool has_item_with( const std::function<bool(const item&)>& filter ) const;
};

#endif
