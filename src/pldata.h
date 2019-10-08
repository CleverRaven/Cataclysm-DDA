#pragma once
#ifndef PLDATA_H
#define PLDATA_H

#include <string>

#include "calendar.h"
#include "enum_traits.h"

class JsonIn;
class JsonOut;

using dis_type = std::string;

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
    NUM_ADD_TYPES // last
};

template<>
struct enum_traits<add_type> {
    static constexpr add_type last = NUM_ADD_TYPES;
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

template<>
struct enum_traits<hp_part> {
    static constexpr hp_part last = num_hp_parts;
};

class addiction
{
    public:
        add_type type      = ADD_NULL;
        int      intensity = 0;
        time_duration sated = 1_hours;

        addiction() = default;
        addiction( add_type const t, const int i = 1 ) : type {t}, intensity {i} { }

        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );
};

#endif
