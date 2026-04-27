#pragma once
#ifndef CATA_SRC_CONTENTS_CHANGE_HANDLER_H
#define CATA_SRC_CONTENTS_CHANGE_HANDLER_H

#include <set>
#include <vector>

#include "item_location.h"

class Character;
class JsonObject;
class JsonOut;
class JsonValue;

class item_loc_with_depth
{
    public:
        item_loc_with_depth() = default;
        explicit item_loc_with_depth( const item_location &_loc )
            : _loc( _loc ) {
            item_location ancestor = _loc;
            while( ancestor.has_parent() ) {
                ++_depth;
                ancestor = ancestor.parent_item();
            }
        }

        const item_location &loc() const {
            return _loc;
        }

        int depth() const {
            return _depth;
        }

        void serialize( JsonOut &jsout ) const;
        void deserialize( const JsonObject &obj );

    private:
        item_location _loc;
        int _depth = 0;
};

class sort_by_depth
{
    public:
        bool operator()( const item_loc_with_depth &lhs, const item_loc_with_depth &rhs ) const {
            return lhs.depth() < rhs.depth();
        }
};

/**
 * Records a batch of unsealed containers and handles spilling at once. This
 * is preferred over handling containers immediately after unsealing because the latter
 * can cause items to be invalidated when later code still needs to access them.
 *
 * You should use `contents_change_activity_actor` to utilize `contents_change_handler`
 * in a `Character` context.
 */
class contents_change_handler
{
    public:
        contents_change_handler() = default;
        // sort `unsealed` into `sorted_containers`
        void sort_containers();
        // are any containers left to process?
        bool finished() const;
        /**
         * Add an already unsealed container to the list. This item location
         * should remain valid when `handle_by` is called.
         */
        void add_unsealed( const item_location &loc );
        /**
         * Unseal the pocket containing `loc` and add `loc`'s parent to the list.
         * Does nothing if `loc` does not have a parent container. The parent of
         * `loc` should remain valid when `handle_by` is called, but `loc` only
         * needs to be valid here (for example, the item may be consumed afterwards).
         */
        void unseal_pocket_containing( const item_location &loc );
        /**
         * Let the character handle a single container that needs spilling. This may invalidate
         * items in and out of the list of containers. The list is cleared after handling.
         *
         * Check any already unsealed pockets in items pointed to by `sorted_containers`
         * and propagate the unsealed status through the container tree.
         */
        void handle_contents_changed( Character &who );
        /**
         * Serialization for activities
         */
        void serialize( JsonOut &jsout ) const;
        /**
         * Deserialization for activities
         */
        void deserialize( const JsonValue &jsin );
    private:
        /*
        * Item locations pointing to containers. Item locations
        * in this vector can contain each other, but should always be valid
        * (i.e. if A contains B and B contains C, A and C can be in the vector
        * at the same time and should be valid, but B shouldn't be invalidated
        * either, otherwise C is invalidated).Item location in this vector
        * should be unique.
        */
        std::vector<item_location> unsealed;
        // sorted, cached, ready-to-process `unsealed`
        std::multiset<item_loc_with_depth, sort_by_depth> sorted_containers;
};

#endif // CATA_SRC_CONTENTS_CHANGE_HANDLER_H
