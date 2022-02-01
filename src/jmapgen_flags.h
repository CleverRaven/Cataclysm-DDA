#ifndef CATA_SRC_JMAPGEN_FLAGS_H
#define CATA_SRC_JMAPGEN_FLAGS_H

enum class jmapgen_flags {
    allow_terrain_under_other_data,
    erase_all_before_placing_terrain,
    // just remove the items from the tile
    erase_items_before_placing_items,
    last
};

template<>
struct enum_traits<jmapgen_flags> {
    static constexpr jmapgen_flags last = jmapgen_flags::last;
};

#endif // CATA_SRC_JMAPGEN_FLAGS_H
