#include "addiction.h"
#include "cata_catch.h"
#include "character.h"
#include "player_helpers.h"

static const addiction_id addiction_alcohol( "alcohol" );
static const addiction_id addiction_amphetamine( "amphetamine" );
static const addiction_id addiction_caffeine( "caffeine" );
static const addiction_id addiction_cocaine( "cocaine" );
static const addiction_id addiction_crack( "crack" );
static const addiction_id addiction_diazepam( "diazepam" );
static const addiction_id addiction_marloss_b( "marloss_b" );
static const addiction_id addiction_marloss_r( "marloss_r" );
static const addiction_id addiction_marloss_y( "marloss_y" );
static const addiction_id addiction_mutagen( "mutagen" );
static const addiction_id addiction_nicotine( "nicotine" );
static const addiction_id addiction_opiate( "opiate" );
static const addiction_id addiction_test_caffeine( "test_caffeine" );
static const addiction_id addiction_test_nicotine( "test_nicotine" );

static const efftype_id effect_hallu( "hallu" );
static const efftype_id effect_shakes( "shakes" );

static const trait_id trait_MUT_JUNKIE( "MUT_JUNKIE" );

static constexpr int max_iters = 100000;

struct addict_effect_totals {
    int fatigue = 0;
    int morale = 0;
    int stim = 0;
    int str_bonus = 0;
    int dex_bonus = 0;
    int per_bonus = 0;
    int int_bonus = 0;
    int health_mod = 0;
    int shakes = 0;
    int hallu = 0;
    int pkiller = 0;
    int pain = 0;
    int focus = 0;

    void after_update( Character &u ) {
        fatigue += u.get_fatigue();
        morale += u.get_morale_level();
        stim += u.get_stim();
        str_bonus += u.str_cur;
        dex_bonus += u.dex_cur;
        per_bonus += u.per_cur;
        int_bonus += u.int_cur;
        health_mod += u.get_daily_health();
        pkiller += u.get_painkiller();
        pain += u.get_pain();
        focus += u.get_focus();
        shakes += u.has_effect( effect_shakes ) ? 1 : 0;
        hallu += u.has_effect( effect_hallu ) ? 1 : 0;
    }
};

static void clear_addictions( Character &u )
{
    u.addictions.clear();
    u.clear_effects();
    u.moves = 100;
    u.set_daily_health( 0 );
    u.str_max = 8;
    u.dex_max = 8;
    u.int_max = 8;
    u.per_max = 8;
    u.set_str_bonus( 0 );
    u.set_dex_bonus( 0 );
    u.set_int_bonus( 0 );
    u.set_per_bonus( 0 );
    u.clear_morale();
    u.set_fatigue( 0 );
    u.set_stim( 0 );
    u.set_painkiller( 0 );
    u.set_pain( 0 );
    u.set_focus( 100 );

    REQUIRE( u.get_fatigue() == 0 );
    REQUIRE( u.get_morale_level() == 0 );
    REQUIRE( u.get_stim() == 0 );
    REQUIRE( !u.has_effect( effect_hallu ) );
    REQUIRE( !u.has_effect( effect_shakes ) );
}

static int suffer_addiction( const addiction_id &aid, const int intens, Character &u, int iters,
                             addict_effect_totals &totals )
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
        totals.after_update( u );
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

    addict_effect_totals totals;

    SECTION( "caffeine json test, intensity 5" ) {
        int res = suffer_addiction( addiction_test_caffeine, 5, u, max_iters, totals );
        CHECK( res == Approx( 50 ).margin( 40 ) );
        CHECK( totals.fatigue == 0 );
        // Morale & stim very rarely get decremented in the effect
        // Just check that they aren't positive
        CHECK( totals.morale <= 0 );
        CHECK( totals.stim <= 0 );
        // Can also cause the "shakes" effect, but RNG makes it inconsistent
    }

    SECTION( "caffeine json test, intensity 20" ) {
        int res = suffer_addiction( addiction_test_caffeine, 20, u, max_iters, totals );
        CHECK( res == Approx( 50 ).margin( 40 ) );
        CHECK( totals.fatigue == 0 );
        CHECK( totals.morale <= 0 );
        CHECK( totals.stim <= 0 );
    }

    SECTION( "nicotine hardcoded test, intensity 5" ) {
        int res = suffer_addiction( addiction_test_nicotine, 5, u, max_iters, totals );
        CHECK( res == Approx( 60 ).margin( 40 ) );
        CHECK( totals.fatigue >= 0 );
        CHECK( totals.morale <= 0 );
        CHECK( totals.stim <= 0 );
    }

    SECTION( "nicotine hardcoded test, intensity 20" ) {
        int res = suffer_addiction( addiction_test_nicotine, 20, u, max_iters, totals );
        CHECK( res == Approx( 70 ).margin( 40 ) );
        CHECK( totals.fatigue == Approx( 70 ).margin( 30 ) );
        CHECK( totals.morale <= 0 );
        CHECK( totals.stim <= 0 );
    }
}

TEST_CASE( "check caffeine addiction effects", "[addiction]" )
{
    Character &u = get_player_character();
    clear_character( u );
    clear_addictions( u );

    addict_effect_totals totals;

    SECTION( "intensity 5" ) {
        int res = suffer_addiction( addiction_caffeine, 5, u, max_iters, totals );
        CHECK( res == Approx( 50 ).margin( 40 ) );
        CHECK( totals.health_mod == 0 );
        CHECK( totals.fatigue == 0 );
        CHECK( totals.morale <= 0 );
        CHECK( totals.stim <= 0 );
        CHECK( totals.pkiller == 0 );
        CHECK( totals.pain == 0 );
        CHECK( totals.focus == max_iters * 100 );
        CHECK( totals.str_bonus == max_iters * 8 );
        CHECK( totals.dex_bonus == max_iters * 8 );
        CHECK( totals.per_bonus == max_iters * 8 );
        CHECK( totals.int_bonus == max_iters * 8 );
        CHECK( totals.shakes == 0 );
        CHECK( totals.hallu == 0 );
    }

    SECTION( "intensity 20" ) {
        int res = suffer_addiction( addiction_caffeine, 20, u, max_iters, totals );
        CHECK( res == Approx( 50 ).margin( 40 ) );
        CHECK( totals.health_mod == 0 );
        CHECK( totals.fatigue == 0 );
        CHECK( totals.morale <= 0 );
        CHECK( totals.stim <= 0 );
        CHECK( totals.pkiller == 0 );
        CHECK( totals.pain == 0 );
        CHECK( totals.focus == max_iters * 100 );
        CHECK( totals.str_bonus == max_iters * 8 );
        CHECK( totals.dex_bonus == max_iters * 8 );
        CHECK( totals.per_bonus == max_iters * 8 );
        CHECK( totals.int_bonus == max_iters * 8 );
        CHECK( totals.shakes >= 0 );
        CHECK( totals.hallu == 0 );
    }
}

TEST_CASE( "check nicotine addiction effects", "[addiction]" )
{
    Character &u = get_player_character();
    clear_character( u );
    clear_addictions( u );

    addict_effect_totals totals;

    SECTION( "intensity 5" ) {
        int res = suffer_addiction( addiction_nicotine, 5, u, max_iters, totals );
        CHECK( res == Approx( 60 ).margin( 40 ) );
        CHECK( totals.health_mod == 0 );
        CHECK( totals.fatigue >= 0 );
        CHECK( totals.morale <= 0 );
        CHECK( totals.stim <= 0 );
        CHECK( totals.pkiller == 0 );
        CHECK( totals.pain == 0 );
        CHECK( totals.focus == max_iters * 100 );
        CHECK( totals.str_bonus == max_iters * 8 );
        CHECK( totals.dex_bonus == max_iters * 8 );
        CHECK( totals.per_bonus == max_iters * 8 );
        CHECK( totals.int_bonus == max_iters * 8 );
        CHECK( totals.shakes == 0 );
        CHECK( totals.hallu == 0 );
    }

    SECTION( "intensity 20" ) {
        int res = suffer_addiction( addiction_nicotine, 20, u, max_iters, totals );
        CHECK( res == Approx( 70 ).margin( 40 ) );
        CHECK( totals.health_mod == 0 );
        CHECK( totals.fatigue == Approx( 70 ).margin( 30 ) );
        CHECK( totals.morale <= 0 );
        CHECK( totals.stim <= 0 );
        CHECK( totals.pkiller == 0 );
        CHECK( totals.pain == 0 );
        CHECK( totals.focus == max_iters * 100 );
        CHECK( totals.str_bonus == max_iters * 8 );
        CHECK( totals.dex_bonus == max_iters * 8 );
        CHECK( totals.per_bonus == max_iters * 8 );
        CHECK( totals.int_bonus == max_iters * 8 );
        CHECK( totals.shakes == 0 );
        CHECK( totals.hallu == 0 );
    }
}

TEST_CASE( "check alcohol addiction effects", "[addiction]" )
{
    Character &u = get_player_character();
    clear_character( u );
    clear_addictions( u );

    addict_effect_totals totals;

    SECTION( "intensity 5" ) {
        int res = suffer_addiction( addiction_alcohol, 5, u, max_iters, totals );
        CHECK( res == Approx( 1300 ).margin( 500 ) );
        CHECK( totals.health_mod < 0 );
        CHECK( totals.fatigue == 0 );
        CHECK( totals.morale == Approx( -40000 ).margin( 20000 ) );
        CHECK( totals.stim == 0 );
        CHECK( totals.pkiller == 0 );
        CHECK( totals.pain == 0 );
        CHECK( totals.focus == max_iters * 100 );
        CHECK( totals.str_bonus == max_iters * 8 );
        CHECK( totals.dex_bonus == max_iters * 8 );
        CHECK( totals.per_bonus == max_iters * 7 );
        CHECK( totals.int_bonus == max_iters * 7 );
        CHECK( totals.shakes == 0 );
        CHECK( totals.hallu == 0 );
    }

    SECTION( "intensity 20" ) {
        int res = suffer_addiction( addiction_alcohol, 20, u, max_iters, totals );
        CHECK( res == Approx( 9000 ).margin( 2000 ) );
        CHECK( totals.health_mod < 0 );
        CHECK( totals.fatigue == 0 );
        CHECK( totals.morale == Approx( -300000 ).margin( 100000 ) );
        CHECK( totals.stim == 0 );
        CHECK( totals.pkiller == 0 );
        CHECK( totals.pain == 0 );
        CHECK( totals.focus == max_iters * 100 );
        CHECK( totals.str_bonus == max_iters * 8 );
        CHECK( totals.dex_bonus == max_iters * 8 );
        CHECK( totals.per_bonus == max_iters * 7 );
        CHECK( totals.int_bonus == max_iters * 7 );
        CHECK( totals.shakes > 0 );
        CHECK( totals.hallu > 0 );
    }
}

TEST_CASE( "check diazepam addiction effects", "[addiction]" )
{
    Character &u = get_player_character();
    clear_character( u );
    clear_addictions( u );

    addict_effect_totals totals;

    SECTION( "intensity 5" ) {
        int res = suffer_addiction( addiction_diazepam, 5, u, max_iters, totals );
        CHECK( res == Approx( 1300 ).margin( 500 ) );
        CHECK( totals.health_mod < 0 );
        CHECK( totals.fatigue == 0 );
        CHECK( totals.morale == Approx( -40000 ).margin( 20000 ) );
        CHECK( totals.stim == 0 );
        CHECK( totals.pkiller == 0 );
        CHECK( totals.pain == 0 );
        CHECK( totals.focus == max_iters * 100 );
        CHECK( totals.str_bonus == max_iters * 8 );
        CHECK( totals.dex_bonus == max_iters * 8 );
        CHECK( totals.per_bonus == max_iters * 7 );
        CHECK( totals.int_bonus == max_iters * 7 );
        CHECK( totals.shakes == 0 );
        CHECK( totals.hallu == 0 );
    }

    SECTION( "intensity 20" ) {
        int res = suffer_addiction( addiction_diazepam, 20, u, max_iters, totals );
        CHECK( res == Approx( 9000 ).margin( 2000 ) );
        CHECK( totals.health_mod < 0 );
        CHECK( totals.fatigue == 0 );
        CHECK( totals.morale == Approx( -300000 ).margin( 100000 ) );
        CHECK( totals.stim == 0 );
        CHECK( totals.pkiller == 0 );
        CHECK( totals.pain == 0 );
        CHECK( totals.focus == max_iters * 100 );
        CHECK( totals.str_bonus == max_iters * 8 );
        CHECK( totals.dex_bonus == max_iters * 8 );
        CHECK( totals.per_bonus == max_iters * 7 );
        CHECK( totals.int_bonus == max_iters * 7 );
        CHECK( totals.shakes > 0 );
        CHECK( totals.hallu > 0 );
    }
}

TEST_CASE( "check opiate addiction effects", "[addiction]" )
{
    Character &u = get_player_character();
    clear_character( u );
    clear_addictions( u );

    addict_effect_totals totals;

    SECTION( "intensity 5" ) {
        int res = suffer_addiction( addiction_opiate, 5, u, max_iters, totals );
        CHECK( res == max_iters );
        CHECK( totals.health_mod < 0 );
        CHECK( totals.fatigue == 0 );
        CHECK( totals.morale < 0 );
        CHECK( totals.stim == 0 );
        CHECK( totals.pkiller == 0 );
        CHECK( totals.pain == max_iters );
        CHECK( totals.focus == max_iters * 100 );
        CHECK( totals.str_bonus == max_iters * 7 );
        CHECK( totals.dex_bonus == max_iters * 7 );
        CHECK( totals.per_bonus == max_iters * 7 );
        CHECK( totals.int_bonus == max_iters * 8 );
        CHECK( totals.shakes > 0 );
        CHECK( totals.hallu == 0 );
    }

    SECTION( "intensity 20" ) {
        int res = suffer_addiction( addiction_opiate, 20, u, max_iters, totals );
        CHECK( res == max_iters );
        CHECK( totals.health_mod < 0 );
        CHECK( totals.fatigue == 0 );
        CHECK( totals.morale < 0 );
        CHECK( totals.stim == 0 );
        CHECK( totals.pkiller == 0 );
        CHECK( totals.pain == max_iters );
        CHECK( totals.focus == max_iters * 100 );
        CHECK( totals.str_bonus == max_iters * 7 );
        CHECK( totals.dex_bonus == max_iters * 7 );
        CHECK( totals.per_bonus == max_iters * 7 );
        CHECK( totals.int_bonus == max_iters * 8 );
        CHECK( totals.shakes > 0 );
        CHECK( totals.hallu == 0 );
    }
}

TEST_CASE( "check amphetamine addiction effects", "[addiction]" )
{
    Character &u = get_player_character();
    clear_character( u );
    clear_addictions( u );

    addict_effect_totals totals;

    SECTION( "intensity 5" ) {
        int res = suffer_addiction( addiction_amphetamine, 5, u, max_iters, totals );
        CHECK( res == Approx( 30000 ).margin( 10000 ) );
        CHECK( totals.health_mod < 0 );
        CHECK( totals.fatigue == 0 );
        CHECK( totals.morale < 0 );
        CHECK( totals.stim < 0 );
        CHECK( totals.pkiller == 0 );
        CHECK( totals.pain == 0 );
        CHECK( totals.focus == max_iters * 100 );
        CHECK( totals.str_bonus == max_iters * 7 );
        CHECK( totals.dex_bonus == max_iters * 8 );
        CHECK( totals.per_bonus == max_iters * 8 );
        CHECK( totals.int_bonus == max_iters * 7 );
        CHECK( totals.shakes >= 0 );
        CHECK( totals.hallu == 0 );
    }

    SECTION( "intensity 20" ) {
        int res = suffer_addiction( addiction_amphetamine, 20, u, max_iters, totals );
        CHECK( res == max_iters );
        CHECK( totals.health_mod < 0 );
        CHECK( totals.fatigue == 0 );
        CHECK( totals.morale < 0 );
        CHECK( totals.stim == -max_iters );
        CHECK( totals.pkiller == 0 );
        CHECK( totals.pain == 0 );
        CHECK( totals.focus == max_iters * 100 );
        CHECK( totals.str_bonus == max_iters * 7 );
        CHECK( totals.dex_bonus == max_iters * 8 );
        CHECK( totals.per_bonus == max_iters * 8 );
        CHECK( totals.int_bonus == max_iters * 7 );
        CHECK( totals.shakes > 0 );
        CHECK( totals.hallu > 0 );
    }
}

TEST_CASE( "check cocaine addiction effects", "[addiction]" )
{
    Character &u = get_player_character();
    clear_character( u );
    clear_addictions( u );

    addict_effect_totals totals;

    SECTION( "intensity 5" ) {
        int res = suffer_addiction( addiction_cocaine, 5, u, max_iters, totals );
        CHECK( res == Approx( 300 ).margin( 100 ) );
        CHECK( totals.health_mod == 0 );
        CHECK( totals.fatigue == 0 );
        CHECK( totals.morale < 0 );
        CHECK( totals.stim < 0 );
        CHECK( totals.pkiller == 0 );
        CHECK( totals.pain == 0 );
        CHECK( totals.focus == max_iters * 100 );
        CHECK( totals.str_bonus == max_iters * 8 );
        CHECK( totals.dex_bonus == max_iters * 8 );
        CHECK( totals.per_bonus == max_iters * 7 );
        CHECK( totals.int_bonus == max_iters * 7 );
        CHECK( totals.shakes == 0 );
        CHECK( totals.hallu == 0 );
    }

    SECTION( "intensity 20" ) {
        int res = suffer_addiction( addiction_cocaine, 20, u, max_iters, totals );
        CHECK( res == Approx( 3000 ).margin( 1000 ) );
        CHECK( totals.health_mod == 0 );
        CHECK( totals.fatigue == 0 );
        CHECK( totals.morale < 0 );
        CHECK( totals.stim < 0 );
        CHECK( totals.pkiller == 0 );
        CHECK( totals.pain == 0 );
        CHECK( totals.focus == max_iters * 100 );
        CHECK( totals.str_bonus == max_iters * 8 );
        CHECK( totals.dex_bonus == max_iters * 8 );
        CHECK( totals.per_bonus == max_iters * 7 );
        CHECK( totals.int_bonus == max_iters * 7 );
        CHECK( totals.shakes == 0 );
        CHECK( totals.hallu == 0 );
    }
}

TEST_CASE( "check crack addiction effects", "[addiction]" )
{
    Character &u = get_player_character();
    clear_character( u );
    clear_addictions( u );

    addict_effect_totals totals;

    SECTION( "intensity 5" ) {
        int res = suffer_addiction( addiction_crack, 5, u, max_iters, totals );
        CHECK( res == Approx( 300 ).margin( 100 ) );
        CHECK( totals.health_mod == 0 );
        CHECK( totals.fatigue == 0 );
        CHECK( totals.morale < 0 );
        CHECK( totals.stim < 0 );
        CHECK( totals.pkiller == 0 );
        CHECK( totals.pain == 0 );
        CHECK( totals.focus == max_iters * 100 );
        CHECK( totals.str_bonus == max_iters * 8 );
        CHECK( totals.dex_bonus == max_iters * 8 );
        CHECK( totals.per_bonus == max_iters * 7 );
        CHECK( totals.int_bonus == max_iters * 7 );
        CHECK( totals.shakes == 0 );
        CHECK( totals.hallu == 0 );
    }

    SECTION( "intensity 20" ) {
        int res = suffer_addiction( addiction_crack, 20, u, max_iters, totals );
        CHECK( res == Approx( 3000 ).margin( 1000 ) );
        CHECK( totals.health_mod == 0 );
        CHECK( totals.fatigue == 0 );
        CHECK( totals.morale < 0 );
        CHECK( totals.stim < 0 );
        CHECK( totals.pkiller == 0 );
        CHECK( totals.pain == 0 );
        CHECK( totals.focus == max_iters * 100 );
        CHECK( totals.str_bonus == max_iters * 8 );
        CHECK( totals.dex_bonus == max_iters * 8 );
        CHECK( totals.per_bonus == max_iters * 7 );
        CHECK( totals.int_bonus == max_iters * 7 );
        CHECK( totals.shakes == 0 );
        CHECK( totals.hallu == 0 );
    }
}

TEST_CASE( "check mutagen addiction effects", "[addiction]" )
{
    Character &u = get_player_character();
    clear_character( u );
    clear_addictions( u );

    addict_effect_totals totals;

    SECTION( "intensity 5" ) {
        int res = suffer_addiction( addiction_mutagen, 5, u, max_iters, totals );
        CHECK( res == 0 );
        CHECK( totals.health_mod == 0 );
        CHECK( totals.fatigue == 0 );
        CHECK( totals.morale < 0 );
        CHECK( totals.stim == 0 );
        CHECK( totals.pkiller == 0 );
        CHECK( totals.pain == 0 );
        CHECK( totals.focus == max_iters * 100 );
        CHECK( totals.str_bonus == max_iters * 8 );
        CHECK( totals.dex_bonus == max_iters * 8 );
        CHECK( totals.per_bonus == max_iters * 8 );
        CHECK( totals.int_bonus == max_iters * 8 );
        CHECK( totals.shakes == 0 );
        CHECK( totals.hallu == 0 );
    }

    SECTION( "intensity 20" ) {
        int res = suffer_addiction( addiction_mutagen, 20, u, max_iters, totals );
        CHECK( res == 0 );
        CHECK( totals.health_mod == 0 );
        CHECK( totals.fatigue == 0 );
        CHECK( totals.morale < 0 );
        CHECK( totals.stim == 0 );
        CHECK( totals.pkiller == 0 );
        CHECK( totals.pain == 0 );
        CHECK( totals.focus == max_iters * 100 );
        CHECK( totals.str_bonus == max_iters * 8 );
        CHECK( totals.dex_bonus == max_iters * 8 );
        CHECK( totals.per_bonus == max_iters * 8 );
        CHECK( totals.int_bonus == max_iters * 8 );
        CHECK( totals.shakes == 0 );
        CHECK( totals.hallu == 0 );
    }

    SECTION( "intensity 5, with MUT_JUNKIE" ) {
        u.toggle_trait( trait_MUT_JUNKIE );
        REQUIRE( u.has_trait( trait_MUT_JUNKIE ) );
        int res = suffer_addiction( addiction_mutagen, 5, u, max_iters, totals );
        CHECK( res == max_iters );
        CHECK( totals.health_mod == 0 );
        CHECK( totals.fatigue == 0 );
        CHECK( totals.morale < 0 );
        CHECK( totals.stim == 0 );
        CHECK( totals.pkiller == 0 );
        CHECK( totals.pain == 0 );
        CHECK( max_iters * 100 - totals.focus == Approx( 1000 ).margin( 800 ) );
        CHECK( totals.str_bonus == max_iters * 8 );
        CHECK( totals.dex_bonus == max_iters * 8 );
        CHECK( totals.per_bonus == max_iters * 8 );
        CHECK( totals.int_bonus == max_iters * 8 );
        CHECK( totals.shakes == 0 );
        CHECK( totals.hallu == 0 );
    }

    SECTION( "intensity 20, with MUT_JUNKIE" ) {
        u.toggle_trait( trait_MUT_JUNKIE );
        REQUIRE( u.has_trait( trait_MUT_JUNKIE ) );
        int res = suffer_addiction( addiction_mutagen, 20, u, max_iters, totals );
        CHECK( res == max_iters );
        CHECK( totals.health_mod == 0 );
        CHECK( totals.fatigue == 0 );
        CHECK( totals.morale < 0 );
        CHECK( totals.stim == 0 );
        CHECK( totals.pkiller == 0 );
        CHECK( totals.pain == 0 );
        CHECK( max_iters * 100 - totals.focus == Approx( 5000 ).margin( 2000 ) );
        CHECK( totals.str_bonus == max_iters * 8 );
        CHECK( totals.dex_bonus == max_iters * 8 );
        CHECK( totals.per_bonus == max_iters * 8 );
        CHECK( totals.int_bonus == max_iters * 8 );
        CHECK( totals.shakes == 0 );
        CHECK( totals.hallu == 0 );
    }
}

TEST_CASE( "check marloss addiction effects", "[addiction]" )
{
    Character &u = get_player_character();
    clear_character( u );
    clear_addictions( u );

    addict_effect_totals totals;

    SECTION( "intensity 5, RED" ) {
        int res = suffer_addiction( addiction_marloss_r, 5, u, max_iters, totals );
        CHECK( res == Approx( 120 ).margin( 100 ) );
        CHECK( totals.health_mod == 0 );
        CHECK( totals.fatigue == 0 );
        CHECK( totals.morale < 0 );
        CHECK( totals.stim == 0 );
        CHECK( totals.pkiller == 0 );
        CHECK( totals.pain == 0 );
        CHECK( max_iters * 100 - totals.focus > 0 );
        CHECK( totals.str_bonus == max_iters * 8 );
        CHECK( totals.dex_bonus == max_iters * 8 );
        CHECK( totals.per_bonus == max_iters * 8 );
        CHECK( totals.int_bonus == max_iters * 8 );
        CHECK( totals.shakes == 0 );
        CHECK( totals.hallu == 0 );
    }

    SECTION( "intensity 20, RED" ) {
        int res = suffer_addiction( addiction_marloss_r, 20, u, max_iters, totals );
        CHECK( res == Approx( 250 ).margin( 150 ) );
        CHECK( totals.health_mod == 0 );
        CHECK( totals.fatigue == 0 );
        CHECK( totals.morale < 0 );
        CHECK( totals.stim == 0 );
        CHECK( totals.pkiller == 0 );
        CHECK( totals.pain == 0 );
        CHECK( max_iters * 100 - totals.focus > 0 );
        CHECK( totals.str_bonus == max_iters * 8 );
        CHECK( totals.dex_bonus == max_iters * 8 );
        CHECK( totals.per_bonus == max_iters * 8 );
        CHECK( totals.int_bonus == max_iters * 8 );
        CHECK( totals.shakes == 0 );
        CHECK( totals.hallu == 0 );
    }

    SECTION( "intensity 5, BLUE" ) {
        int res = suffer_addiction( addiction_marloss_b, 5, u, max_iters, totals );
        CHECK( res == Approx( 120 ).margin( 100 ) );
        CHECK( totals.health_mod == 0 );
        CHECK( totals.fatigue == 0 );
        CHECK( totals.morale < 0 );
        CHECK( totals.stim == 0 );
        CHECK( totals.pkiller == 0 );
        CHECK( totals.pain == 0 );
        CHECK( max_iters * 100 - totals.focus > 0 );
        CHECK( totals.str_bonus == max_iters * 8 );
        CHECK( totals.dex_bonus == max_iters * 8 );
        CHECK( totals.per_bonus == max_iters * 8 );
        CHECK( totals.int_bonus == max_iters * 8 );
        CHECK( totals.shakes == 0 );
        CHECK( totals.hallu == 0 );
    }

    SECTION( "intensity 20, BLUE" ) {
        int res = suffer_addiction( addiction_marloss_b, 20, u, max_iters, totals );
        CHECK( res == Approx( 250 ).margin( 150 ) );
        CHECK( totals.health_mod == 0 );
        CHECK( totals.fatigue == 0 );
        CHECK( totals.morale < 0 );
        CHECK( totals.stim == 0 );
        CHECK( totals.pkiller == 0 );
        CHECK( totals.pain == 0 );
        CHECK( max_iters * 100 - totals.focus > 0 );
        CHECK( totals.str_bonus == max_iters * 8 );
        CHECK( totals.dex_bonus == max_iters * 8 );
        CHECK( totals.per_bonus == max_iters * 8 );
        CHECK( totals.int_bonus == max_iters * 8 );
        CHECK( totals.shakes == 0 );
        CHECK( totals.hallu == 0 );
    }

    SECTION( "intensity 5, YELLOW" ) {
        int res = suffer_addiction( addiction_marloss_y, 5, u, max_iters, totals );
        CHECK( res == Approx( 120 ).margin( 100 ) );
        CHECK( totals.health_mod == 0 );
        CHECK( totals.fatigue == 0 );
        CHECK( totals.morale < 0 );
        CHECK( totals.stim == 0 );
        CHECK( totals.pkiller == 0 );
        CHECK( totals.pain == 0 );
        CHECK( max_iters * 100 - totals.focus > 0 );
        CHECK( totals.str_bonus == max_iters * 8 );
        CHECK( totals.dex_bonus == max_iters * 8 );
        CHECK( totals.per_bonus == max_iters * 8 );
        CHECK( totals.int_bonus == max_iters * 8 );
        CHECK( totals.shakes == 0 );
        CHECK( totals.hallu == 0 );
    }

    SECTION( "intensity 20, YELLOW" ) {
        int res = suffer_addiction( addiction_marloss_y, 20, u, max_iters, totals );
        CHECK( res == Approx( 250 ).margin( 150 ) );
        CHECK( totals.health_mod == 0 );
        CHECK( totals.fatigue == 0 );
        CHECK( totals.morale < 0 );
        CHECK( totals.stim == 0 );
        CHECK( totals.pkiller == 0 );
        CHECK( totals.pain == 0 );
        CHECK( max_iters * 100 - totals.focus > 0 );
        CHECK( totals.str_bonus == max_iters * 8 );
        CHECK( totals.dex_bonus == max_iters * 8 );
        CHECK( totals.per_bonus == max_iters * 8 );
        CHECK( totals.int_bonus == max_iters * 8 );
        CHECK( totals.shakes == 0 );
        CHECK( totals.hallu == 0 );
    }
}
