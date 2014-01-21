#include "overmapbuffer.h"

overmapbuffer overmap_buffer;

overmapbuffer::overmapbuffer()
{
}

overmap &overmapbuffer::get( const int x, const int y )
{
    // Check list first
    for(std::list<overmap>::iterator candidate = overmap_list.begin();
        candidate != overmap_list.end(); ++candidate)
    {
        if(candidate->pos().x == x && candidate->pos().y == y)
        {
            return *candidate;
        }
    }
    // If we don't have one, make it, stash it in the list, and return it.
    overmap new_overmap(x, y);
    overmap_list.push_back( new_overmap );
    return overmap_list.back();
}

void overmapbuffer::save()
{
    for(std::list<overmap>::iterator current_map = overmap_list.begin();
        current_map != overmap_list.end(); ++current_map)
    {
        current_map->save();
    }
}

void overmapbuffer::clear()
{
    overmap_list.clear();
}

const regional_settings& overmapbuffer::get_settings(int x, int y, int z)
{
    overmap &om = get_om_global(x, y);
    return om.get_settings(x, y, z);
}

void overmapbuffer::add_note(int x, int y, int z, const std::string& message)
{
    overmap &om = get_om_global(x, y);
    om.add_note(x, y, z, message);
}

void overmapbuffer::delete_note(int x, int y, int z)
{
    if (has_note(x, y, z)) {
        overmap &om = get_om_global(x, y);
        om.delete_note(x, y, z);
    }
}

const overmap *overmapbuffer::get_existing(int x, int y) const
{
    static const overmap *last_one = NULL;
    if (last_one != NULL && last_one->pos().x == x && last_one->pos().y == y) {
        return last_one;
    }
    for(std::list<overmap>::const_iterator candidate = overmap_list.begin();
        candidate != overmap_list.end(); ++candidate)
    {
        if(candidate->pos().x == x && candidate->pos().y == y)
        {
            return last_one = &*candidate;
        }
    }
    return NULL;
}

bool overmapbuffer::has(int x, int y) const
{
    return get_existing(x, y) != NULL;
}

const overmap *overmapbuffer::get_existing_om_global(int &x, int &y) const
{
    const point om_pos = omt_to_om_remain(x, y);
    return get_existing(om_pos.x, om_pos.y);
}

overmap &overmapbuffer::get_om_global(int &x, int &y)
{
    const point om_pos = omt_to_om_remain(x, y);
    return get(om_pos.x, om_pos.y);
}

const overmap *overmapbuffer::get_existing_om_global(const point& p) const
{
    const point om_pos = omt_to_om_copy(p);
    return get_existing(om_pos.x, om_pos.y);
}

bool overmapbuffer::has_note(int x, int y, int z) const
{
    const overmap *om = get_existing_om_global(x, y);
    return (om != NULL) && om->has_note(x, y, z);
}

const std::string& overmapbuffer::note(int x, int y, int z) const
{
    const overmap *om = get_existing_om_global(x, y);
    if (om == NULL) {
        static const std::string empty_string;
        return empty_string;
    }
    return om->note(x, y, z);
}

bool overmapbuffer::has_npc(int x, int y, int z) const
{
    const overmap *om = get_existing_om_global(x, y);
    return (om != NULL) && om->has_npc(x, y, z);
}

bool overmapbuffer::has_vehicle(int x, int y, int z, bool require_pda) const
{
    const overmap *om = get_existing_om_global(x, y);
    return (om != NULL) && om->has_vehicle(x, y, z, require_pda);
}

bool overmapbuffer::seen(int x, int y, int z) const
{
    const overmap *om = get_existing_om_global(x, y);
    return (om != NULL) && const_cast<overmap*>(om)->seen(x, y, z);
}

void overmapbuffer::set_seen(int x, int y, int z, bool seen)
{
    overmap &om = get_om_global(x, y);
    om.seen(x, y, z) = seen;
}

overmap &overmapbuffer::get_om_global(const point& p)
{
    const point om_pos = omt_to_om_copy(p);
    return get(om_pos.x, om_pos.y);
}

oter_id& overmapbuffer::ter(int x, int y, int z) {
    overmap &om = get_om_global(x, y);
    return om.ter(x, y, z);
}

bool overmapbuffer::reveal(const point &center, int radius, int z)
{
    bool result = false;
    for (int i = -radius; i <= radius; i++) {
        for (int j = -radius; j <= radius; j++) {
            if(!seen(center.x + i, center.y + j, z)) {
                result = true;
                set_seen(center.x + i, center.y + j, z, true);
            }
        }
    }
    return result;
}

overmapbuffer::t_notes_vector overmapbuffer::get_notes(int z, const std::string* pattern) const
{
    t_notes_vector result;
    for(std::list<overmap>::const_iterator it = overmap_list.begin();
        it != overmap_list.end(); ++it)
    {
        const overmap &om = *it;
        const int offset_x = om.pos().x * OMAPX;
        const int offset_y = om.pos().y * OMAPY;
        for (int i = 0; i < OMAPX; i++) {
            for (int j = 0; j < OMAPY; j++) {
                const std::string &note = om.note(i, j, z);
                if (note.empty()) {
                    continue;
                }
                if (pattern != NULL && note.find(*pattern) == std::string::npos) {
                    // pattern not found in note text
                    continue;
                }
                result.push_back(t_point_with_note(
                    point(offset_x + i, offset_y + j),
                    om.note(i, j, z)
                ));
            }
        }
    }
    return result;
}

inline int modulo(int v, int m) {
    if (v >= 0) {
        return v % m;
    }
    return (v - m + 1) % m;
}

inline int divide(int v, int m) {
    if (v >= 0) {
        return v / m;
    }
    return (v - m + 1) / m;
}

inline int divide(int v, int m, int &r) {
    const int result = divide(v, m);
    r = v - result * m;
    return result;
}

point overmapbuffer::omt_to_om_copy(int x, int y) {
    return point(divide(x, OMAPX), divide(y, OMAPY));
}

tripoint overmapbuffer::omt_to_om_copy(const tripoint& p) {
    return tripoint(divide(p.x, OMAPX), divide(p.y, OMAPY), p.z);
}

void overmapbuffer::omt_to_om(int &x, int &y) {
    x = divide(x, OMAPX);
    y = divide(y, OMAPY);
}

point overmapbuffer::omt_to_om_remain(int &x, int &y) {
    return point(divide(x, OMAPX, x), divide(y, OMAPY, y));
}



point overmapbuffer::sm_to_omt_copy(int x, int y) {
    return point(divide(x, 2), divide(y, 2));
}

tripoint overmapbuffer::sm_to_omt_copy(const tripoint& p) {
    return tripoint(divide(p.x, 2), divide(p.y, 2), p.z);
}

void overmapbuffer::sm_to_omt(int &x, int &y) {
    x = divide(x, 2);
    y = divide(y, 2);
}

point overmapbuffer::sm_to_omt_remain(int &x, int &y) {
    return point(divide(x, 2, x), divide(y, 2, y));
}



point overmapbuffer::omt_to_sm_copy(int x, int y) {
    return point(x * 2, y * 2);
}

tripoint overmapbuffer::omt_to_sm_copy(const tripoint& p) {
    return tripoint(p.x * 2, p.y * 2, p.z);
}

void overmapbuffer::omt_to_sm(int &x, int &y) {
    x *= 2;
    y *= 2;
}
