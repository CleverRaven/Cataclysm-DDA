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
// function type that prints an element of tname::segments
using decl_f_print_segment = std::string( item const &it, unsigned int quantity,
                             segment_bitset const &segments );
decl_f_print_segment faults;
decl_f_print_segment dirt_symbol;
decl_f_print_segment overheat_symbol;
decl_f_print_segment pre_asterisk;
decl_f_print_segment durability;
decl_f_print_segment engine_displacement;
decl_f_print_segment wheel_diameter;
decl_f_print_segment burn;
decl_f_print_segment label;
decl_f_print_segment category;
decl_f_print_segment mods;
decl_f_print_segment craft;
decl_f_print_segment wbl_mark;
decl_f_print_segment contents;
decl_f_print_segment contents_abrev;
decl_f_print_segment food_traits;
decl_f_print_segment location_hint;
decl_f_print_segment ethereal;
decl_f_print_segment food_status;
decl_f_print_segment food_irradiated;
decl_f_print_segment temperature;
decl_f_print_segment clothing_size;
decl_f_print_segment filthy;
decl_f_print_segment broken;
decl_f_print_segment cbm_status;
decl_f_print_segment ups;
decl_f_print_segment wetness;
decl_f_print_segment active;
decl_f_print_segment sealed;
decl_f_print_segment post_asterisk;
decl_f_print_segment weapon_mods;
decl_f_print_segment relic_charges;
decl_f_print_segment tags;
decl_f_print_segment vars;

inline std::string noop( item const & /* it */, unsigned int /* quantity */,
                         segment_bitset const & /* segments */ )
{
    return {};
}

inline std::unordered_map<segments, decl_f_print_segment *> const segment_map = {
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
