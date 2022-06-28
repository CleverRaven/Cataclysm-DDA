#include <sstream>

#include "cata_catch.h"
#include "distribution.h"

TEST_CASE( "poisson_distribution", "[distribution]" )
{
    std::string s = R"({ "poisson": 10 })";
    std::istringstream is( s );
    JsonIn jin( is );
    int_distribution d;
    d.deserialize( jin.get_value() );

    CHECK( d.description() == "Poisson(10)" );
    CHECK( d.minimum() == 0 );
}
