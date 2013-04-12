#include "output.h"
#include "rng.h"

std::default_random_engine gen;

long rng(long low, long high)
{
    if (low <= high)
    {
        std::uniform_int_distribution<> dist(low, high);
        return dist(gen);
    }
    else //Because not sure if the 0 ~ -1 at startup is intentional or not.
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
    std::uniform_real_distribution<> dist(0, 1.0f);
 if( dist(gen) <= ((double)x/y) )
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
 unsigned char c = *str++;
 while (c != '\0'){
  hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  c = *str++;
 }
 return hash;
}

