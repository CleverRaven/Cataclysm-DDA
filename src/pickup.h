#ifndef _PICKUP_H_
#define _PICKUP_H_

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
        static void do_pickup( point pickup_target, bool from_vehicle,
                               std::list<int> &indices, std::list<int> &quantities );
        static void pick_up(int posx, int posy, int min); // Pick up items; ',' or via examine()

    private:
        // No instances of Pickup allowed.
        Pickup() {}

        // Pickup helper functions
        static void pick_one_up( const point &pickup_target, std::vector<item> &here,
                                 vehicle *veh, int cargo_part, int index, int quantity,
                                 bool &got_water, bool &offered_swap,
                                 std::map<std::string, int> &mapPickup );

        static int interact_with_vehicle( vehicle *veh, int posx, int posy, int veh_root_part );

        static int handle_quiver_insertion( item &here, bool inv_on_fail, int &moves_to_decrement,
                                            bool &picked_up );
        static void remove_from_map_or_vehicle( int posx, int posy, vehicle *veh, int cargo_part,
                                                int &moves_taken, int curmit );
        static void show_pickup_message( std::map<std::string, int> &mapPickup );
};


#endif //_PICKUP_H_
