#include <string>

#include "cata_catch.h"
#include "test_statistics.h"
#include "weighted_list.h"

// returns true if T is picked from list in trials attempts
template<typename W, typename T>
bool picked_in_trials( const weighted_list<W, T> &list, T value, int trials )
{
    for( int i = 0; i < trials; ++i ) {
        const T *picked = list.pick();
        REQUIRE( picked != nullptr );
        if( *picked == value ) {
            return true;
        }
    }
    return false;
}

template<typename W, typename T>
void list_check_add( weighted_list<W, T> &list, T value, W weight, bool succeeds )
{
    T *ret = list.add( value, weight );
    if( !succeeds ) {
        CHECK( ret == nullptr );
    } else {
        REQUIRE( ret != nullptr );
        CHECK( *ret == value );
    }
}

template<typename W, typename T>
void list_check_add_replace( weighted_list<W, T> &list, T value, W weight, bool succeeds )
{
    T *ret = list.add_or_replace( value, weight );
    if( !succeeds ) {
        CHECK( ret == nullptr );
    } else {
        REQUIRE( ret != nullptr );
        CHECK( *ret == value );
    }
}

TEST_CASE( "weighted_list_basic_function", "[weighted_list]" )
{
    constexpr int A = 1;
    constexpr int B = 2;
    constexpr int C = 3;

    weighted_int_list<int> list;

    SECTION( "adding negative weight" ) {
        list_check_add<int, int>( list, A, -1, false );

        CHECK( list.get_weight() == 0 );
        CHECK( list.get_specific_weight( A ) == 0 );
        CHECK( list.pick() == nullptr );
    }

    SECTION( "adding zero weight" ) {
        list_check_add<int, int>( list, A, 0, false );

        CHECK( list.get_weight() == 0 );
        CHECK( list.get_specific_weight( A ) == 0 );
        CHECK( list.pick() == nullptr );
    }

    SECTION( "adding single" ) {
        list_check_add<int, int>( list, A, 1, true );

        CHECK( list.get_weight() == 1 );
        CHECK( list.get_specific_weight( A ) == 1 );
        int *picked = list.pick();
        REQUIRE( picked != nullptr );
        CHECK( *picked == A );
    }

    SECTION( "clearing list" ) {
        list.clear();

        // ensure this test fails if clearing the list does not work
        REQUIRE( list.get_weight() == 0 );
        REQUIRE( list.pick() == nullptr );
    }

    SECTION( "remove" ) {
        constexpr int B_weight = 3;
        constexpr int C_weight = 4;
        list_check_add<int, int>( list, B, B_weight, true );
        list_check_add<int, int>( list, C, C_weight, true );

        REQUIRE( list.get_weight() == B_weight + C_weight );
        REQUIRE( list.get_specific_weight( B ) == B_weight );
        REQUIRE( list.get_specific_weight( C ) == C_weight );
        // 0.001% chance of random failure with 20 trials
        CHECK( picked_in_trials<int, int>( list, B, 20 ) );

        list.remove( B );

        CHECK( list.get_weight() == C_weight );
        CHECK( list.get_specific_weight( B ) == 0 );
        CHECK( list.get_specific_weight( C ) == C_weight );

        CHECK( picked_in_trials<int, int>( list, C, 1 ) );
        CHECK_FALSE( picked_in_trials<int, int>( list, B, 20 ) );
    }
}

TEST_CASE( "weighted_list_distribution", "[weighted_list]" )
{
    constexpr int H = 1;
    constexpr int I = 2;
    constexpr int J = 3;
    constexpr int K = 4;

    constexpr double MARGIN_OF_ERROR = 0.01;

    weighted_int_list<int> list;
    SECTION( "even distribution" ) {
        list.clear();

        constexpr int H_weight = 1;
        constexpr int I_weight = 1;
        constexpr int J_weight = 1;
        constexpr int K_weight = 1;
        list_check_add<int, int>( list, H, H_weight, true );
        list_check_add<int, int>( list, I, I_weight, true );
        list_check_add<int, int>( list, J, J_weight, true );
        list_check_add<int, int>( list, K, K_weight, true );

        REQUIRE( list.get_weight() == H_weight + I_weight + J_weight + K_weight );
        REQUIRE( list.get_specific_weight( H ) == H_weight );
        REQUIRE( list.get_specific_weight( I ) == I_weight );
        REQUIRE( list.get_specific_weight( J ) == J_weight );
        REQUIRE( list.get_specific_weight( K ) == K_weight );

        // each is at 25%
        const epsilon_threshold threshold{ 0.25, MARGIN_OF_ERROR };
        statistics<bool> H_stats;
        statistics<bool> I_stats;
        statistics<bool> J_stats;
        statistics<bool> K_stats;
        do {
            int *picked = list.pick();
            REQUIRE( picked != nullptr );
            H_stats.add( *picked == H );
            I_stats.add( *picked == I );
            J_stats.add( *picked == J );
            K_stats.add( *picked == K );
        } while( H_stats.uncertain_about( threshold ) || I_stats.uncertain_about( threshold ) ||
                 J_stats.uncertain_about( threshold ) || K_stats.uncertain_about( threshold ) );

        INFO( H_stats.n() );

        CHECK( H_stats.avg() == Approx( 0.25 ).margin( MARGIN_OF_ERROR ) );
        CHECK( I_stats.avg() == Approx( 0.25 ).margin( MARGIN_OF_ERROR ) );
        CHECK( J_stats.avg() == Approx( 0.25 ).margin( MARGIN_OF_ERROR ) );
        CHECK( K_stats.avg() == Approx( 0.25 ).margin( MARGIN_OF_ERROR ) );
    }

    SECTION( "non-even distribution / add_replace" ) {
        constexpr int H_weight = 1;
        constexpr int I_weight = 3;
        constexpr int J_weight = 9;
        constexpr int K_weight = 5;
        // use add_replace to adjust weights
        list_check_add_replace<int, int>( list, H, H_weight, true );
        list_check_add_replace<int, int>( list, I, I_weight, true );
        list_check_add_replace<int, int>( list, J, J_weight, true );
        list_check_add_replace<int, int>( list, K, K_weight, true );

        REQUIRE( list.get_weight() == H_weight + I_weight + J_weight + K_weight );
        REQUIRE( list.get_specific_weight( H ) == H_weight );
        REQUIRE( list.get_specific_weight( I ) == I_weight );
        REQUIRE( list.get_specific_weight( J ) == J_weight );
        REQUIRE( list.get_specific_weight( K ) == K_weight );

        // each is at 25%
        constexpr double total_weight = H_weight + I_weight + J_weight + K_weight;
        const epsilon_threshold H_threshold{ H_weight / total_weight, MARGIN_OF_ERROR };
        const epsilon_threshold I_threshold{ I_weight / total_weight, MARGIN_OF_ERROR };
        const epsilon_threshold J_threshold{ J_weight / total_weight, MARGIN_OF_ERROR };
        const epsilon_threshold K_threshold{ K_weight / total_weight, MARGIN_OF_ERROR };
        statistics<bool> H_stats;
        statistics<bool> I_stats;
        statistics<bool> J_stats;
        statistics<bool> K_stats;
        do {
            int *picked = list.pick();
            REQUIRE( picked != nullptr );
            H_stats.add( *picked == H );
            I_stats.add( *picked == I );
            J_stats.add( *picked == J );
            K_stats.add( *picked == K );
        } while( H_stats.uncertain_about( H_threshold ) || I_stats.uncertain_about( I_threshold ) ||
                 J_stats.uncertain_about( J_threshold ) || K_stats.uncertain_about( K_threshold ) );

        INFO( H_stats.n() );

        CHECK( H_stats.avg() == Approx( H_weight / total_weight ).margin( MARGIN_OF_ERROR ) );
        CHECK( I_stats.avg() == Approx( I_weight / total_weight ).margin( MARGIN_OF_ERROR ) );
        CHECK( J_stats.avg() == Approx( J_weight / total_weight ).margin( MARGIN_OF_ERROR ) );
        CHECK( K_stats.avg() == Approx( K_weight / total_weight ).margin( MARGIN_OF_ERROR ) );
    }
}

TEST_CASE( "weighted_list_weights_after_remove", "[weighted_list]" )
{
    constexpr int Q = 1;
    constexpr int R = 2;
    constexpr int S = 3;

    constexpr double MARGIN_OF_ERROR = 0.01;

    constexpr int Q_weight = 4;
    constexpr int R_weight = 8;
    constexpr int S_weight = 2;

    constexpr double total_weight = Q_weight + R_weight + S_weight;

    weighted_int_list<int> list;

    list_check_add<int, int>( list, Q, Q_weight, true );
    list_check_add<int, int>( list, R, R_weight, true );
    list_check_add<int, int>( list, S, S_weight, true );

    CHECK( list.get_weight() == total_weight );
    CHECK( list.get_specific_weight( Q ) == Q_weight );
    CHECK( list.get_specific_weight( R ) == R_weight );
    CHECK( list.get_specific_weight( S ) == S_weight );

    SECTION( "before_removal" ) {
        statistics<bool> Q_stats;
        statistics<bool> R_stats;
        statistics<bool> S_stats;
        epsilon_threshold Q_threshold{ Q_weight / total_weight, MARGIN_OF_ERROR };
        epsilon_threshold R_threshold{ R_weight / total_weight, MARGIN_OF_ERROR };
        epsilon_threshold S_threshold{ S_weight / total_weight, MARGIN_OF_ERROR };

        do {
            int *picked = list.pick();
            REQUIRE( picked != nullptr );
            Q_stats.add( *picked == Q );
            R_stats.add( *picked == R );
            S_stats.add( *picked == S );
        } while( Q_stats.uncertain_about( Q_threshold ) ||
                 R_stats.uncertain_about( R_threshold ) ||
                 S_stats.uncertain_about( S_threshold ) );

        INFO( Q_stats.n() );

        CHECK( Q_stats.avg() == Approx( Q_weight / total_weight ).margin( MARGIN_OF_ERROR ) );
        CHECK( R_stats.avg() == Approx( R_weight / total_weight ).margin( MARGIN_OF_ERROR ) );
        CHECK( S_stats.avg() == Approx( S_weight / total_weight ).margin( MARGIN_OF_ERROR ) );
    }

    SECTION( "after_removal" ) {
        list.remove( R );

        CHECK( list.get_weight() == Q_weight + S_weight );
        CHECK( list.get_specific_weight( Q ) == Q_weight );
        CHECK( list.get_specific_weight( R ) == 0 );
        CHECK( list.get_specific_weight( S ) == S_weight );

        statistics<bool> Q_stats;
        statistics<bool> R_stats;
        statistics<bool> S_stats;
        constexpr double new_total_weight = Q_weight + S_weight;
        epsilon_threshold Q_threshold{ Q_weight / new_total_weight, MARGIN_OF_ERROR };
        epsilon_threshold R_threshold{ 0.0, MARGIN_OF_ERROR };
        epsilon_threshold S_threshold{ S_weight / new_total_weight, MARGIN_OF_ERROR };

        do {
            int *picked = list.pick();
            REQUIRE( picked != nullptr );
            Q_stats.add( *picked == Q );
            R_stats.add( *picked == R );
            S_stats.add( *picked == S );
        } while( Q_stats.uncertain_about( Q_threshold ) ||
                 R_stats.uncertain_about( R_threshold ) ||
                 S_stats.uncertain_about( S_threshold ) );

        INFO( Q_stats.n() );

        CHECK( Q_stats.avg() == Approx( Q_weight / new_total_weight ).margin( MARGIN_OF_ERROR ) );
        CHECK( R_stats.avg() == Approx( 0.0 ).margin( MARGIN_OF_ERROR ) );
        CHECK( S_stats.avg() == Approx( S_weight / new_total_weight ).margin( MARGIN_OF_ERROR ) );
    }
}

TEST_CASE( "weighted_list_add_or_replace_for_zero", "[weighted_list]" )
{
    constexpr int VALUE = 12;
    constexpr int WEIGHT = 5;
    weighted_int_list<int> list;
    list_check_add( list, VALUE, WEIGHT, true );

    REQUIRE( list.get_specific_weight( VALUE ) == WEIGHT );
    REQUIRE( list.get_weight() == WEIGHT );

    int *picked = list.pick();
    REQUIRE( picked != nullptr );
    CHECK( *picked == VALUE );

    list_check_add_replace( list, VALUE, 0, false );

    CHECK( list.get_weight() == 0 );
    CHECK( list.get_specific_weight( VALUE ) == 0 );
    CHECK( list.pick() == nullptr );
}
