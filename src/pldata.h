#ifndef PLDATA_H
#define PLDATA_H

#include "json.h"
#include "bodypart.h"
#include <map>
#include <string>

typedef std::string matype_id;

typedef std::string mabuff_id;

typedef std::string matec_id;

typedef std::string efftype_id;

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

void realDebugmsg(const char *name, const char *line, const char *mes, ...);

class disease : public JsonSerializer, public JsonDeserializer
{
public:
    std::string type;
    std::string buff_id;
    int         intensity = 0;
    int         duration  = 0;
    int         decay     = 0;
    body_part   bp        = num_bp;
    bool        permanent = false;

    // extra stuff for martial arts, kind of a hack for now
    disease(std::string new_buff_id)
        : type {"ma_buff"}, buff_id {std::move(new_buff_id)}, intensity {1}
    {
    }
    
    bool is_mabuff() const
    {
        return !buff_id.empty() && type == "ma_buff";
    }

    disease() : type("null") { }

    disease(std::string t, int const d, int const i, body_part const part, bool const perm, int const dec)
        : type {std::move(t)}, intensity {i}, duration {d}, decay {dec}, bp {part}, permanent {perm}
    {
    }

    using JsonSerializer::serialize;
    void serialize(JsonOut &json) const override
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
    void deserialize(JsonIn &jsin) override
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
    add_type type      = ADD_NULL;
    int      intensity = 0;
    int      sated     = 600;

    addiction() = default;
    addiction(add_type const t, int const i = 1) : type {t}, intensity {i} { }

    using JsonSerializer::serialize;
    void serialize(JsonOut &json) const override
    {
        json.start_object();
        json.member("type_enum", type);
        json.member("intensity", intensity);
        json.member("sated", sated);
        json.end_object();
    }
    using JsonDeserializer::deserialize;
    void deserialize(JsonIn &jsin) override
    {
        JsonObject jo = jsin.get_object();
        type = (add_type)jo.get_int("type_enum");
        intensity = jo.get_int("intensity");
        sated = jo.get_int("sated");
    }
};

struct trait {
    int charge     = 0;
    int cooldown   = 0;
    bool powered       = false;
};

extern std::map<std::string, trait> traits;

bool trait_display_sort(const std::string &a, const std::string &b) noexcept;

#endif
