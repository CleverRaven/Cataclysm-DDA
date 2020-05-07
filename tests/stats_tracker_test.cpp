#include <memory>

#include "achievement.h"
#include "avatar.h"
#include "cata_variant.h"
#include "catch/catch.hpp"
#include "character_id.h"
#include "event.h"
#include "event_bus.h"
#include "event_statistics.h"
#include "game.h"
#include "stats_tracker.h"
#include "string_id.h"
#include "stringmaker.h"
#include "type_id.h"
#include "options_helpers.h"

TEST_CASE( "stats_tracker_count_events", "[stats]" )
{
    stats_tracker s;
    event_bus b;
    b.subscribe( &s );

    const character_id u_id = g->u.getID();
    const mtype_id mon1( "mon_zombie" );
    const mtype_id mon2( "mon_zombie_brute" );
    const cata::event kill1 = cata::event::make<event_type::character_kills_monster>( u_id, mon1 );
    const cata::event kill2 = cata::event::make<event_type::character_kills_monster>( u_id, mon2 );
    const cata::event::data_type char_is_player{ { "killer", cata_variant( u_id ) } };

    CHECK( s.get_events( kill1.type() ).count( kill1.data() ) == 0 );
    CHECK( s.get_events( kill2.type() ).count( kill2.data() ) == 0 );
    CHECK( s.get_events( event_type::character_kills_monster ).count( char_is_player ) == 0 );
    b.send( kill1 );
    CHECK( s.get_events( kill1.type() ).count( kill1.data() ) == 1 );
    CHECK( s.get_events( kill2.type() ).count( kill2.data() ) == 0 );
    CHECK( s.get_events( event_type::character_kills_monster ).count( char_is_player ) == 1 );
    b.send( kill2 );
    CHECK( s.get_events( kill1.type() ).count( kill1.data() ) == 1 );
    CHECK( s.get_events( kill2.type() ).count( kill2.data() ) == 1 );
    CHECK( s.get_events( event_type::character_kills_monster ).count( char_is_player ) == 2 );
}

TEST_CASE( "stats_tracker_total_events", "[stats]" )
{
    stats_tracker s;
    event_bus b;
    b.subscribe( &s );

    const character_id u_id = g->u.getID();
    character_id other_id = u_id;
    ++other_id;
    const cata::event::data_type damage_to_any{};
    const cata::event::data_type damage_to_u{ { "character", cata_variant( u_id ) } };
    constexpr event_type ctd = event_type::character_takes_damage;

    CHECK( s.get_events( ctd ).total( "damage", damage_to_u ) == 0 );
    CHECK( s.get_events( ctd ).total( "damage", damage_to_any ) == 0 );
    b.send<ctd>( u_id, 10 );
    CHECK( s.get_events( ctd ).total( "damage", damage_to_u ) == 10 );
    CHECK( s.get_events( ctd ).total( "damage", damage_to_any ) == 10 );
    b.send<ctd>( other_id, 10 );
    CHECK( s.get_events( ctd ).total( "damage", damage_to_u ) == 10 );
    CHECK( s.get_events( ctd ).total( "damage", damage_to_any ) == 20 );
    b.send<ctd>( u_id, 10 );
    CHECK( s.get_events( ctd ).total( "damage", damage_to_u ) == 20 );
    CHECK( s.get_events( event_type::character_takes_damage ).total( "damage", damage_to_any ) == 30 );
    b.send<event_type::character_takes_damage>( u_id, 5 );
    CHECK( s.get_events( event_type::character_takes_damage ).total( "damage", damage_to_u ) == 25 );
    CHECK( s.get_events( event_type::character_takes_damage ).total( "damage", damage_to_any ) == 35 );
}

TEST_CASE( "stats_tracker_minimum_events", "[stats]" )
{
    stats_tracker s;
    event_bus b;
    b.subscribe( &s );

    const cata::event::data_type min_z_any {};
    const mtype_id no_monster;
    const ter_id t_null( "t_null" );
    constexpr event_type am = event_type::avatar_moves;

    CHECK( s.get_events( am ).minimum( "z" ) == 0 );
    b.send<am>( no_monster, t_null, character_movemode::CMM_WALK, false, 0 );
    CHECK( s.get_events( am ).minimum( "z" ) == 0 );
    b.send<am>( no_monster, t_null, character_movemode::CMM_WALK, false, -1 );
    CHECK( s.get_events( am ).minimum( "z" ) == -1 );
    b.send<am>( no_monster, t_null, character_movemode::CMM_WALK, false, 1 );
    CHECK( s.get_events( am ).minimum( "z" ) == -1 );
    b.send<am>( no_monster, t_null, character_movemode::CMM_WALK, false, -3 );
    CHECK( s.get_events( am ).minimum( "z" ) == -3 );
    b.send<am>( no_monster, t_null, character_movemode::CMM_WALK, true, -1 );
    CHECK( s.get_events( am ).minimum( "z" ) == -3 );
    b.send<am>( no_monster, t_null, character_movemode::CMM_WALK, true, 1 );
    CHECK( s.get_events( am ).minimum( "z" ) == -3 );
    b.send<am>( no_monster, t_null, character_movemode::CMM_WALK, true, -5 );
    CHECK( s.get_events( am ).minimum( "z" ) == -5 );
}

TEST_CASE( "stats_tracker_maximum_events", "[stats]" )
{
    stats_tracker s;
    event_bus b;
    b.subscribe( &s );

    const cata::event::data_type max_z_any {};
    const mtype_id no_monster;
    const ter_id t_null( "t_null" );
    constexpr event_type am = event_type::avatar_moves;

    CHECK( s.get_events( am ).maximum( "z" ) == 0 );
    b.send<am>( no_monster, t_null, character_movemode::CMM_WALK, false, 0 );
    CHECK( s.get_events( am ).maximum( "z" ) == 0 );
    b.send<am>( no_monster, t_null, character_movemode::CMM_WALK, false, 1 );
    CHECK( s.get_events( am ).maximum( "z" ) == 1 );
    b.send<am>( no_monster, t_null, character_movemode::CMM_WALK, false, 1 );
    CHECK( s.get_events( am ).maximum( "z" ) == 1 );
    b.send<am>( no_monster, t_null, character_movemode::CMM_WALK, false, 3 );
    CHECK( s.get_events( am ).maximum( "z" ) == 3 );
    b.send<am>( no_monster, t_null, character_movemode::CMM_WALK, true, 1 );
    CHECK( s.get_events( am ).maximum( "z" ) == 3 );
    b.send<am>( no_monster, t_null, character_movemode::CMM_WALK, true, 1 );
    CHECK( s.get_events( am ).maximum( "z" ) == 3 );
    b.send<am>( no_monster, t_null, character_movemode::CMM_WALK, true, 5 );
    CHECK( s.get_events( am ).maximum( "z" ) == 5 );
}

TEST_CASE( "stats_tracker_with_event_statistics", "[stats]" )
{
    stats_tracker s;
    event_bus b;
    b.subscribe( &s );

    SECTION( "movement" ) {
        const mtype_id no_monster;
        const mtype_id horse( "mon_horse" );
        const ter_id t_null( "t_null" );
        const ter_id t_water_dp( "t_water_dp" );

        const cata::event walk = cata::event::make<event_type::avatar_moves>( no_monster, t_null,
                                 character_movemode::CMM_WALK,  false, 0 );
        const cata::event ride = cata::event::make<event_type::avatar_moves>( horse, t_null,
                                 character_movemode::CMM_WALK,
                                 false, 0 );
        const cata::event run = cata::event::make<event_type::avatar_moves>( no_monster, t_null,
                                character_movemode::CMM_RUN, false, 0 );
        const cata::event crouch = cata::event::make<event_type::avatar_moves>( no_monster, t_null,
                                   character_movemode::CMM_CROUCH, false, 0 );
        const cata::event swim = cata::event::make<event_type::avatar_moves>( no_monster, t_water_dp,
                                 character_movemode::CMM_WALK, false, 0 );
        const cata::event swim_underwater = cata::event::make<event_type::avatar_moves>( no_monster,
                                            t_water_dp, character_movemode::CMM_WALK, true, 0 );

        const string_id<score> score_moves( "score_moves" );
        const string_id<score> score_walked( "score_distance_walked" );
        const string_id<score> score_mounted( "score_distance_mounted" );
        const string_id<score> score_ran( "score_distance_ran" );
        const string_id<score> score_crouched( "score_distance_crouched" );
        const string_id<score> score_swam( "score_distance_swam" );
        const string_id<score> score_swam_underwater( "score_distance_swam_underwater" );

        CHECK( score_moves->value( s ).get<int>() == 0 );
        CHECK( score_walked->value( s ).get<int>() == 0 );
        CHECK( score_mounted->value( s ).get<int>() == 0 );
        CHECK( score_ran->value( s ).get<int>() == 0 );
        CHECK( score_crouched->value( s ).get<int>() == 0 );
        CHECK( score_swam->value( s ).get<int>() == 0 );
        CHECK( score_swam_underwater->value( s ).get<int>() == 0 );

        b.send( walk );

        CHECK( score_moves->value( s ).get<int>() == 1 );
        CHECK( score_walked->value( s ).get<int>() == 1 );
        CHECK( score_mounted->value( s ).get<int>() == 0 );
        CHECK( score_ran->value( s ).get<int>() == 0 );
        CHECK( score_crouched->value( s ).get<int>() == 0 );
        CHECK( score_swam->value( s ).get<int>() == 0 );
        CHECK( score_swam_underwater->value( s ).get<int>() == 0 );

        b.send( ride );

        CHECK( score_moves->value( s ).get<int>() == 2 );
        CHECK( score_walked->value( s ).get<int>() == 1 );
        CHECK( score_mounted->value( s ).get<int>() == 1 );
        CHECK( score_ran->value( s ).get<int>() == 0 );
        CHECK( score_crouched->value( s ).get<int>() == 0 );
        CHECK( score_swam->value( s ).get<int>() == 0 );
        CHECK( score_swam_underwater->value( s ).get<int>() == 0 );

        b.send( run );

        CHECK( score_moves->value( s ).get<int>() == 3 );
        CHECK( score_walked->value( s ).get<int>() == 1 );
        CHECK( score_mounted->value( s ).get<int>() == 1 );
        CHECK( score_ran->value( s ).get<int>() == 1 );
        CHECK( score_crouched->value( s ).get<int>() == 0 );
        CHECK( score_swam->value( s ).get<int>() == 0 );
        CHECK( score_swam_underwater->value( s ).get<int>() == 0 );

        b.send( crouch );

        CHECK( score_moves->value( s ).get<int>() == 4 );
        CHECK( score_walked->value( s ).get<int>() == 1 );
        CHECK( score_mounted->value( s ).get<int>() == 1 );
        CHECK( score_ran->value( s ).get<int>() == 1 );
        CHECK( score_crouched->value( s ).get<int>() == 1 );
        CHECK( score_swam->value( s ).get<int>() == 0 );
        CHECK( score_swam_underwater->value( s ).get<int>() == 0 );

        b.send( swim );

        CHECK( score_moves->value( s ).get<int>() == 5 );
        CHECK( score_walked->value( s ).get<int>() == 1 );
        CHECK( score_mounted->value( s ).get<int>() == 1 );
        CHECK( score_ran->value( s ).get<int>() == 1 );
        CHECK( score_crouched->value( s ).get<int>() == 1 );
        CHECK( score_swam->value( s ).get<int>() == 1 );
        CHECK( score_swam_underwater->value( s ).get<int>() == 0 );

        b.send( swim_underwater );

        CHECK( score_moves->value( s ).get<int>() == 6 );
        CHECK( score_walked->value( s ).get<int>() == 1 );
        CHECK( score_mounted->value( s ).get<int>() == 1 );
        CHECK( score_ran->value( s ).get<int>() == 1 );
        CHECK( score_crouched->value( s ).get<int>() == 1 );
        CHECK( score_swam->value( s ).get<int>() == 2 );
        CHECK( score_swam_underwater->value( s ).get<int>() == 1 );
    }

    SECTION( "kills" ) {
        const character_id u_id = g->u.getID();
        character_id other_id = u_id;
        ++other_id;
        const mtype_id mon_zombie( "mon_zombie" );
        const mtype_id mon_dog( "mon_dog" );
        const cata::event avatar_zombie_kill =
            cata::event::make<event_type::character_kills_monster>( u_id, mon_zombie );
        const cata::event avatar_dog_kill =
            cata::event::make<event_type::character_kills_monster>( u_id, mon_dog );
        const cata::event other_kill =
            cata::event::make<event_type::character_kills_monster>( other_id, mon_zombie );
        const string_id<event_statistic> avatar_id( "avatar_id" );
        const string_id<event_statistic> num_avatar_kills( "num_avatar_kills" );
        const string_id<event_statistic> num_avatar_zombie_kills( "num_avatar_zombie_kills" );
        const string_id<score> score_kills( "score_kills" );

        b.send<event_type::game_start>( u_id );
        CHECK( avatar_id->value( s ) == cata_variant( u_id ) );
        CHECK( score_kills->value( s ).get<int>() == 0 );
        CHECK( score_kills->description( s ) == "0 monsters killed" );
        b.send( avatar_zombie_kill );
        CHECK( num_avatar_kills->value( s ).get<int>() == 1 );
        CHECK( num_avatar_zombie_kills->value( s ).get<int>() == 1 );
        CHECK( score_kills->value( s ).get<int>() == 1 );
        CHECK( score_kills->description( s ) == "1 monster killed" );
        b.send( avatar_dog_kill );
        CHECK( num_avatar_kills->value( s ).get<int>() == 2 );
        CHECK( num_avatar_zombie_kills->value( s ).get<int>() == 1 );
        CHECK( score_kills->value( s ).get<int>() == 2 );
        b.send( other_kill );
        CHECK( num_avatar_zombie_kills->value( s ).get<int>() == 1 );
        CHECK( score_kills->value( s ).get<int>() == 2 );
    }

    SECTION( "damage" ) {
        const character_id u_id = g->u.getID();
        const cata::event avatar_2_damage =
            cata::event::make<event_type::character_takes_damage>( u_id, 2 );
        const string_id<score> damage_taken( "score_damage_taken" );

        b.send<event_type::game_start>( u_id );
        CHECK( damage_taken->value( s ).get<int>() == 0 );
        b.send( avatar_2_damage );
        CHECK( damage_taken->value( s ).get<int>() == 2 );
    }
}

struct watch_stat : stat_watcher {
    void new_value( const cata_variant &v, stats_tracker & ) override {
        value = v;
    }
    cata_variant value;
};

TEST_CASE( "stats_tracker_watchers", "[stats]" )
{
    stats_tracker s;
    event_bus b;
    b.subscribe( &s );

    SECTION( "movement" ) {
        const mtype_id no_monster;
        const mtype_id horse( "mon_horse" );
        const ter_id t_null( "t_null" );
        const ter_id t_water_dp( "t_water_dp" );

        const cata::event walk = cata::event::make<event_type::avatar_moves>( no_monster, t_null,
                                 character_movemode::CMM_WALK, false, 0 );
        const cata::event ride = cata::event::make<event_type::avatar_moves>( horse, t_null,
                                 character_movemode::CMM_WALK,
                                 false, 0 );
        const cata::event run = cata::event::make<event_type::avatar_moves>( no_monster, t_null,
                                character_movemode::CMM_RUN, false, 0 );
        const cata::event crouch = cata::event::make<event_type::avatar_moves>( no_monster, t_null,
                                   character_movemode::CMM_CROUCH, false, 0 );
        const cata::event swim = cata::event::make<event_type::avatar_moves>( no_monster, t_water_dp,
                                 character_movemode::CMM_WALK, false, 0 );
        const cata::event swim_underwater = cata::event::make<event_type::avatar_moves>( no_monster,
                                            t_water_dp, character_movemode::CMM_WALK, true, 0 );

        const string_id<event_statistic> stat_moves( "num_moves" );
        const string_id<event_statistic> stat_walked( "num_moves_walked" );
        const string_id<event_statistic> stat_mounted( "num_moves_mounted" );
        const string_id<event_statistic> stat_ran( "num_moves_ran" );
        const string_id<event_statistic> stat_crouched( "num_moves_crouched" );
        const string_id<event_statistic> stat_swam( "num_moves_swam" );
        const string_id<event_statistic> stat_swam_underwater( "num_moves_swam_underwater" );

        watch_stat moves_watcher;
        watch_stat walked_watcher;
        watch_stat mounted_watcher;
        watch_stat ran_watcher;
        watch_stat crouched_watcher;
        watch_stat swam_watcher;
        watch_stat swam_underwater_watcher;

        s.add_watcher( stat_moves, &moves_watcher );
        s.add_watcher( stat_walked, &walked_watcher );
        s.add_watcher( stat_mounted, &mounted_watcher );
        s.add_watcher( stat_ran, &ran_watcher );
        s.add_watcher( stat_crouched, &crouched_watcher );
        s.add_watcher( stat_swam, &swam_watcher );
        s.add_watcher( stat_swam_underwater, &swam_underwater_watcher );

        CHECK( moves_watcher.value == cata_variant() );
        CHECK( walked_watcher.value == cata_variant() );
        CHECK( mounted_watcher.value == cata_variant() );
        CHECK( ran_watcher.value == cata_variant() );
        CHECK( crouched_watcher.value == cata_variant() );
        CHECK( swam_watcher.value == cata_variant() );
        CHECK( swam_underwater_watcher.value == cata_variant() );

        b.send( walk );

        CHECK( moves_watcher.value == cata_variant( 1 ) );
        CHECK( walked_watcher.value == cata_variant( 1 ) );
        CHECK( mounted_watcher.value == cata_variant() );
        CHECK( ran_watcher.value == cata_variant() );
        CHECK( crouched_watcher.value == cata_variant() );
        CHECK( swam_watcher.value == cata_variant() );
        CHECK( swam_underwater_watcher.value == cata_variant() );

        b.send( ride );

        CHECK( moves_watcher.value == cata_variant( 2 ) );
        CHECK( walked_watcher.value == cata_variant( 1 ) );
        CHECK( mounted_watcher.value == cata_variant( 1 ) );
        CHECK( ran_watcher.value == cata_variant() );
        CHECK( crouched_watcher.value == cata_variant() );
        CHECK( swam_watcher.value == cata_variant() );
        CHECK( swam_underwater_watcher.value == cata_variant() );

        b.send( run );

        CHECK( moves_watcher.value == cata_variant( 3 ) );
        CHECK( walked_watcher.value == cata_variant( 1 ) );
        CHECK( mounted_watcher.value == cata_variant( 1 ) );
        CHECK( ran_watcher.value == cata_variant( 1 ) );
        CHECK( crouched_watcher.value == cata_variant() );
        CHECK( swam_watcher.value == cata_variant() );
        CHECK( swam_underwater_watcher.value == cata_variant() );

        b.send( crouch );

        CHECK( moves_watcher.value == cata_variant( 4 ) );
        CHECK( walked_watcher.value == cata_variant( 1 ) );
        CHECK( mounted_watcher.value == cata_variant( 1 ) );
        CHECK( ran_watcher.value == cata_variant( 1 ) );
        CHECK( crouched_watcher.value == cata_variant( 1 ) );
        CHECK( swam_watcher.value == cata_variant() );
        CHECK( swam_underwater_watcher.value == cata_variant() );

        b.send( swim );

        CHECK( moves_watcher.value == cata_variant( 5 ) );
        CHECK( walked_watcher.value == cata_variant( 1 ) );
        CHECK( mounted_watcher.value == cata_variant( 1 ) );
        CHECK( ran_watcher.value == cata_variant( 1 ) );
        CHECK( crouched_watcher.value == cata_variant( 1 ) );
        CHECK( swam_watcher.value == cata_variant( 1 ) );
        CHECK( swam_underwater_watcher.value == cata_variant() );

        b.send( swim_underwater );

        CHECK( moves_watcher.value == cata_variant( 6 ) );
        CHECK( walked_watcher.value == cata_variant( 1 ) );
        CHECK( mounted_watcher.value == cata_variant( 1 ) );
        CHECK( ran_watcher.value == cata_variant( 1 ) );
        CHECK( crouched_watcher.value == cata_variant( 1 ) );
        CHECK( swam_watcher.value == cata_variant( 2 ) );
        CHECK( swam_underwater_watcher.value == cata_variant( 1 ) );
    }

    SECTION( "kills" ) {
        const character_id u_id = g->u.getID();
        character_id other_id = u_id;
        ++other_id;
        const mtype_id mon_zombie( "mon_zombie" );
        const mtype_id mon_dog( "mon_dog" );
        const cata::event avatar_zombie_kill =
            cata::event::make<event_type::character_kills_monster>( u_id, mon_zombie );
        const cata::event avatar_dog_kill =
            cata::event::make<event_type::character_kills_monster>( u_id, mon_dog );
        const cata::event other_kill =
            cata::event::make<event_type::character_kills_monster>( other_id, mon_zombie );
        const string_id<event_statistic> num_avatar_kills( "num_avatar_kills" );
        const string_id<event_statistic> num_avatar_zombie_kills( "num_avatar_zombie_kills" );

        watch_stat kills_watcher;
        watch_stat zombie_kills_watcher;
        s.add_watcher( num_avatar_kills, &kills_watcher );
        s.add_watcher( num_avatar_zombie_kills, &zombie_kills_watcher );

        b.send<event_type::game_start>( u_id );
        CHECK( kills_watcher.value == cata_variant( 0 ) );
        b.send( avatar_zombie_kill );
        CHECK( kills_watcher.value == cata_variant( 1 ) );
        CHECK( zombie_kills_watcher.value == cata_variant( 1 ) );
        b.send( avatar_dog_kill );
        CHECK( kills_watcher.value == cata_variant( 2 ) );
        CHECK( zombie_kills_watcher.value == cata_variant( 1 ) );
        b.send( other_kill );
        CHECK( kills_watcher.value == cata_variant( 2 ) );
        CHECK( zombie_kills_watcher.value == cata_variant( 1 ) );
    }

    SECTION( "damage" ) {
        const character_id u_id = g->u.getID();
        const cata::event avatar_2_damage =
            cata::event::make<event_type::character_takes_damage>( u_id, 2 );
        const string_id<event_statistic> damage_taken( "avatar_damage_taken" );
        watch_stat damage_watcher;
        s.add_watcher( damage_taken, &damage_watcher );

        CHECK( damage_watcher.value == cata_variant() );
        b.send<event_type::game_start>( u_id );
        CHECK( damage_watcher.value == cata_variant( 0 ) );
        b.send( avatar_2_damage );
        CHECK( damage_watcher.value == cata_variant( 2 ) );
    }
}

TEST_CASE( "achievments_tracker", "[stats]" )
{
    override_option opt( "24_HOUR", "military" );

    std::map<string_id<achievement>, const achievement *> achievements_completed;
    event_bus b;
    stats_tracker s;
    b.subscribe( &s );
    achievements_tracker a( s, [&]( const achievement * a ) {
        achievements_completed.emplace( a->id, a );
    } );
    b.subscribe( &a );

    SECTION( "time" ) {
        calendar::turn = calendar::start_of_game;
        const character_id u_id = g->u.getID();

        const cata::event avatar_wakes_up =
            cata::event::make<event_type::character_wakes_up>( u_id );

        string_id<achievement> a_survive_one_day( "achievement_survive_one_day" );

        b.send<event_type::game_start>( u_id );

        // Waking up before the time in question should do nothing
        b.send( avatar_wakes_up );
        CHECK( !achievements_completed.count( a_survive_one_day ) );

        // Waking up after a day has passed should get the achievement
        calendar::turn += 2_days;
        b.send( avatar_wakes_up );
        CHECK( achievements_completed.count( a_survive_one_day ) );
    }

    SECTION( "hidden_kills" ) {
        const character_id u_id = g->u.getID();
        const mtype_id mon_zombie( "mon_zombie" );
        const cata::event avatar_zombie_kill =
            cata::event::make<event_type::character_kills_monster>( u_id, mon_zombie );

        string_id<achievement> a_kill_10( "achievement_kill_10_monsters" );
        string_id<achievement> a_kill_100( "achievement_kill_100_monsters" );

        b.send<event_type::game_start>( u_id );

        CHECK( !a.is_hidden( &*a_kill_10 ) );
        CHECK( a.is_hidden( &*a_kill_100 ) );
        for( int i = 0; i < 10; ++i ) {
            b.send( avatar_zombie_kill );
        }
        CHECK( !a.is_hidden( &*a_kill_10 ) );
        CHECK( !a.is_hidden( &*a_kill_100 ) );
    }

    SECTION( "kills" ) {
        time_duration time_since_game_start = GENERATE( 30_seconds, 10_minutes );
        CAPTURE( time_since_game_start );
        calendar::turn = calendar::start_of_game + time_since_game_start;

        const character_id u_id = g->u.getID();
        const mtype_id mon_zombie( "mon_zombie" );
        const cata::event avatar_zombie_kill =
            cata::event::make<event_type::character_kills_monster>( u_id, mon_zombie );

        string_id<achievement> a_kill_zombie( "achievement_kill_zombie" );
        string_id<achievement> a_kill_in_first_minute( "achievement_kill_in_first_minute" );

        b.send<event_type::game_start>( u_id );

        CHECK( a.ui_text_for( &*a_kill_zombie ) ==
               "<color_c_yellow>One down, billions to go…</color>\n"
               "  <color_c_yellow>0/1 zombie killed</color>\n" );
        if( time_since_game_start < 1_minutes ) {
            CHECK( a.ui_text_for( &*a_kill_in_first_minute ) ==
                   "<color_c_yellow>Rude awakening</color>\n"
                   "  <color_c_light_green>Within 1 minute of start of game (30 seconds remaining)</color>\n"
                   "  <color_c_yellow>0/1 monster killed</color>\n" );
        } else {
            CHECK( a.ui_text_for( &*a_kill_in_first_minute ) ==
                   "<color_c_light_gray>Rude awakening</color>\n"
                   "  <color_c_light_gray>Within 1 minute of start of game (passed)</color>\n"
                   "  <color_c_yellow>0/1 monster killed</color>\n" );
        }

        CHECK( achievements_completed.empty() );
        b.send( avatar_zombie_kill );

        if( time_since_game_start < 1_minutes ) {
            CHECK( a.ui_text_for( achievements_completed.at( a_kill_zombie ) ) ==
                   "<color_c_light_green>One down, billions to go…</color>\n"
                   "  <color_c_light_green>Completed Year 1, Spring, day 1 0000.30</color>\n"
                   "  <color_c_green>1/1 zombie killed</color>\n" );
            CHECK( a.ui_text_for( achievements_completed.at( a_kill_in_first_minute ) ) ==
                   "<color_c_light_green>Rude awakening</color>\n"
                   "  <color_c_light_green>Completed Year 1, Spring, day 1 0000.30</color>\n"
                   "  <color_c_green>1/1 monster killed</color>\n" );
        } else {
            CHECK( a.ui_text_for( achievements_completed.at( a_kill_zombie ) ) ==
                   "<color_c_light_green>One down, billions to go…</color>\n"
                   "  <color_c_light_green>Completed Year 1, Spring, day 1 0010.00</color>\n"
                   "  <color_c_green>1/1 zombie killed</color>\n" );
            CHECK( !achievements_completed.count( a_kill_in_first_minute ) );
            CHECK( a.ui_text_for( &*a_kill_in_first_minute ) ==
                   "<color_c_light_gray>Rude awakening</color>\n"
                   "  <color_c_light_gray>Within 1 minute of start of game (passed)</color>\n"
                   "  <color_c_yellow>0/1 monster killed</color>\n" );
        }

        // Advance a minute and kill again
        calendar::turn += 1_minutes;
        b.send( avatar_zombie_kill );

        if( time_since_game_start < 1_minutes ) {
            CHECK( a.ui_text_for( achievements_completed.at( a_kill_zombie ) ) ==
                   "<color_c_light_green>One down, billions to go…</color>\n"
                   "  <color_c_light_green>Completed Year 1, Spring, day 1 0000.30</color>\n"
                   "  <color_c_green>1/1 zombie killed</color>\n" );
            CHECK( a.ui_text_for( achievements_completed.at( a_kill_in_first_minute ) ) ==
                   "<color_c_light_green>Rude awakening</color>\n"
                   "  <color_c_light_green>Completed Year 1, Spring, day 1 0000.30</color>\n"
                   "  <color_c_green>1/1 monster killed</color>\n" );
        } else {
            CHECK( a.ui_text_for( achievements_completed.at( a_kill_zombie ) ) ==
                   "<color_c_light_green>One down, billions to go…</color>\n"
                   "  <color_c_light_green>Completed Year 1, Spring, day 1 0010.00</color>\n"
                   "  <color_c_green>1/1 zombie killed</color>\n" );
            CHECK( !achievements_completed.count( a_kill_in_first_minute ) );
            CHECK( a.ui_text_for( &*a_kill_in_first_minute ) ==
                   "<color_c_light_gray>Rude awakening</color>\n"
                   "  <color_c_light_gray>Within 1 minute of start of game (passed)</color>\n"
                   "  <color_c_yellow>0/1 monster killed</color>\n" );
        }
    }

    SECTION( "movement" ) {
        const mtype_id no_monster;
        const ter_id t_null( "t_null" );
        const ter_id t_water_dp( "t_water_dp" );
        const ter_id t_shrub_raspberry( "t_shrub_raspberry" );
        const cata::event walk = cata::event::make<event_type::avatar_moves>( no_monster, t_null,
                                 character_movemode::CMM_WALK, false, 0 );
        const cata::event run = cata::event::make<event_type::avatar_moves>( no_monster, t_null,
                                character_movemode::CMM_RUN, false, 0 );
        const cata::event sharp_move = cata::event::make<event_type::avatar_moves>( no_monster,
                                       t_shrub_raspberry, character_movemode::CMM_WALK, false, 0 );
        const cata::event swim = cata::event::make<event_type::avatar_moves>( no_monster, t_water_dp,
                                 character_movemode::CMM_WALK, false, 0 );
        const cata::event swim_underwater = cata::event::make<event_type::avatar_moves>( no_monster,
                                            t_water_dp, character_movemode::CMM_WALK, true, 0 );
        const cata::event swim_underwater_deep = cata::event::make<event_type::avatar_moves>( no_monster,
                t_water_dp, character_movemode::CMM_WALK, true, -5 );
        const cata::event walk_max_z = cata::event::make<event_type::avatar_moves>( no_monster, t_null,
                                       character_movemode::CMM_WALK, false, OVERMAP_HEIGHT );
        const cata::event walk_min_z = cata::event::make<event_type::avatar_moves>( no_monster, t_null,
                                       character_movemode::CMM_WALK, false, -OVERMAP_DEPTH );

        SECTION( "achievement_marathon" ) {
            string_id<achievement> a_marathon( "achievement_marathon" );

            GIVEN( "a new game" ) {
                const character_id u_id = g->u.getID();
                b.send<event_type::game_start>( u_id );
                CHECK( achievements_completed.empty() );

                WHEN( "the avatar runs the required distance" ) {
                    for( int i = 0; i < 42196; i++ ) {
                        b.send( run );
                    }
                    THEN( "the achivement should be achieved" ) {
                        CHECK( achievements_completed.count( a_marathon ) > 0 );
                    }
                }
            }
        }

        SECTION( "achievement_walk_1000_miles" ) {
            string_id<achievement> a_proclaimers( "achievement_walk_1000_miles" );

            GIVEN( "a new game" ) {
                const character_id u_id = g->u.getID();
                b.send<event_type::game_start>( u_id );
                CHECK( achievements_completed.empty() );

                WHEN( "the avatar walks the required distance" ) {
                    for( int i = 0; i < 1609340; i++ ) {
                        b.send( walk );
                    }
                    THEN( "the achivement should be achieved" ) {
                        CHECK( achievements_completed.count( a_proclaimers ) > 0 );
                    }
                }
            }
        }

        SECTION( "achievement_traverse_sharp_terrain" ) {
            string_id<achievement> a_traverse_sharp_terrain( "achievement_traverse_sharp_terrain" );

            GIVEN( "a new game" ) {
                const character_id u_id = g->u.getID();
                b.send<event_type::game_start>( u_id );
                CHECK( achievements_completed.empty() );

                WHEN( "the avatar traverses the required number of sharp terrain" ) {
                    for( int i = 0; i < 100; i++ ) {
                        b.send( sharp_move );
                    }
                    THEN( "the achivement should be achieved" ) {
                        CHECK( achievements_completed.count( a_traverse_sharp_terrain ) > 0 );
                    }
                }
            }
        }

        SECTION( "achievement_swim_merit_badge" ) {
            string_id<achievement> a_swim_merit_badge( "achievement_swim_merit_badge" );

            GIVEN( "a new game" ) {
                const character_id u_id = g->u.getID();
                b.send<event_type::game_start>( u_id );
                CHECK( achievements_completed.empty() );

                WHEN( "the avatar does the required actions" ) {
                    for( int i = 0; i < 10000; i++ ) {
                        b.send( swim );
                    }
                    for( int i = 0; i < 1000; i++ ) {
                        b.send( swim_underwater );
                    }
                    b.send( swim_underwater_deep );
                    THEN( "the achivement should be achieved" ) {
                        CHECK( achievements_completed.count( a_swim_merit_badge ) > 0 );
                    }
                }
            }
        }

        SECTION( "achievement_reach_max_z_level" ) {
            string_id<achievement> a_reach_max_z_level( "achievement_reach_max_z_level" );

            GIVEN( "a new game" ) {
                const character_id u_id = g->u.getID();
                b.send<event_type::game_start>( u_id );
                CHECK( achievements_completed.empty() );

                WHEN( "the avatar does the required actions" ) {
                    b.send( walk_max_z );
                    THEN( "the achivement should be achieved" ) {
                        CHECK( achievements_completed.count( a_reach_max_z_level ) > 0 );
                    }
                }
            }
        }

        SECTION( "achievement_reach_min_z_level" ) {
            string_id<achievement> a_reach_min_z_level( "achievement_reach_min_z_level" );

            GIVEN( "a new game" ) {
                const character_id u_id = g->u.getID();
                b.send<event_type::game_start>( u_id );
                CHECK( achievements_completed.empty() );

                WHEN( "the avatar does the required actions" ) {
                    b.send( walk_min_z );
                    THEN( "the achivement should be achieved" ) {
                        CHECK( achievements_completed.count( a_reach_min_z_level ) > 0 );
                    }
                }
            }
        }
    }
}

TEST_CASE( "stats_tracker_in_game", "[stats]" )
{
    g->stats().clear();
    cata::event e = cata::event::make<event_type::awakes_dark_wyrms>();
    g->events().send( e );
    CHECK( g->stats().get_events( e.type() ).count( e.data() ) == 1 );
}
