#include "output.h"
#include "rng.h"

long rng(long low, long high)
{
 return low + long((high - low + 1) * double(rand() / double(RAND_MAX + 1.0)));
}

bool one_in(int chance)
{
 if (chance <= 1 || rng(0, chance - 1) == 0)
  return true;
 return false;
}

int dice(int number, int sides)
{
 int ret = 0;
 for (int i = 0; i < number; i++)
  ret += rng(1, sides);
 return ret;
}
