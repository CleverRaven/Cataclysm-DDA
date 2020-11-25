#include "monfaction.h"

#include <algorithm>
#include <map>
#include <set>
#include <utility>

#include "debug.h"
#include "debug_menu.h"
#include "generic_factory.h"
#include "int_id.h"
#include "json.h"
#include "string_id.h"

// for legacy reasons "monfaction::id" is called "name" in json
static generic_factory<monfaction> faction_factory( "MONSTER_FACTION", "name" );

/** @relates int_id */
template<>
const monfaction &int_id<monfaction>::obj() const
{
    return faction_factory.obj( *this );
}

/** @relates int_id */
template<>
const string_id<monfaction> &int_id<monfaction>::id() const
{
    return faction_factory.convert( *this );
}

/** @relates string_id */
template<>
int_id<monfaction> string_id<monfaction>::id() const
{
    return faction_factory.convert( *this, mfaction_id( 0 ) );
}

/** @relates string_id */
template<>
const monfaction &string_id<monfaction>::obj() const
{
    return faction_factory.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<monfaction>::is_valid() const
{
    return faction_factory.is_valid( *this );
}

/** @relates int_id */
template<>
int_id<monfaction>::int_id( const string_id<monfaction> &id ) : _id( id.id() ) {}

bool monfaction::is_root() const
{
    return id == base_faction;
}

mf_attitude monfaction::attitude( const mfaction_id &other ) const
{
    int idx = other.to_i();
    if( idx < 0 || idx >= static_cast<int>( attitude_vec.size() ) ) {
        debugmsg( "Invalid mfaction_id in the attitude check: %d", idx );
        return MFA_FRIENDLY;
    }
    return static_cast<mf_attitude>( attitude_vec[other.to_i()] );
}

cata::optional<mf_attitude> monfaction::attitude_rec( const mfaction_str_id &other ) const
{
    const auto &found = attitude_map.find( other );
    if( found != attitude_map.end() ) {
        return found->second;
    }

    if( !other->is_root() ) {
        return attitude( other->base_faction );
    }

    return cata::nullopt;
}

void monfactions::reset()
{
    faction_factory.reset();
}

void monfactions::load_monster_faction( const JsonObject &jo, const std::string &src )
{
    faction_factory.load( jo, src );
}

bool monfaction::detect_base_faction_cycle() const
{
    std::list<mfaction_str_id> parents_path;
    std::set<mfaction_str_id> parents;
    mfaction_str_id cur = id;

    while( !cur->is_root() ) {
        parents_path.push_back( cur );
        // cycle detected!
        if( parents.count( cur ) ) {
            debugmsg( "In MONSTER_FACTION '%s': detected a cycle in base_faction relations: %s",
            this -> id.str(), debug_menu::iterable_to_string( parents_path, " -> ", []( const auto & el ) {
                return el.str();
            } ) );
            return true;
        }
        parents.insert( cur );
        cur = cur->base_faction;
    }
    return false;
}

void monfaction::inherit_parent_attitude_rec(
    std::set<mfaction_str_id> &processed,
    std::map<mfaction_str_id, mf_attitude> &child_attitudes ) const
{
    // already processed, just populate the return list and exit
    if( !processed.count( id ) ) {
        if( !is_root() ) {
            base_faction->inherit_parent_attitude_rec( processed, child_attitudes );
        }
        // wish we had 'map::merge'
        for( const auto &ci : child_attitudes ) {
            attitude_map.emplace( ci.first, ci.second );
        }
    }
    for( const auto &att : attitude_map ) {
        // child attitude settings overwrite those of a parent
        child_attitudes[att.first] = att.second;
    }
    processed.insert( id );
}

void monfaction::populate_attitude_vec() const
{
    attitude_vec.clear();
    for( const auto &f : faction_factory.get_all() ) {
        const cata::optional<mf_attitude> &attitude = attitude_rec( f.id );
        if( !attitude ) {
            debugmsg( "Invalid faction relations (no relation found): %s -> %s",
                      id.c_str(), f.id.c_str() );
        }
        attitude_vec.push_back( attitude.value_or( MFA_FRIENDLY ) );
    }
    attitude_vec.shrink_to_fit();
}

void monfactions::finalize()
{
    if( faction_factory.get_all().empty() ) {
        debugmsg( "No monster factions found." );
        return;
    }
    if( !mfaction_str_id::NULL_ID().is_valid() ) {
        debugmsg( "MONSTER_FACTION \"sentinel\" (entry with empty name) is not found in json." );
    }

    for( const auto &f : faction_factory.get_all() ) {
        // `detect_base_faction_cycle` detects a cycle that is formed by valid `base_faction` relations.
        // it will produce a warning if cycle is detected
        if( f.detect_base_faction_cycle() ) {
            // break the cycle
            const_cast<monfaction &>( f ).base_faction = mfaction_str_id::NULL_ID();
        }
    }

    // adds attitudes_map collected from all `base_faction`
    std::set<mfaction_str_id> processed;
    for( const auto &f : faction_factory.get_all() ) {
        std::map<mfaction_str_id, mf_attitude> accum;
        f.inherit_parent_attitude_rec( processed, accum );
    }

    // at this point all factions are loaded, populate the final attitude cache
    for( const auto &f : faction_factory.get_all() ) {
        f.populate_attitude_vec();
    }
}

void monfaction::load( const JsonObject &jo, const std::string & )
{
    optional( jo, was_loaded, "base_faction", base_faction, mfaction_str_id() );

    static const std::array<std::pair<std::string, mf_attitude>, 4> fields_by_attitude {
        {
            {"by_mood", MFA_BY_MOOD},
            {"neutral", MFA_NEUTRAL},
            {"friendly", MFA_FRIENDLY},
            {"hate", MFA_HATE}
        }
    };

    for( const auto &field_att_pair : fields_by_attitude ) {
        const std::string &json_field_name = field_att_pair.first;
        const mf_attitude attitude = field_att_pair.second;

        for( const auto &f : jo.get_tags<mfaction_str_id>( json_field_name ) ) {
            attitude_map[f] = attitude;
        }
    }

    // by default faction is friendly to itself (don't overwrite if explicitly specified)
    attitude_map.emplace( id, MFA_FRIENDLY );
}

const std::vector<monfaction> &monfactions::get_all()
{
    return faction_factory.get_all();
}
