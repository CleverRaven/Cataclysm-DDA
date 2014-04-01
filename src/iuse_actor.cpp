#include "iuse_actor.h"
#include "item.h"
#include "game.h"

iuse_transform::~iuse_transform()
{
}

iuse_actor *iuse_transform::clone() const
{
    return new iuse_transform(*this);
}

long iuse_transform::use(player* p, item* it, bool /*t*/) const
{
    if (need_fire > 0 && p != NULL && p->is_underwater()) {
        g->add_msg_if_player(p, _("You can't do that while underwater"));
        return 0;
    }
    if (need_charges > 0 && it->charges < need_charges) {
        if (!need_charges_msg.empty()) {
            g->add_msg_if_player(p, need_charges_msg.c_str(), it->tname().c_str());
        }
        return 0;
    }
    if (p != NULL && need_fire > 0 && !p->use_charges_if_avail("fire", need_fire)) {
        if (!need_fire_msg.empty()) {
            g->add_msg_if_player(p, need_fire_msg.c_str(), it->tname().c_str());
        }
        return 0;
    }
    // load this from the original item, not the transformed one.
    const int charges_to_use = it->type->charges_to_use();
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
    return charges_to_use;
}
