#ifndef _MORALE_H_
#define _MORALE_H_

#include <string>

#define MIN_MORALE_READ		(-20)
#define MIN_MORALE_CRAFT	(-15)

struct morale_point
{
 std::string name;
 int bonus;

 morale_point(std::string N = "Odd feeling", int B = 0) :
              name (N), bonus (B) {};
};
 
#endif
