#include "pathfinding.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <optional>
#include <queue>
#include <set>
#include <utility>
#include <vector>

#include "cata_utility.h"
#include "coordinates.h"
#include "debug.h"
#include "game.h"
#include "gates.h"
#include "line.h"
#include "map.h"
#include "mapdata.h"
#include "point.h"
#include "submap.h"
#include "trap.h"
#include "type_id.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"

static const ter_id ter_t_pit = ter_id("t_pit");
static const ter_id ter_t_pit_spiked = ter_id("t_pit_spiked");
static const ter_id ter_t_pit_glass = ter_id("t_pit_glass");
static const ter_id ter_t_lava = ter_id("t_lava");

RealityBubblePathfindingCache::RealityBubblePathfindingCache()
{
    for (int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; ++z) {
        dirty_z_levels_.emplace(z);
    }
}

// Modifies `t` to point to a tile with `flag` in a 1-submap radius of `t`'s original value, searching
// nearest points first (starting with `t` itself).
// Returns false if it could not find a suitable point
bool RealityBubblePathfindingCache::vertical_move_destination(const map& here, ter_furn_flag flag,
    tripoint& t) const
{
    const int z = t.z;
    if (const std::optional<point> p = find_point_closest_first(t.xy(), 0, SEEX, [&here, flag,
        z](const point& p) {
            if (p.x >= 0 && p.x < MAPSIZE_X && p.y >= 0 && p.y < MAPSIZE_Y) {
                const tripoint t2(p, z);
                return here.has_flag(flag, t2);
            }
            return false;
        })) {
        t = tripoint(*p, z);
        return true;
    }
    return false;
}

void RealityBubblePathfindingCache::update(const map& here, int min_z, int max_z)
{
    if (dirty_z_levels_.empty() && dirty_positions_.empty()) {
        return;
    }

    cata_assert(min_z <= max_z);

    const int size = here.getmapsize();
    for (const int z : dirty_z_levels_) {
        if (z < min_z || z > max_z || !here.inbounds_z(z)) {
            continue;
        }
        for (int y = 0; y < size * SEEY; ++y) {
            for (int x = 0; x < size * SEEX; ++x) {
                const tripoint_bub_ms p(x, y, z);
                invalidate_dependants(p);
                update(here, p);
            }
        }
    }
    for (const int z : dirty_z_levels_) {
        if (min_z <= z && z <= max_z) {
            dirty_positions_.erase(z);
        }
    }

    for (const auto& [z, dirty_points] : dirty_positions_) {
        if (z < min_z || z > max_z || !here.inbounds_z(z)) {
            continue;
        }
        for (const point_bub_ms& p : dirty_points) {
            tripoint_bub_ms t(p, z);
            if (here.inbounds(t)) {
                update(here, t);
            }
        }
    }

    for (int i = min_z; i <= max_z; ++i) {
        dirty_z_levels_.erase(i);
        dirty_positions_.erase(i);
    }
}

void RealityBubblePathfindingCache::update(const map& here, const tripoint_bub_ms& p)
{
    PathfindingFlags flags;

    const const_maptile& tile = here.maptile_at(p);
    const ter_t& terrain = tile.get_ter_t();
    const ter_id terrain_id = tile.get_ter();
    const furn_t& furniture = tile.get_furn_t();
    const optional_vpart_position veh = here.veh_at(p);

    const int orig_cost = here.move_cost(p);
    const int cost = orig_cost < 0 ? 0 : orig_cost;

    if (cost > 2) {
        flags |= PathfindingFlag::Slow;
    }

    if (!terrain.has_flag(ter_furn_flag::TFLAG_BURROWABLE) &&
        !terrain.has_flag(ter_furn_flag::TFLAG_DIGGABLE)) {
        flags |= PathfindingFlag::HardGround;
    }

    if (veh) {
        flags |= PathfindingFlag::Vehicle;
    }

    bool try_to_bash = false;
    bool impassable = flags.is_set(PathfindingFlag::HardGround);
    if (cost == 0) {
        flags |= PathfindingFlag::Obstacle;
        try_to_bash = true;
        if (terrain.open || furniture.open) {
            impassable = false;
            flags |= PathfindingFlag::Door;

            if (terrain.has_flag(ter_furn_flag::TFLAG_OPENCLOSE_INSIDE) ||
                furniture.has_flag(ter_furn_flag::TFLAG_OPENCLOSE_INSIDE)) {
                flags |= PathfindingFlag::InsideDoor;
            }
        }

        if (veh) {
            if (const auto vpobst = veh->obstacle_at_part()) {
                const int vpobst_i = vpobst->part_index();
                const vehicle& v = veh->vehicle();
                const int open_inside = v.next_part_to_open(vpobst_i, false);
                if (open_inside != -1) {
                    impassable = false;
                    flags |= PathfindingFlag::Door;

                    const int open_outside = v.next_part_to_open(vpobst_i, true);
                    if (open_inside != open_outside) {
                        flags |= PathfindingFlag::InsideDoor;
                    }
                    const int lock = v.next_part_to_unlock(vpobst_i, false);
                    if (lock != -1) {
                        flags |= PathfindingFlag::LockedDoor;
                    }
                }
            }
        }

        if (terrain.has_flag(ter_furn_flag::TFLAG_CLIMBABLE)) {
            impassable = false;
            flags |= PathfindingFlag::Climbable;
        }
    }
    else {
        impassable = false;
        // Size restrictions only apply to otherwise passable tiles.
        if (terrain.has_flag(ter_furn_flag::TFLAG_SMALL_PASSAGE)) {
            flags |= PathfindingFlag::RestrictLarge | PathfindingFlag::RestrictHuge;
        }
        if (veh) {
            if (auto cargo = veh->cargo(); cargo && !cargo->has_feature("CARGO_PASSABLE")) {
                const units::volume free_volume = cargo->items().free_volume();
                if (free_volume < 11719_ml) {
                    flags |= PathfindingFlag::RestrictTiny;
                }
                if (free_volume < 23438_ml) {
                    flags |= PathfindingFlag::RestrictSmall;
                }
                if (free_volume < 46875_ml) {
                    flags |= PathfindingFlag::RestrictMedium;
                }
                if (free_volume < 93750_ml) {
                    flags |= PathfindingFlag::RestrictLarge;
                }
                if (free_volume < 187500_ml) {
                    flags |= PathfindingFlag::RestrictHuge;
                }
            }
        }

        if (flags & PathfindingSettings::AnySizeRestriction) {
            try_to_bash = true;
        }
    }

    if (try_to_bash && here.is_bashable(p.raw())) {
        if (const auto bash_range = here.bash_range(p.raw())) {
            flags |= PathfindingFlag::Bashable;
            impassable = false;
            bash_range_ref(p) = *bash_range;
        }
    }

    if (impassable) {
        flags |= PathfindingFlag::Impassable;
    }

    if (cost > std::numeric_limits<char>::max()) {
        debugmsg("Tile move cost too large for cache: %s, %d", terrain_id.id().str(), cost);
    }
    else {
        move_cost_ref(p) = cost < 2 ? 2 : cost;
    }

    if (terrain.has_flag(ter_furn_flag::TFLAG_NO_FLOOR)) {
        flags |= PathfindingFlag::Air;
    }
    else if (terrain.has_flag(ter_furn_flag::TFLAG_SWIMMABLE)) {
        flags |= PathfindingFlag::Swimmable;
    }
    else {
        flags |= PathfindingFlag::Ground;
    }

    const bool has_vehicle_floor = here.has_vehicle_floor(p);
    if (!has_vehicle_floor) {
        if (terrain_id == ter_t_pit || terrain_id == ter_t_pit_spiked || terrain_id == ter_t_pit_glass) {
            flags |= PathfindingFlag::Pit;
        }

        if (terrain_id == ter_t_lava) {
            flags |= PathfindingFlag::Lava;
        }

        if (terrain.has_flag(ter_furn_flag::TFLAG_DEEP_WATER)) {
            flags |= PathfindingFlag::DeepWater;
        }

        if (!tile.get_trap_t().is_benign()) {
            flags |= PathfindingFlag::DangerousTrap;
        }

        if (terrain.has_flag(ter_furn_flag::TFLAG_SHARP)) {
            flags |= PathfindingFlag::Sharp;
        }
    }

    if (terrain.has_flag(ter_furn_flag::TFLAG_BURROWABLE)) {
        flags |= PathfindingFlag::Burrowable;
    }

    if (!g->is_sheltered(p.raw())) {
        flags |= PathfindingFlag::Unsheltered;
    }

    for (const auto& fld : tile.get_field()) {
        const field_entry& cur = fld.second;
        if (cur.is_dangerous()) {
            flags |= PathfindingFlag::DangerousField;
            break;
        }
    }

    if (p.z() < OVERMAP_HEIGHT) {
        up_destinations_.erase(p);
        const tripoint_bub_ms up(p.xy(), p.z() + 1);
        if (terrain.has_flag(ter_furn_flag::TFLAG_GOES_UP)) {
            if (std::optional<tripoint> dest = g->find_stairs(here, up.z(), p.raw())) {
                if (vertical_move_destination(here, ter_furn_flag::TFLAG_GOES_DOWN, *dest)) {
                    tripoint_bub_ms d(*dest);
                    flags |= PathfindingFlag::GoesUp;
                    up_destinations_.emplace(p, d);
                    dependants_by_position_[d].push_back(p);
                }
            }
            else {
                dependants_by_position_[up].push_back(p);
            }
        }

        if (terrain.has_flag(ter_furn_flag::TFLAG_RAMP) ||
            terrain.has_flag(ter_furn_flag::TFLAG_RAMP_UP)) {
            dependants_by_position_[up].push_back(p);
            if (here.valid_move(p.raw(), up.raw(), false, true, true)) {
                flags |= PathfindingFlag::RampUp;
            }
        }
    }

    if (p.z() > -OVERMAP_DEPTH) {
        down_destinations_.erase(p);
        const tripoint_bub_ms down(p.xy(), p.z() - 1);
        if (terrain.has_flag(ter_furn_flag::TFLAG_GOES_DOWN)) {
            if (std::optional<tripoint> dest = g->find_stairs(here, down.z(), p.raw())) {
                if (vertical_move_destination(here, ter_furn_flag::TFLAG_GOES_UP, *dest)) {
                    tripoint_bub_ms d(*dest);
                    flags |= PathfindingFlag::GoesDown;
                    down_destinations_.emplace(p, d);
                    dependants_by_position_[d].push_back(p);
                }
            }
            else {
                dependants_by_position_[down].push_back(p);
            }
        }

        if (terrain.has_flag(ter_furn_flag::TFLAG_RAMP_DOWN)) {
            dependants_by_position_[down].push_back(p);
            if (here.valid_move(p.raw(), down.raw(), false, true, true)) {
                flags |= PathfindingFlag::RampDown;
            }
        }
    }

    flags_ref(p) = flags;
}

void PathfindingSettings::set_size_restriction(std::optional<creature_size> size_restriction)
{
    size_restriction_mask_.clear();
    if (size_restriction) {
        switch (*size_restriction) {
        case creature_size::tiny:
            size_restriction_mask_ = PathfindingFlag::RestrictTiny;
            break;
        case creature_size::small:
            size_restriction_mask_ = PathfindingFlag::RestrictSmall;
            break;
        case creature_size::medium:
            size_restriction_mask_ = PathfindingFlag::RestrictMedium;
            break;
        case creature_size::large:
            size_restriction_mask_ = PathfindingFlag::RestrictLarge;
            break;
        case creature_size::huge:
            size_restriction_mask_ = PathfindingFlag::RestrictHuge;
            break;
        default:
            break;
        }
    }
    size_restriction_ = size_restriction;
}

int PathfindingSettings::bash_rating_from_range(int min, int max) const
{
    if (avoid_bashing_) {
        return 0;
    }
    // TODO: Move all the bash stuff to map so this logic isn't duplicated.
    ///\EFFECT_STR increases smashing damage
    if (bash_strength_ < min) {
        return 0;
    }
    else if (bash_strength_ >= max) {
        return 10;
    }
    const double ret = (10.0 * (bash_strength_ - min)) / (max - min);
    // Round up to 1, so that desperate NPCs can try to bash down walls
    return std::max(ret, 1.0);
}

std::pair<RealityBubblePathfinder::FastTripointSet::NotIterator, bool>
RealityBubblePathfinder::FastTripointSet::emplace(const tripoint_bub_ms& p)
{
    const int z = p.z() + OVERMAP_DEPTH;
    dirty_[z] = true;
    // Note that this is a reference into the bitset, despite not being a reference.
    // NOLINTNEXTLINE(cata-almost-never-auto)
    auto ref = set_[z][p.y() * MAPSIZE_X + p.x()];
    const bool missing = !ref;
    ref = true;
    return std::make_pair(NotIterator(), missing);
}

void RealityBubblePathfinder::FastTripointSet::clear()
{
    for (int z = 0; z < OVERMAP_LAYERS; ++z) {
        if (!dirty_[z]) {
            continue;
        }
        dirty_[z] = false;
        set_[z].reset();
    }
}

std::pair<std::pair<int, tripoint_bub_ms>*, bool>
RealityBubblePathfinder::FastBestPathMap::try_emplace(const tripoint_bub_ms& child, int cost,
    const tripoint_bub_ms& parent)
{
    std::pair<int, tripoint_bub_ms>& result = best_states_[child.z() +
        OVERMAP_DEPTH][child.y()][child.x()];
    if (const auto [_, inserted] = in_.emplace(child); inserted) {
        result.first = cost;
        result.second = parent;
        return std::make_pair(&result, true);
    }
    return std::make_pair(&result, false);
}

namespace
{

    constexpr int one_axis = 50;
    constexpr int two_axis = 71;
    constexpr int three_axis = 87;

    int adjacent_octile_distance(const tripoint_bub_ms& from, const tripoint_bub_ms& to)
    {
        switch (std::abs(from.x() - to.x()) + std::abs(from.y() - to.y()) + std::abs(
            from.z() - to.z())) {
        case 1:
            return one_axis;
        case 2:
            return two_axis;
        case 3:
            return three_axis;
        default:
            return 0;
        }
    }

    int octile_distance(const tripoint_bub_ms& from, const tripoint_bub_ms& to)
    {
        const tripoint d(std::abs(from.x() - to.x()), std::abs(from.y() - to.y()),
            std::abs(from.z() - to.z()));
        const int min = std::min(d.x, std::min(d.y, d.z));
        const int max = std::max(d.x, std::max(d.y, d.z));
        const int mid = d.x + d.y + d.z - min - max;
        return (three_axis - two_axis) * min + (two_axis - one_axis) * mid + one_axis * max;
    }

    int adjacent_distance_metric(const tripoint_bub_ms& from, const tripoint_bub_ms& to)
    {
        return trigdist ? adjacent_octile_distance(from, to) : 50 * square_dist(from, to);
    }

    int distance_metric(const tripoint_bub_ms& from, const tripoint_bub_ms& to)
    {
        return trigdist ? octile_distance(from, to) : 50 * square_dist(from, to);
    }

    std::optional<int> position_cost(const map& here, const tripoint_bub_ms& p,
        const PathfindingSettings& settings, const RealityBubblePathfindingCache& cache)
    {
        const PathfindingFlags flags = cache.flags(p);
        if (flags & settings.avoid_mask()) {
            return std::nullopt;
        }

        std::optional<int> cost;
        if (flags & (PathfindingFlag::Obstacle | PathfindingSettings::AnySizeRestriction)) {
            if (flags.is_set(PathfindingFlag::Obstacle)) {
                if (settings.is_digging()) {
                    if (!flags.is_set(PathfindingFlag::Burrowable)) {
                        return std::nullopt;
                    }
                    cost = 0;
                }
                else {
                    if (flags.is_set(PathfindingFlag::Door) && !settings.avoid_opening_doors() &&
                        (!flags.is_set(PathfindingFlag::LockedDoor) || !settings.avoid_unlocking_doors())) {
                        const int this_cost = flags.is_set(PathfindingFlag::LockedDoor) ? 4 : 2;
                        cost = std::min(this_cost, cost.value_or(this_cost));
                    }
                    if (flags.is_set(PathfindingFlag::Climbable) && !settings.avoid_climbing() &&
                        settings.climb_cost() > 0) {
                        // Climbing fences
                        const int this_cost = settings.climb_cost();
                        cost = std::min(this_cost, cost.value_or(this_cost));
                    }
                }
            }

            if (flags.is_set(PathfindingFlag::Bashable)) {
                const auto [bash_min, bash_max] = cache.bash_range(p);
                const int bash_rating = settings.bash_rating_from_range(bash_min, bash_max);
                if (bash_rating >= 1) {
                    // Expected number of turns to bash it down
                    const int this_cost = 20 / bash_rating;
                    cost = std::min(this_cost, cost.value_or(this_cost));
                }
            }

            // Don't check size restrictions if we can bash it down, open it, or climb over it.
            if (!cost && (flags & PathfindingSettings::AnySizeRestriction)) {
                // Any tile with a size restriction is passable otherwise.
                if (flags & settings.size_restriction_mask()) {
                    return std::nullopt;
                }
                cost = 0;
            }

            if (!cost) {
                // Can't enter the tile at all.
                return std::nullopt;
            }
        }
        else {
            cost = 0;
        }

        const auto& maybe_avoid_dangerous_fields_fn = settings.maybe_avoid_dangerous_fields_fn();
        if (flags.is_set(PathfindingFlag::DangerousField) && maybe_avoid_dangerous_fields_fn) {
            const field& target_field = here.field_at(p.raw());
            for (const auto& dfield : target_field) {
                if (dfield.second.is_dangerous() && maybe_avoid_dangerous_fields_fn(dfield.first)) {
                    return std::nullopt;
                }
            }
        }

        const auto& maybe_avoid_fn = settings.maybe_avoid_fn();
        if (maybe_avoid_fn && maybe_avoid_fn(p)) {
            return std::nullopt;
        }

        return *cost * 50;
    }

    std::optional<int> transition_cost(const map& here, const tripoint_bub_ms& from,
        const tripoint_bub_ms& to, const PathfindingSettings& settings,
        const RealityBubblePathfindingCache& cache)
    {
        const PathfindingFlags from_flags = cache.flags(from);
        const bool is_falling = from_flags.is_set(PathfindingFlag::Air) && !settings.is_flying();
        if (is_falling) {
            // Can only fall straight down.
            if (from.z() < to.z() || from.xy() != to.xy()) {
                return std::nullopt;
            }
        }

        const bool is_vertical_movement = from.z() != to.z();
        if (!is_falling && is_vertical_movement) {
            const tripoint_bub_ms& upper = from.z() > to.z() ? from : to;
            const tripoint_bub_ms& lower = from.z() < to.z() ? from : to;
            if (cache.flags(lower).is_set(PathfindingFlag::GoesUp) &&
                cache.flags(upper).is_set(PathfindingFlag::GoesDown)) {
                if (settings.avoid_climb_stairway()) {
                    return std::nullopt;
                }
                // Stairs can teleport us, so we need to use non-adjacent calc.
                return 2 * distance_metric(from, to);
            }
            else if (settings.is_flying()) {
                const tripoint_bub_ms above_lower(lower.xy(), lower.z() + 1);
                if (!(cache.flags(upper).is_set(PathfindingFlag::Air) ||
                    cache.flags(above_lower).is_set(PathfindingFlag::Air))) {
                    return std::nullopt;
                }
            }
            else if (to.z() > from.z()) {
                if (!cache.flags(to).is_set(PathfindingFlag::RampDown)) {
                    return std::nullopt;
                }
            }
            else if (to.z() < from.z()) {
                if (!cache.flags(to).is_set(PathfindingFlag::RampUp)) {
                    return std::nullopt;
                }
            }
        }

        const PathfindingFlags to_flags = cache.flags(to);
        if (to_flags.is_set(PathfindingFlag::Obstacle)) {
            // Can't interact with obstacles across z-levels.
            // TODO: allow this
            if (is_vertical_movement) {
                return std::nullopt;
            }
            if (!settings.is_digging()) {
                if (to_flags.is_set(PathfindingFlag::Door) && !settings.avoid_opening_doors() &&
                    (!to_flags.is_set(PathfindingFlag::LockedDoor) || !settings.avoid_unlocking_doors())) {
                    const bool is_inside_door = to_flags.is_set(PathfindingFlag::InsideDoor);
                    if (is_inside_door) {
                        int dummy;
                        const bool is_vehicle = to_flags.is_set(PathfindingFlag::Vehicle);
                        const bool is_outside = is_vehicle ? here.veh_at_internal(from.raw(),
                            dummy) != here.veh_at_internal(to.raw(), dummy) : here.is_outside(from.raw());
                        if (is_outside) {
                            return std::nullopt;
                        }
                    }
                }
            }
        }
        else if (to_flags.is_set(PathfindingFlag::Air)) {
            // This is checking horizontal movement only.
            if (settings.avoid_falling() && !settings.is_flying()) {
                return std::nullopt;
            }
        }

        // TODO: Move the move cost cache into map so this logic isn't duplicated.
        const int mult = adjacent_distance_metric(from, to);

        // Flying monsters aren't slowed down by terrain.
        if (settings.is_flying()) {
            return 2 * mult;
        }

        const int cost = cache.move_cost(from) + cache.move_cost(to);
        return static_cast<unsigned int>(mult * cost) / 2;
    }

}  // namespace


bool map::can_teleport(const tripoint_bub_ms& to, const PathfindingSettings& settings) const
{
    if (!inbounds(to)) {
        return false;
    }
    pathfinding_cache()->update(*this, to.z(), to.z());
    return position_cost(*this, to, settings, *pathfinding_cache()).has_value();
}

bool map::can_move(const tripoint_bub_ms& from, const tripoint_bub_ms& to,
    const PathfindingSettings& settings) const
{
    if (!inbounds(from) || !inbounds(to)) {
        return false;
    }
    if (from == to) {
        return true;
    }
    pathfinding_cache()->update(*this, std::min(from.z(), to.z()), std::max(from.z(), to.z()));
    if (position_cost(*this, to, settings, *pathfinding_cache()).has_value()) {
        return transition_cost(*this, from, to, settings, *pathfinding_cache()).has_value();
    }
    return false;
}

std::optional<int> map::move_cost(const tripoint_bub_ms& from, const tripoint_bub_ms& to,
    const PathfindingSettings& settings) const
{
    if (!inbounds(from) || !inbounds(to)) {
        return std::nullopt;
    }
    if (from == to) {
        return 0;
    }
    pathfinding_cache()->update(*this, std::min(from.z(), to.z()), std::max(from.z(), to.z()));
    if (const std::optional<int> p_cost = position_cost(*this, to, settings, *pathfinding_cache())) {
        if (const std::optional<int> t_cost = transition_cost(*this, from, to, settings,
            *pathfinding_cache())) {
            return *p_cost + *t_cost;
        }
    }
    return std::nullopt;
}

std::vector<tripoint_bub_ms> map::straight_route(const tripoint_bub_ms& from,
    const tripoint_bub_ms& to,
    const PathfindingSettings& settings) const
{

    if (from == to || !inbounds(from) || !inbounds(to)) {
        return {};
    }

    RealityBubblePathfindingCache& cache = *pathfinding_cache();
    const int pad = settings.rb_settings().padding().z();
    cache.update(*this, std::min(from.z(), to.z()) - pad, std::max(from.z(), to.z()) + pad);

    std::vector<tripoint_bub_ms> line_path = line_to(from, to);
    const PathfindingFlags avoid = settings.avoid_mask() | PathfindingSettings::RoughTerrain;
    // Check all points for all fast avoidance.
    if (!std::any_of(line_path.begin(), line_path.end(), [&cache, avoid](const tripoint_bub_ms& p) {
        return cache.flags(p) & avoid;
        })) {
        // Now do the slow check. Check if all the positions are valid.
        if (std::all_of(line_path.begin(), line_path.end(), [this, &cache,
            &settings](const tripoint_bub_ms& p) {
                return position_cost(*this, p, settings, cache);
            })) {
            // Now check that all the transitions between each position are valid.
            const tripoint_bub_ms* prev = &from;
            if (std::find_if_not(line_path.begin(), line_path.end(), [this, &prev, &cache,
                &settings](const tripoint_bub_ms& p) {
                    return transition_cost(*this, *std::exchange(prev, &p), p, settings, cache).has_value();
                }) == line_path.end()) {
                return line_path;
            }
        }
    }
    return {};
}

std::vector<tripoint_bub_ms> map::route(const tripoint_bub_ms& from, const tripoint_bub_ms& to,
    const PathfindingSettings& settings) const
{
    if (from == to || !inbounds(from) || !inbounds(to)) {
        return {};
    }

    RealityBubblePathfindingCache& cache = *pathfinding_cache();
    const int pad = settings.rb_settings().padding().z();
    cache.update(*this, std::min(from.z(), to.z()) - pad, std::max(from.z(), to.z()) + pad);

    // First, check for a simple straight line on flat ground.
    if (from.z() == to.z()) {
        std::vector<tripoint_bub_ms> line_path = straight_route(from, to, settings);
        if (!line_path.empty()) {
            return line_path;
        }
    }

    // If expected path length is greater than max distance, allow only line path, like above
    if (rl_dist(from, to) > settings.max_distance()) {
        return {};
    }

    return pathfinder()->find_path(settings.rb_settings(), from, to,
        [this, &settings, &cache](const tripoint_bub_ms& p) {
            return position_cost(*this, p, settings, cache);
        },
        [this, &settings, &cache](const tripoint_bub_ms& from, const tripoint_bub_ms& to) {
            return transition_cost(*this, from, to, settings, cache);
        },
        [](const tripoint_bub_ms& from, const tripoint_bub_ms& to) {
            return 2 * distance_metric(from, to);
        });
}

std::vector<tripoint> map::route(const tripoint& f, const tripoint& t,
    const pathfinding_settings& settings)
{
    const PathfindingSettings pf_settings = settings.to_new_pathfinding_settings();
    // TODO: Get rid of this.
    const tripoint_bub_ms from = tripoint_bub_ms::make_unchecked(f);
    const tripoint_bub_ms to = tripoint_bub_ms::make_unchecked(t);
    std::vector<tripoint_bub_ms> bub_route = route(from, to, pf_settings);
    std::vector<tripoint> ret(bub_route.size());
    for (tripoint_bub_ms p : bub_route)
    {
        ret.push_back(p.raw());
    }
    return ret;  
}

std::vector<tripoint_bub_ms> map::route(const tripoint_bub_ms& f, const tripoint_bub_ms& t,
    const pathfinding_settings& settings) const
{
    const PathfindingSettings pf_settings = settings.to_new_pathfinding_settings();
    return route(f, t, pf_settings);
}

PathfindingSettings pathfinding_settings::to_new_pathfinding_settings() const
{
    PathfindingSettings settings;
    settings.set_bash_strength(bash_strength);
    settings.set_max_distance(max_dist);
    settings.set_max_cost(max_length * 50);
    settings.set_climb_cost(climb_cost);
    settings.set_avoid_opening_doors(!allow_open_doors);
    settings.set_avoid_unlocking_doors(!allow_unlock_doors);
    settings.set_avoid_dangerous_traps(avoid_traps);
    if (avoid_rough_terrain) {
        settings.set_avoid_rough_terrain(true);
    }
    settings.set_avoid_sharp(avoid_sharp);
    return settings;
}
