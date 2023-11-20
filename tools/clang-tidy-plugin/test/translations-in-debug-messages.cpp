// RUN: %check_clang_tidy -allow-stdinc %s cata-translations-in-debug-messages %t -- --load=%cata_plugin -- -I %cata_include -isystem %cata_include/third-party -DLOCALIZE

#include "calendar.h"
#include "creature.h"
#include "messages.h"
#include "translations.h"

static void f( const time_duration &duration, Creature &c )
{
    char *skill_level;
    add_msg_debug_if_player_sees( c, debugmode::DF_ACT_BUTCHER, _( "Skill: %s" ), skill_level );
    // CHECK-MESSAGES: [[@LINE-1]]:65: warning: string arguments to debug message functions should not be translated, because this is an unnecessary performance cost. [cata-translations-in-debug-messages]

    add_msg_debug_if_player_sees( c, debugmode::DF_ACT_BUTCHER, _( "Skill" ) );
    // CHECK-MESSAGES: [[@LINE-1]]:65: warning: string arguments to debug message functions should not be translated, because this is an unnecessary performance cost. [cata-translations-in-debug-messages]

    c.add_msg_debug_if_player( debugmode::DF_ACT_BUTCHER, _( "Skill" ) );
    // CHECK-MESSAGES: [[@LINE-1]]:59: warning: string arguments to debug message functions should not be translated, because this is an unnecessary performance cost. [cata-translations-in-debug-messages]

    c.add_msg_debug_if_npc( debugmode::DF_ACT_BUTCHER, _( "Skill" ) );
    // CHECK-MESSAGES: [[@LINE-1]]:56: warning: string arguments to debug message functions should not be translated, because this is an unnecessary performance cost. [cata-translations-in-debug-messages]
}
