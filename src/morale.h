#ifndef MORALE_H
#define MORALE_H

#include "json.h"
#include <string>
#include "calendar.h"

#define MIN_MORALE_READ (-40)
#define MIN_MORALE_CRAFT (-50)

struct itype;

enum morale_type : int {
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
    MORALE_MUTAGEN_ELF,
    MORALE_MUTAGEN_CHIMERA,
    MORALE_MUTAGEN_MUTATION,

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

    MORALE_HAIRCUT,
    MORALE_SHAVE,

    MORALE_MUSICAL_INSTRUMENT,

    NUM_MORALE_TYPES
};

class morale_point : public JsonSerializer, public JsonDeserializer
{
    public:
        morale_point(
            morale_type type = MORALE_NULL,
            const itype *item_type = nullptr,
            int bonus = 0,
            int duration = MINUTES( 6 ),
            int decay_start = MINUTES( 3 ),
            int age = 0 );

        using JsonDeserializer::deserialize;
        void deserialize( JsonIn &jsin ) override;
        using JsonSerializer::serialize;
        void serialize( JsonOut &json ) const override;

        std::string get_name() const {
            return name;
        }

        morale_type get_type() const {
            return type;
        }

        const itype *get_item_type() const {
            return item_type;
        }

        int get_bonus() const {
            return bonus;
        }

        bool is_expired() const {
            return age >= duration || bonus == 0;
        }

        void add( int new_bonus, int new_max_bonus, int new_duration, int new_decay_start, bool new_cap );
        void proceed( int ticks = 1 );

    private:
        std::string name;  // Safely assume it's immutable

        morale_type type;
        const itype *item_type;

        int bonus;
        int duration;
        int decay_start;
        int age;

        /**
        Returns either new_time or remaining time (which one is greater).
        Only returns new time if same_sign is true
        */
        int pick_time( int cur_time, int new_time, bool same_sign ) const;
};

#endif
