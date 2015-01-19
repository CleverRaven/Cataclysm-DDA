#include "helper.h"

bool is_between(int low, int in, int high, bool equal_to)
{
    bool is_above_low  = ((equal_to == true) ? (in >= low)  : (in > low));
    bool is_below_high = ((equal_to == true) ? (in <= high) : (in < high));
    return ((is_above_low && is_below_high) ? true : false);
}

int clamp_value(int low, int in, int high)
{
    return ((in > low) ? ((in < high) ? in : high) : low);
}


