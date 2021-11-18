#pragma once
#ifndef CATA_SRC_DIFFICULTY_IMPACT_H
#define CATA_SRC_DIFFICULTY_IMPACT_H

#include <string>

#include "json.h"

struct difficulty_impact {
    enum difficulty_option {
        DIFF_NONE,
        DIFF_VERY_EASY,
        DIFF_EASY,
        DIFF_MEDIUM,
        DIFF_HARD,
        DIFF_VERY_HARD
    };

    difficulty_option offence = DIFF_NONE;
    difficulty_option defence = DIFF_NONE;
    difficulty_option crafting = DIFF_NONE;
    difficulty_option wilderness = DIFF_NONE;
    difficulty_option social = DIFF_NONE;

    const std::string get_diff_desc( const difficulty_option &diff ) const;
    difficulty_option get_opt_from_str( const std::string &diff_str ) const;

    void load( const JsonObject &jo );
};

#endif // CATA_SRC_DIFFICULTY_IMPACT_H
