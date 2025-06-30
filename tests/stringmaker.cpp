#include "calendar.h"
#include "cata_variant.h"
#include "dialogue.h"
#include "enum_conversions.h"
#include "item.h"
#include "line.h"
#include "point.h"
#include "stringmaker.h"

// StringMaker specializations for Cata types for reporting via Catch2 macros

namespace Catch
{

std::string StringMaker<item>::convert( const item &i )
{
    return string_format( "item( itype_id( \"%s\" ) )", i.typeId().str() );
}

std::string StringMaker<point>::convert( const point &p )
{
    return string_format( "point( %d, %d )", p.x, p.y );
}

std::string StringMaker<rl_vec2d>::convert( const rl_vec2d &p )
{
    return string_format( "rl_vec2d( %f, %f )", p.x, p.y );
}

std::string StringMaker<cata_variant>::convert( const cata_variant &v )
{
    return string_format( "cata_variant<%s>(\"%s\")",
                          io::enum_to_string( v.type() ), v.get_string() );
}

std::string StringMaker<time_duration>::convert( const time_duration &d )
{
    return string_format( "time_duration( %d ) [%s]", to_turns<int>( d ), to_string( d ) );
}

std::string StringMaker<time_point>::convert( const time_point &d )
{
    return string_format(
               "time_point( %d ) [%s]", to_turns<int>( d - calendar::turn_zero ), to_string( d ) );
}

std::string StringMaker<talk_response>::convert( const talk_response &r )
{
    return string_format( "talk_response( text=\"%s\" )", r.text );
}

} // namespace Catch
