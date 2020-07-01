#pragma once
#ifndef CATA_SRC_PLDATA_H
#define CATA_SRC_PLDATA_H

#include <string>

#include "calendar.h"

class JsonIn;
class JsonOut;
template <typename E> struct enum_traits;

enum class character_type : int {
    CUSTOM,
    RANDOM,
    TEMPLATE,
    NOW,
    FULL_RANDOM,
};

enum class add_type : int {
    NONE,
    CAFFEINE,
    ALCOHOL,
    SLEEP,
    PKILLER,
    SPEED,
    CIG,
    COKE,
    CRACK,
    MUTAGEN,
    DIAZEPAM,
    MARLOSS_R,
    MARLOSS_B,
    MARLOSS_Y,
    NUM_ADD_TYPES
};

template<>
struct enum_traits<add_type> {
    static constexpr add_type last = add_type::NUM_ADD_TYPES;
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
        add_type type = add_type::NONE;
        int intensity = 0;
        time_duration sated = 1_hours;

        addiction() = default;
        addiction( add_type const t, const int i = 1 ) : type {t}, intensity {i} { }

        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );
};

#endif // CATA_SRC_PLDATA_H
