#ifndef DIALOGUE_H
#define DIALOGUE_H

#include "player.h"
#include "output.h"
#include "npc.h"
#include "mission.h"
#include "color.h"
#include <vector>
#include <string>

class martialart;

struct talk_response;
struct talk_function;
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
    WINDOW *win = nullptr;
    /**
     * If true, we are done talking and the dialog ends.
     */
    bool done = false;
    /**
     * This contains the exchanged words, it is basically like the global message log.
     * Each responses of the player character and the NPC are added as are information about
     * what each of them does (e.g. the npc drops their weapon).
     * This will be displayed in the dialog window and should already be translated.
     */
    std::vector<std::string> history;
    std::vector<talk_topic> topic_stack;

    /** Missions that have been assigned by this npc to the player they currently speak to. */
    std::vector<mission*> missions_assigned;

    int opt(std::string challenge, ...);
    talk_topic opt(talk_topic topic);

    dialogue() = default;

    std::string dynamic_line( talk_topic topic ) const;

    /**
     * Possible responses from the player character, filled in @ref gen_responses.
     */
    mutable std::vector<talk_response> responses;
    void gen_responses( talk_topic topic ) const;

private:
    void clear_window_texts();
    void print_history( size_t hilight_lines );
    bool print_responses( int yoffset );
    int choose_response( int hilight_lines );
    /**
     * Folds and adds the folded text to @ref history. Returns the number of added lines.
     */
    size_t add_to_history( const std::string &text );
    /**
     * Add a simple response that switches the topic to the new one.
     */
    talk_response &add_response( const std::string &text, talk_topic r ) const;
    /**
     * Add a response with the result TALK_DONE.
     */
    talk_response &add_response_done( const std::string &text ) const;
    /**
     * Add a response with the result TALK_NONE.
     */
    talk_response &add_response_none( const std::string &text ) const;
    /**
     * Add a simple response that switches the topic to the new one and executes the given
     * action. The response always succeeds.
     */
    talk_response &add_response( const std::string &text, talk_topic r,
                                 void (talk_function::*effect_success)(npc *) ) const;
    /**
     * Add a simple response that switches the topic to the new one and sets the currently
     * talked about mission to the given one. The mission pointer must be valid.
     */
    talk_response &add_response( const std::string &text, talk_topic r, mission *miss ) const;
    /**
     * Add a simple response that switches the topic to the new one and sets the currently
     * talked about skill to the given one. The skill pointer must be valid.
     */
    talk_response &add_response( const std::string &text, talk_topic r, const Skill *skill ) const;
    /**
     * Add a simple response that switches the topic to the new one and sets the currently
     * talked about martial art style to the given one.
     */
    talk_response &add_response( const std::string &text, talk_topic r, const martialart &style ) const;
};

struct talk_function {
    void nothing              (npc *) {};
    void assign_mission       (npc *);
    void mission_success      (npc *);
    void mission_failure      (npc *);
    void clear_mission        (npc *);
    void mission_reward       (npc *);
    void mission_reward_cash  (npc *);
    void mission_favor        (npc *);
    void give_equipment       (npc *);
    void start_trade          (npc *);
    std::string bulk_trade_inquire   (npc *, itype_id);
    void bulk_trade_accept    (npc *, itype_id);
    void assign_base          (npc *);
    void assign_guard         (npc *);
    void stop_guard           (npc *);
    void end_conversation     (npc *);
    void insult_combat        (npc *);
    void reveal_stats         (npc *);
    void follow               (npc *); // p follows u
    void deny_follow          (npc *); // p gets "asked_to_follow"
    void deny_lead            (npc *); // p gets "asked_to_lead"
    void deny_equipment       (npc *); // p gets "asked_for_item"
    void deny_train           (npc *); // p gets "asked_to_train"
    void deny_personal_info   (npc *); // p gets "asked_personal_info"
    void enslave              (npc *) {}; // p becomes slave of u
    void hostile              (npc *); // p turns hostile to u
    void flee                 (npc *);
    void leave                (npc *); // p becomes indifferant
    void stranger_neutral     (npc *); // p is now neutral towards you

    void start_mugging        (npc *);
    void player_leaving       (npc *);

    void drop_weapon          (npc *);
    void player_weapon_away   (npc *);
    void player_weapon_drop   (npc *);

    void lead_to_safety       (npc *);
    void start_training       (npc *);

    void toggle_use_guns      (npc *);
    void toggle_use_silent    (npc *);
    void toggle_use_grenades  (npc *);
    void set_engagement_none  (npc *);
    void set_engagement_close (npc *);
    void set_engagement_weak  (npc *);
    void set_engagement_hit   (npc *);
    void set_engagement_all   (npc *);
};

enum talk_trial_type {
    TALK_TRIAL_NONE, // No challenge here!
    TALK_TRIAL_LIE, // Straight up lying
    TALK_TRIAL_PERSUADE, // Convince them
    TALK_TRIAL_INTIMIDATE, // Physical intimidation
    NUM_TALK_TRIALS
};

/**
 * If not TALK_TRIAL_NONE, it defines how to decide whether the responses succeeds (e.g. the
 * NPC believes the lie). The difficulty is a 0...100 percent chance of success (!), 100 means
 * always success, 0 means never. It is however affected by mutations/traits/bionics/etc. of
 * the player character.
 */
struct talk_trial {
    talk_trial_type type = TALK_TRIAL_NONE;
    int difficulty = 0;

    int calc_chance( const dialogue &d ) const;
    /**
     * Returns a user-friendly representation of @ref type
     */
    const std::string &name() const;
    operator bool() const
    {
        return type != TALK_TRIAL_NONE;
    }
    /**
     * Roll for success or failure of this trial.
     */
    bool roll( dialogue &d ) const;
};

/**
 * This defines possible responses from the player character.
 */
struct talk_response {
    /**
     * What the player character says (literally). Should already be translated and will be
     * displayed. The first character controls the color of it ('*'/'&'/'!').
     */
    std::string text;
    talk_trial trial;
    /**
     * The following values are forwarded to the chatbin of the NPC (see @ref npc_chatbin).
     * Except @ref miss, it is apparently not used but should be a mission type that can create
     * new mission.
     */
    mission *mission_selected = nullptr;
    mission_type_id miss = MISSION_NULL; // If it generates a new mission
    const Skill* skill = nullptr;
    matype_id style;
    /**
     * Defines what happens when the trial succeeds or fails. If trial is
     * TALK_TRIAL_NONE it always succeeds.
     */
    struct effect_t {
        /**
         * How (if at all) the NPCs opinion of the player character (@ref npc::op_of_u) will change.
         */
        npc_opinion opinion;
        /**
         * Function that is called when the response is chosen.
         */
        void (talk_function::*effect)(npc *) = &talk_function::nothing;
        /**
         * Topic to switch to. TALK_DONE ends the talking, TALK_NONE keeps the current topic.
         */
        talk_topic topic = TALK_NONE;

        talk_topic apply( dialogue &d ) const;
    };
    effect_t success;
    effect_t failure;

    /**
     * Text (already folded) and color that is used to display this response.
     * This is set up in @ref do_formatting.
     */
    std::vector<std::string> formated_text;
    nc_color color = c_white;

    void do_formatting( const dialogue &d, char letter );

    talk_response() = default;
};

struct talk_response_list {
    std::vector<talk_response> none(npc *);
    std::vector<talk_response> shelter(npc *);
    std::vector<talk_response> shopkeep(npc *);
};

/* There is a array of tag_data, "tags", at the bottom of this file.
 * It maps tags to the array of string replacements;
 * e.g. "<name_g>" => talk_good_names
 * Other tags, like "<yrwp>", are mapped to dynamic things
 *  (like the player's weapon), and are defined in parse_tags() (npctalk.cpp)
 */
struct tag_data {
    std::string tag;
    std::string (*replacement)[10];
};

#endif
