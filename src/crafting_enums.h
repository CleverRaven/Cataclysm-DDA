#pragma once
#ifndef CATA_SRC_CRAFTING_ENUMS_H
#define CATA_SRC_CRAFTING_ENUMS_H

#include <optional>

#include "calendar.h"

enum class step_attention : int {
    none = 0,
    unattended,
    supervised,
};

enum class step_choice : int {
    do_wait = 0,
    do_something,
    set_timer,
};

// Per-step choice from the planning modal.
struct attention_plan {
    step_choice choice = step_choice::do_wait;
    std::optional<time_duration> alarm_offset;
};

#endif // CATA_SRC_CRAFTING_ENUMS_H
