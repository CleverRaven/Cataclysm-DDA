#include "line.h"
#include "map.h"

#define SGN(a) (((a)<0) ? -1 : 1)

std::vector <point> line_to(int x1, int y1, int x2, int y2, int t) {
 std::vector<point> ret;
 int dx = x2 - x1;
 int dy = y2 - y1;
 int ax = abs(dx)<<1;
 int ay = abs(dy)<<1;
 int sx = SGN(dx);
 int sy = SGN(dy);
 if (dy == 0) sy = 0;
 if (dx == 0) sx = 0;
 point cur;
 cur.x = x1;
 cur.y = y1;

 int xmin = (x1 < x2 ? x1 : x2), ymin = (y1 < y2 ? y1 : y2),
     xmax = (x1 > x2 ? x1 : x2), ymax = (y1 > y2 ? y1 : y2);

 xmin -= abs(dx);
 ymin -= abs(dy);
 xmax += abs(dx);
 ymax += abs(dy);

/*
 if (x1 < 0 - SEEX * 3 || x1 > SEEX * 6 ||
     y1 < 0 - SEEY * 3 || y1 > SEEY * 6 ||
     x2 < 0 - SEEX * 3 || x2 > SEEX * 6 ||
     y2 < 0 - SEEY * 3 || y2 > SEEY * 6   )
  debugmsg("there's gonna be trouble!");
*/
 if (ax > ay) {
  do {
   if (t > 0) {
    cur.y += sy;
    t -= ax;
   }
   cur.x += sx;
   t += ay;
   ret.push_back(cur);
  } while ((cur.x != x2 || cur.y != y2) &&
           (cur.x >= xmin && cur.x <= xmax && cur.y >= ymin && cur.y <= ymax));
 } else {
  do {
   if (t > 0) {
    cur.x += sx;
    t -= ay;
   }
   cur.y += sy;
   t += ax;
   ret.push_back(cur);
  } while ((cur.x != x2 || cur.y != y2) &&
           (cur.x >= xmin && cur.x <= xmax && cur.y >= ymin && cur.y <= ymax));
 }
 return ret;
}

int trig_dist(int x1, int y1, int x2, int y2)
{
 return int(sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2)));
}

int rl_dist(int x1, int y1, int x2, int y2)
{
 int dx = abs(x1 - x2), dy = abs(y1 - y2);
 if (dx > dy)
  return dx;
 return dy;
}

direction direction_from(int x1, int y1, int x2, int y2)
{
 int dx = x2 - x1;
 int dy = y2 - y1;
 if (dx < 0) {
  if (abs(dx) >> 1 > abs(dy) || dy == 0) {
   return WEST;
  } else if (abs(dy) >> 1 > abs(dx)) {
   if (dy < 0)
    return NORTH;
   else
    return SOUTH;
  } else {
   if (dy < 0)
    return NORTHWEST;
   else
    return SOUTHWEST;
  }
 } else {
  if (dx >> 1 > abs(dy) || dy == 0) {
   return EAST;
  } else if (abs(dy) >> 1 > dx || dx == 0) {
   if (dy < 0)
    return NORTH;
   else
    return SOUTH;
  } else {
   if (dy < 0)
    return NORTHEAST;
   else
    return SOUTHEAST;
  }
 }
}

std::string direction_name(direction dir)
{
 switch (dir) {
  case NORTH:     return "north";
  case NORTHEAST: return "northeast";
  case EAST:      return "east";
  case SOUTHEAST: return "southeast";
  case SOUTH:     return "south";
  case SOUTHWEST: return "southwest";
  case WEST:      return "west";
  case NORTHWEST: return "northwest";
 }
 return "WEIRD DIRECTION_NAME() BUG";
}
