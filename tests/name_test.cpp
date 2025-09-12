#include <set>
#include <string>

#include "cata_catch.h"
#include "cata_path.h"
#include "path_info.h"
#include "text_snippets.h"

class IsOneOf : public Catch::MatcherBase<std::string>
{
        std::set< std::string > values;
    public:
        explicit IsOneOf( const std::set< std::string > &v ): values{v} {}
        bool match( std::string const &s ) const override {
            return values.count( s ) > 0;
        }
        std::string describe() const override {
            std::string s = "is one of {";
            for( auto const &i : values ) {
                s += i + ", ";
            }
            s.back() = '}';
            return s;
        }
};

TEST_CASE( "name_generation", "[name]" )
{
    GIVEN( "Names loaded from tests/data/name.json" ) {
        SNIPPET.reload_names( PATH_INFO::base_path() / "tests" / "data" / "name.json" );
        WHEN( "Getting a town name" ) {
            std::string name = SNIPPET.expand( "<city_name>" );
            CHECK( name == "City" );
        }
        WHEN( "Getting a world name" ) {
            std::string name = SNIPPET.expand( "<world_name>" );
            CHECK( name == "World" );
        }
        WHEN( "Getting a nick name" ) {
            std::string name = SNIPPET.expand( "<nick_name>" );
            CHECK( name == "Nick" );
        }
        WHEN( "Getting a male given name" ) {
            for( int i = 0; i < 8; ++i ) {
                std::string name = SNIPPET.expand( "<male_given_name>" );
                CHECK_THAT( name, IsOneOf( {"Male", "Unisex"} ) );
            }
        }
        WHEN( "Getting a female given name" ) {
            for( int i = 0; i < 8; ++i ) {
                std::string name = SNIPPET.expand( "<female_given_name>" );
                CHECK_THAT( name, IsOneOf( {"Female", "Unisex"} ) );
            }
        }
        WHEN( "Getting a family name" ) {
            std::string name = SNIPPET.expand( "<family_name>" );
            CHECK( name == "Family" );
        }
        WHEN( "Getting a male backer name" ) {
            for( int i = 0; i < 8; ++i ) {
                std::string name = SNIPPET.expand( "<male_backer_name>" );
                CHECK_THAT( name, IsOneOf( {"Male Backer", "Unisex Backer"} ) );
            }
        }
        WHEN( "Getting a female backer name" ) {
            for( int i = 0; i < 8; ++i ) {
                std::string name = SNIPPET.expand( "<female_backer_name>" );
                CHECK_THAT( name, IsOneOf( {"Female Backer", "Unisex Backer"} ) );
            }
        }
        WHEN( "Generating a male name" ) {
            for( int i = 0; i < 16; ++i ) {
                std::string name = SNIPPET.expand( "<male_full_name>" );
                CHECK_THAT( name, IsOneOf( {"Male Backer", "Unisex Backer", "Male Family", "Unisex Family", "Male 'Nick' Family", "Unisex 'Nick' Family"} ) );
            }
        }
        WHEN( "Generating a female name" ) {
            for( int i = 0; i < 16; ++i ) {
                std::string name = SNIPPET.expand( "<female_full_name>" );
                CHECK_THAT( name, IsOneOf( {"Female Backer", "Unisex Backer", "Female Family", "Unisex Family", "Female 'Nick' Family", "Unisex 'Nick' Family"} ) );
            }
        }
    }
}

