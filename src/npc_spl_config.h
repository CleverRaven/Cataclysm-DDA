#pragma once
#ifndef CATA_SRC_NPC_SPL_CONFIG_H
#define CATA_SRC_NPC_SPL_CONFIG_H

#include <bitset>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "type_id.h"

class JsonObject;
class JsonOut;
class npc;

namespace npc_spl
{
enum class foda_kind : int {
    mandatory,      // [●]
    optional,       // [○]
    alternative,    // [△] XOR group: exactly one child
    or_group        // [▲] OR group: 0..N children
};

std::string foda_symbol( foda_kind kind );

enum class feature_group : int {
    attitude = 0,   // [●] mandatory, [△] XOR
    archetype,      // [●] mandatory, [△] XOR
    roles,          // [○] optional,  [▲] OR
    modifiers,      // [○] optional, each child [○]
    num_groups
};

enum class feature : int {
    att_friendly = 0,
    att_neutral,
    att_cautious,
    att_hostile,

    arch_survivor,
    arch_merchant,
    arch_medic,
    arch_raider,

    role_trader,
    role_quest_giver,
    role_recruitable,
    role_instructor,

    mod_bionics,
    mod_local_lore,

    num_features
};

constexpr std::size_t num_features = static_cast<std::size_t>( feature::num_features );
using feature_set = std::bitset<num_features>;

struct feature_info {
    feature id;
    feature_group group;
    std::string str_id;
    std::string label;
    std::string description;
};

struct group_info {
    feature_group id;
    std::string label;
    foda_kind presence;
    foda_kind decomposition;
};

enum class constraint_type : int {
    requires_feature,   // A --> B
    excludes_feature    // A <--> B (A has propagation priority: B gets blocked)
};

struct cross_tree_constraint {
    feature from;
    constraint_type type;
    feature to;
};

const std::vector<feature_info> &all_features();
const std::vector<group_info> &all_groups();
const std::vector<cross_tree_constraint> &all_constraints();
const feature_info &info( feature f );
const group_info &info( feature_group g );

class npc_feature_config
{
    public:
        npc_feature_config();

        std::string name;

        bool is_selected( feature f ) const;
        bool is_blocked( feature f ) const;
        bool is_forced( feature f ) const;
        std::string constraint_reason( feature f ) const;

        bool choose( feature f );

        std::vector<std::string> validate_and_propagate_constraints();

        bool is_valid( std::string *why = nullptr ) const;

        const std::vector<std::string> &last_propagation_log() const {
            return propagation_log;
        }

        feature selected_in( feature_group g ) const;

        void serialize( JsonOut &jsout ) const;
        void deserialize( const JsonObject &jo );
        std::string to_json_string() const;
        static std::optional<npc_feature_config> from_json_string( const std::string &data );

    private:
        feature_set selected;
        feature_set blocked;
        feature_set forced;
        std::vector<std::string> propagation_log;
};

struct dialogue_response_node {
    std::string text;
    std::string topic;
    std::string effect;
    std::optional<int> persuade_difficulty;
    std::string failure_topic;
    std::string failure_effect;
};

struct dialogue_topic_node {
    std::string id;
    foda_kind kind = foda_kind::optional;
    std::string dynamic_line;
    std::string presence_condition;
    std::vector<dialogue_response_node> responses;
};

struct pruned_dialogue_node {
    std::string id;
    std::string presence_condition;
};

struct dialogue_feature_model {
    std::string root_topic;
    std::vector<dialogue_topic_node> topics;
    std::vector<pruned_dialogue_node> pruned;
};

dialogue_feature_model derive_dialogue_tree( const npc_feature_config &cfg );

void register_dialogue_tree( const dialogue_feature_model &model );

const std::vector<std::string> &all_dialogue_topic_ids();

bool run_configuration_menu( npc_feature_config &cfg );

void show_derived_dialogue_tree( const npc_feature_config &cfg );

npc_class_id archetype_class( const npc_feature_config &cfg );

void apply_to_npc( npc &guy, const npc_feature_config &cfg );

void restore_npc_dialogue( const npc &guy );

void set_pending_config( const npc_feature_config &cfg );
std::optional<npc_feature_config> take_pending_config();
void clear_pending_config();

} // namespace npc_spl

#endif // CATA_SRC_NPC_SPL_CONFIG_H
