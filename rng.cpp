#include "output.h"
#include "rng.h"
#include <stdlib.h>


// http://software.intel.com/en-us/articles/fast-random-number-generator-on-the-intel-pentiumr-4-processor
// fastrand() is significantly faster than rand().  Not crypto secure, but good for CataDDA.
// These functions should be revisited if the code moves to C++11
static unsigned int g_seed;

//Used to seed the generator.
void fast_srand( int seed ) {
    g_seed = seed;
}

//fastrand routine returns one integer between 0 and 2^15-1
inline int fastrand() {
    g_seed = (214013*g_seed+2531011);
    return (g_seed>>16)&0x7FFFF; 
}

long rng(long val1, long val2)
{
    long minVal = (val1 < val2) ? val1 : val2;
    long maxVal = (val1 < val2) ? val2 : val1;
    return minVal + long((maxVal - minVal + 1) * double(fastrand() / 32768.0));
}

bool one_in(int chance)
{
    return (chance <= 1 || rng(0, chance - 1) == 0);
}

bool x_in_y(double x, double y)
{
    return ((double)fastrand() / 32768.0) <= ((double)x/y);
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
int djb2_hash(const unsigned char *str){
 unsigned long hash = 5381;
 unsigned char c = *str++;
 while (c != '\0'){
  hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  c = *str++;
 }
 return hash;
}

