#include "coordinates.h"
#include "talker_zone.h"
#include "clzones.h"

/**
 * @brief Gets the absolute position of the zone.
 * 
 * @return tripoint_abs_ms The absolute position of the zone's start point.
 */
tripoint_abs_ms talker_zone_const::pos_abs() const
{
    return me_zone_const->get_start_point();
}

