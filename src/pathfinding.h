#pragma once
#ifndef CATA_SRC_PATHFINDING_H
#define CATA_SRC_PATHFINDING_H

#include <optional>

#include "creature.h"
#include "coords_fwd.h"
#include "game_constants.h"
#include "mdarray.h"
#include "character.h"

// An attribute of a particular map square that is of interest in pathfinding.
// Has a maximum of 32 members. For more, the datatype underlying PathfindingFlags
// needs to be increased.
enum class PathfindingFlag : uint8_t {
    Ground = 0,     // Can walk on
    Slow,           // Move cost > 2
    Swimmable,      // Can swim in
    Air,            // Empty air
    Unsheltered,    // Outside and above ground level
    Obstacle,       // Something stopping us, might be bashable, climbable, diggable, or openable.
    Bashable,       // Something bashable.
    Impassable,     // Impassable obstacle.
    Vehicle,        // Vehicle tile (passable or not)
    DangerousField, // Dangerous field
    DangerousTrap,  // Dangerous trap (i.e. not flagged benign)

    // TODO: Rename these to StairsUp and StairsDown.
    // TODO: Handle ladders, elevators, etc.
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


// Settings that affect either parameters of the pathfinding algorithm
// or fundamental assumptions about movement, e.g. vertical / horizontal movement.
// TODO: Support burrowing as a way to change Z-levels.
// TODO: Allow climbing as a way to change Z-levels.
// TODO: Support swimming as a way to change Z-levels.
// TODO: Support the CLIMB_FLYING flag (e.g. #75515).
class RealityBubblePathfindingSettings
{
    public:
        bool allow_flying() const {
            return allow_flying_;
        }
        void set_allow_flying( bool v = true ) {
            allow_flying_ = v;
        }

        bool allow_falling() const {
            return allow_falling_;
        }
        void set_allow_falling( bool v = true ) {
            allow_falling_ = v;
        }

        bool allow_stairways() const {
            return allow_stairways_;
        }
        void set_allow_stairways( bool v = true ) {
            allow_stairways_ = v;
        }

        int max_cost() const {
            return max_cost_;
        }
        void set_max_cost( int v = 0 ) {
            max_cost_ = v;
        }

        const tripoint_rel_ms &padding() const {
            return padding_;
        }
        void set_padding( const tripoint_rel_ms &v ) {
            padding_ = v;
        }

    private:
        bool allow_flying_ = false;
        bool allow_falling_ = false;
        bool allow_stairways_ = false;
        int max_cost_ = 0;
        tripoint_rel_ms padding_ = tripoint_rel_ms( 16, 16, 1 );
};


// Characteristics of pathfinding as they relate to how an individual creature
// reacts to an individual terrain tile. Attributes that relate to how a creature
// may react to movement that affects Z-Levels, or that act on parameters of the
// pathfinding algorithm (e.g. max_cost_ or padding), belong in RealityBubblePathfindingSettings.
//
// For example, a creature may want to avoid lava, unsheltered terrain, etc.
// Under the hood, a PathfindingFlags instance is used. This allows us to avoid
// any tile with an attribute corresponding to a PathfindingFlag.
//
// All values except avoid impassible initialise to false.
class PathfindingSettings
{
    public:
        static constexpr PathfindingFlags RoughTerrain = PathfindingFlag::Slow | PathfindingFlag::Obstacle |
                PathfindingFlag::Vehicle | PathfindingFlag::Sharp | PathfindingFlag::DangerousTrap;
        static constexpr PathfindingFlags AnySizeRestriction = PathfindingFlag::RestrictTiny |
                PathfindingFlag::RestrictSmall |
                PathfindingFlag::RestrictMedium | PathfindingFlag::RestrictLarge | PathfindingFlag::RestrictHuge;

        bool avoid_falling() const {
            return !rb_settings_.allow_falling();
        }
        void set_avoid_falling( bool v = true ) {
            rb_settings_.set_allow_falling( !v );
        }

        bool avoid_unsheltered() const {
            return is_set( PathfindingFlag::Unsheltered );
        }
        void set_avoid_unsheltered( bool v = true ) {
            set( PathfindingFlag::Unsheltered, v );
        }

        bool avoid_swimming() const {
            return is_set( PathfindingFlag::Swimmable );
        }
        void set_avoid_swimming( bool v = true ) {
            set( PathfindingFlag::Swimmable, v );
        }

        bool avoid_ground() const {
            return is_set( PathfindingFlag::Ground );
        }
        void set_avoid_ground( bool v = true ) {
            set( PathfindingFlag::Ground, v );
        }

        bool avoid_vehicle() const {
            return is_set( PathfindingFlag::Vehicle );
        }
        void set_avoid_vehicle( bool v = true ) {
            set( PathfindingFlag::Vehicle, v );
        }

        // This refers to climbing within the context of moving on the same
        // z-level, e.g. as over a fence.
        bool avoid_climbing() const {
            return avoid_climbing_;
        }
        void set_avoid_climbing( bool v = true ) {
            avoid_climbing_ = v;
            maybe_set_avoid_obstacle();
        }

        bool avoid_climb_stairway() const {
            return !rb_settings_.allow_stairways();
        }
        void set_avoid_climb_stairway( bool v = true ) {
            rb_settings_.set_allow_stairways( !v );
        }

        bool avoid_deep_water() const {
            return is_set( PathfindingFlag::DeepWater );
        }
        void set_avoid_deep_water( bool v = true ) {
            set( PathfindingFlag::DeepWater, v );
        }

        std::optional<creature_size> size_restriction() const {
            return size_restriction_;
        }
        PathfindingFlags size_restriction_mask() const {
            return size_restriction_mask_;
        }
        void set_size_restriction( std::optional<creature_size> size_restriction = std::nullopt );

        bool avoid_pits() const {
            return is_set( PathfindingFlag::Pit );
        }
        void set_avoid_pits( bool v = true ) {
            set( PathfindingFlag::Pit, v );
        }

        bool avoid_opening_doors() const {
            return avoid_opening_doors_;
        }
        void set_avoid_opening_doors( bool v = true ) {
            avoid_opening_doors_ = v;
            maybe_set_avoid_obstacle();
        }

        bool avoid_unlocking_doors() const {
            return avoid_unlocking_doors_;
        }
        void set_avoid_unlocking_doors( bool v = true ) {
            avoid_unlocking_doors_ = v;
        }

        bool avoid_rough_terrain() const {
            return is_set( RoughTerrain );
        }
        void set_avoid_rough_terrain( bool v = true ) {
            set( RoughTerrain, v );
        }

        bool avoid_dangerous_traps() const {
            return is_set( PathfindingFlag::DangerousTrap );
        }
        void set_avoid_dangerous_traps( bool v ) {
            set( PathfindingFlag::DangerousTrap, v );
        }

        bool avoid_sharp() const {
            return is_set( PathfindingFlag::Sharp );
        }
        void set_avoid_sharp( bool v = true ) {
            set( PathfindingFlag::Sharp, v );
        }

        bool avoid_lava() const {
            return is_set( PathfindingFlag::Lava );
        }
        void set_avoid_lava( bool v = true ) {
            set( PathfindingFlag::Lava, v );
        }

        bool avoid_hard_ground() const {
            return is_set( PathfindingFlag::HardGround );
        }
        void set_avoid_hard_ground( bool v = true ) {
            set( PathfindingFlag::HardGround, v );
        }

        bool avoid_dangerous_fields() const {
            return is_set( PathfindingFlag::DangerousField );
        }
        void set_avoid_dangerous_fields( bool v = true ) {
            set( PathfindingFlag::DangerousField, v );
        }

        const std::function<bool( const tripoint_bub_ms & )> &maybe_avoid_fn() const {
            return maybe_avoid_fn_;
        }
        void set_maybe_avoid_fn( std::function<bool( const tripoint_bub_ms & )> fn = nullptr ) {
            maybe_avoid_fn_ = std::move( fn );
        }

        int max_distance() const {
            return max_distance_;
        }
        void set_max_distance( int v = 0 ) {
            max_distance_ = v;
        }

        int max_cost() const {
            return rb_settings_.max_cost();
        }
        void set_max_cost( int v = 0 ) {
            rb_settings_.set_max_cost( v );
        }

        int climb_cost() const {
            return climb_cost_;
        }
        void set_climb_cost( int v = 0 ) {
            climb_cost_ = v;
            maybe_set_avoid_obstacle();
        }

        bool is_digging() const {
            return is_digging_;
        }
        void set_is_digging( bool v = true ) {
            is_digging_ = v;
            maybe_set_avoid_obstacle();
        }

        bool is_flying() const {
            return rb_settings_.allow_flying();
        }
        void set_is_flying( bool v = true ) {
            rb_settings_.set_allow_flying( v );
        }

        PathfindingFlags avoid_mask() const {
            return avoid_mask_;
        }

        bool avoid_bashing() const {
            return avoid_bashing_;
        }
        void set_avoid_bashing( bool v = true ) {
            avoid_bashing_ = v;
            maybe_set_avoid_obstacle();
        }

        int bash_strength() const {
            return bash_strength_;
        }
        void set_bash_strength( int v = 0 ) {
            bash_strength_ = v;
            if( v <= 0 ) {
                set_avoid_bashing( true );
            }
            maybe_set_avoid_obstacle();
        }
        int bash_rating_from_range( int min, int max ) const;

    protected:
        const RealityBubblePathfindingSettings &rb_settings() const {
            return rb_settings_;
        }

    private:
        bool is_set( PathfindingFlags flag ) const {
            return avoid_mask_.is_set( flag );
        }
        // Set the corresponding flags to true if v, and false if not v.
        void set( PathfindingFlags flags, bool v ) {
            if( v ) {
                avoid_mask_.set_union( flags );
            } else {
                avoid_mask_.set_clear( flags );
            }
        }

        void maybe_set_avoid_obstacle() {
            // Check if we can short circuit checking obstacles. Significantly faster if so.
            set( PathfindingFlag::Obstacle, !is_digging() && ( climb_cost() <= 0 || avoid_climbing() ) &&
                 avoid_opening_doors() && ( avoid_bashing() || bash_strength() <= 0 ) );
        }

        PathfindingFlags avoid_mask_ = PathfindingFlag::Impassable;

        std::optional<creature_size> size_restriction_;
        PathfindingFlags size_restriction_mask_;
        int max_distance_ = 0;

        bool avoid_bashing_ = false;
        int bash_strength_ = 0;

        // Expected terrain cost (2 is flat ground) of climbing a wire fence, 0 means no climbing
        bool avoid_climbing_ = false;
        int climb_cost_ = 0;

        bool is_digging_ = false;
        RealityBubblePathfindingSettings rb_settings_;

        bool avoid_opening_doors_ = false;
        bool avoid_unlocking_doors_ = false;
        std::function<bool( const tripoint_bub_ms & )> maybe_avoid_fn_;
};

#endif // CATA_SRC_PATHFINDING_H
