#pragma once
#ifndef CATA_SRC_MATH_PARSER_DIAG_H
#define CATA_SRC_MATH_PARSER_DIAG_H

#include <functional>
#include <map>
#include <string_view>
#include <vector>

#include "math_parser_diag_value.h"

struct dialogue;
struct dialogue_func {
    dialogue_func( std::string_view sc_, int n_ ) :
        scopes( sc_ ), num_params( n_ ) {}
    std::string_view scopes;
    int num_params{};
};
struct dialogue_func_eval : dialogue_func {
    using f_t = std::function<double( dialogue & )> ( * )( char scope,
                std::vector<diag_value> const &, diag_kwargs const & );

    dialogue_func_eval( std::string_view sc_, int n_, f_t f_ )
        : dialogue_func( sc_, n_ ), f( f_ ) {}

    f_t f;
};

struct dialogue_func_ass : dialogue_func {
    using f_t = std::function<void( dialogue &, double )> ( * )( char scope,
                std::vector<diag_value> const &, diag_kwargs const & );

    dialogue_func_ass( std::string_view sc_, int n_, f_t f_ )
        : dialogue_func( sc_, n_ ), f( f_ ) {}

    f_t f;
};

using pdiag_func_eval = dialogue_func_eval const *;
using pdiag_func_ass = dialogue_func_ass const *;

using decl_diag_eval = std::function<double( dialogue & )> ( char scope,
                       std::vector<diag_value> const &params, diag_kwargs const &kwargs );
using decl_diag_ass = std::function<void( dialogue &, double )> ( char scope,
                      std::vector<diag_value> const &params, diag_kwargs const &kwargs );

decl_diag_eval addiction_intensity_eval;
decl_diag_eval addiction_turns_eval;
decl_diag_ass addiction_turns_ass;
decl_diag_eval armor_eval;
decl_diag_eval attack_speed_eval;
decl_diag_eval charge_count_eval;
decl_diag_eval coverage_eval;
decl_diag_eval distance_eval;
decl_diag_eval dodge_eval;
decl_diag_eval effect_intensity_eval;
decl_diag_eval encumbrance_eval;
decl_diag_eval field_strength_eval;
decl_diag_eval has_trait_eval;
decl_diag_eval knows_proficiency_eval;
decl_diag_eval hp_eval;
decl_diag_ass hp_ass;
decl_diag_eval hp_max_eval;
decl_diag_eval item_count_eval;
decl_diag_eval monsters_nearby_eval;
decl_diag_eval num_input_eval;
decl_diag_eval option_eval;
decl_diag_eval pain_eval;
decl_diag_ass pain_ass;
decl_diag_eval school_level_eval;
decl_diag_eval school_level_adjustment_eval;
decl_diag_ass school_level_adjustment_ass;
decl_diag_eval skill_eval;
decl_diag_ass skill_ass;
decl_diag_eval skill_exp_eval;
decl_diag_ass skill_exp_ass;
decl_diag_ass spellcasting_adjustment_ass;
decl_diag_eval spell_count_eval;
decl_diag_eval spell_exp_eval;
decl_diag_ass spell_exp_ass;
decl_diag_eval spell_level_eval;
decl_diag_ass spell_level_ass;
decl_diag_eval spell_level_adjustment_eval;
decl_diag_ass spell_level_adjustment_ass;
decl_diag_eval proficiency_eval;
decl_diag_ass proficiency_ass;
decl_diag_eval test_diag;
decl_diag_eval test_str_len;
decl_diag_eval u_val;
decl_diag_ass u_val_ass;
decl_diag_eval vitamin_eval;
decl_diag_ass vitamin_ass;
decl_diag_eval warmth_eval;
decl_diag_eval weather_eval;
decl_diag_ass weather_ass;

/*
General guidelines for writing dialogue functions

The typical parsing function takes the form:

std::function<double( dialogue & )> myfunction_eval( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &kwargs )
{
    diag_value myval( std::string{} );
    if( kwargs.count( "mykwarg" ) != 0 ) {
        myval = *kwargs.at( "mykwarg" );
    }

    ...parse-time code...

    return[effect_id = params[0], myval, beta = is_beta( scope )]( dialogue const & d ) {
        ...run-time code...
    };
}

- Don't validate the number of arguments (params). The math parser already does that
- Only use variadic functions if all arguments are treated the same way,
  regardless of how many there are (including zero)
- Use kwargs for optional arguments
- Prefer splitting functions instead of using mandatory kwargs
  ex: school_level() split from spell_level() instead of spell_level('school':blorg)
- Use parameter-less functions diag_value::str(), dbl(), and var() only at parse-time
- Use conversion functions diag_value::str( d ) and dbl( d ) only at run-time
- Always throw on errors at parse-time
- Never throw at run-time. Use a debugmsg() and recover gracefully
*/

// { "name", { "scopes", num_args, function } }
// kwargs are not included in num_args
inline std::map<std::string_view, dialogue_func_eval> const dialogue_eval_f{
    { "_test_diag_", { "g", -1, test_diag } },
    { "_test_str_len_", { "g", -1, test_str_len } },
    { "addiction_intensity", { "un", 1, addiction_intensity_eval } },
    { "addiction_turns", { "un", 1, addiction_turns_eval } },
    { "armor", { "un", 2, armor_eval } },
    { "attack_speed", { "un", 0, attack_speed_eval } },
    { "charge_count", { "un", 1, charge_count_eval } },
    { "coverage", { "un", 1, coverage_eval } },
    { "distance", { "g", 2, distance_eval } },
    { "effect_intensity", { "un", 1, effect_intensity_eval } },
    { "encumbrance", { "un", 1, encumbrance_eval } },
    { "field_strength", { "ung", 1, field_strength_eval } },
    { "game_option", { "g", 1, option_eval } },
    { "has_trait", { "un", 1, has_trait_eval } },
    { "has_proficiency", { "un", 1, knows_proficiency_eval } },
    { "hp", { "un", 1, hp_eval } },
    { "hp_max", { "un", 1, hp_max_eval } },
    { "item_count", { "un", 1, item_count_eval } },
    { "monsters_nearby", { "ung", -1, monsters_nearby_eval } },
    { "num_input", { "g", 2, num_input_eval } },
    { "pain", { "un", 0, pain_eval } },
    { "school_level", { "un", 1, school_level_eval}},
    { "school_level_adjustment", { "un", 1, school_level_adjustment_eval } },
    { "skill", { "un", 1, skill_eval } },
    { "skill_exp", { "un", 1, skill_exp_eval } },
    { "spell_count", { "un", 0, spell_count_eval}},
    { "spell_exp", { "un", 1, spell_exp_eval}},
    { "spell_level", { "un", 1, spell_level_eval}},
    { "spell_level_adjustment", { "un", 1, spell_level_adjustment_eval } },
    { "proficiency", { "un", 1, proficiency_eval } },
    { "val", { "un", -1, u_val } },
    { "vitamin", { "un", 1, vitamin_eval } },
    { "warmth", { "un", 1, warmth_eval } },
    { "weather", { "g", 1, weather_eval } },
};

inline std::map<std::string_view, dialogue_func_ass> const dialogue_assign_f{
    { "addiction_turns", { "un", 1, addiction_turns_ass } },
    { "hp", { "un", 1, hp_ass } },
    { "pain", { "un", 0, pain_ass } },
    { "school_level_adjustment", { "un", 1, school_level_adjustment_ass } },
    { "spellcasting_adjustment", { "u", 1, spellcasting_adjustment_ass } },
    { "skill", { "un", 1, skill_ass } },
    { "skill_exp", { "un", 1, skill_exp_ass } },
    { "spell_exp", { "un", 1, spell_exp_ass}},
    { "spell_level", { "un", 1, spell_level_ass}},
    { "spell_level_adjustment", { "un", 1, spell_level_adjustment_ass } },
    { "proficiency", { "un", 1, proficiency_ass } },
    { "val", { "un", -1, u_val_ass } },
    { "vitamin", { "un", 1, vitamin_ass } },
    { "weather", { "g", 1, weather_ass } },
};

#endif // CATA_SRC_MATH_PARSER_DIAG_H
