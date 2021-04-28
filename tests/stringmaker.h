#pragma once
#ifndef CATA_TESTS_STRINGMAKER_H
#define CATA_TESTS_STRINGMAKER_H

#include "cuboid_rectangle.h"
#include "cata_catch.h"
#include "cata_variant.h"
#include "dialogue.h"
#include "item.h"

// StringMaker specializations for Cata types for reporting via Catch2 macros

namespace Catch
{

template<typename T>
struct StringMaker<string_id<T>> {
    static std::string convert( const string_id<T> &i ) {
        return string_format( "string_id( \"%s\" )", i.str() );
    }
};

template<typename T>
struct StringMaker<int_id<T>> {
    static std::string convert( const int_id<T> &i ) {
        return string_format( "int_id( \"%s\" )", i.id().str() );
    }
};

template<>
struct StringMaker<item> {
    static std::string convert( const item &i ) {
        return string_format( "item( itype_id( \"%s\" ) )", i.typeId().str() );
    }
};

template<>
struct StringMaker<point> {
    static std::string convert( const point &p ) {
        return string_format( "point( %d, %d )", p.x, p.y );
    }
};

template<typename Point>
struct StringMaker<rectangle<Point>> {
    static std::string convert( const rectangle<Point> &r ) {
        return string_format( "[%s-%s]", r.p_min.to_string(), r.p_max.to_string() );
    }
};

template<typename Tripoint>
struct StringMaker<cuboid<Tripoint>> {
    static std::string convert( const cuboid<Tripoint> &b ) {
        return string_format( "[%s-%s]", b.p_min.to_string(), b.p_max.to_string() );
    }
};

template<>
struct StringMaker<cata_variant> {
    static std::string convert( const cata_variant &v ) {
        return string_format( "cata_variant<%s>(\"%s\")",
                              io::enum_to_string( v.type() ), v.get_string() );
    }
};

template<>
struct StringMaker<time_duration> {
    static std::string convert( const time_duration &d ) {
        return string_format( "time_duration( %d ) [%s]", to_turns<int>( d ), to_string( d ) );
    }
};

template<>
struct StringMaker<talk_response> {
    static std::string convert( const talk_response &r ) {
        return string_format( "talk_response( text=\"%s\" )", r.text );
    }
};

} // namespace Catch

#endif // CATA_TESTS_STRINGMAKER_H
