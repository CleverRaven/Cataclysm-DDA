#ifndef _LINE_H_
#define _LINE_H_

#include <vector>
#include <string>
#include "enums.h"

// The "t" value decides WHICH Bresenham line is used.
std::vector <point> line_to(int x1, int y1, int x2, int y2, int t);
int trig_dist(int x1, int y1, int x2, int y2);
int rl_dist(int x1, int y1, int x2, int y2);
std::string direction_from(int x1, int y1, int x2, int y2);

#endif
