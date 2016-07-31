#ifndef PICKUP_H
#define PICKUP_H

#include "enums.h"

#include <list>

class vehicle;
class item;

namespace Pickup
{
void do_pickup( const tripoint &pickup_target, bool from_vehicle,
                std::list<int> &indices, std::list<int> &quantities, bool autopickup );

/** Pick up items; ',' or via examine() */
void pick_up( const tripoint &p, int min );
};

#endif
