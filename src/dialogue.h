#pragma once
#ifndef CATA_SRC_DIALOGUE_H
#define CATA_SRC_DIALOGUE_H

#include <functional>
#include <iosfwd>
#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "cata_lazy.h"
#include "dialogue_win.h"
#include "global_vars.h"
#include "npc.h"
#include "talker.h"
#include "translations.h"
#include "type_id.h"

class JsonArray;
class JsonObject;
class martialart;
class mission;
struct dialogue;
struct input_event;

enum talk_trial_type : unsigned char {
    TALK_TRIAL_NONE, // No challenge here!
    TALK_TRIAL_LIE, // Straight up lying
    TALK_TRIAL_PERSUADE, // Convince them
    TALK_TRIAL_INTIMIDATE, // Physical intimidation
    TALK_TRIAL_SKILL_CHECK, // Check a skill's current level against the difficulty
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
    std::function<bool( dialogue & )> condition;

    // If this talk_trial is skill check, this is the string ID of the skill that we check the level of.
    std::string skill_required;

    int calc_chance( dialogue &d ) const;
    /**
     * Returns a user-friendly representation of @ref type
     */
    std::string name() const;
    std::vector<trial_mod> modifiers;
    explicit operator bool() const {
        return type != TALK_TRIAL_NONE;
    }
    /**
     * Roll for success or failure of this trial.
     */
    bool roll( dialogue &d ) const;

    talk_trial() = default;
    explicit talk_trial( const JsonObject & );
};

struct talk_topic {
    explicit talk_topic( std::string i, itype_id const &it = itype_id::NULL_ID(),
                         std::string r = {} )
        : id( std::move( i ) ), item_type( it ), reason( std::move( r ) ) {
    }

    std::string id;
    /** If we're talking about an item, this should be its type. */
    itype_id item_type = itype_id::NULL_ID();
    /** Reason for denying a request. */
    std::string reason;
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

        talk_topic apply( dialogue &d )
        const;
        static void update_missions( dialogue &d );
        dialogue_consequence get_consequence( dialogue const &d ) const;

        /**
          * Sets an effect and consequence based on function pointer.
          */
        void set_effect( talkfunction_ptr );
        void set_effect( const talk_effect_fun_t & );
        /**
          * Sets an effect to a function object and consequence to explicitly given one.
          */
        void set_effect_consequence( const talk_effect_fun_t &fun, dialogue_consequence con );
        void set_effect_consequence( const std::function<void( npc &p )> &ptr, dialogue_consequence con );

        void load_effect( const JsonObject &jo, const std::string &member_name );
        void parse_sub_effect( const JsonObject &jo );
        void parse_string_effect( const std::string &effect_id, const JsonObject &jo );

        talk_effect_t() = default;
        explicit talk_effect_t( const JsonObject &, const std::string & );

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
    translation truetext;
    translation falsetext;
    std::function<bool( dialogue & )> truefalse_condition;

    talk_trial trial;
    /**
     * The following values are forwarded to the chatbin of the NPC (see @ref npc_chatbin).
     */
    mission *mission_selected = nullptr;
    skill_id skill = skill_id();
    matype_id style = matype_id();
    spell_id dialogue_spell = spell_id();
    proficiency_id proficiency = proficiency_id();

    talk_effect_t success;
    talk_effect_t failure;

    talk_data create_option_line( dialogue &d, const input_event &hotkey,
                                  bool is_computer = false );
    std::set<dialogue_consequence> get_consequences( dialogue &d ) const;

    talk_response();
    explicit talk_response( const JsonObject & );
};

struct dialogue {
        /**
         * If true, we are done talking and the dialog ends.
         */
        bool done = false;
        std::vector<talk_topic> topic_stack;

        /** Missions that have been assigned by this npc to the player they currently speak to. */
        std::vector<mission *> missions_assigned;

        talk_topic opt( dialogue_window &d_win, const talk_topic &topic );
        dialogue() = default;
        dialogue( const dialogue &d );
        dialogue( dialogue && ) = default;
        dialogue &operator=( const dialogue & );
        dialogue &operator=( dialogue && ) = default;
        dialogue( std::unique_ptr<talker> alpha_in, std::unique_ptr<talker> beta_in );
        dialogue( std::unique_ptr<talker> alpha_in, std::unique_ptr<talker> beta_in,
                  const std::unordered_map<std::string, std::function<bool( dialogue & )>> &cond );
        dialogue( std::unique_ptr<talker> alpha_in, std::unique_ptr<talker> beta_in,
                  const std::unordered_map<std::string, std::function<bool( dialogue & )>> &cond,
                  const std::unordered_map<std::string, std::string> &ctx );
        talker *actor( bool is_beta ) const;

        mutable itype_id cur_item;
        mutable std::string reason;

        std::string dynamic_line( const talk_topic &topic );
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
        bool has_beta;
        bool has_alpha;

        // Methods for setting/getting misc key/value pairs.
        void set_value( const std::string &key, const std::string &value );
        void remove_value( const std::string &key );
        std::string get_value( const std::string &key ) const;

        void set_conditional( const std::string &key, const std::function<bool( dialogue & )> &value );
        bool evaluate_conditional( const std::string &key, dialogue &d );

        const std::unordered_map<std::string, std::string> &get_context() const;
        const std::unordered_map<std::string, std::function<bool( dialogue & )>> &get_conditionals() const;
        void amend_callstack( const std::string &value );
        std::string get_callstack() const;
    private:
        /**
         * The talker that speaks (almost certainly representing the avatar, ie get_avatar() )
         */
        std::unique_ptr<talker> alpha;
        /**
         * The talker responded to alpha, usually a talker_npc.
         */
        std::unique_ptr<talker> beta;

        // dialogue specific variables that can be passed down to additional EOCs but are one way
        lazy<std::unordered_map<std::string, std::string>> context;
        // Weirdly unnecessarily in context.
        std::string callstack;

        // conditionals that were set at the upper level
        lazy<std::unordered_map<std::string, std::function<bool( dialogue & )>>> conditionals;

        /**
         * Add a simple response that switches the topic to the new one. If first == true, force
         * this topic to the front of the responses.
         */
        talk_response &add_response( const std::string &text, const std::string &r,
                                     bool first = false );
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
                                     const dialogue_fun_ptr &effect_success, bool first = false );

        /**
         * Add a simple response that switches the topic to the new one and executes the given
         * action. The response always succeeds. Consequence must be explicitly specified.
         */
        talk_response &add_response( const std::string &text, const std::string &r,
                                     const std::function<void( npc & )> &effect_success,
                                     dialogue_consequence consequence, bool first = false );
        /**
         * Add a simple response that switches the topic to the new one and sets the currently
         * talked about mission to the given one. The mission pointer must be valid.
         */
        talk_response &add_response( const std::string &text, const std::string &r, mission *miss,
                                     bool first = false );
        /**
         * Add a simple response that switches the topic to the new one and sets the currently
         * talked about skill to the given one.
         */
        talk_response &add_response( const std::string &text, const std::string &r, const skill_id &skill,
                                     bool first = false );
        /**
         * Add a simple response that switches the topic to the new one and sets the currently
         * talked about proficiency to the given one.
         */
        talk_response &add_response( const std::string &text, const std::string &r,
                                     const proficiency_id &proficiency,
                                     bool first = false );
        /**
        * Add a simple response that switches the topic to the new one and sets the currently
        * talked about magic spell to the given one.
        */
        talk_response &add_response( const std::string &text, const std::string &r,
                                     const spell_id &sp, bool first = false );
        /**
         * Add a simple response that switches the topic to the new one and sets the currently
         * talked about martial art style to the given one.
         */
        talk_response &add_response( const std::string &text, const std::string &r,
                                     const martialart &style, bool first = false );
        /**
         * Add a simple response that switches the topic to the new one and sets the currently
         * talked about item type to the given one.
         */
        talk_response &add_response( const std::string &text, const std::string &r,
                                     const itype_id &item_type, bool first = false );

        int get_best_quit_response();
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
        std::function<std::string( dialogue & )> function;

    public:
        dynamic_line_t() = default;
        explicit dynamic_line_t( const translation &line );
        explicit dynamic_line_t( const JsonObject &jo );
        explicit dynamic_line_t( const JsonArray &ja );
        static dynamic_line_t from_member( const JsonObject &jo, std::string_view member_name );

        std::string operator()( dialogue &d ) const {
            if( !function ) {
                return std::string{};
            }
            return function( d );
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
        std::function<bool( dialogue & )> condition;
        bool has_condition_ = false;
        bool is_switch = false;
        bool is_default = false;

        // this text appears if the conditions fail explaining why
        translation failure_explanation;
        // the topic to move to on failure
        std::string failure_topic;

        void load_condition( const JsonObject &jo );
        bool test_condition( dialogue &d ) const;

    public:
        json_talk_response() = default;
        explicit json_talk_response( const JsonObject &jo );

        const talk_response &get_actual_response() const;
        bool has_condition() const {
            return has_condition_;
        }

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
        explicit json_talk_repeat_response( const JsonObject &jo );
        bool is_npc = false;
        bool include_containers = false;
        std::vector<itype_id> for_item;
        std::vector<item_category_id> for_category;
        json_talk_response response;
};

class json_dynamic_line_effect
{
    private:
        std::function<bool( dialogue & )> condition;
        talk_effect_t effect;
    public:
        json_dynamic_line_effect( const JsonObject &jo, const std::string &id );
        bool test_condition( dialogue &d ) const;
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
        void load( const JsonObject &jo );

        std::string get_dynamic_line( dialogue &d ) const;
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

        cata::flat_set<std::string> get_directly_reachable_topics( bool only_unconditional ) const;
};

void unload_talk_topics();
void load_talk_topic( const JsonObject &jo );

#endif // CATA_SRC_DIALOGUE_H
