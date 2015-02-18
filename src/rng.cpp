#include "rng.h"
#include <random>
#include <string> // hash

namespace {
std::default_random_engine random_generator {std::random_device{}()};
} //namespace

void seed_random_number_generator(char const* seed)
{
    if (!seed) {
        // std::random_device returns the same sequence on older version of GCC on windows.
        random_generator.seed(time(nullptr) ^ 0xDEADBEEF);
    } else {
        random_generator.seed(std::hash<std::string> {}(seed));
    }
}

long rng(long const val1, long const val2)
{
    static std::uniform_int_distribution<long> dist;
    dist.param(std::uniform_int_distribution<long>::param_type {
        (val1 < val2) ? val1 : val2, // lo
        (val1 < val2) ? val2 : val1  // hi
    });

    return dist(random_generator);
}

double rng_float(double const val1, double const val2)
{
    static std::uniform_real_distribution<double> dist;
    dist.param(std::uniform_real_distribution<double>::param_type {
        (val1 < val2) ? val1 : val2, // lo
        (val1 < val2) ? val2 : val1  // hi
    });

    return dist(random_generator);
}

bool one_in(int chance)
{
    return (chance <= 1 || rng(0, chance - 1) == 0);
}

// this works just like one_in, but it accepts doubles as input to calculate
// chances like "1 in 350,52"
bool one_in_improved(double chance)
{
    return (chance <= 1 || rng_float(0, chance) < 1);
}

bool x_in_y(double const x, double const y)
{
    static std::uniform_real_distribution<double> dist {0, 1};

    double const ratio = x / y;
    if (ratio >= 1.0) {
        return true;
    }

    return dist(random_generator) < ratio;
}

int dice(int number, int sides)
{
    int ret = 0;
    for (int i = 0; i < number; i++) {
        ret += rng(1, sides);
    }
    return ret;
}
