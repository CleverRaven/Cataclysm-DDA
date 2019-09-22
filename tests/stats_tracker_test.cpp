#include "catch/catch.hpp"

#include "avatar.h"
#include "event_statistics.h"
#include "game.h"
#include "stats_tracker.h"
#include "stringmaker.h"

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

TEST_CASE( "stats_tracker_with_event_statistics", "[stats]" )
{
    stats_tracker s;
    event_bus b;
    b.subscribe( &s );

    SECTION( "movement" ) {
        const mtype_id no_monster;
        const mtype_id horse( "mon_horse" );
        const cata::event walk = cata::event::make<event_type::avatar_moves>( no_monster );
        const cata::event ride = cata::event::make<event_type::avatar_moves>( horse );
        const string_id<score> score_moves( "score_moves" );
        const string_id<score> score_walked( "score_walked" );

        CHECK( score_walked->value( s ) == cata_variant( 0 ) );
        CHECK( score_moves->value( s ) == cata_variant( 0 ) );
        b.send( walk );
        CHECK( score_walked->value( s ) == cata_variant( 1 ) );
        CHECK( score_moves->value( s ) == cata_variant( 1 ) );
        b.send( ride );
        CHECK( score_walked->value( s ) == cata_variant( 1 ) );
        CHECK( score_moves->value( s ) == cata_variant( 2 ) );
    }

    SECTION( "kills" ) {
        const character_id u_id = g->u.getID();
        character_id other_id = u_id;
        ++other_id;
        const mtype_id mon( "mon_zombie" );
        const cata::event avatar_kill =
            cata::event::make<event_type::character_kills_monster>( u_id, mon );
        const cata::event other_kill =
            cata::event::make<event_type::character_kills_monster>( other_id, mon );
        const string_id<event_statistic> avatar_id( "avatar_id" );
        const string_id<event_statistic> num_avatar_kills( "num_avatar_kills" );
        const string_id<score> score_kills( "score_kills" );

        b.send<event_type::game_start>( u_id );
        CHECK( avatar_id->value( s ) == cata_variant( u_id ) );
        CHECK( score_kills->value( s ).get<int>() == 0 );
        b.send( avatar_kill );
        CHECK( num_avatar_kills->value( s ).get<int>() == 1 );
        CHECK( score_kills->value( s ).get<int>() == 1 );
        b.send( other_kill );
        CHECK( score_kills->value( s ).get<int>() == 1 );
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
        const cata::event walk = cata::event::make<event_type::avatar_moves>( no_monster );
        const cata::event ride = cata::event::make<event_type::avatar_moves>( horse );
        const string_id<event_statistic> stat_moves( "num_moves" );
        const string_id<event_statistic> stat_walked( "num_moves_not_mounted" );

        watch_stat moves_watcher;
        watch_stat walks_watcher;
        s.add_watcher( stat_moves, &moves_watcher );
        s.add_watcher( stat_walked, &walks_watcher );

        CHECK( walks_watcher.value == cata_variant() );
        CHECK( moves_watcher.value == cata_variant() );
        b.send( walk );
        CHECK( walks_watcher.value == cata_variant( 1 ) );
        CHECK( moves_watcher.value == cata_variant( 1 ) );
        b.send( ride );
        CHECK( walks_watcher.value == cata_variant( 1 ) );
        CHECK( moves_watcher.value == cata_variant( 2 ) );
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

TEST_CASE( "stats_tracker_in_game", "[stats]" )
{
    g->stats().clear();
    cata::event e = cata::event::make<event_type::awakes_dark_wyrms>();
    g->events().send( e );
    CHECK( g->stats().get_events( e.type() ).count( e.data() ) == 1 );
}
