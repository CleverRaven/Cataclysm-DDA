#include "overmap_ui.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <ratio>
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
#include "cata_assert.h"
#include "cata_variant.h"
#include "character_id.h"
#include "debug.h"
#include "enum_conversions.h"
#include "input_enums.h"
#include "mapdata.h"
#include "mapgen_parameter.h"
#include "mapgendata.h"
#include "memory_fast.h"
#include "monster.h"
#include "simple_pathfinding.h"
#include "translation.h"
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
#include "debug_menu.h"
#include "display.h"
#include "game.h"
#include "game_constants.h"
#include "game_ui.h"
#include "input_context.h"
#include "line.h"
#include "localized_comparator.h"
#include "map.h"
#include "map_iterator.h"
#include "mapbuffer.h"
#include "messages.h"
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
#include "ui_manager.h"
#include "uilist.h"
#include "uistate.h"
#include "units.h"
#include "units_utility.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "weather_gen.h"
#include "weather_type.h"

#ifdef TILES
#include "cached_options.h"
#endif // TILES

enum class cube_direction : int;

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
// returns true if a note was created or edited, false otherwise
static bool create_note( const tripoint_abs_omt &curs,
                         std::optional<std::string> context = std::nullopt );

// {note symbol, note color, offset to text}
std::tuple<char, nc_color, size_t> get_note_display_info( std::string_view note )
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
        om_vision_level vision = has_debug_vision ? om_vision_level::full :
                                 overmap_buffer.seen( dest );
        if( vision != om_vision_level::unseen ) {
            // Only load terrain if we can actually see it
            oter_id cur_ter = overmap_buffer.ter( dest );
            ter_color = cur_ter->get_color( vision );
            ter_sym = cur_ter->get_symbol( vision );
        } else {
            ter_color = oter_unexplored.obj().get_color( om_vision_level::full );
            ter_sym = oter_unexplored.obj().get_symbol( om_vision_level::full );
        }
        map_around[index++] = std::make_pair( ter_color, ter_sym );
    }
    return map_around;
}

static void update_note_preview( std::string_view note,
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
    print_colored_text( *w_preview_title, point::zero, default_color, note_color, note_text,
                        report_color_error::no );
    int note_text_width = utf8_width( note_text );
    wattron( *w_preview_title, c_white );
    mvwaddch( *w_preview_title, point( note_text_width, 0 ), LINE_XOXO );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwhline( *w_preview_title, point( 0, 1 ), LINE_OXOX, note_text_width );
    mvwaddch( *w_preview_title, point( note_text_width, 1 ), LINE_XOOX );
    wattroff( *w_preview_title, c_white );
    wnoutrefresh( *w_preview_title );

    const point npm_offset( point::south_east );
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

        if( !overmap_buffer.seen_more_than( tripoint_abs_omt( city_pos, center.z() ),
                                            om_vision_level::outlines ) ) {
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

        if( !overmap_buffer.seen_more_than( tripoint_abs_omt( camp_pos, center.z() ),
                                            om_vision_level::outlines ) ) {
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
                w_preview_title = catacurses::newwin( 2, max_note_display_length + 1, point::zero );
                w_preview_map = catacurses::newwin( npm_height + 2, npm_width + 2, point( 0, 2 ) );
                preview_windows = std::make_tuple( &w_preview, &w_preview_title, &w_preview_map );

                ui.position( point::zero, point( max_note_display_length + 1, npm_height + 4 ) );
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
                    if( create_note( note_location() ) ) {
                        menu->ret = UILIST_MAP_NOTE_EDITED;
                    }
                    return true;
                }
                if( action == "MARK_DANGER" ) {
                    const int danger_radius = overmap_buffer.note_danger_radius( note_location() );
                    if( danger_radius >= 0 &&
                        query_yn( _( "Remove dangerous mark?" ) ) ) {
                        overmap_buffer.mark_note_dangerous( note_location(), 0, false );
                        menu->ret = UILIST_MAP_NOTE_EDITED;
                        return true;
                    } else {
                        bool has_note = overmap_buffer.has_note( note_location() );
                        if( !has_note ) {
                            has_note = create_note( note_location(), _( "Create a danger note." ) );
                        }
                        if( !has_note ) {
                            return true;
                        }
                        const int max_amount = 20;
                        int amount = clamp( danger_radius, 0, max_amount );
                        // NOLINTNEXTLINE(cata-text-style): No need for two whitespaces
                        if( query_int( amount, true, _( "Danger radius in overmap squares? (0-%d)" ), max_amount )
                            && amount >= 0 && amount <= max_amount ) {
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
    point_abs_omt result = point_abs_omt::invalid;

    bool refresh = true;
    bool first = true;
    uilist nmenu;
    while( refresh ) {
        refresh = false;
        nmenu.color_error( false );
        int selected = nmenu.selected;
        nmenu.init();
        nmenu.selected = selected;
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
            if( first && p == origin.xy() ) {
                nmenu.selected = row;
            }
            const std::string &note = point_with_note.second;
            auto om_symbol = get_note_display_info( note );
            const nc_color note_color = std::get<1>( om_symbol );
            const std::string note_symbol = std::string( 1, std::get<0>( om_symbol ) );
            const std::string note_text = note.substr( std::get<2>( om_symbol ), std::string::npos );
            point_abs_omt p_omt( p );
            const point_abs_omt p_player = get_player_character().pos_abs_omt().xy();
            const int distance_player = rl_dist( p_player, p_omt );
            const point_abs_sm sm_pos = project_to<coords::sm>( p_omt );
            const point_abs_om p_om = project_to<coords::om>( p_omt );
            const std::string location_desc =
                overmap_buffer.get_description_at( tripoint_abs_sm( sm_pos, origin.z() ) );
            const bool is_dangerous =
                overmap_buffer.is_marked_dangerous( tripoint_abs_omt( p, origin.z() ) );
            const int note_danger_radius = overmap_buffer.note_danger_radius( tripoint_abs_omt( p,
                                           origin.z() ) );
            nc_color bracket_color = note_danger_radius >= 0 ? c_red : c_light_gray;
            std::string danger_desc_text = note_danger_radius >= 0 ? string_format(
                                               _( "DANGEROUS AREA!  (R=%d)" ),
                                               note_danger_radius ) : is_dangerous ? _( "IN DANGEROUS AREA!" ) : "";
            nmenu.addentry_desc(
                string_format( colorize( _( "[%s] %s" ), bracket_color ), colorize( note_symbol, note_color ),
                               note_text ),
                string_format(
                    _( "<color_red>LEVEL %i, %d'%d, %d'%d</color>: %s "
                       "(Distance: <color_white>%d</color>) <color_red>%s</color>" ),
                    origin.z(), p_om.x(), p_omt.x(), p_om.y(), p_omt.y(), location_desc,
                    distance_player, danger_desc_text ) );
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
        first = false;
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

static std::unordered_map<point_abs_omt, bool> generated_omts;

bool is_generated_omt( const point_abs_omt &omp )
{
    if( const auto it = generated_omts.find( omp ); it != generated_omts.end() ) {
        return it->second;
    }
    const bool generated = MAPBUFFER.submap_exists_approx( { project_to<coords::sm>( omp ), 0 } );
    generated_omts.insert( { omp, generated } );
    return generated;
}

static void draw_ascii( const catacurses::window &w, overmap_draw_data_t &data )
{
    const tripoint_abs_omt &orig = data.origin_pos;
    const tripoint_abs_omt &cursor_pos = data.cursor_pos;
    const std::vector<tripoint_abs_omt> &display_path = data.display_path;
    bool blink = uistate.overmap_show_overlays;

    const int om_map_width = OVERMAP_WINDOW_WIDTH;
    const int om_map_height = OVERMAP_WINDOW_HEIGHT;
    const int om_half_width = om_map_width / 2;
    const int om_half_height = om_map_height / 2;

    avatar &player_character = get_avatar();
    // Target of current mission
    const tripoint_abs_omt target = player_character.get_active_mission_target();
    const bool has_target = !target.is_invalid();
    oter_id ccur_ter = oter_str_id::NULL_ID();
    // Debug vision allows seeing everything
    const bool has_debug_vision = player_character.has_trait( trait_DEBUG_NIGHTVISION );
    // sight_points is hoisted for speed reasons.
    const int sight_points = !has_debug_vision ?
                             player_character.overmap_modified_sight_range( g->light_level( player_character.posz() ) ) :
                             100;

    oter_display_lru lru_cache;
    oter_display_options oter_opts( orig, sight_points );
    oter_opts.show_weather = ( uistate.overmap_debug_weather || uistate.overmap_visible_weather ) &&
                             cursor_pos.z() == 10;
    oter_opts.show_pc = true;
    oter_opts.debug_scent = data.debug_scent;
    oter_opts.show_map_revealed = uistate.overmap_show_revealed_omts;
    oter_opts.showhordes = uistate.overmap_show_hordes;
    oter_opts.show_explored = data.show_explored;

    std::string &sZoneName = oter_opts.sZoneName;
    tripoint_abs_omt &tripointZone = oter_opts.tripointZone;
    const zone_manager &zones = zone_manager::get_manager();

    oter_opts.mission_target = target;

    if( data.fast_traveling ) {
        tripoint_abs_omt &next_path = player_character.omt_path.back();
        data.cursor_pos = next_path;
        oter_opts.center = next_path;
        blink = true;
    }
    oter_opts.blink = blink;

    if( data.iZoneIndex != -1 ) {
        const zone_data &zone = zones.get_zones()[data.iZoneIndex].get();
        sZoneName = zone.get_name();
        tripointZone = project_to<coords::omt>(
                           zone.get_center_point() );
    }

    // If we're debugging monster groups, find the monster group we've selected
    const mongroup *mgroup = nullptr;
    std::vector<mongroup *> mgroups;
    if( uistate.overmap_debug_mongroup ) {
        mgroups = overmap_buffer.monsters_at( cursor_pos );
        for( mongroup * const &mgp : mgroups ) {
            mgroup = mgp;
            if( mgp->horde ) {
                break;
            }
        }
    }

    const tripoint_abs_omt corner = cursor_pos - point( om_half_width, om_half_height );

    // For use with place_special: cache the color and symbol of each submap
    // and record the bounds to optimize lookups below
    std::unordered_map<point_rel_omt, std::pair<std::string, nc_color>> special_cache;

    point_rel_omt s_begin;
    point_rel_omt s_end;
    if( blink && uistate.place_special ) {
        for( const overmap_special_terrain &s_ter : uistate.place_special->preview_terrains() ) {
            // Preview should only yield the terrains on the zero z-level
            cata_assert( s_ter.p.z() == 0 );

            const point_rel_omt rp( om_direction::rotate( s_ter.p.xy(), uistate.omedit_rotation ) );
            const oter_id oter = s_ter.terrain->get_rotated( uistate.omedit_rotation );

            special_cache.insert( std::make_pair( rp, std::make_pair( oter->get_symbol( om_vision_level::full ),
                                                  oter->get_color( om_vision_level::full ) ) ) );

            s_begin.x() = std::min( s_begin.x(), rp.x() );
            s_begin.y() = std::min( s_begin.y(), rp.y() );
            s_end.x() = std::max( s_end.x(), rp.x() );
            s_end.y() = std::max( s_end.y(), rp.y() );
        }
    }

    // Cache NPCs since time to draw them is linear (per seen tile) with their count
    std::unordered_set<tripoint_abs_omt> &npc_path_route = oter_opts.npc_path_route;
    std::unordered_map<point_abs_omt, int> &player_path_route = oter_opts.player_path_route;
    std::unordered_map<tripoint_abs_omt, oter_display_options::npc_coloring> &npc_color =
        oter_opts.npc_color;
    auto npcs_near_player = overmap_buffer.get_npcs_near_player( sight_points );
    if( blink ) {
        // get seen NPCs
        for( const auto &np : npcs_near_player ) {
            if( np->posz() != cursor_pos.z() ) {
                continue;
            }

            const tripoint_abs_omt pos = np->pos_abs_omt();
            if( has_debug_vision || overmap_buffer.seen_more_than( pos, om_vision_level::details ) ) {
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
        if( !display_path.empty() ) {
            for( const tripoint_abs_omt &elem : display_path ) {
                npc_path_route.insert( elem );
            }
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
            if( np->posz() != cursor_pos.z() ) {
                continue;
            }
            const tripoint_abs_omt pos = np->pos_abs_omt();
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

            const om_vision_level vision = has_debug_vision ? om_vision_level::full :
                                           overmap_buffer.seen( omp );
            if( vision == om_vision_level::unseen ) {
                // Only load terrain if we can actually see it
                cur_ter = overmap_buffer.ter( omp );
            }

            oter_display_args oter_args( vision );
            std::tie( ter_sym, ter_color ) = oter_symbol_and_color( omp, oter_args, oter_opts, &lru_cache );

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
                        if( get_and_assign_los( oter_args.los, player_character, omp, sight_points ) ) {
                            ter_color = c_light_blue;
                        } else {
                            ter_color = c_blue;
                        }
                    }
                }
            }

            // Preview for place_terrain or place_special
            if( uistate.place_terrain || uistate.place_special ) {
                if( blink && uistate.place_terrain && omp.xy() == cursor_pos.xy() ) {
                    ter_color = uistate.place_terrain->get_color( om_vision_level::full );
                    ter_sym = uistate.place_terrain->get_symbol( om_vision_level::full );
                } else if( blink && uistate.place_special ) {
                    const point_rel_omt from_center = omp.xy() - cursor_pos.xy();
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
                if( is_generated_omt( omp.xy() ) ) {
                    ter_color = red_background( ter_color );
                }
            }

            if( omp.xy() == cursor_pos.xy() && !uistate.place_special ) {
                ccur_ter = cur_ter;
                mvwputch_hi( w, point( i, j ), ter_color, ter_sym );
            } else {
                mvwputch( w, point( i, j ), ter_color, ter_sym );
            }
        }
    }

    if( cursor_pos.z() == 0 && uistate.overmap_show_city_labels ) {
        draw_city_labels( w, cursor_pos );
        draw_camp_labels( w, cursor_pos );
    }

    half_open_rectangle<point_abs_omt> screen_bounds(
        corner.xy(), corner.xy() + point( om_map_width, om_map_height ) );

    if( has_target && blink && !screen_bounds.contains( target.xy() ) ) {
        point_rel_omt marker = clamp( target.xy(), screen_bounds ) - corner.xy();
        std::string marker_sym = " ";

        switch( direction_from( cursor_pos.xy(), target.xy() ) ) {
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

    if( !data.message.empty() ) {
        corner_text.emplace_back( c_white, data.message );
    }

    if( uistate.overmap_show_map_notes ) {
        const std::string &note_text = overmap_buffer.note( cursor_pos );
        if( !note_text.empty() ) {
            const std::tuple<char, nc_color, size_t> note_info = get_note_display_info(
                        note_text );
            const size_t pos = std::get<2>( note_info );
            if( pos != std::string::npos ) {
                corner_text.emplace_back( std::get<1>( note_info ), note_text.substr( pos ) );
            }
            if( overmap_buffer.is_marked_dangerous( cursor_pos ) ) {
                corner_text.emplace_back( c_red, _( "DANGEROUS AREA!" ) );
            }
        }
    }

    if( has_debug_vision || overmap_buffer.seen_more_than( cursor_pos, om_vision_level::details ) ) {
        for( const auto &npc : npcs_near_player ) {
            if( !npc->marked_for_death && npc->pos_abs_omt() == cursor_pos ) {
                corner_text.emplace_back( npc->basic_symbol_color(), npc->get_name() );
            }
        }
    }

    for( om_vehicle &v : overmap_buffer.get_vehicle( cursor_pos ) ) {
        corner_text.emplace_back( c_white, v.name );
    }

    if( !corner_text.empty() ) {
        int maxlen = 0;
        for( const auto &line : corner_text ) {
            maxlen = std::max( maxlen, utf8_width( line.second, true ) );
        }

        mvwrectf( w, point( 2, 2 ), c_yellow, ' ', maxlen, corner_text.size() );
        for( size_t i = 0; i < corner_text.size(); i++ ) {
            const auto &pr = corner_text[i];
            nc_color default_color = c_unset;
            print_colored_text( w, point( 2, i + 2 ), default_color, pr.first, pr.second,
                                report_color_error::no );
        }
        wattron( w, c_white );
        mvwaddch( w, point::south_east, LINE_OXXO ); // .-
        mvwhline( w, point( 2, 1 ), LINE_OXOX, maxlen ); // -
        mvwaddch( w, point( 1, corner_text.size() + 2 ), LINE_XXOO ); // '-
        mvwvline( w, point( 1, 2 ), LINE_XOXO, corner_text.size() ); // |
        mvwvline( w, point( maxlen + 2, 2 ), LINE_XOXO, corner_text.size() ); // |
        mvwaddch( w, point( maxlen + 2, 1 ), LINE_OOXX ); // -.
        mvwhline( w, point( 2, corner_text.size() + 2 ), LINE_OXOX, maxlen + 1 ); // -
        mvwaddch( w, point( maxlen + 2, corner_text.size() + 2 ), LINE_XOOX ); // -'
        wattroff( w, c_white );
    }

    if( !sZoneName.empty() && tripointZone.xy() == cursor_pos.xy() ) {
        std::string sTemp = _( "Zone:" );
        sTemp += " " + sZoneName;

        const int length = utf8_width( sTemp );
        mvwprintz( w, point( 0, om_map_height - 1 ), c_yellow, sTemp );
        wattron( w, c_white );
        mvwhline( w, point( 0, om_map_height - 2 ), LINE_OXOX, length + 1 );
        mvwaddch( w, point( length, om_map_height - 2 ), LINE_OOXX );
        mvwaddch( w, point( length, om_map_height - 1 ), LINE_XOXO );
        wattroff( w, c_white );
    }

    // draw nice crosshair around the cursor
    if( blink && !uistate.place_terrain && !uistate.place_special ) {
        wattron( w, c_light_gray );
        mvwaddch( w, point( om_half_width - 1, om_half_height - 1 ), LINE_OXXO );
        mvwaddch( w, point( om_half_width + 1, om_half_height - 1 ), LINE_OOXX );
        mvwaddch( w, point( om_half_width - 1, om_half_height + 1 ), LINE_XXOO );
        mvwaddch( w, point( om_half_width + 1, om_half_height + 1 ), LINE_XOOX );
        wattroff( w, c_light_gray );
    }
    // Done with all drawing!
    wnoutrefresh( w );
}

static void draw_om_sidebar( ui_adaptor &ui,
                             const catacurses::window &wbar, const input_context &inp_ctxt, const overmap_draw_data_t &data )
{
    const tripoint_abs_omt &orig = data.origin_pos;
    const tripoint_abs_omt &cursor_pos = data.cursor_pos;

    avatar &player_character = get_avatar();
    // Debug vision allows seeing everything
    const bool has_debug_vision = player_character.has_trait( trait_DEBUG_NIGHTVISION );
    // sight_points is hoisted for speed reasons.
    const int sight_points = !has_debug_vision ?
                             player_character.overmap_modified_sight_range( g->light_level( player_character.posz() ) ) :
                             100;
    om_vision_level center_vision = has_debug_vision ? om_vision_level::full :
                                    overmap_buffer.seen( cursor_pos );
    const tripoint_abs_omt target = player_character.get_active_mission_target();
    const bool has_target = !target.is_invalid();
    const bool viewing_weather = uistate.overmap_debug_weather || uistate.overmap_visible_weather;

    // If we're debugging monster groups, find the monster group we've selected
    std::vector<mongroup *> mgroups;
    if( uistate.overmap_debug_mongroup ) {
        mgroups = overmap_buffer.monsters_at( cursor_pos );
        for( mongroup * const &mgp : mgroups ) {
            if( mgp->horde ) {
                break;
            }
        }
    }

    // Draw the vertical line
    mvwvline( wbar, point::zero, c_white, LINE_XOXO, TERMY );

    // Clear the legend
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwrectf( wbar, point( 1, 0 ), c_black, ' ', getmaxx( wbar ), TERMY );

    // Draw text describing the overmap tile at the cursor position.
    int lines = 1;
    if( center_vision != om_vision_level::unseen ) {
        if( !mgroups.empty() ) {
            const point desc_pos( 3, 6 );
            ui.set_cursor( wbar, desc_pos );
            int line_number = 0;
            for( mongroup * const &mgroup : mgroups ) {
                wattron( wbar, c_blue );
                mvwprintw( wbar, desc_pos + point( 0, line_number++ ),
                           "  Species: %s", mgroup->type.c_str() );
                mvwprintw( wbar, desc_pos + point( 0, line_number++ ),
                           "# monsters: %d", mgroup->population + mgroup->monsters.size() );
                if( !mgroup->horde ) {
                    wattroff( wbar, c_blue );
                    continue;
                }
                mvwprintw( wbar, desc_pos + point( 0, line_number++ ),
                           "  Interest: %d", mgroup->interest );
                mvwprintw( wbar, desc_pos + point( 0, line_number++ ),
                           "  Target: %s", mgroup->target.to_string() );
                wattroff( wbar, c_blue );
                mvwprintz( wbar, desc_pos + point( 0, line_number++ ),
                           c_red, "x" );
            }
        } else {
            const oter_t &ter = overmap_buffer.ter( cursor_pos ).obj();
            const auto sm_pos = project_to<coords::sm>( cursor_pos );

            if( ter.blends_adjacent( center_vision ) ) {
                oter_vision::blended_omt info = oter_vision::get_blended_omt_info( cursor_pos, center_vision );
                // NOLINTNEXTLINE(cata-use-named-point-constants)
                mvwputch( wbar, point( 1, 1 ), info.color, info.sym );
            } else {
                // NOLINTNEXTLINE(cata-use-named-point-constants)
                mvwputch( wbar, point( 1, 1 ), ter.get_color( center_vision ), ter.get_symbol( center_vision ) );
            }

            const point desc_pos( 3, 1 );
            ui.set_cursor( wbar, desc_pos );
            lines = fold_and_print( wbar, desc_pos, getmaxx( wbar ) - desc_pos.x,
                                    c_light_gray,
                                    overmap_buffer.get_description_at( sm_pos ) );
            if( center_vision != om_vision_level::full ) {
                std::string vision_level_string;
                switch( center_vision ) {
                    case om_vision_level::vague:
                        vision_level_string = _( "You can only make out vague details of what's here." );
                        break;
                    case om_vision_level::outlines:
                        vision_level_string = _( "You can only make out outlines of what's here." );
                        break;
                    case om_vision_level::details:
                        vision_level_string = _( "You can make out some details of what's here." );
                        break;
                    default:
                        vision_level_string = _( "This is a bug!" );
                        break;
                }
                lines += fold_and_print( wbar, point( 3, lines + 1 ), getmaxx( wbar ) - 3, c_light_gray,
                                         vision_level_string );
            }
        }
    } else {
        const oter_t &ter = oter_unexplored.obj();

        // NOLINTNEXTLINE(cata-use-named-point-constants)
        mvwputch( wbar, point( 1, 1 ), ter.get_color( om_vision_level::full ),
                  ter.get_symbol( om_vision_level::full ) );

        const point desc_pos( 3, 1 );
        ui.set_cursor( wbar, desc_pos );
        lines = fold_and_print( wbar, desc_pos, getmaxx( wbar ) - desc_pos.x,
                                ter.get_color( om_vision_level::full ), ter.get_name( om_vision_level::full ) );
    }

    // Describe the weather conditions on the following line, if weather is visible
    if( viewing_weather ) {
        const bool weather_is_visible = uistate.overmap_debug_weather ||
                                        player_character.overmap_los( cursor_pos, sight_points * 2 );
        if( weather_is_visible ) {
            // NOLINTNEXTLINE(cata-use-named-point-constants)
            mvwprintz( wbar, point( 3, ++lines ), get_weather_at_point( cursor_pos )->color,
                       get_weather_at_point( cursor_pos )->name.translated() );
        } else {
            // NOLINTNEXTLINE(cata-use-named-point-constants)
            mvwprintz( wbar, point( 1, ++lines ), c_dark_gray, _( "# Weather unknown" ) );
        }
    }

    if( ( data.debug_editor && center_vision != om_vision_level::unseen ) || data.debug_info ) {
        wattron( wbar, c_white );
        mvwprintw( wbar, point( 1, ++lines ), "abs_omt: %s", cursor_pos.to_string() );
        const oter_t &oter = overmap_buffer.ter( cursor_pos ).obj();
        mvwprintw( wbar, point( 1, ++lines ), "oter: %s (rot %d)", oter.id.str(), oter.get_rotation() );
        mvwprintw( wbar, point( 1, ++lines ), "oter_type: %s", oter.get_type_id().str() );
        // tileset ids come with a prefix that must be stripped
        mvwprintw( wbar, point( 1, ++lines ), "tileset id: '%s'",
                   oter.get_tileset_id( center_vision ).substr( 3 ) );
        std::vector<oter_id> predecessors = overmap_buffer.predecessors( cursor_pos );
        if( !predecessors.empty() ) {
            mvwprintw( wbar, point( 1, ++lines ), "predecessors:" );
            for( auto pred = predecessors.rbegin(); pred != predecessors.rend(); ++pred ) {
                mvwprintw( wbar, point( 1, ++lines ), "- %s", pred->id().str() );
            }
        }
        std::optional<mapgen_arguments> *args = overmap_buffer.mapgen_args( cursor_pos );
        if( args ) {
            if( *args ) {
                for( const std::pair<const std::string, cata_variant> &arg : ( **args ).map ) {
                    mvwprintw( wbar, point( 1, ++lines ), "%s = %s", arg.first, arg.second.get_string() );
                }
            } else {
                mvwprintw( wbar, point( 1, ++lines ), "args not yet set" );
            }
        }

        for( cube_direction dir : all_enum_values<cube_direction>() ) {
            if( std::string *join = overmap_buffer.join_used_at( { cursor_pos, dir } ) ) {
                mvwprintw( wbar, point( 1, ++lines ), "join %s: %s", io::enum_to_string( dir ), *join );
            }
        }
        wattroff( wbar, c_white );

        wattron( wbar, c_red );
        for( const mongroup *mg : overmap_buffer.monsters_at( cursor_pos ) ) {
            mvwprintw( wbar, point( 1, ++lines ), "mongroup %s (%zu/%u), %s %s%s",
                       mg->type.str(), mg->monsters.size(), mg->population,
                       io::enum_to_string( mg->behaviour ),
                       mg->dying ? "x" : "", mg->horde ? "h" : "" );
            mvwprintw( wbar, point( 1, ++lines ), "target: %s (%d)",
                       project_to<coords::omt>( mg->target ).to_string(), mg->interest );
        }
        wattroff( wbar, c_red );
    }

    wattron( wbar, c_white );
    if( has_target ) {
        const int distance = rl_dist( cursor_pos, target );
        mvwprintw( wbar, point( 1, ++lines ), _( "Distance to current objective:" ) );
        mvwprintw( wbar, point( 1, ++lines ), _( "%d tiles" ), distance );
        // One OMT is 24 tiles across, at 1x1 meters each, so we can simply do number of OMTs * 24
        mvwprintw( wbar, point( 1, ++lines ), _( "%s" ), length_to_string_approx( distance * 24_meter ) );

        const int above_below = target.z() - orig.z();
        std::string msg;
        if( above_below > 0 ) {
            msg = _( "Above us" );
        } else if( above_below < 0 ) {
            msg = _( "Below us" );
        }
        if( above_below != 0 ) {
            mvwprintw( wbar, point( 1, ++lines ), _( "%s" ), msg );
        }
    }

    //Show mission targets on this location
    for( mission *&mission : player_character.get_active_missions() ) {
        if( mission->get_target() == cursor_pos ) {
            mvwprintw( wbar, point( 1, ++lines ), mission->name() );
        }
    }
    wattroff( wbar, c_white );

    const auto print_hint = [&]( const std::string & action, nc_color color = c_magenta ) {
        lines += fold_and_print( wbar, point( 1, lines ), getmaxx( wbar ) - 1, color,
                                 string_format( _( "%s - %s" ),  inp_ctxt.get_desc( action ), inp_ctxt.get_action_name( action ) ) );
    };

    if( data.debug_editor ) {
        lines++;
        print_hint( "REVEAL_MAP", c_light_blue );
        print_hint( "LONG_TELEPORT", c_light_blue );
        print_hint( "PLACE_SPECIAL", c_light_blue );
        print_hint( "PLACE_TERRAIN", c_light_blue );
        print_hint( "SET_SPECIAL_ARGS", c_light_blue );
        print_hint( "MODIFY_HORDE", c_light_blue );
        lines--;
    } else {
        lines = 11;
    }

    wattron( wbar, c_magenta );
    mvwprintw( wbar, point( 1, ++lines ), _( "Use movement keys to pan." ) );
    mvwprintw( wbar, point( 1, ++lines ), string_format( _( "Press %s to preview route." ),
               inp_ctxt.get_desc( "CHOOSE_DESTINATION" ) ) );
    mvwprintw( wbar, point( 1, ++lines ), _( "Press again to confirm." ) );
    lines += 2;
    wattroff( wbar, c_magenta );

    const bool show_overlays = uistate.overmap_show_overlays || uistate.overmap_blinking;
    const bool is_explored = overmap_buffer.is_explored( cursor_pos );

    print_hint( "LEVEL_UP" );
    print_hint( "LEVEL_DOWN" );
    print_hint( "look" );
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
    print_hint( "TOGGLE_FAST_SCROLL", uistate.overmap_fast_scroll ? c_pink : c_magenta );
    print_hint( "TOGGLE_FOREST_TRAILS", uistate.overmap_show_forest_trails ? c_pink : c_magenta );
    print_hint( "TOGGLE_FAST_TRAVEL", uistate.overmap_fast_travel ? c_pink : c_magenta );
    print_hint( "TOGGLE_OVERMAP_WEATHER",
                !get_map().is_outside( get_player_character().pos_bub() ) ? c_dark_gray :
                uistate.overmap_visible_weather ? c_pink : c_magenta );
    print_hint( "HELP_KEYBINDINGS" );
    print_hint( "QUIT" );

    const std::string coords = display::overmap_position_text( cursor_pos );
    mvwprintz( wbar, point( 1, getmaxy( wbar ) - 1 ), c_red, coords );
    wnoutrefresh( wbar );
}

#if defined(TILES)
tiles_redraw_info redraw_info;
#endif

static void draw( overmap_draw_data_t &data )
{
    cata_assert( static_cast<bool>( data.ui ) );
    ui_adaptor *ui = data.ui.get();
    draw_om_sidebar( *ui, g->w_omlegend, data.ictxt, data );
#if defined( TILES )
    if( use_tiles && use_tiles_overmap ) {
        redraw_info = tiles_redraw_info { data.cursor_pos, uistate.overmap_show_overlays };
        werase( g->w_overmap );
        // trigger the actual redraw code in sdltiles.cpp
        wnoutrefresh( g->w_overmap );
        return;
    }
#endif // TILES
    draw_ascii( g->w_overmap, data );
}

static bool create_note( const tripoint_abs_omt &curs, std::optional<std::string> context )
{
    const std::string old_note = overmap_buffer.note( curs );
    if( !context ) {
        if( old_note.empty() ) {
            context = _( "Add a note to the map." );
        } else {
            context = _( "Edit an existing note." );
        }
    }
    std::string color_notes = string_format( "%s\n%s\n\n",
                              *context,
                              _( "For a custom GLYPH or COLOR follow the examples below.  "
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
                                              point::zero );
        w_preview_map = catacurses::newwin( npm_height + 2, npm_width + 2,
                                            point( 0, 2 ) );
        preview_windows = std::make_tuple( &w_preview, &w_preview_title, &w_preview_map );

        ui.position( point::zero, point( max_note_display_length + 1, npm_height + 4 ) );
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
            return true;
        }
    } else if( !esc_pressed && old_note != new_note ) {
        overmap_buffer.add_note( curs, new_note );
        return true;
    }
    return false;
}

// if false, search yielded no results
static bool search( const ui_adaptor &om_ui, tripoint_abs_omt &curs, const tripoint_abs_omt &orig )
{
    input_context ctxt( "STRING_INPUT" );
    std::vector<std::string> act_descs;
    const auto add_action_desc = [&]( const std::string & act, const std::string & txt ) {
        act_descs.emplace_back( ctxt.get_desc( act, txt, input_context::allow_all_keys ) );
    };
    add_action_desc( "HISTORY_UP", pgettext( "string input", "History" ) );
    add_action_desc( "TEXT.CLEAR", pgettext( "string input", "Clear text" ) );
    add_action_desc( "TEXT.QUIT", pgettext( "string input", "Abort" ) );
    add_action_desc( "TEXT.CONFIRM", pgettext( "string input", "Save" ) );
    std::string term = string_input_popup()
                       .title( _( "Search term:" ) )
                       .description( string_format( "%s\n%s",
                                     _( "Multiple entries separated with comma (,). Excludes starting with hyphen (-)." ),
                                     colorize( enumerate_as_string( act_descs, enumeration_conjunction::none ), c_green ) ) )
                       .desc_color( c_white )
                       .identifier( "overmap_search" )
                       .query_string();
    if( term.empty() ) {
        return false;
    }

    std::vector<point_abs_omt> locations;
    std::vector<point_abs_om> overmap_checked;

    const int radius = OMAPX * 5; // arbitrary
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

            om_vision_level vision = om_loc.om->seen( om_relative );
            if( vision != om_vision_level::unseen &&
                match_include_exclude( om_loc.om->ter( om_relative )->get_name( vision ), term ) ) {
                locations.push_back( project_combine( om_loc.om->pos(), om_relative.xy() ) );
            }
        }
    }

    if( locations.empty() ) {
        sfx::play_variant_sound( "menu_error", "default", 100 );
        popup( _( "No results found within %d tiles." ), radius );
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
            const std::string entry_text =
                string_format(
                    _( "sym: [ %s %s ], color: [ %s %s], name: [ %s ], id: [ %s ]" ),
                    colorize( oter.get_symbol( om_vision_level::full ),
                              oter.get_color( om_vision_level::full ) ),
                    colorize( oter.get_symbol( om_vision_level::full, true ),
                              oter.get_color( om_vision_level::full, true ) ),
                    colorize( string_from_color( oter.get_color( om_vision_level::full ) ),
                              oter.get_color( om_vision_level::full ) ),
                    colorize( string_from_color( oter.get_color( om_vision_level::full, true ) ),
                              oter.get_color( om_vision_level::full, true ) ),
                    colorize( oter.get_name( om_vision_level::full ),
                              oter.get_color( om_vision_level::full ) ),
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
            w_editor = catacurses::newwin( 15, OVERMAP_LEGEND_WIDTH - 1,
                                           point( TERMX - OVERMAP_LEGEND_WIDTH + 1, 10 ) );

            ui.position_from_window( w_editor );
        } );
        ui.mark_resize();

        input_context ctxt( "OVERMAP_EDITOR" );
        ctxt.register_directions();
        ctxt.register_action( "SELECT" );
        ctxt.register_action( "LEVEL_UP" );
        ctxt.register_action( "LEVEL_DOWN" );
        ctxt.register_action( "zoom_in" );
        ctxt.register_action( "zoom_out" );
        ctxt.register_action( "CONFIRM" );
        ctxt.register_action( "CONFIRM_MULTIPLE" );
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
                mvwprintz( w_editor, point::south_east, c_white, _( "Place overmap special:" ) );
                mvwprintz( w_editor, point( 1, 2 ), c_light_blue, "                         " );
                mvwprintz( w_editor, point( 1, 2 ), c_light_blue, uistate.place_special->id.c_str() );
            }
            const std::string &rotation = om_direction::name( uistate.omedit_rotation );

            mvwprintz( w_editor, point( 1, 3 ), c_light_gray, "                         " );
            mvwprintz( w_editor, point( 1, 3 ), c_light_gray, _( "Rotation: %s %s" ), rotation,
                       can_rotate ? "" : _( "(fixed)" ) );
            fold_and_print( w_editor, point( 1, 5 ), getmaxx( w_editor ) - 2, c_red,
                            _( "Highlighted regions already have map content generated.  Their overmap id will change, but not their contents." ) );
            if( ( terrain && uistate.place_terrain->is_rotatable() ) ||
                ( !terrain && uistate.place_special->is_rotatable() ) ) {
                mvwprintz( w_editor, point( 1, 10 ), c_white, _( "[%s] Rotate" ),
                           ctxt.get_desc( "ROTATE" ) );
            }
            mvwprintz( w_editor, point( 1, 11 ), c_white, _( "[%s] Place" ),
                       ctxt.get_desc( "CONFIRM_MULTIPLE" ) );
            mvwprintz( w_editor, point( 1, 12 ), c_white, _( "[%s] Place and close" ),
                       ctxt.get_desc( "CONFIRM" ) );
            mvwprintz( w_editor, point( 1, 13 ), c_white, _( "[ESCAPE/Q] Cancel" ) );
            wnoutrefresh( w_editor );
        } );

        std::string action;
        do {
            om_ui.invalidate_ui();
            ui_manager::redraw();

            action = ctxt.handle_input( get_option<int>( "BLINK_SPEED" ) );

            if( const std::optional<tripoint_rel_omt> vec = ctxt.get_direction_rel_omt( action ) ) {
                curs += *vec;
            } else if( action == "LEVEL_DOWN" && curs.z() > -OVERMAP_DEPTH ) {
                curs.z()--;
            } else if( action == "LEVEL_UP" && curs.z() < OVERMAP_HEIGHT ) {
                curs.z()++;
            } else if( action == "SELECT" ) {
                if( std::optional<tripoint_rel_omt> mouse_pos = ctxt.get_coordinates_rel_omt( g->w_overmap,
                        point::zero, true ); mouse_pos ) {
                    curs = curs + mouse_pos->xy();
                }
            } else if( action == "zoom_out" ) {
                g->zoom_out_overmap();
                om_ui.mark_resize();
            } else if( action == "zoom_in" ) {
                g->zoom_in_overmap();
                om_ui.mark_resize();
            } else if( action == "CONFIRM" || action == "CONFIRM_MULTIPLE" ) { // Actually modify the overmap
                if( terrain ) {
                    overmap_buffer.ter_set( curs, uistate.place_terrain->id.id() );
                    overmap_buffer.set_seen( curs, om_vision_level::full );
                } else {
                    if( std::optional<std::vector<tripoint_abs_omt>> used_points =
                            overmap_buffer.place_special( *uistate.place_special, curs,
                                                          uistate.omedit_rotation, false, true ) ) {
                        for( const tripoint_abs_omt &pos : *used_points ) {
                            overmap_buffer.set_seen( pos, om_vision_level::full );
                        }
                    }
                }
            } else if( action == "ROTATE" && can_rotate ) {
                uistate.omedit_rotation = om_direction::turn_right( uistate.omedit_rotation );
                if( terrain ) {
                    uistate.place_terrain = &uistate.place_terrain->get_rotated( uistate.omedit_rotation ).obj();
                }
            }
            if( uistate.overmap_blinking ) {
                uistate.overmap_show_overlays = !uistate.overmap_show_overlays;
            }
        } while( action != "CONFIRM" && action != "QUIT" );

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

static void modify_horde_func( tripoint_abs_omt &curs )
{
    overmap &map_at_cursor = overmap_buffer.get( project_to<coords::om>( curs ).xy() );
    std::vector<std::reference_wrapper<mongroup>> hordes =
                map_at_cursor.debug_unsafe_get_groups_at( curs );
    if( hordes.empty() ) {
        if( !query_yn( _( "No hordes there.  Would you like to make a new horde?" ) ) ) {
            return;
        } else {
            debug_menu::wishmonstergroup( curs );
            return;
        }
    }
    uilist horde_list;
    int entry_num = 0;
    for( auto &horde_wrapper : hordes ) {
        mongroup &mg = horde_wrapper.get();
        // We do some special handling here to provide the information in as simple a way as possible
        // emulates horde behavior
        int displayed_monster_num = mg.monsters.empty() ? mg.population : mg.monsters.size();
        std::string horde_description = string_format( _( "group(type: %s) with %d monsters" ),
                                        mg.type.c_str(), displayed_monster_num );
        horde_list.addentry( entry_num, true, -1, horde_description );
    }
    horde_list.query();
    int &selected_group = horde_list.ret;
    if( selected_group < 0 || static_cast<size_t>( selected_group ) > hordes.size() ) {
        return;
    }
    mongroup &chosen_group = hordes[selected_group];

    uilist smenu;
    smenu.addentry( 0, true, 'I', _( "Set horde's interest (in %%)" ) );
    smenu.addentry( 1, true, 'D', _( "Set horde's destination" ) );
    smenu.addentry( 2, true, 'P', _( "Modify horde's population" ) );
    smenu.addentry( 3, true, 'M', _( "Add a new monster to the horde" ) );
    smenu.addentry( 4, true, 'B', _( "Set horde behavior" ) );
    smenu.addentry( 5, true, 'T', _( "Set horde's boolean values" ) );
    smenu.addentry( 6, true, 'A', _( "Add another horde to this location" ) );
    smenu.query();
    int new_value = 0;
    tripoint_abs_omt horde_destination = tripoint_abs_omt::zero;
    switch( smenu.ret ) {
        case 0:
            new_value = chosen_group.interest;
            if( query_int( new_value, false, _( "Set interest to what value?  Currently %d" ),
                           chosen_group.interest ) ) {
                chosen_group.set_interest( new_value );
            }
            break;
        case 1:
            horde_destination = ui::omap::choose_point( _( "Select a target destination for the horde." ),
                                true );
            if( horde_destination.is_invalid() || horde_destination == tripoint_abs_omt::zero ) {
                break;
            }
            chosen_group.target = project_to<coords::sm>( horde_destination ).xy();
            break;
        case 2:
            new_value = chosen_group.population;
            if( query_int( new_value, false, _( "Set population to what value?  Currently %d" ),
                           chosen_group.population ) ) {
                chosen_group.population = new_value;
            }
            break;
        case 3:
            debug_menu::wishmonstergroup_mon_selection( chosen_group );
            break;
        case 4:
            new_value = static_cast<int>( chosen_group.behaviour );
            // Screw it we hardcode a popup, if you really want to use this you're welcome to improve it
            popup( _( "Set behavior to which enum value?  Currently %d.  \nAccepted values:\n0 = none,\n1 = city,\n2=roam,\n3=nemesis" ),
                   static_cast<int>( chosen_group.behaviour ) );
            query_int( new_value, false, "" );
            chosen_group.behaviour = static_cast<mongroup::horde_behaviour>( new_value );
            break;
        case 5:
            // One day we'll be able to simply convert booleans to strings...
            chosen_group.horde = query_yn(
                                     _( "Set group's \"horde\" value to true?  (Select no to set to false)  \nCurrently %s" ),
                                     chosen_group.horde ? _( "true" ) : _( "false" ) );
            chosen_group.dying = query_yn(
                                     _( "Set group's \"dying\" value to true?  (Select no to set to false)  \nCurrently %s" ),
                                     chosen_group.dying ? _( "true" ) : _( "false" ) );
            break;
        case 6:
            debug_menu::wishmonstergroup( curs );
            break;
        default:
            break;
    }
}

static std::vector<tripoint_abs_omt> get_overmap_path_to( const tripoint_abs_omt &dest,
        bool driving )
{
    if( overmap_buffer.seen( dest ) == om_vision_level::unseen ) {
        return {};
    }
    const Character &player_character = get_player_character();
    map &here = get_map();
    const tripoint_abs_omt player_omt_pos = player_character.pos_abs_omt();
    overmap_path_params params;
    vehicle *player_veh = nullptr;
    if( driving ) {
        const optional_vpart_position vp = here.veh_at( player_character.pos_bub() );
        if( !vp.has_value() ) {
            debugmsg( "Failed to find driven vehicle" );
            return {};
        }
        player_veh = &vp->vehicle();
        // for now we can only handle flyers if already in the air
        const bool can_fly = player_veh->is_rotorcraft( here ) && player_veh->is_flying_in_air();
        const bool can_float = player_veh->can_float( here );
        const bool can_drive = player_veh->valid_wheel_config( here );
        // TODO: check engines/fuel
        if( can_fly ) {
            params = overmap_path_params::for_aircraft();
        } else if( can_float && !can_drive ) {
            params = overmap_path_params::for_watercraft();
        } else if( can_drive ) {
            const float offroad_coeff = player_veh->k_traction( here, player_veh->wheel_area() *
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
        if( here.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, player_character.pos_bub() ) ||
            is_water_body( dest_ter ) ) {
            params.set_cost( oter_travel_cost_type::water, 100 );
        }
    }
    // literal "edge" case: the vehicle may be in a different OMT than the player
    const tripoint_abs_omt start_omt_pos = driving ? player_veh->pos_abs_omt() : player_omt_pos;
    if( dest == player_omt_pos || dest == start_omt_pos ) {
        return {};
    } else {
        return overmap_buffer.get_travel_path( start_omt_pos, dest, params ).points;
    }
}

static int overmap_zoom_level = DEFAULT_TILESET_ZOOM;

static bool try_travel_to_destination( avatar &player_character, const tripoint_abs_omt curs,
                                       const tripoint_abs_omt dest, const bool driving )
{
    std::vector<tripoint_abs_omt> path = get_overmap_path_to( dest, driving );

    if( path.empty() ) {
        std::string popupmsg;
        if( dest.z() == player_character.posz() ) {
            popupmsg = _( "Unable to find a path from the current location:" );
        } else {
            popupmsg = _( "Auto travel requires source and destination on same Z level:" );
        }
        string_input_popup pop;
        const std::string ok = _( "OK" );
        pop
        .title( popupmsg )
        .width( ok.length() )
        .text( ok )
        .only_digits( false )
        .query();
        return false;
    }

    bool dest_is_curs = curs == dest;
    bool path_changed = false;
    if( path.front() == player_character.omt_path.front() && path != player_character.omt_path ) {
        // the player is trying to go to their existing destination but the path has changed
        path_changed = true;
        player_character.omt_path.swap( path );
        ui_manager::redraw();
    }
    std::string confirm_msg;
    if( !driving && player_character.weight_carried() > player_character.weight_capacity() ) {
        confirm_msg = _( "You are overburdened, are you sure you want to travel (it may be painful)?" );
    } else if( !driving && player_character.in_vehicle ) {
        confirm_msg = _( "You are in a vehicle but not driving.  Are you sure you want to walk?" );
    } else if( driving ) {
        if( dest_is_curs ) {
            confirm_msg = _( "Drive to this point?" );
        } else {
            confirm_msg = _( "Drive to your destination?" );
        }
    } else {
        if( dest_is_curs ) {
            confirm_msg = _( "Travel to this point?" );
        } else {
            confirm_msg = _( "Travel to your destination?" );
        }
    }
    if( query_yn( confirm_msg ) ) {
        if( !path_changed ) {
            player_character.omt_path.swap( path );
        }
        if( driving ) {
            player_character.assign_activity( autodrive_activity_actor() );
        } else {
            player_character.reset_move_mode();
            player_character.assign_activity( ACT_TRAVELLING );
        }
        return true;
    }
    if( path_changed ) {
        player_character.omt_path.swap( path );
    }
    return false;
}

static tripoint_abs_omt display()
{
    map &here = get_map();

    overmap_draw_data_t &data = g->overmap_data;
    tripoint_abs_omt &orig = data.origin_pos;
    std::vector<tripoint_abs_omt> &display_path = data.display_path;
    tripoint_abs_omt &select = data.select;
    input_context &ictxt = data.ictxt;

    const int previous_zoom = g->get_zoom();
    g->set_zoom( overmap_zoom_level );
    on_out_of_scope reset_zoom( [&]() {
        overmap_zoom_level = g->get_zoom();
        g->set_zoom( previous_zoom );
        g->mark_main_ui_adaptor_resize();
    } );

    background_pane bg_pane;

    data.ui = std::make_shared<ui_adaptor>();
    std::shared_ptr<ui_adaptor> ui = data.ui;

    ui->on_screen_resize( []( ui_adaptor & ui ) {
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
        g->w_overmap = catacurses::newwin( OVERMAP_WINDOW_HEIGHT, OVERMAP_WINDOW_WIDTH, point::zero );

        ui.position_from_window( catacurses::stdscr );
    } );
    ui->mark_resize();

    tripoint_abs_omt ret = tripoint_abs_omt::invalid;
    data.cursor_pos = data.origin_pos;
    tripoint_abs_omt &curs = data.cursor_pos;

    if( select != tripoint_abs_omt( -1, -1, -1 ) ) {
        curs = select;
    }
    // Configure input context for navigating the map.
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
    ictxt.register_action( "look" );
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
    ictxt.register_action( "TOGGLE_FAST_TRAVEL" );
    ictxt.register_action( "MISSIONS" );

    if( data.debug_editor ) {
        ictxt.register_action( "PLACE_TERRAIN" );
        ictxt.register_action( "PLACE_SPECIAL" );
        ictxt.register_action( "SET_SPECIAL_ARGS" );
        ictxt.register_action( "LONG_TELEPORT" );
        ictxt.register_action( "MODIFY_HORDE" );
        ictxt.register_action( "REVEAL_MAP" );
    }
    ictxt.register_action( "QUIT" );
    std::string action;
    data.show_explored = true;
    int fast_scroll_offset = get_option<int>( "FAST_SCROLL_OFFSET" );
    std::optional<tripoint_rel_omt> mouse_pos;
    std::chrono::time_point<std::chrono::steady_clock> last_blink = std::chrono::steady_clock::now();
    std::chrono::time_point<std::chrono::steady_clock> last_advance = std::chrono::steady_clock::now();
    auto display_path_iter = display_path.rbegin();
    std::chrono::milliseconds cursor_advance_time = std::chrono::milliseconds( 0 );
    bool keep_overmap_ui = false;

    ui->on_redraw( [&]( ui_adaptor & ui ) {
        ( void )ui;
        draw( g->overmap_data );
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
        if( !display_path.empty() ) {
            std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
            // We go faster per-tile the more we have to go
            cursor_advance_time = std::chrono::milliseconds( 1000 ) / display_path.size();
            cursor_advance_time = std::max( cursor_advance_time, std::chrono::milliseconds( 1 ) );
            if( now > last_advance + cursor_advance_time ) {
                if( display_path_iter != display_path.rend() ) {
                    curs = *display_path_iter;
                    last_advance = now;
                    display_path_iter++;
                } else if( now > last_advance + cursor_advance_time * 10 ) {
                    action = "QUIT";
                    break;
                }
            }
        }
        if( const std::optional<tripoint_rel_omt> vec = ictxt.get_direction_rel_omt( action ) ) {
            int scroll_d = uistate.overmap_fast_scroll ? fast_scroll_offset : 1;

            curs += vec->xy().raw() *
                    scroll_d; // TODO: Make += etc. available with corresponding relative coordinates.
        } else if( action == "MOUSE_MOVE" || action == "TIMEOUT" ) {
            tripoint_rel_omt edge_scroll = g->mouse_edge_scrolling_overmap( ictxt );
            if( edge_scroll != tripoint_rel_omt::zero ) {
                if( action == "MOUSE_MOVE" ) {
                    edge_scroll.raw() *= 2; // TODO: Make *= etc. available to relative coordinates
                }
                curs += edge_scroll;
            }
        } else if( action == "SELECT" &&
                   ( mouse_pos = ictxt.get_coordinates_rel_omt( g->w_overmap, point::zero, true ) ) ) {
            curs += mouse_pos->xy().raw();
        } else if( action == "look" ) {
            tripoint_abs_ms pos = project_combine( curs, g->overmap_data.origin_remainder );
            tripoint_bub_ms pos_rel = here.get_bub( pos );
            uistate.open_menu = [pos_rel]() {
                tripoint_bub_ms pos_cpy = pos_rel;
                g->look_around( true, pos_cpy, pos_rel, false, false, false, false, pos_rel );
            };
            action = "QUIT";
        } else if( action == "CENTER" ) {
            curs = orig;
        } else if( action == "LEVEL_DOWN" && curs.z() > -OVERMAP_DEPTH ) {
            curs.z() -= 1;
        } else if( action == "LEVEL_UP" && curs.z() < OVERMAP_HEIGHT ) {
            curs.z() += 1;
        } else if( action == "zoom_out" ) {
            g->zoom_out_overmap();
            ui->mark_resize();
        } else  if( action == "zoom_in" ) {
            g->zoom_in_overmap();
            ui->mark_resize();
        } else if( action == "CONFIRM" ) {
            ret = curs;
        } else if( action == "QUIT" ) {
            ret = tripoint_abs_omt::invalid;
        } else if( action == "CREATE_NOTE" ) {
            create_note( curs );
        } else if( action == "DELETE_NOTE" ) {
            if( overmap_buffer.has_note( curs ) && query_yn( _( "Really delete note?" ) ) ) {
                overmap_buffer.delete_note( curs );
            }
        } else if( action == "MARK_DANGER" ) {
            const int danger_radius = overmap_buffer.note_danger_radius( curs );
            if( danger_radius >= 0 &&
                query_yn( _( "Remove dangerous mark?" ) ) ) {
                overmap_buffer.mark_note_dangerous( curs, 0, false );
            } else {
                bool has_note = overmap_buffer.has_note( curs );
                if( !has_note ) {
                    has_note = create_note( curs, _( "Create a danger note." ) );
                }
                if( has_note ) {
                    const int max_amount = 20;
                    int amount = clamp( danger_radius, 0, max_amount );
                    // NOLINTNEXTLINE(cata-text-style): No need for two whitespaces
                    if( query_int( amount, true, _( "Danger radius in overmap squares? (0-%d)" ),
                                   max_amount ) && amount >= 0 && amount <= max_amount ) {
                        overmap_buffer.mark_note_dangerous( curs, amount, true );
                    }
                }
            }
        } else if( action == "LIST_NOTES" ) {
            const point_abs_omt p = draw_notes( curs );
            if( !p.is_invalid() ) {
                curs.x() = p.x();
                curs.y() = p.y();
            }
        } else if( action == "GO_TO_DESTINATION" ) {
            avatar &player_character = get_avatar();
            if( !player_character.omt_path.empty() ) {
                const bool driving = player_character.in_vehicle && player_character.controlling_vehicle;
                if( try_travel_to_destination( player_character, curs, player_character.omt_path.front(),
                                               driving ) ) {
                    action = "QUIT";
                    if( uistate.overmap_fast_travel ) {
                        keep_overmap_ui = true;
                    }
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
                if( try_travel_to_destination( player_character, curs, curs, driving ) ) {
                    action = "QUIT";
                    if( uistate.overmap_fast_travel ) {
                        keep_overmap_ui = true;
                    }
                }
            }
        } else if( action == "TOGGLE_BLINKING" ) {
            uistate.overmap_blinking = !uistate.overmap_blinking;
            // if we turn off overmap blinking, show overlays and explored status
            if( !uistate.overmap_blinking ) {
                uistate.overmap_show_overlays = true;
            } else {
                data.show_explored = true;
            }
        } else if( action == "TOGGLE_OVERLAYS" ) {
            // if we are currently blinking, turn blinking off.
            if( uistate.overmap_blinking ) {
                uistate.overmap_blinking = false;
                uistate.overmap_show_overlays = false;
                data.show_explored = false;
            } else {
                uistate.overmap_show_overlays = !uistate.overmap_show_overlays;
                data.show_explored = !data.show_explored;
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
            if( here.is_outside( get_player_character().pos_bub() ) ) {
                uistate.overmap_visible_weather = !uistate.overmap_visible_weather;
            }
        } else if( action == "TOGGLE_FAST_SCROLL" ) {
            uistate.overmap_fast_scroll = !uistate.overmap_fast_scroll;
        } else if( action == "TOGGLE_FOREST_TRAILS" ) {
            uistate.overmap_show_forest_trails = !uistate.overmap_show_forest_trails;
        } else if( action == "TOGGLE_FAST_TRAVEL" ) {
            uistate.overmap_fast_travel = !uistate.overmap_fast_travel;
        } else if( action == "SEARCH" ) {
            if( !search( *ui, curs, orig ) ) {
                continue;
            }
        } else if( action == "PLACE_TERRAIN" || action == "PLACE_SPECIAL" ) {
            place_ter_or_special( *ui, curs, action );
        } else if( action == "SET_SPECIAL_ARGS" ) {
            set_special_args( curs );
        } else if( action == "LONG_TELEPORT" && !curs.is_invalid() ) {
            g->place_player_overmap( curs );
            add_msg( _( "You teleport to submap %s." ), curs.to_string() );
            action = "QUIT";
        } else if( action == "MODIFY_HORDE" ) {
            modify_horde_func( curs );
        } else if( action == "REVEAL_MAP" ) {
            debug_menu::prompt_map_reveal( curs );
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
    if( !keep_overmap_ui ) {
        ui::omap::force_quit();
    } else {
        data.fast_traveling = true;
    }
    if( overmap_buffer.distance_limit( g->overmap_data.distance, g->overmap_data.origin_pos, ret ) ) {
        return ret;
    } else {
        return tripoint_abs_omt::invalid;
    }
}

} // namespace overmap_ui

struct blended_omt {
    oter_id id;
    std::string sym;
    nc_color color;
    std::string name;
};

oter_vision::blended_omt oter_vision::get_blended_omt_info( const tripoint_abs_omt &omp,
        om_vision_level vision )
{
    std::vector<std::pair<oter_id, om_vision_level>> neighbors;
    const auto add_to_neighbors = [&vision, &neighbors, &omp]( const tripoint_abs_omt & next ) {
        if( next == omp ) {
            return;
        }
        om_vision_level local_vision = overmap_buffer.seen( next );
        if( local_vision == om_vision_level::unseen ) {
            return;
        }
        oter_id ter = overmap_buffer.ter( next );
        // If the target tile is next to a tile that blends adjacent at the vision level the target tile is at,
        // but not at the vision level it is at, the SDL overmap will be fed the id for the vision level of the
        // tile at the vision level of the target tile, resulting in it drawing the placeholder symbol and color
        // for the adjacent-blending tile.
        if( ter->blends_adjacent( vision ) ) {
            return;
        }
        neighbors.emplace_back( ter, vision );
    };
    for( const tripoint_abs_omt &next : tripoint_range<tripoint_abs_omt>( omp + point::north_west,
            omp + point::south_east ) ) {
        add_to_neighbors( next );
    }
    // if nothing's immediately adjacent, reach out further
    if( neighbors.empty() ) {
        for( const tripoint_abs_omt &next : tripoint_range<tripoint_abs_omt>( omp + point( -2, -2 ),
                omp + point( 2, 2 ) ) ) {
            add_to_neighbors( next );
        }
    }
    // Okay, it didn't work, we can't see this tile
    if( neighbors.empty() ) {
        oter_vision::blended_omt ret;
        ret.id = oter_unexplored;
        ret.sym = oter_unexplored->get_symbol( om_vision_level::full );
        ret.color = oter_unexplored->get_color( om_vision_level::full );
        ret.name = oter_unexplored->get_name( om_vision_level::full );
        return ret;
    }
    std::vector<std::pair<size_t, int>> counts;
    for( size_t i = 0; i < neighbors.size(); ++i ) {
        const auto refers_to_same = [i, &neighbors]( const std::pair<size_t, int> &entry ) {
            return neighbors[entry.first] == neighbors[i];
        };
        auto it = std::find_if( counts.begin(), counts.end(), refers_to_same );
        if( it == counts.end() ) {
            counts.emplace_back( i, 1 );
            continue;
        }
        it->second += 1;
    }
    const auto sort_counts = [&neighbors]( const std::pair<size_t, int> &l,
    const std::pair<size_t, int> &r ) {
        // Put in descending order
        if( l.second != r.second ) {
            return l.second > r.second;
        }
        // And do something stable so it doesn't shift if they're all equal
        const std::pair<oter_id, om_vision_level> left = neighbors[l.first];
        const std::pair<oter_id, om_vision_level> right = neighbors[r.first];
        return localized_compare( left.first->get_name( left.second ),
                                  right.first->get_name( right.second ) );
    };
    std::sort( counts.begin(), counts.end(), sort_counts );
    oter_vision::blended_omt ret;
    size_t idx = counts[0].first;
    ret.id = neighbors[idx].first;
    ret.sym = ret.id->get_symbol( neighbors[idx].second );
    ret.color = ret.id->get_color( neighbors[idx].second );
    ret.name = ret.id->get_name( neighbors[idx].second );
    return ret;
}

std::pair<std::string, nc_color> oter_display_lru::get_symbol_and_color( const oter_id &cur_ter,
        om_vision_level vision )
{
    std::pair<std::string, nc_color> ret = {"?", c_red};
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
        ret.second = info->get_color( vision, uistate.overmap_show_land_use_codes );
        ret.first = info->get_symbol( vision, uistate.overmap_show_land_use_codes );
    }
    return ret;
}

std::pair<std::string, nc_color> oter_symbol_and_color( const tripoint_abs_omt &omp,
        oter_display_args &args, const oter_display_options &opts, oter_display_lru *lru )
{
    std::pair<std::string, nc_color> ret;

    oter_id cur_ter = oter_str_id::NULL_ID();
    avatar &player_character = get_avatar();
    std::vector<point_abs_omt> plist;
    const bool blink = opts.blink || g->overmap_data.fast_traveling;

    if( blink && !opts.mission_inbounds && opts.mission_target ) {
        plist = line_to( opts.center.xy(), opts.mission_target->xy() );
    }

    // Only load terrain if we can see it
    if( args.vision != om_vision_level::unseen ) {
        cur_ter = overmap_buffer.ter( omp );
    }

    if( blink && opts.show_pc && !opts.hilite_pc && omp == get_avatar().pos_abs_omt() ) {
        // Display player pos, should always be visible
        ret.second = player_character.symbol_color();
        ret.first = "@";
    } else if( opts.show_weather && ( uistate.overmap_debug_weather ||
                                      overmap_ui::get_and_assign_los( args.los_sky, player_character, omp, opts.sight_points * 2 ) ) ) {
        const weather_type_id type = overmap_ui::get_weather_at_point( omp );
        ret.second = type->map_color;
        ret.first = type->get_symbol();
    } else if( opts.debug_scent && overmap_ui::get_scent_glyph( omp, ret.second, ret.first ) ) {
        // get_scent_glyph has changed ret.second and ret.first if omp has a scent
    } else if( blink &&
               overmap_buffer.distance_limit_line( g->overmap_data.distance, g->overmap_data.origin_pos, omp ) ) {
        ret.second = c_light_red;
        ret.first = "X";
    } else if( blink && overmap_buffer.is_marked_dangerous( omp ) ) {
        ret.second = c_red;
        ret.first = "X";
    } else if( blink && opts.mission_inbounds && opts.mission_target && !opts.hilite_mission &&
               omp.xy() == opts.mission_target->xy() ) {
        // Mission target, display always, player should know where it is anyway.
        ret.second = c_red;
        ret.first = "*";
        if( opts.mission_target->z() > opts.center.z() ) {
            ret.first = "^";
        } else if( opts.mission_target->z() < opts.center.z() ) {
            ret.first = "v";
        }
    } else if( !opts.mission_inbounds && !opts.drawn_mission && args.edge_tile &&
               std::find( plist.begin(), plist.end(), omp.xy() ) != plist.end() ) {
        ret.first = "*";
        ret.second = c_red;
        opts.drawn_mission = true;
    } else if( blink && uistate.overmap_show_map_notes && overmap_buffer.has_note( omp ) ) {
        // Display notes in all situations, even when not seen
        std::tie( ret.first, ret.second,
                  std::ignore ) = overmap_ui::get_note_display_info( overmap_buffer.note( omp ) );
    } else if( args.vision == om_vision_level::unseen ) {
        // All cases above ignore the seen-status,
        ret.second = oter_unexplored.obj().get_color( om_vision_level::full );
        ret.first = oter_unexplored.obj().get_symbol( om_vision_level::full );
        // All cases below assume that see is true.
    } else if( blink && opts.npc_color.count( omp ) != 0 ) {
        // Visible NPCs are cached already
        ret.second = opts.npc_color.at( omp ).color;
        ret.first = "@";
    } else if( blink && opts.player_path_route.find( omp.xy() ) != opts.player_path_route.end() ) {
        // player path
        ret.second = c_blue;
        const int player_path_z = opts.player_path_route.at( omp.xy() );
        if( player_path_z == omp.z() ) {
            ret.first = "!";
        } else if( player_path_z > omp.z() ) {
            ret.first = "^";
        } else {
            ret.first = "v";
        }
    } else if( blink && opts.npc_path_route.find( omp ) != opts.npc_path_route.end() ) {
        // npc path
        ret.second = c_red;
        ret.first = "!";
    } else if( blink && opts.show_map_revealed &&
               player_character.map_revealed_omts.find( omp ) != player_character.map_revealed_omts.end() ) {
        // Revealed map tiles
        ret.second = c_magenta;
        ret.first = "&";
    } else if( blink && opts.showhordes &&
               overmap_buffer.get_horde_size( omp ) >= HORDE_VISIBILITY_SIZE &&
               args.vision > om_vision_level::details &&
               ( overmap_ui::get_and_assign_los( args.los, player_character, omp, opts.sight_points ) ||
                 uistate.overmap_debug_mongroup || player_character.has_trait( trait_DEBUG_CLAIRVOYANCE ) ) ) {
        // Display Hordes only when within player line-of-sight
        ret.second = c_green;
        ret.first = overmap_buffer.get_horde_size( omp ) > HORDE_VISIBILITY_SIZE * 2 ? "Z" : "z";
    } else if( blink && overmap_buffer.has_vehicle( omp ) ) {
        ret.second = c_cyan;
        ret.first = overmap_buffer.get_vehicle_ter_sym( omp );
    } else if( !opts.sZoneName.empty() && opts.tripointZone.xy() == omp.xy() ) {
        ret.second = c_yellow;
        ret.first = "Z";
    } else if( cur_ter->blends_adjacent( args.vision ) ) {
        oter_vision::blended_omt here = oter_vision::get_blended_omt_info( omp, args.vision );
        ret.first = here.sym;
        ret.second = here.color;
    } else if( !uistate.overmap_show_forest_trails && cur_ter &&
               ( cur_ter->get_type_id() == oter_type_forest_trail ) ) {
        // If forest trails shouldn't be displayed, and this is a forest trail, then
        // instead render it like a forest.
        ret = lru ? lru->get_symbol_and_color( oter_forest.id(),
        args.vision ) : std::pair<std::string, nc_color> {
            oter_forest->get_symbol( args.vision, uistate.overmap_show_land_use_codes ),
            oter_forest->get_color( args.vision, uistate.overmap_show_land_use_codes )
        };
        if( opts.show_explored && overmap_buffer.is_explored( omp ) ) {
            ret.second = c_dark_gray;
        }
    } else {
        // Nothing special, but is visible to the player.
        ret = lru ? lru->get_symbol_and_color( cur_ter, args.vision ) : std::pair<std::string, nc_color> {
            cur_ter->get_symbol( args.vision, uistate.overmap_show_land_use_codes ),
            cur_ter->get_color( args.vision, uistate.overmap_show_land_use_codes )
        };
        if( opts.show_explored && overmap_buffer.is_explored( omp ) ) {
            ret.second = c_dark_gray;
        }
    }

    if( opts.hilite_mission && opts.mission_target && opts.mission_target->xy() == omp.xy() ) {
        ret.second = red_background( ret.second );
    }
    if( opts.hilite_pc && opts.show_pc && opts.center.xy() == omp.xy() ) {
        ret.second = hilite( ret.second );
    }

    return ret;
}

void ui::omap::display()
{
    g->overmap_data = overmap_ui::overmap_draw_data_t(); //reset data
    g->overmap_data.origin_pos = get_player_character().pos_abs_omt();
    g->overmap_data.debug_editor = debug_mode; // always display debug editor if game is in debug mode
    overmap_ui::display();
}

void ui::omap::look_around_map( tripoint_abs_ms starting_pos )
{
    g->overmap_data = overmap_ui::overmap_draw_data_t(); //reset data
    std::tie( g->overmap_data.origin_pos,
              g->overmap_data.origin_remainder ) = project_remain<coords::omt>( starting_pos );
    overmap_ui::display();
}

void ui::omap::display_npc_path( tripoint_abs_omt starting_pos,
                                 const std::vector<tripoint_abs_omt> &display_path )
{
    g->overmap_data = overmap_ui::overmap_draw_data_t();
    g->overmap_data.origin_pos = starting_pos;
    g->overmap_data.display_path = display_path;
    overmap_ui::display();
}

void ui::omap::display_hordes()
{
    g->overmap_data = overmap_ui::overmap_draw_data_t();
    g->overmap_data.origin_pos = get_player_character().pos_abs_omt();
    uistate.overmap_debug_mongroup = true;
    overmap_ui::display();
    uistate.overmap_debug_mongroup = false;
}

void ui::omap::display_weather()
{
    g->overmap_data = overmap_ui::overmap_draw_data_t();
    tripoint_abs_omt pos = get_player_character().pos_abs_omt();
    pos.z() = 10;
    g->overmap_data.origin_pos = pos;
    uistate.overmap_debug_weather = true;
    overmap_ui::display();
    uistate.overmap_debug_weather = false;
}

void ui::omap::display_visible_weather()
{
    g->overmap_data = overmap_ui::overmap_draw_data_t();
    tripoint_abs_omt pos = get_player_character().pos_abs_omt();
    pos.z() = 10;
    g->overmap_data.origin_pos = pos;
    uistate.overmap_visible_weather = true;
    overmap_ui::display();
    uistate.overmap_visible_weather = false;
}

void ui::omap::display_scents()
{
    g->overmap_data = overmap_ui::overmap_draw_data_t();
    g->overmap_data.origin_pos = get_player_character().pos_abs_omt();
    g->overmap_data.debug_scent = true;
    overmap_ui::display();
}

void ui::omap::display_editor()
{
    g->overmap_data = overmap_ui::overmap_draw_data_t();
    g->overmap_data.origin_pos = get_player_character().pos_abs_omt();
    g->overmap_data.debug_editor = true;
    overmap_ui::display();
}

void ui::omap::display_zones( const tripoint_abs_omt &center, const tripoint_abs_omt &select,
                              const int iZoneIndex )
{
    g->overmap_data = overmap_ui::overmap_draw_data_t();
    g->overmap_data.origin_pos = center;
    g->overmap_data.select = select;
    g->overmap_data.iZoneIndex = iZoneIndex;
    overmap_ui::display();
}

tripoint_abs_omt ui::omap::choose_point( const std::string &message, bool show_debug_info,
        const int distance )
{
    return choose_point( message, get_player_character().pos_abs_omt(), show_debug_info,
                         distance );
}

tripoint_abs_omt ui::omap::choose_point( const std::string &message, const tripoint_abs_omt &origin,
        bool show_debug_info, const int distance )
{
    g->overmap_data = overmap_ui::overmap_draw_data_t();
    g->overmap_data.message = message;
    g->overmap_data.origin_pos = origin;
    g->overmap_data.debug_info = show_debug_info;
    g->overmap_data.distance = distance;
    return overmap_ui::display();
}

tripoint_abs_omt ui::omap::choose_point( const std::string &message, int z, bool show_debug_info,
        const int distance )
{
    tripoint_abs_omt pos = get_player_character().pos_abs_omt();
    pos.z() = z;
    return choose_point( message, pos, show_debug_info, distance );
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
            uilist_entry entry( c.database_id, true, -1, string_format( _( "%s (size: %d)" ), c.name, c.size ),
                                string_format(
                                    _( "Location: <color_white>%s</color>:<color_white>%s</color>" ),
                                    c.pos_om.to_string(), c.pos.to_string() ),
                                //~ "pop" refers to population count
                                string_format( _( "(pop <color_white>%s</color>)" ), c.population ) );
            cities_menu.entries.emplace_back( entry );
        }
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

void ui::omap::force_quit()
{
    overmap_ui::generated_omts.clear();
    g->overmap_data.ui.reset();
    g->overmap_data.fast_traveling = false;
}
