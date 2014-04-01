#pragma once

#include "iuse.h"

/**
 * Transform an item into a specific type.
 * Optionally activate it.
 * Optionally split it in container and content (like opening a jar).
 *
 * It optionally checks for
 * 1. original item has a minimal amount of charges.
 * 2. player has a minimal amount of "fire" charges and consumes them,
 * 3. if fire is used, checks that the player is not underwater.
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
    /** Need this many fire charges. Values <= 0 don't need fire.
     * The player must not be underwater if fire is used! */
    long need_fire;
    std::string need_fire_msg;
    /** Need this many charges before processing the action. Values <= 0 are ignored. */
    long need_charges;
    std::string need_charges_msg;

    iuse_transform()
    : iuse_actor()
    , active(false)
    , need_fire(0)
    , need_charges(0)
    {
    }
    virtual ~iuse_transform();
    virtual long use(player*, item*, bool) const;
    virtual iuse_actor *clone() const;
};

/**
 * This is a @ref iuse_transform for active items.
 * It can be called each turn.
 * It does the transformation - either when requested by the user,
 * or when the charges of the item reaches 0.
 * It can display different messages therefor.
 */
class auto_iuse_transform : public iuse_transform {
public:
    /**
     * If non-empty: check each turn if the player is underwater
     * and activate the transformation in that case.
     */
    std::string when_underwater;
    /**
     * If non-empty: don't let the user activate the transformation.
     * Instead wait for the item to trigger the transformation
     * (no charges, underwater).
     */
    std::string non_interactive_msg;

    auto_iuse_transform()
    : iuse_transform()
    {
    }
    virtual ~auto_iuse_transform();
    virtual long use(player*, item*, bool) const;
    virtual iuse_actor *clone() const;
};
