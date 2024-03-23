#include <string>

#include "fire.h"
#include "item.h"
#include "cata_catch.h"
#include "map_helpers.h"
#include "npc.h"
#include "projectile.h"

static const damage_type_id damage_acid( "acid" );
static const damage_type_id damage_bash( "bash" );
static const damage_type_id damage_bullet( "bullet" );
static const damage_type_id damage_cut( "cut" );
static const damage_type_id damage_heat( "heat" );
static const damage_type_id damage_stab( "stab" );

static const material_id material_glass( "glass" );
static const material_id material_lycra( "lycra" );
static const material_id material_lycra_resist_override_stab( "lycra_resist_override_stab" );
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

TEST_CASE( "Resistance_vs_material_portions", "[material]" )
{
    const item mostly_steel( "test_shears_mostly_steel" );
    const item mostly_plastic( "test_shears_mostly_plastic" );

    REQUIRE( mostly_steel.get_base_material().id == material_steel );
    REQUIRE( mostly_plastic.get_base_material().id == material_plastic );

    CHECK( mostly_steel.resist( damage_cut ) > mostly_plastic.resist( damage_cut ) );
    CHECK( mostly_steel.resist( damage_acid ) == mostly_plastic.resist( damage_acid ) );
    CHECK( mostly_steel.resist( damage_bash ) > mostly_plastic.resist( damage_bash ) );
    CHECK( mostly_steel.resist( damage_heat ) == mostly_plastic.resist( damage_heat ) );
    CHECK( mostly_steel.resist( damage_stab ) > mostly_plastic.resist( damage_stab ) );
    CHECK( mostly_steel.resist( damage_bullet ) > mostly_plastic.resist( damage_bullet ) );
    CHECK( mostly_steel.chip_resistance() > mostly_plastic.chip_resistance() );
}

TEST_CASE( "Portioned_material_flammability", "[material]" )
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

TEST_CASE( "Glass_portion_breakability", "[material] [slow]" )
{
    clear_creatures();
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

TEST_CASE( "Override_derived_resistance", "[material]" )
{
    // Materials don't store derived resistances, instead they're derived at the item level.
    // Just check that this is still the case.
    REQUIRE( damage_stab->derived_from.first == damage_cut );
    CHECK( material_lycra->resist( damage_cut ) == 1.f );
    CHECK( material_lycra->resist( damage_stab ) == 0.f );
    CHECK( material_lycra_resist_override_stab->resist( damage_cut ) == 2.f );
    CHECK( material_lycra_resist_override_stab->resist( damage_stab ) == 50.f );
}
