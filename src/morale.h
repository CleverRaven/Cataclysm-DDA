#ifndef MORALE_H
#define MORALE_H

#include "json.h"
#include <string>

#define MIN_MORALE_READ (-40)
#define MIN_MORALE_CRAFT (-50)

struct itype;

enum morale_type
{
    MORALE_NULL = 0,
    MORALE_FOOD_GOOD,
    MORALE_FOOD_HOT,
    MORALE_MUSIC,
    MORALE_HONEY,
    MORALE_GAME,
    MORALE_MARLOSS,
    MORALE_MUTAGEN,
    MORALE_FEELING_GOOD,
    MORALE_SUPPORT,
    MORALE_PHOTOS,

    MORALE_CRAVING_NICOTINE,
    MORALE_CRAVING_CAFFEINE,
    MORALE_CRAVING_ALCOHOL,
    MORALE_CRAVING_OPIATE,
    MORALE_CRAVING_SPEED,
    MORALE_CRAVING_COCAINE,
    MORALE_CRAVING_CRACK,
    MORALE_CRAVING_MUTAGEN,
    MORALE_CRAVING_DIAZEPAM,
    MORALE_CRAVING_MARLOSS,

    MORALE_FOOD_BAD,
    MORALE_CANNIBAL,
    MORALE_VEGETARIAN,
    MORALE_MEATARIAN,
    MORALE_ANTIFRUIT,
    MORALE_LACTOSE,
    MORALE_ANTIJUNK,
    MORALE_ANTIWHEAT,
    MORALE_NO_DIGEST,
    MORALE_WET,
    MORALE_DRIED_OFF,
    MORALE_COLD,
    MORALE_HOT,
    MORALE_FEELING_BAD,
    MORALE_KILLED_INNOCENT,
    MORALE_KILLED_FRIEND,
    MORALE_KILLED_MONSTER,
    MORALE_MUTILATE_CORPSE,
    MORALE_MUTAGEN_CHIMERA,
    MORALE_MUTAGEN_ELFA,

    MORALE_MOODSWING,
    MORALE_BOOK,
    MORALE_COMFY,

    MORALE_SCREAM,

    MORALE_PERM_MASOCHIST,
    MORALE_PERM_HOARDER,
    MORALE_PERM_FANCY,
    MORALE_PERM_OPTIMIST,
    MORALE_PERM_BADTEMPER,
    MORALE_PERM_CONSTRAINED,
    MORALE_GAME_FOUND_KITTEN,

    NUM_MORALE_TYPES
};

class morale_point : public JsonSerializer, public JsonDeserializer
{
    public:
        morale_type type;
        itype *item_type;
        int bonus;
        int duration;
        int decay_start;
        int age;

        morale_point(morale_type T = MORALE_NULL, itype *I = NULL, int B = 0,
                     int D = 60, int DS = 30, int A = 0) :
            type (T), item_type (I), bonus (B), duration(D), decay_start(DS), age(A) {};

        using JsonDeserializer::deserialize;
        void deserialize(JsonIn &jsin);
        using JsonSerializer::serialize;
        void serialize(JsonOut &json) const;

        std::string name() const;
};

#endif
