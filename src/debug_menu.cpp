#include "debug_menu.h"

#include "action.h"
#include "coordinate_conversions.h"
#include "game.h"
#include "messages.h"
#include "overmap.h"
#include "player.h"
#include "ui.h"

#include <functional>
#include <vector>

namespace {

void teleport_short()
{
    const tripoint where( g->look_around() );
    if( where == tripoint_min || where == g->u.pos() ) {
        return;
    }
    g->place_player( where );
    const tripoint new_pos( g->u.pos() );
    add_msg( _( "You teleport to point (%d,%d,%d)." ), new_pos.x, new_pos.y, new_pos.z );
}

void teleport_long()
{
    const tripoint where( overmap::draw_overmap() );
    if( where == overmap::invalid_tripoint ) {
        return;
    }
    g->place_player_overmap( where );
    add_msg( _( "You teleport to submap (%d,%d,%d)." ), where.x, where.y, where.z );
}

} // Anonymous namespace

void debug_menu::execute_teleport()
{
    std::vector<std::function<void()>> actions;
    uimenu menu;

    menu.text = _( "Teleport where?" );
    menu.selected = 0;
    menu.return_invalid = true;

    menu.addentry( _( "Few tiles away" ) );
    actions.emplace_back( teleport_short );

    menu.addentry( _( "Some place on the overmap" ) );
    actions.emplace_back( teleport_long );

    menu.query();
    g->refresh_all();

    const size_t chosen = static_cast<size_t>( menu.ret );
    if( chosen < actions.size() ) {
        actions[chosen]();
    }
}
