#include "fault.h"

#include <utility>

#include "debug.h"
#include "generic_factory.h"
#include "json.h"
#include "requirements.h"

static std::map<fault_id, fault> faults_all;
static std::map<fault_fix_id, fault_fix> fault_fixes_all;

/** @relates string_id */
template<>
bool string_id<fault>::is_valid() const
{
    return faults_all.count( *this );
}

/** @relates string_id */
template<>
const fault &string_id<fault>::obj() const
{
    const auto found = faults_all.find( *this );
    if( found == faults_all.end() ) {
        debugmsg( "Tried to get invalid fault: %s", c_str() );
        static const fault null_fault{};
        return null_fault;
    }
    return found->second;
}

/** @relates string_id */
template<>
bool string_id<fault_fix>::is_valid() const
{
    return fault_fixes_all.count( *this );
}

/** @relates string_id */
template<>
const fault_fix &string_id<fault_fix>::obj() const
{
    const auto found = fault_fixes_all.find( *this );
    if( found == fault_fixes_all.end() ) {
        debugmsg( "Tried to get invalid fault fix: %s", c_str() );
        static const fault_fix null_fault_fix{};
        return null_fault_fix;
    }
    return found->second;
}

const fault_id &fault::id() const
{
    return id_;
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

bool fault::has_flag( const std::string &flag ) const
{
    return flags.count( flag );
}

const std::set<fault_fix_id> &fault::get_fixes() const
{
    return fixes;
}

void fault::load( const JsonObject &jo )
{
    fault f;

    mandatory( jo, false, "id", f.id_ );
    mandatory( jo, false, "name", f.name_ );
    mandatory( jo, false, "description", f.description_ );
    optional( jo, false, "item_prefix", f.item_prefix_ );
    optional( jo, false, "flags", f.flags );

    if( !faults_all.emplace( f.id_, f ).second ) {
        jo.throw_error_at( "id", "parsed fault overwrites existing definition" );
    }
}

const std::map<fault_id, fault> &fault::all()
{
    return faults_all;
}

void fault::reset()
{
    faults_all.clear();
}

void fault::check_consistency()
{
    for( const auto& [fault_id, fault] : faults_all ) {
        if( fault.name_.empty() ) {
            debugmsg( "fault '%s' has empty name", fault_id.str() );
        }
        if( fault.description_.empty() ) {
            debugmsg( "fault '%s' has empty description", fault_id.str() );
        }
    }
}

const std::map<fault_fix_id, fault_fix> &fault_fix::all()
{
    return fault_fixes_all;
}

const requirement_data &fault_fix::get_requirements() const
{
    return *requirements;
}

// we'll store requirement_ids here and wait for requirements to load in, then we can actualize them
static std::multimap<fault_fix_id, std::pair<std::string, int>> reqs_temp_storage;

void fault_fix::load( const JsonObject &jo )
{
    fault_fix f;

    assign( jo, "id", f.id_, true );
    assign( jo, "name", f.name, true );
    assign( jo, "success_msg", f.success_msg, true );
    assign( jo, "time", f.time, false );
    assign( jo, "set_variables", f.set_variables, true );
    assign( jo, "skills", f.skills, true );
    assign( jo, "faults_removed", f.faults_removed );
    assign( jo, "faults_added", f.faults_added );
    assign( jo, "mod_damage", f.mod_damage );
    assign( jo, "mod_degradation", f.mod_degradation );
    assign( jo, "time_save_profs", f.time_save_profs, false );
    assign( jo, "time_save_flags", f.time_save_flags, false );

    if( jo.has_array( "requirements" ) ) {
        for( const JsonValue &jv : jo.get_array( "requirements" ) ) {
            if( jv.test_array() ) {
                // array of 2 elements filled with [requirement_id, count]
                const JsonArray &req = static_cast<JsonArray>( jv );
                const std::string req_id = req.get_string( 0 );
                const int req_amount = req.get_int( 1 );
                reqs_temp_storage.emplace( f.id_, std::make_pair( req_id, req_amount ) );
            } else if( jv.test_object() ) {
                // defining single requirement inline
                const JsonObject &req = static_cast<JsonObject>( jv );
                const requirement_id req_id( "fault_fix_" + f.id_.str() + "_inline_req" );
                requirement_data::load_requirement( req, req_id );
                reqs_temp_storage.emplace( f.id_, std::make_pair( req_id.str(), 1 ) );
            } else {
                debugmsg( "fault_fix '%s' has has invalid requirement element", f.id_.str() );
            }
        }
    }
    fault_fixes_all.emplace( f.id_, f );
}

void fault_fix::reset()
{
    fault_fixes_all.clear();
}

void fault_fix::finalize()
{
    for( auto& [fix_id, fix] : fault_fixes_all ) {
        const auto range = reqs_temp_storage.equal_range( fix_id );
        for( auto it = range.first; it != range.second; ++it ) {
            const requirement_id req_id( it->second.first );
            const int amount = it->second.second;
            if( !req_id.is_valid() ) {
                debugmsg( "fault_fix '%s' has invalid requirement_id '%s'", fix_id.str(), req_id.str() );
                continue;
            }
            *fix.requirements = *fix.requirements + ( *req_id ) * amount;
        }
        fix.requirements->consolidate();
        for( const fault_id &fid : fix.faults_removed ) {
            const_cast<fault &>( *fid ).fixes.emplace( fix_id );
        }
    }
    reqs_temp_storage.clear();
}

void fault_fix::check_consistency()
{
    for( auto &[fix_id, fix] : fault_fixes_all ) {
        if( fix.time < 0_turns ) {
            debugmsg( "fault_fix '%s' has negative time", fix_id.str() );
        }
        for( const auto &[skill_id, lvl] : fix.skills ) {
            if( !skill_id.is_valid() ) {
                debugmsg( "fault_fix %s requires unknown skill '%s'",
                          fix_id.str(), skill_id.str() );
            }
            if( lvl <= 0 ) {
                debugmsg( "fault_fix '%s' requires negative level of skill '%s'",
                          fix_id.str(), skill_id.str() );
            }
        }
        for( const fault_id &fault_id : fix.faults_removed ) {
            if( !fault_id.is_valid() ) {
                debugmsg( "fault_fix '%s' has invalid fault_id '%s' in 'faults_removed' field",
                          fix_id.str(), fault_id.str() );
            }
        }
        for( const fault_id &fault_id : fix.faults_added ) {
            if( !fault_id.is_valid() ) {
                debugmsg( "fault_fix '%s' has invalid fault_id '%s' in 'faults_added' field",
                          fix_id.str(), fault_id.str() );
            }
        }
        for( const auto &[proficiency_id, mult] : fix.time_save_profs ) {
            if( !proficiency_id.is_valid() ) {
                debugmsg( "fault_fix %s has unknown proficiency_id '%s'",
                          fix_id.str(), proficiency_id.str() );
            }
            if( mult < 0 ) {
                debugmsg( "fault_fix '%s' has negative mend time if possessing proficiency '%s'",
                          fix_id.str(), proficiency_id.str() );
            }
        }
        for( const auto &[flag_id, mult] : fix.time_save_flags ) {
            if( !flag_id.is_valid() ) {
                debugmsg( "fault_fix %s has unknown flag_id '%s'",
                          fix_id.str(), flag_id.str() );
            }
            if( mult < 0 ) {
                debugmsg( "fault_fix '%s' has negative mend time if item possesses flag '%s'",
                          fix_id.str(), flag_id.str() );
            }
        }
    }
}
