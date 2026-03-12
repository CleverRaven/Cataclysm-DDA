#include <memory>
#include <set>
#include <string>
#include <vector>

#include "cata_catch.h"

TEST_CASE( "enforce_normalized_test_cases" )
{
    const static std::string allowed_chars =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789"
        "_-+/";

    const std::set<char> allowed_set( allowed_chars.begin(), allowed_chars.end() );
    INFO( "Prefer simple characters for TEST_CASE names to avoid need for escaping" );
    CAPTURE( allowed_chars );

    for( const Catch::TestCase &tc :
         Catch::getAllTestCasesSorted( *Catch::getCurrentContext().getConfig() ) ) {
        const std::string &test_case_name = tc.name;
        CAPTURE( test_case_name );
        int i = 0;
        for( ; i < static_cast<int>( test_case_name.size() ); i++ ) {
            if( allowed_set.count( test_case_name[i] ) <= 0 ) {
                break;
            }
        }
        const char invalid_char = test_case_name[i];
        CAPTURE( invalid_char );
        CHECK( i == static_cast<int>( test_case_name.size() ) );
    }
}
