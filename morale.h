#ifndef _MORALE_H_
#define _MORALE_H_

#include <string>

#define MIN_MORALE_READ		(-40)
#define MIN_MORALE_CRAFT	(-50)

enum morale_type
{
 MORALE_NULL = 0,
 MORALE_FOOD_GOOD,
 MORALE_FOOD_HOT,
 MORALE_MUSIC,
 MORALE_MARLOSS,
 MORALE_FEELING_GOOD,

 MORALE_CRAVING_NICOTINE,
 MORALE_CRAVING_CAFFEINE,
 MORALE_CRAVING_ALCOHOL,
 MORALE_CRAVING_OPIATE,
 MORALE_CRAVING_SPEED,
 MORALE_CRAVING_COCAINE,
 MORALE_CRAVING_CRACK,

 MORALE_FOOD_BAD,
 MORALE_CANNIBAL,
 MORALE_VEGETARIAN,
 MORALE_WET,
 MORALE_DRIED_OFF,
 MORALE_COLD,
 MORALE_HOT,
 MORALE_FEELING_BAD,
 MORALE_KILLED_INNOCENT,
 MORALE_KILLED_FRIEND,
 MORALE_KILLED_MONSTER,

 MORALE_MOODSWING,
 MORALE_BOOK,

 MORALE_SCREAM,

 MORALE_PERM_MASOCHIST,
 MORALE_PERM_HOARDER,
 MORALE_PERM_CROSSDRESSER,
 MORALE_PERM_OPTIMIST,

 NUM_MORALE_TYPES
};

struct morale_point
{
    morale_type type;
    itype *item_type;
    int bonus;
    int duration;
    int decay_start;
    int age;

    morale_point(morale_type T = MORALE_NULL, itype *I = NULL, int B = 0,
                 int D = 60, int DS = 30, int A = 0) :
        type (T), item_type (I), bonus (B), duration(D), decay_start(DS), age(A) {};

    std::string name(std::string morale_data[])
    {
        // Start with the morale type's description.
        std::string ret = morale_data[type];

        // Get the name of the referenced item (if any).
        std::string item_name = "";
        if (item_type != NULL)
        {
            item_name = item_type->name;
        }

        // Replace each instance of %i with the item's name.
        size_t it = ret.find("%i");
        while (it != std::string::npos)
        {
            ret.replace(it, 2, item_name);
            it = ret.find("%i");
        }

        return ret;
    }
};

#endif
