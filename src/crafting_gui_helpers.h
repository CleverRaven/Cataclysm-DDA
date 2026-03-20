#pragma once
#ifndef CATA_SRC_CRAFTING_GUI_HELPERS_H
#define CATA_SRC_CRAFTING_GUI_HELPERS_H

#include "color.h"

class Character;
class inventory;
class recipe;

// Returns true if the character cannot gain any skill or proficiency from this recipe.
// Used to mark practice recipes as "useless" when the crafter already exceeds
// the recipe's skill cap and has all used proficiencies.
bool cannot_gain_skill_or_prof( const Character &crafter, const recipe &recp );

// Computes whether a Character can craft a given recipe.
// Stores craftability flags, color-coding, and lazy-cached proficiency maluses.
struct availability {
        explicit availability( Character &_crafter, const recipe *r, int batch_size = 1,
                               bool camp_crafting = false, inventory *inventory_override = nullptr );
        Character &crafter;
        bool can_craft;
        // group can introduce recipe this crafter cannot craft because of low primary skill
        bool crafter_has_primary_skill;
        bool would_use_rotten;
        bool would_use_favorite;
        bool useless_practice;
        bool apparently_craftable;
        bool has_proficiencies;
        bool has_all_skills;
        bool is_nested_category;
        // Used as an indicator to see if crafting is called via camp.
        // If not nullptr, we must be camp crafting.
        inventory *inv_override;
    private:
        const recipe *rec;
        mutable float proficiency_time_maluses = -1.0f;
        mutable float max_proficiency_time_maluses = -1.0f;
        mutable float proficiency_skill_maluses = -1.0f;
        mutable float max_proficiency_skill_maluses = -1.0f;
    public:
        float get_proficiency_time_maluses() const;
        float get_max_proficiency_time_maluses() const;
        float get_proficiency_skill_maluses() const;
        float get_max_proficiency_skill_maluses() const;

        nc_color selected_color() const;
        nc_color color( bool ignore_missing_skills = false ) const;

        static bool check_can_craft_nested( Character &_crafter, const recipe &r );
};

enum class craft_confirm_result {
    ok,
    cannot_craft,
    too_dark
};

// Checks craftability and lighting for the CONFIRM action.
// Does not check container eligibility.
craft_confirm_result can_start_craft(
    const recipe &rec,
    const availability &avail,
    const Character &crafter );

// Recipe sort comparator for the crafting menu recipe list.
// Comparison order:
//   1. Unread before read (when unread_first is true)
//   2. Craftable before uncraftable
//   3. Higher difficulty first
//   4. Alphabetical by result_name()
//   5. Longer craft time first (tiebreaker)
bool recipe_sort_compare(
    const recipe *a, const recipe *b,
    const availability &avail_a, const availability &avail_b,
    const Character &crafter,
    bool a_read, bool b_read,
    bool unread_first );

#endif // CATA_SRC_CRAFTING_GUI_HELPERS_H
