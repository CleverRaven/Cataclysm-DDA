#ifndef COORDINATES_H
#define COORDINATES_H

#include "mapdata.h"
#include "overmap.h"

/* find appropriate subdivided coordinates for absolute tile coordinate.
 * This is less obvious than one might think, for negative coordinates, so this
 * was created to give a definitive answer.
 *
 * 'absolute' is defined as the -actual- submap x,y * 12 + position in submap, and
 * can be obtained from map.getabs(x, y);
 *   usage:
 *    real_coords rc( g->m.getabs(g->u.posx, g->u.posy ) );
 */
struct real_coords {
    static const int tiles_in_sub = SEEX;
    static const int tiles_in_sub_n = tiles_in_sub - 1;
    static const int subs_in_om = OMAPX * 2;
    static const int subs_in_om_n = subs_in_om - 1;

    tripoint abs_pos;     // 1 per tile, starting from tile 0,0 of submap 0,0 of overmap 0,0
    tripoint abs_sub;     // submap: 12 tiles.
    tripoint abs_om;      // overmap: 360 submaps.

    tripoint sub_pos;     // coordinate (0-11) in submap / abs_pos constrained to % 12.

    tripoint om_pos;      // overmap tile: 2x2 submaps.
    tripoint om_sub;      // submap (0-359) in overmap / abs_sub constrained to % 360. equivalent to g->levx

    real_coords()
    {
    }

    real_coords(tripoint ap)
    {
        fromabs( ap.x, ap.y, ap.z );
    }
    void fromabs (tripoint point){
        fromabs(point.x, point.y, point.z);
    }
    void fromabs(const int absx, const int absy, const int absz)
    {
        const int normx = abs(absx);
        const int normy = abs(absy);
        const int normz = abs(absz);
        abs_pos = tripoint(absx, absy, absz);

        if ( absx < 0 ) {
            abs_sub.x = (absx - 11) / 12;
            sub_pos.x = 11 - ((normx - 1) % 12);
            abs_om.x = (abs_sub.x - subs_in_om_n) / subs_in_om;
            om_sub.x = subs_in_om_n - (((normx - 1) / 12) % subs_in_om);
        } else {
            abs_sub.x = normx / 12;
            sub_pos.x = absx % 12;
            abs_om.x = abs_sub.x / subs_in_om;
            om_sub.x = abs_sub.x % subs_in_om;
        }
        om_pos.x = om_sub.x / 2;

        if ( absy < 0 ) {
            abs_sub.y = (absy - 11) / 12;
            sub_pos.y = 11 - ((normy - 1) % 12);
            abs_om.y = (abs_sub.y - subs_in_om_n) / subs_in_om;
            om_sub.y = subs_in_om_n - (((normy - 1) / 12) % subs_in_om);
        } else {
            abs_sub.y = normy / 12;
            sub_pos.y = absy % 12;
            abs_om.y = abs_sub.y / subs_in_om;
            om_sub.y = abs_sub.y % subs_in_om;
        }
        om_pos.y = om_sub.y / 2;
        if ( absz < 0) {
            abs_sub.z = (absz - 11) / 12;
            sub_pos.z = 11 - ((normz - 1) % 12);
            abs_om.z = (abs_sub.z - subs_in_om_n) / subs_in_om;
            om_sub.z = subs_in_om_n - (((normz - 1) / 12) % subs_in_om);
        } else {
            abs_sub.z = normz / 12;
            sub_pos.z = absz % 12;
            abs_om.z = abs_sub.z / subs_in_om;
            om_sub.z = abs_sub.z % subs_in_om;
        }
    }

    void fromabs(point absolute)
    {
        fromabs(absolute.x, absolute.y, 0);
    }

    // specifically for the subjective position returned by overmap::draw
    void fromomap( int rel_omx, int rel_omy, int rel_om_posx, int rel_om_posy )
    {
        int ax = (rel_omx * OMAPX) + rel_om_posx;
        int ay = (rel_omy * OMAPY) + rel_om_posy;
        fromabs(ax * 24, ay * 24, 0);
    }

    // helper functions to return abs_pos of submap/overmap tile/overmap's start

    tripoint begin_sub()
    {
        return tripoint( abs_sub.x * tiles_in_sub, abs_sub.y * tiles_in_sub, abs_sub.z * tiles_in_sub );
    }
    tripoint begin_om_pos()
    {
        return tripoint( (abs_om.x * subs_in_om * tiles_in_sub) + ( om_pos.x * 2 * tiles_in_sub ),
                      (abs_om.y * subs_in_om * tiles_in_sub) + ( om_pos.y * 2 * tiles_in_sub ),
                      (abs_om.z * subs_in_om * tiles_in_sub) + ( om_pos.z * 2 * tiles_in_sub));
    }
    tripoint begin_om()
    {
        return tripoint( abs_om.x * subs_in_om * tiles_in_sub, abs_om.y * subs_in_om * tiles_in_sub, abs_om.z * subs_in_om * tiles_in_sub);
    }
};
#endif
