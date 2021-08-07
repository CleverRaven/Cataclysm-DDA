#include "cata_catch.h"
#include <iomanip>
#include <sstream>
#include <string>

TEST_CASE( "floating_point_arithmetic" )
{
    volatile int a = 45000;
    volatile float b = 1.01f;
    float c = a * b;
    std::ostringstream oss;
    oss.precision( 20 );
    oss << "b = " << std::fixed << b << std::endl;
    oss << "c = " << std::fixed << c << std::endl;
    INFO( oss.str() );
    int d = static_cast<int>( c );
    CHECK( d == 45450 );
}
