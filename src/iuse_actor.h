#pragma once

#include "iuse.h"

class iuse_transform : public iuse_actor {
protected:
    /** Message to the player, %s is replaced with the item name */
    std::string msg_transform;
    std::string target_id;
    std::string container_id;
    bool active;
public:
    iuse_transform(
        const std::string &_msg_transform,
        const std::string &_target_id,
        const std::string &_container_id,
        bool _active
    )
    : iuse_actor()
    , msg_transform(_msg_transform)
    , target_id(_target_id)
    , container_id(_container_id)
    , active(_active)
    {
    }
    virtual ~iuse_transform() { }
    virtual long use(player*, item*, bool) const;
    virtual iuse_actor *clone() const {
        return new iuse_transform(msg_transform, target_id, container_id, active);
    }
};

long iuse_transform::use(player* p, item* it, bool /*t*/) const {
    g->add_msg_if_player(p, msg_transform.c_str(), it->tname().c_str());
    if (container_id.empty()) {
        // No container, assume simple type transformation like foo_off -> foo_on
        it->make(itypes[target_id]);
        it->active = active;
    } else {
        // Transform into something in a container, assume the content is
        // "created" right now and give the content the current time as birthday
        it->make(itypes[container_id]);
        it->contents.push_back(item(itypes[target_id], g->turn));
        it->contents.back().active = active;
    }
    return 0;
}
