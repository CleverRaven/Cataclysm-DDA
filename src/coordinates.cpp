#include "coordinates.h"

#include <cstdlib>

// Moved from obsolete coordinate_conversions
static point om_to_omt_copy( const point &p )
{
    return point( p.x * OMAPX, p.y * OMAPY );
}

static point omt_to_ms_copy( const point &p )
{
    return point( p.x * 2 * SEEX, p.y * 2 * SEEY );
}

// End of moved code.

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

void real_coords::fromomap( const point &rel_om, const point &rel_om_pos )
{
    const point a = om_to_omt_copy( rel_om ) + rel_om_pos;
    fromabs( omt_to_ms_copy( a ) );
}

point_rel_ms rebase_rel( point_sm_ms p )
{
    return point_rel_ms( p.raw() );
}
point_rel_ms rebase_rel( point_omt_ms p )
{
    return point_rel_ms( p.raw() );
}
point_rel_ms rebase_rel( point_bub_ms p )
{
    return point_rel_ms( p.raw() );
}
point_rel_sm rebase_rel( point_bub_sm p )
{
    return point_rel_sm( p.raw() );
}
point_sm_ms rebase_sm( point_rel_ms p )
{
    return point_sm_ms( p.raw() );
}
point_omt_ms rebase_omt( point_rel_ms p )
{
    return point_omt_ms( p.raw() );
}
point_bub_ms rebase_bub( point_rel_ms p )
{
    return point_bub_ms( p.raw() );
}
point_bub_sm rebase_bub( point_rel_sm p )
{
    return point_bub_sm( p.raw() );
}
tripoint_rel_ms rebase_rel( tripoint_sm_ms p )
{
    return tripoint_rel_ms( p.raw() );
}
tripoint_rel_ms rebase_rel( tripoint_omt_ms p )
{
    return tripoint_rel_ms( p.raw() );
}
tripoint_rel_ms rebase_rel( tripoint_bub_ms p )
{
    return tripoint_rel_ms( p.raw() );
}
tripoint_rel_sm rebase_rel( tripoint_bub_sm p )
{
    return tripoint_rel_sm( p.raw() );
}
tripoint_sm_ms rebase_sm( tripoint_rel_ms p )
{
    return tripoint_sm_ms( p.raw() );
}
tripoint_omt_ms rebase_omt( tripoint_rel_ms p )
{
    return tripoint_omt_ms( p.raw() );
}
tripoint_bub_ms rebase_bub( tripoint_rel_ms p )
{
    return tripoint_bub_ms( p.raw() );
}
tripoint_bub_sm rebase_bub( tripoint_rel_sm p )
{
    return tripoint_bub_sm( p.raw() );
}
point_bub_ms rebase_bub( point_omt_ms p )
{
    return point_bub_ms( p.raw() );
}
tripoint_bub_ms rebase_bub( tripoint_omt_ms p )
{
    return tripoint_bub_ms( p.raw() );
}

tripoint_omt_ms rebase_omt( tripoint_bub_ms p )
{
    return tripoint_omt_ms( p.raw() );
}
