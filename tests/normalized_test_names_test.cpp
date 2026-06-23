#include <memory>
#include <set>
#include <string>
#include <vector>

#include "cata_catch.h"

// Wraps Catch2 internal test-enumeration API in one place.
static std::vector<std::string> get_all_test_case_names()
{
    std::vector<std::string> names;
    for( const Catch::TestCase &tc :
         Catch::getAllTestCasesSorted( *Catch::getCurrentContext().getConfig() ) ) {
        names.push_back( tc.getTestCaseInfo().name );
    }
    return names;
}

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

    for( const std::string &test_case_name : get_all_test_case_names() ) {
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
