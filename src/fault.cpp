#include "fault.h"

#include <utility>

#include "debug.h"
#include "generic_factory.h"
#include "json.h"
#include "requirements.h"
#include "translations.h"
#include "units.h"

static std::map<fault_id, fault> faults_all;

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

void fault::load_fault( const JsonObject &jo )
{
    fault f;

    mandatory( jo, false, "id", f.id_ );
    mandatory( jo, false, "name", f.name_ );
    mandatory( jo, false, "description", f.description_ );

    for( const JsonObject jo_method : jo.get_array( "mending_methods" ) ) {
        mending_method m;

        mandatory( jo_method, false, "id", m.id );
        optional( jo_method, false, "name", m.name, f.name_ );
        optional( jo_method, false, "description", m.description, f.description_ );
        mandatory( jo_method, false, "success_msg", m.success_msg );
        mandatory( jo_method, false, "time", m.time );

        for( const JsonObject jo_skill : jo_method.get_array( "skills" ) ) {
            skill_id sk_id;
            mandatory( jo_skill, false, "id", sk_id );
            m.skills.emplace( sk_id, jo_skill.get_int( "level" ) );
        }

        if( jo_method.has_string( "requirements" ) ) {
            mandatory( jo_method, false, "requirements", m.requirements );
        } else {
            JsonObject jo_req = jo_method.get_object( "requirements" );
            m.requirements = requirement_id( m.id );
            requirement_data::load_requirement( jo_req, m.requirements );
        }

        optional( jo_method, false, "turns_into", m.turns_into, cata::nullopt );
        optional( jo_method, false, "also_mends", m.also_mends, cata::nullopt );

        f.mending_methods_.emplace( m.id, m );
    }

    optional( jo, false, "flags", f.flags );

    if( faults_all.find( f.id_ ) != faults_all.end() ) {
        jo.throw_error( "parsed fault overwrites existing definition", "id" );
    } else {
        faults_all[f.id_] = f;
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
    for( const auto &f : faults_all ) {
        for( const auto &m : f.second.mending_methods_ ) {
            if( !m.second.requirements.is_valid() ) {
                debugmsg( "fault %s has invalid requirement id %s for mending method %s",
                          f.second.id_.str(), m.second.requirements.str(), m.first );
            }
            if( m.second.time < 0_turns ) {
                debugmsg( "fault %s requires negative mending time for mending method %s",
                          f.second.id_.str(), m.first );
            }
            for( const auto &sk : m.second.skills ) {
                if( !sk.first.is_valid() ) {
                    debugmsg( "fault %s requires unknown skill %s for mending method %s",
                              f.second.id_.str(), sk.first.str(), m.first );
                }
                if( sk.second <= 0 ) {
                    debugmsg( "fault %s requires non-positive level of skill %s for mending method %s",
                              f.second.id_.str(), sk.first.str(), m.first );
                }
            }
            if( m.second.turns_into && !m.second.turns_into->is_valid() ) {
                debugmsg( "fault %s has invalid turns_into fault id %s for mending method %s",
                          f.second.id_.str(), m.second.turns_into->str(), m.first );
            }
            if( m.second.also_mends && !m.second.also_mends->is_valid() ) {
                debugmsg( "fault %s has invalid also_mends fault id %s for mending method %s",
                          f.second.id_.str(), m.second.also_mends->str(), m.first );
            }
        }
    }
}
