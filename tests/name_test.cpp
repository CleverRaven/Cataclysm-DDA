#include <set>
#include <string>

#include "catch/catch.hpp"
#include "name.h"

class IsOneOf : public Catch::MatcherBase<std::string>
{
        std::set< std::string > values;
    public:
        IsOneOf( std::set< std::string > v ): values{v} {}
        virtual bool match( std::string const &s ) const override {
            return values.count( s ) > 0;
        }
        virtual std::string describe() const override {
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
        Name::clear();
        Name::load_from_file( "tests/data/name.json" );
        WHEN( "Getting a town name" ) {
            auto name = Name::get( nameIsTownName );
            CHECK( name == "City" );
        }
        WHEN( "Getting a world name" ) {
            auto name = Name::get( nameIsWorldName );
            CHECK( name == "World" );
        }
        WHEN( "Getting a male given name" ) {
            for( int i = 0; i < 8; ++i ) {
                auto name = Name::get( nameIsMaleName | nameIsGivenName );
                CHECK_THAT( name, IsOneOf( {"Male", "Unisex"} ) );
            }
        }
        WHEN( "Getting a female given name" ) {
            for( int i = 0; i < 8; ++i ) {
                auto name = Name::get( nameIsFemaleName | nameIsGivenName );
                CHECK_THAT( name, IsOneOf( {"Female", "Unisex"} ) );
            }
        }
        WHEN( "Getting a family name" ) {
            auto name = Name::get( nameIsFamilyName );
            CHECK( name == "Family" );
        }
        WHEN( "Getting a male backer name" ) {
            for( int i = 0; i < 8; ++i ) {
                auto name = Name::get( nameIsMaleName | nameIsFullName );
                CHECK_THAT( name, IsOneOf( {"Male Backer", "Unisex Backer"} ) );
            }
        }
        WHEN( "Getting a female backer name" ) {
            for( int i = 0; i < 8; ++i ) {
                auto name = Name::get( nameIsFemaleName | nameIsFullName );
                CHECK_THAT( name, IsOneOf( {"Female Backer", "Unisex Backer"} ) );
            }
        }
        WHEN( "Getting an invalid name" ) {
            auto name = Name::get( nameIsUnisexName | nameIsTownName );
            CHECK( name == "Tom" );
        }
        WHEN( "Generating a male name" ) {
            for( int i = 0; i < 16; ++i ) {
                auto name = Name::generate( true );
                CHECK_THAT( name, IsOneOf( {"Male Backer", "Unisex Backer", "Male Family", "Unisex Family"} ) );
            }
        }
        WHEN( "Generating a female name" ) {
            for( int i = 0; i < 16; ++i ) {
                auto name = Name::generate( false );
                CHECK_THAT( name, IsOneOf( {"Female Backer", "Unisex Backer", "Female Family", "Unisex Family"} ) );
            }
        }
    }
}

