#include <sstream>
#include <string>
#include <utility>

#include "cata_catch.h"
#include "cata_variant.h"
#include "character_id.h"
#include "debug_menu.h"
#include "enum_conversions.h"
#include "flexbuffer_json.h"
#include "json.h"
#include "json_loader.h"
#include "point.h"
#include "type_id.h"

static const itype_id itype_anvil( "anvil" );

static const mtype_id This_is_not_a_valid_id( "This is not a valid id" );

static const mtype_id mon_zombie( "mon_zombie" );
static const mtype_id zombie( "zombie" );

TEST_CASE( "variant_construction", "[variant][nogame]" )
{
    SECTION( "itype_id" ) {
        cata_variant v = cata_variant::make<cata_variant_type::itype_id>( itype_anvil );
        CHECK( v.type() == cata_variant_type::itype_id );
        CHECK( v.get<cata_variant_type::itype_id>() == itype_anvil );
        CHECK( v.get<itype_id>() == itype_anvil );
        cata_variant v2 = cata_variant( itype_anvil );
        CHECK( v2.type() == cata_variant_type::itype_id );
        CHECK( v2.get<cata_variant_type::itype_id>() == itype_anvil );
        CHECK( v2.get<itype_id>() == itype_anvil );
        itype_id anvil( "anvil" );
        cata_variant v3( anvil );
        CHECK( v3.type() == cata_variant_type::itype_id );
        CHECK( v3.get<cata_variant_type::itype_id>() == itype_anvil );
        CHECK( v3.get<itype_id>() == itype_anvil );
    }
    SECTION( "mtype_id" ) {
        cata_variant v = cata_variant::make<cata_variant_type::mtype_id>( zombie );
        CHECK( v.type() == cata_variant_type::mtype_id );
        CHECK( v.get<cata_variant_type::mtype_id>() == zombie );
        CHECK( v.get<mtype_id>() == zombie );
        cata_variant v2 = cata_variant( zombie );
        CHECK( v2.type() == cata_variant_type::mtype_id );
        CHECK( v2.get<cata_variant_type::mtype_id>() == zombie );
        CHECK( v2.get<mtype_id>() == zombie );
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

TEST_CASE( "variant_copy_move", "[variant][nogame]" )
{
    cata_variant v = cata_variant( zombie );
    cata_variant v2 = v;
    CHECK( v2.get<mtype_id>() == zombie );
    cata_variant v3( v );
    CHECK( v3.get<mtype_id>() == zombie );
    cata_variant v4( std::move( v ) );
    CHECK( v4.get<mtype_id>() == zombie );
    cata_variant v5( std::move( v2 ) );
    CHECK( v5.get<mtype_id>() == zombie );
}

TEST_CASE( "variant_type_name_round_trip", "[variant][nogame]" )
{
    int num_types = static_cast<int>( cata_variant_type::num_types );
    for( int i = 0; i < num_types; ++i ) {
        cata_variant_type type = static_cast<cata_variant_type>( i );
        std::string type_as_string = io::enum_to_string( type );
        CHECK( io::string_to_enum<cata_variant_type>( type_as_string ) == type );
    }
}

TEST_CASE( "variant_default_constructor", "[variant][nogame]" )
{
    cata_variant v;
    CHECK( v.type() == cata_variant_type::void_ );
    CHECK( v.get_string().empty() );
}

TEST_CASE( "variant_serialization", "[variant][nogame]" )
{
    cata_variant v = cata_variant( zombie );
    std::ostringstream os;
    JsonOut jsout( os );
    v.serialize( jsout );
    CHECK( os.str() == R"(["mtype_id","zombie"])" );
}

TEST_CASE( "variant_deserialization", "[variant][nogame]" )
{
    JsonValue jsin = json_loader::from_string( R"(["mtype_id","zombie"])" );
    cata_variant v;
    v.deserialize( jsin );
    CHECK( v == cata_variant( zombie ) );
}

TEST_CASE( "variant_from_string", "[nogame]" )
{
    cata_variant v = cata_variant::from_string( cata_variant_type::mtype_id, "mon_zombie" );
    CHECK( v == cata_variant( mon_zombie ) );
}

TEST_CASE( "variant_type_for", "[variant][nogame]" )
{
    CHECK( cata_variant_type_for<bool>() == cata_variant_type::bool_ );
    CHECK( cata_variant_type_for<int>() == cata_variant_type::int_ );
    CHECK( cata_variant_type_for<point>() == cata_variant_type::point );
    CHECK( cata_variant_type_for<trait_id>() == cata_variant_type::trait_id );
    CHECK( cata_variant_type_for<ter_id>() == cata_variant_type::ter_id );
}

TEST_CASE( "variant_is_valid", "[variant]" )
{
    // A string_id
    CHECK( cata_variant( mon_zombie ).is_valid() );
    CHECK_FALSE( cata_variant( This_is_not_a_valid_id ).is_valid() );

    // An int_id
    CHECK( cata_variant( ter_id( "t_grass" ) ).is_valid() );
    CHECK_FALSE( cata_variant::from_string( cata_variant_type::ter_id, "invalid id" ).is_valid() );

    // An enum
    CHECK( cata_variant( debug_menu::debug_menu_index::WISH ).is_valid() );
    CHECK_FALSE( cata_variant::from_string(
                     cata_variant_type::debug_menu_index, "invalid enum" ).is_valid() );
}
