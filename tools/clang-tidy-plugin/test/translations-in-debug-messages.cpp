// RUN: %check_clang_tidy -allow-stdinc %s cata-translations-in-debug-messages %t -- --load=%cata_plugin -- -I %cata_include -isystem %cata_include/third-party -DLOCALIZE

#include "calendar.h"
#include "creature.h"
#include "messages.h"
#include "translations.h"

static void f( const time_duration &duration, Creature &c )
{
    char *skill_level;
    add_msg_debug( debugmode::DF_ACT_BUTCHER, _( "Skill: %s" ), skill_level );
    // CHECK-MESSAGES: [[@LINE-1]]:47: warning: string arguments to debug message functions should not be translated. [cata-translations-in-debug-messages]

    add_msg_debug( debugmode::DF_ACT_BUTCHER, _( "Skill" ) );
    // CHECK-MESSAGES: [[@LINE-1]]:47: warning: string arguments to debug message functions should not be translated. [cata-translations-in-debug-messages]

    add_msg_debug( debugmode::DF_ACT_BUTCHER, pgettext( "Skill", "foo" ) );
    // CHECK-MESSAGES: [[@LINE-1]]:47: warning: string arguments to debug message functions should not be translated. [cata-translations-in-debug-messages]

    add_msg_debug( debugmode::DF_ACT_BUTCHER, n_gettext( "Skill", "Skills", 0 ) );
    // CHECK-MESSAGES: [[@LINE-1]]:47: warning: string arguments to debug message functions should not be translated. [cata-translations-in-debug-messages]

    add_msg_debug( debugmode::DF_ACT_BUTCHER, "%s", to_translation( "Skill" ) );
    // CHECK-MESSAGES: [[@LINE-1]]:53: warning: string arguments to debug message functions should not be translated. [cata-translations-in-debug-messages]

    add_msg_debug_if( true, debugmode::DF_ACT_BUTCHER, _( "Skill" ) );
    // CHECK-MESSAGES: [[@LINE-1]]:56: warning: string arguments to debug message functions should not be translated. [cata-translations-in-debug-messages]

    add_msg_debug( debugmode::DF_ACT_BUTCHER, "Duration %s", to_string( duration ) );
    // CHECK-MESSAGES: [[@LINE-1]]:62: warning: string arguments to debug message functions should not be translated.  This call to to_string might involve a translation; consider using to_string_writable instead. [cata-translations-in-debug-messages]
}
