#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "cata_catch.h"
#include "monfaction.h"
#include "type_id.h"

static const mfaction_str_id monfaction_animal( "animal" );
static const mfaction_str_id monfaction_bear( "bear" );
static const mfaction_str_id monfaction_fish( "fish" );
static const mfaction_str_id monfaction_small_animal( "small_animal" );
static const mfaction_str_id monfaction_test_monfaction1( "test_monfaction1" );
static const mfaction_str_id monfaction_test_monfaction2( "test_monfaction2" );
static const mfaction_str_id monfaction_test_monfaction3( "test_monfaction3" );
static const mfaction_str_id monfaction_test_monfaction5( "test_monfaction5" );
static const mfaction_str_id monfaction_test_monfaction7( "test_monfaction7" );
static const mfaction_str_id monfaction_test_monfaction_extend( "test_monfaction_extend" );
static const mfaction_str_id monfaction_tiny_animal( "tiny_animal" );

static std::string att_enum_to_string( mf_attitude att )
{
    switch( att ) {
        case MFA_BY_MOOD:
            return "MFA_BY_MOOD";
        case MFA_NEUTRAL:
            return "MFA_NEUTRAL";
        case MFA_FRIENDLY:
            return "MFA_FRIENDLY";
        case MFA_HATE:
            return "MFA_HATE";
        case MFA_SIZE:
            return "MFA_SIZE";
        default:
            break;
    }

    printf( "Unknown monster faction attitude %d\n", static_cast<int>( att ) );
    return "?";
}

// generates a file in current directory that contains dump of all inter-faction attitude
TEST_CASE( "generate_monfactions_attitude_matrix", "[.]" )
{
    std::ofstream outfile;
    outfile.open( std::filesystem::u8path( "monfactions.txt" ) );
    for( const monfaction &f : monfactions::get_all() ) {
        for( const monfaction &f1 : monfactions::get_all() ) {
            mf_attitude att = f.attitude( f1.id );
            mf_attitude rev_att = f1.attitude( f.id );
            // NOLINTNEXTLINE(cata-text-style)
            outfile << f.id.str() << "->" << f1.id.str() << ":\t";
            outfile << att_enum_to_string( att );
            // NOLINTNEXTLINE(cata-text-style)
            outfile << "\t(Rev: ";
            outfile << att_enum_to_string( rev_att );
            outfile << ")\n";
        }
    }
}

TEST_CASE( "monfactions_reciprocate", "[monster][monfactions]" )
{
    for( const monfaction &f : monfactions::get_all() ) {
        SECTION( f.id.str() ) {
            for( const monfaction &f1 : monfactions::get_all() ) {
                mf_attitude att = f.attitude( f1.id );
                mf_attitude rev_att = f1.attitude( f.id );

                INFO( f1.id.str() );
                REQUIRE( att != MFA_SIZE );
                REQUIRE( rev_att != MFA_SIZE );

                if( att == MFA_FRIENDLY || att == MFA_NEUTRAL ) {
                    CHECK_FALSE( rev_att == MFA_BY_MOOD );
                    CHECK_FALSE( rev_att == MFA_HATE );

                    if( ( rev_att != MFA_FRIENDLY ) && ( rev_att != MFA_NEUTRAL ) ) {
                        std::string att_str = att_enum_to_string( att );
                        std::string rev_att_str = att_enum_to_string( rev_att );
                        printf( "\n%s has an attitude of %s to %s, but %s has an attitude of %s to %s."
                                "\nEither %s should not be FRIENDLY/NEUTRAL to %s, or"
                                "\n%s should be FRIENDLY/NEUTRAL to %s\n\n",
                                f.id.c_str(), att_str.c_str(), f1.id.c_str(), f1.id.c_str(), rev_att_str.c_str(), f.id.c_str(),
                                f.id.c_str(), f1.id.c_str(), f1.id.c_str(), f.id.c_str() );

                    }
                }
            }
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
        REQUIRE( monfaction_small_animal->base_faction == monfaction_animal );
        REQUIRE( monfaction_tiny_animal->base_faction == monfaction_small_animal );
        REQUIRE( monfaction_fish->base_faction == monfaction_animal );
        REQUIRE( monfaction_bear->base_faction == monfaction_animal );

        INFO( "fish is a child of animal, is friendly to itself" );
        CHECK( attitude( "animal", "animal" ) == MFA_BY_MOOD );

        INFO( "default attitude towards self takes precedence over inheritance from parents" );
        CHECK( attitude( "animal", "fish" ) == MFA_NEUTRAL );
        CHECK( attitude( "fish", "fish" ) == MFA_FRIENDLY );

        INFO( "fish is inherited from animal and should be neutral toward small_animal" );
        CHECK( attitude( "fish", "small_animal" ) == MFA_NEUTRAL );

        INFO( "dog is inherited from animal, but hates small animals, of which tiny_animal is a child" );
        CHECK( attitude( "dog", "tiny_animal" ) == MFA_HATE );
        CHECK( attitude( "dog", "fish" ) == MFA_NEUTRAL );

    }

    SECTION( "some random samples" ) {
        CHECK( attitude( "aquatic_predator", "fish" ) == MFA_HATE );
        CHECK( attitude( "robofac", "zombie" ) == MFA_HATE );

        CHECK( attitude( "dragonfly", "defense_bot" ) == MFA_NEUTRAL );
        CHECK( attitude( "dragonfly", "dermatik" ) == MFA_HATE );

        CHECK( attitude( "zombie_aquatic", "zombie" ) == MFA_FRIENDLY );
        CHECK( attitude( "zombie", "zombie_aquatic" ) == MFA_FRIENDLY );
        CHECK( attitude( "zombie", "small_animal" ) == MFA_NEUTRAL );

        CHECK( attitude( "plant", "triffid" ) == MFA_FRIENDLY );
        CHECK( attitude( "plant", "utility_bot" ) == MFA_NEUTRAL );

        CHECK( attitude( "bee", "military" ) == MFA_NEUTRAL );

        CHECK( attitude( "bear", "bee" ) == MFA_HATE );
        CHECK( attitude( "wolf", "pig" ) == MFA_HATE );
        CHECK( attitude( "small_animal", "zombie" ) == MFA_NEUTRAL );
    }
}

TEST_CASE( "monfaction_extend", "[monster][monfactions]" )
{
    const monfaction &orig = monfaction_test_monfaction1.obj();
    const monfaction &extn = monfaction_test_monfaction_extend.obj();

    // check that player was extended and ant was deleted
    CHECK( orig.attitude( monfaction_test_monfaction7 ) == MFA_BY_MOOD );
    CHECK( extn.attitude( monfaction_test_monfaction7 ) == MFA_FRIENDLY );

    CHECK( orig.attitude( monfaction_test_monfaction3 ) == MFA_NEUTRAL );
    CHECK( extn.attitude( monfaction_test_monfaction3 ) == MFA_BY_MOOD );

    // check that other attitudes are preserved
    CHECK( orig.attitude( monfaction_test_monfaction2 ) == MFA_FRIENDLY );
    CHECK( extn.attitude( monfaction_test_monfaction2 ) == MFA_FRIENDLY );

    CHECK( orig.attitude( monfaction_test_monfaction5 ) == MFA_BY_MOOD );
    CHECK( extn.attitude( monfaction_test_monfaction5 ) == MFA_BY_MOOD );
}
