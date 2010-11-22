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
  } while ((cur.x != x2 || cur.y != y2));
/*
           (cur.x >= 0 - SEEX * 3 && cur.x <= SEEX * 6 &&
            cur.y >= 0 - SEEY * 3 && cur.y <= SEEY * 6   ));
*/
 } else {
  do {
   if (t > 0) {
    cur.x += sx;
    t -= ay;
   }
   cur.y += sy;
   t += ax;
   ret.push_back(cur);
  } while ((cur.x != x2 || cur.y != y2));
/*
           (cur.x >= 0 - SEEX * 3 && cur.x <= SEEX * 6 &&
            cur.y >= 0 - SEEY * 3 && cur.y <= SEEY * 6   ));
*/
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

std::string direction_from(int x1, int y1, int x2, int y2)
{
 int dx = x2 - x1;
 int dy = y2 - y1;
 if (dx < 0) {
  if (abs(dx) >> 1 > abs(dy) || dy == 0) {
   return "the west";
  } else if (abs(dy) >> 1 > abs(dx)) {
   if (dy < 0)
    return "the north";
   else
    return "the south";
  } else {
   if (dy < 0)
    return "the northwest";
   else
    return "the southwest";
  }
 } else {
  if (dx >> 1 > abs(dy) || dy == 0) {
   return "the east";
  } else if (abs(dy) >> 1 > dx || dx == 0) {
   if (dy < 0)
    return "the north";
   else
    return "the south";
  } else {
   if (dy < 0)
    return "the northeast";
   else
    return "the southeast";
  }
 }
}
