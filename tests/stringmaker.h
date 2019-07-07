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

} // namespace Catch

#endif // CATA_TESTS_STRINGMAKER_H
