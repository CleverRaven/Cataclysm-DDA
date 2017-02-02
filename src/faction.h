#pragma once
#ifndef FACTION_H
#define FACTION_H

#include "json.h"

#include <string>
#include <vector>
#include <map>

// TODO: Redefine?
#define MAX_FAC_NAME_SIZE 40
#ifndef mfb
#define mfb(n) static_cast <unsigned long> (1 << (n))
#endif

std::string fac_ranking_text( int val );
std::string fac_respect_text( int val );
std::string fac_wealth_text( int val, int size );
std::string fac_food_supply_text( int val, int size );
std::string fac_combat_ability_text( int val );

class game;

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
    FACJOB_GAMBLING,    // Maitenance of gambling parlors
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

typedef std::map<std::string, faction> faction_map;

class faction : public JsonSerializer, public JsonDeserializer
{
    public:
        static std::string faction_adj_pos[15];
        static std::string faction_adj_neu[15];
        static std::string faction_adj_bad[15];
        static std::string faction_noun_strong[15];
        static std::string faction_noun_sneak[15];
        static std::string faction_noun_crime[15];
        static std::string faction_noun_cult[15];
        static std::string faction_noun_none[15];

        // See faction.cpp
        static faction_value_datum facgoal_data[NUM_FACGOALS];
        static faction_value_datum facjob_data[NUM_FACJOBS];
        static faction_value_datum facval_data[NUM_FACVALS];

        faction();
        faction( std::string uid );

        static void load_faction( JsonObject &jsobj );
        faction *find_faction( std::string ident );
        void load_faction_template( std::string ident );
        std::vector<std::string> all_json_factions();

        ~faction() override;
        void load_info( std::string data );
        using JsonDeserializer::deserialize;
        void deserialize( JsonIn &jsin ) override;
        using JsonSerializer::serialize;
        void serialize( JsonOut &jsout ) const override;

        static faction_map _all_faction;

        void randomize();
        void make_army();
        bool has_job( faction_job j ) const;
        bool has_value( faction_value v ) const;
        bool matches_us( faction_value v ) const;
        std::string describe() const;

        int response_time() const; // Time it takes for them to get to u

        std::string name;
    unsigned values :
        NUM_FACVALS; // Bitfield of values
        faction_goal goal;
        faction_job job1, job2;
        std::vector<int> opinion_of;
        int likes_u;
        int respects_u;
        bool known_by_u;
        std::string id;
        std::string desc;
        int strength, sneak, crime, cult, good; // Defining values
        /** Global submap coordinates where the center of influence is */
        int mapx, mapy;
        int size; // How big is our sphere of influence?
        int power; // General measure of our power
        int combat_ability;  //Combat multiplier for abstracted combat
        int food_supply;  //Total nutritional value held
        int wealth;  //Total trade currency
};

#endif
