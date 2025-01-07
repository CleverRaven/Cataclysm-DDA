#pragma once
#ifndef CATA_SRC_CONTENTS_CHANGE_HANDLER_H
#define CATA_SRC_CONTENTS_CHANGE_HANDLER_H

#include <vector>

#include "item_location.h"

class Character;
class JsonOut;
class JsonValue;

/**
 * Records a batch of unsealed containers and handles spilling at once. This
 * is preferred over handling containers right after unsealing because the latter
 * can cause items to be invalidated when later code still needs to access them.
 * See @ref `Character::handle_contents_changed` for more detail.
 */
class contents_change_handler
{
    public:
        contents_change_handler() = default;
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
         * Let the guy handle any container that needs spilling. This may invalidate
         * items in and out of the list of containers. The list is cleared after handling.
         */
        void handle_by( Character &guy );
        /**
         * Serialization for activities
         */
        void serialize( JsonOut &jsout ) const;
        /**
         * Deserialization for activities
         */
        void deserialize( const JsonValue &jsin );
    private:
        std::vector<item_location> unsealed;
};

#endif // CATA_SRC_CONTENTS_CHANGE_HANDLER_H
