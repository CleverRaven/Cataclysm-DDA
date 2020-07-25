#include <list>
#include <memory>
#include <string>

#include "avatar.h"
#include "calendar.h"
#include "catch/catch.hpp"
#include "creature.h"
#include "game.h"
#include "item.h"
#include "map_helpers.h"
#include "monster.h"
#include "mtype.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"

// The test cases below cover polymorphic functions related to melee hit and dodge rates
// for the Character, player, and monster classes, including:
//
// - Character::get_hit_base, monster::get_hit_base
// - Character::get_dodge_base, monster::get_dodge_base
// - player::get_dodge, monster::get_dodge

static const efftype_id effect_grabbed( "grabbed" );
static const efftype_id effect_grabbing( "grabbing" );

TEST_CASE( "Monster losing grabbing effect", "[player][melee][grab]" )
{
    clear_map();

    avatar &dummy = g->u;
    clear_character( dummy );

    // Four nearby spots
    tripoint mon_pos = dummy.pos() + tripoint_north;

    // Grabbed by a monster
    monster *zed = g->place_critter_at( mtype_id( "debug_mon" ), mon_pos );

    REQUIRE( g->critter_at<monster>( mon_pos ) );

    // Get grabbed
    dummy.add_effect( efftype_id( "grabbed" ), 1_minutes );
    REQUIRE( dummy.has_effect( efftype_id( "grabbed" ) ) );

    zed->add_effect( efftype_id( "grabbing" ), 1_minutes );
    REQUIRE( zed->has_effect( efftype_id( "grabbing" ) ) );

    SECTION( "Monster is bashed for over 10% its hp" ) {
        damage_type dt = DT_BASH;
        bodypart_str_id bp( "torso" );
        damage_instance di( dt, ( zed->get_hp_max() / 5 ) + zed->get_armor_type( dt, bp ) );
        zed->deal_damage( nullptr, bp, di );
        REQUIRE( zed->get_hp() <= zed->get_hp_max() * 9 / 10 );

        THEN( "Monster is no longer grabbing" ) {
            CHECK( !zed->has_effect( efftype_id( "grabbing" ) ) );
        }
    }

    SECTION( "Monster is burned for over 10% its hp" ) {
        damage_type dt = DT_HEAT;
        bodypart_str_id bp( "torso" );
        damage_instance di( dt, ( zed->get_hp_max() / 5 ) + zed->get_armor_type( dt, bp ) );
        zed->deal_damage( nullptr, bp, di );
        REQUIRE( zed->get_hp() <= zed->get_hp_max() * 9 / 10 );

        THEN( "Monster is still grabbing" ) {
            CHECK( zed->has_effect( efftype_id( "grabbing" ) ) );
        }
    }

    SECTION( "Monster is bashed for 0 damage" ) {
        damage_type dt = DT_BASH;
        damage_instance di( dt, 0 );
        bodypart_str_id bp( "torso" );
        zed->deal_damage( nullptr, bp, di );
        REQUIRE( zed->get_hp() == zed->get_hp_max() );

        THEN( "Monster is still grabbing" ) {
            CHECK( zed->has_effect( efftype_id( "grabbing" ) ) );
        }
    }
}
