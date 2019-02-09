#pragma once
#ifndef FACTION_H
#define FACTION_H

#include <vector>

#include "string_id.h"

// TODO: Redefine?
#define MAX_FAC_NAME_SIZE 40

std::string fac_ranking_text( int val );
std::string fac_respect_text( int val );
std::string fac_wealth_text( int val, int size );
std::string fac_food_supply_text( int val, int size );
std::string fac_combat_ability_text( int val );

class player;
class JsonObject;
class JsonIn;
class JsonOut;
struct tripoint;

enum faction_goal {
    FACGOAL_NULL = 0,
    FACGOAL_NONE,
    FACGOAL_WEALTH,
    FACGOAL_DOMINANCE,
    FACGOAL_CLEANSE,
    FACGOAL_SHADOW,
    FACGOAL_APOCALYPSE,
    FACGOAL_ANARCHY,
    FACGOAL_KNOWLEDGE,
    FACGOAL_NATURE,
    FACGOAL_CIVILIZATION,
    FACGOAL_FUNGUS,
    NUM_FACGOALS
};

enum faction_job {
    FACJOB_NULL = 0,
    FACJOB_EXTORTION,   // Protection rackets, etc
    FACJOB_INFORMATION, // Gathering & sale of information
    FACJOB_TRADE,       // Bazaars, etc.
    FACJOB_CARAVANS,    // Transportation of goods
    FACJOB_SCAVENGE,    // Scavenging & sale of general supplies
    FACJOB_MERCENARIES, // General fighters-for-hire
    FACJOB_ASSASSINS,   // Targeted discreet killing
    FACJOB_RAIDERS,     // Raiding settlements, trade routes, &c
    FACJOB_THIEVES,     // Less violent; theft of property without killing
    FACJOB_GAMBLING,    // Maintenance of gambling parlors
    FACJOB_DOCTORS,     // Doctors for hire
    FACJOB_FARMERS,     // Farming & sale of food
    FACJOB_DRUGS,       // Drug dealing
    FACJOB_MANUFACTURE, // Manufacturing & selling goods
    NUM_FACJOBS
};

enum faction_value {
    FACVAL_NULL = 0,
    FACVAL_CHARITABLE,   // Give their job for free (often)
    FACVAL_LONERS,       // Avoid associating with outsiders
    FACVAL_EXPLORATION,  // Like to explore new territory
    FACVAL_ARTIFACTS,    // Seek out unique items
    FACVAL_BIONICS,      // Collection & installation of bionic parts
    FACVAL_BOOKS,        // Collection of books
    FACVAL_TRAINING,     // Training among themselves and others
    FACVAL_ROBOTS,       // Creation / use of robots
    FACVAL_TREACHERY,    // Untrustworthy
    FACVAL_STRAIGHTEDGE, // No drugs, alcohol, etc.
    FACVAL_LAWFUL,       // Abide by the old laws
    FACVAL_CRUELTY,      // Torture, murder, etc.
    NUM_FACVALS
};

struct faction_value_datum {
    std::string name;
    int good; // A measure of how "good" the value is (naming purposes &c)
    int strength;
    int sneak;
    int crime;
    int cult;
};

class faction;
using faction_id = string_id<faction>;

class faction_template
{
    protected:
        faction_template();

    private:
        explicit faction_template( JsonObject &jsobj );

    public:
        explicit faction_template( const faction_template & ) = default;
        static void load( JsonObject &jsobj );
        static void reset();

        std::string name;
        int likes_u;
        int respects_u;
        bool known_by_u;
        faction_id id;
        std::string desc;
        int strength, sneak, crime, cult, good; // Defining values
        int size; // How big is our sphere of influence?
        int power; // General measure of our power
        int combat_ability;  //Combat multiplier for abstracted combat
        int food_supply;  //Total nutritional value held
        int wealth;  //Total trade currency
};

class faction : public faction_template
{
    public:
        faction() = default;
        faction( const faction_template &templ );

        void deserialize( JsonIn &jsin );
        void serialize( JsonOut &jsout ) const;

        void randomize();
        bool has_job( faction_job j ) const;
        bool has_value( faction_value v ) const;
        bool matches_us( faction_value v ) const;
        std::string describe() const;

        int response_time( const tripoint &abs_sm_pos ) const; // Time it takes for them to get to u

    unsigned values :
        NUM_FACVALS; // Bitfield of values
        faction_goal goal = FACGOAL_NULL;
        faction_job job1 = FACJOB_NULL;
        faction_job job2 = FACJOB_NULL;
        std::vector<int> opinion_of;
        /** Global submap coordinates where the center of influence is */
        int mapx = 0;
        int mapy = 0;
};

class faction_manager
{
    private:
        std::vector<faction> factions;

    public:
        void deserialize( JsonIn &jsin );
        void serialize( JsonOut &jsout ) const;

        void clear();
        void create_if_needed();

        const std::vector<faction> &all() const {
            return factions;
        }

        faction *get( const faction_id &id );

        void display() const;
};

#endif
