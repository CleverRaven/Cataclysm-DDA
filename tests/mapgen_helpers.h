#ifndef CATA_TESTS_MAPGEN_HELPERS_H
#define CATA_TESTS_MAPGEN_HELPERS_H

#include <functional>

#include "coords_fwd.h"
#include "type_id.h"

class map;

using fprep_t = std::function<void( map &m )>;

void common_mapgen_prep( tripoint_abs_omt const &pos, fprep_t const &prep );
void manual_update_mapgen( tripoint_abs_omt const &pos, update_mapgen_id const &id );
void manual_nested_mapgen( tripoint_abs_omt const &pos, nested_mapgen_id const &id );

template<typename F, typename ID>
void manual_mapgen( tripoint_abs_omt const &where, F const &fmg, ID const &id,
                    fprep_t const &prep = {} )
{
    common_mapgen_prep( where, prep );
    fmg( where, id );
}

#endif // CATA_TESTS_MAPGEN_HELPERS_H
