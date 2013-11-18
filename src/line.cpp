#include "line.h"
#include "game.h"
#include <stdlib.h>

#define SGN(a) (((a)<0) ? -1 : 1)

std::vector <point> line_to(int x1, int y1, int x2, int y2, int t)
{
 std::vector<point> ret;
 // Preallocate the number of cells we need instead of allocating them piecewise.
 ret.reserve( square_dist( x1, y1, x2, y2 ) );
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

 if (ax == ay) {
  do {
   cur.y += sy;
   cur.x += sx;
   ret.push_back(cur);
  } while ((cur.x != x2 || cur.y != y2) &&
           (cur.x >= xmin && cur.x <= xmax && cur.y >= ymin && cur.y <= ymax));
 } else if (ax > ay) {
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
   return int( sqrt( double( (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) ) ) );
}

int square_dist(int x1, int y1, int x2, int y2) {
   int dx = abs(x1 - x2), dy = abs(y1 - y2);
   return ( dx > dy ? dx : dy );
}

int rl_dist(int x1, int y1, int x2, int y2)
{
 if(trigdist) {
     return trig_dist( x1, y1, x2, y2 );
 }
 return square_dist( x1, y1, x2, y2 );
}

int rl_dist(point a, point b)
{
 return rl_dist(a.x, a.y, b.x, b.y);
}

double slope_of(std::vector<point> line)
{
 double dX = line.back().x - line.front().x, dY = line.back().y - line.front().y;
 if (dX == 0)
  return SLOPE_VERTICAL;
 return (dY / dX);
}

std::vector<point> continue_line(std::vector<point> line, int distance)
{
 point start = line.back(), end = line.back();
 double slope = slope_of(line);
 int sX = (line.front().x < line.back().x ? 1 : -1),
     sY = (line.front().y < line.back().y ? 1 : -1);
 if (abs(slope) == 1) {
  end.x += distance * sX;
  end.y += distance * sY;
 } else if (abs(slope) < 1) {
  end.x += distance * sX;
  end.y += int(distance * abs(slope) * sY);
 } else {
  end.y += distance * sY;
  if (slope != SLOPE_VERTICAL)
   end.x += int(distance / abs(slope)) * sX;
 }
 return line_to(start.x, start.y, end.x, end.y, 0);
}

direction direction_from(int x1, int y1, int x2, int y2)
{
 int dx = x2 - x1;
 int dy = y2 - y1;
 if (dx < 0) {
  if (abs(dx) / 2 > abs(dy) || dy == 0) {
   return WEST;
  } else if (abs(dy) / 2 > abs(dx)) {
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
  if (dx / 2 > abs(dy) || dy == 0) {
   return EAST;
  } else if (abs(dy) / 2 > dx || dx == 0) {
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
  //~ used for "to the north" etc
  case NORTH:     return _("north");
  case NORTHEAST: return _("northeast");
  case EAST:      return _("east");
  case SOUTHEAST: return _("southeast");
  case SOUTH:     return _("south");
  case SOUTHWEST: return _("southwest");
  case WEST:      return _("west");
  case NORTHWEST: return _("northwest");
 }
 return "BUG. (line.cpp:direction_name)";
}

std::string direction_name_short(direction dir)
{
 switch (dir) {
  //~ abbreviated direction names
  case NORTH:     return _("N ");
  case NORTHEAST: return _("NE");
  case EAST:      return _("E ");
  case SOUTHEAST: return _("SE");
  case SOUTH:     return _("S ");
  case SOUTHWEST: return _("SW");
  case WEST:      return _("W ");
  case NORTHWEST: return _("NW");
 }
 return "Bug. (line.cpp:direction_name_short)";
}


float rl_vec2d::norm(){
 return sqrt(x*x + y*y);
}

rl_vec2d rl_vec2d::normalized(){
 rl_vec2d ret;
 if (is_null()){ // shouldn't happen?
  ret.x = ret.y = 1;
  return ret;
 }
 float n = norm();
 ret.x = x/n;
 ret.y = y/n;
 return ret;
}

rl_vec2d rl_vec2d::get_vertical(){
 rl_vec2d ret;
 ret.x = -y;
 ret.y = x;
 return ret;
}
float rl_vec2d::dot_product (rl_vec2d &v){
 float dot = x*v.x + y*v.y;
 return dot;
}
bool rl_vec2d::is_null(){
 return !(x || y);
}
// scale.
rl_vec2d rl_vec2d::operator* (const float rhs){
 rl_vec2d ret;
 ret.x = x * rhs;
 ret.y = y * rhs;
 return ret;
}
// subtract
rl_vec2d rl_vec2d::operator- (const rl_vec2d &rhs){
 rl_vec2d ret;
 ret.x = x - rhs.x;
 ret.y = y - rhs.y;
 return ret;
}
// unary negation
rl_vec2d rl_vec2d::operator- (){
 rl_vec2d ret;
 ret.x = -x;
 ret.y = -y;
 return ret;
}
rl_vec2d rl_vec2d::operator+ (const rl_vec2d &rhs){
 rl_vec2d ret;
 ret.x = x + rhs.x;
 ret.y = y + rhs.y;
 return ret;
}
rl_vec2d rl_vec2d::operator/ (const float rhs){
 rl_vec2d ret;
 ret.x = x / rhs;
 ret.y = y / rhs;
 return ret;
}
