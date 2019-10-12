// RUN: %check_clang_tidy %s cata-text-style %t -- -plugins=%cata_plugin -config="{CheckOptions: [{key: cata-text-style.EscapeUnicode, value: 1}]}" --

// check_clang_tidy uses -nostdinc++, so we add dummy declaration of std::string here
namespace std
{
template<class CharT, class Traits = void, class Allocator = void>
class basic_string
{
    private:
        using This = basic_string<CharT, Traits, Allocator>;
    public:
        basic_string();
        basic_string( const CharT * );
        CharT *c_str();
        const CharT *c_str() const;
        This &operator+=( const This & );
};
using string = basic_string<char>;
string operator+( const string &, const string & );
} // namespace std

class some_stream
{
    public:
        some_stream &operator<<( const std::string &s );
};

template<typename T>
void foo( const T &t );

#define stringify( x ) #x

static void bar()
{
    foo( "." );
    // work around encoding error in llvm's lit by using escape sequence for unicode ellipses
    foo( "\u2026" );
    foo( "Blah." );
    foo( "Dark days ahead;" );
    foo( "What?!?" );
    foo( "Yes\u2026" );
    foo( "C.R.I.T." );
    foo( "C. D. D. A." );
    foo( "Periods (.) should be followed by double spaces." );
    foo( "foo::bar" );
    // frequently used in the code for concatenation
    foo( "Note: " );
    // do not check macros
    foo( stringify( foo ) ",  bar." );
    // ignore wide/u16/u32 strings
    foo( L"Foo,  bar." );
    foo( u"Foo,  bar." );
    foo( U"Foo,  bar." );
    // ignore string used in concatenation
    foo( std::string( "foo, " ) + "bar." );
    std::string str;
    str += "foo, ";
    str += "bar.";
    some_stream() << "foo, " << "bar.";

    // do not fix to avoid removing the prefix
    foo( "Foobar. " u8" \nBaz." );
    // CHECK-MESSAGES: [[@LINE-1]]:18: warning: unnecessary spaces at end of line.
    // CHECK-FIXES: foo( "Foobar. " u8" \nBaz." );
    // do not fix to avoid removing the prefix
    foo( "Foobar. " R"( )" "\n" );
    // CHECK-MESSAGES: [[@LINE-1]]:18: warning: unnecessary spaces at end of line.
    // CHECK-FIXES: foo( "Foobar. " R"( )" "\n" );

    foo( "\t" );
    // CHECK-MESSAGES: [[@LINE-1]]:11: warning: spaces preferred over tab.
    // CHECK-FIXES: foo( "    " );
    foo( "foo\r\nbar" );
    // CHECK-MESSAGES: [[@LINE-1]]:14: warning: new line preferred over carriage return.
    // CHECK-FIXES: foo( "foo\nbar" );
    foo( "foo\rbar" );
    // CHECK-MESSAGES: [[@LINE-1]]:14: warning: new line preferred over carriage return.
    // CHECK-FIXES: foo( "foo\nbar" );
    foo( "..." );
    // CHECK-MESSAGES: [[@LINE-1]]:11: warning: ellipsis preferred over three dots.
    // CHECK-FIXES: foo( "\u2026" );
    foo( "Three.  \nTwo.  One." );
    // CHECK-MESSAGES: [[@LINE-1]]:17: warning: unnecessary spaces at end of line.
    // CHECK-FIXES: foo( "Three.\nTwo.  One." );
    foo( "Three. Two.  One." );
    // CHECK-MESSAGES: [[@LINE-1]]:17: warning: insufficient spaces at this location.  2 required, but only 1 found.
    // CHECK-FIXES: foo( "Three.  Two.  One." );
    foo( "Three.   Two.  One." );
    // CHECK-MESSAGES: [[@LINE-1]]:17: warning: excessive spaces at this location.  2 required, but 3 found.
    // CHECK-FIXES: foo( "Three.  Two.  One." );
    foo( "Three,  Two, One." );
    // CHECK-MESSAGES: [[@LINE-1]]:17: warning: excessive spaces at this location.  1 required, but 2 found.
    // CHECK-FIXES: foo( "Three, Two, One." );
    foo( "Three! Two!  One!" );
    // CHECK-MESSAGES: [[@LINE-1]]:17: warning: insufficient spaces at this location.  2 required, but only 1 found.
    // CHECK-FIXES: foo( "Three!  Two!  One!" );
    foo( "Three; Two;  One;" );
    // CHECK-MESSAGES: [[@LINE-1]]:22: warning: excessive spaces at this location.  1 required, but 2 found.
    // CHECK-FIXES: foo( "Three; Two; One;" );
    foo( "Three? Two?  One?" );
    // CHECK-MESSAGES: [[@LINE-1]]:17: warning: insufficient spaces at this location.  2 required, but only 1 found.
    // CHECK-FIXES: foo( "Three?  Two?  One?" );
    foo( "Three?! Two!?  One!!" );
    // CHECK-MESSAGES: [[@LINE-1]]:18: warning: insufficient spaces at this location.  2 required, but only 1 found.
    // CHECK-FIXES: foo( "Three?!  Two!?  One!!" );
    foo( "\u2026 foo." );
    // CHECK-MESSAGES: [[@LINE-1]]:17: warning: undesired spaces after a punctuation that starts a string.
    // CHECK-FIXES: foo( "\u2026foo." );
    foo( "foo.\n\u2026  bar." );
    // CHECK-MESSAGES: [[@LINE-1]]:23: warning: undesired spaces after a punctuation that starts a line.
    // CHECK-FIXES: foo( "foo.\n\u2026bar." );
}
