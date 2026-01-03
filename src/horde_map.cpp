#include "horde_map.h"

#include <memory>
#include <string>
#include <tuple>

#include "debug.h"
#include "map_scale_constants.h"
#include "monster.h"
#include "mtype.h"

static const species_id species_FERAL( "FERAL" );
static const species_id species_ZOMBIE( "ZOMBIE" );

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
        std::unordered_map<tripoint_abs_ms, horde_entity>::iterator iter =
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
        std::unordered_map<tripoint_abs_ms, horde_entity>::iterator iter =
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
        std::unordered_map<tripoint_abs_ms, horde_entity>::iterator iter =
            dormant_monster_submap->second.find( entity_loc );
        if( iter != dormant_monster_submap->second.end() ) {
            return  &iter->second;
        }
    }
    std::unordered_map<tripoint_om_sm, std::unordered_map<tripoint_abs_ms, horde_entity>>::iterator
            immobile_monster_submap = immobile_monster_map.find( submap_offset );
    bool immobile_map_exists = immobile_monster_submap != immobile_monster_map.end() &&
                               !immobile_monster_submap->second.empty();
    if( immobile_map_exists ) {
        std::unordered_map<tripoint_abs_ms, horde_entity>::iterator iter =
            immobile_monster_submap->second.find( entity_loc );
        if( iter != immobile_monster_submap->second.end() ) {
            return  &iter->second;
        }
    }
    return nullptr;
}

// TODO: if callers want to filter for dormant vs idle vs active, etc we can do it cheaply.
std::vector < std::unordered_map<tripoint_abs_ms, horde_entity> *> horde_map::entity_group_at(
    const tripoint_om_omt &p, int filter )
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
                entity_group_at( target_submap, filter );
            horde_chunk.insert( horde_chunk.end(), submap_of_hordes.begin(), submap_of_hordes.end() );
        }
    }
    return horde_chunk;
}

std::vector<std::unordered_map<tripoint_abs_ms, horde_entity> *> horde_map::entity_group_at(
    const tripoint_om_sm &p, int filter )
{
    std::vector<std::unordered_map<tripoint_abs_ms, horde_entity>*> horde_chunk;

    if( filter & horde_map_flavors::active ) {
        auto active_monster_map_iter = active_monster_map.find( p );
        if( active_monster_map_iter != active_monster_map.end() ) {
            horde_chunk.push_back( &active_monster_map_iter->second );
        }
    }

    if( filter & horde_map_flavors::idle ) {
        auto idle_monster_map_iter = idle_monster_map.find( p );
        if( idle_monster_map_iter != idle_monster_map.end() ) {
            horde_chunk.push_back( &idle_monster_map_iter->second );
        }
    }

    if( filter & horde_map_flavors::dormant ) {
        auto dormant_monster_map_iter = dormant_monster_map.find( p );
        if( dormant_monster_map_iter != dormant_monster_map.end() ) {
            horde_chunk.push_back( &dormant_monster_map_iter->second );
        }
    }

    if( filter & horde_map_flavors::immobile ) {
        auto immobile_monster_map_iter = immobile_monster_map.find( p );
        if( immobile_monster_map_iter != immobile_monster_map.end() ) {
            horde_chunk.push_back( &immobile_monster_map_iter->second );
        }
    }
    return horde_chunk;
}

// Helper because this is too much to inline.
// This essentially means "these should get sounds broadcast to them".
static bool is_alert( const mtype &type )
{
    bool aggro = type.in_species( species_ZOMBIE ) || type.in_species( species_FERAL );
    return aggro && type.has_flag( mon_flag_HEARS );
}

// These have no goal so they can't go in the active map.
std::unordered_map<tripoint_abs_ms, horde_entity>::iterator horde_map::spawn_entity(
    const tripoint_abs_ms &p, mtype_id id )
{
    std::unordered_map<tripoint_abs_ms, horde_entity>::iterator result;
    if( id.is_null() || !id.is_valid() ) {
        return result; // Bail out, blacklisted monster or something's wrong.
    }
    std::unordered_map <tripoint_om_sm, std::unordered_map<tripoint_abs_ms, horde_entity>> &target_map =
                id->has_flag( mon_flag_DORMANT ) ? dormant_monster_map :
                is_alert( *id ) ? idle_monster_map :
                immobile_monster_map;
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
                mon.type->has_flag( mon_flag_DORMANT ) ? dormant_monster_map :
                !is_alert( *mon.type ) ? immobile_monster_map :
                ( mon.has_dest() || mon.wandf > 0 ) ? active_monster_map :
                idle_monster_map;
    std::unordered_map<tripoint_abs_ms, horde_entity>::iterator result;
    point_abs_om omp;
    tripoint_om_sm sm;
    std::tie( omp, sm ) = project_remain<coords::om>( project_to<coords::sm>( p ) );
    bool inserted;
    // The [] operator creates a nested std::map if not present already.
    std::tie( result, inserted ) = target_map[sm].emplace( p, mon );
    if( inserted ) {
        result->second.monster_data->set_pos_abs_only( p );
    } else {
        debugmsg( "Attempted to insert a %s at %s, but there's already a %s there!",
                  mon.name(), p.to_string(), result->second.get_type()->nname() );
    }
    return result;
}

static void signal_sm( const tripoint_abs_ms &origin, const tripoint_abs_sm &sm_dest,
                       const tripoint_abs_sm &sm_origin, int volume,
                       std::unordered_map <tripoint_om_sm, std::unordered_map<tripoint_abs_ms, horde_entity>>::iterator
                       &sm_iter, bool active, std::unordered_map<tripoint_abs_ms, horde_entity> &migrating_hordes )
{

    const int dist = rl_dist( sm_dest, sm_origin );
    int eff_power = volume - dist;
    if( eff_power <= 0 ) {
        return;
    }
    int scaled_eff_power = eff_power * SEEX;
    for( std::unordered_map<tripoint_abs_ms, horde_entity>::iterator mon = sm_iter->second.begin();
         mon != sm_iter->second.end(); ) {
        // Avoid unecessary extract/insert for already-active horde entities.
        if( !active ) {
            mon->second.destination = origin;
            mon->second.tracking_intensity = scaled_eff_power;
            std::unordered_map<tripoint_abs_ms, horde_entity>::iterator moving_mon = mon;
            // Advance the loop iterator past the current node, which we will be removing.
            mon++;
            auto monster_node = sm_iter->second.extract( moving_mon );
            migrating_hordes.insert( std::move( monster_node ) );
        } else {
            if( mon->second.tracking_intensity < scaled_eff_power ) {
                mon->second.destination = origin;
                mon->second.tracking_intensity = scaled_eff_power;
            }
            ++mon;
        }
    }
}

// Volume is scaled down by SEEX so it matches the scale of tripoint_om_sm
// dormant_monster_map and immobile_monster_map are intentionally excluded here.
void horde_map::signal_entities( const tripoint_abs_ms &origin, int volume )
{
    std::unordered_map<tripoint_abs_ms, horde_entity> migrating_hordes;
    tripoint_abs_sm sm_dest = project_to<coords::sm>( origin );
    for( std::unordered_map
         <tripoint_om_sm, std::unordered_map<tripoint_abs_ms, horde_entity>>::iterator active_sm_iter =
             active_monster_map.begin(); active_sm_iter != active_monster_map.end(); ++active_sm_iter ) {
        tripoint_abs_sm abs_sm = project_combine( location, active_sm_iter->first );
        signal_sm( origin, sm_dest, abs_sm, volume, active_sm_iter, true, migrating_hordes );
    }
    for( std::unordered_map
         <tripoint_om_sm, std::unordered_map<tripoint_abs_ms, horde_entity>>::iterator idle_sm_iter =
             idle_monster_map.begin(); idle_sm_iter != idle_monster_map.end(); ++idle_sm_iter ) {
        tripoint_abs_sm abs_sm = project_combine( location, idle_sm_iter->first );
        signal_sm( origin, sm_dest, abs_sm, volume, idle_sm_iter, false, migrating_hordes );
    }

    while( !migrating_hordes.empty() ) {
        auto monster_node = migrating_hordes.extract( migrating_hordes.begin() );
        insert( std::move( monster_node ) );
    }
}

void horde_map::insert( std::unordered_map<tripoint_abs_ms, horde_entity>::node_type &&node )
{
    std::unordered_map <tripoint_om_sm, std::unordered_map<tripoint_abs_ms, horde_entity>> &target_map =
                node.mapped().get_type()->has_flag( mon_flag_DORMANT ) ? dormant_monster_map :
                node.mapped().is_active() ? active_monster_map :
                is_alert( *node.mapped().get_type() ) ? idle_monster_map :
                immobile_monster_map;
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
    immobile_monster_map.clear();
}

void horde_map::clear_chunk( const tripoint_om_sm &p )
{
    active_monster_map.erase( p );
    idle_monster_map.erase( p );
    dormant_monster_map.erase( p );
    immobile_monster_map.erase( p );
}

// horde_map::iterator definitions

void horde_map::iterator::next_map()
{
    if( outer_map == &parent->active_monster_map ) {
        if( filter & horde_map_flavors::idle ) {
            outer_map = &parent->idle_monster_map;
        } else if( filter & horde_map_flavors::dormant ) {
            outer_map = &parent->dormant_monster_map;
        } else if( filter & horde_map_flavors::immobile ) {
            outer_map = &parent->immobile_monster_map;
        } else {
            outer_map = nullptr;
        }
    } else if( outer_map == &parent->idle_monster_map ) {
        if( filter & horde_map_flavors::dormant ) {
            outer_map = &parent->dormant_monster_map;
        } else if( filter & horde_map_flavors::immobile ) {
            outer_map = &parent->immobile_monster_map;
        } else {
            outer_map = nullptr;
        }
    } else if( outer_map == &parent->dormant_monster_map ) {
        if( filter & horde_map_flavors::immobile ) {
            outer_map = &parent->immobile_monster_map;
        } else {
            outer_map = nullptr;
        }
    } else {
        // At the end.
        outer_map = nullptr;
    }
}

// This helper insures that a freshly created iterator is in a valid state.
void horde_map::iterator::insure_valid()
{
    // Iterate forward if we don't have a valid initial state.
    if( !( filter & horde_map_flavors::active ) || outer_map->empty() ) {
        do {
            next_map();
            if( outer_map == nullptr ) {
                return;
            }
        } while( outer_map->empty() );
        outer_iter = outer_map->begin();
    }
    // This is not obviously correct, but it is correct because
    // horde_map::erase() insures that we cull empty maps, so if
    // outer_map is not empty, outer_map->begin() is valid and so is
    // outer_map->begin()->second.begin()
    inner_iter = outer_iter->second.begin();
}

horde_map::iterator &horde_map::iterator::operator++()
{
    if( inner_iter != outer_iter->second.end() ) {
        ++inner_iter;
    }
    while( inner_iter == outer_iter->second.end() ) {
        ++outer_iter;
        while( outer_iter == outer_map->end() ) {
            next_map();
            if( outer_map == nullptr ) {
                return *this;
            }
            outer_iter = outer_map->begin();
        }
        inner_iter = outer_iter->second.begin();
    }
    return *this;
}

horde_map::iterator horde_map::iterator::operator++( int )
{
    iterator retval = *this;
    ++( *this );
    return retval;
}

bool horde_map::iterator::operator==( iterator other ) const
{
    return ( outer_map == nullptr && other.outer_map == nullptr ) ||
           ( outer_map == other.outer_map && outer_iter == other.outer_iter &&
             inner_iter == other.inner_iter );
}

bool horde_map::iterator::operator!=( iterator other ) const
{
    return !( *this == other );
}

horde_map::iterator::reference horde_map::iterator::operator*() const
{
    return *inner_iter;
}

horde_map::iterator::pointer horde_map::iterator::operator->() const
{
    return &*inner_iter;
}

horde_map::iterator horde_map::find( const tripoint_om_ms &loc )
{
    tripoint_om_sm submap_loc = project_to<coords::sm>( loc );
    tripoint_abs_ms monster_loc = project_combine( location, loc );
    map_type::iterator submap_iter = active_monster_map.find( submap_loc );
    if( submap_iter != active_monster_map.end() ) {
        std::unordered_map<tripoint_abs_ms, horde_entity>::iterator mon_iter =
            submap_iter->second.find( monster_loc );
        if( mon_iter != submap_iter->second.end() ) {
            return iterator( *this, active_monster_map, submap_iter, mon_iter );
        }
    }
    submap_iter = idle_monster_map.find( submap_loc );
    if( submap_iter != idle_monster_map.end() ) {
        std::unordered_map<tripoint_abs_ms, horde_entity>::iterator mon_iter =
            submap_iter->second.find( monster_loc );
        if( mon_iter != submap_iter->second.end() ) {
            return iterator( *this, idle_monster_map, submap_iter, mon_iter );
        }
    }
    submap_iter = dormant_monster_map.find( submap_loc );
    if( submap_iter != dormant_monster_map.end() ) {
        std::unordered_map<tripoint_abs_ms, horde_entity>::iterator mon_iter =
            submap_iter->second.find( monster_loc );
        if( mon_iter != submap_iter->second.end() ) {
            return iterator( *this, dormant_monster_map, submap_iter, mon_iter );
        }
    }
    return end();
}

horde_map::iterator horde_map::erase( iterator iter )
{
    iterator old_iter = iter;
    iter.inner_iter =
        iter.outer_iter->second.erase( iter.inner_iter );
    if( iter.inner_iter == iter.outer_iter->second.end() ) {
        ++iter;
        if( old_iter.outer_iter->second.empty() ) {
            old_iter.outer_map->erase( old_iter.outer_iter );
        }
    }
    return iter;
}

horde_map::node_type horde_map::extract( iterator iter )
{
    node_type node = iter.outer_iter->second.extract( iter.inner_iter );
    if( iter.outer_iter->second.empty() ) {
        iter.outer_map->erase( iter.outer_iter );
    }
    return node;
}
