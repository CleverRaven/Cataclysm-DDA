#include "overmap_ui.h"

#include <cstddef>
#include <algorithm>
#include <array>
#include <chrono>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>
#include <set>
#include <type_traits>

#include "avatar.h"
#include "basecamp.h"
#include "cata_utility.h"
#include "clzones.h"
#include "coordinate_conversions.h"
#include "cursesdef.h"
#include "game.h"
#include "input.h"
#include "line.h"
#include "map_iterator.h"
#include "mapbuffer.h"
#include "mission.h"
#include "mongroup.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "sounds.h"
#include "string_input_popup.h"
#include "ui.h"
#include "uistate.h"
#include "weather.h"
#include "weather_gen.h"
#include "calendar.h"
#include "catacharset.h"
#include "color.h"
#include "game_constants.h"
#include "int_id.h"
#include "omdata.h"
#include "optional.h"
#include "overmap_types.h"
#include "regional_settings.h"
#include "rng.h"
#include "string_formatter.h"
#include "string_id.h"
#include "translations.h"
#include "type_id.h"
#include "vpart_position.h"
#include "vehicle.h"
#include "enums.h"
#include "map.h"
#include "player_activity.h"

#if defined(__ANDROID__)
#include <SDL_keyboard.h>
#endif

static constexpr int UILIST_MAP_NOTE_DELETED = -2047;
static constexpr int UILIST_MAP_NOTE_EDITED = -2048;

static constexpr int max_note_length = 450;
static constexpr int max_note_display_length = 45;

/** Note preview map width without borders. Odd number. */
static const int npm_width = 3;
/** Note preview map height without borders. Odd number. */
static const int npm_height = 3;

namespace overmap_ui
{
// {note symbol, note color, offset to text}
static std::tuple<char, nc_color, size_t> get_note_display_info( const std::string &note )
{
    std::tuple<char, nc_color, size_t> result {'N', c_yellow, 0};
    bool set_color  = false;
    bool set_symbol = false;

    size_t pos = 0;
    for( int i = 0; i < 2; ++i ) {
        // find the first non-whitespace non-delimiter
        pos = note.find_first_not_of( " :;", pos, 3 );
        if( pos == std::string::npos ) {
            return result;
        }

        // find the first following delimiter
        const auto end = note.find_first_of( " :;", pos, 3 );
        if( end == std::string::npos ) {
            return result;
        }

        // set color or symbol
        if( !set_symbol && note[end] == ':' ) {
            std::get<0>( result ) = note[end - 1];
            std::get<2>( result ) = end + 1;
            set_symbol = true;
        } else if( !set_color && note[end] == ';' ) {
            std::get<1>( result ) = get_note_color( note.substr( pos, end - pos ) );
            std::get<2>( result ) = end + 1;
            set_color = true;
        }

        pos = end + 1;
    }

    return result;
}

static std::array<std::pair<nc_color, std::string>, npm_width *npm_height> get_overmap_neighbors(
    const tripoint &current )
{
    const bool has_debug_vision = g->u.has_trait( trait_id( "DEBUG_NIGHTVISION" ) );

    std::array<std::pair<nc_color, std::string>, npm_width *npm_height> map_around;
    int index = 0;
    const point shift( npm_width / 2, npm_height / 2 );
    for( const tripoint &dest : tripoint_range( current - shift, current + shift ) ) {
        nc_color ter_color = c_black;
        std::string ter_sym = " ";
        const bool see = has_debug_vision || overmap_buffer.seen( dest );
        if( see ) {
            // Only load terrain if we can actually see it
            oter_id cur_ter = overmap_buffer.ter( dest );
            ter_color = cur_ter->get_color();
            ter_sym = cur_ter->get_symbol();
        } else {
            ter_color = c_dark_gray;
            ter_sym = "#";
        }
        map_around[index++] = std::make_pair( ter_color, ter_sym );
    }
    return map_around;
}

static void update_note_preview( const std::string &note,
                                 const std::array<std::pair<nc_color, std::string>, npm_width *npm_height> &map_around,
                                 const std::tuple<catacurses::window *, catacurses::window *, catacurses::window *>
                                 &preview_windows )
{
    auto om_symbol = get_note_display_info( note );
    const nc_color note_color = std::get<1>( om_symbol );
    const char symbol = std::get<0>( om_symbol );
    const std::string note_text = note.substr( std::get<2>( om_symbol ), std::string::npos );

    auto w_preview       = std::get<0>( preview_windows );
    auto w_preview_title = std::get<1>( preview_windows );
    auto w_preview_map   = std::get<2>( preview_windows );

    draw_border( *w_preview );
    mvwprintz( *w_preview, 1, 1, c_white, _( "Note preview" ) );
    wrefresh( *w_preview );

    werase( *w_preview_title );
    nc_color default_color = c_unset;
    print_colored_text( *w_preview_title, 0, 0, default_color, note_color, note_text );
    mvwputch( *w_preview_title, 0, note_text.length(), c_white, LINE_XOXO );
    for( size_t i = 0; i < note_text.length(); i++ ) {
        mvwputch( *w_preview_title, 1, i, c_white, LINE_OXOX );
    }
    mvwputch( *w_preview_title, 1, note_text.length(), c_white, LINE_XOOX );
    wrefresh( *w_preview_title );

    const int npm_offset_x = 1;
    const int npm_offset_y = 1;
    draw_border( *w_preview_map, c_yellow );
    for( int i = 0; i < npm_height; i++ ) {
        for( int j = 0; j < npm_width; j++ ) {
            const auto &ter = map_around[i * npm_width + j];
            mvwputch( *w_preview_map, i + npm_offset_y, j + npm_offset_x, ter.first, ter.second );
        }
    }
    mvwputch( *w_preview_map, npm_height / 2 + npm_offset_y, npm_width / 2 + npm_offset_x,
              note_color, symbol );
    wrefresh( *w_preview_map );
}

static weather_type get_weather_at_point( const tripoint &pos )
{
    // Weather calculation is a bit expensive, so it's cached here.
    static std::map<tripoint, weather_type> weather_cache;
    static time_point last_weather_display = calendar::before_time_starts;
    if( last_weather_display != calendar::turn ) {
        last_weather_display = calendar::turn;
        weather_cache.clear();
    }
    auto iter = weather_cache.find( pos );
    if( iter == weather_cache.end() ) {
        const tripoint abs_ms_pos = sm_to_ms_copy( omt_to_sm_copy( pos ) );
        const auto &wgen = overmap_buffer.get_settings( pos ).weather;
        const auto weather = wgen.get_weather_conditions( abs_ms_pos, calendar::turn, g->get_seed() );
        iter = weather_cache.insert( std::make_pair( pos, weather ) ).first;
    }
    return iter->second;
}

static bool get_scent_glyph( const tripoint &pos, nc_color &ter_color, std::string &ter_sym )
{
    auto possible_scent = overmap_buffer.scent_at( pos );
    if( possible_scent.creation_time != calendar::before_time_starts ) {
        color_manager &color_list = get_all_colors();
        int i = 0;
        time_duration scent_age = calendar::turn - possible_scent.creation_time;
        while( i < num_colors && scent_age > 0_turns ) {
            i++;
            scent_age /= 10;
        }
        ter_color = color_list.get( static_cast<color_id>( i ) );
        int scent_strength = possible_scent.initial_strength;
        char c = '0';
        while( c <= '9' && scent_strength > 0 ) {
            c++;
            scent_strength /= 10;
        }
        ter_sym = std::string( 1, c );
        return true;
    }
    // but it makes no scents!
    return false;
}

static void draw_city_labels( const catacurses::window &w, const tripoint &center )
{
    const int win_x_max = getmaxx( w );
    const int win_y_max = getmaxy( w );
    const int sm_radius = std::max( win_x_max, win_y_max );

    const point screen_center_pos( win_x_max / 2, win_y_max / 2 );

    for( const auto &element : overmap_buffer.get_cities_near( omt_to_sm_copy( center ), sm_radius ) ) {
        const point city_pos( sm_to_omt_copy( element.abs_sm_pos.xy() ) );
        const point screen_pos( city_pos - center.xy() + screen_center_pos );

        const int text_width = utf8_width( element.city->name, true );
        const int text_x_min = screen_pos.x - text_width / 2;
        const int text_x_max = text_x_min + text_width;
        const int text_y = screen_pos.y;

        if( text_x_min < 0 ||
            text_x_max > win_x_max ||
            text_y < 0 ||
            text_y > win_y_max ) {
            continue;   // outside of the window bounds.
        }

        if( screen_center_pos.x >= ( text_x_min - 1 ) &&
            screen_center_pos.x <= ( text_x_max ) &&
            screen_center_pos.y >= ( text_y - 1 ) &&
            screen_center_pos.y <= ( text_y + 1 ) ) {
            continue;   // right under the cursor.
        }

        if( !overmap_buffer.seen( tripoint( city_pos, center.z ) ) ) {
            continue;   // haven't seen it.
        }

        mvwprintz( w, text_y, text_x_min, i_yellow, element.city->name );
    }
}

static void draw_camp_labels( const catacurses::window &w, const tripoint &center )
{
    const int win_x_max = getmaxx( w );
    const int win_y_max = getmaxy( w );
    const int sm_radius = std::max( win_x_max, win_y_max );

    const point screen_center_pos( win_x_max / 2, win_y_max / 2 );

    for( const auto &element : overmap_buffer.get_camps_near( omt_to_sm_copy( center ), sm_radius ) ) {
        const point camp_pos( element.camp->camp_omt_pos().xy() );
        const point screen_pos( camp_pos - center.xy() + screen_center_pos );
        const int text_width = utf8_width( element.camp->name, true );
        const int text_x_min = screen_pos.x - text_width / 2;
        const int text_x_max = text_x_min + text_width;
        const int text_y = screen_pos.y;
        const std::string camp_name = element.camp->name;
        if( text_x_min < 0 ||
            text_x_max > win_x_max ||
            text_y < 0 ||
            text_y > win_y_max ) {
            continue;   // outside of the window bounds.
        }

        if( screen_center_pos.x >= ( text_x_min - 1 ) &&
            screen_center_pos.x <= ( text_x_max ) &&
            screen_center_pos.y >= ( text_y - 1 ) &&
            screen_center_pos.y <= ( text_y + 1 ) ) {
            continue;   // right under the cursor.
        }

        if( !overmap_buffer.seen( tripoint( camp_pos, center.z ) ) ) {
            continue;   // haven't seen it.
        }

        mvwprintz( w, text_y, text_x_min, i_white, camp_name );
    }
}

class map_notes_callback : public uilist_callback
{
    private:
        overmapbuffer::t_notes_vector _notes;
        int _z;
        int _selected = 0;
        point point_selected() {
            return _notes[_selected].first;
        }
        tripoint note_location() {
            return tripoint( point_selected(), _z );
        }
        std::string old_note() {
            return overmap_buffer.note( note_location() );
        }
    public:
        map_notes_callback( const overmapbuffer::t_notes_vector &notes, int z ) {
            _notes = notes;
            _z = z;
        }
        bool key( const input_context &ctxt, const input_event &event, int, uilist *menu ) override {
            _selected = menu->selected;
            if( _selected >= 0 && _selected < static_cast<int>( _notes.size() ) ) {
                const std::string &action = ctxt.input_to_action( event );
                if( action == "DELETE_NOTE" ) {
                    if( overmap_buffer.has_note( note_location() ) &&
                        query_yn( _( "Really delete note?" ) ) ) {
                        overmap_buffer.delete_note( note_location() );
                    }
                    menu->ret = UILIST_MAP_NOTE_DELETED;
                    return true;
                }
                if( action == "EDIT_NOTE" ) {
                    create_note( note_location() );
                    menu->ret = UILIST_MAP_NOTE_EDITED;
                    return true;
                }
            }
            return false;
        }
        void select( int, uilist *menu ) override {
            _selected = menu->selected;
            const auto map_around = get_overmap_neighbors( note_location() );
            catacurses::window w_preview =
                catacurses::newwin( npm_height + 2, max_note_display_length - npm_width - 1,
                                    point( npm_width + 2, 2 ) );
            catacurses::window w_preview_title =
                catacurses::newwin( 2, max_note_display_length + 1, point_zero );
            catacurses::window w_preview_map =
                catacurses::newwin( npm_height + 2, npm_width + 2, point( 0, 2 ) );
            const std::tuple<catacurses::window *, catacurses::window *, catacurses::window *> preview_windows =
                std::make_tuple( &w_preview, &w_preview_title, &w_preview_map );
            update_note_preview( old_note(), map_around, preview_windows );
        }
};

static point draw_notes( const tripoint &origin )
{
    point result = point_min;

    bool refresh = true;
    uilist nmenu;
    while( refresh ) {
        refresh = false;
        nmenu.init();
        g->refresh_all();
        nmenu.desc_enabled = true;
        nmenu.input_category = "OVERMAP_NOTES";
        nmenu.additional_actions.emplace_back( "DELETE_NOTE", "" );
        nmenu.additional_actions.emplace_back( "EDIT_NOTE", "" );
        const input_context ctxt( nmenu.input_category );
        nmenu.text = string_format(
                         _( "<%s> - center on note, <%s> - edit note, <%s> - delete note, <%s> - close window" ),
                         colorize( "RETURN", c_yellow ),
                         colorize( ctxt.key_bound_to( "EDIT_NOTE" ), c_yellow ),
                         colorize( ctxt.key_bound_to( "DELETE_NOTE" ), c_yellow ),
                         colorize( "ESCAPE", c_yellow )
                     );
        int row = 0;
        overmapbuffer::t_notes_vector notes = overmap_buffer.get_all_notes( origin.z );
        nmenu.title = string_format( _( "Map notes (%d)" ), notes.size() );
        for( const auto &point_with_note : notes ) {
            const point p = point_with_note.first;
            if( p.x == origin.x && p.y == origin.y ) {
                nmenu.selected = row;
            }
            const std::string &note = point_with_note.second;
            auto om_symbol = get_note_display_info( note );
            const nc_color note_color = std::get<1>( om_symbol );
            const std::string note_symbol = std::string( 1, std::get<0>( om_symbol ) );
            const std::string note_text = note.substr( std::get<2>( om_symbol ), std::string::npos );
            point p_omt( p );
            const point p_player = g->u.global_omt_location().xy();
            const int distance_player = rl_dist( p_player, p_omt );
            const point sm_pos = omt_to_sm_copy( p_omt );
            const point p_om = omt_to_om_remain( p_omt );
            const std::string location_desc =
                overmap_buffer.get_description_at( tripoint( sm_pos, origin.z ) );
            nmenu.addentry_desc( string_format( _( "[%s] %s" ), colorize( note_symbol, note_color ),
                                                note_text ),
                                 string_format(
                                     _( "<color_red>LEVEL %i, %d'%d, %d'%d</color> : %s (Distance: <color_white>%d</color>)" ),
                                     origin.z, p_om.x, p_omt.x, p_om.y, p_omt.y, location_desc, distance_player ) );
            nmenu.entries[row].ctxt = string_format(
                                          _( "<color_light_gray>Distance: </color><color_white>%d</color>" ), distance_player );
            row++;
        }
        map_notes_callback cb( notes, origin.z );
        nmenu.callback = &cb;
        nmenu.query();
        if( nmenu.ret == UILIST_MAP_NOTE_DELETED || nmenu.ret == UILIST_MAP_NOTE_EDITED ) {
            refresh = true;
        } else if( nmenu.ret >= 0 && nmenu.ret < static_cast<int>( notes.size() ) ) {
            result = notes[nmenu.ret].first;
            refresh = false;
        }
    }
    return result;
}

void draw( const catacurses::window &w, const catacurses::window &wbar, const tripoint &center,
           const tripoint &orig, bool blink, bool show_explored, bool fast_scroll, input_context *inp_ctxt,
           const draw_data_t &data )
{
    const int om_map_width  = OVERMAP_WINDOW_WIDTH;
    const int om_map_height = OVERMAP_WINDOW_HEIGHT;
    const int om_half_width = om_map_width / 2;
    const int om_half_height = om_map_height / 2;
    const bool viewing_weather = ( ( data.debug_weather || data.visible_weather ) && center.z == 10 );

    // Target of current mission
    const tripoint target = g->u.get_active_mission_target();
    const bool has_target = target != overmap::invalid_tripoint;
    // seen status & terrain of center position
    bool csee = false;
    oter_id ccur_ter = oter_str_id::NULL_ID();
    // Debug vision allows seeing everything
    const bool has_debug_vision = g->u.has_trait( trait_id( "DEBUG_NIGHTVISION" ) );
    // sight_points is hoisted for speed reasons.
    const int sight_points = !has_debug_vision ?
                             g->u.overmap_sight_range( g->light_level( g->u.posz() ) ) :
                             100;
    // Whether showing hordes is currently enabled
    const bool showhordes = uistate.overmap_show_hordes;

    const oter_id forest = oter_str_id( "forest" ).id();

    std::string sZoneName;
    tripoint tripointZone = tripoint( -1, -1, -1 );
    const auto &zones = zone_manager::get_manager();

    if( data.iZoneIndex != -1 ) {
        const auto &zone = zones.get_zones()[data.iZoneIndex].get();
        sZoneName = zone.get_name();
        tripointZone = ms_to_omt_copy( zone.get_center_point() );
    }

    // If we're debugging monster groups, find the monster group we've selected
    const mongroup *mgroup = nullptr;
    std::vector<mongroup *> mgroups;
    if( data.debug_mongroup ) {
        mgroups = overmap_buffer.monsters_at( center );
        for( const auto &mgp : mgroups ) {
            mgroup = mgp;
            if( mgp->horde ) {
                break;
            }
        }
    }

    // A small LRU cache: most oter_id's occur in clumps like forests of swamps.
    // This cache helps avoid much more costly lookups in the full hashmap.
    constexpr size_t cache_size = 8; // used below to calculate the next index
    std::array<std::pair<oter_id, oter_t const *>, cache_size> cache {{}};
    size_t cache_next = 0;

    const auto set_color_and_symbol = [&]( const oter_id & cur_ter, const tripoint & omp,
    std::string & ter_sym, nc_color & ter_color ) {
        // First see if we have the oter_t cached
        oter_t const *info = nullptr;
        for( const auto &c : cache ) {
            if( c.first == cur_ter ) {
                info = c.second;
                break;
            }
        }
        // Nope, look in the hash map next
        if( !info ) {
            info = &cur_ter.obj();
            cache[cache_next] = std::make_pair( cur_ter, info );
            cache_next = ( cache_next + 1 ) % cache_size;
        }
        // Ok, we found something
        if( info ) {
            const bool explored = show_explored && overmap_buffer.is_explored( omp );
            ter_color = explored ? c_dark_gray : info->get_color( uistate.overmap_show_land_use_codes );
            ter_sym = info->get_symbol( uistate.overmap_show_land_use_codes );
        }
    };

    const tripoint corner = center - point( om_half_width, om_half_height );

    // For use with place_special: cache the color and symbol of each submap
    // and record the bounds to optimize lookups below
    std::unordered_map<point, std::pair<std::string, nc_color>> special_cache;

    point s_begin;
    point s_end;
    if( blink && uistate.place_special ) {
        for( const auto &s_ter : uistate.place_special->terrains ) {
            if( s_ter.p.z == 0 ) {
                const point rp = om_direction::rotate( s_ter.p.xy(), uistate.omedit_rotation );
                const oter_id oter = s_ter.terrain->get_rotated( uistate.omedit_rotation );

                special_cache.insert( std::make_pair(
                                          rp, std::make_pair( oter->get_symbol(), oter->get_color() ) ) );

                s_begin.x = std::min( s_begin.x, rp.x );
                s_begin.y = std::min( s_begin.y, rp.y );
                s_end.x = std::max( s_end.x, rp.x );
                s_end.y = std::max( s_end.y, rp.y );
            }
        }
    }

    // Cache NPCs since time to draw them is linear (per seen tile) with their count
    struct npc_coloring {
        nc_color color;
        size_t count;
    };
    std::vector<tripoint> path_route;
    std::vector<tripoint> player_path_route;
    std::unordered_map<tripoint, npc_coloring> npc_color;
    if( blink ) {
        const auto &npcs = overmap_buffer.get_npcs_near_player( sight_points );
        for( const auto &np : npcs ) {
            if( np->posz() != center.z ) {
                continue;
            }

            const tripoint pos = np->global_omt_location();
            if( has_debug_vision || overmap_buffer.seen( pos ) ) {
                auto iter = npc_color.find( pos );
                nc_color np_color = np->basic_symbol_color();
                if( iter == npc_color.end() ) {
                    npc_color[ pos ] = { np_color, 1 };
                } else {
                    iter->second.count++;
                    // Randomly change to new NPC's color
                    if( iter->second.color != np_color && one_in( iter->second.count ) ) {
                        iter->second.color = np_color;
                    }
                }
            }
        }
        std::vector<npc *> followers;
        for( auto &elem : g->get_follower_list() ) {
            std::shared_ptr<npc> npc_to_get = overmap_buffer.find_npc( elem );
            if( !npc_to_get ) {
                continue;
            }
            npc *npc_to_add = npc_to_get.get();
            followers.push_back( npc_to_add );
        }
        for( auto &elem : overmap_buffer.get_npcs_near_player( 75 ) ) {
            if( !elem ) {
                continue;
            }
            npc *npc_to_add = elem.get();
            if( npc_to_add->mission == NPC_MISSION_TRAVELLING ) {
                followers.push_back( npc_to_add );
            }
        }
        for( auto &elem : g->u.omt_path ) {
            tripoint tri_to_add = tripoint( elem.xy(), g->u.posz() );
            player_path_route.push_back( tri_to_add );
        }
        for( const auto &np : followers ) {
            if( np->posz() != center.z ) {
                continue;
            }
            if( !np->omt_path.empty() ) {
                for( auto &elem : np->omt_path ) {
                    tripoint tri_to_add = tripoint( elem.xy(), np->posz() );
                    path_route.push_back( tri_to_add );
                }
            }
            const tripoint pos = np->global_omt_location();
            auto iter = npc_color.find( pos );
            nc_color np_color = np->basic_symbol_color();
            if( iter == npc_color.end() ) {
                npc_color[ pos ] = { np_color, 1 };
            } else {
                iter->second.count++;
                // Randomly change to new NPC's color
                if( iter->second.color != np_color && one_in( iter->second.count ) ) {
                    iter->second.color = np_color;
                }
            }
        }
    }

    for( int i = 0; i < om_map_width; ++i ) {
        for( int j = 0; j < om_map_height; ++j ) {
            const tripoint omp = corner + point( i, j );

            oter_id cur_ter = oter_str_id::NULL_ID();
            nc_color ter_color = c_black;
            std::string ter_sym = " ";

            const bool see = has_debug_vision || overmap_buffer.seen( omp );
            if( see ) {
                // Only load terrain if we can actually see it
                cur_ter = overmap_buffer.ter( omp );
            }

            // Check if location is within player line-of-sight
            const bool los = see && g->u.overmap_los( omp, sight_points );
            const bool los_sky = g->u.overmap_los( omp, sight_points * 2 );
            int mycount = std::count( path_route.begin(), path_route.end(), omp );
            bool player_path_count = false;
            std::vector<tripoint>::iterator it;
            it = std::find( player_path_route.begin(), player_path_route.end(), omp );
            if( it != player_path_route.end() ) {
                player_path_count = true;
            }
            if( blink && omp == orig ) {
                // Display player pos, should always be visible
                ter_color = g->u.symbol_color();
                ter_sym = "@";
            } else if( viewing_weather && ( data.debug_weather || los_sky ) ) {
                const weather_type type = get_weather_at_point( omp );
                ter_color = weather::map_color( type );
                ter_sym = weather::glyph( type );
            } else if( data.debug_scent && get_scent_glyph( omp, ter_color, ter_sym ) ) {
            } else if( blink && has_target && omp.xy() == target.xy() ) {
                // Mission target, display always, player should know where it is anyway.
                ter_color = c_red;
                ter_sym = "*";
                if( target.z > center.z ) {
                    ter_sym = "^";
                } else if( target.z < center.z ) {
                    ter_sym = "v";
                }
            } else if( blink && uistate.overmap_show_map_notes && overmap_buffer.has_note( omp ) ) {
                // Display notes in all situations, even when not seen
                std::tie( ter_sym, ter_color, std::ignore ) =
                    get_note_display_info( overmap_buffer.note( omp ) );
            } else if( !see ) {
                // All cases above ignore the seen-status,
                ter_color = c_dark_gray;
                ter_sym   = "#";
                // All cases below assume that see is true.
            } else if( blink && npc_color.count( omp ) != 0 ) {
                // Visible NPCs are cached already
                ter_color = npc_color[ omp ].color;
                ter_sym   = "@";
            } else if( blink && mycount != 0 && g->debug_pathfinding ) {
                ter_color = c_red;
                ter_sym   = "!";
            } else if( blink && player_path_count ) {
                ter_color = c_blue;
                ter_sym = "!";
            } else if( blink && showhordes && los &&
                       overmap_buffer.get_horde_size( omp ) >= HORDE_VISIBILITY_SIZE ) {
                // Display Hordes only when within player line-of-sight
                ter_color = c_green;
                ter_sym   = overmap_buffer.get_horde_size( omp ) > HORDE_VISIBILITY_SIZE * 2 ? "Z" : "z";
            } else if( blink && overmap_buffer.has_vehicle( omp ) ) {
                // Display Vehicles only when player can see the location
                ter_color = c_cyan;
                ter_sym   = "c";
            } else if( !sZoneName.empty() && tripointZone.xy() == omp.xy() ) {
                ter_color = c_yellow;
                ter_sym   = "Z";
            } else if( !uistate.overmap_show_forest_trails && cur_ter &&
                       is_ot_match( "forest_trail", cur_ter, ot_match_type::type ) ) {
                // If forest trails shouldn't be displayed, and this is a forest trail, then
                // instead render it like a forest.
                set_color_and_symbol( forest, omp, ter_sym, ter_color );
            } else {
                // Nothing special, but is visible to the player.
                set_color_and_symbol( cur_ter, omp, ter_sym, ter_color );
            }

            // Are we debugging monster groups?
            if( blink && data.debug_mongroup ) {
                // Check if this tile is the target of the currently selected group

                // Convert to position within overmap
                point omp_in_om( omp.xy() );
                omt_to_om_remain( omp_in_om );
                point group_target = sm_to_omt_copy( mgroup->target.xy() );

                if( mgroup && group_target == omp_in_om ) {
                    ter_color = c_red;
                    ter_sym = "x";
                } else {
                    const auto &groups = overmap_buffer.monsters_at( omp );
                    for( auto &mgp : groups ) {
                        if( mgp->type == mongroup_id( "GROUP_FOREST" ) ) {
                            // Don't flood the map with forest creatures.
                            continue;
                        }
                        if( mgp->horde ) {
                            // Hordes show as +
                            ter_sym = "+";
                            break;
                        } else {
                            // Regular groups show as -
                            ter_sym = "-";
                        }
                    }
                    // Set the color only if we encountered an eligible group.
                    if( ter_sym == "+" || ter_sym == "-" ) {
                        if( los ) {
                            ter_color = c_light_blue;
                        } else {
                            ter_color = c_blue;
                        }
                    }
                }
            }

            // Preview for place_terrain or place_special
            if( uistate.place_terrain || uistate.place_special ) {
                if( blink && uistate.place_terrain && omp.xy() == center.xy() ) {
                    ter_color = uistate.place_terrain->get_color();
                    ter_sym = uistate.place_terrain->get_symbol();
                } else if( blink && uistate.place_special ) {
                    const point from_center = omp.xy() - center.xy();
                    if( from_center.x >= s_begin.x && from_center.x <= s_end.x &&
                        from_center.y >= s_begin.y && from_center.y <= s_end.y ) {
                        const auto sm = special_cache.find( from_center );

                        if( sm != special_cache.end() ) {
                            ter_color = sm->second.second;
                            ter_sym = sm->second.first;
                        }
                    }
                }
                // Highlight areas that already have been generated
                if( MAPBUFFER.lookup_submap(
                        omt_to_sm_copy( omp ) ) ) {
                    ter_color = red_background( ter_color );
                }
            }

            if( omp.xy() == center.xy() && !uistate.place_special ) {
                csee = see;
                ccur_ter = cur_ter;
                mvwputch_hi( w, j, i, ter_color, ter_sym );
            } else {
                mvwputch( w, j, i, ter_color, ter_sym );
            }
        }
    }

    if( center.z == 0 && uistate.overmap_show_city_labels ) {
        draw_city_labels( w, center );
        draw_camp_labels( w, center );
    }

    rectangle screen_bounds( corner.xy(), corner.xy() + point( om_map_width, om_map_height ) );

    if( has_target && blink && !screen_bounds.contains_half_open( target.xy() ) ) {
        point marker = clamp_half_open( target.xy(), screen_bounds ) - corner.xy();
        std::string marker_sym = " ";

        switch( direction_from( center, target ) ) {
            case NORTH:
                marker_sym = "^";
                break;
            case NORTHEAST:
                marker_sym = LINE_OOXX_S;
                break;
            case EAST:
                marker_sym = ">";
                break;
            case SOUTHEAST:
                marker_sym = LINE_XOOX_S;
                break;
            case SOUTH:
                marker_sym = "v";
                break;
            case SOUTHWEST:
                marker_sym = LINE_XXOO_S;
                break;
            case WEST:
                marker_sym = "<";
                break;
            case NORTHWEST:
                marker_sym = LINE_OXXO_S;
                break;
            default:
                break; //Do nothing
        }
        mvwputch( w, marker.y, marker.x, c_red, marker_sym );
    }

    std::vector<std::pair<nc_color, std::string>> corner_text;

    if( uistate.overmap_show_map_notes ) {
        const std::string &note_text = overmap_buffer.note( center );
        if( !note_text.empty() ) {
            const std::tuple<char, nc_color, size_t> note_info = get_note_display_info(
                        note_text );
            const size_t pos = std::get<2>( note_info );
            if( pos != std::string::npos ) {
                corner_text.emplace_back( std::get<1>( note_info ), note_text.substr( pos ) );
            }
        }
    }

    for( const auto &npc : overmap_buffer.get_npcs_near_omt( center, 0 ) ) {
        if( !npc->marked_for_death ) {
            corner_text.emplace_back( npc->basic_symbol_color(), npc->name );
        }
    }

    for( auto &v : overmap_buffer.get_vehicle( center ) ) {
        corner_text.emplace_back( c_white, v.name );
    }

    if( !corner_text.empty() ) {
        int maxlen = 0;
        for( const auto &line : corner_text ) {
            maxlen = std::max( maxlen, utf8_width( line.second ) );
        }

        const std::string spacer( maxlen, ' ' );
        for( size_t i = 0; i < corner_text.size(); i++ ) {
            const auto &pr = corner_text[ i ];
            // clear line, print line, print vertical line at the right side.
            mvwprintz( w, i, 0, c_yellow, spacer );
            nc_color default_color = c_unset;
            print_colored_text( w, i, 0, default_color, pr.first, pr.second );
            mvwputch( w, i, maxlen, c_white, LINE_XOXO );
        }
        for( int i = 0; i <= maxlen; i++ ) {
            mvwputch( w, corner_text.size(), i, c_white, LINE_OXOX );
        }
        mvwputch( w, corner_text.size(), maxlen, c_white, LINE_XOOX );
    }

    if( !sZoneName.empty() && tripointZone.xy() == center.xy() ) {
        std::string sTemp = _( "Zone:" );
        sTemp += " " + sZoneName;

        const int length = utf8_width( sTemp );
        for( int i = 0; i <= length; i++ ) {
            mvwputch( w, om_map_height - 2, i, c_white, LINE_OXOX );
        }

        mvwprintz( w, om_map_height - 1, 0, c_yellow, sTemp );
        mvwputch( w, om_map_height - 2, length, c_white, LINE_OOXX );
        mvwputch( w, om_map_height - 1, length, c_white, LINE_XOXO );
    }

    // Draw the vertical line
    for( int j = 0; j < TERMY; j++ ) {
        mvwputch( wbar, j, 0, c_white, LINE_XOXO );
    }

    // Clear the legend
    for( int i = 1; i < 55; i++ ) {
        for( int j = 0; j < TERMY; j++ ) {
            mvwputch( wbar, j, i, c_black, ' ' );
        }
    }

    // Draw text describing the overmap tile at the cursor position.
    int lines = 1;
    if( csee && !viewing_weather ) {
        if( !mgroups.empty() ) {
            int line_number = 6;
            for( const auto &mgroup : mgroups ) {
                mvwprintz( wbar, line_number++, 3,
                           c_blue, "  Species: %s", mgroup->type.c_str() );
                mvwprintz( wbar, line_number++, 3,
                           c_blue, "# monsters: %d", mgroup->population + mgroup->monsters.size() );
                if( !mgroup->horde ) {
                    continue;
                }
                mvwprintz( wbar, line_number++, 3,
                           c_blue, "  Interest: %d", mgroup->interest );
                mvwprintz( wbar, line_number, 3,
                           c_blue, "  Target: %d, %d", mgroup->target.x, mgroup->target.y );
                mvwprintz( wbar, line_number++, 3,
                           c_red, "x" );
            }
        } else {
            const auto &ter = ccur_ter.obj();
            const auto sm_pos = omt_to_sm_copy( center );

            mvwputch( wbar, 1, 1, ter.get_color(), ter.get_symbol() );

            lines = fold_and_print( wbar, 1, 3, 25, c_light_gray, overmap_buffer.get_description_at( sm_pos ) );
        }
    } else if( viewing_weather ) {
        const bool weather_is_visible = ( data.debug_weather ||
                                          g->u.overmap_los( center, sight_points * 2 ) );
        if( weather_is_visible ) {
            weather_datum weather = weather_data( get_weather_at_point( center ) );
            mvwprintz( wbar, 1, 1, weather.color, weather.name );
        } else {
            mvwprintz( wbar, 1, 1, c_dark_gray, _( "# Unexplored" ) );
        }
    } else {
        mvwprintz( wbar, 1, 1, c_dark_gray, _( "# Unexplored" ) );
    }

    if( has_target ) {
        const int distance = rl_dist( center, target );
        mvwprintz( wbar, ++lines, 1, c_white, _( "Distance to active mission:" ) );
        mvwprintz( wbar, ++lines, 1, c_white, _( "%d tiles" ), distance );

        const int above_below = target.z - orig.z;
        std::string msg;
        if( above_below > 0 ) {
            msg = _( "Above us" );
        } else if( above_below < 0 ) {
            msg = _( "Below us" );
        }
        if( above_below != 0 ) {
            mvwprintz( wbar, ++lines, 1, c_white, _( "%s" ), msg );
        }
    }

    //Show mission targets on this location
    for( auto &mission : g->u.get_active_missions() ) {
        if( mission->get_target() == center ) {
            mvwprintz( wbar, ++lines, 1, c_white, mission->name() );
        }
    }

    mvwprintz( wbar, 12, 1, c_magenta, _( "Use movement keys to pan." ) );
    mvwprintz( wbar, 13, 1, c_magenta, _( "Press W to preview route." ) );
    mvwprintz( wbar, 14, 1, c_magenta, _( "Press again to confirm." ) );
    if( inp_ctxt != nullptr ) {
        int y = 16;

        const auto print_hint = [&]( const std::string & action, nc_color color = c_magenta ) {
            y += fold_and_print( wbar, y, 1, 27, color, string_format( _( "%s - %s" ),
                                 inp_ctxt->get_desc( action ),
                                 inp_ctxt->get_action_name( action ) ) );
        };

        if( data.debug_editor ) {
            print_hint( "PLACE_TERRAIN", c_light_blue );
            print_hint( "PLACE_SPECIAL", c_light_blue );
            ++y;
        }

        const bool show_overlays = uistate.overmap_show_overlays || uistate.overmap_blinking;
        const bool is_explored = overmap_buffer.is_explored( center );

        print_hint( "LEVEL_UP" );
        print_hint( "LEVEL_DOWN" );
        print_hint( "CENTER" );
        print_hint( "SEARCH" );
        print_hint( "CREATE_NOTE" );
        print_hint( "DELETE_NOTE" );
        print_hint( "LIST_NOTES" );
        print_hint( "MISSIONS" );
        print_hint( "TOGGLE_MAP_NOTES", uistate.overmap_show_map_notes ? c_pink : c_magenta );
        print_hint( "TOGGLE_BLINKING", uistate.overmap_blinking ? c_pink : c_magenta );
        print_hint( "TOGGLE_OVERLAYS", show_overlays ? c_pink : c_magenta );
        print_hint( "TOGGLE_LAND_USE_CODES", uistate.overmap_show_land_use_codes ? c_pink : c_magenta );
        print_hint( "TOGGLE_CITY_LABELS", uistate.overmap_show_city_labels ? c_pink : c_magenta );
        print_hint( "TOGGLE_HORDES", uistate.overmap_show_hordes ? c_pink : c_magenta );
        print_hint( "TOGGLE_EXPLORED", is_explored ? c_pink : c_magenta );
        print_hint( "TOGGLE_FAST_SCROLL", fast_scroll ? c_pink : c_magenta );
        print_hint( "TOGGLE_FOREST_TRAILS", uistate.overmap_show_forest_trails ? c_pink : c_magenta );
        print_hint( "HELP_KEYBINDINGS" );
        print_hint( "QUIT" );
    }

    point omt = center.xy();
    const point om = omt_to_om_remain( omt );
    mvwprintz( wbar, getmaxy( wbar ) - 1, 1, c_red,
               _( "LEVEL %i, %d'%d, %d'%d" ), center.z, om.x, omt.x, om.y, omt.y );

    // draw nice crosshair around the cursor
    if( blink && !uistate.place_terrain && !uistate.place_special ) {
        mvwputch( w, om_half_height - 1, om_half_width - 1, c_light_gray, LINE_OXXO );
        mvwputch( w, om_half_height - 1, om_half_width + 1, c_light_gray, LINE_OOXX );
        mvwputch( w, om_half_height + 1, om_half_width - 1, c_light_gray, LINE_XXOO );
        mvwputch( w, om_half_height + 1, om_half_width + 1, c_light_gray, LINE_XOOX );
    }
    // Done with all drawing!
    wrefresh( wbar );
    wmove( w, point( om_half_width, om_half_height ) );
    wrefresh( w );
}

void create_note( const tripoint &curs )
{
    std::string color_notes = _( "Color codes: " );
    for( const std::pair<std::string, std::string> &color_pair : get_note_color_names() ) {
        // The color index is not translatable, but the name is.
        color_notes += string_format( "%1$s:<color_%3$s>%2$s</color>, ", color_pair.first.c_str(),
                                      _( color_pair.second ), string_replace( color_pair.second, " ", "_" ) );
    }

    std::string helper_text = string_format( ".\n\n%s\n%s\n%s\n",
                              _( "Type GLYPH:TEXT to set a custom glyph." ),
                              _( "Type COLOR;TEXT to set a custom color." ),
                              _( "Examples: B:Base | g;Loot | !:R;Minefield" ) );
    color_notes = color_notes.replace( color_notes.end() - 2, color_notes.end(), helper_text );
    std::string title = _( "Note:" );

    const std::string old_note = overmap_buffer.note( curs );
    std::string new_note = old_note;
    auto map_around = get_overmap_neighbors( curs );

    catacurses::window w_preview = catacurses::newwin( npm_height + 2,
                                   max_note_display_length - npm_width - 1,
                                   point( npm_width + 2, 2 ) );
    catacurses::window w_preview_title = catacurses::newwin( 2, max_note_display_length + 1,
                                         point_zero );
    catacurses::window w_preview_map = catacurses::newwin( npm_height + 2, npm_width + 2, point( 0,
                                       2 ) );
    std::tuple<catacurses::window *, catacurses::window *, catacurses::window *> preview_windows =
        std::make_tuple( &w_preview, &w_preview_title, &w_preview_map );

#if defined(__ANDROID__)
    if( get_option<bool>( "ANDROID_AUTO_KEYBOARD" ) ) {
        SDL_StartTextInput();
    }
#endif

    bool esc_pressed = false;
    string_input_popup input_popup;
    input_popup
    .title( title )
    .width( max_note_length )
    .text( new_note )
    .description( color_notes )
    .title_color( c_white )
    .desc_color( c_light_gray )
    .string_color( c_yellow )
    .identifier( "map_note" );

    update_note_preview( old_note, map_around, preview_windows );

    do {
        new_note = input_popup.query_string( false );
        const int first_input = input_popup.context().get_raw_input().get_first_input();
        if( first_input == KEY_ESCAPE ) {
            new_note = old_note;
            esc_pressed = true;
            break;
        } else if( first_input == '\n' ) {
            break;
        } else {
            update_note_preview( new_note, map_around, preview_windows );
        }
    } while( true );

    if( !esc_pressed && new_note.empty() && !old_note.empty() ) {
        if( query_yn( _( "Really delete note?" ) ) ) {
            overmap_buffer.delete_note( curs );
        }
    } else if( !esc_pressed && old_note != new_note ) {
        overmap_buffer.add_note( curs, new_note );
    }
}

// if false, search yielded no results
static bool search( tripoint &curs, const tripoint &orig, const bool show_explored,
                    const bool fast_scroll, std::string &action )
{
    std::string term = string_input_popup()
                       .title( _( "Search term:" ) )
                       .description( _( "Multiple entries separated with , Excludes starting with -" ) )
                       .query_string();
    if( term.empty() ) {
        return false;
    }

    std::vector<point> locations;
    std::vector<point> overmap_checked;

    for( int x = curs.x - OMAPX / 2; x < curs.x + OMAPX / 2; x++ ) {
        for( int y = curs.y - OMAPY / 2; y < curs.y + OMAPY / 2; y++ ) {
            tripoint p( x, y, curs.z );
            overmap_with_local_coords om_loc = overmap_buffer.get_existing_om_global( p );

            if( om_loc ) {
                tripoint om_relative = om_loc.local;
                point om_cache = omt_to_om_copy( p.xy() );

                if( std::find( overmap_checked.begin(), overmap_checked.end(), om_cache ) ==
                    overmap_checked.end() ) {
                    overmap_checked.push_back( om_cache );
                    std::vector<point> notes = om_loc.om->find_notes( curs.z, term );
                    locations.insert( locations.end(), notes.begin(), notes.end() );
                }

                if( om_loc.om->seen( om_relative ) &&
                    match_include_exclude( om_loc.om->ter( om_relative )->get_name(), term ) ) {
                    locations.push_back( om_loc.om->global_base_point() + om_relative.xy() );
                }
            }
        }
    }

    if( locations.empty() ) {
        sfx::play_variant_sound( "menu_error", "default", 100 );
        popup( _( "No results found." ) );
        return false;
    }

    std::sort( locations.begin(), locations.end(), [&]( const point & lhs, const point & rhs ) {
        return trig_dist( curs, tripoint( lhs, curs.z ) ) < trig_dist( curs, tripoint( rhs, curs.z ) );
    } );

    int i = 0;
    //Navigate through results
    tripoint tmp = curs;
    catacurses::window w_search = catacurses::newwin( 13, 27, point( TERMX - 27, 3 ) );

    input_context ctxt( "OVERMAP_SEARCH" );
    ctxt.register_leftright();
    ctxt.register_action( "NEXT_TAB", translate_marker( "Next target" ) );
    ctxt.register_action( "PREV_TAB", translate_marker( "Previous target" ) );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "ANY_INPUT" );

    do {
        tmp.x = locations[i].x;
        tmp.y = locations[i].y;
        draw( g->w_overmap, g->w_omlegend, tmp, orig, uistate.overmap_show_overlays, show_explored,
              fast_scroll, nullptr,
              draw_data_t() );
        //Draw search box
        mvwprintz( w_search, 1, 1, c_light_blue, _( "Search:" ) );
        mvwprintz( w_search, 1, 10, c_light_red, "%*s", 12, term );

        mvwprintz( w_search, 2, 1, c_light_blue, _( "Result(s):" ) );
        mvwprintz( w_search, 2, 16, c_light_red, "%*d/%d", 3, i + 1, locations.size() );

        mvwprintz( w_search, 3, 1, c_light_blue, _( "Direction:" ) );
        mvwprintz( w_search, 3, 14, c_white, "%*d %s",
                   5, static_cast<int>( trig_dist( orig, tripoint( locations[i], orig.z ) ) ),
                   direction_name_short( direction_from( orig, tripoint( locations[i], orig.z ) ) )
                 );

        mvwprintz( w_search, 6, 1, c_white, _( "'<-' '->' Cycle targets." ) );
        mvwprintz( w_search, 10, 1, c_white, _( "Enter/Spacebar to select." ) );
        mvwprintz( w_search, 11, 1, c_white, _( "q or ESC to return." ) );
        draw_border( w_search );
        wrefresh( w_search );
        action = ctxt.handle_input( BLINK_SPEED );
        if( uistate.overmap_blinking ) {
            uistate.overmap_show_overlays = !uistate.overmap_show_overlays;
        }
        if( action == "NEXT_TAB" || action == "RIGHT" ) {
            i = ( i + 1 ) % locations.size();
        } else if( action == "PREV_TAB" || action == "LEFT" ) {
            i = ( i + locations.size() - 1 ) % locations.size();
        } else if( action == "CONFIRM" ) {
            curs = tmp;
        }
    } while( action != "CONFIRM" && action != "QUIT" );
    action.clear();
    return true;
}

static void place_ter_or_special( tripoint &curs, const tripoint &orig, const bool show_explored,
                                  const bool fast_scroll, std::string &action )
{
    uilist pmenu;
    // This simplifies overmap_special selection using uilist
    std::vector<const overmap_special *> oslist;
    const bool terrain = action == "PLACE_TERRAIN";

    if( terrain ) {
        pmenu.title = _( "Select terrain to place:" );
        for( const oter_t &oter : overmap_terrains::get_all() ) {
            pmenu.addentry( oter.id.id(), true, 0, oter.id.str() );
        }
    } else {
        pmenu.title = _( "Select special to place:" );
        for( const overmap_special &elem : overmap_specials::get_all() ) {
            oslist.push_back( &elem );
            pmenu.addentry( oslist.size() - 1, true, 0, elem.id.str() );
        }
    }
    pmenu.query();

    if( pmenu.ret >= 0 ) {
        catacurses::window w_editor = catacurses::newwin( 15, 27, point( TERMX - 27, 3 ) );
        input_context ctxt( "OVERMAP_EDITOR" );
        ctxt.register_directions();
        ctxt.register_action( "CONFIRM" );
        ctxt.register_action( "ROTATE" );
        ctxt.register_action( "QUIT" );
        ctxt.register_action( "ANY_INPUT" );

        if( terrain ) {
            uistate.place_terrain = &oter_id( pmenu.ret ).obj();
        } else {
            uistate.place_special = oslist[pmenu.ret];
        }
        // TODO: Unify these things.
        const bool can_rotate = terrain ? uistate.place_terrain->is_rotatable() :
                                uistate.place_special->rotatable;

        uistate.omedit_rotation = om_direction::type::none;
        // If user chose an already rotated submap, figure out its direction
        if( terrain && can_rotate ) {
            for( om_direction::type r : om_direction::all ) {
                if( uistate.place_terrain->id.id() == uistate.place_terrain->get_rotated( r ) ) {
                    uistate.omedit_rotation = r;
                    break;
                }
            }
        }

        do {
            // overmap::draw will handle actually showing the preview
            draw( g->w_overmap, g->w_omlegend, curs, orig, uistate.overmap_show_overlays, show_explored,
                  fast_scroll, nullptr, draw_data_t() );

            draw_border( w_editor );
            if( terrain ) {
                mvwprintz( w_editor, 1, 1, c_white, _( "Place overmap terrain:" ) );
                mvwprintz( w_editor, 2, 1, c_light_blue, "                         " );
                mvwprintz( w_editor, 2, 1, c_light_blue, uistate.place_terrain->id.c_str() );
            } else {
                mvwprintz( w_editor, 1, 1, c_white, _( "Place overmap special:" ) );
                mvwprintz( w_editor, 2, 1, c_light_blue, "                         " );
                mvwprintz( w_editor, 2, 1, c_light_blue, uistate.place_special->id.c_str() );
            }
            const std::string &rotation = om_direction::name( uistate.omedit_rotation );

            mvwprintz( w_editor, 3, 1, c_light_gray, "                         " );
            mvwprintz( w_editor, 3, 1, c_light_gray, _( "Rotation: %s %s" ), rotation,
                       can_rotate ? "" : _( "(fixed)" ) );
            mvwprintz( w_editor, 5, 1, c_red, _( "Areas highlighted in red" ) );
            mvwprintz( w_editor, 6, 1, c_red, _( "already have map content" ) );
            mvwprintz( w_editor, 7, 1, c_red, _( "generated. Their overmap" ) );
            mvwprintz( w_editor, 8, 1, c_red, _( "id will change, but not" ) );
            mvwprintz( w_editor, 9, 1, c_red, _( "their contents." ) );
            if( ( terrain && uistate.place_terrain->is_rotatable() ) ||
                ( !terrain && uistate.place_special->rotatable ) ) {
                mvwprintz( w_editor, 11, 1, c_white, _( "[%s] Rotate" ),
                           ctxt.get_desc( "ROTATE" ) );
            }
            mvwprintz( w_editor, 12, 1, c_white, _( "[%s] Apply" ),
                       ctxt.get_desc( "CONFIRM" ) );
            mvwprintz( w_editor, 13, 1, c_white, _( "[ESCAPE/Q] Cancel" ) );
            wrefresh( w_editor );

            action = ctxt.handle_input( BLINK_SPEED );

            if( const cata::optional<tripoint> vec = ctxt.get_direction( action ) ) {
                curs.x += vec->x;
                curs.y += vec->y;
            } else if( action == "CONFIRM" ) { // Actually modify the overmap
                if( terrain ) {
                    overmap_buffer.ter( curs ) = uistate.place_terrain->id.id();
                    overmap_buffer.set_seen( curs, true );
                } else {
                    overmap_buffer.place_special( *uistate.place_special, curs, uistate.omedit_rotation, false, true );
                    for( const overmap_special_terrain &s_ter : uistate.place_special->terrains ) {
                        const tripoint pos = curs + om_direction::rotate( s_ter.p, uistate.omedit_rotation );
                        overmap_buffer.set_seen( pos, true );
                    }
                }
                break;
            } else if( action == "ROTATE" && can_rotate ) {
                uistate.omedit_rotation = om_direction::turn_right( uistate.omedit_rotation );
                if( terrain ) {
                    uistate.place_terrain = &uistate.place_terrain->get_rotated( uistate.omedit_rotation ).obj();
                }
            }
            if( uistate.overmap_blinking ) {
                uistate.overmap_show_overlays = !uistate.overmap_show_overlays;
            }
        } while( action != "QUIT" );

        uistate.place_terrain = nullptr;
        uistate.place_special = nullptr;
        action.clear();
    }
}

static tripoint display( const tripoint &orig, const draw_data_t &data = draw_data_t() )
{
    g->w_omlegend = catacurses::newwin( TERMY, 28, point( TERMX - 28, 0 ) );
    g->w_overmap = catacurses::newwin( OVERMAP_WINDOW_HEIGHT, OVERMAP_WINDOW_WIDTH, point_zero );

    // Draw black padding space to avoid gap between map and legend
    // also clears the pixel minimap in TILES
    g->w_blackspace = catacurses::newwin( TERMY, TERMX, point_zero );
    mvwputch( g->w_blackspace, 0, 0, c_black, ' ' );
    wrefresh( g->w_blackspace );

    tripoint ret = overmap::invalid_tripoint;
    tripoint curs( orig );

    if( data.select != tripoint( -1, -1, -1 ) ) {
        curs = tripoint( data.select );
    }

    // Configure input context for navigating the map.
    input_context ictxt( "OVERMAP" );
    ictxt.register_action( "ANY_INPUT" );
    ictxt.register_directions();
    ictxt.register_action( "CONFIRM" );
    ictxt.register_action( "LEVEL_UP" );
    ictxt.register_action( "LEVEL_DOWN" );
    ictxt.register_action( "HELP_KEYBINDINGS" );
    ictxt.register_action( "MOUSE_MOVE" );
    ictxt.register_action( "SELECT" );
    ictxt.register_action( "CHOOSE_DESTINATION" );

    // Actions whose keys we want to display.
    ictxt.register_action( "CENTER" );
    ictxt.register_action( "CREATE_NOTE" );
    ictxt.register_action( "DELETE_NOTE" );
    ictxt.register_action( "SEARCH" );
    ictxt.register_action( "LIST_NOTES" );
    ictxt.register_action( "TOGGLE_MAP_NOTES" );
    ictxt.register_action( "TOGGLE_BLINKING" );
    ictxt.register_action( "TOGGLE_OVERLAYS" );
    ictxt.register_action( "TOGGLE_HORDES" );
    ictxt.register_action( "TOGGLE_LAND_USE_CODES" );
    ictxt.register_action( "TOGGLE_CITY_LABELS" );
    ictxt.register_action( "TOGGLE_EXPLORED" );
    ictxt.register_action( "TOGGLE_FAST_SCROLL" );
    ictxt.register_action( "TOGGLE_FOREST_TRAILS" );
    ictxt.register_action( "MISSIONS" );

    if( data.debug_editor ) {
        ictxt.register_action( "PLACE_TERRAIN" );
        ictxt.register_action( "PLACE_SPECIAL" );
    }
    ictxt.register_action( "QUIT" );
    std::string action;
    bool show_explored = true;
    bool fast_scroll = false; /* fast scroll state should reset every time overmap UI is opened */
    int fast_scroll_offset = get_option<int>( "FAST_SCROLL_OFFSET" );
    cata::optional<tripoint> mouse_pos;
    bool redraw = true;
    std::chrono::time_point<std::chrono::steady_clock> last_blink = std::chrono::steady_clock::now();
    do {
        if( redraw ) {
            draw( g->w_overmap, g->w_omlegend, curs, orig, uistate.overmap_show_overlays,
                  show_explored, fast_scroll, &ictxt, data );
        }
        redraw = true;
#if (defined TILES || defined _WIN32 || defined WINDOWS )
        int scroll_timeout = get_option<int>( "EDGE_SCROLL" );
        // If EDGE_SCROLL is disabled, it will have a value of -1.
        // blinking won't work if handle_input() is passed a negative integer.
        if( scroll_timeout < 0 ) {
            scroll_timeout = BLINK_SPEED;
        }
        action = ictxt.handle_input( scroll_timeout );
#else
        action = ictxt.handle_input( BLINK_SPEED );
#endif
        if( const cata::optional<tripoint> vec = ictxt.get_direction( action ) ) {
            int scroll_d = fast_scroll ? fast_scroll_offset : 1;
            curs.x += vec->x * scroll_d;
            curs.y += vec->y * scroll_d;
        } else if( action == "MOUSE_MOVE" || action == "TIMEOUT" ) {
            tripoint edge_scroll = g->mouse_edge_scrolling_terrain( ictxt );
            if( edge_scroll == tripoint_zero ) {
                redraw = false;
            } else {
                if( action == "MOUSE_MOVE" ) {
                    edge_scroll *= 2;
                }
                curs += edge_scroll;
            }
        } else if( action == "SELECT" && ( mouse_pos = ictxt.get_coordinates( g->w_overmap ) ) ) {
            curs.x += mouse_pos->x;
            curs.y += mouse_pos->y;
        } else if( action == "CENTER" ) {
            curs = orig;
        } else if( action == "LEVEL_DOWN" && curs.z > -OVERMAP_DEPTH ) {
            curs.z -= 1;
        } else if( action == "LEVEL_UP" && curs.z < OVERMAP_HEIGHT ) {
            curs.z += 1;
        } else if( action == "CONFIRM" ) {
            ret = curs;
        } else if( action == "QUIT" ) {
            ret = overmap::invalid_tripoint;
        } else if( action == "CREATE_NOTE" ) {
            create_note( curs );
        } else if( action == "DELETE_NOTE" ) {
            if( overmap_buffer.has_note( curs ) && query_yn( _( "Really delete note?" ) ) ) {
                overmap_buffer.delete_note( curs );
            }
        } else if( action == "LIST_NOTES" ) {
            const point p = draw_notes( curs );
            if( p != point_min ) {
                curs.x = p.x;
                curs.y = p.y;
            }
        } else if( action == "CHOOSE_DESTINATION" ) {
            const tripoint player_omt_pos = g->u.global_omt_location();
            if( !g->u.omt_path.empty() && g->u.omt_path.front() == curs ) {
                if( query_yn( _( "Travel to this point?" ) ) ) {
                    // renew the path incase of a leftover dangling path point
                    g->u.omt_path = overmap_buffer.get_npc_path( player_omt_pos, curs, g->u.in_vehicle &&
                                    g->u.controlling_vehicle );
                    if( g->u.in_vehicle && g->u.controlling_vehicle ) {
                        vehicle *player_veh = veh_pointer_or_null( g->m.veh_at( g->u.pos() ) );
                        player_veh->omt_path = g->u.omt_path;
                        player_veh->is_autodriving = true;
                        g->u.assign_activity( activity_id( "ACT_AUTODRIVE" ) );
                    } else {
                        g->u.reset_move_mode();
                        g->u.assign_activity( activity_id( "ACT_TRAVELLING" ) );
                    }
                    action = "QUIT";
                }
            }
            if( curs == player_omt_pos ) {
                g->u.omt_path.clear();
            } else {
                g->u.omt_path = overmap_buffer.get_npc_path( player_omt_pos, curs, g->u.in_vehicle &&
                                g->u.controlling_vehicle );
            }
        } else if( action == "TOGGLE_BLINKING" ) {
            uistate.overmap_blinking = !uistate.overmap_blinking;
            // if we turn off overmap blinking, show overlays and explored status
            if( !uistate.overmap_blinking ) {
                uistate.overmap_show_overlays = true;
            } else {
                show_explored = true;
            }
        } else if( action == "TOGGLE_OVERLAYS" ) {
            // if we are currently blinking, turn blinking off.
            if( uistate.overmap_blinking ) {
                uistate.overmap_blinking = false;
                uistate.overmap_show_overlays = false;
                show_explored = false;
            } else {
                uistate.overmap_show_overlays = !uistate.overmap_show_overlays;
                show_explored = !show_explored;
            }
        } else if( action == "TOGGLE_LAND_USE_CODES" ) {
            uistate.overmap_show_land_use_codes = !uistate.overmap_show_land_use_codes;
        } else if( action == "TOGGLE_MAP_NOTES" ) {
            uistate.overmap_show_map_notes = !uistate.overmap_show_map_notes;
        } else if( action == "TOGGLE_HORDES" ) {
            uistate.overmap_show_hordes = !uistate.overmap_show_hordes;
        } else if( action == "TOGGLE_CITY_LABELS" ) {
            uistate.overmap_show_city_labels = !uistate.overmap_show_city_labels;
        } else if( action == "TOGGLE_EXPLORED" ) {
            overmap_buffer.toggle_explored( curs );
        } else if( action == "TOGGLE_FAST_SCROLL" ) {
            fast_scroll = !fast_scroll;
        } else if( action == "TOGGLE_FOREST_TRAILS" ) {
            uistate.overmap_show_forest_trails = !uistate.overmap_show_forest_trails;
        } else if( action == "SEARCH" ) {
            if( !search( curs, orig, show_explored, fast_scroll, action ) ) {
                continue;
            }
        } else if( action == "PLACE_TERRAIN" || action == "PLACE_SPECIAL" ) {
            place_ter_or_special( curs, orig, show_explored, fast_scroll, action );
        } else if( action == "MISSIONS" ) {
            g->list_missions();
        }

        std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
        if( now > last_blink + std::chrono::milliseconds( BLINK_SPEED ) ) {
            if( uistate.overmap_blinking ) {
                uistate.overmap_show_overlays = !uistate.overmap_show_overlays;
                redraw = true;
            }
            last_blink = now;
        }
    } while( action != "QUIT" && action != "CONFIRM" );
    werase( g->w_overmap );
    werase( g->w_omlegend );
    catacurses::erase();
    g->init_ui( true );
    return ret;
}

} // namespace overmap_ui

void ui::omap::display()
{
    overmap_ui::display( g->u.global_omt_location(), overmap_ui::draw_data_t() );
}

void ui::omap::display_hordes()
{
    overmap_ui::draw_data_t data;
    data.debug_mongroup = true;
    overmap_ui::display( g->u.global_omt_location(), data );
}

void ui::omap::display_weather()
{
    overmap_ui::draw_data_t data;
    data.debug_weather = true;
    tripoint pos = g->u.global_omt_location();
    pos.z = 10;
    overmap_ui::display( pos, data );
}

void ui::omap::display_visible_weather()
{
    overmap_ui::draw_data_t data;
    data.visible_weather = true;
    tripoint pos = g->u.global_omt_location();
    pos.z = 10;
    overmap_ui::display( pos, data );
}

void ui::omap::display_scents()
{
    overmap_ui::draw_data_t data;
    data.debug_scent = true;
    overmap_ui::display( g->u.global_omt_location(), data );
}

void ui::omap::display_editor()
{
    overmap_ui::draw_data_t data;
    data.debug_editor = true;
    overmap_ui::display( g->u.global_omt_location(), data );
}

void ui::omap::display_zones( const tripoint &center, const tripoint &select, const int iZoneIndex )
{
    overmap_ui::draw_data_t data;
    data.select = select;
    data.iZoneIndex = iZoneIndex;
    overmap_ui::display( center, data );
}

tripoint ui::omap::choose_point()
{
    return overmap_ui::display( g->u.global_omt_location() );
}

tripoint ui::omap::choose_point( const tripoint &origin )
{
    return overmap_ui::display( origin );
}

tripoint ui::omap::choose_point( int z )
{
    tripoint loc = g->u.global_omt_location();
    loc.z = z;
    return overmap_ui::display( loc );
}
