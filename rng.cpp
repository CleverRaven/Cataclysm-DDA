#include "output.h"
#include "rng.h"
#include <stdlib.h>


// http://software.intel.com/en-us/articles/fast-random-number-generator-on-the-intel-pentiumr-4-processor
<<<<<<< HEAD
// fastrand() is significantly faster than rand().  Not crypto secure, but good for CataDDA.
// These functions should be revisited if the code moves to C++11
=======
// fastrand() is 5x faster than rand().  Not crypto secure, but better than rand.
>>>>>>> d56c8c5c09ac7376f9c944206f386f2ed62b82b5
static unsigned int g_seed;

//Used to seed the generator.
void fast_srand( int seed ) {
    g_seed = seed;
}

<<<<<<< HEAD
//fastrand routine returns one integer between 0 and 2^15-1
inline int fastrand() {
    g_seed = (214013*g_seed+2531011);
    return (g_seed>>16)&0x7FFFF; 
=======
//fastrand routine returns one integer, modified from the intel version to give a max of 1048576.
inline int fastrand() {
    g_seed = (214013*g_seed+2531011);
    return (g_seed>>12); 
>>>>>>> d56c8c5c09ac7376f9c944206f386f2ed62b82b5
}

long rng(long val1, long val2)
{
    long minVal = (val1 < val2) ? val1 : val2;
    long maxVal = (val1 < val2) ? val2 : val1;
<<<<<<< HEAD
    return minVal + long((maxVal - minVal + 1) * double(fastrand() / 32768.0));
=======
    return minVal + long((maxVal - minVal + 1) * double(fastrand() / double(1048577)));
>>>>>>> d56c8c5c09ac7376f9c944206f386f2ed62b82b5
}

bool one_in(int chance)
{
    return (chance <= 1 || rng(0, chance - 1) == 0);
}

bool x_in_y(double x, double y)
{
<<<<<<< HEAD
    return ((double)fastrand() / 32768.0) <= ((double)x/y);
=======
    return ((double)fastrand() / 1048577) <= ((double)x/y);
>>>>>>> d56c8c5c09ac7376f9c944206f386f2ed62b82b5
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

