#include "horde_map.h"

#include <memory>
#include <tuple>

#include "monster.h"
#include "mtype.h"

// Is just entity enough or do we need to wrap it in a tuple with a coordinate?
// Or worse an iterator?
horde_entity *horde_map::entity_at( const tripoint_om_ms &p )
{
    tripoint_om_sm submap_offset = project_to<coords::sm>( p );
    // TODO reconsider pruning p down to tripoint_om_ms in the first place.
    tripoint_abs_ms entity_loc = project_combine( location, p );
    std::unordered_map<tripoint_om_sm, std::unordered_map<tripoint_abs_ms, horde_entity>>::iterator
            active_monster_submap = active_monster_map.find( submap_offset );
    bool active_map_exists = active_monster_submap != active_monster_map.end() &&
                             !active_monster_submap->second.empty();
    if( active_map_exists ) {
        std::unordered_map<const tripoint_abs_ms, horde_entity>::iterator iter =
            active_monster_submap->second.find( entity_loc );
        if( iter != active_monster_submap->second.end() ) {
            return &iter->second;
        }
    }
    std::unordered_map<tripoint_om_sm, std::unordered_map<tripoint_abs_ms, horde_entity>>::iterator
            idle_monster_submap = idle_monster_map.find( submap_offset );
    bool idle_map_exists = idle_monster_submap != idle_monster_map.end() &&
                           !idle_monster_submap->second.empty();
    if( idle_map_exists ) {
        std::unordered_map<const tripoint_abs_ms, horde_entity>::iterator iter =
            idle_monster_submap->second.find( entity_loc );
        if( iter != idle_monster_submap->second.end() ) {
            return &iter->second;
        }
    }
    std::unordered_map<tripoint_om_sm, std::unordered_map<tripoint_abs_ms, horde_entity>>::iterator
            dormant_monster_submap = dormant_monster_map.find( submap_offset );
    bool dormant_map_exists = dormant_monster_submap != dormant_monster_map.end() &&
                              !dormant_monster_submap->second.empty();
    if( dormant_map_exists ) {
        std::unordered_map<const tripoint_abs_ms, horde_entity>::iterator iter =
            dormant_monster_submap->second.find( entity_loc );
        if( iter != dormant_monster_submap->second.end() ) {
            return  &iter->second;
        }
    }
    return nullptr;
}

// TODO: if callers want to filter for dormant vs idle vs active we can do it cheaply.
std::vector < std::unordered_map<tripoint_abs_ms, horde_entity> *> horde_map::entity_group_at(
    const tripoint_om_omt &p )
{
    // TODO: It might be worthwhile to have a single top level map of per-submap containers,
    // and each entry of that map holds each variant container.
    // This eliminates multiple top-level lookups.
    std::vector<std::unordered_map<tripoint_abs_ms, horde_entity>*> horde_chunk;
    // TODO: Find all 4 submaps worth of monsters and return them.
    for( int y = 0; y <= 1; ++y ) {
        for( int x = 0; x <= 1; ++x ) {
            tripoint_om_sm target_submap = project_to<coords::sm>( p ) + point{ x, y };
            std::vector<std::unordered_map<tripoint_abs_ms, horde_entity> *> submap_of_hordes =
                entity_group_at( target_submap );
            horde_chunk.insert( horde_chunk.end(), submap_of_hordes.begin(), submap_of_hordes.end() );
        }
    }
    return horde_chunk;
}

// TODO: if callers want to filter for dormant vs idle vs active we can do it cheaply.
std::vector<std::unordered_map<tripoint_abs_ms, horde_entity> *> horde_map::entity_group_at(
    const tripoint_om_sm &p )
{
    std::vector<std::unordered_map<tripoint_abs_ms, horde_entity>*> horde_chunk;
    auto active_monster_map_iter = active_monster_map.find( p );
    if( active_monster_map_iter != active_monster_map.end() ) {
        horde_chunk.push_back( &active_monster_map_iter->second );
    }
    auto idle_monster_map_iter = idle_monster_map.find( p );
    if( idle_monster_map_iter != idle_monster_map.end() ) {
        horde_chunk.push_back( &idle_monster_map_iter->second );
    }
    auto dormant_monster_map_iter = dormant_monster_map.find( p );
    if( dormant_monster_map_iter != dormant_monster_map.end() ) {
        horde_chunk.push_back( &dormant_monster_map_iter->second );
    }
    return horde_chunk;
}

// These have no goal so they can't go in the active map.
std::unordered_map<tripoint_abs_ms, horde_entity>::iterator horde_map::spawn_entity(
    const tripoint_abs_ms &p, mtype_id id )
{
    std::unordered_map <tripoint_om_sm, std::unordered_map<tripoint_abs_ms, horde_entity>> &target_map =
                id->has_flag( mon_flag_DORMANT ) ? dormant_monster_map : idle_monster_map;
    std::unordered_map<tripoint_abs_ms, horde_entity>::iterator result;
    point_abs_om omp;
    tripoint_om_sm sm;
    std::tie( omp, sm ) = project_remain<coords::om>( project_to<coords::sm>( p ) );
    bool inserted;
    // The [] operator creates a nested std::map if not present already.
    std::tie( result, inserted ) = target_map[sm].emplace( p, id );
    return result;
}

// TODO: check for a goal in horde_entity and put in active vs idle.
std::unordered_map<tripoint_abs_ms, horde_entity>::iterator horde_map::spawn_entity(
    const tripoint_abs_ms &p, const monster &mon )
{
    std::unordered_map <tripoint_om_sm, std::unordered_map<tripoint_abs_ms, horde_entity>> &target_map =
                mon.type->has_flag( mon_flag_DORMANT ) ?
                dormant_monster_map :
                ( mon.has_dest() || mon.wandf > 0 ) ?
                active_monster_map :
                idle_monster_map;
    std::unordered_map<tripoint_abs_ms, horde_entity>::iterator result;
    point_abs_om omp;
    tripoint_om_sm sm;
    std::tie( omp, sm ) = project_remain<coords::om>( project_to<coords::sm>( p ) );
    bool inserted;
    // The [] operator creates a nested std::map if not present already.
    std::tie( result, inserted ) = target_map[sm].emplace( p, mon );
    result->second.monster_data->set_pos_abs_only( p );
    return result;
}

// TODO: This placement is wrong, just getting something compiling
void horde_map::insert( std::unordered_map<tripoint_abs_ms, horde_entity>::node_type &&node )
{
    std::unordered_map <tripoint_om_sm, std::unordered_map<tripoint_abs_ms, horde_entity>> &target_map =
                node.mapped().get_type()->has_flag( mon_flag_DORMANT ) ? dormant_monster_map :
                node.mapped().is_active() ? active_monster_map : idle_monster_map;
    point_abs_om omp;
    tripoint_om_sm sm;
    std::tie( omp, sm ) = project_remain<coords::om>( project_to<coords::sm> ( node.key() ) );
    // The [] operator creates a nested std::map if not present already.
    target_map[sm].insert( std::move( node ) );
}

void horde_map::clear()
{
    active_monster_map.clear();
    idle_monster_map.clear();
    dormant_monster_map.clear();
}

void horde_map::clear_chunk( const tripoint_om_sm &p )
{
    active_monster_map.erase( p );
    idle_monster_map.erase( p );
    dormant_monster_map.erase( p );
}
