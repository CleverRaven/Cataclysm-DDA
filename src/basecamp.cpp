#include "basecamp.h"

#include <algorithm>
#include <map>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "character_id.h"
#include "clzones.h"
#include "color.h"
#include "compatibility.h"
#include "coordinate_conversions.h"
#include "debug.h"
#include "faction_camp.h"
#include "flat_set.h"
#include "game.h"
#include "inventory.h"
#include "item.h"
#include "item_group.h"
#include "map.h"
#include "map_iterator.h"
#include "npc.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "player.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "recipe_groups.h"
#include "requirements.h"
#include "string_formatter.h"
#include "string_id.h"
#include "string_input_popup.h"
#include "translations.h"
#include "type_id.h"

static const zone_type_id zone_type_CAMP_STORAGE( "CAMP_STORAGE" );

const std::map<point, base_camps::direction_data> base_camps::all_directions = {
    // direction, direction id, tab order, direction abbreviation with bracket, direction tab title
    { base_camps::base_dir, { "[B]", base_camps::TAB_MAIN, to_translation( "base camp: base", "[B]" ), to_translation( "base camp: base", " MAIN " ) } },
    { point_north, { "[N]", base_camps::TAB_N, to_translation( "base camp: north", "[N]" ), to_translation( "base camp: north", "  [N] " ) } },
    { point_north_east, { "[NE]", base_camps::TAB_NE, to_translation( "base camp: northeast", "[NE]" ), to_translation( "base camp: northeast", " [NE] " ) } },
    { point_east, { "[E]", base_camps::TAB_E, to_translation( "base camp: east", "[E]" ), to_translation( "base camp: east", "  [E] " ) } },
    { point_south_east, { "[SE]", base_camps::TAB_SE, to_translation( "base camp: southeast", "[SE]" ), to_translation( "base camp: southeast", " [SE] " ) } },
    { point_south, { "[S]", base_camps::TAB_S, to_translation( "base camp: south", "[S]" ), to_translation( "base camp: south", "  [S] " ) } },
    { point_south_west, { "[SW]", base_camps::TAB_SW, to_translation( "base camp: southwest", "[SW]" ), to_translation( "base camp: southwest", " [SW] " ) } },
    { point_west, { "[W]", base_camps::TAB_W, to_translation( "base camp: west", "[W]" ), to_translation( "base camp: west", "  [W] " ) } },
    { point_north_west, { "[NW]", base_camps::TAB_NW, to_translation( "base camp: northwest", "[NW]" ), to_translation( "base camp: northwest", " [NW] " ) } },
};

point base_camps::direction_from_id( const std::string &id )
{
    for( const auto &dir : all_directions ) {
        if( dir.second.id == id ) {
            return dir.first;
        }
    }
    return base_dir;
}

std::string base_camps::faction_encode_short( const std::string &type )
{
    return prefix + type + "_";
}

std::string base_camps::faction_encode_abs( const expansion_data &e, int number )
{
    return faction_encode_short( e.type ) + to_string( number );
}

std::string base_camps::faction_decode( const std::string &full_type )
{
    if( full_type.size() < ( prefix_len + 2 ) ) {
        return "camp";
    }
    int last_bar = full_type.find_last_of( '_' );

    return full_type.substr( prefix_len, last_bar - prefix_len );
}

time_duration base_camps::to_workdays( const time_duration &work_time )
{
    if( work_time < 11_hours ) {
        return work_time;
    }
    int work_days = work_time / 10_hours;
    time_duration excess_time = work_time - work_days * 10_hours;
    return excess_time + 24_hours * work_days;
}

static std::map<std::string, int> max_upgrade_cache;

int base_camps::max_upgrade_by_type( const std::string &type )
{
    if( max_upgrade_cache.find( type ) == max_upgrade_cache.end() ) {
        int max = -1;
        const std::string faction_base = faction_encode_short( type );
        while( recipe_id( faction_base + to_string( max + 1 ) ).is_valid() ) {
            max += 1;
        }
        max_upgrade_cache[type] = max;
    }
    return max_upgrade_cache[type];
}

basecamp::basecamp() = default;

basecamp::basecamp( const std::string &name_, const tripoint &omt_pos_ ): name( name_ ),
    omt_pos( omt_pos_ )
{
}

basecamp::basecamp( const std::string &name_, const tripoint &bb_pos_,
                    const std::vector<point> &directions_,
                    const std::map<point, expansion_data> &expansions_ )
    : directions( directions_ ), name( name_ ), bb_pos( bb_pos_ ), expansions( expansions_ )
{
}

std::string basecamp::board_name() const
{
    //~ Name of a basecamp
    return string_format( _( "%s Board" ), name );
}

void basecamp::set_by_radio( bool access_by_radio )
{
    by_radio = access_by_radio;
}

// read an expansion's terrain ID of the form faction_base_$TYPE_$CURLEVEL
// find the last underbar, strip off the prefix of faction_base_ (which is 13 chars),
// and the pull out the $TYPE and $CURLEVEL
// This is legacy support for existing camps; future camps don't use cur_level at all
expansion_data basecamp::parse_expansion( const std::string &terrain, const tripoint &new_pos )
{
    expansion_data e;
    int last_bar = terrain.find_last_of( '_' );
    e.type = terrain.substr( base_camps::prefix_len, last_bar - base_camps::prefix_len );
    e.cur_level = std::stoi( terrain.substr( last_bar + 1 ) );
    e.pos = new_pos;
    return e;
}

void basecamp::add_expansion( const std::string &terrain, const tripoint &new_pos )
{
    if( terrain.find( base_camps::prefix ) == std::string::npos ) {
        return;
    }

    const point dir = talk_function::om_simple_dir( omt_pos, new_pos );
    expansions[ dir ] = parse_expansion( terrain, new_pos );
    resources_updated = false;
    reset_camp_resources();
    update_provides( terrain, expansions[ dir ] );
    directions.push_back( dir );
}

void basecamp::add_expansion( const std::string &bldg, const tripoint &new_pos,
                              const point &dir )
{
    expansion_data e;
    e.type = base_camps::faction_decode( bldg );
    e.cur_level = -1;
    e.pos = new_pos;
    expansions[ dir ] = e;
    directions.push_back( dir );
    update_provides( bldg, expansions[ dir ] );
    update_resources( bldg );
}

void basecamp::define_camp( const tripoint &p, const std::string &camp_type )
{
    query_new_name();
    omt_pos = p;
    const oter_id &omt_ref = overmap_buffer.ter( omt_pos );
    // purging the regions guarantees all entries will start with faction_base_
    for( const std::pair<std::string, tripoint> &expansion :
         talk_function::om_building_region( omt_pos, 1, true ) ) {
        add_expansion( expansion.first, expansion.second );
    }
    const std::string om_cur = omt_ref.id().c_str();
    if( om_cur.find( base_camps::prefix ) == std::string::npos ) {
        expansion_data e;
        e.type = base_camps::faction_decode( camp_type );
        e.cur_level = -1;
        e.pos = omt_pos;
        expansions[base_camps::base_dir] = e;
        const std::string direction = oter_get_rotation_string( omt_ref );
        const oter_id bcid( direction.empty() ? "faction_base_camp_0" : "faction_base_camp_new_0" +
                            direction );
        overmap_buffer.ter_set( omt_pos, bcid );
        update_provides( base_camps::faction_encode_abs( e, 0 ),
                         expansions[base_camps::base_dir] );
    } else {
        expansions[base_camps::base_dir] = parse_expansion( om_cur, omt_pos );
    }
}

/// Returns the description for the recipe of the next building @ref bldg
std::string basecamp::om_upgrade_description( const std::string &bldg, bool trunc ) const
{
    const recipe &making = recipe_id( bldg ).obj();

    std::vector<std::string> component_print_buffer;
    const int pane = FULL_SCREEN_WIDTH;
    const auto tools = making.simple_requirements().get_folded_tools_list( pane, c_white, _inv, 1 );
    const auto comps = making.simple_requirements().get_folded_components_list( pane, c_white, _inv,
                       making.get_component_filter(), 1 );
    component_print_buffer.insert( component_print_buffer.end(), tools.begin(), tools.end() );
    component_print_buffer.insert( component_print_buffer.end(), comps.begin(), comps.end() );

    std::string comp;
    for( auto &elem : component_print_buffer ) {
        comp = comp + elem + "\n";
    }
    comp = string_format( _( "Notes:\n%s\n\nSkills used: %s\n%s\n" ),
                          making.description, making.required_all_skills_string(), comp );
    if( !trunc ) {
        time_duration base_time = making.batch_duration();
        comp += string_format( _( "Risk: None\nTime: %s\n" ),
                               to_string( base_camps::to_workdays( base_time ) ) );
    }
    return comp;
}

// upgrade levels
// legacy next upgrade
std::string basecamp::next_upgrade( const point &dir, const int offset ) const
{
    const auto &e = expansions.find( dir );
    if( e == expansions.end() ) {
        return "null";
    }
    const expansion_data &e_data = e->second;

    int cur_level = -1;
    for( int i = 0; i < base_camps::max_upgrade_by_type( e_data.type ); i++ ) {
        const std::string candidate = base_camps::faction_encode_abs( e_data, i );
        if( e_data.provides.find( candidate ) == e_data.provides.end() ) {
            break;
        } else {
            cur_level = i;
        }
    }
    if( cur_level >= 0 ) {
        return base_camps::faction_encode_abs( e_data, cur_level + offset );
    }
    return "null";
}

bool basecamp::has_provides( const std::string &req, const expansion_data &e_data, int level ) const
{
    for( const auto &provide : e_data.provides ) {
        if( provide.first == req && provide.second > level ) {
            return true;
        }
    }
    return false;
}

bool basecamp::has_provides( const std::string &req, const cata::optional<point> &dir,
                             int level ) const
{
    if( !dir ) {
        for( const auto &e : expansions ) {
            if( has_provides( req, e.second, level ) ) {
                return true;
            }
        }
    } else {
        const auto &e = expansions.find( *dir );
        if( e != expansions.end() ) {
            return has_provides( req, e->second, level );
        }
    }
    return false;
}

bool basecamp::can_expand()
{
    return has_provides( "bed", base_camps::base_dir, directions.size() * 2 );
}

bool basecamp::has_water()
{
    return has_provides( "water_well" ) || has_provides( "fbmh_well_north" ) ||
           has_provides( "faction_base_camp_12" ) || has_provides( "faction_base_kitchen_6" ) ||
           has_provides( "faction_base_blacksmith_11" );
}

std::vector<basecamp_upgrade> basecamp::available_upgrades( const point &dir )
{
    std::vector<basecamp_upgrade> ret_data;
    auto e = expansions.find( dir );
    if( e != expansions.end() ) {
        expansion_data &e_data = e->second;
        for( const recipe *recp_p : recipe_dict.all_blueprints() ) {
            const recipe &recp = *recp_p;
            const std::string &bldg = recp.result().str();
            // skip buildings that are completed
            if( e_data.provides.find( bldg ) != e_data.provides.end() ) {
                continue;
            }
            // skip building that have unmet requirements
            size_t needed_requires = recp.blueprint_requires().size();
            size_t met_requires = 0;
            for( const auto &bp_require : recp.blueprint_requires() ) {
                if( e_data.provides.find( bp_require.first ) == e_data.provides.end() ) {
                    break;
                }
                if( e_data.provides[bp_require.first] < bp_require.second ) {
                    break;
                }
                met_requires += 1;
            }
            if( met_requires < needed_requires ) {
                continue;
            }
            bool should_display = true;
            bool in_progress = false;
            for( const auto &bp_exclude : recp.blueprint_excludes() ) {
                // skip buildings that are excluded by previous builds
                if( e_data.provides.find( bp_exclude.first ) != e_data.provides.end() ) {
                    if( e_data.provides[bp_exclude.first] >= bp_exclude.second ) {
                        should_display = false;
                        break;
                    }
                }
                // track buildings that are currently being built
                if( e_data.in_progress.find( bp_exclude.first ) != e_data.in_progress.end() ) {
                    if( e_data.in_progress[bp_exclude.first] >= bp_exclude.second ) {
                        in_progress = true;
                        break;
                    }
                }
            }
            if( !should_display ) {
                continue;
            }
            basecamp_upgrade data;
            data.bldg = bldg;
            data.name = recp.blueprint_name();
            const auto &reqs = recp.deduped_requirements();
            data.avail = reqs.can_make_with_inventory( _inv, recp.get_component_filter(), 1 );
            data.in_progress = in_progress;
            ret_data.emplace_back( data );
        }
    }
    return ret_data;
}

// recipes and craft support functions
std::map<recipe_id, translation> basecamp::recipe_deck( const point &dir ) const
{
    std::map<recipe_id, translation> recipes;
    const auto &e = expansions.find( dir );
    if( e == expansions.end() ) {
        return recipes;
    }
    for( const auto &provides : e->second.provides ) {
        const auto &test_s = recipe_group::get_recipes_by_id( provides.first );
        recipes.insert( test_s.cbegin(), test_s.cend() );
    }
    return recipes;
}

std::map<recipe_id, translation> basecamp::recipe_deck( const std::string &bldg ) const
{
    const std::map<recipe_id, translation> recipes = recipe_group::get_recipes_by_bldg( bldg );
    return recipes;
}

std::string basecamp::get_gatherlist() const
{
    const auto &e = expansions.find( base_camps::base_dir );
    if( e != expansions.end() ) {
        const std::string gatherlist = "gathering_" +
                                       base_camps::faction_encode_abs( e->second, 4 );
        if( item_group::group_is_defined( gatherlist ) ) {
            return gatherlist;
        }
    }
    return "forest";
}

void basecamp::add_resource( const itype_id &camp_resource )
{
    basecamp_resource bcp_r;
    bcp_r.fake_id = camp_resource;
    item camp_item( bcp_r.fake_id, 0 );
    bcp_r.ammo_id = camp_item.ammo_default();
    resources.emplace_back( bcp_r );
    fuel_types.insert( bcp_r.ammo_id );
}

void basecamp::update_resources( const std::string &bldg )
{
    if( !recipe_id( bldg ).is_valid() ) {
        return;
    }

    const recipe &making = recipe_id( bldg ).obj();
    for( const itype_id &bp_resource : making.blueprint_resources() ) {
        add_resource( bp_resource );
    }
}

void basecamp::update_provides( const std::string &bldg, expansion_data &e_data )
{
    if( !recipe_id( bldg ).is_valid() ) {
        return;
    }

    const recipe &making = recipe_id( bldg ).obj();
    for( const auto &bp_provides : making.blueprint_provides() ) {
        if( e_data.provides.find( bp_provides.first ) == e_data.provides.end() ) {
            e_data.provides[bp_provides.first] = 0;
        }
        e_data.provides[bp_provides.first] += bp_provides.second;
    }
}

void basecamp::update_in_progress( const std::string &bldg, const point &dir )
{
    if( !recipe_id( bldg ).is_valid() ) {
        return;
    }
    auto e = expansions.find( dir );
    if( e == expansions.end() ) {
        return;
    }
    expansion_data &e_data = e->second;

    const recipe &making = recipe_id( bldg ).obj();
    for( const auto &bp_provides : making.blueprint_provides() ) {
        if( e_data.in_progress.find( bp_provides.first ) == e_data.in_progress.end() ) {
            e_data.in_progress[bp_provides.first] = 0;
        }
        e_data.in_progress[bp_provides.first] += bp_provides.second;
    }
}

void basecamp::reset_camp_resources()
{
    reset_camp_workers();
    if( !resources_updated ) {
        resources_updated = true;
        for( auto &e : expansions ) {
            expansion_data &e_data = e.second;
            for( int level = 0; level <= e_data.cur_level; level++ ) {
                const std::string &bldg = base_camps::faction_encode_abs( e_data, level );
                if( bldg == "null" ) {
                    break;
                }
                update_provides( bldg, e_data );
            }
            for( const auto &bp_provides : e_data.provides ) {
                update_resources( bp_provides.first );
            }
        }
    }
    form_crafting_inventory();
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

void basecamp::add_assignee( character_id id )
{
    npc_ptr npc_to_add = overmap_buffer.find_npc( id );
    if( !npc_to_add ) {
        debugmsg( "cant find npc to assign to basecamp, on the overmap_buffer" );
        return;
    }
    npc_to_add->assigned_camp = omt_pos;
    assigned_npcs.push_back( npc_to_add );
}

void basecamp::remove_assignee( character_id id )
{
    npc_ptr npc_to_remove = overmap_buffer.find_npc( id );
    if( !npc_to_remove ) {
        debugmsg( "cant find npc to remove from basecamp, on the overmap_buffer" );
        return;
    }
    npc_to_remove->assigned_camp = cata::nullopt;
    assigned_npcs.erase( std::remove( assigned_npcs.begin(), assigned_npcs.end(), npc_to_remove ),
                         assigned_npcs.end() );
}

void basecamp::validate_assignees()
{
    std::vector<npc_ptr>::iterator iter = assigned_npcs.begin();
    while( iter != assigned_npcs.end() ) {
        if( !( *iter ) || !( *iter )->assigned_camp || *( *iter )->assigned_camp != omt_pos ) {
            iter = assigned_npcs.erase( iter );
        } else {
            ++iter;
        }
    }
    for( character_id elem : g->get_follower_list() ) {
        npc_ptr npc_to_add = overmap_buffer.find_npc( elem );
        if( !npc_to_add ) {
            continue;
        }
        if( std::find( assigned_npcs.begin(), assigned_npcs.end(), npc_to_add ) != assigned_npcs.end() ) {
            continue;
        } else {
            if( npc_to_add->assigned_camp && *npc_to_add->assigned_camp == omt_pos ) {
                assigned_npcs.push_back( npc_to_add );
            }
        }
    }
    // remove duplicates - for legacy handling.
    std::sort( assigned_npcs.begin(), assigned_npcs.end() );
    auto last = std::unique( assigned_npcs.begin(), assigned_npcs.end() );
    assigned_npcs.erase( last, assigned_npcs.end() );
}

std::vector<npc_ptr> basecamp::get_npcs_assigned()
{
    validate_assignees();
    return assigned_npcs;
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
    popup.title( _( "Name this camp" ) )
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

/*
 * we could put this logic in map::use_charges() the way the vehicle code does, but I think
 * that's sloppy
 */
std::list<item> basecamp::use_charges( const itype_id &fake_id, int &quantity )
{
    std::list<item> ret;
    if( quantity <= 0 ) {
        return ret;
    }
    for( basecamp_resource &bcp_r : resources ) {
        if( bcp_r.fake_id == fake_id ) {
            item camp_item( bcp_r.fake_id, 0 );
            camp_item.charges = std::min( bcp_r.available, quantity );
            quantity -= camp_item.charges;
            bcp_r.available -= camp_item.charges;
            bcp_r.consumed += camp_item.charges;
            ret.push_back( camp_item );
            if( quantity <= 0 ) {
                break;
            }
        }
    }
    return ret;
}

void basecamp::form_crafting_inventory( map &target_map )
{
    _inv.clear();
    const tripoint &dump_spot = get_dumping_spot();
    const tripoint &origin = target_map.getlocal( dump_spot );
    auto &mgr = zone_manager::get_manager();
    if( g->m.check_vehicle_zones( g->get_levz() ) ) {
        mgr.cache_vzones();
    }
    if( mgr.has_near( zone_type_CAMP_STORAGE, dump_spot, 60 ) ) {
        std::unordered_set<tripoint> src_set = mgr.get_near( zone_type_CAMP_STORAGE, dump_spot, 60 );
        _inv.form_from_zone( target_map, src_set, nullptr, false );
    }
    /*
     * something of a hack: add the resources we know the camp has
     * the hacky part is that we're adding resources based on the camp's flags, which were
     * generated based on upgrade missions, instead of having resources added when the
     * map changes
     */
    // make sure the array is empty
    fuels.clear();
    for( const itype_id &fuel_id : fuel_types ) {
        basecamp_fuel bcp_f;
        bcp_f.ammo_id = fuel_id;
        fuels.emplace_back( bcp_f );
    }

    // find available fuel
    for( const tripoint &pt : target_map.points_in_radius( origin, inv_range ) ) {
        if( target_map.accessible_items( pt ) ) {
            for( const item &i : target_map.i_at( pt ) ) {
                for( basecamp_fuel &bcp_f : fuels ) {
                    if( bcp_f.ammo_id == i.typeId() ) {
                        bcp_f.available += i.charges;
                        break;
                    }
                }
            }
        }
    }
    for( basecamp_resource &bcp_r : resources ) {
        bcp_r.consumed = 0;
        item camp_item( bcp_r.fake_id, 0 );
        camp_item.item_tags.insert( "PSEUDO" );
        if( !bcp_r.ammo_id.is_null() ) {
            for( basecamp_fuel &bcp_f : fuels ) {
                if( bcp_f.ammo_id == bcp_r.ammo_id ) {
                    if( bcp_f.available > 0 ) {
                        bcp_r.available = bcp_f.available;
                        camp_item = camp_item.ammo_set( bcp_f.ammo_id, bcp_f.available );
                    }
                    break;
                }
            }
        }
        _inv.add_item( camp_item );
    }
}

void basecamp::form_crafting_inventory()
{
    if( by_radio ) {
        tinymap target_map;
        target_map.load( tripoint( omt_pos.x * 2, omt_pos.y * 2, omt_pos.z ), false );
        form_crafting_inventory( target_map );
    } else {
        form_crafting_inventory( g->m );
    }
}

// display names
std::string basecamp::expansion_tab( const point &dir ) const
{
    if( dir == base_camps::base_dir ) {
        return _( "Base Missions" );
    }
    const auto &expansion_types = recipe_group::get_recipes_by_id( "all_faction_base_expansions" );

    const auto &e = expansions.find( dir );
    if( e != expansions.end() ) {
        recipe_id id( base_camps::faction_encode_abs( e->second, 0 ) );
        const auto e_type = expansion_types.find( id );
        if( e_type != expansion_types.end() ) {
            return e_type->second + _( "Expansion" );
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

basecamp_action_components::basecamp_action_components(
    const recipe &making, int batch_size, basecamp &base ) :
    making_( making ),
    batch_size_( batch_size ),
    base_( base )
{
}

bool basecamp_action_components::choose_components()
{
    const auto filter = is_crafting_component;
    const requirement_data *req =
        making_.deduped_requirements().select_alternative( g->u, base_._inv, filter, batch_size_ );
    if( !req ) {
        return false;
    }
    if( !item_selections_.empty() || !tool_selections_.empty() ) {
        debugmsg( "Reused basecamp_action_components" );
        return false;
    }
    for( const auto &it : req->get_components() ) {
        comp_selection<item_comp> is =
            g->u.select_item_component( it, batch_size_, base_._inv, true, filter,
                                        !base_.by_radio );
        if( is.use_from == cancel ) {
            return false;
        }
        item_selections_.push_back( is );
    }
    // this may consume pseudo-resources from fake items
    for( const auto &it : req->get_tools() ) {
        comp_selection<tool_comp> ts =
            g->u.select_tool_component( it, batch_size_, base_._inv, DEFAULT_HOTKEYS, true,
                                        !base_.by_radio );
        if( ts.use_from == cancel ) {
            return false;
        }
        tool_selections_.push_back( ts );
    }
    return true;
}

void basecamp_action_components::consume_components()
{
    map *target_map = &g->m;
    if( base_.by_radio ) {
        map_ = std::make_unique<tinymap>();
        map_->load( omt_to_sm_copy( base_.camp_omt_pos() ), false );
        target_map = map_.get();
    }
    const tripoint &origin = target_map->getlocal( base_.get_dumping_spot() );
    for( const comp_selection<item_comp> &sel : item_selections_ ) {
        g->u.consume_items( *target_map, sel, batch_size_, is_crafting_component, origin,
                            basecamp::inv_range );
    }
    // this may consume pseudo-resources from fake items
    for( const comp_selection<tool_comp> &sel : tool_selections_ ) {
        g->u.consume_tools( *target_map, sel, batch_size_, origin, basecamp::inv_range, &base_ );
    }
    // go back and consume the actual resources
    for( basecamp_resource &bcp_r : base_.resources ) {
        if( bcp_r.consumed > 0 ) {
            target_map->use_charges( origin, basecamp::inv_range, bcp_r.ammo_id, bcp_r.consumed );
            bcp_r.consumed = 0;
        }
    }
    if( map_ ) {
        map_->save();
        map_.reset();
    }
}
