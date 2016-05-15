#include "fault.h"

#include "debug.h"
#include "translations.h"

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
    f.name_ = _( jo.get_string( "name" ).c_str() );
    f.description_ = _( jo.get_string( "description" ).c_str() );

    auto sk = jo.get_array( "skills" );
    while( sk.has_more() ) {
        auto cur = sk.next_array();
        f.skills_.emplace( skill_id( cur.get_string( 0 ) ) , cur.size() >= 2 ? cur.get_int( 1 ) : 1 );
    }

    auto req = jo.get_object( "requirements" );
    f.requirements_.load( req );

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
        f.second.requirements_.check_consistency( f.second.id_.c_str() );
    }
}
