#ifndef _RNG_H_
#define _RNG_H_
#include <stdlib.h>
#include <random>

long rng(long low, long high);
bool one_in(int chance);
bool x_in_y(double x, double y);
int dice(int number, int sides);
int djb2_hash(unsigned char *input);

namespace RNG_functions
{
    double normal(); // The standard normal distribution.
    double normal(const double& mean, const double& stdev);
    extern std::default_random_engine gen;
}


#endif
