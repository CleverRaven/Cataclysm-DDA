// RUN: %check_clang_tidy %s cata-no-static-translation %t -- -plugins=%cata_plugin -- -I %test_include

#include "mock-translation.h"

class foo
{
    public:
        foo( const char *, const char * );
};

// ok, doesn't contain translation calls
const std::string global_str_0 = "global_str_0";

const std::string global_str_1 = _( "global_str_1" );
// CHECK-MESSAGES: [[@LINE-1]]:34: warning: Translation functions should not be called when initializing a static variable.  See the `### Static string variables` section in `doc/TRANSLATING.md` for details.

const std::string global_str_2{ _( "global_str_2" ) };
// CHECK-MESSAGES: [[@LINE-1]]:33: warning: Translation functions should not be called when initializing a static variable.  See the `### Static string variables` section in `doc/TRANSLATING.md` for details.

static const std::string global_static_str = _( "global_static_str" );
// CHECK-MESSAGES: [[@LINE-1]]:46: warning: Translation functions should not be called when initializing a static variable.  See the `### Static string variables` section in `doc/TRANSLATING.md` for details.

const std::string global_gettext_str = gettext( "global_gettext_str" );
// CHECK-MESSAGES: [[@LINE-1]]:40: warning: Translation functions should not be called when initializing a static variable.  See the `### Static string variables` section in `doc/TRANSLATING.md` for details.

const std::string global_pgettext_str = pgettext( "ctxt", "global_pgettext_str" );
// CHECK-MESSAGES: [[@LINE-1]]:41: warning: Translation functions should not be called when initializing a static variable.  See the `### Static string variables` section in `doc/TRANSLATING.md` for details.

const std::string global_ngettext_str = n_gettext( "global_ngettext_str", "global_ngettext_strs",
                                        1 );
// CHECK-MESSAGES: [[@LINE-2]]:41: warning: Translation functions should not be called when initializing a static variable.  See the `### Static string variables` section in `doc/TRANSLATING.md` for details.

const std::string global_npgettext_str = npgettext( "ctxt", "global_npgettext_str",
        "global_npgettext_strs", 1 );
// CHECK-MESSAGES: [[@LINE-2]]:42: warning: Translation functions should not be called when initializing a static variable.  See the `### Static string variables` section in `doc/TRANSLATING.md` for details.

const std::string global_translation_translated = to_translation(
            "global_translation_translated" ).translated();
// CHECK-MESSAGES: [[@LINE-2]]:51: warning: Translation functions should not be called when initializing a static variable.  See the `### Static string variables` section in `doc/TRANSLATING.md` for details.

// ok, a translation object will be properly translated
const translation compare = to_translation( "compare" );
const translation compare2 = to_translation( "compare2" );

const bool global_translation_translated_eq = to_translation(
            "global_translation_translated_eq" ).translated_eq( compare );
// CHECK-MESSAGES: [[@LINE-2]]:47: warning: Translation functions should not be called when initializing a static variable.  See the `### Static string variables` section in `doc/TRANSLATING.md` for details.

const bool global_translation_translated_ne = to_translation(
            "global_translation_translated_ne" ).translated_ne( compare );
// CHECK-MESSAGES: [[@LINE-2]]:47: warning: Translation functions should not be called when initializing a static variable.  See the `### Static string variables` section in `doc/TRANSLATING.md` for details.

const bool global_translation_translated_lt = to_translation(
            "global_translation_translated_lt" ).translated_lt( compare );
// CHECK-MESSAGES: [[@LINE-2]]:47: warning: Translation functions should not be called when initializing a static variable.  See the `### Static string variables` section in `doc/TRANSLATING.md` for details.

const bool global_translation_translated_gt = to_translation(
            "global_translation_translated_gt" ).translated_gt( compare );
// CHECK-MESSAGES: [[@LINE-2]]:47: warning: Translation functions should not be called when initializing a static variable.  See the `### Static string variables` section in `doc/TRANSLATING.md` for details.

const bool global_translation_translated_le = to_translation(
            "global_translation_translated_le" ).translated_le( compare );
// CHECK-MESSAGES: [[@LINE-2]]:47: warning: Translation functions should not be called when initializing a static variable.  See the `### Static string variables` section in `doc/TRANSLATING.md` for details.

const bool global_translation_translated_ge = to_translation(
            "global_translation_translated_ge" ).translated_ge( compare );
// CHECK-MESSAGES: [[@LINE-2]]:47: warning: Translation functions should not be called when initializing a static variable.  See the `### Static string variables` section in `doc/TRANSLATING.md` for details.

const bool global_translated_less = translated_less()( compare, compare2 );
// CHECK-MESSAGES: [[@LINE-1]]:37: warning: Translation functions should not be called when initializing a static variable.  See the `### Static string variables` section in `doc/TRANSLATING.md` for details.

const bool global_translated_greater = translated_greater()( compare, compare2 );
// CHECK-MESSAGES: [[@LINE-1]]:40: warning: Translation functions should not be called when initializing a static variable.  See the `### Static string variables` section in `doc/TRANSLATING.md` for details.

const bool global_translated_less_equal = translated_less_equal()( compare, compare2 );
// CHECK-MESSAGES: [[@LINE-1]]:43: warning: Translation functions should not be called when initializing a static variable.  See the `### Static string variables` section in `doc/TRANSLATING.md` for details.

const bool global_translated_greater_equal = translated_greater_equal()( compare, compare2 );
// CHECK-MESSAGES: [[@LINE-1]]:46: warning: Translation functions should not be called when initializing a static variable.  See the `### Static string variables` section in `doc/TRANSLATING.md` for details.

const bool global_translated_equal_to = translated_equal_to()( compare, compare2 );
// CHECK-MESSAGES: [[@LINE-1]]:41: warning: Translation functions should not be called when initializing a static variable.  See the `### Static string variables` section in `doc/TRANSLATING.md` for details.

const bool global_translated_not_equal_to = translated_not_equal_to()( compare, compare2 );
// CHECK-MESSAGES: [[@LINE-1]]:45: warning: Translation functions should not be called when initializing a static variable.  See the `### Static string variables` section in `doc/TRANSLATING.md` for details.

static const char *const global_static_cstr = _( "global_static_cstr" );
// CHECK-MESSAGES: [[@LINE-1]]:47: warning: Translation functions should not be called when initializing a static variable.  See the `### Static string variables` section in `doc/TRANSLATING.md` for details.

static const foo global_static_foo {
    _( "global_static_foo" ),
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: Translation functions should not be called when initializing a static variable.  See the `### Static string variables` section in `doc/TRANSLATING.md` for details.
    _( "global_static_foo" )
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: Translation functions should not be called when initializing a static variable.  See the `### Static string variables` section in `doc/TRANSLATING.md` for details.
};

namespace cata
{
static const std::string namespace_static_str = _( "namespace_static_str" );
// CHECK-MESSAGES: [[@LINE-1]]:49: warning: Translation functions should not be called when initializing a static variable.  See the `### Static string variables` section in `doc/TRANSLATING.md` for details.

const std::string namespace_str = _( "namespace_str" );
// CHECK-MESSAGES: [[@LINE-1]]:35: warning: Translation functions should not be called when initializing a static variable.  See the `### Static string variables` section in `doc/TRANSLATING.md` for details.
} // namespace cata

static void local()
{
    // ok, non-static
    const std::string ok_str_1 = _( "ok_str_1" );

    static const std::string local_str_1 = _( "local_str_1" );
    // CHECK-MESSAGES: [[@LINE-1]]:44: warning: Translation functions should not be called when initializing a static variable.  See the `### Static string variables` section in `doc/TRANSLATING.md` for details.
}

class bar
{
        static const std::string class_static_str0;
        static const std::string class_static_str1;
};

// ok, no translation call
const std::string bar::class_static_str0 = "class_static_str0";

const std::string bar::class_static_str1 = _( "class_static_str1" );
// CHECK-MESSAGES: [[@LINE-1]]:44: warning: Translation functions should not be called when initializing a static variable.  See the `### Static string variables` section in `doc/TRANSLATING.md` for details.
