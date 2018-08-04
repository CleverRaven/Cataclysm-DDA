#include "overmap_ui.h"

#include "clzones.h"
#include "coordinate_conversions.h"
#include "cursesdef.h"
#include "game.h"
#include "input.h"
#include "line.h"
#include "mapbuffer.h"
#include "mongroup.h"
#include "npc.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "output.h"
#include "player.h"
#include "sounds.h"
#include "string_input_popup.h"
#include "ui.h"
#include "uistate.h"
#include "weather.h"
#include "weather_gen.h"

namespace
{

// drawing relevant data, e.g. what to draw.
struct draw_data_t {
    // draw monster groups on the overmap.
    bool debug_mongroup = false;
    // draw weather, e.g. clouds etc.
    bool debug_weather = false;
    // draw editor.
    bool debug_editor = false;
    // draw scent traces.
    bool debug_scent = false;
    // draw zone location.
    tripoint select = tripoint( -1, -1, -1 );
    int iZoneIndex = -1;
};

// {note symbol, note color, offset to text}
std::tuple<char, nc_color, size_t> get_note_display_info( std::string const &note )
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
        auto const end = note.find_first_of( " :;", pos, 3 );
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

bool get_weather_glyph( const tripoint &pos, nc_color &ter_color, long &ter_sym )
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
        auto const abs_ms_pos =  tripoint( pos.x * SEEX * 2, pos.y * SEEY * 2, pos.z );
        const auto &wgen = overmap_buffer.get_settings( pos.x, pos.y, pos.z ).weather;
        auto const weather = wgen.get_weather_conditions( abs_ms_pos, calendar::turn, g->get_seed() );
        iter = weather_cache.insert( std::make_pair( pos, weather ) ).first;
    }
    switch( iter->second ) {
        case WEATHER_SUNNY:
        case WEATHER_CLEAR:
        case WEATHER_NULL:
        case NUM_WEATHER_TYPES:
            // show the terrain as usual
            return false;
        case WEATHER_CLOUDY:
            ter_color = c_white;
            ter_sym = '8';
            break;
        case WEATHER_DRIZZLE:
        case WEATHER_FLURRIES:
            ter_color = c_light_blue;
            ter_sym = '8';
            break;
        case WEATHER_ACID_DRIZZLE:
            ter_color = c_light_green;
            ter_sym = '8';
            break;
        case WEATHER_RAINY:
        case WEATHER_SNOW:
            ter_color = c_blue;
            ter_sym = '8';
            break;
        case WEATHER_ACID_RAIN:
            ter_color = c_green;
            ter_sym = '8';
            break;
        case WEATHER_THUNDER:
        case WEATHER_LIGHTNING:
        case WEATHER_SNOWSTORM:
            ter_color = c_dark_gray;
            ter_sym = '8';
            break;
    }
    return true;
}

bool get_scent_glyph( const tripoint &pos, nc_color &ter_color, long &ter_sym )
{
    auto possible_scent = overmap_buffer.scent_at( pos );
    if( possible_scent.creation_time != calendar::before_time_starts ) {
        color_manager &color_list = get_all_colors();
        int i = 0;
        time_duration scent_age = calendar::turn - possible_scent.creation_time;
        while( i < num_colors && scent_age > 0 ) {
            i++;
            scent_age /= 10;
        }
        ter_color = color_list.get( ( color_id )i );
        int scent_strength = possible_scent.initial_strength;
        char c = '0';
        while( c <= '9' && scent_strength > 0 ) {
            c++;
            scent_strength /= 10;
        }
        ter_sym = c;
        return true;
    }
    // but it makes no scents!
    return false;
}

void draw_city_labels( const catacurses::window &w, const tripoint &center )
{
    const int win_x_max = getmaxx( w );
    const int win_y_max = getmaxy( w );
    const int sm_radius = std::max( win_x_max, win_y_max );

    const point screen_center_pos( win_x_max / 2, win_y_max / 2 );

    for( const auto &element : overmap_buffer.get_cities_near( omt_to_sm_copy( center ), sm_radius ) ) {
        const point city_pos( sm_to_omt_copy( element.abs_sm_pos.x, element.abs_sm_pos.y ) );
        const point screen_pos( city_pos - point( center.x, center.y ) + screen_center_pos );

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

        if( !overmap_buffer.seen( city_pos.x, city_pos.y, center.z ) ) {
            continue;   // haven't seen it.
        }

        mvwprintz( w, text_y, text_x_min, i_yellow, element.city->name );
    }
}

point draw_notes( int z )
{
    const overmapbuffer::t_notes_vector notes = overmap_buffer.get_all_notes( z );
    catacurses::window w_notes = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                 ( TERMY > FULL_SCREEN_HEIGHT ) ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0,
                                 ( TERMX > FULL_SCREEN_WIDTH ) ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0 );

    draw_border( w_notes );

    const std::string title = _( "Notes:" );
    const std::string back_msg = _( "< Prev notes" );
    const std::string forward_msg = _( "Next notes >" );

    // Number of items to show at one time, 2 rows for border, 2 for title & bottom text
    const unsigned maxitems = FULL_SCREEN_HEIGHT - 4;
    int ch = '.';
    unsigned start = 0;
    const int back_len = utf8_width( back_msg );
    bool redraw = true;
    point result( -1, -1 );

    mvwprintz( w_notes, 1, 1, c_light_gray, title.c_str() );
    do {
        if( redraw ) {
            for( int i = 2; i < FULL_SCREEN_HEIGHT - 1; i++ ) {
                for( int j = 1; j < FULL_SCREEN_WIDTH - 1; j++ ) {
                    mvwputch( w_notes, i, j, c_black, ' ' );
                }
            }
            for( unsigned i = 0; i < maxitems; i++ ) {
                const unsigned cur_it = start + i;
                if( cur_it >= notes.size() ) {
                    continue;
                }
                // Print letter ('a' <=> cur_it == start)
                mvwputch( w_notes, i + 2, 1, c_white, 'a' + i );
                mvwprintz( w_notes, i + 2, 3, c_light_gray, "- %s", notes[cur_it].second.c_str() );
            }
            if( start >= maxitems ) {
                mvwprintw( w_notes, maxitems + 2, 1, back_msg.c_str() );
            }
            if( start + maxitems < notes.size() ) {
                mvwprintw( w_notes, maxitems + 2, 2 + back_len, forward_msg.c_str() );
            }
            mvwprintz( w_notes, 1, 40, c_white, _( "Press letter to center on note" ) );
            mvwprintz( w_notes, FULL_SCREEN_HEIGHT - 2, 40, c_white, _( "Spacebar - Return to map  " ) );
            wrefresh( w_notes );
            redraw = false;
        }
        // @todo: use input context
        ch = inp_mngr.get_input_event().get_first_input();
        if( ch == '<' && start >= maxitems ) {
            start -= maxitems;
            redraw = true;
        } else if( ch == '>' && start + maxitems < notes.size() ) {
            start += maxitems;
            redraw = true;
        } else if( ch >= 'a' && ch <= 'z' ) {
            const unsigned chosen = ch - 'a' + start;
            if( chosen < notes.size() ) {
                result = notes[chosen].first;
                break; // -> return result
            }
        }
    } while( ch != ' ' && ch != '\n' && ch != KEY_ESCAPE );
    return result;
}

void draw( const catacurses::window &w, const catacurses::window &wbar, const tripoint &center,
           const tripoint &orig, bool blink, bool show_explored, input_context *inp_ctxt,
           const draw_data_t &data )
{
    const int z     = center.z;
    const int cursx = center.x;
    const int cursy = center.y;
    const int om_map_width  = OVERMAP_WINDOW_WIDTH;
    const int om_map_height = OVERMAP_WINDOW_HEIGHT;
    const int om_half_width = om_map_width / 2;
    const int om_half_height = om_map_height / 2;

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

    std::string sZoneName;
    tripoint tripointZone = tripoint( -1, -1, -1 );
    const auto &zones = zone_manager::get_manager();

    if( data.iZoneIndex != -1 ) {
        sZoneName = zones.zones[data.iZoneIndex].get_name();
        tripointZone = ms_to_omt_copy( zones.zones[data.iZoneIndex].get_center_point() );
    }

    // If we're debugging monster groups, find the monster group we've selected
    const mongroup *mgroup = nullptr;
    std::vector<mongroup *> mgroups;
    if( data.debug_mongroup ) {
        mgroups = overmap_buffer.monsters_at( center.x, center.y, center.z );
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

    int const offset_x = cursx - om_half_width;
    int const offset_y = cursy - om_half_height;

    // For use with place_special: cache the color and symbol of each submap
    // and record the bounds to optimize lookups below
    std::unordered_map<point, std::pair<long, nc_color>> special_cache;

    point s_begin, s_end = point( 0, 0 );
    if( blink && uistate.place_special ) {
        for( const auto &s_ter : uistate.place_special->terrains ) {
            if( s_ter.p.z == 0 ) {
                const point rp = om_direction::rotate( point( s_ter.p.x, s_ter.p.y ), uistate.omedit_rotation );
                const oter_id oter =  s_ter.terrain->get_rotated( uistate.omedit_rotation );

                special_cache.insert( std::make_pair(
                                          rp, std::make_pair( oter->get_sym(), oter->get_color() ) ) );

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
    std::unordered_map<tripoint, npc_coloring> npc_color;
    if( blink ) {
        const auto &npcs = overmap_buffer.get_npcs_near_player( sight_points );
        for( const auto &np : npcs ) {
            if( np->posz() != z ) {
                continue;
            }

            const tripoint pos = np->global_omt_location();
            if( has_debug_vision || overmap_buffer.seen( pos.x, pos.y, pos.z ) ) {
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
    }

    for( int i = 0; i < om_map_width; ++i ) {
        for( int j = 0; j < om_map_height; ++j ) {
            const int omx = i + offset_x;
            const int omy = j + offset_y;

            oter_id cur_ter = oter_str_id::NULL_ID();
            nc_color ter_color = c_black;
            long ter_sym = ' ';

            const bool see = has_debug_vision || overmap_buffer.seen( omx, omy, z );
            if( see ) {
                // Only load terrain if we can actually see it
                cur_ter = overmap_buffer.ter( omx, omy, z );
            }

            tripoint const cur_pos {omx, omy, z};
            // Check if location is within player line-of-sight
            const bool los = see && g->u.overmap_los( cur_pos, sight_points );

            if( blink && cur_pos == orig ) {
                // Display player pos, should always be visible
                ter_color = g->u.symbol_color();
                ter_sym   = '@';
            } else if( data.debug_weather &&
                       get_weather_glyph( tripoint( omx, omy, z ), ter_color, ter_sym ) ) {
                // ter_color and ter_sym have been set by get_weather_glyph
            } else if( data.debug_scent && get_scent_glyph( cur_pos, ter_color, ter_sym ) ) {
            } else if( blink && has_target && omx == target.x && omy == target.y ) {
                // Mission target, display always, player should know where it is anyway.
                ter_color = c_red;
                ter_sym   = '*';
                if( target.z > z ) {
                    ter_sym = '^';
                } else if( target.z < z ) {
                    ter_sym = 'v';
                }
            } else if( blink && overmap_buffer.has_note( cur_pos ) ) {
                // Display notes in all situations, even when not seen
                std::tie( ter_sym, ter_color, std::ignore ) =
                    get_note_display_info( overmap_buffer.note( cur_pos ) );
            } else if( !see ) {
                // All cases above ignore the seen-status,
                ter_color = c_dark_gray;
                ter_sym   = '#';
                // All cases below assume that see is true.
            } else if( blink && npc_color.count( cur_pos ) != 0 ) {
                // Visible NPCs are cached already
                ter_color = npc_color[ cur_pos ].color;
                ter_sym   = '@';
            } else if( blink && showhordes && los &&
                       overmap_buffer.get_horde_size( omx, omy, z ) >= HORDE_VISIBILITY_SIZE ) {
                // Display Hordes only when within player line-of-sight
                ter_color = c_green;
                ter_sym   = overmap_buffer.get_horde_size( omx, omy, z ) > HORDE_VISIBILITY_SIZE * 2 ? 'Z' : 'z';
            } else if( blink && overmap_buffer.has_vehicle( omx, omy, z ) ) {
                // Display Vehicles only when player can see the location
                ter_color = c_cyan;
                ter_sym   = 'c';
            } else if( !sZoneName.empty() && tripointZone.x == omx && tripointZone.y == omy ) {
                ter_color = c_yellow;
                ter_sym   = 'Z';
            } else {
                // Nothing special, but is visible to the player.
                // First see if we have the oter_t cached
                oter_t const *info = nullptr;
                for( auto const &c : cache ) {
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
                // Okay, we found something
                if( info ) {
                    // Map tile marked as explored
                    bool const explored = show_explored && overmap_buffer.is_explored( omx, omy, z );
                    ter_color = explored ? c_dark_gray : info->get_color();
                    ter_sym   = info->get_sym();
                }
            }

            // Are we debugging monster groups?
            if( blink && data.debug_mongroup ) {
                // Check if this tile is the target of the currently selected group

                // Convert to position within overmap
                int localx = omx * 2;
                int localy = omy * 2;
                sm_to_om_remain( localx, localy );

                if( mgroup && mgroup->target.x / 2 == localx / 2 && mgroup->target.y / 2 == localy / 2 ) {
                    ter_color = c_red;
                    ter_sym = 'x';
                } else {
                    const auto &groups = overmap_buffer.monsters_at( omx, omy, center.z );
                    for( auto &mgp : groups ) {
                        if( mgp->type == mongroup_id( "GROUP_FOREST" ) ) {
                            // Don't flood the map with forest creatures.
                            continue;
                        }
                        if( mgp->horde ) {
                            // Hordes show as +
                            ter_sym = '+';
                            break;
                        } else {
                            // Regular groups show as -
                            ter_sym = '-';
                        }
                    }
                    // Set the color only if we encountered an eligible group.
                    if( ter_sym == '+' || ter_sym == '-' ) {
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
                if( blink && uistate.place_terrain && omx == cursx && omy == cursy ) {
                    ter_color = uistate.place_terrain->get_color();
                    ter_sym = uistate.place_terrain->get_sym();
                } else if( blink && uistate.place_special ) {
                    if( omx - cursx >= s_begin.x && omx - cursx <= s_end.x &&
                        omy - cursy >= s_begin.y && omy - cursy <= s_end.y ) {
                        const point cache_point( omx - cursx, omy - cursy );
                        const auto sm = special_cache.find( cache_point );

                        if( sm != special_cache.end() ) {
                            ter_color = sm->second.second;
                            ter_sym = sm->second.first;
                        }
                    }
                }
                // Highlight areas that already have been generated
                if( MAPBUFFER.lookup_submap(
                        omt_to_sm_copy( tripoint( omx, omy, z ) ) ) ) {
                    ter_color = red_background( ter_color );
                }
            }

            if( omx == cursx && omy == cursy && !uistate.place_special ) {
                csee = see;
                ccur_ter = cur_ter;
                mvwputch_hi( w, j, i, ter_color, ter_sym );
            } else {
                mvwputch( w, j, i, ter_color, ter_sym );
            }
        }
    }

    if( z == 0 && uistate.overmap_show_city_labels ) {
        draw_city_labels( w, tripoint( cursx, cursy, z ) );
    }

    if( has_target && blink &&
        ( target.x < offset_x ||
          target.x >= offset_x + om_map_width ||
          target.y < offset_y ||
          target.y >= offset_y + om_map_height ) ) {
        int marker_x = std::max( 0, std::min( om_map_width  - 1, target.x - offset_x ) );
        int marker_y = std::max( 0, std::min( om_map_height - 1, target.y - offset_y ) );
        long marker_sym = ' ';

        switch( direction_from( cursx, cursy, target.x, target.y ) ) {
            case NORTH:
                marker_sym = '^';
                break;
            case NORTHEAST:
                marker_sym = LINE_OOXX;
                break;
            case EAST:
                marker_sym = '>';
                break;
            case SOUTHEAST:
                marker_sym = LINE_XOOX;
                break;
            case SOUTH:
                marker_sym = 'v';
                break;
            case SOUTHWEST:
                marker_sym = LINE_XXOO;
                break;
            case WEST:
                marker_sym = '<';
                break;
            case NORTHWEST:
                marker_sym = LINE_OXXO;
                break;
            default:
                break; //Do nothing
        }
        mvwputch( w, marker_y, marker_x, c_red, marker_sym );
    }

    std::vector<std::pair<nc_color, std::string>> corner_text;

    std::string const &note_text = overmap_buffer.note( cursx, cursy, z );
    if( !note_text.empty() ) {
        size_t const pos = std::get<2>( get_note_display_info( note_text ) );
        if( pos != std::string::npos ) {
            corner_text.emplace_back( c_yellow, note_text.substr( pos ) );
        }
    }

    for( const auto &npc : overmap_buffer.get_npcs_near_omt( cursx, cursy, z, 0 ) ) {
        if( !npc->marked_for_death ) {
            corner_text.emplace_back( npc->basic_symbol_color(), npc->name );
        }
    }

    for( auto &v : overmap_buffer.get_vehicle( cursx, cursy, z ) ) {
        corner_text.emplace_back( c_white, v.name );
    }

    if( !corner_text.empty() ) {
        int maxlen = 0;
        for( auto const &line : corner_text ) {
            maxlen = std::max( maxlen, utf8_width( line.second ) );
        }

        const std::string spacer( maxlen, ' ' );
        for( size_t i = 0; i < corner_text.size(); i++ ) {
            const auto &pr = corner_text[ i ];
            // clear line, print line, print vertical line at the right side.
            mvwprintz( w, i, 0, c_yellow, spacer.c_str() );
            mvwprintz( w, i, 0, pr.first, pr.second );
            mvwputch( w, i, maxlen, c_white, LINE_XOXO );
        }
        for( int i = 0; i <= maxlen; i++ ) {
            mvwputch( w, corner_text.size(), i, c_white, LINE_OXOX );
        }
        mvwputch( w, corner_text.size(), maxlen, c_white, LINE_XOOX );
    }

    if( !sZoneName.empty() && tripointZone.x == cursx && tripointZone.y == cursy ) {
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
    if( csee ) {
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
            const auto sm_pos = omt_to_sm_copy( tripoint( cursx, cursy, z ) );

            mvwputch( wbar, 1, 1, ter.get_color(), ter.get_sym() );

            const std::vector<std::string> name = foldstring( overmap_buffer.get_description_at( sm_pos ), 25 );
            for( size_t i = 0; i < name.size(); i++ ) {
                mvwprintz( wbar, i + 1, 3, ter.get_color(), name[i] );
            }
        }
    } else {
        mvwprintz( wbar, 1, 1, c_dark_gray, _( "# Unexplored" ) );
    }

    if( has_target ) {
        // @todo: Add a note that the target is above/below us
        int distance = rl_dist( orig, target );
        mvwprintz( wbar, 3, 1, c_white, _( "Distance to target: %d" ), distance );
    }
    mvwprintz( wbar, 14, 1, c_magenta, _( "Use movement keys to pan." ) );
    if( inp_ctxt != nullptr ) {
        int y = 16;

        const auto print_hint = [&]( const std::string & action, nc_color color = c_magenta ) {
            y += fold_and_print( wbar, y, 1, 27, color, string_format( _( "%s - %s" ),
                                 inp_ctxt->get_desc( action ).c_str(),
                                 inp_ctxt->get_action_name( action ).c_str() ) );
        };

        if( data.debug_editor ) {
            print_hint( "PLACE_TERRAIN", c_light_blue );
            print_hint( "PLACE_SPECIAL", c_light_blue );
            ++y;
        }

        print_hint( "LEVEL_UP" );
        print_hint( "LEVEL_DOWN" );
        print_hint( "CENTER" );
        print_hint( "SEARCH" );
        print_hint( "CREATE_NOTE" );
        print_hint( "DELETE_NOTE" );
        print_hint( "LIST_NOTES" );
        print_hint( "TOGGLE_BLINKING" );
        print_hint( "TOGGLE_OVERLAYS" );
        print_hint( "TOGGLE_CITY_LABELS" );
        print_hint( "TOGGLE_HORDES" );
        print_hint( "TOGGLE_EXPLORED" );
        print_hint( "HELP_KEYBINDINGS" );
        print_hint( "QUIT" );
    }

    point omt( cursx, cursy );
    const point om = omt_to_om_remain( omt );
    mvwprintz( wbar, getmaxy( wbar ) - 1, 1, c_red,
               _( "LEVEL %i, %d'%d, %d'%d" ), z, om.x, omt.x, om.y, omt.y );

    // draw nice crosshair around the cursor
    if( blink && !uistate.place_terrain && !uistate.place_special ) {
        mvwputch( w, om_half_height - 1, om_half_width - 1, c_light_gray, LINE_OXXO );
        mvwputch( w, om_half_height - 1, om_half_width + 1, c_light_gray, LINE_OOXX );
        mvwputch( w, om_half_height + 1, om_half_width - 1, c_light_gray, LINE_XXOO );
        mvwputch( w, om_half_height + 1, om_half_width + 1, c_light_gray, LINE_XOOX );
    }
    // Done with all drawing!
    wrefresh( wbar );
    wmove( w, om_half_height, om_half_width );
    wrefresh( w );
}

tripoint display( const tripoint &orig, const draw_data_t &data = draw_data_t() )
{
    g->w_omlegend = catacurses::newwin( TERMY, 28, 0, TERMX - 28 );
    g->w_overmap = catacurses::newwin( OVERMAP_WINDOW_HEIGHT, OVERMAP_WINDOW_WIDTH, 0, 0 );

    // Draw black padding space to avoid gap between map and legend
    // also clears the pixel minimap in TILES
    g->w_blackspace = catacurses::newwin( TERMY, TERMX, 0, 0 );
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

    // Actions whose keys we want to display.
    ictxt.register_action( "CENTER" );
    ictxt.register_action( "CREATE_NOTE" );
    ictxt.register_action( "DELETE_NOTE" );
    ictxt.register_action( "SEARCH" );
    ictxt.register_action( "LIST_NOTES" );
    ictxt.register_action( "TOGGLE_BLINKING" );
    ictxt.register_action( "TOGGLE_OVERLAYS" );
    ictxt.register_action( "TOGGLE_HORDES" );
    ictxt.register_action( "TOGGLE_CITY_LABELS" );
    ictxt.register_action( "TOGGLE_EXPLORED" );
    if( data.debug_editor ) {
        ictxt.register_action( "PLACE_TERRAIN" );
        ictxt.register_action( "PLACE_SPECIAL" );
    }
    ictxt.register_action( "QUIT" );
    std::string action;
    bool show_explored = true;
    do {
        draw( g->w_overmap, g->w_omlegend, curs, orig, uistate.overmap_show_overlays, show_explored, &ictxt,
              data );
        action = ictxt.handle_input( BLINK_SPEED );

        int dirx = 0;
        int diry = 0;
        if( ictxt.get_direction( dirx, diry, action ) ) {
            curs.x += dirx;
            curs.y += diry;
        } else if( action == "CENTER" ) {
            curs = orig;
        } else if( action == "LEVEL_DOWN" && curs.z > -OVERMAP_DEPTH ) {
            curs.z -= 1;
        } else if( action == "LEVEL_UP" && curs.z < OVERMAP_HEIGHT ) {
            curs.z += 1;
        } else if( action == "CONFIRM" ) {
            ret = tripoint( curs.x, curs.y, curs.z );
        } else if( action == "QUIT" ) {
            ret = overmap::invalid_tripoint;
        } else if( action == "CREATE_NOTE" ) {
            std::string color_notes = _( "Color codes: " );
            for( auto color_pair : get_note_color_names() ) {
                // The color index is not translatable, but the name is.
                color_notes += string_format( "%s:%s, ", color_pair.first.c_str(),
                                              _( color_pair.second.c_str() ) );
            }
            const std::string old_note = overmap_buffer.note( curs );
            const std::string new_note = string_input_popup()
                                         .title( _( "Note (X:TEXT for custom symbol, G; for color):" ) )
                                         .width( 45 )
                                         .text( old_note )
                                         .description( color_notes )
                                         .query_string();
            if( new_note.empty() && !old_note.empty() ) {
                // do nothing, the player should be using [D]elete
            } else if( old_note != new_note ) {
                overmap_buffer.add_note( curs, new_note );
            }
        } else if( action == "DELETE_NOTE" ) {
            if( overmap_buffer.has_note( curs ) && query_yn( _( "Really delete note?" ) ) ) {
                overmap_buffer.delete_note( curs );
            }
        } else if( action == "LIST_NOTES" ) {
            const point p = draw_notes( curs.z );
            if( p.x != -1 && p.y != -1 ) {
                curs.x = p.x;
                curs.y = p.y;
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
        } else if( action == "TOGGLE_HORDES" ) {
            uistate.overmap_show_hordes = !uistate.overmap_show_hordes;
        } else if( action == "TOGGLE_CITY_LABELS" ) {
            uistate.overmap_show_city_labels = !uistate.overmap_show_city_labels;
        } else if( action == "TOGGLE_EXPLORED" ) {
            overmap_buffer.toggle_explored( curs.x, curs.y, curs.z );
        } else if( action == "SEARCH" ) {
            std::string term = string_input_popup()
                               .title( _( "Search term:" ) )
                               .description( _( "Multiple entries separated with , Excludes starting with -" ) )
                               .query_string();
            if( term.empty() ) {
                continue;
            }

            std::vector<point> locations;
            std::vector<point> overmap_checked;

            for( int x = curs.x - OMAPX / 2; x < curs.x + OMAPX / 2; x++ ) {
                for( int y = curs.y - OMAPY / 2; y < curs.y + OMAPY / 2; y++ ) {
                    overmap *om = overmap_buffer.get_existing_om_global( point( x, y ) );

                    if( om ) {
                        int om_relative_x = x;
                        int om_relative_y = y;
                        omt_to_om_remain( om_relative_x, om_relative_y );

                        int om_cache_x = x;
                        int om_cache_y = y;
                        omt_to_om( om_cache_x, om_cache_y );

                        if( std::find( overmap_checked.begin(), overmap_checked.end(), point( om_cache_x,
                                       om_cache_y ) ) == overmap_checked.end() ) {
                            overmap_checked.push_back( point( om_cache_x, om_cache_y ) );
                            std::vector<point> notes = om->find_notes( curs.z, term );
                            locations.insert( locations.end(), notes.begin(), notes.end() );
                        }

                        if( om->seen( om_relative_x, om_relative_y, curs.z ) &&
                            match_include_exclude( om->ter( om_relative_x, om_relative_y, curs.z )->get_name(), term ) ) {
                            locations.push_back( om->global_base_point() + point( om_relative_x, om_relative_y ) );
                        }
                    }
                }
            }

            if( locations.empty() ) {
                sfx::play_variant_sound( "menu_error", "default", 100 );
                popup( _( "No results found." ) );
                continue;
            }

            std::sort( locations.begin(), locations.end(), [&]( const point & lhs, const point & rhs ) {
                return trig_dist( curs, tripoint( lhs, curs.z ) ) < trig_dist( curs, tripoint( rhs, curs.z ) );
            } );

            int i = 0;
            //Navigate through results
            tripoint tmp = curs;
            catacurses::window w_search = catacurses::newwin( 13, 27, 3, TERMX - 27 );

            input_context ctxt( "OVERMAP_SEARCH" );
            ctxt.register_leftright();
            ctxt.register_action( "NEXT_TAB", _( "Next target" ) );
            ctxt.register_action( "PREV_TAB", _( "Previous target" ) );
            ctxt.register_action( "QUIT" );
            ctxt.register_action( "CONFIRM" );
            ctxt.register_action( "HELP_KEYBINDINGS" );
            ctxt.register_action( "ANY_INPUT" );

            do {
                tmp.x = locations[i].x;
                tmp.y = locations[i].y;
                draw( g->w_overmap, g->w_omlegend, tmp, orig, uistate.overmap_show_overlays, show_explored, NULL,
                      draw_data_t() );
                //Draw search box
                mvwprintz( w_search, 1, 1, c_light_blue, _( "Search:" ) );
                mvwprintz( w_search, 1, 10, c_light_red, "%*s", 12, term.c_str() );

                mvwprintz( w_search, 2, 1, c_light_blue, _( "Result(s):" ) );
                mvwprintz( w_search, 2, 16, c_light_red, "%*d/%d", 3, i + 1, locations.size() );

                mvwprintz( w_search, 3, 1, c_light_blue, _( "Direction:" ) );
                mvwprintz( w_search, 3, 14, c_white, "%*d %s",
                           5, static_cast<int>( trig_dist( orig, tripoint( locations[i], orig.z ) ) ),
                           direction_name_short( direction_from( orig, tripoint( locations[i], orig.z ) ) ).c_str()
                         );

                mvwprintz( w_search, 6, 1, c_white, _( "'<' '>' Cycle targets." ) );
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
        } else if( action == "PLACE_TERRAIN" || action == "PLACE_SPECIAL" ) {
            uimenu pmenu;
            // This simplifies overmap_special selection using uimenu
            std::vector<const overmap_special *> oslist;
            const bool terrain = action == "PLACE_TERRAIN";

            if( terrain ) {
                pmenu.title = _( "Select terrain to place:" );
                for( const auto &oter : overmap_terrains::get_all() ) {
                    pmenu.addentry( oter.id.id(), true, 0, oter.id.str() );
                }
            } else {
                pmenu.title = _( "Select special to place:" );
                for( const auto &elem : overmap_specials::get_all() ) {
                    oslist.push_back( &elem );
                    pmenu.addentry( oslist.size() - 1, true, 0, elem.id.str() );
                }
            }
            pmenu.return_invalid = true;
            pmenu.query();

            if( pmenu.ret >= 0 ) {
                catacurses::window w_editor = catacurses::newwin( 15, 27, 3, TERMX - 27 );
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
                // @todo: Unify these things.
                const bool can_rotate = terrain ? uistate.place_terrain->is_rotatable() :
                                        uistate.place_special->rotatable;

                uistate.omedit_rotation = om_direction::type::none;
                // If user chose an already rotated submap, figure out its direction
                if( terrain && can_rotate ) {
                    for( auto r : om_direction::all ) {
                        if( uistate.place_terrain->id.id() == uistate.place_terrain->get_rotated( r ) ) {
                            uistate.omedit_rotation = r;
                            break;
                        }
                    }
                }

                do {
                    // overmap::draw will handle actually showing the preview
                    draw( g->w_overmap, g->w_omlegend, curs, orig, uistate.overmap_show_overlays,
                          show_explored, NULL, draw_data_t() );

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
                    const std::string rotation = om_direction::name( uistate.omedit_rotation );

                    mvwprintz( w_editor, 3, 1, c_light_gray, "                         " );
                    mvwprintz( w_editor, 3, 1, c_light_gray, _( "Rotation: %s %s" ), rotation.c_str(),
                               can_rotate ? "" : _( "(fixed)" ) );
                    mvwprintz( w_editor, 5, 1, c_red, _( "Areas highlighted in red" ) );
                    mvwprintz( w_editor, 6, 1, c_red, _( "already have map content" ) );
                    mvwprintz( w_editor, 7, 1, c_red, _( "generated. Their overmap" ) );
                    mvwprintz( w_editor, 8, 1, c_red, _( "id will change, but not" ) );
                    mvwprintz( w_editor, 9, 1, c_red, _( "their contents." ) );
                    if( ( terrain && uistate.place_terrain->is_rotatable() ) ||
                        ( !terrain && uistate.place_special->rotatable ) ) {
                        mvwprintz( w_editor, 11, 1, c_white, _( "[%s] Rotate" ),
                                   ctxt.get_desc( "ROTATE" ).c_str() );
                    }
                    mvwprintz( w_editor, 12, 1, c_white, _( "[%s] Apply" ),
                               ctxt.get_desc( "CONFIRM" ).c_str() );
                    mvwprintz( w_editor, 13, 1, c_white, _( "[ESCAPE/Q] Cancel" ) );
                    wrefresh( w_editor );

                    action = ctxt.handle_input( BLINK_SPEED );

                    if( ictxt.get_direction( dirx, diry, action ) ) {
                        curs.x += dirx;
                        curs.y += diry;
                    } else if( action == "CONFIRM" ) { // Actually modify the overmap
                        if( terrain ) {
                            overmap_buffer.ter( curs ) = uistate.place_terrain->id.id();
                            overmap_buffer.set_seen( curs.x, curs.y, curs.z, true );
                        } else {
                            for( const auto &s_ter : uistate.place_special->terrains ) {
                                const tripoint pos = curs + om_direction::rotate( s_ter.p, uistate.omedit_rotation );

                                overmap_buffer.ter( pos ) = s_ter.terrain->get_rotated( uistate.omedit_rotation );
                                overmap_buffer.set_seen( pos.x, pos.y, pos.z, true );
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
        } else if( action == "TIMEOUT" ) {
            if( uistate.overmap_blinking ) {
                uistate.overmap_show_overlays = !uistate.overmap_show_overlays;
            }
        } else if( action == "ANY_INPUT" ) {
            if( uistate.overmap_blinking ) {
                uistate.overmap_show_overlays = !uistate.overmap_show_overlays;
            }
        }
    } while( action != "QUIT" && action != "CONFIRM" );
    werase( g->w_overmap );
    werase( g->w_omlegend );
    catacurses::erase();
    g->init_ui( true );
    return ret;
}

} // anonymous namespace


void ui::omap::display()
{
    ::display( g->u.global_omt_location(), draw_data_t() );
}

void ui::omap::display_hordes()
{
    draw_data_t data;
    data.debug_mongroup = true;
    ::display( g->u.global_omt_location(), data );
}

void ui::omap::display_weather()
{
    draw_data_t data;
    data.debug_weather = true;
    ::display( g->u.global_omt_location(), data );
}

void ui::omap::display_scents()
{
    draw_data_t data;
    data.debug_scent = true;
    ::display( g->u.global_omt_location(), data );
}

void ui::omap::display_editor()
{
    draw_data_t data;
    data.debug_editor = true;
    ::display( g->u.global_omt_location(), data );
}

void ui::omap::display_zones( const tripoint &center, const tripoint &select, int const iZoneIndex )
{
    draw_data_t data;
    data.select = select;
    data.iZoneIndex = iZoneIndex;
    ::display( center, data );
}

tripoint ui::omap::choose_point()
{
    return ::display( g->u.global_omt_location() );
}

tripoint ui::omap::choose_point( const tripoint &origin )
{
    return ::display( origin );
}

tripoint ui::omap::choose_point( int z )
{
    tripoint loc = g->u.global_omt_location();
    loc.z = z;
    return ::display( loc );
}
