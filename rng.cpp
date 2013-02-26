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
bool x_in_y(double x, double y)
{
 if( ((double)rand() / RAND_MAX) <= ((double)x/y) )
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


// http://www.cse.yorku.ca/~oz/hash.html
// for world seeding.
int djb2_hash(unsigned char *str){
 unsigned long hash = 5381;
 int c;
 while (c = *str++)
  hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
 return hash;
}

