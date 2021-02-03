#include "catch/catch.hpp"

#include <fstream>
#include "monfaction.h"

// generates a file in current directory that contains dump of all inter-faction attitude
TEST_CASE( "generate_monfactions_attitude_matrix", "[.]" )
{
    std::ofstream outfile;
    outfile.open( "monfactions.txt" );
    for( const auto &f : monfactions::get_all() ) {
        for( const auto &f1 : monfactions::get_all() ) {
            mf_attitude att = f.attitude( f1.id );
            outfile << f.id.str() << "->" << f1.id.str() << ": ";
            switch( att ) {
                case MFA_BY_MOOD:
                    outfile << "MFA_BY_MOOD";
                    break;
                case MFA_NEUTRAL:
                    outfile << "MFA_NEUTRAL";
                    break;
                case MFA_FRIENDLY:
                    outfile << "MFA_FRIENDLY";
                    break;
                case MFA_HATE:
                    outfile << "MFA_HATE";
                    break;
                case MFA_SIZE:
                    outfile << "MFA_SIZE";
                    break;
            }
            outfile << "\n";
        }
    }
}

TEST_CASE( "monfactions_attitude", "[monster][monfactions]" )
{
    // check some common cases
    const auto attitude = []( const std::string & f, const std::string & t ) {
        return mfaction_str_id( f )->attitude( mfaction_str_id( t ) );
    };

    SECTION( "faction is friendly to itself" ) {
        CHECK( attitude( "zombie", "zombie" ) == MFA_FRIENDLY );
        CHECK( attitude( "human", "human" ) == MFA_FRIENDLY );
    }

    SECTION( "inheritance" ) {
        // based on the current state of json
        REQUIRE( attitude( "animal", "small_animal" ) == MFA_NEUTRAL );
        REQUIRE( mfaction_str_id( "small_animal" )->base_faction == mfaction_str_id( "animal" ) );
        REQUIRE( mfaction_str_id( "fish" )->base_faction == mfaction_str_id( "animal" ) );
        REQUIRE( attitude( "animal", "small_animal" ) == MFA_NEUTRAL );

        INFO( "default attitude towards self takes precedence over inheritance from parents" );
        CHECK( attitude( "small_animal", "small_animal" ) == MFA_FRIENDLY );

        INFO( "fish is inherited from animal and should be neutral toward small_animal" );
        CHECK( attitude( "fish", "small_animal" ) == MFA_NEUTRAL );

        INFO( "fish is a child of small_animal, and small_animal is friendly to itself" );
        CHECK( attitude( "small_animal", "fish" ) == MFA_FRIENDLY );
    }

    SECTION( "some random samples" ) {
        CHECK( attitude( "aquatic_predator", "fish" ) == MFA_HATE );
        CHECK( attitude( "robofac", "cop_zombie" ) == MFA_HATE );

        CHECK( attitude( "dragonfly", "defense_bot" ) == MFA_NEUTRAL );
        CHECK( attitude( "dragonfly", "dermatik" ) == MFA_HATE );

        CHECK( attitude( "zombie_aquatic", "zombie" ) == MFA_FRIENDLY );
        CHECK( attitude( "zombie", "zombie_aquatic" ) == MFA_FRIENDLY );
        CHECK( attitude( "zombie", "spider_web" ) == MFA_NEUTRAL );

        CHECK( attitude( "plant", "triffid" ) == MFA_FRIENDLY );
        CHECK( attitude( "plant", "utility_bot" ) == MFA_NEUTRAL );

        CHECK( attitude( "bee", "military" ) == MFA_NEUTRAL );
    }
}