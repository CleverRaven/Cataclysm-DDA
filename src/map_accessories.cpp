#include "map_accessories.h"

#include <algorithm>

#include "damage.h"
#include "debug.h"
#include "generic_factory.h"

namespace
{
generic_factory<bash_damage_profile> damage_profile_factory( "bash_damage_profile" );
} // namespace

// boilerplate
void bash_damage_profile::load_all( const JsonObject &jo, const std::string &src )
{
    damage_profile_factory.load( jo, src );
}

void bash_damage_profile::finalize_all()
{
    damage_profile_factory.finalize();
    for( bash_damage_profile &profile : damage_profile_factory.get_all_mod() ) {
        profile.finalize();
    }
}

void bash_damage_profile::check_all()
{
    damage_profile_factory.check();
}

void bash_damage_profile::reset()
{
    damage_profile_factory.reset();
}

template<>
const bash_damage_profile &string_id<bash_damage_profile>::obj() const
{
    return damage_profile_factory.obj( *this );
}
// end boilerplate

void bash_damage_profile::load( const JsonObject &jo, const std::string_view )
{
    mandatory( jo, was_loaded, "profile", profile );
}

void bash_damage_profile::check() const
{
    for( const std::pair<const damage_type_id, double> &pr : profile ) {
        if( !pr.first.is_valid() ) {
            debugmsg( "Invalid damage type %s in %s.", pr.first.str(), id.str() );
            continue;
        }
        if( pr.second < 0.0 ) {
            debugmsg( "For damage type %s in %s, susceptability is < 0.", pr.first.str(), id.str() );
        }
    }
}

void bash_damage_profile::finalize()
{
    for( const damage_type &dam : damage_type::get_all() ) {
        // emplace fails if it's already in the map
        profile.emplace( dam.id, dam.bash_conversion_factor );
    }
}

int bash_damage_profile::damage_from( const std::map<damage_type_id, int> &str, int armor ) const
{
    int ret = 0;
    for( const std::pair<const damage_type_id, int> &type : str ) {
        if( profile.find( type.first ) == profile.end() ) {
            debugmsg( "Damage type %s not included in damage profile?", type.first.str() );
            continue;
        }
        // finalize ensures any valid damage_type_id is in profile
        ret += std::max<int>( 0, ( type.second * profile.at( type.first ) ) - armor );
    }
    return ret;
}
