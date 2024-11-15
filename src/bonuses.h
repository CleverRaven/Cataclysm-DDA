#pragma once
#ifndef CATA_SRC_BONUSES_H
#define CATA_SRC_BONUSES_H

#include <map>
#include <string>
#include <vector>

#include "type_id.h"

class Character;
class JsonArray;
class JsonObject;

enum scaling_stat : int {
    STAT_NULL = 0,
    STAT_STR,
    STAT_DEX,
    STAT_INT,
    STAT_PER,
    SKILL_BASHING,
    SKILL_CUTTING,
    SKILL_DODGE,
    SKILL_MELEE,
    SKILL_STABBING,
    SKILL_SWIMMING,
    SKILL_UNARMED,
    SKILL_GUN,
    SKILL_PISTOL,
    SKILL_RIFLE,
    SKILL_SHOTGUN,
    SKILL_SMG,
    SKILL_ARCHERY,
    SKILL_THROW,
    SKILL_LAUNCHER,
    SKILL_DRIVE,
    SKILL_FIRSTAID,
    SKILL_SPELLCRAFT,
    NUM_STATS
};

enum class affected_stat : int {
    NONE = 0,
    HIT,
    CRITICAL_HIT_CHANCE,
    DODGE,
    BLOCK,
    BLOCK_EFFECTIVENESS,
    SPEED,
    MOVE_COST,
    DAMAGE,
    ARMOR,
    ARMOR_PENETRATION,
    TARGET_ARMOR_MULTIPLIER,
    NUM_AFFECTED
};

// We'll be indexing bonuses with this
struct affected_type {
    public:
        explicit affected_type( affected_stat s );
        affected_type( affected_stat s, const damage_type_id &t );
        bool operator<( const affected_type & ) const;
        bool operator==( const affected_type & ) const;

        affected_stat get_stat() const {
            return stat;
        }

        const damage_type_id &get_damage_type() const {
            return type;
        }

    private:
        affected_stat stat = affected_stat::NONE;
        damage_type_id type;
};

// This is the bonus we are indexing
struct effect_scaling {
    scaling_stat stat;
    float scale;

    float get( const Character &u ) const;

    explicit effect_scaling( const JsonObject &obj );
};

class bonus_container
{
    public:
        bonus_container();
        void load( const JsonObject &jo );

        float get_flat( const Character &u, affected_stat stat, const damage_type_id &dt ) const;
        float get_flat( const Character &u, affected_stat stat ) const;

        float get_mult( const Character &u, affected_stat stat, const damage_type_id &dt ) const;
        float get_mult( const Character &u, affected_stat stat ) const;

        std::string get_description() const;

    private:
        void load( const JsonArray &jarr, bool mult );

        using bonus_map = std::map<affected_type, std::vector<effect_scaling>>;
        /** All kinds of bonuses by types to damage, hit etc. */
        bonus_map bonuses_flat;
        bonus_map bonuses_mult;
};

#endif // CATA_SRC_BONUSES_H
