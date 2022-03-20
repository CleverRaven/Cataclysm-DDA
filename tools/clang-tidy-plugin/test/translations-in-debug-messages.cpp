// RUN: %check_clang_tidy %s cata-translations-in-debug-messages %t -- -plugins=%cata_plugin -- -I %test_include

#include "mock-translation.h"

// dummy messages interface
class time_duration;
class tripoint;

namespace debugmode
{
// Please try to keep this alphabetically sorted
enum debug_filter : int {
    DF_ACT_BUTCHER = 0, // butcher activity handler
};
}

struct game_message_params;

class Creature
{
    public:
        void add_msg_debug_if_player( debugmode::debug_filter, const std::string & ) const;
        void add_msg_debug_if_npc( debugmode::debug_filter, const std::string & ) const;
};

void add_msg_debug( debugmode::debug_filter type, std::string msg );
template<typename ...Args>
void add_msg_debug( debugmode::debug_filter type, const std::string &msg, Args &&... args );
template<typename ...Args>
void add_msg_debug( debugmode::debug_filter type, const char *const msg, Args &&... args );

void add_msg_if_player_sees( const tripoint &target, std::string msg );
void add_msg_if_player_sees( const Creature &target, std::string msg );
template<typename ...Args>
void add_msg_if_player_sees( const tripoint &target, const std::string &msg,
                             Args &&... args );
template<typename ...Args>
void add_msg_if_player_sees( const Creature &target, const std::string &msg,
                             Args &&... args );
template<typename ...Args>
void add_msg_if_player_sees( const tripoint &target, const char *const msg, Args &&... args );
template<typename ...Args>
void add_msg_if_player_sees( const Creature &target, const char *const msg, Args &&... args );
template<typename ...Args>
void add_msg_if_player_sees( const tripoint &target, const translation &msg,
                             Args &&... args );
template<typename ...Args>
void add_msg_if_player_sees( const Creature &target, const translation &msg,
                             Args &&... args );

void add_msg_if_player_sees( const tripoint &target, const game_message_params &params,
                             std::string msg );
void add_msg_if_player_sees( const Creature &target, const game_message_params &params,
                             std::string msg );
template<typename ...Args>
void add_msg_if_player_sees( const tripoint &target, const game_message_params &params,
                             const std::string &msg, Args &&... args );
template<typename ...Args>
void add_msg_if_player_sees( const Creature &target, const game_message_params &params,
                             const std::string &msg, Args &&... args );
template<typename ...Args>
void add_msg_if_player_sees( const tripoint &target, const game_message_params &params,
                             const char *const msg, Args &&... args );
template<typename ...Args>
void add_msg_if_player_sees( const Creature &target, const game_message_params &params,
                             const char *const msg, Args &&... args );

void add_msg_debug_if_player_sees( const tripoint &target, debugmode::debug_filter type,
                                   std::string msg );
void add_msg_debug_if_player_sees( const Creature &target, debugmode::debug_filter type,
                                   std::string msg );
template<typename ...Args>
void add_msg_debug_if_player_sees( const tripoint &target, debugmode::debug_filter type,
                                   const std::string &msg, Args &&... args );
template<typename ...Args>
void add_msg_debug_if_player_sees( const Creature &target, debugmode::debug_filter type,
                                   const std::string &msg, Args &&... args );
template<typename ...Args>
void add_msg_debug_if_player_sees( const tripoint &target, debugmode::debug_filter type,
                                   const char *const msg, Args &&... args );
template<typename ...Args>
void add_msg_debug_if_player_sees( const Creature &target, debugmode::debug_filter type,
                                   const char *const msg, Args &&... args );

std::string to_string( const time_duration &d, bool compact = false );

static void f( const time_duration &duration, Creature &c )
{
    char *skill_level;
    add_msg_debug( debugmode::DF_ACT_BUTCHER, _( "Skill: %s" ), skill_level );
    // CHECK-MESSAGES: [[@LINE-1]]:47: warning: string arguments to debug message functions should not be translated, because this is an unnecessary performance cost. [cata-translations-in-debug-messages]

    add_msg_debug( debugmode::DF_ACT_BUTCHER, _( "Skill" ) );
    // CHECK-MESSAGES: [[@LINE-1]]:47: warning: string arguments to debug message functions should not be translated, because this is an unnecessary performance cost. [cata-translations-in-debug-messages]

    add_msg_debug( debugmode::DF_ACT_BUTCHER, pgettext( "Skill", "foo" ) );
    // CHECK-MESSAGES: [[@LINE-1]]:47: warning: string arguments to debug message functions should not be translated, because this is an unnecessary performance cost. [cata-translations-in-debug-messages]

    add_msg_debug( debugmode::DF_ACT_BUTCHER, n_gettext( "Skill", "Skills", 0 ) );
    // CHECK-MESSAGES: [[@LINE-1]]:47: warning: string arguments to debug message functions should not be translated, because this is an unnecessary performance cost. [cata-translations-in-debug-messages]

    add_msg_debug( debugmode::DF_ACT_BUTCHER, "%s", to_translation( "Skill" ) );
    // CHECK-MESSAGES: [[@LINE-1]]:53: warning: string arguments to debug message functions should not be translated, because this is an unnecessary performance cost. [cata-translations-in-debug-messages]

    add_msg_debug_if_player_sees( c, debugmode::DF_ACT_BUTCHER, _( "Skill: %s" ), skill_level );
    // CHECK-MESSAGES: [[@LINE-1]]:65: warning: string arguments to debug message functions should not be translated, because this is an unnecessary performance cost. [cata-translations-in-debug-messages]

    add_msg_debug_if_player_sees( c, debugmode::DF_ACT_BUTCHER, _( "Skill" ) );
    // CHECK-MESSAGES: [[@LINE-1]]:65: warning: string arguments to debug message functions should not be translated, because this is an unnecessary performance cost. [cata-translations-in-debug-messages]

    c.add_msg_debug_if_player( debugmode::DF_ACT_BUTCHER, _( "Skill" ) );
    // CHECK-MESSAGES: [[@LINE-1]]:59: warning: string arguments to debug message functions should not be translated, because this is an unnecessary performance cost. [cata-translations-in-debug-messages]

    c.add_msg_debug_if_npc( debugmode::DF_ACT_BUTCHER, _( "Skill" ) );
    // CHECK-MESSAGES: [[@LINE-1]]:56: warning: string arguments to debug message functions should not be translated, because this is an unnecessary performance cost. [cata-translations-in-debug-messages]

    add_msg_debug( debugmode::DF_ACT_BUTCHER, "Duration %s", to_string( duration ) );
    // CHECK-MESSAGES: [[@LINE-1]]:62: warning: string arguments to debug message functions should not be translated, because this is an unnecessary performance cost.  This call to to_string might involve a translation; consider using to_string_writable instead. [cata-translations-in-debug-messages]
}
