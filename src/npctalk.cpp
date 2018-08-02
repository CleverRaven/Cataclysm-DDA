#include "npc.h"
#include "npc_class.h"
#include "auto_pickup.h"
#include "output.h"
#include "game.h"
#include "map.h"
#include "dialogue.h"
#include "rng.h"
#include "line.h"
#include "debug.h"
#include "catacharset.h"
#include "messages.h"
#include "mission.h"
#include "morale_types.h"
#include "ammo.h"
#include "units.h"
#include "overmapbuffer.h"
#include "json.h"
#include "vpart_position.h"
#include "translations.h"
#include "martialarts.h"
#include "input.h"
#include "item_group.h"
#include "compatibility.h"
#include "basecamp.h"
#include "cata_utility.h"
#include "itype.h"
#include "text_snippets.h"
#include "map_selector.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "skill.h"
#include "ui.h"
#include "help.h"
#include "coordinate_conversions.h"
#include "overmap.h"
#include "editmap.h"

#include "string_formatter.h"
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

const skill_id skill_speech( "speech" );
const skill_id skill_barter( "barter" );

const efftype_id effect_allow_sleep( "allow_sleep" );
const efftype_id effect_asked_for_item( "asked_for_item" );
const efftype_id effect_asked_personal_info( "asked_personal_info" );
const efftype_id effect_asked_to_follow( "asked_to_follow" );
const efftype_id effect_asked_to_lead( "asked_to_lead" );
const efftype_id effect_asked_to_train( "asked_to_train" );
const efftype_id effect_bite( "bite" );
const efftype_id effect_bleed( "bleed" );
const efftype_id effect_currently_busy( "currently_busy" );
const efftype_id effect_gave_quest_item( "gave_quest_item" );
const efftype_id effect_infected( "infected" );
const efftype_id effect_infection( "infection" );
const efftype_id effect_lying_down( "lying_down" );
const efftype_id effect_sleep( "sleep" );

static const trait_id trait_DEBUG_MIND_CONTROL( "DEBUG_MIND_CONTROL" );
static const trait_id trait_PROF_FED( "PROF_FED" );

struct dialogue;

enum talk_trial_type {
    TALK_TRIAL_NONE, // No challenge here!
    TALK_TRIAL_LIE, // Straight up lying
    TALK_TRIAL_PERSUADE, // Convince them
    TALK_TRIAL_INTIMIDATE, // Physical intimidation
    NUM_TALK_TRIALS
};

enum class dialogue_consequence {
    none = 0,
    hostile,
    helpless,
    action
};

using dialogue_fun_ptr = std::add_pointer<void( npc & )>::type;

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

/**
 * This defines possible responses from the player character.
 */
struct talk_response {
    /**
     * What the player character says (literally). Should already be translated and will be
     * displayed.
     */
    std::string text;
    talk_trial trial;
    /**
     * The following values are forwarded to the chatbin of the NPC (see @ref npc_chatbin).
     */
    mission *mission_selected = nullptr;
    skill_id skill = skill_id::NULL_ID();
    matype_id style = matype_id::NULL_ID();
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
             * Topic to switch to. TALK_DONE ends the talking, TALK_NONE keeps the current topic.
             */
            talk_topic next_topic = talk_topic( "TALK_NONE" );

            talk_topic apply( dialogue &d ) const;
            dialogue_consequence get_consequence( const dialogue &d ) const;

            const std::function<void( npc & )> &get_effect() const {
                return effect;
            }

            /**
             * Sets the effect and consequence based on function pointer.
             */
            void set_effect( dialogue_fun_ptr effect );
            /**
             * Sets the effect to a function object and consequence to explicitly given one.
             */
            void set_effect_consequence( std::function<void( npc & )> eff, dialogue_consequence con );

            void load_effect( JsonObject &jo );

            effect_t() = default;
            effect_t( JsonObject );
        private:
            /**
             * Function that is called when the response is chosen.
             */
            std::function<void( npc & )> effect = &talk_function::nothing;
            dialogue_consequence guaranteed_consequence = dialogue_consequence::none;
    };
    effect_t success;
    effect_t failure;

    /**
     * Text (already folded) and color that is used to display this response.
     * This is set up in @ref do_formatting.
     */
    std::vector<std::string> formatted_text;
    nc_color color = c_white;

    void do_formatting( const dialogue &d, char letter );
    std::set<dialogue_consequence> get_consequences( const dialogue &d ) const;

    talk_response() = default;
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
        catacurses::window win;
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
        std::vector<mission *> missions_assigned;

        talk_topic opt( const talk_topic &topic );

        dialogue() = default;

        std::string dynamic_line( const talk_topic &topic ) const;

        /**
         * Possible responses from the player character, filled in @ref gen_responses.
         */
        std::vector<talk_response> responses;
        void gen_responses( const talk_topic &topic );

        void add_topic( const std::string &topic );
        void add_topic( const talk_topic &topic );

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
        talk_response &add_response( const std::string &text, const std::string &r );
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
                                     dialogue_fun_ptr effect_success );

        /**
         * Add a simple response that switches the topic to the new one and executes the given
         * action. The response always succeeds. Consequence must be explicitly specified.
         */
        talk_response &add_response( const std::string &text, const std::string &r,
                                     std::function<void( npc & )> effect_success,
                                     dialogue_consequence consequence );
        /**
         * Add a simple response that switches the topic to the new one and sets the currently
         * talked about mission to the given one. The mission pointer must be valid.
         */
        talk_response &add_response( const std::string &text, const std::string &r, mission *miss );
        /**
         * Add a simple response that switches the topic to the new one and sets the currently
         * talked about skill to the given one.
         */
        talk_response &add_response( const std::string &text, const std::string &r, const skill_id &skill );
        /**
         * Add a simple response that switches the topic to the new one and sets the currently
         * talked about martial art style to the given one.
         */
        talk_response &add_response( const std::string &text, const std::string &r,
                                     const martialart &style );
        /**
         * Add a simple response that switches the topic to the new one and sets the currently
         * talked about item type to the given one.
         */
        talk_response &add_response( const std::string &text, const std::string &r,
                                     const itype_id &item_type );
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
/**
 * An extended response. It contains the response itself and a condition, so we can include the
 * response if, and only if the condition is met.
 */
class json_talk_response
{
    private:
        talk_response actual_response;
        std::function<bool( const dialogue & )> condition;

        void load_condition( JsonObject &jo );
        bool test_condition( const dialogue &d ) const;

    public:
        json_talk_response( JsonObject jo );

        /**
         * Callback from @ref json_talk_topic::gen_responses, see there.
         */
        void gen_responses( dialogue &d ) const;
};
/**
 * Talk topic definitions load from json.
 */
class json_talk_topic
{
    public:

    private:
        bool replace_built_in_responses = false;
        std::vector<json_talk_response> responses;
        dynamic_line_t dynamic_line;

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

struct item_pricing {
    item_pricing( Character &c, item *it, int v, bool s ) : loc( c, it ), price( v ), selected( s ) {
    }

    item_pricing( item_location &&l, int v, bool s ) : loc( std::move( l ) ), price( v ),
        selected( s ) {
    }

    item_location loc;
    int price;
    // Whether this is selected for trading, init_buying and init_selling initialize
    // this to `false`.
    bool selected;
};

static std::map<std::string, json_talk_topic> json_talk_topics;

// Every OWED_VAL that the NPC owes you counts as +1 towards convincing
#define OWED_VAL 1000

// Some aliases to help with gen_responses
#define RESPONSE(txt)      ret.push_back(talk_response());\
    ret.back().text = txt

#define TRIAL(tr, diff) ret.back().trial.type = tr;\
    ret.back().trial.difficulty = diff

#define SUCCESS(topic_)  ret.back().success.next_topic = talk_topic( topic_ )
#define FAILURE(topic_)  ret.back().failure.next_topic = talk_topic( topic_ )

// trust (T), fear (F), value (V), anger(A), owed (O) { };
#define SUCCESS_OPINION(T, F, V, A, O)   ret.back().success.opinion =\
        npc_opinion(T, F, V, A, O)
#define FAILURE_OPINION(T, F, V, A, O)   ret.back().failure.opinion =\
        npc_opinion(T, F, V, A, O)

#define SUCCESS_ACTION(func)  ret.back().success.set_effect( func )
#define FAILURE_ACTION(func)  ret.back().failure.set_effect( func )

#define SUCCESS_ACTION_CONSEQUENCE(func, con)  ret.back().success.set_effect_consequence( func, con )
#define FAILURE_ACTION_CONSEQUENCE(func, con)  ret.back().failure.set_effect_consequence( func, con )

#define dbg(x) DebugLog((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

int topic_category( const talk_topic &topic );

const talk_topic &special_talk( char ch );

bool trade( npc &p, int cost, const std::string &deal );
std::vector<item_pricing> init_selling( npc &p );
std::vector<item_pricing> init_buying( npc &p, player &u );

std::string give_item_to( npc &p, bool allow_use, bool allow_carry );

std::string bulk_trade_inquire( const npc &, const itype_id &it );
void bulk_trade_accept( npc &, const itype_id &it );

const std::string &talk_trial::name() const
{
    static std::array<std::string, NUM_TALK_TRIALS> const texts = { {
            "", _( "LIE" ), _( "PERSUADE" ), _( "INTIMIDATE" )
        }
    };
    if( static_cast<size_t>( type ) >= texts.size() ) {
        debugmsg( "invalid trial type %d", static_cast<int>( type ) );
        return texts[0];
    }
    return texts[type];
}

/** Time (in turns) and cost (in cent) for training: */
static time_duration calc_skill_training_time( const npc &p, const skill_id &skill )
{
    return 1_minutes + 5_turns * g->u.get_skill_level( skill ) - 1_turns * p.get_skill_level( skill );
}

static int calc_skill_training_cost( const npc &p, const skill_id &skill )
{
    if( p.is_friend() ) {
        return 0;
    }

    return 1000 * ( 1 + g->u.get_skill_level( skill ) ) * ( 1 + g->u.get_skill_level( skill ) );
}

// TODO: all styles cost the same and take the same time to train,
// maybe add values to the ma_style class to makes this variable
// TODO: maybe move this function into the ma_style class? Or into the NPC class?
static time_duration calc_ma_style_training_time( const npc &, const matype_id & /* id */ )
{
    return 30_minutes;
}

static int calc_ma_style_training_cost( const npc &p, const matype_id & /* id */ )
{
    if( p.is_friend() ) {
        return 0;
    }

    return 800;
}

// Rescale values from "mission scale" to "opinion scale"
static int cash_to_favor( const npc &, int cash )
{
    // @todo: It should affect different NPCs to a different degree
    // Square root of mission value in dollars
    // ~31 for zed mom, 50 for horde master, ~63 for plutonium cells
    double scaled_mission_val = sqrt( cash / 100.0 );
    return roll_remainder( scaled_mission_val );
}

void npc_chatbin::check_missions()
{
    // TODO: or simply fail them? Some missions might only need to be reported.
    auto &ma = missions_assigned;
    auto const last = std::remove_if( ma.begin(), ma.end(), []( class mission const * m ) {
        return !m->is_assigned();
    } );
    std::copy( last, ma.end(), std::back_inserter( missions ) );
    ma.erase( last, ma.end() );
}

void npc::talk_to_u()
{
    if( g->u.is_dead_state() ) {
        set_attitude( NPCATT_NULL );
        return;
    }
    const bool has_mind_control = g->u.has_trait( trait_DEBUG_MIND_CONTROL );
    // This is necessary so that we don't bug the player over and over
    if( get_attitude() == NPCATT_TALK ) {
        set_attitude( NPCATT_NULL );
    } else if( get_attitude() == NPCATT_FLEE && !has_mind_control ) {
        add_msg( _( "%s is fleeing from you!" ), name.c_str() );
        return;
    } else if( get_attitude() == NPCATT_KILL && !has_mind_control ) {
        add_msg( _( "%s is hostile!" ), name.c_str() );
        return;
    }
    dialogue d;
    d.alpha = &g->u;
    d.beta = this;

    chatbin.check_missions();

    for( auto &mission : chatbin.missions_assigned ) {
        if( mission->get_assigned_player_id() == g->u.getID() ) {
            d.missions_assigned.push_back( mission );
        }
    }

    d.add_topic( chatbin.first_topic );

    if( is_leader() ) {
        d.add_topic( "TALK_LEADER" );
    } else if( is_friend() ) {
        d.add_topic( "TALK_FRIEND" );
    }

    int most_difficult_mission = 0;
    for( auto &mission : chatbin.missions ) {
        const auto &type = mission->get_type();
        if( type.urgent && type.difficulty > most_difficult_mission ) {
            d.add_topic( "TALK_MISSION_DESCRIBE" );
            chatbin.mission_selected = mission;
            most_difficult_mission = type.difficulty;
        }
    }
    most_difficult_mission = 0;
    bool chosen_urgent = false;
    for( auto &mission : chatbin.missions_assigned ) {
        if( mission->get_assigned_player_id() != g->u.getID() ) {
            // Not assigned to the player that is currently talking to the npc
            continue;
        }
        const auto &type = mission->get_type();
        if( ( type.urgent && !chosen_urgent ) || ( type.difficulty > most_difficult_mission &&
                ( type.urgent || !chosen_urgent ) ) ) {
            chosen_urgent = type.urgent;
            d.add_topic( "TALK_MISSION_INQUIRE" );
            chatbin.mission_selected = mission;
            most_difficult_mission = type.difficulty;
        }
    }
    if( chatbin.mission_selected != nullptr ) {
        if( chatbin.mission_selected->get_assigned_player_id() != g->u.getID() ) {
            // Don't talk about a mission that is assigned to someone else.
            chatbin.mission_selected = nullptr;
        }
    }
    if( chatbin.mission_selected == nullptr ) {
        // if possible, select a mission to talk about
        if( !chatbin.missions.empty() ) {
            chatbin.mission_selected = chatbin.missions.front();
        } else if( !d.missions_assigned.empty() ) {
            chatbin.mission_selected = d.missions_assigned.front();
        }
    }

    // Needs
    if( has_effect( effect_sleep ) || has_effect( effect_lying_down ) ) {
        d.add_topic( "TALK_WAKE_UP" );
    }

    if( d.topic_stack.back().id == "TALK_NONE" ) {
        d.topic_stack.back() = talk_topic( pick_talk_topic( g->u ) );
    }

    moves -= 100;

    if( g->u.is_deaf() ) {
        if( d.topic_stack.back().id == "TALK_MUG" ||
            d.topic_stack.back().id == "TALK_STRANGER_AGGRESSIVE" ) {
            make_angry();
            d.add_topic( "TALK_DEAF_ANGRY" );
        } else {
            d.add_topic( "TALK_DEAF" );
        }
    }

    decide_needs();

    d.win = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                ( TERMY > FULL_SCREEN_HEIGHT ) ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0,
                                ( TERMX > FULL_SCREEN_WIDTH ) ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0 );

    // Main dialogue loop
    do {
        draw_border( d.win );
        mvwvline( d.win, 1, ( FULL_SCREEN_WIDTH / 2 ) + 1, LINE_XOXO, FULL_SCREEN_HEIGHT - 1 );
        mvwputch( d.win, 0, ( FULL_SCREEN_WIDTH / 2 ) + 1, BORDER_COLOR, LINE_OXXX );
        mvwputch( d.win, FULL_SCREEN_HEIGHT - 1, ( FULL_SCREEN_WIDTH / 2 ) + 1, BORDER_COLOR, LINE_XXOX );
        mvwprintz( d.win, 1,  1, c_white, _( "Dialogue: %s" ), name.c_str() );
        mvwprintz( d.win, 1, ( FULL_SCREEN_WIDTH / 2 ) + 3, c_white, _( "Your response:" ) );
        const talk_topic next = d.opt( d.topic_stack.back() );
        if( next.id == "TALK_NONE" ) {
            int cat = topic_category( d.topic_stack.back() );
            do {
                d.topic_stack.pop_back();
            } while( cat != -1 && topic_category( d.topic_stack.back() ) == cat );
        }
        if( next.id == "TALK_DONE" || d.topic_stack.empty() ) {
            d.done = true;
        } else if( next.id != "TALK_NONE" ) {
            d.add_topic( next );
        }
    } while( !d.done );
    g->refresh_all();

    if( g->u.activity.id() == activity_id( "ACT_AIM" ) && !g->u.has_weapon() ) {
        g->u.cancel_activity();

        // Don't query if we're training the player
    } else if( g->u.activity.id() != activity_id( "ACT_TRAIN" ) || g->u.activity.index != getID() ) {
        g->cancel_activity_query( string_format( _( "%s talked to you." ), name.c_str() ) );
    }
}

std::string dialogue::dynamic_line( const talk_topic &the_topic ) const
{
    // For compatibility
    const auto &topic = the_topic.id;
    auto const iter = json_talk_topics.find( topic );
    if( iter != json_talk_topics.end() ) {
        const std::string line = iter->second.get_dynamic_line( *this );
        if( !line.empty() ) {
            return line;
        }
    }

    if( topic == "TALK_DEAF" ) {
        return _( "&You are deaf and can't talk." );

    } else if( topic == "TALK_DEAF_ANGRY" ) {
        return string_format(
                   _( "&You are deaf and can't talk. When you don't respond, %s becomes angry!" ),
                   beta->name.c_str() );
    }

    const auto &p = beta; // for compatibility, later replace it in the code below
    // Those topics are handled by the mission system, see there.
    static const std::unordered_set<std::string> mission_topics = { {
            "TALK_MISSION_DESCRIBE", "TALK_MISSION_OFFER", "TALK_MISSION_ACCEPTED",
            "TALK_MISSION_REJECTED", "TALK_MISSION_ADVICE", "TALK_MISSION_INQUIRE",
            "TALK_MISSION_SUCCESS", "TALK_MISSION_SUCCESS_LIE", "TALK_MISSION_FAILURE"
        }
    };
    if( mission_topics.count( topic ) > 0 ) {
        if( p->chatbin.mission_selected == nullptr ) {
            return "mission_selected == nullptr; BUG! (npctalk.cpp:dynamic_line)";
        }
        mission *miss = p->chatbin.mission_selected;
        const auto &type = miss->get_type();
        // TODO: make it a member of the mission class, maybe at mission instance specific data
        const std::string &ret = miss->dialogue_for_topic( topic );
        if( ret.empty() ) {
            debugmsg( "Bug in npctalk.cpp:dynamic_line. Wrong mission_id(%s) or topic(%s)",
                      type.id.c_str(), topic.c_str() );
            return "";
        }

        if( topic == "TALK_MISSION_SUCCESS" && miss->has_follow_up() ) {
            switch( rng( 1, 3 ) ) {
                case 1:
                    return ret + _( "  And I have more I'd like you to do." );
                case 2:
                    return ret + _( "  I could use a hand with something else if you are interested." );
                case 3:
                    return ret + _( "  If you are interested, I have another job for you." );
            }
        }

        return ret;
    }

    if( topic == "TALK_NONE" || topic == "TALK_DONE" ) {
        return _( "Bye." );

    } else if( topic == "TALK_GUARD" ) {
        switch( rng( 1, 5 ) ) {
            case 1:
                return _( "I'm not in charge here, you're looking for someone else..." );
            case 2:
                return _( "Keep civil or I'll bring the pain." );
            case 3:
                return _( "Just on watch, move along." );
            case 4:
                if( g->u.male ) {
                    return _( "Sir." );
                } else {
                    return _( "Ma'am" );
                }
            case 5:
                if( g->u.male ) {
                    return _( "Rough out there, isn't it?" );
                } else {
                    return _( "Ma'am, you really shouldn't be traveling out there." );
                }
        }
    } else if( topic == "TALK_MISSION_LIST" ) {
        if( p->chatbin.missions.empty() ) {
            if( missions_assigned.empty() ) {
                return _( "I don't have any jobs for you." );
            } else {
                return _( "I don't have any more jobs for you." );
            }
        } else if( p->chatbin.missions.size() == 1 ) {
            if( missions_assigned.empty() ) {
                return _( "I just have one job for you.  Want to hear about it?" );
            } else {
                return _( "I have another job for you.  Want to hear about it?" );
            }
        } else if( missions_assigned.empty() ) {
            return _( "I have several jobs for you.  Which should I describe?" );
        } else {
            return _( "I have several more jobs for you.  Which should I describe?" );
        }

    } else if( topic == "TALK_MISSION_LIST_ASSIGNED" ) {
        if( missions_assigned.empty() ) {
            return _( "You're not working on anything for me right now." );
        } else if( missions_assigned.size() == 1 ) {
            return _( "What about it?" );
        } else {
            return _( "Which job?" );
        }

    } else if( topic == "TALK_MISSION_REWARD" ) {
        return _( "Sure, here you go!" );

    } else if( topic == "TALK_EVAC_HUNTER_ADVICE" ) {
        switch( rng( 1, 7 ) ) {
            case 1:
                return _( "Feed a man a fish, he's full for a day. Feed a man a bullet, "
                          "he's full for the rest of his life." );
            case 2:
                return _( "Spot your prey before something nastier spots you." );
            case 3:
                return _( "I've heard that cougars sometimes leap. Maybe it's just a myth." );
            case 4:
                return _( "The Jabberwock is real, don't listen to what anybody else says. "
                          "If you see it, RUN." );
            case 5:
                return _( "Zombie animal meat isn't good for eating, but sometimes you "
                          "might find usable fur on 'em." );
            case 6:
                return _( "A steady diet of cooked meat and clean water will keep you "
                          "alive forever, but your taste buds and your colon may start "
                          "to get angry at you. Eat a piece of fruit every once in a while." );
            case 7:
                return _( "Smoke crack to get more shit done." );
        }

    } else if( topic == "TALK_OLD_GUARD_SOLDIER" ) {
        if( g->u.is_wearing( "badge_marshal" ) )
            switch( rng( 1, 4 ) ) {
                case 1:
                    return _( "Hello, marshal." );
                case 2:
                    return _( "Marshal, I'm afraid I can't talk now." );
                case 3:
                    return _( "I'm not in charge here, marshal." );
                case 4:
                    return _( "I'm supposed to direct all questions to my leadership, marshal." );
            }
        switch( rng( 1, 5 ) ) {
            case 1:
                return _( "Hey, citizen... I'm not sure you belong here." );
            case 2:
                return _( "You should mind your own business, nothing to see here." );
            case 3:
                return _( "If you need something you'll need to talk to someone else." );
            case 4:
                if( g->u.male ) {
                    return _( "Sir." );
                } else {
                    return _( "Ma'am" );
                }
            case 5:
                if( g->u.male ) {
                    return _( "Dude, if you can hold your own you should look into enlisting." );
                } else {
                    return _( "Hey miss, don't you think it would be safer if you stuck with me?" );
                }
        }

    } else if( topic == "TALK_OLD_GUARD_NEC_CPT" ) {
        if( g->u.has_trait( trait_PROF_FED ) ) {
            return _( "Marshal, I hope you're here to assist us." );
        }
        if( g->u.male ) {
            return _( "Sir, I don't know how the hell you got down here but if you have any sense you'll get out while you can." );
        }
        return _( "Ma'am, I don't know how the hell you got down here but if you have any sense you'll get out while you can." );

    } else if( topic == "TALK_OLD_GUARD_NEC_CPT_GOAL" ) {
        return _( "I'm leading what remains of my company on a mission to re-secure this facility.  We entered the complex with two "
                  "dozen men and immediately went about securing this control room.  From here I dispatched my men to secure vital "
                  "systems located on this floor and the floors below this one.  If  we are successful, this facility can be cleared "
                  "and used as a permanent base of operations in the region.  Most importantly it will allow us to redirect refugee "
                  "traffic away from overcrowded outposts and free up more of our forces to conduct recovery operations." );

    } else if( topic == "TALK_OLD_GUARD_NEC_CPT_VAULT" ) {
        return _( "This facility was constructed to provide a safe haven in the event of a global conflict.  The vault can support "
                  "several thousand people for a few years if all systems are operational and sufficient notification is given.  "
                  "Unfortunately, the power system was damaged or sabotaged at some point and released a single extremely lethal "
                  "burst of radiation.  The catastrophic event lasted for several minutes and resulted in the deaths of most people "
                  "located on the 2nd and lower floors.  Those working on this floor were able to seal the access ways to the "
                  "lower floors before succumbing to radiation sickness.  The only other thing the logs tell us is that all water "
                  "pressure was diverted to the lower levels." );

    } else if( topic == "TALK_OLD_GUARD_NEC_COMMO" ) {
        if( g->u.has_trait( trait_PROF_FED ) ) {
            return _( "Marshal, I'm rather surprised to see you here." );
        }
        if( g->u.male ) {
            return _( "Sir you are not authorized to be here... you should leave." );
        }
        return _( "Ma'am you are not authorized to be here... you should leave." );

    } else if( topic == "TALK_OLD_GUARD_NEC_COMMO_GOAL" ) {
        return _( "We are securing the external communications array for this facility.  I'm rather restricted in what I can release"
                  "... go find my commander if you have any questions." );

    } else if( topic == "TALK_OLD_GUARD_NEC_COMMO_FREQ" ) {
        return _( "I was expecting the captain to send a runner.  Here is the list you are looking for.  What we can identify from here are simply the "
                  "frequencies that have traffic on them.  Many of the transmissions are indecipherable without repairing or "
                  "replacing the equipment here.  When the facility was being overrun, standard procedure was to destroy encryption "
                  "hardware to protect federal secrets and maintain the integrity of the comms network.  We are hoping a few plain "
                  "text messages can get picked up though." );

    } else if( topic == "TALK_FREE_MERCHANT_STOCKS" ) {
        return _( "Hope you're here to trade." );

    } else if( topic == "TALK_FREE_MERCHANT_STOCKS_NEW" ) {
        return _( "I oversee the food stocks for the center.  There was significant looting during "
                  "the panic when we first arrived so most of our food was carried away.  I manage "
                  "what we have left and do everything I can to increase our supplies.  Rot and mold "
                  "are more significant in the damp basement so I prioritize non-perishable food, "
                  "such as cornmeal, jerky, and fruit wine." );

    } else if( topic == "TALK_FREE_MERCHANT_STOCKS_WHY" ) {
        return _( "All three are easy to locally produce in significant quantities and are "
                  "non-perishable.  We have a local farmer or two and a few hunter types that have "
                  "been making attempts to provide us with the nutritious supplies.  We do always "
                  "need more suppliers though.  Because this stuff is rather cheap in bulk I can "
                  "pay a premium for any you have on you.  Canned food and other edibles are "
                  "handled by the merchant in the front." );

    } else if( topic == "TALK_FREE_MERCHANT_STOCKS_ALL" ) {
        return _( "I'm actually accepting a number of different foodstuffs: beer, sugar, flour, "
                  "smoked meat, smoked fish, cooking oil; and as mentioned before, jerky, cornmeal, "
                  "and fruit wine." );

    } else if( topic == "TALK_DELIVER_ASK" ) {
        return bulk_trade_inquire( *p, the_topic.item_type );

    } else if( topic == "TALK_DELIVER_CONFIRM" ) {
        return _( "Pleasure doing business!" );

    } else if( topic == "TALK_RANCH_FOREMAN" ) {
        if( g->u.has_trait( trait_PROF_FED ) ) {
            return _( "Can I help you, marshal?" );
        }
        if( g->u.male ) {
            return _( "Morning sir, how can I help you?" );
        }
        return _( "Morning ma'am, how can I help you?" );

    } else if( topic == "TALK_RANCH_FOREMAN_PROSPECTUS" ) {
        return _( "I was starting to wonder if they were really interested in the "
                  "project or were just trying to get rid of me." );

    } else if( topic == "TALK_RANCH_FOREMAN_OUTPOST" ) {
        return _( "Ya, that representative from the Old Guard asked the two "
                  "of us to come out here and begin fortifying this place as "
                  "a refugee camp.  I'm not sure how fast he expects the two "
                  "of us to get setup but we were assured additional men were "
                  "coming out here to assist us. " );

    } else if( topic == "TALK_RANCH_FOREMAN_REFUGEES" ) {
        return _( "Could easily be hundreds as far as I know.  They chose this "
                  "ranch because of its rather remote location, decent fence, "
                  "and huge cleared field.  With as much land as we have fenced "
                  "off we could build a village if we had the materials.  We "
                  "would have tried to secure a small town or something but the "
                  "lack of good farmland and number of undead makes it more "
                  "practical for us to build from scratch.  The refugee center I "
                  "came from is constantly facing starvation and undead assaults." );

    } else if( topic == "TALK_RANCH_FOREMAN_JOB" ) {
        return _( "I'm the engineer in charge of turning this place into a working "
                  "camp.  This is going to be an uphill battle, we used most of our "
                  "initial supplies getting here and boarding up the windows.  I've "
                  "got a huge list of tasks that need to get done so if you could "
                  "help us keep supplied I'd appreciate it.  If you have material to "
                  "drop off you can just back your vehicle into here and dump it on "
                  "the ground, we'll sort it." );

    } else if( topic == "TALK_RANCH_CONSTRUCTION_1" ) {
        return _( "My partner is in charge of fortifying this place, you should ask him about what needs to be done." );

    } else if( topic == "TALK_RANCH_CONSTRUCTION_2" ) {
        return _( "Howdy." );

    } else if( topic == "TALK_RANCH_CONSTRUCTION_2_JOB" ) {
        return _( "I was among one of the first groups of immigrants sent here to fortify the outpost.  I might "
                  "have exaggerated my construction skills to get the hell out of the refugee center.  Unless you "
                  "are a trader there isn't much work there and food was really becoming scarce when I left." );

    } else if( topic == "TALK_RANCH_WOODCUTTER" ) {
        return _( "You need something?" );

    } else if( topic == "TALK_RANCH_WOODCUTTER_JOB" ) {
        return _( "I'm one of the migrants that got diverted to this outpost when I arrived at the refugee "
                  "center.  They said I was big enough to swing an ax so my profession became lumberjack"
                  "... didn't have any say in it.  If I want to eat then I'll be cutting wood from now till "
                  "kingdom come." );

    } else if( topic == "TALK_RANCH_WOODCUTTER_HIRE" ) {
        if( p->has_effect( effect_currently_busy ) ) {
            return _( "Come back later, I need to take care of a few things first." );
        } else {
            return _( "The rate is a bit steep but I still have my quotas that I need to fulfill.  The logs will "
                      "be dropped off in the garage at the entrance to the camp.  I'll need a bit of time before "
                      "I can deliver another load." );
        }

    } else if( topic == "TALK_RANCH_WOODCUTTER_2" ) {
        return _( "Don't have much time to talk." );

    } else if( topic == "TALK_RANCH_WOODCUTTER_2_JOB" ) {
        return _( "I turn the logs that laborers bring in into lumber to expand the outpost.  Maintaining the "
                  "saw is a chore but breaks the monotony." );

    } else if( topic == "TALK_RANCH_WOODCUTTER_2_HIRE" ) {
        return _( "Bringing in logs is one of the few tasks we can give to the unskilled so I'd be hurting "
                  "them if I outsourced it.  Ask around though, I'm sure most people could use a hand." );

    } else if( topic == "TALK_RANCH_FARMER_1" ) {
        return _( "..." );

    } else if( topic == "TALK_RANCH_FARMER_1_JOB" ) {
        return _( "I was sent here to assist in setting-up the farm.  Most of us have no real skills that "
                  "transfer from before the cataclysm so things are a bit of trial and error." );

    } else if( topic == "TALK_RANCH_FARMER_1_HIRE" ) {
        return _( "I'm sorry, I don't have anything to trade.  The work program here splits what we produce "
                  "between the refugee center, the farm, and ourselves.  If you are a skilled laborer then "
                  "you can trade your time for a bit of extra income on the side.  Not much I can do to assist "
                  "you as a farmer though." );

    } else if( topic == "TALK_RANCH_FARMER_2" ) {
        return _( "You mind?" );

    } else if( topic == "TALK_RANCH_FARMER_2_JOB" ) {
        return _( "I'm just a lucky guy that went from being chased by the undead to the noble life of a dirt "
                  "farmer.  We get room and board but won't see a share of our labor unless the crop is a success." );

    } else if( topic == "TALK_RANCH_FARMER_2_HIRE" ) {
        return _( "I've got no time for you.  If you want to make a trade or need a job look for the foreman or crop overseer." );

    } else if( topic == "TALK_RANCH_CROP_OVERSEER" ) {
        return _( "I hope you are here to do business." );

    } else if( topic == "TALK_RANCH_CROP_OVERSEER_JOB" ) {
        return _( "My job is to manage our outpost's agricultural production.  I'm constantly searching for trade "
                  "partners and investors to increase our capacity.  If you are interested I typically have tasks "
                  "that I need assistance with." );

    } else if( topic == "TALK_RANCH_ILL_1" ) {
        return _( "Please leave me alone..." );

    } else if( topic == "TALK_RANCH_ILL_1_JOB" ) {
        return _( "I was just a laborer till they could find me something a bit more permanent but being "
                  "constantly sick has prevented me from doing much of anything." );

    } else if( topic == "TALK_RANCH_ILL_1_HIRE" ) {
        return _( "I don't know what you could do.  I've tried everything.  Just give me time..." );

    } else if( topic == "TALK_RANCH_ILL_1_SICK" ) {
        return _( "I keep getting sick!  At first I thought it was something I ate but now it seems like I can't "
                  "keep anything down..." );

    } else if( topic == "TALK_RANCH_NURSE" ) {
        return _( "How can I help you?" );

    } else if( topic == "TALK_RANCH_NURSE_JOB" ) {
        return _( "I was a practicing nurse so I've taken over the medical responsibilities of the outpost "
                  "till we can locate a physician." );

    } else if( topic == "TALK_RANCH_NURSE_HIRE" ) {
        return _( "I'm willing to pay a premium for medical supplies that you might be able to scavenge up.  "
                  "I also have a few miscellaneous jobs from time to time." );

    } else if( topic == "TALK_RANCH_NURSE_AID" ) {
        if( p->has_effect( effect_currently_busy ) ) {
            return _( "Come back later, I need to take care of a few things first." );
        } else {
            return _( "I can take a look at you or your companions if you are injured." );
        }

    } else if( topic == "TALK_RANCH_NURSE_AID_DONE" ) {
        return _( "That's the best I can do on short notice..." );

    } else if( topic == "TALK_RANCH_DOCTOR" ) {
        return _( "I'm sorry, I don't have time to see you at the moment." );

    } else if( topic == "TALK_RANCH_DOCTOR_BIONICS" ) {
        return _( "I imagine we might be able to work something out..." );

    } else if( topic == "TALK_RANCH_SCRAPPER" ) {
        return _( "Don't mind me." );

    } else if( topic == "TALK_RANCH_SCRAPPER_JOB" ) {
        return _( "I chop up useless vehicles for spare parts and raw materials.  If we can't use a "
                  "vehicle immediately we haul it into the ring we are building to surround the outpost... "
                  "it provides a measure of defense in the event that we get attacked." );

    } else if( topic == "TALK_RANCH_SCRAPPER_HIRE" ) {
        return _( "I don't personally, the teams we send out to recover the vehicles usually need a hand "
                  "but can be hard to catch since they spend most of their time outside the outpost." );

    } else if( topic == "TALK_RANCH_SCAVENGER_1" ) {
        return _( "Welcome to the junk shop..." );

    } else if( topic == "TALK_RANCH_SCAVENGER_1_JOB" ) {
        return _( "I organize scavenging runs to bring in supplies that we can't produce ourselves.  I "
                  "try and provide incentives to get migrants to join one of the teams... its dangerous "
                  "work but keeps our outpost alive.  Selling anything we can't use helps keep us afloat "
                  "with the traders.  If you wanted to drop off a companion or two to assist in one of "
                  "the runs, I'd appreciate it." );

    } else if( topic == "TALK_RANCH_SCAVENGER_1_HIRE" ) {
        return _( "Are you interested in the scavenging runs or one of the other tasks that I might have for you?" );

    } else if( topic == "TALK_RANCH_BARKEEP" ) {
        return _( "Want a drink?" );

    } else if( topic == "TALK_RANCH_BARKEEP_JOB" ) {
        return _( "If it isn't obvious, I oversee the bar here.  The scavengers bring in old world alcohol that we "
                  "sell for special occasions.  For most that come through here though, the drinks we brew "
                  "ourselves are the only thing they can afford." );

    } else if( topic == "TALK_RANCH_BARKEEP_INFORMATION" ) {
        return _( "We have a policy of keeping information to ourselves.  Ask the patrons if you want to hear "
                  "rumors or news." );

    } else if( topic == "TALK_RANCH_BARKEEP_TAP" ) {
        return _( "Our selection is a bit limited at the moment." );

    } else if( topic == "TALK_RANCH_BARBER" ) {
        return _( "Can I interest you in a trim?" );

    } else if( topic == "TALK_RANCH_BARBER_JOB" ) {
        return _( "What?  I'm a barber... I cut hair.  There's demand for cheap cuts and a shave out here." );

    } else if( topic == "TALK_RANCH_BARBER_HIRE" ) {
        return _( "I can't imagine what I'd need your assistance with." );

    } else if( topic == "TALK_RANCH_BARBER_CUT" ) {
        return _( "Stand still while I get my clippers..." );


    } else if( topic == "TALK_SHELTER" ) {
        switch( rng( 1, 2 ) ) {
            case 1:
                return _( "Well, I guess it's just us." );
            case 2:
                return _( "At least we've got shelter." );
        }
    } else if( topic == "TALK_SHELTER_ADVICE" ) {
        return get_hint();
    } else if( topic == "TALK_SHELTER_PLANS" ) {
        switch( rng( 1, 5 ) ) {
            case 1:
                return _( "I don't know, look for supplies and other survivors I guess." );
            case 2:
                return _( "Maybe we should start boarding up this place." );
            case 3:
                return _( "I suppose getting a car up and running should really be useful if we have to disappear quickly from here." );
            case 4:
                return _( "We could look for one of those farms out here. They can provide plenty of food and aren't close to the cities." );
            case 5:
                return _( "We should probably stay away from those cities, even if there's plenty of useful stuff there." );
        }

    } else if( topic == "TALK_SHARE_EQUIPMENT" ) {
        if( p->has_effect( effect_asked_for_item ) ) {
            return _( "You just asked me for stuff; ask later." );
        }
        return _( "Why should I share my equipment with you?" );

    } else if( topic == "TALK_GIVE_EQUIPMENT" ) {
        return _( "Okay, here you go." );

    } else if( topic == "TALK_DENY_EQUIPMENT" ) {
        if( p->op_of_u.anger >= p->hostile_anger_level() - 4 ) {
            return _( "<no>, and if you ask again, <ill_kill_you>!" );
        } else {
            return _( "<no><punc> <fuck_you>!" );
        }

    }
    if( topic == "TALK_TRAIN" ) {
        if( !g->u.backlog.empty() && g->u.backlog.front().id() == activity_id( "ACT_TRAIN" ) ) {
            return _( "Shall we resume?" );
        }
        std::vector<skill_id> trainable = p->skills_offered_to( g->u );
        std::vector<matype_id> styles = p->styles_offered_to( g->u );
        if( trainable.empty() && styles.empty() ) {
            return _( "Sorry, but it doesn't seem I have anything to teach you." );
        } else {
            return _( "Here's what I can teach you..." );
        }

    } else if( topic == "TALK_TRAIN_START" ) {
        return _( "Alright, let's begin." );

    } else if( topic == "TALK_TRAIN_FORCE" ) {
        return _( "Alright, let's begin." );

    } else if( topic == "TALK_SUGGEST_FOLLOW" ) {
        if( p->has_effect( effect_infection ) ) {
            return _( "Not until I get some antibiotics..." );
        }
        if( p->has_effect( effect_asked_to_follow ) ) {
            return _( "You asked me recently; ask again later." );
        }
        return _( "Why should I travel with you?" );

    } else if( topic == "TALK_AGREE_FOLLOW" ) {
        return _( "You got it, I'm with you!" );

    } else if( topic == "TALK_DENY_FOLLOW" ) {
        return _( "Yeah... I don't think so." );

    } else if( topic == "TALK_LEADER" ) {
        return _( "What is it?" );

    } else if( topic == "TALK_LEAVE" ) {
        return _( "You're really leaving?" );

    } else if( topic == "TALK_PLAYER_LEADS" ) {
        return _( "Alright.  You can lead now." );

    } else if( topic == "TALK_LEADER_STAYS" ) {
        return _( "No.  I'm the leader here." );

    } else if( topic == "TALK_HOW_MUCH_FURTHER" ) {
        // TODO: this ignores the z-component
        const tripoint player_pos = p->global_omt_location();
        int dist = rl_dist( player_pos, p->goal );
        std::ostringstream response;
        dist *= 100;
        if( dist >= 1300 ) {
            int miles = dist / 25; // *100, e.g. quarter mile is "25"
            miles -= miles % 25; // Round to nearest quarter-mile
            int fullmiles = ( miles - miles % 100 ) / 100; // Left of the decimal point
            if( fullmiles <= 0 ) {
                fullmiles = 0;
            }
            response << string_format( _( "%d.%d miles." ), fullmiles, miles );
        } else {
            response << string_format( ngettext( "%d foot.", "%d feet.", dist ), dist );
        }
        return response.str();

    } else if( topic == "TALK_FRIEND" ) {
        return _( "What is it?" );

    } else if( topic == "TALK_FRIEND_GUARD" ) {
        return _( "I'm on watch." );

    } else if( topic == "TALK_CAMP_OVERSEER" ) {
        return _( "Hey Boss..." );

    } else if( topic == "TALK_DENY_GUARD" ) {
        return _( "Not a bloody chance, I'm going to get left behind!" );

    } else if( topic == "TALK_DENY_TRAIN" ) {
        return the_topic.reason;

    } else if( topic == "TALK_DENY_PERSONAL" ) {
        return _( "I'd prefer to keep that to myself." );

    } else if( topic == "TALK_FRIEND_UNCOMFORTABLE" ) {
        return _( "I really don't feel comfortable doing so..." );

    } else if( topic == "TALK_COMBAT_COMMANDS" ) {
        std::stringstream status;
        // Prepending * makes this an action, not a phrase
        switch( p->rules.engagement ) {
            case ENGAGE_NONE:
                status << _( "*is not engaging enemies." );
                break;
            case ENGAGE_CLOSE:
                status << _( "*is engaging nearby enemies." );
                break;
            case ENGAGE_WEAK:
                status << _( "*is engaging weak enemies." );
                break;
            case ENGAGE_HIT:
                status << _( "*is engaging enemies you attack." );
                break;
            case ENGAGE_NO_MOVE:
                status << _( "*is engaging enemies close enough to attack without moving." );
                break;
            case ENGAGE_ALL:
                status << _( "*is engaging all enemies." );
                break;
        }
        std::string npcstr = p->male ? pgettext( "npc", "He" ) : pgettext( "npc", "She" );
        if( p->rules.use_guns ) {
            if( p->rules.use_silent ) {
                status << string_format( _( " %s will use silenced firearms." ), npcstr.c_str() );
            } else {
                status << string_format( _( " %s will use firearms." ), npcstr.c_str() );
            }
        } else {
            status << string_format( _( " %s will not use firearms." ), npcstr.c_str() );
        }
        if( p->rules.use_grenades ) {
            status << string_format( _( " %s will use grenades." ), npcstr.c_str() );
        } else {
            status << string_format( _( " %s will not use grenades." ), npcstr.c_str() );
        }

        return status.str();

    } else if( topic == "TALK_COMBAT_ENGAGEMENT" ) {
        return _( "What should I do?" );

    } else if( topic == "TALK_AIM_RULES" ) {
        return _( "How should I aim?" );

    } else if( topic == "TALK_STRANGER_NEUTRAL" ) {
        return _( "Hello there." );

    } else if( topic == "TALK_STRANGER_WARY" ) {
        return _( "Okay, no sudden movements..." );

    } else if( topic == "TALK_STRANGER_SCARED" ) {
        return _( "Keep your distance!" );

    } else if( topic == "TALK_STRANGER_FRIENDLY" ) {
        return _( "Hey there, <name_g>." );

    } else if( topic == "TALK_STRANGER_AGGRESSIVE" ) {
        if( !g->u.unarmed_attack() ) {
            return "<drop_it>";
        } else {
            return _( "This is my territory, <name_b>." );
        }

    } else if( topic == "TALK_MUG" ) {
        if( !g->u.unarmed_attack() ) {
            return "<drop_it>";
        } else {
            return "<hands_up>";
        }

    } else if( topic == "TALK_DESCRIBE_MISSION" ) {
        switch( p->mission ) {
            case NPC_MISSION_SHELTER:
                return _( "I'm holing up here for safety." );
            case NPC_MISSION_SHOPKEEP:
                return _( "I run the shop here." );
            case NPC_MISSION_BASE:
                return _( "I'm guarding this location." );
            case NPC_MISSION_GUARD:
                return _( "I'm guarding this location." );
            case NPC_MISSION_NULL:
                return p->myclass.obj().get_job_description();
            default:
                return "ERROR: Someone forgot to code an npc_mission text.";
        } // switch (p->mission)

    } else if( topic == "TALK_WEAPON_DROPPED" ) {
        std::string npcstr = p->male ? pgettext( "npc", "his" ) : pgettext( "npc", "her" );
        return string_format( _( "*drops %s weapon." ), npcstr.c_str() );

    } else if( topic == "TALK_DEMAND_LEAVE" ) {
        return _( "Now get out of here, before I kill you." );

    } else if( topic == "TALK_SHOUT" ) {
        alpha->shout();
        if( alpha->is_deaf() ) {
            return _( "&You yell, but can't hear yourself." );
        } else {
            return _( "&You yell." );
        }

    } else if( topic == "TALK_SIZE_UP" ) {
        ///\EFFECT_PER affects whether player can size up NPCs

        ///\EFFECT_INT slightly affects whether player can size up NPCs
        int ability = g->u.per_cur * 3 + g->u.int_cur;
        if( ability <= 10 ) {
            return "&You can't make anything out.";
        }

        if( p->is_friend() || ability > 100 ) {
            ability = 100;
        }

        std::stringstream info;
        info << "&";
        int str_range = int( 100 / ability );
        int str_min = int( p->str_max / str_range ) * str_range;
        info << string_format( _( "Str %d - %d" ), str_min, str_min + str_range );

        if( ability >= 40 ) {
            int dex_range = int( 160 / ability );
            int dex_min = int( p->dex_max / dex_range ) * dex_range;
            info << "  " << string_format( _( "Dex %d - %d" ), dex_min, dex_min + dex_range );
        }

        if( ability >= 50 ) {
            int int_range = int( 200 / ability );
            int int_min = int( p->int_max / int_range ) * int_range;
            info << "  " << string_format( _( "Int %d - %d" ), int_min, int_min + int_range );
        }

        if( ability >= 60 ) {
            int per_range = int( 240 / ability );
            int per_min = int( p->per_max / per_range ) * per_range;
            info << "  " << string_format( _( "Per %d - %d" ), per_min, per_min + per_range );
        }

        if( ability >= 100 - ( p->get_fatigue() / 10 ) ) {
            std::string how_tired;
            if( p->get_fatigue() > EXHAUSTED ) {
                how_tired = _( "Exhausted" );
            } else if( p->get_fatigue() > DEAD_TIRED ) {
                how_tired = _( "Dead tired" );
            } else if( p->get_fatigue() > TIRED ) {
                how_tired = _( "Tired" );
            } else {
                how_tired = _( "Not tired" );
            }

            info << std::endl << how_tired;
        }

        return info.str();

    } else if( topic == "TALK_LOOK_AT" ) {
        std::stringstream look;
        look << "&" << p->short_description();
        return look.str();

    } else if( topic == "TALK_OPINION" ) {
        std::stringstream opinion;
        opinion << "&" << p->opinion_text();
        return opinion.str();

    } else if( topic == "TALK_WAKE_UP" ) {
        if( p->has_effect( effect_sleep ) ) {
            if( p->get_fatigue() > EXHAUSTED ) {
                return _( "No, just <swear> no..." );
            } else if( p->get_fatigue() > DEAD_TIRED ) {
                return _( "Just let me sleep, <name_b>!" );
            } else if( p->get_fatigue() > TIRED ) {
                return _( "Make it quick, I want to go back to sleep." );
            } else {
                return _( "Just few minutes more..." );
            }
        } else {
            return _( "Anything to do before I go to sleep?" );
        }

    } else if( topic == "TALK_MISC_RULES" ) {
        std::stringstream status;
        std::string npcstr = p->male ? pgettext( "npc", "He" ) : pgettext( "npc", "She" );

        if( p->rules.allow_pick_up && p->rules.pickup_whitelist->empty() ) {
            status << string_format( _( " %s will pick up all items." ), npcstr.c_str() );
        } else if( p->rules.allow_pick_up ) {
            status << string_format( _( " %s will pick up items from the whitelist." ), npcstr.c_str() );
        } else {
            status << string_format( _( " %s will not pick up items." ), npcstr.c_str() );
        }

        if( p->rules.allow_bash ) {
            status << string_format( _( " %s will bash down obstacles." ), npcstr.c_str() );
        } else {
            status << string_format( _( " %s will not bash down obstacles." ), npcstr.c_str() );
        }

        if( p->rules.allow_sleep ) {
            status << string_format( _( " %s will sleep when tired." ), npcstr.c_str() );
        } else {
            status << string_format( _( " %s will sleep only when exhausted." ), npcstr.c_str() );
        }

        if( p->rules.allow_complain ) {
            status << string_format( _( " %s will complain about wounds and needs." ), npcstr.c_str() );
        } else {
            status << string_format( _( " %s will only complain in an emergency." ), npcstr.c_str() );
        }

        if( p->rules.allow_pulp ) {
            status << string_format( _( " %s will smash nearby zombie corpses." ), npcstr.c_str() );
        } else {
            status << string_format( _( " %s will leave zombie corpses intact." ), npcstr.c_str() );
        }

        if( p->rules.close_doors ) {
            status << string_format( _( " %s will close doors behind themselves." ), npcstr.c_str() );
        } else {
            status << string_format( _( " %s will leave doors open." ), npcstr.c_str() );
        }

        return status.str();

    } else if( topic == "TALK_USE_ITEM" ) {
        return give_item_to( *p, true, false );
    } else if( topic == "TALK_GIVE_ITEM" ) {
        return give_item_to( *p, false, true );
        // Maybe TODO: Allow an option to "just take it, use it if you want"
    } else if( topic == "TALK_MIND_CONTROL" ) {
        p->set_attitude( NPCATT_FOLLOW );
        return _( "YES, MASTER!" );
    }

    return string_format( "I don't know what to say for %s. (BUG (npctalk.cpp:dynamic_line))",
                          topic.c_str() );
}

talk_response &dialogue::add_response( const std::string &text, const std::string &r )
{
    responses.push_back( talk_response() );
    talk_response &result = responses.back();
    result.text = text;
    result.success.next_topic = talk_topic( r );
    return result;
}

talk_response &dialogue::add_response_done( const std::string &text )
{
    return add_response( text, "TALK_DONE" );
}

talk_response &dialogue::add_response_none( const std::string &text )
{
    return add_response( text, "TALK_NONE" );
}

talk_response &dialogue::add_response( const std::string &text, const std::string &r,
                                       dialogue_fun_ptr effect_success )
{
    talk_response &result = add_response( text, r );
    result.success.set_effect( effect_success );
    return result;
}

talk_response &dialogue::add_response( const std::string &text, const std::string &r,
                                       std::function<void( npc & )> effect_success,
                                       dialogue_consequence consequence )
{
    talk_response &result = add_response( text, r );
    result.success.set_effect_consequence( effect_success, consequence );
    return result;
}

talk_response &dialogue::add_response( const std::string &text, const std::string &r,
                                       mission *miss )
{
    if( miss == nullptr ) {
        debugmsg( "tried to select null mission" );
    }
    talk_response &result = add_response( text, r );
    result.mission_selected = miss;
    return result;
}

talk_response &dialogue::add_response( const std::string &text, const std::string &r,
                                       const skill_id &skill )
{
    talk_response &result = add_response( text, r );
    result.skill = skill;
    return result;
}

talk_response &dialogue::add_response( const std::string &text, const std::string &r,
                                       const martialart &style )
{
    talk_response &result = add_response( text, r );
    result.style = style.id;
    return result;
}

talk_response &dialogue::add_response( const std::string &text, const std::string &r,
                                       const itype_id &item_type )
{
    if( item_type == "null" ) {
        debugmsg( "explicitly specified null item" );
    }

    talk_response &result = add_response( text, r );
    result.success.next_topic.item_type = item_type;
    return result;
}

void dialogue::gen_responses( const talk_topic &the_topic )
{
    const auto &topic = the_topic.id; // for compatibility, later replace it in the code below
    const auto p = beta; // for compatibility, later replace it in the code below
    auto &ret = responses; // for compatibility, later replace it in the code below
    ret.clear();
    auto const iter = json_talk_topics.find( topic );
    if( iter != json_talk_topics.end() ) {
        json_talk_topic &jtt = iter->second;
        if( jtt.gen_responses( *this ) ) {
            return;
        }
    }
    // Can be nullptr! Check before dereferencing
    mission *miss = p->chatbin.mission_selected;

    if( topic == "TALK_GUARD" ) {
        add_response_done( _( "Don't mind me..." ) );

    } else if( topic == "TALK_MISSION_LIST" ) {
        if( p->chatbin.missions.empty() ) {
            add_response_none( _( "Oh, okay." ) );
        } else if( p->chatbin.missions.size() == 1 ) {
            add_response( _( "Tell me about it." ), "TALK_MISSION_OFFER",  p->chatbin.missions.front() );
            add_response_none( _( "Never mind, I'm not interested." ) );
        } else {
            for( auto &mission : p->chatbin.missions ) {
                add_response( mission->get_type().name, "TALK_MISSION_OFFER", mission );
            }
            add_response_none( _( "Never mind, I'm not interested." ) );
        }

    } else if( topic == "TALK_MISSION_LIST_ASSIGNED" ) {
        if( missions_assigned.empty() ) {
            add_response_none( _( "Never mind then." ) );
        } else if( missions_assigned.size() == 1 ) {
            add_response( _( "I have news." ), "TALK_MISSION_INQUIRE", missions_assigned.front() );
            add_response_none( _( "Never mind." ) );
        } else {
            for( auto &miss_it : missions_assigned ) {
                add_response( miss_it->get_type().name, "TALK_MISSION_INQUIRE", miss_it );
            }
            add_response_none( _( "Never mind." ) );
        }

    } else if( topic == "TALK_MISSION_DESCRIBE" ) {
        add_response( _( "What's the matter?" ), "TALK_MISSION_OFFER" );
        add_response( _( "I don't care." ), "TALK_MISSION_REJECTED" );

    } else if( topic == "TALK_MISSION_OFFER" ) {
        add_response( _( "I'll do it!" ), "TALK_MISSION_ACCEPTED", &talk_function::assign_mission );
        add_response( _( "Not interested." ), "TALK_MISSION_REJECTED" );

    } else if( topic == "TALK_MISSION_ACCEPTED" ) {
        add_response_none( _( "Not a problem." ) );
        add_response( _( "Got any advice?" ), "TALK_MISSION_ADVICE" );
        add_response( _( "Can you share some equipment?" ), "TALK_SHARE_EQUIPMENT" );
        add_response_done( _( "I'll be back soon!" ) );

    } else if( topic == "TALK_MISSION_ADVICE" ) {
        add_response_none( _( "Sounds good, thanks." ) );
        add_response_done( _( "Sounds good.  Bye!" ) );

    } else if( topic == "TALK_MISSION_REJECTED" ) {
        add_response_none( _( "I'm sorry." ) );
        add_response_done( _( "Whatever.  Bye." ) );

    } else if( topic == "TALK_MISSION_INQUIRE" ) {
        const auto mission = p->chatbin.mission_selected;
        if( mission == nullptr ) {
            debugmsg( "dialogue::gen_responses(\"TALK_MISSION_INQUIRE\") called for null mission" );
        } else if( mission->has_failed() ) {
            RESPONSE( _( "I'm sorry... I failed." ) );
            SUCCESS( "TALK_MISSION_FAILURE" );
            SUCCESS_OPINION( -1, 0, -1, 1, 0 );
            RESPONSE( _( "Not yet." ) );
            TRIAL( TALK_TRIAL_LIE, 10 + p->op_of_u.trust * 3 );
            SUCCESS( "TALK_NONE" );
            FAILURE( "TALK_MISSION_FAILURE" );
            FAILURE_OPINION( -3, 0, -1, 2, 0 );
        } else if( !mission->is_complete( p->getID() ) ) {
            add_response_none( _( "Not yet." ) );
            if( mission->get_type().goal == MGOAL_KILL_MONSTER ) {
                RESPONSE( _( "Yup, I killed it." ) );
                TRIAL( TALK_TRIAL_LIE, 10 + p->op_of_u.trust * 5 );
                SUCCESS( "TALK_MISSION_SUCCESS" );
                SUCCESS_ACTION( &talk_function::mission_success );
                FAILURE( "TALK_MISSION_SUCCESS_LIE" );
                FAILURE_OPINION( -5, 0, -1, 5, 0 );
                FAILURE_ACTION( &talk_function::mission_failure );
            }
            add_response_done( _( "No.  I'll get back to it, bye!" ) );
        } else {
            // TODO: Lie about mission
            switch( mission->get_type().goal ) {
                case MGOAL_FIND_ITEM:
                case MGOAL_FIND_ANY_ITEM:
                    add_response( _( "Yup!  Here it is!" ), "TALK_MISSION_SUCCESS",
                                  &talk_function::mission_success );
                    break;
                case MGOAL_GO_TO_TYPE:
                    add_response( _( "We're here!" ), "TALK_MISSION_SUCCESS", &talk_function::mission_success );
                    break;
                case MGOAL_GO_TO:
                case MGOAL_FIND_NPC:
                    add_response( _( "Here I am." ), "TALK_MISSION_SUCCESS", &talk_function::mission_success );
                    break;
                case MGOAL_FIND_MONSTER:
                    add_response( _( "Here it is!" ), "TALK_MISSION_SUCCESS", &talk_function::mission_success );
                    break;
                case MGOAL_ASSASSINATE:
                    add_response( _( "Justice has been served." ), "TALK_MISSION_SUCCESS",
                                  &talk_function::mission_success );
                    break;
                case MGOAL_KILL_MONSTER:
                    add_response( _( "I killed it." ), "TALK_MISSION_SUCCESS", &talk_function::mission_success );
                    break;
                case MGOAL_RECRUIT_NPC:
                case MGOAL_RECRUIT_NPC_CLASS:
                    add_response( _( "I brought'em." ), "TALK_MISSION_SUCCESS", &talk_function::mission_success );
                    break;
                case MGOAL_COMPUTER_TOGGLE:
                    add_response( _( "I've taken care of it..." ), "TALK_MISSION_SUCCESS",
                                  &talk_function::mission_success );
                    break;
                default:
                    add_response( _( "Mission success!  I don't know what else to say." ),
                                  "TALK_MISSION_SUCCESS", &talk_function::mission_success );
                    break;
            }
        }

    } else if( topic == "TALK_MISSION_SUCCESS" ) {
        int mission_value = 0;
        if( miss == nullptr ) {
            debugmsg( "dialogue::gen_responses(\"TALK_MISSION_SUCCESS\") called for null mission" );
        } else {
            mission_value = cash_to_favor( *p, miss->get_value() );
        }
        RESPONSE( _( "Glad to help.  I need no payment." ) );
        SUCCESS( "TALK_NONE" );
        SUCCESS_OPINION( mission_value / 4, -1,
                         mission_value / 3, -1, 0 );
        SUCCESS_ACTION( &talk_function::clear_mission );
        if( !p->is_friend() ) {
            add_response( _( "How about some items as payment?" ), "TALK_MISSION_REWARD",
                          &talk_function::mission_reward );
        }
        if( !p->skills_offered_to( g->u ).empty() || !p->styles_offered_to( g->u ).empty() ) {
            RESPONSE( _( "Maybe you can teach me something as payment." ) );
            SUCCESS( "TALK_TRAIN" );
        }
        RESPONSE( _( "Glad to help.  I need no payment.  Bye!" ) );
        SUCCESS( "TALK_DONE" );
        SUCCESS_ACTION( &talk_function::clear_mission );
        SUCCESS_OPINION( mission_value / 4, -1,
                         mission_value / 3, -1, 0 );

    } else if( topic == "TALK_MISSION_SUCCESS_LIE" ) {
        add_response( _( "Well, um, sorry." ), "TALK_NONE", &talk_function::clear_mission );

    } else if( topic == "TALK_MISSION_FAILURE" ) {
        add_response_none( _( "I'm sorry.  I did what I could." ) );

    } else if( topic == "TALK_MISSION_REWARD" ) {
        add_response( _( "Thank you." ), "TALK_NONE", &talk_function::clear_mission );
        add_response( _( "Thanks, bye." ), "TALK_DONE", &talk_function::clear_mission );

    } else if( topic == "TALK_EVAC_MERCHANT" ) {
        if( p->has_trait( trait_id( "NPC_MISSION_LEV_1" ) ) ) {
            add_response( _( "I figured you might be looking for some help..." ), "TALK_EVAC_MERCHANT" );
            p->companion_mission_role_id = "REFUGEE MERCHANT";
            SUCCESS_ACTION( &talk_function::companion_mission );
        }

    } else if( topic == "TALK_EVAC_MERCHANT_PLANS2" ) {
        ///\EFFECT_INT >11 adds useful dialog option in TALK_EVAC_MERCHANT
        if( g->u.int_cur >= 12 ) {
            add_response(
                _( "[INT 12] Wait, six buses and refugees... how many people do you still have crammed in here?" ),
                "TALK_EVAC_MERCHANT_PLANS3" );
        }

    } else if( topic == "TALK_EVAC_MERCHANT_ASK_JOIN" ) {
        ///\EFFECT_INT >10 adds bad dialog option in TALK_EVAC_MERCHANT (NEGATIVE)
        if( g->u.int_cur > 10 ) {
            add_response(
                _( "[INT 11] I'm sure I can organize salvage operations to increase the bounty scavengers bring in!" ),
                "TALK_EVAC_MERCHANT_NO" );
        }
        ///\EFFECT_INT <7 allows bad dialog option in TALK_EVAC_MERCHANT

        ///\EFFECT_STR >10 allows bad dialog option in TALK_EVAC_MERCHANT
        if( g->u.int_cur <= 6 && g->u.str_cur > 10 ) {
            add_response( _( "[STR 11] I punch things in face real good!" ), "TALK_EVAC_MERCHANT_NO" );
        }

    } else if( topic == "TALK_EVAC_GUARD3_HIDE2" ) {
        RESPONSE( _( "Get bent, traitor!" ) );
        TRIAL( TALK_TRIAL_INTIMIDATE, 20 + p->op_of_u.fear * 3 );
        SUCCESS( "TALK_EVAC_GUARD3_HOSTILE" );
        FAILURE( "TALK_EVAC_GUARD3_INSULT" );
        RESPONSE( _( "Got something to hide?" ) );
        TRIAL( TALK_TRIAL_PERSUADE, 10 + p->op_of_u.trust * 3 );
        SUCCESS( "TALK_EVAC_GUARD3_DEAD" );
        FAILURE( "TALK_EVAC_GUARD3_INSULT" );

    } else if( topic == "TALK_EVAC_GUARD3_HOSTILE" ) {
        p->my_fac->likes_u -= 15;//The Free Merchants are insulted by your actions!
        p->my_fac->respects_u -= 15;
        p->my_fac = g->faction_manager_ptr->get( faction_id( "hells_raiders" ) );

    } else if( topic == "TALK_EVAC_GUARD3_INSULT" ) {
        p->my_fac->likes_u -= 5;//The Free Merchants are insulted by your actions!
        p->my_fac->respects_u -= 5;

    } else if( topic == "TALK_EVAC_GUARD3_DEAD" ) {
        p->my_fac = g->faction_manager_ptr->get( faction_id( "hells_raiders" ) );

    } else if( topic == "TALK_OLD_GUARD_SOLDIER" ) {
        add_response_done( _( "Don't mind me..." ) );

    } else if( topic == "TALK_OLD_GUARD_NEC_CPT" ) {
        if( g->u.has_trait( trait_PROF_FED ) ) {
            add_response( _( "What are you doing down here?" ), "TALK_OLD_GUARD_NEC_CPT_GOAL" );
            add_response( _( "Can you tell me about this facility?" ), "TALK_OLD_GUARD_NEC_CPT_VAULT" );
            add_response( _( "What do you need done?" ), "TALK_MISSION_LIST" );
            if( p->chatbin.missions_assigned.size() == 1 ) {
                add_response( _( "About the mission..." ), "TALK_MISSION_INQUIRE" );
            } else if( p->chatbin.missions_assigned.size() >= 2 ) {
                add_response( _( "About one of those missions..." ), "TALK_MISSION_LIST_ASSIGNED" );
            }
        }
        add_response_done( _( "I've got to go..." ) );

    } else if( topic == "TALK_OLD_GUARD_NEC_CPT_GOAL" ) {
        add_response( _( "Seems like a decent plan..." ), "TALK_OLD_GUARD_NEC_CPT" );

    } else if( topic == "TALK_OLD_GUARD_NEC_CPT_VAULT" ) {
        add_response( _( "Whatever they did it must have worked since we are still alive..." ),
                      "TALK_OLD_GUARD_NEC_CPT" );

    } else if( topic == "TALK_OLD_GUARD_NEC_COMMO" ) {
        if( g->u.has_trait( trait_PROF_FED ) ) {
            for( auto miss_it : g->u.get_active_missions() ) {
                if( miss_it->mission_id() == mission_type_id( "MISSION_OLD_GUARD_NEC_1" ) &&
                    !p->has_effect( effect_gave_quest_item ) ) {
                    add_response( _( "[MISSION] The captain sent me to get a frequency list from you." ),
                                  "TALK_OLD_GUARD_NEC_COMMO_FREQ" );
                }
            }
            add_response( _( "What are you doing here?" ), "TALK_OLD_GUARD_NEC_COMMO_GOAL" );
            add_response( _( "Do you need any help?" ), "TALK_MISSION_LIST" );
            if( p->chatbin.missions_assigned.size() == 1 ) {
                add_response( _( "About the mission..." ), "TALK_MISSION_INQUIRE" );
            } else if( p->chatbin.missions_assigned.size() >= 2 ) {
                add_response( _( "About one of those missions..." ), "TALK_MISSION_LIST_ASSIGNED" );
            }
        }
        add_response_done( _( "I should be going..." ) );

    } else if( topic == "TALK_OLD_GUARD_NEC_COMMO_GOAL" ) {
        add_response( _( "I'll try and find your commander then..." ), "TALK_OLD_GUARD_NEC_COMMO" );

    } else if( topic == "TALK_OLD_GUARD_NEC_COMMO_FREQ" ) {
        popup( _( "%1$s gives you a %2$s" ), p->name.c_str(), item( "necropolis_freq",
                0 ).tname().c_str() );
        g->u.i_add( item( "necropolis_freq", 0 ) );
        p->add_effect( effect_gave_quest_item, 9999_turns ); //@todo choose sane duration
        add_response( _( "Thanks." ), "TALK_OLD_GUARD_NEC_COMMO" );

    } else if( topic == "TALK_SCAVENGER_MERC_HIRE" ) {
        if( g->u.cash >= 800000 ) {
            add_response( _( "[$8000] You have a deal." ), "TALK_SCAVENGER_MERC_HIRE_SUCCESS" );
        }

    } else if( topic == "TALK_SCAVENGER_MERC_HIRE_SUCCESS" ) {
        if( g->u.cash < 800000 ) {
            debugmsg( "Money appeared out of thin air! (or someone accidentally linked to this talk_topic)" );
        } else {
            g->u.cash -= 800000;
        }

    } else if( topic == "TALK_FREE_MERCHANT_STOCKS" ) {
        add_response( _( "Who are you?" ), "TALK_FREE_MERCHANT_STOCKS_NEW" );
        static const std::vector<itype_id> wanted = {{
                "jerky", "meat_smoked", "fish_smoked",
                "cooking_oil", "cornmeal", "flour",
                "fruit_wine", "beer", "sugar",
            }
        };

        for( const auto &id : wanted ) {
            if( g->u.charges_of( id ) > 0 ) {
                const std::string msg = string_format( _( "Delivering %s." ), item::nname( id ).c_str() );
                add_response( msg, "TALK_DELIVER_ASK", id );
            }
        }
        add_response_done( _( "Well, bye." ) );

    } else if( topic == "TALK_DELIVER_ASK" ) {
        if( the_topic.item_type == "null" ) {
            debugmsg( "delivering nulls" );
        }
        add_response( _( "Works for me." ), "TALK_DELIVER_CONFIRM", the_topic.item_type );
        add_response( _( "Maybe later." ), "TALK_DONE" );

    } else if( topic == "TALK_DELIVER_CONFIRM" ) {
        bulk_trade_accept( *p, the_topic.item_type );
        add_response_done( _( "You might be seeing more of me..." ) );

    } else if( topic == "TALK_FREE_MERCHANT_STOCKS_NEW" ) {
        add_response( _( "Why cornmeal, jerky, and fruit wine?" ), "TALK_FREE_MERCHANT_STOCKS_WHY" );
    } else if( topic == "TALK_FREE_MERCHANT_STOCKS_WHY" ) {
        add_response( _( "Are you looking to buy anything else?" ), "TALK_FREE_MERCHANT_STOCKS_ALL" );
        add_response( _( "Very well..." ), "TALK_FREE_MERCHANT_STOCKS" );
    } else if( topic == "TALK_FREE_MERCHANT_STOCKS_ALL" ) {
        add_response( _( "Interesting..." ), "TALK_FREE_MERCHANT_STOCKS" );
    } else if( topic == "TALK_RANCH_FOREMAN" ) {
        for( auto miss_it : g->u.get_active_missions() ) {
            if( miss_it->mission_id() == mission_type_id( "MISSION_FREE_MERCHANTS_EVAC_3" ) &&
                !p->has_effect( effect_gave_quest_item ) ) {
                add_response(
                    _( "[MISSION] The merchant at the Refugee Center sent me to get a prospectus from you." ),
                    "TALK_RANCH_FOREMAN_PROSPECTUS" );
            }
        }
        add_response( _( "I heard you were setting up an outpost out here..." ),
                      "TALK_RANCH_FOREMAN_OUTPOST" );
        add_response( _( "What's your job here?" ), "TALK_RANCH_FOREMAN_JOB" );
        add_response( _( "What do you need done?" ), "TALK_MISSION_LIST" );
        if( p->chatbin.missions_assigned.size() == 1 ) {
            add_response( _( "About that job..." ), "TALK_MISSION_INQUIRE" );
        } else if( p->chatbin.missions_assigned.size() >= 2 ) {
            add_response( _( "About one of those jobs..." ), "TALK_MISSION_LIST_ASSIGNED" );
        }
        add_response( _( "I figured you might be looking for some help..." ), "TALK_RANCH_FOREMAN" );
        p->companion_mission_role_id = "FOREMAN";
        SUCCESS_ACTION( &talk_function::companion_mission );
        add_response( _( "I've got to go..." ), "TALK_DONE" );
    } else if( topic == "TALK_RANCH_FOREMAN_PROSPECTUS" ) {
        popup( _( "%1$s gives you a %2$s" ), p->name.c_str(), item( "commune_prospectus",
                0 ).tname().c_str() );
        g->u.i_add( item( "commune_prospectus", 0 ) );
        p->add_effect( effect_gave_quest_item, 9999_turns ); //@todo choose a sane duration
        add_response( _( "Thanks." ), "TALK_RANCH_FOREMAN" );
    } else if( topic == "TALK_RANCH_FOREMAN_OUTPOST" ) {
        add_response( _( "How many refugees are you expecting?" ), "TALK_RANCH_FOREMAN_REFUGEES" );
    } else if( topic == "TALK_RANCH_FOREMAN_REFUGEES" ) {
        add_response( _( "Hopefully moving out here was worth it..." ), "TALK_RANCH_FOREMAN" );
    } else if( topic == "TALK_RANCH_FOREMAN_JOB" ) {
        add_response( _( "I'll keep that in mind." ), "TALK_RANCH_FOREMAN" );
    } else if( topic == "TALK_RANCH_CONSTRUCTION_1" ) {
        add_response( _( "I'll talk to him then..." ), "TALK_DONE" );
    } else if( topic == "TALK_RANCH_CONSTRUCTION_2" ) {
        add_response( _( "What are you doing here?" ), "TALK_RANCH_CONSTRUCTION_2_JOB" );
        add_response( _( "I've got to go..." ), "TALK_DONE" );
    } else if( topic == "TALK_RANCH_CONSTRUCTION_2_JOB" ) {
        add_response( _( "..." ), "TALK_RANCH_CONSTRUCTION_2" );
    } else if( topic == "TALK_RANCH_WOODCUTTER" ) {
        add_response( _( "What are you doing here?" ), "TALK_RANCH_WOODCUTTER_JOB" );
        add_response( _( "I'd like to hire your services." ), "TALK_RANCH_WOODCUTTER_HIRE" );
        add_response( _( "I've got to go..." ), "TALK_DONE" );
    } else if( topic == "TALK_RANCH_WOODCUTTER_JOB" ) {
        add_response( _( "..." ), "TALK_RANCH_WOODCUTTER" );
    } else if( topic == "TALK_RANCH_WOODCUTTER_HIRE" ) {
        if( !p->has_effect( effect_currently_busy ) ) {
            if( g->u.cash >= 200000 ) {
                add_response( _( "[$2000,1d] 10 logs" ), "TALK_DONE" );
                SUCCESS_ACTION( &talk_function::buy_10_logs );
            }
            if( g->u.cash >= 1200000 ) {
                add_response( _( "[$12000,7d] 100 logs" ), "TALK_DONE" );
                SUCCESS_ACTION( &talk_function::buy_100_logs );
            }
            add_response( _( "Maybe later..." ), "TALK_RANCH_WOODCUTTER" );
        } else {
            add_response( _( "I'll be back later..." ), "TALK_RANCH_WOODCUTTER" );
        }
    } else if( topic == "TALK_RANCH_WOODCUTTER_2" ) {
        add_response( _( "What is your job here?" ), "TALK_RANCH_WOODCUTTER_2_JOB" );
        add_response( _( "Do you need any help?" ), "TALK_RANCH_WOODCUTTER_2_HIRE" );
        add_response( _( "I've got to go..." ), "TALK_DONE" );
    } else if( topic == "TALK_RANCH_WOODCUTTER_2_JOB" ) {
        add_response( _( "..." ), "TALK_RANCH_WOODCUTTER_2" );
    } else if( topic == "TALK_RANCH_WOODCUTTER_2_HIRE" ) {
        add_response( _( "..." ), "TALK_RANCH_WOODCUTTER_2" );
    } else if( topic == "TALK_RANCH_FARMER_1" ) {
        add_response( _( "What are you doing here?" ), "TALK_RANCH_FARMER_1_JOB" );
        add_response( _( "I'd like to hire your services." ), "TALK_RANCH_FARMER_1_HIRE" );
        add_response( _( "I've got to go..." ), "TALK_DONE" );
    } else if( topic == "TALK_RANCH_FARMER_1_JOB" ) {
        add_response( _( "..." ), "TALK_RANCH_FARMER_1" );
    } else if( topic == "TALK_RANCH_FARMER_1_HIRE" ) {
        add_response( _( "..." ), "TALK_RANCH_FARMER_1" );
    } else if( topic == "TALK_RANCH_FARMER_2" ) {
        add_response( _( "What are you doing here?" ), "TALK_RANCH_FARMER_2_JOB" );
        add_response( _( "I'd like to hire your services." ), "TALK_RANCH_FARMER_2_HIRE" );
        add_response( _( "I've got to go..." ), "TALK_DONE" );
    } else if( topic == "TALK_RANCH_FARMER_2_JOB" ) {
        add_response( _( "It could be worse..." ), "TALK_RANCH_FARMER_2" );
    } else if( topic == "TALK_RANCH_FARMER_2_HIRE" ) {
        add_response( _( "I'll talk with them then..." ), "TALK_RANCH_FARMER_2" );
    } else if( topic == "TALK_RANCH_CROP_OVERSEER" ) {
        add_response( _( "What are you doing here?" ), "TALK_RANCH_CROP_OVERSEER_JOB" );
        add_response( _( "I'm interested in investing in agriculture..." ), "TALK_RANCH_CROP_OVERSEER" );
        p->companion_mission_role_id = "COMMUNE CROPS";
        SUCCESS_ACTION( &talk_function::companion_mission );
        add_response( _( "Can I help you with anything?" ), "TALK_MISSION_LIST" );
        if( p->chatbin.missions_assigned.size() == 1 ) {
            add_response( _( "About that job..." ), "TALK_MISSION_INQUIRE" );
        } else if( p->chatbin.missions_assigned.size() >= 2 ) {
            add_response( _( "About one of those jobs..." ), "TALK_MISSION_LIST_ASSIGNED" );
        }
        add_response( _( "I've got to go..." ), "TALK_DONE" );
    } else if( topic == "TALK_RANCH_CROP_OVERSEER_JOB" ) {
        add_response( _( "I'll keep that in mind." ), "TALK_RANCH_CROP_OVERSEER" );
    } else if( topic == "TALK_RANCH_ILL_1" ) {
        add_response( _( "What is your job here?" ), "TALK_RANCH_ILL_1_JOB" );
        add_response( _( "Do you need any help?" ), "TALK_RANCH_ILL_1_HIRE" );
        add_response( _( "What's wrong?" ), "TALK_RANCH_ILL_1_SICK" );
        add_response( _( "I've got to go..." ), "TALK_DONE" );
    } else if( topic == "TALK_RANCH_ILL_1_JOB" ) {
        add_response( _( "..." ), "TALK_RANCH_ILL_1" );
    } else if( topic == "TALK_RANCH_ILL_1_HIRE" ) {
        add_response( _( "..." ), "TALK_RANCH_ILL_1" );
    } else if( topic == "TALK_RANCH_ILL_1_SICK" ) {
        add_response( _( "..." ), "TALK_RANCH_ILL_1" );
    } else if( topic == "TALK_RANCH_NURSE" ) {
        add_response( _( "What is your job here?" ), "TALK_RANCH_NURSE_JOB" );
        add_response( _( "Do you need any help?" ), "TALK_RANCH_NURSE_HIRE" );
        add_response( _( "I could use your medical assistance..." ), "TALK_RANCH_NURSE_AID" );
        add_response( _( "I've got to go..." ), "TALK_DONE" );
    } else if( topic == "TALK_RANCH_NURSE_JOB" ) {
        add_response( _( "..." ), "TALK_RANCH_NURSE" );
    } else if( topic == "TALK_RANCH_NURSE_HIRE" ) {
        add_response( _( "What kind of jobs do you have for me?" ), "TALK_MISSION_LIST" );
        if( g->u.charges_of( "bandages" ) > 0 ) {
            add_response( _( "Delivering bandages." ), "TALK_DELIVER_ASK", itype_id( "bandages" ) );
        }
        add_response( _( "..." ), "TALK_RANCH_NURSE" );
    } else if( topic == "TALK_RANCH_NURSE_AID" ) {
        if( g->u.cash >= 20000 && !p->has_effect( effect_currently_busy ) ) {
            add_response( _( "[$200, 30m] I need you to patch me up..." ), "TALK_RANCH_NURSE_AID_DONE" );
            SUCCESS_ACTION( &talk_function::give_aid );
        }
        if( g->u.cash >= 50000 && !p->has_effect( effect_currently_busy ) ) {
            add_response( _( "[$500, 1h] Could you look at me and my companions?" ),
                          "TALK_RANCH_NURSE_AID_DONE" );
            SUCCESS_ACTION( &talk_function::give_all_aid );
        }
        add_response( _( "I should be fine..." ), "TALK_RANCH_NURSE" );
    } else if( topic == "TALK_RANCH_NURSE_AID_DONE" ) {
        add_response( _( "..." ), "TALK_DONE" );
    } else if( topic == "TALK_RANCH_DOCTOR" ) {
        add_response( _( "For the right price could I borrow your services?" ),
                      "TALK_RANCH_DOCTOR_BIONICS" );
        add_response( _( "..." ), "TALK_DONE" );
    } else if( topic == "TALK_RANCH_DOCTOR_BIONICS" ) {
        add_response( _( "I was wondering if you could install a cybernetic implant..." ), "TALK_DONE" );
        SUCCESS_ACTION( &talk_function::bionic_install );
        add_response( _( "I need help removing an implant..." ), "TALK_DONE" );
        SUCCESS_ACTION( &talk_function::bionic_remove );
        add_response( _( "Nevermind." ), "TALK_DONE" );
    } else if( topic == "TALK_RANCH_SCRAPPER" ) {
        add_response( _( "What is your job here?" ), "TALK_RANCH_SCRAPPER_JOB" );
        add_response( _( "Do you need any help?" ), "TALK_RANCH_SCRAPPER_HIRE" );
        add_response( _( "I've got to go..." ), "TALK_DONE" );
    } else if( topic == "TALK_RANCH_SCRAPPER_JOB" ) {
        add_response( _( "..." ), "TALK_RANCH_SCRAPPER" );
    } else if( topic == "TALK_RANCH_SCRAPPER_HIRE" ) {
        add_response( _( "..." ), "TALK_RANCH_SCRAPPER" );
    } else if( topic == "TALK_RANCH_SCAVENGER_1" ) {
        add_response( _( "What is your job here?" ), "TALK_RANCH_SCAVENGER_1_JOB" );
        add_response( _( "Do you need any help?" ), "TALK_RANCH_SCAVENGER_1_HIRE" );
        add_response( _( "Let's see what you've managed to find..." ), "TALK_RANCH_SCAVENGER_1" );
        SUCCESS_ACTION( &talk_function::start_trade );
        add_response( _( "I've got to go..." ), "TALK_DONE" );
    } else if( topic == "TALK_RANCH_SCAVENGER_1_JOB" ) {
        add_response( _( "..." ), "TALK_RANCH_SCAVENGER_1" );
    } else if( topic == "TALK_RANCH_SCAVENGER_1_HIRE" ) {
        add_response( _( "Tell me more about the scavenging runs..." ), "TALK_RANCH_SCAVENGER_1" );
        p->companion_mission_role_id = "SCAVENGER";
        SUCCESS_ACTION( &talk_function::companion_mission );
        add_response( _( "What kind of tasks do you have for me?" ), "TALK_MISSION_LIST" );
        add_response( _( "..." ), "TALK_RANCH_SCAVENGER_1" );
    } else if( topic == "TALK_RANCH_BARKEEP" ) {
        add_response( _( "What is your job here?" ), "TALK_RANCH_BARKEEP_JOB" );
        add_response( _( "I'm looking for information..." ), "TALK_RANCH_BARKEEP_INFORMATION" );
        add_response( _( "Do you need any help?" ), "TALK_MISSION_LIST" );
        add_response( _( "Let me see what you keep behind the counter..." ), "TALK_RANCH_BARKEEP" );
        SUCCESS_ACTION( &talk_function::start_trade );
        add_response( _( "What do you have on tap?" ), "TALK_RANCH_BARKEEP_TAP" );
        add_response( _( "I'll be going..." ), "TALK_DONE" );
    } else if( topic == "TALK_RANCH_BARKEEP_TAP" ) {
        struct buy_alcohol_entry {
            unsigned long cost;
            int count;
            std::string desc;
            itype_id alc;
        };
        const auto buy_alcohol = [p]( const buy_alcohol_entry & entry ) {
            item cont( "bottle_glass" );
            cont.emplace_back( entry.alc, calendar::turn, entry.count );
            g->u.i_add( cont );
            g->u.cash -= entry.cost;
            add_msg( m_good, _( "%s hands you a %s" ), p->disp_name().c_str(),
                     cont.tname().c_str() );
        };
        static const std::vector<buy_alcohol_entry> entries = {{
                { 800, 2, _( "[$10] I'll take a beer" ), "beer" },
                { 1000, 1, _( "[$10] I'll take a shot of brandy" ), "brandy" },
                { 1000, 1, _( "[$10] I'll take a shot of rum" ), "rum" },
                { 1200, 1, _( "[$12] I'll take a shot of whiskey" ), "whiskey" },
            }
        };
        for( const auto &entry : entries ) {
            if( g->u.cash >= entry.cost ) {
                add_response( entry.desc, "TALK_DONE" );
                SUCCESS_ACTION_CONSEQUENCE( std::bind( buy_alcohol, entry ), dialogue_consequence::none );
            }
        }

        add_response( _( "Never mind." ), "TALK_RANCH_BARKEEP" );
    } else if( topic == "TALK_RANCH_BARKEEP_JOB" ) {
        add_response( _( "..." ), "TALK_RANCH_BARKEEP" );
    } else if( topic == "TALK_RANCH_BARKEEP_INFORMATION" ) {
        add_response( _( "..." ), "TALK_RANCH_BARKEEP" );
    } else if( topic == "TALK_RANCH_BARBER" ) {
        add_response( _( "What is your job here?" ), "TALK_RANCH_BARBER_JOB" );
        add_response( _( "Do you need any help?" ), "TALK_RANCH_BARBER_HIRE" );
        if( g->u.cash >= 500 ) {
            add_response( _( "[$5] I'll have a shave" ), "TALK_RANCH_BARBER_CUT" );
            SUCCESS_ACTION( &talk_function::buy_shave );
        }
        if( g->u.cash >= 1000 ) {
            add_response( _( "[$10] I'll get a haircut" ), "TALK_RANCH_BARBER_CUT" );
            SUCCESS_ACTION( &talk_function::buy_haircut );
        }
        add_response( _( "Maybe another time..." ), "TALK_DONE" );
    } else if( topic == "TALK_RANCH_BARBER_JOB" ) {
        add_response( _( "..." ), "TALK_RANCH_BARBER" );
    }
    if( topic == "TALK_RANCH_BARBER_HIRE" ) {
        add_response( _( "..." ), "TALK_RANCH_BARBER" );
    } else if( topic == "TALK_RANCH_BARBER_CUT" ) {
        add_response( _( "Thanks..." ), "TALK_DONE" );
    } else if( topic == "TALK_SHELTER" ) {
        add_response( _( "What should we do now?" ), "TALK_SHELTER_PLANS" );
        add_response( _( "Any tips?" ), "TALK_SHELTER_ADVICE" );
        add_response( _( "Can I do anything for you?" ), "TALK_MISSION_LIST" );
        if( !p->is_following() ) {
            add_response( _( "Want to travel with me?" ), "TALK_SUGGEST_FOLLOW" );
        }
        add_response( _( "Let's trade items." ), "TALK_NONE", &talk_function::start_trade );
        add_response( _( "I can't leave the shelter without equipment..." ), "TALK_SHARE_EQUIPMENT" );
        add_response_done( _( "Well, bye." ) );
    } else if( topic == "TALK_SHELTER_ADVICE" ) {
        add_response_none( _( "Thanks!" ) );
    } else if( topic == "TALK_SHELTER_PLANS" ) {
        // TODO: Add _("follow me")
        add_response_none( _( "Hmm, okay." ) );

    } else if( topic == "TALK_SHARE_EQUIPMENT" ) {
        if( p->has_effect( effect_asked_for_item ) ) {
            add_response_none( _( "Okay, fine." ) );
        } else {
            int score = p->op_of_u.trust + p->op_of_u.value * 3 +
                        p->personality.altruism * 2;
            int missions_value = p->assigned_missions_value();
            if( g->u.has_amount( "mininuke", 1 ) ) {
                RESPONSE( _( "Because I'm holding a thermal detonator!" ) );
                SUCCESS( "TALK_GIVE_EQUIPMENT" );
                SUCCESS_ACTION( &talk_function::give_equipment );
                SUCCESS_OPINION( 0, 0, -1, 0, score * 300 );
                FAILURE( "TALK_DENY_EQUIPMENT" );
                FAILURE_OPINION( 0, 0, -1, 0, 0 );
            }
            RESPONSE( _( "Because I'm your friend!" ) );
            TRIAL( TALK_TRIAL_PERSUADE, 10 + score );
            SUCCESS( "TALK_GIVE_EQUIPMENT" );
            SUCCESS_ACTION( &talk_function::give_equipment );
            SUCCESS_OPINION( 0, 0, -1, 0, score * 300 );
            FAILURE( "TALK_DENY_EQUIPMENT" );
            FAILURE_OPINION( 0, 0, -1, 0, 0 );
            if( missions_value >= 1 ) {
                RESPONSE( _( "Well, I am helping you out..." ) );
                TRIAL( TALK_TRIAL_PERSUADE, 12 + ( .8 * score ) + missions_value / OWED_VAL );
                SUCCESS( "TALK_GIVE_EQUIPMENT" );
                SUCCESS_ACTION( &talk_function::give_equipment );
                FAILURE( "TALK_DENY_EQUIPMENT" );
                FAILURE_OPINION( 0, 0, -1, 0, 0 );
            }
            RESPONSE( _( "I'll give it back!" ) );
            TRIAL( TALK_TRIAL_LIE, score * 1.5 );
            SUCCESS( "TALK_GIVE_EQUIPMENT" );
            SUCCESS_ACTION( &talk_function::give_equipment );
            SUCCESS_OPINION( 0, 0, -1, 0, score * 300 );
            FAILURE( "TALK_DENY_EQUIPMENT" );
            FAILURE_OPINION( 0, -1, -1, 1, 0 );
            RESPONSE( _( "Give it to me, or else!" ) );
            TRIAL( TALK_TRIAL_INTIMIDATE, 40 );
            SUCCESS( "TALK_GIVE_EQUIPMENT" );
            SUCCESS_ACTION( &talk_function::give_equipment );
            SUCCESS_OPINION( -3, 2, -2, 2,
                             ( g->u.intimidation() + p->op_of_u.fear -
                               p->personality.bravery - p->intimidation() ) * 500 );
            FAILURE( "TALK_DENY_EQUIPMENT" );
            FAILURE_OPINION( -3, 1, -3, 5, 0 );
            add_response_none( _( "Eh, never mind." ) );
            add_response_done( _( "Never mind, I'll do without.  Bye." ) );
        }

    } else if( topic == "TALK_GIVE_EQUIPMENT" ) {
        add_response_none( _( "Thank you!" ) );
        add_response( _( "Thanks!  But can I have some more?" ), "TALK_SHARE_EQUIPMENT" );
        add_response_done( _( "Thanks, see you later!" ) );

    } else if( topic == "TALK_DENY_EQUIPMENT" ) {
        add_response_none( _( "Okay, okay, sorry." ) );
        add_response( _( "Seriously, give me more stuff!" ), "TALK_SHARE_EQUIPMENT" );
        add_response_done( _( "Okay, fine, bye." ) );

    } else if( topic == "TALK_TRAIN" ) {
        if( !g->u.backlog.empty() && g->u.backlog.front().id() == activity_id( "ACT_TRAIN" ) ) {
            player_activity &backlog = g->u.backlog.front();
            std::stringstream resume;
            resume << _( "Yes, let's resume training " );
            const skill_id skillt( backlog.name );
            // TODO: This is potentially dangerous. A skill and a martial art could have the same ident!
            if( !skillt.is_valid() ) {
                auto &style = matype_id( backlog.name ).obj();
                resume << style.name;
                add_response( resume.str(), "TALK_TRAIN_START", style );
            } else {
                resume << skillt.obj().name();
                add_response( resume.str(), "TALK_TRAIN_START", skillt );
            }
        }
        std::vector<matype_id> styles = p->styles_offered_to( g->u );
        std::vector<skill_id> trainable = p->skills_offered_to( g->u );
        if( trainable.empty() && styles.empty() ) {
            add_response_none( _( "Oh, okay." ) );
            return;
        }
        for( auto &style_id : styles ) {
            auto &style = style_id.obj();
            const int cost = calc_ma_style_training_cost( *p, style.id );
            //~Martial art style (cost in dollars)
            const std::string text = string_format( cost > 0 ? _( "%s ( cost $%d )" ) : "%s",
                                                    _( style.name.c_str() ), cost / 100 );
            add_response( text, "TALK_TRAIN_START", style );
        }
        for( auto &trained : trainable ) {
            const int cost = calc_skill_training_cost( *p, trained );
            const int cur_level = g->u.get_skill_level( trained );
            //~Skill name: current level -> next level (cost in dollars)
            std::string text = string_format( cost > 0 ? _( "%s: %d -> %d (cost $%d)" ) : _( "%s: %d -> %d" ),
                                              trained.obj().name().c_str(), cur_level, cur_level + 1, cost / 100 );
            add_response( text, "TALK_TRAIN_START", trained );
        }
        add_response_none( _( "Eh, never mind." ) );

    } else if( topic == "TALK_TRAIN_START" ) {
        if( overmap_buffer.is_safe( p->global_omt_location() ) ) {
            add_response( _( "Sounds good." ), "TALK_DONE", &talk_function::start_training );
            add_response_none( _( "On second thought, never mind." ) );
        } else {
            add_response( _( "Okay.  Lead the way." ), "TALK_DONE", &talk_function::lead_to_safety );
            add_response( _( "No, we'll be okay here." ), "TALK_TRAIN_FORCE" );
            add_response_none( _( "On second thought, never mind." ) );
        }

    } else if( topic == "TALK_TRAIN_FORCE" ) {
        add_response( _( "Sounds good." ), "TALK_DONE", &talk_function::start_training );
        add_response_none( _( "On second thought, never mind." ) );

    } else if( topic == "TALK_SUGGEST_FOLLOW" ) {
        if( p->has_effect( effect_infection ) ) {
            add_response_none( _( "Understood.  I'll get those antibiotics." ) );
        } else if( p->has_effect( effect_asked_to_follow ) ) {
            add_response_none( _( "Right, right, I'll ask later." ) );
        } else {
            int strength = 4 * p->op_of_u.fear + p->op_of_u.value + p->op_of_u.trust +
                           ( 10 - p->personality.bravery );
            int weakness = 3 * ( p->personality.altruism - std::max( 0, p->op_of_u.fear ) ) +
                           p->personality.bravery - 3 * p->op_of_u.anger + 2 * p->op_of_u.value;
            int friends = 2 * p->op_of_u.trust + 2 * p->op_of_u.value - 2 * p->op_of_u.anger;
            RESPONSE( _( "I can keep you safe." ) );
            TRIAL( TALK_TRIAL_PERSUADE, strength * 2 );
            SUCCESS( "TALK_AGREE_FOLLOW" );
            SUCCESS_ACTION( &talk_function::follow );
            SUCCESS_OPINION( 1, 0, 1, 0, 0 );
            FAILURE( "TALK_DENY_FOLLOW" );
            FAILURE_ACTION( &talk_function::deny_follow );
            FAILURE_OPINION( 0, 0, -1, 1, 0 );
            RESPONSE( _( "You can keep me safe." ) );
            TRIAL( TALK_TRIAL_PERSUADE, weakness * 2 );
            SUCCESS( "TALK_AGREE_FOLLOW" );
            SUCCESS_ACTION( &talk_function::follow );
            SUCCESS_OPINION( 0, 0, -1, 0, 0 );
            FAILURE( "TALK_DENY_FOLLOW" );
            FAILURE_ACTION( &talk_function::deny_follow );
            FAILURE_OPINION( 0, -1, -1, 1, 0 );
            RESPONSE( _( "We're friends, aren't we?" ) );
            TRIAL( TALK_TRIAL_PERSUADE, friends * 1.5 );
            SUCCESS( "TALK_AGREE_FOLLOW" );
            SUCCESS_ACTION( &talk_function::follow );
            SUCCESS_OPINION( 2, 0, 0, -1, 0 );
            FAILURE( "TALK_DENY_FOLLOW" );
            FAILURE_ACTION( &talk_function::deny_follow );
            FAILURE_OPINION( -1, -2, -1, 1, 0 );
            RESPONSE( _( "I'll kill you if you don't." ) );
            TRIAL( TALK_TRIAL_INTIMIDATE, strength * 2 );
            SUCCESS( "TALK_AGREE_FOLLOW" );
            SUCCESS_ACTION( &talk_function::follow );
            SUCCESS_OPINION( -4, 3, -1, 4, 0 );
            FAILURE( "TALK_DENY_FOLLOW" );
            FAILURE_OPINION( -4, 0, -5, 10, 0 );
        }

    } else if( topic == "TALK_AGREE_FOLLOW" ) {
        add_response( _( "Awesome!" ), "TALK_FRIEND" );
        add_response_done( _( "Okay, let's go!" ) );

    } else if( topic == "TALK_DENY_FOLLOW" ) {
        add_response_none( _( "Oh, okay." ) );

    } else if( topic == "TALK_LEADER" ) {
        int persuade = p->op_of_u.fear + p->op_of_u.value + p->op_of_u.trust -
                       p->personality.bravery - p->personality.aggression;
        if( p->has_destination() ) {
            add_response( _( "How much further?" ), "TALK_HOW_MUCH_FURTHER" );
        }
        add_response( _( "I'm going to go my own way for a while." ), "TALK_LEAVE" );
        if( !p->has_effect( effect_asked_to_lead ) ) {
            RESPONSE( _( "I'd like to lead for a while." ) );
            TRIAL( TALK_TRIAL_PERSUADE, persuade );
            SUCCESS( "TALK_PLAYER_LEADS" );
            SUCCESS_ACTION( &talk_function::follow );
            FAILURE( "TALK_LEADER_STAYS" );
            FAILURE_OPINION( 0, 0, -1, -1, 0 );
            RESPONSE( _( "Step aside.  I'm leader now." ) );
            TRIAL( TALK_TRIAL_INTIMIDATE, 40 );
            SUCCESS( "TALK_PLAYER_LEADS" );
            SUCCESS_ACTION( &talk_function::follow );
            SUCCESS_OPINION( -1, 1, -1, 1, 0 );
            FAILURE( "TALK_LEADER_STAYS" );
            FAILURE_OPINION( -1, 0, -1, 1, 0 );
        }
        add_response( _( "Can I do anything for you?" ), "TALK_MISSION_LIST" );
        add_response( _( "Let's trade items." ), "TALK_NONE", &talk_function::start_trade );
        add_response_done( _( "Let's go." ) );

    } else if( topic == "TALK_LEAVE" ) {
        add_response_none( _( "Nah, I'm just kidding." ) );
        add_response( _( "Yeah, I'm sure.  Bye." ), "TALK_DONE", &talk_function::leave );

    } else if( topic == "TALK_PLAYER_LEADS" ) {
        add_response( _( "Good.  Something else..." ), "TALK_FRIEND" );
        add_response_done( _( "Alright, let's go." ) );

    } else if( topic == "TALK_LEADER_STAYS" ) {
        add_response_none( _( "Okay, okay." ) );

    } else if( topic == "TALK_HOW_MUCH_FURTHER" ) {
        add_response_none( _( "Okay, thanks." ) );
        add_response_done( _( "Let's keep moving." ) );

    } else if( topic == "TALK_FRIEND_GUARD" ) {
        add_response( _( "I need you to come with me." ), "TALK_FRIEND", &talk_function::stop_guard );
        add_response_done( _( "See you around." ) );

    } else if( topic == "TALK_CAMP_OVERSEER" ) {
        p->companion_mission_role_id = "FACTION_CAMP";
        add_response( _( "What needs to be done?" ), "TALK_CAMP_OVERSEER", &talk_function::companion_mission );
        add_response( _( "We're abandoning this camp." ), "TALK_DONE", &talk_function::remove_overseer );
        add_response_done( _( "See you around." ) );

    } else if( topic == "TALK_FRIEND" || topic == "TALK_GIVE_ITEM" || topic == "TALK_USE_ITEM" ) {
        if( p->is_following() ) {
            add_response( _( "Combat commands..." ), "TALK_COMBAT_COMMANDS" );
        }

        add_response( _( "Can I do anything for you?" ), "TALK_MISSION_LIST" );
        if( p->is_following() ) {
            // TODO: Allow NPCs to break training properly
            // Don't allow them to walk away in the middle of training
            std::stringstream reasons;
            if( const optional_vpart_position vp = g->m.veh_at( p->pos() ) ) {
                if( abs( vp->vehicle().velocity ) > 0 ) {
                    reasons << _( "I can't train you properly while you're operating a vehicle!" ) << std::endl;
                }
            }

            if( p->has_effect( effect_asked_to_train ) ) {
                reasons << _( "Give it some time, I'll show you something new later..." ) << std::endl;
            }

            if( p->get_thirst() > 80 ) {
                reasons << _( "I'm too thirsty, give me something to drink." ) << std::endl;
            }

            if( p->get_hunger() > 160 ) {
                reasons << _( "I'm too hungry, give me something to eat." ) << std::endl;
            }

            if( p->get_fatigue() > TIRED ) {
                reasons << _( "I'm too tired, let me rest first." ) << std::endl;
            }

            if( !reasons.str().empty() ) {
                RESPONSE( _( "[N/A] Can you teach me anything?" ) );
                SUCCESS( "TALK_DENY_TRAIN" );
                ret.back().success.next_topic.reason = reasons.str();
            } else {
                RESPONSE( _( "Can you teach me anything?" ) );
                int commitment = 3 * p->op_of_u.trust + 1 * p->op_of_u.value -
                                 3 * p->op_of_u.anger;
                TRIAL( TALK_TRIAL_PERSUADE, commitment * 2 );
                SUCCESS( "TALK_TRAIN" );
                SUCCESS_ACTION( []( npc &p ) { p.chatbin.mission_selected = nullptr; } );
                FAILURE( "TALK_DENY_PERSONAL" );
                FAILURE_ACTION( &talk_function::deny_train );
            }
        }
        add_response( _( "Let's trade items." ), "TALK_FRIEND", &talk_function::start_trade );
        if( p->is_following() && g->m.camp_at( g->u.pos() ) ) {
            add_response( _( "Wait at this base." ), "TALK_DONE", &talk_function::assign_base );
        }
        if( p->is_following() ) {
            RESPONSE( _( "Guard this position." ) );
            SUCCESS( "TALK_FRIEND_GUARD" );
            SUCCESS_ACTION( &talk_function::assign_guard );

            RESPONSE( _( "I'd like to know a bit more about you..." ) );
            SUCCESS( "TALK_FRIEND" );
            SUCCESS_ACTION( &talk_function::reveal_stats );

            add_response( _( "I want you to use this item" ), "TALK_USE_ITEM" );
            add_response( _( "Hold on to this item" ), "TALK_GIVE_ITEM" );
            add_response( _( "Miscellaneous rules..." ), "TALK_MISC_RULES" );

            add_response( _( "I'm going to go my own way for a while." ), "TALK_LEAVE" );
            add_response_done( _( "Let's go." ) );

            add_response( _( "I want you to build a camp here." ), "TALK_DONE", &talk_function::become_overseer );
        }

        if( !p->is_following() ) {
            add_response( _( "I need you to come with me." ), "TALK_FRIEND", &talk_function::stop_guard );
            add_response_done( _( "Bye." ) );
        }

    } else if( topic == "TALK_FRIEND_UNCOMFORTABLE" ) {
        add_response( _( "I'll give you some space." ), "TALK_FRIEND" );

    } else if( topic == "TALK_DENY_TRAIN" ) {
        add_response( _( "Very well..." ), "TALK_FRIEND" );

    } else if( topic == "TALK_DENY_PERSONAL" ) {
        add_response( _( "I understand..." ), "TALK_FRIEND" );

    } else if( topic == "TALK_COMBAT_COMMANDS" ) {
        add_response( _( "Change your engagement rules..." ), "TALK_COMBAT_ENGAGEMENT" );
        add_response( _( "Change your aiming rules..." ), "TALK_AIM_RULES" );
        add_response( p->rules.use_guns ? _( "Don't use guns anymore." ) : _( "You can use guns." ),
        "TALK_COMBAT_COMMANDS",  []( npc & np ) {
            np.rules.use_guns = !np.rules.use_guns;
        } );
        add_response( p->rules.use_silent ? _( "Don't worry about noise." ) :
                      _( "Use only silent weapons." ),
        "TALK_COMBAT_COMMANDS", []( npc & np ) {
            np.rules.use_silent = !np.rules.use_silent;
        } );
        add_response( p->rules.use_grenades ? _( "Don't use grenades anymore." ) :
                      _( "You can use grenades." ),
        "TALK_COMBAT_COMMANDS", []( npc & np ) {
            np.rules.use_grenades = !np.rules.use_grenades;
        } );
        add_response_none( _( "Never mind." ) );

    } else if( topic == "TALK_COMBAT_ENGAGEMENT" ) {
        struct engagement_setting {
            combat_engagement rule;
            std::string description;
        };
        static const std::vector<engagement_setting> engagement_settings = {{
                { ENGAGE_NONE, _( "Don't fight unless your life depends on it." ) },
                { ENGAGE_CLOSE, _( "Attack enemies that get too close." ) },
                { ENGAGE_WEAK, _( "Attack enemies that you can kill easily." ) },
                { ENGAGE_HIT, _( "Attack only enemies that I attack first." ) },
                { ENGAGE_NO_MOVE, _( "Attack only enemies you can reach without moving." ) },
                { ENGAGE_ALL, _( "Attack anything you want." ) },
            }
        };

        for( const auto &setting : engagement_settings ) {
            if( p->rules.engagement == setting.rule ) {
                continue;
            }

            combat_engagement eng = setting.rule;
            add_response( setting.description, "TALK_NONE", [eng]( npc & np ) {
                np.rules.engagement = eng;
            }, dialogue_consequence::none );
        }
        add_response_none( _( "Never mind." ) );

    } else if( topic == "TALK_AIM_RULES" ) {
        struct aim_setting {
            aim_rule rule;
            std::string description;
        };
        static const std::vector<aim_setting> aim_settings = {{
                { AIM_WHEN_CONVENIENT, _( "Aim when it's convenient." ) },
                { AIM_SPRAY, _( "Go wild, you don't need to aim much." ) },
                { AIM_PRECISE, _( "Take your time, aim carefully." ) },
                { AIM_STRICTLY_PRECISE, _( "Don't shoot if you can't aim really well." ) },
            }
        };

        for( const auto &setting : aim_settings ) {
            if( p->rules.aim == setting.rule ) {
                continue;
            }

            aim_rule ar = setting.rule;
            add_response( setting.description, "TALK_NONE", [ar]( npc & np ) {
                np.rules.aim = ar;
            }, dialogue_consequence::none );
        }
        add_response_none( _( "Never mind." ) );

    } else if( topic == "TALK_STRANGER_NEUTRAL" || topic == "TALK_STRANGER_WARY" ||
               topic == "TALK_STRANGER_SCARED" || topic == "TALK_STRANGER_FRIENDLY" ) {
        if( topic == "TALK_STRANGER_NEUTRAL" || topic == "TALK_STRANGER_FRIENDLY" ) {
            add_response( _( "Another survivor!  We should travel together." ), "TALK_SUGGEST_FOLLOW" );
            add_response( _( "What are you doing?" ), "TALK_DESCRIBE_MISSION" );
            add_response( _( "Care to trade?" ), "TALK_DONE", &talk_function::start_trade );
            add_response_done( _( "Bye." ) );
        } else {
            if( !g->u.unarmed_attack() ) {
                if( g->u.volume_carried() + g->u.weapon.volume() <= g->u.volume_capacity() ) {
                    RESPONSE( _( "&Put away weapon." ) );
                    SUCCESS( "TALK_STRANGER_NEUTRAL" );
                    SUCCESS_ACTION( &talk_function::player_weapon_away );
                    SUCCESS_OPINION( 2, -2, 0, 0, 0 );
                    SUCCESS_ACTION( &talk_function::stranger_neutral );
                }
                RESPONSE( _( "&Drop weapon." ) );
                SUCCESS( "TALK_STRANGER_NEUTRAL" );
                SUCCESS_ACTION( &talk_function::player_weapon_drop );
                SUCCESS_OPINION( 4, -3, 0, 0, 0 );
            }
            int diff = 50 + p->personality.bravery - 2 * p->op_of_u.fear + 2 * p->op_of_u.trust;
            RESPONSE( _( "Don't worry, I'm not going to hurt you." ) );
            TRIAL( TALK_TRIAL_PERSUADE, diff );
            SUCCESS( "TALK_STRANGER_NEUTRAL" );
            SUCCESS_OPINION( 1, -1, 0, 0, 0 );
            SUCCESS_ACTION( &talk_function::stranger_neutral );
            FAILURE( "TALK_DONE" );
            FAILURE_ACTION( &talk_function::flee );
        }
        if( !p->unarmed_attack() ) {
            RESPONSE( _( "Drop your weapon!" ) );
            TRIAL( TALK_TRIAL_INTIMIDATE, 30 );
            SUCCESS( "TALK_WEAPON_DROPPED" );
            SUCCESS_ACTION( &talk_function::drop_weapon );
            FAILURE( "TALK_DONE" );
            FAILURE_ACTION( &talk_function::hostile );
        }
        RESPONSE( _( "Get out of here or I'll kill you." ) );
        TRIAL( TALK_TRIAL_INTIMIDATE, 20 );
        SUCCESS( "TALK_DONE" );
        SUCCESS_ACTION( &talk_function::flee );
        FAILURE( "TALK_DONE" );
        FAILURE_ACTION( &talk_function::hostile );

    } else if( topic == "TALK_STRANGER_AGGRESSIVE" || topic == "TALK_MUG" ) {
        if( !g->u.unarmed_attack() ) {
            int chance = 30 + p->personality.bravery - 3 * p->personality.aggression +
                         2 * p->personality.altruism - 2 * p->op_of_u.fear +
                         3 * p->op_of_u.trust;
            RESPONSE( _( "Calm down.  I'm not going to hurt you." ) );
            TRIAL( TALK_TRIAL_PERSUADE, chance );
            SUCCESS( "TALK_STRANGER_WARY" );
            SUCCESS_OPINION( 1, -1, 0, 0, 0 );
            SUCCESS_ACTION( &talk_function::stranger_neutral );
            FAILURE( "TALK_DONE" );
            FAILURE_ACTION( &talk_function::hostile );
            RESPONSE( _( "Screw you, no." ) );
            TRIAL( TALK_TRIAL_INTIMIDATE, chance - 5 );
            SUCCESS( "TALK_STRANGER_SCARED" );
            SUCCESS_OPINION( -2, 1, 0, 1, 0 );
            FAILURE( "TALK_DONE" );
            FAILURE_ACTION( &talk_function::hostile );
            RESPONSE( _( "&Drop weapon." ) );
            if( topic == "TALK_MUG" ) {
                SUCCESS( "TALK_MUG" );
            } else {
                SUCCESS( "TALK_DEMAND_LEAVE" );
            }
            SUCCESS_ACTION( &talk_function::player_weapon_drop );

        } else if( topic == "TALK_MUG" ) {
            int chance = 35 + p->personality.bravery - 3 * p->personality.aggression +
                         2 * p->personality.altruism - 2 * p->op_of_u.fear +
                         3 * p->op_of_u.trust;
            RESPONSE( _( "Calm down.  I'm not going to hurt you." ) );
            TRIAL( TALK_TRIAL_PERSUADE, chance );
            SUCCESS( "TALK_STRANGER_WARY" );
            SUCCESS_OPINION( 1, -1, 0, 0, 0 );
            SUCCESS_ACTION( &talk_function::stranger_neutral );
            FAILURE( "TALK_DONE" );
            FAILURE_ACTION( &talk_function::hostile );
            RESPONSE( _( "Screw you, no." ) );
            TRIAL( TALK_TRIAL_INTIMIDATE, chance - 5 );
            SUCCESS( "TALK_STRANGER_SCARED" );
            SUCCESS_OPINION( -2, 1, 0, 1, 0 );
            FAILURE( "TALK_DONE" );
            FAILURE_ACTION( &talk_function::hostile );
            add_response( _( "&Put hands up." ), "TALK_DONE", &talk_function::start_mugging );
        }

    } else if( topic == "TALK_DESCRIBE_MISSION" ) {
        add_response_none( _( "I see." ) );
        add_response_done( _( "Bye." ) );

    } else if( topic == "TALK_WEAPON_DROPPED" ) {
        RESPONSE( _( "Now get out of here." ) );
        SUCCESS( "TALK_DONE" );
        SUCCESS_OPINION( -1, -2, 0, -2, 0 );
        SUCCESS_ACTION( &talk_function::flee );

    } else if( topic == "TALK_DEMAND_LEAVE" ) {
        RESPONSE( _( "Okay, I'm going." ) );
        SUCCESS( "TALK_DONE" );
        SUCCESS_OPINION( 0, -1, 0, 0, 0 );
        SUCCESS_ACTION( &talk_function::player_leaving );

    } else if( topic == "TALK_SIZE_UP" || topic == "TALK_LOOK_AT" ||
               topic == "TALK_OPINION" || topic == "TALK_SHOUT" ) {
        add_response_none( _( "Okay." ) );

    } else if( topic == "TALK_WAKE_UP" ) {
        add_response( _( "Wake up!" ), "TALK_NONE", &talk_function::wake_up );
        add_response_done( _( "Go back to sleep." ) );

    } else if( topic == "TALK_MISC_RULES" ) {
        add_response( _( "Follow same rules as this follower." ), "TALK_MISC_RULES", []( npc & np ) {
            const npc *other = pick_follower();
            if( other != nullptr && other != &np ) {
                np.rules = other->rules;
            }
        } );

        add_response( p->rules.allow_pick_up ? _( "Don't pick up items." ) :
                      _( "You can pick up items now." ),
        "TALK_MISC_RULES", []( npc & np ) {
            np.rules.allow_pick_up = !np.rules.allow_pick_up;
        } );
        add_response( p->rules.allow_bash ? _( "Don't bash obstacles." ) : _( "You can bash obstacles." ),
        "TALK_MISC_RULES", []( npc & np ) {
            np.rules.allow_bash = !np.rules.allow_bash;
        } );
        add_response( p->rules.allow_sleep ? _( "Stay awake." ) : _( "Sleep when you feel tired." ),
        "TALK_MISC_RULES", []( npc & np ) {
            np.rules.allow_sleep = !np.rules.allow_sleep;
        } );
        add_response( p->rules.allow_complain ? _( "Stay quiet." ) :
                      _( "Tell me when you need something." ),
        "TALK_MISC_RULES", []( npc & np ) {
            np.rules.allow_complain = !np.rules.allow_complain;
        } );
        add_response( p->rules.allow_pulp ? _( "Leave corpses alone." ) : _( "Smash zombie corpses." ),
        "TALK_MISC_RULES", []( npc & np ) {
            np.rules.allow_pulp = !np.rules.allow_pulp;
        } );
        add_response( p->rules.close_doors ? _( "Leave doors open." ) : _( "Close the doors." ),
        "TALK_MISC_RULES", []( npc & np ) {
            np.rules.close_doors = !np.rules.close_doors;
        } );

        add_response( _( "Set up pickup rules." ), "TALK_MISC_RULES", []( npc & np ) {
            const std::string title = string_format( _( "Pickup rules for %s" ), np.name.c_str() );
            np.rules.pickup_whitelist->show( title, false );
        } );

        add_response_none( _( "Never mind." ) );

    }

    if( g->u.has_trait( trait_DEBUG_MIND_CONTROL ) && !p->is_friend() ) {
        add_response( _( "OBEY ME!" ), "TALK_MIND_CONTROL" );
        add_response_done( _( "Bye." ) );
    }

    if( ret.empty() ) {
        add_response_done( _( "Bye." ) );
    }
}

int talk_trial::calc_chance( const dialogue &d ) const
{
    player &u = *d.alpha;
    if( u.has_trait( trait_DEBUG_MIND_CONTROL ) ) {
        return 100;
    }
    const social_modifiers &u_mods = u.get_mutation_social_mods();

    npc &p = *d.beta;
    int chance = difficulty;
    switch( type ) {
        case TALK_TRIAL_NONE:
        case NUM_TALK_TRIALS:
            dbg( D_ERROR ) << "called calc_chance with invalid talk_trial value: " << type;
            break;
        case TALK_TRIAL_LIE:
            chance += u.talk_skill() - p.talk_skill() + p.op_of_u.trust * 3;
            chance += u_mods.lie;

            if( u.has_bionic( bionic_id( "bio_voice" ) ) ) { //come on, who would suspect a robot of lying?
                chance += 10;
            }
            if( u.has_bionic( bionic_id( "bio_face_mask" ) ) ) {
                chance += 20;
            }
            break;
        case TALK_TRIAL_PERSUADE:
            chance += u.talk_skill() - int( p.talk_skill() / 2 ) +
                      p.op_of_u.trust * 2 + p.op_of_u.value;
            chance += u_mods.persuade;

            if( u.has_bionic( bionic_id( "bio_face_mask" ) ) ) {
                chance += 10;
            }
            if( u.has_bionic( bionic_id( "bio_deformity" ) ) ) {
                chance -= 50;
            }
            if( u.has_bionic( bionic_id( "bio_voice" ) ) ) {
                chance -= 20;
            }
            break;
        case TALK_TRIAL_INTIMIDATE:
            chance += u.intimidation() - p.intimidation() + p.op_of_u.fear * 2 -
                      p.personality.bravery * 2;
            chance += u_mods.intimidate;

            if( u.has_bionic( bionic_id( "bio_face_mask" ) ) ) {
                chance += 10;
            }
            if( u.has_bionic( bionic_id( "bio_armor_eyes" ) ) ) {
                chance += 10;
            }
            if( u.has_bionic( bionic_id( "bio_deformity" ) ) ) {
                chance += 20;
            }
            if( u.has_bionic( bionic_id( "bio_voice" ) ) ) {
                chance += 20;
            }
            break;
    }

    return std::max( 0, std::min( 100, chance ) );
}

bool talk_trial::roll( dialogue &d ) const
{
    player &u = *d.alpha;
    if( type == TALK_TRIAL_NONE || u.has_trait( trait_DEBUG_MIND_CONTROL ) ) {
        return true;
    }
    int const chance = calc_chance( d );
    bool const success = rng( 0, 99 ) < chance;
    if( success ) {
        u.practice( skill_speech, ( 100 - chance ) / 10 );
    } else {
        u.practice( skill_speech, ( 100 - chance ) / 7 );
    }
    return success;
}

int topic_category( const talk_topic &the_topic )
{
    const auto &topic = the_topic.id;
    // TODO: ideally, this would be a property of the topic itself.
    // How this works: each category has a set of topics that belong to it, each set is checked
    // for the given topic and if a set contains, the category number is returned.
    static const std::unordered_set<std::string> topic_1 = { {
            "TALK_MISSION_START", "TALK_MISSION_DESCRIBE", "TALK_MISSION_OFFER", "TALK_MISSION_ACCEPTED",
            "TALK_MISSION_REJECTED", "TALK_MISSION_ADVICE", "TALK_MISSION_INQUIRE", "TALK_MISSION_SUCCESS",
            "TALK_MISSION_SUCCESS_LIE", "TALK_MISSION_FAILURE", "TALK_MISSION_REWARD", "TALK_MISSION_END"
        }
    };
    if( topic_1.count( topic ) > 0 ) {
        return 1;
    }
    static const std::unordered_set<std::string> topic_2 = { {
            "TALK_SHARE_EQUIPMENT", "TALK_GIVE_EQUIPMENT", "TALK_DENY_EQUIPMENT"
        }
    };
    if( topic_2.count( topic ) > 0 ) {
        return 2;
    }
    static const std::unordered_set<std::string> topic_3 = { {
            "TALK_SUGGEST_FOLLOW", "TALK_AGREE_FOLLOW", "TALK_DENY_FOLLOW",
        }
    };
    if( topic_3.count( topic ) > 0 ) {
        return 3;
    }
    static const std::unordered_set<std::string> topic_4 = { {
            "TALK_COMBAT_ENGAGEMENT",
        }
    };
    if( topic_4.count( topic ) > 0 ) {
        return 4;
    }
    static const std::unordered_set<std::string> topic_5 = { {
            "TALK_COMBAT_COMMANDS",
        }
    };
    if( topic_5.count( topic ) > 0 ) {
        return 5;
    }
    static const std::unordered_set<std::string> topic_6 = { {
            "TALK_TRAIN", "TALK_TRAIN_START", "TALK_TRAIN_FORCE"
        }
    };
    if( topic_6.count( topic ) > 0 ) {
        return 6;
    }
    static const std::unordered_set<std::string> topic_7 = { {
            "TALK_MISC_RULES",
        }
    };
    if( topic_7.count( topic ) > 0 ) {
        return 7;
    }
    static const std::unordered_set<std::string> topic_8 = { {
            "TALK_AIM_RULES",
        }
    };
    if( topic_8.count( topic ) > 0 ) {
        return 8;
    }
    static const std::unordered_set<std::string> topic_9 = { {
            "TALK_FRIEND", "TALK_GIVE_ITEM", "TALK_USE_ITEM",
        }
    };
    if( topic_9.count( topic ) > 0 ) {
        return 9;
    }
    static const std::unordered_set<std::string> topic_99 = { {
            "TALK_SIZE_UP", "TALK_LOOK_AT", "TALK_OPINION", "TALK_SHOUT"
        }
    };
    if( topic_99.count( topic ) > 0 ) {
        return 99;
    }
    return -1; // Not grouped with other topics
}

void talk_function::nothing( npc & )
{
}

void talk_function::assign_mission( npc &p )
{
    mission *miss = p.chatbin.mission_selected;
    if( miss == nullptr ) {
        debugmsg( "assign_mission: mission_selected == nullptr" );
        return;
    }
    miss->assign( g->u );
    p.chatbin.missions_assigned.push_back( miss );
    const auto it = std::find( p.chatbin.missions.begin(), p.chatbin.missions.end(), miss );
    p.chatbin.missions.erase( it );
}

void talk_function::mission_success( npc &p )
{
    mission *miss = p.chatbin.mission_selected;
    if( miss == nullptr ) {
        debugmsg( "mission_success: mission_selected == nullptr" );
        return;
    }

    int miss_val = cash_to_favor( p, miss->get_value() );
    npc_opinion tmp( 0, 0, 1 + miss_val / 5, -1, 0 );
    p.op_of_u += tmp;
    if( p.my_fac != nullptr ) {
        int fac_val = std::min( 1 + miss_val / 10, 10 );
        p.my_fac->likes_u += fac_val;
        p.my_fac->respects_u += fac_val;
        p.my_fac->power += fac_val;
    }
    miss->wrap_up();
}

void talk_function::mission_failure( npc &p )
{
    mission *miss = p.chatbin.mission_selected;
    if( miss == nullptr ) {
        debugmsg( "mission_failure: mission_selected == nullptr" );
        return;
    }
    npc_opinion tmp( -1, 0, -1, 1, 0 );
    p.op_of_u += tmp;
    miss->fail();
}

void talk_function::clear_mission( npc &p )
{
    mission *miss = p.chatbin.mission_selected;
    if( miss == nullptr ) {
        debugmsg( "clear_mission: mission_selected == nullptr" );
        return;
    }
    const auto it = std::find( p.chatbin.missions_assigned.begin(), p.chatbin.missions_assigned.end(),
                               miss );
    if( it == p.chatbin.missions_assigned.end() ) {
        debugmsg( "clear_mission: mission_selected not in assigned" );
        return;
    }
    p.chatbin.missions_assigned.erase( it );
    if( p.chatbin.missions_assigned.empty() ) {
        p.chatbin.mission_selected = nullptr;
    } else {
        p.chatbin.mission_selected = p.chatbin.missions_assigned.front();
    }
    if( miss->has_follow_up() ) {
        p.add_new_mission( mission::reserve_new( miss->get_follow_up(), p.getID() ) );
    }
}

void talk_function::mission_reward( npc &p )
{
    const mission *miss = p.chatbin.mission_selected;
    if( miss == nullptr ) {
        debugmsg( "Called mission_reward with null mission" );
        return;
    }

    int mission_value = miss->get_value();
    p.op_of_u.owed += mission_value;
    trade( p, 0, _( "Reward" ) );
}

void talk_function::start_trade( npc &p )
{
    trade( p, 0, _( "Trade" ) );
}

std::string bulk_trade_inquire( const npc &, const itype_id &it )
{
    int you_have = g->u.charges_of( it );
    item tmp( it );
    int item_cost = tmp.price( true );
    tmp.charges = you_have;
    int total_cost = tmp.price( true );
    return string_format( _( "I'm willing to pay %s per batch for a total of %s" ),
                          format_money( item_cost ), format_money( total_cost ) );
}

void bulk_trade_accept( npc &, const itype_id &it )
{
    int you_have = g->u.charges_of( it );
    item tmp( it );
    tmp.charges = you_have;
    int total = tmp.price( true );
    g->u.use_charges( it, you_have );
    g->u.cash += total;
}

void talk_function::assign_base( npc &p )
{
    // TODO: decide what to do upon assign? maybe pathing required
    basecamp *camp = g->m.camp_at( g->u.pos() );
    if( !camp ) {
        dbg( D_ERROR ) << "talk_function::assign_base: Assigned to base but no base here.";
        return;
    }

    add_msg( _( "%1$s waits at %2$s" ), p.name.c_str(), camp->camp_name().c_str() );
    p.mission = NPC_MISSION_BASE;
    p.set_attitude( NPCATT_NULL );
}

void talk_function::assign_guard( npc &p )
{
    add_msg( _( "%s is posted as a guard." ), p.name.c_str() );
    p.set_attitude( NPCATT_NULL );
    p.mission = NPC_MISSION_GUARD;
    p.chatbin.first_topic = "TALK_FRIEND_GUARD";
    p.set_destination();
}

void talk_function::stop_guard( npc &p )
{
    p.set_attitude( NPCATT_FOLLOW );
    add_msg( _( "%s begins to follow you." ), p.name.c_str() );
    p.mission = NPC_MISSION_NULL;
    p.chatbin.first_topic = "TALK_FRIEND";
    p.goal = npc::no_goal_point;
    p.guard_pos = npc::no_goal_point;
}

void talk_function::become_overseer( npc &p )
{
    if( query_yn( _("Would you like to review the faction camp description?") ) ){
        faction_camp_tutorial();
    }

    const point omt_pos = ms_to_omt_copy( g->m.getabs( p.posx(), p.posy() ) );
    oter_id &omt_ref = overmap_buffer.ter( omt_pos.x, omt_pos.y, p.posz() );

    if( omt_ref.id() != "field" ){
        popup( _("You must build your camp in an empty field.") );
        return;
    }

    std::vector<std::pair<std::string, tripoint>> om_region = om_building_region( p, 1 );
    for( const auto &om_near : om_region ){
        if ( om_near.first != "field" && om_near.first != "forest" && om_near.first != "forest_thick" &&
                om_near.first != "forest_water" && om_near.first.find("river_") == std::string::npos ){
            popup( _("You need more room for camp expansions!") );
            return;
        }
    }
    std::vector<std::pair<std::string, tripoint>> om_region_extended = om_building_region( p, 3 );
    int forests = 0;
    int waters = 0;
    int swamps = 0;
    int fields = 0;
    for( const auto &om_near : om_region_extended ){
        if( om_near.first.find("faction_base_camp") != std::string::npos ){
            popup( _("You are too close to another camp!") );
            return;
        }
        if( om_near.first == "forest" || om_near.first == "forest_thick" ){
            forests++;
        } else if( om_near.first.find("river_") != std::string::npos ){
            waters++;
        } else if( om_near.first == "forest_water" ){
            swamps++;
        } else if( om_near.first == "field" ){
            fields++;
        }
    }

    bool display = false;
    std::string buffer = _("Warning, you have selected a region with the following issues:\n \n");
    if( forests < 3 ){
        display = true;
        buffer = buffer + _("There are few forests.  Wood is your primary construction material.\n");
    }
    if( waters == 0 ){
        display = true;
        buffer = buffer + _("There are few large clean-ish water sources.\n");
    }
    if( swamps == 0 ){
        display = true;
        buffer = buffer + _("There are no swamps.  Swamps provide access to a few late game industries.\n");
    }
    if( fields < 4 ){
        display = true;
        buffer = buffer + _("There are few fields.  Producing enough food to supply your camp may be difficult.\n");
    }
    if ( display && !query_yn( "%s \nAre you sure you wish to continue? ", buffer )) {
        return;
    }

    editmap edit;
    if (!edit.mapgen_set( "faction_base_camp_0", tripoint(omt_pos.x, omt_pos.y, p.posz() ) ) ){
        popup( _("You weren't able to survey the camp site.") );
        return;
    }

    add_msg( _( "%s has become a camp manager." ), p.name.c_str() );
    if( p.name.find( _(", Camp Manager") ) == std::string::npos ){
        p.name = p.name + _(", Camp Manager");
    }
    p.companion_mission_role_id = "FACTION_CAMP";
    p.set_attitude( NPCATT_NULL );
    p.mission = NPC_MISSION_GUARD;
    p.chatbin.first_topic = "TALK_CAMP_OVERSEER";
    p.set_destination();
}

void talk_function::remove_overseer( npc &p )
{
    if ( !query_yn( "This is permanent, any companions away on mission will be lost and the camp cannot be reclaimed!  Are "
                   "you sure?") ) {
        return;
    }
    add_msg( _( "%s has abandoned the camp." ), p.name.c_str() );
    p.companion_mission_role_id.clear();
    stop_guard(p);
}

void talk_function::wake_up( npc &p )
{
    p.rules.allow_sleep = false;
    p.remove_effect( effect_allow_sleep );
    p.remove_effect( effect_lying_down );
    p.remove_effect( effect_sleep );
    // TODO: Get mad at player for waking us up unless we're in danger
}

void talk_function::reveal_stats( npc &p )
{
    p.disp_info();
}

void talk_function::end_conversation( npc &p )
{
    add_msg( _( "%s starts ignoring you." ), p.name.c_str() );
    p.chatbin.first_topic = "TALK_DONE";
}

void talk_function::insult_combat( npc &p )
{
    add_msg( _( "You start a fight with %s!" ), p.name.c_str() );
    p.chatbin.first_topic = "TALK_DONE";
    p.set_attitude( NPCATT_KILL );
}

void talk_function::give_equipment( npc &p )
{
    std::vector<item_pricing> giving = init_selling( p );
    int chosen = -1;
    while( chosen == -1 && giving.size() > 1 ) {
        int index = rng( 0, giving.size() - 1 );
        if( giving[index].price < p.op_of_u.owed ) {
            chosen = index;
        }
        giving.erase( giving.begin() + index );
    }
    if( giving.empty() ) {
        popup( _( "%s has nothing to give!" ), p.name.c_str() );
        return;
    }
    if( chosen == -1 ) {
        chosen = 0;
    }
    item it = *giving[chosen].loc.get_item();
    giving[chosen].loc.remove_item();
    popup( _( "%1$s gives you a %2$s" ), p.name.c_str(), it.tname().c_str() );

    g->u.i_add( it );
    p.op_of_u.owed -= giving[chosen].price;
    p.add_effect( effect_asked_for_item, 3_hours );
}

void talk_function::give_aid( npc &p )
{
    g->u.cash -= 20000;
    p.add_effect( effect_currently_busy, 30_minutes );
    body_part bp_healed;
    for( int i = 0; i < num_hp_parts; i++ ) {
        bp_healed = player::hp_to_bp( hp_part( i ) );
        g->u.heal( hp_part( i ), 5 * rng( 2, 5 ) );
        if( g->u.has_effect( effect_bite, bp_healed ) ) {
            g->u.remove_effect( effect_bite, bp_healed );
        }
        if( g->u.has_effect( effect_bleed, bp_healed ) ) {
            g->u.remove_effect( effect_bleed, bp_healed );
        }
        if( g->u.has_effect( effect_infected, bp_healed ) ) {
            g->u.remove_effect( effect_infected, bp_healed );
        }
    }
    g->u.assign_activity( activity_id( "ACT_WAIT_NPC" ), 10000 );
    g->u.activity.str_values.push_back( p.name );
}

void talk_function::give_all_aid( npc &p )
{
    g->u.cash -= 30000;
    p.add_effect( effect_currently_busy, 30_minutes );
    give_aid( p );
    body_part bp_healed;
    for( npc &guy : g->all_npcs() ) {
        if( rl_dist( guy.pos(), g->u.pos() ) < PICKUP_RANGE && guy.is_friend() ) {
            for( int i = 0; i < num_hp_parts; i++ ) {
                bp_healed = player::hp_to_bp( hp_part( i ) );
                guy.heal( hp_part( i ), 5 * rng( 2, 5 ) );
                if( guy.has_effect( effect_bite, bp_healed ) ) {
                    guy.remove_effect( effect_bite, bp_healed );
                }
                if( guy.has_effect( effect_bleed, bp_healed ) ) {
                    guy.remove_effect( effect_bleed, bp_healed );
                }
                if( guy.has_effect( effect_infected, bp_healed ) ) {
                    guy.remove_effect( effect_infected, bp_healed );
                }
            }
        }
    }
}

void talk_function::buy_haircut( npc &p )
{
    g->u.add_morale( MORALE_HAIRCUT, 5, 5, 720_minutes, 3_minutes );
    g->u.cash -= 1000;
    g->u.assign_activity( activity_id( "ACT_WAIT_NPC" ), 300 );
    g->u.activity.str_values.push_back( p.name );
    add_msg( m_good, _( "%s gives you a decent haircut..." ), p.name.c_str() );
}

void talk_function::buy_shave( npc &p )
{
    g->u.add_morale( MORALE_SHAVE, 10, 10, 360_minutes, 3_minutes );
    g->u.cash -= 500;
    g->u.assign_activity( activity_id( "ACT_WAIT_NPC" ), 100 );
    g->u.activity.str_values.push_back( p.name );
    add_msg( m_good, _( "%s gives you a decent shave..." ), p.name.c_str() );
}

void talk_function::buy_10_logs( npc &p )
{
    std::vector<tripoint> places = overmap_buffer.find_all(
                                       g->u.global_omt_location(), "ranch_camp_67", 1, false );
    if( places.empty() ) {
        debugmsg( "Couldn't find %s", "ranch_camp_67" );
        return;
    }
    const auto &cur_om = g->get_cur_om();
    std::vector<tripoint> places_om;
    for( auto &i : places ) {
        if( &cur_om == overmap_buffer.get_existing_om_global( i ) ) {
            places_om.push_back( i );
        }
    }

    const tripoint site = random_entry( places_om );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.spawn_item( 7, 15, "log", 10 );
    bay.save();

    p.add_effect( effect_currently_busy, 1_days );
    g->u.cash -= 200000;
    add_msg( m_good, _( "%s drops the logs off in the garage..." ), p.name.c_str() );
}

void talk_function::buy_100_logs( npc &p )
{
    std::vector<tripoint> places = overmap_buffer.find_all(
                                       g->u.global_omt_location(), "ranch_camp_67", 1, false );
    if( places.empty() ) {
        debugmsg( "Couldn't find %s", "ranch_camp_67" );
        return;
    }
    const auto &cur_om = g->get_cur_om();
    std::vector<tripoint> places_om;
    for( auto &i : places ) {
        if( &cur_om == overmap_buffer.get_existing_om_global( i ) ) {
            places_om.push_back( i );
        }
    }

    const tripoint site = random_entry( places_om );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.spawn_item( 7, 15, "log", 100 );
    bay.save();

    p.add_effect( effect_currently_busy, 7_days );
    g->u.cash -= 1200000;
    add_msg( m_good, _( "%s drops the logs off in the garage..." ), p.name.c_str() );
}


void talk_function::follow( npc &p )
{
    p.set_attitude( NPCATT_FOLLOW );
    g->u.cash += p.cash;
    p.cash = 0;
}

void talk_function::deny_follow( npc &p )
{
    p.add_effect( effect_asked_to_follow, 6_hours );
}

void talk_function::deny_lead( npc &p )
{
    p.add_effect( effect_asked_to_lead, 6_hours );
}

void talk_function::deny_equipment( npc &p )
{
    p.add_effect( effect_asked_for_item, 1_hours );
}

void talk_function::deny_train( npc &p )
{
    p.add_effect( effect_asked_to_train, 6_hours );
}

void talk_function::deny_personal_info( npc &p )
{
    p.add_effect( effect_asked_personal_info, 3_hours );
}

void talk_function::hostile( npc &p )
{
    if( p.get_attitude() == NPCATT_KILL ) {
        return;
    }

    if( p.sees( g->u ) ) {
        add_msg( _( "%s turns hostile!" ), p.name.c_str() );
    }

    g->u.add_memorial_log( pgettext( "memorial_male", "%s became hostile." ),
                           pgettext( "memorial_female", "%s became hostile." ),
                           p.name.c_str() );
    p.set_attitude( NPCATT_KILL );
}

void talk_function::flee( npc &p )
{
    add_msg( _( "%s turns to flee!" ), p.name.c_str() );
    p.set_attitude( NPCATT_FLEE );
}

void talk_function::leave( npc &p )
{
    add_msg( _( "%s leaves." ), p.name.c_str() );
    p.set_attitude( NPCATT_NULL );
}

void talk_function::stranger_neutral( npc &p )
{
    add_msg( _( "%s feels less threatened by you." ), p.name.c_str() );
    p.set_attitude( NPCATT_NULL );
    p.chatbin.first_topic = "TALK_STRANGER_NEUTRAL";
}

void talk_function::start_mugging( npc &p )
{
    p.set_attitude( NPCATT_MUG );
    add_msg( _( "Pause to stay still.  Any movement may cause %s to attack." ),
             p.name.c_str() );
}

void talk_function::player_leaving( npc &p )
{
    p.set_attitude( NPCATT_WAIT_FOR_LEAVE );
    p.patience = 15 - p.personality.aggression;
}

void talk_function::drop_weapon( npc &p )
{
    g->m.add_item_or_charges( p.pos(), p.remove_weapon() );
}

void talk_function::player_weapon_away( npc &p )
{
    ( void )p; //unused
    g->u.i_add( g->u.remove_weapon() );
}

void talk_function::player_weapon_drop( npc &p )
{
    ( void )p; // unused
    g->m.add_item_or_charges( g->u.pos(), g->u.remove_weapon() );
}

void talk_function::lead_to_safety( npc &p )
{
    const auto mission = mission::reserve_new( mission_type_id( "MISSION_REACH_SAFETY" ), -1 );
    mission->assign( g->u );
    p.goal = mission->get_target();
    p.set_attitude( NPCATT_LEAD );
}

bool pay_npc( npc &np, int cost )
{
    if( np.op_of_u.owed >= cost ) {
        np.op_of_u.owed -= cost;
        return true;
    }

    if( g->u.cash + ( unsigned long )np.op_of_u.owed >= ( unsigned long )cost ) {
        g->u.cash -= cost - np.op_of_u.owed;
        np.op_of_u.owed = 0;
        return true;
    }

    return trade( np, -cost, _( "Pay:" ) );
}

void talk_function::start_training( npc &p )
{
    int cost;
    time_duration time = 0_turns;
    std::string name;
    const skill_id &skill = p.chatbin.skill;
    const matype_id &style = p.chatbin.style;
    if( skill.is_valid() && g->u.get_skill_level( skill ) < p.get_skill_level( skill ) ) {
        cost = calc_skill_training_cost( p, skill );
        time = calc_skill_training_time( p, skill );
        name = skill.str();
    } else if( p.chatbin.style.is_valid() && !g->u.has_martialart( style ) ) {
        cost = calc_ma_style_training_cost( p, style );
        time = calc_ma_style_training_time( p, style );
        name = p.chatbin.style.str();
    } else {
        debugmsg( "start_training with no valid skill or style set" );
        return;
    }

    mission *miss = p.chatbin.mission_selected;
    if( miss != nullptr && miss->get_assigned_player_id() == g->u.getID() ) {
        clear_mission( p );
    } else if( !pay_npc( p, cost ) ) {
        return;
    }
    g->u.assign_activity( activity_id( "ACT_TRAIN" ), to_moves<int>( time ), p.getID(), 0, name );
    p.add_effect( effect_asked_to_train, 6_hours );
}

void parse_tags( std::string &phrase, const player &u, const player &me )
{
    phrase = remove_color_tags( phrase );

    size_t fa;
    size_t fb;
    std::string tag;
    do {
        fa = phrase.find( '<' );
        fb = phrase.find( '>' );
        int l = fb - fa + 1;
        if( fa != std::string::npos && fb != std::string::npos ) {
            tag = phrase.substr( fa, fb - fa + 1 );
        } else {
            return;
        }

        const std::string &replacement = SNIPPET.random_from_category( tag );
        if( !replacement.empty() ) {
            phrase.replace( fa, l, replacement );
            continue;
        }

        // Special, dynamic tags go here
        if( tag == "<yrwp>" ) {
            phrase.replace( fa, l, remove_color_tags( u.weapon.tname() ) );
        } else if( tag == "<mywp>" ) {
            if( !me.is_armed() ) {
                phrase.replace( fa, l, _( "fists" ) );
            } else {
                phrase.replace( fa, l, remove_color_tags( me.weapon.tname() ) );
            }
        } else if( tag == "<ammo>" ) {
            if( !me.weapon.is_gun() ) {
                phrase.replace( fa, l, _( "BADAMMO" ) );
            } else {
                phrase.replace( fa, l, me.weapon.ammo_type()->name() );
            }
        } else if( tag == "<punc>" ) {
            switch( rng( 0, 2 ) ) {
                case 0:
                    phrase.replace( fa, l, pgettext( "punctuation", "." ) );
                    break;
                case 1:
                    phrase.replace( fa, l, pgettext( "punctuation", "..." ) );
                    break;
                case 2:
                    phrase.replace( fa, l, pgettext( "punctuation", "!" ) );
                    break;
            }
        } else if( !tag.empty() ) {
            debugmsg( "Bad tag. '%s' (%d - %d)", tag.c_str(), fa, fb );
            phrase.replace( fa, fb - fa + 1, "????" );
        }
    } while( fa != std::string::npos && fb != std::string::npos );
}

void dialogue::clear_window_texts()
{
    // Note: don't erase the borders, therefore start and end one unit inwards.
    // Note: start at second line because the first line contains the headers which are not
    // reprinted.
    // TODO: make this call werase and reprint the border & the header
    for( int i = 2; i < FULL_SCREEN_HEIGHT - 1; i++ ) {
        for( int j = 1; j < FULL_SCREEN_WIDTH - 1; j++ ) {
            if( j != ( FULL_SCREEN_WIDTH / 2 ) + 1 ) {
                mvwputch( win, i, j, c_black, ' ' );
            }
        }
    }
}

size_t dialogue::add_to_history( const std::string &text )
{
    auto const folded = foldstring( text, FULL_SCREEN_WIDTH / 2 );
    history.insert( history.end(), folded.begin(), folded.end() );
    return folded.size();
}

void dialogue::print_history( size_t const hilight_lines )
{
    int curline = FULL_SCREEN_HEIGHT - 2;
    int curindex = history.size() - 1;
    // index of the first line that is highlighted
    int newindex = history.size() - hilight_lines;
    // Print at line 2 and below, line 1 contains the header, line 0 the border
    while( curindex >= 0 && curline >= 2 ) {
        // red for new text, gray for old, similar to coloring of messages
        nc_color const col = ( curindex >= newindex ) ? c_red : c_dark_gray;
        mvwprintz( win, curline, 1, col, history[curindex] );
        curline--;
        curindex--;
    }
}

// Number of lines that can be used for the list of responses:
// -2 for border, -2 for options that are always there, -1 for header
static int RESPONSE_AREA_HEIGHT()
{
    return FULL_SCREEN_HEIGHT - 2 - 2 - 1;
}

bool dialogue::print_responses( int const yoffset )
{
    // Responses go on the right side of the window, add 2 for spacing
    size_t const xoffset = FULL_SCREEN_WIDTH / 2 + 2;
    // First line we can print to, +2 for borders, +1 for the header.
    int const min_line = 2 + 1;
    // Bottom most line we can print to
    int const max_line = min_line + RESPONSE_AREA_HEIGHT() - 1;

    int curline = min_line - ( int ) yoffset;
    size_t i;
    for( i = 0; i < responses.size() && curline <= max_line; i++ ) {
        auto const &folded = responses[i].formatted_text;
        auto const &color = responses[i].color;
        for( size_t j = 0; j < folded.size(); j++, curline++ ) {
            if( curline < min_line ) {
                continue;
            } else if( curline > max_line ) {
                break;
            }
            int const off = ( j != 0 ) ? +3 : 0;
            mvwprintz( win, curline, xoffset + off, color, folded[j] );
        }
    }
    // Those are always available, their key bindings are fixed as well.
    mvwprintz( win, curline + 1, xoffset, c_magenta, _( "Shift+L: Look at" ) );
    mvwprintz( win, curline + 2, xoffset, c_magenta, _( "Shift+S: Size up stats" ) );
    mvwprintz( win, curline + 3, xoffset, c_magenta, _( "Shift+Y: Yell" ) );
    mvwprintz( win, curline + 4, xoffset, c_magenta, _( "Shift+O: Check opinion" ) );
    return curline > max_line; // whether there is more to print.
}

int dialogue::choose_response( int const hilight_lines )
{
    int yoffset = 0;
    while( true ) {
        clear_window_texts();
        print_history( hilight_lines );
        bool const can_sroll_down = print_responses( yoffset );
        bool const can_sroll_up = yoffset > 0;
        if( can_sroll_up ) {
            mvwprintz( win, 2, FULL_SCREEN_WIDTH - 2 - 2, c_green, "^^" );
        }
        if( can_sroll_down ) {
            mvwprintz( win, FULL_SCREEN_HEIGHT - 2, FULL_SCREEN_WIDTH - 2 - 2, c_green, "vv" );
        }
        wrefresh( win );
        // TODO: input_context?
        const long ch = inp_mngr.get_input_event().get_first_input();
        switch( ch ) {
            case KEY_DOWN:
            case KEY_NPAGE:
                if( can_sroll_down ) {
                    yoffset += RESPONSE_AREA_HEIGHT();
                }
                break;
            case KEY_UP:
            case KEY_PPAGE:
                if( can_sroll_up ) {
                    yoffset = std::max( 0, yoffset - RESPONSE_AREA_HEIGHT() );
                }
                break;
            default:
                return ch;
        }
    }
}

void dialogue::add_topic( const std::string &topic_id )
{
    topic_stack.push_back( talk_topic( topic_id ) );
}

void dialogue::add_topic( const talk_topic &topic )
{
    topic_stack.push_back( topic );
}


void talk_response::do_formatting( const dialogue &d, char const letter )
{
    std::string ftext;
    if( trial != TALK_TRIAL_NONE ) { // dialogue w/ a % chance to work
        ftext = string_format( pgettext( "talk option",
                                         "%1$c: [%2$s %3$d%%] %4$s" ),
                               letter,                         // option letter
                               trial.name().c_str(),     // trial type
                               trial.calc_chance( d ), // trial % chance
                               text.c_str()                // response
                             );
    } else { // regular dialogue
        ftext = string_format( pgettext( "talk option",
                                         "%1$c: %2$s" ),
                               letter,          // option letter
                               text.c_str() // response
                             );
    }
    parse_tags( ftext, *d.alpha, *d.beta );
    // Remaining width of the responses area, -2 for the border, -2 for indentation
    int const fold_width = FULL_SCREEN_WIDTH / 2 - 2 - 2;
    formatted_text = foldstring( ftext, fold_width );

    std::set<dialogue_consequence> consequences = get_consequences( d );
    if( consequences.count( dialogue_consequence::hostile ) > 0 ) {
        color = c_red;
    } else if( text[0] == '*' || consequences.count( dialogue_consequence::helpless ) > 0 ) {
        color = c_light_red;
    } else if( text[0] == '&' || consequences.count( dialogue_consequence::action ) > 0 ) {
        color = c_green;
    } else {
        color = c_white;
    }
}

talk_topic talk_response::effect_t::apply( dialogue &d ) const
{
    effect( *d.beta );
    d.beta->op_of_u += opinion;
    if( d.beta->turned_hostile() ) {
        d.beta->make_angry();
        return talk_topic( "TALK_DONE" );
    }

    // TODO: this is a hack, it should be in clear_mission or so, but those functions have
    // no access to the dialogue object.
    auto &ma = d.missions_assigned;
    ma.clear();
    // Update the missions we can talk about (must only be current, non-complete ones)
    for( auto &mission : d.beta->chatbin.missions_assigned ) {
        if( mission->get_assigned_player_id() == d.alpha->getID() ) {
            ma.push_back( mission );
        }
    }

    return next_topic;
}

void talk_response::effect_t::set_effect_consequence( std::function<void ( npc & )> fun,
        dialogue_consequence con )
{
    effect = fun;
    guaranteed_consequence = con;
}

void talk_response::effect_t::set_effect( dialogue_fun_ptr ptr )
{
    effect = ptr;
    // Kinda hacky
    if( ptr == &talk_function::hostile ) {
        guaranteed_consequence = dialogue_consequence::hostile;
    } else if( ptr == &talk_function::player_weapon_drop || ptr == &talk_function::player_weapon_away ||
               ptr == &talk_function::start_mugging ) {
        guaranteed_consequence = dialogue_consequence::helpless;
    } else {
        guaranteed_consequence = dialogue_consequence::none;
    }
}

std::set<dialogue_consequence> talk_response::get_consequences( const dialogue &d ) const
{
    int chance = trial.calc_chance( d );
    if( chance >= 100 ) {
        return { success.get_consequence( d ) };
    } else if( chance <= 0 ) {
        return { failure.get_consequence( d ) };
    }

    return {{ success.get_consequence( d ), failure.get_consequence( d ) }};
}

dialogue_consequence talk_response::effect_t::get_consequence( const dialogue &d ) const
{
    if( d.beta->op_of_u.anger + opinion.anger >= d.beta->hostile_anger_level() ) {
        return dialogue_consequence::hostile;
    }
    return guaranteed_consequence;
}

talk_topic dialogue::opt( const talk_topic &topic )
{
    std::string challenge = dynamic_line( topic );
    gen_responses( topic );
    // Put quotes around challenge (unless it's an action)
    if( challenge[0] != '*' && challenge[0] != '&' ) {
        std::stringstream tmp;
        tmp << "\"" << challenge << "\"";
    }

    // Parse any tags in challenge
    parse_tags( challenge, *alpha, *beta );
    capitalize_letter( challenge );

    // Prepend "My Name: "
    if( challenge[0] == '&' ) {
        // No name prepended!
        challenge = challenge.substr( 1 );
    } else if( challenge[0] == '*' ) {
        challenge = string_format( pgettext( "npc does something", "%s %s" ), beta->name.c_str(),
                                   challenge.substr( 1 ).c_str() );
    } else {
        challenge = string_format( pgettext( "npc says something", "%s: %s" ), beta->name.c_str(),
                                   challenge.c_str() );
    }

    history.push_back( "" ); // Empty line between lines of dialogue

    // Number of lines to highlight
    size_t const hilight_lines = add_to_history( challenge );
    for( size_t i = 0; i < responses.size(); i++ ) {
        responses[i].do_formatting( *this, 'a' + i );
    }

    int ch;
    bool okay;
    do {
        do {
            ch = choose_response( hilight_lines );
            auto st = special_talk( ch );
            if( st.id != "TALK_NONE" ) {
                return st;
            }
            ch -= 'a';
        } while( ( ch < 0 || ch >= ( int )responses.size() ) );
        okay = true;
        std::set<dialogue_consequence> consequences = responses[ch].get_consequences( *this );
        if( consequences.count( dialogue_consequence::hostile ) > 0 ) {
            okay = query_yn( _( "You may be attacked! Proceed?" ) );
        } else if( consequences.count( dialogue_consequence::helpless ) > 0 ) {
            okay = query_yn( _( "You'll be helpless! Proceed?" ) );
        }
    } while( !okay );
    history.push_back( "" );

    std::string response_printed = string_format( pgettext( "you say something", "You: %s" ),
                                   responses[ch].text.c_str() );
    add_to_history( response_printed );

    talk_response chosen = responses[ch];
    if( chosen.mission_selected != nullptr ) {
        beta->chatbin.mission_selected = chosen.mission_selected;
    }

    // We can't set both skill and style or training will bug out
    // @todo: Allow setting both skill and style
    if( chosen.skill ) {
        beta->chatbin.skill = chosen.skill;
        beta->chatbin.style = matype_id::NULL_ID();
    } else if( chosen.style ) {
        beta->chatbin.style = chosen.style;
        beta->chatbin.skill = skill_id::NULL_ID();
    }

    const bool success = chosen.trial.roll( *this );
    const auto &effects = success ? chosen.success : chosen.failure;
    return effects.apply( *this );
}

const talk_topic &special_talk( char ch )
{
    static const std::map<char, talk_topic> key_map = {{
            { 'L', talk_topic( "TALK_LOOK_AT" ) },
            { 'S', talk_topic( "TALK_SIZE_UP" ) },
            { 'O', talk_topic( "TALK_OPINION" ) },
            { 'Y', talk_topic( "TALK_SHOUT" ) },
        }
    };

    const auto iter = key_map.find( ch );
    if( iter != key_map.end() ) {
        return iter->second;
    }

    static const talk_topic no_topic = talk_topic( "TALK_NONE" );
    return no_topic;
}

// Creates a new inventory that contains `added` items, but not `without` ones
// `without` should point to items in `inv`
inventory inventory_exchange( inventory &inv,
                              const std::set<item *> &without, const std::vector<item *> &added )
{
    std::vector<item *> item_dump;
    inv.dump( item_dump );
    item_dump.insert( item_dump.end(), added.begin(), added.end() );
    inventory new_inv;
    new_inv.copy_invlet_of( inv );

    for( item *it : item_dump ) {
        if( without.count( it ) == 0 ) {
            new_inv.add_item( *it, true, false );
        }
    }

    return new_inv;
}

std::vector<item_pricing> init_selling( npc &p )
{
    std::vector<item_pricing> result;
    invslice slice = p.inv.slice();
    for( auto &i : slice ) {
        auto &it = i->front();

        const int price = it.price( true );
        int val = p.value( it );
        if( p.wants_to_sell( it, val, price ) ) {
            result.emplace_back( p, &i->front(), val, false );
        }
    }

    if( p.is_friend() & !p.weapon.is_null() && !p.weapon.has_flag( "NO_UNWIELD" ) ) {
        result.emplace_back( p, &p.weapon, p.value( p.weapon ), false );
    }

    return result;
}

template <typename T, typename Callback>
void buy_helper( T &src, Callback cb )
{
    src.visit_items( [&src, &cb]( item * node ) {
        cb( std::move( item_location( src, node ) ) );

        return VisitResponse::SKIP;
    } );
}

std::vector<item_pricing> init_buying( npc &p, player &u )
{
    std::vector<item_pricing> result;

    const auto check_item = [&p, &result]( item_location && loc ) {
        item *it_ptr = loc.get_item();
        if( it_ptr == nullptr || it_ptr->is_null() ) {
            return;
        }

        auto &it = *it_ptr;
        int market_price = it.price( true );
        int val = p.value( it, market_price );
        if( p.wants_to_buy( it, val, market_price ) ) {
            result.emplace_back( std::move( loc ), val, false );
        }
    };

    invslice slice = u.inv.slice();
    for( auto &i : slice ) {
        // @todo: Sane way of handling multi-item stacks
        check_item( item_location( u, &i->front() ) );
    }

    if( !u.weapon.has_flag( "NO_UNWIELD" ) ) {
        check_item( item_location( u, &u.weapon ) );
    }

    for( auto &cursor : map_selector( u.pos(), 1 ) ) {
        buy_helper( cursor, check_item );
    }
    for( auto &cursor : vehicle_selector( u.pos(), 1 ) ) {
        buy_helper( cursor, check_item );
    }

    return result;
}

bool trade( npc &p, int cost, const std::string &deal )
{
    catacurses::window w_head = catacurses::newwin( 4, TERMX, 0, 0 );
    const int win_they_w = TERMX / 2;
    catacurses::window w_them = catacurses::newwin( TERMY - 4, win_they_w, 4, 0 );
    catacurses::window w_you = catacurses::newwin( TERMY - 4, TERMX - win_they_w, 4, win_they_w );
    catacurses::window w_tmp;
    std::string header_message = _( "\
TAB key to switch lists, letters to pick items, Enter to finalize, Esc to quit,\n\
? to get information on an item." );
    mvwprintz( w_head, 0, 0, c_white, header_message.c_str(), p.name.c_str() );

    // If entries were to get over a-z and A-Z, we wouldn't have good keys for them
    const size_t entries_per_page = std::min( TERMY - 7, 2 + ( 'z' - 'a' ) + ( 'Z' - 'A' ) );

    // Set up line drawings
    for( int i = 0; i < TERMX; i++ ) {
        mvwputch( w_head, 3, i, c_white, LINE_OXOX );
    }
    wrefresh( w_head );
    // End of line drawings

    // Populate the list of what the NPC is willing to buy, and the prices they pay
    // Note that the NPC's barter skill is factored into these prices.
    // TODO: Recalc item values every time a new item is selected
    // Trading is not linear - starving NPC may pay $100 for 3 jerky, but not $100000 for 300 jerky
    std::vector<item_pricing> theirs = init_selling( p );
    std::vector<item_pricing> yours = init_buying( p, g->u );

    // Adjust the prices based on your barter skill.
    // cap adjustment so nothing is ever sold below value
    ///\EFFECT_INT_NPC slightly increases bartering price changes, relative to your INT

    ///\EFFECT_BARTER_NPC increases bartering price changes, relative to your BARTER
    double their_adjust = ( price_adjustment( p.get_skill_level( skill_barter ) - g->u.get_skill_level(
                                skill_barter ) ) +
                            ( p.int_cur - g->u.int_cur ) / 20.0 );
    if( their_adjust < 1.0 ) {
        their_adjust = 1.0;
    }
    for( item_pricing &p : theirs ) {
        p.price *= their_adjust;
    }
    ///\EFFECT_INT slightly increases bartering price changes, relative to NPC INT

    ///\EFFECT_BARTER increases bartering price changes, relative to NPC BARTER
    double your_adjust = ( price_adjustment( g->u.get_skill_level( skill_barter ) - p.get_skill_level(
                               skill_barter ) ) +
                           ( g->u.int_cur - p.int_cur ) / 20.0 );
    if( your_adjust < 1.0 ) {
        your_adjust = 1.0;
    }
    for( item_pricing &p : yours ) {
        p.price *= your_adjust;
    }

    // Just exchanging items, no barter involved
    const bool ex = p.is_friend();

    // How much cash you get in the deal (negative = losing money)
    long cash = cost + p.op_of_u.owed;
    bool focus_them = true; // Is the focus on them?
    bool update = true;     // Re-draw the screen?
    size_t them_off = 0, you_off = 0; // Offset from the start of the list
    size_t ch, help;

    if( ex ) {
        // Sometimes owed money fails to reset for friends
        // NPC AI is way too weak to manage money, so let's just make them give stuff away for free
        cash = 0;
    }

    // Make a temporary copy of the NPC to make sure volume calculations are correct
    npc temp = p;
    units::volume volume_left = temp.volume_capacity() - temp.volume_carried();
    units::mass weight_left = temp.weight_capacity() - temp.weight_carried();

    do {
        auto &target_list = focus_them ? theirs : yours;
        auto &offset = focus_them ? them_off : you_off;
        if( update ) { // Time to re-draw
            update = false;
            // Draw borders, one of which is highlighted
            werase( w_them );
            werase( w_you );
            for( int i = 1; i < TERMX; i++ ) {
                mvwputch( w_head, 3, i, c_white, LINE_OXOX );
            }

            std::set<item *> without;
            std::vector<item *> added;

            for( auto &pricing : yours ) {
                if( pricing.selected ) {
                    added.push_back( pricing.loc.get_item() );
                }
            }

            for( auto &pricing : theirs ) {
                if( pricing.selected ) {
                    without.insert( pricing.loc.get_item() );
                }
            }
            temp.inv = inventory_exchange( p.inv, without, added );

            volume_left = temp.volume_capacity() - temp.volume_carried();
            weight_left = temp.weight_capacity() - temp.weight_carried();
            mvwprintz( w_head, 3, 2, ( volume_left < 0 || weight_left < 0 ) ? c_red : c_green,
                       _( "Volume: %s %s, Weight: %.1f %s" ),
                       format_volume( volume_left ).c_str(), volume_units_abbr(),
                       convert_weight( weight_left ), weight_units() );

            std::string cost_string = ex ? _( "Exchange" ) : ( cash >= 0 ? _( "Profit %s" ) :
                                      _( "Cost %s" ) );
            mvwprintz( w_head, 3, TERMX / 2 + ( TERMX / 2 - cost_string.length() ) / 2,
                       ( cash < 0 && ( int )g->u.cash >= cash * -1 ) || ( cash >= 0 &&
                               ( int )p.cash  >= cash ) ? c_green : c_red,
                       cost_string.c_str(), format_money( std::abs( cash ) ) );

            if( !deal.empty() ) {
                mvwprintz( w_head, 3, ( TERMX - deal.length() ) / 2, cost < 0 ? c_light_red : c_light_green,
                           deal.c_str() );
            }
            draw_border( w_them, ( focus_them ? c_yellow : BORDER_COLOR ) );
            draw_border( w_you, ( !focus_them ? c_yellow : BORDER_COLOR ) );

            mvwprintz( w_them, 0, 2, ( cash < 0 || ( int )p.cash >= cash ? c_green : c_red ),
                       _( "%s: %s" ), p.name.c_str(), format_money( p.cash ) );
            mvwprintz( w_you,  0, 2, ( cash > 0 || ( int )g->u.cash >= cash * -1 ? c_green : c_red ),
                       _( "You: %s" ), format_money( g->u.cash ) );
            // Draw lists of items, starting from offset
            for( size_t whose = 0; whose <= 1; whose++ ) {
                const bool they = whose == 0;
                const auto &list = they ? theirs : yours;
                const auto &offset = they ? them_off : you_off;
                const auto &person = they ? p : g->u;
                auto &w_whose = they ? w_them : w_you;
                int win_h = getmaxy( w_whose );
                int win_w = getmaxx( w_whose );
                // Borders
                win_h -= 2;
                win_w -= 2;
                for( size_t i = offset; i < list.size() && i < entries_per_page + offset; i++ ) {
                    const item_pricing &ip = list[i];
                    const item *it = ip.loc.get_item();
                    auto color = it == &person.weapon ? c_yellow : c_light_gray;
                    std::string itname = it->display_name();
                    if( ip.loc.where() != item_location::type::character ) {
                        itname = itname + " " + ip.loc.describe( &g->u );
                        color = c_light_blue;
                    }

                    if( ip.selected ) {
                        color = c_white;
                    }

                    int keychar = i - offset + 'a';
                    if( keychar > 'z' ) {
                        keychar = keychar - 'z' - 1 + 'A';
                    }
                    trim_and_print( w_whose, i - offset + 1, 1, win_w, color, "%c %c %s",
                                    ( char )keychar, ip.selected ? '+' : '-', itname.c_str() );

                    std::string price_str = string_format( "%.2f", ip.price / 100.0 );
                    nc_color price_color = ex ? c_dark_gray : ( ip.selected ? c_white : c_light_gray );
                    mvwprintz( w_whose, i - offset + 1, win_w - price_str.length(),
                               price_color, price_str.c_str() );
                }
                if( offset > 0 ) {
                    mvwprintw( w_whose, entries_per_page + 2, 1, _( "< Back" ) );
                }
                if( offset + entries_per_page < list.size() ) {
                    mvwprintw( w_whose, entries_per_page + 2, 9, _( "More >" ) );
                }
            }
            wrefresh( w_head );
            wrefresh( w_them );
            wrefresh( w_you );
        } // Done updating the screen
        // TODO: use input context
        ch = inp_mngr.get_input_event().get_first_input();
        switch( ch ) {
            case '\t':
                focus_them = !focus_them;
                update = true;
                break;
            case '<':
                if( offset > 0 ) {
                    offset -= entries_per_page;
                    update = true;
                }
                break;
            case '>':
                if( offset + entries_per_page < target_list.size() ) {
                    offset += entries_per_page;
                    update = true;
                }
                break;
            case '?':
                update = true;
                w_tmp = catacurses::newwin( 3, 21, 1 + ( TERMY - FULL_SCREEN_HEIGHT ) / 2,
                                            30 + ( TERMX - FULL_SCREEN_WIDTH ) / 2 );
                mvwprintz( w_tmp, 1, 1, c_red, _( "Examine which item?" ) );
                draw_border( w_tmp );
                wrefresh( w_tmp );
                // TODO: use input context
                help = inp_mngr.get_input_event().get_first_input() - 'a';
                mvwprintz( w_head, 0, 0, c_white, header_message.c_str(), p.name.c_str() );
                wrefresh( w_head );
                help += offset;
                if( help < target_list.size() ) {
                    popup( target_list[help].loc.get_item()->info(), PF_NONE );
                }
                break;
            case '\n': // Check if we have enough cash...
                // The player must pay cash, and it should not put the player negative.
                if( cash < 0 && ( int )g->u.cash < cash * -1 ) {
                    popup( _( "Not enough cash!  You have %s, price is %s." ), format_money( g->u.cash ),
                           format_money( -cash ) );
                    update = true;
                    ch = ' ';
                } else if( volume_left < 0 || weight_left < 0 ) {
                    // Make sure NPC doesn't go over allowed volume
                    popup( _( "%s can't carry all that." ), p.name.c_str() );
                    update = true;
                    ch = ' ';
                }
                break;
            default: // Letters & such
                if( ch >= 'a' && ch <= 'z' ) {
                    ch -= 'a';
                } else if( ch >= 'A' && ch <= 'Z' ) {
                    ch = ch - 'A' + ( 'z' - 'a' ) + 1;
                } else {
                    continue;
                }

                ch += offset;
                if( ch < target_list.size() ) {
                    update = true;
                    item_pricing &ip = target_list[ch];
                    ip.selected = !ip.selected;
                    if( !ex && ip.selected == focus_them ) {
                        cash -= ip.price;
                    } else if( !ex ) {
                        cash += ip.price;
                    }
                }
                ch = 0;
        }
    } while( ch != KEY_ESCAPE && ch != '\n' );

    const bool traded = ch == '\n';
    if( traded ) {
        int practice = 0;

        std::list<item_location *> from_map;
        const auto mark_for_exchange =
            [&practice, &from_map]( item_pricing & pricing, std::set<item *> &removing,
        std::vector<item *> &giving ) {
            if( !pricing.selected ) {
                return;
            }

            giving.push_back( pricing.loc.get_item() );
            practice++;

            if( pricing.loc.where() == item_location::type::character ) {
                removing.insert( pricing.loc.get_item() );
            } else {
                from_map.push_back( &pricing.loc );
            }
        };
        // This weird exchange is needed to prevent pointer bugs
        // Removing items from an inventory invalidates the pointers
        std::set<item *> removing_yours;
        std::vector<item *> giving_them;

        for( auto &pricing : yours ) {
            mark_for_exchange( pricing, removing_yours, giving_them );
        }

        std::set<item *> removing_theirs;
        std::vector<item *> giving_you;
        for( auto &pricing : theirs ) {
            mark_for_exchange( pricing, removing_theirs, giving_you );
        }

        const inventory &your_new_inv = inventory_exchange( g->u.inv,
                                        removing_yours, giving_you );
        const inventory &their_new_inv = inventory_exchange( p.inv,
                                         removing_theirs, giving_them );

        g->u.inv = your_new_inv;
        p.inv = their_new_inv;

        if( removing_yours.count( &g->u.weapon ) ) {
            g->u.remove_weapon();
        }

        if( removing_theirs.count( &p.weapon ) ) {
            p.remove_weapon();
        }

        for( item_location *loc_ptr : from_map ) {
            loc_ptr->remove_item();
        }

        if( !ex && cash > ( int )p.cash ) {
            // Trade was forced, give the NPC's cash to the player.
            p.op_of_u.owed = ( cash - p.cash );
            g->u.cash += p.cash;
            p.cash = 0;
        } else if( !ex ) {
            g->u.cash += cash;
            p.cash -= cash;
        }

        // TODO: Make this depend on prices
        // TODO: Make this depend on npc price adjustment vs. your price adjustment
        if( !ex ) {
            g->u.practice( skill_barter, practice / 2 );
        }
    }
    g->refresh_all();
    return traded;
}

talk_trial::talk_trial( JsonObject jo )
{
    static const std::unordered_map<std::string, talk_trial_type> types_map = { {
#define WRAP(value) { #value, TALK_TRIAL_##value }
            WRAP( NONE ),
            WRAP( LIE ),
            WRAP( PERSUADE ),
            WRAP( INTIMIDATE )
#undef WRAP
        }
    };
    auto const iter = types_map.find( jo.get_string( "type", "NONE" ) );
    if( iter == types_map.end() ) {
        jo.throw_error( "invalid talk trial type", "type" );
    }
    type = iter->second;
    if( type != TALK_TRIAL_NONE ) {
        difficulty = jo.get_int( "difficulty" );
    }
}

talk_topic load_inline_topic( JsonObject jo )
{
    const std::string id = jo.get_string( "id" );
    json_talk_topics[id].load( jo );
    return talk_topic( id );
}

talk_response::effect_t::effect_t( JsonObject jo )
{
    load_effect( jo );
    if( jo.has_object( "topic" ) ) {
        next_topic = load_inline_topic( jo.get_object( "topic" ) );
    } else {
        next_topic = talk_topic( jo.get_string( "topic" ) );
    }
    if( jo.has_member( "opinion" ) ) {
        JsonIn *ji = jo.get_raw( "opinion" );
        // Same format as when saving a game (-:
        opinion.deserialize( *ji );
    }
}

void talk_response::effect_t::load_effect( JsonObject &jo )
{
    static const std::string member_name( "effect" );
    if( !jo.has_member( member_name ) ) {
        return;
    } else if( jo.has_string( member_name ) ) {
        const std::string type = jo.get_string( member_name );
        static const std::unordered_map<std::string, void( * )( npc & )> static_functions_map = { {
#define WRAP( function ) { #function, &talk_function::function }
                WRAP( start_trade ),
                WRAP( hostile ),
                WRAP( leave ),
                WRAP( flee ),
                WRAP( follow ),
                WRAP( stop_guard ),
                WRAP( assign_guard ),
                WRAP( end_conversation ),
                WRAP( insult_combat ),
                WRAP( drop_weapon ),
                WRAP( player_weapon_away ),
                WRAP( player_weapon_drop )
#undef WRAP
            }
        };
        const auto iter = static_functions_map.find( type );
        if( iter != static_functions_map.end() ) {
            set_effect( iter->second );
            return;
        }
        // more functions can be added here, they don't need to be in the map above.
        {
            jo.throw_error( "unknown effect type", member_name );
        }
    } else {
        jo.throw_error( "invalid effect syntax", member_name );
    }
}

talk_response::talk_response( JsonObject jo )
{
    if( jo.has_member( "trial" ) ) {
        trial = talk_trial( jo.get_object( "trial" ) );
    }
    if( jo.has_member( "success" ) ) {
        success = effect_t( jo.get_object( "success" ) );
    } else if( jo.has_string( "topic" ) ) {
        // This is for simple topic switching without a possible failure
        success.next_topic = talk_topic( jo.get_string( "topic" ) );
        success.load_effect( jo );
    } else if( jo.has_object( "topic" ) ) {
        success.next_topic = load_inline_topic( jo.get_object( "topic" ) );
    }
    if( trial && !jo.has_member( "failure" ) ) {
        jo.throw_error( "the failure effect is mandatory if a talk_trial has been defined" );
    }
    if( jo.has_member( "failure" ) ) {
        failure = effect_t( jo.get_object( "failure" ) );
    }
    text = _( jo.get_string( "text" ).c_str() );
    // TODO: mission_selected
    // TODO: skill
    // TODO: style
}

json_talk_response::json_talk_response( JsonObject jo )
    : actual_response( jo )
{
    load_condition( jo );
}

void json_talk_response::load_condition( JsonObject &jo )
{
    static const std::string member_name( "condition" );
    if( !jo.has_member( member_name ) ) {
        // Leave condition unset, defaults to true.
        return;
    } else if( jo.has_string( member_name ) ) {
        const std::string type = jo.get_string( member_name );
        if( type == "has_assigned_mission" ) {
            condition = []( const dialogue & d ) {
                return d.missions_assigned.size() == 1;
            };
        } else if( type == "has_many_assigned_missions" ) {
            condition = []( const dialogue & d ) {
                return d.missions_assigned.size() >= 2;
            };
        } else {
            jo.throw_error( "unknown condition type", member_name );
        }
    } else {
        jo.throw_error( "invalid condition syntax", member_name );
    }
}

bool json_talk_response::test_condition( const dialogue &d ) const
{
    if( condition ) {
        return condition( d );
    }
    return true;
}

void json_talk_response::gen_responses( dialogue &d ) const
{
    if( test_condition( d ) ) {
        d.responses.emplace_back( actual_response );
    }
}

dynamic_line_t dynamic_line_t::from_member( JsonObject &jo, const std::string &member_name )
{
    if( jo.has_array( member_name ) ) {
        return dynamic_line_t( jo.get_array( member_name ) );
    } else if( jo.has_object( member_name ) ) {
        return dynamic_line_t( jo.get_object( member_name ) );
    } else if( jo.has_string( member_name ) ) {
        return dynamic_line_t( jo.get_string( member_name ) );
    } else {
        return dynamic_line_t{};
    }
}

dynamic_line_t::dynamic_line_t( const std::string &line )
{
    function = [line]( const dialogue & ) {
        return _( line.c_str() );
    };
}

dynamic_line_t::dynamic_line_t( JsonObject jo )
{
    if( jo.has_member( "u_male" ) && jo.has_member( "u_female" ) ) {
        const dynamic_line_t u_male = from_member( jo, "u_male" );
        const dynamic_line_t u_female = from_member( jo, "u_female" );
        function = [u_male, u_female]( const dialogue & d ) {
            return ( d.alpha->male ? u_male : u_female )( d );
        };
    } else if( jo.has_member( "npc_male" ) && jo.has_member( "npc_female" ) ) {
        const dynamic_line_t npc_male = from_member( jo, "npc_male" );
        const dynamic_line_t npc_female = from_member( jo, "npc_female" );
        function = [npc_male, npc_female]( const dialogue & d ) {
            return ( d.beta->male ? npc_male : npc_female )( d );
        };
    } else if( jo.has_member( "u_is_wearing" ) ) {
        const std::string item_id = jo.get_string( "u_is_wearing" );
        const dynamic_line_t yes = from_member( jo, "yes" );
        const dynamic_line_t no = from_member( jo, "no" );
        function = [item_id, yes, no]( const dialogue & d ) {
            const bool wearing = d.alpha->is_wearing( item_id );
            return ( wearing ? yes : no )( d );
        };
    } else if( jo.has_member( "u_has_any_trait" ) ) {
        std::vector<trait_id> traits_to_check;
        for( auto &&f : jo.get_string_array( "u_has_any_trait" ) ) {
            traits_to_check.emplace_back( f );
        }
        const dynamic_line_t yes = from_member( jo, "yes" );
        const dynamic_line_t no = from_member( jo, "no" );
        function = [traits_to_check, yes, no]( const dialogue & d ) {
            for( const auto &trait : traits_to_check ) {
                if( d.alpha->has_trait( trait ) ) {
                    return yes( d );
                }
            }
            return no( d );
        };
    } else {
        jo.throw_error( "no supported" );
    }
}

dynamic_line_t::dynamic_line_t( JsonArray ja )
{
    std::vector<dynamic_line_t> lines;
    while( ja.has_more() ) {
        if( ja.test_string() ) {
            lines.emplace_back( ja.next_string() );
        } else if( ja.test_array() ) {
            lines.emplace_back( ja.next_array() );
        } else if( ja.test_object() ) {
            lines.emplace_back( ja.next_object() );
        } else {
            ja.throw_error( "invalid format: must be string, array or object" );
        }
    }
    function = [lines]( const dialogue & d ) {
        const dynamic_line_t &line = random_entry_ref( lines );
        return line( d );
    };
}

void json_talk_topic::load( JsonObject &jo )
{
    if( jo.has_member( "dynamic_line" ) ) {
        dynamic_line = dynamic_line_t::from_member( jo, "dynamic_line" );
    }
    JsonArray ja = jo.get_array( "responses" );
    responses.reserve( responses.size() + ja.size() );
    while( ja.has_more() ) {
        responses.emplace_back( ja.next_object() );
    }
    if( responses.empty() ) {
        jo.throw_error( "no responses for talk topic defined", "responses" );
    }
    replace_built_in_responses = jo.get_bool( "replace_built_in_responses",
                                 replace_built_in_responses );
}

bool json_talk_topic::gen_responses( dialogue &d ) const
{
    d.responses.reserve( responses.size() ); // A wild guess, can actually be more or less
    for( auto &r : responses ) {
        r.gen_responses( d );
    }
    return replace_built_in_responses;
}

std::string json_talk_topic::get_dynamic_line( const dialogue &d ) const
{
    return dynamic_line( d );
}

void json_talk_topic::check_consistency() const
{
    // TODO: check that all referenced topic actually exist. This is currently not possible
    // as they only exist as built in strings, not in the json_talk_topics map.
}

void unload_talk_topics()
{
    json_talk_topics.clear();
}

void load_talk_topic( JsonObject &jo )
{
    if( jo.has_array( "id" ) ) {
        for( auto &id : jo.get_string_array( "id" ) ) {
            json_talk_topics[id].load( jo );
        }
    } else {
        const std::string id = jo.get_string( "id" );
        json_talk_topics[id].load( jo );
    }
}

std::string npc::pick_talk_topic( const player &u )
{
    //form_opinion(u);
    ( void )u;
    if( personality.aggression > 0 ) {
        if( op_of_u.fear * 2 < personality.bravery && personality.altruism < 0 ) {
            return "TALK_MUG";
        }

        if( personality.aggression + personality.bravery - op_of_u.fear > 0 ) {
            return "TALK_STRANGER_AGGRESSIVE";
        }
    }

    if( op_of_u.fear * 2 > personality.altruism + personality.bravery ) {
        return "TALK_STRANGER_SCARED";
    }

    if( op_of_u.fear * 2 > personality.bravery + op_of_u.trust ) {
        return "TALK_STRANGER_WARY";
    }

    if( op_of_u.trust - op_of_u.fear +
        ( personality.bravery + personality.altruism ) / 2 > 0 ) {
        return "TALK_STRANGER_FRIENDLY";
    }

    return "TALK_STRANGER_NEUTRAL";
}

enum consumption_result {
    REFUSED = 0,
    CONSUMED_SOME, // Consumption didn't fail, but don't delete the item
    CONSUMED_ALL   // Consumption succeeded, delete the item
};

// Returns true if we destroyed the item through consumption
consumption_result try_consume( npc &p, item &it, std::string &reason )
{
    // @todo: Unify this with 'player::consume_item()'
    bool consuming_contents = it.is_container() && !it.contents.empty();
    item &to_eat = consuming_contents ? it.contents.front() : it;
    const auto &comest = to_eat.type->comestible;
    if( !comest ) {
        // Don't inform the player that we don't want to eat the lighter
        return REFUSED;
    }

    if( !p.will_accept_from_player( it ) ) {
        reason = _( "I don't <swear> trust you enough to eat THIS..." );
        return REFUSED;
    }

    // TODO: Make it not a copy+paste from player::consume_item
    int amount_used = 1;
    if( to_eat.is_food() ) {
        if( !p.eat( to_eat ) ) {
            reason = _( "It doesn't look like a good idea to consume this..." );
            return REFUSED;
        }
    } else if( to_eat.is_medication() || to_eat.get_contained().is_medication() ) {
        if( comest->tool != "null" ) {
            bool has = p.has_amount( comest->tool, 1 );
            if( item::count_by_charges( comest->tool ) ) {
                has = p.has_charges( comest->tool, 1 );
            }
            if( !has ) {
                reason = string_format( _( "I need a %s to consume that!" ),
                                        item::nname( comest->tool ).c_str() );
                return REFUSED;
            }
            p.use_charges( comest->tool, 1 );
        }
        if( to_eat.type->has_use() ) {
            amount_used = to_eat.type->invoke( p, to_eat, p.pos() );
            if( amount_used <= 0 ) {
                reason = _( "It doesn't look like a good idea to consume this.." );
                return REFUSED;
            }
        }

        to_eat.charges -= amount_used;
        p.consume_effects( to_eat );
        p.moves -= 250;
    } else {
        debugmsg( "Unknown comestible type of item: %s\n", to_eat.tname().c_str() );
    }

    if( to_eat.charges > 0 ) {
        return CONSUMED_SOME;
    }

    if( consuming_contents ) {
        it.contents.erase( it.contents.begin() );
        return CONSUMED_SOME;
    }

    // If not consuming contents and charge <= 0, we just ate the last charge from the stack
    return CONSUMED_ALL;
}

std::string give_item_to( npc &p, bool allow_use, bool allow_carry )
{
    const int inv_pos = g->inv_for_all( _( "Offer what?" ), _( "You have no items to offer." ) );
    item &given = g->u.i_at( inv_pos );
    if( given.is_null() ) {
        return _( "Changed your mind?" );
    }

    if( &given == &g->u.weapon && given.has_flag( "NO_UNWIELD" ) ) {
        // Bionic weapon or shackles
        return _( "How?" );
    }

    if( given.is_dangerous() && !g->u.has_trait( trait_DEBUG_MIND_CONTROL ) ) {
        return _( "Are you <swear> insane!?" );
    }

    std::string no_consume_reason;
    if( allow_use ) {
        // Eating first, to avoid evaluating bread as a weapon
        const auto consume_res = try_consume( p, given, no_consume_reason );
        if( consume_res == CONSUMED_ALL ) {
            g->u.i_rem( inv_pos );
        }
        if( consume_res != REFUSED ) {
            g->u.moves -= 100;
            if( given.is_container() ) {
                given.on_contents_changed();
            }
            return _( "Here we go..." );
        }
    }

    bool taken = false;
    long our_ammo = p.ammo_count_for( p.weapon );
    long new_ammo = p.ammo_count_for( given );
    const double new_weapon_value = p.weapon_value( given, new_ammo );
    const double cur_weapon_value = p.weapon_value( p.weapon, our_ammo );
    if( allow_use ) {
        add_msg( m_debug, "NPC evaluates own %s (%d ammo): %0.1f",
                 p.weapon.tname().c_str(), our_ammo, cur_weapon_value );
        add_msg( m_debug, "NPC evaluates your %s (%d ammo): %0.1f",
                 given.tname().c_str(), new_ammo, new_weapon_value );
        if( new_weapon_value > cur_weapon_value ) {
            p.wield( given );
            taken = true;
        }

        // is_gun here is a hack to prevent NPCs wearing guns if they don't want to use them
        if( !taken && !given.is_gun() && p.wear_if_wanted( given ) ) {
            taken = true;
        }
    }

    if( !taken && allow_carry &&
        p.can_pickVolume( given ) &&
        p.can_pickWeight( given ) ) {
        taken = true;
        p.i_add( given );
    }

    if( taken ) {
        g->u.i_rem( inv_pos );
        g->u.moves -= 100;
        p.has_new_items = true;
        return _( "Thanks!" );
    }

    std::stringstream reason;
    reason << _( "Nope." );
    reason << std::endl;
    if( allow_use ) {
        if( !no_consume_reason.empty() ) {
            reason << no_consume_reason;
            reason << std::endl;
        }

        reason << _( "My current weapon is better than this." );
        reason << std::endl;
        reason << string_format( _( "(new weapon value: %.1f vs %.1f)." ),
                                 new_weapon_value, cur_weapon_value );
        if( !given.is_gun() && given.is_armor() ) {
            reason << std::endl;
            reason << string_format( _( "It's too encumbering to wear." ) );
        }
    }
    if( allow_carry ) {
        if( !p.can_pickVolume( given ) ) {
            const units::volume free_space = p.volume_capacity() - p.volume_carried();
            reason << std::endl;
            reason << string_format( _( "I have no space to store it." ) );
            reason << std::endl;
            if( free_space > 0 ) {
                reason << string_format( _( "I can only store %s %s more." ),
                                         format_volume( free_space ).c_str(), volume_units_long() );
            } else {
                reason << string_format( _( "...or to store anything else for that matter." ) );
            }
        }
        if( !p.can_pickWeight( given ) ) {
            reason << std::endl;
            reason << string_format( _( "It is too heavy for me to carry." ) );
        }
    }

    return reason.str();
}

bool npc::has_item_whitelist() const
{
    return is_following() && !rules.pickup_whitelist->empty();
}

bool npc::item_name_whitelisted( const std::string &to_match )
{
    if( !has_item_whitelist() ) {
        return true;
    }

    auto &wlist = *rules.pickup_whitelist;
    const auto rule = wlist.check_item( to_match );
    if( rule == RULE_WHITELISTED ) {
        return true;
    }

    if( rule == RULE_BLACKLISTED ) {
        return false;
    }

    wlist.create_rule( to_match );
    return wlist.check_item( to_match ) == RULE_WHITELISTED;
}

bool npc::item_whitelisted( const item &it )
{
    if( !has_item_whitelist() ) {
        return true;
    }

    const auto to_match = it.tname( 1, false );
    return item_name_whitelisted( to_match );
}

npc_follower_rules::npc_follower_rules()
{
    engagement = ENGAGE_CLOSE;
    aim = AIM_WHEN_CONVENIENT;
    use_guns = true;
    use_grenades = true;
    use_silent = false;

    allow_pick_up = false;
    allow_bash = false;
    allow_sleep = false;
    allow_complain = true;
    allow_pulp = true;

    close_doors = false;
};

npc *pick_follower()
{
    std::vector<npc *> followers;
    std::vector<tripoint> locations;

    for( npc &guy : g->all_npcs() ) {
        if( guy.is_following() && g->u.sees( guy ) ) {
            followers.push_back( &guy );
            locations.push_back( guy.pos() );
        }
    }

    pointmenu_cb callback( locations );

    uimenu menu;
    menu.text = _( "Select a follower" );
    menu.return_invalid = true;
    menu.callback = &callback;
    menu.w_y = 2;

    for( const npc *p : followers ) {
        menu.addentry( -1, true, MENU_AUTOASSIGN, p->name );
    }

    menu.query();
    if( menu.ret < 0 || menu.ret >= static_cast<int>( followers.size() ) ) {
        return nullptr;
    }

    return followers[ menu.ret ];
}
