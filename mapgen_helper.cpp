#include "mapgen_helper.h"
#include "output.h"

mapgendata::mapgendata(oter_id north, oter_id east, oter_id south, oter_id west)
{
    t_nesw[0] = north;
    t_nesw[1] = east;
    t_nesw[2] = south;
    t_nesw[3] = west;
    n_fac = 0;
    e_fac = 0;
    s_fac = 0;
    w_fac = 0;
}

void mapgendata::set_dir(int dir, int val)
{
    switch (dir)
    {
        case 0:
            n_fac = val;
            break;
        case 1:
            e_fac = val;
            break;
        case 2:
            s_fac = val;
            break;
        case 3:
            w_fac = val;
            break;
        default:
            debugmsg("Invalid direction for mapgendata::set_dir. dir = %d", dir);
            break;
    }
}
