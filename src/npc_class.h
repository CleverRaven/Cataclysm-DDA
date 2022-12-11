#pragma once
#ifndef CATA_SRC_NPC_CLASS_H
#define CATA_SRC_NPC_CLASS_H

#include <functional>
#include <iosfwd>
#include <map>
#include <vector>

#include "shop_cons_rate.h"
#include "translations.h"
#include "type_id.h"

class npc;
class JsonObject;
class Trait_group;

struct dialogue;
struct faction_price_rule;

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
    std::string refusal;
    std::function<bool( const dialogue & )> condition;

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

        distribution bonus_str;
        distribution bonus_dex;
        distribution bonus_int;
        distribution bonus_per;

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

        int roll_skill( const skill_id & ) const;

        const std::vector<shopkeeper_item_group> &get_shopkeeper_items() const;
        const shopkeeper_cons_rates &get_shopkeeper_cons_rates() const;
        const shopkeeper_blacklist &get_shopkeeper_blacklist() const;
        const time_duration &get_shop_restock_interval() const;
        faction_price_rule const *get_price_rules( item const &it, npc const &guy ) const;

        void load( const JsonObject &jo, const std::string &src );

        static const npc_class_id &from_legacy_int( int i );

        static const npc_class_id &random_common();

        static void load_npc_class( const JsonObject &jo, const std::string &src );

        static const std::vector<npc_class> &get_all();

        static void reset_npc_classes();

        static void finalize_all();

        static void check_consistency();
};

// TODO: Get rid of that
extern const npc_class_id NC_NONE;
extern const npc_class_id NC_EVAC_SHOPKEEP;
extern const npc_class_id NC_SHOPKEEP;
extern const npc_class_id NC_HACKER;
extern const npc_class_id NC_CYBORG;
extern const npc_class_id NC_DOCTOR;
extern const npc_class_id NC_TRADER;
extern const npc_class_id NC_NINJA;
extern const npc_class_id NC_COWBOY;
extern const npc_class_id NC_SCIENTIST;
extern const npc_class_id NC_BOUNTY_HUNTER;
extern const npc_class_id NC_THUG;
extern const npc_class_id NC_SCAVENGER;
extern const npc_class_id NC_ARSONIST;
extern const npc_class_id NC_HUNTER;
extern const npc_class_id NC_SOLDIER;
extern const npc_class_id NC_BARTENDER;
extern const npc_class_id NC_JUNK_SHOPKEEP;
extern const npc_class_id NC_HALLU;

#endif // CATA_SRC_NPC_CLASS_H
