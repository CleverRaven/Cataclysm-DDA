#include "magic_teleporter_list.h"

#include <cstddef>
#include <map>
#include <algorithm>
#include <memory>
#include <utility>

#include "avatar.h"
#include "coordinate_conversions.h"
#include "enums.h"
#include "game.h"
#include "game_constants.h"
#include "json.h"
#include "line.h"
#include "map.h"
#include "messages.h"
#include "map_iterator.h"
#include "output.h"
#include "panels.h"
#include "string_input_popup.h"
#include "ui.h"
#include "color.h"
#include "string_formatter.h"
#include "translations.h"

static bool popup_string( std::string &result, std::string &title )
{
    string_input_popup popup;
    popup.title( title );
    popup.text( "" ).only_digits( false );
    popup.query();
    if( popup.canceled() ) {
        return false;
    }
    result = popup.text();
    return true;
}

bool teleporter_list::activate_teleporter( const tripoint &omt_pt, const tripoint & )
{
    std::string point_name;
    std::string title = _( "Name this gate." );
    popup_string( point_name, title );
    return known_teleporters.emplace( omt_pt, point_name ).second;
}

void teleporter_list::deactivate_teleporter( const tripoint &omt_pt, const tripoint & )
{
    known_teleporters.erase( omt_pt );
}

// returns the first valid teleport location near a teleporter
// returns map square (global coordinates)
static cata::optional<tripoint> find_valid_teleporters_omt( const tripoint &omt_pt )
{
    // this is the top left hand square of the global absolute coordinate
    // of the overmap terrain we want to try to teleport to.
    // an OMT is SEEX * SEEY in size
    const tripoint sm_pt = omt_to_sm_copy( omt_pt );
    tinymap checker;
    checker.load( sm_pt, true );
    for( const tripoint &p : checker.points_on_zlevel() ) {
        if( checker.has_flag_furn( "TRANSLOCATOR", p ) ) {
            return checker.getabs( p );
        }
    }
    return cata::nullopt;
}

bool teleporter_list::place_avatar_overmap( avatar &you, const tripoint &omt_pt ) const
{
    tinymap omt_dest( 2, true );
    tripoint sm_dest = omt_to_sm_copy( omt_pt );
    omt_dest.load( sm_dest, true );
    cata::optional<tripoint> global_dest = find_valid_teleporters_omt( omt_pt );
    if( !global_dest ) {
        return false;
    }
    tripoint local_dest = omt_dest.getlocal( *global_dest ) + point( 60, 60 );
    you.add_effect( efftype_id( "ignore_fall_damage" ), 1_seconds, num_bp, false, 0, true );
    g->place_player_overmap( omt_pt );
    g->place_player( local_dest );
    return true;
}

void teleporter_list::translocate( const std::set<tripoint> &targets )
{
    if( known_teleporters.empty() ) {
        // we can't go somewhere if we don't know how to get there!
        add_msg( m_bad, _( "No translocator target known." ) );
        return;
    }
    cata::optional<tripoint> omt_dest = choose_teleport_location();
    if( !omt_dest ) {
        add_msg( _( "Teleport canceled." ) );
        return;
    }

    bool valid_targets = false;
    for( const tripoint &pt : targets ) {
        avatar *you = g->critter_at<avatar>( pt );

        if( you ) {
            valid_targets = true;
            if( !place_avatar_overmap( *you, *omt_dest ) ) {
                add_msg( _( "Failed to teleport.  Teleporter obstructed or destroyed." ) );
                deactivate_teleporter( *omt_dest, pt );
            }
        }
    }

    if( !valid_targets ) {
        add_msg( _( "No valid targets to teleport." ) );
    }
}

bool teleporter_list::knows_translocator( const tripoint &omt_pos ) const
{
    return known_teleporters.find( omt_pos ) != known_teleporters.end();
}

void teleporter_list::serialize( JsonOut &json ) const
{
    json.start_object();

    json.member( "known_teleporters" );
    json.start_array();
    for( std::pair<tripoint, std::string> pair : known_teleporters ) {
        json.start_object();
        json.member( "position", pair.first );
        json.member( "name", pair.second );
        json.end_object();
    }
    json.end_array();

    json.end_object();
}

void teleporter_list::deserialize( JsonIn &jsin )
{
    JsonObject data = jsin.get_object();

    for( JsonObject jo : data.get_array( "known_teleporters" ) ) {
        tripoint temp_pos;
        jo.read( "position", temp_pos );
        std::string name;
        jo.read( "name", name );

        known_teleporters.emplace( temp_pos, name );
    }
}

class teleporter_callback : public uilist_callback
{
    private:
        // to make it easier to get the callback from the known_teleporters
        std::map<int, tripoint> index_pairs;
    public:
        teleporter_callback( std::map<int, tripoint> &ip ) : index_pairs( ip ) {}
        void select( int entnum, uilist *menu ) override {
            const int start_x = menu->w_width - menu->pad_right;
            mvwputch( menu->window, point( start_x, 0 ), c_magenta, LINE_OXXX );
            mvwputch( menu->window, point( start_x, menu->w_height - 1 ), c_magenta, LINE_XXOX );
            for( int i = 1; i < menu->w_height - 1; i++ ) {
                mvwputch( menu->window, point( start_x, i ), c_magenta, LINE_XOXO );
            }
            overmap_ui::draw_overmap_chunk( menu->window, g->u, index_pairs[entnum], 1, start_x + 1, 29, 21 );
            mvwprintz( menu->window, point( start_x + 2, 1 ), c_white,
                       string_format( _( "Distance: %d (%d, %d)" ),
                                      rl_dist( ms_to_omt_copy( g->m.getabs( g->u.pos() ) ), index_pairs[entnum] ),
                                      index_pairs[entnum].x, index_pairs[entnum].y ) );
        }
};

cata::optional<tripoint> teleporter_list::choose_teleport_location()
{
    cata::optional<tripoint> ret = cata::nullopt;
    g->refresh_all();

    uilist teleport_selector;
    teleport_selector.w_height = 24;

    int index = 0;
    int column_width = 25;
    std::map<int, tripoint> index_pairs;
    for( const std::pair<const tripoint, std::string> &gate : known_teleporters ) {
        teleport_selector.addentry( index, true, 0, gate.second );
        column_width = std::max( column_width, utf8_width( gate.second ) );
        index_pairs.emplace( index, gate.first );
        index++;
    }
    teleporter_callback cb( index_pairs );
    teleport_selector.callback = &cb;
    teleport_selector.w_width = 38 + column_width;
    teleport_selector.pad_right = 33;
    teleport_selector.w_x = ( TERMX - teleport_selector.w_width ) / 2;
    teleport_selector.w_y = ( TERMY - teleport_selector.w_height ) / 2;
    teleport_selector.title = _( "Choose Translocator Gate" );

    teleport_selector.query();

    if( teleport_selector.ret >= 0 ) {
        ret = index_pairs[teleport_selector.ret];
    }
    return ret;
}
