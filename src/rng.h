#ifndef _RNG_H_
#define _RNG_H_

long rng(long val1, long val2);
double rng_float(double val1, double val2);
bool one_in(int chance);
bool x_in_y(double x, double y);
int dice(int number, int sides);

int djb2_hash(const unsigned char *input);
#endif
