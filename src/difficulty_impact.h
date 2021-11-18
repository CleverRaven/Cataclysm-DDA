#pragma once
#ifndef CATA_SRC_DIFFICULTY_IMPACT_H
#define CATA_SRC_DIFFICULTY_IMPACT_H

#include <string>

#include "json.h"

struct difficulty_impact {
    enum difficulty_option {
        DIFF_NONE      = 0,
        DIFF_VERY_EASY = 1,
        DIFF_EASY      = 2,
        DIFF_MEDIUM    = 3,
        DIFF_HARD      = 4,
        DIFF_VERY_HARD = 5
    };

    difficulty_option offence = DIFF_NONE;
    difficulty_option defence = DIFF_NONE;
    difficulty_option crafting = DIFF_NONE;
    difficulty_option wilderness = DIFF_NONE;
    difficulty_option social = DIFF_NONE;

    static std::string get_diff_desc( const difficulty_option &diff );
    difficulty_option get_opt_from_str( const std::string &diff_str ) const;

    void load( const JsonObject &jo );
};

#endif // CATA_SRC_DIFFICULTY_IMPACT_H
