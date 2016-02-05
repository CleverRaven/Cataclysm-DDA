#ifndef VISITABLE_H
#define VISITABLE_H

#include <vector>
#include <functional>

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
     * @param func visitor function called for each node which controls whether traversal continues.
     * Typically a lambda making use of captured state. The first argument is the node and the second is
     * the parent node (if any). It should return VisitResponse::Next to recursively process child items,
     * VisitResponse::Skip to ignore children of the current node or VisitResponse::Abort to skip all remaining nodes
     * @return This method itself only ever returns VisitResponse::Next or VisitResponse::Abort.
     */
    VisitResponse visit_items( const std::function<VisitResponse(item *, item *)>& func );
    VisitResponse visit_items( const std::function<VisitResponse(const item *, const item *)>& func ) const;

    /** Lightweight version which provides only the current node */
    VisitResponse visit_items( const std::function<VisitResponse(item *)>& func );
    VisitResponse visit_items( const std::function<VisitResponse(const item *)>& func ) const;

    /** Returns true if any item (including those within a container) matches the filter */
    bool has_item_with( const std::function<bool(const item&)>& filter ) const;
};

#endif
