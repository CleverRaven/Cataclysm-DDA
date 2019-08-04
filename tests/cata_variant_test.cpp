#include "catch/catch.hpp"

#include "cata_variant.h"

TEST_CASE( "variant_construction", "[variant]" )
{
    SECTION( "itype_id" ) {
        cata_variant v = cata_variant::make<cata_variant_type::itype_id>( itype_id( "anvil" ) );
        CHECK( v.get<cata_variant_type::itype_id>() == itype_id( "anvil" ) );
        CHECK( v.get<itype_id>() == itype_id( "anvil" ) );
    }
    SECTION( "mtype_id" ) {
        cata_variant v = cata_variant::make<cata_variant_type::mtype_id>( mtype_id( "zombie" ) );
        CHECK( v.get<cata_variant_type::mtype_id>() == mtype_id( "zombie" ) );
        CHECK( v.get<mtype_id>() == mtype_id( "zombie" ) );
    }
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
