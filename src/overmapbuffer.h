#ifndef _OVERMAPBUFFER_H_
#define _OVERMAPBUFFER_H_

#include "overmap.h"

/**
 * Coordinate systems used here are:
 * overmap (om): the position of an overmap. Each overmap stores
 * this as overmap::loc (access with overmap::pos()).
 * There is a unique overmap for each overmap coord.
 *
 * overmap terrain (omt): the position of a overmap terrain (oter_id).
 * Each overmap contains (OMAPX * OMAPY) overmap terrains.
 * Translation from omt to om:
 * om.x /= OMAPX
 * om.y /= OMAPY
 * (with special handling for negative values).
 *
 * Z-components are never considered and simply copied.
 *
 * submap (sm): each overmap terrain contains (2*2) submaps.
 * Translating from sm to omt coordinates:
 * sm.x /= 2
 * sm.y /= 2
 *
 * The class provides static translation functions, named like this:
    static point <from>_to_<to>_copy(int x, int y);
    static point <from>_to_<to>_copy(const point& p);
    static tripoint <from>_to_<to>_copy(const tripoint& p);
    static void <from>_to_<to>(int &x, int &y);
    static void <from>_to_<to>(point& p);
    static void <from>_to_<to>(tripoint& p);
    static point <from>_to_<to>_remain(int &x, int &y);
    static point <from>_to_<to>_remain(point& p);
 * Functions ending with _copy return the translated coordinates,
 * other functions change the parameters itself and don't return anything.
 * Functions ending with _remain return teh translated coordinates and
 * store the remainder in the parameters.
 */
class overmapbuffer
{
public:
    overmapbuffer();

    overmap &get( const int x, const int y );
    void save();
    void clear();

    /**
     * Uses global overmap terrain coordinates, creates the
     * overmap if needed.
     */
    oter_id& ter(int x, int y, int z);
    oter_id& ter(const tripoint& p) { return ter(p.x, p.y, p.z); }

    /* These 4 functions return the overmap that contains the given
     * overmap terrain coordinate.
     * get_existing_om_global will not create a new overmap and
     * instead return NULL, if the requested overmap does not yet exist.
     * get_om_global creates a new overmap if needed.
     *
     * The parameters x and y will be cropped to be local to the
     * returned overmap, the parameter p will not be changed.
     */
    const overmap* get_existing_om_global(int& x, int& y) const;
    const overmap* get_existing_om_global(const point& p) const;
    overmap& get_om_global(int& x, int& y);
    overmap& get_om_global(const point& p);
    /**
     * (x,y) are global overmap coordinates (same as @ref get).
     * @returns true if the buffer has a overmap with
     * the given coordinates.
     */
    bool has(int x, int y) const;
    /**
     * Get an existing overmap, does not create a new one
     * and may return NULL if the requested overmap does not
     * exist.
     * (x,y) are global overmap coordinates (same as @ref get).
     */
    const overmap *get_existing(int x, int y) const;

    // overmap terrain to overmap
    static point omt_to_om_copy(int x, int y);
    static point omt_to_om_copy(const point& p) { return omt_to_om_copy(p.x, p.y); }
    static tripoint omt_to_om_copy(const tripoint& p);
    static void omt_to_om(int &x, int &y);
    static void omt_to_om(point& p) { omt_to_om(p.x, p.y); }
    static void omt_to_om(tripoint& p) { omt_to_om(p.x, p.y); }
    static point omt_to_om_remain(int &x, int &y);
    static point omt_to_om_remain(point& p) { return omt_to_om_remain(p.x, p.y); }

private:
    std::list<overmap> overmap_list;
};

extern overmapbuffer overmap_buffer;

#endif /* _OVERMAPBUFFER_H_ */
