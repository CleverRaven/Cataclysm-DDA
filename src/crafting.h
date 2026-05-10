#pragma once
#ifndef CATA_SRC_CRAFTING_H
#define CATA_SRC_CRAFTING_H

#include <list>
#include <optional>
#include <string>
#include <vector>

#include "item_wakeup.h"

class Character;
class item;
class item_location;
class recipe;
class time_point;
struct attention_plan;
template <typename E> struct enum_traits;

enum class craft_flags : int {
    none = 0,
    start_only = 1, // Only require 5% (plus remainder) of tool charges
};

template<>
struct enum_traits<craft_flags> {
    static constexpr bool is_flag_enum = true;
};

// removes any (removable) ammo from the item and stores it in the
// players inventory.
void remove_ammo( item &dis_item, Character &p );
// same as above but for each item in the list
void remove_ammo( std::list<item> &dis_items, Character &p );

void drop_or_handle( const item &newit, Character &p );

// Asks the avatar what to do at each remaining unattended step (skipping
// any step before `from_step`).  Returns nullopt if the avatar cancelled.
// `existing` pre-fills choices on resume.
std::optional<std::vector<attention_plan>> show_craft_planning_modal(
        const recipe &rec, const Character &crafter, int batch, int from_step,
        const std::vector<attention_plan> &existing );

// Interrupts the avatar's current activity with a craft_step_complete
// distraction when the avatar can read time; falls back to a vague flavor
// message otherwise.  Suppresses entirely if the avatar is already engaged
// on this same craft.
void fire_step_complete_distraction( const std::string &interrupt_msg,
                                     const std::string &no_watch_msg,
                                     const item_location &loc );

// Wakeup hooks for in-progress craft items.  Called by item_wakeup
// dispatchers when the item is a craft.
std::vector<desired_wakeup> craft_enumerate_scheduled_wakeups(
    const item &craft, const item_location &loc );
void craft_actualize_scheduled( item &craft, item_wakeup_kind kind,
                                time_point now, const item_location &loc );

// Inline ready-transition for past-due passive steps: when the wakeup did not
// dispatch (off-bubble drop, save reload, same-turn race), this advances the
// step or finalizes the craft so callers do not see stale current_step.
// No-op when not in a passive step or when ready_at is still in the future.
void craft_resolve_overdue_passive( item &craft, time_point now, item_location &loc );

// Re-arms or clears alarm_at on an already-entered passive step when the
// player has just edited that step's plan via the resume modal.  Without
// this, switching to or away from set_timer mid-step has no effect.
void craft_apply_resume_replan( item_location &loc );

// Stamps passive-step runtime state (passive_started_at, ready_at, alarm_at,
// fail_at, counter bounds) on a craft sitting at the start of an unattended
// step, then rebuilds wakeups.  Called from the actor's first-entry path and
// from craft_resolve_overdue_passive when chaining through consecutive
// unattended steps.
void craft_stamp_passive_entry( item &craft, const Character &crafter,
                                time_point now, const item_location &loc );

#endif // CATA_SRC_CRAFTING_H
