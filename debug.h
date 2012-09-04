#ifndef DEBUG_H_1YCYLZSS
#define DEBUG_H_1YCYLZSS

// Includes                                                         {{{1
// ---------------------------------------------------------------------

#include <iostream>
#include <vector>

// Enumerations                                                     {{{1
// ---------------------------------------------------------------------

enum DebugLevel
{
 D_INFO          = 1,
 D_WARNING       = 1<<2,
 D_ERROR         = 1<<3,
 D_PEDANTIC_INFO = 1<<4,

 DL_ALL = (1<<5)-1
};

enum DebugClass
{
 D_MAIN    = 1,
 D_MAP     = 1<<2,
 D_MAP_GEN = 1<<3,
 D_GAME    = 1<<4,
 D_NPC     = 1<<5,

 DC_ALL    = (1<<6)-1
};

void setupDebug();

// Function Declatations                                            {{{1
// ---------------------------------------------------------------------

void limitDebugLevel( int );
void limitDebugClass( int );

// Debug Only                                                       {{{1
// ---------------------------------------------------------------------

#ifdef ENABLE_LOGGING
std::ostream & dout(DebugLevel=DL_ALL,DebugClass=DC_ALL);
#else // if NOT defined ENABLE_LOGGING

// Non debug only                                                   {{{1
// ---------------------------------------------------------------------

struct DebugVoid {};
 template<class T>
DebugVoid operator<< ( const DebugVoid & dv, const T & )
{ return dv; }
DebugVoid dout(DebugLevel=DL_ALL,DebugClass=DC_ALL);

#endif // END if NOT defined ENABLE_LOGGING

// OStream operators                                                {{{1
// ---------------------------------------------------------------------

template<typename C, typename A>
std::ostream & operator<<(std::ostream & out, const std::vector<C,A> & elm)
{
 bool first = true;
 for( typename std::vector<C>::const_iterator 
        it = elm.begin(),
        end = elm.end();
        it != end; ++it )
 {
  if( first )
   first = false;
  else
   out << ",";
  out << *it;
 }
}



// vim:tw=72:sw=1:fdm=marker:fdl=0:
#endif /* end of include guard: DEBUG_H_1YCYLZSS */
