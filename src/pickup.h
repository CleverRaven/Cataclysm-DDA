#ifndef PICKUP_H
#define PICKUP_H

#include "enums.h"

#include <map>
#include <list>
#include <vector>
#include <string>

class vehicle;
class item;

class Pickup
{
    public:
        static void do_pickup( const tripoint &pickup_target, bool from_vehicle,
                               std::list<int> &indices, std::list<int> &quantities, bool autopickup );
        static void pick_up( const tripoint &p, int min ); // Pick up items; ',' or via examine()

    private:
        // No instances of Pickup allowed.
        Pickup() {}

        typedef std::pair<item, int> ItemCount;
        typedef std::map<std::string, ItemCount> PickupMap;

        // Pickup helper functions
        static void pick_one_up( const tripoint &pickup_target, item &newit,
                                 vehicle *veh, int cargo_part, int index, int quantity,
                                 bool &got_water, bool &offered_swap,
                                 PickupMap &mapPickup, bool autopickup );

        typedef enum {
            DONE, ITEMS_FROM_CARGO, ITEMS_FROM_GROUND,
        } interact_results;

        static interact_results interact_with_vehicle( vehicle *veh, const tripoint &vpos,
                int veh_root_part );

        static int handle_quiver_insertion( item &here, int &moves_to_decrement, bool &picked_up );
        static void remove_from_map_or_vehicle( const tripoint &pos, vehicle *veh, int cargo_part,
                                                int &moves_taken, int curmit );
        static void show_pickup_message( const PickupMap &mapPickup );
};

#endif
