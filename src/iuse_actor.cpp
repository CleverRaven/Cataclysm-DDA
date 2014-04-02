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
    item *target;
    if (container_id.empty()) {
        // No container, assume simple type transformation like foo_off -> foo_on
        it->make(itypes[target_id]);
        target = it;
    } else {
        // Transform into something in a container, assume the content is
        // "created" right now and give the content the current time as birthday
        it->make(itypes[container_id]);
        it->contents.push_back(item(itypes[target_id], g->turn));
        target = &it->contents.back();
    }
    target->active = active;
    if (target_charges > -2) {
        // -1 is for items that can not have any charges at all.
        target->charges = target_charges;
    }
    p->moves -= moves;
    return charges_to_use;
}



auto_iuse_transform::~auto_iuse_transform()
{
}

iuse_actor *auto_iuse_transform::clone() const
{
    return new auto_iuse_transform(*this);
}

long auto_iuse_transform::use(player *p, item *it, bool t) const
{
    if (t) {
        if (!when_underwater.empty() && p != NULL && p->is_underwater()) {
            // Dirty hack to display the "when unterwater" message instead of the normal message
            std::swap(const_cast<auto_iuse_transform*>(this)->when_underwater, const_cast<auto_iuse_transform*>(this)->msg_transform);
            const long tmp = iuse_transform::use(p, it, t);
            std::swap(const_cast<auto_iuse_transform*>(this)->when_underwater, const_cast<auto_iuse_transform*>(this)->msg_transform);
            return tmp;
        }
        // Normal use, don't need to do anything here.
        return 0;
    }
    if (it->charges > 0 && !non_interactive_msg.empty()) {
        g->add_msg_if_player(p, non_interactive_msg.c_str(), it->tname().c_str());
        // Activated by the player, but not allowed to do so
        return 0;
    }
    return iuse_transform::use(p, it, t);
}



explosion_iuse::~explosion_iuse()
{
}

iuse_actor *explosion_iuse::clone() const
{
    return new explosion_iuse(*this);
}

// defined in iuse.cpp
extern std::vector<point> points_for_gas_cloud(const point &center, int radius);

long explosion_iuse::use(player *p, item *it, bool t) const
{
    // This ise used for active items, their charges are autmatically
    // decremented, therfor this function always returns 0.
    const point pos = g->find_item(it);
    if (pos.x == -999 || pos.y == -999) {
        return 0;
    }
    if (t) {
        if (sound_volume >= 0) {
            g->sound(pos.x, pos.y, sound_volume, sound_msg);
        }
        return 0;
    }
    if (it->charges > 0) {
        if (no_deactivate_msg.empty()) {
            g->add_msg_if_player(p, _("You've already set the %s's timer you might want to get away from it."), it->tname().c_str());
        } else {
            g->add_msg_if_player(p, no_deactivate_msg.c_str(), it->tname().c_str());
        }
        return 0;
    }
    if (explosion_power >= 0) {
        g->explosion(pos.x, pos.y, explosion_power, explosion_shrapnel, explosion_fire, explosion_blast);
    }
    if (draw_explosion_radius >= 0) {
        g->draw_explosion(pos.x, pos.y, draw_explosion_radius, draw_explosion_color);
    }
    if (do_flashbang) {
        g->flashbang(pos.x, pos.y, flashbang_player_immune);
    }
    if (fields_radius >= 0 && fields_type != fd_null) {
        std::vector<point> gas_sources = points_for_gas_cloud(pos, fields_radius);
        for(size_t i = 0; i < gas_sources.size(); i++) {
            const point &p = gas_sources[i];
            const int dens = rng(fields_min_density, fields_max_density);
            g->m.add_field(p.x, p.y, fields_type, dens);
        }
    }
    if (scrambler_blast_radius >= 0) {
        for (int x = pos.x - scrambler_blast_radius; x <= pos.x + scrambler_blast_radius; x++) {
            for (int y = pos.y - scrambler_blast_radius; y <= pos.y + scrambler_blast_radius; y++) {
                g->scrambler_blast(x, y);
            }
        }
    }
    if (emp_blast_radius >= 0) {
        for (int x = pos.x - emp_blast_radius; x <= pos.x + emp_blast_radius; x++) {
            for (int y = pos.y - emp_blast_radius; y <= pos.y + emp_blast_radius; y++) {
                g->emp_blast(x, y);
            }
        }
    }
    return 0;
}
