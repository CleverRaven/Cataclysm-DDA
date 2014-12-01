#ifndef PLDATA_H
#define PLDATA_H

#include "enums.h"
#include "json.h"
#include "translations.h"
#include "bodypart.h"
#include <vector>
#include <map>
#include <climits>

typedef std::string matype_id;

typedef std::string mabuff_id;

typedef std::string matec_id;

typedef std::string efftype_id;

enum character_type {
    PLTYPE_CUSTOM,
    PLTYPE_RANDOM,
    PLTYPE_RANDOM_WITH_SCENARIO,
    PLTYPE_TEMPLATE,
    PLTYPE_NOW,
    PLTYPE_MAX
};

typedef std::string dis_type;

enum add_type {
    ADD_NULL,
    ADD_CAFFEINE, ADD_ALCOHOL, ADD_SLEEP, ADD_PKILLER, ADD_SPEED, ADD_CIG,
    ADD_COKE, ADD_CRACK, ADD_MUTAGEN, ADD_DIAZEPAM, ADD_MARLOSS_R, ADD_MARLOSS_B,
    ADD_MARLOSS_Y,
};

void realDebugmsg(const char *name, const char *line, const char *mes, ...);

class disease : public JsonSerializer, public JsonDeserializer
{
    public:
        dis_type type;
        int intensity;
        int duration;
        body_part bp;
        bool permanent;
        int decay;

        // extra stuff for martial arts, kind of a hack for now
        std::string buff_id;
        disease(std::string new_buff_id)
        {
            type = "ma_buff";
            buff_id = new_buff_id;
            intensity = 1;
        }
        bool is_mabuff()
        {
            return (buff_id != "" && type == "ma_buff");
        }

        disease() : type("null")
        {
            duration = 0;
            intensity = 0;
            bp = num_bp;
            permanent = false;
            decay = 0;
        }
        disease(dis_type t, int d, int i = 0, body_part part = num_bp, bool perm = false, int dec = 0) :
            type(t)
        {
            duration = d;
            intensity = i;
            bp = part;
            permanent = perm;
            decay = dec;
        }

        using JsonSerializer::serialize;
        void serialize(JsonOut &json) const
        {
            json.start_object();
            json.member("type", type);
            json.member("intensity", intensity);
            json.member("duration", duration);
            json.member("bp", (int)bp);
            json.member("permanent", permanent);
            json.member("decay", decay);
            json.member("ma_buff_id", buff_id);
            json.end_object();
        }
        using JsonDeserializer::deserialize;
        void deserialize(JsonIn &jsin)
        {
            JsonObject jo = jsin.get_object();
            type = jo.get_string("type");
            intensity = jo.get_int("intensity");
            duration = jo.get_int("duration");

            int tmp_bp = jo.get_int("bp");
            if (jo.has_member("side")) {
                int side = jo.get_int("side");
                if (side == 0) {
                    switch (tmp_bp) {
                    case 4:
                        break; //Already 4
                    case 5:
                        tmp_bp = 6;
                        break;
                    case 6:
                        tmp_bp = 8;
                        break;
                    case 7:
                        tmp_bp = 10;
                        break;
                    default:
                        break;
                    }
                } else if (side == 1) {
                    switch (tmp_bp) {
                    case 4:
                        tmp_bp = 5;
                        break;
                    case 5:
                        tmp_bp = 7;
                        break;
                    case 6:
                        tmp_bp = 9;
                        break;
                    case 7:
                        tmp_bp = 11;
                        break;
                    default:
                        break;
                    }
                }
            }
            bp = (body_part)tmp_bp;

            permanent = jo.get_bool("permanent");
            decay = jo.get_int("decay");
            buff_id = jo.get_string("ma_buff_id");
        }
};

class addiction : public JsonSerializer, public JsonDeserializer
{
    public:
        add_type type;
        int intensity;
        int sated;
        addiction()
        {
            type = ADD_NULL;
            intensity = 0;
            sated = 600;
        }
        addiction(add_type t)
        {
            type = t;
            intensity = 1;
            sated = 600;
        }
        addiction(add_type t, int i)
        {
            type = t;
            intensity = i;
            sated = 600;
        }

        using JsonSerializer::serialize;
        void serialize(JsonOut &json) const
        {
            json.start_object();
            json.member("type_enum", type);
            json.member("intensity", intensity);
            json.member("sated", sated);
            json.end_object();
        }
        using JsonDeserializer::deserialize;
        void deserialize(JsonIn &jsin)
        {
            JsonObject jo = jsin.get_object();
            type = (add_type)jo.get_int("type_enum");
            intensity = jo.get_int("intensity");
            sated = jo.get_int("sated");
        }
};

struct trait {
    std::string name;
    std::string id;
    int points; // How many points it costs in character creation
    int visibility; // How visible it is
    int ugliness; // How ugly it is
    bool mixed_effect; // Wheather it has positive as well as negative effects.
    bool startingtrait; // Starting Trait True/False
    bool purifiable; // Whether it's vulnerable to Purifier
    bool activated;
    bool fatigue; //IF any of the three are true, it drains that as the "cost"
    bool hunger;
    bool thirst;
    int cost;
    int charge;
    bool powered;
    int cooldown;
    std::string description;
    trait() : name("NULL_TRAIT"), points(0), visibility(0), ugliness(0), mixed_effect(false),
        startingtrait(false), purifiable(false), activated(false), fatigue(false),
        hunger(false), thirst(false), cost(0), charge(0), powered(false), cooldown(0)
    {
    }

    trait(std::string pid) : name(pid), points(0), visibility(0), ugliness(0),
        mixed_effect(false), startingtrait(false), purifiable(false), activated(false),
        fatigue(false), hunger(false), thirst(false), cost(0), charge(0), powered(false),
        cooldown(0)
    {
        id = pid;
    };
};

extern std::map<std::string, trait> traits;

inline bool trait_display_sort(const std::string &a, const std::string &b)
{
    return traits[a].name < traits[b].name;
}

enum hp_part {
    hp_head = 0,
    hp_torso,
    hp_arm_l,
    hp_arm_r,
    hp_leg_l,
    hp_leg_r,
    num_hp_parts
};

inline hp_part bodypart_to_hp_part(body_part p_bp)
{
    switch(p_bp) {
    case bp_torso:
        return hp_torso;

    case bp_head:
    case bp_eyes:
    case bp_mouth:
        return hp_torso;

    case bp_arm_l:
    case bp_hand_l:
        return hp_arm_l;

    case bp_arm_r:
    case bp_hand_r:
        return hp_arm_r;

    case bp_leg_l:
    case bp_foot_l:
        return hp_leg_l;

    case bp_leg_r:
    case bp_foot_r:
        return hp_leg_r;

    case num_bp:
        return num_hp_parts;

    default:
        break;
    }

    return hp_torso;
}

#endif
