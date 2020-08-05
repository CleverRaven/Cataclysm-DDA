#pragma once
#ifndef CATA_SRC_ARTIFACT_H
#define CATA_SRC_ARTIFACT_H

#include <string>

#include "enums.h"
#include "itype.h"

class JsonObject;
class JsonOut;
class item;
template<typename T> struct enum_traits;

enum art_effect_active : int {
    AEA_NULL = 0,

    AEA_STORM, // Emits shock fields
    AEA_FIREBALL, // Targeted
    AEA_ADRENALINE, // Adrenaline rush
    AEA_MAP, // Maps the area around you
    AEA_BLOOD, // Shoots blood all over
    AEA_FATIGUE, // Creates interdimensional fatigue
    AEA_ACIDBALL, // Targeted acid
    AEA_PULSE, // Destroys adjacent terrain
    AEA_HEAL, // Heals minor damage
    AEA_CONFUSED, // Confuses all monsters in view
    AEA_ENTRANCE, // Chance to make nearby monsters friendly
    AEA_BUGS, // Chance to summon friendly insects
    AEA_TELEPORT, // Teleports you
    AEA_LIGHT, // Temporary light source
    AEA_GROWTH, // Grow plants, a la triffid queen
    AEA_HURTALL, // Hurts all monsters!
    AEA_FUN, // Temporary morale bonus

    AEA_SPLIT, // Split between good and bad

    AEA_RADIATION, // Spew radioactive gas
    AEA_PAIN, // Increases player pain
    AEA_MUTATE, // Chance of mutation
    AEA_PARALYZE, // You lose several turns
    AEA_FIRESTORM, // Spreads minor fire all around you
    AEA_ATTENTION, // Attention from sub-prime denizens
    AEA_TELEGLOW, // Teleglow disease
    AEA_NOISE, // Loud noise
    AEA_SCREAM, // Noise & morale penalty
    AEA_DIM, // Darkens the sky slowly
    AEA_FLASH, // Flashbang
    AEA_VOMIT, // User vomits
    AEA_SHADOWS, // Summon shadow creatures
    AEA_STAMINA_EMPTY, // Empties most of the player's stamina gauge

    NUM_AEAS
};

template<>
struct enum_traits<art_effect_active> {
    static constexpr art_effect_active last = art_effect_active::NUM_AEAS;
};

enum art_charge : int {
    ARTC_NULL,    // Never recharges!
    ARTC_TIME,    // Very slowly recharges with time
    ARTC_SOLAR,   // Recharges in sunlight
    ARTC_PAIN,    // Creates pain to recharge
    ARTC_HP,      // Drains HP to recharge
    ARTC_FATIGUE, // Creates fatigue to recharge
    ARTC_PORTAL,  // Consumes portals
    NUM_ARTCS
};

template<>
struct enum_traits<art_charge> {
    static constexpr art_charge last = art_charge::NUM_ARTCS;
};

enum art_charge_req : int {
    ACR_NULL = 0, //No extra requirement
    ACR_EQUIP,    //Must be worn/wielded as appropriate
    ACR_SKIN,     //As ACR_EQUIP, plus must be only 50+-coverage thing on a bodypart (or no gloves if wielded)
    ACR_SLEEP,    //Only while sleeping
    ACR_RAD,      //Must be irradiated/in irradiated tile
    ACR_WET,      //Must be wet or in rain
    ACR_SKY,      //Must be on a Z-level above the surface
    NUM_ACRS
};

template<>
struct enum_traits<art_charge_req> {
    static constexpr art_charge_req last = art_charge_req::NUM_ACRS;
};

/* CLASSES */

class it_artifact_tool : public itype
{
    public:
        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &jo );

        it_artifact_tool();
        it_artifact_tool( const JsonObject &jo );
        it_artifact_tool( const itype &base ) : itype( base ) {}

        void create_name( const std::string &type );
        void create_name( const std::string &property_name, const std::string &shape_name );
};

class it_artifact_armor : public itype
{
    public:
        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &jo );

        it_artifact_armor();
        it_artifact_armor( const JsonObject &jo );
        it_artifact_armor( const itype &base ) : itype( base ) {}

        void create_name( const std::string &type );
};

/* FUNCTIONS */

itype_id new_artifact();
itype_id new_natural_artifact( artifact_natural_property prop );

// note: needs to be called by main() before MAPBUFFER.load
void load_artifacts( const std::string &path );
// save artifact definitions to json, path must be the same as for loading.
bool save_artifacts( const std::string &path );

bool check_art_charge_req( item &it );

#endif // CATA_SRC_ARTIFACT_H
