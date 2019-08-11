#pragma once
#ifndef CATA_TESTS_STRINGMAKER_H
#define CATA_TESTS_STRINGMAKER_H

#include "catch/catch.hpp"
#include "item.h"

// StringMaker specializations for Cata types for reporting via Catch2 macros

namespace Catch
{

template<>
struct StringMaker<item> {
    static std::string convert( const item &i ) {
        return string_format( "item( \"%s\" )", i.typeId() );
    }
};

template<>
struct StringMaker<rectangle> {
    static std::string convert( const rectangle &r ) {
        return string_format( "[%s-%s]", r.p_min.to_string(), r.p_max.to_string() );
    }
};

template<>
struct StringMaker<box> {
    static std::string convert( const box &b ) {
        return string_format( "[%s-%s]", b.p_min.to_string(), b.p_max.to_string() );
    }
};

} // namespace Catch

#endif // CATA_TESTS_STRINGMAKER_H
