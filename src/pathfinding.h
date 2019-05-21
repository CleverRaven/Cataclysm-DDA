#pragma once
#ifndef PATHFINDING_H
#define PATHFINDING_H

#include "game_constants.h"

enum pf_special : char {
    PF_NORMAL = 0x00,    // Plain boring tile (grass, dirt, floor etc.)
    PF_SLOW = 0x01,      // Tile with move cost >2
    PF_WALL = 0x02,      // Unpassable ter/furn/vehicle
    PF_VEHICLE = 0x04,   // Any vehicle tile (passable or not)
    PF_FIELD = 0x08,     // Dangerous field
    PF_TRAP = 0x10,      // Dangerous trap
    PF_UPDOWN = 0x20,    // Stairs, ramp etc.
    PF_CLIMBABLE = 0x40, // 0 move cost but can be climbed on examine
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

    pf_special special[MAPSIZE_X][MAPSIZE_Y];
};

struct pathfinding_settings {
    int bash_strength = 0;
    int max_dist = 0;
    // At least 2 times the above, usually more
    int max_length = 0;

    // Expected terrain cost (2 is flat ground) of climbing a wire fence
    // 0 means no climbing
    int climb_cost = 0;

    bool allow_open_doors = false;
    bool avoid_traps = false;
    bool allow_climb_stairs = true;
    bool avoid_rough_terrain = false;

    pathfinding_settings() = default;
    pathfinding_settings( const pathfinding_settings & ) = default;
    pathfinding_settings( int bs, int md, int ml, int cc, bool aod, bool at, bool acs, bool art )
        : bash_strength( bs ), max_dist( md ), max_length( ml ), climb_cost( cc ),
          allow_open_doors( aod ), avoid_traps( at ), allow_climb_stairs( acs ), avoid_rough_terrain( art ) {}
};

#endif
