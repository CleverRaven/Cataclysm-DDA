#ifndef _COORDINATES_H_
#define _COORDINATES_H_

#include <stdlib.h>
#include <vector>
#include <string>
#include <set>
#include <map>

#include "mapdata.h"
#include "overmap.h"

// translate game.levx/y + game->object.posx/y to submap and submap row/col, and deal with !INBOUND correctly
// usage: rc real_coords( levx, levy, g->u.posx, g->u.posy );
//          popup("I'm on submap %d, %d, row %d, column %d.", rc.sub.x, rc.sub.y, rc.sub_pos.x, rc.sub_pos.y );
//
//        point rcom=rc.overmap(); point gom=g->om_location();
//        popup("Oh, and this is really overmap %d,%d, but everything uses %d,%d anyway.",
//            rcom.x, rcom.y, gom.x, gom.y );
//
//        real_coords( lx, ly, px, py, true ); // will precalculate overmap as .true_om (point)
//
struct real_coords {
  static const int subsize = SEEX;
  static const int subsizen = subsize - 1;
  static const int omsize = OMAPX * 2;
  static const int omsizen = omsize - 1;
    
  point abs_pos;     // in: 1 per tile, starting from tile 0,0 of submap 0,0 of overmap 0,0
  point abs_sub;     // 12 tiles per submap
  point abs_sub_pos; // coordinate (0-11) in submap / abs_pos constrained to 12
  point abs_om;      // 360 submaps per overmap.
  point abs_om_sub;  // submap (0-359) in overmap / abs_sub constrained to 360. Divide by 2 to get overmap tile. 
  point abs_om_pos;  // overmap tile

  real_coords() {
  }

  real_coords(point ap) {
    fromabs( ap.x, ap.y );
  }
  /* find appropriate subdivided coordinates for absolute tile coordinate. This is less obvious than one might think, for negative coordinates.
   usage: 
     real_coords rc( g->m.getabs(g->u.posx, g->u.posy ) );
     or rc.fromabs( g->m, 4,-2343 );
  */

  void fromabs(const int absx, const int absy) {
    const int normx = abs(absx);
    const int normy = abs(absy);
    abs_pos = point(absx, absy);

    if ( absx < 0 ) {
      abs_sub.x = (absx-11)/12;
      abs_sub_pos.x = 11-((normx-1) % 12);
      abs_om.x = (abs_sub.x-omsizen)/omsize;
      abs_om_sub.x = omsizen-(((normx-1)/12) % omsize);
      abs_om_pos.x = abs_om_sub.x / 2;
    } else {
      abs_sub.x = normx/12;
      abs_sub_pos.x = absx % 12;
      abs_om.x = abs_sub.x/omsize;
      abs_om_sub.x = abs_sub.x % omsize;
      abs_om_pos.x = abs_om_sub.x / 2;
    }

    if ( absy < 0 ) {
      abs_sub.y = (absy-11)/12;
      abs_sub_pos.y = 11-((normy-1) % 12);
      abs_om.y = (abs_sub.y-omsizen)/omsize;
      abs_om_sub.y = omsizen-(((normy-1)/12) % omsize);
      abs_om_pos.y = abs_om_sub.y / 2;
    } else {
      abs_sub.y = normy/12;
      abs_sub_pos.y = absy % 12;
      abs_om.y=abs_sub.y/omsize;
      abs_om_sub.y = abs_sub.y % omsize;
      abs_om_pos.y = abs_om_sub.y / 2;
    }
  }

  void fromabs(point absolute) {
    fromabs(absolute.x, absolute.y);
  }

  // specifically for the subjective position returned by overmap::draw
  void fromomap( int rel_omx, int rel_omy, int rel_om_posx, int rel_om_posy ) {
    int ax=(rel_omx*OMAPX) + rel_om_posx;
    int ay=(rel_omy*OMAPY) + rel_om_posy;
    fromabs(ax*24, ay*24);
  }

};
#endif
