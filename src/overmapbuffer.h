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

    /**
     * Uses overmap coordinates, that means x and y are directly
     * compared with the position of the overmap.
     */
    overmap &get( const int x, const int y );
    void save();
    void clear();

    /**
     * Uses global overmap terrain coordinates, creates the
     * overmap if needed.
     */
    oter_id& ter(int x, int y, int z);
    oter_id& ter(const tripoint& p) { return ter(p.x, p.y, p.z); }
    /**
     * Uses global overmap terrain coordinates.
     */
    bool has_note(int x, int y, int z) const;
    bool has_note(const tripoint& p) const { return has_note(p.x, p.y, p.z); }
    const std::string& note(int x, int y, int z) const;
    const std::string& note(const tripoint& p) const { return note(p.x, p.y, p.z); }
    void add_note(int x, int y, int z, const std::string& message);
    void add_note(const tripoint& p, const std::string& message) { add_note(p.x, p.y, p.z, message); }
    void delete_note(int x, int y, int z);
    void delete_note(const tripoint& p) { delete_note(p.x, p.y, p.z); }
    bool seen(int x, int y, int z) const;
    void set_seen(int x, int y, int z, bool seen = true);
    bool has_npc(int x, int y, int z) const;
    bool has_vehicle(int x, int y, int z, bool require_pda = true) const;
    const regional_settings& get_settings(int x, int y, int z);

    /**
     * Mark a square area around center on z-level z
     * as seen.
     * center is in absolute overmap terrain coords.
     * @param radius The half size of the square to make visible.
     * A value of 0 makes only center visible, radius 1 makes a
     * square 3x3 visible.
     * @return true if something has actually been revealed.
     */
    bool reveal(const point &center, int radius, int z);

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

    typedef std::pair<point, std::string> t_point_with_note;
    typedef std::vector<t_point_with_note> t_notes_vector;
    t_notes_vector get_all_notes(int z) const {
        return get_notes(z, NULL); // NULL => don't filter notes
    }
    t_notes_vector find_notes(int z, const std::string& pattern) const {
        return get_notes(z, &pattern); // filter with pattern
    }

    // overmap terrain to overmap
    static point omt_to_om_copy(int x, int y);
    static point omt_to_om_copy(const point& p) { return omt_to_om_copy(p.x, p.y); }
    static tripoint omt_to_om_copy(const tripoint& p);
    static void omt_to_om(int &x, int &y);
    static void omt_to_om(point& p) { omt_to_om(p.x, p.y); }
    static void omt_to_om(tripoint& p) { omt_to_om(p.x, p.y); }
    static point omt_to_om_remain(int &x, int &y);
    static point omt_to_om_remain(point& p) { return omt_to_om_remain(p.x, p.y); }
    // submap to overmap terrain
    static point sm_to_omt_copy(int x, int y);
    static point sm_to_omt_copy(const point& p) { return sm_to_omt_copy(p.x, p.y); }
    static tripoint sm_to_omt_copy(const tripoint& p);
    static void sm_to_omt(int &x, int &y);
    static void sm_to_omt(point& p) { sm_to_omt(p.x, p.y); }
    static void sm_to_omt(tripoint& p) { sm_to_omt(p.x, p.y); }
    static point sm_to_omt_remain(int &x, int &y);
    static point sm_to_omt_remain(point& p) { return sm_to_omt_remain(p.x, p.y); }
    // overmap terrain to submap, basically: x *= 2
    static point omt_to_sm_copy(int x, int y);
    static point omt_to_sm_copy(const point& p) { return omt_to_sm_copy(p.x, p.y); }
    static tripoint omt_to_sm_copy(const tripoint& p);
    static void omt_to_sm(int &x, int &y);
    static void omt_to_sm(point& p) { omt_to_sm(p.x, p.y); }
    static void omt_to_sm(tripoint& p) { omt_to_sm(p.x, p.y); }

private:
    std::list<overmap> overmap_list;

    /**
     * Get a list of notes in the (loaded) overmaps.
     * @param z only this specific z-level is search for notes.
     * @param pattern only notes that contain this pattern are returned.
     * If the pattern is NULL, every note matches.
     */
    t_notes_vector get_notes(int z, const std::string* pattern) const;
};

extern overmapbuffer overmap_buffer;

#endif /* _OVERMAPBUFFER_H_ */
