#pragma once
#ifndef CATA_SRC_NPC_CLASS_H
#define CATA_SRC_NPC_CLASS_H

#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "calendar.h"
#include "faction.h"
#include "translation.h"
#include "type_id.h"

class JsonObject;
class Trait_group;
class item;
class npc;
struct const_dialogue;
struct shopkeeper_blacklist;
struct shopkeeper_cons_rates;

namespace trait_group
{

using Trait_group_tag = string_id<Trait_group>;

} // namespace trait_group

// TODO: Move to better suited file (rng.h/.cpp?)
class distribution
{
    private:
        std::function<float()> generator_function;
        explicit distribution( const std::function<float()> &gen );

    public:
        distribution();
        distribution( const distribution & );

        float roll() const;

        distribution operator+( const distribution &other ) const;
        distribution operator*( const distribution &other ) const;
        distribution &operator=( const distribution &other );

        static distribution constant( float val );
        static distribution rng_roll( int from, int to );
        static distribution dice_roll( int sides, int size );
        static distribution one_in( float in );
};
struct shopkeeper_item_group {
    item_group_id id = item_group_id( "EMPTY_GROUP" );
    int trust = 0;
    bool strict = false;
    translation refusal;
    std::function<bool( const_dialogue const & )> condition;

    // Rigid shopkeeper groups will be processed a single time. Default groups are not rigid, and will be processed until the shopkeeper has no more room or remaining value to populate goods with.
    bool rigid = false;

    shopkeeper_item_group() = default;
    shopkeeper_item_group( const std::string &id, int trust, bool strict, bool rigid = false ) :
        id( item_group_id( id ) ), trust( trust ), strict( strict ), rigid( rigid ) {}

    bool can_sell( npc const &guy ) const;
    bool can_restock( npc const &guy ) const;
    std::string get_refusal() const;

    void deserialize( const JsonObject &jo );
};

class npc_class
{
    private:
        translation name;
        translation job_description;

        bool common = true;
        double common_spawn_weight = 1;

        distribution bonus_str;
        distribution bonus_dex;
        distribution bonus_int;
        distribution bonus_per;

        distribution bonus_aggression;
        distribution bonus_bravery;
        distribution bonus_collector;
        distribution bonus_altruism;

        std::map<skill_id, distribution> skills;
        // Just for finalization
        std::map<skill_id, distribution> bonus_skills;

        // first -> item group, second -> trust
        std::vector<shopkeeper_item_group> shop_item_groups;
        std::vector<faction_price_rule> shop_price_rules;
        shopkeeper_cons_rates_id shop_cons_rates_id = shopkeeper_cons_rates_id::NULL_ID();
        shopkeeper_blacklist_id shop_blacklist_id = shopkeeper_blacklist_id::NULL_ID();
        time_duration restock_interval = 6_days;

    public:
        npc_class_id id;
        std::vector<std::pair<npc_class_id, mod_id>> src;
        bool was_loaded = false;

        // By default, NPCs will be open to trade anything in their inventory, including worn items. If this is set to false, they won't sell items that they're directly wearing or wielding. Items inside of pockets/bags/etc are still fair game.
        bool sells_belongings = true;

        item_group_id worn_override;
        item_group_id carry_override;
        item_group_id weapon_override;

        translation bye_message_override;

        std::map<mutation_category_id, distribution> mutation_rounds;
        trait_group::Trait_group_tag traits = trait_group::Trait_group_tag( "EMPTY_GROUP" );
        // the int is what level the spell starts at
        std::map<spell_id, int> _starting_spells;
        std::map<bionic_id, int> bionic_list;
        std::vector<proficiency_id> _starting_proficiencies;
        npc_class();

        std::string get_name() const;
        std::string get_job_description() const;

        int roll_strength() const;
        int roll_dexterity() const;
        int roll_intelligence() const;
        int roll_perception() const;

        int roll_aggression() const;
        int roll_bravery() const;
        int roll_collector() const;
        int roll_altruism() const;

        int roll_skill( const skill_id & ) const;

        const std::vector<shopkeeper_item_group> &get_shopkeeper_items() const;
        const shopkeeper_cons_rates &get_shopkeeper_cons_rates() const;
        const shopkeeper_blacklist &get_shopkeeper_blacklist() const;
        const time_duration &get_shop_restock_interval() const;
        faction_price_rule const *get_price_rules( item const &it, npc const &guy ) const;

        bool is_common() const;

        void load( const JsonObject &jo, std::string_view src );

        static const npc_class_id &random_common();

        static void load_npc_class( const JsonObject &jo, const std::string &src );

        static const std::vector<npc_class> &get_all();

        static void reset_npc_classes();

        static void finalize_all();

        static void check_consistency();
};

#endif // CATA_SRC_NPC_CLASS_H
