#include "output.h"
#include "rng.h"
#include <stdlib.h>

long rng(long val1, long val2)
{
    long minVal = (val1 < val2) ? val1 : val2;
    long maxVal = (val1 < val2) ? val2 : val1;
    return minVal + long((maxVal - minVal + 1) * double(rand() / double(RAND_MAX + 1.0)));
}

double rng_float(double val1, double val2)
{
    double minVal = (val1 < val2) ? val1 : val2;
    double maxVal = (val1 < val2) ? val2 : val1;
    return minVal + (maxVal - minVal) * double(rand()) / double(RAND_MAX + 1.0);
}

bool one_in(int chance)
{
    return (chance <= 1 || rng(0, chance - 1) == 0);
}

bool x_in_y(double x, double y)
{
    return ((double)rand() / RAND_MAX) <= ((double)x/y);
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

