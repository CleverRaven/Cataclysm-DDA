#pragma once
#ifndef CATA_SRC_CALENDAR_UI_H
#define CATA_SRC_CALENDAR_UI_H

#include <string_view>

#include "calendar.h"
#include "translations.h"

namespace calendar_ui
{

enum class granularity : int {
    year,
    season,
    day,
    hour,
    minute,
    turn,
    last,
};

/**
 * Displays ui element that allows to select and return time point.
 */
time_point select_time_point( time_point initial_value,
                              std::string_view title = _( "Select time point" ),
                              calendar_ui::granularity granularity_level = calendar_ui::granularity::turn );
} // namespace calendar_ui

#endif // CATA_SRC_CALENDAR_UI_H
