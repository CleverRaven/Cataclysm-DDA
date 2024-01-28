#pragma once
#ifndef CATA_SRC_ITEM_TNAME_H
#define CATA_SRC_ITEM_TNAME_H

#include <cstddef>
#include <string>
#include <unordered_map>

#include "enum_bitset.h"
#include "enum_traits.h"

class item;

namespace tname
{
// segments for item::tname()
// Each element corresponds to a piece of information about an item and is displayed in the item's name
// Each element is checked individually for stacking in item::stacks_with(), and all elements much stack for any 2 items to stack
enum class segments : std::size_t {
    FAULTS = 0,
    DIRT,
    OVERHEAT,
    FAVORITE_PRE,
    DURABILITY,
    WHEEL_DIAMETER,
    BURN,
    WEAPON_MODS,
    TYPE,
    CATEGORY,
    MODS,
    CRAFT,
    WHITEBLACKLIST,
    CHARGES,
    FOOD_TRAITS,
    FOOD_STATUS,
    FOOD_IRRADIATED,
    TEMPERATURE,
    LOCATION_HINT,
    CLOTHING_SIZE,
    ETHEREAL,
    FILTHY,
    BROKEN,
    CBM_STATUS,
    UPS,
    TAGS,
    VARS,
    WETNESS,
    ACTIVE,
    SEALED,
    FAVORITE_POST,
    RELIC,
    LINK,
    TECHNIQUES,
    CONTENTS,

    last_segment,

    // separate flags for TYPE
    VARIANT,
    COMPONENTS,
    CORPSE,

    // separate flags for CONTENTS
    CONTENTS_FULL,
    CONTENTS_ABREV,

    // separate flags for CATEGORY
    FOOD_PERISHABLE,

    last,
};

using segment_bitset = enum_bitset<tname::segments>;

#ifndef CATA_IN_TOOL

std::string print_segment( tname::segments segment, item const &it, unsigned int quantity,
                           segment_bitset const &segments );

#endif // CATA_IN_TOOL
} // namespace tname

template<>
struct enum_traits<tname::segments> {
    static constexpr tname::segments last = tname::segments::last;
};

namespace tname
{
constexpr uint64_t default_tname_bits =
    std::numeric_limits<uint64_t>::max() &
    ~( 1ULL << static_cast<size_t>( tname::segments::CATEGORY ) );
constexpr uint64_t tname_prefix_bits =
    1ULL << static_cast<size_t>( tname::segments::FAVORITE_PRE ) |
    1ULL << static_cast<size_t>( tname::segments::DURABILITY ) |
    1ULL << static_cast<size_t>( tname::segments::BURN );
constexpr uint64_t tname_contents_bits =
    1ULL << static_cast<size_t>( tname::segments::CONTENTS ) |
    1ULL << static_cast<size_t>( tname::segments::CONTENTS_FULL ) |
    1ULL << static_cast<size_t>( tname::segments::CONTENTS_ABREV );
constexpr uint64_t tname_conditional_bits =    // TODO: fine grain?
    1ULL << static_cast<size_t>( tname::segments::COMPONENTS ) |
    1ULL << static_cast<size_t>( tname::segments::TAGS ) |
    1ULL << static_cast<size_t>( tname::segments::VARS );
constexpr segment_bitset default_tname( default_tname_bits );
constexpr segment_bitset unprefixed_tname( default_tname_bits & ~tname_prefix_bits );
constexpr segment_bitset tname_contents( tname_contents_bits );
constexpr segment_bitset tname_conditional( tname_conditional_bits );

} // namespace tname

#endif // CATA_SRC_ITEM_TNAME_H
