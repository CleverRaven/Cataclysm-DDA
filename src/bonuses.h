#pragma once
#ifndef BONUSES_H
#define BONUSES_H

#include <map>
#include <vector>
#include <string>

enum damage_type : int;

class Character;
class JsonObject;
class JsonArray;

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
    AFFECTED_TARGET_ARMOR_MULTIPLIER,
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
    scaling_stat stat;
    float scale;

    float get( const Character &u ) const;

    void load( JsonArray &jarr );
};

class bonus_container
{
    public:
        bonus_container();
        void load( JsonObject &jo );
        void load( JsonArray &jarr, bool mult );

        float get_flat( const Character &u, affected_stat stat, damage_type dt ) const;
        float get_flat( const Character &u, affected_stat stat ) const;

        float get_mult( const Character &u, affected_stat stat, damage_type dt ) const;
        float get_mult( const Character &u, affected_stat stat ) const;

        std::string get_description() const;

    private:
        using bonus_map = std::map<affected_type, std::vector<effect_scaling>>;
        /** All kinds of bonuses by types to damage, hit etc. */
        bonus_map bonuses_flat;
        bonus_map bonuses_mult;
};

#endif
