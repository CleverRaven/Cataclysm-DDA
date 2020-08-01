#pragma once
#ifndef CATA_SRC_EFFECT_ON_CONDITION_H
#define CATA_SRC_EFFECT_ON_CONDITION_H

#include <string>
#include <climits>

#include "calendar.h"
#include "dialogue.h"
#include "json.h"
#include "type_id.h"

template<typename T>
class generic_factory;

struct effect_on_condition {
    public:
        friend class generic_factory<effect_on_condition>;
        bool was_loaded = false;
        effect_on_condition_id id;
        json_dynamic_line_effect jdle;
        std::function<bool( const dialogue & )> deactivate_condition;
        bool has_deactivate_condition = false;
        time_duration once_every = 1_seconds;
        bool activate() const;
        bool check_deactivate() const;
        void load( const JsonObject &jo, const std::string &src );
        void finalize();
        void check() const;
        effect_on_condition() = default;
};
namespace effect_on_conditions
{
/** Get all currently loaded generic_preconditions */
const std::vector<effect_on_condition> &get_all();
/** Finalize all loaded generic_preconditions */
void finalize_all();
/** Clear all loaded effects on condition (invalidating any pointers) */
void reset();
/** Load effect on condition from JSON definition */
void load( const JsonObject &jo, const std::string &src );
/** Checks all loaded from JSON are valid */
void check_consistency();
void queue_effect_on_condition( time_duration duration, effect_on_condition_id eoc );
void process_effect_on_conditions();
void process_reactivate();
} // namespace effect_on_conditions

#endif // CATA_SRC_EFFECT_ON_CONDITION_H
