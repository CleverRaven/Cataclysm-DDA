#pragma once
#ifndef CATA_SRC_JMAPGEN_FLAGS_H
#define CATA_SRC_JMAPGEN_FLAGS_H

enum class jmapgen_flags : std::uint8_t {
    allow_terrain_under_other_data,
    dismantle_all_before_placing_terrain,
    erase_all_before_placing_terrain,
    allow_terrain_under_furniture,
    dismantle_furniture_before_placing_terrain,
    erase_furniture_before_placing_terrain,
    allow_terrain_under_trap,
    dismantle_trap_before_placing_terrain,
    erase_trap_before_placing_terrain,
    allow_terrain_under_items,
    erase_items_before_placing_terrain,
    no_underlying_rotate,
    avoid_creatures,
    last
};

template<>
struct enum_traits<jmapgen_flags> {
    static constexpr jmapgen_flags last = jmapgen_flags::last;
};

#endif // CATA_SRC_JMAPGEN_FLAGS_H
