#include "catch/catch.hpp"

#include <sstream>

#include "json.h"
#include "poly_serialized.h"
#include "string_formatter.h"

class has_type
{
    public:
        virtual ~has_type() = default;

        virtual void serialize( JsonOut &jsout ) const = 0;
        virtual void deserialize( JsonIn &jsin ) = 0;
        virtual std::string get_type() const = 0;

        static has_type *create( std::string type );
};

class has_int : public has_type
{
    public:
        int a;

        void serialize( JsonOut &jsout ) const override {
            jsout.member( "a", a );
        }
        void deserialize( JsonIn &jsin ) override {
            a = JsonObject( jsin ).get_int( "a" );
        }
        std::string get_type() const override {
            return "has_int";
        }
};

class has_float : public has_type
{
    public:
        float b;

        void serialize( JsonOut &jsout ) const override {
            jsout.member( "b", b );
        }
        void deserialize( JsonIn &jsin ) override {
            b = JsonObject( jsin ).get_float( "b" );
        }
        std::string get_type() const override {
            return "has_float";
        }
};

has_type *has_type::create( std::string type )
{
    if( type == "has_int" ) {
        return new has_int();
    } else if( type == "has_float" ) {
        return new has_float();
    } else {
        throw "Wrong type";
    }
}

template<typename T>
void test_serialization( const T &val, const std::string &s )
{
    CAPTURE( val );
    {
        INFO( "test_serialization" );
        std::ostringstream os;
        JsonOut jsout( os );
        jsout.write( val );
        CHECK( os.str() == s );
    }
    {
        INFO( "test_deserialization" );
        std::istringstream is( s );
        JsonIn jsin( is );
        T read_val;
        CHECK( jsin.read( read_val ) );
        CHECK( val->get_type() == read_val->get_type() );
    }
}


TEST_CASE( "Save+load creates poly_serialized with correct type and value", "[polymorphism]" )
{
    SECTION( "has_int" ) {
        auto ptr = cata::poly_serialized<has_type>( has_type::create( "has_int" ) );
        test_serialization( ptr, R"(["has_int",{"a":0}])" );
    }

    SECTION( "has_float" ) {
        auto ptr = cata::poly_serialized<has_type>( has_type::create( "has_float" ) );
        test_serialization( ptr, R"(["has_float",{"b":0.000000}])" );
    }
}
