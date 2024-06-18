#include "magic_teleporter_list.h"

#include <algorithm>
#include <cstddef>
#include <map>
#include <memory>
#include <new>
#include <string>
#include <utility>

#include "avatar.h"
#include "calendar.h"
#include "catacharset.h"
#include "character.h"
#include "color.h"
#include "coordinates.h"
#include "creature_tracker.h"
#include "cursesdef.h"
#include "enums.h"
#include "game.h"
#include "json.h"
#include "map.h"
#include "map_iterator.h"
#include "messages.h"
#include "output.h"
#include "panels.h"
#include "point.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "type_id.h"
#include "ui.h"

static const efftype_id effect_ignore_fall_damage( "ignore_fall_damage" );

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

bool teleporter_list::activate_teleporter( const tripoint_abs_omt &omt_pt, const tripoint & )
{
    std::string point_name;
    std::string title = _( "Name this gate." );
    popup_string( point_name, title );
    return known_teleporters.emplace( omt_pt, point_name ).second;
}

void teleporter_list::deactivate_teleporter( const tripoint_abs_omt &omt_pt, const tripoint & )
{
    known_teleporters.erase( omt_pt );
}

// returns the first valid teleport location near a teleporter
// returns map square (global coordinates)
static std::optional<tripoint> find_valid_teleporters_omt( const tripoint_abs_omt &omt_pt )
{
    // this is the top left hand square of the global absolute coordinate
    // of the overmap terrain we want to try to teleport to.
    // an OMT is (2 * SEEX) * (2 * SEEY) in size
    tinymap checker;
    checker.load( omt_pt, true );
    for( const tripoint &p : checker.points_on_zlevel() ) {
        if( checker.has_flag_furn( ter_furn_flag::TFLAG_TRANSLOCATOR, p ) ) {
            return checker.getabs( p );
        }
    }
    return std::nullopt;
}

bool teleporter_list::place_avatar_overmap( Character &you, const tripoint_abs_omt &omt_pt ) const
{
    tinymap omt_dest;
    omt_dest.load( omt_pt, true );
    std::optional<tripoint> global_dest = find_valid_teleporters_omt( omt_pt );
    if( !global_dest ) {
        return false;
    }
    tripoint local_dest = omt_dest.getlocal( *global_dest ) + point( 60, 60 );
    you.add_effect( effect_ignore_fall_damage, 1_seconds, false, 0, true );
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
    std::optional<tripoint_abs_omt> omt_dest = choose_teleport_location();
    if( !omt_dest ) {
        add_msg( _( "Teleport canceled." ) );
        return;
    }

    bool valid_targets = false;
    for( const tripoint &pt : targets ) {
        Character *you = get_creature_tracker().creature_at<Character>( pt );

        if( you && you->is_avatar() ) {
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

bool teleporter_list::knows_translocator( const tripoint_abs_omt &omt_pos ) const
{
    return known_teleporters.find( omt_pos ) != known_teleporters.end();
}

void teleporter_list::serialize( JsonOut &json ) const
{
    json.start_object();

    json.member( "known_teleporters" );
    json.start_array();
    for( const std::pair<const tripoint_abs_omt, std::string> &pair : known_teleporters ) {
        json.start_object();
        json.member( "position", pair.first );
        json.member( "name", pair.second );
        json.end_object();
    }
    json.end_array();

    json.end_object();
}

void teleporter_list::deserialize( const JsonObject &data )
{
    for( JsonObject jo : data.get_array( "known_teleporters" ) ) {
        tripoint_abs_omt temp_pos;
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
        std::map<int, tripoint_abs_omt> index_pairs;
    public:
        explicit teleporter_callback( std::map<int, tripoint_abs_omt> &ip ) : index_pairs( ip ) {}
        void refresh( uilist *menu ) override {
            const int entnum = menu->selected;
            const int start_x = menu->w_width - menu->pad_right;
            mvwputch( menu->window, point( start_x, 0 ), c_magenta, LINE_OXXX );
            mvwputch( menu->window, point( start_x, menu->w_height - 1 ), c_magenta, LINE_XXOX );
            for( int i = 1; i < menu->w_height - 1; i++ ) {
                mvwputch( menu->window, point( start_x, i ), c_magenta, LINE_XOXO );
            }
            if( entnum >= 0 && static_cast<size_t>( entnum ) < index_pairs.size() ) {
                avatar &player_character = get_avatar();
                overmap_ui::draw_overmap_chunk( menu->window, player_character, index_pairs[entnum],
                                                point( start_x + 1, 1 ),
                                                29, 21 );
                int dist = rl_dist( player_character.global_omt_location(), index_pairs[entnum] );
                mvwprintz( menu->window, point( start_x + 2, 1 ), c_white,
                           string_format( _( "Distance: %d %s" ), dist,
                                          index_pairs[entnum].to_string() ) );
            }
            wnoutrefresh( menu->window );
        }
};

std::optional<tripoint_abs_omt> teleporter_list::choose_teleport_location()
{
    std::optional<tripoint_abs_omt> ret = std::nullopt;

    uilist teleport_selector;
    teleport_selector.w_height_setup = 24;

    int index = 0;
    int column_width = 25;
    std::map<int, tripoint_abs_omt> index_pairs;
    for( const std::pair<const tripoint_abs_omt, std::string> &gate : known_teleporters ) {
        teleport_selector.addentry( index, true, 0, gate.second );
        column_width = std::max( column_width, utf8_width( gate.second ) );
        index_pairs.emplace( index, gate.first );
        index++;
    }
    teleporter_callback cb( index_pairs );
    teleport_selector.callback = &cb;
    teleport_selector.w_width_setup = 38 + column_width;
    teleport_selector.pad_right_setup = 33;
    teleport_selector.title = _( "Choose Translocator Gate" );

    teleport_selector.query();

    if( teleport_selector.ret >= 0 ) {
        ret = index_pairs[teleport_selector.ret];
    }
    return ret;
}
