#ifndef CATA_SRC_BUILD_REQS_H
#define CATA_SRC_BUILD_REQS_H

#include <map>

#include "type_id.h"

struct build_reqs {
    std::map<skill_id, int> skills;
    std::map<requirement_id, int> reqs;
    int time = 0;

    void clear() {
        *this = build_reqs();
    }
};

void get_build_reqs_for_furn_ter_ids( const std::pair<std::map<ter_id, int>,
                                      std::map<furn_id, int>> &changed_ids,
                                      build_reqs &total_reqs );

#endif // CATA_SRC_BUILD_REQS_H
