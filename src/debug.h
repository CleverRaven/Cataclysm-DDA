#pragma once
#ifndef CATA_SRC_DEBUG_H
#define CATA_SRC_DEBUG_H

#include "string_formatter.h"
#include <unordered_set>

/**
 *      debugmsg(msg, ...)
 * varg-style functions: the first argument is the format (see printf),
 * the other optional arguments must match that format.
 * The message is formatted and printed on screen to player, they must
 * acknowledge the message by pressing SPACE. This can be quite annoying, so
 * avoid this function if possible.
 * The message is also logged with DebugLog, a separate call to DebugLog
 * is not needed.
 *
 *      DebugLog
 * The main debugging system. It uses debug levels (how critical/important
 * the message is) and debug classes (which part of the code it comes from,
 * e.g. mapgen, SDL, npcs). This allows disabling debug classes and levels
 * (e.g. only write SDL messages, but no npc or map related ones).
 * It returns a reference to an outputs stream. Simply write your message into
 * that stream (using the << operators).
 * DebugLog always returns a stream that starts on a new line. Don't add a
 * newline at the end of your debug message.
 * If the specific debug level or class have been disabled, the message is
 * actually discarded, otherwise it is written to a log file.
 * If a single source file contains mostly messages for the same debug class
 * (e.g. mapgen.cpp), create and use the macro dbg.
 *
 *      dbg
 * Usually a single source contains only debug messages for a single debug class
 * (e.g. mapgen.cpp contains only messages for D_MAP_GEN, npcmove.cpp only D_NPC).
 * Those files contain a macro at top:
@code
#define dbg(x) DebugLog((x), D_NPC) << __FILE__ << ":" << __LINE__ << ": "
@endcode
 * It allows to call the debug system and just supply the debug level, the debug
 * class is automatically inserted as it is the same for the whole file. Also this
 * adds the file name and the line of the statement to the debug message.
 * This can be replicated in any source file, just copy the above macro and change
 * D_NPC to the debug class to use. Don't add this macro to a header file
 * as the debug class is source file specific.
 * As dbg calls DebugLog, it returns the stream, its usage is the same.
 */

#include <functional>
// Includes                                                         {{{1
// ---------------------------------------------------------------------
#include <iostream>
#include <type_traits>

#define STRING2(x) #x
#define STRING(x) STRING2(x)

#if defined(__GNUC__)
#define CATA_FUNCTION_NAME __PRETTY_FUNCTION__
#else
#define CATA_FUNCTION_NAME __func__
#endif

/**
 * Debug message of level D_ERROR and class D_MAIN, also includes the source
 * file name and line, uses varg style arguments, the first argument must be
 * a printf style format string.
 */

#define debugmsg(...) realDebugmsg(__FILE__, STRING(__LINE__), CATA_FUNCTION_NAME, __VA_ARGS__)

// Don't use this, use debugmsg instead.
void realDebugmsg( const char *filename, const char *line, const char *funcname,
                   const std::string &text );
template<typename ...Args>
inline void realDebugmsg( const char *const filename, const char *const line,
                          const char *const funcname, const char *const mes, Args &&... args )
{
    return realDebugmsg( filename, line, funcname, string_format( mes,
                         std::forward<Args>( args )... ) );
}

// Fatal error with a message
#define cata_fatal(...) \
    do { \
        debugmsg(__VA_ARGS__); \
        std::abort(); \
    } while( false )

// A fatal error for use in constexpr functions
// This exists for compatibility reasons.  On gcc before 9 we need a
// different implementation that is messier.
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67371
// Pass a placeholder return value to be used on old gcc (it won't
// actually be returned, it's just needed for the type), and then
// args as if to debugmsg for the remaining args.
#if defined(__GNUC__) && __GNUC__ < 9
#define constexpr_fatal(ret, ...) \
    do { return false ? ( ret ) : ( abort(), ( ret ) ); } while(false)
#else
#define constexpr_fatal(ret, ...) \
    do { debugmsg(__VA_ARGS__); abort(); } while(false)
#endif

/**
 * Used to generate game report information.
 */
namespace game_info
{
/** Return the name of the current operating system.
 */
std::string operating_system();
/** Return a detailed version of the operating system; e.g. "Ubuntu 18.04" or "(Windows) 10 1809".
 */
std::string operating_system_version();
/** Return the "bitness" of the game (not necessarily of the operating system); either: 64-bit, 32-bit or Unknown.
 */
std::string bitness();
/** Return the game version, as in the entry screen.
 */
std::string game_version();
/** Return the underlying graphics version used by the game; either Tiles or Curses.
*/
std::string graphics_version();
/** Return a list of the loaded mods, including the mod full name and its id name in brackets, e.g. "Dark Days Ahead [dda]".
*/
std::string mods_loaded();
/** Generate a game report, including the information returned by all of the other functions.
 */
std::string game_report();
} // namespace game_info

// Enumerations                                                     {{{1
// ---------------------------------------------------------------------

/**
 * If you add an entry, add an entry in that function:
 * std::ostream &operator<<(std::ostream &out, DebugLevel lev)
 */
enum DebugLevel {
    D_INFO          = 1,
    D_WARNING       = 1 << 2,
    D_ERROR         = 1 << 3,
    D_PEDANTIC_INFO = 1 << 4,

    DL_ALL = ( 1 << 5 ) - 1
};

inline DebugLevel operator|( DebugLevel l, DebugLevel r )
{
    return static_cast<DebugLevel>(
               static_cast<std::underlying_type_t<DebugLevel>>( l ) | r );
}

/**
 * Debugging areas can be enabled for each of those areas separately.
 * If you add an entry, add an entry in that function:
 * std::ostream &operator<<(std::ostream &out, DebugClass cl)
 */
enum DebugClass {
    /** Messages from realDebugmsg */
    D_MAIN    = 1,
    /** Related to map and mapbuffer (map.cpp, mapbuffer.cpp) */
    D_MAP     = 1 << 2,
    /** Mapgen (mapgen*.cpp), also overmap.cpp */
    D_MAP_GEN = 1 << 3,
    /** Main game class */
    D_GAME    = 1 << 4,
    /** npcs*.cpp */
    D_NPC     = 1 << 5,
    /** SDL & tiles & anything graphical */
    D_SDL     = 1 << 6,
    /** Related to tile memory (map_memory.cpp) */
    D_MMAP    = 1 << 7,

    DC_ALL    = ( 1 << 30 ) - 1
};

enum class DebugOutput : int {
    std_err,
    file,
};

/** Initializes the debugging system, called exactly once from main() */
void setupDebug( DebugOutput );
/** Opposite of setupDebug, shuts the debugging system down. */
void deinitDebug();

// Function Declarations                                            {{{1
// ---------------------------------------------------------------------
/**
 * Set debug levels that should be logged. bitmask is a OR-combined
 * set of DebugLevel values. Use 0 to disable all.
 * Note that D_ERROR is always logged.
 */
void limitDebugLevel( int );
/**
 * Set the debug classes should be logged. bitmask is a OR-combined
 * set of DebugClass values. Use 0 to disable all.
 * Note that D_UNSPECIFIC is always logged.
 */
void limitDebugClass( int );

/**
 * @return true if any error has been logged in this run.
 */
bool debug_has_error_been_observed();

/**
 * Capturing debug messages during func execution,
 * used to test debugmsg calls in the unit tests
 * @return std::string debugmsg
 */
std::string capture_debugmsg_during( const std::function<void()> &func );

/**
 * Should be called after catacurses::stdscr is initialized.
 * If catacurses::stdscr is available, shows all buffered debugmsg prompts.
 */
void replay_buffered_debugmsg_prompts();

// Debug Only                                                       {{{1
// ---------------------------------------------------------------------

// See documentation at the top.
std::ostream &DebugLog( DebugLevel, DebugClass );

/**
 * Extended debugging mode, can be toggled during game.
 * If enabled some debug message in the normal player message log are shown,
 * and other windows might have verbose display (e.g. vehicle window).
 */
extern bool debug_mode;

namespace debugmode
{
// Please try to keep this alphabetically sorted
enum debug_filter : int {
    DF_ACT_BUTCHER = 0, // butcher activity handler
    DF_ACT_EBOOK, // ebook activity actor
    DF_ACT_HARVEST, // harvest activity actor
    DF_ACT_LOCKPICK, // lockpicking activity actor
    DF_ACT_READ, // reading activity actor
    DF_ACT_SAFECRACKING, // safecracking activity actor
    DF_ACT_SHEARING, // shearing activity actor
    DF_ACT_WORKOUT, // workout activity actor
    DF_ACTIVITY, // activity actor generic
    DF_ANATOMY_BP, // anatomy::select_body_part()
    DF_AVATAR, // avatar generic
    DF_BALLISTIC, // ballistic generic
    DF_CHARACTER, // character generic
    DF_CHAR_CALORIES, // character stomach and calories
    DF_CHAR_HEALTH, // character health related
    DF_CRAFTING, // Crafting everything
    DF_CREATURE, // creature generic
    DF_EFFECT, // effects generic
    DF_EXPLOSION, // explosion generic
    DF_FOOD, // food generic
    DF_GAME, // game generic
    DF_IEXAMINE, // iexamine generic
    DF_IUSE, // iuse generic
    DF_MAP, // map generic
    DF_MATTACK, // monster attack generic
    DF_MELEE, // melee generic
    DF_MONMOVE, // movement/pathfinding-related
    DF_MONSTER, // monster generic
    DF_MUTATION, // mutation/purification logic
    DF_NPC, // npc generic, less verbose comments
    DF_NPC_COMBATAI, // npc combat and danger assessment logic
    DF_NPC_ITEMAI, // npc weapon/item logic - weapon choices, decision to reload, etc.
    DF_NPC_MOVEAI, // Pathfinding and movement logic.  For the NPC with places to be.
    DF_OVERMAP, // overmap generic
    DF_RADIO, // radio stuff
    DF_RANGED, // ranged generic
    DF_REQUIREMENTS_MAP, // activity_item_handler requirements_map()
    DF_SOUND, // sound generic
    DF_TALKER, // talker generic
    DF_VEHICLE, // vehicle generic
    DF_VEHICLE_DRAG, // vehicle coeff_air_drag()
    DF_VEHICLE_MOVE, // vehicle move generic
    DF_LAST // This is always the last entry
};

extern std::unordered_set<debug_filter> enabled_filters;
std::string filter_name( debug_filter value );
} // namespace debugmode

#if defined(BACKTRACE)
/**
 * Write a stack backtrace to the given ostream
 */
void debug_write_backtrace( std::ostream &out );
#endif

// vim:tw=72:sw=4:fdm=marker:fdl=0:
#endif // CATA_SRC_DEBUG_H
