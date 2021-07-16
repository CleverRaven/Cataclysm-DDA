// RUN: %check_clang_tidy %s cata-translations-in-debug-messages %t -- -plugins=%cata_plugin -- -I %test_include

#include "mock-translation.h"

// dummy messages interface
class tripoint;
class Creature;

namespace debugmode
{
// Please try to keep this alphabetically sorted
enum debug_filter : int {
    DF_ACT_BUTCHER = 0, // butcher activity handler
};
}

struct game_message_params;

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

static void f()
{
    char *skill_level;
    add_msg_debug( debugmode::DF_ACT_BUTCHER, _( "Skill: %s" ), skill_level );
    // CHECK-MESSAGES: [[@LINE-1]]:47: warning: string arguments to debug message functions should not be translated, because this is an unnecessary performance cost. [cata-translations-in-debug-messages]
}
