// RUN: %check_clang_tidy %s cata-no-static-gettext %t -- -plugins=%cata_plugin -- -I %test_include

#include "mock-translation.h"

class foo
{
    public:
        foo( const char *, const char * );
};

// ok, doesn't contain gettext calls
const std::string global_str_0 = "global_str_0";

const std::string global_str_1 = _( "global_str_1" );
// CHECK-MESSAGES: [[@LINE-1]]:34: warning: Gettext calls in static variable initialization will cause text to be untranslated (global static) or not updated when switching language (local static). Consider using translation objects (to_translation() or pl_translation()) or translate_marker(), and translate the text on demand (with translation::translated() or gettext calls outside static vars)

const std::string global_str_2{ _( "global_str_2" ) };
// CHECK-MESSAGES: [[@LINE-1]]:33: warning: Gettext calls in static variable initialization will cause text to be untranslated (global static) or not updated when switching language (local static). Consider using translation objects (to_translation() or pl_translation()) or translate_marker(), and translate the text on demand (with translation::translated() or gettext calls outside static vars)

static const std::string global_static_str = _( "global_static_str" );
// CHECK-MESSAGES: [[@LINE-1]]:46: warning: Gettext calls in static variable initialization will cause text to be untranslated (global static) or not updated when switching language (local static). Consider using translation objects (to_translation() or pl_translation()) or translate_marker(), and translate the text on demand (with translation::translated() or gettext calls outside static vars)

const std::string global_gettext_str = gettext( "global_gettext_str" );
// CHECK-MESSAGES: [[@LINE-1]]:40: warning: Gettext calls in static variable initialization will cause text to be untranslated (global static) or not updated when switching language (local static). Consider using translation objects (to_translation() or pl_translation()) or translate_marker(), and translate the text on demand (with translation::translated() or gettext calls outside static vars)

const std::string global_pgettext_str = pgettext( "ctxt", "global_pgettext_str" );
// CHECK-MESSAGES: [[@LINE-1]]:41: warning: Gettext calls in static variable initialization will cause text to be untranslated (global static) or not updated when switching language (local static). Consider using translation objects (to_translation() or pl_translation()) or translate_marker(), and translate the text on demand (with translation::translated() or gettext calls outside static vars)

const std::string global_ngettext_str = n_gettext( "global_ngettext_str", "global_ngettext_strs",
                                        1 );
// CHECK-MESSAGES: [[@LINE-2]]:41: warning: Gettext calls in static variable initialization will cause text to be untranslated (global static) or not updated when switching language (local static). Consider using translation objects (to_translation() or pl_translation()) or translate_marker(), and translate the text on demand (with translation::translated() or gettext calls outside static vars)

const std::string global_npgettext_str = npgettext( "ctxt", "global_npgettext_str",
        "global_npgettext_strs", 1 );
// CHECK-MESSAGES: [[@LINE-2]]:42: warning: Gettext calls in static variable initialization will cause text to be untranslated (global static) or not updated when switching language (local static). Consider using translation objects (to_translation() or pl_translation()) or translate_marker(), and translate the text on demand (with translation::translated() or gettext calls outside static vars)

static const char *const global_static_cstr = _( "global_static_cstr" );
// CHECK-MESSAGES: [[@LINE-1]]:47: warning: Gettext calls in static variable initialization will cause text to be untranslated (global static) or not updated when switching language (local static). Consider using translation objects (to_translation() or pl_translation()) or translate_marker(), and translate the text on demand (with translation::translated() or gettext calls outside static vars)

static const foo global_static_foo {
    _( "global_static_foo" ),
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: Gettext calls in static variable initialization will cause text to be untranslated (global static) or not updated when switching language (local static). Consider using translation objects (to_translation() or pl_translation()) or translate_marker(), and translate the text on demand (with translation::translated() or gettext calls outside static vars)
    _( "global_static_foo" )
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: Gettext calls in static variable initialization will cause text to be untranslated (global static) or not updated when switching language (local static). Consider using translation objects (to_translation() or pl_translation()) or translate_marker(), and translate the text on demand (with translation::translated() or gettext calls outside static vars)
};

namespace cata
{
static const std::string namespace_static_str = _( "namespace_static_str" );
// CHECK-MESSAGES: [[@LINE-1]]:49: warning: Gettext calls in static variable initialization will cause text to be untranslated (global static) or not updated when switching language (local static). Consider using translation objects (to_translation() or pl_translation()) or translate_marker(), and translate the text on demand (with translation::translated() or gettext calls outside static vars)

const std::string namespace_str = _( "namespace_str" );
// CHECK-MESSAGES: [[@LINE-1]]:35: warning: Gettext calls in static variable initialization will cause text to be untranslated (global static) or not updated when switching language (local static). Consider using translation objects (to_translation() or pl_translation()) or translate_marker(), and translate the text on demand (with translation::translated() or gettext calls outside static vars)
} // namespace cata

static void local()
{
    // ok, non-static
    const std::string ok_str_1 = _( "ok_str_1" );

    static const std::string local_str_1 = _( "local_str_1" );
    // CHECK-MESSAGES: [[@LINE-1]]:44: warning: Gettext calls in static variable initialization will cause text to be untranslated (global static) or not updated when switching language (local static). Consider using translation objects (to_translation() or pl_translation()) or translate_marker(), and translate the text on demand (with translation::translated() or gettext calls outside static vars)
}

class bar
{
        static const std::string class_static_str0;
        static const std::string class_static_str1;
};

// ok, no gettext call
const std::string bar::class_static_str0 = "class_static_str0";

const std::string bar::class_static_str1 = _( "class_static_str1" );
// CHECK-MESSAGES: [[@LINE-1]]:44: warning: Gettext calls in static variable initialization will cause text to be untranslated (global static) or not updated when switching language (local static). Consider using translation objects (to_translation() or pl_translation()) or translate_marker(), and translate the text on demand (with translation::translated() or gettext calls outside static vars)
