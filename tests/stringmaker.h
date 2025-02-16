#pragma once
#ifndef CATA_TESTS_STRINGMAKER_H
#define CATA_TESTS_STRINGMAKER_H

#include <string>

#include "cata_catch.h"
#include "cuboid_rectangle.h"
#include "string_formatter.h"

template <typename T> class int_id;
template <typename T> class string_id;

// StringMaker specializations for Cata types for reporting via Catch2 macros

class cata_variant;
class item;
class time_duration;
class time_point;
struct point;
struct rl_vec2d;
struct talk_response;

namespace Catch
{

template<>
struct StringMaker<item> {
    static std::string convert( const item &i );
};

template<>
struct StringMaker<point> {
    static std::string convert( const point &p );
};

template<>
struct StringMaker<rl_vec2d> {
    static std::string convert( const rl_vec2d &p );
};

template<>
struct StringMaker<cata_variant> {
    static std::string convert( const cata_variant &v );
};

template<>
struct StringMaker<time_duration> {
    static std::string convert( const time_duration &d );
};

template <>
struct StringMaker<time_point> {
    static std::string convert( const time_point &d );
};

template <>
struct StringMaker<talk_response> {
    static std::string convert( const talk_response &r );
};

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

} // namespace Catch

#endif // CATA_TESTS_STRINGMAKER_H
