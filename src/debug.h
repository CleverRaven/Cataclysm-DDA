#ifndef DEBUG_H_1YCYLZSS
#define DEBUG_H_1YCYLZSS

// Includes                                                         {{{1
// ---------------------------------------------------------------------
#include <iostream>
#include <vector>

#define STRING2(x) #x
#define STRING(x) STRING2(x)

// classy
#define debugmsg(...) realDebugmsg(__FILE__, STRING(__LINE__), __VA_ARGS__)

/**
 * Debug message of level D_ERROR and class D_MAIN
 */
void realDebugmsg( const char *name, const char *line, const char *mes, ... );

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
    /** ncps*.cpp */
    D_NPC     = 1 << 5,
    /** SDL & tiles & anything graphical */
    D_SDL     = 1 << 6,

    DC_ALL    = ( 1 << 30 ) - 1
};

/** Initializes the debugging system, called exactly once from main() */
void setupDebug();
/** Opposite of setupDebug, shuts the debugging system down. */
void deinitDebug();

// Function Declatations                                            {{{1
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

// Debug Only                                                       {{{1
// ---------------------------------------------------------------------

std::ostream &dout( DebugLevel = DL_ALL, DebugClass = DC_ALL );

// OStream operators                                                {{{1
// ---------------------------------------------------------------------

template<typename C, typename A>
std::ostream &operator<<( std::ostream &out, const std::vector<C, A> &elm )
{
    bool first = true;
    for( typename std::vector<C>::const_iterator
         it = elm.begin(),
         end = elm.end();
         it != end; ++it ) {
        if( first ) {
            first = false;
        } else {
            out << ",";
        }
        out << *it;
    }

    return out;
}

#ifndef DebugLog
#define DebugLog dout
#endif


// vim:tw=72:sw=1:fdm=marker:fdl=0:
#endif /* end of include guard: DEBUG_H_1YCYLZSS */
