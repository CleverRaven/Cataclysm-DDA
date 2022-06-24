#pragma once
#ifndef CATA_SRC_EFFECT_ON_CONDITION_H
#define CATA_SRC_EFFECT_ON_CONDITION_H

#include <string>
#include <climits>

#include "calendar.h"
#include "condition.h"
#include "dialogue.h"
#include "json.h"
#include "optional.h"
#include "type_id.h"

template<typename T>
class generic_factory;
enum eoc_type {
    ACTIVATION,
    RECURRING,
    SCENARIO_SPECIFIC,
    AVATAR_DEATH,
    NPC_DEATH,
    OM_MOVE,
    NUM_EOC_TYPES
};
struct effect_on_condition {
    public:
        friend class generic_factory<effect_on_condition>;
        bool was_loaded = false;
        /* If this is true each npc will have their own copy of it.  Deactivate conditions for traits/mutations/bionics will work but not weather.  */
        bool run_for_npcs = false;
        /* If this is true it will be run on the player and every npc.  Deactivate conditions will work based on the player.  */
        bool global = false;
        effect_on_condition_id id;
        std::vector<std::pair<effect_on_condition_id, mod_id>> src;
        eoc_type type;
        std::function<bool( const dialogue & )> condition;
        std::function<bool( const dialogue & )> deactivate_condition;
        talk_effect_t<dialogue> true_effect;
        talk_effect_t<dialogue> false_effect;
        bool has_deactivate_condition = false;
        bool has_condition = false;
        bool has_false_effect = false;
        duration_or_var<dialogue> recurrence;
        bool activate( dialogue &d ) const;
        bool check_deactivate( dialogue &d ) const;
        bool test_condition( dialogue &d ) const;
        void apply_true_effects( dialogue &d ) const;
        void load( const JsonObject &jo, const std::string &src );
        void finalize();
        void check() const;
        effect_on_condition() = default;
};
namespace effect_on_conditions
{
/** Get all currently loaded effect_on_conditions */
const std::vector<effect_on_condition> &get_all();
/** Finalize all loaded effect_on_conditions */
void finalize_all();
/** Clear all loaded effects on condition (invalidating any pointers) */
void reset();
/** Load effect on condition from JSON definition */
void load( const JsonObject &jo, const std::string &src );
/** Checks all loaded from JSON are valid */
void check_consistency();
/** Sets up the initial queue for a new character */
void load_new_character( Character &you );
/** Load any new eocs that don't exist in the save. */
void load_existing_character( Character &you );
/** Loads an inline eoc */
effect_on_condition_id load_inline_eoc( const JsonValue &jv, const std::string &src );
/** queue an eoc to happen in the future */
void queue_effect_on_condition( time_duration duration, effect_on_condition_id eoc,
                                Character &you );
/** called every turn to process the queued eocs */
void process_effect_on_conditions( Character &you );
/** called after certain events to test whether to reactivate eocs */
void process_reactivate( Character &you );
void process_reactivate();
/** clear all queued and inactive eocs */
void clear( Character &you );
/** write out all queued eocs and inactive eocs to a file for testing */
void write_eocs_to_file( Character &you );
void write_global_eocs_to_file();
/** Run all avatar death eocs */
void avatar_death();
/** Run all OM_MOVE eocs */
void om_move();
} // namespace effect_on_conditions

template<>
struct enum_traits<eoc_type> {
    static constexpr eoc_type last = eoc_type::NUM_EOC_TYPES;
};
#endif // CATA_SRC_EFFECT_ON_CONDITION_H
