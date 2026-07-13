#include <vector>

#include "cata_catch.h"
#include "character.h"
#include "player_helpers.h"
#include "type_id.h"
#include "units.h"

static const trait_id trait_HUGE( "HUGE" );
static const trait_id trait_LARGE( "LARGE" );
static const trait_id trait_SMALL( "SMALL" );
static const trait_id trait_SMALL2( "SMALL2" );

static units::volume your_volume_with_trait( trait_id new_trait )
{
    clear_avatar();
    Character &you = get_player_character();
    you.set_mutation( new_trait );
    return you.get_base_volume();
}

static int your_height_with_trait( trait_id new_trait )
{
    clear_avatar();
    Character &you = get_player_character();
    you.set_mutation( new_trait );
    return you.height();
}

TEST_CASE( "character_baseline_volumes", "[volume]" )
{
    clear_avatar();
    Character &you = get_player_character();
    you.set_stored_kcal( you.get_healthy_kcal() );
    REQUIRE( you.get_mutations().empty() );
    REQUIRE( you.height() == 175 );
    REQUIRE( you.bodyweight() == 76562390_milligram );
    CHECK( you.get_base_volume() == 73485_ml );

    REQUIRE( your_height_with_trait( trait_SMALL2 ) == 70 );
    CHECK( your_volume_with_trait( trait_SMALL2 ) == 23326_ml );

    REQUIRE( your_height_with_trait( trait_SMALL ) == 122 );
    CHECK( your_volume_with_trait( trait_SMALL ) == 42476_ml );

    REQUIRE( your_height_with_trait( trait_LARGE ) == 227 );
    CHECK( your_volume_with_trait( trait_LARGE ) == 116034_ml );

    REQUIRE( your_height_with_trait( trait_HUGE ) == 280 );
    CHECK( your_volume_with_trait( trait_HUGE ) == 156228_ml );
}
