// RUN: %check_clang_tidy %s cata-translator-comments %t -- -plugins=%cata_plugin --

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
} // namespace std

// check_clang_tidy uses -nostdinc++, so we add dummy translation interface here instead of including translations.h
#define translate_marker( s ) ( s )
#define translate_marker_context( c, s ) ( s )

const char *_( const char * );
const char *gettext( const char * );
const char *pgettext( const char *, const char * );
const char *ngettext( const char *, const char *, int );
const char *npgettext( const char *, const char *, const char *, int );

class translation
{
    public:
        static translation to_translation( const std::string & );
        static translation to_translation( const std::string &, const std::string & );
        static translation pl_translation( const std::string &, const std::string & );
        static translation pl_translation( const std::string &, const std::string &, const std::string & );
        static translation no_translation( const std::string & );
};

translation to_translation( const std::string & );
translation to_translation( const std::string &, const std::string & );
translation pl_translation( const std::string &, const std::string & );
translation pl_translation( const std::string &, const std::string &, const std::string & );
translation no_translation( const std::string & );

void foo()
{
    // translation markers only accepts string literals as arguments
    static_cast<void>( translate_marker( "bar" ) );
    static_cast<void>( translate_marker( R"(foo)" "bar" ) );
    static_cast<void>( translate_marker_context( "foo", "bar" ) );

    static_cast<void>( translate_marker( ( "bar" ) ) );
    // CHECK-MESSAGES: [[@LINE-1]]:42: warning: Translation marker macros only accepts string literal arguments
    static_cast<void>( translate_marker_context( "foo", ( "bar" ) ) );
    // CHECK-MESSAGES: [[@LINE-1]]:57: warning: Translation marker macros only accepts string literal arguments
#define pass_through( x ) x
    static_cast<void>( translate_marker( pass_through( "bar" ) ) );
    // CHECK-MESSAGES: [[@LINE-1]]:42: warning: Translation marker macros only accepts string literal arguments
    static_cast<void>( translate_marker_context(, "bar" ) );
    // CHECK-MESSAGES: [[@LINE-1]]:49: warning: Empty argument to a translation marker macro

    // valid translator comments

    // ~xgettext will treat this as a translator comment, but we ignore it
    // to allow using the tilde (~) inside normal comments

    //~ bar
    _( "bar" );

    ngettext( /*~ foo */ ( "bar" ), _( "baz" ), 0 );

    //~ bar
    gettext( "bar" );

    //~bar
    ngettext( "bar", "baz", 1 );

    /*~ bar */ to_translation( "bar" );

    /*~ bar */
    pl_translation( "bar", "baz" );

    _( /*~ bar */ "bar" );

    /*~
    bar*/
    _( "bar" );

    //~ bar
    pgettext( "foo", "bar" );

    //~ bark
    static_cast<void>( translate_marker( "bar" ) );

    static_cast<void>( translate_marker_context( "foo", /*~ bard */ "bar" ) );

    // misplaced translator comments

    //~ barbarian
    const char *bar = "bar";
    static_cast<void>( bar );
    _( "bar" );
    // CHECK-MESSAGES: [[@LINE-4]]:5: warning: Translator comment without a matching raw string

    _( "bar" );
    //~ barbara
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: Translator comment without a matching raw string

    _( "bar" /*~ banana */ );
    // CHECK-MESSAGES: [[@LINE-1]]:14: warning: Translator comment without a matching raw string

    //~ barb
    _( ( "bar" ) );
    // CHECK-MESSAGES: [[@LINE-2]]:5: warning: Translator comment without a matching raw string

    //~ barf
    pgettext( ( "foo" ), "bar" );
    // CHECK-MESSAGES: [[@LINE-2]]:5: warning: Translator comment without a matching raw string

    /*~
    barnacle */
    npgettext( "foo",
               "bar", "baz", 2 );
    // CHECK-MESSAGES: [[@LINE-4]]:5: warning: Translator comment without a matching raw string

    //~ barn
    npgettext( "foo", "bar", ( "baz" ), 3 );
    // CHECK-MESSAGES: [[@LINE-2]]:5: warning: Translator comment without a matching raw string

    //~ baron
    to_translation( "foo", { "bar" } );
    // CHECK-MESSAGES: [[@LINE-2]]:5: warning: Translator comment without a matching raw string

    //~ barricade
    to_translation( "foo", ( "bar" ) );
    // CHECK-MESSAGES: [[@LINE-2]]:5: warning: Translator comment without a matching raw string

    //~ bartender
    to_translation( "foo", std::string( "bar" ) );
    // CHECK-MESSAGES: [[@LINE-2]]:5: warning: Translator comment without a matching raw string

    //~ barbeque
    to_translation( "foo",
                    "bar" );
    // CHECK-MESSAGES: [[@LINE-3]]:5: warning: Translator comment without a matching raw string
}

//~ barometer
// CHECK-MESSAGES: [[@LINE-1]]:1: warning: Translator comment without a matching raw string
