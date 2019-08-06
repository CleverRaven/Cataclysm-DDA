#pragma once
#ifndef DIALOGUE_H
#define DIALOGUE_H

#include <functional>
#include <string>
#include <vector>
#include <set>
#include <type_traits>
#include <unordered_set>
#include <utility>

#include "dialogue_win.h"
#include "npc.h"
#include "json.h"
#include "string_id.h"
#include "material.h"
#include "type_id.h"

class mission;
struct dialogue;
class martialart;
class player;

enum talk_trial_type : unsigned char {
    TALK_TRIAL_NONE, // No challenge here!
    TALK_TRIAL_LIE, // Straight up lying
    TALK_TRIAL_PERSUADE, // Convince them
    TALK_TRIAL_INTIMIDATE, // Physical intimidation
    TALK_TRIAL_CONDITION, // Some other condition
    NUM_TALK_TRIALS
};

enum dialogue_consequence : unsigned char {
    none = 0,
    hostile,
    helpless,
    action
};

using talkfunction_ptr = std::add_pointer<void ( npc & )>::type;
using dialogue_fun_ptr = std::add_pointer<void( npc & )>::type;

using trial_mod = std::pair<std::string, int>;

/**
 * If not TALK_TRIAL_NONE, it defines how to decide whether the responses succeeds (e.g. the
 * NPC believes the lie). The difficulty is a 0...100 percent chance of success (!), 100 means
 * always success, 0 means never. It is however affected by mutations/traits/bionics/etc. of
 * the player character.
 */
struct talk_trial {
    talk_trial_type type = TALK_TRIAL_NONE;
    int difficulty = 0;
    std::function<bool( const dialogue & )> condition;

    int calc_chance( const dialogue &d ) const;
    /**
     * Returns a user-friendly representation of @ref type
     */
    const std::string &name() const;
    std::vector<trial_mod> modifiers;
    operator bool() const {
        return type != TALK_TRIAL_NONE;
    }
    /**
     * Roll for success or failure of this trial.
     */
    bool roll( dialogue &d ) const;

    talk_trial() = default;
    talk_trial( JsonObject );
};

struct talk_topic {
    explicit talk_topic( const std::string &i ) : id( i ) { }

    std::string id;
    /** If we're talking about an item, this should be its type. */
    itype_id item_type = "null";
    /** Reason for denying a request. */
    std::string reason;
};

struct talk_effect_fun_t {
    private:
        std::function<void( const dialogue &d )> function;

    public:
        talk_effect_fun_t() = default;
        talk_effect_fun_t( talkfunction_ptr effect );
        talk_effect_fun_t( const std::function<void( npc & )> effect );
        talk_effect_fun_t( std::function<void( const dialogue &d )> fun );
        void set_companion_mission( const std::string &role_id );
        void set_add_effect( JsonObject jo, const std::string &member, bool is_npc = false );
        void set_remove_effect( JsonObject jo, const std::string &member, bool is_npc = false );
        void set_add_trait( JsonObject jo, const std::string &member, bool is_npc = false );
        void set_remove_trait( JsonObject jo, const std::string &member, bool is_npc = false );
        void set_add_var( JsonObject jo, const std::string &member, bool is_npc = false );
        void set_remove_var( JsonObject jo, const std::string &member, bool is_npc = false );
        void set_u_buy_item( const std::string &new_trait, int cost, int count,
                             const std::string &container_name );
        void set_u_spend_cash( int amount );
        void set_u_sell_item( const std::string &new_trait, int cost, int count );
        void set_consume_item( JsonObject jo, const std::string &member, int count,
                               bool is_npc = false );
        void set_remove_item_with( JsonObject jo, const std::string &member, bool is_npc = false );
        void set_npc_change_faction( const std::string &faction_name );
        void set_npc_change_class( const std::string &faction_class );
        void set_change_faction_rep( int amount );
        void set_add_debt( const std::vector<trial_mod> &debt_modifiers );
        void set_toggle_npc_rule( const std::string &rule );
        void set_set_npc_rule( const std::string &rule );
        void set_clear_npc_rule( const std::string &rule );
        void set_npc_engagement_rule( const std::string &setting );
        void set_npc_aim_rule( const std::string &setting );
        void set_npc_cbm_reserve_rule( const std::string &setting );
        void set_npc_cbm_recharge_rule( const std::string &setting );
        void set_mapgen_update( JsonObject jo, const std::string &member );
        void set_bulk_trade_accept( bool is_trade, bool is_npc = false );
        void set_npc_gets_item( bool to_use );
        void set_add_mission( const std::string mission_id );

        void operator()( const dialogue &d ) const {
            if( !function ) {
                return;
            }
            return function( d );
        }
};

/**
 * Defines what happens when the trial succeeds or fails. If trial is
 * TALK_TRIAL_NONE it always succeeds.
 */
struct talk_effect_t {
        /**
          * How (if at all) the NPCs opinion of the player character (@ref npc::op_of_u)
          * will change.
          */
        npc_opinion opinion;
        /**
          * How (if at all) the NPCs opinion of the player character (@ref npc::op_of_u)
          * will change.  These values are divisors of the mission value.
          */
        npc_opinion mission_opinion;
        /**
          * Topic to switch to. TALK_DONE ends the talking, TALK_NONE keeps the current topic.
          */
        talk_topic next_topic = talk_topic( "TALK_NONE" );

        talk_topic apply( dialogue &d ) const;
        dialogue_consequence get_consequence( const dialogue &d ) const;

        /**
          * Sets an effect and consequence based on function pointer.
          */
        void set_effect( talkfunction_ptr effect );
        void set_effect( const talk_effect_fun_t &effect );
        /**
          * Sets an effect to a function object and consequence to explicitly given one.
          */
        void set_effect_consequence( const talk_effect_fun_t &eff, dialogue_consequence con );
        void set_effect_consequence( std::function<void( npc &p )> ptr, dialogue_consequence con );

        void load_effect( JsonObject &jo );
        void parse_sub_effect( JsonObject jo );
        void parse_string_effect( const std::string &effect_id, JsonObject &jo );

        talk_effect_t() = default;
        talk_effect_t( JsonObject );

        /**
         * Functions that are called when the response is chosen.
         */
        std::vector<talk_effect_fun_t> effects;
    private:
        dialogue_consequence guaranteed_consequence = dialogue_consequence::none;
};

/**
 * This defines possible responses from the player character.
 */
struct talk_response {
    /**
     * What the player character says (literally). Should already be translated and will be
     * displayed.
     */
    std::string text;
    /*
     * Optional responses from a true/false test that defaults to true.
     */
    std::string truetext;
    std::string falsetext;
    std::function<bool( const dialogue & )> truefalse_condition;

    talk_trial trial;
    /**
     * The following values are forwarded to the chatbin of the NPC (see @ref npc_chatbin).
     */
    mission *mission_selected = nullptr;
    skill_id skill = skill_id::NULL_ID();
    matype_id style = matype_id::NULL_ID();

    talk_effect_t success;
    talk_effect_t failure;

    talk_data create_option_line( const dialogue &d, char letter );
    std::set<dialogue_consequence> get_consequences( const dialogue &d ) const;

    talk_response();
    talk_response( JsonObject );
};

struct dialogue {
        /**
         * The player character that speaks (always g->u).
         * TODO: make it a reference, not a pointer.
         */
        player *alpha = nullptr;
        /**
         * The NPC we talk to. Never null.
         * TODO: make it a reference, not a pointer.
         */
        npc *beta = nullptr;
        /**
         * If true, we are done talking and the dialog ends.
         */
        bool done = false;
        std::vector<talk_topic> topic_stack;

        /** Missions that have been assigned by this npc to the player they currently speak to. */
        std::vector<mission *> missions_assigned;

        talk_topic opt( dialogue_window &d_win, const talk_topic &topic );

        dialogue() = default;

        mutable itype_id cur_item;
        mutable std::string reason;

        std::string dynamic_line( const talk_topic &topic ) const;
        void apply_speaker_effects( const talk_topic &the_topic );

        /** This dialogue is happening over a radio */
        bool by_radio = false;
        /**
         * Possible responses from the player character, filled in @ref gen_responses.
         */
        std::vector<talk_response> responses;
        void gen_responses( const talk_topic &topic );

        void add_topic( const std::string &topic );
        void add_topic( const talk_topic &topic );

    private:
        /**
         * Add a simple response that switches the topic to the new one. If first == true, force
         * this topic to the front of the responses.
         */
        talk_response &add_response( const std::string &text, const std::string &r,
                                     const bool first = false );
        /**
         * Add a response with the result TALK_DONE.
         */
        talk_response &add_response_done( const std::string &text );
        /**
         * Add a response with the result TALK_NONE.
         */
        talk_response &add_response_none( const std::string &text );
        /**
         * Add a simple response that switches the topic to the new one and executes the given
         * action. The response always succeeds. Consequence is based on function used.
         */
        talk_response &add_response( const std::string &text, const std::string &r,
                                     dialogue_fun_ptr effect_success, const bool first = false );

        /**
         * Add a simple response that switches the topic to the new one and executes the given
         * action. The response always succeeds. Consequence must be explicitly specified.
         */
        talk_response &add_response( const std::string &text, const std::string &r,
                                     std::function<void( npc & )> effect_success,
                                     dialogue_consequence consequence, const bool first = false );
        /**
         * Add a simple response that switches the topic to the new one and sets the currently
         * talked about mission to the given one. The mission pointer must be valid.
         */
        talk_response &add_response( const std::string &text, const std::string &r, mission *miss,
                                     const bool first = false );
        /**
         * Add a simple response that switches the topic to the new one and sets the currently
         * talked about skill to the given one.
         */
        talk_response &add_response( const std::string &text, const std::string &r, const skill_id &skill,
                                     const bool first = false );
        /**
         * Add a simple response that switches the topic to the new one and sets the currently
         * talked about martial art style to the given one.
         */
        talk_response &add_response( const std::string &text, const std::string &r,
                                     const martialart &style, const bool first = false );
        /**
         * Add a simple response that switches the topic to the new one and sets the currently
         * talked about item type to the given one.
         */
        talk_response &add_response( const std::string &text, const std::string &r,
                                     const itype_id &item_type, const bool first = false );
};

/**
 * A dynamically generated line, spoken by the NPC.
 * This struct only adds the constructors which will load the data from json
 * into a lambda, stored in the std::function object.
 * Invoking the function operator with a dialog reference (so the function can access the NPC)
 * returns the actual line.
 */
struct dynamic_line_t {
    private:
        std::function<std::string( const dialogue & )> function;

    public:
        dynamic_line_t() = default;
        dynamic_line_t( const std::string &line );
        dynamic_line_t( JsonObject jo );
        dynamic_line_t( JsonArray ja );
        static dynamic_line_t from_member( JsonObject &jo, const std::string &member_name );

        std::string operator()( const dialogue &d ) const {
            if( !function ) {
                return std::string{};
            }
            return function( d );
        }
};

namespace dialogue_data
{
const std::unordered_set<std::string> simple_string_conds = { {
        "u_male", "u_female", "npc_male", "npc_female",
        "has_no_assigned_mission", "has_assigned_mission", "has_many_assigned_missions",
        "has_no_available_mission", "has_available_mission", "has_many_available_missions",
        "mission_complete", "mission_incomplete",
        "npc_available", "npc_following", "npc_friend", "npc_hostile",
        "npc_train_skills", "npc_train_styles",
        "at_safe_space", "is_day", "npc_has_activity", "is_outside", "u_has_camp",
        "u_can_stow_weapon", "npc_can_stow_weapon", "u_has_weapon", "npc_has_weapon",
        "u_driving", "npc_driving",
        "has_pickup_list", "is_by_radio", "has_reason"
    }
};
const std::unordered_set<std::string> complex_conds = { {
        "u_has_any_trait", "npc_has_any_trait", "u_has_trait", "npc_has_trait",
        "u_has_trait_flag", "npc_has_trait_flag", "npc_has_class", "u_has_mission",
        "u_has_strength", "npc_has_strength", "u_has_dexterity", "npc_has_dexterity",
        "u_has_intelligence", "npc_has_intelligence", "u_has_perception", "npc_has_perception",
        "u_is_wearing", "npc_is_wearing", "u_has_item", "npc_has_item",
        "u_has_items", "npc_has_items", "u_has_item_category", "npc_has_item_category",
        "u_has_bionics", "npc_has_bionics", "u_has_effect", "npc_has_effect", "u_need", "npc_need",
        "u_at_om_location", "npc_at_om_location", "npc_role_nearby", "npc_allies", "npc_service",
        "u_has_cash", "u_are_owed",
        "npc_aim_rule", "npc_engagement_rule", "npc_rule", "npc_override",
        "npc_cbm_reserve_rule", "npc_cbm_recharge_rule",
        "days_since_cataclysm", "is_season", "mission_goal", "u_has_var", "npc_has_var"
    }
};
} // namespace dialogue_data

// the truly awful declaration for the conditional_t loading helper_function
void read_dialogue_condition( JsonObject &jo, std::function<bool( const dialogue & )> &condition,
                              bool default_val );
/**
 * A condition for a response spoken by the player.
 * This struct only adds the constructors which will load the data from json
 * into a lambda, stored in the std::function object.
 * Invoking the function operator with a dialog reference (so the function can access the NPC)
 * returns whether the response is allowed.
 */
struct conditional_t {
    private:
        std::function<bool ( const dialogue & )> condition;

    public:
        conditional_t() = default;
        conditional_t( const std::string &type );
        conditional_t( JsonObject jo );

        void set_has_any_trait( JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_trait( JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_trait_flag( JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_var( JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_activity( bool is_npc = false );
        void set_npc_has_class( JsonObject &jo );
        void set_u_has_mission( JsonObject &jo );
        void set_has_strength( JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_dexterity( JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_intelligence( JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_perception( JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_is_wearing( JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_item( JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_items( JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_item_category( JsonObject &jo, const std::string &member,
                                    bool is_npc = false );
        void set_has_bionics( JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_effect( JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_need( JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_at_om_location( JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_npc_role_nearby( JsonObject &jo );
        void set_npc_allies( JsonObject &jo );
        void set_u_has_cash( JsonObject &jo );
        void set_u_are_owed( JsonObject &jo );
        void set_npc_aim_rule( JsonObject &jo );
        void set_npc_engagement_rule( JsonObject &jo );
        void set_npc_cbm_reserve_rule( JsonObject &jo );
        void set_npc_cbm_recharge_rule( JsonObject &jo );
        void set_npc_rule( JsonObject &jo );
        void set_npc_override( JsonObject &jo );
        void set_days_since( JsonObject &jo );
        void set_is_season( JsonObject &jo );
        void set_mission_goal( JsonObject &jo );
        void set_no_assigned_mission();
        void set_has_assigned_mission();
        void set_has_many_assigned_missions();
        void set_no_available_mission();
        void set_has_available_mission();
        void set_has_many_available_missions();
        void set_mission_complete();
        void set_mission_incomplete();
        void set_npc_available();
        void set_npc_following();
        void set_npc_friend();
        void set_npc_hostile();
        void set_npc_train_skills();
        void set_npc_train_styles();
        void set_at_safe_space();
        void set_can_stow_weapon( bool is_npc = false );
        void set_has_weapon( bool is_npc = false );
        void set_is_driving( bool is_npc = false );
        void set_is_day();
        void set_has_stolen_item( bool is_npc = false );
        void set_is_outside();
        void set_is_by_radio();
        void set_u_has_camp();
        void set_has_pickup_list();
        void set_has_reason();
        void set_is_gender( bool is_male, bool is_npc = false );

        bool operator()( const dialogue &d ) const {
            if( !condition ) {
                return false;
            }
            return condition( d );
        }
};

/**
 * An extended response. It contains the response itself and a condition, so we can include the
 * response if, and only if the condition is met.
 */
class json_talk_response
{
    private:
        talk_response actual_response;
        std::function<bool( const dialogue & )> condition;
        bool is_switch = false;
        bool is_default = false;

        void load_condition( JsonObject &jo );
        bool test_condition( const dialogue &d ) const;

    public:
        json_talk_response() = default;
        json_talk_response( JsonObject jo );

        /**
         * Callback from @ref json_talk_topic::gen_responses, see there.
         */
        bool gen_responses( dialogue &d, bool switch_done ) const;
        bool gen_repeat_response( dialogue &d, const itype_id &item_id, bool switch_done ) const;
};

/**
 * A structure for generating repeated responses
 */
class json_talk_repeat_response
{
    public:
        json_talk_repeat_response() = default;
        json_talk_repeat_response( JsonObject jo );
        bool is_npc = false;
        bool include_containers = false;
        std::vector<std::string> for_item;
        std::vector<std::string> for_category;
        json_talk_response response;
};

class json_dynamic_line_effect
{
    private:
        std::function<bool( const dialogue & )> condition;
        talk_effect_t effect;
    public:
        json_dynamic_line_effect( JsonObject jo, const std::string &id );
        bool test_condition( const dialogue &d ) const;
        void apply( dialogue &d ) const;
};

/**
 * Talk topic definitions load from json.
 */
class json_talk_topic
{
    private:
        bool replace_built_in_responses = false;
        std::vector<json_talk_response> responses;
        dynamic_line_t dynamic_line;
        std::vector<json_dynamic_line_effect> speaker_effects;
        std::vector<json_talk_repeat_response> repeat_responses;

    public:
        json_talk_topic() = default;
        /**
         * Load data from json.
         * This will append responses (not change existing ones).
         * It will override dynamic_line and replace_built_in_responses if those entries
         * exist in the input, otherwise they will not be changed at all.
         */
        void load( JsonObject &jo );

        std::string get_dynamic_line( const dialogue &d ) const;
        std::vector<json_dynamic_line_effect> get_speaker_effects() const;

        void check_consistency() const;
        /**
         * Callback from @ref dialogue::gen_responses, it should add the response from here
         * into the list of possible responses (that will be presented to the player).
         * It may add an arbitrary number of responses (including none at all).
         * @return true if built in response should excluded (not added). If false, built in
         * responses will be added (behind those added here).
         */
        bool gen_responses( dialogue &d ) const;
};

void unload_talk_topics();
void load_talk_topic( JsonObject &jo );

#endif
