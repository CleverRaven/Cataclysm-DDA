#include <string>

#include "cata_catch.h"
#include "distribution.h"
#include "flexbuffer_json.h"
#include "json_loader.h"

TEST_CASE( "poisson_distribution", "[distribution]" )
{
    std::string s = R"({ "poisson": 10 })";
    JsonValue jin = json_loader::from_string( s );
    int_distribution d;
    d.deserialize( jin );

    CHECK( d.description() == "Poisson(10)" );
    CHECK( d.minimum() == 0 );
}
