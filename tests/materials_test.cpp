#include <string>

#include "fire.h"
#include "item.h"
#include "cata_catch.h"
#include "npc.h"
#include "projectile.h"

static const material_id material_glass( "glass" );
static const material_id material_plastic( "plastic" );
static const material_id material_steel( "steel" );
static const material_id material_wood( "wood" );

static constexpr int num_iters = 1000;
static constexpr tripoint dude_pos( HALF_MAPSIZE_X, HALF_MAPSIZE_Y, 0 );
static constexpr tripoint target_pos( HALF_MAPSIZE_X - 10, HALF_MAPSIZE_Y, 0 );

static void check_near( const std::string &subject, float prob, const float expected,
                        const float tolerance )
{
    const float low = expected - tolerance;
    const float high = expected + tolerance;
    THEN( string_format( "%s is between %.1f and %.1f", subject, low, high ) ) {
        REQUIRE( prob > low );
        REQUIRE( prob < high );
    }
}

TEST_CASE( "Resistance vs. material portions", "[material]" )
{
    const item mostly_steel( "test_shears_mostly_steel" );
    const item mostly_plastic( "test_shears_mostly_plastic" );

    REQUIRE( mostly_steel.get_base_material().id == material_steel );
    REQUIRE( mostly_plastic.get_base_material().id == material_plastic );

    CHECK( mostly_steel.resist( damage_type::CUT ) > mostly_plastic.resist( damage_type::CUT ) );
    CHECK( mostly_steel.resist( damage_type::ACID ) == mostly_plastic.resist( damage_type::ACID ) );
    CHECK( mostly_steel.resist( damage_type::BASH ) > mostly_plastic.resist( damage_type::BASH ) );
    CHECK( mostly_steel.resist( damage_type::HEAT ) == mostly_plastic.resist( damage_type::HEAT ) );
    CHECK( mostly_steel.resist( damage_type::STAB ) > mostly_plastic.resist( damage_type::STAB ) );
    CHECK( mostly_steel.resist( damage_type::BULLET ) > mostly_plastic.resist( damage_type::BULLET ) );
    CHECK( mostly_steel.chip_resistance() > mostly_plastic.chip_resistance() );
}

TEST_CASE( "Portioned material flammability", "[material]" )
{
    const item mostly_steel( "test_fire_ax_mostly_steel" );
    const item mostly_wood( "test_fire_ax_mostly_wood" );

    REQUIRE( mostly_steel.get_base_material().id == material_steel );
    REQUIRE( mostly_wood.get_base_material().id == material_wood );

    fire_data frd;
    frd.contained = true;
    const float steel_burn = mostly_steel.simulate_burn( frd );
    frd = fire_data();
    frd.contained = true;
    const float wood_burn = mostly_wood.simulate_burn( frd );

    CHECK( steel_burn < wood_burn );
}

TEST_CASE( "Glass portion breakability", "[material] [slow]" )
{
    standard_npc dude( "TestCharacter", dude_pos, {}, 0, 8, 8, 8, 8 );
    item mostly_glass( "test_glass_pipe_mostly_glass" );
    item mostly_steel( "test_glass_pipe_mostly_steel" );

    REQUIRE( mostly_glass.get_base_material().id == material_glass );
    REQUIRE( mostly_steel.get_base_material().id == material_steel );

    SECTION( "Throwing pipe made mostly of glass, 8 STR" ) {
        REQUIRE( dude.wield( mostly_glass ) );
        int shatter_count = 0;
        for( int i = 0; i < num_iters; i++ ) {
            dealt_projectile_attack atk = dude.throw_item( target_pos, *dude.get_wielded_item() );
            if( atk.proj.proj_effects.find( "SHATTER_SELF" ) != atk.proj.proj_effects.end() ) {
                shatter_count++;
            }
        }
        check_near( "Chance of glass breaking", static_cast<float>( shatter_count ) / num_iters, 0.72f,
                    0.08f );
    }

    SECTION( "Throwing pipe made mostly of steel, 8 STR" ) {
        REQUIRE( dude.wield( mostly_steel ) );
        int shatter_count = 0;
        for( int i = 0; i < num_iters; i++ ) {
            dealt_projectile_attack atk = dude.throw_item( target_pos, *dude.get_wielded_item() );
            if( atk.proj.proj_effects.find( "SHATTER_SELF" ) != atk.proj.proj_effects.end() ) {
                shatter_count++;
            }
        }
        check_near( "Chance of glass breaking", static_cast<float>( shatter_count ) / num_iters, 0.42f,
                    0.08f );
    }
}
