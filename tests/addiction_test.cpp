#include "addiction.h"
#include "cata_catch.h"
#include "character.h"
#include "player_helpers.h"

static const addiction_id addiction_test_caffeine( "test_caffeine" );
static const addiction_id addiction_test_nicotine( "test_nicotine" );

static const efftype_id effect_shakes( "shakes" );

static constexpr int max_iters = 100000;

static void clear_addictions( Character &u )
{
    u.addictions.clear();
    u.clear_morale();
    u.set_focus( 100 );
    u.set_stim( 0 );
}

static int suffer_addiction( const addiction_id &aid, const int intens, Character &u, int iters,
                             const std::function<void( Character &, addiction & )> &after_update )
{
    int ret = 0;
    while( iters-- > 0 ) {
        clear_addictions( u );
        REQUIRE( u.addictions.empty() );
        u.addictions.emplace_back( aid, intens );
        REQUIRE( u.addiction_level( aid ) == intens );
        auto add = std::find_if( u.addictions.begin(), u.addictions.end(), [&aid]( const addiction & a ) {
            return a.type == aid;
        } );
        REQUIRE( add != u.addictions.end() );
        if( add->run_effect( u ) ) {
            ret++;
        }
        after_update( u, *add );
    }
    return ret;
}

TEST_CASE( "hardcoded and json addictions", "[addiction]" )
{
    REQUIRE( !addiction_test_caffeine->get_effect().is_null() );
    REQUIRE( addiction_test_caffeine->get_builtin().empty() );
    REQUIRE( addiction_test_nicotine->get_effect().is_null() );
    REQUIRE( !addiction_test_nicotine->get_builtin().empty() );

    Character &u = get_player_character();
    clear_character( u );
    clear_addictions( u );
    REQUIRE( u.get_focus() == 100 );
    REQUIRE( u.get_morale_level() == 0 );
    REQUIRE( u.get_stim() == 0 );
    REQUIRE( !u.has_effect( effect_shakes ) );

    int total_focus = 100;
    int total_morale = 0;
    int total_stim = 0;
    auto proc = [&total_focus, &total_morale, &total_stim]( Character & ch, addiction & ) {
        total_focus += ch.get_focus();
        total_morale += ch.get_morale_level();
        total_stim += ch.get_stim();
    };

    SECTION( "caffeine json test, intensity 5" ) {
        int res = suffer_addiction( addiction_test_caffeine, 5, u, max_iters, proc );
        CHECK( res == Approx( 50 ).margin( 40 ) );
        CHECK( total_focus == 100 * ( max_iters + 1 ) );
        CHECK( total_morale <= 0 );
        CHECK( total_stim <= 0 );
    }

    SECTION( "caffeine json test, intensity 20" ) {
        int res = suffer_addiction( addiction_test_caffeine, 20, u, max_iters, proc );
        CHECK( res == Approx( 50 ).margin( 40 ) );
        CHECK( total_focus == 100 * ( max_iters + 1 ) );
        CHECK( total_morale <= 0 );
        CHECK( total_stim <= 0 );
    }

    SECTION( "nicotine hardcoded test, intensity 5" ) {
        int res = suffer_addiction( addiction_test_nicotine, 5, u, max_iters, proc );
        CHECK( res == Approx( 60 ).margin( 40 ) );
        CHECK( total_focus == 100 * ( max_iters + 1 ) );
        CHECK( total_morale <= 0 );
        CHECK( total_stim <= 0 );
    }

    SECTION( "nicotine hardcoded test, intensity 20" ) {
        int res = suffer_addiction( addiction_test_nicotine, 20, u, max_iters, proc );
        CHECK( res == Approx( 70 ).margin( 40 ) );
        CHECK( total_focus == 100 * ( max_iters + 1 ) );
        CHECK( total_morale <= 0 );
        CHECK( total_stim <= 0 );
    }
}