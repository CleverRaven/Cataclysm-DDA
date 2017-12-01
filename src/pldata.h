#pragma once
#ifndef PLDATA_H
#define PLDATA_H

#include "json.h"
#include "bodypart.h"
#include "string_id.h"
#include <map>
#include <string>

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

class addiction : public JsonSerializer, public JsonDeserializer
{
    public:
        add_type type      = ADD_NULL;
        int      intensity = 0;
        int      sated     = 600;

        addiction() = default;
        addiction( add_type const t, int const i = 1 ) : type {t}, intensity {i} { }

        using JsonSerializer::serialize;
        void serialize( JsonOut &json ) const override {
            json.start_object();
            json.member( "type_enum", type );
            json.member( "intensity", intensity );
            json.member( "sated", sated );
            json.end_object();
        }
        using JsonDeserializer::deserialize;
        void deserialize( JsonIn &jsin ) override {
            JsonObject jo = jsin.get_object();
            type = ( add_type )jo.get_int( "type_enum" );
            intensity = jo.get_int( "intensity" );
            sated = jo.get_int( "sated" );
        }
};

#endif
