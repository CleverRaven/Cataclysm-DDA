#include <memory>
#include <utility>

#include "avatar.h"
#include "avatar_action.h"
#include "cata_catch.h"
#include "creature.h"
#include "game.h"
#include "map.h"
#include "map_helpers.h"
#include "mutation.h"
#include "profession.h"
#include "skill.h"
#include "player_helpers.h"
#include "type_id.h"

static void setup_test_lake()
{
    const ter_id t_water_dp( "t_water_dp" );
    const ter_id t_water_cube( "t_water_cube" );
    const ter_id t_lake_bed( "t_lake_bed" );

    build_water_test_map( t_water_dp, t_water_cube, t_lake_bed );
    const map &here = get_map();
    constexpr tripoint test_origin( 60, 60, 0 );

    REQUIRE( here.ter( test_origin ) == t_water_dp );
    REQUIRE( here.ter( test_origin + tripoint_below ) == t_water_cube );
    REQUIRE( here.ter( test_origin + tripoint( 0, 0, -2 ) ) == t_lake_bed );
}

TEST_CASE( "avatar_diving", "[diving]" )
{
    clear_avatar();
    setup_test_lake();

    Character &dummy = get_player_character();
    map &here = get_map();
    constexpr tripoint test_origin( 60, 60, 0 );

    GIVEN( "avatar is above water at z0" ) {
        dummy.set_underwater( false );
        dummy.setpos( test_origin );
        g->vertical_shift( 0 );

        WHEN( "avatar dives down" ) {
            g->vertical_move( -1, false );

            THEN( "avatar is underwater at z0" ) {
                REQUIRE( dummy.pos() == test_origin );
                REQUIRE( here.ter( dummy.pos() ) == ter_id( "t_water_dp" ) );
                REQUIRE( dummy.is_underwater() );
            }
        }

        WHEN( "avatar swims up" ) {
            g->vertical_move( 1, false );

            THEN( "avatar is not underwater at z0" ) {
                REQUIRE( dummy.pos() == test_origin );
                REQUIRE( here.ter( dummy.pos() ) == ter_id( "t_water_dp" ) );
                REQUIRE( !dummy.is_underwater() );
            }
        }
    }

    GIVEN( "avatar is underwater at z0" ) {
        dummy.set_underwater( true );
        dummy.setpos( test_origin );
        g->vertical_shift( 0 );

        WHEN( "avatar dives down" ) {
            g->vertical_move( -1, false );

            THEN( "avatar is underwater at z-1" ) {
                REQUIRE( dummy.pos() == test_origin + tripoint_below );
                REQUIRE( here.ter( dummy.pos() ) == ter_id( "t_water_cube" ) );
                REQUIRE( dummy.is_underwater() );
            }
        }

        WHEN( "avatar swims up" ) {
            g->vertical_move( 1, false );

            THEN( "avatar is not underwater at z0" ) {
                REQUIRE( dummy.pos() == test_origin );
                REQUIRE( here.ter( dummy.pos() ) == ter_id( "t_water_dp" ) );
                REQUIRE( !dummy.is_underwater() );
            }
        }
    }

    GIVEN( "avatar is underwater at z-1" ) {
        dummy.set_underwater( true );
        dummy.setpos( test_origin + tripoint_below );
        g->vertical_shift( -1 );

        WHEN( "avatar dives down" ) {
            g->vertical_move( -1, false );

            THEN( "avatar is underwater at z-2" ) {
                REQUIRE( dummy.pos() == test_origin + tripoint( 0, 0, -2 ) );
                REQUIRE( here.ter( dummy.pos() ) == ter_id( "t_lake_bed" ) );
                REQUIRE( dummy.is_underwater() );
            }
        }

        WHEN( "avatar swims up" ) {
            g->vertical_move( 1, false );

            THEN( "avatar is underwater at z0" ) {
                REQUIRE( dummy.pos() == test_origin );
                REQUIRE( here.ter( dummy.pos() ) == ter_id( "t_water_dp" ) );
                REQUIRE( dummy.is_underwater() );
            }
        }
    }

    GIVEN( "avatar is underwater at z-2" ) {
        dummy.set_underwater( true );
        dummy.setpos( test_origin + tripoint( 0, 0, -2 ) );
        g->vertical_shift( -2 );

        WHEN( "avatar dives down" ) {
            g->vertical_move( -1, false );

            THEN( "avatar is underwater at z-2" ) {
                REQUIRE( dummy.pos() == test_origin + tripoint( 0, 0, -2 ) );
                REQUIRE( here.ter( dummy.pos() ) == ter_id( "t_lake_bed" ) );
                REQUIRE( dummy.is_underwater() );
            }
        }

        WHEN( "avatar swims up" ) {
            g->vertical_move( 1, false );

            THEN( "avatar is underwater at z-1" ) {
                REQUIRE( dummy.pos() == test_origin + tripoint_below );
                REQUIRE( here.ter( dummy.pos() ) == ter_id( "t_water_cube" ) );
                REQUIRE( dummy.is_underwater() );
            }
        }
    }

    // Put us back at 0. We shouldn't have to do this but other tests are
    // making assumptions about what z-level they're on.
    g->vertical_shift( 0 );
}

static const efftype_id effect_winded( "winded" );
static const move_mode_id move_mode_crouch( "crouch" );
static const move_mode_id move_mode_prone( "prone" );
static const move_mode_id move_mode_run( "run" );
static const move_mode_id move_mode_walk( "walk" );
static const skill_id skill_swimming( "swimming" );
static const trait_id trait_DISIMMUNE( "DISIMMUNE" );

struct swimmer_stats {
    int strength = 0;
    int dexterity = 0;
};

struct swimmer_skills {
    int athletics = 0;
};

struct swimmer_gear {
    std::vector<std::string> worn;
};

struct swimmer_traits {
    std::vector<std::string> traits;
};

struct swimmer_config {
    std::string stats_name;
    std::string skills_name;
    std::string gear_name;
    std::string traits_name;
    swimmer_stats stats{};
    swimmer_skills skills{};
    swimmer_gear gear;
    swimmer_traits traits;
    const profession *prof = nullptr;

    swimmer_config() = default;

    swimmer_config( std::string stats_name,
                    std::string skills_name,
                    std::string gear_name,
                    std::string traits_name,
                    swimmer_stats stats,
                    swimmer_skills skills,
                    swimmer_gear gear,
                    swimmer_traits traits ) :
        stats_name( std::move( stats_name ) ), skills_name( std::move( skills_name ) ),
        gear_name( std::move( gear_name ) ),
        traits_name( std::move( traits_name ) ), stats( stats ), skills( skills ),
        gear( std::move( gear ) ),
        traits( std::move( traits ) ) {
    }

    swimmer_config( swimmer_stats stats,
                    swimmer_skills skills,
                    swimmer_gear gear,
                    swimmer_traits traits ): stats( stats ), skills( skills ), gear( std::move( gear ) ),
        traits( std::move( traits ) )
    {}
};

struct swim_result {
    int move_cost = 0;
    int steps = 0;
};

static const std::unordered_map<std::string, swimmer_stats> stats_map = {
    {"minimum", { 4, 4 }},
    {"average", {8, 8 }},
    {"maximum", {20, 20 }},
};

static const std::unordered_map<std::string, swimmer_skills> skills_map = {
    {"none", { 0 }},
    {"professional", {5 }},
    {"maximum", {10 }},
};

static const std::unordered_map<std::string, swimmer_gear> gear_map = {
    {"none", {}},
    {"fins", {{"swim_fins"}}},
    {"flotation vest", {{"flotation_vest"}}},
};

static const std::unordered_map<std::string, swimmer_traits> traits_map = {
    {"none", {}},
    {"paws", {{"PAWS"}}},
    {"large paws", {{"PAWS_LARGE"}}},
    {"webbed hands", {{"WEBBED"}}},
    {"webbed hands and feet", {{"WEBBED", "WEBBED_FEET"}}},
};

struct swim_scenario {
    move_mode_id move_mode;
    swimmer_config config;

    swim_scenario( const move_mode_id move_mode,
                   const std::string &stats_key, const std::string &skills_key, const std::string &gear_key,
                   const std::string
                   & traits_key ) :
        move_mode( move_mode ) {
        config = swimmer_config( stats_key, skills_key, gear_key, traits_key, stats_map.at( stats_key ),
                                 skills_map.at( skills_key ), gear_map.at( gear_key ), traits_map.at( traits_key ) );
    }

    std::string name() const {
        return string_format( "move: %s, stats: %s, skills: %s, gear: %s, traits: %s", move_mode.str(),
                              config.stats_name, config.skills_name, config.gear_name, config.traits_name );
    }
};

static int swimming_steps( avatar &swimmer )
{
    map &here = get_map();
    const tripoint left = swimmer.pos();
    const tripoint right = left + tripoint_east;
    int steps = 0;
    constexpr int STOP_STEPS = 9000;
    int last_moves = swimmer.get_speed();
    int last_stamina = swimmer.get_stamina_max();
    swimmer.moves = last_moves;
    swimmer.set_stamina( last_stamina );
    while( swimmer.get_stamina() > 0 && !swimmer.has_effect( effect_winded ) && steps < STOP_STEPS ) {
        if( steps % 2 == 0 ) {
            REQUIRE( swimmer.pos() == left );
            REQUIRE( avatar_action::move( swimmer, here, tripoint_east ) );
        } else {
            REQUIRE( swimmer.pos() == right );
            REQUIRE( avatar_action::move( swimmer, here, tripoint_west ) );
        }
        ++steps;
        REQUIRE( swimmer.moves < last_moves );
        if( swimmer.moves <= 0 ) {
            calendar::turn += 1_turns;
            const int stamina_cost = last_stamina - swimmer.get_stamina();
            REQUIRE( stamina_cost > 0 );
            swimmer.update_body();
            swimmer.process_turn();
            last_stamina = swimmer.get_stamina();
        }
        last_moves = swimmer.moves;
    }
    swimmer.setpos( left );
    return steps;
}

static void configure_swimmer( avatar &swimmer, const move_mode_id move_mode,
                               const swimmer_config &config )
{
    swimmer.str_max = config.stats.strength;
    swimmer.dex_max = config.stats.dexterity;
    swimmer.set_str_bonus( 0 );
    swimmer.set_dex_bonus( 0 );

    swimmer.set_skill_level( skill_swimming, config.skills.athletics );
    SkillLevel &level = swimmer.get_skill_level_object( skill_swimming );
    if( level.isTraining() ) {
        level.toggleTraining();
    }

    for( const std::string &trait : config.traits.traits ) {
        swimmer.toggle_trait( trait_id( trait ) );
    }

    for( const std::string &worn : config.gear.worn ) {
        swimmer.wear_item( item( worn ), false );
    }

    if( config.prof != nullptr ) {
        swimmer.prof = config.prof;
        swimmer.add_profession_items();
    }

    swimmer.toggle_trait( trait_DISIMMUNE ); // random diseases can flake the test
    swimmer.set_movement_mode( move_mode );
}

static swim_result swim( avatar &swimmer, const move_mode_id move_mode,
                         const swimmer_config &config )
{
    clear_avatar();
    configure_swimmer( swimmer, move_mode, config );

    const swim_result result{ swimmer.swim_speed(), swimming_steps( swimmer ) };
    return result;
}

// Combinatorial explosion! Generate all possible combinations of stats, skills, gear, and
// traits based our our predefined entries (which represent the things that influence swimming
// in the current logic), and then combine them with each possible move mode.
static std::vector<swim_scenario> generate_scenarios()
{
    std::vector<swim_scenario> scenarios;

    const std::vector<move_mode_id> move_modes = { move_mode_walk, move_mode_crouch, move_mode_run, move_mode_prone };

    for( const move_mode_id &move_mode : move_modes ) {
        for( const std::pair<const std::string, swimmer_stats> &stats : stats_map ) {
            for( const std::pair<const std::string, swimmer_skills> &skills : skills_map ) {
                for( const std::pair<const std::string, swimmer_gear> &gear : gear_map ) {
                    for( const std::pair<const std::string, swimmer_traits> &traits : traits_map ) {
                        scenarios.emplace_back( move_mode, stats.first, skills.first, gear.first,
                                                traits.first );
                    }
                }
            }
        }
    }

    return scenarios;
}

// Generate the contents of this map using the "generate swim move cost and distance values" test below.
static std::map<std::string, swim_result> expected_results = {
    {"move: crouch, stats: average, skills: maximum, gear: fins, traits: large paws", swim_result{30, 9000}},
    {"move: crouch, stats: average, skills: maximum, gear: fins, traits: none", swim_result{30, 9000}},
    {"move: crouch, stats: average, skills: maximum, gear: fins, traits: paws", swim_result{30, 9000}},
    {"move: crouch, stats: average, skills: maximum, gear: fins, traits: webbed hands", swim_result{30, 9000}},
    {"move: crouch, stats: average, skills: maximum, gear: fins, traits: webbed hands and feet", swim_result{30, 9000}},
    {"move: crouch, stats: average, skills: maximum, gear: flotation vest, traits: large paws", swim_result{250, 382}},
    {"move: crouch, stats: average, skills: maximum, gear: flotation vest, traits: none", swim_result{250, 382}},
    {"move: crouch, stats: average, skills: maximum, gear: flotation vest, traits: paws", swim_result{250, 382}},
    {"move: crouch, stats: average, skills: maximum, gear: flotation vest, traits: webbed hands", swim_result{250, 382}},
    {"move: crouch, stats: average, skills: maximum, gear: flotation vest, traits: webbed hands and feet", swim_result{250, 382}},
    {"move: crouch, stats: average, skills: maximum, gear: none, traits: large paws", swim_result{30, 9000}},
    {"move: crouch, stats: average, skills: maximum, gear: none, traits: none", swim_result{30, 9000}},
    {"move: crouch, stats: average, skills: maximum, gear: none, traits: paws", swim_result{30, 9000}},
    {"move: crouch, stats: average, skills: maximum, gear: none, traits: webbed hands", swim_result{30, 9000}},
    {"move: crouch, stats: average, skills: maximum, gear: none, traits: webbed hands and feet", swim_result{30, 9000}},
    {"move: crouch, stats: average, skills: none, gear: fins, traits: large paws", swim_result{390, 60}},
    {"move: crouch, stats: average, skills: none, gear: fins, traits: none", swim_result{419, 56}},
    {"move: crouch, stats: average, skills: none, gear: fins, traits: paws", swim_result{387, 61}},
    {"move: crouch, stats: average, skills: none, gear: fins, traits: webbed hands", swim_result{370, 64}},
    {"move: crouch, stats: average, skills: none, gear: fins, traits: webbed hands and feet", swim_result{370, 64}},
    {"move: crouch, stats: average, skills: none, gear: flotation vest, traits: large paws", swim_result{450, 51}},
    {"move: crouch, stats: average, skills: none, gear: flotation vest, traits: none", swim_result{450, 51}},
    {"move: crouch, stats: average, skills: none, gear: flotation vest, traits: paws", swim_result{450, 51}},
    {"move: crouch, stats: average, skills: none, gear: flotation vest, traits: webbed hands", swim_result{450, 51}},
    {"move: crouch, stats: average, skills: none, gear: flotation vest, traits: webbed hands and feet", swim_result{450, 51}},
    {"move: crouch, stats: average, skills: none, gear: none, traits: large paws", swim_result{390, 60}},
    {"move: crouch, stats: average, skills: none, gear: none, traits: none", swim_result{410, 57}},
    {"move: crouch, stats: average, skills: none, gear: none, traits: paws", swim_result{384, 62}},
    {"move: crouch, stats: average, skills: none, gear: none, traits: webbed hands", swim_result{364, 65}},
    {"move: crouch, stats: average, skills: none, gear: none, traits: webbed hands and feet", swim_result{314, 78}},
    {"move: crouch, stats: average, skills: professional, gear: fins, traits: large paws", swim_result{53, 9000}},
    {"move: crouch, stats: average, skills: professional, gear: fins, traits: none", swim_result{105, 9000}},
    {"move: crouch, stats: average, skills: professional, gear: fins, traits: paws", swim_result{60, 9000}},
    {"move: crouch, stats: average, skills: professional, gear: fins, traits: webbed hands", swim_result{56, 9000}},
    {"move: crouch, stats: average, skills: professional, gear: fins, traits: webbed hands and feet", swim_result{56, 9000}},
    {"move: crouch, stats: average, skills: professional, gear: flotation vest, traits: large paws", swim_result{450, 85}},
    {"move: crouch, stats: average, skills: professional, gear: flotation vest, traits: none", swim_result{450, 85}},
    {"move: crouch, stats: average, skills: professional, gear: flotation vest, traits: paws", swim_result{450, 85}},
    {"move: crouch, stats: average, skills: professional, gear: flotation vest, traits: webbed hands", swim_result{450, 85}},
    {"move: crouch, stats: average, skills: professional, gear: flotation vest, traits: webbed hands and feet", swim_result{431, 90}},
    {"move: crouch, stats: average, skills: professional, gear: none, traits: large paws", swim_result{119, 1071}},
    {"move: crouch, stats: average, skills: professional, gear: none, traits: none", swim_result{160, 413}},
    {"move: crouch, stats: average, skills: professional, gear: none, traits: paws", swim_result{122, 947}},
    {"move: crouch, stats: average, skills: professional, gear: none, traits: webbed hands", swim_result{114, 9000}},
    {"move: crouch, stats: average, skills: professional, gear: none, traits: webbed hands and feet", swim_result{64, 9000}},
    {"move: crouch, stats: maximum, skills: maximum, gear: fins, traits: large paws", swim_result{30, 9000}},
    {"move: crouch, stats: maximum, skills: maximum, gear: fins, traits: none", swim_result{30, 9000}},
    {"move: crouch, stats: maximum, skills: maximum, gear: fins, traits: paws", swim_result{30, 9000}},
    {"move: crouch, stats: maximum, skills: maximum, gear: fins, traits: webbed hands", swim_result{30, 9000}},
    {"move: crouch, stats: maximum, skills: maximum, gear: fins, traits: webbed hands and feet", swim_result{30, 9000}},
    {"move: crouch, stats: maximum, skills: maximum, gear: flotation vest, traits: large paws", swim_result{250, 382}},
    {"move: crouch, stats: maximum, skills: maximum, gear: flotation vest, traits: none", swim_result{250, 382}},
    {"move: crouch, stats: maximum, skills: maximum, gear: flotation vest, traits: paws", swim_result{250, 382}},
    {"move: crouch, stats: maximum, skills: maximum, gear: flotation vest, traits: webbed hands", swim_result{250, 382}},
    {"move: crouch, stats: maximum, skills: maximum, gear: flotation vest, traits: webbed hands and feet", swim_result{250, 382}},
    {"move: crouch, stats: maximum, skills: maximum, gear: none, traits: large paws", swim_result{30, 9000}},
    {"move: crouch, stats: maximum, skills: maximum, gear: none, traits: none", swim_result{30, 9000}},
    {"move: crouch, stats: maximum, skills: maximum, gear: none, traits: paws", swim_result{30, 9000}},
    {"move: crouch, stats: maximum, skills: maximum, gear: none, traits: webbed hands", swim_result{30, 9000}},
    {"move: crouch, stats: maximum, skills: maximum, gear: none, traits: webbed hands and feet", swim_result{30, 9000}},
    {"move: crouch, stats: maximum, skills: none, gear: fins, traits: large paws", swim_result{30, 1715}},
    {"move: crouch, stats: maximum, skills: none, gear: fins, traits: none", swim_result{109, 367}},
    {"move: crouch, stats: maximum, skills: none, gear: fins, traits: paws", swim_result{30, 1715}},
    {"move: crouch, stats: maximum, skills: none, gear: fins, traits: webbed hands", swim_result{30, 1715}},
    {"move: crouch, stats: maximum, skills: none, gear: fins, traits: webbed hands and feet", swim_result{30, 1715}},
    {"move: crouch, stats: maximum, skills: none, gear: flotation vest, traits: large paws", swim_result{450, 51}},
    {"move: crouch, stats: maximum, skills: none, gear: flotation vest, traits: none", swim_result{450, 51}},
    {"move: crouch, stats: maximum, skills: none, gear: flotation vest, traits: paws", swim_result{450, 51}},
    {"move: crouch, stats: maximum, skills: none, gear: flotation vest, traits: webbed hands", swim_result{450, 51}},
    {"move: crouch, stats: maximum, skills: none, gear: flotation vest, traits: webbed hands and feet", swim_result{450, 51}},
    {"move: crouch, stats: maximum, skills: none, gear: none, traits: large paws", swim_result{218, 123}},
    {"move: crouch, stats: maximum, skills: none, gear: none, traits: none", swim_result{290, 86}},
    {"move: crouch, stats: maximum, skills: none, gear: none, traits: paws", swim_result{226, 117}},
    {"move: crouch, stats: maximum, skills: none, gear: none, traits: webbed hands", swim_result{214, 126}},
    {"move: crouch, stats: maximum, skills: none, gear: none, traits: webbed hands and feet", swim_result{134, 251}},
    {"move: crouch, stats: maximum, skills: professional, gear: fins, traits: large paws", swim_result{30, 9000}},
    {"move: crouch, stats: maximum, skills: professional, gear: fins, traits: none", swim_result{30, 9000}},
    {"move: crouch, stats: maximum, skills: professional, gear: fins, traits: paws", swim_result{30, 9000}},
    {"move: crouch, stats: maximum, skills: professional, gear: fins, traits: webbed hands", swim_result{30, 9000}},
    {"move: crouch, stats: maximum, skills: professional, gear: fins, traits: webbed hands and feet", swim_result{30, 9000}},
    {"move: crouch, stats: maximum, skills: professional, gear: flotation vest, traits: large paws", swim_result{315, 135}},
    {"move: crouch, stats: maximum, skills: professional, gear: flotation vest, traits: none", swim_result{407, 96}},
    {"move: crouch, stats: maximum, skills: professional, gear: flotation vest, traits: paws", swim_result{332, 125}},
    {"move: crouch, stats: maximum, skills: professional, gear: flotation vest, traits: webbed hands", swim_result{331, 126}},
    {"move: crouch, stats: maximum, skills: professional, gear: flotation vest, traits: webbed hands and feet", swim_result{251, 186}},
    {"move: crouch, stats: maximum, skills: professional, gear: none, traits: large paws", swim_result{30, 9000}},
    {"move: crouch, stats: maximum, skills: professional, gear: none, traits: none", swim_result{40, 9000}},
    {"move: crouch, stats: maximum, skills: professional, gear: none, traits: paws", swim_result{30, 9000}},
    {"move: crouch, stats: maximum, skills: professional, gear: none, traits: webbed hands", swim_result{30, 9000}},
    {"move: crouch, stats: maximum, skills: professional, gear: none, traits: webbed hands and feet", swim_result{30, 9000}},
    {"move: crouch, stats: minimum, skills: maximum, gear: fins, traits: large paws", swim_result{30, 9000}},
    {"move: crouch, stats: minimum, skills: maximum, gear: fins, traits: none", swim_result{30, 9000}},
    {"move: crouch, stats: minimum, skills: maximum, gear: fins, traits: paws", swim_result{30, 9000}},
    {"move: crouch, stats: minimum, skills: maximum, gear: fins, traits: webbed hands", swim_result{30, 9000}},
    {"move: crouch, stats: minimum, skills: maximum, gear: fins, traits: webbed hands and feet", swim_result{30, 9000}},
    {"move: crouch, stats: minimum, skills: maximum, gear: flotation vest, traits: large paws", swim_result{250, 382}},
    {"move: crouch, stats: minimum, skills: maximum, gear: flotation vest, traits: none", swim_result{250, 382}},
    {"move: crouch, stats: minimum, skills: maximum, gear: flotation vest, traits: paws", swim_result{250, 382}},
    {"move: crouch, stats: minimum, skills: maximum, gear: flotation vest, traits: webbed hands", swim_result{250, 382}},
    {"move: crouch, stats: minimum, skills: maximum, gear: flotation vest, traits: webbed hands and feet", swim_result{250, 382}},
    {"move: crouch, stats: minimum, skills: maximum, gear: none, traits: large paws", swim_result{30, 9000}},
    {"move: crouch, stats: minimum, skills: maximum, gear: none, traits: none", swim_result{30, 9000}},
    {"move: crouch, stats: minimum, skills: maximum, gear: none, traits: paws", swim_result{30, 9000}},
    {"move: crouch, stats: minimum, skills: maximum, gear: none, traits: webbed hands", swim_result{30, 9000}},
    {"move: crouch, stats: minimum, skills: maximum, gear: none, traits: webbed hands and feet", swim_result{30, 9000}},
    {"move: crouch, stats: minimum, skills: none, gear: fins, traits: large paws", swim_result{517, 44}},
    {"move: crouch, stats: minimum, skills: none, gear: fins, traits: none", swim_result{522, 44}},
    {"move: crouch, stats: minimum, skills: none, gear: fins, traits: paws", swim_result{507, 45}},
    {"move: crouch, stats: minimum, skills: none, gear: fins, traits: webbed hands", swim_result{484, 47}},
    {"move: crouch, stats: minimum, skills: none, gear: fins, traits: webbed hands and feet", swim_result{484, 47}},
    {"move: crouch, stats: minimum, skills: none, gear: flotation vest, traits: large paws", swim_result{450, 51}},
    {"move: crouch, stats: minimum, skills: none, gear: flotation vest, traits: none", swim_result{450, 51}},
    {"move: crouch, stats: minimum, skills: none, gear: flotation vest, traits: paws", swim_result{450, 51}},
    {"move: crouch, stats: minimum, skills: none, gear: flotation vest, traits: webbed hands", swim_result{450, 51}},
    {"move: crouch, stats: minimum, skills: none, gear: flotation vest, traits: webbed hands and feet", swim_result{450, 51}},
    {"move: crouch, stats: minimum, skills: none, gear: none, traits: large paws", swim_result{448, 52}},
    {"move: crouch, stats: minimum, skills: none, gear: none, traits: none", swim_result{450, 51}},
    {"move: crouch, stats: minimum, skills: none, gear: none, traits: paws", swim_result{437, 53}},
    {"move: crouch, stats: minimum, skills: none, gear: none, traits: webbed hands", swim_result{414, 56}},
    {"move: crouch, stats: minimum, skills: none, gear: none, traits: webbed hands and feet", swim_result{374, 63}},
    {"move: crouch, stats: minimum, skills: professional, gear: fins, traits: large paws", swim_result{180, 324}},
    {"move: crouch, stats: minimum, skills: professional, gear: fins, traits: none", swim_result{208, 250}},
    {"move: crouch, stats: minimum, skills: professional, gear: fins, traits: paws", swim_result{180, 324}},
    {"move: crouch, stats: minimum, skills: professional, gear: fins, traits: webbed hands", swim_result{170, 363}},
    {"move: crouch, stats: minimum, skills: professional, gear: fins, traits: webbed hands and feet", swim_result{170, 363}},
    {"move: crouch, stats: minimum, skills: professional, gear: flotation vest, traits: large paws", swim_result{450, 85}},
    {"move: crouch, stats: minimum, skills: professional, gear: flotation vest, traits: none", swim_result{450, 85}},
    {"move: crouch, stats: minimum, skills: professional, gear: flotation vest, traits: paws", swim_result{450, 85}},
    {"move: crouch, stats: minimum, skills: professional, gear: flotation vest, traits: webbed hands", swim_result{450, 85}},
    {"move: crouch, stats: minimum, skills: professional, gear: flotation vest, traits: webbed hands and feet", swim_result{450, 85}},
    {"move: crouch, stats: minimum, skills: professional, gear: none, traits: large paws", swim_result{177, 335}},
    {"move: crouch, stats: minimum, skills: professional, gear: none, traits: none", swim_result{200, 268}},
    {"move: crouch, stats: minimum, skills: professional, gear: none, traits: paws", swim_result{175, 343}},
    {"move: crouch, stats: minimum, skills: professional, gear: none, traits: webbed hands", swim_result{164, 391}},
    {"move: crouch, stats: minimum, skills: professional, gear: none, traits: webbed hands and feet", swim_result{124, 872}},
    {"move: prone, stats: average, skills: maximum, gear: fins, traits: large paws", swim_result{30, 9000}},
    {"move: prone, stats: average, skills: maximum, gear: fins, traits: none", swim_result{30, 9000}},
    {"move: prone, stats: average, skills: maximum, gear: fins, traits: paws", swim_result{30, 9000}},
    {"move: prone, stats: average, skills: maximum, gear: fins, traits: webbed hands", swim_result{30, 9000}},
    {"move: prone, stats: average, skills: maximum, gear: fins, traits: webbed hands and feet", swim_result{30, 9000}},
    {"move: prone, stats: average, skills: maximum, gear: flotation vest, traits: large paws", swim_result{250, 9000}},
    {"move: prone, stats: average, skills: maximum, gear: flotation vest, traits: none", swim_result{250, 9000}},
    {"move: prone, stats: average, skills: maximum, gear: flotation vest, traits: paws", swim_result{250, 9000}},
    {"move: prone, stats: average, skills: maximum, gear: flotation vest, traits: webbed hands", swim_result{250, 9000}},
    {"move: prone, stats: average, skills: maximum, gear: flotation vest, traits: webbed hands and feet", swim_result{250, 9000}},
    {"move: prone, stats: average, skills: maximum, gear: none, traits: large paws", swim_result{30, 9000}},
    {"move: prone, stats: average, skills: maximum, gear: none, traits: none", swim_result{30, 9000}},
    {"move: prone, stats: average, skills: maximum, gear: none, traits: paws", swim_result{30, 9000}},
    {"move: prone, stats: average, skills: maximum, gear: none, traits: webbed hands", swim_result{30, 9000}},
    {"move: prone, stats: average, skills: maximum, gear: none, traits: webbed hands and feet", swim_result{30, 9000}},
    {"move: prone, stats: average, skills: none, gear: fins, traits: large paws", swim_result{390, 197}},
    {"move: prone, stats: average, skills: none, gear: fins, traits: none", swim_result{419, 177}},
    {"move: prone, stats: average, skills: none, gear: fins, traits: paws", swim_result{387, 200}},
    {"move: prone, stats: average, skills: none, gear: fins, traits: webbed hands", swim_result{370, 214}},
    {"move: prone, stats: average, skills: none, gear: fins, traits: webbed hands and feet", swim_result{370, 214}},
    {"move: prone, stats: average, skills: none, gear: flotation vest, traits: large paws", swim_result{450, 160}},
    {"move: prone, stats: average, skills: none, gear: flotation vest, traits: none", swim_result{450, 160}},
    {"move: prone, stats: average, skills: none, gear: flotation vest, traits: paws", swim_result{450, 160}},
    {"move: prone, stats: average, skills: none, gear: flotation vest, traits: webbed hands", swim_result{450, 160}},
    {"move: prone, stats: average, skills: none, gear: flotation vest, traits: webbed hands and feet", swim_result{450, 160}},
    {"move: prone, stats: average, skills: none, gear: none, traits: large paws", swim_result{390, 197}},
    {"move: prone, stats: average, skills: none, gear: none, traits: none", swim_result{410, 183}},
    {"move: prone, stats: average, skills: none, gear: none, traits: paws", swim_result{384, 202}},
    {"move: prone, stats: average, skills: none, gear: none, traits: webbed hands", swim_result{364, 219}},
    {"move: prone, stats: average, skills: none, gear: none, traits: webbed hands and feet", swim_result{314, 281}},
    {"move: prone, stats: average, skills: professional, gear: fins, traits: large paws", swim_result{53, 9000}},
    {"move: prone, stats: average, skills: professional, gear: fins, traits: none", swim_result{105, 9000}},
    {"move: prone, stats: average, skills: professional, gear: fins, traits: paws", swim_result{60, 9000}},
    {"move: prone, stats: average, skills: professional, gear: fins, traits: webbed hands", swim_result{56, 9000}},
    {"move: prone, stats: average, skills: professional, gear: fins, traits: webbed hands and feet", swim_result{56, 9000}},
    {"move: prone, stats: average, skills: professional, gear: flotation vest, traits: large paws", swim_result{450, 324}},
    {"move: prone, stats: average, skills: professional, gear: flotation vest, traits: none", swim_result{450, 324}},
    {"move: prone, stats: average, skills: professional, gear: flotation vest, traits: paws", swim_result{450, 324}},
    {"move: prone, stats: average, skills: professional, gear: flotation vest, traits: webbed hands", swim_result{450, 324}},
    {"move: prone, stats: average, skills: professional, gear: flotation vest, traits: webbed hands and feet", swim_result{431, 353}},
    {"move: prone, stats: average, skills: professional, gear: none, traits: large paws", swim_result{119, 9000}},
    {"move: prone, stats: average, skills: professional, gear: none, traits: none", swim_result{160, 9000}},
    {"move: prone, stats: average, skills: professional, gear: none, traits: paws", swim_result{122, 9000}},
    {"move: prone, stats: average, skills: professional, gear: none, traits: webbed hands", swim_result{114, 9000}},
    {"move: prone, stats: average, skills: professional, gear: none, traits: webbed hands and feet", swim_result{64, 9000}},
    {"move: prone, stats: maximum, skills: maximum, gear: fins, traits: large paws", swim_result{30, 9000}},
    {"move: prone, stats: maximum, skills: maximum, gear: fins, traits: none", swim_result{30, 9000}},
    {"move: prone, stats: maximum, skills: maximum, gear: fins, traits: paws", swim_result{30, 9000}},
    {"move: prone, stats: maximum, skills: maximum, gear: fins, traits: webbed hands", swim_result{30, 9000}},
    {"move: prone, stats: maximum, skills: maximum, gear: fins, traits: webbed hands and feet", swim_result{30, 9000}},
    {"move: prone, stats: maximum, skills: maximum, gear: flotation vest, traits: large paws", swim_result{250, 9000}},
    {"move: prone, stats: maximum, skills: maximum, gear: flotation vest, traits: none", swim_result{250, 9000}},
    {"move: prone, stats: maximum, skills: maximum, gear: flotation vest, traits: paws", swim_result{250, 9000}},
    {"move: prone, stats: maximum, skills: maximum, gear: flotation vest, traits: webbed hands", swim_result{250, 9000}},
    {"move: prone, stats: maximum, skills: maximum, gear: flotation vest, traits: webbed hands and feet", swim_result{250, 9000}},
    {"move: prone, stats: maximum, skills: maximum, gear: none, traits: large paws", swim_result{30, 9000}},
    {"move: prone, stats: maximum, skills: maximum, gear: none, traits: none", swim_result{30, 9000}},
    {"move: prone, stats: maximum, skills: maximum, gear: none, traits: paws", swim_result{30, 9000}},
    {"move: prone, stats: maximum, skills: maximum, gear: none, traits: webbed hands", swim_result{30, 9000}},
    {"move: prone, stats: maximum, skills: maximum, gear: none, traits: webbed hands and feet", swim_result{30, 9000}},
    {"move: prone, stats: maximum, skills: none, gear: fins, traits: large paws", swim_result{30, 9000}},
    {"move: prone, stats: maximum, skills: none, gear: fins, traits: none", swim_result{109, 9000}},
    {"move: prone, stats: maximum, skills: none, gear: fins, traits: paws", swim_result{30, 9000}},
    {"move: prone, stats: maximum, skills: none, gear: fins, traits: webbed hands", swim_result{30, 9000}},
    {"move: prone, stats: maximum, skills: none, gear: fins, traits: webbed hands and feet", swim_result{30, 9000}},
    {"move: prone, stats: maximum, skills: none, gear: flotation vest, traits: large paws", swim_result{450, 160}},
    {"move: prone, stats: maximum, skills: none, gear: flotation vest, traits: none", swim_result{450, 160}},
    {"move: prone, stats: maximum, skills: none, gear: flotation vest, traits: paws", swim_result{450, 160}},
    {"move: prone, stats: maximum, skills: none, gear: flotation vest, traits: webbed hands", swim_result{450, 160}},
    {"move: prone, stats: maximum, skills: none, gear: flotation vest, traits: webbed hands and feet", swim_result{450, 160}},
    {"move: prone, stats: maximum, skills: none, gear: none, traits: large paws", swim_result{218, 629}},
    {"move: prone, stats: maximum, skills: none, gear: none, traits: none", swim_result{290, 325}},
    {"move: prone, stats: maximum, skills: none, gear: none, traits: paws", swim_result{226, 567}},
    {"move: prone, stats: maximum, skills: none, gear: none, traits: webbed hands", swim_result{214, 668}},
    {"move: prone, stats: maximum, skills: none, gear: none, traits: webbed hands and feet", swim_result{134, 9000}},
    {"move: prone, stats: maximum, skills: professional, gear: fins, traits: large paws", swim_result{30, 9000}},
    {"move: prone, stats: maximum, skills: professional, gear: fins, traits: none", swim_result{30, 9000}},
    {"move: prone, stats: maximum, skills: professional, gear: fins, traits: paws", swim_result{30, 9000}},
    {"move: prone, stats: maximum, skills: professional, gear: fins, traits: webbed hands", swim_result{30, 9000}},
    {"move: prone, stats: maximum, skills: professional, gear: fins, traits: webbed hands and feet", swim_result{30, 9000}},
    {"move: prone, stats: maximum, skills: professional, gear: flotation vest, traits: large paws", swim_result{315, 818}},
    {"move: prone, stats: maximum, skills: professional, gear: flotation vest, traits: none", swim_result{407, 398}},
    {"move: prone, stats: maximum, skills: professional, gear: flotation vest, traits: paws", swim_result{332, 676}},
    {"move: prone, stats: maximum, skills: professional, gear: flotation vest, traits: webbed hands", swim_result{331, 683}},
    {"move: prone, stats: maximum, skills: professional, gear: flotation vest, traits: webbed hands and feet", swim_result{251, 9000}},
    {"move: prone, stats: maximum, skills: professional, gear: none, traits: large paws", swim_result{30, 9000}},
    {"move: prone, stats: maximum, skills: professional, gear: none, traits: none", swim_result{40, 9000}},
    {"move: prone, stats: maximum, skills: professional, gear: none, traits: paws", swim_result{30, 9000}},
    {"move: prone, stats: maximum, skills: professional, gear: none, traits: webbed hands", swim_result{30, 9000}},
    {"move: prone, stats: maximum, skills: professional, gear: none, traits: webbed hands and feet", swim_result{30, 9000}},
    {"move: prone, stats: minimum, skills: maximum, gear: fins, traits: large paws", swim_result{30, 9000}},
    {"move: prone, stats: minimum, skills: maximum, gear: fins, traits: none", swim_result{30, 9000}},
    {"move: prone, stats: minimum, skills: maximum, gear: fins, traits: paws", swim_result{30, 9000}},
    {"move: prone, stats: minimum, skills: maximum, gear: fins, traits: webbed hands", swim_result{30, 9000}},
    {"move: prone, stats: minimum, skills: maximum, gear: fins, traits: webbed hands and feet", swim_result{30, 9000}},
    {"move: prone, stats: minimum, skills: maximum, gear: flotation vest, traits: large paws", swim_result{250, 9000}},
    {"move: prone, stats: minimum, skills: maximum, gear: flotation vest, traits: none", swim_result{250, 9000}},
    {"move: prone, stats: minimum, skills: maximum, gear: flotation vest, traits: paws", swim_result{250, 9000}},
    {"move: prone, stats: minimum, skills: maximum, gear: flotation vest, traits: webbed hands", swim_result{250, 9000}},
    {"move: prone, stats: minimum, skills: maximum, gear: flotation vest, traits: webbed hands and feet", swim_result{250, 9000}},
    {"move: prone, stats: minimum, skills: maximum, gear: none, traits: large paws", swim_result{30, 9000}},
    {"move: prone, stats: minimum, skills: maximum, gear: none, traits: none", swim_result{30, 9000}},
    {"move: prone, stats: minimum, skills: maximum, gear: none, traits: paws", swim_result{30, 9000}},
    {"move: prone, stats: minimum, skills: maximum, gear: none, traits: webbed hands", swim_result{30, 9000}},
    {"move: prone, stats: minimum, skills: maximum, gear: none, traits: webbed hands and feet", swim_result{30, 9000}},
    {"move: prone, stats: minimum, skills: none, gear: fins, traits: large paws", swim_result{517, 132}},
    {"move: prone, stats: minimum, skills: none, gear: fins, traits: none", swim_result{522, 130}},
    {"move: prone, stats: minimum, skills: none, gear: fins, traits: paws", swim_result{507, 136}},
    {"move: prone, stats: minimum, skills: none, gear: fins, traits: webbed hands", swim_result{484, 144}},
    {"move: prone, stats: minimum, skills: none, gear: fins, traits: webbed hands and feet", swim_result{484, 144}},
    {"move: prone, stats: minimum, skills: none, gear: flotation vest, traits: large paws", swim_result{450, 160}},
    {"move: prone, stats: minimum, skills: none, gear: flotation vest, traits: none", swim_result{450, 160}},
    {"move: prone, stats: minimum, skills: none, gear: flotation vest, traits: paws", swim_result{450, 160}},
    {"move: prone, stats: minimum, skills: none, gear: flotation vest, traits: webbed hands", swim_result{450, 160}},
    {"move: prone, stats: minimum, skills: none, gear: flotation vest, traits: webbed hands and feet", swim_result{450, 160}},
    {"move: prone, stats: minimum, skills: none, gear: none, traits: large paws", swim_result{448, 161}},
    {"move: prone, stats: minimum, skills: none, gear: none, traits: none", swim_result{450, 160}},
    {"move: prone, stats: minimum, skills: none, gear: none, traits: paws", swim_result{437, 167}},
    {"move: prone, stats: minimum, skills: none, gear: none, traits: webbed hands", swim_result{414, 180}},
    {"move: prone, stats: minimum, skills: none, gear: none, traits: webbed hands and feet", swim_result{374, 210}},
    {"move: prone, stats: minimum, skills: professional, gear: fins, traits: large paws", swim_result{180, 9000}},
    {"move: prone, stats: minimum, skills: professional, gear: fins, traits: none", swim_result{208, 9000}},
    {"move: prone, stats: minimum, skills: professional, gear: fins, traits: paws", swim_result{180, 9000}},
    {"move: prone, stats: minimum, skills: professional, gear: fins, traits: webbed hands", swim_result{170, 9000}},
    {"move: prone, stats: minimum, skills: professional, gear: fins, traits: webbed hands and feet", swim_result{170, 9000}},
    {"move: prone, stats: minimum, skills: professional, gear: flotation vest, traits: large paws", swim_result{450, 324}},
    {"move: prone, stats: minimum, skills: professional, gear: flotation vest, traits: none", swim_result{450, 324}},
    {"move: prone, stats: minimum, skills: professional, gear: flotation vest, traits: paws", swim_result{450, 324}},
    {"move: prone, stats: minimum, skills: professional, gear: flotation vest, traits: webbed hands", swim_result{450, 324}},
    {"move: prone, stats: minimum, skills: professional, gear: flotation vest, traits: webbed hands and feet", swim_result{450, 324}},
    {"move: prone, stats: minimum, skills: professional, gear: none, traits: large paws", swim_result{177, 9000}},
    {"move: prone, stats: minimum, skills: professional, gear: none, traits: none", swim_result{200, 9000}},
    {"move: prone, stats: minimum, skills: professional, gear: none, traits: paws", swim_result{175, 9000}},
    {"move: prone, stats: minimum, skills: professional, gear: none, traits: webbed hands", swim_result{164, 9000}},
    {"move: prone, stats: minimum, skills: professional, gear: none, traits: webbed hands and feet", swim_result{124, 9000}},
    {"move: run, stats: average, skills: maximum, gear: fins, traits: large paws", swim_result{30, 59}},
    {"move: run, stats: average, skills: maximum, gear: fins, traits: none", swim_result{30, 59}},
    {"move: run, stats: average, skills: maximum, gear: fins, traits: paws", swim_result{30, 59}},
    {"move: run, stats: average, skills: maximum, gear: fins, traits: webbed hands", swim_result{30, 59}},
    {"move: run, stats: average, skills: maximum, gear: fins, traits: webbed hands and feet", swim_result{30, 59}},
    {"move: run, stats: average, skills: maximum, gear: flotation vest, traits: large paws", swim_result{120, 15}},
    {"move: run, stats: average, skills: maximum, gear: flotation vest, traits: none", swim_result{120, 15}},
    {"move: run, stats: average, skills: maximum, gear: flotation vest, traits: paws", swim_result{120, 15}},
    {"move: run, stats: average, skills: maximum, gear: flotation vest, traits: webbed hands", swim_result{120, 15}},
    {"move: run, stats: average, skills: maximum, gear: flotation vest, traits: webbed hands and feet", swim_result{120, 15}},
    {"move: run, stats: average, skills: maximum, gear: none, traits: large paws", swim_result{30, 59}},
    {"move: run, stats: average, skills: maximum, gear: none, traits: none", swim_result{30, 59}},
    {"move: run, stats: average, skills: maximum, gear: none, traits: paws", swim_result{30, 59}},
    {"move: run, stats: average, skills: maximum, gear: none, traits: webbed hands", swim_result{30, 59}},
    {"move: run, stats: average, skills: maximum, gear: none, traits: webbed hands and feet", swim_result{30, 59}},
    {"move: run, stats: average, skills: none, gear: fins, traits: large paws", swim_result{260, 3}},
    {"move: run, stats: average, skills: none, gear: fins, traits: none", swim_result{289, 3}},
    {"move: run, stats: average, skills: none, gear: fins, traits: paws", swim_result{257, 3}},
    {"move: run, stats: average, skills: none, gear: fins, traits: webbed hands", swim_result{240, 3}},
    {"move: run, stats: average, skills: none, gear: fins, traits: webbed hands and feet", swim_result{240, 3}},
    {"move: run, stats: average, skills: none, gear: flotation vest, traits: large paws", swim_result{320, 2}},
    {"move: run, stats: average, skills: none, gear: flotation vest, traits: none", swim_result{320, 2}},
    {"move: run, stats: average, skills: none, gear: flotation vest, traits: paws", swim_result{320, 2}},
    {"move: run, stats: average, skills: none, gear: flotation vest, traits: webbed hands", swim_result{320, 2}},
    {"move: run, stats: average, skills: none, gear: flotation vest, traits: webbed hands and feet", swim_result{320, 2}},
    {"move: run, stats: average, skills: none, gear: none, traits: large paws", swim_result{260, 3}},
    {"move: run, stats: average, skills: none, gear: none, traits: none", swim_result{280, 3}},
    {"move: run, stats: average, skills: none, gear: none, traits: paws", swim_result{254, 3}},
    {"move: run, stats: average, skills: none, gear: none, traits: webbed hands", swim_result{234, 3}},
    {"move: run, stats: average, skills: none, gear: none, traits: webbed hands and feet", swim_result{184, 4}},
    {"move: run, stats: average, skills: professional, gear: fins, traits: large paws", swim_result{30, 39}},
    {"move: run, stats: average, skills: professional, gear: fins, traits: none", swim_result{30, 39}},
    {"move: run, stats: average, skills: professional, gear: fins, traits: paws", swim_result{30, 39}},
    {"move: run, stats: average, skills: professional, gear: fins, traits: webbed hands", swim_result{30, 39}},
    {"move: run, stats: average, skills: professional, gear: fins, traits: webbed hands and feet", swim_result{30, 39}},
    {"move: run, stats: average, skills: professional, gear: flotation vest, traits: large paws", swim_result{320, 4}},
    {"move: run, stats: average, skills: professional, gear: flotation vest, traits: none", swim_result{320, 4}},
    {"move: run, stats: average, skills: professional, gear: flotation vest, traits: paws", swim_result{320, 4}},
    {"move: run, stats: average, skills: professional, gear: flotation vest, traits: webbed hands", swim_result{320, 4}},
    {"move: run, stats: average, skills: professional, gear: flotation vest, traits: webbed hands and feet", swim_result{301, 4}},
    {"move: run, stats: average, skills: professional, gear: none, traits: large paws", swim_result{30, 39}},
    {"move: run, stats: average, skills: professional, gear: none, traits: none", swim_result{30, 39}},
    {"move: run, stats: average, skills: professional, gear: none, traits: paws", swim_result{30, 39}},
    {"move: run, stats: average, skills: professional, gear: none, traits: webbed hands", swim_result{30, 39}},
    {"move: run, stats: average, skills: professional, gear: none, traits: webbed hands and feet", swim_result{30, 39}},
    {"move: run, stats: maximum, skills: maximum, gear: fins, traits: large paws", swim_result{30, 59}},
    {"move: run, stats: maximum, skills: maximum, gear: fins, traits: none", swim_result{30, 59}},
    {"move: run, stats: maximum, skills: maximum, gear: fins, traits: paws", swim_result{30, 59}},
    {"move: run, stats: maximum, skills: maximum, gear: fins, traits: webbed hands", swim_result{30, 59}},
    {"move: run, stats: maximum, skills: maximum, gear: fins, traits: webbed hands and feet", swim_result{30, 59}},
    {"move: run, stats: maximum, skills: maximum, gear: flotation vest, traits: large paws", swim_result{120, 15}},
    {"move: run, stats: maximum, skills: maximum, gear: flotation vest, traits: none", swim_result{120, 15}},
    {"move: run, stats: maximum, skills: maximum, gear: flotation vest, traits: paws", swim_result{120, 15}},
    {"move: run, stats: maximum, skills: maximum, gear: flotation vest, traits: webbed hands", swim_result{120, 15}},
    {"move: run, stats: maximum, skills: maximum, gear: flotation vest, traits: webbed hands and feet", swim_result{120, 15}},
    {"move: run, stats: maximum, skills: maximum, gear: none, traits: large paws", swim_result{30, 59}},
    {"move: run, stats: maximum, skills: maximum, gear: none, traits: none", swim_result{30, 59}},
    {"move: run, stats: maximum, skills: maximum, gear: none, traits: paws", swim_result{30, 59}},
    {"move: run, stats: maximum, skills: maximum, gear: none, traits: webbed hands", swim_result{30, 59}},
    {"move: run, stats: maximum, skills: maximum, gear: none, traits: webbed hands and feet", swim_result{30, 59}},
    {"move: run, stats: maximum, skills: none, gear: fins, traits: large paws", swim_result{30, 25}},
    {"move: run, stats: maximum, skills: none, gear: fins, traits: none", swim_result{30, 25}},
    {"move: run, stats: maximum, skills: none, gear: fins, traits: paws", swim_result{30, 25}},
    {"move: run, stats: maximum, skills: none, gear: fins, traits: webbed hands", swim_result{30, 25}},
    {"move: run, stats: maximum, skills: none, gear: fins, traits: webbed hands and feet", swim_result{30, 25}},
    {"move: run, stats: maximum, skills: none, gear: flotation vest, traits: large paws", swim_result{320, 2}},
    {"move: run, stats: maximum, skills: none, gear: flotation vest, traits: none", swim_result{320, 2}},
    {"move: run, stats: maximum, skills: none, gear: flotation vest, traits: paws", swim_result{320, 2}},
    {"move: run, stats: maximum, skills: none, gear: flotation vest, traits: webbed hands", swim_result{320, 2}},
    {"move: run, stats: maximum, skills: none, gear: flotation vest, traits: webbed hands and feet", swim_result{320, 2}},
    {"move: run, stats: maximum, skills: none, gear: none, traits: large paws", swim_result{88, 9}},
    {"move: run, stats: maximum, skills: none, gear: none, traits: none", swim_result{160, 5}},
    {"move: run, stats: maximum, skills: none, gear: none, traits: paws", swim_result{96, 8}},
    {"move: run, stats: maximum, skills: none, gear: none, traits: webbed hands", swim_result{84, 9}},
    {"move: run, stats: maximum, skills: none, gear: none, traits: webbed hands and feet", swim_result{30, 25}},
    {"move: run, stats: maximum, skills: professional, gear: fins, traits: large paws", swim_result{30, 39}},
    {"move: run, stats: maximum, skills: professional, gear: fins, traits: none", swim_result{30, 39}},
    {"move: run, stats: maximum, skills: professional, gear: fins, traits: paws", swim_result{30, 39}},
    {"move: run, stats: maximum, skills: professional, gear: fins, traits: webbed hands", swim_result{30, 39}},
    {"move: run, stats: maximum, skills: professional, gear: fins, traits: webbed hands and feet", swim_result{30, 39}},
    {"move: run, stats: maximum, skills: professional, gear: flotation vest, traits: large paws", swim_result{185, 6}},
    {"move: run, stats: maximum, skills: professional, gear: flotation vest, traits: none", swim_result{277, 4}},
    {"move: run, stats: maximum, skills: professional, gear: flotation vest, traits: paws", swim_result{202, 6}},
    {"move: run, stats: maximum, skills: professional, gear: flotation vest, traits: webbed hands", swim_result{201, 6}},
    {"move: run, stats: maximum, skills: professional, gear: flotation vest, traits: webbed hands and feet", swim_result{121, 10}},
    {"move: run, stats: maximum, skills: professional, gear: none, traits: large paws", swim_result{30, 39}},
    {"move: run, stats: maximum, skills: professional, gear: none, traits: none", swim_result{30, 39}},
    {"move: run, stats: maximum, skills: professional, gear: none, traits: paws", swim_result{30, 39}},
    {"move: run, stats: maximum, skills: professional, gear: none, traits: webbed hands", swim_result{30, 39}},
    {"move: run, stats: maximum, skills: professional, gear: none, traits: webbed hands and feet", swim_result{30, 39}},
    {"move: run, stats: minimum, skills: maximum, gear: fins, traits: large paws", swim_result{30, 59}},
    {"move: run, stats: minimum, skills: maximum, gear: fins, traits: none", swim_result{30, 59}},
    {"move: run, stats: minimum, skills: maximum, gear: fins, traits: paws", swim_result{30, 59}},
    {"move: run, stats: minimum, skills: maximum, gear: fins, traits: webbed hands", swim_result{30, 59}},
    {"move: run, stats: minimum, skills: maximum, gear: fins, traits: webbed hands and feet", swim_result{30, 59}},
    {"move: run, stats: minimum, skills: maximum, gear: flotation vest, traits: large paws", swim_result{120, 15}},
    {"move: run, stats: minimum, skills: maximum, gear: flotation vest, traits: none", swim_result{120, 15}},
    {"move: run, stats: minimum, skills: maximum, gear: flotation vest, traits: paws", swim_result{120, 15}},
    {"move: run, stats: minimum, skills: maximum, gear: flotation vest, traits: webbed hands", swim_result{120, 15}},
    {"move: run, stats: minimum, skills: maximum, gear: flotation vest, traits: webbed hands and feet", swim_result{120, 15}},
    {"move: run, stats: minimum, skills: maximum, gear: none, traits: large paws", swim_result{30, 59}},
    {"move: run, stats: minimum, skills: maximum, gear: none, traits: none", swim_result{30, 59}},
    {"move: run, stats: minimum, skills: maximum, gear: none, traits: paws", swim_result{30, 59}},
    {"move: run, stats: minimum, skills: maximum, gear: none, traits: webbed hands", swim_result{30, 59}},
    {"move: run, stats: minimum, skills: maximum, gear: none, traits: webbed hands and feet", swim_result{30, 59}},
    {"move: run, stats: minimum, skills: none, gear: fins, traits: large paws", swim_result{387, 2}},
    {"move: run, stats: minimum, skills: none, gear: fins, traits: none", swim_result{392, 2}},
    {"move: run, stats: minimum, skills: none, gear: fins, traits: paws", swim_result{377, 2}},
    {"move: run, stats: minimum, skills: none, gear: fins, traits: webbed hands", swim_result{354, 2}},
    {"move: run, stats: minimum, skills: none, gear: fins, traits: webbed hands and feet", swim_result{354, 2}},
    {"move: run, stats: minimum, skills: none, gear: flotation vest, traits: large paws", swim_result{320, 2}},
    {"move: run, stats: minimum, skills: none, gear: flotation vest, traits: none", swim_result{320, 2}},
    {"move: run, stats: minimum, skills: none, gear: flotation vest, traits: paws", swim_result{320, 2}},
    {"move: run, stats: minimum, skills: none, gear: flotation vest, traits: webbed hands", swim_result{320, 2}},
    {"move: run, stats: minimum, skills: none, gear: flotation vest, traits: webbed hands and feet", swim_result{320, 2}},
    {"move: run, stats: minimum, skills: none, gear: none, traits: large paws", swim_result{318, 2}},
    {"move: run, stats: minimum, skills: none, gear: none, traits: none", swim_result{320, 2}},
    {"move: run, stats: minimum, skills: none, gear: none, traits: paws", swim_result{307, 3}},
    {"move: run, stats: minimum, skills: none, gear: none, traits: webbed hands", swim_result{284, 3}},
    {"move: run, stats: minimum, skills: none, gear: none, traits: webbed hands and feet", swim_result{244, 3}},
    {"move: run, stats: minimum, skills: professional, gear: fins, traits: large paws", swim_result{50, 23}},
    {"move: run, stats: minimum, skills: professional, gear: fins, traits: none", swim_result{78, 15}},
    {"move: run, stats: minimum, skills: professional, gear: fins, traits: paws", swim_result{50, 23}},
    {"move: run, stats: minimum, skills: professional, gear: fins, traits: webbed hands", swim_result{40, 29}},
    {"move: run, stats: minimum, skills: professional, gear: fins, traits: webbed hands and feet", swim_result{40, 29}},
    {"move: run, stats: minimum, skills: professional, gear: flotation vest, traits: large paws", swim_result{320, 4}},
    {"move: run, stats: minimum, skills: professional, gear: flotation vest, traits: none", swim_result{320, 4}},
    {"move: run, stats: minimum, skills: professional, gear: flotation vest, traits: paws", swim_result{320, 4}},
    {"move: run, stats: minimum, skills: professional, gear: flotation vest, traits: webbed hands", swim_result{320, 4}},
    {"move: run, stats: minimum, skills: professional, gear: flotation vest, traits: webbed hands and feet", swim_result{320, 4}},
    {"move: run, stats: minimum, skills: professional, gear: none, traits: large paws", swim_result{47, 25}},
    {"move: run, stats: minimum, skills: professional, gear: none, traits: none", swim_result{70, 17}},
    {"move: run, stats: minimum, skills: professional, gear: none, traits: paws", swim_result{45, 26}},
    {"move: run, stats: minimum, skills: professional, gear: none, traits: webbed hands", swim_result{34, 34}},
    {"move: run, stats: minimum, skills: professional, gear: none, traits: webbed hands and feet", swim_result{30, 39}},
    {"move: walk, stats: average, skills: maximum, gear: fins, traits: large paws", swim_result{30, 3140}},
    {"move: walk, stats: average, skills: maximum, gear: fins, traits: none", swim_result{30, 3140}},
    {"move: walk, stats: average, skills: maximum, gear: fins, traits: paws", swim_result{30, 3140}},
    {"move: walk, stats: average, skills: maximum, gear: fins, traits: webbed hands", swim_result{30, 3140}},
    {"move: walk, stats: average, skills: maximum, gear: fins, traits: webbed hands and feet", swim_result{30, 3140}},
    {"move: walk, stats: average, skills: maximum, gear: flotation vest, traits: large paws", swim_result{200, 170}},
    {"move: walk, stats: average, skills: maximum, gear: flotation vest, traits: none", swim_result{200, 170}},
    {"move: walk, stats: average, skills: maximum, gear: flotation vest, traits: paws", swim_result{200, 170}},
    {"move: walk, stats: average, skills: maximum, gear: flotation vest, traits: webbed hands", swim_result{200, 170}},
    {"move: walk, stats: average, skills: maximum, gear: flotation vest, traits: webbed hands and feet", swim_result{200, 170}},
    {"move: walk, stats: average, skills: maximum, gear: none, traits: large paws", swim_result{30, 2624}},
    {"move: walk, stats: average, skills: maximum, gear: none, traits: none", swim_result{30, 2624}},
    {"move: walk, stats: average, skills: maximum, gear: none, traits: paws", swim_result{30, 2624}},
    {"move: walk, stats: average, skills: maximum, gear: none, traits: webbed hands", swim_result{30, 2624}},
    {"move: walk, stats: average, skills: maximum, gear: none, traits: webbed hands and feet", swim_result{30, 2624}},
    {"move: walk, stats: average, skills: none, gear: fins, traits: large paws", swim_result{340, 33}},
    {"move: walk, stats: average, skills: none, gear: fins, traits: none", swim_result{369, 30}},
    {"move: walk, stats: average, skills: none, gear: fins, traits: paws", swim_result{337, 33}},
    {"move: walk, stats: average, skills: none, gear: fins, traits: webbed hands", swim_result{320, 35}},
    {"move: walk, stats: average, skills: none, gear: fins, traits: webbed hands and feet", swim_result{320, 35}},
    {"move: walk, stats: average, skills: none, gear: flotation vest, traits: large paws", swim_result{400, 27}},
    {"move: walk, stats: average, skills: none, gear: flotation vest, traits: none", swim_result{400, 27}},
    {"move: walk, stats: average, skills: none, gear: flotation vest, traits: paws", swim_result{400, 27}},
    {"move: walk, stats: average, skills: none, gear: flotation vest, traits: webbed hands", swim_result{400, 27}},
    {"move: walk, stats: average, skills: none, gear: flotation vest, traits: webbed hands and feet", swim_result{400, 27}},
    {"move: walk, stats: average, skills: none, gear: none, traits: large paws", swim_result{340, 33}},
    {"move: walk, stats: average, skills: none, gear: none, traits: none", swim_result{360, 31}},
    {"move: walk, stats: average, skills: none, gear: none, traits: paws", swim_result{334, 33}},
    {"move: walk, stats: average, skills: none, gear: none, traits: webbed hands", swim_result{314, 36}},
    {"move: walk, stats: average, skills: none, gear: none, traits: webbed hands and feet", swim_result{264, 43}},
    {"move: walk, stats: average, skills: professional, gear: fins, traits: large paws", swim_result{30, 964}},
    {"move: walk, stats: average, skills: professional, gear: fins, traits: none", swim_result{55, 510}},
    {"move: walk, stats: average, skills: professional, gear: fins, traits: paws", swim_result{30, 964}},
    {"move: walk, stats: average, skills: professional, gear: fins, traits: webbed hands", swim_result{30, 964}},
    {"move: walk, stats: average, skills: professional, gear: fins, traits: webbed hands and feet", swim_result{30, 964}},
    {"move: walk, stats: average, skills: professional, gear: flotation vest, traits: large paws", swim_result{400, 44}},
    {"move: walk, stats: average, skills: professional, gear: flotation vest, traits: none", swim_result{400, 44}},
    {"move: walk, stats: average, skills: professional, gear: flotation vest, traits: paws", swim_result{400, 44}},
    {"move: walk, stats: average, skills: professional, gear: flotation vest, traits: webbed hands", swim_result{400, 44}},
    {"move: walk, stats: average, skills: professional, gear: flotation vest, traits: webbed hands and feet", swim_result{381, 46}},
    {"move: walk, stats: average, skills: professional, gear: none, traits: large paws", swim_result{69, 392}},
    {"move: walk, stats: average, skills: professional, gear: none, traits: none", swim_result{110, 228}},
    {"move: walk, stats: average, skills: professional, gear: none, traits: paws", swim_result{72, 374}},
    {"move: walk, stats: average, skills: professional, gear: none, traits: webbed hands", swim_result{64, 423}},
    {"move: walk, stats: average, skills: professional, gear: none, traits: webbed hands and feet", swim_result{30, 934}},
    {"move: walk, stats: maximum, skills: maximum, gear: fins, traits: large paws", swim_result{30, 3140}},
    {"move: walk, stats: maximum, skills: maximum, gear: fins, traits: none", swim_result{30, 3140}},
    {"move: walk, stats: maximum, skills: maximum, gear: fins, traits: paws", swim_result{30, 3140}},
    {"move: walk, stats: maximum, skills: maximum, gear: fins, traits: webbed hands", swim_result{30, 3140}},
    {"move: walk, stats: maximum, skills: maximum, gear: fins, traits: webbed hands and feet", swim_result{30, 3140}},
    {"move: walk, stats: maximum, skills: maximum, gear: flotation vest, traits: large paws", swim_result{200, 170}},
    {"move: walk, stats: maximum, skills: maximum, gear: flotation vest, traits: none", swim_result{200, 170}},
    {"move: walk, stats: maximum, skills: maximum, gear: flotation vest, traits: paws", swim_result{200, 170}},
    {"move: walk, stats: maximum, skills: maximum, gear: flotation vest, traits: webbed hands", swim_result{200, 170}},
    {"move: walk, stats: maximum, skills: maximum, gear: flotation vest, traits: webbed hands and feet", swim_result{200, 170}},
    {"move: walk, stats: maximum, skills: maximum, gear: none, traits: large paws", swim_result{30, 2624}},
    {"move: walk, stats: maximum, skills: maximum, gear: none, traits: none", swim_result{30, 2624}},
    {"move: walk, stats: maximum, skills: maximum, gear: none, traits: paws", swim_result{30, 2624}},
    {"move: walk, stats: maximum, skills: maximum, gear: none, traits: webbed hands", swim_result{30, 2624}},
    {"move: walk, stats: maximum, skills: maximum, gear: none, traits: webbed hands and feet", swim_result{30, 2624}},
    {"move: walk, stats: maximum, skills: none, gear: fins, traits: large paws", swim_result{30, 479}},
    {"move: walk, stats: maximum, skills: none, gear: fins, traits: none", swim_result{59, 239}},
    {"move: walk, stats: maximum, skills: none, gear: fins, traits: paws", swim_result{30, 479}},
    {"move: walk, stats: maximum, skills: none, gear: fins, traits: webbed hands", swim_result{30, 479}},
    {"move: walk, stats: maximum, skills: none, gear: fins, traits: webbed hands and feet", swim_result{30, 479}},
    {"move: walk, stats: maximum, skills: none, gear: flotation vest, traits: large paws", swim_result{400, 27}},
    {"move: walk, stats: maximum, skills: none, gear: flotation vest, traits: none", swim_result{400, 27}},
    {"move: walk, stats: maximum, skills: none, gear: flotation vest, traits: paws", swim_result{400, 27}},
    {"move: walk, stats: maximum, skills: none, gear: flotation vest, traits: webbed hands", swim_result{400, 27}},
    {"move: walk, stats: maximum, skills: none, gear: flotation vest, traits: webbed hands and feet", swim_result{400, 27}},
    {"move: walk, stats: maximum, skills: none, gear: none, traits: large paws", swim_result{168, 72}},
    {"move: walk, stats: maximum, skills: none, gear: none, traits: none", swim_result{240, 48}},
    {"move: walk, stats: maximum, skills: none, gear: none, traits: paws", swim_result{176, 68}},
    {"move: walk, stats: maximum, skills: none, gear: none, traits: webbed hands", swim_result{164, 74}},
    {"move: walk, stats: maximum, skills: none, gear: none, traits: webbed hands and feet", swim_result{84, 164}},
    {"move: walk, stats: maximum, skills: professional, gear: fins, traits: large paws", swim_result{30, 964}},
    {"move: walk, stats: maximum, skills: professional, gear: fins, traits: none", swim_result{30, 964}},
    {"move: walk, stats: maximum, skills: professional, gear: fins, traits: paws", swim_result{30, 964}},
    {"move: walk, stats: maximum, skills: professional, gear: fins, traits: webbed hands", swim_result{30, 964}},
    {"move: walk, stats: maximum, skills: professional, gear: fins, traits: webbed hands and feet", swim_result{30, 964}},
    {"move: walk, stats: maximum, skills: professional, gear: flotation vest, traits: large paws", swim_result{265, 70}},
    {"move: walk, stats: maximum, skills: professional, gear: flotation vest, traits: none", swim_result{357, 50}},
    {"move: walk, stats: maximum, skills: professional, gear: flotation vest, traits: paws", swim_result{282, 65}},
    {"move: walk, stats: maximum, skills: professional, gear: flotation vest, traits: webbed hands", swim_result{281, 65}},
    {"move: walk, stats: maximum, skills: professional, gear: flotation vest, traits: webbed hands and feet", swim_result{201, 98}},
    {"move: walk, stats: maximum, skills: professional, gear: none, traits: large paws", swim_result{30, 934}},
    {"move: walk, stats: maximum, skills: professional, gear: none, traits: none", swim_result{30, 934}},
    {"move: walk, stats: maximum, skills: professional, gear: none, traits: paws", swim_result{30, 934}},
    {"move: walk, stats: maximum, skills: professional, gear: none, traits: webbed hands", swim_result{30, 934}},
    {"move: walk, stats: maximum, skills: professional, gear: none, traits: webbed hands and feet", swim_result{30, 934}},
    {"move: walk, stats: minimum, skills: maximum, gear: fins, traits: large paws", swim_result{30, 3140}},
    {"move: walk, stats: minimum, skills: maximum, gear: fins, traits: none", swim_result{30, 3140}},
    {"move: walk, stats: minimum, skills: maximum, gear: fins, traits: paws", swim_result{30, 3140}},
    {"move: walk, stats: minimum, skills: maximum, gear: fins, traits: webbed hands", swim_result{30, 3140}},
    {"move: walk, stats: minimum, skills: maximum, gear: fins, traits: webbed hands and feet", swim_result{30, 3140}},
    {"move: walk, stats: minimum, skills: maximum, gear: flotation vest, traits: large paws", swim_result{200, 170}},
    {"move: walk, stats: minimum, skills: maximum, gear: flotation vest, traits: none", swim_result{200, 170}},
    {"move: walk, stats: minimum, skills: maximum, gear: flotation vest, traits: paws", swim_result{200, 170}},
    {"move: walk, stats: minimum, skills: maximum, gear: flotation vest, traits: webbed hands", swim_result{200, 170}},
    {"move: walk, stats: minimum, skills: maximum, gear: flotation vest, traits: webbed hands and feet", swim_result{200, 170}},
    {"move: walk, stats: minimum, skills: maximum, gear: none, traits: large paws", swim_result{30, 2624}},
    {"move: walk, stats: minimum, skills: maximum, gear: none, traits: none", swim_result{30, 2624}},
    {"move: walk, stats: minimum, skills: maximum, gear: none, traits: paws", swim_result{30, 2624}},
    {"move: walk, stats: minimum, skills: maximum, gear: none, traits: webbed hands", swim_result{30, 2624}},
    {"move: walk, stats: minimum, skills: maximum, gear: none, traits: webbed hands and feet", swim_result{30, 2624}},
    {"move: walk, stats: minimum, skills: none, gear: fins, traits: large paws", swim_result{467, 23}},
    {"move: walk, stats: minimum, skills: none, gear: fins, traits: none", swim_result{472, 23}},
    {"move: walk, stats: minimum, skills: none, gear: fins, traits: paws", swim_result{457, 24}},
    {"move: walk, stats: minimum, skills: none, gear: fins, traits: webbed hands", swim_result{434, 25}},
    {"move: walk, stats: minimum, skills: none, gear: fins, traits: webbed hands and feet", swim_result{434, 25}},
    {"move: walk, stats: minimum, skills: none, gear: flotation vest, traits: large paws", swim_result{400, 27}},
    {"move: walk, stats: minimum, skills: none, gear: flotation vest, traits: none", swim_result{400, 27}},
    {"move: walk, stats: minimum, skills: none, gear: flotation vest, traits: paws", swim_result{400, 27}},
    {"move: walk, stats: minimum, skills: none, gear: flotation vest, traits: webbed hands", swim_result{400, 27}},
    {"move: walk, stats: minimum, skills: none, gear: flotation vest, traits: webbed hands and feet", swim_result{400, 27}},
    {"move: walk, stats: minimum, skills: none, gear: none, traits: large paws", swim_result{398, 28}},
    {"move: walk, stats: minimum, skills: none, gear: none, traits: none", swim_result{400, 27}},
    {"move: walk, stats: minimum, skills: none, gear: none, traits: paws", swim_result{387, 28}},
    {"move: walk, stats: minimum, skills: none, gear: none, traits: webbed hands", swim_result{364, 30}},
    {"move: walk, stats: minimum, skills: none, gear: none, traits: webbed hands and feet", swim_result{324, 34}},
    {"move: walk, stats: minimum, skills: professional, gear: fins, traits: large paws", swim_result{130, 176}},
    {"move: walk, stats: minimum, skills: professional, gear: fins, traits: none", swim_result{158, 134}},
    {"move: walk, stats: minimum, skills: professional, gear: fins, traits: paws", swim_result{130, 176}},
    {"move: walk, stats: minimum, skills: professional, gear: fins, traits: webbed hands", swim_result{120, 199}},
    {"move: walk, stats: minimum, skills: professional, gear: fins, traits: webbed hands and feet", swim_result{120, 199}},
    {"move: walk, stats: minimum, skills: professional, gear: flotation vest, traits: large paws", swim_result{400, 44}},
    {"move: walk, stats: minimum, skills: professional, gear: flotation vest, traits: none", swim_result{400, 44}},
    {"move: walk, stats: minimum, skills: professional, gear: flotation vest, traits: paws", swim_result{400, 44}},
    {"move: walk, stats: minimum, skills: professional, gear: flotation vest, traits: webbed hands", swim_result{400, 44}},
    {"move: walk, stats: minimum, skills: professional, gear: flotation vest, traits: webbed hands and feet", swim_result{400, 44}},
    {"move: walk, stats: minimum, skills: professional, gear: none, traits: large paws", swim_result{127, 183}},
    {"move: walk, stats: minimum, skills: professional, gear: none, traits: none", swim_result{150, 144}},
    {"move: walk, stats: minimum, skills: professional, gear: none, traits: paws", swim_result{125, 187}},
    {"move: walk, stats: minimum, skills: professional, gear: none, traits: webbed hands", swim_result{114, 215}},
    {"move: walk, stats: minimum, skills: professional, gear: none, traits: webbed hands and feet", swim_result{74, 364}},
};

TEST_CASE( "check_swim_move_cost_and_distance_values", "[swimming][slow]" )
{
    setup_test_lake();

    avatar &dummy = get_avatar();

    for( const swim_scenario &scenario : generate_scenarios() ) {
        GIVEN( "swimmer with " + scenario.name() ) {
            const swim_result result = swim( dummy, scenario.move_mode, scenario.config );
            const swim_result expected = expected_results[scenario.name()];
            CHECK( result.move_cost == Approx( expected.move_cost ).margin( 0 ) );
            CHECK( result.steps == Approx( expected.steps ).margin( 5 ) );
        }
    }
}

// This "test" is used to generate the expected_results map above.
TEST_CASE( "generate_swim_move_cost_and_distance_values", "[.]" )
{
    setup_test_lake();

    avatar &dummy = get_avatar();

    std::map<std::string, swim_result> results;

    for( const swim_scenario &scenario : generate_scenarios() ) {
        const swim_result result = swim( dummy, scenario.move_mode, scenario.config );
        results[scenario.name()] = result;
    }

    for( std::pair<const std::string, swim_result> &x : results ) {
        printf( "{\"%s\", swim_result{%i, %i}},\n", x.first.c_str(), x.second.move_cost, x.second.steps );
    }
}

TEST_CASE( "export_scenario_swim_move_cost_and_distance_values", "[.]" )
{
    setup_test_lake();

    avatar &dummy = get_avatar();

    std::ofstream testfile;
    testfile.open( fs::u8path( "swim-scenarios.csv" ), std::ofstream::trunc );
    testfile << "scenario, move cost, steps" << std::endl;

    for( const swim_scenario &scenario : generate_scenarios() ) {
        const swim_result result = swim( dummy, scenario.move_mode, scenario.config );
        testfile << "\"" << scenario.name() << "\"" << ", " << result.move_cost << ", " << result.steps <<
                 std::endl;
    }

    testfile.close();
}

// This "test" reports the swim cost and distance for all professions,
// assuming average stats, no traits, and whatever athletics skill and
// gear is provided by the profession itself. We don't really need to
// assert anything here (yet) but for informational purposes it is
// interesting to see the data as a slightly better "gut-check". Our
// "professional swimmer" doesn't swim very well.
TEST_CASE( "export_profession_swim_cost_and_distance", "[.]" )
{
    setup_test_lake();

    avatar &dummy = get_avatar();

    std::ofstream testfile;
    testfile.open( fs::u8path( "swim-profession.csv" ), std::ofstream::trunc );
    testfile << "profession, move cost, steps" << std::endl;

    const std::vector<profession> &all = profession::get_all();
    for( const profession &prof : all ) {
        std::vector<trait_id> traits;
        std::vector<std::string> trait_ids;
        for( const trait_and_var &trait : prof.get_locked_traits() ) {
            traits.push_back( trait.trait );
            trait_ids.push_back( trait.trait.str() );
        }
        int max_athletics = 0;
        for( const profession::StartingSkill &skill : prof.skills() ) {
            if( skill.first == skill_swimming && skill.second > max_athletics ) {
                max_athletics = skill.second;
            }
        }

        swimmer_config config( stats_map.at( "average" ), swimmer_skills{ max_athletics },
                               gear_map.at( "none" ), swimmer_traits{ trait_ids } );
        config.prof = &prof;
        const swim_result result = swim( dummy, move_mode_walk, config );
        testfile << prof.ident().str() << ", " << result.move_cost << ", " << result.steps << std::endl;
    }

    testfile.close();
}

// This "test" exports swim move cost and distance data to csv for analysis and review.
TEST_CASE( "export_swim_move_cost_and_distance_data", "[.]" )
{
    setup_test_lake();

    avatar &dummy = get_avatar();

    std::ofstream testfile;
    testfile.open( fs::u8path( "swim-skill.csv" ), std::ofstream::trunc );
    testfile << "athletics, move cost, steps" << std::endl;
    for( int i = 0; i <= 10; i++ ) {
        swimmer_config config( stats_map.at( "average" ), swimmer_skills{i}, gear_map.at( "none" ),
                               traits_map.at( "none" ) );
        const swim_result result = swim( dummy, move_mode_walk, config );
        testfile << i << ", " << result.move_cost << ", " << result.steps << std::endl;
    }
    testfile.close();

    testfile.open( fs::u8path( "swim-strength.csv" ), std::ofstream::trunc );
    testfile << "strength, move cost, steps" << std::endl;
    for( int i = 4; i <= 20; i++ ) {
        swimmer_config config( {i, 8}, skills_map.at( "none" ), gear_map.at( "none" ),
                               traits_map.at( "none" ) );
        const swim_result result = swim( dummy, move_mode_walk, config );
        testfile << i << ", " << result.move_cost << ", " << result.steps << std::endl;
    }
    testfile.close();

    testfile.open( fs::u8path( "swim-dexterity.csv" ), std::ofstream::trunc );
    testfile << "dexterity, move cost, steps" << std::endl;
    for( int i = 4; i <= 20; i++ ) {
        swimmer_config config( { 8, i }, skills_map.at( "none" ), gear_map.at( "none" ),
                               traits_map.at( "none" ) );
        const swim_result result = swim( dummy, move_mode_walk, config );
        testfile << i << ", " << result.move_cost << ", " << result.steps << std::endl;
    }
    testfile.close();
}
