#include "basecamp.h"

#include <algorithm>
#include <array>
#include <sstream>
#include <map>
#include <string>
#include <vector>

#include "output.h"
#include "string_formatter.h"
#include "translations.h"
#include "enums.h"
#include "game.h"
#include "item_group.h"
#include "map.h"
#include "map_iterator.h"
#include "mapbuffer.h"
#include "mapdata.h"
#include "messages.h"
#include "overmap.h"
#include "overmap_ui.h"
#include "overmapbuffer.h"
#include "npc.h"
#include "recipe.h"
#include "recipe_groups.h"
#include "requirements.h"
#include "skill.h"
#include "string_input_popup.h"
#include "faction_camp.h"

static const std::string base_dir = "[B]";
static const std::string prefix = "faction_base_";
static const int prefix_len = 13;

static const std::string faction_encode( const std::string &type )
{
    return prefix + type + "_";
}

static const std::string faction_encode_full( const expansion_data &e, int offset = 0 )
{
    return faction_encode( e.type ) + to_string( e.cur_level + offset );
}

static int max_upgrade_by_type( const std::string &type )
{
    int max = -1;
    const std::string faction_base = faction_encode( type );
    while( oter_str_id( faction_base + to_string( max + 1 ) ).is_valid() ) {
        max += 1;
    }
    return max;
}

basecamp::basecamp(): bb_pos( tripoint_zero )
{
}

basecamp::basecamp( const std::string &name_, const tripoint &omt_pos_ ): name( name_ ),
    omt_pos( omt_pos_ )
{
}

basecamp::basecamp( const std::string &name_, const tripoint &bb_pos_,
                    std::vector<tripoint> sort_points_, std::vector<std::string> directions_,
                    std::map<std::string, expansion_data> expansions_ ): sort_points( sort_points_ ),
    directions( directions_ ), name( name_ ), bb_pos( bb_pos_ ), expansions( expansions_ )
{
}

std::string basecamp::board_name() const
{
    //~ Name of a basecamp
    return string_format( _( "%s Board" ), name.c_str() );
}

// read an expansion's terrain ID of the form faction_base_$TYPE_$CURLEVEL
// find the last underbar, strip off the prefix of faction_base_ (which is 13 chars),
// and the pull out the $TYPE and $CURLEVEL
// This is legacy support for existing camps; future camps will just set type and level
static expansion_data parse_expansion( const std::string &terrain, const tripoint &new_pos )
{
    expansion_data e;
    int last_bar = terrain.find_last_of( '_' );
    e.type = terrain.substr( prefix_len, last_bar - prefix_len );
    e.cur_level = std::stoi( terrain.substr( last_bar + 1 ) );
    e.pos = new_pos;
    return e;
}

void basecamp::add_expansion( const std::string &terrain, const tripoint &new_pos )
{
    if( terrain.find( prefix ) == std::string::npos ) {
        return;
    }

    const std::string dir = talk_function::om_simple_dir( omt_pos, new_pos );
    expansions[ dir ] = parse_expansion( terrain, new_pos );
    directions.push_back( dir );
}

void basecamp::define_camp( npc &p )
{
    query_new_name();
    omt_pos = p.global_omt_location();
    sort_points = p.companion_mission_points;
    // purging the regions guarantees all entries will start with faction_base_
    for( const std::pair<std::string, tripoint> &expansion :
         talk_function::om_building_region( omt_pos, 1, true ) ) {
        add_expansion( expansion.first, expansion.second );
    }
    const std::string om_cur = overmap_buffer.ter( omt_pos ).id().c_str();
    if( om_cur.find( prefix ) == std::string::npos ) {
        expansion_data e;
        e.type = "camp";
        e.cur_level = 0;
        e.pos = omt_pos;
        expansions[ base_dir ] = e;
    } else {
        expansions[ base_dir ] = parse_expansion( om_cur, omt_pos );
    }
}

bool basecamp::reset_camp()
{
    const std::string base_dir = "[B]";
    const std::string bldg = next_upgrade( base_dir, 0 );
    if( !om_upgrade( bldg, omt_pos ) ) {
        return false;
    }
    return true;
}

/// Returns the description for the recipe of the next building @ref bldg
std::string basecamp::om_upgrade_description( const std::string &bldg, bool trunc )
{
    const recipe &making = recipe_id( bldg ).obj();
    const inventory &total_inv = g->u.crafting_inventory();

    std::vector<std::string> component_print_buffer;
    const int pane = FULL_SCREEN_WIDTH;
    const auto tools = making.requirements().get_folded_tools_list( pane, c_white, total_inv, 1 );
    const auto comps = making.requirements().get_folded_components_list( pane, c_white, total_inv, 1 );
    component_print_buffer.insert( component_print_buffer.end(), tools.begin(), tools.end() );
    component_print_buffer.insert( component_print_buffer.end(), comps.begin(), comps.end() );

    std::string comp;
    for( auto &elem : component_print_buffer ) {
        comp = comp + elem + "\n";
    }
    time_duration duration = time_duration::from_turns( making.time / 100 );
    if( trunc ) {
        comp = string_format( _( "Notes:\n%s\n\nSkill used: %s\n%s\n" ),
                              making.description, making.skill_used.obj().name(), comp );
    } else {
        comp = string_format( _( "Notes:\n%s\n\nSkill used: %s\n"
                                 "Difficulty: %d\n%s \nRisk: None\nTime: %s\n" ),
                              making.description, making.skill_used.obj().name(),
                              making.difficulty, comp, to_string( duration ) );
    }
    return comp;
}

// upgrade levels
bool basecamp::has_level( const std::string &type, int min_level, const std::string &dir ) const
{
    auto e = expansions.find( dir );
    if( e != expansions.end() ) {
        return e->second.type == type && e->second.cur_level >= min_level;
    }
    return false;
}

bool basecamp::any_has_level( const std::string &type, int min_level ) const
{
    for( auto &expansion : expansions ) {
        if( expansion.second.type == type && expansion.second.cur_level >= min_level ) {
            return true;
        }
    }
    return false;
}

bool basecamp::can_expand() const
{
    auto e = expansions.find( base_dir );
    if( e == expansions.end() ) {
        return false;
    }
    return static_cast<int>( directions.size() ) < ( e->second.cur_level / 2 - 1 );
}

const std::string basecamp::next_upgrade( const std::string &dir, const int offset ) const
{
    auto e = expansions.find( dir );
    if( e != expansions.end() ) {
        int cur_level = e->second.cur_level;
        if( cur_level >= 0 ) {
            // cannot upgrade anymore
            if( offset > 0 && cur_level >= max_upgrade_by_type( e->second.type ) ) {
                return "null";
            } else {
                return faction_encode_full( e->second, offset );
            }
        }
    }
    return "null";
}

// recipes and craft support functions
std::map<std::string, std::string> basecamp::recipe_deck( const std::string &dir ) const
{
    if( dir == "ALL" || dir == "COOK" || dir == "BASE" || dir == "FARM" || dir == "SMITH" ) {
        return recipe_group::get_recipes( dir );
    }
    std::map<std::string, std::string> cooking_recipes;
    auto e = expansions.find( dir );
    if( e == expansions.end() ) {
        return cooking_recipes;
    }
    const std::string building = faction_encode( e->second.type );
    int building_max = max_upgrade_by_type( e->second.type );
    for( int building_cur = e->second.cur_level; building_cur <= building_max; building_cur++ ) {
        const std::string building_level = building + to_string( building_cur );
        if( !oter_str_id( building_level ) ) {
            continue;
        }
        std::map<std::string, std::string> test_s = recipe_group::get_recipes( building_level );
        cooking_recipes.insert( test_s.begin(), test_s.end() );
    }
    return cooking_recipes;
}

const std::string basecamp::get_gatherlist() const
{
    auto e = expansions.find( base_dir );
    if( e != expansions.end() ) {
        const std::string gatherlist = "gathering_" + faction_encode_full( e->second );
        if( item_group::group_is_defined( gatherlist ) ) {
            return gatherlist;
        }
    }
    return "forest";

}

// available companion list manipulation
// get all the companions currently performing missions at this camp
void basecamp::reset_camp_workers()
{
    camp_workers.clear();
    for( const auto &elem : overmap_buffer.get_companion_mission_npcs() ) {
        npc_companion_mission c_mission = elem->get_companion_mission();
        if( c_mission.position == omt_pos && c_mission.role_id == "FACTION_CAMP" ) {
            camp_workers.push_back( elem );
        }
    }
}

// get the subset of companions working on a specific task
comp_list basecamp::get_mission_workers( const std::string &mission_id, bool contains )
{
    comp_list available;
    for( const auto &elem : camp_workers ) {
        npc_companion_mission c_mission = elem->get_companion_mission();
        if( ( c_mission.mission_id == mission_id ) ||
            ( contains && c_mission.mission_id.find( mission_id ) != std::string::npos ) ) {
            available.push_back( elem );
        }
    }
    return available;
}

void basecamp::query_new_name()
{
    std::string camp_name;
    string_input_popup popup;
    popup.title( string_format( _( "Name this camp" ) ) )
    .width( 40 )
    .text( "" )
    .max_length( 25 )
    .query();
    if( popup.canceled() || popup.text().empty() ) {
        camp_name = "faction_camp";
    } else {
        camp_name = popup.text();
    }
    name = camp_name;
}

void basecamp::set_name( const std::string &new_name )
{
    name = new_name;
}

// display names
std::string basecamp::expansion_tab( const std::string &dir ) const
{
    if( dir == base_dir ) {
        return _( "Base Missions" );
    }
    auto e = expansions.find( dir );
    if( e != expansions.end() ) {
        if( e->second.type == "garage" ) {
            return _( "Garage Expansion" );
        }
        if( e->second.type == "kitchen" ) {
            return _( "Kitchen Expansion" );
        }
        if( e->second.type == "blacksmith" ) {
            return _( "Blacksmith Expansion" );
        }
        if( e->second.type == "farm" ) {
            return _( "Farm Expansion" );
        }
    }
    return _( "Empty Expansion" );
}

// legacy load and save
void basecamp::load_data( const std::string &data )
{
    std::stringstream stream( data );
    stream >> name >> bb_pos.x >> bb_pos.y;
    // add space to name
    replace( name.begin(), name.end(), '_', ' ' );
}
