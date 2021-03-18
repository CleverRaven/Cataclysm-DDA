#include "catch/catch.hpp"

#include "magic.h"
#include "magic_enchantment.h"
#include "map.h"
#include "map_helpers.h"
#include "item.h"
#include "options.h"
#include "player.h"
#include "player_helpers.h"

static trait_id trait_CARNIVORE( "CARNIVORE" );
static efftype_id effect_debug_clairvoyance( "debug_clairvoyance" );

static void advance_turn( Character &guy )
{
    guy.process_turn();
    calendar::turn += 1_turns;
}

static void give_item( Character &guy, const std::string &item_id )
{
    guy.i_add( item( item_id ) );
    guy.recalculate_enchantment_cache();
}

static void clear_items( Character &guy )
{
    guy.inv.clear();
    guy.recalculate_enchantment_cache();
}

TEST_CASE( "Enchantments grant mutations", "[magic][enchantment][trait][mutation]" )
{
    clear_map();
    Character &guy = get_player_character();
    clear_character( *guy.as_player(), true );

    guy.recalculate_enchantment_cache();
    advance_turn( guy );

    std::string s_relic = "test_relic_gives_trait";

    WHEN( "Character doesn't have trait" ) {
        REQUIRE( !guy.has_trait( trait_CARNIVORE ) );
        AND_WHEN( "Character receives relic" ) {
            give_item( guy, s_relic );
            THEN( "Character gains trait" ) {
                CHECK( guy.has_trait( trait_CARNIVORE ) );
            }
            AND_WHEN( "Turn passes" ) {
                advance_turn( guy );
                THEN( "Character still has trait" ) {
                    CHECK( guy.has_trait( trait_CARNIVORE ) );
                }
                AND_WHEN( "Character loses relic" ) {
                    clear_items( guy );
                    THEN( "Character loses trait" ) {
                        CHECK_FALSE( guy.has_trait( trait_CARNIVORE ) );
                    }
                    AND_WHEN( "Turn passes" ) {
                        advance_turn( guy );
                        THEN( "Character still has no trait" ) {
                            CHECK_FALSE( guy.has_trait( trait_CARNIVORE ) );
                        }
                    }
                }
            }
        }
    }

    WHEN( "Character has trait" ) {
        guy.set_mutation( trait_CARNIVORE );
        REQUIRE( guy.has_trait( trait_CARNIVORE ) );
        AND_WHEN( "Character receives relic" ) {
            give_item( guy, s_relic );
            THEN( "Nothing changes" ) {
                CHECK( guy.has_trait( trait_CARNIVORE ) );
            }
            AND_WHEN( "Turn passes" ) {
                advance_turn( guy );
                THEN( "Nothing changes" ) {
                    CHECK( guy.has_trait( trait_CARNIVORE ) );
                }
                AND_WHEN( "Character loses relic" ) {
                    clear_items( guy );
                    THEN( "Nothing changes" ) {
                        CHECK( guy.has_trait( trait_CARNIVORE ) );
                    }
                    AND_WHEN( "Turn passes" ) {
                        advance_turn( guy );
                        THEN( "Nothing changes" ) {
                            CHECK( guy.has_trait( trait_CARNIVORE ) );
                        }
                    }
                }
            }
        }
    }
}

TEST_CASE( "Enchantments apply effects", "[magic][enchantment][effect]" )
{
    clear_map();
    Character &guy = get_player_character();
    clear_character( *guy.as_player(), true );

    guy.recalculate_enchantment_cache();
    advance_turn( guy );

    std::string s_relic = "architect_cube";

    // TODO: multiple enchantments apply same effect of
    //       same or different intensity
    // TODO: enchantments apply effect while char already has effect of
    //       same, stronger or weaker intensity

    WHEN( "Character doesn't have effect" ) {
        REQUIRE( !guy.has_effect( effect_debug_clairvoyance ) );
        AND_WHEN( "Character receives relic" ) {
            give_item( guy, s_relic );
            THEN( "Character still doesn't have effect" ) {
                CHECK_FALSE( guy.has_effect( effect_debug_clairvoyance ) );
            }
            AND_WHEN( "Turn passes" ) {
                advance_turn( guy );
                THEN( "Character receives effect" ) {
                    CHECK( guy.has_effect( effect_debug_clairvoyance ) );
                }
                AND_WHEN( "Character loses relic" ) {
                    clear_items( guy );
                    THEN( "Character still has effect" ) {
                        CHECK( guy.has_effect( effect_debug_clairvoyance ) );
                    }
                    AND_WHEN( "Turn passes" ) {
                        advance_turn( guy );

                        // FIXME: effects should go away after 1 turn!
                        CHECK( guy.has_effect( effect_debug_clairvoyance ) );
                        advance_turn( guy );

                        THEN( "Character loses effect" ) {
                            CHECK_FALSE( guy.has_effect( effect_debug_clairvoyance ) );
                        }
                    }
                }
            }
        }
    }
}

static void tests_stats( Character &guy, int s_base, int d_base, int p_base, int i_base, int s_exp,
                         int d_exp, int p_exp, int i_exp )
{
    guy.str_max = s_base;
    guy.dex_max = d_base;
    guy.per_max = p_base;
    guy.int_max = i_base;

    guy.recalculate_enchantment_cache();
    advance_turn( guy );

    auto check_stats = [&]( int s, int d, int p, int i ) {
        REQUIRE( guy.get_str_base() == s_base );
        REQUIRE( guy.get_dex_base() == d_base );
        REQUIRE( guy.get_per_base() == p_base );
        REQUIRE( guy.get_int_base() == i_base );

        CHECK( guy.get_str() == s );
        CHECK( guy.get_dex() == d );
        CHECK( guy.get_per() == p );
        CHECK( guy.get_int() == i );
    };
    auto check_stats_base = [&]() {
        check_stats( s_base, d_base, p_base, i_base );
    };

    check_stats_base();

    std::string s_relic = "test_relic_mods_stats";

    WHEN( "Character receives relic" ) {
        give_item( guy, s_relic );
        THEN( "Stats remain unchanged" ) {
            check_stats_base();
        }
        AND_WHEN( "Turn passes" ) {
            advance_turn( guy );
            THEN( "Stats are modified and don't overflow" ) {
                check_stats( s_exp, d_exp, p_exp, i_exp );
            }
            AND_WHEN( "Character loses relic" ) {
                clear_items( guy );
                THEN( "Stats remain unchanged" ) {
                    check_stats( s_exp, d_exp, p_exp, i_exp );
                }
                AND_WHEN( "Turn passes" ) {
                    advance_turn( guy );
                    THEN( "Stats return to normal" ) {
                        check_stats_base();
                    }
                }
            }
        }
    }
}

TEST_CASE( "Enchantments modify stats", "[magic][enchantment][character]" )
{
    clear_map();
    Character &guy = get_player_character();
    clear_character( *guy.as_player(), true );

    SECTION( "base stats 8" ) {
        tests_stats( guy, 8, 8, 8, 8, 20, 6, 5, 0 );
    }
    SECTION( "base stats 12" ) {
        tests_stats( guy, 12, 12, 12, 12, 28, 10, 7, 1 );
    }
    SECTION( "base stats 4" ) {
        tests_stats( guy, 4, 4, 4, 4, 12, 2, 3, 0 );
    }
}

static void tests_speed( Character &guy, int sp_base, int sp_exp )
{
    guy.recalculate_enchantment_cache();
    guy.set_speed_base( sp_base );
    guy.set_speed_bonus( 0 );

    guy.set_moves( 0 );
    advance_turn( guy );

    std::string s_relic = "test_relic_mods_speed";

    auto check_speed = [&]( int speed, int moves ) {
        REQUIRE( guy.get_speed_base() == sp_base );
        REQUIRE( guy.get_speed() == speed );
        REQUIRE( guy.get_moves() == moves );
    };
    auto check_speed_is_base = [&] {
        check_speed( sp_base, sp_base );
    };

    WHEN( "Character has no relics" ) {
        THEN( "Speed and moves gain equel base" ) {
            check_speed_is_base();
        }
    }
    WHEN( "Character receives relic" ) {
        give_item( guy, s_relic );
        THEN( "Nothing changes" ) {
            check_speed_is_base();
        }
        AND_WHEN( "Turn passes" ) {
            guy.set_moves( 0 );
            advance_turn( guy );
            THEN( "Speed changes, moves gain changes" ) {
                check_speed( sp_exp, sp_exp );
            }
            AND_WHEN( "Character loses relic" ) {
                clear_items( guy );
                THEN( "Nothing changes" ) {
                    check_speed( sp_exp, sp_exp );
                }
                AND_WHEN( "Turn passes" ) {
                    guy.set_moves( 0 );
                    advance_turn( guy );
                    THEN( "Speed and moves gain return to normal" ) {
                        check_speed_is_base();
                    }
                }
            }
        }
    }
    WHEN( "Character receives 10 relics" ) {
        for( int i = 0; i < 10; i++ ) {
            give_item( guy, s_relic );
        }
        THEN( "Nothing changes" ) {
            check_speed_is_base();
        }
        AND_WHEN( "Turn passes" ) {
            guy.set_moves( 0 );
            advance_turn( guy );
            THEN( "Speed and moves gain do not fall below 25% of base" ) {
                check_speed( sp_base / 4, sp_base / 4 );
            }
        }
    }
}

TEST_CASE( "Enchantments modify speed", "[magic][enchantment][speed]" )
{
    clear_map();
    Character &guy = get_player_character();
    clear_character( *guy.as_player(), true );

    SECTION( "base = 100" ) {
        tests_speed( guy, 100, 75 );
    }
    SECTION( "base = 80" ) {
        tests_speed( guy, 80, 65 );
    }
    SECTION( "base = 120" ) {
        tests_speed( guy, 120, 85 );
    }
}

static void tests_attack_cost( Character &guy, const item &weap, int item_atk_cost,
                               int guy_atk_cost, int exp_guy_atk_cost )
{
    guy.recalculate_enchantment_cache();
    advance_turn( guy );

    REQUIRE( weap.attack_cost() == item_atk_cost );
    REQUIRE( guy.attack_cost( weap ) == guy_atk_cost );

    std::string s_relic = "test_relic_mods_atk_cost";

    WHEN( "Character receives relic" ) {
        give_item( guy, s_relic );
        THEN( "Attack cost changes" ) {
            CHECK( guy.attack_cost( weap ) == exp_guy_atk_cost );
        }
        AND_WHEN( "Character loses relic" ) {
            clear_items( guy );
            THEN( "Attack cost returns to normal" ) {
                CHECK( guy.attack_cost( weap ) == guy_atk_cost );
            }
        }
    }
    WHEN( "Character receives 10 relics" ) {
        for( int i = 0; i < 10; i++ ) {
            give_item( guy, s_relic );
        }
        THEN( "Attack cost does not drop below 25" ) {
            CHECK( guy.attack_cost( weap ) == 25 );
        }
    }
}

TEST_CASE( "Enchantments modify attack cost", "[magic][enchantment][melee]" )
{
    clear_map();
    Character &guy = get_player_character();
    clear_character( *guy.as_player(), true );

    SECTION( "normal sword" ) {
        tests_attack_cost( guy, item( "test_normal_sword" ), 101, 96, 77 );
    }
    SECTION( "normal sword + ITEM_ATTACK_COST" ) {
        tests_attack_cost( guy, item( "test_relic_sword" ), 86, 82, 66 );
    }
}

static void tests_move_cost( Character &guy, int tile_move_cost, int move_cost, int exp_move_cost )
{
    guy.recalculate_enchantment_cache();
    advance_turn( guy );

    std::string s_relic = "test_relic_mods_mv_cost";

    REQUIRE( guy.run_cost( tile_move_cost ) == move_cost );

    WHEN( "Character receives relic" ) {
        give_item( guy, s_relic );
        THEN( "Move cost changes" ) {
            CHECK( guy.run_cost( tile_move_cost ) == exp_move_cost );
        }
        AND_WHEN( "Character loses relic" ) {
            clear_items( guy );
            THEN( "Move cost goes back to normal" ) {
                CHECK( guy.run_cost( tile_move_cost ) == move_cost );
            }
        }
    }
    WHEN( "Character receives 15 relics" ) {
        for( int i = 0; i < 15; i++ ) {
            give_item( guy, s_relic );
        }
        THEN( "Move cost does not drop below 20" ) {
            CHECK( guy.run_cost( tile_move_cost ) == 20 );
        }
    }
}

TEST_CASE( "Enchantments modify move cost", "[magic][enchantment][move]" )
{
    clear_map();
    Character &guy = get_player_character();
    clear_character( *guy.as_player(), true );

    SECTION( "Naked character" ) {
        SECTION( "tile move cost = 100" ) {
            tests_move_cost( guy, 100, 116, 104 );
        }
        SECTION( "tile move cost = 120" ) {
            tests_move_cost( guy, 120, 136, 122 );
        }
    }
    SECTION( "Naked character with PADDED_FEET" ) {
        trait_id tr( "PADDED_FEET" );
        guy.set_mutation( tr );
        REQUIRE( guy.has_trait( tr ) );

        SECTION( "tile move cost = 100" ) {
            tests_move_cost( guy, 100, 90, 81 );
        }
        SECTION( "tile move cost = 120" ) {
            tests_move_cost( guy, 120, 108, 97 );
        }
    }
}

static void tests_metabolic_rate( Character &guy, float norm, float exp )
{
    guy.recalculate_enchantment_cache();
    advance_turn( guy );

    std::string s_relic = "test_relic_mods_metabolism";

    REQUIRE( guy.metabolic_rate_base() == Approx( norm ) );

    WHEN( "Character receives relic" ) {
        give_item( guy, s_relic );
        THEN( "Metabolic rate changes" ) {
            CHECK( guy.metabolic_rate_base() == Approx( exp ) );
            AND_WHEN( "Character loses relic" ) {
                clear_items( guy );
                THEN( "Metabolic rate goes back to normal" ) {
                    CHECK( guy.metabolic_rate_base() == Approx( norm ) );
                }
            }
        }
    }
    WHEN( "Character receives 15 relics" ) {
        for( int i = 0; i < 15; i++ ) {
            give_item( guy, s_relic );
        }
        THEN( "Metabolic rate does not go below 0" ) {
            CHECK( guy.metabolic_rate_base() == Approx( 0.0f ) );
        }
    }
}

TEST_CASE( "Enchantments modify metabolic rate", "[magic][enchantment][metabolism]" )
{
    clear_map();
    Character &guy = get_player_character();
    clear_character( *guy.as_player(), true );

    guy.recalculate_enchantment_cache();
    advance_turn( guy );

    const float normal_mr = get_option<float>( "PLAYER_HUNGER_RATE" );
    REQUIRE( guy.metabolic_rate_base() == normal_mr );
    REQUIRE( normal_mr == 1.0f );

    SECTION( "Clean character" ) {
        tests_metabolic_rate( guy, 1.0f, 0.9f );
    }
    SECTION( "Character with HUNGER trait" ) {
        trait_id tr( "HUNGER" );
        guy.set_mutation( tr );
        REQUIRE( guy.has_trait( tr ) );

        tests_metabolic_rate( guy, 1.5f, 1.35f );
    }
}

struct mana_test_case {
    int idx;
    int intellect;
    units::energy power_level;
    int norm_cap;
    int exp_cap;
    double norm_regen_amt_8h;
    double exp_regen_amt_8h;
};

static const std::vector<mana_test_case> mana_test_data = {{
        {0, 8, 0_kJ, 1000, 800, 1000.0, 560.0},
        {1, 12, 0_kJ, 1400, 1080, 1400.0, 686.0},
        {2, 8, 250_kJ, 750, 450, 750.0, 385.0},
        {3, 12, 250_kJ, 1150, 830, 1150.0, 581.0},
        {4, 8, 1250_kJ, 0, 0, 0.0, 0.0},
        {5, 16, 1250_kJ, 550, 110, 550.0, 77.0},
    }
};

static void tests_mana_pool( Character &guy, const mana_test_case &t )
{
    double norm_regen_rate = t.norm_regen_amt_8h / to_turns<double>( time_duration::from_hours( 8 ) );
    double exp_regen_rate = t.exp_regen_amt_8h / to_turns<double>( time_duration::from_hours( 8 ) );

    guy.recalculate_enchantment_cache();
    advance_turn( guy );

    guy.set_max_power_level( 2000_kJ );
    REQUIRE( guy.get_max_power_level() == 2000_kJ );

    guy.set_power_level( t.power_level );
    REQUIRE( guy.get_power_level() == t.power_level );

    guy.int_max = t.intellect;
    guy.int_cur = guy.int_max;
    REQUIRE( guy.get_int() == t.intellect );

    REQUIRE( guy.magic->max_mana( guy ) == t.norm_cap );
    REQUIRE( guy.magic->mana_regen_rate( guy ) == Approx( norm_regen_rate ) );

    const std::string s_relic = "test_relic_mods_manapool";

    WHEN( "Character receives relic" ) {
        give_item( guy, s_relic );
        THEN( "Mana pool capacity and regen rate change" ) {
            CHECK( guy.magic->max_mana( guy ) == t.exp_cap );
            CHECK( guy.magic->mana_regen_rate( guy ) == Approx( exp_regen_rate ) );
            AND_WHEN( "Character loses relic" ) {
                clear_items( guy );
                THEN( "Mana pool capacity and regen rate go back to normal" ) {
                    REQUIRE( guy.magic->max_mana( guy ) == t.norm_cap );
                    REQUIRE( guy.magic->mana_regen_rate( guy ) == Approx( norm_regen_rate ) );
                }
            }
        }
    }
    WHEN( "Character receives 10 relics" ) {
        for( int i = 0; i < 10; i++ ) {
            give_item( guy, s_relic );
        }
        THEN( "Mana pool capacity and regen rate don't drop below 0" ) {
            REQUIRE( guy.magic->max_mana( guy ) == 0 );
            REQUIRE( guy.magic->mana_regen_rate( guy ) == Approx( 0.0 ) );
        }
    }
}

static void tests_mana_pool_section( const mana_test_case &t )
{
    CAPTURE( t.idx );

    clear_map();
    Character &guy = get_player_character();
    clear_character( *guy.as_player(), true );

    tests_mana_pool( guy, t );
}

TEST_CASE( "Mana pool", "[magic][enchantment][mana][bionic]" )
{
    for( const mana_test_case &it : mana_test_data ) {
        tests_mana_pool_section( it );
    }
}
