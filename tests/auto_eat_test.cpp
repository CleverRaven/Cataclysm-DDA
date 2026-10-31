#include "auto_eat.h"

#include <functional>
#include <memory>
#include <string>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "crafting.h"
#include "item.h"
#include "itype.h"
#include "map_selector.h"
#include "player_helpers.h"
#include "recipe.h"
#include "requirements.h"
#include "ret_val.h"
#include "type_id.h"
#include "value_ptr.h"
#include "visitable.h"

static const flag_id json_flag_COLD( "COLD" );

static const itype_id itype_backpack_tactical_large( "backpack_tactical_large" );
static const itype_id itype_meat( "meat" );
static const itype_id itype_meat_cooked( "meat_cooked" );
static const itype_id itype_mess_kit( "mess_kit" );
static const itype_id itype_popcan_stove( "popcan_stove" );
static const itype_id itype_powder_eggs( "powder_eggs" );
static const itype_id itype_rehydrated_eggs( "rehydrated_eggs" );
static const itype_id itype_salt( "salt" );
static const itype_id itype_water_clean( "water_clean" );

static const quality_id qual_COOK( "COOK" );

static const recipe_id recipe_broth_meat( "broth_meat" );
static const recipe_id recipe_meat_cooked( "meat_cooked" );

class auto_eat_test
{
    private:
        static const auto_eat_settings base_settings;
        avatar &you;

    public:
        explicit auto_eat_test( avatar &you ) : you( you ) {}
        // scores should be the same as each other
        void score_food_check_all( auto_eat_settings::potential_edible &food,
                                   const int score_expected ) const {
            CHECK( base_settings.score_food( you, food ) == score_expected );
            CHECK( you.auto_eat_handler.score_food( you, food ) == score_expected );
        }
        // scores should be the same as each other
        void score_quench_check_all( const auto_eat_settings::potential_edible &food,
                                     const int score_expected ) const {
            CHECK( base_settings.quench_score( you, food ) == score_expected );
            CHECK( you.auto_eat_handler.quench_score( you, food ) == score_expected );
        }
        // scores should be the same as each other
        void score_joy_check_all( const auto_eat_settings::potential_edible &food,
                                  const int score_expected ) const {
            CHECK( base_settings.joy_score( you, food ) == score_expected );
            CHECK( you.auto_eat_handler.joy_score( you, food ) == score_expected );
        }
        // scores should be the same as each other
        void score_recipe_check_all( const auto_eat_settings::potential_edible &food,
                                     const int score_expected ) const {
            CHECK( base_settings.recipe_score( you, food ) == score_expected );
            CHECK( you.auto_eat_handler.recipe_score( you, food ) == score_expected );
        }
        // scores should be the same as each other
        void score_calorie_check_all( const auto_eat_settings::potential_edible &food,
                                      const int score_expected ) const {
            CHECK( base_settings.calorie_score( you, food ) == score_expected );
            CHECK( you.auto_eat_handler.calorie_score( you, food ) == score_expected );
        }
        // scores should be the same as each other
        void score_vitamin_check_all( const auto_eat_settings::potential_edible &food,
                                      const int score_expected ) const {
            CHECK( base_settings.vitamin_score( you, food ) == score_expected );
            CHECK( you.auto_eat_handler.vitamin_score( you, food ) == score_expected );
        }
        // scores should be the same as each other
        void score_spoil_check_all( const auto_eat_settings::potential_edible &food,
                                    const int score_expected ) const {
            CHECK( base_settings.spoil_score( you, food ) == score_expected );
            CHECK( you.auto_eat_handler.spoil_score( you, food ) == score_expected );
        }
};

const auto_eat_settings auto_eat_test::base_settings = auto_eat_settings();

/**
 *  It's important to note here that the clean water used in the normal game is used here
 *  on purpose as a litmus test for the "quench" system overall, rather than generating
 *  a test json item
 */
TEST_CASE( "water_food_score", "[auto_eat]" )
{
    clear_avatar();
    avatar &you = get_avatar();
    const auto_eat_test water_test( you );

    const item clean_water( itype_water_clean );
    item_location clean_water_container = you.i_add( clean_water.in_its_container() );
    item_location clean_water_contained;
    clean_water_container.visit_items( [&clean_water_contained]( const item_location & loc ) {
        if( loc.has_parent() ) {
            clean_water_contained = loc;
            return VisitResponse::ABORT;
        }
        return VisitResponse::NEXT;
    } );
    REQUIRE( clean_water_contained );
    clean_water_contained->set_owner( you );
    CAPTURE( clean_water_contained->tname() );
    REQUIRE( clean_water_contained->is_comestible() );
    REQUIRE( clean_water_contained->get_comestible()->quench == 50 );
    REQUIRE( you.will_auto_eat( *clean_water_contained ).success() );

    WHEN( "regular clean water" ) {
        auto_eat_settings::potential_edible clean_water_edible( clean_water_contained );
        WHEN( "the avatar is slightly thirsty" ) {
            water_test.score_food_check_all( clean_water_edible, 50 );
            water_test.score_quench_check_all( clean_water_edible, 50 );
        }

        WHEN( "avatar is not thirsty" ) {
            you.set_thirst( -1 );
            water_test.score_food_check_all( clean_water_edible, -50 );
            water_test.score_quench_check_all( clean_water_edible, -50 );
        }

        WHEN( "avatar is slaked" ) {
            you.set_thirst( -41 );
            water_test.score_food_check_all( clean_water_edible, -200 );
            water_test.score_quench_check_all( clean_water_edible, -200 );
        }
        WHEN( "avatar is thirsty" ) {
            you.set_thirst( 21 );
            water_test.score_food_check_all( clean_water_edible, 100 );
            water_test.score_quench_check_all( clean_water_edible, 100 );
        }
        WHEN( "avatar is very thirsty" ) {
            you.set_thirst( 41 );
            water_test.score_food_check_all( clean_water_edible, 200 );
            water_test.score_quench_check_all( clean_water_edible, 200 );
        }
    }
    WHEN( "the water is cold" ) {
        clean_water_contained->set_flag( json_flag_COLD );
        auto_eat_settings::potential_edible clean_water_edible( clean_water_contained );
        WHEN( "the avatar is slightly thirsty" ) {
            you.set_thirst( 0 );
            water_test.score_food_check_all( clean_water_edible, 60 );
            water_test.score_quench_check_all( clean_water_edible, 50 );
        }
        clean_water_contained->unset_flag( json_flag_COLD );
    }
}

TEST_CASE( "auto_craft_eat", "[auto_eat]" )
{
    clear_avatar();
    avatar &you = get_avatar();
    you.wear( you.i_add( item( itype_backpack_tactical_large ) ), false );

    WHEN( "recipe is more valuable than ingredients" ) {
        const tripoint_bub_ms you_at = you.pos_bub();
        const map_cursor cur_at( you_at );

        auto_eat_test broth_test( you );
        auto_eat_settings::potential_edible broth( &recipe_broth_meat.obj() );
        auto_eat_settings::potential_edible meat_cooked( &recipe_meat_cooked.obj() );
        auto_eat_settings::potential_edible meat( you.i_add( item( itype_meat ) ) );
        auto_eat_settings::potential_edible powder_eggs( you.i_add( item( itype_powder_eggs ) ) );

        // add some ingredients
        you.i_add( item( itype_water_clean, calendar::turn, 2 ).in_its_container() );
        you.i_add( item( itype_salt ) );
        you.i_add( item( itype_salt ) );
        you.i_add( item( itype_salt ) );
        you.i_add( item( itype_salt ) );
        you.i_add( item( itype_mess_kit ) );
        item_location popcan_stove = you.i_add( tool_with_ammo( itype_popcan_stove, 500 ) );

        REQUIRE( popcan_stove->has_ammo() );
        REQUIRE( you.has_quality( qual_COOK, 3 ) );
        CHECK( recipe_broth_meat->deduped_requirements().can_make_with_inventory( &you,
                you.crafting_inventory(), recipe_broth_meat->get_component_filter( recipe_filter_flags::no_rotten ),
                1, craft_flags::start_only ) );
        CHECK( recipe_meat_cooked->deduped_requirements().can_make_with_inventory( &you,
                you.crafting_inventory(), recipe_broth_meat->get_component_filter( recipe_filter_flags::no_rotten ),
                1, craft_flags::start_only ) );

        broth_test.score_calorie_check_all( meat, 11 );
        broth_test.score_calorie_check_all( meat_cooked, 12 );
        broth_test.score_calorie_check_all( powder_eggs, 8 );
        broth_test.score_calorie_check_all( broth, 2 );

        broth_test.score_quench_check_all( meat, 0 );
        broth_test.score_quench_check_all( meat_cooked, 0 );
        broth_test.score_quench_check_all( powder_eggs, -2 );
        broth_test.score_quench_check_all( broth, 60 );

        broth_test.score_joy_check_all( meat, -100 );
        broth_test.score_joy_check_all( meat_cooked, 0 );
        broth_test.score_joy_check_all( powder_eggs, -40 );
        broth_test.score_joy_check_all( broth, 10 );

        broth_test.score_recipe_check_all( meat, 0 );
        broth_test.score_recipe_check_all( meat_cooked, -60 );
        broth_test.score_recipe_check_all( powder_eggs, 0 );
        broth_test.score_recipe_check_all( broth, -60 );

        broth_test.score_vitamin_check_all( meat, 1 );
        broth_test.score_vitamin_check_all( meat_cooked, 1 );
        broth_test.score_vitamin_check_all( powder_eggs, 0 );
        broth_test.score_vitamin_check_all( broth, 1 );

        broth_test.score_spoil_check_all( meat, 50 );
        broth_test.score_spoil_check_all( meat_cooked, 50 );
        broth_test.score_spoil_check_all( powder_eggs, 0 );
        broth_test.score_spoil_check_all( broth, 15 );

        broth_test.score_food_check_all( meat, -38 );
        broth_test.score_food_check_all( meat_cooked, 3 );
        broth_test.score_food_check_all( powder_eggs, -34 );
        broth_test.score_food_check_all( broth, 28 );

        REQUIRE( you.get_hunger() == 0 );
        you.set_thirst( -41 );
        auto food_list = you.auto_eat_handler.get_surrounding_available_food( you );
        auto food_iter = food_list.begin();
        REQUIRE( food_iter != food_list.end() );
        CHECK( food_iter->get_item_copy().typeId() == itype_meat_cooked );
        ++food_iter;
        REQUIRE( food_iter != food_list.end() );
        CHECK( food_iter->get_item_copy().typeId() == itype_rehydrated_eggs );
        ++food_iter;
        REQUIRE( food_iter != food_list.end() );
        CHECK( food_iter->get_item_copy().typeId() == itype_water_clean );

        // note that broth does not show up at all. this is because it is a drink that has less quench than water
        // and not enough joy and nutrition to counteract the recipe score negative.
    }

}
