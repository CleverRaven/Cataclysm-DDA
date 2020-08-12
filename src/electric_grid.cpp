#include "electric_grid.h"
#include "active_tile_data.h"
#include "submap.h"

class battery_tile;

electric_grid::electric_grid( std::set<submap *> sms )
    : submaps( sms )
{
    for( submap *sm : sms ) {
        for( auto &pr : sm->active_furniture ) {
            active_tile_data *act = pr.second.get();

            // TODO: Avoid dynamic cast hacks
            battery_tile *battery = dynamic_cast<battery_tile *>( act );
            if( battery != nullptr ) {
                batteries.push_back( battery );
            }
        }
    }
}

int electric_grid::mod_energy( int amt )
{
    for( battery_tile *bat : batteries ) {
        amt = bat->mod_energy( amt );
        if( amt == 0 ) {
            return 0;
        }
    }
    return amt;
}
