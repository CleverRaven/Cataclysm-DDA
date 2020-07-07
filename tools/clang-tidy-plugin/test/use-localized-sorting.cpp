// RUN: %check_clang_tidy %s cata-use-localized-sorting %t -- -plugins=%cata_plugin -- -isystem %cata_include

namespace std
{

template<class T>
struct allocator;

template<class CharT>
class char_traits;

template <
    class CharT,
    class Traits = std::char_traits<CharT>,
    class Allocator = std::allocator<CharT>
    > class basic_string;

typedef basic_string<char> string;

template<class CharT, class Traits, class Alloc>
bool operator<( const std::basic_string<CharT, Traits, Alloc> &lhs,
                const std::basic_string<CharT, Traits, Alloc> &rhs ) noexcept;

template<class RandomIt>
void sort( RandomIt first, RandomIt last );

template<class T1, class T2>
struct pair;

template< class T1, class T2 >
bool operator<( const std::pair<T1, T2> &lhs, const std::pair<T1, T2> &rhs );

template< class... Types >
class tuple;

template<class... TTypes, class... UTypes>
bool operator<( const tuple<TTypes...> &lhs,
                const tuple<UTypes...> &rhs );
}

template<class T>
struct iterator {
    using value_type = T;
};

class NonString
{
};

bool operator<( const NonString &, const NonString &rhs ) noexcept;

bool f0( const std::string &l, const std::string &r )
{
    return l < r;
    // CHECK-MESSAGES: warning: Raw comparison of 'const std::string' (aka 'const basic_string<char>').  For UI purposes please use localized_compare from translations.h. [cata-use-localized-sorting]
}

bool f1( const NonString &l, const NonString &r )
{
    return l < r;
}

bool f2( const std::pair<int, std::string> &l, const std::pair<int, std::string> &r )
{
    return l < r;
    // CHECK-MESSAGES: warning: Raw comparison of 'const std::pair<int, std::string>' (aka 'const pair<int, basic_string<char> >'). For UI purposes please use localized_compare from translations.h. [cata-use-localized-sorting]
}

bool f3( const std::pair<int, NonString> &l, const std::pair<int, NonString> &r )
{
    return l < r;
}

bool f4( const std::tuple<int, std::string> &l, const std::tuple<int, std::string> &r )
{
    return l < r;
    // CHECK-MESSAGES: warning: Raw comparison of 'const std::tuple<int, std::string>' (aka 'const tuple<int, basic_string<char> >').  For UI purposes please use localized_compare from translations.h. [cata-use-localized-sorting]
}

bool f5( const std::tuple<int, NonString> &l, const std::tuple<int, NonString> &r )
{
    return l < r;
}

bool f4( const std::tuple<std::tuple<std::string>> &l,
         const std::tuple<std::tuple<std::string>> &r )
{
    return l < r;
    // CHECK-MESSAGES: warning: Raw comparison of 'const std::tuple<std::tuple<std::string> >' (aka 'const tuple<tuple<basic_string<char> > >'). For UI purposes please use localized_compare from translations.h. [cata-use-localized-sorting]
}

bool sort0( const std::string *start, const std::string *end )
{
    std::sort( start, end );
    // CHECK-MESSAGES: warning: Raw sort of 'const std::string' (aka 'const basic_string<char>').  For UI purposes please use localized_compare from translations.h. [cata-use-localized-sorting]
}

bool sort1( const NonString *start, const NonString *end )
{
    std::sort( start, end );
}

bool sortit0( iterator<std::string> start, iterator<std::string> end )
{
    std::sort( start, end );
    // CHECK-MESSAGES: warning: Raw sort of 'std::basic_string<char, std::char_traits<char>, std::allocator<char> >'.  For UI purposes please use localized_compare from translations.h. [cata-use-localized-sorting]
}

bool sortit1( iterator<NonString> start, iterator<NonString> end )
{
    std::sort( start, end );
}
