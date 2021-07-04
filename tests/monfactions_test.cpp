#include <fstream>
#include <vector>

#include "cata_catch.h"
#include "monfaction.h"
#include "type_id.h"

// generates a file in current directory that contains dump of all inter-faction attitude
TEST_CASE( "generate_monfactions_attitude_matrix", "[.]" )
{
    std::ofstream outfile;
    outfile.open( "monfactions.txt" );
    for( const auto &f : monfactions::get_all() ) {
        for( const auto &f1 : monfactions::get_all() ) {
            mf_attitude att = f.attitude( f1.id );
            mf_attitude rev_att = f1.attitude( f.id );
            // NOLINTNEXTLINE(cata-text-style)
            outfile << f.id.str() << "->" << f1.id.str() << ":\t";
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
            // NOLINTNEXTLINE(cata-text-style)
            outfile << "\t(Rev: ";
            switch( rev_att ) {
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
            outfile << ")\n";
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
        REQUIRE( mfaction_str_id( "vermin" )->base_faction == mfaction_str_id( "small_animal" ) );
        REQUIRE( mfaction_str_id( "fish" )->base_faction == mfaction_str_id( "animal" ) );
        REQUIRE( mfaction_str_id( "bear" )->base_faction == mfaction_str_id( "animal" ) );

        INFO( "fish is a child of animal, is friendly to itself" );
        CHECK( attitude( "animal", "animal" ) == MFA_BY_MOOD );

        INFO( "default attitude towards self takes precedence over inheritance from parents" );
        CHECK( attitude( "animal", "fish" ) == MFA_NEUTRAL );
        CHECK( attitude( "fish", "fish" ) == MFA_FRIENDLY );

        INFO( "fish is inherited from animal and should be neutral toward small_animal" );
        CHECK( attitude( "fish", "small_animal" ) == MFA_NEUTRAL );

        INFO( "dog is inherited from animal, but hates small animals, of which vermin is a child" );
        CHECK( attitude( "dog", "vermin" ) == MFA_HATE );
        CHECK( attitude( "dog", "fish" ) == MFA_NEUTRAL );

    }

    SECTION( "some random samples" ) {
        CHECK( attitude( "aquatic_predator", "fish" ) == MFA_HATE );
        CHECK( attitude( "robofac", "zombie" ) == MFA_HATE );

        CHECK( attitude( "dragonfly", "defense_bot" ) == MFA_NEUTRAL );
        CHECK( attitude( "dragonfly", "dermatik" ) == MFA_HATE );

        CHECK( attitude( "zombie_aquatic", "zombie" ) == MFA_FRIENDLY );
        CHECK( attitude( "zombie", "zombie_aquatic" ) == MFA_FRIENDLY );
        CHECK( attitude( "zombie", "spider_web" ) == MFA_NEUTRAL );
        CHECK( attitude( "zombie", "small_animal" ) == MFA_NEUTRAL );

        CHECK( attitude( "plant", "triffid" ) == MFA_FRIENDLY );
        CHECK( attitude( "plant", "utility_bot" ) == MFA_NEUTRAL );

        CHECK( attitude( "bee", "military" ) == MFA_NEUTRAL );

        CHECK( attitude( "bear", "bee" ) == MFA_HATE );
        CHECK( attitude( "wolf", "pig" ) == MFA_HATE );
        CHECK( attitude( "small_animal", "zombie" ) == MFA_NEUTRAL );
    }
}
