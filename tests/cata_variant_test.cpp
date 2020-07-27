#include <sstream>
#include <string>
#include <utility>

#include "cata_variant.h"
#include "catch/catch.hpp"
#include "character_id.h"
#include "enum_conversions.h"
#include "item.h"
#include "json.h"
#include "string_id.h"
#include "type_id.h"

TEST_CASE( "variant_construction", "[variant]" )
{
    SECTION( "itype_id" ) {
        cata_variant v = cata_variant::make<cata_variant_type::itype_id>( itype_id( "anvil" ) );
        CHECK( v.type() == cata_variant_type::itype_id );
        CHECK( v.get<cata_variant_type::itype_id>() == itype_id( "anvil" ) );
        CHECK( v.get<itype_id>() == itype_id( "anvil" ) );
        cata_variant v2 = cata_variant( itype_id( "anvil" ) );
        CHECK( v2.type() == cata_variant_type::itype_id );
        CHECK( v2.get<cata_variant_type::itype_id>() == itype_id( "anvil" ) );
        CHECK( v2.get<itype_id>() == itype_id( "anvil" ) );
        itype_id anvil( "anvil" );
        cata_variant v3( anvil );
        CHECK( v3.type() == cata_variant_type::itype_id );
        CHECK( v3.get<cata_variant_type::itype_id>() == itype_id( "anvil" ) );
        CHECK( v3.get<itype_id>() == itype_id( "anvil" ) );
    }
    SECTION( "mtype_id" ) {
        cata_variant v = cata_variant::make<cata_variant_type::mtype_id>( mtype_id( "zombie" ) );
        CHECK( v.type() == cata_variant_type::mtype_id );
        CHECK( v.get<cata_variant_type::mtype_id>() == mtype_id( "zombie" ) );
        CHECK( v.get<mtype_id>() == mtype_id( "zombie" ) );
        cata_variant v2 = cata_variant( mtype_id( "zombie" ) );
        CHECK( v2.type() == cata_variant_type::mtype_id );
        CHECK( v2.get<cata_variant_type::mtype_id>() == mtype_id( "zombie" ) );
        CHECK( v2.get<mtype_id>() == mtype_id( "zombie" ) );
    }
    SECTION( "point" ) {
        point p( 7, 63 );
        cata_variant v = cata_variant::make<cata_variant_type::point>( p );
        CHECK( v.type() == cata_variant_type::point );
        CHECK( v.get<cata_variant_type::point>() == p );
        CHECK( v.get<point>() == p );
        CHECK( v.get_string() == p.to_string() );
    }
    SECTION( "construction_from_const_lvalue" ) {
        const character_id i;
        cata_variant v( i );
        CHECK( v.type() == cata_variant_type::character_id );
        CHECK( v.get<character_id>() == i );
    }
}

TEST_CASE( "variant_copy_move", "[variant]" )
{
    cata_variant v = cata_variant( mtype_id( "zombie" ) );
    cata_variant v2 = v;
    CHECK( v2.get<mtype_id>() == mtype_id( "zombie" ) );
    cata_variant v3( v );
    CHECK( v3.get<mtype_id>() == mtype_id( "zombie" ) );
    cata_variant v4( std::move( v ) );
    CHECK( v4.get<mtype_id>() == mtype_id( "zombie" ) );
    cata_variant v5( std::move( v2 ) );
    CHECK( v5.get<mtype_id>() == mtype_id( "zombie" ) );
}

TEST_CASE( "variant_type_name_round_trip", "[variant]" )
{
    int num_types = static_cast<int>( cata_variant_type::num_types );
    for( int i = 0; i < num_types; ++i ) {
        cata_variant_type type = static_cast<cata_variant_type>( i );
        std::string type_as_string = io::enum_to_string( type );
        CHECK( io::string_to_enum<cata_variant_type>( type_as_string ) == type );
    }
}

TEST_CASE( "variant_default_constructor", "[variant]" )
{
    cata_variant v;
    CHECK( v.type() == cata_variant_type::void_ );
    CHECK( v.get_string().empty() );
}

TEST_CASE( "variant_serialization", "[variant]" )
{
    cata_variant v = cata_variant( mtype_id( "zombie" ) );
    std::ostringstream os;
    JsonOut jsout( os );
    v.serialize( jsout );
    CHECK( os.str() == R"(["mtype_id","zombie"])" );
}

TEST_CASE( "variant_deserialization", "[variant]" )
{
    std::istringstream is( R"(["mtype_id","zombie"])" );
    JsonIn jsin( is );
    cata_variant v;
    v.deserialize( jsin );
    CHECK( v == cata_variant( mtype_id( "zombie" ) ) );
}
