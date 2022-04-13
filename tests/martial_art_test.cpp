#include "catch/catch.hpp"
#include "player_helpers.h"

#include "avatar.h"
#include "character_martial_arts.h"
#include "martialarts.h"
#include "npc.h"

static const itype_id itype_test_weapon1( "test_weapon1" );
static const itype_id itype_test_weapon2( "test_weapon2" );

static const matype_id test_style_ma1( "test_style_ma1" );

static constexpr tripoint dude_pos( HALF_MAPSIZE_X, HALF_MAPSIZE_Y, 0 );

TEST_CASE( "martial arts", "[martial_arts]" )
{
    SECTION( "martial art valid weapon" ) {
        GIVEN( "a weapon that fits the martial art" ) {
            CHECK( test_style_ma1->has_weapon( itype_test_weapon1 ) );
        }

        GIVEN( "a weapon that fits the martial art weapon category" ) {
            CHECK( test_style_ma1->has_weapon( itype_test_weapon2 ) );
        }
    }
}

TEST_CASE( "Martial art required weapon categories", "[martial_arts]" )
{
    SECTION( "Weapon categories required for buff" ) {
        REQUIRE( !test_style_ma1->onmiss_buffs.empty() );
        const mabuff_id &buff1 = test_style_ma1->onmiss_buffs[0];
        const mabuff_id &buff2 = test_style_ma1->onmiss_buffs[1];

        REQUIRE( buff1->reqs.weapon_categories_allowed[0] == *test_style_ma1->weapon_category.begin() );
        standard_npc dude( "TestCharacter", dude_pos, {}, 0, 8, 8, 8, 8 );
        item weap2( itype_test_weapon2 );
        clear_character( dude, true );
        CHECK( !buff1->is_valid_character( dude ) );
        CHECK( !buff2->is_valid_character( dude ) );
        dude.wield( weap2 );
        dude.martial_arts_data->add_martialart( test_style_ma1 );
        dude.martial_arts_data->set_style( test_style_ma1, false );
        CHECK( buff1->is_valid_character( dude ) );
        CHECK( !buff2->is_valid_character( dude ) );
    }

    SECTION( "Weapon categories required for technique" ) {
        REQUIRE( !test_style_ma1->techniques.empty() );
        const matec_id &tec = *test_style_ma1->techniques.begin();

        REQUIRE( tec->reqs.weapon_categories_allowed[0] == *test_style_ma1->weapon_category.begin() );
        standard_npc dude( "TestCharacter", dude_pos, {}, 0, 8, 8, 8, 8 );
        item weap2( itype_test_weapon2 );
        clear_character( dude, true );
        CHECK( !tec->is_valid_character( dude ) );
        dude.wield( weap2 );
        dude.martial_arts_data->add_martialart( test_style_ma1 );
        dude.martial_arts_data->set_style( test_style_ma1, false );
        CHECK( tec->is_valid_character( dude ) );
    }
}
