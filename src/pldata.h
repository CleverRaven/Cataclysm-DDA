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

typedef std::string dis_type;

enum character_type : int {
    PLTYPE_CUSTOM,
    PLTYPE_RANDOM,
    PLTYPE_RANDOM_WITH_SCENARIO,
    PLTYPE_TEMPLATE,
    PLTYPE_NOW,
    PLTYPE_MAX
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

void realDebugmsg( const char *name, const char *line, const char *mes, ... );

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

struct mutation_category_trait {
    std::string name;
    std::string id;
    std::string category; // Mutation catagory i.e "BIRD", "CHIMERA"
    std::string mutagen_message; // message when you consume mutagen
    int mutagen_hunger  = 10;//these are defaults
    int mutagen_thirst  = 10;
    int mutagen_pain    = 2;
    int mutagen_fatigue = 5;
    int mutagen_morale  = 0;
    std::string iv_message; //message when you inject an iv;
    int iv_min_mutations    = 1; //the minimum mutations an injection provides
    int iv_additional_mutations = 2;
    int iv_additional_mutations_chance = 3; //chance of additional mutations
    int iv_hunger   = 10;
    int iv_thirst   = 10;
    int iv_pain     = 2;
    int iv_fatigue  = 5;
    int iv_morale   = 0;
    int iv_morale_max = 0;
    bool iv_sound = false;  //determines if you make a sound when you inject mutagen
    std::string iv_sound_message = "NULL";
    int iv_noise = 0;    //the amount of noise produced by the sound
    bool iv_sleep = false;  //whether the iv has a chance of knocking you out.
    std::string iv_sleep_message = "NULL";
    int iv_sleep_dur = 0;
    std::string junkie_message;
    std::string memorial_message; //memorial message when you cross a threshold

    mutation_category_trait( std::string pid = "NULL_TRAIT" ) : name( pid ), id( std::move( pid ) ) {}
};

extern std::map<std::string, mutation_category_trait> mutation_category_traits;

bool trait_display_sort( const std::string &a, const std::string &b ) noexcept;

#endif
