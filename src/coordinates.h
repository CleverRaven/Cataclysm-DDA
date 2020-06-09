#pragma once
#ifndef CATA_SRC_COORDINATES_H
#define CATA_SRC_COORDINATES_H

#include <cstdlib>

#include "enums.h"
#include "game_constants.h"
#include "point.h"

/* find appropriate subdivided coordinates for absolute tile coordinate.
 * This is less obvious than one might think, for negative coordinates, so this
 * was created to give a definitive answer.
 *
 * 'absolute' is defined as the -actual- submap x,y * SEEX + position in submap, and
 * can be obtained from map.getabs(x, y);
 *   usage:
 *    real_coords rc( g->m.getabs(g->u.posx(), g->u.posy() ) );
 */
struct real_coords {
    static const int tiles_in_sub = SEEX;
    static const int tiles_in_sub_n = tiles_in_sub - 1;
    static const int subs_in_om = OMAPX * 2;
    static const int subs_in_om_n = subs_in_om - 1;

    point abs_pos;     // 1 per tile, starting from tile 0,0 of submap 0,0 of overmap 0,0
    point abs_sub;     // submap: 12 tiles.
    point abs_om;      // overmap: 360 submaps.

    point sub_pos;     // coordinate (0-11) in submap / abs_pos constrained to % 12.

    point om_pos;      // overmap tile: 2x2 submaps.
    point om_sub;      // submap (0-359) in overmap / abs_sub constrained to % 360. equivalent to g->levx

    real_coords() = default;

    real_coords( const point &ap ) {
        fromabs( ap );
    }

    void fromabs( const point &abs ) {
        const int normx = std::abs( abs.x );
        const int normy = std::abs( abs.y );
        abs_pos = abs;

        if( abs.x < 0 ) {
            abs_sub.x = ( abs.x - SEEX + 1 ) / SEEX;
            sub_pos.x = SEEX - 1 - ( ( normx - 1 ) % SEEX );
            abs_om.x = ( abs_sub.x - subs_in_om_n ) / subs_in_om;
            om_sub.x = subs_in_om_n - ( ( ( normx - 1 ) / SEEX ) % subs_in_om );
        } else {
            abs_sub.x = normx / SEEX;
            sub_pos.x = abs.x % SEEX;
            abs_om.x = abs_sub.x / subs_in_om;
            om_sub.x = abs_sub.x % subs_in_om;
        }
        om_pos.x = om_sub.x / 2;

        if( abs.y < 0 ) {
            abs_sub.y = ( abs.y - SEEY + 1 ) / SEEY;
            sub_pos.y = SEEY - 1 - ( ( normy - 1 ) % SEEY );
            abs_om.y = ( abs_sub.y - subs_in_om_n ) / subs_in_om;
            om_sub.y = subs_in_om_n - ( ( ( normy - 1 ) / SEEY ) % subs_in_om );
        } else {
            abs_sub.y = normy / SEEY;
            sub_pos.y = abs.y % SEEY;
            abs_om.y = abs_sub.y / subs_in_om;
            om_sub.y = abs_sub.y % subs_in_om;
        }
        om_pos.y = om_sub.y / 2;
    }

    // specifically for the subjective position returned by overmap::draw
    void fromomap( const point &rel_om, const point &rel_om_pos ) {
        const int ax = rel_om.x * OMAPX + rel_om_pos.x;
        const int ay = rel_om.y * OMAPY + rel_om_pos.y;
        fromabs( point( ax * SEEX * 2, ay * SEEY * 2 ) );
    }

    // helper functions to return abs_pos of submap/overmap tile/overmap's start

    point begin_sub() {
        return point( abs_sub.x * tiles_in_sub, abs_sub.y * tiles_in_sub );
    }
    point begin_om_pos() {
        return point( ( abs_om.x * subs_in_om * tiles_in_sub ) + ( om_pos.x * 2 * tiles_in_sub ),
                      ( abs_om.y * subs_in_om * tiles_in_sub ) + ( om_pos.y * 2 * tiles_in_sub ) );
    }
    point begin_om() {
        return point( abs_om.x * subs_in_om * tiles_in_sub, abs_om.y * subs_in_om * tiles_in_sub );
    }
};
#endif // CATA_SRC_COORDINATES_H
