#pragma once
#ifndef CATA_SRC_BUILD_REQS_H
#define CATA_SRC_BUILD_REQS_H

#include <map>
#include <unordered_map>
#include <vector>

#include "mapdata.h"
#include "mapgendata.h"
#include "requirements.h"
#include "type_id.h"

struct build_reqs {
    std::map<skill_id, int> skills;
    std::map<requirement_id, int> raw_reqs;
    int time = 0;

    requirement_data consolidated_reqs;

    template<typename Reqs1, typename Reqs2>
    void consolidate( const Reqs1 &add_reqs1, const Reqs2 &add_reqs2 ) {
        consolidated_reqs = requirement_data{ raw_reqs };
        auto add_reqs = [&]( const auto & r ) {
            consolidated_reqs = std::accumulate( r.begin(), r.end(), consolidated_reqs );
        };
        add_reqs( add_reqs1 );
        add_reqs( add_reqs2 );
        consolidated_reqs.consolidate();
    }

    void clear() {
        *this = build_reqs();
    }
};

struct parameterized_build_reqs {
    std::unordered_map<mapgen_arguments, build_reqs> reqs_by_parameters;
};

build_reqs get_build_reqs_for_furn_ter_ids(
    const std::pair<std::map<ter_id, int>, std::map<furn_id, int>> &changed_ids );

#endif // CATA_SRC_BUILD_REQS_H
