// RUN: %check_clang_tidy %s cata-no-static-gettext %t -- -plugins=%cata_plugin --

// check_clang_tidy uses -nostdinc++, so we add dummy declaration of std::string here
namespace std
{
template<class CharT, class Traits = void, class Allocator = void>
class basic_string
{
    public:
        basic_string( const CharT * );
};
using string = basic_string<char>;
} // namespace std

// check_clang_tidy uses -nostdinc++, so we add dummy declarations of gettext functions here instead of including translations.h
const char *_( const char *const );
const char *gettext( const char *const );
const char *pgettext( const char *const, const char *const );
const char *ngettext( const char *const, const char *const, int );
const char *npgettext( const char *const, const char *const, const char *const, int );

class foo
{
    public:
        foo( const char *, const char * );
};

// ok, doesn't contain gettext calls
const std::string global_str_0 = "global_str_0";

const std::string global_str_1 = _( "global_str_1" );
// CHECK-MESSAGES: warning: Gettext calls in static variable initialization will cause text to be untranslated (global static) or not updated when switching language (local static). Consider using translation objects (to_translation()) or translate_marker(), and translate the text on demand (with translation::translated() or gettext calls outside static vars)

const std::string global_str_2{ _( "global_str_2" ) };
// CHECK-MESSAGES: warning: Gettext calls in static variable initialization will cause text to be untranslated (global static) or not updated when switching language (local static). Consider using translation objects (to_translation()) or translate_marker(), and translate the text on demand (with translation::translated() or gettext calls outside static vars)

static const std::string global_static_str = _( "global_static_str" );
// CHECK-MESSAGES: warning: Gettext calls in static variable initialization will cause text to be untranslated (global static) or not updated when switching language (local static). Consider using translation objects (to_translation()) or translate_marker(), and translate the text on demand (with translation::translated() or gettext calls outside static vars)

const std::string global_gettext_str = gettext( "global_gettext_str" );
// CHECK-MESSAGES: warning: Gettext calls in static variable initialization will cause text to be untranslated (global static) or not updated when switching language (local static). Consider using translation objects (to_translation()) or translate_marker(), and translate the text on demand (with translation::translated() or gettext calls outside static vars)

const std::string global_pgettext_str = pgettext( "ctxt", "global_pgettext_str" );
// CHECK-MESSAGES: warning: Gettext calls in static variable initialization will cause text to be untranslated (global static) or not updated when switching language (local static). Consider using translation objects (to_translation()) or translate_marker(), and translate the text on demand (with translation::translated() or gettext calls outside static vars)

const std::string global_ngettext_str = ngettext( "global_ngettext_str", "global_ngettext_strs",
                                        1 );
// CHECK-MESSAGES: warning: Gettext calls in static variable initialization will cause text to be untranslated (global static) or not updated when switching language (local static). Consider using translation objects (to_translation()) or translate_marker(), and translate the text on demand (with translation::translated() or gettext calls outside static vars)

const std::string global_npgettext_str = npgettext( "ctxt", "global_npgettext_str",
        "global_npgettext_strs", 1 );
// CHECK-MESSAGES: warning: Gettext calls in static variable initialization will cause text to be untranslated (global static) or not updated when switching language (local static). Consider using translation objects (to_translation()) or translate_marker(), and translate the text on demand (with translation::translated() or gettext calls outside static vars)

static const char *const global_static_cstr = _( "global_static_cstr" );
// CHECK-MESSAGES: warning: Gettext calls in static variable initialization will cause text to be untranslated (global static) or not updated when switching language (local static). Consider using translation objects (to_translation()) or translate_marker(), and translate the text on demand (with translation::translated() or gettext calls outside static vars)

static const foo global_static_foo {
    _( "global_static_foo" ),
    // CHECK-MESSAGES: warning: Gettext calls in static variable initialization will cause text to be untranslated (global static) or not updated when switching language (local static). Consider using translation objects (to_translation()) or translate_marker(), and translate the text on demand (with translation::translated() or gettext calls outside static vars)
    _( "global_static_foo" )
    // CHECK-MESSAGES: warning: Gettext calls in static variable initialization will cause text to be untranslated (global static) or not updated when switching language (local static). Consider using translation objects (to_translation()) or translate_marker(), and translate the text on demand (with translation::translated() or gettext calls outside static vars)
};

namespace cata
{
static const std::string namespace_static_str = _( "namespace_static_str" );
// CHECK-MESSAGES: warning: Gettext calls in static variable initialization will cause text to be untranslated (global static) or not updated when switching language (local static). Consider using translation objects (to_translation()) or translate_marker(), and translate the text on demand (with translation::translated() or gettext calls outside static vars)

const std::string namespace_str = _( "namespace_str" );
// CHECK-MESSAGES: warning: Gettext calls in static variable initialization will cause text to be untranslated (global static) or not updated when switching language (local static). Consider using translation objects (to_translation()) or translate_marker(), and translate the text on demand (with translation::translated() or gettext calls outside static vars)
} // namespace cata

static void local()
{
    // ok, non-static
    const std::string ok_str_1 = _( "ok_str_1" );

    static const std::string local_str_1 = _( "local_str_1" );
    // CHECK-MESSAGES: warning: Gettext calls in static variable initialization will cause text to be untranslated (global static) or not updated when switching language (local static). Consider using translation objects (to_translation()) or translate_marker(), and translate the text on demand (with translation::translated() or gettext calls outside static vars)
}

class bar
{
        static const std::string class_static_str0;
        static const std::string class_static_str1;
};

// ok, no gettext call
const std::string bar::class_static_str0 = "class_static_str0";

const std::string bar::class_static_str1 = _( "class_static_str1" );
// CHECK-MESSAGES: warning: Gettext calls in static variable initialization will cause text to be untranslated (global static) or not updated when switching language (local static). Consider using translation objects (to_translation()) or translate_marker(), and translate the text on demand (with translation::translated() or gettext calls outside static vars)
