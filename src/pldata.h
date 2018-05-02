#pragma once
#ifndef PLDATA_H
#define PLDATA_H

#include "bodypart.h"
#include "string_id.h"
#include "calendar.h"

#include <map>
#include <string>

class JsonIn;
class JsonOut;

class martialart;
using matype_id = string_id<martialart>;

class ma_buff;
using mabuff_id = string_id<ma_buff>;

class ma_technique;
using matec_id = string_id<ma_technique>;

struct mutation_branch;
using trait_id = string_id<mutation_branch>;

typedef std::string dis_type;

enum character_type : int {
    PLTYPE_CUSTOM,
    PLTYPE_RANDOM,
    PLTYPE_TEMPLATE,
    PLTYPE_NOW,
    PLTYPE_FULL_RANDOM,
};

enum add_type : int {
    ADD_NULL,
    ADD_CAFFEINE, ADD_ALCOHOL, ADD_SLEEP, ADD_PKILLER, ADD_SPEED, ADD_CIG,
    ADD_COKE, ADD_CRACK, ADD_MUTAGEN, ADD_DIAZEPAM, ADD_MARLOSS_R, ADD_MARLOSS_B,
    ADD_MARLOSS_Y,
};

enum hp_part : int {
    hp_head = 0,
    hp_torso,
    hp_arm_l,
    hp_arm_r,
    hp_leg_l,
    hp_leg_r,
    num_hp_parts
};

class addiction
{
    public:
        add_type type      = ADD_NULL;
        int      intensity = 0;
        time_duration sated = 1_hours;

        addiction() = default;
        addiction( add_type const t, int const i = 1 ) : type {t}, intensity {i} { }

        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );
};

#endif
