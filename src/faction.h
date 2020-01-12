#pragma once
#ifndef FACTION_H
#define FACTION_H

#include <bitset>
#include <vector>
#include <map>
#include <string>
#include <unordered_map>

#include "character.h"
#include "color.h"
#include "cursesdef.h"
#include "string_id.h"

// TODO: Redefine?
#define MAX_FAC_NAME_SIZE 40

std::string fac_ranking_text( int val );
std::string fac_respect_text( int val );
std::string fac_wealth_text( int val, int size );
std::string fac_combat_ability_text( int val );

class JsonObject;
class JsonIn;
class JsonOut;
class faction;

using faction_id = string_id<faction>;

namespace npc_factions
{
void finalize();
enum relationship : int {
    kill_on_sight,
    watch_your_back,
    share_my_stuff,
    guard_your_stuff,
    lets_you_in,
    defend_your_space,
    knows_your_voice,
    // final value is the count
    rel_types
};

const std::unordered_map<std::string, relationship> relation_strs = { {
        { "kill on sight", kill_on_sight },
        { "watch your back", watch_your_back },
        { "share my stuff", share_my_stuff },
        { "guard your stuff", guard_your_stuff },
        { "lets you in", lets_you_in },
        { "defends your space", defend_your_space },
        { "knows your voice", knows_your_voice }
    }
};
} // namespace npc_factions

class faction_template
{
    protected:
        faction_template();
        void load_relations( const JsonObject &jsobj );

    private:
        explicit faction_template( const JsonObject &jsobj );

    public:
        explicit faction_template( const faction_template & ) = default;
        static void load( const JsonObject &jsobj );
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
        bool lone_wolf_faction; // is this a faction for just one person?
        std::string currency; // itype_id of the faction currency
        std::map<std::string, std::bitset<npc_factions::rel_types>> relations;
        std::string mon_faction; // mon_faction_id of the monster faction; defaults to human
        std::set<std::tuple<int, int, std::string>> epilogue_data;
};

class faction : public faction_template
{
    public:
        faction() = default;
        faction( const faction_template &templ );

        void deserialize( JsonIn &jsin );
        void serialize( JsonOut &json ) const;
        void faction_display( const catacurses::window &fac_w, int width ) const;

        std::string describe() const;
        std::vector<std::string> epilogue() const;

        std::string food_supply_text();
        nc_color food_supply_color();

        bool has_relationship( const faction_id &guy_id, npc_factions::relationship flag ) const;
        void add_to_membership( const character_id &guy_id, std::string guy_name, bool known );
        void remove_member( const character_id &guy_id );
        std::vector<int> opinion_of;
        bool validated = false;
        std::map<character_id, std::pair<std::string, bool>> members;
};

class faction_manager
{
    private:
        std::map<faction_id, faction> factions;

    public:
        void deserialize( JsonIn &jsin );
        void serialize( JsonOut &jsout ) const;

        void clear();
        void create_if_needed();
        void display() const;
        faction *add_new_faction( const std::string &name_new, const faction_id &id_new,
                                  const faction_id &template_id );
        void remove_faction( const faction_id &id );
        const std::map<faction_id, faction> &all() const {
            return factions;
        }

        faction *get( const faction_id &id, bool complain = true );
};

#endif
