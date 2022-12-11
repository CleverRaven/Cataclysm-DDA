#ifndef CATA_SRC_JMAPGEN_FLAGS_H
#define CATA_SRC_JMAPGEN_FLAGS_H

enum class jmapgen_flags {
    allow_terrain_under_other_data,
    erase_all_before_placing_terrain,
    no_underlying_rotate,
    avoid_creatures,
    last
};

template<>
struct enum_traits<jmapgen_flags> {
    static constexpr jmapgen_flags last = jmapgen_flags::last;
};

#endif // CATA_SRC_JMAPGEN_FLAGS_H
