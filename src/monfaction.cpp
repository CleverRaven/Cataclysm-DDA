#include "monfaction.h"

#include <cstddef>
#include <queue>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <utility>

#include "debug.h"
#include "json.h"
#include "string_id.h"

std::unordered_map< mfaction_str_id, mfaction_id > faction_map;
std::vector< monfaction > faction_list;

void add_to_attitude_map( const std::set< std::string > &keys, mfaction_att_map &map,
                          mf_attitude value );

void apply_base_faction( const monfaction &base, monfaction &faction );

/** @relates int_id */
template<>
const monfaction &int_id<monfaction>::obj() const
{
    if( static_cast<size_t>( _id ) >= faction_list.size() ) {
        debugmsg( "invalid monfaction id %d", _id );
        return mfaction_id( 0 ).obj();
    }

    return faction_list[_id];
}

/** @relates int_id */
template<>
const string_id<monfaction> &int_id<monfaction>::id() const
{
    return obj().id;
}

/** @relates string_id */
template<>
int_id<monfaction> string_id<monfaction>::id() const
{
    const auto &iter = faction_map.find( *this );
    if( iter == faction_map.end() ) {
        debugmsg( "invalid monfaction id %s", c_str() );
        return mfaction_id( 0 );
    }
    return iter->second;
}

/** @relates string_id */
template<>
const monfaction &string_id<monfaction>::obj() const
{
    return id().obj();
}

/** @relates string_id */
template<>
bool string_id<monfaction>::is_valid() const
{
    return faction_map.count( *this ) > 0;
}

/** @relates int_id */
template<>
int_id<monfaction>::int_id( const string_id<monfaction> &id )
    : _id( id.id() )
{
}

mfaction_id monfactions::get_or_add_faction( const mfaction_str_id &id )
{
    auto found = faction_map.find( id );
    if( found == faction_map.end() ) {
        monfaction mfact;
        mfact.id = id;
        mfact.loadid = mfaction_id( faction_map.size() );
        // -1 base faction marks this faction as not initialized.
        // If it is not changed before validation, it will become a child of
        // the root of the faction tree.
        mfact.base_faction = mfaction_id( -1 );
        faction_map[mfact.id] = mfact.loadid;
        faction_list.push_back( mfact );
        found = faction_map.find( mfact.id );
    }

    return found->second;
}

static void apply_base_faction( mfaction_id base, mfaction_id faction_id )
{
    for( const auto &pair : base.obj().attitude_map ) {
        // Fill in values set in base faction, but not in derived one
        auto &faction = faction_list[faction_id];
        if( faction.attitude_map.count( pair.first ) == 0 ) {
            faction.attitude_map.insert( pair );
        }
    }
}

mf_attitude monfaction::attitude( const mfaction_id &other ) const
{
    const auto &found = attitude_map.find( other );
    if( found != attitude_map.end() ) {
        return found->second;
    }

    const auto base = other.obj().base_faction;
    if( other != base ) {
        return attitude( base );
    }

    // Shouldn't happen
    debugmsg( "Invalid faction relations (no relation found): %s -> %s",
              id.c_str(), other.obj().id.c_str() );
    return MFA_FRIENDLY;
}

void monfactions::finalize()
{
    if( faction_list.empty() ) {
        debugmsg( "No monster factions found." );
        return;
    }

    // Create a tree of faction dependence
    std::multimap< mfaction_id, mfaction_id > child_map;
    std::set< mfaction_id > unloaded; // To check if cycles exist
    std::queue< mfaction_id > queue;
    for( auto &faction : faction_list ) {
        unloaded.insert( faction.loadid );
        if( faction.loadid == faction.base_faction ) {
            // No parent = root of the (a?) tree
            queue.push( faction.loadid );
            continue;
        }

        // Point parent to children
        if( faction.base_faction >= 0 ) {
            child_map.insert( std::make_pair( faction.base_faction, faction.loadid ) );
        }

        // Set faction as friendly to itself if not explicitly set to anything
        if( faction.attitude_map.count( faction.loadid ) == 0 ) {
            faction.attitude_map[faction.loadid] = MFA_FRIENDLY;
        }
    }

    if( queue.empty() && !faction_list.empty() ) {
        debugmsg( "No valid root monster faction!" );
        return;
    }

    // Set uninitialized factions to be children of the root.
    // If more than one root exists, use the first one.
    const auto root = queue.front();
    for( auto &faction : faction_list ) {
        if( faction.base_faction < 0 ) {
            faction.base_faction = root;
            // If it is the (new) root, connecting it to own parent (self) would create a cycle.
            // So only try to connect it to the parent if it isn't own parent.
            if( faction.base_faction != faction.loadid ) {
                child_map.insert( std::make_pair( faction.base_faction, faction.loadid ) );
            }
        }
    }

    // Traverse the tree (breadth-first), starting from root
    while( !queue.empty() ) {
        mfaction_id cur = queue.front();
        queue.pop();
        if( unloaded.count( cur ) != 0 ) {
            unloaded.erase( cur );
        } else {
            debugmsg( "Tried to load monster faction %s more than once", cur.obj().id.c_str() );
            continue;
        }
        auto children = child_map.equal_range( cur );
        for( auto &it = children.first; it != children.second; ++it ) {
            // Copy attributes to child
            apply_base_faction( cur, it->second );
            queue.push( it->second );
        }
    }

    // Bad json
    if( !unloaded.empty() ) {
        std::string names;
        for( auto &fac : unloaded ) {
            names.append( fac.id().str() );
            names.append( " " );
            auto &the_faction = faction_list[fac];
            the_faction.base_faction = root;
        }

        debugmsg( "Cycle encountered when processing monster factions. Bad factions:\n %s", names.c_str() );
    }

    faction_list.shrink_to_fit(); // Save a couple of bytes
}

// Ensures all those factions exist
static void prealloc( const std::set< std::string > &facs )
{
    for( const auto &f : facs ) {
        monfactions::get_or_add_faction( mfaction_str_id( f ) );
    }
}

// Get pointers to factions from 'keys' and add them to 'map' with value == 'value'
void add_to_attitude_map( const std::set< std::string > &keys, mfaction_att_map &map,
                          mf_attitude value )
{
    for( const auto &k : keys ) {
        const auto &faction = mfaction_str_id( k ).id();
        map[faction] = value;
    }
}

void monfactions::load_monster_faction( JsonObject &jo )
{
    // Factions inherit values from their parent factions - this is set during finalization
    std::set< std::string > by_mood = jo.get_tags( "by_mood" );
    std::set< std::string > neutral = jo.get_tags( "neutral" );
    std::set< std::string > friendly = jo.get_tags( "friendly" );
    // Need to make sure adding new factions won't invalidate our current faction's reference
    // That +1 is for base faction
    faction_list.reserve( faction_list.size() + by_mood.size() + neutral.size() + friendly.size() + 1 );
    prealloc( by_mood );
    prealloc( neutral );
    prealloc( friendly );

    std::string name = jo.get_string( "name" );
    mfaction_id cur_id = get_or_add_faction( mfaction_str_id( name ) );
    std::string base_faction = jo.get_string( "base_faction", "" );
    mfaction_id base_id = get_or_add_faction( mfaction_str_id( base_faction ) );
    // Don't get the reference until here (to avoid vector reallocation messing it up)
    monfaction &faction = faction_list[cur_id];
    faction.base_faction = base_id;

    add_to_attitude_map( by_mood, faction.attitude_map, MFA_BY_MOOD );
    add_to_attitude_map( neutral, faction.attitude_map, MFA_NEUTRAL );
    add_to_attitude_map( friendly, faction.attitude_map, MFA_FRIENDLY );
}
