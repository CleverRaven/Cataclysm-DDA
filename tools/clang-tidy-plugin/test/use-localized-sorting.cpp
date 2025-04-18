// RUN: %check_clang_tidy -allow-stdinc %s cata-use-localized-sorting %t -- --load=%cata_plugin -- -isystem %cata_include

#include <algorithm>
#include <string>
#include <tuple>
#include <vector>

class NonString
{
};

bool operator<( const NonString &, const NonString &rhs ) noexcept;

bool f0( const std::string &l, const std::string &r )
{
    return l < r;
    // CHECK-MESSAGES: warning: Raw comparison of 'const std::string' (aka '{{.*basic_string.*}}').  For UI purposes please use localized_compare from translations.h. [cata-use-localized-sorting]
}

bool f1( const NonString &l, const NonString &r )
{
    return l < r;
}

bool f2( const std::pair<int, std::string> &l, const std::pair<int, std::string> &r )
{
    return l < r;
    // CHECK-MESSAGES: warning: Raw comparison of 'const std::pair<int, std::string>' (aka '{{.*basic_string.*}}'). For UI purposes please use localized_compare from translations.h. [cata-use-localized-sorting]
}

bool f3( const std::pair<int, NonString> &l, const std::pair<int, NonString> &r )
{
    return l < r;
}

bool f4( const std::tuple<int, std::string> &l, const std::tuple<int, std::string> &r )
{
    return l < r;
    // CHECK-MESSAGES: warning: Raw comparison of 'const std::tuple<int, std::string>' (aka '{{.*basic_string.*}}').  For UI purposes please use localized_compare from translations.h. [cata-use-localized-sorting]
}

bool f5( const std::tuple<int, NonString> &l, const std::tuple<int, NonString> &r )
{
    return l < r;
}

bool f4( const std::tuple<std::tuple<std::string>> &l,
         const std::tuple<std::tuple<std::string>> &r )
{
    return l < r;
    // CHECK-MESSAGES: warning: Raw comparison of 'const std::tuple<std::tuple<std::string>>' (aka '{{.*basic_string.*}}'). For UI purposes please use localized_compare from translations.h. [cata-use-localized-sorting]
}

void sort0( std::string *start, std::string *end )
{
    std::sort( start, end );
    // CHECK-MESSAGES: warning: Raw sort of 'std::string' (aka '{{.*basic_string.*}}').  For UI purposes please use localized_compare from translations.h. [cata-use-localized-sorting]
}

void sort1( NonString *start, NonString *end )
{
    std::sort( start, end );
}

void sortit0( std::vector<std::string>::iterator start, std::vector<std::string>::iterator end )
{
    std::sort( start, end );
    // CHECK-MESSAGES: warning: Raw sort of 'typename {{.*}}' (aka 'std::{{.*string.*}}').  For UI purposes please use localized_compare from translations.h. [cata-use-localized-sorting]
}

void sortit1( std::vector<NonString>::iterator start, std::vector<NonString>::iterator end )
{
    std::sort( start, end );
}
