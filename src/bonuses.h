#ifndef BONUSES_H
#define BONUSES_H

#include <map>

enum damage_type : int;

    class Character;
    class JsonObject;

    enum scaling_stat : int {
    STAT_NULL = 0,
    STAT_STR,
    STAT_DEX,
    STAT_INT,
    STAT_PER,
    NUM_STATS
};

enum affected_stat : int {
    AFFECTED_NULL = 0,
    AFFECTED_HIT,
    AFFECTED_DODGE,
    AFFECTED_BLOCK,
    AFFECTED_SPEED,
    AFFECTED_MOVE_COST,
    AFFECTED_DAMAGE,
    AFFECTED_ARMOR,
    AFFECTED_ARMOR_PENETRATION,
    NUM_AFFECTED
};

// We'll be indexing bonuses with this
struct affected_type {
    public:
        affected_type( affected_stat s );
        affected_type( affected_stat s, damage_type t );
        bool operator<( const affected_type & ) const;
        bool operator==( const affected_type & ) const;

        affected_stat get_stat() const {
            return stat;
        }

        damage_type get_damage_type() const {
            return type;
        }

    private:
        affected_stat stat;
        damage_type type;
};

// This is the bonus we are indexing
struct effect_scaling {
    float base;
    scaling_stat stat;
    float scale;

    float get( const Character &u ) const;
};

class bonus_container
{
    public:
        bonus_container();
        void load( JsonObject &jo );

        float get_flat( const Character &u, affected_stat stat, damage_type type ) const;
        float get_flat( const Character &u, affected_stat stat ) const;

        float get_mult( const Character &u, affected_stat stat, damage_type type ) const;
        float get_mult( const Character &u, affected_stat stat ) const;

    private:
        /** All kinds of bonuses by types to damage, hit etc. */
        std::map<affected_type, effect_scaling> bonuses_flat;
        std::map<affected_type, effect_scaling> bonuses_mult;
};

#endif
