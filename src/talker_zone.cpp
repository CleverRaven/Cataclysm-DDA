#include <cmath>
#include <map>
#include <utility>
#include <vector>

#include "avatar.h"
#include "character.h"
#include "coordinates.h"
#include "effect.h"
#include "effect_source.h"
#include "map.h"
#include "math_parser_diag_value.h"
#include "talker_zone.h"
#include "tileray.h"
#include "units.h"
#include "clzones.h"

tripoint_abs_ms talker_zone_const::pos_abs() const
{
    return me_zone_const->get_start_point();
}

