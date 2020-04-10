#include "itype.h"

#include <utility>

#include "debug.h"
#include "player.h"
#include "translations.h"
#include "item.h"
#include "ret_val.h"
#include "rng.h"

struct tripoint;

std::string gunmod_location::name() const
{
    // Yes, currently the name is just the translated id.
    return _( _id );
}

namespace io
{
template<>
std::string enum_to_string<condition_type>( condition_type data )
{
    switch( data ) {
        case condition_type::FLAG:
            return "FLAG";
        case condition_type::COMPONENT_ID:
            return "COMPONENT_ID";
        case condition_type::num_condition_types:
            break;
    }
    debugmsg( "Invalid condition_type" );
    abort();
}
} // namespace io

std::string itype::nname( unsigned int quantity ) const
{
    // Always use singular form for liquids.
    // (Maybe gases too?  There are no gases at the moment)
    if( phase == LIQUID ) {
        quantity = 1;
    }
    return name.translated( quantity );
}

int itype::charges_per_volume( const units::volume &vol ) const
{
    if( volume == 0_ml ) {
        // TODO: items should not have 0 volume at all!
        return item::INFINITE_CHARGES;
    }
    return ( count_by_charges() ? stack_size : 1 ) * vol / volume;
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
    const auto iter = use_methods.find( iuse_name );
    return iter != use_methods.end() ? &iter->second : nullptr;
}

int itype::tick( player &p, item &it, const tripoint &pos ) const
{
    // Note: can go higher than current charge count
    // Maybe should move charge decrementing here?
    // also steps on charges_to_use function defined in itype.h,
    // and I wonder if that is what was meant to be used here
    int charges_to_use = 0;
    // looping through all the use_methods and asking them how
    // many charges they use... why?
    // also this makes them do whatever they do just by calling
    // the method - maybe the "mystery" bool has some deeper meaning
    // than "I called you automatically"
    for( auto &method : use_methods ) {
        const int val = method.second.call( p, it, true, pos );
        if( charges_to_use < 0 || val < 0 ) {
            charges_to_use = -1;
        } else {
            charges_to_use += val;
        }
    }

    return charges_to_use;
}

int itype::invoke( player &p, item &it, const tripoint &pos ) const
{
    if( !has_use() ) {
        return 0;
    }
    return invoke( p, it, pos, use_methods.begin()->first );
}

int itype::invoke( player &p, item &it, const tripoint &pos, const std::string &iuse_name ) const
{
    const use_function *use = get_use( iuse_name );
    if( use == nullptr ) {
        debugmsg( "Tried to invoke %s on a %s, which doesn't have this use_function",
                  iuse_name, nname( 1 ) );
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

int itype::power_draw_as_battery_charge( time_duration time ) const
{
    if( tool && tool->power_draw > 0 ) {
        float seconds = to_seconds<float>( time );
        // get charge (legacy battery unit) per period time from power_draw (milliwatts)
        // assuming a "disposable light battery" is an AA battery, it has 300 charges
        // so lets call 300 charges ~= 15kJ (2850mAh, 1.5v reference)
        // so 1 charge ~= .05kJ so draw 1 charge per 50,000mw each second
        float charges_consumed = ( static_cast<float>( tool->power_draw ) / 50000.0f ) * seconds;
        int charges_floored = std::floor<int>( charges_consumed );
        // ... except charges is an int so we lack the precision to do this directly
        // so for decimal amounts treat them as a 1 in X chance to deplete a charge
        return ( charges_floored + ( x_in_y( 1, 1 / charges_consumed - charges_floored ) ? 1 : 0 ) );

    }
    return 0; // we don't know, because the tool doesn't have power_draw
    // you should probably deal with charges_to_use instead
}


bool itype::can_have_charges() const
{
    if( count_by_charges() ) {
        return true;
    }
    if( tool && tool->max_charges > 0 ) {
        return true;
    }
    if( gun && gun->clip > 0 ) {
        return true;
    }
    if( item_tags.count( "CAN_HAVE_CHARGES" ) ) {
        return true;
    }
    return false;
}
