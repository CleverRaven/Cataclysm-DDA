#ifndef PATHFINDING_H
#define PATHFINDING_H

#include "game_constants.h"

class JsonObject;

enum pf_special : char {
    PF_NORMAL = 0x00,    // Plain boring tile (grass, dirt, floor etc.)
    PF_SLOW = 0x01,      // Tile with move cost >2
    PF_WALL = 0x02,      // Unpassable ter/furn/vehicle
    PF_VEHICLE = 0x04,   // Any vehicle tile (passable or not)
    PF_FIELD = 0x08,     // Dangerous field
    PF_TRAP = 0x10,      // Dangerous trap
    PF_UPDOWN = 0x20,    // Stairs, ramp etc.
};

constexpr pf_special operator | ( pf_special lhs, pf_special rhs )
{
    return static_cast<pf_special>( static_cast< int >( lhs ) | static_cast< int >( rhs ) );
}

constexpr pf_special operator & ( pf_special lhs, pf_special rhs )
{
    return static_cast<pf_special>( static_cast< int >( lhs ) & static_cast< int >( rhs ) );
}

inline pf_special &operator |= ( pf_special &lhs, pf_special rhs )
{
    lhs = static_cast<pf_special>( static_cast< int >( lhs ) | static_cast< int >( rhs ) );
    return lhs;
}

inline pf_special &operator &= ( pf_special &lhs, pf_special rhs )
{
    lhs = static_cast<pf_special>( static_cast< int >( lhs ) & static_cast< int >( rhs ) );
    return lhs;
}

struct pathfinding_cache {
    pathfinding_cache();
    ~pathfinding_cache();

    bool dirty;

    pf_special special[MAPSIZE * SEEX][MAPSIZE * SEEY];
};

struct pathfinding_settings {
    int bash_strength = 0;
    int max_dist = 0;
    // At least 2 times the above, usually more
    int max_length = 0;

    bool allow_open_doors = false;
    bool avoid_traps = false;

    bool allow_climb_stairs = true;

    pathfinding_settings() = default;
    pathfinding_settings( const pathfinding_settings & ) = default;
    pathfinding_settings( int bs, int md, int ml, bool aod, bool at, bool acs )
        : bash_strength( bs ), max_dist( md ), max_length( ml ), allow_open_doors( aod ),
          avoid_traps( at ), allow_climb_stairs( acs ) {}
};

#endif
