#include "overmap_ui.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "activity_actor_definitions.h"
#include "all_enum_values.h"
#include "avatar.h"
#include "basecamp.h"
#include "calendar.h"
#include "enum_conversions.h"
#ifdef TILES
#include "cata_tiles.h"
#endif // TILES
#include "cata_scope_helpers.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "character.h"
#include "city.h"
#include "clzones.h"
#include "color.h"
#include "coordinates.h"
#include "cuboid_rectangle.h"
#include "cursesdef.h"
#include "display.h"
#include "game.h"
#include "game_constants.h"
#include "game_ui.h"
#include "input_context.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "mapbuffer.h"
#include "mission.h"
#include "mongroup.h"
#include "npc.h"
#include "omdata.h"
#include "options.h"
#include "output.h"
#include "overmap.h"
#include "overmap_types.h"
#include "overmapbuffer.h"
#include "point.h"
#include "regional_settings.h"
#include "rng.h"
#include "sounds.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "type_id.h"
#include "ui.h"
#include "ui_manager.h"
#include "uistate.h"
#include "units.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "weather_gen.h"
#include "weather_type.h"

class character_id;

static const activity_id ACT_TRAVELLING( "ACT_TRAVELLING" );

static const mongroup_id GROUP_FOREST( "GROUP_FOREST" );
static const mongroup_id GROUP_NEMESIS( "GROUP_NEMESIS" );

static const oter_str_id oter_forest( "forest" );
static const oter_str_id oter_unexplored( "unexplored" );

static const oter_type_str_id oter_type_forest_trail( "forest_trail" );

static const trait_id trait_DEBUG_CLAIRVOYANCE( "DEBUG_CLAIRVOYANCE" );
static const trait_id trait_DEBUG_NIGHTVISION( "DEBUG_NIGHTVISION" );

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
static void create_note( const tripoint_abs_omt &curs );

// {note symbol, note color, offset to text}
std::tuple<char, nc_color, size_t> get_note_display_info( const std::string_view note )
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
        const size_t end = note.find_first_of( " :;", pos, 3 );
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
    const tripoint_abs_omt &current )
{
    const bool has_debug_vision = get_player_character().has_trait( trait_DEBUG_NIGHTVISION );

    std::array<std::pair<nc_color, std::string>, npm_width *npm_height> map_around;
    int index = 0;
    const point shift( npm_width / 2, npm_height / 2 );
    for( const tripoint_abs_omt &dest :
         tripoint_range<tripoint_abs_omt>( current - shift, current + shift ) ) {
        nc_color ter_color = c_black;
        std::string ter_sym = " ";
        const bool see = has_debug_vision || overmap_buffer.seen( dest );
        if( see ) {
            // Only load terrain if we can actually see it
            oter_id cur_ter = overmap_buffer.ter( dest );
            ter_color = cur_ter->get_color();
            ter_sym = cur_ter->get_symbol();
        } else {
            ter_color = oter_unexplored.obj().get_color();
            ter_sym = oter_unexplored.obj().get_symbol();
        }
        map_around[index++] = std::make_pair( ter_color, ter_sym );
    }
    return map_around;
}

static void update_note_preview( const std::string_view note,
                                 const std::array<std::pair<nc_color, std::string>, npm_width *npm_height> &map_around,
                                 const std::tuple<catacurses::window *, catacurses::window *, catacurses::window *>
                                 &preview_windows )
{
    auto om_symbol = get_note_display_info( note );
    const nc_color note_color = std::get<1>( om_symbol );
    const char symbol = std::get<0>( om_symbol );
    const std::string_view note_text = note.substr( std::get<2>( om_symbol ), std::string::npos );

    catacurses::window *w_preview = std::get<0>( preview_windows );
    catacurses::window *w_preview_title = std::get<1>( preview_windows );
    catacurses::window *w_preview_map   = std::get<2>( preview_windows );

    draw_border( *w_preview );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( *w_preview, point( 1, 1 ), c_white, _( "Note preview" ) );
    wnoutrefresh( *w_preview );

    werase( *w_preview_title );
    nc_color default_color = c_unset;
    print_colored_text( *w_preview_title, point_zero, default_color, note_color, note_text,
                        report_color_error::no );
    int note_text_width = utf8_width( note_text );
    mvwputch( *w_preview_title, point( note_text_width, 0 ), c_white, LINE_XOXO );
    for( int i = 0; i < note_text_width; i++ ) {
        mvwputch( *w_preview_title, point( i, 1 ), c_white, LINE_OXOX );
    }
    mvwputch( *w_preview_title, point( note_text_width, 1 ), c_white, LINE_XOOX );
    wnoutrefresh( *w_preview_title );

    const point npm_offset( point_south_east );
    werase( *w_preview_map );
    draw_border( *w_preview_map, c_yellow );
    for( int i = 0; i < npm_height; i++ ) {
        for( int j = 0; j < npm_width; j++ ) {
            const auto &ter = map_around[i * npm_width + j];
            mvwputch( *w_preview_map, npm_offset + point( j, i ), ter.first, ter.second );
        }
    }
    mvwputch( *w_preview_map, npm_offset + point( npm_width / 2, npm_height / 2 ),
              note_color, symbol );
    wnoutrefresh( *w_preview_map );
}

weather_type_id get_weather_at_point( const tripoint_abs_omt &pos )
{
    // Weather calculation is a bit expensive, so it's cached here.
    static std::map<tripoint_abs_omt, weather_type_id> weather_cache;
    static time_point last_weather_display = calendar::before_time_starts;
    if( last_weather_display != calendar::turn ) {
        last_weather_display = calendar::turn;
        weather_cache.clear();
    }
    auto iter = weather_cache.find( pos );
    if( iter == weather_cache.end() ) {
        const tripoint_abs_ms abs_ms_pos = project_to<coords::ms>( pos );
        const weather_generator &wgen = overmap_buffer.get_settings( pos ).weather;
        const weather_type_id weather = wgen.get_weather_conditions( abs_ms_pos, calendar::turn,
                                        g->get_seed() );
        iter = weather_cache.insert( std::make_pair( pos, weather ) ).first;
    }
    return iter->second;
}

static bool get_scent_glyph( const tripoint_abs_omt &pos, nc_color &ter_color,
                             std::string &ter_sym )
{
    scent_trace possible_scent = overmap_buffer.scent_at( pos );
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

static void draw_city_labels( const catacurses::window &w, const tripoint_abs_omt &center )
{
    const int win_x_max = getmaxx( w );
    const int win_y_max = getmaxy( w );
    const int sm_radius = std::max( win_x_max, win_y_max );

    const point screen_center_pos( win_x_max / 2, win_y_max / 2 );

    for( const city_reference &element : overmap_buffer.get_cities_near(
             project_to<coords::sm>( center ), sm_radius ) ) {
        const point_abs_omt city_pos =
            project_to<coords::omt>( element.abs_sm_pos.xy() );
        const point_rel_omt screen_pos( city_pos - center.xy() + screen_center_pos );

        const int text_width = utf8_width( element.city->name, true );
        const int text_x_min = screen_pos.x() - text_width / 2;
        const int text_x_max = text_x_min + text_width;
        const int text_y = screen_pos.y();

        if( text_x_min < 0 ||
            text_x_max > win_x_max ||
            text_y < 0 ||
            text_y > win_y_max ) {
            continue;   // outside of the window bounds.
        }

        if( screen_center_pos.x >= ( text_x_min - 1 ) &&
            screen_center_pos.x <= text_x_max &&
            screen_center_pos.y >= ( text_y - 1 ) &&
            screen_center_pos.y <= ( text_y + 1 ) ) {
            continue;   // right under the cursor.
        }

        if( !overmap_buffer.seen( tripoint_abs_omt( city_pos, center.z() ) ) ) {
            continue;   // haven't seen it.
        }

        mvwprintz( w, point( text_x_min, text_y ), i_yellow, element.city->name );
    }
}

static void draw_camp_labels( const catacurses::window &w, const tripoint_abs_omt &center )
{
    const int win_x_max = getmaxx( w );
    const int win_y_max = getmaxy( w );
    const int sm_radius = std::max( win_x_max, win_y_max );

    const point screen_center_pos( win_x_max / 2, win_y_max / 2 );

    for( const camp_reference &element : overmap_buffer.get_camps_near(
             project_to<coords::sm>( center ), sm_radius ) ) {
        const point_abs_omt camp_pos( element.camp->camp_omt_pos().xy() );
        const point screen_pos( ( camp_pos - center.xy() ).raw() + screen_center_pos );
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
            screen_center_pos.x <= text_x_max &&
            screen_center_pos.y >= ( text_y - 1 ) &&
            screen_center_pos.y <= ( text_y + 1 ) ) {
            continue;   // right under the cursor.
        }

        if( !overmap_buffer.seen( tripoint_abs_omt( camp_pos, center.z() ) ) ) {
            continue;   // haven't seen it.
        }

        mvwprintz( w, point( text_x_min, text_y ), i_white, camp_name );
    }
}

class map_notes_callback : public uilist_callback
{
    private:
        overmapbuffer::t_notes_vector _notes;
        int _z;
        int _selected = 0;

        catacurses::window w_preview;
        catacurses::window w_preview_title;
        catacurses::window w_preview_map;
        std::tuple<catacurses::window *, catacurses::window *, catacurses::window *> preview_windows;
        ui_adaptor ui;

        point_abs_omt point_selected() {
            return _notes[_selected].first;
        }
        tripoint_abs_omt note_location() {
            return tripoint_abs_omt( point_selected(), _z );
        }
    public:
        map_notes_callback( const overmapbuffer::t_notes_vector &notes, int z )
            : _notes( notes ), _z( z ) {
            ui.on_screen_resize( [this]( ui_adaptor & ui ) {
                w_preview = catacurses::newwin( npm_height + 2, max_note_display_length - npm_width - 1,
                                                point( npm_width + 2, 2 ) );
                w_preview_title = catacurses::newwin( 2, max_note_display_length + 1, point_zero );
                w_preview_map = catacurses::newwin( npm_height + 2, npm_width + 2, point( 0, 2 ) );
                preview_windows = std::make_tuple( &w_preview, &w_preview_title, &w_preview_map );

                ui.position( point_zero, point( max_note_display_length + 1, npm_height + 4 ) );
            } );
            ui.mark_resize();

            ui.on_redraw( [this]( const ui_adaptor & ) {
                if( _selected >= 0 && static_cast<size_t>( _selected ) < _notes.size() ) {
                    const tripoint_abs_omt note_pos = note_location();
                    const auto map_around = get_overmap_neighbors( note_pos );
                    update_note_preview( overmap_buffer.note( note_pos ), map_around, preview_windows );
                } else {
                    update_note_preview( {}, {}, preview_windows );
                }
            } );
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
                if( action == "MARK_DANGER" ) {
                    if( overmap_buffer.is_marked_dangerous( note_location() ) &&
                        query_yn( _( "Remove dangerous mark?" ) ) ) {
                        overmap_buffer.mark_note_dangerous( note_location(), 0, false );
                    }
                    // NOLINTNEXTLINE(cata-text-style): No need for two whitespaces
                    else if( ( overmap_buffer.is_marked_dangerous( note_location() ) &&
                               query_yn( _( "Edit dangerous mark?" ) ) ) ||
                             ( !overmap_buffer.is_marked_dangerous( note_location() ) &&
                               // NOLINTNEXTLINE(cata-text-style): No need for two whitespaces
                               query_yn( _( "Mark area as dangerous ( to avoid on auto move paths?  This will create a note if none exists already )" ) ) ) ) {
                        const int max_amount = 20;
                        // NOLINTNEXTLINE(cata-text-style): No need for two whitespaces
                        const std::string popupmsg = _( "Danger radius in overmap squares? ( 0-20 )" );
                        int amount = string_input_popup()
                                     .title( popupmsg )
                                     .width( 20 )
                                     .text( "0" )
                                     .only_digits( true )
                                     .query_int();
                        if( amount > -1 && amount <= max_amount ) {
                            overmap_buffer.mark_note_dangerous( note_location(), amount, true );
                            menu->ret = UILIST_MAP_NOTE_EDITED;
                            return true;
                        }
                    }
                }
            }
            return false;
        }
        void select( uilist *menu ) override {
            _selected = menu->selected;
            ui.invalidate_ui();
        }
};

static point_abs_omt draw_notes( const tripoint_abs_omt &origin )
{
    point_abs_omt result( point_min );

    bool refresh = true;
    uilist nmenu;
    while( refresh ) {
        refresh = false;
        nmenu.color_error( false );
        nmenu.init();
        nmenu.desc_enabled = true;
        nmenu.input_category = "OVERMAP_NOTES";
        nmenu.additional_actions.emplace_back( "DELETE_NOTE", translation() );
        nmenu.additional_actions.emplace_back( "EDIT_NOTE", translation() );
        nmenu.additional_actions.emplace_back( "MARK_DANGER", translation() );
        const input_context ctxt( nmenu.input_category, keyboard_mode::keycode );
        nmenu.text = string_format(
                         _( "<%s> - center on note, <%s> - edit note, <%s> - mark as dangerous, <%s> - delete note, <%s> - close window" ),
                         colorize( ctxt.get_desc( "CONFIRM", 1 ), c_yellow ),
                         colorize( ctxt.get_desc( "EDIT_NOTE", 1 ), c_yellow ),
                         colorize( ctxt.get_desc( "MARK_DANGER", 1 ), c_red ),
                         colorize( ctxt.get_desc( "DELETE_NOTE", 1 ), c_yellow ),
                         colorize( ctxt.get_desc( "QUIT", 1 ), c_yellow )
                     );
        int row = 0;
        overmapbuffer::t_notes_vector notes = overmap_buffer.get_all_notes( origin.z() );
        nmenu.title = string_format( _( "Map notes (%d)" ), notes.size() );
        for( const auto &point_with_note : notes ) {
            const point_abs_omt p = point_with_note.first;
            if( p == origin.xy() ) {
                nmenu.selected = row;
            }
            const std::string &note = point_with_note.second;
            auto om_symbol = get_note_display_info( note );
            const nc_color note_color = std::get<1>( om_symbol );
            const std::string note_symbol = std::string( 1, std::get<0>( om_symbol ) );
            const std::string note_text = note.substr( std::get<2>( om_symbol ), std::string::npos );
            point_abs_omt p_omt( p );
            const point_abs_omt p_player = get_player_character().global_omt_location().xy();
            const int distance_player = rl_dist( p_player, p_omt );
            const point_abs_sm sm_pos = project_to<coords::sm>( p_omt );
            const point_abs_om p_om = project_to<coords::om>( p_omt );
            const std::string location_desc =
                overmap_buffer.get_description_at( tripoint_abs_sm( sm_pos, origin.z() ) );
            const bool is_dangerous =
                overmap_buffer.is_marked_dangerous( tripoint_abs_omt( p, origin.z() ) );
            nmenu.addentry_desc(
                string_format( _( "[%s] %s" ), colorize( note_symbol, note_color ), note_text ),
                string_format(
                    _( "<color_red>LEVEL %i, %d'%d, %d'%d</color>: %s "
                       "(Distance: <color_white>%d</color>) <color_red>%s</color>" ),
                    origin.z(), p_om.x(), p_omt.x(), p_om.y(), p_omt.y(), location_desc,
                    distance_player, is_dangerous ? "DANGEROUS AREA!" : "" ) );
            nmenu.entries[row].ctxt =
                string_format( _( "<color_light_gray>Distance: </color><color_white>%d</color>" ),
                               distance_player );
            row++;
        }
        map_notes_callback cb( notes, origin.z() );
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

static bool get_and_assign_los( int &los, avatar &player_character, const tripoint_abs_omt &omp,
                                const int &sight_points )
{
    if( los == -1 ) {
        los = player_character.overmap_los( omp, sight_points );
    }

    return los;
}

static void draw_ascii(
    const catacurses::window &w, const tripoint_abs_omt &center,
    const tripoint_abs_omt &orig, bool blink, bool show_explored, bool /* fast_scroll */,
    input_context * /* inp_ctxt */, const draw_data_t &data )
{

    const int om_map_width = OVERMAP_WINDOW_WIDTH;
    const int om_map_height = OVERMAP_WINDOW_HEIGHT;
    const int om_half_width = om_map_width / 2;
    const int om_half_height = om_map_height / 2;
    const bool viewing_weather =
        ( uistate.overmap_debug_weather || uistate.overmap_visible_weather ) && center.z() == 10;

    avatar &player_character = get_avatar();
    // Target of current mission
    const tripoint_abs_omt target = player_character.get_active_mission_target();
    const bool has_target = target != overmap::invalid_tripoint;
    oter_id ccur_ter = oter_str_id::NULL_ID();
    // Debug vision allows seeing everything
    const bool has_debug_vision = player_character.has_trait( trait_DEBUG_NIGHTVISION );
    // sight_points is hoisted for speed reasons.
    const int sight_points = !has_debug_vision ?
                             player_character.overmap_sight_range( g->light_level( player_character.posz() ) ) :
                             100;
    // Whether showing hordes is currently enabled
    const bool showhordes = uistate.overmap_show_hordes;
    const bool show_map_revealed = uistate.overmap_show_revealed_omts;

    const oter_id forest = oter_forest.id();

    std::string sZoneName;
    tripoint_abs_omt tripointZone( -1, -1, -1 );
    const zone_manager &zones = zone_manager::get_manager();

    if( data.iZoneIndex != -1 ) {
        const zone_data &zone = zones.get_zones()[data.iZoneIndex].get();
        sZoneName = zone.get_name();
        // TODO: fix point types
        tripointZone = project_to<coords::omt>(
                           tripoint_abs_ms( zone.get_center_point() ) );
    }

    // If we're debugging monster groups, find the monster group we've selected
    const mongroup *mgroup = nullptr;
    std::vector<mongroup *> mgroups;
    if( uistate.overmap_debug_mongroup ) {
        mgroups = overmap_buffer.monsters_at( center );
        for( mongroup * const &mgp : mgroups ) {
            mgroup = mgp;
            if( mgp->horde ) {
                break;
            }
        }
    }

    // A small LRU cache: most oter_id's occur in clumps like forests of swamps.
    // This cache helps avoid much more costly lookups in the full hashmap.
    constexpr size_t cache_size = 8; // used below to calculate the next index
    std::array<std::pair<oter_id, oter_t const *>, cache_size> cache;
    size_t cache_next = 0;

    const auto set_color_and_symbol = [&]( const oter_id & cur_ter, const tripoint_abs_omt & omp,
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

    const tripoint_abs_omt corner = center - point( om_half_width, om_half_height );

    // For use with place_special: cache the color and symbol of each submap
    // and record the bounds to optimize lookups below
    std::unordered_map<point_rel_omt, std::pair<std::string, nc_color>> special_cache;

    point_rel_omt s_begin;
    point_rel_omt s_end;
    if( blink && uistate.place_special ) {
        for( const overmap_special_terrain &s_ter : uistate.place_special->preview_terrains() ) {
            // Preview should only yield the terrains on the zero z-level
            cata_assert( s_ter.p.z == 0 );

            // TODO: fix point types
            const point_rel_omt rp( om_direction::rotate( s_ter.p.xy(), uistate.omedit_rotation ) );
            const oter_id oter = s_ter.terrain->get_rotated( uistate.omedit_rotation );

            special_cache.insert( std::make_pair(
                                      rp, std::make_pair( oter->get_symbol(), oter->get_color() ) ) );

            s_begin.x() = std::min( s_begin.x(), rp.x() );
            s_begin.y() = std::min( s_begin.y(), rp.y() );
            s_end.x() = std::max( s_end.x(), rp.x() );
            s_end.y() = std::max( s_end.y(), rp.y() );
        }
    }

    // Cache NPCs since time to draw them is linear (per seen tile) with their count
    struct npc_coloring {
        nc_color color;
        size_t count = 0;
    };
    std::unordered_set<tripoint_abs_omt> npc_path_route;
    std::unordered_set<tripoint_abs_omt> &revealed_highlights = get_avatar().map_revealed_omts;
    std::unordered_map<point_abs_omt, int> player_path_route;
    std::unordered_map<tripoint_abs_omt, npc_coloring> npc_color;
    auto npcs_near_player = overmap_buffer.get_npcs_near_player( sight_points );
    if( blink ) {
        // get seen NPCs
        for( const auto &np : npcs_near_player ) {
            if( np->posz() != center.z() ) {
                continue;
            }

            const tripoint_abs_omt pos = np->global_omt_location();
            if( has_debug_vision || overmap_buffer.seen( pos ) ) {
                auto iter = npc_color.find( pos );
                nc_color np_color = np->basic_symbol_color();
                if( iter == npc_color.end() ) {
                    npc_color[pos] = { np_color, 1 };
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
        // get friendly followers
        for( const character_id &elem : g->get_follower_list() ) {
            shared_ptr_fast<npc> npc_to_get = overmap_buffer.find_npc( elem );
            if( !npc_to_get ) {
                continue;
            }
            npc *npc_to_add = npc_to_get.get();
            followers.push_back( npc_to_add );
        }
        // get all traveling NPCs for the debug menu to show pathfinding routes.
        if( g->debug_pathfinding ) {
            for( auto &elem : overmap_buffer.get_npcs_near_player( 200 ) ) {
                if( !elem ) {
                    continue;
                }
                npc *npc_to_add = elem.get();
                if( npc_to_add->mission == NPC_MISSION_TRAVELLING && !npc_to_add->omt_path.empty() ) {
                    for( auto &elem : npc_to_add->omt_path ) {
                        npc_path_route.insert( elem );
                    }
                }
            }
        }
        for( auto &elem : player_character.omt_path ) {
            player_path_route[ elem.xy() ] = elem.z();
        }
        for( npc * const &np : followers ) {
            if( np->posz() != center.z() ) {
                continue;
            }
            const tripoint_abs_omt pos = np->global_omt_location();
            auto iter = npc_color.find( pos );
            nc_color np_color = np->basic_symbol_color();
            if( iter == npc_color.end() ) {
                npc_color[pos] = { np_color, 1 };
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
            const tripoint_abs_omt omp = corner + point( i, j );
            oter_id cur_ter = oter_str_id::NULL_ID();
            nc_color ter_color = c_black;
            std::string ter_sym = " ";

            const bool see = has_debug_vision || overmap_buffer.seen( omp );
            if( see ) {
                // Only load terrain if we can actually see it
                cur_ter = overmap_buffer.ter( omp );
            }

            // Check if location is within player line-of-sight
            // These ints are treated as unassigned booleans. Use get_and_assign_los() to reference
            // This allows for easy re-use of these variables without the unnecessary lookups if they aren't used
            int los = -1;
            int los_sky = -1;
            if( blink && omp == orig ) {
                // Display player pos, should always be visible
                ter_color = player_character.symbol_color();
                ter_sym = "@";
            } else if( viewing_weather && ( uistate.overmap_debug_weather ||
                                            get_and_assign_los( los_sky, player_character, omp, sight_points * 2 ) ) ) {
                const weather_type_id type = get_weather_at_point( omp );
                ter_color = type->map_color;
                ter_sym = type->get_symbol();
            } else if( data.debug_scent && get_scent_glyph( omp, ter_color, ter_sym ) ) {
                // get_scent_glyph has changed ter_color and ter_sym if omp has a scent
            } else if( blink && has_target && omp.xy() == target.xy() ) {
                // Mission target, display always, player should know where it is anyway.
                ter_color = c_red;
                ter_sym = "*";
                if( target.z() > center.z() ) {
                    ter_sym = "^";
                } else if( target.z() < center.z() ) {
                    ter_sym = "v";
                }
            } else if( blink && uistate.overmap_show_map_notes && overmap_buffer.has_note( omp ) ) {
                // Display notes in all situations, even when not seen
                std::tie( ter_sym, ter_color, std::ignore ) =
                    get_note_display_info( overmap_buffer.note( omp ) );
            } else if( !see ) {
                // All cases above ignore the seen-status,
                ter_color = oter_unexplored.obj().get_color();
                ter_sym = oter_unexplored.obj().get_symbol();
                // All cases below assume that see is true.
            } else if( blink && npc_color.count( omp ) != 0 ) {
                // Visible NPCs are cached already
                ter_color = npc_color[omp].color;
                ter_sym = "@";
            } else if( blink && player_path_route.find( omp.xy() ) != player_path_route.end() ) {
                // player path
                ter_color = c_blue;
                const int player_path_z = player_path_route[omp.xy()];
                if( player_path_z == omp.z() ) {
                    ter_sym = "!";
                } else if( player_path_z > omp.z() ) {
                    ter_sym = "^";
                } else {
                    ter_sym = "v";
                }
            } else if( blink && npc_path_route.find( omp ) != npc_path_route.end() ) {
                // npc path
                ter_color = c_red;
                ter_sym = "!";
            } else if( blink && show_map_revealed &&
                       revealed_highlights.find( omp ) != revealed_highlights.end() ) {
                // Revealed map tiles
                ter_color = c_magenta;
                ter_sym = "&";
            } else if( blink && showhordes &&
                       overmap_buffer.get_horde_size( omp ) >= HORDE_VISIBILITY_SIZE &&
                       ( get_and_assign_los( los, player_character, omp, sight_points ) ||
                         uistate.overmap_debug_mongroup || player_character.has_trait( trait_DEBUG_CLAIRVOYANCE ) ) ) {
                // Display Hordes only when within player line-of-sight
                ter_color = c_green;
                ter_sym = overmap_buffer.get_horde_size( omp ) > HORDE_VISIBILITY_SIZE * 2 ? "Z" : "z";
            } else if( blink && overmap_buffer.has_vehicle( omp ) ) {
                ter_color = c_cyan;
                ter_sym = overmap_buffer.get_vehicle_ter_sym( omp );
            } else if( !sZoneName.empty() && tripointZone.xy() == omp.xy() ) {
                ter_color = c_yellow;
                ter_sym = "Z";
            } else if( !uistate.overmap_show_forest_trails && cur_ter &&
                       ( cur_ter->get_type_id() == oter_type_forest_trail ) ) {
                // If forest trails shouldn't be displayed, and this is a forest trail, then
                // instead render it like a forest.
                set_color_and_symbol( forest, omp, ter_sym, ter_color );
            } else {
                // Nothing special, but is visible to the player.
                set_color_and_symbol( cur_ter, omp, ter_sym, ter_color );
            }

            // Are we debugging monster groups?
            if( blink && uistate.overmap_debug_mongroup ) {
                // Check if this tile is the target of the currently selected group

                if( mgroup && project_to<coords::omt>( mgroup->target ) == omp.xy() ) {
                    ter_color = c_red;
                    ter_sym = "x";
                } else {
                    const auto &groups = overmap_buffer.monsters_at( omp );
                    for( const mongroup *mgp : groups ) {
                        if( mgp->type == GROUP_FOREST ) {
                            // Don't flood the map with forest creatures.
                            continue;
                        }
                        if( mgp->horde ) {
                            // Hordes show as +
                            ter_sym = "+";

                            if( mgp->type == GROUP_NEMESIS ) {
                                // nemesis horde shows as &
                                ter_sym = "&";
                                ter_color = c_red;
                            }

                            break;

                        } else {
                            // Regular groups show as -
                            ter_sym = "-";
                        }
                    }
                    // Set the color only if we encountered an eligible group.
                    if( ter_sym == "+" || ter_sym == "-" ) {
                        if( get_and_assign_los( los, player_character, omp, sight_points ) ) {
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
                    const point_rel_omt from_center = omp.xy() - center.xy();
                    if( from_center.x() >= s_begin.x() && from_center.x() <= s_end.x() &&
                        from_center.y() >= s_begin.y() && from_center.y() <= s_end.y() ) {
                        const auto sm = special_cache.find( from_center );

                        if( sm != special_cache.end() ) {
                            ter_color = sm->second.second;
                            ter_sym = sm->second.first;
                        }
                    }
                }
                // Highlight areas that already have been generated
                if( MAPBUFFER.lookup_submap( project_to<coords::sm>( omp ) ) ) {
                    ter_color = red_background( ter_color );
                }
            }

            if( omp.xy() == center.xy() && !uistate.place_special ) {
                ccur_ter = cur_ter;
                mvwputch_hi( w, point( i, j ), ter_color, ter_sym );
            } else {
                mvwputch( w, point( i, j ), ter_color, ter_sym );
            }
        }
    }

    if( center.z() == 0 && uistate.overmap_show_city_labels ) {
        draw_city_labels( w, center );
        draw_camp_labels( w, center );
    }

    half_open_rectangle<point_abs_omt> screen_bounds(
        corner.xy(), corner.xy() + point( om_map_width, om_map_height ) );

    if( has_target && blink && !screen_bounds.contains( target.xy() ) ) {
        point_rel_omt marker = clamp( target.xy(), screen_bounds ) - corner.xy();
        std::string marker_sym = " ";

        switch( direction_from( center.xy(), target.xy() ) ) {
            case direction::NORTH:
                marker_sym = "^";
                break;
            case direction::NORTHEAST:
                marker_sym = LINE_OOXX_S;
                break;
            case direction::EAST:
                marker_sym = ">";
                break;
            case direction::SOUTHEAST:
                marker_sym = LINE_XOOX_S;
                break;
            case direction::SOUTH:
                marker_sym = "v";
                break;
            case direction::SOUTHWEST:
                marker_sym = LINE_XXOO_S;
                break;
            case direction::WEST:
                marker_sym = "<";
                break;
            case direction::NORTHWEST:
                marker_sym = LINE_OXXO_S;
                break;
            default:
                break; //Do nothing
        }
        mvwputch( w, marker.raw(), c_red, marker_sym );
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
            if( overmap_buffer.is_marked_dangerous( center ) ) {
                corner_text.emplace_back( c_red, _( "DANGEROUS AREA!" ) );
            }
        }
    }

    if( has_debug_vision || overmap_buffer.seen( center ) ) {
        for( const auto &npc : npcs_near_player ) {
            if( !npc->marked_for_death && npc->global_omt_location() == center ) {
                corner_text.emplace_back( npc->basic_symbol_color(), npc->get_name() );
            }
        }
    }

    for( om_vehicle &v : overmap_buffer.get_vehicle( center ) ) {
        corner_text.emplace_back( c_white, v.name );
    }

    if( !corner_text.empty() ) {
        int maxlen = 0;
        for( const auto &line : corner_text ) {
            maxlen = std::max( maxlen, utf8_width( line.second, true ) );
        }

        mvwputch( w, point_south_east, c_white, LINE_OXXO );
        for( int i = 0; i <= maxlen; i++ ) {
            mvwputch( w, point( i + 2, 1 ), c_white, LINE_OXOX );
        }
        mvwputch( w, point( 1, corner_text.size() + 2 ), c_white, LINE_XXOO );
        const std::string spacer( maxlen, ' ' );
        for( size_t i = 0; i < corner_text.size(); i++ ) {
            const auto &pr = corner_text[i];
            // clear line, print line, print vertical line on each side.
            mvwputch( w, point( 1, i + 2 ), c_white, LINE_XOXO );
            mvwprintz( w, point( 2, i + 2 ), c_yellow, spacer );
            nc_color default_color = c_unset;
            print_colored_text( w, point( 2, i + 2 ), default_color, pr.first, pr.second,
                                report_color_error::no );
            mvwputch( w, point( maxlen + 2, i + 2 ), c_white, LINE_XOXO );
        }
        mvwputch( w, point( maxlen + 2, 1 ), c_white, LINE_OOXX );
        for( int i = 0; i <= maxlen; i++ ) {
            mvwputch( w, point( i + 2, corner_text.size() + 2 ), c_white, LINE_OXOX );
        }
        mvwputch( w, point( maxlen + 2, corner_text.size() + 2 ), c_white, LINE_XOOX );
    }

    if( !sZoneName.empty() && tripointZone.xy() == center.xy() ) {
        std::string sTemp = _( "Zone:" );
        sTemp += " " + sZoneName;

        const int length = utf8_width( sTemp );
        for( int i = 0; i <= length; i++ ) {
            mvwputch( w, point( i, om_map_height - 2 ), c_white, LINE_OXOX );
        }

        mvwprintz( w, point( 0, om_map_height - 1 ), c_yellow, sTemp );
        mvwputch( w, point( length, om_map_height - 2 ), c_white, LINE_OOXX );
        mvwputch( w, point( length, om_map_height - 1 ), c_white, LINE_XOXO );
    }

    // draw nice crosshair around the cursor
    if( blink && !uistate.place_terrain && !uistate.place_special ) {
        mvwputch( w, point( om_half_width - 1, om_half_height - 1 ), c_light_gray, LINE_OXXO );
        mvwputch( w, point( om_half_width + 1, om_half_height - 1 ), c_light_gray, LINE_OOXX );
        mvwputch( w, point( om_half_width - 1, om_half_height + 1 ), c_light_gray, LINE_XXOO );
        mvwputch( w, point( om_half_width + 1, om_half_height + 1 ), c_light_gray, LINE_XOOX );
    }
    // Done with all drawing!
    wnoutrefresh( w );
}

static void draw_om_sidebar(
    ui_adaptor &ui, const catacurses::window &wbar, const tripoint_abs_omt &center,
    const tripoint_abs_omt &orig, bool /* blink */, bool fast_scroll,
    input_context *inp_ctxt, const draw_data_t &data )
{
    avatar &player_character = get_avatar();
    // Debug vision allows seeing everything
    const bool has_debug_vision = player_character.has_trait( trait_DEBUG_NIGHTVISION );
    // sight_points is hoisted for speed reasons.
    const int sight_points = !has_debug_vision ?
                             player_character.overmap_sight_range( g->light_level( player_character.posz() ) ) :
                             100;
    const bool center_seen = has_debug_vision || overmap_buffer.seen( center );
    const tripoint_abs_omt target = player_character.get_active_mission_target();
    const bool has_target = target != overmap::invalid_tripoint;
    const bool viewing_weather = uistate.overmap_debug_weather || uistate.overmap_visible_weather;

    // If we're debugging monster groups, find the monster group we've selected
    std::vector<mongroup *> mgroups;
    if( uistate.overmap_debug_mongroup ) {
        mgroups = overmap_buffer.monsters_at( center );
        for( mongroup * const &mgp : mgroups ) {
            if( mgp->horde ) {
                break;
            }
        }
    }

    // Draw the vertical line
    for( int j = 0; j < TERMY; j++ ) {
        mvwputch( wbar, point( 0, j ), c_white, LINE_XOXO );
    }

    // Clear the legend
    for( int i = 1; i < getmaxx( wbar ); i++ ) {
        for( int j = 0; j < TERMY; j++ ) {
            mvwputch( wbar, point( i, j ), c_black, ' ' );
        }
    }

    // Draw text describing the overmap tile at the cursor position.
    int lines = 1;
    if( center_seen ) {
        if( !mgroups.empty() ) {
            const point desc_pos( 3, 6 );
            ui.set_cursor( wbar, desc_pos );
            int line_number = 0;
            for( mongroup * const &mgroup : mgroups ) {
                mvwprintz( wbar, desc_pos + point( 0, line_number++ ),
                           c_blue, "  Species: %s", mgroup->type.c_str() );
                mvwprintz( wbar, desc_pos + point( 0, line_number++ ),
                           c_blue, "# monsters: %d", mgroup->population + mgroup->monsters.size() );
                if( !mgroup->horde ) {
                    continue;
                }
                mvwprintz( wbar, desc_pos + point( 0, line_number++ ),
                           c_blue, "  Interest: %d", mgroup->interest );
                mvwprintz( wbar, desc_pos + point( 0, line_number++ ),
                           c_blue, "  Target: %s", mgroup->target.to_string() );
                mvwprintz( wbar, desc_pos + point( 0, line_number++ ),
                           c_red, "x" );
            }
        } else {
            const oter_t &ter = overmap_buffer.ter( center ).obj();
            const auto sm_pos = project_to<coords::sm>( center );

            // NOLINTNEXTLINE(cata-use-named-point-constants)
            mvwputch( wbar, point( 1, 1 ), ter.get_color(), ter.get_symbol() );

            const point desc_pos( 3, 1 );
            ui.set_cursor( wbar, desc_pos );
            lines = fold_and_print( wbar, desc_pos, getmaxx( wbar ) - desc_pos.x,
                                    c_light_gray,
                                    overmap_buffer.get_description_at( sm_pos ) );
        }
    } else {
        const oter_t &ter = oter_unexplored.obj();

        // NOLINTNEXTLINE(cata-use-named-point-constants)
        mvwputch( wbar, point( 1, 1 ), ter.get_color(), ter.get_symbol() );

        const point desc_pos( 3, 1 );
        ui.set_cursor( wbar, desc_pos );
        lines = fold_and_print( wbar, desc_pos, getmaxx( wbar ) - desc_pos.x,
                                ter.get_color(), ter.get_name() );
    }

    // Describe the weather conditions on the following line, if weather is visible
    if( viewing_weather ) {
        const bool weather_is_visible = uistate.overmap_debug_weather ||
                                        player_character.overmap_los( center, sight_points * 2 );
        if( weather_is_visible ) {
            // NOLINTNEXTLINE(cata-use-named-point-constants)
            mvwprintz( wbar, point( 3, ++lines ), get_weather_at_point( center )->color,
                       get_weather_at_point( center )->name.translated() );
        } else {
            // NOLINTNEXTLINE(cata-use-named-point-constants)
            mvwprintz( wbar, point( 1, ++lines ), c_dark_gray, _( "# Weather unknown" ) );
        }
    }

    if( ( data.debug_editor && center_seen ) || data.debug_info ) {
        mvwprintz( wbar, point( 1, ++lines ), c_white,
                   "abs_omt: %s", center.to_string() );
        const oter_t &oter = overmap_buffer.ter( center ).obj();
        mvwprintz( wbar, point( 1, ++lines ), c_white, "oter: %s (rot %d)", oter.id.str(),
                   oter.get_rotation() );
        mvwprintz( wbar, point( 1, ++lines ), c_white,
                   "oter_type: %s", oter.get_type_id().str() );
        std::vector<oter_id> predecessors = overmap_buffer.predecessors( center );
        if( !predecessors.empty() ) {
            mvwprintz( wbar, point( 1, ++lines ), c_white, "predecessors:" );
            for( auto pred = predecessors.rbegin(); pred != predecessors.rend(); ++pred ) {
                mvwprintz( wbar, point( 1, ++lines ), c_white, "- %s", pred->id().str() );
            }
        }
        std::optional<mapgen_arguments> *args = overmap_buffer.mapgen_args( center );
        if( args ) {
            if( *args ) {
                for( const std::pair<const std::string, cata_variant> &arg : ( **args ).map ) {
                    mvwprintz( wbar, point( 1, ++lines ), c_white, "%s = %s",
                               arg.first, arg.second.get_string() );
                }
            } else {
                mvwprintz( wbar, point( 1, ++lines ), c_white, "args not yet set" );
            }
        }

        for( cube_direction dir : all_enum_values<cube_direction>() ) {
            if( std::string *join = overmap_buffer.join_used_at( { center, dir } ) ) {
                mvwprintz( wbar, point( 1, ++lines ), c_white, "join %s: %s",
                           io::enum_to_string( dir ), *join );
            }
        }

        for( const mongroup *mg : overmap_buffer.monsters_at( center ) ) {
            mvwprintz( wbar, point( 1, ++lines ), c_red, "mongroup %s (%zu/%u), %s %s%s",
                       mg->type.str(), mg->monsters.size(), mg->population,
                       io::enum_to_string( mg->behaviour ),
                       mg->dying ? "x" : "", mg->horde ? "h" : "" );
            mvwprintz( wbar, point( 1, ++lines ), c_red, "target: %s (%d)",
                       project_to<coords::omt>( mg->target ).to_string(), mg->interest );
        }
    }

    if( has_target ) {
        const int distance = rl_dist( center, target );
        mvwprintz( wbar, point( 1, ++lines ), c_white, _( "Distance to active mission:" ) );
        mvwprintz( wbar, point( 1, ++lines ), c_white, _( "%d tiles" ), distance );

        const int above_below = target.z() - orig.z();
        std::string msg;
        if( above_below > 0 ) {
            msg = _( "Above us" );
        } else if( above_below < 0 ) {
            msg = _( "Below us" );
        }
        if( above_below != 0 ) {
            mvwprintz( wbar, point( 1, ++lines ), c_white, _( "%s" ), msg );
        }
    }

    //Show mission targets on this location
    for( mission *&mission : player_character.get_active_missions() ) {
        if( mission->get_target() == center ) {
            mvwprintz( wbar, point( 1, ++lines ), c_white, mission->name() );
        }
    }

    mvwprintz( wbar, point( 1, 12 ), c_magenta, _( "Use movement keys to pan." ) );
    mvwprintz( wbar, point( 1, 13 ), c_magenta, _( "Press W to preview route." ) );
    mvwprintz( wbar, point( 1, 14 ), c_magenta, _( "Press again to confirm." ) );
    if( inp_ctxt != nullptr ) {
        int y = 16;

        const auto print_hint = [&]( const std::string & action, nc_color color = c_magenta ) {
            y += fold_and_print( wbar, point( 1, y ), getmaxx( wbar ) - 1, color, string_format( _( "%s - %s" ),
                                 inp_ctxt->get_desc( action ),
                                 inp_ctxt->get_action_name( action ) ) );
        };

        if( data.debug_editor ) {
            print_hint( "PLACE_TERRAIN", c_light_blue );
            print_hint( "PLACE_SPECIAL", c_light_blue );
            print_hint( "SET_SPECIAL_ARGS", c_light_blue );
            print_hint( "LONG_TELEPORT", c_light_blue );
            ++y;
        }

        const bool show_overlays = uistate.overmap_show_overlays || uistate.overmap_blinking;
        const bool is_explored = overmap_buffer.is_explored( center );

        print_hint( "LEVEL_UP" );
        print_hint( "LEVEL_DOWN" );
        print_hint( "CENTER" );
        print_hint( "CENTER_ON_DESTINATION" );
        print_hint( "GO_TO_DESTINATION" );
        print_hint( "SEARCH" );
        print_hint( "CREATE_NOTE" );
        print_hint( "DELETE_NOTE" );
        print_hint( "MARK_DANGER" );
        print_hint( "LIST_NOTES" );
        print_hint( "MISSIONS" );
        print_hint( "TOGGLE_MAP_NOTES", uistate.overmap_show_map_notes ? c_pink : c_magenta );
        print_hint( "TOGGLE_BLINKING", uistate.overmap_blinking ? c_pink : c_magenta );
        print_hint( "TOGGLE_OVERLAYS", show_overlays ? c_pink : c_magenta );
        print_hint( "TOGGLE_LAND_USE_CODES", uistate.overmap_show_land_use_codes ? c_pink : c_magenta );
        print_hint( "TOGGLE_CITY_LABELS", uistate.overmap_show_city_labels ? c_pink : c_magenta );
        print_hint( "TOGGLE_HORDES", uistate.overmap_show_hordes ? c_pink : c_magenta );
        print_hint( "TOGGLE_MAP_REVEALS", uistate.overmap_show_revealed_omts ? c_pink : c_magenta );
        print_hint( "TOGGLE_EXPLORED", is_explored ? c_pink : c_magenta );
        print_hint( "TOGGLE_FAST_SCROLL", fast_scroll ? c_pink : c_magenta );
        print_hint( "TOGGLE_FOREST_TRAILS", uistate.overmap_show_forest_trails ? c_pink : c_magenta );
        print_hint( "TOGGLE_OVERMAP_WEATHER",
                    !get_map().is_outside( get_player_character().pos() ) ? c_dark_gray :
                    uistate.overmap_visible_weather ? c_pink : c_magenta );
        print_hint( "HELP_KEYBINDINGS" );
        print_hint( "QUIT" );
    }

    const std::string coords = display::overmap_position_text( center );
    mvwprintz( wbar, point( 1, getmaxy( wbar ) - 1 ), c_red, coords );
    wnoutrefresh( wbar );
}

#if defined(TILES)
tiles_redraw_info redraw_info;
#endif

static void draw(
    ui_adaptor &ui, const tripoint_abs_omt &center, const tripoint_abs_omt &orig,
    bool blink, bool show_explored, bool fast_scroll,
    input_context *inp_ctxt, const draw_data_t &data )
{
    draw_om_sidebar( ui, g->w_omlegend, center, orig, blink, fast_scroll, inp_ctxt, data );
#if defined( TILES )
    if( use_tiles && use_tiles_overmap ) {
        redraw_info = tiles_redraw_info { center, blink };
        werase( g->w_overmap );
        // trigger the actual redraw code in sdltiles.cpp
        wnoutrefresh( g->w_overmap );
        return;
    }
#endif // TILES
    draw_ascii( g->w_overmap, center, orig, blink, show_explored, fast_scroll, inp_ctxt, data );
}

static void create_note( const tripoint_abs_omt &curs )
{
    std::string color_notes = string_format( "%s\n\n\n",
                              _( "Add a note to the map.  "
                                 "For a custom GLYPH or COLOR follow the examples below.  "
                                 "Default GLYPH and COLOR looks like this: "
                                 "<color_yellow>N</color>" ) );

    color_notes += _( "Color codes: " );
    for( const std::pair<const std::string, note_color> &color_pair : get_note_color_names() ) {
        // The color index is not translatable, but the name is.
        //~ %1$s: note color abbreviation, %2$s: note color name
        color_notes += string_format( pgettext( "note color", "%1$s:%2$s, " ), color_pair.first,
                                      colorize( color_pair.second.name, color_pair.second.color ) );
    }

    std::string helper_text = string_format( ".\n\n%s\n%s\n%s\n\n",
                              _( "Type GLYPH<color_yellow>:</color>TEXT to set a custom glyph." ),
                              _( "Type COLOR<color_yellow>;</color>TEXT to set a custom color." ),
                              // NOLINTNEXTLINE(cata-text-style): literal exclamation mark
                              _( "Examples: B:Base | g;Loot | !:R;Minefield" ) );
    color_notes = color_notes.replace( color_notes.end() - 2, color_notes.end(),
                                       helper_text );
    std::string title = _( "Note:" );

    const std::string old_note = overmap_buffer.note( curs );
    std::string new_note = old_note;
    auto map_around = get_overmap_neighbors( curs );

    catacurses::window w_preview;
    catacurses::window w_preview_title;
    catacurses::window w_preview_map;
    std::tuple<catacurses::window *, catacurses::window *, catacurses::window *> preview_windows;

    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        w_preview = catacurses::newwin( npm_height + 2,
                                        max_note_display_length - npm_width - 1,
                                        point( npm_width + 2, 2 ) );
        w_preview_title = catacurses::newwin( 2, max_note_display_length + 1,
                                              point_zero );
        w_preview_map = catacurses::newwin( npm_height + 2, npm_width + 2,
                                            point( 0, 2 ) );
        preview_windows = std::make_tuple( &w_preview, &w_preview_title, &w_preview_map );

        ui.position( point_zero, point( max_note_display_length + 1, npm_height + 4 ) );
    } );
    ui.mark_resize();

    ui.on_redraw( [&]( const ui_adaptor & ) {
        update_note_preview( new_note, map_around, preview_windows );
    } );

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

    do {
        new_note = input_popup.query_string( false );
        if( input_popup.canceled() ) {
            new_note = old_note;
            esc_pressed = true;
            break;
        } else if( input_popup.confirmed() ) {
            break;
        }
        ui.invalidate_ui();
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
static bool search( const ui_adaptor &om_ui, tripoint_abs_omt &curs, const tripoint_abs_omt &orig )
{
    std::string term = string_input_popup()
                       .title( _( "Search term:" ) )
                       .description( string_format( "%s\n%s",
                                     _( "Multiple entries separated with comma (,). Excludes starting with hyphen (-)." ),
                                     colorize( _( "UP: history, CTRL-U: clear line, ESC: abort, ENTER: save" ), c_green ) ) )
                       .desc_color( c_white )
                       .identifier( "overmap_search" )
                       .query_string();
    if( term.empty() ) {
        return false;
    }

    std::vector<point_abs_omt> locations;
    std::vector<point_abs_om> overmap_checked;

    const int radius = OMAPX; // arbitrary
    for( const tripoint_abs_omt &p : points_in_radius( curs, radius ) ) {
        overmap_with_local_coords om_loc = overmap_buffer.get_existing_om_global( p );

        if( om_loc ) {
            tripoint_om_omt om_relative = om_loc.local;
            point_abs_om om_cache = project_to<coords::om>( p.xy() );

            if( std::find( overmap_checked.begin(), overmap_checked.end(),
                           om_cache ) == overmap_checked.end() ) {
                overmap_checked.push_back( om_cache );
                std::vector<point_abs_omt> notes = om_loc.om->find_notes( curs.z(), term );
                locations.insert( locations.end(), notes.begin(), notes.end() );
            }

            if( om_loc.om->seen( om_relative ) &&
                match_include_exclude( om_loc.om->ter( om_relative )->get_name(), term ) ) {
                locations.push_back( project_combine( om_loc.om->pos(), om_relative.xy() ) );
            }
        }
    }

    if( locations.empty() ) {
        sfx::play_variant_sound( "menu_error", "default", 100 );
        popup( _( "No results found." ) );
        return false;
    }

    std::sort( locations.begin(), locations.end(),
    [&]( const point_abs_omt & lhs, const point_abs_omt & rhs ) {
        return trig_dist( curs, tripoint_abs_omt( lhs, curs.z() ) ) <
               trig_dist( curs, tripoint_abs_omt( rhs, curs.z() ) );
    } );

    int i = 0;
    //Navigate through results
    const tripoint_abs_omt prev_curs = curs;

    catacurses::window w_search;

    ui_adaptor ui;
    int search_width = OVERMAP_LEGEND_WIDTH - 1;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        w_search = catacurses::newwin( 13, search_width, point( TERMX - search_width, 3 ) );

        ui.position_from_window( w_search );
    } );
    ui.mark_resize();

    input_context ctxt( "OVERMAP_SEARCH" );
    ctxt.register_action( "NEXT_TAB", to_translation( "Next result" ) );
    ctxt.register_action( "PREV_TAB", to_translation( "Previous result" ) );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "ANY_INPUT" );

    ui.on_redraw( [&]( const ui_adaptor & ) {
        //Draw search box

        int a = utf8_width( _( "Search:" ) );
        int b = utf8_width( _( "Result:" ) );
        int c = utf8_width( _( "Results:" ) );
        int d = utf8_width( _( "Direction:" ) );
        int align_width = 0;
        std::array<int, 4> align_w_value = { a, b, c, d};
        for( int n : align_w_value ) {
            if( n > align_width ) {
                align_width = n + 2;
            }
        }

        // NOLINTNEXTLINE(cata-use-named-point-constants)
        mvwprintz( w_search, point( 1, 1 ), c_light_blue, _( "Search:" ) );
        mvwprintz( w_search, point( align_width, 1 ), c_light_red, "%s", term );

        mvwprintz( w_search, point( 1, 2 ), c_light_blue,
                   locations.size() == 1 ? _( "Result:" ) : _( "Results:" ) );
        mvwprintz( w_search, point( align_width, 2 ), c_light_red, "%d/%d     ", i + 1,
                   locations.size() );

        mvwprintz( w_search, point( 1, 3 ), c_light_blue, _( "Direction:" ) );
        mvwprintz( w_search, point( align_width, 3 ), c_light_red, "%d %s",
                   static_cast<int>( trig_dist( orig, tripoint_abs_omt( locations[i], orig.z() ) ) ),
                   direction_name_short( direction_from( orig, tripoint_abs_omt( locations[i], orig.z() ) ) ) );

        if( locations.size() > 1 ) {
            fold_and_print( w_search, point( 1, 6 ), search_width, c_white,
                            _( "Press [<color_yellow>%s</color>] or [<color_yellow>%s</color>] "
                               "to cycle through search results." ),
                            ctxt.get_desc( "NEXT_TAB" ), ctxt.get_desc( "PREV_TAB" ) );
        }
        fold_and_print( w_search, point( 1, 10 ), search_width, c_white,
                        _( "Press [<color_yellow>%s</color>] to confirm." ), ctxt.get_desc( "CONFIRM" ) );
        fold_and_print( w_search, point( 1, 11 ), search_width, c_white,
                        _( "Press [<color_yellow>%s</color>] to quit." ), ctxt.get_desc( "QUIT" ) );
        draw_border( w_search );
        wnoutrefresh( w_search );
    } );

    std::string action;
    do {
        curs.x() = locations[i].x();
        curs.y() = locations[i].y();
        om_ui.invalidate_ui();
        ui_manager::redraw();
        action = ctxt.handle_input( get_option<int>( "BLINK_SPEED" ) );
        if( uistate.overmap_blinking ) {
            uistate.overmap_show_overlays = !uistate.overmap_show_overlays;
        }
        if( action == "NEXT_TAB" ) {
            i = ( i + 1 ) % locations.size();
        } else if( action == "PREV_TAB" ) {
            i = ( i + locations.size() - 1 ) % locations.size();
        } else if( action == "QUIT" ) {
            curs = prev_curs;
            om_ui.invalidate_ui();
        }
    } while( action != "CONFIRM" && action != "QUIT" );
    return true;
}

static void place_ter_or_special( const ui_adaptor &om_ui, tripoint_abs_omt &curs,
                                  const std::string &om_action )
{
    uilist pmenu;
    // This simplifies overmap_special selection using uilist
    std::vector<const overmap_special *> oslist;
    const bool terrain = om_action == "PLACE_TERRAIN";

    if( terrain ) {
        pmenu.title = _( "Select terrain to place:" );
        for( const oter_t &oter : overmap_terrains::get_all() ) {
            const std::string entry_text = string_format(
                                               _( "sym: [ %s %s ], color: [ %s %s], name: [ %s ], id: [ %s ]" ),
                                               colorize( oter.get_symbol(), oter.get_color() ),
                                               colorize( oter.get_symbol( true ), oter.get_color( true ) ),
                                               colorize( string_from_color( oter.get_color() ), oter.get_color() ),
                                               colorize( string_from_color( oter.get_color( true ) ), oter.get_color( true ) ),
                                               colorize( oter.get_name(), oter.get_color() ),
                                               colorize( oter.id.str(), c_white ) );
            pmenu.addentry( oter.id.id().to_i(), true, 0, entry_text );
        }
    } else {
        pmenu.title = _( "Select special to place:" );
        for( const overmap_special &elem : overmap_specials::get_all() ) {
            oslist.push_back( &elem );
            const std::string entry_text = elem.id.str();
            pmenu.addentry( oslist.size() - 1, true, 0, entry_text );
        }
    }
    pmenu.query();

    if( pmenu.ret >= 0 ) {
        catacurses::window w_editor;

        ui_adaptor ui;
        ui.on_screen_resize( [&]( ui_adaptor & ui ) {
            w_editor = catacurses::newwin( 15, 27, point( TERMX - 27, 3 ) );

            ui.position_from_window( w_editor );
        } );
        ui.mark_resize();

        input_context ctxt( "OVERMAP_EDITOR" );
        ctxt.register_directions();
        ctxt.register_action( "CONFIRM" );
        ctxt.register_action( "ROTATE" );
        ctxt.register_action( "QUIT" );
        ctxt.register_action( "HELP_KEYBINDINGS" );
        ctxt.register_action( "ANY_INPUT" );

        if( terrain ) {
            uistate.place_terrain = &oter_id( pmenu.ret ).obj();
        } else {
            uistate.place_special = oslist[pmenu.ret];
        }
        // TODO: Unify these things.
        const bool can_rotate = terrain ? uistate.place_terrain->is_rotatable() :
                                uistate.place_special->is_rotatable();

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

        ui.on_redraw( [&]( const ui_adaptor & ) {
            draw_border( w_editor );
            if( terrain ) {
                // NOLINTNEXTLINE(cata-use-named-point-constants)
                mvwprintz( w_editor, point( 1, 1 ), c_white, _( "Place overmap terrain:" ) );
                mvwprintz( w_editor, point( 1, 2 ), c_light_blue, "                         " );
                mvwprintz( w_editor, point( 1, 2 ), c_light_blue, uistate.place_terrain->id.c_str() );
            } else {
                mvwprintz( w_editor, point_south_east, c_white, _( "Place overmap special:" ) );
                mvwprintz( w_editor, point( 1, 2 ), c_light_blue, "                         " );
                mvwprintz( w_editor, point( 1, 2 ), c_light_blue, uistate.place_special->id.c_str() );
            }
            const std::string &rotation = om_direction::name( uistate.omedit_rotation );

            mvwprintz( w_editor, point( 1, 3 ), c_light_gray, "                         " );
            mvwprintz( w_editor, point( 1, 3 ), c_light_gray, _( "Rotation: %s %s" ), rotation,
                       can_rotate ? "" : _( "(fixed)" ) );
            mvwprintz( w_editor, point( 1, 5 ), c_red, _( "Highlighted regions" ) );
            mvwprintz( w_editor, point( 1, 6 ), c_red, _( "already have map content" ) );
            // NOLINTNEXTLINE(cata-text-style): single space after period for compactness
            mvwprintz( w_editor, point( 1, 7 ), c_red, _( "generated. Their overmap" ) );
            mvwprintz( w_editor, point( 1, 8 ), c_red, _( "id will change, but not" ) );
            mvwprintz( w_editor, point( 1, 9 ), c_red, _( "their contents." ) );
            if( ( terrain && uistate.place_terrain->is_rotatable() ) ||
                ( !terrain && uistate.place_special->is_rotatable() ) ) {
                mvwprintz( w_editor, point( 1, 11 ), c_white, _( "[%s] Rotate" ),
                           ctxt.get_desc( "ROTATE" ) );
            }
            mvwprintz( w_editor, point( 1, 12 ), c_white, _( "[%s] Apply" ),
                       ctxt.get_desc( "CONFIRM" ) );
            mvwprintz( w_editor, point( 1, 13 ), c_white, _( "[ESCAPE/Q] Cancel" ) );
            wnoutrefresh( w_editor );
        } );

        std::string action;
        do {
            om_ui.invalidate_ui();
            ui_manager::redraw();

            action = ctxt.handle_input( get_option<int>( "BLINK_SPEED" ) );

            if( const std::optional<tripoint> vec = ctxt.get_direction( action ) ) {
                curs += vec->xy();
            } else if( action == "CONFIRM" ) { // Actually modify the overmap
                if( terrain ) {
                    overmap_buffer.ter_set( curs, uistate.place_terrain->id.id() );
                    overmap_buffer.set_seen( curs, true );
                } else {
                    if( std::optional<std::vector<tripoint_abs_omt>> used_points =
                            overmap_buffer.place_special( *uistate.place_special, curs,
                                                          uistate.omedit_rotation, false, true ) ) {
                        for( const tripoint_abs_omt &pos : *used_points ) {
                            overmap_buffer.set_seen( pos, true );
                        }
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
    }
}

static void set_special_args( tripoint_abs_omt &curs )
{
    std::optional<mapgen_arguments> *maybe_args = overmap_buffer.mapgen_args( curs );
    if( !maybe_args ) {
        popup( _( "No overmap special args at this location." ) );
        return;
    }
    if( *maybe_args ) {
        popup( _( "Overmap special args at this location have already been set." ) );
        return;
    }
    std::optional<overmap_special_id> s = overmap_buffer.overmap_special_at( curs );
    if( !s ) {
        popup( _( "No overmap special at this location from which to fetch parameters." ) );
        return;
    }
    const overmap_special &special = **s;
    const mapgen_parameters &params = special.get_params();
    mapgen_arguments args;
    for( const std::pair<const std::string, mapgen_parameter> &p : params.map ) {
        const std::string param_name = p.first;
        const mapgen_parameter &param = p.second;
        std::vector<std::string> possible_values = param.all_possible_values( params );
        uilist arg_menu;
        arg_menu.title = string_format( _( "Select value for mapgen argument %s: " ), param_name );
        for( size_t i = 0; i != possible_values.size(); ++i ) {
            const std::string &v = possible_values[i];
            arg_menu.addentry( i, true, 0, v );
        }
        arg_menu.query();

        if( arg_menu.ret < 0 ) {
            return;
        }
        args.map[param_name] =
            cata_variant::from_string( param.type(), std::move( possible_values[arg_menu.ret] ) );
    }
    *maybe_args = args;
}

static std::vector<tripoint_abs_omt> get_overmap_path_to( const tripoint_abs_omt &dest,
        bool driving )
{
    if( !overmap_buffer.seen( dest ) ) {
        return {};
    }
    const Character &player_character = get_player_character();
    map &here = get_map();
    const tripoint_abs_omt player_omt_pos = player_character.global_omt_location();
    overmap_path_params params;
    vehicle *player_veh = nullptr;
    if( driving ) {
        const optional_vpart_position vp = here.veh_at( player_character.pos() );
        if( !vp.has_value() ) {
            debugmsg( "Failed to find driven vehicle" );
            return {};
        }
        player_veh = &vp->vehicle();
        // for now we can only handle flyers if already in the air
        const bool can_fly = player_veh->is_rotorcraft() && player_veh->is_flying_in_air();
        const bool can_float = player_veh->can_float();
        const bool can_drive = player_veh->valid_wheel_config();
        // TODO: check engines/fuel
        if( can_fly ) {
            params = overmap_path_params::for_aircraft();
        } else if( can_float && !can_drive ) {
            params = overmap_path_params::for_watercraft();
        } else if( can_drive ) {
            const float offroad_coeff = player_veh->k_traction( player_veh->wheel_area() *
                                        player_veh->average_offroad_rating() );
            const bool tiny = player_veh->get_points().size() <= 3;
            params = overmap_path_params::for_land_vehicle( offroad_coeff, tiny, can_float );
        } else {
            return {};
        }
    } else {
        params = overmap_path_params::for_player();
        const oter_id dest_ter = overmap_buffer.ter_existing( dest );
        // already in water or going to a water tile
        if( here.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, player_character.pos() ) ||
            is_water_body( dest_ter ) ) {
            params.set_cost( oter_travel_cost_type::water, 100 );
        }
    }
    // literal "edge" case: the vehicle may be in a different OMT than the player
    const tripoint_abs_omt start_omt_pos = driving ? player_veh->global_omt_location() : player_omt_pos;
    if( dest == player_omt_pos || dest == start_omt_pos ) {
        return {};
    } else {
        return overmap_buffer.get_travel_path( start_omt_pos, dest, params );
    }
}

static int overmap_zoom_level = DEFAULT_TILESET_ZOOM;

static bool try_travel_to_destination( avatar &player_character, const tripoint_abs_omt curs,
                                       const bool driving )
{
    std::vector<tripoint_abs_omt> path = get_overmap_path_to( curs, driving );
    bool same_path_selected = path == player_character.omt_path;
    std::string confirm_msg;
    if( !driving && player_character.weight_carried() > player_character.weight_capacity() ) {
        confirm_msg = _( "You are overburdened, are you sure you want to travel (it may be painful)?" );
    } else if( !driving && player_character.in_vehicle ) {
        confirm_msg = _( "You are in a vehicle but not driving.  Are you sure you want to walk?" );
    } else if( driving ) {
        if( same_path_selected ) {
            confirm_msg = _( "Drive to this point?" );
        } else {
            confirm_msg = _( "Drive to your destination?" );
        }
    } else {
        if( same_path_selected ) {
            confirm_msg = _( "Travel to this point?" );
        } else {
            confirm_msg = _( "Travel to your destination?" );
        }
    }
    if( query_yn( confirm_msg ) ) {
        if( driving ) {
            player_character.assign_activity( autodrive_activity_actor() );
        } else {
            player_character.reset_move_mode();
            player_character.assign_activity( ACT_TRAVELLING );
        }
        return true;
    }
    return false;
}

static tripoint_abs_omt display( const tripoint_abs_omt &orig,
                                 const draw_data_t &data = draw_data_t() )
{
    const int previous_zoom = g->get_zoom();
    g->set_zoom( overmap_zoom_level );
    on_out_of_scope reset_zoom( [&]() {
        overmap_zoom_level = g->get_zoom();
        g->set_zoom( previous_zoom );
        g->mark_main_ui_adaptor_resize();
    } );

    background_pane bg_pane;

    ui_adaptor ui;
    ui.on_screen_resize( []( ui_adaptor & ui ) {
        /**
         * Handle possibly different overmap font size
         */
        OVERMAP_LEGEND_WIDTH = clamp( TERMX / 5, 28, 55 );
        OVERMAP_WINDOW_HEIGHT = TERMY;
        OVERMAP_WINDOW_WIDTH = TERMX - OVERMAP_LEGEND_WIDTH;
        OVERMAP_WINDOW_TERM_WIDTH = OVERMAP_WINDOW_WIDTH;
        OVERMAP_WINDOW_TERM_HEIGHT = OVERMAP_WINDOW_HEIGHT;

        to_overmap_font_dimension( OVERMAP_WINDOW_WIDTH, OVERMAP_WINDOW_HEIGHT );

        g->w_omlegend = catacurses::newwin( OVERMAP_WINDOW_TERM_HEIGHT, OVERMAP_LEGEND_WIDTH,
                                            point( OVERMAP_WINDOW_TERM_WIDTH, 0 ) );
        g->w_overmap = catacurses::newwin( OVERMAP_WINDOW_HEIGHT, OVERMAP_WINDOW_WIDTH, point_zero );

        ui.position_from_window( catacurses::stdscr );
    } );
    ui.mark_resize();

    tripoint_abs_omt ret = overmap::invalid_tripoint;
    tripoint_abs_omt curs( orig );

    if( data.select != tripoint_abs_omt( -1, -1, -1 ) ) {
        curs = data.select;
    }
    // Configure input context for navigating the map.
    input_context ictxt( "OVERMAP" );
    ictxt.register_action( "ANY_INPUT" );
    ictxt.register_directions();
    ictxt.register_action( "CONFIRM" );
    ictxt.register_action( "LEVEL_UP" );
    ictxt.register_action( "LEVEL_DOWN" );
    ictxt.register_action( "zoom_in" );
    ictxt.register_action( "zoom_out" );
    ictxt.register_action( "HELP_KEYBINDINGS" );
    ictxt.register_action( "MOUSE_MOVE" );
    ictxt.register_action( "SELECT" );
    ictxt.register_action( "CHOOSE_DESTINATION" );
    ictxt.register_action( "CENTER_ON_DESTINATION" );
    ictxt.register_action( "GO_TO_DESTINATION" );

    // Actions whose keys we want to display.
    ictxt.register_action( "CENTER" );
    ictxt.register_action( "CREATE_NOTE" );
    ictxt.register_action( "DELETE_NOTE" );
    ictxt.register_action( "MARK_DANGER" );
    ictxt.register_action( "SEARCH" );
    ictxt.register_action( "LIST_NOTES" );
    ictxt.register_action( "TOGGLE_MAP_NOTES" );
    ictxt.register_action( "TOGGLE_BLINKING" );
    ictxt.register_action( "TOGGLE_OVERLAYS" );
    ictxt.register_action( "TOGGLE_HORDES" );
    ictxt.register_action( "TOGGLE_LAND_USE_CODES" );
    ictxt.register_action( "TOGGLE_CITY_LABELS" );
    ictxt.register_action( "TOGGLE_MAP_REVEALS" );
    ictxt.register_action( "TOGGLE_EXPLORED" );
    ictxt.register_action( "TOGGLE_FAST_SCROLL" );
    ictxt.register_action( "TOGGLE_OVERMAP_WEATHER" );
    ictxt.register_action( "TOGGLE_FOREST_TRAILS" );
    ictxt.register_action( "MISSIONS" );

    if( data.debug_editor ) {
        ictxt.register_action( "PLACE_TERRAIN" );
        ictxt.register_action( "PLACE_SPECIAL" );
        ictxt.register_action( "SET_SPECIAL_ARGS" );
        ictxt.register_action( "LONG_TELEPORT" );
    }
    ictxt.register_action( "QUIT" );
    std::string action;
    bool show_explored = true;
    static bool fast_scroll = false;
    int fast_scroll_offset = get_option<int>( "FAST_SCROLL_OFFSET" );
    std::optional<tripoint> mouse_pos;
    std::chrono::time_point<std::chrono::steady_clock> last_blink = std::chrono::steady_clock::now();

    ui.on_redraw( [&]( ui_adaptor & ui ) {
        draw( ui, curs, orig, uistate.overmap_show_overlays,
              show_explored, fast_scroll, &ictxt, data );
    } );

    do {
        ui_manager::redraw();
#if (defined TILES || defined _WIN32 || defined WINDOWS )
        int scroll_timeout = get_option<int>( "EDGE_SCROLL" );
        // If EDGE_SCROLL is disabled, it will have a value of -1.
        // blinking won't work if handle_input() is passed a negative integer.
        if( scroll_timeout < 0 ) {
            scroll_timeout = get_option<int>( "BLINK_SPEED" );
        }
        action = ictxt.handle_input( scroll_timeout );
#else
        action = ictxt.handle_input( get_option<int>( "BLINK_SPEED" ) );
#endif
        if( const std::optional<tripoint> vec = ictxt.get_direction( action ) ) {
            int scroll_d = fast_scroll ? fast_scroll_offset : 1;
            curs += vec->xy() * scroll_d;
        } else if( action == "MOUSE_MOVE" || action == "TIMEOUT" ) {
            tripoint edge_scroll = g->mouse_edge_scrolling_overmap( ictxt );
            if( edge_scroll != tripoint_zero ) {
                if( action == "MOUSE_MOVE" ) {
                    edge_scroll *= 2;
                }
                curs += edge_scroll;
            }
        } else if( action == "SELECT" &&
                   ( mouse_pos = ictxt.get_coordinates( g->w_overmap, point_zero, true ) ) ) {
            curs += mouse_pos->xy();
        } else if( action == "CENTER" ) {
            curs = orig;
        } else if( action == "LEVEL_DOWN" && curs.z() > -OVERMAP_DEPTH ) {
            curs.z() -= 1;
        } else if( action == "LEVEL_UP" && curs.z() < OVERMAP_HEIGHT ) {
            curs.z() += 1;
        } else if( action == "zoom_out" ) {
            g->zoom_out_overmap();
            ui.mark_resize();
        } else  if( action == "zoom_in" ) {
            g->zoom_in_overmap();
            ui.mark_resize();
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
        } else if( action == "MARK_DANGER" ) {
            if( overmap_buffer.is_marked_dangerous( curs ) &&
                query_yn( _( "Remove dangerous mark?" ) ) ) {
                overmap_buffer.mark_note_dangerous( curs, 0,
                                                    false );
            } else if( ( overmap_buffer.is_marked_dangerous( curs ) &&
                         query_yn( _( "Edit dangerous mark?" ) ) ) || ( !overmap_buffer.is_marked_dangerous( curs ) &&
                                 // NOLINTNEXTLINE(cata-text-style): No need for two whitespaces
                                 query_yn( _( "Mark area as dangerous ( to avoid on auto move paths?  This will create a note if none exists already )" ) ) ) ) {
                if( !overmap_buffer.has_note( curs ) ) {
                    create_note( curs );
                }
                const int max_amount = 20;
                // NOLINTNEXTLINE(cata-text-style): No need for two whitespaces
                const std::string popupmsg = _( "Danger radius in overmap squares? ( 0-20 )" );
                int amount = string_input_popup()
                             .title( popupmsg )
                             .width( 20 )
                             .text( "0" )
                             .only_digits( true )
                             .query_int();
                if( amount > -1 && amount <= max_amount ) {
                    overmap_buffer.mark_note_dangerous( curs, amount, true );
                }
            }
        } else if( action == "LIST_NOTES" ) {
            const point_abs_omt p = draw_notes( curs );
            if( p != point_abs_omt( point_min ) ) {
                curs.x() = p.x();
                curs.y() = p.y();
            }
        } else if( action == "GO_TO_DESTINATION" ) {
            avatar &player_character = get_avatar();
            if( !player_character.omt_path.empty() ) {
                const bool driving = player_character.in_vehicle && player_character.controlling_vehicle;
                if( try_travel_to_destination( player_character, curs, driving ) ) {
                    action = "QUIT";
                }
            }
        } else if( action == "CENTER_ON_DESTINATION" ) {
            avatar &player_character = get_avatar();
            if( !player_character.omt_path.empty() ) {
                tripoint_abs_omt p = player_character.omt_path[0];
                curs.x() = p.x();
                curs.y() = p.y();
            }
        } else if( action == "CHOOSE_DESTINATION" ) {
            avatar &player_character = get_avatar();
            const bool driving = player_character.in_vehicle && player_character.controlling_vehicle;
            std::vector<tripoint_abs_omt> path = get_overmap_path_to( curs, driving );
            bool same_path_selected = false;
            if( path == player_character.omt_path ) {
                same_path_selected = true;
            } else {
                player_character.omt_path.swap( path );
            }
            if( same_path_selected && !player_character.omt_path.empty() ) {
                if( try_travel_to_destination( player_character, curs, driving ) ) {
                    action = "QUIT";
                }
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
        } else if( action == "TOGGLE_MAP_REVEALS" ) {
            uistate.overmap_show_revealed_omts = !uistate.overmap_show_revealed_omts;
        } else if( action == "TOGGLE_EXPLORED" ) {
            overmap_buffer.toggle_explored( curs );
        } else if( action == "TOGGLE_OVERMAP_WEATHER" ) {
            if( get_map().is_outside( get_player_character().pos() ) ) {
                uistate.overmap_visible_weather = !uistate.overmap_visible_weather;
            }
        } else if( action == "TOGGLE_FAST_SCROLL" ) {
            fast_scroll = !fast_scroll;
        } else if( action == "TOGGLE_FOREST_TRAILS" ) {
            uistate.overmap_show_forest_trails = !uistate.overmap_show_forest_trails;
        } else if( action == "SEARCH" ) {
            if( !search( ui, curs, orig ) ) {
                continue;
            }
        } else if( action == "PLACE_TERRAIN" || action == "PLACE_SPECIAL" ) {
            place_ter_or_special( ui, curs, action );
        } else if( action == "SET_SPECIAL_ARGS" ) {
            set_special_args( curs );
        } else if( action == "LONG_TELEPORT" && curs != overmap::invalid_tripoint ) {
            g->place_player_overmap( curs );
            add_msg( _( "You teleport to submap %s." ), curs.to_string() );
            action = "QUIT";
        } else if( action == "MISSIONS" ) {
            g->list_missions();
        }

        std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
        if( now > last_blink + std::chrono::milliseconds( get_option<int>( "BLINK_SPEED" ) ) ) {
            if( uistate.overmap_blinking ) {
                uistate.overmap_show_overlays = !uistate.overmap_show_overlays;
            }
            last_blink = now;
        }
    } while( action != "QUIT" && action != "CONFIRM" );
    return ret;
}

} // namespace overmap_ui

void ui::omap::display()
{
    overmap_ui::display( get_player_character().global_omt_location(), overmap_ui::draw_data_t() );
}

void ui::omap::display_hordes()
{
    overmap_ui::draw_data_t data;
    uistate.overmap_debug_mongroup = true;
    overmap_ui::display( get_player_character().global_omt_location(), data );
    uistate.overmap_debug_mongroup = false;
}

void ui::omap::display_weather()
{
    overmap_ui::draw_data_t data;
    uistate.overmap_debug_weather = true;
    tripoint_abs_omt pos = get_player_character().global_omt_location();
    pos.z() = 10;
    overmap_ui::display( pos, data );
    uistate.overmap_debug_weather = false;
}

void ui::omap::display_visible_weather()
{
    overmap_ui::draw_data_t data;
    uistate.overmap_visible_weather = true;
    tripoint_abs_omt pos = get_player_character().global_omt_location();
    pos.z() = 10;
    overmap_ui::display( pos, data );
    uistate.overmap_visible_weather = false;
}

void ui::omap::display_scents()
{
    overmap_ui::draw_data_t data;
    data.debug_scent = true;
    overmap_ui::display( get_player_character().global_omt_location(), data );
}

void ui::omap::display_editor()
{
    overmap_ui::draw_data_t data;
    data.debug_editor = true;
    overmap_ui::display( get_player_character().global_omt_location(), data );
}

void ui::omap::display_zones( const tripoint_abs_omt &center, const tripoint_abs_omt &select,
                              const int iZoneIndex )
{
    overmap_ui::draw_data_t data;
    data.select = select;
    data.iZoneIndex = iZoneIndex;
    overmap_ui::display( center, data );
}

tripoint_abs_omt ui::omap::choose_point( bool show_debug_info )
{
    overmap_ui::draw_data_t data;
    data.debug_info = show_debug_info;
    return overmap_ui::display( get_player_character().global_omt_location(), data );
}

tripoint_abs_omt ui::omap::choose_point( const tripoint_abs_omt &origin, bool show_debug_info )
{
    overmap_ui::draw_data_t data;
    data.debug_info = show_debug_info;
    return overmap_ui::display( origin, data );
}

tripoint_abs_omt ui::omap::choose_point( int z, bool show_debug_info )
{
    overmap_ui::draw_data_t data;
    data.debug_info = show_debug_info;
    tripoint_abs_omt loc = get_player_character().global_omt_location();
    loc.z() = z;
    return overmap_ui::display( loc, data );
}

void ui::omap::setup_cities_menu( uilist &cities_menu, std::vector<city> &cities_container )
{
    if( get_option<bool>( "SELECT_STARTING_CITY" ) ) {
        uilist_entry entry_random_city( RANDOM_CITY_ENTRY, true, '*',
                                        _( "<color_red>* Random city *</color>" ),
                                        _( "Location: <color_white>(?,?)</color>:<color_white>(?,?)</color>" ),
                                        //~ "pop" refers to population count
                                        _( "(pop <color_white>?</color>)" )
                                      );
        cities_menu.entries.emplace_back( entry_random_city );
        cities_menu.desc_enabled = true;
        cities_menu.title = _( "Select a starting city" );
        for( const city &c : cities_container ) {
            uilist_entry entry( c.database_id, true, -1, c.name,
                                string_format(
                                    _( "Location: <color_white>%s</color>:<color_white>%s</color>" ),
                                    c.pos_om.to_string(), c.pos.to_string() ),
                                //~ "pop" refers to population count
                                string_format( _( "(pop <color_white>%s</color>)" ), c.population ) );
            cities_menu.entries.emplace_back( entry );
        }
        cities_menu.w_height_setup = TERMY - 4;
    }
}

std::optional<city> ui::omap::select_city( uilist &cities_menu,
        std::vector<city> &cities_container, bool random )
{
    std::optional<city> ret_val = std::nullopt;
    if( random ) {
        ret_val = random_entry( cities_container );
    } else {
        cities_menu.query();
        if( cities_menu.ret == RANDOM_CITY_ENTRY ) {
            ret_val = random_entry( cities_container );
        } else if( cities_menu.ret == UILIST_CANCEL ) {
            ret_val = std::nullopt;
        } else {
            if( cities_menu.entries.size() > 1 ) {
                ret_val = cities_container[cities_menu.selected - 1];
            }
        }
    }
    return ret_val;
}
