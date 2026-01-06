#include "game.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "catacharset.h"
#include "color.h"
#include "construction.h"
#include "construction_group.h"
#include "coordinates.h"
#include "creature.h"
#include "creature_tracker.h"
#include "cursesdef.h"
#include "debug.h"
#include "dialogue.h"
#include "enums.h"
#include "field.h"
#include "field_type.h"
#include "flag.h"
#include "item.h"
#include "iteminfo_query.h"
#include "lightmap.h"
#include "magic_enchantment.h"
#include "map.h"
#include "map_scale_constants.h"
#include "mapdata.h"
#include "npc.h"
#include "omdata.h"
#include "output.h"
#include "overmapbuffer.h"
#include "pimpl.h"
#include "point.h"
#include "sounds.h"
#include "string_formatter.h"
#include "timed_event.h"
#include "translation.h"
#include "translations.h"
#include "trap.h"
#include "type_id.h"
#include "vehicle.h"
#include "vpart_position.h"

enum class om_vision_level : int8_t;

static const efftype_id effect_blind( "blind" );

static const ter_str_id ter_t_grave_new( "t_grave_new" );
static const ter_str_id ter_t_pit( "t_pit" );
static const ter_str_id ter_t_pit_shallow( "t_pit_shallow" );

static const trait_id trait_ILLITERATE( "ILLITERATE" );

void game::print_all_tile_info( const tripoint_bub_ms &lp, const catacurses::window &w_look,
                                const std::string &area_name, int column,
                                int &line,
                                const int last_line,
                                const visibility_variables &cache )
{
    map &here = get_map();

    visibility_type visibility = visibility_type::HIDDEN;
    const bool inbounds = here.inbounds( lp );
    if( inbounds ) {
        visibility = here.get_visibility( here.apparent_light_at( lp, cache ), cache );
    }
    const Creature *creature = get_creature_tracker().creature_at( lp, true );
    switch( visibility ) {
        case visibility_type::CLEAR: {
            const optional_vpart_position vp = here.veh_at( lp );
            print_terrain_info( lp, w_look, area_name, column, line );
            print_fields_info( lp, w_look, column, line );
            print_trap_info( lp, w_look, column, line );
            print_part_con_info( lp, w_look, column, line );
            print_creature_info( creature, w_look, column, line, last_line );
            print_vehicle_info( veh_pointer_or_null( vp ), vp ? vp->part_index() : -1, w_look, column, line,
                                last_line );
            print_items_info( lp, w_look, column, line, last_line );
            print_graffiti_info( lp, w_look, column, line, last_line );
        }
        break;
        case visibility_type::BOOMER:
        case visibility_type::BOOMER_DARK:
        case visibility_type::DARK:
        case visibility_type::LIT:
        case visibility_type::HIDDEN:
            print_visibility_info( w_look, column, line, visibility );

            static std::string raw_description;
            static std::string parsed_description;
            if( creature != nullptr ) {
                const_dialogue d( get_const_talker_for( u ), get_const_talker_for( *creature ) );
                const enchant_cache::special_vision sees_with_special = u.enchantment_cache->get_vision( d );
                if( !sees_with_special.is_empty() ) {
                    // handling against re-evaluation and snippet replacement on redraw
                    if( raw_description.empty() ) {
                        const enchant_cache::special_vision_descriptions special_vis_desc =
                            u.enchantment_cache->get_vision_description_struct( sees_with_special, d );
                        raw_description = special_vis_desc.description.translated();
                        parse_tags( raw_description, *u.as_character(), *creature );
                        parsed_description = raw_description;
                    }
                    mvwprintw( w_look, point( 1, ++line ), parsed_description );
                }
            } else {
                raw_description.clear();
            }
            break;
    }
    print_debug_info( lp, w_look, column, line );
    if( !inbounds ) {
        return;
    }
    const int max_width = getmaxx( w_look ) - column - 1;

    std::string this_sound = sounds::sound_at( lp );
    if( !this_sound.empty() ) {
        const int lines = fold_and_print( w_look, point( 1, ++line ), max_width, c_light_gray,
                                          _( "From here you heard %s" ),
                                          this_sound );
        line += lines - 1;
    } else {
        // Check other z-levels
        tripoint_bub_ms tmp = lp;
        for( tmp.z() = -OVERMAP_DEPTH; tmp.z() <= OVERMAP_HEIGHT; tmp.z()++ ) {
            if( tmp.z() == lp.z() ) {
                continue;
            }

            std::string zlev_sound = sounds::sound_at( tmp );
            if( !zlev_sound.empty() ) {
                const int lines = fold_and_print( w_look, point( 1, ++line ), max_width, c_light_gray,
                                                  tmp.z() > lp.z() ?
                                                  _( "From above you heard %s" ) : _( "From below you heard %s" ), zlev_sound );
                line += lines - 1;
            }
        }
    }
}

void game::print_visibility_info( const catacurses::window &w_look, int column, int &line,
                                  visibility_type visibility )
{
    const char *visibility_message = nullptr;
    switch( visibility ) {
        case visibility_type::CLEAR:
            visibility_message = _( "Clearly visible." );
            break;
        case visibility_type::BOOMER:
            visibility_message = _( "A bright pink blur." );
            break;
        case visibility_type::BOOMER_DARK:
            visibility_message = _( "A pink blur." );
            break;
        case visibility_type::DARK:
            visibility_message = _( "Darkness." );
            break;
        case visibility_type::LIT:
            visibility_message = _( "Bright light." );
            break;
        case visibility_type::HIDDEN:
            visibility_message = _( "Unseen." );
            break;
    }

    mvwprintz( w_look, point( line, column ), c_light_gray, visibility_message );
    line += 2;
}

void game::print_terrain_info( const tripoint_bub_ms &lp, const catacurses::window &w_look,
                               const std::string &area_name, int column, int &line )
{
    map &here = get_map();

    const int max_width = getmaxx( w_look ) - column - 1;

    // Print OMT type and terrain type on first two lines
    // if can't fit in one line.
    std::string tile = uppercase_first_letter( here.tername( lp ) );
    std::string area = uppercase_first_letter( area_name );
    if( const timed_event *e = get_timed_events().get( timed_event_type::OVERRIDE_PLACE ) ) {
        area = e->string_id;
    }
    mvwprintz( w_look, point( column, line++ ), c_yellow, area );
    mvwprintz( w_look, point( column, line++ ), c_white, tile );
    std::string desc = string_format( here.ter( lp ).obj().description );
    std::vector<std::string> lines = foldstring( desc, max_width );
    int numlines = lines.size();
    wattron( w_look, c_light_gray );
    for( int i = 0; i < numlines; i++ ) {
        mvwprintw( w_look, point( column, line++ ), lines[i] );
    }
    wattroff( w_look, c_light_gray );

    // Furniture, if any
    print_furniture_info( lp, w_look, column, line );

    // Cover percentage from terrain and furniture next.
    fold_and_print( w_look, point( column, ++line ), max_width, c_light_gray, _( "Concealment: %d%%" ),
                    here.coverage( lp ) );

    if( here.has_flag( ter_furn_flag::TFLAG_TREE, lp ) ) {
        const int lines = fold_and_print( w_look, point( column, ++line ), max_width, c_light_gray,
                                          _( "Can be <color_green>cut down</color> with the right tools." ) );
        line += lines - 1;
    }

    // Terrain and furniture flags next. These can be several lines for some combinations of
    // furnitures and terrains.
    lines = foldstring( here.features( lp ), max_width );
    numlines = lines.size();
    wattron( w_look, c_light_gray );
    for( int i = 0; i < numlines; i++ ) {
        mvwprintw( w_look, point( column, ++line ), lines[i] );
    }
    wattroff( w_look, c_light_gray );

    // Move cost from terrain and furniture and vehicle parts.
    // Vehicle part information is printed in a different function.
    if( here.impassable( lp ) ) {
        mvwprintz( w_look, point( column, ++line ), c_light_red, _( "Impassable" ) );
    } else {
        mvwprintz( w_look, point( column, ++line ), c_light_gray, _( "Move cost: %d" ),
                   here.move_cost( lp ) * 50 );
    }

    // Next print the string on any SIGN flagged furniture if any.
    std::string signage = here.get_signage( lp );
    if( !signage.empty() ) {
        std::string sign_string = u.has_trait( trait_ILLITERATE ) ? "???" : signage;
        const int lines = fold_and_print( w_look, point( column, ++line ), max_width, c_light_gray,
                                          _( "Sign: %s" ), sign_string );
        line += lines - 1;
    }

    // Print light level on the selected tile.
    std::pair<std::string, nc_color> ll = get_light_level( std::max( 1.0,
                                          LIGHT_AMBIENT_LIT - here.ambient_light_at( lp ) + 1.0 ) );
    mvwprintz( w_look, point( column, ++line ), c_light_gray, _( "Lighting: " ) );
    mvwprintz( w_look, point( column + utf8_width( _( "Lighting: " ) ), line ), ll.second, ll.first );

    // Print the terrain and any furntiure on the tile below and whether it is walkable.
    if( lp.z() > -OVERMAP_DEPTH && !here.has_floor( lp ) ) {
        tripoint_bub_ms below( lp + tripoint::below );
        std::string tile_below = here.tername( below );
        if( here.has_furn( below ) ) {
            tile_below += ", " + here.furnname( below );
        }

        if( !here.has_floor_or_support( lp ) ) {
            fold_and_print( w_look, point( column, ++line ), max_width, c_dark_gray,
                            _( "Below: %s; No support" ), tile_below );
        } else {
            fold_and_print( w_look, point( column, ++line ), max_width, c_dark_gray, _( "Below: %s; Walkable" ),
                            tile_below );
        }
    }

    ++line;
}

void game::print_furniture_info( const tripoint_bub_ms &lp, const catacurses::window &w_look,
                                 int column,
                                 int &line )
{
    map &here = get_map();

    // Do nothing if there is no furniture here
    if( !here.has_furn( lp ) ) {
        return;
    }
    const int max_width = getmaxx( w_look ) - column - 1;

    // Print furniture name in white
    std::string desc = uppercase_first_letter( here.furnname( lp ) );
    mvwprintz( w_look, point( column, line++ ), c_white, desc );

    // Print each line of furniture description in gray
    const furn_id &f = here.furn( lp );
    desc = string_format( f.obj().description );
    std::vector<std::string> lines = foldstring( desc, max_width );
    int numlines = lines.size();
    wattron( w_look, c_light_gray );
    for( int i = 0; i < numlines; i++ ) {
        mvwprintw( w_look, point( column, line++ ), lines[i] );
    }
    wattroff( w_look, c_light_gray );

    // If this furniture has a crafting pseudo item, check for tool qualities and print them
    if( !f->crafting_pseudo_item.is_empty() ) {
        // Make a pseudo item instance so we can use qualities_info later
        const item pseudo( f->crafting_pseudo_item );
        // Set up iteminfo query to show qualities
        std::vector<iteminfo_parts> quality_part = { iteminfo_parts::QUALITIES };
        const iteminfo_query quality_query( quality_part );
        // Render info into info_vec
        std::vector<iteminfo> info_vec;
        pseudo.qualities_info( info_vec, &quality_query, 1, false );
        // Get a newline-separated string of quality info, then parse and print each line
        std::string quality_string = format_item_info( info_vec, {} );
        size_t strpos = 0;
        while( ( strpos = quality_string.find( '\n' ) ) != std::string::npos ) {
            trim_and_print( w_look, point( column, line++ ), max_width, c_light_gray,
                            quality_string.substr( 0, strpos + 1 ) );
            // Delete used token
            quality_string.erase( 0, strpos + 1 );
        }
    }
}

void game::print_fields_info( const tripoint_bub_ms &lp, const catacurses::window &w_look,
                              int column,
                              int &line )
{
    map &here = get_map();

    const field &tmpfield = here.field_at( lp );
    for( const auto &fld : tmpfield ) {
        const field_entry &cur = fld.second;
        if( fld.first.obj().has_fire && ( here.has_flag( ter_furn_flag::TFLAG_FIRE_CONTAINER, lp ) ||
                                          here.ter( lp ) == ter_t_pit_shallow || here.ter( lp ) == ter_t_pit ) ) {
            const int max_width = getmaxx( w_look ) - column - 2;
            int lines = fold_and_print( w_look, point( column, ++line ), max_width, cur.color(),
                                        get_fire_fuel_string( lp ) ) - 1;
            line += lines;
        } else {
            mvwprintz( w_look, point( column, ++line ), cur.color(), cur.name() );
        }
    }

    int size = std::distance( tmpfield.begin(), tmpfield.end() );
    if( size > 0 ) {
        mvwprintz( w_look, point( column, ++line ), c_white, "\n" );
    }
}

void game::print_trap_info( const tripoint_bub_ms &lp, const catacurses::window &w_look,
                            const int column,
                            int &line )
{
    map &here = get_map();

    const trap &tr = here.tr_at( lp );
    if( tr.can_see( lp, u ) ) {
        std::string tr_name = tr.name();
        mvwprintz( w_look, point( column, ++line ), tr.color, tr_name );
    }

    ++line;
}

void game::print_part_con_info( const tripoint_bub_ms &lp, const catacurses::window &w_look,
                                const int column,
                                int &line )
{
    map &here = get_map();

    partial_con *pc = here.partial_con_at( lp );
    std::string tr_name;
    if( pc != nullptr ) {
        const construction &built = pc->id.obj();
        tr_name = string_format( _( "Unfinished task: %s, %d%% complete" ), built.group->name(),
                                 pc->counter / 100000 );

        int const width = getmaxx( w_look ) - column - 2;
        fold_and_print( w_look, point( column, ++line ), width, c_white, tr_name );

        ++line;
    }
}

void game::print_creature_info( const Creature *creature, const catacurses::window &w_look,
                                const int column, int &line, const int last_line )
{
    const map &here = get_map();

    int vLines = last_line - line;
    if( creature != nullptr && ( u.sees( here,  *creature ) || creature == &u ) ) {
        line = creature->print_info( w_look, ++line, vLines, column );
    }
}

void game::print_vehicle_info( const vehicle *veh, int veh_part, const catacurses::window &w_look,
                               const int column, int &line, const int last_line )
{
    if( veh ) {
        // Print the name of the vehicle.
        mvwprintz( w_look, point( column, ++line ), c_light_gray, _( "Vehicle: " ) );
        mvwprintz( w_look, point( column + utf8_width( _( "Vehicle: " ) ), line ), c_white, "%s",
                   veh->name );
        // Then the list of parts on that tile.
        line = veh->print_part_list( w_look, ++line, last_line, getmaxx( w_look ), veh_part );
    }
}

static void add_visible_items_recursive( std::map<std::string, std::pair<int, nc_color>>
        &item_names, const item &it )
{
    ++item_names[it.tname()].first;
    item_names[it.tname()].second = it.color_in_inventory();

    for( const item *content : it.all_known_contents() ) {
        add_visible_items_recursive( item_names, *content );
    }
}

void game::print_items_info( const tripoint_bub_ms &lp, const catacurses::window &w_look,
                             const int column,
                             int &line,
                             const int last_line )
{
    map &here = get_map();

    if( !here.sees_some_items( lp, u ) ) {
        return;
    } else if( here.has_flag( ter_furn_flag::TFLAG_CONTAINER, lp ) && !here.could_see_items( lp, u ) ) {
        mvwprintw( w_look, point( column, ++line ), _( "You cannot see what is inside of it." ) );
    } else if( u.has_effect( effect_blind ) || u.worn_with_flag( flag_BLIND ) ) {
        mvwprintz( w_look, point( column, ++line ), c_yellow,
                   _( "There's something there, but you can't see what it is." ) );
        return;
    } else {
        std::map<std::string, std::pair<int, nc_color>> item_names;
        for( const item &it : here.i_at( lp ) ) {
            add_visible_items_recursive( item_names, it );
        }

        const int max_width = getmaxx( w_look ) - column - 1;
        for( auto it = item_names.begin(); it != item_names.end(); ++it ) {
            // last line but not last item
            if( line + 1 >= last_line && std::next( it ) != item_names.end() ) {
                mvwprintz( w_look, point( column, ++line ), c_yellow, _( "More items here…" ) );
                break;
            }

            if( it->second.first > 1 ) {
                trim_and_print( w_look, point( column, ++line ), max_width, it->second.second,
                                pgettext( "%s is the name of the item.  %d is the quantity of that item.", "%s [%d]" ),
                                it->first.c_str(), it->second.first );
            } else {
                trim_and_print( w_look, point( column, ++line ), max_width, it->second.second, it->first );
            }
        }
    }
}

void game::print_graffiti_info( const tripoint_bub_ms &lp, const catacurses::window &w_look,
                                const int column, int &line,
                                const int last_line )
{
    map &here = get_map();

    if( line > last_line ) {
        return;
    }

    const int max_width = getmaxx( w_look ) - column - 2;
    if( here.has_graffiti_at( lp ) ) {
        const int lines = fold_and_print( w_look, point( column, ++line ), max_width, c_light_gray,
                                          here.ter( lp ) == ter_t_grave_new ? _( "Graffiti: %s" ) : _( "Inscription: %s" ),
                                          here.graffiti_at( lp ) );
        line += lines - 1;
    }
}

void game::print_debug_info( const tripoint_bub_ms &lp, const catacurses::window &w_look,
                             const int column, int &line )
{
    if( debug_mode ) {
        const map &here = get_map();

        mvwprintz( w_look, point( column, ++line ), c_white, "tripoint_bub_ms: %s",
                   lp.to_string_writable() );
        mvwprintz( w_look, point( column, ++line ), c_white, "tripoint_abs_ms: %s",
                   here.get_abs( lp ).to_string_writable() );

        for( const std::pair<const field_type_id, field_entry> &fd : here.field_at( lp ) ) {
            mvwprintz( w_look, point( column, ++line ), c_white, "field: " );
            mvwprintz( w_look, point( column + utf8_width( "field: " ), line ), c_yellow, "%s",
                       fd.first.id().c_str() );
            mvwprintz( w_look, point( column, ++line ), c_white, "age: %s (%s seconds)",
                       to_string( fd.second.get_field_age() ), to_seconds<int>( fd.second.get_field_age() ) );
            mvwprintz( w_look, point( column, ++line ), c_white, "intensity: %s",
                       fd.second.get_field_intensity() );
            mvwprintz( w_look, point( column, ++line ), c_white, "causer: %s",
                       fd.second.get_causer() == nullptr ? "none" : fd.second.get_causer()->disp_name() );
            mvwprintz( w_look, point( column, ++line ), c_white, "\n" );
        }
    }
}

void game::pre_print_all_tile_info( const tripoint_bub_ms &lp, const catacurses::window &w_info,
                                    int &first_line, const int last_line,
                                    const visibility_variables &cache )
{
    map &here = get_map();

    // get global area info according to look_around caret position
    tripoint_abs_omt omp( coords::project_to<coords::omt>( here.get_abs( lp ) ) );
    const oter_id &cur_ter_m = overmap_buffer.ter( omp );
    om_vision_level vision = overmap_buffer.seen( omp );
    // we only need the area name and then pass it to print_all_tile_info() function below
    const std::string area_name = cur_ter_m->get_name( vision );
    print_all_tile_info( lp, w_info, area_name, 1, first_line, last_line, cache );
}
