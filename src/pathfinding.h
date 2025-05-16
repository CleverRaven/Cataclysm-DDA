#pragma once
#ifndef CATA_SRC_PATHFINDING_H
#define CATA_SRC_PATHFINDING_H

#include <cstdint>
#include <optional>
#include <unordered_set>

#include "coordinates.h"
#include "mdarray.h"
#include "point.h"

enum class creature_size : int;

// An attribute of a particular map square that is of interest in pathfinding.
// Has a maximum of 32 members. For more, the datatype underlying PathfindingFlags
// needs to be increased.
enum class PathfindingFlag : uint8_t {
    Ground = 0,     // Can walk on
    Slow,           // Move cost > 2
    Swimmable,      // Can swim in
    Air,            // Empty air
    Unsheltered,    // Outside and above ground level
    Obstacle,       // Something stopping us, might be bashable.
    Bashable,       // Something bashable.
    Impassable,     // Impassable obstacle.
    Vehicle,        // Vehicle tile (passable or not)
    DangerousField, // Dangerous field
    DangerousTrap,  // Dangerous trap (i.e. not flagged benign)
    GoesUp,         // Valid stairs up
    GoesDown,       // Valid stairs down
    RampUp,         // Valid ramp up
    RampDown,       // Valid ramp down
    Climbable,      // Obstacle but can be climbed on examine
    Sharp,          // Sharp items (barbed wire, etc)
    Door,           // A door (any kind)
    InsideDoor,     // A door that can be opened from the inside only
    LockedDoor,     // A locked door
    Pit,            // A pit you can fall into / climb out of.
    DeepWater,      // Deep water.
    Burrowable,     // Can burrow into
    HardGround,     // Can not dig & burrow intotiny = 1,
    RestrictTiny,   // Tiny cannot enter
    RestrictSmall,  // Small cannot enter
    RestrictMedium, // Medium cannot enter
    RestrictLarge,  // Large cannot enter
    RestrictHuge,   // Huge cannot enter
    Lava,           // Lava terrain
};

class PathfindingFlags
{
    public:
        constexpr PathfindingFlags() = default;

        // NOLINTNEXTLINE(google-explicit-constructor)
        constexpr PathfindingFlags( PathfindingFlag flag ) : flags_( uint32_t{ 1 } << static_cast<uint8_t>
                    ( flag ) ) {}

        constexpr void set_union( PathfindingFlags flags ) {
            flags_ |= flags.flags_;
        }
        constexpr void set_intersect( PathfindingFlags flags ) {
            flags_ &= flags.flags_;
        }
        constexpr void set_clear( PathfindingFlags flags ) {
            flags_ &= ~flags.flags_;
        }

        constexpr void clear() {
            flags_ = 0;
        }

        constexpr bool is_set( PathfindingFlag flag ) const {
            return flags_ & ( uint32_t{ 1 } << static_cast<uint8_t>( flag ) );
        }
        constexpr bool is_set( PathfindingFlags flags ) const {
            return ( flags_ & flags.flags_ ) == flags.flags_;
        }

        constexpr bool is_any_set() const {
            return flags_;
        }

        constexpr explicit operator bool() const {
            return is_any_set();
        }

        constexpr PathfindingFlags &operator|=( PathfindingFlags flags ) {
            set_union( flags );
            return *this;
        }

        constexpr PathfindingFlags &operator&=( PathfindingFlags flags ) {
            set_intersect( flags );
            return *this;
        }

    private:
        uint32_t flags_ = 0;
};

constexpr PathfindingFlags operator|( PathfindingFlags lhs, PathfindingFlags rhs )
{
    return lhs |= rhs;
}

constexpr PathfindingFlags operator&( PathfindingFlags lhs, PathfindingFlags rhs )
{
    return lhs &= rhs;
}

constexpr PathfindingFlags operator|( PathfindingFlags lhs, PathfindingFlag rhs )
{
    return lhs |= rhs;
}

constexpr PathfindingFlags operator&( PathfindingFlags lhs, PathfindingFlag rhs )
{
    return lhs &= rhs;
}

constexpr PathfindingFlags operator |( const PathfindingFlag &a, const PathfindingFlag &b )
{
    return PathfindingFlags( a ) | PathfindingFlags( b );
}

struct pathfinding_cache {
    pathfinding_cache();

    bool dirty = false;
    std::unordered_set<point_bub_ms> dirty_points;

    cata::mdarray<PathfindingFlags, point_bub_ms> special;
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
    bool allow_unlock_doors = false;
    bool avoid_traps = false;
    bool allow_climb_stairs = true;
    bool avoid_rough_terrain = false;
    bool avoid_sharp = false;
    bool avoid_dangerous_fields = false;

    std::optional<creature_size> size = std::nullopt;

    pathfinding_settings() = default;
    pathfinding_settings( const pathfinding_settings & ) = default;

    pathfinding_settings( int bs, int md, int ml, int cc, bool aod, bool aud, bool at, bool acs,
                          bool art, bool as, std::optional<creature_size> sz = std::nullopt )
        : bash_strength( bs ), max_dist( md ), max_length( ml ), climb_cost( cc ),
          allow_open_doors( aod ), allow_unlock_doors( aud ), avoid_traps( at ), allow_climb_stairs( acs ),
          avoid_rough_terrain( art ), avoid_sharp( as ), size( sz )  {}

    pathfinding_settings &operator=( const pathfinding_settings & ) = default;
};

struct pathfinding_target {
    const tripoint_bub_ms center;
    const int r;
    bool contains( const tripoint_bub_ms &p ) const;

    // Finds a path that ends on a specific tile
    static pathfinding_target point( const tripoint_bub_ms &p ) {
        return { p, 0 };
    }

    // Finds a path that ends on either the given tile, or one of the tiles directly adjacent to it
    static pathfinding_target adjacent( const tripoint_bub_ms &p ) {
        return { p, 1 };
    }

    // Finds a path that ends on any tile within the given radius of the specified tile, calculated by square distance
    static pathfinding_target radius( const tripoint_bub_ms &p, int radius ) {
        return { p, radius };
    }
};

#endif // CATA_SRC_PATHFINDING_H
