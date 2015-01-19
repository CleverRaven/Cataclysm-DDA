#include "helper.h"

bool is_between(int low, int in, int high)
{
    return ((in >= low) ? ((in <= high) ? true : false) : false);
}


