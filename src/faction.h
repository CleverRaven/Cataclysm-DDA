#pragma once
#ifndef CATA_SRC_FACTION_H
#define CATA_SRC_FACTION_H

#include <bitset>
#include <cstddef>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "character_id.h"
#include "color.h"
#include "generic_factory.h"
#include "shop_cons_rate.h"
#include "stomach.h"
#include "translation.h"
#include "type_id.h"

enum class vitamin_type : int;

namespace catacurses
{
class window;
}  // namespace catacurses

// TODO: Redefine?
constexpr int MAX_FAC_NAME_SIZE = 40;

std::string fac_ranking_text( int val );
std::string fac_respect_text( int val );
std::string fac_wealth_text( int val, int size );
std::string fac_combat_ability_text( int val );

class JsonObject;
class JsonOut;
class JsonValue;
class item;
class npc;

namespace npc_factions
{
void finalize();
enum class relationship : int {
    kill_on_sight,
    watch_your_back,
    share_my_stuff,
    share_public_goods,
    guard_your_stuff,
    lets_you_in,
    defend_your_space,
    knows_your_voice,
    // final value is the count
    rel_types
};

const std::unordered_map<std::string, relationship> relation_strs = { {
        { "kill on sight", npc_factions::relationship::kill_on_sight },
        { "watch your back", npc_factions::relationship::watch_your_back },
        { "share my stuff", npc_factions::relationship::share_my_stuff },
        { "share public goods", npc_factions::relationship::share_public_goods },
        { "guard your stuff", npc_factions::relationship::guard_your_stuff },
        { "lets you in", npc_factions::relationship::lets_you_in },
        { "defends your space", npc_factions::relationship::defend_your_space },
        { "knows your voice", npc_factions::relationship::knows_your_voice }
    }
};
} // namespace npc_factions

struct faction_price_rule: public icg_entry {
    double markup = 1.0;
    double premium = 1.0;
    std::optional<double> fixed_adj = std::nullopt;
    std::optional<int> price = std::nullopt;

    faction_price_rule() = default;
    faction_price_rule( itype_id const &id, double m, double f )
        : icg_entry{ id, {}, {}, {}, {} }, markup( m ), fixed_adj( f ) {};
    explicit faction_price_rule( icg_entry const &rhs ) : icg_entry( rhs ) {}

    void deserialize( JsonObject const &jo );
};

class faction_price_rules_reader : public generic_typed_reader<faction_price_rules_reader>
{
    public:
        static faction_price_rule get_next( JsonValue &jv );
};


struct faction_power_spec {
    faction_id faction;
    std::optional<int> power_min;
    std::optional<int> power_max;

    void deserialize( const JsonObject &jo );
};


struct faction_epilogue_data {
    std::optional<int> power_min;
    std::optional<int> power_max;

    std::vector<faction_power_spec> dynamic_conditions;

    snippet_id epilogue;

    void deserialize( const JsonObject &jo );
};


class faction_template
{
    protected:
        faction_template();
        void load_relations( const JsonObject &jsobj );

    private:
        explicit faction_template( const JsonObject &jsobj );


    public:
        static void load( const JsonObject &jsobj );
        static void check_consistency();
        static void reset();

        std::string name;
        int likes_u;
        int respects_u;
        int trusts_u; // Determines which item groups are available for trading
        bool known_by_u;
        faction_id id;
        translation desc;
        int size; // How big is our sphere of influence?
        int power; // General measure of our power
        nutrients food_supply; //Total nutritional value held
        bool consumes_food; //Whether this faction actually draws down the food_supply when eating from it
        int wealth;  //Total trade currency
        bool lone_wolf_faction; // is this a faction for just one person?
        bool limited_area_claim;
        itype_id currency; // id of the faction currency
        std::vector<faction_price_rule> price_rules; // additional pricing rules
        std::map<std::string, std::bitset<static_cast<size_t>( npc_factions::relationship::rel_types )>>
                relations;
        mfaction_str_id mon_faction; // mon_faction_id of the monster faction; defaults to human
        std::vector<faction_epilogue_data> epilogue_data;
};

class faction : public faction_template
{
    public:
        faction() = default;
        explicit faction( const faction_template &templ );

        void deserialize( const JsonObject &jo );
        void serialize( JsonOut &json ) const;
        void faction_display( const catacurses::window &fac_w, int width ) const;


        std::string describe() const;
        bool check_relations( const std::vector<faction_power_spec> &faction_power_specs ) const;
        std::vector<std::string> epilogue() const;

        std::string food_supply_text();
        nc_color food_supply_color();

        std::pair<nc_color, std::string> vitamin_stores( vitamin_type vit );

        faction_price_rule const *get_price_rules( item const &it, npc const &guy ) const;

        bool has_relationship( const faction_id &guy_id, npc_factions::relationship flag ) const;
        void add_to_membership( const character_id &guy_id, const std::string &guy_name, bool known );
        void remove_member( const character_id &guy_id );
        std::vector<int> opinion_of;
        bool validated = false; // NOLINT(cata-serialize)
        std::map<character_id, std::pair<std::string, bool>> members; // NOLINT(cata-serialize)
};

class faction_manager
{
    private:
        std::map<faction_id, faction> factions;

    public:
        void deserialize( const JsonValue &jv );
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

#endif // CATA_SRC_FACTION_H
