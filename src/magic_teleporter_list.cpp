#include "magic_teleporter_list.h"

#include <algorithm>
#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include "avatar.h"
#include "calendar.h"
#include "cata_imgui.h"
#include "cata_utility.h"
#include "character.h"
#include "coordinates.h"
#include "creature_tracker.h"
#include "enums.h"
#include "flexbuffer_json.h"
#include "game.h"
#include "imgui/imgui.h"
#include "json.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "messages.h"
#include "output.h"
#include "panels.h"
#include "point.h"
#include "string_input_popup.h"
#include "translations.h"
#include "type_id.h"
#include "uilist.h"

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

bool teleporter_list::activate_teleporter( const tripoint_abs_omt &omt_pt, const tripoint_bub_ms & )
{
    std::string point_name;
    std::string title = _( "Name this gate." );
    popup_string( point_name, title );
    return known_teleporters.emplace( omt_pt, point_name ).second;
}

void teleporter_list::deactivate_teleporter( const tripoint_abs_omt &omt_pt,
        const tripoint_bub_ms & )
{
    known_teleporters.erase( omt_pt );
}

// returns the first valid teleport location near a teleporter
// returns map square (global coordinates)
static std::optional<tripoint_abs_ms> find_valid_teleporters_omt( const tripoint_abs_omt &omt_pt )
{
    // this is the top left hand square of the global absolute coordinate
    // of the overmap terrain we want to try to teleport to.
    // an OMT is (2 * SEEX) * (2 * SEEY) in size
    tinymap checker;
    checker.load( omt_pt, true );
    for( const tripoint_omt_ms &p : checker.points_on_zlevel() ) {
        if( checker.has_flag_furn( ter_furn_flag::TFLAG_TRANSLOCATOR, p ) ) {
            return checker.get_abs( p );
        }
    }
    return std::nullopt;
}

bool teleporter_list::place_avatar_overmap( Character &you, const tripoint_abs_omt &omt_pt ) const
{
    tinymap omt_dest;
    omt_dest.load( omt_pt, true );
    std::optional<tripoint_abs_ms> global_dest = find_valid_teleporters_omt( omt_pt );
    if( !global_dest ) {
        return false;
    }
    tripoint_omt_ms local_dest = omt_dest.get_omt( *global_dest ) + point( 60,
                                 60 );
    you.add_effect( effect_ignore_fall_damage, 1_seconds, false, 0, true );
    g->place_player_overmap( omt_pt );
    g->place_player( rebase_bub( local_dest ) );
    return true;
}

void teleporter_list::translocate( const std::set<tripoint_bub_ms> &targets )
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
    for( const tripoint_bub_ms &pt : targets ) {
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
        float desired_extra_space_right( ) override {
            return 33 * ImGui::CalcTextSize( "X" ).x;
        }
        void refresh( uilist *menu ) override {
            ImGui::TableSetColumnIndex( 2 );
            const int entnum = menu->previewing;
            if( entnum >= 0 && static_cast<size_t>( entnum ) < index_pairs.size() ) {
                avatar &player_character = get_avatar();
                int dist = rl_dist( player_character.pos_abs_omt(), index_pairs[entnum] );
                ImGui::Text( _( "Distance: %d %s" ), dist, index_pairs[entnum].to_string().c_str() );
                overmap_ui::draw_overmap_chunk_imgui( player_character, index_pairs[entnum], 29, 21 );
            }
        }
};

std::optional<tripoint_abs_omt> teleporter_list::choose_teleport_location()
{
    std::optional<tripoint_abs_omt> ret = std::nullopt;

    uilist teleport_selector;
    teleport_selector.desired_bounds = {
        -1.0,
            -1.0,
            std::max( 80, TERMX * 3 / 8 ) *ImGui::CalcTextSize( "X" ).x,
            clamp( static_cast<int>( known_teleporters.size() ), 24, TERMY * 9 / 10 ) *ImGui::GetTextLineHeightWithSpacing(),
        };

    int index = 0;
    std::map<int, tripoint_abs_omt> index_pairs;
    for( const std::pair<const tripoint_abs_omt, std::string> &gate : known_teleporters ) {
        teleport_selector.addentry( index, true, 0, gate.second );
        index_pairs.emplace( index, gate.first );
        index++;
    }
    teleporter_callback cb( index_pairs );
    teleport_selector.callback = &cb;
    teleport_selector.text = _( "Choose Translocation Location" );

    teleport_selector.query();

    if( teleport_selector.ret >= 0 ) {
        ret = index_pairs[teleport_selector.ret];
    }
    return ret;
}
