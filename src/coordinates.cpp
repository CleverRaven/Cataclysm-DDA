#include "coordinates.h"

#include <cstdlib>

void real_coords::fromabs( const point &abs )
{
    const point norm( std::abs( abs.x ), std::abs( abs.y ) );
    abs_pos = abs;

    if( abs.x < 0 ) {
        abs_sub.x = ( abs.x - SEEX + 1 ) / SEEX;
        sub_pos.x = SEEX - 1 - ( ( norm.x - 1 ) % SEEX );
        abs_om.x = ( abs_sub.x - subs_in_om_n ) / subs_in_om;
        om_sub.x = subs_in_om_n - ( ( ( norm.x - 1 ) / SEEX ) % subs_in_om );
    } else {
        abs_sub.x = norm.x / SEEX;
        sub_pos.x = abs.x % SEEX;
        abs_om.x = abs_sub.x / subs_in_om;
        om_sub.x = abs_sub.x % subs_in_om;
    }
    om_pos.x = om_sub.x / 2;

    if( abs.y < 0 ) {
        abs_sub.y = ( abs.y - SEEY + 1 ) / SEEY;
        sub_pos.y = SEEY - 1 - ( ( norm.y - 1 ) % SEEY );
        abs_om.y = ( abs_sub.y - subs_in_om_n ) / subs_in_om;
        om_sub.y = subs_in_om_n - ( ( ( norm.y - 1 ) / SEEY ) % subs_in_om );
    } else {
        abs_sub.y = norm.y / SEEY;
        sub_pos.y = abs.y % SEEY;
        abs_om.y = abs_sub.y / subs_in_om;
        om_sub.y = abs_sub.y % subs_in_om;
    }
    om_pos.y = om_sub.y / 2;
}
