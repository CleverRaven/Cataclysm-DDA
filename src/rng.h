#ifndef RNG_H
#define RNG_H

#include <string>

long rng(long val1, long val2);
double rng_float(double val1, double val2);
bool one_in(int chance);
bool one_in_improved(double chance);
bool x_in_y(double x, double y);
int dice(int number, int sides);

int djb2_hash(const unsigned char *input);

inline int djb2_hash(char const *input) {
    return djb2_hash(reinterpret_cast<unsigned char const*>(input));
}

inline int djb2_hash(std::string const &input) {
    return djb2_hash(input.c_str());
}

#endif
