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

overmap &overmapbuffer::get_om_global(const point& p)
{
    const point om_pos = omt_to_om_copy(p);
    return get(om_pos.x, om_pos.y);
}

oter_id& overmapbuffer::ter(int x, int y, int z) {
    overmap &om = get_om_global(x, y);
    return om.ter(x, y, z);
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
