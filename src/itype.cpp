#include "itype.h"

#include <stdexcept>

#include "debug.h"
#include "output.h"
#include "player.h"
#include "translations.h"

std::string gunmod_location::name() const
{
    // Yes, currently the name is just the translated id.
    return _( _id.c_str() );
}

std::string itype::nname( unsigned int quantity ) const
{
    // Always use singular form for liquids.
    // (Maybe gases too?  There are no gases at the moment)
    if( phase == LIQUID ) {
        quantity = 1;
    }
    return ngettext( name.c_str(), name_plural.c_str(), quantity );
}

long itype::charges_per_volume( const units::volume &vol ) const
{
    if( volume == 0_ml ) {
        return item::INFINITE_CHARGES; // TODO: items should not have 0 volume at all!
    }
    return ( stackable ? stack_size : 1 ) * vol / volume;
}

// Members of iuse struct, which is slowly morphing into a class.
bool itype::has_use() const
{
    return !use_methods.empty();
}

bool itype::can_use( const std::string &iuse_name ) const
{
    return get_use( iuse_name ) != nullptr;
}

const use_function *itype::get_use( const std::string &iuse_name ) const
{
    auto iter = use_methods.find( iuse_name );
    return iter != use_methods.end() ? &iter->second : nullptr;
}

long itype::tick( player &p, item &it, const tripoint &pos ) const
{
    // Note: can go higher than current charge count
    // Maybe should move charge decrementing here?
    int charges_to_use = 0;
    for( auto &method : use_methods ) {
        int val = method.second.call( p, it, true, pos );
        if( charges_to_use < 0 || val < 0 ) {
            charges_to_use = -1;
        } else {
            charges_to_use += val;
        }
    }

    return charges_to_use;
}

long itype::invoke( player &p, item &it, const tripoint &pos ) const
{
    if( !has_use() ) {
        return 0;
    }
    return invoke( p, it, pos, use_methods.begin()->first );
}

long itype::invoke( player &p, item &it, const tripoint &pos, const std::string &iuse_name ) const
{
    const use_function *use = get_use( iuse_name );
    if( use == nullptr ) {
        debugmsg( "Tried to invoke %s on a %s, which doesn't have this use_function",
                  iuse_name.c_str(), nname( 1 ).c_str() );
        return 0;
    }

    const auto ret = use->can_call( p, it, false, pos );

    if( !ret.success() ) {
        p.add_msg_if_player( m_info, ret.str() );
        return 0;
    }

    return use->call( p, it, false, pos );
}

std::string gun_type_type::name() const
{
    return pgettext( "gun_type_type", name_.c_str() );
}
