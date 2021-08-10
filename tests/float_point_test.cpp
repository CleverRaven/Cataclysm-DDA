#include "cata_catch.h"
#include <iomanip>
#include <sstream>
#include <string>

TEST_CASE( "floating_point_arithmetic" )
{
    volatile int a = 45000000;
    volatile float b = 1.01f;
    volatile long long c = 1000ULL;
    INFO( "type of \"a*b/c\" is " << typeid( a * b / c ).name() );
    float d = a * b / c;
    std::ostringstream oss;
    oss.precision( 20 );
    oss << "b = " << std::fixed << b << std::endl;
    oss << "d = " << std::fixed << d << std::endl;
    INFO( oss.str() );
    int e = static_cast<int>( d );
#if defined(_WIN32)
    CHECK( e == 42 );
#else
    CHECK( e == 45450 );
#endif
}
