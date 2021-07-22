#pragma once
#ifndef CATA_SRC_EFFECT_ON_CONDITION_H
#define CATA_SRC_EFFECT_ON_CONDITION_H

#include <string>
#include <climits>

#include "calendar.h"
#include "dialogue.h"
#include "json.h"
#include "optional.h"
#include "type_id.h"

template<typename T>
class generic_factory;

struct queued_eoc {
    public:
        effect_on_condition_id eoc;
        bool recurring = false;
        time_point time;
};

struct eoc_compare {
    bool operator()( const queued_eoc &lhs, const queued_eoc &rhs ) const {
        return lhs.time > rhs.time;
    }
};

struct effect_on_condition {
    public:
        friend class generic_factory<effect_on_condition>;
        bool was_loaded = false;
        effect_on_condition_id id;

        std::function<bool( const dialogue & )> condition;
        std::function<bool( const dialogue & )> deactivate_condition;
        talk_effect_t true_effect;
        talk_effect_t false_effect;
        bool has_deactivate_condition = false;
        bool has_condition = false;
        bool has_false_effect = false;
        bool activate_only = true;

        time_duration recurrence_min = 1_seconds;
        time_duration recurrence_max = 1_seconds;
        bool activate( dialogue &d ) const;
        bool check_deactivate() const;
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
void load_new_character();
/** Loads an inline eoc */
effect_on_condition_id load_inline_eoc( const JsonValue &jv, const std::string &src );
/** queue an eoc to happen in the future */
void queue_effect_on_condition( time_duration duration, effect_on_condition_id eoc );
/** called every turn to process the queued eocs */
void process_effect_on_conditions();
/** called after certain events to test whether to reactivate eocs */
void process_reactivate();
/** clear all queued and inactive eocs */
void clear();
/** write out all queued eocs and inactive eocs to a file for testing */
void write_eocs_to_file();
} // namespace effect_on_conditions

#endif // CATA_SRC_EFFECT_ON_CONDITION_H
