#include "fault.h"

#include "debug.h"
#include "json.h"
#include "requirements.h"
#include "translations.h"

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

void fault::load_fault( JsonObject &jo )
{
    fault f;

    f.id_ = fault_id( jo.get_string( "id" ) );
    f.name_ = _( jo.get_string( "name" ).c_str() );
    f.description_ = _( jo.get_string( "description" ).c_str() );

    f.time_ = jo.get_int( "time" );

    auto sk = jo.get_array( "skills" );
    while( sk.has_more() ) {
        auto cur = sk.next_array();
        f.skills_.emplace( skill_id( cur.get_string( 0 ) ), cur.size() >= 2 ? cur.get_int( 1 ) : 1 );
    }

    if( jo.has_string( "requirements" ) ) {
        f.requirements_ = requirement_id( jo.get_string( "requirements" ) );

    } else {
        auto req = jo.get_object( "requirements" );
        const requirement_id req_id( std::string( "inline_fault_" ) + f.id_.str() );
        requirement_data::load_requirement( req, req_id );
        f.requirements_ = req_id;
    }

    if( faults_all.find( f.id_ ) != faults_all.end() ) {
        jo.throw_error( "parsed fault overwrites existing definition", "id" );
    } else {
        faults_all[ f.id_ ] = f;
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
        if( f.second.time_ < 0 ) {
            debugmsg( "fault %s has negative time requirement", f.second.id_.c_str() );
        }
        for( auto &e : f.second.skills_ ) {
            if( !e.first.is_valid() ) {
                debugmsg( "fault %s has unknown skill %s", f.second.id_.c_str(), e.first.c_str() );
            }
        }
        if( !f.second.requirements_.is_valid() ) {
            debugmsg( "fault %s has missing requirement data %s",
                      f.second.id_.c_str(), f.second.requirements_.c_str() );
        }
    }
}
