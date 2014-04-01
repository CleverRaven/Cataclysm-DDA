#pragma once

#include "iuse.h"

/**
 * Transform an item into a specific type.
 * Optionally activate it.
 * Optionally split it in container and content (like opening a jar).
 */
class iuse_transform : public iuse_actor {
public:
    /** Message to the player, %s is replaced with the item name */
    std::string msg_transform;
    /** Id of the resulting item. */
    std::string target_id;
    /** Id of the container (or empty if no container is needed).
     * If not empty, the item is transformed to the container, and a
     * new item (with type @ref target_id) is placed inside.
     * In that case the new item will have the current turn as birthday.
     */
    std::string container_id;
    /** Set the active property of the resulting item to this. */
    bool active;

    iuse_transform()
    : iuse_actor()
    , active(false)
    {
    }
    virtual ~iuse_transform();
    virtual long use(player*, item*, bool) const;
    virtual iuse_actor *clone() const;
};
