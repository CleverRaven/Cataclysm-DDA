#include "fault.h"

#include <utility>
#include <vector>

#include "debug.h"
#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "item.h"
#include "requirements.h"
#include "rng.h"

namespace
{

generic_factory<fault> fault_factory( "fault", "id" );
generic_factory<fault_fix> fault_fixes_factory( "fault_fix", "id" );
generic_factory<fault_group> fault_group_factory( "fault_group", "id" );

// we'll store requirement_ids here and wait for requirements to load in, then we can actualize them
std::multimap<fault_fix_id, std::pair<std::string, int>> reqs_temp_storage;

// Have a list of faults by type, the type right now is item prefix to avoid adding more JSON data
std::map<std::string, std::vector<fault_id>> faults_by_type;

} // namespace

std::vector<fault_id> faults::all_of_type( const std::string &type )
{
    const auto &typed = faults_by_type.find( type );
    if( typed == faults_by_type.end() ) {
        debugmsg( "there are no faults with type '%s'", type );
        return {};
    }
    return typed->second ;
}

const fault_id &faults::random_of_type( const std::string &type )
{
    return random_entry_ref( all_of_type( type ) );
}

const fault_id &faults::random_of_type_item_has( const item &it, const std::string &type )
{
    const std::vector<fault_id> &typed = all_of_type( type );

    // not actually random
    for( const fault_id &fid : typed ) {
        if( it.has_fault( fid ) ) {
            return fid;
        }
    }

    return fault_id::NULL_ID();
}

void faults::load_fault( const JsonObject &jo, const std::string &src )
{
    fault_factory.load( jo, src );
}

void faults::load_fix( const JsonObject &jo, const std::string &src )
{
    fault_fixes_factory.load( jo, src );
}

void faults::load_group( const JsonObject &jo, const std::string &src )
{
    fault_group_factory.load( jo, src );
}

void faults::reset()
{
    fault_factory.reset();
    fault_fixes_factory.reset();
    faults_by_type.clear();
}

void faults::finalize()
{
    fault_factory.finalize();
    fault_fixes_factory.finalize();

    // actualize the requirements
    for( const fault_fix &const_fix : fault_fixes_factory.get_all() ) {
        fault_fix &fix = const_cast<fault_fix &>( const_fix );
        fix.finalize();
    }
    for( const fault &f : fault_factory.get_all() ) {
        if( !f.type().empty() ) {
            faults_by_type[f.type()].emplace_back( f.id.str() );
        }
    }
    reqs_temp_storage.clear();
}

void faults::check_consistency()
{
    fault_factory.check();
    fault_fixes_factory.check();
}

/** @relates string_id */
template<>
bool string_id<fault>::is_valid() const
{
    return fault_factory.is_valid( *this );
}

/** @relates string_id */
template<>
const fault &string_id<fault>::obj() const
{
    return fault_factory.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<fault_fix>::is_valid() const
{
    return fault_fixes_factory.is_valid( *this );
}

/** @relates string_id */
template<>
const fault_fix &string_id<fault_fix>::obj() const
{
    return fault_fixes_factory.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<fault_group>::is_valid() const
{
    return fault_group_factory.is_valid( *this );
}

/** @relates string_id */
template<>
const fault_group &string_id<fault_group>::obj() const
{
    return fault_group_factory.obj( *this );
}


std::string fault::name() const
{
    return name_.translated();
}

std::string fault::description() const
{
    return description_.translated();
}

std::string fault::item_prefix() const
{
    return item_prefix_.translated();
}

std::string fault::item_suffix() const
{
    return item_suffix_.translated();
}

std::string fault::message() const
{
    return message_.translated();
}

double fault::price_mod() const
{
    return price_modifier;
}

int fault::degradation_mod() const
{
    return degradation_mod_;
}

std::vector<std::tuple<int, float, damage_type_id>> fault::melee_damage_mod() const
{
    return melee_damage_mod_;
}

std::vector<std::tuple<int, float, damage_type_id>> fault::armor_mod() const
{
    return armor_mod_;
}

bool fault::affected_by_degradation() const
{
    return affected_by_degradation_;
}

std::string fault::type() const
{
    return type_;
}

bool fault::has_flag( const std::string &flag ) const
{
    return flags.count( flag );
}

const std::set<fault_fix_id> &fault::get_fixes() const
{
    return fixes;
}

const std::set<fault_id> &fault::get_block_faults() const
{
    return block_faults;
}

void fault::load( const JsonObject &jo, std::string_view )
{

    mandatory( jo, was_loaded, "name", name_ );
    mandatory( jo, was_loaded, "description", description_ );
    optional( jo, was_loaded, "item_prefix", item_prefix_ );
    optional( jo, was_loaded, "item_suffix", item_suffix_ );
    optional( jo, was_loaded, "message", message_ );
    optional( jo, was_loaded, "fault_type", type_ );
    optional( jo, was_loaded, "flags", flags );
    optional( jo, was_loaded, "block_faults", block_faults );
    optional( jo, was_loaded, "price_modifier", price_modifier, 1.0 );
    optional( jo, was_loaded, "degradation_mod", degradation_mod_, 0 );
    optional( jo, was_loaded, "affected_by_degradation", affected_by_degradation_, false );

    if( jo.has_array( "melee_damage_mod" ) ) {
        for( JsonObject jo_f : jo.get_array( "melee_damage_mod" ) ) {
            melee_damage_mod_.emplace_back(
                jo_f.get_int( "add", 0 ),
                static_cast<float>( jo_f.get_float( "multiply", 1.0f ) ),
                jo_f.get_string( "damage_id" ) );
        }
    }

    if( jo.has_array( "armor_mod" ) ) {
        for( JsonObject jo_f : jo.get_array( "armor_mod" ) ) {
            armor_mod_.emplace_back(
                jo_f.get_int( "add", 0 ),
                static_cast<float>( jo_f.get_float( "multiply", 1.0f ) ),
                jo_f.get_string( "damage_id" ) );
        }
    }
}

void fault::check() const
{
    if( name_.empty() ) {
        debugmsg( "fault '%s' has empty name", id.str() );
    }
    if( description_.empty() ) {
        debugmsg( "fault '%s' has empty description", id.str() );
    }
}

const requirement_data &fault_fix::get_requirements() const
{
    return *requirements;
}

void fault_fix::load( const JsonObject &jo, std::string_view )
{
    mandatory( jo, was_loaded, "name", name );
    optional( jo, was_loaded, "success_msg", success_msg );
    optional( jo, was_loaded, "time", time );
    optional( jo, was_loaded, "set_variables", set_variables );
    optional( jo, was_loaded, "adjust_variables_multiply", adjust_variables_multiply );
    optional( jo, was_loaded, "skills", skills );
    optional( jo, was_loaded, "faults_removed", faults_removed );
    optional( jo, was_loaded, "faults_added", faults_added );
    optional( jo, was_loaded, "mod_damage", mod_damage );
    optional( jo, was_loaded, "mod_degradation", mod_degradation );
    optional( jo, was_loaded, "time_save_profs", time_save_profs );
    optional( jo, was_loaded, "time_save_flags", time_save_flags );

    if( jo.has_array( "requirements" ) ) {
        for( const JsonValue &jv : jo.get_array( "requirements" ) ) {
            if( jv.test_array() ) {
                // array of 2 elements filled with [requirement_id, count]
                const JsonArray &req = static_cast<JsonArray>( jv );
                const std::string req_id = req.get_string( 0 );
                const int req_amount = req.get_int( 1 );
                reqs_temp_storage.emplace( id, std::make_pair( req_id, req_amount ) );
            } else if( jv.test_object() ) {
                // defining single requirement inline
                const JsonObject &req = static_cast<JsonObject>( jv );
                const requirement_id req_id( "fault_fix_" + id.str() + "_inline_req" );
                requirement_data::load_requirement( req, req_id );
                reqs_temp_storage.emplace( id, std::make_pair( req_id.str(), 1 ) );
            } else {
                debugmsg( "fault_fix '%s' has has invalid requirement element", id.str() );
            }
        }
    }
}

void fault_fix::finalize()
{
    const auto range = reqs_temp_storage.equal_range( id );
    for( auto it = range.first; it != range.second; ++it ) {
        const requirement_id req_id( it->second.first );
        const int amount = it->second.second;
        if( !req_id.is_valid() ) {
            debugmsg( "fault_fix '%s' has invalid requirement_id '%s'", id.str(), req_id.str() );
            continue;
        }
        *requirements = *requirements + ( *req_id ) * amount;
    }
    requirements->consolidate();
    for( const fault_id &fid : faults_removed ) {
        const_cast<fault &>( *fid ).fixes.emplace( id );
    }
}

void fault_fix::check() const
{
    if( time < 0_turns ) {
        debugmsg( "fault_fix '%s' has negative time", id.str() );
    }
    for( const auto &[skill_id, lvl] : skills ) {
        if( !skill_id.is_valid() ) {
            debugmsg( "fault_fix %s requires unknown skill '%s'", id.str(), skill_id.str() );
        }
        if( lvl <= 0 ) {
            debugmsg( "fault_fix '%s' requires negative level of skill '%s'", id.str(), skill_id.str() );
        }
    }
    for( const fault_id &fault_id : faults_removed ) {
        if( !fault_id.is_valid() ) {
            debugmsg( "fault_fix '%s' has invalid fault_id '%s' in 'faults_removed' field",
                      id.str(), fault_id.str() );
        }
    }
    for( const fault_id &fault_id : faults_added ) {
        if( !fault_id.is_valid() ) {
            debugmsg( "fault_fix '%s' has invalid fault_id '%s' in 'faults_added' field",
                      id.str(), fault_id.str() );
        }
    }
    for( const auto &[proficiency_id, mult] : time_save_profs ) {
        if( !proficiency_id.is_valid() ) {
            debugmsg( "fault_fix %s has unknown proficiency_id '%s'",
                      id.str(), proficiency_id.str() );
        }
        if( mult < 0 ) {
            debugmsg( "fault_fix '%s' has negative mend time if possessing proficiency '%s'",
                      id.str(), proficiency_id.str() );
        }
    }
    for( const auto &[flag_id, mult] : time_save_flags ) {
        if( !flag_id.is_valid() ) {
            debugmsg( "fault_fix %s has unknown flag_id '%s'",
                      id.str(), flag_id.str() );
        }
        if( mult < 0 ) {
            debugmsg( "fault_fix '%s' has negative mend time if item possesses flag '%s'",
                      id.str(), flag_id.str() );
        }
    }
}

weighted_int_list<fault_id> fault_group::get_weighted_list() const
{
    return fault_weighted_list;
}

void fault_group::load( const JsonObject &jo, std::string_view )
{
    if( jo.has_array( "group" ) ) {
        for( const JsonObject jog : jo.get_array( "group" ) ) {
            fault_weighted_list.add( fault_id( jog.get_string( "fault" ) ), jog.get_int( "weight", 100 ) );
        }

    }
}
