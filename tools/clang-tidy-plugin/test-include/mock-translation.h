// check_clang_tidy uses -nostdinc++, so we add dummy declaration of std::string here
namespace std
{
template<class CharT, class Traits = void, class Allocator = void>
class basic_string
{
    public:
        basic_string();
        basic_string( const CharT * );
        CharT *c_str();
        const CharT *c_str() const;
};
using string = basic_string<char>;
string operator+( const string &, const string & );
} // namespace std

// check_clang_tidy uses -nostdinc++, so we add dummy translation interface here instead of including translations.h
namespace detail
{

template<typename T>
struct local_translation_cache {
    T operator()( T arg );
};

static inline local_translation_cache<const char *> get_local_translation_cache( const char * )
{
    return local_translation_cache<const char *>();
}
static inline local_translation_cache<std::string> get_local_translation_cache(
    const std::string & )
{
    return local_translation_cache<std::string>();
}

} // namespace detail

template<typename T>
inline const T &translation_argument_identity( const T &t )
{
    return t;
}

#define _( msg ) \
    ( ( []( const auto & arg ) { \
        static auto cache = detail::get_local_translation_cache( arg ); \
        return cache( arg ); \
    } )( translation_argument_identity( msg ) ) )

std::string gettext( const std::string & );
std::string pgettext( const std::string &, const std::string & );
std::string n_gettext( const std::string &, const std::string &, int );
std::string npgettext( const std::string &, const std::string &, const std::string &, int );

#define translate_marker( s ) ( s )
#define translate_marker_context( c, s ) ( s )

class translation
{
    public:
        std::string translated( int num = 1 ) const;
        bool translated_lt( const translation &that ) const; // <
        bool translated_gt( const translation &that ) const; // >
        bool translated_le( const translation &that ) const; // <=
        bool translated_ge( const translation &that ) const; // >=
        bool translated_eq( const translation &that ) const; // ==
        bool translated_ne( const translation &that ) const; // !=

        static translation to_translation( const std::string & );
        static translation to_translation( const std::string &, const std::string & );
        static translation pl_translation( const std::string &, const std::string & );
        static translation pl_translation( const std::string &, const std::string &, const std::string & );
        static translation no_translation( const std::string & );
};

class translated_less
{
    public:
        bool operator()( const translation &lhs, const translation &rhs ) const;
};

class translated_greater
{
    public:
        bool operator()( const translation &lhs, const translation &rhs ) const;
};

class translated_less_equal
{
    public:
        bool operator()( const translation &lhs, const translation &rhs ) const;
};

class translated_greater_equal
{
    public:
        bool operator()( const translation &lhs, const translation &rhs ) const;
};

class translated_equal_to
{
    public:
        bool operator()( const translation &lhs, const translation &rhs ) const;
};

class translated_not_equal_to
{
    public:
        bool operator()( const translation &lhs, const translation &rhs ) const;
};

translation to_translation( const std::string & );
translation to_translation( const std::string &, const std::string & );
translation pl_translation( const std::string &, const std::string & );
translation pl_translation( const std::string &, const std::string &, const std::string & );
translation no_translation( const std::string & );
