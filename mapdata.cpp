#include "mapdata.h"

#include <ostream>

std::ostream & operator<<(std::ostream & out, const submap * sm)
{
 out << "submap(";
 if( !sm )
 {
  out << "NULL)";
  return out;
 }

 out << "\n\tter:";
 for(int x = 0; x < SEEX; ++x)
 {
  out << "\n\t" << x << ": ";
  for(int y = 0; y < SEEY; ++y)
   out << sm->ter[x][y] << ", ";
 }

 out << "\n\titm:";
 for(int x = 0; x < SEEX; ++x)
 {
  for(int y = 0; y < SEEY; ++y)
  {
   if( !sm->itm[x][y].empty() )
   {
    for( std::vector<item>::const_iterator it = sm->itm[x][y].begin(),
      end = sm->itm[x][y].end(); it != end; ++it )
    {
     out << "\n\t("<<x<<","<<y<<") ";
     out << *it << ", ";
    }
   }
  }
 }

   out << "\n\t)";
 return out;
}

std::ostream & operator<<(std::ostream & out, const submap & sm)
{
 out << (&sm);
 return out;
}
