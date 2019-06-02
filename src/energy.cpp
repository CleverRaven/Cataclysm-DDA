#include "energy.h"

energy_quantity rng( energy_quantity lo, energy_quantity hi )
{
    return energy_quantity( rng( lo.millijoules, hi.millijoules ) );
}
