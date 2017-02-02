#pragma once
#ifndef NPC_CLASS_H
#define NPC_CLASS_H

#include <vector>
#include <map>
#include <array>
#include <random>
#include <functional>

#include "string_id.h"

class JsonObject;

class npc_class;
using npc_class_id = string_id<npc_class>;

class Skill;
using skill_id = string_id<Skill>;

typedef std::string Group_tag;

// @todo Move to better suited file (rng.h/.cpp?)
class distribution
{
    private:
        std::function<float()> generator_function;
        distribution( std::function<float()> gen );

    public:
        distribution();

        float roll() const;

        distribution operator+( const distribution &other ) const;
        distribution operator*( const distribution &other ) const;
        distribution &operator=( const distribution &other );

        static distribution constant( float val );
        static distribution rng_roll( int from, int to );
        static distribution dice_roll( int sides, int sizes );
        static distribution one_in( float in );
};

class npc_class
{
    private:
        std::string name;
        std::string job_description;

        bool common = true;

        distribution bonus_str;
        distribution bonus_dex;
        distribution bonus_int;
        distribution bonus_per;

        std::map<skill_id, distribution> skills;
        // Just for finalization
        std::map<skill_id, distribution> bonus_skills;

        Group_tag shopkeeper_item_group = "EMPTY_GROUP";

    public:
        npc_class_id id;
        bool was_loaded = false;

        Group_tag worn_override;
        Group_tag carry_override;
        Group_tag weapon_override;

        std::map<std::string, int> traits;

        npc_class();

        const std::string &get_name() const;
        const std::string &get_job_description() const;

        int roll_strength() const;
        int roll_dexterity() const;
        int roll_intelligence() const;
        int roll_perception() const;

        int roll_skill( const skill_id & ) const;

        const Group_tag &get_shopkeeper_items() const;

        void load( JsonObject &jo, const std::string &src );

        static const npc_class_id &from_legacy_int( int i );

        static const npc_class_id &random_common();

        static void load_npc_class( JsonObject &jo, const std::string &src );

        static const std::vector<npc_class> &get_all();

        static void reset_npc_classes();

        static void finalize_all();

        static void check_consistency();
};

// @todo Get rid of that
extern npc_class_id NC_NONE;
extern npc_class_id NC_EVAC_SHOPKEEP;
extern npc_class_id NC_SHOPKEEP;
extern npc_class_id NC_HACKER;
extern npc_class_id NC_DOCTOR;
extern npc_class_id NC_TRADER;
extern npc_class_id NC_NINJA;
extern npc_class_id NC_COWBOY;
extern npc_class_id NC_SCIENTIST;
extern npc_class_id NC_BOUNTY_HUNTER;
extern npc_class_id NC_THUG;
extern npc_class_id NC_SCAVENGER;
extern npc_class_id NC_ARSONIST;
extern npc_class_id NC_HUNTER;
extern npc_class_id NC_SOLDIER;
extern npc_class_id NC_BARTENDER;
extern npc_class_id NC_JUNK_SHOPKEEP;

#endif
