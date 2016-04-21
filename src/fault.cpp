#include "fault.h"

#include "item.h"
#include "requirements.h"
#include "debug.h"

static std::map<fault_id, fault> faults_all;

template<>
bool string_id<fault>::is_valid() const
{
    return faults_all.count( *this );
}

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
    f.name_ = jo.get_string( "name" );
    f.description_ = jo.get_string( "description" );

    auto pt = jo.get_array( "parts" );
    while( pt.has_more() ) {
        auto cur = pt.next_array();
        f.parts_.emplace( cur.get_string( 0 ), cur.size() >= 2 ? cur.get_int( 1 ) : 1 );
    }

    auto qt = jo.get_array( "qualities" );
    while( qt.has_more() ) {
        auto cur = qt.next_array();
        f.qualities_.emplace( cur.get_string( 0 ), cur.size() >= 2 ? cur.get_int( 1 ) : 1 );
    }

    auto sk = jo.get_array( "skills" );
    while( sk.has_more() ) {
        auto cur = sk.next_array();
        f.skills_.emplace( skill_id( cur.get_string( 0 ) ) , cur.size() >= 2 ? cur.get_int( 1 ) : 1 );
    }

    if( faults_all.find( f.id_ ) != faults_all.end() ) {
        jo.throw_error( "parsed fault overwrites existing definition", "id" );
    } else {
        faults_all[ f.id_ ] = f;
        DebugLog( D_INFO, DC_ALL ) << "Loaded fault: " << f.name_;
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
        for( auto &e : f.second.skills_ ) {
            if( !e.first.is_valid() ) {
                debugmsg( "fault %s has unknown skill %s", f.second.id_.c_str(), e.first.c_str() );
            }
        }
        for( auto &e : f.second.qualities_ ) {
            if( !quality::has( e.first ) ) {
                debugmsg( "fault %s has unknown tool quality %s", f.second.id_.c_str(), e.first.c_str() );
            }
        }
        for( auto &e : f.second.parts_ ) {
            if( !item::type_is_defined( e.first ) ) {
                debugmsg( "fault %s has unknown item as part %s", f.second.id_.c_str(), e.first.c_str() );
            }
        }
    }
}