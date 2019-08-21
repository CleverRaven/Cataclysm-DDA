#include "catch/catch.hpp"

#include "cata_variant.h"

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
        std::string type_as_string = cata_variant_detail::to_string( type );
        CHECK( cata_variant_detail::from_string( type_as_string ) == type );
    }
}
