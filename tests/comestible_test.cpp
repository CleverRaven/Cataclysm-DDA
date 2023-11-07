#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iosfwd>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "cata_catch.h"
#include "character.h"
#include "item.h"
#include "item_factory.h"
#include "itype.h"
#include "make_static.h"
#include "output.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "requirements.h"
#include "stomach.h"
#include "test_statistics.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"

static const item_category_id item_category_drugs( "drugs" );
static const item_category_id item_category_mutagen( "mutagen" );
static const itype_id itype_marloss_berry( "marloss_berry" );
static const itype_id itype_marloss_gel( "marloss_gel" );
static const itype_id itype_marloss_seed( "marloss_seed" );

static const recipe_id recipe_veggy_wild_cooked( "veggy_wild_cooked" );

static const vitamin_id vitamin_mutagen( "mutagen" );
static const vitamin_id vitamin_mutagen_alpha( "mutagen_alpha" );
static const vitamin_id vitamin_mutagen_batrachian( "mutagen_batrachian" );
static const vitamin_id vitamin_mutagen_beast( "mutagen_beast" );
static const vitamin_id vitamin_mutagen_bird( "mutagen_bird" );
static const vitamin_id vitamin_mutagen_cattle( "mutagen_cattle" );
static const vitamin_id vitamin_mutagen_cephalopod( "mutagen_cephalopod" );
static const vitamin_id vitamin_mutagen_chimera( "mutagen_chimera" );
static const vitamin_id vitamin_mutagen_elfa( "mutagen_elfa" );
static const vitamin_id vitamin_mutagen_feline( "mutagen_feline" );
static const vitamin_id vitamin_mutagen_fish( "mutagen_fish" );
static const vitamin_id vitamin_mutagen_gastropod( "mutagen_gastropod" );
static const vitamin_id vitamin_mutagen_human( "mutagen_human" );
static const vitamin_id vitamin_mutagen_insect( "mutagen_insect" );
static const vitamin_id vitamin_mutagen_lizard( "mutagen_lizard" );
static const vitamin_id vitamin_mutagen_lupine( "mutagen_lupine" );
static const vitamin_id vitamin_mutagen_medical( "mutagen_medical" );
static const vitamin_id vitamin_mutagen_mouse( "mutagen_mouse" );
static const vitamin_id vitamin_mutagen_plant( "mutagen_plant" );
static const vitamin_id vitamin_mutagen_rabbit( "mutagen_rabbit" );
static const vitamin_id vitamin_mutagen_raptor( "mutagen_raptor" );
static const vitamin_id vitamin_mutagen_rat( "mutagen_rat" );
static const vitamin_id vitamin_mutagen_slime( "mutagen_slime" );
static const vitamin_id vitamin_mutagen_spider( "mutagen_spider" );
static const vitamin_id vitamin_mutagen_troglobite( "mutagen_troglobite" );
static const vitamin_id vitamin_mutagen_ursine( "mutagen_ursine" );
static const vitamin_id vitamin_mutagenic_slurry( "mutagenic_slurry" );

static const std::vector<vitamin_id> mutagen_vit_list{ vitamin_mutagen, vitamin_mutagen_alpha, vitamin_mutagen_batrachian, vitamin_mutagen_beast, vitamin_mutagen_bird, vitamin_mutagen_cattle, vitamin_mutagen_cephalopod, vitamin_mutagen_chimera, vitamin_mutagen_elfa, vitamin_mutagen_feline, vitamin_mutagen_fish, vitamin_mutagen_gastropod, vitamin_mutagen_human, vitamin_mutagen_insect, vitamin_mutagen_lizard, vitamin_mutagen_lupine, vitamin_mutagen_medical, vitamin_mutagen_mouse, vitamin_mutagen_plant, vitamin_mutagen_rabbit, vitamin_mutagen_raptor, vitamin_mutagen_rat, vitamin_mutagen_slime, vitamin_mutagen_spider, vitamin_mutagen_troglobite, vitamin_mutagen_ursine, vitamin_mutagenic_slurry };

static const std::vector<itype_id> marloss_food{ itype_marloss_berry, itype_marloss_gel, itype_marloss_seed };

struct all_stats {
    statistics<int> calories;
};

// given a list of components, adds all the calories together
static int comp_calories( const std::vector<item_comp> &components )
{
    int calories = 0;
    for( const item_comp &it : components ) {
        const cata::value_ptr<islot_comestible> &temp = item::find_type( it.type )->comestible;
        if( temp && temp->cooks_like.is_empty() ) {
            calories += temp->default_nutrition.kcal() * it.count;
        } else if( temp ) {
            const itype *cooks_like = item::find_type( temp->cooks_like );
            calories += cooks_like->comestible->default_nutrition.kcal() * it.count;
        }
    }
    return calories;
}

// puts one permutation of item components into a vector
static std::vector<item_comp> item_comp_vector_create(
    const std::vector<std::vector<item_comp>> &vv, const std::vector<int> &ndx )
{
    std::vector<item_comp> list;
    for( int i = 0, sz = vv.size(); i < sz; ++i ) {
        list.emplace_back( vv[i][ndx[i]] );
    }
    return list;
}

static all_stats recipe_permutations(
    const std::vector< std::vector< item_comp > > &vv, int byproduct_calories )
{
    std::vector<int> muls;
    std::vector<int> szs;

    int total_mul = 1;
    int sz = vv.size();

    // Collect multipliers and sizes
    for( const std::vector<item_comp> &iv : vv ) {
        muls.push_back( total_mul );
        szs.push_back( iv.size() );
        total_mul *= iv.size();
    }

    // total_mul is number of [ v.pick(1) : vv] there are
    // iterate over each
    // container to hold the indices:
    std::vector<int> ndx;
    ndx.resize( sz );
    all_stats mystats;
    for( int i = 0; i < total_mul; ++i ) {
        for( int j = 0; j < sz; ++j ) {
            ndx[j] = ( i / muls[j] ) % szs[j];
        }

        const std::vector<item_comp> permut( item_comp_vector_create( vv, ndx ) );
        // Accumulate the stats.
        mystats.calories.add( comp_calories( permut ) - byproduct_calories );
    }
    return mystats;
}

static int byproduct_calories( const recipe &recipe_obj )
{

    std::vector<item> byproducts = recipe_obj.create_byproducts();
    int kcal = 0;
    for( const item &it : byproducts ) {
        if( it.is_comestible() ) {
            kcal += it.type->comestible->default_nutrition.kcal() * it.count();
        }
    }
    return kcal;
}

static bool has_mutagen_vit( const islot_comestible &comest )
{
    const std::map<vitamin_id, int> &vits = comest.default_nutrition.vitamins;
    for( const vitamin_id &vit : mutagen_vit_list ) {
        if( vits.find( vit ) != vits.end() && vits.at( vit ) > 0 ) {
            return true;
        }
    }
    return false;
}

// Test that every comestible heathy is <=0 and >=-1
TEST_CASE( "comestible_health_bounds", "[comestible]" )
{
    for( const itype *it : item_controller->all() ) {
        if( !it->comestible || it->category_force == item_category_mutagen ||
            it->category_force == item_category_drugs ||
            std::count( marloss_food.begin(), marloss_food.end(), it->get_id() ) ) {
            continue;
        }
        const islot_comestible &comest = *it->comestible;
        const std::string &comest_type = comest.comesttype;
        if( ( comest_type != "FOOD" && comest_type != "DRINK" ) || has_mutagen_vit( comest ) ) {
            continue;
        }
        if( it->src.back().second.str() != "dda" ) {
            continue;
        }

        INFO( it->get_id() );
        CHECK( comest.healthy <= 0 );
        CHECK( comest.healthy >= -1 );
    }
}

static int get_default_calories_recursive( item &it )
{
    int calories = it.type->comestible ? it.type->comestible->default_nutrition.kcal() : 0;

    if( it.count_by_charges() ) {
        calories *= it.charges;
    }

    for( item *cont : it.all_items_top() ) {
        calories += get_default_calories_recursive( *cont );
    }

    return calories;
}

TEST_CASE( "recipe_permutations", "[recipe]" )
{
    // Are these tests failing? Here's how to fix that:
    // If the average is over the upper bound, you need to increase the calories for the item
    // that is causing the test to fail (or decrease the total calories of the ingredients)
    // If the average is under the lower bound, you need to decrease the calories for the item
    // that is causing the test to fail (or increase the total calories of the ingredients)
    // If it doesn't make sense for your component and resultant calories to match, you probably
    // want to add the NUTRIENT_OVERRIDE flag to the resultant item.
    for( const auto &recipe_pair : recipe_dict ) {
        // the resulting item
        const recipe &recipe_obj = recipe_pair.first.obj();
        item temp( recipe_obj.result() );
        const bool is_food = temp.is_food();
        const bool has_override = temp.has_flag( STATIC( flag_id( "NUTRIENT_OVERRIDE" ) ) );
        if( is_food && !has_override ) {
            // Collection of kcal values of all ingredient permutations
            all_stats mystats = recipe_permutations( recipe_obj.simple_requirements().get_components(),
                                byproduct_calories( recipe_obj ) );
            if( mystats.calories.n() < 2 ) {
                continue;
            }

            // The calories of the result
            int default_calories = 0;
            for( item &it : recipe_obj.create_results() ) {
                default_calories += get_default_calories_recursive( it );
            }

            // Make the range of acceptable average calories of permutations, using result's calories
            const float lower_bound = std::min( default_calories - mystats.calories.stddev() * 2,
                                                default_calories * 0.75 );
            const float upper_bound = std::max( default_calories + mystats.calories.stddev() * 2,
                                                default_calories * 1.25 );
            CHECK( mystats.calories.min() >= 0 );
            CHECK( lower_bound <= mystats.calories.avg() );
            CHECK( mystats.calories.avg() <= upper_bound );
            if( mystats.calories.min() < 0 || lower_bound > mystats.calories.avg() ||
                mystats.calories.avg() > upper_bound ) {
                printf( "\n\nRecipeID: %s, default is %d Calories,\nCurrent recipe range: %d-%d, Average %.1f, Stddev %.1f"
                        "\nAverage recipe Calories must fall within this range, derived from default Calories: %.0f-%.0f\n\n",
                        recipe_pair.first.c_str(), default_calories,
                        mystats.calories.min(), mystats.calories.max(), mystats.calories.avg(), mystats.calories.stddev(),
                        lower_bound, upper_bound );
            }
        }
    }
}

TEST_CASE( "cooked_veggies_get_correct_calorie_prediction", "[recipe]" )
{
    // This test verifies that predicted calorie ranges properly take into
    // account the "RAW"/"COOKED" flags.
    const item veggy_wild_cooked( "veggy_wild_cooked" );

    const Character &u = get_player_character();

    nutrients default_nutrition = u.compute_effective_nutrients( veggy_wild_cooked );
    std::map<recipe_id, std::pair<nutrients, nutrients>> rec_cache;
    std::pair<nutrients, nutrients> predicted_nutrition =
        u.compute_nutrient_range( veggy_wild_cooked, recipe_veggy_wild_cooked, rec_cache );

    CHECK( default_nutrition.kcal() == predicted_nutrition.first.kcal() );
    CHECK( default_nutrition.kcal() == predicted_nutrition.second.kcal() );
}

// The Character::compute_effective_food_volume_ratio function returns a floating-point ratio
// used as a multiplier for the food volume when it is eaten, based on the energy density
// (kcal/gram) of the food, as follows:
//
// - low-energy food     (0.0 < kcal/gram < 1.0)  returns 1.0
// - medium-energy food  (1.0 < kcal/gram < 3.0)  returns (kcal/gram)
// - high-energy food    (3.0 < kcal/gram)        returns sqrt( 3 * kcal/gram )
//
// The Character::compute_calories_per_effective_volume function returns a dimensionless integer
// representing the "satiety" of the food, with higher numbers being more calorie-dense, and lower
// numbers being less so.
//
TEST_CASE( "effective_food_volume_and_satiety", "[character][food][satiety]" )
{
    const Character &u = get_player_character();
    double expect_ratio;

    // Apple: 95 kcal / 200 g (1 serving)
    const item apple( "test_apple" );
    const nutrients apple_nutr = u.compute_effective_nutrients( apple );
    REQUIRE( apple.count() == 1 );
    REQUIRE( apple.weight() == 200_gram );
    REQUIRE( apple.volume() == 250_ml );
    REQUIRE( apple_nutr.kcal() == 95 );
    // If kcal per gram < 1.0, return 1.0
    CHECK( u.compute_effective_food_volume_ratio( apple ) == Approx( 1.0f ).margin( 0.01f ) );
    CHECK( u.compute_calories_per_effective_volume( apple ) == 500 );
    CHECK( satiety_bar( 500 ) == "<color_c_yellow>||\\</color>.." );

    // Egg: 80 kcal / 40 g (1 serving)
    const item egg( "test_egg" );
    const nutrients egg_nutr = u.compute_effective_nutrients( egg );
    REQUIRE( egg.count() == 1 );
    REQUIRE( egg.weight() == 40_gram );
    REQUIRE( egg.volume() == 50_ml );
    REQUIRE( egg_nutr.kcal() == 80 );
    // If kcal per gram > 1.0 but less than 3.0, return ( kcal / gram )
    CHECK( u.compute_effective_food_volume_ratio( egg ) == Approx( 2.0f ).margin( 0.01f ) );
    CHECK( u.compute_calories_per_effective_volume( egg ) == 2000 );
    CHECK( satiety_bar( 2000 ) == "<color_c_green>|||||</color>" );

    // Pine nuts: 202 kcal / 30 g (4 servings)
    const item nuts( "test_pine_nuts" );
    const nutrients nuts_nutr = u.compute_effective_nutrients( nuts );
    // If food count > 1, total weight is divided by count before computing kcal/gram
    REQUIRE( nuts.count() == 4 );
    REQUIRE( nuts.weight() == 120_gram );
    REQUIRE( nuts.volume() == 250_ml );
    REQUIRE( nuts_nutr.kcal() == 202 );
    // If kcal per gram > 3.0, return sqrt( 3 * kcal / gram )
    expect_ratio = std::sqrt( 3.0f * 202 / 30 );
    CHECK( u.compute_effective_food_volume_ratio( nuts ) == Approx( expect_ratio ).margin( 0.01f ) );
    CHECK( u.compute_calories_per_effective_volume( nuts ) == 1498 );
    CHECK( satiety_bar( 1498 ) == "<color_c_green>||||\\</color>" );
}

// satiety_bar returns a colorized string indicating a satiety level, similar to hit point bars
// where "....." is minimum (~ 0) and "|||||" is maximum (~ 1500)
//
TEST_CASE( "food_satiety_bar", "[character][food][satiety]" )
{
    // NOLINTNEXTLINE(cata-text-style): verbatim ellipses necessary for validation
    CHECK( satiety_bar( 0 ) == "<color_c_red></color>....." );
    // NOLINTNEXTLINE(cata-text-style): verbatim ellipses necessary for validation
    CHECK( satiety_bar( 1 ) == "<color_c_red>:</color>...." );
    // NOLINTNEXTLINE(cata-text-style): verbatim ellipses necessary for validation
    CHECK( satiety_bar( 50 ) == "<color_c_light_red>\\</color>...." );
    // NOLINTNEXTLINE(cata-text-style): verbatim ellipses necessary for validation
    CHECK( satiety_bar( 100 ) == "<color_c_light_red>|</color>...." );
    // NOLINTNEXTLINE(cata-text-style): verbatim ellipses necessary for validation
    CHECK( satiety_bar( 200 ) == "<color_c_light_red>|\\</color>..." );
    // NOLINTNEXTLINE(cata-text-style): verbatim ellipses necessary for validation
    CHECK( satiety_bar( 300 ) == "<color_c_yellow>||</color>..." );
    CHECK( satiety_bar( 400 ) == "<color_c_yellow>||\\</color>.." );
    CHECK( satiety_bar( 500 ) == "<color_c_yellow>||\\</color>.." );
    CHECK( satiety_bar( 600 ) == "<color_c_light_green>|||</color>.." );
    CHECK( satiety_bar( 700 ) == "<color_c_light_green>|||</color>.." );
    CHECK( satiety_bar( 800 ) == "<color_c_light_green>|||\\</color>." );
    CHECK( satiety_bar( 900 ) == "<color_c_light_green>|||\\</color>." );
    CHECK( satiety_bar( 1000 ) == "<color_c_light_green>||||</color>." );
    CHECK( satiety_bar( 1100 ) == "<color_c_light_green>||||</color>." );
    CHECK( satiety_bar( 1200 ) == "<color_c_green>||||</color>." );
    CHECK( satiety_bar( 1300 ) == "<color_c_green>||||\\</color>" );
    CHECK( satiety_bar( 1400 ) == "<color_c_green>||||\\</color>" );
    CHECK( satiety_bar( 1500 ) == "<color_c_green>|||||</color>" );
}
