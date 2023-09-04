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
enum class segments : std::size_t {
    FAULTS = 0,
    DIRT,
    OVERHEAT,
    FAVORITE_PRE,
    DURABILITY,
    ENGINE_DISPLACEMENT,
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
using decl_fsegment = std::string( item const &it, unsigned int quantity,
                                   segment_bitset const &segments );
decl_fsegment faults;
decl_fsegment dirt_symbol;
decl_fsegment overheat_symbol;
decl_fsegment pre_asterisk;
decl_fsegment durability;
decl_fsegment engine_displacement;
decl_fsegment wheel_diameter;
decl_fsegment burn;
decl_fsegment label;
decl_fsegment category;
decl_fsegment mods;
decl_fsegment craft;
decl_fsegment wbl_mark;
decl_fsegment contents;
decl_fsegment contents_abrev;
decl_fsegment food_traits;
decl_fsegment location_hint;
decl_fsegment ethereal;
decl_fsegment food_status;
decl_fsegment food_irradiated;
decl_fsegment temperature;
decl_fsegment clothing_size;
decl_fsegment filthy;
decl_fsegment broken;
decl_fsegment cbm_status;
decl_fsegment ups;
decl_fsegment wetness;
decl_fsegment active;
decl_fsegment sealed;
decl_fsegment post_asterisk;
decl_fsegment weapon_mods;
decl_fsegment relic_charges;
decl_fsegment tags;
decl_fsegment vars;

inline std::string noop( item const & /* it */, unsigned int /* quantity */,
                         segment_bitset const & /* segments */ )
{
    return {};
}

inline std::unordered_map<segments, decl_fsegment *> const segment_map = {
    { segments::FAULTS, faults },
    { segments::DIRT, dirt_symbol },
    { segments::OVERHEAT, overheat_symbol },
    { segments::FAVORITE_PRE, pre_asterisk },
    { segments::DURABILITY, durability },
    { segments::ENGINE_DISPLACEMENT, engine_displacement },
    { segments::WHEEL_DIAMETER, wheel_diameter },
    { segments::BURN, burn },
    { segments::TYPE, label },
    { segments::CATEGORY, category },
    { segments::MODS, mods },
    { segments::CRAFT, craft },
    { segments::WHITEBLACKLIST, wbl_mark },
    { segments::CHARGES, noop },
    { segments::CONTENTS, contents },
    { segments::FOOD_TRAITS, food_traits },
    { segments::LOCATION_HINT, location_hint },
    { segments::ETHEREAL, ethereal },
    { segments::FOOD_STATUS, food_status },
    { segments::FOOD_IRRADIATED, food_irradiated },
    { segments::TEMPERATURE, temperature },
    { segments::CLOTHING_SIZE, clothing_size },
    { segments::FILTHY, filthy },
    { segments::BROKEN, broken },
    { segments::CBM_STATUS, cbm_status },
    { segments::UPS, ups },
    { segments::WETNESS, wetness },
    { segments::ACTIVE, active },
    { segments::SEALED, sealed },
    { segments::FAVORITE_POST, post_asterisk },
    { segments::WEAPON_MODS, weapon_mods },
    { segments::RELIC, relic_charges },
    { segments::LINK, noop },
    { segments::VARS, vars },
    { segments::TECHNIQUES, noop },
    { segments::TAGS, tags },
};

#endif // CATA_IN_TOOL
} // namespace tname

template<>
struct enum_traits<tname::segments> {
    static constexpr tname::segments last = tname::segments::last;
};

namespace tname
{
constexpr unsigned long long default_tname_bits =
    std::numeric_limits<unsigned long long>::max() &
    ~( 1ULL << static_cast<size_t>( tname::segments::CATEGORY ) );
constexpr unsigned long long tname_prefix_bits =
    1ULL << static_cast<size_t>( tname::segments::FAVORITE_PRE ) |
    1ULL << static_cast<size_t>( tname::segments::DURABILITY ) |
    1ULL << static_cast<size_t>( tname::segments::BURN );
constexpr unsigned long long tname_contents_bits =
    1ULL << static_cast<size_t>( tname::segments::CONTENTS ) |
    1ULL << static_cast<size_t>( tname::segments::CONTENTS_FULL ) |
    1ULL << static_cast<size_t>( tname::segments::CONTENTS_ABREV );
constexpr unsigned long long tname_conditional_bits =    // TODO: fine grain?
    1ULL << static_cast<size_t>( tname::segments::COMPONENTS ) |
    1ULL << static_cast<size_t>( tname::segments::TAGS ) |
    1ULL << static_cast<size_t>( tname::segments::VARS );
constexpr segment_bitset default_tname( default_tname_bits );
constexpr segment_bitset unprefixed_tname( default_tname_bits & ~tname_prefix_bits );
constexpr segment_bitset tname_contents( tname_contents_bits );
constexpr segment_bitset tname_conditional( tname_conditional_bits );

} // namespace tname

#endif // CATA_SRC_ITEM_TNAME_H
