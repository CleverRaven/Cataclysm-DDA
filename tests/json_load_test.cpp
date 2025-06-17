#include "damage.h"
#include "cata_catch.h"
#include "itype.h"

static const damage_type_id damage_cut( "cut" );
static const damage_type_id damage_heat( "heat" );

static const itype_id itype_test_copyfrom_base( "test_copyfrom_base" );
static const itype_id itype_test_copyfrom_overwrite( "test_copyfrom_overwrite" );

TEST_CASE( "damage_instance_load_does_not_extend", "[json][damage][load]" )
{
    REQUIRE( itype_test_copyfrom_base.is_valid() );
    REQUIRE( itype_test_copyfrom_overwrite.is_valid() );


    REQUIRE( itype_test_copyfrom_base->gun );
    REQUIRE( itype_test_copyfrom_overwrite->gun );

    const damage_instance &base = itype_test_copyfrom_base->gun->damage;
    const damage_instance &overwrite = itype_test_copyfrom_overwrite->gun->damage;
    REQUIRE_FALSE( base.empty() );
    REQUIRE_FALSE( overwrite.empty() );

    CHECK_FALSE( base == overwrite );

    CHECK( base.total_damage() == 3.f );
    CHECK( base.type_damage( damage_heat ) == 3.f );
    CHECK( base.type_damage( damage_cut ) == 0.f );

    CHECK( overwrite.total_damage() == 5.f );
    CHECK( overwrite.type_damage( damage_cut ) == 5.f );
    CHECK( overwrite.type_damage( damage_heat ) == 0.f );
}
