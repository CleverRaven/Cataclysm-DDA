#pragma once
#ifndef FACTION_H
#define FACTION_H

#include <vector>

#include "color.h"
#include "string_id.h"

// TODO: Redefine?
#define MAX_FAC_NAME_SIZE 40

std::string fac_ranking_text( int val );
std::string fac_respect_text( int val );
std::string fac_wealth_text( int val, int size );
std::string fac_combat_ability_text( int val );

class player;
class JsonObject;
class JsonIn;
class JsonOut;
struct tripoint;

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
        int size; // How big is our sphere of influence?
        int power; // General measure of our power
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

        std::string describe() const;

        std::string food_supply_text();
        nc_color food_supply_color();

        std::vector<int> opinion_of;
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
};

class new_faction_manager
{
    public:
        void display() const;
};

#endif
