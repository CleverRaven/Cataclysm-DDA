#ifndef DEBUG_H_1YCYLZSS
#define DEBUG_H_1YCYLZSS

#include <iostream>
#include <vector>

enum DebugLevel
{
 D_INFO = 1,
 D_WARNING = 2,
 D_ERROR = 4,
 D_PEDANTIC_INFO = 8,

 DL_ALL = 15
};

enum DebugClass
{
 D_MAIN    = 1,
 D_MAP     = 2,
 D_MAP_GEN = 4,

 DC_ALL    = 7
};

void limitDebugLevel( int );
void limitDebugClass( int );

#ifndef NDEBUG
std::ostream & dout(DebugLevel=DL_ALL,DebugClass=DC_ALL);
#else
struct DebugVoid {};
 template<class T>
DebugVoid operator<< ( const DebugVoid & dv, const T & )
{ return dv; }
DebugVoid dout(DebugLevel=DL_ALL,DebugClass=DC_ALL);
#endif

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




#endif /* end of include guard: DEBUG_H_1YCYLZSS */
