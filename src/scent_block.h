#pragma once
#ifndef CATA_SRC_SCENT_BLOCK_H
#define CATA_SRC_SCENT_BLOCK_H

#include <array>

#include "coordinates.h"
#include "map_scale_constants.h"
#include "point.h"

class scent_map;

struct scent_block {
    template<typename T>
    using data_block = std::array < std::array < T, SEEY + 2 >, SEEX + 2 >;

    enum class data_mode : int {
        NONE = 0,
        SET = 1,
        MAX = 2
    };

    struct datum {
        data_mode mode;
        int intensity;
    };
    data_block<datum> assignment;

    tripoint_bub_ms origin;
    scent_map &scents;
    int modification_count;

    scent_block( const tripoint_bub_sm &sub, scent_map &scents );

    void commit_modifications();

    point_rel_ms index( const tripoint_bub_ms &p ) const {
        return p.xy() - origin.xy();
    }

    // We should be working entirely within the range, so don't range check here
    void apply_gas( const tripoint_bub_ms &p, int nintensity = 0 );
    void apply_slime( const tripoint_bub_ms &p, int intensity );
};

#endif // CATA_SRC_SCENT_BLOCK_H
