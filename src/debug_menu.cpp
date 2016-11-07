#include "debug_menu.h"

#include "action.h"
#include "coordinate_conversions.h"
#include "game.h"
#include "messages.h"
#include "overmap.h"
#include "player.h"

void debug_menu::teleport_short()
{
    const tripoint where( g->look_around() );
    if( where == tripoint_min || where == g->u.pos() ) {
        return;
    }
    g->place_player( where );
    const tripoint new_pos( g->u.pos() );
    add_msg( _( "You teleport to point (%d,%d,%d)." ), new_pos.x, new_pos.y, new_pos.z );
}

void debug_menu::teleport_long()
{
    const tripoint where( overmap::draw_overmap() );
    if( where == overmap::invalid_tripoint ) {
        return;
    }
    g->place_player_overmap( where );
    add_msg( _( "You teleport to submap (%d,%d,%d)." ), where.x, where.y, where.z );
}

void debug_menu::teleport_overmap()
{
    tripoint dir;

    if( !choose_direction( _( "Where is the desired overmap?" ), dir ) ) {
        return;
    }

    const tripoint offset( OMAPX * dir.x, OMAPY * dir.y, dir.z );
    const tripoint where( g->u.global_omt_location() + offset );

    g->place_player_overmap( where );

    const tripoint new_pos( omt_to_om_copy( g->u.global_omt_location() ) );
    add_msg( _( "You teleport to overmap (%d,%d,%d)." ), new_pos.x, new_pos.y, new_pos.z );
}
