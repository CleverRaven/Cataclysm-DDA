#include "units.h"

#include <cstdint>

#include "calendar.h"
#include "json.h"
#include "string_formatter.h"
#include "translations.h"

namespace units
{
template<>
void volume::serialize( JsonOut &jsout ) const
{
    if( value_ % 1000 == 0 ) {
        jsout.write( string_format( "%d L", value_ / 1000 ) );
    } else {
        jsout.write( string_format( "%d ml", value_ ) );
    }
}

template<>
void volume::deserialize( const JsonValue &jv )
{
    *this = read_from_json_string( jv, units::volume_units );
}

template<>
void mass::serialize( JsonOut &jsout ) const
{
    if( value_ % 1000000 == 0 ) {
        jsout.write( string_format( "%d kg", value_ / 1000000 ) );
    } else if( value_ % 1000 == 0 ) {
        jsout.write( string_format( "%d g", value_ / 1000 ) );
    } else {
        jsout.write( string_format( "%d mg", value_ ) );
    }
}

template<>
void mass::deserialize( const JsonValue &jv )
{
    *this = read_from_json_string( jv, units::mass_units );
}

template<>
void money::deserialize( const JsonValue &jv )
{
    *this = read_from_json_string( jv, units::money_units );
}

template<>
void length::deserialize( const JsonValue &jv )
{
    *this = read_from_json_string( jv, units::length_units );
}

template<>
void specific_energy::serialize( JsonOut &jsout ) const
{
    jsout.write( string_format( "%f", value_ ) );
}

template<>
void specific_energy::deserialize( const JsonValue &jv )
{
    *this = units::from_joule_per_gram( std::stof( jv.get_string() ) );
}

template<>
void temperature::serialize( JsonOut &jsout ) const
{
    jsout.write( string_format( "%f", value_ ) );
}

template<>
void temperature::deserialize( const JsonValue &jv )
{
    *this = from_kelvin( std::stof( jv.get_string() ) );
}

template<>
void energy::serialize( JsonOut &jsout ) const
{
    if( value_ % 1000000 == 0 ) {
        jsout.write( string_format( "%d kJ", value_ / 1000000 ) );
    } else if( value_ % 1000 == 0 ) {
        jsout.write( string_format( "%d J", value_ / 1000 ) ) ;
    } else {
        jsout.write( string_format( "%d mJ", value_ ) );
    }
}

template<>
void energy::deserialize( const JsonValue &jv )
{
    *this = read_from_json_string( jv, units::energy_units );
}

template<>
void power::serialize( JsonOut &jsout ) const
{
    if( value_ % 1000000 == 0 ) {
        jsout.write( string_format( "%d kW", value_ / 1000000 ) ); //NOLINT
    } else if( value_ % 1000 == 0 ) {
        jsout.write( string_format( "%d W", value_ / 1000 ) ) ; //NOLINT
    } else {
        jsout.write( string_format( "%d mW", value_ ) ); //NOLINT
    }
}

template<>
void power::deserialize( const JsonValue &jv )
{
    *this = read_from_json_string( jv, units::power_units );
}

template<>
void angle::serialize( JsonOut &jsout ) const
{
    jsout.write( string_format( "%f rad", value_ ) );
}

template<>
void angle::deserialize( const JsonValue &jv )
{
    *this = read_from_json_string( jv, units::angle_units );
}

template<>
void ememory::serialize( JsonOut &jsout ) const
{
    jsout.write( string_format( "%d KB", value_ ) );
}
template<>
void ememory::deserialize( const JsonValue &jv )
{
    *this = read_from_json_string( jv, units::ememory_units );
}

std::string display( const units::ememory &v )
{
    //TODO: generic metric units
    int64_t ebytes = v.value();
    int i;
    int ipart;
    int64_t metric_factor;
    double ebytes_decimal;
    for( i = ememory_units.size() - 1; i > 0; i-- ) {
        metric_factor = std::pow( 10, 3 * i );
        if( ebytes >= metric_factor ) {
            ipart = ebytes / metric_factor;
            ebytes_decimal = static_cast<double>( ebytes ) / metric_factor;
            if( ebytes_decimal > 0.0f || i > 1 ) {
                return string_format( "%.1f %s", ebytes_decimal, units::ememory_units[i].first );
            } else {
                ebytes = ipart;
            }
            break;
        }
    }
    return string_format( "%d %s", ebytes, units::ememory_units[i].first );
}

std::string display( const units::energy &v )
{
    using value_type = units::energy::value_type;
    const value_type kj = units::to_kilojoule( v );
    const value_type j = units::to_joule( v );
    // at least 1 kJ and there is no fraction
    if( kj >= 1 && static_cast<double>( j ) / kj == 1000 ) {
        return std::to_string( kj ) + ' ' + pgettext( "energy unit: kilojoule", "kJ" );
    }
    const value_type mj = units::to_millijoule( v );
    // at least 1 J and there is no fraction
    if( j >= 1 && static_cast<double>( mj ) / j  == 1000 ) {
        return std::to_string( j ) + ' ' + pgettext( "energy unit: joule", "J" );
    }
    return std::to_string( mj ) + ' ' + pgettext( "energy unit: millijoule", "mJ" );
}

std::string display( const units::power v )
{
    const int kw = units::to_kilowatt( v );
    const int w = units::to_watt( v );
    // at least 1 kW and there is no fraction
    if( kw >= 1 && static_cast<float>( w ) / kw == 1000 ) {
        return std::to_string( kw ) + ' ' + pgettext( "energy unit: kilowatt", "kW" );
    }
    const int mw = units::to_milliwatt( v );
    // at least 1 W and there is no fraction
    if( w >= 1 && static_cast<float>( mw ) / w == 1000 ) {
        return std::to_string( w ) + ' ' + pgettext( "energy unit: watt", "W" );
    }
    return std::to_string( mw ) + ' ' + pgettext( "energy unit: milliwatt", "mW" );
}

units::energy operator*( const units::power &power, const time_duration &time )
{
    const int64_t mW = units::to_milliwatt<int64_t>( power );
    const int64_t seconds = to_seconds<int64_t>( time );
    return units::from_millijoule( mW * seconds );
}

units::energy operator*( const time_duration &time, const units::power &power )
{
    return power * time;
}

units::power operator/( const units::energy &energy, const time_duration &time )
{
    const int64_t mj = to_millijoule( energy );
    const int64_t seconds = to_seconds<int64_t>( time );
    return from_milliwatt( mj / seconds );
}

time_duration operator/( const units::energy &energy, const units::power &power )
{
    const int64_t mj = to_millijoule( energy );
    const int64_t mw = to_milliwatt( power );
    return time_duration::from_seconds( mj / mw );
}

units::temperature_delta operator-( const units::temperature &T1, const units::temperature &T2 )
{
    return from_kelvin_delta( to_kelvin( T1 ) - to_kelvin( T2 ) );
}

units::temperature operator+( const units::temperature &T, const units::temperature_delta &T_delta )
{
    return from_kelvin( to_kelvin( T ) + to_kelvin_delta( T_delta ) );
}

units::temperature operator-( const units::temperature &T, const units::temperature_delta &T_delta )
{
    return from_kelvin( to_kelvin( T ) - to_kelvin_delta( T_delta ) );
}

units::temperature operator+( const units::temperature_delta &T_delta, const units::temperature &T )
{
    return from_kelvin( to_kelvin( T ) + to_kelvin_delta( T_delta ) );
}

units::temperature &operator+=( units::temperature &T, const units::temperature_delta &T_delta )
{
    T = T + T_delta;
    return T;
}

} // namespace units
