
#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstddef>
#include <exception>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "achievement.h"
#include "action.h"
#include "auto_pickup.h"
#include "avatar.h"
#include "bionics.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_lazy.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "character.h"
#include "character_id.h"
#include "city.h"
#include "clzones.h"
#include "color.h"
#include "condition.h"
#include "coordinates.h"
#include "crafting_gui.h"
#include "creature.h"
#include "creature_tracker.h"
#include "current_map.h"
#include "damage.h"
#include "debug.h"
#include "dialogue.h"
#include "dialogue_chatbin.h"
#include "dialogue_helpers.h"
#include "dialogue_win.h"
#include "effect_on_condition.h"
#include "enum_conversions.h"
#include "enum_traits.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "explosion.h"
#include "faction.h"
#include "faction_camp.h"
#include "field.h"
#include "flag.h"
#include "flat_set.h"
#include "flexbuffer_json.h"
#include "game.h"
#include "game_constants.h"
#include "game_inventory.h"
#include "generic_factory.h"
#include "global_vars.h"
#include "gun_mode.h"
#include "help.h"
#include "input_context.h"
#include "input_enums.h"
#include "inventory.h"
#include "inventory_ui.h"
#include "item.h"
#include "item_category.h"
#include "item_group.h"
#include "item_location.h"
#include "itype.h"
#include "kill_tracker.h"
#include "magic.h"
#include "map.h"
#include "map_iterator.h"
#include "map_scale_constants.h"
#include "map_selector.h"
#include "mapbuffer.h"
#include "mapgen_functions.h"
#include "mapgendata.h"
#include "martialarts.h"
#include "math_parser_type.h"
#include "math_parser_diag_value.h"
#include "memory_fast.h"
#include "messages.h"
#include "mission.h"
#include "mongroup.h"
#include "monster.h"
#include "mtype.h"
#include "mutation.h"
#include "npc.h"
#include "npc_class.h"
#include "npc_opinion.h"
#include "npctalk.h"
#include "npctalk_rules.h"
#include "npctrade.h"
#include "output.h"
#include "overmap_ui.h"
#include "overmapbuffer.h"
#include "pimpl.h"
#include "player_activity.h"
#include "pocket_type.h"
#include "point.h"
#include "popup.h"
#include "proficiency.h"
#include "ranged.h"
#include "recipe.h"
#include "recipe_groups.h"
#include "requirements.h"
#include "ret_val.h"
#include "rng.h"
#include "simple_pathfinding.h"
#include "skill.h"
#include "sounds.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "submap.h"
#include "talker.h"
#include "talker_topic.h"
#include "teleport.h"
#include "text_snippets.h"
#include "timed_event.h"
#include "translation.h"
#include "translation_gendered.h"
#include "translations.h"
#include "trap.h"
#include "uilist.h"
#include "ui_manager.h"
#include "uistate.h"
#include "units.h"
#include "veh_type.h"
#include "vehicle.h"
#include "visitable.h"
#include "vitamin.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "weather.h"
#include "weighted_list.h"

class recipe_subset;

static const activity_id ACT_AIM( "ACT_AIM" );
static const activity_id ACT_SOCIALIZE( "ACT_SOCIALIZE" );
static const activity_id ACT_TRAIN( "ACT_TRAIN" );
static const activity_id ACT_WAIT_NPC( "ACT_WAIT_NPC" );

static const efftype_id effect_asked_to_train( "asked_to_train" );
static const efftype_id effect_narcosis( "narcosis" );
static const efftype_id effect_riding( "riding" );
static const efftype_id effect_sleep( "sleep" );
static const efftype_id effect_under_operation( "under_operation" );

static const flag_id json_flag_NO_UNLOAD( "NO_UNLOAD" );

static const itype_id fuel_type_animal( "animal" );
static const itype_id itype_foodperson_mask( "foodperson_mask" );
static const itype_id itype_foodperson_mask_on( "foodperson_mask_on" );

static const skill_id skill_firstaid( "firstaid" );

static const skill_id skill_speech( "speech" );

static const trait_id trait_DEBUG_MIND_CONTROL( "DEBUG_MIND_CONTROL" );
static const trait_id trait_HALLUCINATION( "HALLUCINATION" );
static const trait_id trait_PROF_CHURL( "PROF_CHURL" );
static const trait_id trait_PROF_FOODP( "PROF_FOODP" );

static const zone_type_id zone_type_NPC_INVESTIGATE_ONLY( "NPC_INVESTIGATE_ONLY" );
static const zone_type_id zone_type_NPC_NO_INVESTIGATE( "NPC_NO_INVESTIGATE" );

static std::map<std::string, json_talk_topic> json_talk_topics;

using item_menu = std::function<item_location( const item_location_filter & )>;
using item_menu_mul = std::function<drop_locations( const item_location_filter & )>;

struct sub_effect_parser {
    using f_t = talk_effect_fun_t::func( * )( const JsonObject &, std::string_view,
                std::string_view src );
    using f_t_beta = talk_effect_fun_t::func( * )( const JsonObject &, std::string_view,
                     std::string_view src, bool );
    using f_t_effect = talk_effect_fun_t ( * )( const JsonObject &, std::string_view,
                       std::string_view src );
    using f_t_beta_effect = talk_effect_fun_t ( * )( const JsonObject &, std::string_view,
                            std::string_view src, bool );

    sub_effect_parser( std::string_view key_alpha_, jarg arg_, f_t f_ ) : key_alpha( key_alpha_ ),
        arg( arg_ ) {
        f = [f_]( const JsonObject & jo, std::string_view key, std::string_view src, bool ) {
            return talk_effect_fun_t( f_( jo, key, src ) );
        };
    }
    sub_effect_parser( std::string_view key_alpha_, std::string_view key_beta_, jarg arg_,
                       f_t_beta f_ ) : key_alpha( key_alpha_ ), key_beta( key_beta_ ), arg( arg_ ) {
        f = [f_]( const JsonObject & jo, std::string_view key, std::string_view src, bool beta ) {
            return talk_effect_fun_t( f_( jo, key, src, beta ) );
        };
        has_beta = true;
    }
    sub_effect_parser( std::string_view key_alpha_,
                       jarg arg_, f_t_effect f_ )
        : key_alpha( key_alpha_ ), arg( arg_ ) {
        f = [f_]( const JsonObject & jo, std::string_view key, std::string_view src, bool ) {
            return f_( jo, key, src );
        };
    }

    bool check( const JsonObject &jo, bool beta = false ) const {
        std::string_view key = beta ? key_beta : key_alpha;
        if( ( ( arg & jarg::member ) && jo.has_member( key ) ) ||
            ( ( arg & jarg::object ) && jo.has_object( key ) ) ||
            ( ( arg & jarg::string ) && jo.has_string( key ) ) ||
            ( ( arg & jarg::array ) && jo.has_array( key ) ) ) {
            return true;
        }
        return false;
    }

    bool has_beta = false;
    std::string_view key_alpha;
    std::string_view key_beta;
    jarg arg;
    std::function<talk_effect_fun_t( const JsonObject &, std::string_view, std::string_view src, bool )>
    f;
};

struct item_search_data {

    std::vector<str_or_var> id;
    std::vector<str_or_var> id_blacklist;
    std::vector<str_or_var> category;
    std::vector<str_or_var> material;
    std::vector<str_or_var> flags;
    std::vector<str_or_var> excluded_flags;
    std::optional<bool> uses_energy;
    std::optional<bool> is_chargeable;
    bool worn_only;
    bool wielded_only;
    bool held_only;

    std::function<bool( const_dialogue const & )> condition;
    bool has_condition = false;

    explicit item_search_data( const JsonObject &jo ) {

        if( jo.has_array( "id" ) ) {
            for( JsonValue jv : jo.get_array( "id" ) ) {
                id.emplace_back( get_str_or_var( jv, "id" ) );
            }
        }
        if( jo.has_object( "id" ) || jo.has_string( "id" ) ) {
            id.emplace_back( get_str_or_var( jo.get_member( "id" ), "id", false, "" ) );
        }

        if( jo.has_array( "id_blacklist" ) ) {
            for( JsonValue jv : jo.get_array( "id_blacklist" ) ) {
                id_blacklist.emplace_back( get_str_or_var( jv, "id_blacklist" ) );
            }
        }
        if( jo.has_object( "id_blacklist" ) || jo.has_string( "id_blacklist" ) ) {
            id_blacklist.emplace_back( get_str_or_var( jo.get_member( "id_blacklist" ), "id_blacklist", false,
                                       "" ) );
        }

        if( jo.has_array( "category" ) ) {
            for( JsonValue jv : jo.get_array( "category" ) ) {
                category.emplace_back( get_str_or_var( jv, "category" ) );
            }
        }
        if( jo.has_object( "category" ) || jo.has_string( "category" ) ) {
            category.emplace_back( get_str_or_var( jo.get_member( "category" ), "category", false, "" ) );
        }

        if( jo.has_array( "material" ) ) {
            for( JsonValue jv : jo.get_array( "material" ) ) {
                material.emplace_back( get_str_or_var( jv, "material" ) );
            }
        }
        if( jo.has_object( "material" ) || jo.has_string( "material" ) ) {
            material.emplace_back( get_str_or_var( jo.get_member( "material" ), "material", false, "" ) );
        }

        if( jo.has_array( "flags" ) ) {
            for( JsonValue jv : jo.get_array( "flags" ) ) {
                flags.emplace_back( get_str_or_var( jv, "flags" ) );
            }
        }
        if( jo.has_object( "flags" ) || jo.has_string( "flags" ) ) {
            flags.emplace_back( get_str_or_var( jo.get_member( "flags" ), "flags", false, "" ) );
        }

        if( jo.has_array( "excluded_flags" ) ) {
            for( JsonValue jv : jo.get_array( "excluded_flags" ) ) {
                excluded_flags.emplace_back( get_str_or_var( jv, "excluded_flags" ) );
            }
        }
        if( jo.has_object( "excluded_flags" ) || jo.has_string( "excluded_flags" ) ) {
            excluded_flags.emplace_back( get_str_or_var( jo.get_member( "excluded_flags" ), "excluded_flags",
                                         false, "" ) );
        }
        if( jo.has_member( "condition" ) ) {
            read_condition( jo, "condition", condition, false );
            has_condition = true;
        }
        if( jo.has_member( "uses_energy" ) ) {
            uses_energy.emplace( jo.get_member( "uses_energy" ) );
        }
        if( jo.has_member( "is_chargeable" ) ) {
            is_chargeable.emplace( jo.get_member( "is_chargeable" ) );
        }

        worn_only = jo.get_bool( "worn_only", false );
        wielded_only = jo.get_bool( "wielded_only", false );
        held_only = jo.get_bool( "held_only", false );
    }

    bool check( const Character *guy, const item_location &loc, const_dialogue const &d ) {

        bool match;

        if( !id.empty() ) {
            std::vector<itype_id> id_evaluated;
            id_evaluated.reserve( id.size() );
            for( const str_or_var &id_eval : id ) {
                id_evaluated.emplace_back( id_eval.evaluate( d ) );
            }
            match = false;
            for( itype_id id : id_evaluated ) {
                if( id == loc->typeId() ) {
                    match = true;
                }
            }
            if( !id_evaluated.empty() && !match ) {
                return false;
            }
        }

        if( !id_blacklist.empty() ) {
            std::vector<itype_id> id_blacklist_evaluated;
            id_blacklist_evaluated.reserve( id_blacklist.size() );
            for( const str_or_var &id_blacklist_eval : id_blacklist ) {
                id_blacklist_evaluated.emplace_back( id_blacklist_eval.evaluate( d ) );
            }
            match = false;
            for( itype_id id : id_blacklist_evaluated ) {
                if( id == loc->typeId() ) {
                    match = true;
                }
            }
            if( !id_blacklist_evaluated.empty() && match ) {
                return false;
            }
        }

        if( !category.empty() ) {
            std::vector<item_category_id> category_evaluated;
            category_evaluated.reserve( category.size() );
            for( const str_or_var &category_eval : category ) {
                category_evaluated.emplace_back( category_eval.evaluate( d ) );
            }
            match = false;
            for( item_category_id category : category_evaluated ) {
                if( category == loc->get_category_shallow().id ) {
                    match = true;
                }
            }
            if( !category_evaluated.empty() && !match ) {
                return false;
            }
        }

        if( !material.empty() ) {
            std::vector<material_id> material_evaluated;
            material_evaluated.reserve( material.size() );
            for( const str_or_var &material_eval : material ) {
                material_evaluated.emplace_back( material_eval.evaluate( d ) );
            }
            match = false;
            for( material_id id : material_evaluated ) {
                if( loc->made_of( id ) > 0 ) {
                    match = true;
                }
            }
            if( !material_evaluated.empty() && !match ) {
                return false;
            }
        }

        if( !flags.empty() ) {
            std::vector<flag_id> flags_evaluated;
            flags_evaluated.reserve( flags.size() );
            for( const str_or_var &flags_eval : flags ) {
                flags_evaluated.emplace_back( flags_eval.evaluate( d ) );
            }
            match = false;
            for( flag_id flag : flags_evaluated ) {
                if( loc->has_flag( flag ) ) {
                    match = true;
                }
            }
            if( !flags_evaluated.empty() && !match ) {
                return false;
            }
        }

        if( !excluded_flags.empty() ) {
            std::vector<flag_id> excluded_flags_evaluated;
            excluded_flags_evaluated.reserve( excluded_flags.size() );
            for( const str_or_var &excluded_flags_eval : excluded_flags ) {
                excluded_flags_evaluated.emplace_back( excluded_flags_eval.evaluate( d ) );
            }
            match = false;
            for( flag_id flag : excluded_flags_evaluated ) {
                if( loc->has_flag( flag ) ) {
                    match = true;
                }
            }
            if( !excluded_flags_evaluated.empty() && match ) {
                return false;
            }
        }

        if( uses_energy.has_value() && loc->uses_energy() != uses_energy.value() ) {
            return false;
        }

        if( is_chargeable.has_value() && ( ( is_chargeable.value() && !loc->is_chargeable() ) ||
                                           ( !is_chargeable.value() && ( loc->is_chargeable() || !loc->uses_energy() ) ) ) ) {
            return false;
        }

        if( worn_only && !guy->is_worn( *loc ) ) {
            return false;
        }
        if( wielded_only && !guy->is_wielding( *loc ) ) {
            return false;
        }
        if( held_only && !( guy->is_worn( *loc ) || guy->is_wielding( *loc ) ) ) {
            return false;
        }

        if( has_condition ) {
            const_dialogue dial( d.const_actor( false )->const_clone(), get_const_talker_for( loc ) );
            if( !condition( dial ) ) {
                return false;
            }
        }

        return true;
    }
};

#define dbg(x) DebugLog((x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

static int topic_category( const talk_topic &the_topic );

static const talk_topic &special_talk( const std::string &action );

static bool friendly_teacher( const Character &student, const Character &teacher )
{
    return ( student.is_npc() && teacher.is_avatar() ) ||
           ( teacher.is_npc() && teacher.as_npc()->is_player_ally() );
}

std::string talk_trial::name() const
{
    static const std::array<std::string, NUM_TALK_TRIALS> texts = { {
            "", translate_marker( "LIE" ), translate_marker( "PERSUADE" ), translate_marker( "INTIMIDATE" ), ""
        }
    };
    if( static_cast<size_t>( type ) >= texts.size() ) {
        debugmsg( "invalid trial type %d", static_cast<int>( type ) );
        return std::string();
    }
    return texts[type].empty() ? std::string() : _( texts[type] );
}

static void run_eoc_vector( const std::vector<effect_on_condition_id> &eocs, const dialogue &d )
{
    dialogue newDialog( d );
    for( const effect_on_condition_id &eoc : eocs ) {
        eoc->activate( newDialog );
    }
}

static std::vector<effect_on_condition_id> load_eoc_vector( const JsonObject &jo,
        std::string_view member, std::string_view src )
{
    std::vector<effect_on_condition_id> eocs;
    if( jo.has_array( member ) ) {
        for( JsonValue jv : jo.get_array( member ) ) {
            eocs.push_back( effect_on_conditions::load_inline_eoc( jv, src ) );
        }
    } else if( jo.has_member( member ) ) {
        eocs.push_back( effect_on_conditions::load_inline_eoc( jo.get_member( member ), src ) );
    }
    return eocs;
}

struct eoc_entry {
    effect_on_condition_id id;
    std::optional<str_or_var> var;
};
static std::vector<eoc_entry>
load_eoc_vector_id_and_var(
    const JsonObject &jo, std::string_view member, std::string_view src )
{
    std::vector<eoc_entry> eocs_entries;
    auto process_jv = [member, &eocs_entries, &src]( const JsonValue & jv ) {
        eoc_entry entry;
        if( jv.test_object() ) {
            JsonObject jo = jv.get_object();
            jo.allow_omitted_members();
            if( !jo.has_member( "id" ) ) {
                entry.var = get_str_or_var( jv, member, false );
            }
        }
        if( !entry.var ) {
            entry.id = effect_on_conditions::load_inline_eoc( jv, src );
        }
        eocs_entries.push_back( entry );
    };
    if( jo.has_array( member ) ) {
        for( JsonValue jv : jo.get_array( member ) ) {
            process_jv( jv );
        }
    } else if( jo.has_member( member ) ) {
        process_jv( jo.get_member( member ) );
    }
    return eocs_entries;
}


/** Time (in turns) and cost (in cent) for training: */
time_duration calc_skill_training_time_char( const Character &teacher, const Character &student,
        const skill_id &skill )
{
    return 1_hours + 30_minutes * student.get_skill_level( skill ) -
           1_minutes * teacher.get_skill_level( skill );
}

int calc_skill_training_cost_char( const Character &teacher, const Character &student,
                                   const skill_id &skill )
{
    if( friendly_teacher( student, teacher ) ) {
        return 0;
    }
    int skill_level = student.get_knowledge_level( skill );
    return 1000 * ( 1 + skill_level ) * ( 1 + skill_level );
}

time_duration calc_proficiency_training_time( const Character &, const Character &student,
        const proficiency_id &proficiency )
{
    return std::min( 30_minutes, student.proficiency_training_needed( proficiency ) );
}

int calc_proficiency_training_cost( const Character &teacher, const Character &student,
                                    const proficiency_id &proficiency )
{
    if( friendly_teacher( student, teacher ) ) {
        return 0;
    }
    return to_seconds<int>( calc_proficiency_training_time( teacher, student, proficiency ) );
}

// TODO: all styles cost the same and take the same time to train,
// maybe add values to the ma_style class to makes this variable
// TODO: maybe move this function into the ma_style class? Or into the NPC class?
time_duration calc_ma_style_training_time( const Character &, const Character &,
        const matype_id & )
{
    return 30_minutes;
}

int calc_ma_style_training_cost( const Character &teacher, const Character &student,
                                 const matype_id & )
{
    if( friendly_teacher( student, teacher ) ) {
        return 0;
    }
    return 800;
}

// quicker to learn with instruction as opposed to books.
// if this is a known spell, then there is a set time to gain some exp.
// if player doesn't know this spell, then the NPC will teach all of it
// which takes max 6 hours, min 3 hours.
// TODO: a system for NPCs to train new stuff in bits and pieces
// and remember the progress.
time_duration calc_spell_training_time( const Character &, const Character &student,
                                        const spell_id &id )
{
    if( student.magic->knows_spell( id ) ) {
        return 1_hours;
    } else {
        const int time_int = student.magic->time_to_learn_spell( student, id ) / 50;
        return time_duration::from_seconds( clamp( time_int, 7200, 21600 ) );
    }
}

static int calc_spell_training_cost_gen( const bool knows, int difficulty, int level )
{
    int ret = ( 100 * std::max( 1, difficulty ) * std::max( 1, level ) );
    if( !knows ) {
        ret = ret * 2;
    }
    return ret;
}

int calc_spell_training_cost( const Character &teacher, const Character &student,
                              const spell_id &id )
{
    if( friendly_teacher( student, teacher ) ) {
        return 0;
    }
    const spell &temp_spell = teacher.magic->get_spell( id );
    const bool knows = student.magic->knows_spell( id );
    return calc_spell_training_cost_gen( knows, temp_spell.get_difficulty( student ),
                                         temp_spell.get_level() );
}

int Character::calc_spell_training_cost( const bool knows, int difficulty, int level ) const
{
    const npc *n = as_npc();
    if( !n || n->is_player_ally() ) {
        return 0;
    }
    return calc_spell_training_cost_gen( knows, difficulty, level );
}

// Rescale values from "mission scale" to "opinion scale"
int npc_trading::cash_to_favor( const npc &, int cash )
{
    // TODO: It should affect different NPCs to a different degree
    // Square root of mission value in dollars
    // ~31 for zed mom, 50 for horde master, ~63 for plutonium cells
    double scaled_mission_val = std::sqrt( cash / 100.0 );
    return roll_remainder( scaled_mission_val );
}

enum npc_chat_menu {
    NPC_CHAT_DONE,
    NPC_CHAT_TALK,
    NPC_CHAT_YELL,
    NPC_CHAT_EMOTE,
    NPC_CHAT_START_SEMINAR,
    NPC_CHAT_SENTENCE,
    NPC_CHAT_GUARD,
    NPC_CHAT_MOVE_TO_POS,
    NPC_CHAT_FOLLOW,
    NPC_CHAT_AWAKE,
    NPC_CHAT_MOUNT,
    NPC_CHAT_DISMOUNT,
    NPC_CHAT_DANGER,
    NPC_CHAT_ORDERS,
    NPC_CHAT_NO_GUNS,
    NPC_CHAT_PULP,
    NPC_CHAT_FOLLOW_CLOSE,
    NPC_CHAT_MOVE_FREELY,
    NPC_CHAT_SLEEP,
    NPC_CHAT_FORBID_ENGAGE,
    NPC_CHAT_CLEAR_OVERRIDES,
    NPC_CHAT_ANIMAL_VEHICLE_FOLLOW,
    NPC_CHAT_ANIMAL_VEHICLE_STOP_FOLLOW,
    NPC_CHAT_COMMAND_MAGIC_VEHICLE_FOLLOW,
    NPC_CHAT_COMMAND_MAGIC_VEHICLE_STOP_FOLLOW,
    NPC_CHAT_ACTIVITIES,
    NPC_CHAT_ACTIVITIES_MOVE_LOOT,
    NPC_CHAT_ACTIVITIES_BUTCHERY,
    NPC_CHAT_ACTIVITIES_CHOP_PLANKS,
    NPC_CHAT_ACTIVITIES_CHOP_TREES,
    NPC_CHAT_ACTIVITIES_CONSTRUCTION,
    NPC_CHAT_ACTIVITIES_CRAFT,
    NPC_CHAT_ACTIVITIES_DISASSEMBLY,
    NPC_CHAT_ACTIVITIES_FARMING,
    NPC_CHAT_ACTIVITIES_FISHING,
    NPC_CHAT_ACTIVITIES_MINING,
    NPC_CHAT_ACTIVITIES_MOPPING,
    NPC_CHAT_ACTIVITIES_READ_REPEATEDLY,
    NPC_CHAT_ACTIVITIES_VEHICLE_DECONSTRUCTION,
    NPC_CHAT_ACTIVITIES_VEHICLE_REPAIR,
    NPC_CHAT_ACTIVITIES_UNASSIGN
};

// given a vector of NPCs, presents a menu to allow a player to pick one.
// everyone == true adds another entry at the end to allow selecting all listed NPCs
// this implies a return value of npc_list.size() means "everyone"
static int npc_select_menu( const std::vector<npc *> &npc_list, const std::string &prompt,
                            const bool everyone = true )
{
    if( npc_list.empty() ) {
        return -1;
    }
    const int npc_count = npc_list.size();
    if( npc_count == 1 ) {
        return 0;
    } else {
        uilist nmenu;
        std::vector<tripoint_bub_ms> locations;
        nmenu.text = prompt;
        for( const npc *elem : npc_list ) {
            nmenu.addentry( -1, true, MENU_AUTOASSIGN, elem->name_and_activity() );
            locations.emplace_back( elem->pos_bub() );
        }
        if( npc_count > 1 && everyone ) {
            nmenu.addentry( -1, true, MENU_AUTOASSIGN, _( "Everyone" ) );
            locations.emplace_back( get_avatar().pos_bub() );
        }
        pointmenu_cb callback( locations );
        nmenu.callback = &callback;
        nmenu.query();
        return nmenu.ret;
    }

}

static int creature_select_menu( const std::vector<Creature *> &talker_list,
                                 const std::string &prompt,
                                 const bool everyone = true )
{
    if( talker_list.empty() ) {
        return -1;
    }
    const int npc_count = talker_list.size();
    if( npc_count == 1 ) {
        return 0;
    } else {
        uilist nmenu;
        std::vector<tripoint_bub_ms> locations;
        nmenu.text = prompt;
        for( const Creature *elem : talker_list ) {
            if( elem->is_npc() ) {
                nmenu.addentry( -1, true, MENU_AUTOASSIGN, elem->as_npc()->name_and_activity() );
            } else {
                nmenu.addentry( -1, true, MENU_AUTOASSIGN, elem->disp_name() );
            }
            locations.emplace_back( elem->pos_bub() );
        }
        if( npc_count > 1 && everyone ) {
            nmenu.addentry( -1, true, MENU_AUTOASSIGN, _( "Everyone" ) );
            locations.emplace_back( get_avatar().pos_bub() );
        }
        pointmenu_cb callback( locations );
        nmenu.callback = &callback;
        nmenu.query();
        return nmenu.ret;
    }
}

std::vector<int> npcs_select_menu( const std::vector<Character *> &npc_list,
                                   const std::string &prompt,
                                   const std::function<bool( const Character * )> &exclude_func )
{
    std::vector<int> picked;
    if( npc_list.empty() ) {
        return picked;
    }
    const int npc_count = npc_list.size();
    int last_index = 0;
    do {
        uilist nmenu;
        nmenu.text = prompt;
        for( int i = 0; i < npc_count; i++ ) {
            std::string entry;
            if( std::find( picked.begin(), picked.end(), i ) != picked.end() ) {
                entry = "* ";
            }
            bool enable = exclude_func == nullptr || !exclude_func( npc_list[i] );
            entry += npc_list[i]->name_and_maybe_activity();
            nmenu.addentry( i, enable, MENU_AUTOASSIGN, entry );
        }
        nmenu.addentry( npc_count, true, 'e', _( "Everyone" ) );
        nmenu.addentry( npc_count + 1, true, 'f', _( "Finish selection" ) );
        nmenu.selected = nmenu.fselected = last_index;
        nmenu.query();
        if( nmenu.ret < 0 ) {
            return std::vector<int>();
        } else if( nmenu.ret == npc_count ) {
            picked.resize( npc_count );
            std::iota( picked.begin(), picked.end(), 0 );
            last_index = nmenu.fselected;
            continue;
        } else if( nmenu.ret > npc_count ) {
            break;
        }
        std::vector<int>::iterator exists = std::find( picked.begin(), picked.end(), nmenu.ret );
        if( exists != picked.end() ) {
            picked.erase( exists );
        } else {
            picked.push_back( nmenu.ret );
        }
        last_index = nmenu.fselected;
    } while( true );
    return picked;
}

static std::string training_select_menu( const Character &c, const std::string &prompt )
{
    int i = 0;
    uilist nmenu;
    nmenu.text = prompt;
    std::vector<std::string> trainlist;
    for( const std::pair<const skill_id, SkillLevel> &s : *c._skills ) {
        bool enabled = s.first->is_teachable() && s.second.level() > 0;
        std::string entry = string_format( "%s: %s (%d)", _( "Skill" ), s.first->name(), s.second.level() );
        nmenu.addentry( i, enabled, MENU_AUTOASSIGN, entry );
        trainlist.emplace_back( s.first.c_str() );
        i++;
    }
    for( const proficiency_id &p : c.known_proficiencies() ) {
        std::string entry = string_format( "%s: %s", _( "Proficiency" ), p->name() );
        nmenu.addentry( i, p->is_teachable(), MENU_AUTOASSIGN, entry );
        trainlist.emplace_back( p.c_str() );
        i++;
    }
    for( const matype_id &m : c.known_styles( true ) ) {
        std::string entry = string_format( "%s: %s", _( "Style" ), m->name.translated() );
        nmenu.addentry( i, m->teachable, MENU_AUTOASSIGN, entry );
        trainlist.emplace_back( m.c_str() );
        i++;
    }
    for( const spell_id &s : c.magic->spells() ) {
        std::string entry = string_format( "%s: %s", _( "Spell" ), s->name.translated() );
        nmenu.addentry( i, s->teachable, MENU_AUTOASSIGN, entry );
        trainlist.emplace_back( s.c_str() );
        i++;
    }
    nmenu.query();
    if( nmenu.ret > -1 && nmenu.ret < static_cast<int>( trainlist.size() ) ) {
        return trainlist[nmenu.ret];
    }
    return "";
}

static void npc_batch_override_toggle(
    const std::vector<npc *> &npc_list, ally_rule rule, bool state )
{
    for( npc *p : npc_list ) {
        p->rules.toggle_specific_override_state( rule, state );
    }
}

static void npc_temp_orders_menu( const std::vector<npc *> &npc_list )
{
    if( npc_list.empty() ) {
        return;
    }
    npc *guy = npc_list.front();
    Character &player_character = get_player_character();
    bool done = false;
    uilist nmenu;

    while( !done ) {
        int override_count = 0;
        std::string output_string = string_format( _( "%s currently has these temporary orders:" ),
                                    guy->get_name() );
        for( const auto &rule : ally_rule_strs ) {
            if( guy->rules.has_override_enable( rule.second.rule ) ) {
                override_count++;
                output_string += "\n  ";
                output_string += ( guy->rules.has_override( rule.second.rule ) ? rule.second.rule_true_text :
                                   rule.second.rule_false_text );
            }
        }
        if( override_count == 0 ) {
            output_string += std::string( "\n  " ) + _( "None." ) + "\n";
        }
        if( npc_list.size() > 1 ) {
            output_string += std::string( "\n" ) +
                             _( "Other followers might have different temporary orders." );
        }
        nmenu.reset();
        nmenu.text = _( "Issue what temporary order?" );
        nmenu.desc_enabled = true;
        parse_tags( output_string, player_character, *guy );
        nmenu.footer_text = output_string;
        nmenu.addentry( NPC_CHAT_DONE, true, 'd', _( "Done issuing orders" ) );
        nmenu.addentry( NPC_CHAT_FORBID_ENGAGE, true, 'f',
                        guy->rules.has_override_enable( ally_rule::forbid_engage ) ?
                        _( "Go back to your usual engagement habits" ) : _( "Don't engage hostiles for the time being" ) );
        nmenu.addentry( NPC_CHAT_NO_GUNS, true, 'g', guy->rules.has_override_enable( ally_rule::use_guns ) ?
                        _( "Use whatever weapon you normally would" ) : _( "Don't use ranged weapons for a while" ) );
        nmenu.addentry( NPC_CHAT_PULP, true, 'p', guy->rules.has_override_enable( ally_rule::allow_pulp ) ?
                        _( "Pulp zombies if you like" ) : _( "Hold off on pulping zombies for a while" ) );
        nmenu.addentry( NPC_CHAT_FOLLOW_CLOSE, true, 'c',
                        guy->rules.has_override_enable( ally_rule::follow_close ) &&
                        guy->rules.has_override( ally_rule::follow_close ) ?
                        _( "Go back to keeping your usual distance" ) : _( "Stick close to me for now" ) );
        nmenu.addentry( NPC_CHAT_MOVE_FREELY, true, 'm',
                        guy->rules.has_override_enable( ally_rule::follow_close ) &&
                        !guy->rules.has_override( ally_rule::follow_close ) ?
                        _( "Go back to keeping your usual distance" ) : _( "Move farther from me if you need to" ) );
        nmenu.addentry( NPC_CHAT_SLEEP, true, 's',
                        guy->rules.has_override_enable( ally_rule::allow_sleep ) ?
                        _( "Go back to your usual sleeping habits" ) : _( "Take a nap if you need it" ) );
        nmenu.addentry( NPC_CHAT_CLEAR_OVERRIDES, true, 'o', _( "Let's go back to your usual behaviors" ) );
        nmenu.query();

        switch( nmenu.ret ) {
            case NPC_CHAT_FORBID_ENGAGE:
                npc_batch_override_toggle( npc_list, ally_rule::forbid_engage, true );
                break;
            case NPC_CHAT_NO_GUNS:
                npc_batch_override_toggle( npc_list, ally_rule::use_guns, false );
                break;
            case NPC_CHAT_PULP:
                npc_batch_override_toggle( npc_list, ally_rule::allow_pulp, false );
                break;
            case NPC_CHAT_FOLLOW_CLOSE:
                npc_batch_override_toggle( npc_list, ally_rule::follow_close, true );
                break;
            case NPC_CHAT_MOVE_FREELY:
                npc_batch_override_toggle( npc_list, ally_rule::follow_close, false );
                break;
            case NPC_CHAT_SLEEP:
                npc_batch_override_toggle( npc_list, ally_rule::allow_sleep, true );
                break;
            case NPC_CHAT_CLEAR_OVERRIDES:
                for( npc *p : npc_list ) {
                    p->rules.clear_overrides();
                }
                break;
            default:
                done = true;
                break;
        }
    }

}

static int npc_activities_menu()
{
    uilist nmenu;
    nmenu.text = _( "What should be worked on?" );

    nmenu.addentry( NPC_CHAT_ACTIVITIES_MOVE_LOOT, true, 'l', _( "Organizing loot into zones" ) );
    nmenu.addentry( NPC_CHAT_ACTIVITIES_BUTCHERY, true, 'b', _( "Butchering corpses" ) );
    nmenu.addentry( NPC_CHAT_ACTIVITIES_CHOP_TREES, true, 't', _( "Chopping down trees" ) );
    nmenu.addentry( NPC_CHAT_ACTIVITIES_CHOP_PLANKS, true, 'p', _( "Chopping logs into planks" ) );
    nmenu.addentry( NPC_CHAT_ACTIVITIES_CONSTRUCTION, true, 'c', _( "Constructing blueprints" ) );
    nmenu.addentry( NPC_CHAT_ACTIVITIES_CRAFT, true, 'C', _( "Crafting item" ) );
    nmenu.addentry( NPC_CHAT_ACTIVITIES_DISASSEMBLY, true, 'd', _( "Disassembly of items" ) );
    nmenu.addentry( NPC_CHAT_ACTIVITIES_FARMING, true, 'f', _( "Farming plots" ) );
    nmenu.addentry( NPC_CHAT_ACTIVITIES_FISHING, true, 'F', _( "Fishing in a zone" ) );
    nmenu.addentry( NPC_CHAT_ACTIVITIES_MINING, true, 'M', _( "Mining out tiles" ) );
    nmenu.addentry( NPC_CHAT_ACTIVITIES_MOPPING, true, 'm', _( "Mopping up stains" ) );
    nmenu.addentry( NPC_CHAT_ACTIVITIES_READ_REPEATEDLY, true, 'R',
                    _( "Study from books you have in order" ) );
    nmenu.addentry( NPC_CHAT_ACTIVITIES_VEHICLE_DECONSTRUCTION, true, 'v',
                    _( "Deconstructing vehicles" ) );
    nmenu.addentry( NPC_CHAT_ACTIVITIES_VEHICLE_REPAIR, true, 'V', _( "Repairing vehicles" ) );
    nmenu.addentry( NPC_CHAT_ACTIVITIES_UNASSIGN, true, '-',
                    _( "Taking it easy (Stop what they are working on)" ) );

    nmenu.query();

    return nmenu.ret;
}

static void tell_veh_stop_following()
{
    Character &player_character = get_player_character();
    for( wrapped_vehicle &veh : get_map().get_vehicles() ) {
        vehicle *v = veh.v;
        if( v->has_engine_type( fuel_type_animal, false ) && v->is_owned_by( player_character ) ) {
            v->is_following = false;
            v->engine_on = false;
        }
    }
}

static void assign_veh_to_follow()
{
    map &here = get_map();

    Character &player_character = get_player_character();
    for( wrapped_vehicle &veh : here.get_vehicles() ) {
        vehicle *v = veh.v;
        if( v->has_engine_type( fuel_type_animal, false ) && v->is_owned_by( player_character ) ) {
            v->activate_animal_follow( here );
        }
    }
}

static void tell_magic_veh_to_follow()
{
    Character &player_character = get_player_character();
    for( wrapped_vehicle &veh : get_map().get_vehicles() ) {
        vehicle *v = veh.v;
        if( v->magic ) {
            for( const vpart_reference &vp : v->get_all_parts() ) {
                const vpart_info &vpi = vp.info();
                if( vpi.has_flag( "MAGIC_FOLLOW" ) && v->is_owned_by( player_character ) ) {
                    v->activate_magical_follow();
                    break;
                }
            }
        }
    }
}

static void tell_magic_veh_stop_following()
{
    for( wrapped_vehicle &veh : get_map().get_vehicles() ) {
        vehicle *v = veh.v;
        if( v->magic ) {
            for( const vpart_reference &vp : v->get_all_parts() ) {
                const vpart_info &vpi = vp.info();
                if( vpi.has_flag( "MAGIC_FOLLOW" ) ) {
                    v->is_following = false;
                    v->engine_on = false;
                    break;
                }
            }
        }
    }
}

void game::chat()
{
    map &here = get_map();

    Character &player_character = get_player_character();
    int volume = player_character.get_shout_volume();

    const std::vector<Creature *> available = get_creatures_if( [&]( const Creature & guy ) {
        // TODO: Get rid of the z-level check when z-level vision gets "better"
        return ( guy.is_npc() || ( guy.is_monster() &&
                                   guy.as_monster()->has_flag( mon_flag_CONVERSATION ) &&
                                   !guy.as_monster()->type->chat_topics.empty() ) ) && u.posz() == guy.posz() &&
               u.sees( here, guy.pos_bub( here ) ) &&
               rl_dist( u.pos_abs(), guy.pos_abs() ) <= SEEX * 2;
    } );
    const int available_count = available.size();
    const std::vector<npc *> followers = get_npcs_if( [&]( const npc & guy ) {
        return guy.is_player_ally() && guy.is_following() && guy.can_hear( u.pos_bub(), volume );
    } );
    const int follower_count = followers.size();
    const std::vector<npc *> guards = get_npcs_if( [&]( const npc & guy ) {
        return guy.mission == NPC_MISSION_GUARD_ALLY &&
               guy.companion_mission_role_id != "FACTION_CAMP" &&
               guy.can_hear( u.pos_bub(), volume );
    } );
    const int guard_count = guards.size();

    const std::vector<npc *> available_for_activities = get_npcs_if( [&]( const npc & guy ) {
        return guy.is_player_ally() && guy.can_hear( u.pos_bub(), volume ) &&
               guy.companion_mission_role_id != "FACTION CAMP";
    } );
    const int available_for_activities_count = available_for_activities.size();

    if( player_character.has_trait( trait_PROF_FOODP ) &&
        !( player_character.is_wearing( itype_foodperson_mask ) ||
           player_character.is_wearing( itype_foodperson_mask_on ) ) ) {
        add_msg( m_warning, _( "You can't speak without your face!" ) );
        return;
    }
    std::vector<vehicle *> animal_vehicles;
    std::vector<vehicle *> following_vehicles;
    std::vector<vehicle *> magic_vehicles;
    std::vector<vehicle *> magic_following_vehicles;
    for( wrapped_vehicle &veh : here.get_vehicles() ) {
        vehicle *&v = veh.v;
        if( v->has_engine_type( fuel_type_animal, false ) &&
            v->is_owned_by( player_character ) ) {
            animal_vehicles.push_back( v );
            if( v->is_following ) {
                following_vehicles.push_back( v );
            }
        }
        if( v->magic ) {
            for( const vpart_reference &vp : v->get_all_parts() ) {
                const vpart_info &vpi = vp.info();
                if( vpi.has_flag( "MAGIC_FOLLOW" ) ) {
                    magic_vehicles.push_back( v );
                    if( v->is_following ) {
                        magic_following_vehicles.push_back( v );
                    }
                    break;
                }
            }
        }
    }

    uilist nmenu;
    nmenu.text = std::string( _( "What do you want to do?" ) );

    if( !available.empty() ) {
        const Creature *guy = available.front();
        std::string title;
        if( guy->is_npc() ) {
            title = guy->as_npc()->name_and_activity();
        } else if( guy->is_monster() ) {
            title = guy->as_monster()->disp_name();
        }
        nmenu.addentry( NPC_CHAT_TALK, true, 't', available_count == 1 ?
                        string_format( _( "Talk to %s" ), title ) :
                        _( "Talk to…" ) );
    }

    if( !available_for_activities.empty() ) {
        const Creature *guy = available_for_activities.front();
        std::string title;
        if( guy->is_npc() ) {
            title = guy->as_npc()->name_and_activity();
        } else if( guy->is_monster() ) {
            title = guy->as_monster()->disp_name();
        }
        nmenu.addentry( NPC_CHAT_ACTIVITIES, true, 'A', available_for_activities_count == 1 ?
                        string_format( _( "Tell %s to work on…" ), title ) :
                        _( "Tell someone to work on…" )
                      );
    }

    nmenu.addentry( NPC_CHAT_YELL, true, 'a', _( "Yell" ) );
    nmenu.addentry( NPC_CHAT_SENTENCE, true, 'b', _( "Yell a sentence" ) );
    nmenu.addentry( NPC_CHAT_EMOTE, true, 'E', _( "Emote" ) );
    if( !animal_vehicles.empty() ) {
        nmenu.addentry( NPC_CHAT_ANIMAL_VEHICLE_FOLLOW, true, 'F',
                        _( "Whistle at your animals pulling vehicles to follow you." ) );
    }
    if( !magic_vehicles.empty() ) {
        nmenu.addentry( NPC_CHAT_COMMAND_MAGIC_VEHICLE_FOLLOW, true, 'Q',
                        _( "Utter a magical command that will order your magical vehicles to follow you." ) );
    }
    if( !magic_following_vehicles.empty() ) {
        nmenu.addentry( NPC_CHAT_COMMAND_MAGIC_VEHICLE_STOP_FOLLOW, true, 'q',
                        _( "Utter a magical command that will order your magical vehicles to stop following you." ) );
    }
    if( !following_vehicles.empty() ) {
        nmenu.addentry( NPC_CHAT_ANIMAL_VEHICLE_STOP_FOLLOW, true, 'S',
                        _( "Whistle at your animals pulling vehicles to stop following you." ) );
    }
    if( !guards.empty() ) {
        nmenu.addentry( NPC_CHAT_FOLLOW, true, 'f', guard_count == 1 ?
                        string_format( _( "Tell %s to follow" ), guards.front()->get_name() ) :
                        _( "Tell someone to follow…" )
                      );
    }
    if( !followers.empty() ) {
        bool enable_seminar = !player_character.has_effect( effect_asked_to_train );
        nmenu.addentry( NPC_CHAT_START_SEMINAR, enable_seminar, 'T',
                        enable_seminar ? _( "Start a training seminar" ) :
                        _( "Start a training seminar (You've already taught enough for now)" ) );
        nmenu.addentry( NPC_CHAT_GUARD, true, 'g', follower_count == 1 ?
                        string_format( _( "Tell %s to guard" ), followers.front()->get_name() ) :
                        _( "Tell someone to guard…" )
                      );
        nmenu.addentry( NPC_CHAT_MOVE_TO_POS, true, 'G',
                        follower_count == 1 ? string_format( _( "Tell %s to move to location" ),
                                followers.front()->get_name() ) : _( "Tell someone to move to location…" ) );
        nmenu.addentry( NPC_CHAT_AWAKE, true, 'w', _( "Tell everyone on your team to wake up" ) );
        nmenu.addentry( NPC_CHAT_MOUNT, true, 'M', _( "Tell everyone on your team to mount up" ) );
        nmenu.addentry( NPC_CHAT_DISMOUNT, true, 'm', _( "Tell everyone on your team to dismount" ) );
        nmenu.addentry( NPC_CHAT_DANGER, true, 'D',
                        _( "Tell everyone on your team to prepare for danger" ) );
        nmenu.addentry( NPC_CHAT_CLEAR_OVERRIDES, true, 'r',
                        _( "Tell everyone on your team to relax (Clear Overrides)" ) );
        nmenu.addentry( NPC_CHAT_ORDERS, true, 'o', _( "Tell everyone on your team to temporarily…" ) );
    }
    std::string message;
    std::string yell_msg;
    std::string emote_msg;
    bool is_order = true;
    nmenu.query();

    if( nmenu.ret < 0 ) {
        return;
    }

    switch( nmenu.ret ) {
        case NPC_CHAT_TALK: {
            const int npcselect = creature_select_menu( available, _( "Talk to whom?" ), false );
            if( npcselect < 0 ) {
                return;
            }
            get_avatar().talk_to( get_talker_for( *available[npcselect] ) );
            break;
        }
        case NPC_CHAT_YELL:
            is_order = false;
            message = _( "loudly." );
            break;
        case NPC_CHAT_SENTENCE: {
            std::string popupdesc = _( "Enter a sentence to yell" );
            string_input_popup popup;
            popup.title( _( "Yell a sentence" ) )
            .width( 64 )
            .description( popupdesc )
            .identifier( "sentence" )
            .max_length( 128 )
            .query();
            yell_msg = popup.text();
            is_order = false;
            break;
        }
        case NPC_CHAT_EMOTE: {
            std::string popupdesc =
                _( "What do you want to emote?  (This will have no in-game effect!)" );
            string_input_popup popup;
            popup.title( _( "Emote" ) )
            .width( 64 )
            .description( popupdesc )
            .identifier( "sentence" )
            .max_length( 128 )
            .query();
            emote_msg = popup.text();
            is_order = false;
            break;
        }
        case NPC_CHAT_START_SEMINAR: {
            const std::string &t = training_select_menu( player_character,
                                   _( "What would you like to teach?" ) );
            if( t.empty() ) {
                return;
            }
            int id_type = -1;
            std::vector<Character *> clist( followers.begin(), followers.end() );
            const std::string query_str = _( "Who should participate in the training seminar?" );
            std::vector<int> selected;
            skill_id sk( t );
            if( sk.is_valid() ) {
                selected = npcs_select_menu( clist, query_str, [&]( const Character * n ) {
                    return !n ||
                           n->get_knowledge_level( sk ) >= static_cast<int>( player_character.get_skill_level( sk ) );
                } );
                id_type = 0;
            }
            matype_id ma( t );
            if( ma.is_valid() ) {
                selected = npcs_select_menu( clist, query_str, [&]( const Character * n ) {
                    return !n || n->has_martialart( ma );
                } );
                id_type = 1;
            }
            proficiency_id pr( t );
            if( pr.is_valid() ) {
                selected = npcs_select_menu( clist, query_str, [&]( const Character * n ) {
                    return !n || n->has_proficiency( pr );
                } );
                id_type = 2;
            }
            spell_id sp( t );
            if( sp.is_valid() ) {
                selected = npcs_select_menu( clist, query_str, [&]( const Character * n ) {
                    return !n || ( n->magic->knows_spell( sp ) &&
                                   n->magic->get_spell( sp ).get_level() >= player_character.magic->get_spell( sp ).get_level() );
                } );
                id_type = 3;
            }
            if( selected.empty() ) {
                return;
            }
            std::vector<Character *> to_train;
            for( int i : selected ) {
                if( followers[i] ) {
                    to_train.push_back( followers[i] );
                }
            }
            talk_function::teach_domain d;
            d.skill = id_type == 0 ? sk : skill_id();
            d.style = id_type == 1 ? ma : matype_id();
            d.prof = id_type == 2 ? pr : proficiency_id();
            d.spell = id_type == 3 ? sp : spell_id();
            talk_function::start_training_gen( player_character, to_train, d );
            break;
        }
        case NPC_CHAT_GUARD: {
            const int npcselect = npc_select_menu( followers, _( "Who should guard here?" ) );
            if( npcselect < 0 ) {
                return;
            }
            if( npcselect == follower_count ) {
                for( npc *them : followers ) {
                    talk_function::assign_guard( *them );
                }
                yell_msg = _( "Everyone guard here!" );
            } else {
                talk_function::assign_guard( *followers[npcselect] );
                yell_msg = string_format( _( "Guard here, %s!" ), followers[npcselect]->get_name() );
            }
            break;
        }
        case NPC_CHAT_MOVE_TO_POS: {
            const int npcselect = npc_select_menu( followers, _( "Who should move?" ) );
            if( npcselect < 0 ) {
                return;
            }

            std::optional<tripoint_bub_ms> p = look_around();

            if( !p ) {
                return;
            }

            if( here.impassable( *p ) ) {
                add_msg( m_info, _( "This destination can't be reached." ) );
                return;
            }

            if( npcselect == follower_count ) {
                for( npc *them : followers ) {
                    them->goto_to_this_pos = here.get_abs( *p );
                }
                yell_msg = _( "Everyone move there!" );
            } else {
                followers[npcselect]->goto_to_this_pos = here.get_abs( *p );
                yell_msg = string_format( _( "Move there, %s!" ), followers[npcselect]->get_name() );
            }
            break;
        }
        case NPC_CHAT_FOLLOW: {
            const int npcselect = npc_select_menu( guards, _( "Who should follow you?" ) );
            if( npcselect < 0 ) {
                return;
            }
            if( npcselect == guard_count ) {
                for( npc *them : guards ) {
                    talk_function::stop_guard( *them );
                }
                yell_msg = _( "Everyone follow me!" );
            } else {
                talk_function::stop_guard( *guards[npcselect] );
                yell_msg = string_format( _( "Follow me, %s!" ), guards[npcselect]->get_name() );
            }
            break;
        }
        case NPC_CHAT_AWAKE:
            for( npc *them : followers ) {
                talk_function::wake_up( *them );
            }
            yell_msg = _( "Stay awake!" );
            break;
        case NPC_CHAT_MOUNT:
            for( npc *them : followers ) {
                if( them->has_effect( effect_riding ) || them->is_hallucination() ) {
                    continue;
                }
                talk_function::find_mount( *them );
            }
            yell_msg = _( "Mount up!" );
            break;
        case NPC_CHAT_DISMOUNT:
            for( npc *them : followers ) {
                if( them->has_effect( effect_riding ) ) {
                    them->npc_dismount();
                }
            }
            yell_msg = _( "Dismount!" );
            break;
        case NPC_CHAT_DANGER:
            for( npc *them : followers ) {
                them->rules.set_danger_overrides();
            }
            yell_msg = _( "We're in danger.  Stay awake, stay close, don't go wandering off, "
                          "and don't open any doors." );
            break;
        case NPC_CHAT_CLEAR_OVERRIDES:
            for( npc *p : followers ) {
                talk_function::clear_overrides( *p );
            }
            yell_msg = _( "As you were." );
            break;
        case NPC_CHAT_ORDERS:
            npc_temp_orders_menu( followers );
            break;
        case NPC_CHAT_ANIMAL_VEHICLE_FOLLOW:
            assign_veh_to_follow();
            break;
        case NPC_CHAT_ANIMAL_VEHICLE_STOP_FOLLOW:
            tell_veh_stop_following();
            break;
        case NPC_CHAT_COMMAND_MAGIC_VEHICLE_FOLLOW:
            tell_magic_veh_to_follow();
            break;
        case NPC_CHAT_COMMAND_MAGIC_VEHICLE_STOP_FOLLOW:
            tell_magic_veh_stop_following();
            break;
        case NPC_CHAT_ACTIVITIES: {
            const int activity = npc_activities_menu();
            if( activity == UILIST_CANCEL ) {
                return;
            }

            std::vector<int> npcs_selected;

            if( available_for_activities_count == 1 ) {
                npcs_selected.push_back( 0 );
            } else {
                std::vector<Character *> clist( available_for_activities.begin(), available_for_activities.end() );
                npcs_selected = npcs_select_menu( clist, _( "Who should we assign?" ), nullptr );
            }

            for( int i : npcs_selected ) {

                npc *selected_npc = available_for_activities[i];

                switch( activity ) {
                    case NPC_CHAT_ACTIVITIES_MOVE_LOOT: {
                        talk_function::sort_loot( *selected_npc );
                        break;
                    }
                    case NPC_CHAT_ACTIVITIES_BUTCHERY: {
                        talk_function::do_butcher( *selected_npc );
                        break;
                    }
                    case NPC_CHAT_ACTIVITIES_CHOP_PLANKS: {
                        talk_function::do_chop_plank( *selected_npc );
                        break;
                    }
                    case NPC_CHAT_ACTIVITIES_CHOP_TREES: {
                        talk_function::do_chop_trees( *selected_npc );
                        break;
                    }
                    case NPC_CHAT_ACTIVITIES_CONSTRUCTION: {
                        talk_function::do_construction( *selected_npc );
                        break;
                    }
                    case NPC_CHAT_ACTIVITIES_CRAFT: {
                        talk_function::do_craft( *selected_npc );
                        break;
                    }
                    case NPC_CHAT_ACTIVITIES_DISASSEMBLY: {
                        talk_function::do_disassembly( *selected_npc );
                        break;
                    }
                    case NPC_CHAT_ACTIVITIES_FARMING: {
                        talk_function::do_farming( *selected_npc );
                        break;
                    }
                    case NPC_CHAT_ACTIVITIES_FISHING: {
                        talk_function::do_fishing( *selected_npc );
                        break;
                    }
                    case NPC_CHAT_ACTIVITIES_READ_REPEATEDLY: {
                        talk_function::do_read_repeatedly( *selected_npc );
                        break;
                    }
                    case NPC_CHAT_ACTIVITIES_MINING: {
                        talk_function::do_mining( *selected_npc );
                        break;
                    }
                    case NPC_CHAT_ACTIVITIES_MOPPING: {
                        talk_function::do_mopping( *selected_npc );
                        break;
                    }
                    case NPC_CHAT_ACTIVITIES_VEHICLE_DECONSTRUCTION: {
                        talk_function::do_vehicle_deconstruct( *selected_npc );
                        break;
                    }
                    case NPC_CHAT_ACTIVITIES_VEHICLE_REPAIR: {
                        talk_function::do_vehicle_repair( *selected_npc );
                        break;
                    }
                    case NPC_CHAT_ACTIVITIES_UNASSIGN: {
                        talk_function::revert_activity( *selected_npc );
                        break;
                    }
                    default:
                        break;
                }
            }
            break;
        }
        default:
            return;
    }

    if( !yell_msg.empty() ) {
        message = string_format( _( "\"%s\"" ), yell_msg );
    }
    if( !message.empty() ) {
        add_msg( _( "You yell %s" ), message );
        u.shout( string_format( _( "%s yelling %s" ), u.disp_name(), message ), is_order );
    }
    if( !emote_msg.empty() ) {
        add_msg( emote_msg );
    }

    u.mod_moves( -u.get_speed() );
}

void npc::handle_sound( const sounds::sound_t spriority, const std::string &description,
                        int heard_volume, const tripoint_bub_ms &spos )
{
    const map &here = get_map();
    const tripoint_abs_ms s_abs_pos = here.get_abs( spos );
    const tripoint_abs_ms my_abs_pos = pos_abs();

    add_msg_debug( debugmode::DF_NPC,
                   "%s heard '%s', priority %d at volume %d from %d:%d, my pos %d:%d",
                   disp_name(), description, static_cast<int>( spriority ), heard_volume,
                   s_abs_pos.x(), s_abs_pos.y(), my_abs_pos.x(), my_abs_pos.y() );

    Character &player_character = get_player_character();
    bool player_ally = player_character.pos_bub() == spos && is_player_ally();
    Character *const sound_source = get_creature_tracker().creature_at<Character>( spos );
    bool npc_ally = sound_source && sound_source->is_npc() && is_ally( *sound_source );

    if( ( player_ally || npc_ally ) && spriority == sounds::sound_t::order ) {
        say( chat_snippets().snip_acknowledged.translated() );
    }

    if( sees( here, spos ) || is_hallucination() ) {
        return;
    }
    // ignore low priority sounds if the NPC "knows" it came from a friend.
    // TODO: NPC will need to respond to talking noise eventually
    // but only for bantering purposes, not for investigating.
    if( spriority < sounds::sound_t::alarm ) {
        if( player_ally ) {
            add_msg_debug( debugmode::DF_NPC, "Allied NPC ignored same faction %s", get_name() );
            return;
        }
        if( npc_ally ) {
            add_msg_debug( debugmode::DF_NPC, "NPC ignored same faction %s", get_name() );
            return;
        }
    }
    // discount if sound source is player, or seen by player,
    // and listener is friendly and sound source is combat or alert only.
    if( spriority < sounds::sound_t::alarm && player_character.sees( here, spos ) ) {
        if( is_player_ally() ) {
            add_msg_debug( debugmode::DF_NPC, "NPC %s ignored low priority noise that player can see",
                           get_name() );
            return;
            // discount if sound source is player, or seen by player,
            // listener is neutral and sound type is worth investigating.
        } else if( spriority < sounds::sound_t::destructive_activity &&
                   get_attitude_group( get_attitude() ) != attitude_group::hostile ) {
            return;
        }
    }
    // patrolling guards will investigate more readily than stationary NPCS
    int investigate_dist = 10;
    if( mission == NPC_MISSION_GUARD_ALLY || mission == NPC_MISSION_GUARD_PATROL ) {
        investigate_dist = 50;
    }
    if( rules.has_flag( ally_rule::ignore_noise ) ) {
        investigate_dist = 0;
    }
    if( ai_cache.total_danger < 1.0f ) {
        if( spriority == sounds::sound_t::movement && !in_vehicle ) {
            warn_about( "movement_noise", rng( 1, 10 ) * 1_minutes, description );
        } else if( spriority > sounds::sound_t::movement ) {
            if( ( spriority == sounds::sound_t::speech || spriority == sounds::sound_t::alert ||
                  spriority == sounds::sound_t::order ) && sound_source &&
                !has_faction_relationship( *sound_source, npc_factions::relationship::knows_your_voice ) ) {
                warn_about( "speech_noise", rng( 1, 10 ) * 1_minutes );
            } else if( spriority > sounds::sound_t::activity ) {
                warn_about( "combat_noise", rng( 1, 10 ) * 1_minutes );
            }
            bool should_check = rl_dist( pos_bub(), spos ) < investigate_dist;
            if( should_check ) {
                const zone_manager &mgr = zone_manager::get_manager();
                // NOLINTNEXTLINE(bugprone-branch-clone)
                if( mgr.has( zone_type_NPC_NO_INVESTIGATE, s_abs_pos, fac_id ) ) {
                    should_check = false;
                } else if( mgr.has( zone_type_NPC_INVESTIGATE_ONLY, my_abs_pos, fac_id ) &&
                           !mgr.has( zone_type_NPC_INVESTIGATE_ONLY, s_abs_pos, fac_id ) ) {
                    should_check = false;
                }
            }
            if( should_check ) {
                add_msg_debug( debugmode::DF_NPC, "%s added noise at pos %d:%d", get_name(),
                               s_abs_pos.x(), s_abs_pos.y() );
                dangerous_sound temp_sound;
                temp_sound.abs_pos = s_abs_pos;
                temp_sound.volume = heard_volume;
                temp_sound.type = spriority;
                if( !ai_cache.sound_alerts.empty() ) {
                    if( ai_cache.sound_alerts.back().abs_pos != s_abs_pos ) {
                        ai_cache.sound_alerts.push_back( temp_sound );
                    }
                } else {
                    ai_cache.sound_alerts.push_back( temp_sound );
                }
            }
        }
    }
}

void avatar::talk_to( std::unique_ptr<talker> talk_with, bool radio_contact,
                      bool is_computer, bool is_not_conversation, const std::string &debug_topic )
{
    const bool has_mind_control = has_trait( trait_DEBUG_MIND_CONTROL );
    const bool force_topic = !debug_topic.empty();
    if( !talk_with->will_talk_to_u( *this, has_mind_control || force_topic ) ) {
        return;
    }
    dialogue d( get_talker_for( *this ), std::move( talk_with ), {} );
    d.by_radio = radio_contact;
    dialogue_by_radio = radio_contact;
    d.actor( true )->check_missions();
    for( mission *&mission : d.actor( true )->assigned_missions() ) {
        if( mission->get_assigned_player_id() == getID() ) {
            d.missions_assigned.push_back( mission );
        }
    }
    if( !force_topic ) {
        for( const std::string &topic_id : d.actor( true )->get_topics( radio_contact ) ) {
            d.add_topic( topic_id );
        }
        for( const std::string &topic_id : d.actor( true )->get_topics( radio_contact ) ) {
            d.add_topic( topic_id );
        }
    } else {
        d.add_topic( debug_topic );
    }
    dialogue_window d_win;
    d_win.is_computer = is_computer;
    d_win.is_not_conversation = is_not_conversation;
    // Main dialogue loop
    do {
        d.actor( true )->update_missions( d.missions_assigned );
        const talk_topic next = d.opt( d_win, d.topic_stack.back() );
        if( next.id == "TALK_NONE" ) {
            int cat = topic_category( d.topic_stack.back() );
            do {
                d.topic_stack.pop_back();
            } while( cat != -1 && topic_category( d.topic_stack.back() ) == cat );
        }
        if( next.id == "TALK_DONE" || d.topic_stack.empty() ) {
            npc *npc_actor = d.actor( true )->get_npc();
            if( npc_actor ) {
                if( npc_actor->myclass->bye_message_override.empty() ) {
                    d.actor( true )->say( npc_actor->chat_snippets().snip_bye.translated() );
                } else {
                    d.actor( true )->say( npc_actor->myclass->bye_message_override.translated() );
                }
            }
            d.done = true;
        } else if( next.id != "TALK_NONE" ) {
            d.add_topic( next );
        }
    } while( !d.done );

    if( activity.id() == ACT_AIM && !has_weapon() ) {
        cancel_activity();
        // don't query certain activities that are started from dialogue
    } else if( activity.id() == ACT_TRAIN || activity.id() == ACT_WAIT_NPC ||
               activity.id() == ACT_SOCIALIZE || activity.index == d.actor( true )->getID().get_value() ) {
        return;
    }

    if( uistate.distraction_conversation &&
        !d.actor( true )->has_effect( effect_under_operation, bodypart_str_id::NULL_ID() ) ) {
        g->cancel_activity_or_ignore_query( distraction_type::talked_to,
                                            string_format( _( "%s talked to you." ),
                                                    d.actor( true )->disp_name() ) );
    }
}

std::string dialogue::dynamic_line( const talk_topic &the_topic )
{
    if( !the_topic.item_type.is_null() ) {
        cur_item = the_topic.item_type;
    }

    // For compatibility
    const std::string &topic = the_topic.id;
    const auto iter = json_talk_topics.find( topic );
    if( iter != json_talk_topics.end() ) {
        std::string line = iter->second.get_dynamic_line( *this );
        if( !line.empty() ) {
            return line;
        }
    }

    if( topic == "TALK_NPC_NOFACE" ) {
        return string_format( _( "&%s stays silent." ), actor( true )->disp_name() );
    }

    if( topic == "TALK_NOFACE" ) {
        return _( "&You can't talk without your face." );
    } else if( topic == "TALK_DEAF" ) {
        return _( "&You are deaf and can't talk." );

    } else if( topic == "TALK_DEAF_ANGRY" ) {
        return string_format(
                   _( "&You are deaf and can't talk.  When you don't respond, %s becomes angry!" ),
                   actor( true )->disp_name() );
    } else if( topic == "TALK_MUTE" ) {
        return _( "&You are mute and can't talk." );

    } else if( topic == "TALK_MUTE_ANGRY" ) {
        return string_format(
                   _( "&You are mute and can't talk.  When you don't respond, %s becomes angry!" ),
                   actor( true )->disp_name() );
    } else if( topic == "TALK_CHURL" ) {
        return string_format(
                   _( "&Thou art but a lowley churl and ye know not this newe tongue.  %s seems unable to understand what you're saying." ),
                   actor( true )->disp_name() );

    } else if( topic == "TALK_CHURL_ANGRY" ) {
        return string_format(
                   _( "&Thou art but a lowley churl and ye know not this newe tongue.  Unable to understand your dialect, %s becomes angry!" ),
                   actor( true )->disp_name() );
    } else if( topic == "TALK_CHURL_TRADE" ) {
        return string_format(
                   _( "&Thou art but a lowley churl wyth litel understonding of this newe langage, yet %s can understand you and seems willing to trade!" ),
                   actor( true )->disp_name() );
    }
    avatar &player_character = get_avatar();
    if( topic == "TALK_SEDATED" ) {
        return string_format( _( "%1$s is sedated and can't be moved or woken up until the "
                                 "medication or sedation wears off.\nYou estimate it will wear "
                                 "off in %2$s." ),
                              actor( true )->disp_name(),
                              to_string_approx( player_character.estimate_effect_dur( skill_firstaid,
                                                effect_narcosis, 90_minutes, 60_minutes, 6,
                                                *actor( true )->get_npc() ) ) );
    }

    // Those topics are handled by the mission system, see there.
    static const std::unordered_set<std::string> mission_topics = { {
            "TALK_MISSION_DESCRIBE", "TALK_MISSION_DESCRIBE_URGENT",
            "TALK_MISSION_OFFER", "TALK_MISSION_ACCEPTED",
            "TALK_MISSION_REJECTED", "TALK_MISSION_ADVICE", "TALK_MISSION_INQUIRE",
            "TALK_MISSION_SUCCESS", "TALK_MISSION_SUCCESS_LIE", "TALK_MISSION_FAILURE"
        }
    };
    if( mission_topics.count( topic ) > 0 ) {
        mission *miss = actor( true )->selected_mission();
        if( miss == nullptr ) {
            return "mission_selected == nullptr; BUG!  (npctalk.cpp:dynamic_line)";
        }
        const mission_type &type = miss->get_type();
        // TODO: make it a member of the mission class, maybe at mission instance specific data
        const std::string &ret = miss->dialogue_for_topic( topic );
        if( ret.empty() ) {
            debugmsg( "Bug in npctalk.cpp:dynamic_line.  Wrong mission_id(%s) or topic(%s)",
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
        npc *guy = actor( true )->get_npc();
        if( guy->myclass->bye_message_override.empty() ) {
            return guy->chat_snippets().snip_bye.translated();
        } else {
            return guy->myclass->bye_message_override.translated();
        }
    } else if( topic == "TALK_TRAIN" ) {
        if( !player_character.backlog.empty() && player_character.backlog.front().id() == ACT_TRAIN ) {
            return _( "Shall we resume?" );
        } else if( actor( true )->skills_offered_to( *actor( false ) ).empty() &&
                   actor( true )->styles_offered_to( *actor( false ) ).empty() &&
                   actor( true )->spells_offered_to( *actor( false ) ).empty() &&
                   actor( true )->proficiencies_offered_to( *actor( false ) ).empty() ) {
            return _( "Sorry, but it doesn't seem I have anything to teach you." );
        } else {
            return _( "Here's what I can teach you…" );
        }
    } else if( topic == "TALK_TRAIN_NPC" ) {
        if( actor( false )->skills_offered_to( *actor( true ) ).empty() &&
            actor( false )->styles_offered_to( *actor( true ) ).empty() &&
            actor( false )->spells_offered_to( *actor( true ) ).empty() &&
            actor( false )->proficiencies_offered_to( *actor( true ) ).empty() ) {
            return _( "Sorry, there's nothing I can learn from you." );
        } else {
            return _( "Sure, I'm all ears." );
        }
    } else if( topic == "TALK_TRAIN_SEMINAR" ) {
        return _( "What do you want me to teach?" );
    } else if( topic == "TALK_HOW_MUCH_FURTHER" ) {
        return actor( true )->distance_to_goal();
    } else if( topic == "TALK_DESCRIBE_MISSION" ) {
        return actor( true )->get_job_description();
    } else if( topic == "TALK_SHOUT" ) {
        actor( false )->shout();
        if( actor( false )->is_deaf() ) {
            return _( "&You yell, but can't hear yourself." );
        } else {
            if( actor( false )->is_mute() ) {
                return _( "&You yell, but can't form words." );
            } else {
                return _( "&You yell." );
            }
        }
    } else if( topic == "TALK_SIZE_UP" ) {
        return actor( true )->evaluation_by( *actor( false ) );
    } else if( topic == "TALK_ASSESS_PERSON" ) {
        return actor( true )->view_personality_traits();
    } else if( topic == "TALK_LOOK_AT" ) {
        if( actor( false )->can_see() ) {
            return "&" + actor( true )->short_description();
        } else {
            return string_format( _( "&You're blind and can't look at %s." ), actor( true )->disp_name() );
        }
    } else if( topic == "TALK_OPINION" ) {
        return "&" + actor( true )->opinion_text();
    } else if( topic == "TALK_MIND_CONTROL" ) {
        if( actor( true )->enslave_mind() ) {
            return _( "YES, MASTER!" );
        }
    }

    debugmsg( "I don't know what to say for %s. (BUG (npctalk.cpp:dynamic_line))", topic );
    return "";
}

void dialogue::apply_speaker_effects( const talk_topic &the_topic )
{
    const std::string &topic = the_topic.id;
    const auto iter = json_talk_topics.find( topic );
    if( iter == json_talk_topics.end() ) {
        return;
    }
    for( json_dynamic_line_effect &npc_effect : iter->second.get_speaker_effects() ) {
        if( npc_effect.test_condition( *this ) ) {
            npc_effect.apply( *this );
        }
    }
}

talk_response &dialogue::add_response( const std::string &text, const std::string &r,
                                       const bool first )
{
    talk_response result = talk_response();
    result.truetext = no_translation( text );
    result.truefalse_condition = []( const_dialogue const & ) {
        return true;
    };
    result.success.next_topic = talk_topic( r );
    if( first ) {
        add_gen_response( result, true );
        return responses.front();
    } else {
        add_gen_response( result, false );
        return responses.back();
    }
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
                                       const talkfunction_ptr &effect_success, const bool first )
{
    talk_response &result = add_response( text, r, first );
    result.success.set_effect( effect_success );
    return result;
}

talk_response &dialogue::add_response( const std::string &text, const std::string &r,
                                       const std::function<void( npc & )> &effect_success,
                                       dialogue_consequence consequence, const bool first )
{
    talk_response &result = add_response( text, r, first );
    result.success.set_effect_consequence( effect_success, consequence );
    return result;
}

talk_response &dialogue::add_response( const std::string &text, const std::string &r,
                                       mission *miss, const bool first )
{
    if( miss == nullptr ) {
        debugmsg( "tried to select null mission" );
    }
    talk_response &result = add_response( text, r, first );
    result.mission_selected = miss;
    return result;
}

talk_response &dialogue::add_response( const std::string &text, const std::string &r,
                                       const skill_id &skill, const bool first )
{
    talk_response &result = add_response( text, r, first );
    result.skill = skill;
    return result;
}

talk_response &dialogue::add_response( const std::string &text, const std::string &r,
                                       const proficiency_id &proficiency, const bool first )
{
    talk_response &result = add_response( text, r, first );
    result.proficiency = proficiency;
    return result;
}

talk_response &dialogue::add_response( const std::string &text, const std::string &r,
                                       const spell_id &sp, const bool first )
{
    talk_response &result = add_response( text, r, first );
    result.dialogue_spell = sp;
    return result;
}

talk_response &dialogue::add_response( const std::string &text, const std::string &r,
                                       const martialart &style, const bool first )
{
    talk_response &result = add_response( text, r, first );
    result.style = style.id;
    return result;
}

talk_response &dialogue::add_response( const std::string &text, const std::string &r,
                                       const itype_id &item_type, const bool first )
{
    if( item_type.is_null() ) {
        debugmsg( "explicitly specified null item" );
    }

    talk_response &result = add_response( text, r, first );
    result.success.next_topic.item_type = item_type;
    return result;
}

void dialogue::gen_responses( const talk_topic &the_topic )
{
    responses.clear();
    response_condition_exists.clear();
    response_condition_eval.clear();

    const auto iter = json_talk_topics.find( the_topic.id );
    if( iter != json_talk_topics.end() ) {
        json_talk_topic &jtt = iter->second;
        if( jtt.gen_responses( *this ) ) {
            return;
        }
    }

    Character &player_character = get_player_character();
    if( the_topic.id == "TALK_MISSION_LIST" ) {
        if( actor( true )->available_missions().size() == 1 ) {
            add_response( _( "Tell me about it." ), "TALK_MISSION_OFFER",
                          actor( true )->available_missions().front(), true );
        } else {
            for( mission *&mission : actor( true )->available_missions() ) {
                add_response( mission->get_type().tname(), "TALK_MISSION_OFFER", mission, true );
            }
        }
    } else if( the_topic.id == "TALK_MISSION_LIST_ASSIGNED" ) {
        if( missions_assigned.size() == 1 ) {
            add_response( _( "I have news." ), "TALK_MISSION_INQUIRE", missions_assigned.front() );
        } else {
            for( mission *&miss_it : missions_assigned ) {
                add_response( miss_it->get_type().tname(), "TALK_MISSION_INQUIRE", miss_it );
            }
        }
    } else if( the_topic.id == "TALK_TRAIN_NPC" ) {
        const std::vector<matype_id> &styles = actor( false )->styles_offered_to( *actor( true ) );
        const std::vector<skill_id> &skills = actor( false )->skills_offered_to( *actor( true ) );
        const std::vector<spell_id> &spells = actor( false )->spells_offered_to( *actor( true ) );
        const std::vector<proficiency_id> &profs =
            actor( false )->proficiencies_offered_to( *actor( true ) );
        if( skills.empty() && styles.empty() && spells.empty() && profs.empty() ) {
            add_response_none( _( "Oh, okay." ) );
            return;
        }
        for( const spell_id &sp : spells ) {
            const std::string &text =
                string_format( "%s: %s", _( "Spell" ), actor( false )->spell_training_text( *actor( true ), sp ) );
            if( !text.empty() ) {
                add_response( text, "TALK_TRAIN_NPC_START", sp );
            }
        }
        for( const matype_id &ma : styles ) {
            const std::string &text =
                string_format( "%s: %s", _( "Style" ), actor( false )->style_training_text( *actor( true ), ma ) );
            if( !text.empty() ) {
                add_response( text, "TALK_TRAIN_NPC_START", ma.obj() );
            }
        }
        for( const skill_id &sk : skills ) {
            const std::string &text =
                string_format( "%s: %s", _( "Skill" ), actor( false )->skill_training_text( *actor( true ), sk ) );
            if( !text.empty() && !sk->obsolete() ) {
                add_response( text, "TALK_TRAIN_NPC_START", sk );
            }
        }
        for( const proficiency_id &pr : profs ) {
            const std::string &text =
                string_format( "%s: %s", _( "Proficiency" ),
                               actor( false )->proficiency_training_text( *actor( true ), pr ) );
            if( !text.empty() ) {
                add_response( text, "TALK_TRAIN_NPC_START", pr );
            }
        }
        add_response_none( _( "Eh, never mind." ) );
    } else if( the_topic.id == "TALK_TRAIN_SEMINAR" ) {
        const std::vector<skill_id> &sklist = actor( true )->skills_teacheable();
        const std::vector<proficiency_id> &prlist = actor( true )->proficiencies_teacheable();
        const std::vector<matype_id> &malist = actor( true )->styles_teacheable();
        const std::vector<spell_id> &splist = actor( true )->spells_teacheable();
        if( sklist.empty() && prlist.empty() && malist.empty() && splist.empty() ) {
            add_response_none( _( "Oh, okay." ) );
            return;
        }
        for( const skill_id &sk : sklist ) {
            if( sk->obsolete() ) {
                continue;
            }
            const std::string &text =
                string_format( "%s: %s", _( "Skill" ), actor( true )->skill_seminar_text( sk ) );
            add_response( text, "TALK_TRAIN_SEMINAR_START", sk );
        }
        for( const proficiency_id &pr : prlist ) {
            const std::string &text =
                string_format( "%s: %s", _( "Proficiency" ), actor( true )->proficiency_seminar_text( pr ) );
            add_response( text, "TALK_TRAIN_SEMINAR_START", pr );
        }
        for( const matype_id &ma : malist ) {
            const std::string &text =
                string_format( "%s: %s", _( "Style" ), actor( true )->style_seminar_text( ma ) );
            add_response( text, "TALK_TRAIN_SEMINAR_START", ma.obj() );
        }
        for( const spell_id &sp : splist ) {
            const std::string &text =
                string_format( "%s: %s", _( "Spell" ), actor( true )->spell_seminar_text( sp ) );
            add_response( text, "TALK_TRAIN_SEMINAR_START", sp );
        }
        add_response_none( _( "Eh, never mind." ) );
    } else if( the_topic.id == "TALK_TRAIN" ) {
        if( !player_character.backlog.empty() && player_character.backlog.front().id() == ACT_TRAIN &&
            player_character.backlog.front().index == actor( true )->getID().get_value() ) {
            player_activity &backlog = player_character.backlog.front();
            const skill_id skillt( backlog.name );
            // TODO: This is potentially dangerous. A skill and a martial art
            // could have the same ident!
            if( !skillt.is_valid() ) {
                const matype_id styleid = matype_id( backlog.name );
                if( !styleid.is_valid() ) {
                    const proficiency_id profid = proficiency_id( backlog.name );
                    if( !profid.is_valid() ) {
                        const spell_id &sp_id = spell_id( backlog.name );
                        if( actor( true )->knows_spell( sp_id ) ) {
                            add_response( string_format( _( "Yes, let's resume training %s" ),
                                                         sp_id->name ), "TALK_TRAIN_START", sp_id );
                        }
                    } else {
                        add_response( string_format( _( "Yes, let's resume training %s" ),
                                                     profid->name() ), "TALK_TRAIN_START", profid );
                    }
                } else {
                    const martialart &style = styleid.obj();
                    add_response( string_format( _( "Yes, let's resume training %s" ),
                                                 style.name ), "TALK_TRAIN_START", style );
                }
            } else {
                add_response( string_format( _( "Yes, let's resume training %s" ), skillt->name() ),
                              "TALK_TRAIN_START", skillt );
            }
        }
        const std::vector<matype_id> &styles = actor( true )->styles_offered_to( *actor( false ) );
        const std::vector<skill_id> &trainable = actor( true )->skills_offered_to( *actor( false ) );
        const std::vector<spell_id> &teachable = actor( true )->spells_offered_to( *actor( false ) );
        const std::vector<proficiency_id> &proficiencies = actor( true )->proficiencies_offered_to( *actor(
                    false ) );
        if( trainable.empty() && styles.empty() && teachable.empty() && proficiencies.empty() ) {
            add_response_none( _( "Oh, okay." ) );
            return;
        }
        for( const spell_id &sp : teachable ) {
            const std::string &text =
                string_format( "%s: %s", _( "Spell" ), actor( true )->spell_training_text( *actor( false ), sp ) );
            if( !text.empty() ) {
                add_response( text, "TALK_TRAIN_START", sp );
            }
        }
        for( const matype_id &style_id : styles ) {
            const std::string &text =
                string_format( "%s: %s", _( "Style" ),
                               actor( true )->style_training_text( *actor( false ), style_id ) );
            if( !text.empty() ) {
                add_response( text, "TALK_TRAIN_START", style_id.obj() );
            }
        }
        for( const skill_id &trained : trainable ) {
            const std::string &text =
                string_format( "%s: %s", _( "Skill" ),
                               actor( true )->skill_training_text( *actor( false ), trained ) );
            if( !text.empty() && !trained->obsolete() ) {
                add_response( text, "TALK_TRAIN_START", trained );
            }
        }
        for( const proficiency_id &trained : proficiencies ) {
            const std::string &text =
                string_format( "%s: %s", _( "Proficiency" ),
                               actor( true )->proficiency_training_text( *actor( false ), trained ) );
            if( !text.empty() ) {
                add_response( text, "TALK_TRAIN_START", trained );
            }
        }
        add_response_none( _( "Eh, never mind." ) );
    } else if( the_topic.id == "TALK_HOW_MUCH_FURTHER" ) {
        add_response_none( _( "Okay, thanks." ) );
        add_response_done( _( "Let's keep moving." ) );
    }

    if( actor( false )->has_trait( trait_DEBUG_MIND_CONTROL ) && !actor( true )->is_player_ally() ) {
        add_response( _( "OBEY ME!" ), "TALK_MIND_CONTROL" );
        add_response_done( _( "Bye." ) );
    }

    if( player_character.has_trait( trait_PROF_CHURL ) && ( actor( true )->get_npc_trust() >= 0 ) &&
        ( actor( true )->get_npc_anger() <= 0 ) && ( actor( true )->int_cur() >= 9 ) &&
        !( the_topic.id == "TALK_CHURL_FRIENDLY" ) ) {
        add_response( _( "Ho there, otherwyrldly devyl!  Have yow ware for to chaffare?" ),
                      "TALK_CHURL_FRIENDLY" );
        add_response_done( _( "Farewell!" ) );
    }

    if( responses.empty() ) {
        add_response_done( _( "Bye." ) );
    }
}

static int parse_mod( const_dialogue const &d, const std::string &attribute, const int factor )
{
    return d.const_actor( true )->parse_mod( attribute, factor ) +
           d.const_actor( false )->parse_mod( attribute, factor );
}

static int total_price( const_talker const &seller, const itype_id &item_type )
{
    int price = 0;
    item tmp( item_type );

    if( tmp.count_by_charges() ) {
        tmp.charges =  seller.charges_of( item_type );
        price = tmp.price( true );
    } else {
        std::vector<const item *> items = seller.const_items_with( [&item_type]( const item & e ) {
            return item_type == e.type->get_id();
        } );
        for( const item *it : items ) {
            price += it->price( true );
        }
    }
    return price;
}

int talk_trial::calc_chance( const_dialogue const &d ) const
{
    if( d.const_actor( false )->has_trait( trait_DEBUG_MIND_CONTROL ) ) {
        return 100;
    }
    int chance = difficulty;
    switch( type ) {
        case NUM_TALK_TRIALS:
            dbg( D_ERROR ) << "called calc_chance with invalid talk_trial value: " << type;
            break;
        case TALK_TRIAL_NONE:
            chance = 100;
            break;
        case TALK_TRIAL_SKILL_CHECK:
            chance = d.const_actor( false )->get_skill_level( skill_id( skill_required ) ) >= difficulty ? 100 :
                     0;
            break;
        case TALK_TRIAL_CONDITION:
            if( condition ) {
                chance = condition( d ) ? 100 : 0;
            } else {
                chance = 0;
            }
            break;
        case TALK_TRIAL_LIE:
            chance += d.const_actor( false )->trial_chance_mod( "lie" ) + d.const_actor(
                          true )->trial_chance_mod( "lie" );
            break;
        case TALK_TRIAL_PERSUADE:
            chance += d.const_actor( false )->trial_chance_mod( "persuade" ) +
                      d.const_actor( true )->trial_chance_mod( "persuade" );
            break;
        case TALK_TRIAL_INTIMIDATE:
            chance += d.const_actor( false )->trial_chance_mod( "intimidate" ) +
                      d.const_actor( true )->trial_chance_mod( "intimidate" );
            break;
    }
    for( const auto &this_mod : modifiers ) {
        chance += parse_mod( d, this_mod.first, this_mod.second );
    }

    return std::max( 0, std::min( 100, chance ) );
}

bool talk_trial::roll( dialogue &d ) const
{
    if( type == TALK_TRIAL_NONE || d.actor( false )->has_trait( trait_DEBUG_MIND_CONTROL ) ) {
        return true;
    }
    const int chance = calc_chance( d );
    const bool success = rng( 0, 99 ) < chance;
    const bool speech_trial = type == TALK_TRIAL_PERSUADE || type == TALK_TRIAL_INTIMIDATE ||
                              type == TALK_TRIAL_LIE;
    if( speech_trial && d.actor( false )->get_character() ) {
        Character &u = *d.actor( false )->get_character();
        if( success ) {
            u.practice( skill_speech, ( 100 - chance ) / 10 );
        } else {
            u.practice( skill_speech, ( 100 - chance ) / 7 );
        }
    }
    return success;
}

int topic_category( const talk_topic &the_topic )
{
    const std::string &topic = the_topic.id;
    // TODO: ideally, this would be a property of the topic itself.
    // How this works: each category has a set of topics that belong to it, each set is checked
    // for the given topic and if a set contains, the category number is returned.
    static const std::unordered_set<std::string> topic_1 = { {
            "TALK_MISSION_START", "TALK_MISSION_DESCRIBE", "TALK_MISSION_OFFER",
            "TALK_MISSION_ACCEPTED", "TALK_MISSION_REJECTED", "TALK_MISSION_ADVICE",
            "TALK_MISSION_INQUIRE", "TALK_MISSION_SUCCESS", "TALK_MISSION_SUCCESS_LIE",
            "TALK_MISSION_FAILURE", "TALK_MISSION_REWARD", "TALK_MISSION_END",
            "TALK_MISSION_DESCRIBE_URGENT"
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
            "TALK_TRAIN", "TALK_TRAIN_START", "TALK_TRAIN_FORCE",
            "TALK_TRAIN_NPC_START", "TALK_TRAIN_NPC_FORCE"
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
            "TALK_SIZE_UP", "TALK_ASSESS_PERSON", "TALK_LOOK_AT", "TALK_OPINION", "TALK_SHOUT"
        }
    };
    if( topic_99.count( topic ) > 0 ) {
        return 99;
    }
    return -1; // Not grouped with other topics
}

void parse_tags( std::string &phrase, const Character &u, const Creature &me,
                 const itype_id &item_type )
{
    const_dialogue d( get_const_talker_for( u ), get_const_talker_for( me ) );
    parse_tags( phrase, u, me, d, item_type );
}

void parse_tags( std::string &phrase, const Character &u, const Creature &me,
                 const_dialogue const &d,
                 const itype_id &item_type )
{
    parse_tags( phrase, *get_const_talker_for( u ), *get_const_talker_for( me ), d, item_type );
}

void parse_tags( std::string &phrase, const_talker const &u, const_talker const &me,
                 const_dialogue const &d,
                 const itype_id &item_type )
{
    phrase = SNIPPET.expand( phrase );

    const Character *u_chr = u.get_const_character();
    const Character *me_chr = me.get_const_character();
    size_t fa;
    size_t fb;
    size_t fa_;
    std::string tag;
    do {
        fa = phrase.find( '<' );
        fb = phrase.find( '>' );
        // Skip the <color_XXX> and </color> tag
        while( fa != std::string::npos && ( phrase.compare( fa + 1, 6, "color_" ) == 0 ||
                                            phrase.compare( fa + 1, 7, "/color>" ) == 0 ) ) {
            if( phrase.compare( fa + 1, 6, "color_" ) == 0 ) {
                fa = phrase.find( '<', fa + 7 );
            } else { // phrase.compare(fa + 1, 7, "/color>") == 0
                fa = phrase.find( '<', fa + 8 );
            }
            fb = phrase.find( '>', fa );
        }
        if( fa != std::string::npos ) {
            size_t nest = 0;
            fa_ = phrase.find( '<', fa + 1 );
            while( fa_ < fb && fa_ != std::string::npos ) {
                nest++;
                fa_ = phrase.find( '<', fa_ + 1 );
            }
            while( nest > 0 && fb != std::string::npos ) {
                nest--;
                fb = phrase.find( '>', fb + 1 );
            }
        }
        int l = fb - fa + 1;
        if( fa != std::string::npos && fb != std::string::npos ) {
            tag = phrase.substr( fa, fb - fa + 1 );
        } else {
            return;
        }

        const item_location u_weapon = u_chr ? u_chr->get_wielded_item() : item_location();
        const item_location me_weapon = me_chr ? me_chr->get_wielded_item() : item_location();
        // Special, dynamic tags go here
        if( tag == "<yrwp>" ) {
            phrase.replace( fa, l, remove_color_tags( u_weapon->tname() ) );
        } else if( tag == "<mywp>" ) {
            if( me_chr && !me_chr->is_armed() ) {
                phrase.replace( fa, l, _( "fists" ) );
            } else {
                phrase.replace( fa, l, remove_color_tags( me_weapon->tname() ) );
            }
        } else if( tag == "<u_name>" ) {
            phrase.replace( fa, l, u.get_name() );
        } else if( tag == "<npc_name>" ) {
            phrase.replace( fa, l, me.get_name() );
        } else if( tag == "<ammo>" ) {
            if( !me_weapon || !me_weapon->is_gun() ) {
                phrase.replace( fa, l, _( "BADAMMO" ) );
            } else {
                phrase.replace( fa, l, me_weapon->ammo_current()->nname( 1 ) );
            }
        } else if( tag == "<current_activity>" ) {
            std::string activity_name;
            const npc *guy = dynamic_cast<const npc *>( me_chr );
            if( guy && guy->current_activity_id ) {
                activity_name = guy->get_current_activity();
            } else {
                activity_name = _( "doing this and that" );
            }
            phrase.replace( fa, l, activity_name );
        } else if( tag == "<punc>" ) {
            switch( rng( 0, 2 ) ) {
                case 0:
                    phrase.replace( fa, l, pgettext( "punctuation", "." ) );
                    break;
                case 1:
                    phrase.replace( fa, l, pgettext( "punctuation", "…" ) );
                    break;
                case 2:
                    phrase.replace( fa, l, pgettext( "punctuation", "!" ) );
                    break;
            }
        } else if( tag == "<mypronoun>" ) {
            std::string npcstr = me.is_male() ? pgettext( "npc", "He" ) : pgettext( "npc", "She" );
            phrase.replace( fa, l, npcstr );
        } else if( tag == "<mypossesivepronoun>" ) {
            std::string npcstr = me.is_male() ? pgettext( "npc", "his" ) : pgettext( "npc", "her" );
            phrase.replace( fa, l, npcstr );
        } else if( tag == "<topic_item>" ) {
            phrase.replace( fa, l, item::nname( item_type, 2 ) );
        } else if( tag == "<topic_item_price>" ) {
            item tmp( item_type );
            phrase.replace( fa, l, format_money( tmp.price( true ) ) );
        } else if( tag == "<topic_item_my_total_price>" ) {
            int price = total_price( me, item_type );
            phrase.replace( fa, l, format_money( price ) );
        } else if( tag == "<topic_item_your_total_price>" ) {
            int price = total_price( u, item_type );
            phrase.replace( fa, l, format_money( price ) );
        } else if( tag == "<interval>" ) {
            const npc *guy = dynamic_cast<const npc *>( me_chr );
            std::string restock_interval = guy ? guy->get_restock_interval() : _( "a few days" );
            phrase.replace( fa, l, restock_interval );
        } else if( tag.find( "<u_val:" ) == 0 ) {
            //adding a user variable to the string
            std::string var = tag.substr( tag.find( ':' ) + 1 );
            // remove the trailing >
            var.pop_back();
            // resolve nest
            parse_tags( var, u, me, d, item_type );
            phrase.replace( fa, l, u.get_value( var ).to_string() );
        } else if( tag.find( "<npc_val:" ) == 0 ) {
            //adding a npc variable to the string
            std::string var = tag.substr( tag.find( ':' ) + 1 );
            // remove the trailing >
            var.pop_back();
            // resolve nest
            parse_tags( var, u, me, d, item_type );
            phrase.replace( fa, l, me.get_value( var ).to_string() );
        } else if( tag.find( "<global_val:" ) == 0 ) {
            //adding a global variable to the string
            std::string var = tag.substr( tag.find( ':' ) + 1 );
            // remove the trailing >
            var.pop_back();
            // resolve nest
            parse_tags( var, u, me, d, item_type );
            global_variables &globvars = get_globals();
            phrase.replace( fa, l, globvars.get_global_value( var ).to_string() );
        } else if( tag.find( "<context_val:" ) == 0 ) {
            //adding a context variable to the string requires dialogue to exist
            std::string var = tag.substr( tag.find( ':' ) + 1 );
            // remove the trailing >
            var.pop_back();
            // resolve nest
            parse_tags( var, u, me, d, item_type );
            phrase.replace( fa, l, d.get_value( var ).to_string() );
        } else if( tag.find( "<item_name:" ) == 0 ) {
            //embedding an items name in the string
            std::string var = tag.substr( tag.find( ':' ) + 1 );
            // remove the trailing >
            var.pop_back();
            // resolve nest
            parse_tags( var, u, me, d, item_type );
            // attempt to cast as an item
            phrase.replace( fa, l, itype_id( var )->nname( 1 ) );
        } else if( tag.find( "<item_description:" ) == 0 ) {
            //embedding an items name in the string
            std::string var = tag.substr( tag.find( ':' ) + 1 );
            // remove the trailing >
            var.pop_back();
            // resolve nest
            parse_tags( var, u, me, d, item_type );
            // attempt to cast as an item
            phrase.replace( fa, l, itype_id( var )->description.translated() );
        } else if( tag.find( "<trait_name:" ) == 0 ) {
            //embedding an items name in the string
            std::string var = tag.substr( tag.find( ':' ) + 1 );
            // remove the trailing >
            var.pop_back();
            // resolve nest
            parse_tags( var, u, me, d, item_type );
            // attempt to cast as an item
            phrase.replace( fa, l, trait_id( var )->name() );
        } else if( tag.find( "<trait_description:" ) == 0 ) {
            //embedding an items name in the string
            std::string var = tag.substr( tag.find( ':' ) + 1 );
            // remove the trailing >
            var.pop_back();
            // resolve nest
            parse_tags( var, u, me, d, item_type );
            // attempt to cast as an item
            phrase.replace( fa, l, me_chr->mutation_desc( trait_id( var ) ) );
        } else if( tag.find( "<spell_name:" ) == 0 ) {
            //embedding an items name in the string
            std::string var = tag.substr( tag.find( ':' ) + 1 );
            // remove the trailing >
            var.pop_back();
            // resolve nest
            parse_tags( var, u, me, d, item_type );
            // attempt to cast as an item
            phrase.replace( fa, l, spell_id( var )->name.translated() );
        } else if( tag.find( "<spell_description:" ) == 0 ) {
            //embedding an items name in the string
            std::string var = tag.substr( tag.find( ':' ) + 1 );
            // remove the trailing >
            var.pop_back();
            // resolve nest
            parse_tags( var, u, me, d, item_type );
            // attempt to cast as an item
            phrase.replace( fa, l, spell_id( var )->description.translated() );
        } else if( tag.find( "<keybind:" ) == 0 ) {
            //embedding an items name in the string
            std::string var = tag.substr( tag.find( ':' ) + 1 );
            // remove the trailing >
            var.pop_back();
            std::string keybind = var;

            //deal with category specific binds like <keybind:TARGET:PREV_TARGET>
            size_t pos_category_split = var.find( ':' );

            std::string category = "DEFAULTMODE";
            if( pos_category_split != std::string::npos ) {
                category = var.substr( 0, pos_category_split );
                keybind = var.substr( pos_category_split + 1 );
            }
            input_context ctxt( category );

            std::string keybind_desc;
            std::vector<input_event> keys = ctxt.keys_bound_to( keybind, -1, false, false );
            if( keys.empty() ) { // Display description for unbound keys
                keybind_desc = colorize( '<' + ctxt.get_desc( keybind ) + '>', c_red );

                if( !ctxt.is_registered_action( keybind ) ) {
                    debugmsg( "Keybind specified by <keybind:%s> is invalid/missing", var );
                }
            } else {
                keybind_desc = enumerate_as_string( keys.begin(), keys.end(), []( const input_event & k ) {
                    return colorize( '\'' + k.long_description() + '\'', c_yellow );
                }, enumeration_conjunction::or_ );
            }

            phrase.replace( fa, l, keybind_desc );
        } else if( tag.find( "<city>" ) == 0 ) {
            std::string cityname = "nowhere";
            tripoint_abs_sm abs_sub = get_map().get_abs_sub();
            const city *c = overmap_buffer.closest_city( abs_sub ).city;
            if( c != nullptr ) {
                cityname = c->name;
            }
            phrase.replace( fa, l, cityname );
        } else if( tag.find( "<time_survived>" ) == 0 ) {
            std::string time_survived;
            const time_duration survived = calendar::turn - calendar::start_of_game;
            const int minutes = to_minutes<int>( survived ) % 60;
            const int hours = to_hours<int>( survived ) % 24;
            const int days = to_days<int>( survived );
            if( days > 0 ) {
                // NOLINTNEXTLINE(cata-translate-string-literal)
                time_survived = string_format( "%dd %dh %dm", days, hours, minutes );
            } else if( hours > 0 ) {
                // NOLINTNEXTLINE(cata-translate-string-literal)
                time_survived = string_format( "%dh %dm", hours, minutes );
            } else {
                // NOLINTNEXTLINE(cata-translate-string-literal)
                time_survived = string_format( "%dm", minutes );
            }
            phrase.replace( fa, l, time_survived );
        } else if( tag.find( "<total_kills>" ) == 0 ) {
            const std::string total_kills = std::to_string( g->get_kill_tracker().monster_kill_count() );
            phrase.replace( fa, l, total_kills );
        } else if( !tag.empty() ) {
            debugmsg( "Bad tag.  '%s' (%d - %d)", tag.c_str(), fa, fb );
            phrase.replace( fa, fb - fa + 1, "????" );
        }
    } while( fa != std::string::npos && fb != std::string::npos );
}

void dialogue::add_gen_response( const talk_response &resp, bool insert_front,
                                 bool condition_exists, bool condition_result )
{
    if( insert_front ) {
        responses.insert( responses.begin(), resp );
    } else {
        responses.push_back( resp );
    }
    response_condition_exists.emplace_back( condition_exists );
    response_condition_eval.emplace_back( condition_result );
}

void dialogue::add_topic( const std::string &topic_id )
{
    if( actor( true )->get_npc() ) {
        topic_stack.emplace_back( actor( true )->get_npc()->get_specified_talk_topic( topic_id ) );
    } else {
        topic_stack.emplace_back( topic_id );
    }
}

void dialogue::add_topic( const talk_topic &topic )
{
    if( actor( true )->get_npc() ) {
        std::string const &newid = actor( true )->get_npc()->get_specified_talk_topic( topic.id );
        topic_stack.emplace_back( newid, topic.item_type, topic.reason );
    } else {
        topic_stack.push_back( topic );
    }
}

void const_dialogue::set_value( const std::string &key, diag_value value )
{
    context[key] = std::move( value );
}

void const_dialogue::remove_value( const std::string &key )
{
    context->erase( key );
}

diag_value const &const_dialogue::get_value( const std::string &key ) const
{
    return global_variables::_common_get_value( key, context );
}

diag_value const *const_dialogue::maybe_get_value( const std::string &key ) const
{
    return global_variables::_common_maybe_get_value( key, context );
}

void const_dialogue::set_conditional( const std::string &key,
                                      const std::function<bool( const_dialogue const & )> &value )
{
    conditionals[key] = value;
}

bool const_dialogue::evaluate_conditional( const std::string &key, const_dialogue const &d ) const
{
    auto it = conditionals->find( key );
    return ( it == conditionals->end() ) ? false : it->second( d );
}

global_variables::impl_t const &const_dialogue::get_context() const
{
    return context;
}

const std::unordered_map<std::string, std::function<bool( const_dialogue const & )>>
        &const_dialogue::get_conditionals() const
{
    return conditionals;
}

void const_dialogue::amend_callstack( const std::string &value )
{
    if( !callstack.empty() ) {
        callstack += " \\ " + value;
    } else {
        callstack = value;
    }
}

std::string const_dialogue::get_callstack() const
{
    if( !callstack.empty() ) {
        return "Callstack: " + callstack;
    }
    return "";
}

const_talker *const_dialogue::const_actor( bool is_beta ) const
{
    if( !has_beta && is_beta ) {
        debugmsg( "Tried to use an invalid beta talker.  %s", get_callstack() );
    } else if( !has_alpha && !is_beta ) {
        debugmsg( "Tried to use an invalid alpha talker.  %s", get_callstack() );
    }
    return ( is_beta ? beta : alpha ).get();
}

talker *dialogue::actor( const bool is_beta ) const
{
    if( !has_beta && !has_alpha ) {
        debugmsg( "Attempted to use a dialogue with no actors!  %s", get_callstack() );
    }
    if( is_beta && !has_beta ) {
        debugmsg( "Tried to use an invalid beta talker.  %s", get_callstack() );
        // Try to avoid a crash by using the alpha if it exists
        if( has_alpha ) {
            return alpha.get();
        }
    }
    if( !is_beta && !has_alpha ) {
        debugmsg( "Tried to use an invalid alpha talker.  %s", get_callstack() );
        // Try to avoid a crash by using the beta if it exists
        if( has_beta ) {
            return beta.get();
        }
    }
    return ( is_beta ? beta : alpha ).get();
}

bool const_dialogue::has_actor( bool is_beta ) const
{
    return is_beta ? has_beta : has_alpha;
}

const_dialogue::const_dialogue( const const_dialogue &d )
    : has_beta( d.has_beta ), has_alpha( d.has_alpha )
{
    if( has_alpha ) {
        alpha = d.const_actor( false )->const_clone();
    }
    if( has_beta ) {
        beta = d.const_actor( true )->const_clone();
    }
    if( !has_alpha && !has_beta ) {
        debugmsg( "Constructed a dialogue with no actors!  %s", get_callstack() );
    }
    if( d.context.has_value() ) {
        context = *d.context;
    }
    if( d.conditionals.has_value() ) {
        conditionals = *d.conditionals;
    }
    callstack = d.callstack;
}

dialogue::dialogue( const dialogue &d ) : const_dialogue( d )
{
    if( has_alpha ) {
        alpha = d.actor( false )->clone();
    }
    if( has_beta ) {
        beta = d.actor( true )->clone();
    }
}

const_dialogue::const_dialogue( std::unique_ptr<const_talker> alpha_in,
                                std::unique_ptr<const_talker> beta_in,
                                const std::unordered_map<std::string, std::function<bool( const_dialogue const & )>> &cond,
                                global_variables::impl_t const &ctx )
    : alpha( std::move( alpha_in ) ), beta( std::move( beta_in ) ), context( ctx ), conditionals( cond )
{
    has_alpha = static_cast<bool>( alpha );
    has_beta = static_cast<bool>( beta );
}

dialogue::dialogue(
    std::unique_ptr<talker> alpha_in, std::unique_ptr<talker> beta_in,
    const std::unordered_map<std::string, std::function<bool( const_dialogue const & )>> &cond,
    global_variables::impl_t const &ctx )
    : const_dialogue( alpha_in ? alpha_in->const_clone() : nullptr,
                      beta_in ? beta_in->const_clone() : nullptr, cond, ctx ),
      alpha( std::move( alpha_in ) ), beta( std::move( beta_in ) )
{
}

talk_data talk_response::create_option_line( dialogue &d, const input_event &hotkey,
        const bool is_computer )
{
    std::string ftext;
    text = ( truefalse_condition( d ) ? truetext : falsetext ).translated();
    if( trial.type == TALK_TRIAL_NONE || trial.type == TALK_TRIAL_CONDITION ) {
        // regular dialogue
        ftext = text;
    } else if( trial.type == TALK_TRIAL_SKILL_CHECK ) {
        const Skill &req_skill = skill_id( trial.skill_required ).obj();
        ftext = string_format( pgettext( "talk option", "[%1$s %2$d/%3$d] %4$s" ),
                               req_skill.name(),
                               std::min( d.actor( false )->get_skill_level( req_skill.ident() ),
                                         trial.difficulty ),
                               trial.difficulty,
                               text );
    } else {
        // dialogue w/ a % chance to work
        //~ %1$s is translated trial type, %2$d is a number, and %3$s is the translated response text
        ftext = string_format( pgettext( "talk option", "[%1$s %2$d%%] %3$s" ),
                               trial.name(), trial.calc_chance( d ), text );
    }

    if( ignore_conditionals ) {
        ftext = colorize( ftext, c_dark_gray );
        if( !show_reason.empty() ) {
            ftext += colorize( " -- [" + show_reason + "]", c_light_red );
        }
    }

    if( d.actor( true )->get_npc() ) {
        parse_tags( ftext, *d.actor( false )->get_character(), *d.actor( true )->get_npc(), d,
                    success.next_topic.item_type );
    } else {
        parse_tags( ftext, *d.actor( false )->get_character(), *d.actor( false )->get_character(), d,
                    success.next_topic.item_type );
    }

    nc_color color;
    std::set<dialogue_consequence> consequences = get_consequences( d );
    if( consequences.count( dialogue_consequence::hostile ) > 0 ) {
        color = c_red;
    } else if( text[0] == '*' || consequences.count( dialogue_consequence::helpless ) > 0 ) {
        color = c_light_red;
    } else if( text[0] == '&' || consequences.count( dialogue_consequence::action ) > 0 ||
               is_computer ) {
        color = c_green;
    } else {
        color = c_white;
    }
    talk_data results;
    results.color = color;
    results.hotkey_desc = right_justify( hotkey.short_description(), 2 );
    results.text = ftext;
    return results;
}

std::set<dialogue_consequence> talk_response::get_consequences( dialogue &d ) const
{
    int chance = trial.calc_chance( d );
    if( chance >= 100 ) {
        return { success.get_consequence( d ) };
    } else if( chance <= 0 ) {
        return { failure.get_consequence( d ) };
    }

    return {{ success.get_consequence( d ), failure.get_consequence( d ) }};
}

bool json_talk_response::show_anyways( dialogue &d ) const
{
    return actual_response.show_always || ( actual_response.show_condition &&
                                            actual_response.show_condition( d ) );
}

dialogue_consequence talk_effect_t::get_consequence( dialogue const &d ) const
{
    if( d.actor( true )->check_hostile_response( opinion.anger ) ) {
        return dialogue_consequence::hostile;
    }
    return guaranteed_consequence;
}

const talk_topic &special_talk( const std::string &action )
{
    static const std::map<std::string, talk_topic> key_map = {{
            { "LOOK_AT", talk_topic( "TALK_LOOK_AT" ) },
            { "SIZE_UP_STATS", talk_topic( "TALK_SIZE_UP" ) },
            { "ASSESS_PERSONALITY", talk_topic( "TALK_ASSESS_PERSON" ) },
            { "CHECK_OPINION", talk_topic( "TALK_OPINION" ) },
            { "YELL", talk_topic( "TALK_SHOUT" ) },
        }
    };

    const auto iter = key_map.find( action );
    if( iter != key_map.end() ) {
        return iter->second;
    }

    static const talk_topic no_topic = talk_topic( "TALK_NONE" );
    return no_topic;
}

talk_topic dialogue::opt( dialogue_window &d_win, const talk_topic &topic )
{
    d_win.add_history_separator();

    ui_adaptor ui;
    const auto resize_cb = [&]( ui_adaptor & ui ) {
        d_win.resize( ui );
    };
    ui.on_screen_resize( resize_cb );
    resize_cb( ui );

    // Construct full line
    std::string challenge = dynamic_line( topic );
    gen_responses( topic );

    // Put quotes around challenge (unless it's an action)
    if( challenge[0] != '*' && challenge[0] != '&' ) {
        challenge = string_format( _( "\"%s\"" ), challenge );
    }

    // Parse any tags in challenge
    if( actor( true )->get_npc() ) {
        parse_tags( challenge, *actor( false )->get_character(), *actor( true )->get_npc(), *this,
                    topic.item_type );
    } else {
        parse_tags( challenge, *actor( false )->get_character(), *actor( false )->get_character(), *this,
                    topic.item_type );
    }
    challenge = uppercase_first_letter( challenge );

    d_win.clear_history_highlights();
    if( challenge[0] == '&' ) {
        // No name prepended!
        challenge = challenge.substr( 1 );
        d_win.add_to_history( challenge );
    } else if( challenge[0] == '*' ) {
        // Prepend name
        challenge = string_format( pgettext( "npc does something", "%s %s" ), actor( true )->disp_name(),
                                   challenge.substr( 1 ) );
        d_win.add_to_history( challenge );
    } else {
        npc *npc_actor = actor( true )->get_npc();
        d_win.add_to_history( challenge, d_win.is_not_conversation ? "" : actor( true )->disp_name(),
                              npc_actor ? npc_actor->basic_symbol_color() : c_red );
    }
    if( debug_mode ) {
        std::vector<std::string> dynamic_line_debug = build_debug_info( d_win, topic );
        for( auto &line : dynamic_line_debug ) {
            d_win.add_to_history( line );
        }
    }
    apply_speaker_effects( topic );

    if( responses.empty() ) {
        debugmsg( "No dialogue responses" );
        return talk_topic( "TALK_NONE" );
    }

    input_context ctxt( "DIALOGUE_CHOOSE_RESPONSE" );
    d_win.set_up_scrolling( ctxt );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "ANY_INPUT" );
    ctxt.register_action( "DEBUG_DIALOGUE_DL_CONDITIONAL" );
    ctxt.register_action( "DEBUG_DIALOGUE_RESP_CONDITIONAL" );
    ctxt.register_action( "DEBUG_DIALOGUE_DL_EFFECT" );
    ctxt.register_action( "DEBUG_DIALOGUE_RESP_EFFECT" );
    ctxt.register_action( "DEBUG_DIALOGUE_SHOW_ALL_RESPONSE" );
    ctxt.register_action( "QUIT" );
    std::vector<talk_data> response_lines;
    std::vector<input_event> response_hotkeys;
    const auto generate_response_lines = [&]() {
#if defined(__ANDROID__)
        ctxt.get_registered_manual_keys().clear();
#endif
        const hotkey_queue &queue = hotkey_queue::alphabets();
        response_lines.clear();
        response_hotkeys.clear();
        input_event evt = ctxt.first_unassigned_hotkey( queue );
        for( talk_response &response : responses ) {
            const talk_data &td = response.create_option_line( *this, evt, d_win.is_computer );
            response_lines.emplace_back( td );
            response_hotkeys.emplace_back( evt );
#if defined(__ANDROID__)
            ctxt.register_manual_key( evt.get_first_input(), td.text );
#endif
            evt = ctxt.next_unassigned_hotkey( queue, evt );
        }
        d_win.set_responses( response_lines );
    };
    generate_response_lines();

    ui.on_redraw( [&]( const ui_adaptor & ) {
        d_win.draw( d_win.is_not_conversation ? "" : actor( true )->disp_name() );
    } );

    size_t response_ind = response_hotkeys.size();
    bool okay;
    do {
        std::string action;
        do {
            if( debug_mode ) {
                d_win.set_responses_debug( build_debug_info( d_win, topic, d_win.sel_response ) );
                d_win.debug_topic_name = topic.id;
            }
            ui_manager::redraw();
            input_event evt;
            action = ctxt.handle_input();
            evt = ctxt.get_raw_input();
            d_win.handle_scrolling( action, ctxt );
            talk_topic st = special_talk( action );
            if( st.id != "TALK_NONE" ) {
                return st;
            }
            if( action == "HELP_KEYBINDINGS" ) {
                // Reallocate hotkeys as keybindings may have changed
                generate_response_lines();
            } else if( action == "CONFIRM" ) {
                response_ind = d_win.sel_response;
                //response condition must be reverified since non-selectable responses can be displayed
                if( response_condition_exists[response_ind] && ( !response_condition_eval[response_ind] &&
                        !debug_mode ) ) {
                    action = "NONE";
                }
            } else if( action == "DEBUG_DIALOGUE_DL_CONDITIONAL" ) {
                d_win.show_dynamic_line_conditionals = !d_win.show_dynamic_line_conditionals;
            } else if( action == "DEBUG_DIALOGUE_RESP_CONDITIONAL" ) {
                d_win.show_response_conditionals = !d_win.show_response_conditionals;
            } else if( action == "DEBUG_DIALOGUE_DL_EFFECT" ) {
                d_win.show_dynamic_line_effects = !d_win.show_dynamic_line_effects;
            } else if( action == "DEBUG_DIALOGUE_RESP_EFFECT" ) {
                d_win.show_response_effects = !d_win.show_response_effects;
            } else if( action == "DEBUG_DIALOGUE_SHOW_ALL_RESPONSE" ) {
                d_win.show_all_responses = !d_win.show_all_responses;
                if( debug_mode ) {
                    this->debug_ignore_conditionals = !this->debug_ignore_conditionals;
                    gen_responses( topic );
                    generate_response_lines();
                }
            } else if( action == "ANY_INPUT" ) {
                // Check real hotkeys; equivalent functionally to CONFIRM
                const auto hotkey_it = std::find( response_hotkeys.begin(),
                                                  response_hotkeys.end(), evt );
                response_ind = std::distance( response_hotkeys.begin(), hotkey_it );
                if( response_condition_exists[response_ind] && ( !response_condition_eval[response_ind] &&
                        !debug_mode ) ) {
                    action = "NONE";
                }
            } else if( action == "QUIT" ) {
                response_ind = get_best_quit_response();
            }
        } while( response_ind >= response_hotkeys.size() ||
                 ( action != "ANY_INPUT" && action != "QUIT" && action != "CONFIRM" ) );
        okay = true;
        std::set<dialogue_consequence> consequences = responses[response_ind].get_consequences( *this );
        if( consequences.count( dialogue_consequence::hostile ) > 0 ) {
            okay = query_yn( _( "You may be attacked!  Proceed?" ) );
        } else if( consequences.count( dialogue_consequence::helpless ) > 0 ) {
            okay = query_yn( _( "You'll be helpless!  Proceed?" ) );
        }
    } while( !okay );

    d_win.add_history_separator();
    d_win.add_to_history( response_lines[response_ind].text, _( "You" ), c_light_blue );

    talk_response chosen = responses[response_ind];
    if( chosen.mission_selected != nullptr ) {
        actor( true )->select_mission( chosen.mission_selected );
    }

    // We can't set both skill and style or training will bug out
    // TODO: Allow setting both skill and style
    actor( true )->store_chosen_training( chosen.skill, chosen.style, chosen.dialogue_spell,
                                          chosen.proficiency );
    const bool success = chosen.trial.roll( *this );
    talk_effect_t const &effects = success ? chosen.success : chosen.failure;
    talk_topic ret_topic =  effects.apply( *this );
    talk_effect_t::update_missions( *this );
    return ret_topic;
}

/**
 * Finds the best response to use when the player is trying to quit.
 *
 * Returns the index into the response list.
 */
int dialogue::get_best_quit_response()
{
    if( responses.size() == 1 ) {
        // Only one response. Use it. Consequences will be prompted for by the caller.
        return 0;
    }

    // Find relevant responses
    for( size_t i = 0; i < responses.size(); ++i ) {
        const talk_response &response = responses[i];
        if( response.trial.calc_chance( *this ) < 100 ) {
            // Don't pick anything with a chance to fail.
            continue;
        }

        if( !response.success.effects.empty() ) {
            // Don't pick anything with side effects
            continue;
        }

        // Unfortunately, while we'd like to be able to go "back" from nested dialogue trees, the
        // topic stack doesn't always shrink. Returning to the previous topic is sometimes done
        // with TALK_NONE, or sometimes by referencing the topic id directly. No solution really
        // gives us something that feels right in all cases, so we only support completely leaving
        // the conversation via the quit key.

        if( response.success.next_topic.id == "TALK_DONE" ) {
            return i;
        }
    }

    return responses.size(); // Didn't find a good option
}

void get_raw_debug_fields( const JsonObject &jo, std::map<std::string, std::string> &debug_info )
{
    //check for object, add raw JSON text to map if exists
    auto add_to_debug = [&]( const JsonObject & jo, const char *key ) {
        std::string key_str( key );
        if( jo.has_string( key_str ) ) {
            debug_info.emplace( key_str, jo.get_string( key_str ) );
        } else if( jo.has_object( key_str ) ) {
            JsonObject json_cond = jo.get_object( key_str );
            json_cond.allow_omitted_members();
            debug_info.emplace( key_str, json_cond.str() );
        } else if( jo.has_array( key_str ) ) {
            JsonArray json_cond = jo.get_array( key_str );
            std::string concat;
            for( JsonValue elem : json_cond ) {
                if( elem.test_string() ) {
                    concat += elem.get_string();
                } else if( elem.test_object() ) {
                    JsonObject json_obj = elem.get_object();
                    json_obj.allow_omitted_members();
                    concat += json_obj.str();
                }
            }
            debug_info.emplace( key_str, concat );
        }
    };

    add_to_debug( jo, "condition" );
    add_to_debug( jo, "effect" );
    add_to_debug( jo, "trial" );
    add_to_debug( jo, "success" );
    add_to_debug( jo, "failure" );
}


std::vector<std::string> dialogue::build_debug_info( const dialogue_window &d_win,
        const talk_topic &topic, int do_response )
{
    std::vector<std::string> debug_output;
    json_talk_topic json_topic = json_talk_topics[topic.id];
    if( do_response > -1 ) {
        std::string write_str;
        std::vector<json_talk_response> json_responses = json_topic.get_responses();
        if( json_responses.empty() ) {
            return debug_output;
        }

        talk_response &actual_response = responses[do_response];
        std::map<std::string, std::string> &debug_info = actual_response.debug_info;
        if( d_win.show_response_conditionals ) {
            if( actual_response.condition && debug_info.find( "condition" ) != debug_info.end() ) {
                debug_output.emplace_back( std::string( "Conditional: [" ) + ( actual_response.condition(
                                               *this ) ? colorize( "true", c_light_green ) : colorize( "false",
                                                       c_light_red ) ) + std::string( "] - " ) + debug_info["condition"] );
            } else {
                debug_output.emplace_back( "No conditionals" );
            }
        }
        if( debug_info.find( "trial" ) != debug_info.end() ) {
            if( d_win.show_response_conditionals ) {
                if( actual_response.trial.condition ) {
                    debug_output.emplace_back( std::string( "Trial: [" ) + ( actual_response.trial.condition(
                                                   *this ) ? colorize( "true", c_light_green ) : colorize( "false",
                                                           c_light_red ) ) + std::string( "] - " ) + debug_info["trial"] );
                } else {
                    debug_output.emplace_back( "Trial: " + debug_info["trial"] );
                }
            }
            if( d_win.show_response_effects ) {
                debug_output.emplace_back( colorize( "Success: ", c_green ) + debug_info["success"] );
                debug_output.emplace_back( colorize( "Failure: ", c_red ) + debug_info["failure"] );
            }
        }
        if( d_win.show_response_effects ) {
            if( debug_info.find( "effect" ) != debug_info.end() ) {
                debug_output.emplace_back( "Effect: " + debug_info["effect"] );
            } else {
                debug_output.emplace_back( "No effects" );
            }
        }
    } else {
        std::vector<json_dynamic_line_effect> speaker_effects = json_topic.get_speaker_effects();
        int eff_count = 1;
        bool added_cond = false;
        bool added_eff = false;
        for( json_dynamic_line_effect eff : speaker_effects ) {
            std::map<std::string, std::string> &debug_info = eff.debug_info;
            std::string eff_count_str = std::to_string( eff_count );
            if( debug_info.find( "condition" ) != debug_info.end() && d_win.show_dynamic_line_conditionals ) {
                debug_output.emplace_back( colorize( "CND" + eff_count_str + std::string( ": [" ),
                                                     c_yellow ) + ( eff.test_condition(
                                                             *this ) ? colorize( "true", c_light_green ) : colorize( "false",
                                                                     c_light_red ) ) + std::string( "] - " ) + colorize( debug_info["condition"], c_yellow ) );
                added_cond = true;
            }
            if( debug_info.find( "effect" ) != debug_info.end() && d_win.show_dynamic_line_effects ) {
                debug_output.emplace_back( colorize( "EFF" + eff_count_str + ": " + debug_info["effect"],
                                                     c_yellow ) );
                added_eff = true;
            }
            eff_count++;
        }
        if( !added_cond && d_win.show_dynamic_line_conditionals ) {
            debug_output.emplace_back( colorize( "No conditionals", c_yellow ) );
        }
        if( !added_eff && d_win.show_dynamic_line_effects ) {
            debug_output.emplace_back( colorize( "No effects", c_yellow ) );
        }
    }

    return debug_output;
}

talk_trial::talk_trial( const JsonObject &jo )
{
    static const std::unordered_map<std::string, talk_trial_type> types_map = { {
#define WRAP(value) { #value, TALK_TRIAL_##value }
            WRAP( NONE ),
            WRAP( LIE ),
            WRAP( PERSUADE ),
            WRAP( INTIMIDATE ),
            WRAP( SKILL_CHECK ),
            WRAP( CONDITION )
#undef WRAP
        }
    };
    const auto iter = types_map.find( jo.get_string( "type", "NONE" ) );
    if( iter == types_map.end() ) {
        jo.throw_error_at( "type", "invalid talk trial type" );
    }
    type = iter->second;
    if( type != TALK_TRIAL_NONE && type != TALK_TRIAL_CONDITION ) {
        difficulty = jo.get_int( "difficulty" );
    }
    if( type == TALK_TRIAL_SKILL_CHECK ) {
        skill_required = jo.get_string( "skill_required" );
    }
    //condition remains empty instead of adding a default
    if( jo.has_member( "condition" ) ) {
        read_condition( jo, "condition", condition, false );
    }
    if( jo.has_member( "mod" ) ) {
        for( JsonArray jmod : jo.get_array( "mod" ) ) {
            trial_mod this_modifier;
            this_modifier.first = jmod.next_string();
            this_modifier.second = jmod.next_int();
            modifiers.push_back( this_modifier );
        }
    }
}

static talk_topic load_inline_topic( const JsonObject &jo, std::string_view src )
{
    const std::string id = jo.get_string( "id" );
    json_talk_topics[id].load( jo, src );
    return talk_topic( id );
}

talk_effect_fun_t::talk_effect_fun_t( const talkfunction_ptr &ptr )
{
    function = [ptr]( dialogue const & d ) {
        if( d.actor( true )->get_npc() ) {
            ptr( *d.actor( true )->get_npc() );
        }
    };
}

talk_effect_fun_t::talk_effect_fun_t( const std::function<void( npc &p )> &ptr )
{
    function = [ptr]( dialogue const & d ) {
        if( d.actor( true )->get_npc() ) {
            ptr( *d.actor( true )->get_npc() );
        }
    };
}

talk_effect_fun_t::talk_effect_fun_t( func &&fun )
{
    function = fun;
}

talk_effect_fun_t::likely_rewards_t &talk_effect_fun_t::get_likely_rewards()
{
    return likely_rewards;
}

static Character *get_character_from_id( const std::string &id_str, game *g )
{
    // Return the character with character_id, return nullptr if counldn't find the character with character_id
    Character *temp_guy = nullptr;

    character_id char_id;
    try {
        char_id = character_id( std::stoi( id_str ) );
    } catch( const std::exception & ) {
        return nullptr;
    }

    // Check Avatar
    if( char_id == get_avatar().getID() ) {
        return &get_avatar();
    }

    // Check visible NPC
    for( npc *guy : g->get_npcs_if( [char_id]( const npc & guy ) {
    return guy.getID() == char_id;
    } ) ) {
        temp_guy = guy;
    }

    // Check NPC in the OverMapBuffer
    if( temp_guy == nullptr ) {
        temp_guy = g->find_npc( char_id );
    }
    return temp_guy;
}

static void run_item_eocs( const dialogue &d, bool is_npc, const std::vector<item_location> &items,
                           std::string_view option, const std::vector<effect_on_condition_id> &true_eocs,
                           const std::vector<effect_on_condition_id> &false_eocs, const std::vector <item_search_data> &data,
                           const item_menu &f, const item_menu_mul &f_mul )
{
    Character *guy = d.actor( is_npc )->get_character();
    guy = guy ? guy : &get_player_character();
    std::vector<item_location> true_items;
    for( const item_location &loc : items ) {
        // Check if item matches any search_data.
        bool true_tgt = data.empty();
        for( item_search_data datum : data ) {
            if( datum.check( guy, loc, d ) ) {
                true_tgt = true;
                break;
            }
        }
        if( true_tgt ) {
            true_items.push_back( loc );
        }
    }
    const auto run_eoc = [&d, is_npc]( item_location & loc,
    const std::vector<effect_on_condition_id> &eocs ) {
        for( const effect_on_condition_id &eoc : eocs ) {
            // Check if item is outdated.
            if( loc.get_item() ) {
                dialogue newDialog( d.actor( is_npc )->clone(), get_talker_for( loc ),
                                    d.get_conditionals(), d.get_context() );
                eoc->activate( newDialog );
            }
        }
    };
    auto filter = [true_items]( const item_location & it ) {
        for( const item_location &true_it : true_items ) {
            if( true_it == it ) {
                return true;
            }
        }
        return false;
    };
    if( option == "all" ) {
        for( item_location target : true_items ) {
            run_eoc( target, true_eocs );
        }
        if( true_items.empty() ) {
            run_eoc_vector( false_eocs, d );
        }
    } else if( option == "random" ) {
        if( !true_items.empty() ) {
            std::shuffle( true_items.begin(), true_items.end(), rng_get_engine() );
            run_eoc( true_items.back(), true_eocs );
            true_items.pop_back();
        } else {
            run_eoc_vector( false_eocs, d );
        }
    } else if( option == "manual" ) {
        item_location selected = f( filter );
        run_eoc( selected, true_eocs );
        if( selected.get_item() == nullptr ) {
            run_eoc_vector( false_eocs, d );
        }
    } else if( option == "manual_mult" ) {
        const drop_locations &selected = f_mul( filter );
        for( item_location target : true_items ) {
            bool true_eoc = false;
            for( const drop_location &dloc : selected ) {
                if( target == dloc.first ) {
                    true_eoc = true;
                    break;
                }
            }
            if( true_eoc ) {
                run_eoc( target, true_eocs );
            }
        }
        if( true_items.empty() ) {
            run_eoc_vector( false_eocs, d );
        }
    }
}
namespace talk_effect_fun
{
namespace
{
talk_effect_fun_t::func f_companion_mission( const JsonObject &jo, std::string_view member,
        std::string_view )
{
    str_or_var id = get_str_or_var( jo.get_member( member ), member, true );
    return [id]( dialogue const & d ) {
        std::string role_id = id.evaluate( d );
        d.actor( true )->set_companion_mission( role_id );
    };
}

talk_effect_fun_t::func f_add_effect( const JsonObject &jo, std::string_view member,
                                      std::string_view, bool is_npc )
{
    str_or_var new_effect = get_str_or_var( jo.get_member( member ), member, true );
    bool permanent = false;
    bool force = jo.get_bool( "force", false );
    duration_or_var dov_duration;
    dbl_or_var dov_intensity;
    if( jo.has_string( "duration" ) ) {
        const std::string dur_string = jo.get_string( "duration" );
        if( dur_string == "PERMANENT" ) {
            permanent = true;
            dov_duration = get_duration_or_var( jo, "", false, 1_turns );
        } else {
            dov_duration = get_duration_or_var( jo, "duration", false, 1000_turns );
        }
    } else {
        dov_duration = get_duration_or_var( jo, "duration", true );
    }
    dov_intensity = get_dbl_or_var( jo, "intensity", false, 0 );
    str_or_var target;
    if( jo.has_member( "target_part" ) ) {
        target = get_str_or_var( jo.get_member( "target_part" ), "target_part", false, "bp_null" );
    } else {
        target.str_val = "bp_null";
    }
    return [is_npc, new_effect, dov_duration, target, permanent, force,
            dov_intensity]( dialogue & d ) {
        d.actor( is_npc )->add_effect( efftype_id( new_effect.evaluate( d ) ),
                                       dov_duration.evaluate( d ),
                                       target.evaluate( d ), permanent, force,
                                       dov_intensity.evaluate( d ) );
    };
}

talk_effect_fun_t::func f_remove_effect( const JsonObject &jo, std::string_view member,
        std::string_view, bool is_npc )
{
    std::vector<str_or_var> old_effects;
    if( jo.has_string( member ) ) {
        old_effects.emplace_back( get_str_or_var( jo.get_member( member ), member ) );
    } else if( jo.has_array( member ) ) {
        for( JsonValue jv : jo.get_array( member ) ) {
            old_effects.emplace_back( get_str_or_var( jv, member ) );
        }
    }

    str_or_var target;
    if( jo.has_member( "target_part" ) ) {
        target = get_str_or_var( jo.get_member( "target_part" ), "target_part", false, "bp_null" );
    } else {
        target.str_val = "bp_null";
    }

    return [is_npc, old_effects, target]( dialogue const & d ) {
        if( target.evaluate( d ) == "ALL" ) {
            std::vector<bodypart_id> all_bprts = d.actor( is_npc )->get_all_body_parts(
                    get_body_part_flags::none );
            all_bprts.emplace_back( "bp_null" );
            for( const str_or_var &old_effect : old_effects ) {
                for( bodypart_id bp : all_bprts ) {
                    d.actor( is_npc )->remove_effect( efftype_id( old_effect.evaluate( d ) ), bp.id().c_str() );
                }
            }
        } else {
            for( const str_or_var &old_effect : old_effects ) {
                d.actor( is_npc )->remove_effect( efftype_id( old_effect.evaluate( d ) ), target.evaluate( d ) );
            }
        }
    };
}

talk_effect_fun_t::func f_add_trait( const JsonObject &jo, std::string_view member,
                                     std::string_view, bool is_npc )
{
    str_or_var new_trait = get_str_or_var( jo.get_member( member ), member, true );
    str_or_var new_variant;

    if( jo.has_member( "variant" ) ) {
        new_variant = get_str_or_var( jo.get_member( "variant" ), "variant", true );
    } else {
        new_variant.str_val = "";
    }

    return [is_npc, new_trait, new_variant]( dialogue const & d ) {
        const trait_id trait = trait_id( new_trait.evaluate( d ) );
        const mutation_variant *variant = trait->variant( new_variant.evaluate( d ) );

        d.actor( is_npc )->set_mutation( trait, variant );
    };
}

talk_effect_fun_t::func f_activate_trait( const JsonObject &jo, std::string_view member,
        std::string_view, bool is_npc )
{
    str_or_var new_trait = get_str_or_var( jo.get_member( member ), member, true );
    return [is_npc, new_trait]( dialogue const & d ) {
        d.actor( is_npc )->activate_mutation( trait_id( new_trait.evaluate( d ) ) );
    };
}

talk_effect_fun_t::func f_deactivate_trait( const JsonObject &jo, std::string_view member,
        std::string_view, bool is_npc )
{
    str_or_var new_trait = get_str_or_var( jo.get_member( member ), member, true );
    return [is_npc, new_trait]( dialogue const & d ) {
        d.actor( is_npc )->deactivate_mutation( trait_id( new_trait.evaluate( d ) ) );
    };
}

talk_effect_fun_t::func f_remove_trait( const JsonObject &jo, std::string_view member,
                                        std::string_view, bool is_npc )
{
    str_or_var old_trait = get_str_or_var( jo.get_member( member ), member, true );
    return [is_npc, old_trait]( dialogue const & d ) {
        d.actor( is_npc )->unset_mutation( trait_id( old_trait.evaluate( d ) ) );
    };
}

talk_effect_fun_t::func f_learn_martial_art( const JsonObject &jo, std::string_view member,
        std::string_view, bool is_npc )
{
    str_or_var ma_to_learn = get_str_or_var( jo.get_member( member ), member, true );
    return [is_npc, ma_to_learn]( dialogue const & d ) {
        d.actor( is_npc )->learn_martial_art( matype_id( ma_to_learn.evaluate( d ) ) );
    };
}

talk_effect_fun_t::func f_forget_martial_art( const JsonObject &jo, std::string_view member,
        std::string_view, bool is_npc )
{
    str_or_var ma_to_forget = get_str_or_var( jo.get_member( member ), member, true );
    return [is_npc, ma_to_forget]( dialogue const & d ) {
        d.actor( is_npc )->forget_martial_art( matype_id( ma_to_forget.evaluate( d ) ) );
    };
}

talk_effect_fun_t::func f_mutate( const JsonObject &jo, std::string_view member,
                                  std::string_view, bool is_npc )
{
    dbl_or_var highest_cat = get_dbl_or_var( jo, member, true, 0 );
    const bool use_vitamins = jo.get_bool( "use_vitamins", true );
    return [is_npc, highest_cat, use_vitamins]( dialogue & d ) {
        d.actor( is_npc )->mutate( highest_cat.evaluate( d ), use_vitamins );
    };
}

talk_effect_fun_t::func f_mutate_category( const JsonObject &jo, std::string_view member,
        std::string_view, bool is_npc )
{
    str_or_var mut_cat = get_str_or_var( jo.get_member( member ), member, true, "" );
    const bool use_vitamins = jo.get_bool( "use_vitamins", true );
    const bool true_random = jo.get_bool( "true_random", false );
    return [is_npc, mut_cat, use_vitamins, true_random]( dialogue const & d ) {
        d.actor( is_npc )->mutate_category( mutation_category_id( mut_cat.evaluate( d ) ), use_vitamins,
                                            true_random );
    };
}

talk_effect_fun_t::func f_mutate_towards( const JsonObject &jo, std::string_view member,
        std::string_view, bool is_npc )
{
    str_or_var trait = get_str_or_var( jo.get_member( member ), member, true, "" );
    str_or_var mut_cat;
    if( jo.has_member( "category" ) ) {
        mut_cat = get_str_or_var( jo.get_member( "category" ), "category", false, "" );
    } else {
        mut_cat.str_val = "";
    }
    const bool use_vitamins = jo.get_bool( "use_vitamins", true );

    return [is_npc, trait, mut_cat, use_vitamins]( dialogue const & d ) {
        d.actor( is_npc )->mutate_towards( trait_id( trait.evaluate( d ) ),
                                           mutation_category_id( mut_cat.evaluate( d ) ), use_vitamins );
    };
}

talk_effect_fun_t::func f_set_trait_purifiability( const JsonObject &jo, std::string_view member,
        std::string_view, bool is_npc )
{
    str_or_var trait = get_str_or_var( jo.get_member( member ), member, true, "" );
    const bool purifiable = jo.get_bool( "purifiable", true );

    return [is_npc, trait, purifiable]( dialogue const & d ) {
        d.actor( is_npc )->set_trait_purifiability( trait_id( trait.evaluate( d ) ), purifiable );
    };
}

talk_effect_fun_t::func f_add_bionic( const JsonObject &jo, std::string_view member,
                                      std::string_view, bool is_npc )
{
    str_or_var new_bionic = get_str_or_var( jo.get_member( member ), member, true );
    return [is_npc, new_bionic]( dialogue const & d ) {
        d.actor( is_npc )->add_bionic( bionic_id( new_bionic.evaluate( d ) ) );
    };
}

talk_effect_fun_t::func f_lose_bionic( const JsonObject &jo, std::string_view member,
                                       std::string_view, bool is_npc )
{
    str_or_var old_bionic = get_str_or_var( jo.get_member( member ), member, true );
    return [is_npc, old_bionic]( dialogue const & d ) {
        d.actor( is_npc )->remove_bionic( bionic_id( old_bionic.evaluate( d ) ) );
    };
}

talk_effect_fun_t::func f_add_var( const JsonObject &jo, std::string_view member,
                                   std::string_view, bool is_npc )
{
    dbl_or_var empty;
    const std::string var_name = get_talk_varname( jo, member );
    const std::string var_base_name = get_talk_var_basename( jo, member, false );
    const bool time_check = jo.has_member( "time" ) && jo.get_bool( "time" );
    std::vector<std::string> possible_values = jo.get_string_array( "possible_values" );
    if( possible_values.empty() ) {
        const std::string value = time_check ? "" : jo.get_string( "value" );
        possible_values.push_back( value );
    }
    return [is_npc, var_name, possible_values, time_check, var_base_name]( dialogue const & d ) {
        talker *actor = d.actor( is_npc );
        if( time_check ) {
            actor->set_value( var_name, string_format( "%d", to_turn<int>( calendar::turn ) ) );
        } else {
            int index = rng( 0, possible_values.size() - 1 );
            actor->set_value( var_name, possible_values[index] );
            get_event_bus().send<event_type::u_var_changed>( var_base_name, possible_values[index] );
        }
    };
}

talk_effect_fun_t::func f_remove_var( const JsonObject &jo, std::string_view member,
                                      std::string_view, bool is_npc )
{
    const std::string var_name = get_talk_varname( jo, member );
    return [is_npc, var_name]( dialogue const & d ) {
        d.actor( is_npc )->remove_value( var_name );
    };
}

void map_add_item( item &it, tripoint_abs_ms target_pos )
{
    map &here = get_map();

    if( here.inbounds( target_pos ) ) {
        here.add_item_or_charges( here.get_bub( target_pos ), it );
    } else {
        tinymap target_bay;
        target_bay.load( project_to<coords::omt>( target_pos ), false );
        // Redundant as long as map operations aren't using get_map() in a transitive call chain. Added for future proofing.
        swap_map swap( *target_bay.cast_to_map() );
        target_bay.add_item_or_charges( target_bay.get_omt( target_pos ), it );
    }
}

void receive_item( const itype_id &item_id, int count, const itype_id &container_id,
                   const dialogue &d, bool suppress_message,
                   const std::vector<std::string> &flags,
                   bool add_talker = true,
                   const tripoint_abs_ms &p = tripoint_abs_ms::zero, bool force_equip = false )
{
    item new_item = item( item_id, calendar::turn );
    for( const std::string &flag : flags ) {
        new_item.set_flag( flag_id( flag ) );
    }

    if( !container_id.is_valid() ) {
        if( new_item.count_by_charges() ) {
            new_item.charges = count;
            if( add_talker ) {
                d.actor( false )->i_add_or_drop( new_item, force_equip );
            } else {
                map_add_item( new_item, p );
            }
        } else {
            for( int i_cnt = 0; i_cnt < count; i_cnt++ ) {
                if( !new_item.ammo_default().is_null() ) {
                    new_item.ammo_set( new_item.ammo_default() );
                }
                if( add_talker ) {
                    d.actor( false )->i_add_or_drop( new_item, force_equip );
                } else {
                    map_add_item( new_item, p );
                }
            }
        }
        if( add_talker && !suppress_message && d.has_beta && !d.actor( true )->disp_name().empty() ) {
            if( count == 1 ) {
                //~ %1%s is the NPC name, %2$s is an item
                popup( _( "%1$s gives you a %2$s." ), d.actor( true )->disp_name(), new_item.tname() );
            } else {
                //~ %1%s is the NPC name, %2$d is a number of items, %3$s are items
                popup( _( "%1$s gives you %2$d %3$s." ), d.actor( true )->disp_name(), count,
                       new_item.tname() );
            }
        }
    } else {
        item container( container_id, calendar::turn );
        new_item.charges = count;
        container.put_in( new_item,
                          pocket_type::CONTAINER );
        if( add_talker ) {
            d.actor( false )->i_add_or_drop( container, force_equip );
        } else {
            map_add_item( container, p );
        }
        if( add_talker && !suppress_message && d.has_beta && !d.actor( true )->disp_name().empty() ) {
            //~ %1%s is the NPC name, %2$s is an item
            popup( _( "%1$s gives you a %2$s." ), d.actor( true )->disp_name(), container.tname() );
        }
    }
}

void receive_item_group( const item_group_id &group_id, const dialogue &d, bool suppress_message,
                         const std::vector<std::string> &flags, bool add_talker = true,
                         const tripoint_abs_ms &p = tripoint_abs_ms::zero, bool force_equip = false )
{
    item_group::ItemList new_items;
    new_items = item_group::items_from( group_id, calendar::turn );
    std::string popup_message;
    for( item &new_item : new_items ) {
        for( const std::string &flag : flags ) {
            new_item.set_flag( flag_id( flag ) );
        }
        if( add_talker ) {
            d.actor( false )->i_add_or_drop( new_item, force_equip );
        } else {
            map_add_item( new_item, p );
        }
        if( add_talker && !suppress_message && d.has_beta && !d.actor( true )->disp_name().empty() ) {
            if( new_item.count() == 1 ) {
                //~ %1%s is the NPC name, %2$s is an item
                popup_message += string_format( _( "%1$s gives you a %2$s." ), d.actor( true )->disp_name(),
                                                new_item.tname() ) + "\n";
            } else {
                //~ %1%s is the NPC name, %2$d is a number of items, %3$s are items
                popup_message += string_format( _( "%1$s gives you %2$d %3$s." ), d.actor( true )->disp_name(),
                                                new_item.count(), new_item.tname() ) + "\n";
            }
        }
    }
    if( !popup_message.empty() ) {
        popup( popup_message );
    }
}

talk_effect_fun_t f_spawn_item( const JsonObject &jo, std::string_view member,
                                std::string_view )
{
    str_or_var item_name = get_str_or_var( jo.get_member( member ), member, true );
    str_or_var container_name;
    if( jo.has_member( "container" ) ) {
        container_name = get_str_or_var( jo.get_member( "container" ), "container", true );
    } else {
        container_name.str_val = "";
    }
    bool use_item_group = jo.get_bool( "use_item_group", false );
    bool suppress_message = jo.get_bool( "suppress_message", false );
    dbl_or_var count;
    if( !jo.has_int( "charges" ) ) {
        count = get_dbl_or_var( jo, "count", false, 1 );
    } else {
        count = get_dbl_or_var( jo, "count", false, 0 );
    }
    bool add_talker = true;
    if( member == "u_spawn_item" ) {
        add_talker = true;
    } else if( member == "map_spawn_item" ) {
        add_talker = false;
    }
    std::optional<var_info> loc_var;
    if( jo.has_object( "loc" ) ) {
        loc_var = read_var_info( jo.get_object( "loc" ) );
    }
    bool force_equip = jo.get_bool( "force_equip", false );

    std::vector<str_or_var> flags;
    for( JsonValue jv : jo.get_array( "flags" ) ) {
        flags.emplace_back( get_str_or_var( jv, "flags" ) );
    }
    talk_effect_fun_t ret( [item_name, count, container_name, use_item_group, suppress_message,
               add_talker, loc_var, force_equip, flags]( dialogue & d ) {
        tripoint_abs_ms target_location;
        if( loc_var ) {
            target_location = read_var_value( *loc_var, d ).tripoint();
        } else {
            target_location = d.actor( false )->pos_abs();
        }
        std::vector<std::string> flags_str;
        flags_str.reserve( flags.size() );
        for( const str_or_var &flat_sov : flags ) {
            flags_str.emplace_back( flat_sov.evaluate( d ) );
        }
        if( use_item_group ) {
            const item_group_id group_id = item_group_id( item_name.evaluate( d ) );
            receive_item_group( group_id, d, suppress_message, flags_str, add_talker, target_location,
                                force_equip );
        } else {
            const itype_id item_id = itype_id( item_name.evaluate( d ) );
            const itype_id container_id = itype_id( container_name.evaluate( d ) );
            receive_item( item_id, count.evaluate( d ), container_id, d, suppress_message,
                          flags_str, add_talker, target_location, force_equip );
        }
    } );
    ret.get_likely_rewards().emplace_back( count, item_name );
    return ret;
}

talk_effect_fun_t::func f_u_buy_item( const JsonObject &jo, std::string_view member,
                                      std::string_view src )
{
    std::vector<effect_on_condition_id> true_eocs = load_eoc_vector( jo, "true_eocs", src );
    std::vector<effect_on_condition_id> false_eocs = load_eoc_vector( jo, "false_eocs", src );
    dbl_or_var cost = get_dbl_or_var( jo, "cost", false, 0 );
    dbl_or_var count;
    if( !jo.has_int( "charges" ) ) {
        count = get_dbl_or_var( jo, "count", false, 1 );
    } else {
        count = get_dbl_or_var( jo, "count", false, 0 );
    }
    bool use_item_group = jo.get_bool( "use_item_group", false );
    bool suppress_message = jo.get_bool( "suppress_message", false );
    str_or_var container_name;
    if( jo.has_member( "container" ) ) {
        container_name = get_str_or_var( jo.get_member( "container" ), "container", true );
    } else {
        container_name.str_val = "";
    }

    std::vector<str_or_var> flags;
    for( JsonValue jv : jo.get_array( "flags" ) ) {
        flags.emplace_back( get_str_or_var( jv, "flags" ) );
    }
    str_or_var item_name = get_str_or_var( jo.get_member( member ), member, true );
    return [item_name, cost, count, container_name, true_eocs, false_eocs,
               use_item_group, suppress_message, flags]( dialogue & d ) {
        if( !d.actor( true )->buy_from( cost.evaluate( d ) ) ) {
            popup( _( "You can't afford it!" ) );
            run_eoc_vector( false_eocs, d );
            return;
        }
        std::vector<std::string> flags_str;
        flags_str.reserve( flags.size() );
        for( const str_or_var &flat_sov : flags ) {
            flags_str.emplace_back( flat_sov.evaluate( d ) );
        }
        if( use_item_group ) {
            const item_group_id group_id = item_group_id( item_name.evaluate( d ) );
            receive_item_group( group_id, d, suppress_message, flags_str );
        } else {
            const itype_id item_id = itype_id( item_name.evaluate( d ) );
            const itype_id container_id = itype_id( container_name.evaluate( d ) );
            receive_item( item_id, count.evaluate( d ), container_id, d, suppress_message,
                          flags_str );
        }
        run_eoc_vector( true_eocs, d );
    };
}

talk_effect_fun_t::func f_u_sell_item( const JsonObject &jo, std::string_view member,
                                       std::string_view src )
{
    std::vector<effect_on_condition_id> true_eocs = load_eoc_vector( jo, "true_eocs", src );
    std::vector<effect_on_condition_id> false_eocs = load_eoc_vector( jo, "false_eocs", src );
    dbl_or_var cost = get_dbl_or_var( jo, "cost", false, 0 );
    dbl_or_var count;
    if( !jo.has_int( "charges" ) ) {
        count = get_dbl_or_var( jo, "count", false, 1 );
    } else {
        count = get_dbl_or_var( jo, "count", false, 0 );
    }
    str_or_var item_name = get_str_or_var( jo.get_member( member ), member, true );
    return [item_name, cost, count, true_eocs, false_eocs]( dialogue & d ) {
        int current_count = count.evaluate( d );
        itype_id current_item_name = itype_id( item_name.evaluate( d ) );
        if( item::count_by_charges( current_item_name ) &&
            d.actor( false )->has_charges( current_item_name, current_count ) ) {
            for( item &it : d.actor( false )->use_charges( current_item_name, current_count ) ) {
                it.set_owner( d.actor( true )->get_faction()->id );
                d.actor( true )->i_add( it );
            }
        } else if( d.actor( false )->has_amount( current_item_name, current_count ) ) {
            for( item &it : d.actor( false )->use_amount( current_item_name, current_count ) ) {
                it.set_owner( d.actor( true )->get_faction()->id );
                d.actor( true )->i_add( it );
            }
        } else {
            //~ %1$s is a translated item name
            popup( _( "You don't have a %1$s!" ), item::nname( current_item_name ) );
            run_eoc_vector( false_eocs, d );
            return;
        }
        if( current_count == 1 ) {
            //~ %1%s is the NPC name, %2$s is an item
            popup( _( "You give %1$s a %2$s." ), d.actor( true )->disp_name(),
                   item::nname( current_item_name ) );
        } else {
            //~ %1%s is the NPC name, %2$d is a number of items, %3$s are items
            popup( _( "You give %1$s %2$d %3$s." ), d.actor( true )->disp_name(), current_count,
                   item::nname( current_item_name, current_count ) );
        }
        d.actor( true )->add_debt( cost.evaluate( d ) );
        run_eoc_vector( true_eocs, d );
    };
}

talk_effect_fun_t::func f_consume_item( const JsonObject &jo, std::string_view member,
                                        std::string_view, bool is_npc )
{
    str_or_var item_name = get_str_or_var( jo.get_member( member ), member, true );
    dbl_or_var charges = get_dbl_or_var( jo, "charges", false, 0 );
    dbl_or_var count;
    if( !jo.has_int( "charges" ) ) {
        count = get_dbl_or_var( jo, "count", false, 1 );
    } else {
        count = get_dbl_or_var( jo, "count", false, 0 );
    }
    const bool do_popup = jo.get_bool( "popup", false );
    return [do_popup, is_npc, item_name, count, charges]( dialogue & d ) {
        // this is stupid, but I couldn't get the assignment to work
        int current_count = count.evaluate( d );
        int current_charges = charges.evaluate( d );
        itype_id current_item_name = itype_id( item_name.evaluate( d ) );
        const auto consume_item = [&]( talker & p, const itype_id & item_name, int current_count,
        int current_charges ) {
            if( current_charges == 0 && item::count_by_charges( item_name ) ) {
                current_charges = current_count;
                current_count = 0;
            }

            if( current_count == 0 && current_charges > 0 &&
                p.has_charges( item_name, current_charges, true ) ) {
                p.use_charges( item_name, current_charges, true );
            } else if( p.has_amount( item_name, current_count ) ) {
                if( current_charges > 0 && p.has_charges( item_name, current_charges, true ) ) {
                    p.use_charges( item_name, current_charges, true );
                }
                p.use_amount( item_name, current_count );
            } else {
                item old_item( item_name );
                //~ %1%s is the "You" or the NPC name, %2$s are a translated item name
                popup( _( "%1$s doesn't have a %2$s!" ), p.disp_name(), old_item.tname() );
            }
        };
        if( is_npc ) {
            consume_item( *d.actor( true ), current_item_name, current_count, current_charges );
        } else {
            if( do_popup ) {
                if( current_count == 1 ) {
                    popup( _( "You give %1$s a %2$s." ), d.actor( true )->disp_name(),
                           item::nname( current_item_name ) );
                } else {
                    popup( _( "You give %1$s %2$d %3$s." ), d.actor( true )->disp_name(), current_count,
                           item::nname( current_item_name ), current_count );
                }
            }
            consume_item( *d.actor( false ), current_item_name, current_count, current_charges );
        }
    };
}

talk_effect_fun_t::func f_consume_item_sum( const JsonObject &jo, std::string_view member,
        std::string_view, bool is_npc )
{
    // Only after i implemented it, i realised it is a bad way to make it
    // What it should be is an expansion of f_run_inv_eocs
    // that allow to pick multiple ids and limit it to some amount of it

    std::vector<std::pair<str_or_var, dbl_or_var>> item_and_amount;

    for( const JsonObject jsobj : jo.get_array( member ) ) {
        const str_or_var item = get_str_or_var( jsobj.get_member( "item" ), "item", true );
        const dbl_or_var amount = get_dbl_or_var( jsobj, "amount", true, 1 );
        item_and_amount.emplace_back( item, amount );
    }

    return [item_and_amount, is_npc]( dialogue & d ) {
        add_msg_debug( debugmode::DF_TALKER, "using _consume_item_sum:" );

        itype_id item_to_remove;
        double percent = 0.0f;
        double ratio = 0.0f;
        double amount_desired = 0.0f;
        int count_present = 0;
        Character *you = d.actor( is_npc )->get_character();
        inventory inventory_and_around = you->crafting_inventory( you->pos_bub(), PICKUP_RANGE );
        std::vector<item_comp> items_to_remove_vector;

        for( const auto &pair : item_and_amount ) {
            int amount_to_remove = 0;
            item_to_remove = itype_id( pair.first.evaluate( d ) );
            amount_desired = pair.second.evaluate( d );
            count_present = inventory_and_around.count_item( item_to_remove );

            if( count_present == 0 ) {
                continue;
            }

            percent += count_present / amount_desired;

            if( percent <= 1.0 ) {
                // either lack or just right amount of items to consume
                items_to_remove_vector = { { item_to_remove, static_cast<int>( count_present ) } };
                you->consume_items( items_to_remove_vector );

            } else {
                // too much items to consume, consuming only to hit 1.00 percent
                percent -= count_present / amount_desired;
                ratio = count_present / amount_desired;

                while( percent < 1.0 ) {
                    percent += ratio / count_present;
                    ++amount_to_remove;
                }
                items_to_remove_vector = { { item_to_remove, amount_to_remove } };
                you->consume_items( items_to_remove_vector );
            }
        }
    };
}

talk_effect_fun_t::func f_remove_item_with( const JsonObject &jo, std::string_view member,
        std::string_view, bool is_npc )
{
    str_or_var item_name = get_str_or_var( jo.get_member( member ), member, true );
    return [is_npc, item_name]( dialogue const & d ) {
        itype_id item_id = itype_id( item_name.evaluate( d ) );
        d.actor( is_npc )->remove_items_with( [item_id]( const item & it ) {
            return it.typeId() == item_id;
        } );
    };
}

talk_effect_fun_t::func f_u_spend_cash( const JsonObject &jo, std::string_view member,
                                        std::string_view src )
{
    dbl_or_var amount = get_dbl_or_var( jo, member );
    std::vector<effect_on_condition_id> true_eocs = load_eoc_vector( jo, "true_eocs", src );
    std::vector<effect_on_condition_id> false_eocs = load_eoc_vector( jo, "false_eocs", src );
    return [amount, true_eocs, false_eocs]( dialogue & d ) {
        if( d.actor( true )->buy_from( amount.evaluate( d ) ) ) {
            run_eoc_vector( true_eocs, d );
        } else {
            run_eoc_vector( false_eocs, d );
        }
    };
}

talk_effect_fun_t::func f_npc_change_faction( const JsonObject &jo, std::string_view member,
        std::string_view )
{
    str_or_var faction_name = get_str_or_var( jo.get_member( member ), member, true );
    return [faction_name]( dialogue const & d ) {
        d.actor( true )->set_fac( faction_id( faction_name.evaluate( d ) ) );
    };
}

talk_effect_fun_t::func f_npc_change_class( const JsonObject &jo, std::string_view member,
        std::string_view )
{
    str_or_var class_name = get_str_or_var( jo.get_member( member ), member, true );
    return [class_name]( dialogue const & d ) {
        d.actor( true )->set_class( npc_class_id( class_name.evaluate( d ) ) );
    };
}

talk_effect_fun_t::func f_change_faction_rep( const JsonObject &jo, std::string_view member,
        std::string_view )
{
    dbl_or_var rep_change = get_dbl_or_var( jo, member );
    return [rep_change]( dialogue & d ) {
        d.actor( true )->add_faction_rep( rep_change.evaluate( d ) );
    };
}

talk_effect_fun_t::func f_add_debt( const JsonObject &jo, std::string_view member,
                                    std::string_view )
{
    std::vector<trial_mod> debt_modifiers;
    for( JsonArray jmod : jo.get_array( member ) ) {
        trial_mod this_modifier;
        this_modifier.first = jmod.next_string();
        this_modifier.second = jmod.next_int();
        debt_modifiers.push_back( this_modifier );
    }
    return [debt_modifiers]( dialogue const & d ) {
        int debt = 0;
        for( const trial_mod &this_mod : debt_modifiers ) {
            if( this_mod.first == "TOTAL" ) {
                debt *= this_mod.second;
            } else {
                debt += parse_mod( d, this_mod.first, this_mod.second );
            }
        }
        d.actor( true )->add_debt( debt );
    };
}

talk_effect_fun_t::func f_npc_rules_menu()
{
    return []( dialogue const & d ) {
        follower_rules_ui new_ui;
        new_ui.draw_follower_rules_ui( d.actor( true )->get_npc() );
    };
}

talk_effect_fun_t::func f_toggle_npc_rule( const JsonObject &jo, std::string_view member,
        std::string_view )
{
    str_or_var rule = get_str_or_var( jo.get_member( member ), member, true );
    return [rule]( dialogue const & d ) {
        d.actor( true )->toggle_ai_rule( "ally_rule", rule.evaluate( d ) );
    };
}

talk_effect_fun_t::func f_set_npc_rule( const JsonObject &jo, std::string_view member,
                                        std::string_view )
{
    str_or_var rule = get_str_or_var( jo.get_member( member ), member, true );
    return [rule]( dialogue const & d ) {
        d.actor( true )->set_ai_rule( "ally_rule", rule.evaluate( d ) );
    };
}

talk_effect_fun_t::func f_clear_npc_rule( const JsonObject &jo, std::string_view member,
        std::string_view )
{
    str_or_var rule = get_str_or_var( jo.get_member( member ), member, true );
    return [rule]( dialogue const & d ) {
        d.actor( true )->clear_ai_rule( "ally_rule", rule.evaluate( d ) );
    };
}

talk_effect_fun_t::func f_npc_engagement_rule( const JsonObject &jo,
        std::string_view member, std::string_view )
{
    str_or_var rule = get_str_or_var( jo.get_member( member ), member, true );
    return [rule]( dialogue const & d ) {
        d.actor( true )->set_ai_rule( "engagement_rule", rule.evaluate( d ) );
    };
}

talk_effect_fun_t::func f_npc_aim_rule( const JsonObject &jo, std::string_view member,
                                        std::string_view )
{
    str_or_var rule = get_str_or_var( jo.get_member( member ), member, true );
    return [rule]( dialogue const & d ) {
        d.actor( true )->set_ai_rule( "aim_rule", rule.evaluate( d ) );
    };
}

talk_effect_fun_t::func f_npc_cbm_reserve_rule( const JsonObject &jo,
        std::string_view member, std::string_view )
{
    str_or_var rule = get_str_or_var( jo.get_member( member ), member, true );
    return [rule]( dialogue const & d ) {
        d.actor( true )->set_ai_rule( "cbm_reserve_rule", rule.evaluate( d ) );
    };
}

talk_effect_fun_t::func f_npc_cbm_recharge_rule( const JsonObject &jo,
        std::string_view member, std::string_view )
{
    str_or_var rule = get_str_or_var( jo.get_member( member ), member, true );
    return [rule]( dialogue const & d ) {
        d.actor( true )->set_ai_rule( "cbm_recharge_rule", rule.evaluate( d ) );
    };
}

talk_effect_fun_t::func f_location_variable( const JsonObject &jo, std::string_view member,
        std::string_view src, bool is_npc )
{
    dbl_or_var dov_min_radius = get_dbl_or_var( jo, "min_radius", false, 0 );
    dbl_or_var dov_max_radius = get_dbl_or_var( jo, "max_radius", false, 0 );
    dbl_or_var dov_z_adjust = get_dbl_or_var( jo, "z_adjust", false, 0 );
    dbl_or_var dov_x_adjust = get_dbl_or_var( jo, "x_adjust", false, 0 );
    dbl_or_var dov_y_adjust = get_dbl_or_var( jo, "y_adjust", false, 0 );
    bool z_override = jo.get_bool( "z_override", false );
    const bool outdoor_only = jo.get_bool( "outdoor_only", false );
    const bool passable_only = jo.get_bool( "passable_only", false );
    std::optional<mission_target_params> target_params;
    if( jo.has_object( "target_params" ) ) {
        JsonObject target_obj = jo.get_object( "target_params" );
        target_params = mission_util::parse_mission_om_target( target_obj );
    }

    std::optional<str_or_var> search_target;
    std::optional<std::string> search_type;
    dbl_or_var dov_target_min_radius = get_dbl_or_var( jo, "target_min_radius", false, 0 );
    dbl_or_var dov_target_max_radius = get_dbl_or_var( jo, "target_max_radius", false, 0 );
    int target_types = 0;
    if( jo.has_member( "terrain" ) ) {
        target_types++;
        search_type = "terrain";
    }
    if( jo.has_member( "furniture" ) ) {
        target_types++;
        search_type = "furniture";
    }
    if( jo.has_member( "field" ) ) {
        target_types++;
        search_type = "field";
    }
    if( jo.has_member( "monster" ) ) {
        target_types++;
        search_type = "monster";
    }
    if( jo.has_member( "npc" ) ) {
        target_types++;
        search_type = "npc";
    }
    if( jo.has_member( "species" ) ) {
        target_types++;
        search_type = "species";
    }
    if( jo.has_member( "trap" ) ) {
        target_types++;
        search_type = "trap";
    }
    if( jo.has_member( "zone" ) ) {
        target_types++;
        search_type = "zone";
    }
    if( target_types == 1 ) {
        search_target = get_str_or_var( jo.get_member( search_type.value() ), search_type.value(), true );
    } else if( target_types > 1 ) {
        jo.throw_error( "Can only have one of terrain, furniture, field, monster, species, trap, zone, or npc." );
    }

    var_info var = read_var_info( jo.get_object( member ) );
    var_type type = var.type;
    std::string var_name = var.name;

    std::vector<effect_on_condition_id> true_eocs = load_eoc_vector( jo, "true_eocs", src );
    std::vector<effect_on_condition_id> false_eocs = load_eoc_vector( jo, "false_eocs", src );

    return [dov_min_radius, dov_max_radius, var_name, outdoor_only, passable_only, target_params,
                            is_npc, type, dov_x_adjust, dov_y_adjust, dov_z_adjust, z_override, true_eocs, false_eocs,
                    search_target, search_type, dov_target_min_radius, dov_target_max_radius]( dialogue & d ) {
        talker *target = d.actor( is_npc );
        tripoint_abs_ms talker_pos = target->pos_abs();
        tripoint_abs_ms target_pos = talker_pos;
        if( target_params.has_value() ) {
            const tripoint_abs_omt omt_pos = mission_util::get_om_terrain_pos( target_params.value(), d );
            target_pos = tripoint_abs_ms( project_to<coords::ms>( omt_pos ).x(),
                                          project_to<coords::ms>( omt_pos ).y(),
                                          project_to<coords::ms>( omt_pos ).z() );
        }
        const tripoint_abs_ms abs_ms( target_pos );
        map *here_ptr = &get_map();
        std::unique_ptr<map> distant_map = std::make_unique<map>();
        if( !here_ptr->inbounds( abs_ms ) ) {
            distant_map->load( project_to<coords::sm>( abs_ms ), false );
            here_ptr = distant_map.get();
        }
        map &here = *here_ptr;
        if( search_target.has_value() ) {
            if( search_type.value() == "monster" && !here.inbounds( abs_ms ) ) {
                here.spawn_monsters( true, true );
            }
            int min_target_dist = dov_target_min_radius.evaluate( d );
            std::string cur_search_target = search_target.value().evaluate( d );
            bool found = false;
            tripoint_range<tripoint_bub_ms> points = here.points_in_radius( here.get_bub( abs_ms ),
                    size_t( dov_target_max_radius.evaluate( d ) ), size_t( 0 ) );
            for( const tripoint_bub_ms &search_loc : points ) {
                if( rl_dist( here.get_bub( talker_pos ), search_loc ) <= min_target_dist ) {
                    continue;
                }
                if( search_type.value() == "terrain" ) {
                    if( here.ter( search_loc ).id().c_str() == cur_search_target ) {
                        target_pos = here.get_abs( search_loc );
                        found = true;
                        break;
                    }
                } else if( search_type.value() == "furniture" ) {
                    const furn_str_id &fi = here.furn( search_loc ).id();
                    if( fi.c_str() == cur_search_target ||
                        ( !fi.is_null() && cur_search_target.empty() ) ) {
                        target_pos = here.get_abs( search_loc );
                        found = true;
                        break;
                    }
                } else if( search_type.value() == "field" ) {
                    field &fields_here = here.field_at( search_loc );
                    if( fields_here.find_field( field_type_id( cur_search_target ) ) || cur_search_target.empty() ) {
                        target_pos = here.get_abs( search_loc );
                        found = true;
                        break;
                    }
                } else if( search_type.value() == "trap" ) {
                    const trap &tr = here.tr_at( search_loc );
                    if( tr.id.c_str() == cur_search_target ||
                        ( !tr.is_null() &&
                          cur_search_target.empty() ) ) {
                        target_pos = here.get_abs( search_loc );
                        found = true;
                        break;
                    }
                } else if( search_type.value() == "monster" ) {
                    Creature *tmp_critter = get_creature_tracker().creature_at( here.get_abs( search_loc ) );
                    if( tmp_critter != nullptr && tmp_critter->is_monster() &&
                        ( tmp_critter->as_monster()->type->id.c_str() == cur_search_target ||
                          cur_search_target.empty() ) ) {
                        target_pos = here.get_abs( search_loc );
                        found = true;
                        g->despawn_nonlocal_monsters();
                        break;
                    }
                } else if( search_type.value() == "species" ) {
                    Creature *tmp_critter = get_creature_tracker().creature_at( here.get_abs( search_loc ) );
                    const species_id species_target( cur_search_target );
                    if( tmp_critter != nullptr && tmp_critter->is_monster() &&
                        ( tmp_critter->as_monster()->in_species( species_target ) ||
                          cur_search_target.empty() ) ) {
                        target_pos = here.get_abs( search_loc );
                        found = true;
                        g->despawn_nonlocal_monsters();
                        break;
                    }
                } else if( search_type.value() == "npc" ) {
                    for( shared_ptr_fast<npc> &person : overmap_buffer.get_npcs_near( project_to<coords::sm>( abs_ms ),
                            1 ) ) {
                        if( person->pos_bub() == search_loc && ( person->myclass.c_str() == cur_search_target ||
                                cur_search_target.empty() ) ) {
                            target_pos = here.get_abs( search_loc );
                            found = true;
                            break;
                        }
                    }
                } else if( search_type.value() == "zone" ) {
                    zone_manager &mgr = zone_manager::get_manager();
                    if( mgr.get_zone_at( here.get_abs( search_loc ), zone_type_id( cur_search_target ) ) ) {
                        target_pos = here.get_abs( search_loc );
                        found = true;
                        break;
                    }
                }
            }
            talker_pos = target_pos;
            if( search_type.value() == "monster" ) {
                g->despawn_nonlocal_monsters();
            }
            if( !found ) {
                run_eoc_vector( false_eocs, d );
                return;
            }
        }

        int max_radius = dov_max_radius.evaluate( d );
        if( max_radius > 0 ) {
            bool found = false;
            int min_radius = dov_min_radius.evaluate( d );
            for( int attempts = 0; attempts < 25; attempts++ ) {
                target_pos = talker_pos + tripoint_rel_ms( rng( -max_radius, max_radius ), rng( -max_radius,
                             max_radius ),
                             0 );
                if( ( !outdoor_only || here.is_outside( here.get_bub( target_pos ) ) ) &&
                    ( !passable_only || here.passable_through( here.get_bub( target_pos ) ) ) &&
                    rl_dist( target_pos, talker_pos ) >= min_radius ) {
                    found = true;
                    break;
                }
            }
            if( !found ) {
                run_eoc_vector( false_eocs, d );
                return;
            }
        }

        // move the found value by the adjusts
        target_pos = target_pos + tripoint( dov_x_adjust.evaluate( d ), dov_y_adjust.evaluate( d ), 0 );

        if( z_override ) {
            target_pos = { target_pos.xy(),
                           dov_z_adjust.evaluate( d )
                         };
        } else {
            target_pos = target_pos + tripoint( 0, 0,
                                                dov_z_adjust.evaluate( d ) );
        }
        write_var_value( type, var_name, &d, target_pos );
        run_eoc_vector( true_eocs, d );
    };
}

talk_effect_fun_t::func f_location_variable_adjust( const JsonObject &jo,
        std::string_view member, std::string_view )
{
    dbl_or_var dov_z_adjust = get_dbl_or_var( jo, "z_adjust", false, 0 );
    dbl_or_var dov_x_adjust = get_dbl_or_var( jo, "x_adjust", false, 0 );
    dbl_or_var dov_y_adjust = get_dbl_or_var( jo, "y_adjust", false, 0 );
    bool z_override = jo.get_bool( "z_override", false );
    bool overmap_tile = jo.get_bool( "overmap_tile", false );

    var_info input_var = read_var_info( jo.get_object( member ) );

    std::optional<var_info> output_var;
    if( jo.has_member( "output_var" ) ) {
        output_var = read_var_info( jo.get_object( "output_var" ) );
    }
    return [input_var, dov_x_adjust, dov_y_adjust, dov_z_adjust, z_override,
               output_var, overmap_tile ]( dialogue & d ) {
        tripoint_abs_ms target_pos = read_var_value( input_var, d ).tripoint();

        if( overmap_tile ) {
            target_pos = target_pos + tripoint( dov_x_adjust.evaluate( d ) * coords::map_squares_per(
                                                    coords::omt ), dov_y_adjust.evaluate( d ) * coords::map_squares_per( coords::omt ), 0 );
        } else {
            target_pos = target_pos + tripoint( dov_x_adjust.evaluate( d ), dov_y_adjust.evaluate( d ), 0 );
        }

        if( z_override ) {
            target_pos = { target_pos.xy(), dov_z_adjust.evaluate( d ) };
        } else {
            target_pos = target_pos + tripoint( 0, 0, dov_z_adjust.evaluate( d ) );
        }
        if( output_var.has_value() ) {
            write_var_value( output_var.value().type, output_var.value().name, &d, target_pos );
        } else {
            write_var_value( input_var.type, input_var.name,   &d, target_pos );
        }
    };
}

talk_effect_fun_t::func f_transform_radius( const JsonObject &jo, std::string_view member,
        std::string_view, bool is_npc )
{
    str_or_var transform = get_str_or_var( jo.get_member( "ter_furn_transform" ),
                                           "ter_furn_transform", true );
    dbl_or_var dov = get_dbl_or_var( jo, member );
    duration_or_var dov_time_in_future = get_duration_or_var( jo, "time_in_future", false,
                                         0_seconds );
    std::optional<var_info> target_var;
    if( jo.has_member( "target_var" ) ) {
        target_var = read_var_info( jo.get_object( "target_var" ) );
    }
    str_or_var key;
    if( jo.has_member( "key" ) ) {
        key = get_str_or_var( jo.get_member( "key" ), "key", false, "" );
    } else {
        key.str_val = "";
    }
    return [dov, transform, target_var, dov_time_in_future, key, is_npc]( dialogue & d ) {
        tripoint_abs_ms target_pos = d.actor( is_npc )->pos_abs();
        if( target_var.has_value() ) {
            target_pos = read_var_value( *target_var, d ).tripoint();
        } else {
            target_pos = d.actor( is_npc )->pos_abs();
        }

        int radius = dov.evaluate( d );
        time_duration future = dov_time_in_future.evaluate( d );
        if( future > 0_seconds ) {
            get_timed_events().add( timed_event_type::TRANSFORM_RADIUS,
                                    calendar::turn + future + 1_seconds,
                                    //Timed events happen before the player turn and eocs are during so we add a second here to sync them up using the same variable
                                    -1, target_pos, radius, transform.evaluate( d ), key.evaluate( d ) );
        } else {
            map &bubble_map = reality_bubble();
            // Use the main map when possible to reduce performance overhead.
            if( bubble_map.inbounds( target_pos - point{ radius, radius} ) &&
                bubble_map.inbounds( target_pos + point{ radius, radius} ) ) {
                bubble_map.transform_radius( ter_furn_transform_id( transform.evaluate( d ) ), radius, target_pos );
            } else {
                map tm;
                tm.load( project_to<coords::sm>( target_pos - point{ radius, radius} ), false );
                tm.transform_radius( ter_furn_transform_id( transform.evaluate( d ) ), radius, target_pos );
            }

        }
    };
}

talk_effect_fun_t::func f_explosion( const JsonObject &jo, std::string_view member,
                                     std::string_view, bool is_npc )
{
    map &here = get_map();

    dbl_or_var dov_power;
    dbl_or_var dov_distance_factor;
    dbl_or_var dov_max_noise;
    bool fire;
    dbl_or_var dov_shrapnel_casing_mass = dbl_or_var( 0 );
    dbl_or_var dov_shrapnel_fragment_mass = dbl_or_var( 0.005f );
    dbl_or_var dov_shrapnel_recovery = dbl_or_var( 0 );
    str_or_var dov_shrapnel_drop = str_or_var( "null" );

    std::optional<var_info> target_var;
    if( jo.has_member( "target_var" ) ) {
        target_var = read_var_info( jo.get_object( "target_var" ) );
    }

    JsonObject jo_explosion = jo.get_object( member );
    dov_power = get_dbl_or_var( jo_explosion, "power", false, 0 );
    dov_distance_factor = get_dbl_or_var( jo_explosion, "distance_factor", false, 0.75f );
    dov_max_noise = get_dbl_or_var( jo_explosion, "max_noise", false, 90000000 );
    fire = jo_explosion.get_bool( "fire", false );
    if( jo_explosion.has_int( "shrapnel" ) ) {
        dov_shrapnel_casing_mass = get_dbl_or_var( jo_explosion, "casing_mass", true );
    } else if( jo.has_object( "shrapnel" ) ) {
        JsonObject jo_shr = jo_explosion.get_object( "shrapnel" );
        dov_shrapnel_casing_mass = get_dbl_or_var( jo_shr, "casing_mass", true );
        dov_shrapnel_fragment_mass = get_dbl_or_var( jo_shr, "fragment_mass", false, 0.08 );
        dov_shrapnel_recovery = get_dbl_or_var( jo_shr, "recovery", false, 0 );
        dov_shrapnel_drop = get_str_or_var( jo_shr.get_member( "drop" ), "drop", false, "null" );
    }

    bool emp_blast = jo.get_bool( "emp_blast", false );
    bool scrambler_blast = jo.get_bool( "scrambler_blast", false );

    return [target_var, dov_power, dov_distance_factor, dov_max_noise, fire, dov_shrapnel_casing_mass,
                        dov_shrapnel_fragment_mass, dov_shrapnel_recovery, dov_shrapnel_drop, emp_blast, scrambler_blast,
                is_npc, &here]( dialogue const & d ) {
        tripoint_bub_ms target_pos;
        if( target_var.has_value() ) {
            tripoint_abs_ms abs_ms = read_var_value( *target_var, d ).tripoint();
            target_pos = here.get_bub( abs_ms );
        } else {
            target_pos = d.actor( is_npc )->pos_bub( here );
        }
        Creature &guy = *d.actor( is_npc )->get_creature();

        explosion_data expl_data;
        expl_data.power = dov_power.evaluate( d );
        expl_data.distance_factor = dov_distance_factor.evaluate( d );
        expl_data.max_noise = dov_max_noise.evaluate( d );
        expl_data.fire = fire;
        expl_data.shrapnel.casing_mass = dov_shrapnel_casing_mass.evaluate( d );
        expl_data.shrapnel.fragment_mass = dov_shrapnel_fragment_mass.evaluate( d );
        expl_data.shrapnel.recovery = dov_shrapnel_recovery.evaluate( d );
        expl_data.shrapnel.drop = itype_id( dov_shrapnel_drop.evaluate( d ) );

        explosion_handler::explosion( &guy, target_pos, expl_data );

        if( emp_blast ) {
            explosion_handler::emp_blast( target_pos );
        }
        if( scrambler_blast ) {
            explosion_handler::scrambler_blast( target_pos );
        }
    };
}

talk_effect_fun_t::func f_query_tile( const JsonObject &jo, std::string_view member,
                                      std::string_view, bool is_npc )
{
    std::string type = jo.get_string( member.data() );
    var_info target_var = read_var_info( jo.get_object( "target_var" ) );
    std::string message;
    if( jo.has_member( "message" ) ) {
        message = jo.get_string( "message" );
    }
    dbl_or_var range = get_dbl_or_var( jo, "range", false, 0 );
    bool z_level = jo.get_bool( "z_level", false );
    std::optional<var_info> center_var;
    if( jo.has_member( "center_var" ) ) {
        center_var = read_var_info( jo.get_object( "center_var" ) );
    }
    return [type, target_var, message, range, z_level, center_var, is_npc]( dialogue & d ) {
        map &here = get_map();
        std::optional<tripoint_bub_ms> loc;
        Character *ch = d.actor( is_npc )->get_character();
        tripoint_bub_ms center;
        if( center_var.has_value() ) {
            tripoint_abs_ms abs_ms = read_var_value( *center_var, d ).tripoint();
            center = here.get_bub( abs_ms );
        } else {
            center = d.actor( is_npc )->pos_bub( here );
        }

        if( type != "line_of_sight" && range.evaluate( d ) != 0 ) {
            debugmsg( "Effect has range, but uses type \"%s\".  %s", type, d.get_callstack() );
        }

        if( ch && ch->as_avatar() ) {
            if( type == "anywhere" ) {
                if( !message.empty() ) {
                    static_popup popup;
                    popup.on_top( true );
                    popup.message( "%s", message );
                }
                const look_around_params looka_params = { true, center, center, false, true, true, z_level };
                loc = g->look_around( looka_params ).position;
            } else if( type == "line_of_sight" ) {
                if( !message.empty() ) {
                    static_popup popup;
                    popup.on_top( true );
                    popup.message( "%s", message );
                }
                avatar dummy;
                dummy.set_pos_abs_only( get_avatar().pos_abs() );
                target_handler::trajectory traj = target_handler::mode_select_only( dummy, range.evaluate( d ) );
                if( !traj.empty() ) {
                    loc = traj.back();
                }
            } else {
                debugmsg( string_format( "Invalid selection type: %s", type ) );
            }

        }
        if( loc.has_value() ) {
            tripoint_abs_ms pos_global = here.get_abs( *loc );
            write_var_value( target_var.type, target_var.name, &d, pos_global );
        }
        return loc.has_value();
    };
}

talk_effect_fun_t::func f_query_omt( const JsonObject &jo, std::string_view member,
                                     std::string_view, bool is_npc )
{
    std::optional<var_info> output_var;
    if( jo.has_member( "output_var" ) ) {
        output_var = read_var_info( jo.get_object( "output_var" ) );
    }

    var_info var = read_var_info( jo.get_object( member ) );

    std::optional<var_info> target_var;
    if( jo.has_member( "target_var" ) ) {
        target_var = read_var_info( jo.get_object( "target_var" ) );
    }

    translation_or_var message = get_translation_or_var( jo.get_member( "message" ), "message", true );
    dbl_or_var distance_limit = get_dbl_or_var( jo, "distance_limit", false, INT_MAX );
    dbl_or_var spread = get_dbl_or_var( jo, "spread", false, 1.0 );

    return [message, var, spread, distance_limit, target_var, is_npc]( dialogue & d ) {

        Character const *ch = d.const_actor( is_npc )->get_const_character();
        if( ch && ch->as_avatar() ) {

            tripoint_abs_omt target_pos = d.actor( is_npc )->pos_abs_omt();
            if( target_var.has_value() ) {
                tripoint_abs_ms abs_ms = read_var_value( *target_var, d ).tripoint();
                target_pos = project_to<coords::omt>( abs_ms );
            }

            tripoint_abs_omt pick = ui::omap::choose_point( message.evaluate( d ), target_pos, false,
                                    distance_limit.evaluate( d ) );
            if( pick != tripoint_abs_omt::invalid ) {
                tripoint_abs_ms abs_pos_ms = project_to<coords::ms>( pick );
                // aim at the center of OMT
                abs_pos_ms.x() += rng_float( 12 - spread.evaluate( d ), 12 + spread.evaluate( d ) );
                abs_pos_ms.y() += rng_float( 12 - spread.evaluate( d ), 12 + spread.evaluate( d ) );
                write_var_value( var.type, var.name, &d, abs_pos_ms );
                return;
            }
            return;
        }
        return;
    };
}

talk_effect_fun_t::func f_choose_adjacent_highlight( const JsonObject &jo, std::string_view member,
        std::string_view src, bool is_npc )
{
    map &here = get_map();

    var_info output_var = read_var_info( jo.get_object( member ) );

    std::optional<var_info> target_var;
    if( jo.has_member( "target_var" ) ) {
        target_var = read_var_info( jo.get_object( "target_var" ) );
    }

    // i hate get_translation_or_var() `required` var do not work
    translation_or_var message;
    if( jo.has_member( "message" ) ) {
        message = get_translation_or_var( jo.get_member( "message" ), "message", true );
    } else {
        message.str_val = translation();
    }

    translation_or_var failure_message;
    if( jo.has_member( "failure_message" ) ) {
        failure_message = get_translation_or_var( jo.get_member( "failure_message" ), "failure_message",
                          true );
    } else {
        failure_message.str_val = translation();
    }

    bool allow_vertical = jo.get_bool( "allow_vertical", false );
    bool allow_autoselect = jo.get_bool( "allow_autoselect", true );

    std::vector<effect_on_condition_id> false_eocs = load_eoc_vector( jo, "false_eocs", src );

    std::function<bool( const_dialogue const & )> cond;
    read_condition( jo, "condition", cond, true );

    return [output_var, target_var, message, failure_message, allow_vertical, allow_autoselect, cond,
                false_eocs, is_npc, &here]( dialogue & d ) {
        tripoint_bub_ms target_pos;
        if( target_var.has_value() ) {
            tripoint_abs_ms abs_ms = read_var_value( *target_var, d ).tripoint();
            target_pos = here.get_bub( abs_ms );
        } else {
            target_pos = d.actor( is_npc )->pos_bub( here );
        }

        const std::function<bool( const tripoint_bub_ms & )> f = [cond, &d, &here](
        const tripoint_bub_ms & pnt ) {
            write_var_value( var_type::context, "loc", &d, here.get_abs( pnt ) );
            return cond( d );
        };

        std::optional<tripoint_bub_ms> picked_coord = choose_adjacent_highlight( here, target_pos,
                message.evaluate( d ), failure_message.evaluate( d ), f, allow_vertical, allow_autoselect );

        if( picked_coord.has_value() ) {
            write_var_value( output_var.type, output_var.name, &d,
                             here.get_abs( picked_coord.value() ) );
        } else {
            run_eoc_vector( false_eocs, d );
        }
    };
}

talk_effect_fun_t::func f_mirror_coordinates( const JsonObject &jo, std::string_view member,
        std::string_view )
{
    // remove it once we get proper coordinate operations in math
    var_info center_var;
    if( jo.has_member( "center_var" ) ) {
        center_var = read_var_info( jo.get_object( "center_var" ) );
    }
    var_info relative_var;
    if( jo.has_member( "relative_var" ) ) {
        relative_var = read_var_info( jo.get_object( "relative_var" ) );
    }
    var_info output_var;
    if( jo.has_member( member ) ) {
        output_var = read_var_info( jo.get_object( member ) );
    }
    return [center_var, relative_var, output_var]( dialogue & d ) {
        tripoint_abs_ms const center = read_var_value( center_var, d ).tripoint();
        tripoint_abs_ms const relative = read_var_value( relative_var, d ).tripoint();

        tripoint_abs_ms const mirrored( center.x() * 2 - relative.x(),
                                        center.y() * 2 - relative.y(),
                                        center.z() * 2 - relative.z() );

        write_var_value( output_var.type, output_var.name, &d, mirrored );

    };
}

talk_effect_fun_t::func f_transform_line( const JsonObject &jo, std::string_view member,
        std::string_view )
{
    str_or_var transform = get_str_or_var( jo.get_member( member ), member, true );
    var_info first = read_var_info( jo.get_object( "first" ) );
    var_info second = read_var_info( jo.get_object( "second" ) );

    return [transform, first, second]( dialogue const & d ) {
        tripoint_abs_ms const t_first = read_var_value( first, d ).tripoint();
        tripoint_abs_ms const t_second = read_var_value( second, d ).tripoint();
        tripoint_abs_ms const orig = coord_min( t_first, t_second );
        map tm;
        tm.load( project_to<coords::sm>( orig ), false );
        tm.transform_line( ter_furn_transform_id( transform.evaluate( d ) ), t_first, t_second );
    };
}

talk_effect_fun_t::func f_place_override( const JsonObject &jo, std::string_view member,
        std::string_view )
{
    translation_or_var new_place = get_translation_or_var( jo.get_member( member ), member );
    duration_or_var dov_length = get_duration_or_var( jo, "length", true );
    str_or_var key;
    if( jo.has_member( "key" ) ) {
        key = get_str_or_var( jo.get_member( "key" ), "key", false, "" );
    } else {
        key.str_val = "";
    }
    return [new_place, dov_length, key]( dialogue & d ) {
        get_timed_events().add( timed_event_type::OVERRIDE_PLACE,
                                calendar::turn + dov_length.evaluate( d ) + 1_seconds,
                                //Timed events happen before the player turn and eocs are during so we add a second here to sync them up using the same variable
                                -1, tripoint_abs_ms::zero, -1, new_place.evaluate( d ), key.evaluate( d ) );
    };
}

talk_effect_fun_t::func f_mapgen_update( const JsonObject &jo, std::string_view member,
        std::string_view )
{
    mission_target_params target_params = mission_util::parse_mission_om_target( jo );
    std::vector<str_or_var> update_ids;
    duration_or_var dov_time_in_future = get_duration_or_var( jo, "time_in_future", false,
                                         0_seconds );
    if( jo.has_string( member ) ) {
        update_ids.emplace_back( get_str_or_var( jo.get_member( member ), member ) );
    } else if( jo.has_array( member ) ) {
        for( JsonValue jv : jo.get_array( member ) ) {
            update_ids.emplace_back( get_str_or_var( jv, member ) );
        }
    }
    std::optional<var_info> target_var;
    if( jo.has_member( "target_var" ) ) {
        target_var = read_var_info( jo.get_object( "target_var" ) );
    }
    str_or_var key;
    if( jo.has_member( "key" ) ) {
        key = get_str_or_var( jo.get_member( "key" ), "key", false, "" );
    } else {
        key.str_val = "";
    }
    return [target_params, update_ids, target_var, dov_time_in_future, key]( dialogue & d ) {
        tripoint_abs_omt omt_pos;
        if( target_var.has_value() ) {
            const tripoint_abs_ms abs_ms = read_var_value( *target_var, d ).tripoint();
            omt_pos = project_to<coords::omt>( abs_ms );
        } else {
            mission_target_params update_params = target_params;
            if( d.has_beta ) {
                update_params.guy = d.actor( true )->get_npc();
            }
            omt_pos = mission_util::get_om_terrain_pos( update_params, d );
        }
        time_duration future = dov_time_in_future.evaluate( d );
        if( future > 0_seconds ) {
            time_point tif = calendar::turn + future + 1_seconds;
            //Timed events happen before the player turn and eocs are during so we add a second here to sync them up using the same variable
            for( const str_or_var &mapgen_update_id : update_ids ) {
                get_timed_events().add( timed_event_type::UPDATE_MAPGEN, tif, -1, project_to<coords::ms>( omt_pos ),
                                        0, mapgen_update_id.evaluate( d ), key.evaluate( d ) );
            }

        } else {
            for( const str_or_var &mapgen_update_id : update_ids ) {
                run_mapgen_update_func( update_mapgen_id( mapgen_update_id.evaluate( d ) ), omt_pos, {},
                                        d.actor( d.has_beta )->selected_mission() );
                set_queued_points();
            }
            reality_bubble().invalidate_map_cache( omt_pos.z() );
        }
    };
}

talk_effect_fun_t::func f_alter_timed_events( const JsonObject &jo, std::string_view member,
        std::string_view )
{
    str_or_var key = get_str_or_var( jo.get_member( member ), member, true );
    duration_or_var time_in_future = get_duration_or_var( jo, "time_in_future", false,
                                     0_seconds );
    return [key, time_in_future]( dialogue & d ) {
        get_timed_events().set_all( key.evaluate( d ), time_in_future.evaluate( d ) );
    };
}

talk_effect_fun_t::func f_revert_location( const JsonObject &jo, std::string_view member,
        std::string_view )
{
    duration_or_var dov_time_in_future = get_duration_or_var( jo, "time_in_future", true );
    str_or_var key;
    if( jo.has_member( "key" ) ) {
        key = get_str_or_var( jo.get_member( "key" ), "key", false, "" );
    } else {
        key.str_val = "";
    }
    var_info target_var = read_var_info( jo.get_object( member ) );
    return [target_var, dov_time_in_future, key]( dialogue & d ) {
        const tripoint_abs_ms abs_ms = read_var_value( target_var, d ).tripoint();
        tripoint_abs_omt omt_pos = project_to<coords::omt>( abs_ms );
        time_point tif = calendar::turn + dov_time_in_future.evaluate( d ) + 1_seconds;
        // Timed events happen before the player turn and eocs are during so we add a second here to sync them up using the same variable
        // maptile is 4 submaps so queue up 4 submap reverts
        const tripoint_abs_sm revert_sm_base = project_to<coords::sm>( omt_pos );

        if( !MAPBUFFER.submap_exists( revert_sm_base ) ) {
            tinymap tm;
            // This creates the submaps if they didn't already exist.
            // Note that all four submaps are loaded/created by this
            // call, so the submap lookup can fail at most once.
            tm.load( omt_pos, true );
        }
        for( int x = 0; x < 2; x++ ) {
            for( int y = 0; y < 2; y++ ) {
                const tripoint_abs_sm revert_sm = revert_sm_base + point( x, y );
                submap *sm = MAPBUFFER.lookup_submap( revert_sm );
                get_timed_events().add( timed_event_type::REVERT_SUBMAP, tif, -1,
                                        project_to<coords::ms>( revert_sm ), 0, "",
                                        sm->get_revert_submap(), key.evaluate( d ) );
            }
        }

        reality_bubble().invalidate_map_cache( omt_pos.z() );
    };
}

talk_effect_fun_t::func f_npc_goal( const JsonObject &jo, std::string_view member,
                                    std::string_view src, bool is_npc )
{
    mission_target_params dest_params = mission_util::parse_mission_om_target( jo.get_object(
                                            member ) );
    std::vector<effect_on_condition_id> true_eocs = load_eoc_vector( jo, "true_eocs", src );
    std::vector<effect_on_condition_id> false_eocs = load_eoc_vector( jo, "false_eocs", src );
    return [dest_params, true_eocs, false_eocs, is_npc]( dialogue & d ) {
        npc *guy = d.actor( is_npc )->get_npc();
        if( guy ) {
            tripoint_abs_omt destination = mission_util::get_om_terrain_pos( dest_params, d );
            guy->goal = destination;
            guy->omt_path = overmap_buffer.get_travel_path( guy->pos_abs_omt(), guy->goal,
                            overmap_path_params::for_npc() ).points;
            if( destination == tripoint_abs_omt() || destination.is_invalid() ||
                guy->omt_path.empty() ) {
                guy->goal = npc::no_goal_point;
                guy->omt_path.clear();
                run_eoc_vector( false_eocs, d );
                return;
            }
            guy->set_mission( NPC_MISSION_TRAVELLING );
            guy->guard_pos = std::nullopt;
            guy->set_attitude( NPCATT_NULL );
            run_eoc_vector( true_eocs, d );
            return;
        }
        run_eoc_vector( false_eocs, d );
    };
}

talk_effect_fun_t::func f_guard_pos( const JsonObject &jo, std::string_view member,
                                     std::string_view, bool is_npc )
{
    var_info target_var = read_var_info( jo.get_object( member ) );
    bool unique_id = jo.get_bool( "unique_id", false );
    return [target_var, unique_id, is_npc]( dialogue const & d ) {
        npc *guy = d.actor( is_npc )->get_npc();
        if( guy ) {
            var_info cur_var = target_var;
            if( unique_id ) {
                cur_var.name.insert( 0, guy->get_unique_id() );
            }
            tripoint_abs_ms target_location = read_var_value( cur_var, d ).tripoint();
            guy->set_guard_pos( target_location );
        }
    };
}

talk_effect_fun_t::func f_bulk_trade_accept( const JsonObject &jo, std::string_view member,
        std::string_view, bool is_npc )
{
    dbl_or_var dov_quantity;
    if( jo.has_member( member ) ) {
        dov_quantity = get_dbl_or_var( jo, member, false, -1 );
    } else {
        dov_quantity.min.dbl_val = -1;
    }

    bool is_trade = member == "u_bulk_trade_accept" || member == "npc_bulk_trade_accept";

    return [is_trade, is_npc, dov_quantity]( dialogue & d ) {
        talker *seller = d.actor( is_npc );
        talker *buyer = d.actor( !is_npc );
        itype_id traded_itype_id = d.cur_item;
        int number_to_transfer = dov_quantity.evaluate( d );

        // Use a set! No duplicates! Duplicate pointers will be bad(crash) when we try to remove the item later.
        std::set<item *> items_to_transfer;

        seller->get_character()->visit_items( [&]( item * visited, item * parent ) {
            if( visited->typeId() == traded_itype_id ) {
                if( parent && ( parent->all_pockets_sealed() || visited->made_of( phase_id::LIQUID ) ||
                                parent->has_flag( json_flag_NO_UNLOAD ) ) ) {
                    items_to_transfer.emplace( parent );
                } else {
                    items_to_transfer.emplace( visited );
                }
            }
            return VisitResponse::NEXT;
        } );

        if( seller->get_character()->is_avatar() ) {
            std::string warning = _( "Really continue?  You will hand over the following items:" );
            int num_warnings = 0;
            for( item *checked_item : items_to_transfer ) {
                warning += "\n" + checked_item->tname();
                num_warnings++;
                if( !is_trade && num_warnings >= number_to_transfer ) {
                    break; // bulk donate has a variable number to transfer but bulk trade always takes everything
                }
            }
            if( !query_yn( warning ) ) {
                return;
            }
        }

        if( is_trade ) {

            const int npc_debt = d.actor( true )->debt();
            int price = total_price( *seller, traded_itype_id ) * ( is_npc ? -1 : 1 ) + npc_debt;
            if( d.actor( true )->get_faction() && d.actor( true )->get_faction()->currency.is_empty() ) {
                debugmsg( "%s has no faction currency to pay with in bulk_trade_accept!",
                          d.actor( true )->disp_name() );
                return; // Fatal, no reasonable way to recover.
            }

            // This can be very confusing, so for the benefit of future contributors I have elected to make the variable names
            // terribly verbose.
            const itype_id &currency_type = d.actor( true )->get_faction()->currency;
            item one_currency_unit( currency_type );
            const int int_value_of_one_currency_unit = d.actor( true )->value( one_currency_unit );
            if( int_value_of_one_currency_unit <= 0 ) {
                debugmsg( "%s pays in bulk_trade_accept with faction currency worth %d!",
                          d.actor( true )->disp_name(), int_value_of_one_currency_unit );
                return; // Fatal, no reasonable way to recover.
            }

            int num_transferred_currency_units = price / int_value_of_one_currency_unit;
            if( is_npc ) {
                num_transferred_currency_units = std::min( num_transferred_currency_units,
                                                 buyer->charges_of( currency_type ) );
                buyer->use_charges( currency_type, num_transferred_currency_units );
            } else {

                if( num_transferred_currency_units == 1 ) {
                    //~ %1%s is the NPC name, %2$s is an item
                    popup( _( "%1$s gives you a %2$s." ), buyer->disp_name(),
                           one_currency_unit.tname() );
                } else if( num_transferred_currency_units > 1 ) {
                    //~ %1%s is the NPC name, %2$d is a number of items, %3$s are items
                    popup( _( "%1$s gives you %2$d %3$s." ), buyer->disp_name(), num_transferred_currency_units,
                           one_currency_unit.tname() );
                }

            }

            // Currency is actually transferred
            for( int i = 0; i < num_transferred_currency_units; i++ ) {
                seller->i_add( one_currency_unit );
                price -= int_value_of_one_currency_unit;
            }

            // Tally up debts
            d.actor( true )->add_debt( -npc_debt );
            d.actor( true )->add_debt( price );

            // Now let's actually transfer the items!
            for( item *transferred : items_to_transfer ) {
                item transfer_copy( *transferred );
                buyer->i_add_or_drop( transfer_copy );

                auto remove_items_filter = [&]( const item & it ) {
                    return &it == const_cast<const item *>( transferred );
                };

                seller->remove_items_with( remove_items_filter );
            }


        } else { // u_bulk_donate

            int number_transferred = 0;
            for( item *transferred : items_to_transfer ) {
                item transfer_copy( *transferred );
                buyer->i_add_or_drop( transfer_copy );

                // Count now, while the item still exists
                if( transferred->typeId() != traded_itype_id ) {
                    // Container traded over
                    if( item( traded_itype_id ).count_by_charges() ) {
                        number_transferred += transferred->charges_of( traded_itype_id );
                    } else {
                        number_transferred += transferred->amount_of( traded_itype_id );
                    }
                } else {
                    // Loose items traded over
                    if( transferred->count_by_charges() ) {
                        number_transferred += transferred->charges;
                    } else {
                        number_transferred++;
                    }
                }

                auto remove_items_filter = [&]( const item & it ) {
                    return &it == const_cast<const item *>( transferred );
                };

                seller->remove_items_with( remove_items_filter );

                if( number_transferred >= number_to_transfer ) {
                    return;
                }
            }

        }
    };
}

talk_effect_fun_t::func f_npc_gets_item( bool to_use )
{
    return [to_use]( dialogue const & d ) {
        d.reason = d.actor( true )->give_item_to( to_use );
    };
}

talk_effect_fun_t::func f_add_mission( const JsonObject &jo, std::string_view member,
                                       std::string_view )
{
    str_or_var mission_id = get_str_or_var( jo.get_member( member ), member, true );
    return [mission_id]( dialogue const & d ) {
        d.actor( true )->add_mission( mission_type_id( mission_id.evaluate( d ) ) );
    };
}

talk_effect_fun_t::func f_u_buy_monster( const JsonObject &jo, std::string_view member,
        std::string_view src )
{
    str_or_var monster_type_id = get_str_or_var( jo.get_member( member ), member, true );
    dbl_or_var cost = get_dbl_or_var( jo, "cost", false, 0 );
    dbl_or_var count = get_dbl_or_var( jo, "count", false, 1 );
    const bool pacified = jo.get_bool( "pacified", false );
    translation_or_var name;
    if( jo.has_member( "name" ) ) {
        name = get_translation_or_var( jo.get_member( "name" ), "name", true );
    } else {
        name.str_val = translation();
    }
    std::vector<effect_on_condition_id> true_eocs = load_eoc_vector( jo, "true_eocs", src );
    std::vector<effect_on_condition_id> false_eocs = load_eoc_vector( jo, "false_eocs", src );
    return [monster_type_id, cost, count, pacified, name, true_eocs,
                     false_eocs]( dialogue & d ) {
        const mtype_id mtype( monster_type_id.evaluate( d ) );
        translation translated_name = no_translation( name.evaluate( d ) );
        if( d.actor( false )->buy_monster( *d.actor( true ), mtype, cost.evaluate( d ), count.evaluate( d ),
                                           pacified, translated_name ) ) {
            run_eoc_vector( true_eocs, d );
        } else {
            run_eoc_vector( false_eocs, d );
        }
    };
}


talk_effect_fun_t::func f_level_spell_class( const JsonObject &jo, std::string_view member,
        std::string_view, bool is_npc )
{
    str_or_var spell_class = get_str_or_var( jo.get_member( member ), member, true );
    dbl_or_var levels = get_dbl_or_var( jo, "levels", false, 1 );
    const bool random = jo.get_bool( "random", false );
    return [spell_class, levels, random, is_npc]( dialogue const & d ) {
        std::vector<spell_id> known_spells = d.actor( is_npc )->get_character()->magic->spells();
        std::vector<spell_id> valid_spells ;
        if( spell_class.evaluate( d ) == "all" ) {
            valid_spells = known_spells;
        } else {
            for( const spell_id &sp : known_spells ) {
                if( sp->spell_class == trait_id( spell_class.evaluate( d ) ) ) {
                    valid_spells.push_back( sp );
                }
            }
        }
        if( !valid_spells.empty() ) {
            if( random ) {
                std::shuffle( valid_spells.begin(), valid_spells.end(), rng_get_engine() );
                const spell_id improve = valid_spells.front();
                const int current_level = d.actor( is_npc )->get_spell_level( improve );
                d.actor( is_npc )->set_spell_level( improve, current_level + levels.evaluate( d ) );
            } else {
                for( const spell_id &improve : valid_spells ) {
                    const int current_level = d.actor( is_npc )->get_spell_level( improve );
                    d.actor( is_npc )->set_spell_level( improve, current_level + levels.evaluate( d ) );
                }
            }
        }
    };
}

talk_effect_fun_t::func f_learn_recipe( const JsonObject &jo, std::string_view member,
                                        std::string_view, bool is_npc )
{
    str_or_var learned_recipe_id = get_str_or_var( jo.get_member( member ), member, true );
    return [learned_recipe_id, is_npc]( dialogue const & d ) {
        const recipe_id &r = recipe_id( learned_recipe_id.evaluate( d ) );
        d.actor( is_npc )->learn_recipe( r );
    };
}

talk_effect_fun_t::func f_forget_recipe( const JsonObject &jo, std::string_view member,
        std::string_view, bool is_npc )
{
    const bool use_subcategory = jo.has_member( "subcategory" );
    const bool forgotten_recipe_is_category = jo.get_bool( "category", false ) || use_subcategory;
    str_or_var forgotten_recipe = get_str_or_var( jo.get_member( member ), member, true );
    std::optional<str_or_var> forgotten_recipe_subcategory = std::nullopt;
    if( use_subcategory ) {
        forgotten_recipe_subcategory = get_str_or_var( jo.get_member( "subcategory" ), "subcategory",
                                       true );
    }

    return [forgotten_recipe, forgotten_recipe_is_category, forgotten_recipe_subcategory,
                      is_npc]( dialogue const & d ) {
        if( forgotten_recipe_is_category ) {
            const recipe_subset &known_recipes = d.actor( is_npc )->get_character()->get_learned_recipes();
            const crafting_category_id category_to_use( forgotten_recipe.evaluate( d ) );
            const std::string subcategory_to_forget = !forgotten_recipe_subcategory ? "" :
                    forgotten_recipe_subcategory.value().evaluate( d );
            const std::vector<const recipe *> recipes_to_forget =
                recipes_from_cat( known_recipes, category_to_use, subcategory_to_forget ).first;
            for( const recipe *recipe_to_forget : recipes_to_forget ) {
                d.actor( is_npc )->forget_recipe( recipe_to_forget->ident() );
            }
        } else {
            const recipe_id &r = recipe_id( forgotten_recipe.evaluate( d ) );
            d.actor( is_npc )->forget_recipe( r );
        }
    };
}

talk_effect_fun_t::func f_turn_cost( const JsonObject &jo, std::string_view member,
                                     std::string_view )
{
    duration_or_var cost = get_duration_or_var( jo, member, true );
    return [cost]( dialogue & d ) {
        Character *target = d.actor( false )->get_character();
        if( target ) {
            target->mod_moves( -to_moves<int>( cost.evaluate( d ) ) );
        }
    };
}

talk_effect_fun_t::func f_npc_first_topic( const JsonObject &jo, std::string_view member,
        std::string_view )
{
    str_or_var chat_topic = get_str_or_var( jo.get_member( member ), member, true );
    return [chat_topic]( dialogue const & d ) {
        d.actor( true )->set_first_topic( chat_topic.evaluate( d ) );
    };
}

talk_effect_fun_t::func f_message( const JsonObject &jo, std::string_view member,
                                   std::string_view, bool is_npc )
{
    const bool snippet = jo.get_bool( "snippet", false );
    str_or_var snip_id;
    translation_or_var message;
    if( snippet ) {
        snip_id = get_str_or_var( jo.get_member( member ), member );
    } else {
        message = get_translation_or_var( jo.get_member( member ), member );
    }
    const bool same_snippet = jo.get_bool( "same_snippet", false );
    const bool outdoor_only = jo.get_bool( "outdoor_only", false );
    const bool store_in_lore = jo.get_bool( "store_in_lore", false );
    const bool sound = jo.get_bool( "sound", false );
    const bool popup_msg = jo.get_bool( "popup", false );
    const bool popup_w_interrupt_query_msg = jo.get_bool( "popup_w_interrupt_query", false );
    const PopupFlags popup_flag = jo.has_member( "popup_flag" ) ?
                                  popup_flag_from_string( jo.get_string( "popup_flag" ) ) : PF_NONE;
    str_or_var interrupt_type;
    if( jo.has_member( "interrupt_type" ) ) {
        interrupt_type = get_str_or_var( jo.get_member( "interrupt_type" ), "interrupt_type", true );
    } else {
        interrupt_type.str_val = "default";
    }
    const bool global = member == "message";
    str_or_var type_string;
    if( jo.has_member( "type" ) ) {
        type_string = get_str_or_var( jo.get_member( "type" ), "type", true );
    } else {
        type_string.str_val = "neutral";
    }
    return [snip_id, message, outdoor_only, sound, snippet, same_snippet, type_string, popup_msg,
                     popup_w_interrupt_query_msg, popup_flag, interrupt_type, global, store_in_lore, is_npc]
    ( dialogue const & d ) {
        Character *target;
        if( global ) {
            target = &get_player_character();
        } else {
            target = d.actor( is_npc )->get_character();
        }
        if( !target || target->is_npc() ) {
            return;
        }
        game_message_type type = m_neutral;
        if( type_string.evaluate( d ) == "good" ) {
            type = m_good;
        } else if( type_string.evaluate( d ) == "neutral" ) {
            type = m_neutral;
        } else if( type_string.evaluate( d ) == "bad" ) {
            type = m_bad;
        } else if( type_string.evaluate( d ) == "mixed" ) {
            type = m_mixed;
        } else if( type_string.evaluate( d ) == "warning" ) {
            type = m_warning;
        } else if( type_string.evaluate( d ) == "info" ) {
            type = m_info;
        } else if( type_string.evaluate( d ) == "debug" ) {
            type = m_debug;
        } else if( type_string.evaluate( d ) == "headshot" ) {
            type = m_headshot;
        } else if( type_string.evaluate( d ) == "critical" ) {
            type = m_critical;
        } else if( type_string.evaluate( d ) == "grazing" ) {
            type = m_grazing;
        } else {
            debugmsg( "Invalid message type." );
        }
        std::string translated_message;
        std::string sid;
        if( snippet ) {
            if( same_snippet ) {
                talker *target_talker = d.actor( !is_npc );
                sid = target_talker->get_value( snip_id.evaluate( d ) + "_snippet_id" ).to_string();
                if( sid.empty() ) {
                    sid = SNIPPET.random_id_from_category( snip_id.evaluate( d ) ).c_str();
                    target_talker->set_value( snip_id.evaluate( d ) + "_snippet_id", sid );
                    if( store_in_lore ) {
                        target->as_avatar()->add_snippet( snippet_id( sid ) );
                    }
                }
                translated_message = SNIPPET.expand( SNIPPET.get_snippet_by_id( snippet_id( sid ) ).value_or(
                        translation() ).translated() );
            } else {
                if( store_in_lore ) {
                    sid = SNIPPET.random_id_from_category( snip_id.evaluate( d ) ).c_str();
                    target->as_avatar()->add_snippet( snippet_id( sid ) );
                    translated_message = SNIPPET.expand( SNIPPET.get_snippet_by_id( snippet_id( sid ) ).value_or(
                            translation() ).translated() );
                } else {
                    translated_message = SNIPPET.expand( SNIPPET.random_from_category( snip_id.evaluate( d ) ).value_or(
                            translation() ).translated() );
                }
            }
        } else {
            translated_message = message.evaluate( d );
        }
        std::unique_ptr<talker> default_talker = get_talker_for( get_player_character() );
        talker &alpha = d.has_alpha ? *d.actor( false ) : *default_talker;
        talker &beta = d.has_beta ? *d.actor( true ) : *default_talker;
        parse_tags( translated_message, alpha, beta, d );
        if( sound ) {
            bool display = false;
            map &here = get_map();
            if( !target->has_effect( effect_sleep ) && !target->is_deaf() ) {
                if( !outdoor_only || here.get_abs_sub().z() >= 0 ||
                    one_in( std::max( roll_remainder( 2.0f * here.get_abs_sub().z() / target->hearing_ability() ),
                                      1 ) ) ) {
                    display = true;
                }
            }
            if( !display ) {
                return;
            }
        }
        if( popup_msg ) {
            popup( translated_message, popup_flag );
            g->cancel_activity_or_ignore_query( distraction_type::eoc, "" );
        }
        if( popup_w_interrupt_query_msg ) {
            if( interrupt_type.evaluate( d ) == "portal_storm_popup" ) {
                g->portal_storm_query( distraction_type::portal_storm_popup,
                                       translated_message );
            } else if( interrupt_type.evaluate( d ) == "default" ) {
                debugmsg( "Interrupt query called in json without proper interrupt type." );
            }
            // Would probably need an else-if for every possible distraction type, like this:
            //else if (interrupt_type == "hostile_spotted_near"){
            //    g->cancel_activity_or_ignore_query(distraction_type::hostile_spotted_near, "sample message");
            //}
            // I leave this to contributors who might actually wish to implement such interrupts,
            // so as to not overcomplicate the code.
        } else {
            target->add_msg_if_player( type, translated_message );
        }

    };
}

talk_effect_fun_t::func f_assign_activity( const JsonObject &jo, std::string_view member,
        std::string_view, bool is_npc )
{
    duration_or_var dov = get_duration_or_var( jo, "duration", true );
    str_or_var act = get_str_or_var( jo.get_member( member ), member, true );
    return [is_npc, dov, act]( dialogue & d ) {
        Character *target = d.actor( is_npc )->get_character();
        if( target ) {
            target->assign_activity( activity_id( act.evaluate( d ) ), to_moves<int>( dov.evaluate( d ) ) );
        }
    };
}

talk_effect_fun_t::func f_add_wet( const JsonObject &jo, std::string_view member,
                                   std::string_view, bool is_npc )
{
    dbl_or_var dov = get_dbl_or_var( jo, member );
    return [is_npc, dov]( dialogue & d ) {
        Character *target = d.actor( is_npc )->get_character();
        if( target ) {
            wet_character( *target, dov.evaluate( d ) );
        }
    };
}

talk_effect_fun_t::func f_open_dialogue( const JsonObject &jo, std::string_view member,
        std::string_view src )
{
    std::vector<effect_on_condition_id> true_eocs;
    std::vector<effect_on_condition_id> false_eocs;
    str_or_var topic;
    bool has_member = false;
    if( jo.has_object( member ) ) {
        has_member = true;
        JsonObject innerJo = jo.get_object( member );
        true_eocs = load_eoc_vector( innerJo, "true_eocs", src );
        false_eocs = load_eoc_vector( innerJo, "false_eocs", src );
        topic = get_str_or_var( innerJo.get_member( "topic" ), "topic" );
    }
    return [true_eocs, false_eocs, topic, has_member]( dialogue const & d ) {
        std::string actual_topic;
        if( has_member ) {
            actual_topic = topic.evaluate( d );
        }
        if( !d.actor( false )->get_character()->is_avatar() ) { //only open a dialog if the avatar is alpha
            run_eoc_vector( false_eocs, d );
            return;
        } else if( !actual_topic.empty() ) {
            get_avatar().talk_to( get_talker_for( std::vector<std::string> { actual_topic } ), false, false,
                                  true );
        } else {
            get_avatar().talk_to( d.actor( true )->clone() );
        }
        run_eoc_vector( true_eocs, d );
    };
}

talk_effect_fun_t::func f_take_control( const JsonObject &jo, std::string_view,
                                        std::string_view src )
{
    std::vector<effect_on_condition_id> true_eocs = load_eoc_vector( jo, "true_eocs", src );
    std::vector<effect_on_condition_id> false_eocs = load_eoc_vector( jo, "false_eocs", src );
    return [true_eocs, false_eocs]( dialogue const & d ) {
        if( !d.actor( false )->get_character()->is_avatar() ) { //only take control if the avatar is alpha
            run_eoc_vector( false_eocs, d );
            return;
        } else if( d.actor( true )->get_npc() != nullptr ) {
            get_avatar().control_npc( *d.actor( true )->get_npc() );
        }
        run_eoc_vector( true_eocs, d );
    };
}

talk_effect_fun_t::func f_take_control_menu()
{
    return []( dialogue const &/* d */ ) {
        get_avatar().control_npc_menu();
    };
}

talk_effect_fun_t::func f_sound_effect( const JsonObject &jo, std::string_view member,
                                        std::string_view )
{
    str_or_var variant = get_str_or_var( jo.get_member( member ), member, true );
    str_or_var id = get_str_or_var( jo.get_member( "id" ), "id", true );
    const bool outdoor_event = jo.get_bool( "outdoor_event", false );
    dbl_or_var volume;
    if( jo.has_member( "volume" ) ) {
        volume = get_dbl_or_var( jo, "volume", false, -1 );
    } else {
        volume.min.dbl_val = -1;
    }
    return [variant, id, outdoor_event, volume]( dialogue & d ) {
        map &here = get_map();
        int local_volume = volume.evaluate( d );
        Character *target = &get_player_character(); //Only the player can hear sound effects.
        if( target && !target->has_effect( effect_sleep ) && !target->is_deaf() ) {
            if( !outdoor_event || here.get_abs_sub().z() >= 0 ) {
                if( local_volume == -1 ) {
                    local_volume = 80;
                }
                sfx::play_variant_sound( id.evaluate( d ), variant.evaluate( d ), local_volume,
                                         random_direction() );
            } else if( one_in( std::max( roll_remainder( 2.0f * here.get_abs_sub().z() /
                                         target->hearing_ability() ), 1 ) ) ) {
                if( local_volume == -1 ) {
                    local_volume = 80 * target->hearing_ability();
                }
                sfx::play_variant_sound( id.evaluate( d ), variant.evaluate( d ), local_volume,
                                         random_direction() );
            }
        }
    };
}

talk_effect_fun_t::func f_give_achievment( const JsonObject &jo, std::string_view member,
        std::string_view )
{
    str_or_var achieve = get_str_or_var( jo.get_member( member ), member, true );
    return [achieve]( dialogue const & d ) {
        const achievement_id achievement_to_give( achieve.evaluate( d ) );
        // make sure the achievement is being tracked and that it is currently pending
        std::vector<const achievement *> all_achievements = get_achievements().valid_achievements();
        if( std::find_if( all_achievements.begin(),
        all_achievements.end(), [&achievement_to_give]( const achievement * ach ) {
        return ach->id == achievement_to_give;
    } ) != all_achievements.end() ) {
            if( get_achievements().is_completed( achievement_to_give ) == achievement_completion::pending ) {
                get_achievements().report_achievement( &achievement_to_give.obj(),
                                                       achievement_completion::completed );
            }
        }
    };
}

talk_effect_fun_t::func f_cast_spell( const JsonObject &jo, std::string_view member,
                                      std::string_view src, bool is_npc )
{
    std::vector<effect_on_condition_id> true_eocs = load_eoc_vector( jo, "true_eocs", src );
    std::vector<effect_on_condition_id> false_eocs = load_eoc_vector( jo, "false_eocs", src );
    bool targeted = jo.get_bool( "targeted", false );

    std::optional<var_info> loc_var;
    if( jo.has_object( "loc" ) ) {
        loc_var = read_var_info( jo.get_object( "loc" ) );
    }
    JsonObject spell_jo = jo.get_object( member );
    str_or_var id = get_str_or_var( spell_jo.get_member( "id" ), "id" );
    bool hit_self = spell_jo.get_bool( "hit_self", false );

    int trigger_once_in = spell_jo.get_int( "once_in", 1 );
    translation_or_var trigger_message;
    if( spell_jo.has_member( "message" ) ) {
        trigger_message = get_translation_or_var( spell_jo.get_member( "message" ), "message", true );
    } else {
        trigger_message.str_val = translation();
    }
    translation_or_var npc_trigger_message;
    if( spell_jo.has_member( "npc_message" ) ) {
        npc_trigger_message = get_translation_or_var( spell_jo.get_member( "npc_message" ), "npc_message",
                              true );
    } else {
        npc_trigger_message.str_val = translation();
    }

    dbl_or_var dov_max_level = get_dbl_or_var( spell_jo, "max_level", false, -1 );
    dbl_or_var level = get_dbl_or_var( spell_jo, "min_level", false );
    if( spell_jo.has_string( "level" ) ) {
        debugmsg( "level member for fake_spell was renamed to min_level.  id: %s",
                  id.str_val.value_or( "" ) );
    }

    return [is_npc, id, hit_self, dov_max_level, trigger_once_in, level, trigger_message,
                    npc_trigger_message, targeted, loc_var, true_eocs,
            false_eocs]( dialogue & d ) {
        std::optional<int> max_level;
        int max_level_int = dov_max_level.evaluate( d );
        if( max_level_int == -1 ) {
            max_level = std::nullopt;
        } else {
            max_level = max_level_int;
        }
        fake_spell fake( spell_id( id.evaluate( d ) ), hit_self, max_level );
        fake.trigger_once_in = trigger_once_in;
        fake.level = level.evaluate( d );
        fake.trigger_message = no_translation( trigger_message.evaluate( d ) );
        fake.npc_trigger_message = no_translation( npc_trigger_message.evaluate( d ) );
        Creature *caster = d.actor( is_npc )->get_creature();
        if( !caster ) {
            debugmsg( "No valid caster for spell.  %s", d.get_callstack() );
            run_eoc_vector( false_eocs, d );
            return;
        } else {
            if( !fake.is_valid() ) {
                debugmsg( "%s is not a valid spell.  %s", fake.id.c_str(), d.get_callstack() );
                run_eoc_vector( false_eocs, d );
                return;
            }
            spell sp = fake.get_spell( *caster, 0 );
            if( targeted ) {
                if( std::optional<tripoint_bub_ms> target = sp.select_target( caster ) ) {
                    sp.cast_all_effects( *caster, *target );
                    caster->add_msg_player_or_npc( fake.trigger_message, fake.npc_trigger_message );
                }
            } else {
                const tripoint_bub_ms target_pos = loc_var
                                                   ? get_map().get_bub( read_var_value( *loc_var, d ).tripoint() )
                                                   : caster->pos_bub();
                sp.cast_all_effects( *caster, target_pos );
                caster->add_msg_player_or_npc( fake.trigger_message, fake.npc_trigger_message );
            }
        }
        run_eoc_vector( true_eocs, d );
    };
}

talk_effect_fun_t::func f_attack( const JsonObject &jo, std::string_view member,
                                  std::string_view, bool is_npc )
{
    str_or_var force_technique = get_str_or_var( jo.get_member( member ), member, true );
    bool allow_special = jo.get_bool( "allow_special", true );
    bool allow_unarmed = jo.get_bool( "allow_unarmed", true );
    dbl_or_var forced_movecost = get_dbl_or_var( jo, "forced_movecost", false, -1.0 );

    return [is_npc, allow_special, force_technique, allow_unarmed,
            forced_movecost]( dialogue & d ) {
        // if beta is attacking then target is the alpha
        talker *target = d.actor( !is_npc );
        talker *attacker = d.actor( is_npc );
        Creature *c = target->get_creature();

        if( c ) {
            matec_id m( force_technique.evaluate( d ) );
            attacker->attack_target( *c, allow_special, m, allow_unarmed, forced_movecost.evaluate( d ) );
        }
    };
}


talk_effect_fun_t::func f_ranged_attack( bool is_npc )
{

    return [is_npc]( dialogue & d ) {
        // if beta is attacking then target is the alpha
        talker *target = d.actor( !is_npc );
        Character *attacker = d.actor( is_npc )->get_character();
        Creature *c = target->get_creature();

        if( c ) {
            tripoint_bub_ms target_pos = c->pos_bub();
            item_location wielded = attacker->get_wielded_item();
            if( wielded->is_gun() ) {
                gun_mode mode = wielded->gun_current_mode();
                if( wielded->ammo_sufficient( attacker, mode.qty * 2 ) ) {
                    attacker->fire_gun( target_pos, mode.qty );

                }
            }
        }
    };
}

talk_effect_fun_t::func f_die( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        map &here = get_map();
        d.actor( is_npc )->die( &here );
    };
}

talk_effect_fun_t::func f_die_advanced( const JsonObject &jo, std::string_view member,
                                        std::string_view, bool is_npc )
{
    JsonObject job = jo.get_object( member );
    std::optional<bool> remove_corpse;
    std::optional<bool> supress_message;
    std::optional<bool> remove_from_creature_tracker;

    if( job.has_bool( "remove_corpse" ) ) {
        remove_corpse = job.get_bool( "remove_corpse" );
    }
    if( job.has_bool( "supress_message" ) ) {
        supress_message = job.get_bool( "supress_message" );
    }
    if( job.has_bool( "remove_from_creature_tracker" ) ) {
        remove_from_creature_tracker = job.get_bool( "remove_from_creature_tracker" );
    }

    return [remove_corpse, supress_message, remove_from_creature_tracker,
                   is_npc]( dialogue const & d ) {
        map &here = get_map();

        if( d.actor( is_npc )->get_monster() ) {
            monster &mon = *d.actor( is_npc )->get_monster();
            if( remove_from_creature_tracker ) {
                get_creature_tracker().remove( mon );
                return;
            }
            mon.death_drops = remove_corpse.has_value() ? !remove_corpse.value() : mon.death_drops;
            mon.quiet_death = supress_message.has_value() ? supress_message.value() : mon.quiet_death;
        } else if( d.actor( is_npc )->get_npc() ) {
            npc &guy_npc = *d.actor( is_npc )->get_npc();
            guy_npc.spawn_corpse = remove_corpse.has_value() ? !remove_corpse.value() : guy_npc.spawn_corpse;
            guy_npc.quiet_death = supress_message.has_value() ? supress_message.value() : guy_npc.quiet_death;
        }

        d.actor( is_npc )->die( &here );
    };
}

talk_effect_fun_t::func f_cancel_activity( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        Character *target = d.actor( is_npc )->get_character();
        if( target ) {
            target->cancel_activity();
        }
    };
}

talk_effect_fun_t::func f_prevent_death( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        Character *ch = d.actor( is_npc )->get_character();
        if( ch ) {
            ch->prevent_death();
        }
    };
}

talk_effect_fun_t::func f_lightning()
{
    return []( dialogue const &/* d */ ) {
        if( get_player_character().posz() >= 0 ) {
            get_weather().lightning_active = true;
        }
    };
}

talk_effect_fun_t::func f_next_weather()
{
    return []( dialogue const &/* d */ ) {
        get_weather().set_nextweather( calendar::turn );
    };
}

talk_effect_fun_t::func f_copy_var( const JsonObject &jo, std::string_view member,
                                    std::string_view /* src */ )
{
    var_info from = read_var_info( jo.get_member( member ) );
    var_info to = read_var_info( jo.get_member( "target_var" ) );

    return [from, to]( dialogue & d ) {
        diag_value const &fd = read_var_value( from, d );
        write_var_value( to.type, to.name, &d, fd );
    };
}

talk_effect_fun_t::func f_set_string_var( const JsonObject &jo, std::string_view member,
        std::string_view )
{
    const bool i18n = jo.get_bool( "i18n", false );
    std::optional<string_input_params> input_params;
    if( jo.has_object( "string_input" ) ) {
        JsonObject input_obj = jo.get_object( "string_input" );
        input_params = string_input_params::parse_string_input_params( input_obj );
    }

    std::vector<str_or_var> str_vals;
    std::vector<translation_or_var> i18n_vals;
    if( jo.has_array( member ) ) {
        for( JsonValue value : jo.get_array( member ) ) {
            if( i18n ) {
                i18n_vals.emplace_back( get_translation_or_var( value, member ) );
            } else {
                str_vals.emplace_back( get_str_or_var( value, member ) );
            }
        }
    } else {
        if( i18n ) {
            i18n_vals.emplace_back( get_translation_or_var( jo.get_member( member ), member ) );
        } else {
            str_vals.emplace_back( get_str_or_var( jo.get_member( member ), member ) );
        }
    }
    bool parse = jo.get_bool( "parse_tags", false );
    var_info var = read_var_info( jo.get_member( "target_var" ) );
    return [i18n, input_params, str_vals, i18n_vals, var, parse]( dialogue & d ) {
        int index = rng( 0, ( i18n ? i18n_vals.size() : str_vals.size() ) - 1 );
        std::string str = i18n ? i18n_vals[index].evaluate( d ) : str_vals[index].evaluate( d );

        if( input_params.has_value() ) {
            string_input_popup popup;
            popup
            .title( input_params.value().title ? input_params.value().title->evaluate( d ) : "" )
            .description( input_params.value().description ? input_params.value().description->evaluate(
                              d ) : "" )
            .width( input_params.value().width )
            .identifier( input_params.value().identifier ? input_params.value().identifier->evaluate(
                             d ) : "" );

            std::string str_temp = input_params.value().default_text ?
                                   input_params.value().default_text->evaluate( d ) : "";
            popup.edit( str_temp );
            if( !popup.canceled() ) {
                str = str_temp;
            }
        }

        if( parse ) {
            std::unique_ptr<talker> default_talker = get_talker_for( get_player_character() );
            talker &alpha = d.has_alpha ? *d.actor( false ) : *default_talker;
            talker &beta = d.has_beta ? *d.actor( true ) : *default_talker;
            parse_tags( str, alpha, beta, d );
        }
        write_var_value( var.type, var.name, &d, str );
    };
}

talk_effect_fun_t::func f_set_condition( const JsonObject &jo, std::string_view member,
        std::string_view )
{
    str_or_var value;
    value = get_str_or_var( jo.get_member( member ), member );

    std::function<bool( const_dialogue const & )> cond;
    read_condition( jo, "condition", cond, false );
    return [value, cond]( dialogue & d ) {
        d.set_conditional( value.evaluate( d ), cond );
    };
}

talk_effect_fun_t::func f_set_item_category_spawn_rates( const JsonObject &jo,
        std::string_view member, std::string_view )
{
    if( jo.has_array( member ) ) {
        std::set<std::pair<item_category_id, float>> rates;
        for( const JsonObject joi : jo.get_array( member ) ) {
            item_category_id cat( joi.get_string( "id" ) );
            float spawn_rate = joi.get_float( "spawn_rate" );
            rates.insert( std::make_pair( cat, spawn_rate ) );
        }
        return [rates]( dialogue const &/* d */ ) {
            for( const std::pair<item_category_id, float> &rate : rates ) {
                rate.first.obj().set_spawn_rate( rate.second );
            }
        };
    } else {
        JsonObject joi = jo.get_member( member );
        item_category_id cat( joi.get_string( "id" ) );
        float spawn_rate = joi.get_float( "spawn_rate" );
        return [cat, spawn_rate]( dialogue const &/* d */ ) {
            cat.obj().set_spawn_rate( spawn_rate );
        };
    }
}

talk_effect_fun_t::func f_assign_mission( const JsonObject &jo, std::string_view member,
        std::string_view )
{
    str_or_var mission_name = get_str_or_var( jo.get_member( member ), member, true );
    dbl_or_var deadline = get_dbl_or_var( jo, "deadline", false );
    return [mission_name, deadline]( dialogue & d ) {
        avatar &player_character = get_avatar();

        const mission_type_id &mission_type = mission_type_id( mission_name.evaluate( d ) );
        mission *new_mission = mission::reserve_new( mission_type, character_id() );
        new_mission->assign( player_character );
        // mission::assign assigns the type's deadline (which is often 0!), so if we want our specific
        // deadline to be kept, it must always be done after assignment
        const double deadline_dbl = deadline.evaluate( d );
        if( deadline_dbl ) {
            new_mission->set_deadline( calendar::turn_zero + time_duration::from_turns( deadline_dbl ) );
        }
    };
}

talk_effect_fun_t::func f_finish_mission( const JsonObject &jo, std::string_view member,
        std::string_view )
{
    str_or_var mission_name = get_str_or_var( jo.get_member( member ), member, true );
    bool success = false;
    std::optional<int> step;
    if( jo.has_int( "step" ) ) {
        step = jo.get_int( "step" );
    } else {
        success = jo.get_bool( "success" );
    }
    return [mission_name, success, step]( dialogue const & d ) {
        avatar &player_character = get_avatar();
        const mission_type_id &mission_type = mission_type_id( mission_name.evaluate( d ) );
        std::vector<mission *> missions = player_character.get_active_missions();

        for( mission *mission : missions ) {
            if( mission->mission_id() == mission_type ) {
                if( step.has_value() ) {
                    mission->step_complete( step.value() );
                } else if( success ) {
                    mission->wrap_up();
                } else {
                    mission->fail();
                }
                break;
            }
        }
    };
}

talk_effect_fun_t::func f_remove_active_mission( const JsonObject &jo,
        std::string_view member, std::string_view )
{
    str_or_var mission_name = get_str_or_var( jo.get_member( member ), member, true );
    return [mission_name]( dialogue const & d ) {
        avatar &player_character = get_avatar();
        const mission_type_id &mission_type = mission_type_id( mission_name.evaluate( d ) );
        std::vector<mission *> missions = player_character.get_active_missions();
        for( mission *mission : missions ) {
            if( mission->mission_id() == mission_type ) {
                player_character.remove_active_mission( *mission );
                break;
            }
        }
    };
}

talk_effect_fun_t::func f_offer_mission( const JsonObject &jo, std::string_view member,
        std::string_view )
{
    std::vector<std::string> mission_names;

    if( jo.has_array( member ) ) {
        for( const std::string mission_name : jo.get_array( member ) ) {
            mission_names.push_back( mission_name );
        }
    } else if( jo.has_string( member ) ) {
        mission_names.push_back( jo.get_string( std::string( member ) ) );
    } else {
        jo.throw_error( "Invalid input for set_offer_mission" );
    }

    return [mission_names]( dialogue const & d ) {
        // assume that the alpha is the npc if there isn't a beta
        npc *p = d.actor( d.has_beta )->get_npc();

        if( p ) {
            for( const std::string &mission_name : mission_names ) {
                p->add_new_mission( mission::reserve_new( mission_type_id( mission_name ), p->getID() ) );
            }
        }
    };
}

talk_effect_fun_t::func f_set_flag( const JsonObject &jo, std::string_view member,
                                    std::string_view, bool is_npc )
{
    str_or_var flag = get_str_or_var( jo.get_member( member ), member, true );

    return [is_npc, flag]( dialogue & d ) {
        item_location *it = d.actor( is_npc )->get_item();

        if( it && it->get_item() ) {
            flag_id f_id( flag.evaluate( d ) );
            it->get_item()->set_flag( f_id );
        } else {
            debugmsg( "No valid %s talker.", is_npc ? "beta" : "alpha" );
        }
    };
}

talk_effect_fun_t::func f_unset_flag( const JsonObject &jo, std::string_view member,
                                      std::string_view, bool is_npc )
{
    str_or_var flag = get_str_or_var( jo.get_member( member ), member, true );

    return [is_npc, flag]( dialogue & d ) {
        item_location *it = d.actor( is_npc )->get_item();

        if( it && it->get_item() ) {
            flag_id f_id( flag.evaluate( d ) );
            it->get_item()->unset_flag( f_id );
        } else {
            debugmsg( "No valid %s talker.", is_npc ? "beta" : "alpha" );
        }
    };
}

talk_effect_fun_t::func f_set_fault( const JsonObject &jo, std::string_view member,
                                     const std::string_view, bool is_npc )
{
    str_or_var fault_var = get_str_or_var( jo.get_member( member ), member, true );
    bool force = jo.get_bool( "force", false );
    bool msg = jo.get_bool( "message", true );
    return [fault_var, force, msg, is_npc]( dialogue const & d ) {
        item_location &it = *d.actor( is_npc )->get_item();
        it.get_item()->set_fault( fault_id( fault_var.evaluate( d ) ), force, msg );
    };
}

talk_effect_fun_t::func f_set_random_fault_of_type( const JsonObject &jo, std::string_view member,
        const std::string_view, bool is_npc )
{
    str_or_var fault_type_var = get_str_or_var( jo.get_member( member ), member, true );
    bool force = jo.get_bool( "force", false );
    bool msg = jo.get_bool( "message", true );
    return [fault_type_var, force, msg, is_npc]( dialogue const & d ) {
        item_location &it = *d.actor( is_npc )->get_item();
        it.get_item()->set_random_fault_of_type( fault_type_var.evaluate( d ), force, msg );
    };
}

talk_effect_fun_t::func f_activate( const JsonObject &jo, std::string_view member,
                                    std::string_view, bool is_npc )
{
    map &here = get_map();

    str_or_var method = get_str_or_var( jo.get_member( member ), member, true );
    std::optional<var_info> target_var;
    if( jo.has_member( "target_var" ) ) {
        target_var = read_var_info( jo.get_object( "target_var" ) );
    }

    return [is_npc, method, target_var, &here]( dialogue & d ) {
        Character *guy = d.actor( is_npc )->get_character();
        item_location *it = d.actor( !is_npc )->get_item();

        if( guy ) {
            const std::string method_str = method.evaluate( d );
            if( it && it->get_item() ) {
                if( !it->get_item()->get_usable_item( method_str ) ) {
                    add_msg_debug( debugmode::DF_NPC, "Invalid use action.  %s", method_str );
                    return;
                }
                if( target_var.has_value() ) {
                    tripoint_abs_ms target_pos = read_var_value( *target_var, d ).tripoint();
                    if( get_map().inbounds( target_pos ) ) {
                        guy->invoke_item( it->get_item(), method_str, here.get_bub( target_pos ) );
                        return;
                    }
                }
                guy->invoke_item( it->get_item(), method.evaluate( d ) );

            } else {
                debugmsg( "%s talker must be Item.", is_npc ? "alpha" : "beta" );
            }
        } else {
            debugmsg( "%s talker must be Character.", is_npc ? "beta" : "alpha" );
        }
    };
}

talk_effect_fun_t::func f_math( const JsonObject &jo, std::string_view member,
                                std::string_view )
{
    eoc_math math;
    math.from_json( jo, member, math_type_t::assign );
    return [math = std::move( math )]( dialogue & d ) {
        return math.act( d );
    };
}


talk_effect_fun_t::func f_transform_item( const JsonObject &jo, std::string_view member,
        std::string_view )
{
    str_or_var target_id = get_str_or_var( jo.get_member( member ), member, true );
    bool activate = jo.get_bool( "active", false );

    return [target_id, activate]( dialogue & d ) {
        item_location *it = d.actor( true )->get_item();

        if( it && it->get_item() ) {
            const std::string target_str = target_id.evaluate( d );
            ( *it )->convert( itype_id( target_str ), it->carrier() );
            ( *it )->active = activate || ( *it )->has_temperature();
        } else {
            debugmsg( "beta talker must be Item." );
        }
    };
}

talk_effect_fun_t::func f_make_sound( const JsonObject &jo, std::string_view member,
                                      std::string_view, bool is_npc )
{
    dbl_or_var volume = get_dbl_or_var( jo, "volume", true );
    bool ambient = jo.get_bool( "ambient", false );
    bool snippet = jo.get_bool( "snippet", false );
    bool same_snippet = jo.get_bool( "same_snippet", false );
    str_or_var snip_id;
    translation_or_var message;
    if( snippet ) {
        snip_id = get_str_or_var( jo.get_member( member ), member, true );
    } else {
        message = get_translation_or_var( jo.get_member( member ), member, true );
    }
    sounds::sound_t type = sounds::sound_t::background;
    std::string type_string = jo.get_string( "type", "background" );
    if( type_string == "background" ) {
        type = sounds::sound_t::background;
    } else if( type_string == "weather" ) {
        type = sounds::sound_t::weather;
    } else if( type_string == "music" ) {
        type = sounds::sound_t::music;
    } else if( type_string == "movement" ) {
        type = sounds::sound_t::movement;
    } else if( type_string == "speech" ) {
        type = sounds::sound_t::speech;
    } else if( type_string == "electronic_speech" ) {
        type = sounds::sound_t::electronic_speech;
    } else if( type_string == "activity" ) {
        type = sounds::sound_t::activity;
    } else if( type_string == "destructive_activity" ) {
        type = sounds::sound_t::destructive_activity;
    } else if( type_string == "alarm" ) {
        type = sounds::sound_t::alarm;
    } else if( type_string == "combat" ) {
        type = sounds::sound_t::combat;
    } else if( type_string == "alert" ) {
        type = sounds::sound_t::alert;
    } else if( type_string == "order" ) {
        type = sounds::sound_t::order;
    } else {
        jo.throw_error( "Invalid message type." );
    }
    std::optional<var_info> target_var;
    if( jo.has_member( "target_var" ) ) {
        target_var = read_var_info( jo.get_object( "target_var" ) );
    }
    return [is_npc, snip_id, message, volume, ambient, type, target_var, snippet,
            same_snippet]( dialogue & d ) {
        tripoint_abs_ms target_pos;
        if( target_var ) {
            target_pos = read_var_value( *target_var, d ).tripoint();
        } else {
            target_pos = d.actor( is_npc )->pos_abs();
        }
        std::string translated_message;
        if( snippet ) {
            if( same_snippet ) {
                talker *target = d.actor( !is_npc );
                std::string sid = target->get_value( snip_id.evaluate( d ) + "_snippet_id" ).to_string();
                if( sid.empty() ) {
                    sid = SNIPPET.random_id_from_category( snip_id.evaluate( d ) ).c_str();
                    target->set_value( snip_id.evaluate( d ) + "_snippet_id", sid );
                }
                translated_message = SNIPPET.expand( SNIPPET.get_snippet_by_id( snippet_id( sid ) ).value_or(
                        translation() ).translated() );
            } else {
                translated_message = SNIPPET.expand( SNIPPET.random_from_category( snip_id.evaluate( d ) ).value_or(
                        translation() ).translated() );
            }
        } else {
            translated_message = message.evaluate( d );
        }
        sounds::sound( reality_bubble().get_bub( target_pos ), volume.evaluate( d ), type,
                       translated_message,
                       ambient );
    };
}

void process_eoc( const effect_on_condition_id &eoc, dialogue &d,
                  time_duration time_in_future )
{
    if( eoc->type == eoc_type::ACTIVATION ) {
        Character *alpha = d.has_alpha ? d.actor( false )->get_character() : nullptr;
        if( alpha ) {
            effect_on_conditions::queue_effect_on_condition( time_in_future, eoc, *alpha, d.get_context() );
        } else if( eoc->global ) {
            effect_on_conditions::queue_effect_on_condition( time_in_future, eoc, get_player_character(),
                    d.get_context() );
        }
        // If the target is a monster or item and the eoc is non global it won't be queued and will silently "fail"
        // this is so monster attacks against other monsters won't give error messages.
    } else {
        debugmsg( "Cannot queue a non activation effect_on_condition.  %s", d.get_callstack() );
    }
}

void queue_eocs( const std::vector<eoc_entry> &eocs, dialogue &d, dialogue newDialog,
                 duration_or_var const &dov_time, bool random_time )
{
    time_duration time_in_future = dov_time.evaluate( d );
    for( const eoc_entry &entry : eocs ) {
        effect_on_condition_id eoc_id =
            entry.var ? effect_on_condition_id( entry.var->evaluate( d ) ) : entry.id;
        if( random_time ) {
            process_eoc( eoc_id, newDialog, dov_time.evaluate( d ) );
        } else {
            process_eoc( eoc_id, newDialog, time_in_future );
        }
    }
}

void run_eocs( const std::vector<eoc_entry> &eocs, dialogue &d, dialogue newDialog,
               std::optional<dbl_or_var> const &iterations,
               std::optional<std::function<bool( dialogue & )>> const &cond )
{
    int i = 0;

    // if it's a loop, we will use iterations_amount as limit to amount of loops allowed
    // and bump it to 100 as default value
    int iterations_amount = iterations ? iterations->evaluate( d ) : cond ? 100 : 1;
    while( i < iterations_amount && ( !cond || ( *cond )( d ) ) ) {
        for( const eoc_entry &entry : eocs ) {
            effect_on_condition_id eoc_id =
                entry.var ? effect_on_condition_id( entry.var->evaluate( d ) ) : entry.id;
            eoc_id->activate( newDialog );
        }
        ++i;
    }
}

std::unique_ptr<talker> get_talker( dialogue const &d, std::optional<str_or_var> const &var,
                                    std::optional<var_info> loc, bool parent_beta )
{
    if( !var && !loc ) {
        if( d.has_actor( parent_beta ) ) {
            return d.actor( parent_beta )->clone();
        }
        return {};
    }

    Creature *crit = nullptr;

    if( var ) {
        std::string str = var->evaluate( d );

        if( str.empty() ) {
            return {};
        } else if( str == "u" && d.has_alpha ) {
            return  d.actor( false )->clone();
        } else if( str == "npc" && d.has_beta ) {
            return d.actor( true )->clone();
        } else if( str == "avatar" ) {
            return get_talker_for( get_avatar() );
        } else {
            crit = get_character_from_id( str, g.get() );
        }

    } else if( loc ) {
        crit = get_creature_tracker().creature_at( read_var_value( *loc, d ).tripoint() );
    }

    if( crit != nullptr ) {
        return get_talker_for( *crit );
    }

    debugmsg( R"(run_eocs error: no valid talker\n%s)", d.get_callstack() );
    return {};
}

talk_effect_fun_t::func f_run_eocs( const JsonObject &jo, std::string_view member,
                                    std::string_view src )
{
    std::vector<eoc_entry> eocs = load_eoc_vector_id_and_var( jo, member, src );
    if( eocs.empty() ) {
        jo.throw_error( "Invalid input for run_eocs" );
    }

    std::optional<dbl_or_var> iterations;
    if( jo.has_member( "iterations" ) ) {
        iterations = get_dbl_or_var( jo, "iterations" );
    }
    std::optional<std::function<bool( const_dialogue const & )>> cond;
    if( jo.has_object( "condition" ) ) {
        std::function<bool( const_dialogue const & )> cond_;
        read_condition( jo, "condition", cond_, false );
        cond = { std::move( cond_ ) };
    }

    std::optional<duration_or_var> dov_time;
    bool random_time = false;
    if( jo.has_member( "time_in_future" ) ) {
        if( iterations || cond ) {
            jo.throw_error_at(
                "time_in_future",
                R"("time_in_future" cannot be used with loops ("condition" or "iterations"). Use nested EOCs instead.)" );
        }
        dov_time = get_duration_or_var( jo, "time_in_future" );
        random_time = jo.get_bool( "randomize_time_in_future", false );
    }

    std::unordered_map<std::string, str_translation_or_var> context;
    if( jo.has_object( "variables" ) ) {
        const JsonObject &variables = jo.get_object( "variables" );
        for( const JsonMember &jv : variables ) {
            context[jv.name()] = get_str_translation_or_var( jv, jv.name(), true );
        }
    }

    std::optional<str_or_var> alpha_var;
    std::optional<var_info> alpha_loc;
    std::optional<str_or_var> beta_var;
    std::optional<var_info> beta_loc;

    if( jo.has_member( "beta_loc" ) ) {
        beta_loc = read_var_info( jo.get_object( "beta_loc" ) );
    } else if( jo.has_member( "beta_talker" ) ) {
        beta_var = get_str_or_var( jo.get_member( "beta_talker" ), "beta_talker" );
    }

    if( jo.has_member( "alpha_loc" ) ) {
        alpha_loc = read_var_info( jo.get_object( "alpha_loc" ) );
    } else if( jo.has_member( "alpha_talker" ) ) {
        alpha_var = get_str_or_var( jo.get_member( "alpha_talker" ), "alpha_talker" );
    }

    std::vector<effect_on_condition_id> false_eocs = load_eoc_vector( jo, "false_eocs", src );

    return [eocs, cond, iterations, dov_time, random_time, alpha_var, beta_var, alpha_loc,
          beta_loc, false_eocs, context]( dialogue & d ) {

        std::unique_ptr<talker> alpha = get_talker( d, alpha_var, alpha_loc, false );
        std::unique_ptr<talker> beta = get_talker( d, beta_var, beta_loc, true );
        if( !alpha && !beta ) {
            run_eoc_vector( false_eocs, d );
            return;
        }

        dialogue newDialog{ std::move( alpha ), std::move( beta ), d.get_conditionals(),
                            d.get_context() };

        for( const auto &val : context ) {
            // FXIXME: typed variables
            newDialog.set_value( val.first, diag_value::legacy_value{ val.second.evaluate( d, true ) } );
        }

        if( dov_time ) {
            queue_eocs( eocs, d, newDialog, *dov_time, random_time );
        } else {
            run_eocs( eocs, d, newDialog, iterations, cond );
        }
    };
}

talk_effect_fun_t::func f_run_eoc_selector( const JsonObject &jo, std::string_view member,
        std::string_view src )
{

    std::vector<eoc_entry> eocs = load_eoc_vector_id_and_var( jo, member, src );
    if( eocs.empty() ) {
        jo.throw_error( "Invalid input for run_eocs" );
    }

    std::vector<translation_or_var> eoc_names;
    if( jo.has_array( "names" ) ) {
        for( const JsonValue &jv : jo.get_array( "names" ) ) {
            eoc_names.emplace_back( get_translation_or_var( jv, "names", true ) );
        }
    }

    std::vector<translation_or_var> eoc_descriptions;
    if( jo.has_array( "descriptions" ) ) {
        for( const JsonValue &jv : jo.get_array( "descriptions" ) ) {
            eoc_descriptions.emplace_back( get_translation_or_var( jv, "descriptions", true ) );
        }
    }

    std::vector<char> eoc_keys;
    if( jo.has_array( "keys" ) ) {
        for( const JsonValue &jv : jo.get_array( "keys" ) ) {
            std::string val = jv.get_string();
            if( val.size() != 1 ) {
                jo.throw_error( "Invalid input for run_eoc_selector, key strings must be exactly 1 character." );
            } else {
                eoc_keys.push_back( val[0] );
            }
        }
    }

    if( !eoc_names.empty() && eoc_names.size() != eocs.size() ) {
        jo.throw_error( "Invalid input for run_eoc_selector, size of eocs and names needs to be identical, or names need to be empty" );
    }

    if( !eoc_descriptions.empty() && eoc_descriptions.size() != eocs.size() ) {
        jo.throw_error( "Invalid input for run_eoc_selector, size of eocs and descriptions needs to be identical, or descriptions need to be empty" );
    }

    if( !eoc_keys.empty() && eoc_keys.size() != eocs.size() ) {
        jo.throw_error( "Invalid input for run_eoc_selector, size of eocs and keys needs to be identical, or keys need to be empty." );
    }

    std::vector<std::unordered_map<std::string, str_translation_or_var>> context;
    if( jo.has_array( "variables" ) ) {
        for( const JsonValue &member : jo.get_array( "variables" ) ) {
            const JsonObject &variables = member.get_object();
            std::unordered_map<std::string, str_translation_or_var> temp_context;
            for( const JsonMember &jv : variables ) {
                temp_context[jv.name()] = get_str_translation_or_var( jv, jv.name(), true );
            }
            context.emplace_back( temp_context );
        }
    }

    if( !context.empty() && context.size() != 1 && context.size() != eocs.size() ) {
        jo.throw_error(
            string_format( "Invalid input for run_eoc_selector, size of vars needs to be 0 (no vars), 1 (all have the same vars), or the same size as the eocs (each has their own vars). Current size is: %d",
                           context.size() ) );
    }

    bool hide_failing = jo.get_bool( "hide_failing", false );
    bool allow_cancel = jo.get_bool( "allow_cancel", false );
    bool hilight_disabled = jo.get_bool( "hilight_disabled", false ) ;

    translation title = to_translation( "Select an option." );
    jo.read( "title", title );

    return [eocs, context, title, eoc_names, eoc_keys, eoc_descriptions,
          hide_failing, allow_cancel, hilight_disabled]( dialogue & d ) {
        uilist eoc_list;

        std::unique_ptr<talker> default_talker = get_talker_for( get_player_character() );
        talker &alpha = d.has_alpha ? *d.actor( false ) : *default_talker;
        talker &beta = d.has_beta ? *d.actor( true ) : *default_talker;


        eoc_list.text = title.translated();
        eoc_list.allow_cancel = allow_cancel;
        eoc_list.desc_enabled = !eoc_descriptions.empty();
        eoc_list.hilight_disabled = hilight_disabled;
        parse_tags( eoc_list.text, alpha, beta, d );

        for( size_t i = 0; i < eocs.size(); i++ ) {

            effect_on_condition_id eoc_id =
                eocs[i].var ? effect_on_condition_id( eocs[i].var->evaluate( d ) ) : eocs[i].id;
            // check and set condition
            bool display = false;
            if( eoc_id->has_condition ) {
                // if it has a condition check that it is true
                display = eoc_id->test_condition( d );
            } else {
                display = true;
            }

            if( hide_failing && !display ) {
                // skip hidden entries
                continue;
            }

            auto wrap60 = []( const std::string & text ) {
                return string_join( foldstring( text, 60 ), "\n" );
            };
            std::string name;
            std::string description;
            if( eoc_names.empty() ) {
                name = eoc_id.str();
            } else {
                name = eoc_names[i].evaluate( d );
                parse_tags( name, alpha, beta, d );
            }
            if( !eoc_descriptions.empty() ) {
                description = eoc_descriptions[i].evaluate( d );
                parse_tags( description, alpha, beta, d );
                description = wrap60( description );
            }

            if( eoc_keys.empty() ) {
                eoc_list.entries.emplace_back( static_cast<int>( i ), display, std::nullopt, name, description );
            } else {
                eoc_list.entries.emplace_back( static_cast<int>( i ), display, eoc_keys[i], name, description );
            }
        }

        if( eoc_list.entries.empty() ) {
            // if we have no entries should exit with error
            debugmsg( "No options for EOC_LIST" );
            return;
        }

        eoc_list.query();
        if( eoc_list.ret < 0 ) {
            return;
        }

        // add context variables
        dialogue newDialog( d );
        int contextIndex = 0;
        if( context.size() > 1 ) {
            contextIndex = eoc_list.ret;
        }
        if( !context.empty() ) {
            for( const auto &val : context[contextIndex] ) {
                // FIXME: typed variables
                newDialog.set_value( val.first, diag_value::legacy_value{ val.second.evaluate( d, true ) } );
            }
        }

        effect_on_condition_id chosen_eoc_id =
            eocs[eoc_list.ret].var ? effect_on_condition_id( eocs[eoc_list.ret].var->evaluate(
                        d ) ) : eocs[eoc_list.ret].id;
        chosen_eoc_id->activate( newDialog );
    };
}

talk_effect_fun_t::func f_run_npc_eocs( const JsonObject &jo,
                                        std::string_view member, std::string_view src, bool is_npc )
{
    const map &here = get_map();

    std::vector<effect_on_condition_id> eocs = load_eoc_vector( jo, member, src );
    std::vector<str_or_var> unique_ids;
    for( JsonValue jv : jo.get_array( "unique_ids" ) ) {
        unique_ids.emplace_back( get_str_or_var( jv, "unique_ids" ) );
    }

    bool local = jo.get_bool( "local", false );
    std::optional<int> npc_range;
    if( jo.has_int( "npc_range" ) ) {
        npc_range = jo.get_int( "npc_range" );
    }
    bool npc_must_see = jo.get_bool( "npc_must_see", false );
    if( local ) {
        return [eocs, unique_ids, npc_must_see, npc_range, is_npc, &here]( dialogue const & d ) {
            tripoint_bub_ms actor_pos = d.actor( is_npc )->pos_bub( here );
            std::vector<std::string> ids;
            ids.reserve( unique_ids.size() );
            for( const str_or_var &id : unique_ids ) {
                ids.emplace_back( id.evaluate( d ) );
            }
            const std::vector<npc *> available = g->get_npcs_if( [npc_must_see, npc_range, actor_pos,
                          ids, &here]( const npc & guy ) {
                bool id_valid = ids.empty();
                for( const std::string &id : ids ) {
                    if( id == guy.get_unique_id() ) {
                        id_valid = true;
                        break;
                    }
                }
                return id_valid && ( !npc_range.has_value() || actor_pos.z() == guy.posz() ) && ( !npc_must_see ||
                        guy.sees( here, actor_pos ) ) &&
                       ( !npc_range.has_value() || rl_dist( actor_pos, guy.pos_bub( here ) ) <= npc_range.value() );
            } );
            for( npc *target : available ) {
                for( const effect_on_condition_id &eoc : eocs ) {
                    dialogue newDialog( get_talker_for( target ), nullptr, d.get_conditionals(), d.get_context() );
                    eoc->activate( newDialog );
                }
            }
        };
    } else {
        return [eocs, unique_ids]( dialogue const & d ) {
            for( const str_or_var &target : unique_ids ) {
                if( g->unique_npc_exists( target.evaluate( d ) ) ) {
                    for( const effect_on_condition_id &eoc : eocs ) {
                        npc *npc = g->find_npc_by_unique_id( target.evaluate( d ) );
                        if( npc ) {
                            dialogue newDialog( get_talker_for( npc ), nullptr, d.get_conditionals(), d.get_context() );
                            eoc->activate( newDialog );
                        } else {
                            debugmsg( "Tried to use invalid npc: %s. %s", target.evaluate( d ), d.get_callstack() );
                        }
                    }
                }
            }
        };
    }
}

talk_effect_fun_t::func f_run_monster_eocs( const JsonObject &jo,
        std::string_view member, std::string_view src, bool is_npc )
{
    const map &here = get_map();

    std::vector<effect_on_condition_id> eocs = load_eoc_vector( jo, member, src );
    std::vector<str_or_var> mtype_ids;
    for( JsonValue jv : jo.get_array( "mtype_ids" ) ) {
        mtype_ids.emplace_back( get_str_or_var( jv, "mtype_ids" ) );
    }
    std::optional<int> monster_range;
    if( jo.has_int( "monster_range" ) ) {
        monster_range = jo.get_int( "monster_range" );
    }
    bool monster_must_see = jo.get_bool( "monster_must_see", false );
    return [eocs, mtype_ids, monster_must_see, monster_range, is_npc, &here]( dialogue const & d ) {
        std::vector<mtype_id> ids;
        ids.reserve( mtype_ids.size() );
        for( const str_or_var &id : mtype_ids ) {
            ids.emplace_back( id.evaluate( d ) );
        }
        tripoint_bub_ms actor_pos = d.actor( is_npc )->pos_bub( here );
        const std::vector<Creature *> available = g->get_creatures_if( [ ids, monster_must_see,
             monster_range, actor_pos, &here ]( const Creature & critter ) {
            bool id_valid = ids.empty();
            bool creature_is_monster = critter.is_monster();
            if( creature_is_monster ) {
                for( const mtype_id &id : ids ) {
                    if( id == critter.as_monster()->type->id ) {
                        id_valid = true;
                        break;
                    }
                }
            }
            return creature_is_monster && id_valid && ( !monster_range.has_value() ||
                    actor_pos.z() == critter.posz() ) &&
                   ( !monster_must_see ||
                     critter.sees( here, actor_pos ) ) &&
                   ( !monster_range.has_value() ||
                     rl_dist( actor_pos, critter.pos_bub( here ) ) <= monster_range.value() );
        } );
        for( Creature *target : available ) {
            for( const effect_on_condition_id &eoc : eocs ) {
                dialogue newDialog( get_talker_for( target ), nullptr, d.get_conditionals(), d.get_context() );
                eoc->activate( newDialog );
            }
        }
    };
}

talk_effect_fun_t::func f_run_vehicle_eocs( const JsonObject &jo,
        std::string_view member, std::string_view src, bool is_npc )
{
    std::vector<effect_on_condition_id> eocs = load_eoc_vector( jo, member, src );
    std::optional<int> vehicle_range;
    if( jo.has_int( "vehicle_range" ) ) {
        vehicle_range = jo.get_int( "vehicle_range" );
    }
    return [eocs, vehicle_range, is_npc]( dialogue const & d ) {
        tripoint_abs_ms actor_pos = d.actor( is_npc )->pos_abs();
        for( wrapped_vehicle &elem : get_map().get_vehicles() ) {
            vehicle &veh = *elem.v;
            if( !vehicle_range.has_value() ||
                rl_dist( actor_pos, veh.pos_abs() ) <= vehicle_range.value() ) {
                for( const effect_on_condition_id &eoc : eocs ) {
                    dialogue newDialog( get_talker_for( veh ), nullptr, d.get_conditionals(), d.get_context() );
                    eoc->activate( newDialog );
                }
            }
        }
    };
}

talk_effect_fun_t::func f_run_inv_eocs( const JsonObject &jo,
                                        std::string_view member, std::string_view src, bool is_npc )
{
    str_or_var option = get_str_or_var( jo.get_member( member ), member );
    std::vector<effect_on_condition_id> true_eocs = load_eoc_vector( jo, "true_eocs", src );
    std::vector<effect_on_condition_id> false_eocs = load_eoc_vector( jo, "false_eocs", src );
    std::vector <item_search_data> data;
    for( const JsonValue &search_data_jo : jo.get_array( "search_data" ) ) {
        data.emplace_back( search_data_jo );
    }
    translation_or_var title;
    if( jo.has_member( "title" ) ) {
        title = get_translation_or_var( jo.get_member( "title" ), "title", true );
    } else {
        title.str_val = translation();
    }

    return [option, true_eocs, false_eocs, data, is_npc, title]( dialogue & d ) {
        Character *guy = d.actor( is_npc )->get_character();
        if( guy ) {
            const auto f = [d, guy, title]( const item_location_filter & filter ) {
                return game_menus::inv::titled_filter_menu( filter, *guy, title.evaluate( d ) );
            };
            const auto f_mul = [d, guy, title]( const item_location_filter & filter ) {
                return game_menus::inv::titled_multi_filter_menu( filter, *guy, title.evaluate( d ) );
            };
            run_item_eocs( d, is_npc, guy->all_items_loc(), option.evaluate( d ), true_eocs, false_eocs, data,
                           f, f_mul );
        }
    };
}

talk_effect_fun_t::func f_map_run_item_eocs( const JsonObject &jo, std::string_view member,
        std::string_view src, bool is_npc )
{
    str_or_var option = get_str_or_var( jo.get_member( member ), member );
    std::vector<effect_on_condition_id> true_eocs = load_eoc_vector( jo, "true_eocs", src );
    std::vector<effect_on_condition_id> false_eocs = load_eoc_vector( jo, "false_eocs", src );
    std::vector <item_search_data> data;
    for( const JsonValue &search_data_jo : jo.get_array( "search_data" ) ) {
        data.emplace_back( search_data_jo );
    }
    translation_or_var title;
    if( jo.has_member( "title" ) ) {
        title = get_translation_or_var( jo.get_member( "title" ), "title", true );
    } else {
        title.str_val = translation();
    }
    std::optional<var_info> loc_var;
    if( jo.has_object( "loc" ) ) {
        loc_var = read_var_info( jo.get_object( "loc" ) );
    }
    dbl_or_var dov_min_radius = get_dbl_or_var( jo, "min_radius", false, 0 );
    dbl_or_var dov_max_radius = get_dbl_or_var( jo, "max_radius", false, 0 );

    return [is_npc, option, true_eocs, false_eocs, data, loc_var, dov_min_radius, dov_max_radius,
            title]( dialogue & d ) {
        tripoint_abs_ms target_location;
        if( loc_var ) {
            target_location = read_var_value( *loc_var, d ).tripoint();
        } else {
            target_location = d.actor( is_npc )->pos_abs();
        }
        std::vector<item_location> items;
        map &here = get_map();
        tripoint_bub_ms center = here.get_bub( target_location );
        int max_radius = dov_max_radius.evaluate( d );
        int min_radius = dov_min_radius.evaluate( d );
        for( const tripoint_bub_ms &pos : here.points_in_radius( center, max_radius ) ) {
            if( rl_dist( center, pos ) >= min_radius && here.inbounds( pos ) ) {
                for( item &it : here.i_at( pos ) ) {
                    items.emplace_back( map_cursor( pos ), &it );
                }
            }
        }
        Character *guy = d.actor( is_npc )->get_character();
        guy = guy ? guy : &get_player_character();
        const auto f = [d, guy, title, center, min_radius,
           max_radius, &here]( const item_location_filter & filter ) {
            inventory_filter_preset preset( filter );
            inventory_pick_selector inv_s( *guy, preset );
            inv_s.set_title( title.evaluate( d ) );
            inv_s.set_display_stats( false );
            inv_s.clear_items();
            for( const tripoint_bub_ms &pos : here.points_in_radius( center, max_radius ) ) {
                if( rl_dist( center, pos ) >= min_radius ) {
                    inv_s.add_map_items( pos );
                }
            }
            if( inv_s.empty() ) {
                popup( _( "You don't have the necessary item at hand." ), PF_GET_KEY );
                return item_location();
            }
            return inv_s.execute();
        };
        const item_menu_mul f_mul = [d, guy, title, center, min_radius,
           max_radius, &here]( const item_location_filter & filter ) {
            inventory_filter_preset preset( filter );
            inventory_multiselector inv_s( *guy, preset );
            inv_s.set_title( title.evaluate( d ) );
            inv_s.set_display_stats( false );
            inv_s.clear_items();
            for( const tripoint_bub_ms &pos : here.points_in_radius( center, max_radius ) ) {
                if( rl_dist( center, pos ) >= min_radius ) {
                    inv_s.add_map_items( pos );
                }
            }
            if( inv_s.empty() ) {
                popup( _( "You don't have the necessary item at hand." ), PF_GET_KEY );
                return drop_locations();
            }
            return inv_s.execute();
        };
        run_item_eocs( d, is_npc, items, option.evaluate( d ), true_eocs, false_eocs, data,
                       f, f_mul );
    };
}

talk_effect_fun_t::func f_map_run_eocs( const JsonObject &jo, std::string_view member,
                                        std::string_view src, bool is_npc )
{
    std::vector<effect_on_condition_id> eocs = load_eoc_vector( jo, member, src );
    std::optional<var_info> target_var;
    if( jo.has_member( "target_var" ) ) {
        target_var = read_var_info( jo.get_object( "target_var" ) );
    }
    std::function<bool( const_dialogue const & )> cond;
    read_condition( jo, "condition", cond, true );
    dbl_or_var range = get_dbl_or_var( jo, "range", false, 1 );

    var_info store_coordinates_in;
    if( jo.has_member( "store_coordinates_in" ) ) {
        store_coordinates_in = read_var_info( jo.get_member( "store_coordinates_in" ) );
    }
    bool stop_at_first = jo.get_bool( "stop_at_first", true );

    return [is_npc, eocs, target_var, cond, range, store_coordinates_in,
            stop_at_first]( dialogue & d ) {

        tripoint_abs_ms pos;
        if( target_var.has_value() ) {
            pos = read_var_value( *target_var, d ).tripoint();
        } else {
            pos = d.actor( is_npc )->pos_abs();
        }

        std::vector<tripoint_abs_ms> adjacent = closest_points_first( pos, range.evaluate( d ) );

        for( tripoint_abs_ms point : adjacent ) {
            write_var_value( store_coordinates_in.type, store_coordinates_in.name, &d, point );
            for( effect_on_condition_id eoc_id : eocs ) {
                if( cond( d ) ) {
                    eoc_id->activate( d );
                    if( stop_at_first ) {
                        return;
                    }
                }
            }
        }
    };
}

talk_effect_fun_t::func f_set_talker( const JsonObject &jo, std::string_view member,
                                      std::string_view, bool is_npc )
{
    var_info var = read_var_info( jo.get_object( member ) );
    var_type type = var.type;
    std::string var_name = var.name;
    return [is_npc, var, type, var_name]( dialogue & d ) {
        int id = d.actor( is_npc )->getID().get_value();
        // stringify for `run_eocs` - see get_talker() above
        write_var_value( type, var_name, &d, std::to_string( id ) );
    };
}

talk_effect_fun_t::func f_weighted_list_eocs( const JsonObject &jo,
        std::string_view member, std::string_view src )
{
    std::vector<std::pair<effect_on_condition_id, dbl_or_var>> eoc_pairs;
    for( JsonArray ja : jo.get_array( member ) ) {
        JsonValue eoc = ja.next_value();
        dbl_or_var eoc_weight;
        if( ja.test_int() ) {
            eoc_weight.min = dbl_or_var_part{ ja.next_int() };
        } else {
            eoc_weight.min = get_dbl_or_var_part( ja.next_value() );
        }
        eoc_pairs.emplace_back( effect_on_conditions::load_inline_eoc( eoc, src ),
                                eoc_weight );
    }
    return [eoc_pairs]( dialogue & d ) {
        weighted_int_list<effect_on_condition_id> eocs;
        for( const std::pair<effect_on_condition_id, dbl_or_var> &eoc_pair :
             eoc_pairs ) {
            eocs.add( eoc_pair.first, eoc_pair.second.evaluate( d ) );
        }
        effect_on_condition_id picked_eoc = *eocs.pick();
        dialogue newDialog( d );
        picked_eoc->activate( newDialog );
    };
}

talk_effect_fun_t::func f_if( const JsonObject &jo, std::string_view member,
                              std::string_view src )
{
    std::function<bool( const_dialogue const & )> cond;
    talk_effect_t then_effect;
    talk_effect_t else_effect;
    read_condition( jo, std::string( member ), cond, false );
    then_effect.load_effect( jo, "then", src );
    if( jo.has_member( "else" ) || jo.has_array( "else" ) ) {
        else_effect.load_effect( jo, "else", src );
    }

    return [cond, then_effect, else_effect]( dialogue & d ) {
        if( cond( d ) ) {
            then_effect.apply( d );
        } else {
            else_effect.apply( d );
        }
    };
}

talk_effect_fun_t::func f_switch( const JsonObject &jo, std::string_view member,
                                  std::string_view src )
{
    dbl_or_var eoc_switch = get_dbl_or_var( jo, member );
    std::vector<std::pair<dbl_or_var, talk_effect_t>> case_pairs;
    for( const JsonValue jv : jo.get_array( "cases" ) ) {
        JsonObject array_case = jv.get_object();
        talk_effect_t case_effect;
        case_effect.load_effect( array_case, "effect", src );
        case_pairs.emplace_back( get_dbl_or_var( array_case, "case" ), case_effect );
    }
    return [eoc_switch, case_pairs]( dialogue & d ) {
        const double switch_int = eoc_switch.evaluate( d );
        talk_effect_t case_effect;
        for( const std::pair<dbl_or_var, talk_effect_t> &case_pair :
             case_pairs ) {
            if( switch_int >= case_pair.first.evaluate( d ) ) {
                case_effect = case_pair.second;
            }
        }
        case_effect.apply( d );
    };
}

talk_effect_fun_t::func f_foreach( const JsonObject &jo, std::string_view member,
                                   std::string_view src )
{
    std::string type = jo.get_string( member.data() );
    var_info itr = read_var_info( jo.get_object( "var" ) );
    talk_effect_t effect;
    effect.load_effect( jo, "effect", src );
    std::string target;
    std::vector<str_or_var> array;
    if( jo.has_string( "target" ) ) {
        target = jo.get_string( "target" );
    } else if( jo.has_array( "target" ) ) {
        for( const JsonValue &jv : jo.get_array( "target" ) ) {
            array.emplace_back( get_str_or_var( jv, "target" ) );
        }
    }
    return [type, itr, effect, target, array]( dialogue & d ) {
        std::vector<std::string> list;

        if( type == "ids" ) {
            if( target == "bodypart" ) {
                for( const body_part_type &bp : body_part_type::get_all() ) {
                    list.push_back( bp.id.str() );
                }
            } else if( target == "flag" ) {
                for( const json_flag &f : json_flag::get_all() ) {
                    list.push_back( f.id.str() );
                }
            } else if( target == "trait" ) {
                for( const mutation_branch &m : mutation_branch::get_all() ) {
                    list.push_back( m.id.str() );
                }
            } else if( target == "vitamin" ) {
                for( const std::pair<const vitamin_id, vitamin> &v : vitamin::all() ) {
                    list.push_back( v.first.str() );
                }
            }
        } else if( type == "item_group" ) {
            item_group_id ig( target );
            for( const itype *type : item_group::every_possible_item_from( ig ) ) {
                list.push_back( type->get_id().str() );
            }
        } else if( type == "monstergroup" ) {
            mongroup_id mg( target );
            for( const auto &m : MonsterGroupManager::GetMonstersFromGroup( mg, true ) ) {
                list.push_back( m.str() );
            }
        } else if( type == "array" ) {
            for( const str_or_var &v : array ) {
                list.emplace_back( v.evaluate( d ) );
            }
        }

        for( std::string_view str : list ) {
            write_var_value( itr.type, itr.name, &d, str.data() );
            effect.apply( d );
        }
    };
}

talk_effect_fun_t::func f_roll_remainder( const JsonObject &jo,
        std::string_view member, std::string_view src, bool is_npc )
{
    std::vector<str_or_var> list;
    for( JsonValue jv : jo.get_array( member ) ) {
        list.emplace_back( get_str_or_var( jv, member ) );
    }
    str_or_var type = get_str_or_var( jo.get_member( "type" ), "type", true );
    translation_or_var message;
    if( jo.has_member( "message" ) ) {
        message = get_translation_or_var( jo.get_member( "message" ), "message", true );
    } else {
        message.str_val = translation();
    }
    std::vector<effect_on_condition_id> true_eocs = load_eoc_vector( jo, "true_eocs", src );
    std::vector<effect_on_condition_id> false_eocs = load_eoc_vector( jo, "false_eocs", src );

    return [list, type, is_npc, true_eocs, false_eocs, message]( dialogue const & d ) {
        std::vector<std::string> not_had;
        for( const str_or_var &cur_string : list ) {
            if( type.evaluate( d ) == "bionic" ) {
                if( !d.actor( is_npc )->has_bionic( bionic_id( cur_string.evaluate( d ) ) ) ) {
                    not_had.push_back( cur_string.evaluate( d ) );
                }
            } else if( type.evaluate( d ) == "mutation" ) {
                if( !d.actor( is_npc )->has_trait( trait_id( cur_string.evaluate( d ) ) ) ) {
                    not_had.push_back( cur_string.evaluate( d ) );
                }
            } else if( type.evaluate( d ) == "spell" ) {
                if( d.actor( is_npc )->get_spell_level( spell_id( cur_string.evaluate( d ) ) ) == - 1 ) {
                    not_had.push_back( cur_string.evaluate( d ) );
                }
            } else if( type.evaluate( d ) == "recipe" ) {
                if( !d.actor( is_npc )->has_recipe( recipe_id( cur_string.evaluate( d ) ) ) ) {
                    not_had.push_back( cur_string.evaluate( d ) );
                }
            } else {
                debugmsg( "Invalid roll remainder type.  %s", d.get_callstack() );
            }
        }
        if( !not_had.empty() ) {
            int index = rng( 0, not_had.size() - 1 );
            std::string cur_choice = not_had[index];
            std::string name;
            if( type.evaluate( d ) == "bionic" ) {
                bionic_id bionic( cur_choice );
                d.actor( is_npc )->add_bionic( bionic );
                name = bionic->name.translated();
            } else if( type.evaluate( d ) == "mutation" ) {
                trait_id trait( cur_choice );
                d.actor( is_npc )->set_mutation( trait );
                name = trait->name();
            } else if( type.evaluate( d ) == "spell" ) {
                spell_id spell( cur_choice );
                d.actor( is_npc )->set_spell_level( spell, 1 );
                name = spell->name.translated();
            } else if( type.evaluate( d ) == "recipe" ) {
                recipe_id recipe( cur_choice );
                d.actor( is_npc )->learn_recipe( recipe );
                name = recipe->result_name();
            } else {
                debugmsg( "Invalid roll remainder type.  %s", d.get_callstack() );
            }
            std::string cur_message = message.evaluate( d );
            if( !cur_message.empty() ) {
                Character *target = d.actor( is_npc )->get_character();
                if( target ) {
                    target->add_msg_if_player( cur_message, name );
                }
            }
            run_eoc_vector( true_eocs, d );
        } else {
            run_eoc_vector( false_eocs, d );
        }
    };
}

talk_effect_fun_t::func f_set_fac_relation( const JsonObject &jo, std::string_view member,
        std::string_view, bool is_npc )
{
    str_or_var new_relation = get_str_or_var( jo.get_member( member ), member, true );
    const bool should_set_value = jo.get_bool( "set_value_to", true );
    return [is_npc, new_relation, should_set_value]( dialogue & d ) {
        std::string relationship_rule = new_relation.evaluate( d );
        auto rel_map_iter = npc_factions::relation_strs.find( relationship_rule );
        if( rel_map_iter == npc_factions::relation_strs.end() ) {
            debugmsg( "Invalid relationship string: %s", relationship_rule );
            return;
        } else {
            d.actor( is_npc )->set_fac_relation( d.actor( !is_npc )->get_character(),
                                                 rel_map_iter->second, should_set_value );
        }
    };
}

talk_effect_fun_t::func f_add_morale( const JsonObject &jo, std::string_view member,
                                      std::string_view, bool is_npc )
{
    str_or_var new_type = get_str_or_var( jo.get_member( member ), member, true );
    dbl_or_var dov_bonus = get_dbl_or_var( jo, "bonus" );
    dbl_or_var dov_max_bonus = get_dbl_or_var( jo, "max_bonus" );
    duration_or_var dov_duration = get_duration_or_var( jo, "duration", false, 1_hours );
    duration_or_var dov_decay_start = get_duration_or_var( jo, "decay_start", false, 30_minutes );
    const bool capped = jo.get_bool( "capped", false );
    return [is_npc, new_type, dov_bonus, dov_max_bonus, dov_duration, dov_decay_start,
            capped]( dialogue & d ) {
        d.actor( is_npc )->add_morale( morale_type( new_type.evaluate( d ) ),
                                       dov_bonus.evaluate( d ),
                                       dov_max_bonus.evaluate( d ),
                                       dov_duration.evaluate( d ),
                                       dov_decay_start.evaluate( d ),
                                       capped );
    };
}

talk_effect_fun_t::func f_lose_morale( const JsonObject &jo, std::string_view member,
                                       std::string_view, bool is_npc )
{
    str_or_var old_morale = get_str_or_var( jo.get_member( member ), member, true );
    return [is_npc, old_morale]( dialogue const & d ) {
        d.actor( is_npc )->remove_morale( morale_type( old_morale.evaluate( d ) ) );
    };
}

talk_effect_fun_t::func f_add_faction_trust( const JsonObject &jo, std::string_view member,
        std::string_view )
{
    dbl_or_var dov = get_dbl_or_var( jo, member );
    return [dov]( dialogue & d ) {
        d.actor( true )->get_faction()->trusts_u += dov.evaluate( d );
    };
}

talk_effect_fun_t::func f_custom_light_level( const JsonObject &jo,
        std::string_view member, std::string_view )
{
    dbl_or_var dov = get_dbl_or_var( jo, member, true );
    duration_or_var dov_length = get_duration_or_var( jo, "length", false, 0_seconds );
    str_or_var key;
    if( jo.has_member( "key" ) ) {
        key = get_str_or_var( jo.get_member( "key" ), "key", false, "" );
    } else {
        key.str_val = "";
    }
    return [dov_length, dov, key]( dialogue & d ) {
        get_timed_events().add( timed_event_type::CUSTOM_LIGHT_LEVEL,
                                calendar::turn + dov_length.evaluate( d ) +
                                1_seconds/*We add a second here because this will get ticked on the turn its applied before it has an effect*/,
                                -1, dov.evaluate( d ), key.evaluate( d ) );
    };
}

talk_effect_fun_t::func f_knockback( const JsonObject &jo, std::string_view member,
                                     std::string_view, bool is_npc )
{
    dbl_or_var force = get_dbl_or_var( jo, member, false, 0 );
    dbl_or_var stun = get_dbl_or_var( jo, "stun", false, 0 );
    dbl_or_var dam_mult = get_dbl_or_var( jo, "dam_mult", false, 0 );

    std::optional<var_info> target_var;
    if( jo.has_member( "target_var" ) ) {
        target_var = read_var_info( jo.get_object( "target_var" ) );
    }

    std::optional<var_info> direction_var;
    if( jo.has_member( "direction_var" ) ) {
        direction_var = read_var_info( jo.get_object( "direction_var" ) );
    }

    return [force, stun, dam_mult, target_var, direction_var, is_npc]( dialogue & d ) {
        map &here = get_map();
        tripoint_bub_ms target_pos;
        if( target_var.has_value() ) {
            tripoint_abs_ms abs_ms = read_var_value( *target_var, d ).tripoint();
            target_pos = here.get_bub( abs_ms );
        } else {
            target_pos = d.actor( is_npc )->pos_bub( here );
        }

        tripoint_bub_ms direction_pos;
        if( direction_var.has_value() ) {
            tripoint_abs_ms abs_ms = read_var_value( *direction_var, d ).tripoint();
            direction_pos = here.get_bub( abs_ms );
        } else {
            direction_pos = d.actor( is_npc )->pos_bub( here );

            point d2( rng( -1, 1 ), rng( -1, 1 ) );

            // if we pass target_pos == direction_pos to knockback()->continue_line() the game gonna crash
            while( d2 == point::zero ) {
                d2 = point( rng( -1, 1 ), rng( -1, 1 ) );
            }

            direction_pos += d2;
        }

        g->knockback( direction_pos, target_pos, force.evaluate( d ), stun.evaluate( d ),
                      dam_mult.evaluate( d ) );
    };
}

talk_effect_fun_t::func f_give_equipment( const JsonObject &jo, std::string_view member,
        std::string_view )
{
    JsonObject jobj = jo.get_object( member );
    int allowance = 0;
    std::vector<trial_mod> debt_modifiers;
    if( jobj.has_int( "allowance" ) ) {
        allowance = jobj.get_int( "allowance" );
    } else if( jobj.has_array( "allowance" ) ) {
        for( JsonArray jmod : jobj.get_array( "allowance" ) ) {
            trial_mod this_modifier;
            this_modifier.first = jmod.next_string();
            this_modifier.second = jmod.next_int();
            debt_modifiers.push_back( this_modifier );
        }
    }
    return [debt_modifiers, allowance]( dialogue const & d ) {
        int debt = allowance;
        for( const trial_mod &this_mod : debt_modifiers ) {
            if( this_mod.first == "TOTAL" ) {
                debt *= this_mod.second;
            } else {
                debt += parse_mod( d, this_mod.first, this_mod.second );
            }
        }
        if( npc *p = d.actor( true )->get_npc() ) {
            talk_function::give_equipment_allowance( *p, debt );
        }
    };
}


talk_effect_fun_t::func f_deal_damage( const JsonObject &jo, std::string_view member,
                                       std::string_view, bool is_npc )
{
    str_or_var dmg_type = get_str_or_var( jo.get_member( member ), member );
    dbl_or_var dmg_amount = get_dbl_or_var( jo, "amount", false, 0 );
    dbl_or_var arpen = get_dbl_or_var( jo, "arpen", false, 0 );
    dbl_or_var arpen_mult = get_dbl_or_var( jo, "arpen_mult", false, 1 );
    dbl_or_var dmg_mult = get_dbl_or_var( jo, "dmg_mult", false, 1 );

    dbl_or_var dbl_min_hit = get_dbl_or_var( jo, "min_hit", false, -1 );
    dbl_or_var dbl_max_hit = get_dbl_or_var( jo, "max_hit", false, -1 );
    dbl_or_var dbl_hit_roll = get_dbl_or_var( jo, "hit_roll", false, 0 );
    bool can_attack_high = jo.get_bool( "can_attack_high", true );


    str_or_var bodypart;

    if( jo.has_member( "bodypart" ) ) {
        bodypart = get_str_or_var( jo.get_member( "bodypart" ), "bodypart", false, "RANDOM" );
    } else {
        bodypart.str_val = "RANDOM";
    }

    return [is_npc, dmg_type, dmg_amount, bodypart, arpen, arpen_mult, dmg_mult, dbl_min_hit,
            dbl_max_hit, dbl_hit_roll, can_attack_high]( dialogue & d ) {

        damage_instance dmg_inst;
        damage_type_id damage_type = damage_type_id( dmg_type.evaluate( d ) );
        std::string const bp_str = bodypart.evaluate( d );
        bodypart_id bp;

        if( d.actor( is_npc )->get_character() ) {
            if( bp_str == "RANDOM" ) {
                Character &guy = *d.actor( is_npc )->get_character();
                int min_hit = dbl_min_hit.evaluate( d );
                int max_hit = dbl_max_hit.evaluate( d );
                int hit_roll = dbl_hit_roll.evaluate( d );

                if( max_hit == -1 ) {
                    max_hit = guy.get_max_hitsize_bodypart()->hit_size;
                }
                bp = guy.select_body_part( min_hit, max_hit, can_attack_high, hit_roll );
            } else {
                bp = bodypart_id( bp_str );
            }
        }

        if( d.actor( is_npc )->get_monster() ) {
            bp = bodypart_str_id::NULL_ID();
        }

        dmg_inst.add_damage( damage_type, dmg_amount.evaluate( d ), arpen.evaluate( d ),
                             arpen_mult.evaluate( d ), dmg_mult.evaluate( d ), 1, 1 );
        d.actor( is_npc )->deal_damage( d.actor( is_npc )->get_creature(), bp, dmg_inst );
    };
}

talk_effect_fun_t::func f_spawn_monster( const JsonObject &jo, std::string_view member,
        std::string_view src, bool is_npc )
{
    const map &here = get_map();

    bool group = jo.get_bool( "group", false );
    bool single_target = jo.get_bool( "single_target", false );
    str_or_var monster_id = get_str_or_var( jo.get_member( member ), member );
    dbl_or_var dov_target_range = get_dbl_or_var( jo, "target_range", false, 0 );
    dbl_or_var dov_hallucination_count = get_dbl_or_var( jo, "hallucination_count", false, 0 );
    dbl_or_var dov_real_count = get_dbl_or_var( jo, "real_count", false, 0 );
    dbl_or_var dov_min_radius = get_dbl_or_var( jo, "min_radius", false, 1 );
    dbl_or_var dov_max_radius = get_dbl_or_var( jo, "max_radius", false, 10 );
    bool summoner_is_alpha = jo.get_bool( "summoner_is_alpha", false );
    bool summoner_is_beta = jo.get_bool( "summoner_is_beta", false );
    std::unordered_map<std::string, str_or_var> set_mon_var;
    if( jo.has_object( "mon_variables" ) ) {
        const JsonObject &variables = jo.get_object( "mon_variables" );
        for( const JsonMember &jv : variables ) {
            set_mon_var[jv.name()] = get_str_or_var( jv, jv.name(), true );
        }
    }

    const bool outdoor_only = jo.get_bool( "outdoor_only", false );
    const bool indoor_only = jo.get_bool( "indoor_only", false );
    if( indoor_only && outdoor_only ) {
        jo.throw_error( "Cannot be outdoor_only and indoor_only at the same time." );
    }
    const bool open_air_allowed = jo.get_bool( "open_air_allowed", false );
    const bool temporary_drop_items = jo.get_bool( "temporary_drop_items", false );
    const bool friendly = jo.get_bool( "friendly", false );

    duration_or_var dov_lifespan = get_duration_or_var( jo, "lifespan", false, 0_seconds );
    std::optional<var_info> target_var;
    if( jo.has_member( "target_var" ) ) {
        target_var = read_var_info( jo.get_object( "target_var" ) );
    }
    translation spawn_message;
    jo.read( "spawn_message", spawn_message );
    translation spawn_message_plural;
    jo.read( "spawn_message_plural", spawn_message_plural );
    std::vector<effect_on_condition_id> true_eocs = load_eoc_vector( jo, "true_eocs", src );
    std::vector<effect_on_condition_id> false_eocs = load_eoc_vector( jo, "false_eocs", src );
    return [monster_id, dov_target_range, dov_hallucination_count, dov_real_count, dov_min_radius,
                        dov_max_radius, outdoor_only, indoor_only, group, single_target, dov_lifespan, target_var,
                        spawn_message, spawn_message_plural, true_eocs, false_eocs, open_air_allowed, temporary_drop_items,
                friendly, summoner_is_alpha, summoner_is_beta, set_mon_var, is_npc, &here]( dialogue & d ) {
        monster target_monster;
        std::vector<Creature *> target_monsters;
        mongroup_id target_mongroup;
        bool use_target_monster = single_target;

        if( group ) {
            if( monster_id.evaluate( d ).empty() ) {
                debugmsg( "Cannot use group without a valid monstergroup.  %s", d.get_callstack() );
            }
            if( single_target ) {
                target_monster = monster( MonsterGroupManager::GetRandomMonsterFromGroup( mongroup_id(
                                              monster_id.evaluate( d ) ) ) );
            } else {
                target_mongroup = mongroup_id( monster_id.evaluate( d ) );
            }
        } else if( monster_id.evaluate( d ).empty() ) {
            int target_range = dov_target_range.evaluate( d );
            if( single_target ) {
                // Find a hostile creature in range to be used to create a hallucination or a copy of
                Creature *copy = g->get_creature_if( [target_range]( const Creature & critter ) -> bool {
                    bool not_self = get_player_character().pos_bub() != critter.pos_bub();
                    bool in_range = std::round( rl_dist_exact( get_player_character().pos_bub(), critter.pos_bub() ) ) <= target_range;
                    bool valid_target = get_player_character().attitude_to( critter ) == Creature::Attitude::HOSTILE;
                    return not_self && in_range && valid_target;
                } );
                if( copy == nullptr ) {
                    run_eoc_vector( false_eocs, d );
                    return;
                }
                target_monster = *copy->as_monster();
            } else {
                // Find all hostile creatures in range to be used to create hallucinations or copies of
                std::vector<Creature *> monsters_in_range = g->get_creatures_if( [target_range](
                const Creature & critter ) -> bool {
                    bool not_self = get_player_character().pos_bub() != critter.pos_bub();
                    bool in_range = std::round( rl_dist_exact( get_player_character().pos_bub(), critter.pos_bub() ) ) <= target_range;
                    bool valid_target = get_player_character().attitude_to( critter ) == Creature::Attitude::HOSTILE;
                    return not_self && in_range && valid_target;
                } );
                int valid_monsters = monsters_in_range.size();
                if( valid_monsters == 0 ) {
                    run_eoc_vector( false_eocs, d );
                    return;
                } else if( valid_monsters == 1 ) {
                    Creature *copy = monsters_in_range[0];
                    target_monster = *copy->as_monster();
                    use_target_monster = true;
                } else {
                    target_monsters = monsters_in_range;
                }
            }
        } else {
            if( single_target ) {
                debugmsg( "single_target doesn't need to be defined for a singlular monster_id.  %s",
                          d.get_callstack() );
            }
            target_monster = monster( mtype_id( monster_id.evaluate( d ) ) );
            use_target_monster = true;
        }
        int min_radius = dov_min_radius.evaluate( d );
        int max_radius = dov_max_radius.evaluate( d );
        int real_count = dov_real_count.evaluate( d );
        int hallucination_count = dov_hallucination_count.evaluate( d );
        std::optional<time_duration> lifespan;
        tripoint_bub_ms target_pos = d.actor( is_npc )->pos_bub( here );
        if( target_var.has_value() ) {
            target_pos = here.get_bub( read_var_value( *target_var, d ).tripoint() );
        }
        int visible_spawns = 0;
        int spawns = 0;
        for( int i = 0; i < hallucination_count; i++ ) {
            tripoint_bub_ms spawn_point;
            if( !use_target_monster ) {
                if( group ) {
                    target_monster = monster( MonsterGroupManager::GetRandomMonsterFromGroup( target_mongroup ) );
                } else {
                    Creature *copy = target_monsters[ rng( 0, target_monsters.size() - 1 ) ];
                    target_monster = *copy->as_monster();
                }
            }
            if( g->find_nearby_spawn_point( target_pos, target_monster.type->id, min_radius,
                                            max_radius, spawn_point, outdoor_only, indoor_only, open_air_allowed ) ) {
                lifespan = dov_lifespan.evaluate( d );
                if( lifespan.value() == 0_seconds ) {
                    lifespan.reset();
                }
                if( g->spawn_hallucination( spawn_point, target_monster.type->id, lifespan ) ) {
                    Creature *critter = get_creature_tracker().creature_at( spawn_point );
                    if( critter ) {
                        if( friendly ) {
                            critter->as_monster()->friendly = -1;
                        }
                        spawns++;
                        if( get_avatar().sees( here, *critter ) ) {
                            visible_spawns++;
                        }
                    }
                }
            }
        }
        for( int i = 0; i < real_count; i++ ) {
            tripoint_bub_ms spawn_point;
            if( !use_target_monster ) {
                if( group ) {
                    target_monster = monster( MonsterGroupManager::GetRandomMonsterFromGroup( target_mongroup ) );
                } else {
                    Creature *copy = target_monsters[ rng( 0, target_monsters.size() - 1 ) ];
                    target_monster = *copy->as_monster();
                }
            }
            if( g->find_nearby_spawn_point( target_pos, target_monster.type->id, min_radius,
                                            max_radius, spawn_point, outdoor_only, indoor_only, open_air_allowed ) ) {
                monster *spawned = g->place_critter_at( target_monster.type->id, spawn_point );
                if( spawned ) {
                    if( friendly ) {
                        spawned->friendly = -1;
                    }
                    if( summoner_is_alpha && d.actor( false )->get_creature() != nullptr ) {
                        spawned->set_summoner( d.actor( false )->get_creature() );
                    }
                    if( summoner_is_beta && d.actor( true )->get_creature() != nullptr ) {
                        spawned->set_summoner( d.actor( true )->get_creature() );
                    }
                    if( !set_mon_var.empty() ) {
                        for( const auto &val : set_mon_var ) {
                            // FIXME: typed variables
                            spawned->set_value( val.first, diag_value::legacy_value{ val.second.evaluate( d, true ) } );
                        }
                    }
                    spawns++;
                    if( get_avatar().sees( here, *spawned ) ) {
                        visible_spawns++;
                    }
                    lifespan = dov_lifespan.evaluate( d );
                    if( lifespan.value() > 0_seconds ) {
                        spawned->set_summon_time( lifespan.value() );
                        // Temporary monsters shouldn't drop items unless told to
                        spawned->no_extra_death_drops = !temporary_drop_items;
                        spawned->no_corpse_quiet = !temporary_drop_items;
                    }
                }
            }
        }
        if( visible_spawns > 1 && !spawn_message_plural.empty() ) {
            get_avatar().add_msg_if_player( m_bad, spawn_message_plural );
        } else if( visible_spawns > 0 && !spawn_message.empty() ) {
            get_avatar().add_msg_if_player( m_bad, spawn_message );
        }
        if( spawns > 0 ) {
            run_eoc_vector( true_eocs, d );
        } else {
            run_eoc_vector( false_eocs, d );
        }
    };
}

talk_effect_fun_t::func f_spawn_npc( const JsonObject &jo, std::string_view member,
                                     std::string_view src, bool is_npc )
{
    const map &here = get_map();

    str_or_var sov_npc_class = get_str_or_var( jo.get_member( member ), member );
    str_or_var unique_id;
    if( jo.has_member( "unique_id" ) ) {
        unique_id = get_str_or_var( jo.get_member( "unique_id" ), "unique_id" );
    } else {
        unique_id.str_val = "";
    }
    std::vector<str_or_var> traits;
    for( JsonValue jv : jo.get_array( "traits" ) ) {
        str_or_var entry = get_str_or_var( jv, "traits" );
        traits.emplace_back( entry );
    }

    dbl_or_var dov_hallucination_count = get_dbl_or_var( jo, "hallucination_count", false, 0 );
    dbl_or_var dov_real_count = get_dbl_or_var( jo, "real_count", false, 0 );
    dbl_or_var dov_min_radius = get_dbl_or_var( jo, "min_radius", false, 1 );
    dbl_or_var dov_max_radius = get_dbl_or_var( jo, "max_radius", false, 10 );

    const bool open_air_allowed = jo.get_bool( "open_air_allowed", false );
    const bool outdoor_only = jo.get_bool( "outdoor_only", false );
    const bool indoor_only = jo.get_bool( "indoor_only", false );
    if( indoor_only && outdoor_only ) {
        jo.throw_error( "Cannot be outdoor_only and indoor_only at the same time." );
    }

    duration_or_var dov_lifespan = get_duration_or_var( jo, "lifespan", false, 0_seconds );
    std::optional<var_info> target_var;
    if( jo.has_member( "target_var" ) ) {
        target_var = read_var_info( jo.get_object( "target_var" ) );
    }
    translation spawn_message;
    jo.read( "spawn_message", spawn_message );
    translation spawn_message_plural;
    jo.read( "spawn_message_plural", spawn_message_plural );
    std::vector<effect_on_condition_id> true_eocs = load_eoc_vector( jo, "true_eocs", src );
    std::vector<effect_on_condition_id> false_eocs = load_eoc_vector( jo, "false_eocs", src );
    return [sov_npc_class, unique_id, traits, dov_hallucination_count, dov_real_count,
                           dov_min_radius,
                           dov_max_radius, outdoor_only, indoor_only, dov_lifespan, target_var, spawn_message,
                   spawn_message_plural, true_eocs, false_eocs, open_air_allowed, is_npc, &here]( dialogue & d ) {
        int min_radius = dov_min_radius.evaluate( d );
        int max_radius = dov_max_radius.evaluate( d );
        int real_count = dov_real_count.evaluate( d );
        int hallucination_count = dov_hallucination_count.evaluate( d );
        string_id<npc_template> cur_npc_class( sov_npc_class.evaluate( d ) );
        std::string cur_unique_id = unique_id.evaluate( d );
        std::vector<trait_id> cur_traits;
        cur_traits.reserve( traits.size() ); // Reserve space for all elements in traits
        for( const str_or_var &cur_trait : traits ) {
            cur_traits.emplace_back( cur_trait.evaluate( d ) );
        }
        std::optional<time_duration> lifespan;
        tripoint_bub_ms target_pos = d.actor( is_npc )->pos_bub( here );
        if( target_var.has_value() ) {
            target_pos = here.get_bub( read_var_value( *target_var, d ).tripoint() );
        }
        int visible_spawns = 0;
        int spawns = 0;
        for( int i = 0; i < real_count; i++ ) {
            tripoint_bub_ms spawn_point;
            if( g->find_nearby_spawn_point( target_pos, min_radius,
                                            max_radius, spawn_point, outdoor_only, indoor_only, open_air_allowed ) ) {
                lifespan = dov_lifespan.evaluate( d );
                if( lifespan.value() == 0_seconds ) {
                    lifespan.reset();
                }
                if( g->spawn_npc( spawn_point, cur_npc_class, cur_unique_id, cur_traits, lifespan ) ) {
                    Creature *guy = get_creature_tracker().creature_at( spawn_point );
                    if( guy ) {
                        spawns++;
                        if( get_avatar().sees( here, *guy ) ) {
                            visible_spawns++;
                        }
                    }
                }
            }
        }
        cur_traits.emplace_back( trait_HALLUCINATION );
        for( int i = 0; i < hallucination_count; i++ ) {
            tripoint_bub_ms spawn_point;
            if( g->find_nearby_spawn_point( target_pos, min_radius,
                                            max_radius, spawn_point, outdoor_only, indoor_only, open_air_allowed ) ) {
                lifespan = dov_lifespan.evaluate( d );
                if( lifespan.value() == 0_seconds ) {
                    lifespan.reset();
                }
                std::string empty;
                if( g->spawn_npc( spawn_point, cur_npc_class, empty, cur_traits, lifespan ) ) {
                    Creature *guy = get_creature_tracker().creature_at( spawn_point );
                    if( guy ) {
                        spawns++;
                        if( get_avatar().sees( here, *guy ) ) {
                            visible_spawns++;
                        }
                    }
                }
            }
        }
        if( visible_spawns > 1 && !spawn_message_plural.empty() ) {
            get_avatar().add_msg_if_player( m_bad, spawn_message_plural );
        } else if( visible_spawns > 0 && !spawn_message.empty() ) {
            get_avatar().add_msg_if_player( m_bad, spawn_message );
        }
        if( spawns > 0 ) {
            run_eoc_vector( true_eocs, d );
        } else {
            run_eoc_vector( false_eocs, d );
        }
    };
}

talk_effect_fun_t::func f_field( const JsonObject &jo, std::string_view member,
                                 std::string_view, bool is_npc )
{
    str_or_var new_field = get_str_or_var( jo.get_member( member ), member, true );
    dbl_or_var dov_intensity = get_dbl_or_var( jo, "intensity", false, 1 );
    duration_or_var dov_age = get_duration_or_var( jo, "age", false, 1_turns );
    dbl_or_var dov_radius = get_dbl_or_var( jo, "radius", false, 10000000 );

    const bool outdoor_only = jo.get_bool( "outdoor_only", false );
    const bool indoor_only = jo.get_bool( "indoor_only", false );
    const bool hit_player = jo.get_bool( "hit_player", true );

    std::optional<var_info> target_var;
    if( jo.has_member( "target_var" ) ) {
        target_var = read_var_info( jo.get_object( "target_var" ) );
    }
    return [new_field, dov_intensity, dov_age, dov_radius, outdoor_only,
               hit_player, target_var, is_npc, indoor_only]( dialogue & d ) {
        map &here = get_map();

        int radius = dov_radius.evaluate( d );
        int intensity = dov_intensity.evaluate( d );

        tripoint_abs_ms target_pos = d.actor( is_npc )->pos_abs();
        if( target_var.has_value() ) {
            target_pos = read_var_value( *target_var, d ).tripoint();
        }
        for( const tripoint_bub_ms &dest : here.points_in_radius( here.get_bub( target_pos ),
                radius ) ) {
            if( ( !outdoor_only || here.is_outside( dest ) ) && ( !indoor_only ||
                    !here.is_outside( dest ) ) ) {
                here.add_field( dest, field_type_str_id( new_field.evaluate( d ) ), intensity,
                                dov_age.evaluate( d ),
                                hit_player );
            }
        }
    };
}

talk_effect_fun_t::func f_emit( const JsonObject &jo, std::string_view member,
                                std::string_view, bool is_npc )
{
    str_or_var emit = get_str_or_var( jo.get_member( member ), member, true );
    dbl_or_var chance_mult = get_dbl_or_var( jo, "chance_mult", false, 1 );

    std::optional<var_info> target_var;
    if( jo.has_member( "target_var" ) ) {
        target_var = read_var_info( jo.get_object( "target_var" ) );
    }
    return [emit, target_var, chance_mult, is_npc ]( dialogue & d ) {
        map &here = get_map();

        tripoint_abs_ms target_pos = d.actor( is_npc )->pos_abs();
        if( target_var.has_value() ) {
            target_pos = read_var_value( *target_var, d ).tripoint();
        }
        tripoint_bub_ms target = here.get_bub( target_pos );
        here.emit_field( target, emit_id( emit.evaluate( d ) ), chance_mult.evaluate( d ) );
    };
}

talk_effect_fun_t::func f_reveal_map( const JsonObject &jo, std::string_view member,
                                      std::string_view )
{
    var_info target_var = read_var_info( jo.get_object( member ) );
    dbl_or_var radius = get_dbl_or_var( jo, "radius", false, 0 );

    return [ radius, target_var ]( dialogue & d ) {
        tripoint_abs_omt omt = project_to<coords::omt>( read_var_value( target_var, d ).tripoint() );
        overmap_buffer.reveal( omt, radius.evaluate( d ) );
    };
}

talk_effect_fun_t::func f_reveal_route( const JsonObject &jo, std::string_view member,
                                        std::string_view )
{
    var_info from = read_var_info( jo.get_object( member ) );
    var_info to = read_var_info( jo.get_object( "target_var" ) );
    dbl_or_var radius = get_dbl_or_var( jo, "radius", false, 0 );
    bool road_only = jo.get_bool( "road_only", false );

    return [ radius, from, to, road_only ]( dialogue & d ) {
        tripoint_abs_ms from_pos = read_var_value( from, d ).tripoint();
        tripoint_abs_omt omt_from = project_to<coords::omt>( from_pos );
        tripoint_abs_ms to_pos = read_var_value( to, d ).tripoint();
        tripoint_abs_omt omt_to = project_to<coords::omt>( to_pos );

        overmap_buffer.reveal_route( omt_from, omt_to, radius.evaluate( d ), road_only );
    };
}

talk_effect_fun_t::func f_closest_city( const JsonObject &jo, std::string_view member,
                                        std::string_view )
{
    var_info var = read_var_info( jo.get_object( member ) );
    var_type type = var.type;
    std::string var_name = var.name;
    bool known = jo.get_bool( "known", true );

    return [var, type, var_name, known]( dialogue & d ) {
        tripoint_abs_ms loc_ms = read_var_value( var, d ).tripoint();
        tripoint_abs_sm loc_sm = project_to<coords::sm>( loc_ms );

        if( known ) {
            city_reference city = overmap_buffer.closest_known_city( loc_sm );
            if( city ) {
                tripoint_abs_omt city_center_omt = project_to<coords::omt>( city.abs_sm_pos );
                write_var_value( type, var_name, &d, tripoint_abs_ms{ city_center_omt.raw() } );
                write_var_value( var_type::context, "city_name", &d, city.city->name );
                write_var_value( var_type::context, "city_size", &d, city.city->size );
            } else {
                return;
            }
        } else {
            city_reference city = overmap_buffer.closest_city( loc_sm );
            if( city ) {
                tripoint_abs_omt city_center_omt = project_to<coords::omt>( city.abs_sm_pos );
                write_var_value( type, var_name, &d, tripoint_abs_ms{ city_center_omt.raw() } );
                write_var_value( var_type::context, "city_name", &d, city.city->name );
                write_var_value( var_type::context, "city_size", &d, city.city->size );
            }
        }
    };
}

talk_effect_fun_t::func f_teleport( const JsonObject &jo, std::string_view member,
                                    std::string_view, bool is_npc )
{
    var_info target_var = read_var_info( jo.get_object( member ) );
    translation_or_var fail_message;
    if( jo.has_member( "fail_message" ) ) {
        fail_message = get_translation_or_var( jo.get_member( "fail_message" ), "fail_message",
                                               false, translation() );
    } else {
        fail_message.str_val = translation();
    }
    translation_or_var success_message;
    if( jo.has_member( "success_message" ) ) {
        success_message = get_translation_or_var( jo.get_member( "success_message" ), "success_message",
                          false, translation() );
    } else {
        success_message.str_val = translation();
    }
    bool force = jo.get_bool( "force", false );
    bool force_safe = jo.get_bool( "force_safe", false );
    return [is_npc, target_var, fail_message, success_message, force,
            force_safe]( dialogue const & d ) {
        tripoint_abs_ms target_pos = read_var_value( target_var, d ).tripoint();
        Creature *teleporter = d.actor( is_npc )->get_creature();
        if( teleporter ) {
            if( teleport::teleport_to_point( *teleporter, get_map().get_bub( target_pos ), true, false,
                                             false, force, force_safe ) ) {
                teleporter->add_msg_if_player( success_message.evaluate( d ) );
            } else {
                teleporter->add_msg_if_player( fail_message.evaluate( d ) );
            }
        }
        item_location *it = d.actor( is_npc )->get_item();
        if( it && it->get_item() ) {
            map_add_item( *it->get_item(), target_pos );
            add_msg( success_message.evaluate( d ) );
            it->remove_item();
        }
        vehicle *veh = d.actor( is_npc )->get_vehicle();
        if( veh ) {
            if( teleport::teleport_vehicle( *veh, target_pos ) ) {
                add_msg( success_message.evaluate( d ) );
            } else {
                add_msg( fail_message.evaluate( d ) );
            }
        }
    };
}

talk_effect_fun_t::func f_wants_to_talk( bool is_npc )
{
    const map &here = get_map();

    return [is_npc, &here]( dialogue const & d ) {
        npc *p = d.actor( is_npc )->get_npc();
        if( p ) {
            if( p->get_attitude() == NPCATT_TALK ) {
                return;
            }
            if( p->sees( here, get_player_character() ) ) {
                add_msg( _( "%s wants to talk to you." ), p->get_name() );
            }
            p->set_attitude( NPCATT_TALK );
        }
    };
}

talk_effect_fun_t::func f_trigger_event( const JsonObject &jo, std::string_view member,
        std::string_view )
{
    std::string const type_str = jo.get_string( member.data() );
    JsonArray const &jargs = jo.get_array( "args" );

    event_type type = io::string_to_enum<event_type>( type_str );
    std::vector<str_or_var> args;
    args.reserve( jargs.size() );
    for( JsonValue const &jv : jargs ) {
        args.emplace_back( get_str_or_var( jv, "args" ) );
    }

    return [type, args]( dialogue & d ) {
        std::vector<std::string> args_str;
        args_str.reserve( args.size() );
        std::transform( args.cbegin(), args.cend(),
        std::back_inserter( args_str ), [&d]( str_or_var const & sov ) {
            return sov.evaluate( d );
        } );
        get_event_bus().send( cata::event::make_dyn( type, args_str ) );
    };
}

} // namespace
} // namespace talk_effect_fun

void talk_effect_t::set_effect_consequence( const talk_effect_fun_t &fun,
        dialogue_consequence con )
{
    effects.push_back( fun );
    guaranteed_consequence = std::max( guaranteed_consequence, con );
}

void talk_effect_t::set_effect_consequence( const std::function<void( npc &p )> &ptr,
        dialogue_consequence con )
{
    talk_effect_fun_t npctalk_setter( ptr );
    set_effect_consequence( npctalk_setter, con );
}

void talk_effect_t::set_effect( const talk_effect_fun_t &fun )
{
    effects.push_back( fun );
    guaranteed_consequence = std::max( guaranteed_consequence, dialogue_consequence::none );
}

void talk_effect_t::set_effect( talkfunction_ptr ptr )
{
    talk_effect_fun_t npctalk_setter( ptr );
    dialogue_consequence response;
    if( ptr == &talk_function::hostile ) {
        response = dialogue_consequence::hostile;
    } else if( ptr == &talk_function::player_weapon_drop ||
               ptr == &talk_function::player_weapon_away ||
               ptr == &talk_function::start_mugging ) {
        response = dialogue_consequence::helpless;
    } else {
        response = dialogue_consequence::none;
    }
    set_effect_consequence( npctalk_setter, response );
}

talk_topic talk_effect_t::apply( dialogue &d ) const
{
    if( d.has_beta ) {
        // Need to get a reference to the mission before effects are applied, because effects can remove the mission
        const mission *miss = d.actor( true )->selected_mission();
        for( const talk_effect_fun_t &effect : effects ) {
            effect( d );
        }
        d.actor( true )->add_opinion( opinion );
        if( miss && ( mission_opinion.trust || mission_opinion.fear ||
                      mission_opinion.value || mission_opinion.anger ) ) {
            const int m_value = d.actor( true )->cash_to_favor( miss->get_value() );
            npc_opinion op;
            op.trust = mission_opinion.trust ? m_value / mission_opinion.trust : 0;
            op.fear = mission_opinion.fear ? m_value / mission_opinion.fear : 0;
            op.value = mission_opinion.value ? m_value / mission_opinion.value : 0;
            op.anger = mission_opinion.anger ? m_value / mission_opinion.anger : 0;
            d.actor( true )->add_opinion( op );
        }
        if( d.actor( true )->turned_hostile() ) {
            d.actor( true )->make_angry();
            return talk_topic( "TALK_DONE" );
        }
    } else {
        for( const talk_effect_fun_t &effect : effects ) {
            effect( d );
        }
    }

    return next_topic;
}

void talk_effect_t::update_missions( dialogue &d )
{
    auto &ma = d.missions_assigned;
    ma.clear();
    if( d.has_beta ) {
        // Update the missions we can talk about (must only be current, non-complete ones)
        for( mission *&mission : d.actor( true )->assigned_missions() ) {
            if( mission->get_assigned_player_id() == d.actor( false )->getID() ) {
                ma.push_back( mission );
            }
        }
    }
}

talk_effect_t::talk_effect_t( const JsonObject &jo, const std::string &member_name,
                              std::string_view src )
{
    load_effect( jo, member_name, src );
    if( jo.has_object( "topic" ) ) {
        next_topic = load_inline_topic( jo.get_object( "topic" ), src );
    } else if( jo.has_string( "topic" ) ) {
        next_topic = talk_topic( jo.get_string( "topic" ) );
    }
}

static const
std::vector<sub_effect_parser>
parsers = {
    { "u_add_effect", "npc_add_effect", jarg::member, &talk_effect_fun::f_add_effect },
    { "u_lose_effect", "npc_lose_effect", jarg::member, &talk_effect_fun::f_remove_effect },
    { "u_add_var", "npc_add_var", jarg::string, &talk_effect_fun::f_add_var },
    { "u_lose_var", "npc_lose_var", jarg::string, &talk_effect_fun::f_remove_var },
    { "u_add_trait", "npc_add_trait", jarg::member, &talk_effect_fun::f_add_trait },
    { "u_lose_trait", "npc_lose_trait", jarg::member, &talk_effect_fun::f_remove_trait },
    { "u_deactivate_trait", "npc_deactivate_trait", jarg::member, &talk_effect_fun::f_deactivate_trait },
    { "u_activate_trait", "npc_activate_trait", jarg::member, &talk_effect_fun::f_activate_trait },
    { "u_mutate", "npc_mutate", jarg::member | jarg::array, &talk_effect_fun::f_mutate },
    { "u_mutate_category", "npc_mutate_category", jarg::member, &talk_effect_fun::f_mutate_category },
    { "u_mutate_towards", "npc_mutate_towards", jarg::member, &talk_effect_fun::f_mutate_towards},
    { "u_set_trait_purifiability", "npc_set_trait_purifiability", jarg::member, &talk_effect_fun::f_set_trait_purifiability},
    { "u_learn_martial_art", "npc_learn_martial_art", jarg::member, &talk_effect_fun::f_learn_martial_art },
    { "u_forget_martial_art", "npc_forget_martial_art", jarg::member, &talk_effect_fun::f_forget_martial_art },
    { "u_location_variable", "npc_location_variable", jarg::object, &talk_effect_fun::f_location_variable },
    { "u_transform_radius", "npc_transform_radius", jarg::member | jarg::array, &talk_effect_fun::f_transform_radius },
    { "u_explosion", "npc_explosion", jarg::member, &talk_effect_fun::f_explosion },
    { "u_query_tile", "npc_query_tile", jarg::member, &talk_effect_fun::f_query_tile },
    { "u_query_omt", "npc_query_omt", jarg::member, &talk_effect_fun::f_query_omt },
    { "u_choose_adjacent_highlight", "npc_choose_adjacent_highlight", jarg::member, &talk_effect_fun::f_choose_adjacent_highlight },
    { "u_set_goal", "npc_set_goal", jarg::member, &talk_effect_fun::f_npc_goal },
    { "u_set_guard_pos", "npc_set_guard_pos", jarg::member, &talk_effect_fun::f_guard_pos },
    { "u_level_spell_class", "npc_level_spell_class", jarg::member, &talk_effect_fun::f_level_spell_class },
    { "u_learn_recipe", "npc_learn_recipe", jarg::member, &talk_effect_fun::f_learn_recipe },
    { "u_forget_recipe", "npc_forget_recipe", jarg::member, &talk_effect_fun::f_forget_recipe },
    { "u_message", "npc_message", jarg::member, &talk_effect_fun::f_message },
    { "u_add_wet", "npc_add_wet", jarg::member | jarg::array, &talk_effect_fun::f_add_wet },
    { "u_assign_activity", "npc_assign_activity", jarg::member, &talk_effect_fun::f_assign_activity },
    { "u_make_sound", "npc_make_sound", jarg::member, &talk_effect_fun::f_make_sound },
    { "u_run_npc_eocs", "npc_run_npc_eocs", jarg::array, &talk_effect_fun::f_run_npc_eocs },
    { "u_run_monster_eocs", "npc_run_monster_eocs", jarg::array, &talk_effect_fun::f_run_monster_eocs },
    { "u_run_vehicle_eocs", "npc_run_vehicle_eocs", jarg::array, &talk_effect_fun::f_run_vehicle_eocs },
    { "u_run_inv_eocs", "npc_run_inv_eocs", jarg::member, &talk_effect_fun::f_run_inv_eocs },
    { "u_roll_remainder", "npc_roll_remainder", jarg::member, &talk_effect_fun::f_roll_remainder },
    { "u_set_fac_relation", "npc_set_fac_relation", jarg::member, &talk_effect_fun::f_set_fac_relation },
    { "u_add_morale", "npc_add_morale", jarg::member, &talk_effect_fun::f_add_morale },
    { "u_lose_morale", "npc_lose_morale", jarg::member, &talk_effect_fun::f_lose_morale },
    { "u_add_bionic", "npc_add_bionic", jarg::member, &talk_effect_fun::f_add_bionic },
    { "u_lose_bionic", "npc_lose_bionic", jarg::member, &talk_effect_fun::f_lose_bionic },
    { "u_attack", "npc_attack", jarg::member, &talk_effect_fun::f_attack },
    { "u_die", "npc_die", jarg::object, &talk_effect_fun::f_die_advanced},
    { "u_spawn_monster", "npc_spawn_monster", jarg::member, &talk_effect_fun::f_spawn_monster },
    { "u_spawn_npc", "npc_spawn_npc", jarg::member, &talk_effect_fun::f_spawn_npc },
    { "u_set_field", "npc_set_field", jarg::member, &talk_effect_fun::f_field },
    { "u_emit", "npc_emit", jarg::member, &talk_effect_fun::f_emit },
    { "u_teleport", "npc_teleport", jarg::object, &talk_effect_fun::f_teleport },
    { "u_set_flag", "npc_set_flag", jarg::member, &talk_effect_fun::f_set_flag },
    { "u_unset_flag", "npc_unset_flag", jarg::member, &talk_effect_fun::f_unset_flag },
    { "u_set_fault", "npc_set_fault", jarg::member, &talk_effect_fun::f_set_fault },
    { "u_set_random_fault_of_type", "npc_set_random_fault_of_type", jarg::member, &talk_effect_fun::f_set_random_fault_of_type },
    { "u_activate", "npc_activate", jarg::member, &talk_effect_fun::f_activate },
    { "u_consume_item", "npc_consume_item", jarg::member, &talk_effect_fun::f_consume_item },
    { "u_consume_item_sum", "npc_consume_item_sum", jarg::array, &talk_effect_fun::f_consume_item_sum },
    { "u_remove_item_with", "npc_remove_item_with", jarg::member, &talk_effect_fun::f_remove_item_with },
    { "u_bulk_trade_accept", "npc_bulk_trade_accept", jarg::member, &talk_effect_fun::f_bulk_trade_accept },
    { "u_deal_damage", "npc_deal_damage", jarg::member, &talk_effect_fun::f_deal_damage },
    { "u_bulk_donate", "npc_bulk_donate", jarg::member, &talk_effect_fun::f_bulk_trade_accept },
    { "u_cast_spell", "npc_cast_spell", jarg::member, &talk_effect_fun::f_cast_spell },
    { "u_map_run_eocs", "npc_map_run_eocs", jarg::member, &talk_effect_fun::f_map_run_eocs },
    { "u_map_run_item_eocs", "npc_map_run_item_eocs", jarg::member, &talk_effect_fun::f_map_run_item_eocs },
    { "companion_mission", jarg::string, &talk_effect_fun::f_companion_mission },
    { "copy_var", jarg::member, &talk_effect_fun::f_copy_var },
    { "reveal_map", jarg::object, &talk_effect_fun::f_reveal_map },
    { "reveal_route", jarg::object, &talk_effect_fun::f_reveal_route },
    { "closest_city", jarg::object, &talk_effect_fun::f_closest_city },
    { "u_spend_cash", jarg::member | jarg::array, &talk_effect_fun::f_u_spend_cash },
    { "npc_change_faction", jarg::member, &talk_effect_fun::f_npc_change_faction },
    { "npc_change_class", jarg::member, &talk_effect_fun::f_npc_change_class },
    { "u_faction_rep", jarg::member | jarg::array, &talk_effect_fun::f_change_faction_rep },
    { "add_mission", jarg::member, &talk_effect_fun::f_add_mission },
    { "u_sell_item", jarg::member, &talk_effect_fun::f_u_sell_item },
    { "u_buy_item", jarg::member, &talk_effect_fun::f_u_buy_item },
    { "u_spawn_item", jarg::member, &talk_effect_fun::f_spawn_item },
    { "toggle_npc_rule", jarg::member, &talk_effect_fun::f_toggle_npc_rule },
    { "set_npc_rule", jarg::member, &talk_effect_fun::f_set_npc_rule },
    { "clear_npc_rule", jarg::member, &talk_effect_fun::f_clear_npc_rule },
    { "set_npc_engagement_rule", jarg::member, &talk_effect_fun::f_npc_engagement_rule },
    { "set_npc_aim_rule", jarg::member, &talk_effect_fun::f_npc_aim_rule },
    { "set_npc_cbm_reserve_rule", jarg::member, &talk_effect_fun::f_npc_cbm_reserve_rule },
    { "set_npc_cbm_recharge_rule", jarg::member, &talk_effect_fun::f_npc_cbm_recharge_rule },
    { "map_spawn_item", jarg::member, &talk_effect_fun::f_spawn_item },
    { "mapgen_update", jarg::member, &talk_effect_fun::f_mapgen_update },
    { "alter_timed_events", jarg::member, &talk_effect_fun::f_alter_timed_events },
    { "revert_location", jarg::member, &talk_effect_fun::f_revert_location },
    { "place_override", jarg::member, &talk_effect_fun::f_place_override },
    { "transform_line", jarg::member, &talk_effect_fun::f_transform_line },
    { "location_variable_adjust", jarg::member, &talk_effect_fun::f_location_variable_adjust },
    { "u_buy_monster", jarg::member, &talk_effect_fun::f_u_buy_monster },
    { "u_add_faction_trust", jarg::member | jarg::array, &talk_effect_fun::f_add_faction_trust },
    { "npc_first_topic", jarg::member, &talk_effect_fun::f_npc_first_topic },
    { "sound_effect", jarg::member, &talk_effect_fun::f_sound_effect },
    { "give_achievement", jarg::member, &talk_effect_fun::f_give_achievment },
    { "assign_mission", jarg::member, &talk_effect_fun::f_assign_mission },
    { "finish_mission", jarg::member, &talk_effect_fun::f_finish_mission },
    { "remove_active_mission", jarg::member, &talk_effect_fun::f_remove_active_mission },
    { "offer_mission", jarg::array | jarg::string, &talk_effect_fun::f_offer_mission },
    { "run_eocs", jarg::member | jarg::array, &talk_effect_fun::f_run_eocs },
    { "run_eoc_selector", jarg::member, &talk_effect_fun::f_run_eoc_selector },
    { "weighted_list_eocs", jarg::array, &talk_effect_fun::f_weighted_list_eocs },
    { "if", jarg::member, &talk_effect_fun::f_if },
    { "switch", jarg::member, &talk_effect_fun::f_switch },
    { "foreach", jarg::string, &talk_effect_fun::f_foreach },
    { "math", jarg::array, &talk_effect_fun::f_math },
    { "custom_light_level", jarg::member | jarg::array, &talk_effect_fun::f_custom_light_level },
    { "mirror_coordinates", jarg::member, &talk_effect_fun::f_mirror_coordinates },
    { "u_knockback", "npc_knockback", jarg::member, &talk_effect_fun::f_knockback },
    { "give_equipment", jarg::object, &talk_effect_fun::f_give_equipment },
    { "set_string_var", jarg::member | jarg::array, &talk_effect_fun::f_set_string_var },
    { "set_condition", jarg::member, &talk_effect_fun::f_set_condition },
    { "set_item_category_spawn_rates", jarg::member | jarg::array, &talk_effect_fun::f_set_item_category_spawn_rates },
    { "open_dialogue", jarg::member, &talk_effect_fun::f_open_dialogue },
    { "take_control", jarg::member, &talk_effect_fun::f_take_control },
    { "trigger_event", jarg::member, &talk_effect_fun::f_trigger_event },
    { "add_debt", jarg::array, &talk_effect_fun::f_add_debt },
    { "u_set_talker", "npc_set_talker", jarg::member, &talk_effect_fun::f_set_talker },
    { "turn_cost", jarg::member, &talk_effect_fun::f_turn_cost },
    { "transform_item", jarg::member, &talk_effect_fun::f_transform_item },
    // since parser checks all effects in order, having "message" field in any another effect (like in f_roll_remainder)
    // would cause parser to think it's a "message" effect
    { "message", "message", jarg::member, &talk_effect_fun::f_message },
};

void talk_effect_t::parse_sub_effect( const JsonObject &jo, std::string_view src )
{
    for( const sub_effect_parser &p : parsers ) {
        if( p.has_beta ) {
            if( p.check( jo ) ) {
                set_effect( p.f( jo, p.key_alpha, src, false ) );
                return;
            } else if( p.check( jo, true ) ) {
                set_effect( p.f( jo, p.key_beta, src, true ) );
                return;
            }
        } else if( p.check( jo ) ) {
            set_effect( p.f( jo, p.key_alpha, src, false ) );
            return;
        }
    }
    jo.throw_error( "invalid sub effect syntax: " + jo.str() );
}

void talk_effect_t::parse_string_effect( const std::string &effect_id, const JsonObject &jo,
        std::string_view src )
{
    static const std::unordered_map<std::string, void( * )( npc & )> static_functions_map = {
        {
#define WRAP( function ) { #function, &talk_function::function }
            WRAP( assign_mission ),
            WRAP( mission_success ),
            WRAP( mission_failure ),
            WRAP( clear_mission ),
            WRAP( mission_reward ),
            WRAP( start_trade ),
            WRAP( sort_loot ),
            WRAP( find_mount ),
            WRAP( dismount ),
            WRAP( do_chop_plank ),
            WRAP( do_vehicle_deconstruct ),
            WRAP( do_vehicle_repair ),
            WRAP( do_chop_trees ),
            WRAP( do_fishing ),
            WRAP( do_construction ),
            WRAP( do_mining ),
            WRAP( do_mopping ),
            WRAP( do_read ),
            WRAP( do_eread ),
            WRAP( do_read_repeatedly ),
            WRAP( do_craft ),
            WRAP( do_butcher ),
            WRAP( do_farming ),
            WRAP( assign_guard ),
            WRAP( assign_camp ),
            WRAP( abandon_camp ),
            WRAP( stop_guard ),
            WRAP( start_camp ),
            WRAP( buy_cow ),
            WRAP( buy_chicken ),
            WRAP( buy_horse ),
            WRAP( basecamp_mission ),
            WRAP( wake_up ),
            WRAP( reveal_stats ),
            WRAP( end_conversation ),
            WRAP( insult_combat ),
            WRAP( give_equipment ),
            WRAP( lesser_give_aid ),
            WRAP( lesser_give_all_aid ),
            WRAP( give_aid ),
            WRAP( give_all_aid ),
            WRAP( barber_beard ),
            WRAP( barber_hair ),
            WRAP( buy_haircut ),
            WRAP( buy_shave ),
            WRAP( morale_chat_activity ),
            WRAP( bionic_install ),
            WRAP( bionic_install_allies ),
            WRAP( bionic_remove ),
            WRAP( bionic_remove_allies ),
            WRAP( drop_items_in_place ),
            WRAP( follow ),
            WRAP( follow_only ),
            WRAP( deny_follow ),
            WRAP( deny_lead ),
            WRAP( deny_equipment ),
            WRAP( deny_train ),
            WRAP( deny_personal_info ),
            WRAP( hostile ),
            WRAP( flee ),
            WRAP( leave ),
            WRAP( stop_following ),
            WRAP( revert_activity ),
            WRAP( goto_location ),
            WRAP( stranger_neutral ),
            WRAP( start_mugging ),
            WRAP( player_leaving ),
            WRAP( drop_weapon ),
            WRAP( drop_stolen_item ),
            WRAP( remove_stolen_status ),
            WRAP( player_weapon_away ),
            WRAP( player_weapon_drop ),
            WRAP( lead_to_safety ),
            WRAP( start_training ),
            WRAP( start_training_npc ),
            WRAP( start_training_seminar ),
            WRAP( copy_npc_rules ),
            WRAP( set_npc_pickup ),
            WRAP( npc_thankful ),
            WRAP( clear_overrides ),
            WRAP( pick_style ),
            WRAP( do_disassembly ),
            WRAP( distribute_food_auto ),
            WRAP( nothing )
#undef WRAP
        }
    };
    const auto iter = static_functions_map.find( effect_id );
    if( iter != static_functions_map.end() ) {
        set_effect( iter->second );
        return;
    }

    if( effect_id == "u_bulk_trade_accept" || effect_id == "npc_bulk_trade_accept" ||
        effect_id == "u_bulk_donate" || effect_id == "npc_bulk_donate" ) {
        bool is_npc = effect_id == "npc_bulk_trade_accept" || effect_id == "npc_bulk_donate";
        set_effect( talk_effect_fun_t( talk_effect_fun::f_bulk_trade_accept( jo, effect_id, src,
                                       is_npc ) ) );
        return;
    }

    if( effect_id == "lightning" ) {
        set_effect( talk_effect_fun_t( talk_effect_fun::f_lightning() ) );
        return;
    }

    if( effect_id == "u_die" ) {
        set_effect( talk_effect_fun_t( talk_effect_fun::f_die( false ) ) );
        return;
    }

    if( effect_id == "npc_die" ) {
        set_effect( talk_effect_fun_t( talk_effect_fun::f_die( true ) ) );
        return;
    }

    if( effect_id == "u_cancel_activity" ) {
        set_effect( talk_effect_fun_t( talk_effect_fun::f_cancel_activity( false ) ) );
        return;
    }

    if( effect_id == "npc_cancel_activity" ) {
        set_effect( talk_effect_fun_t( talk_effect_fun::f_cancel_activity( true ) ) );
        return;
    }


    if( effect_id == "u_prevent_death" ) {
        set_effect( talk_effect_fun_t( talk_effect_fun::f_prevent_death( false ) ) );
        return;
    }

    if( effect_id == "npc_prevent_death" ) {
        set_effect( talk_effect_fun_t( talk_effect_fun::f_prevent_death( true ) ) );
        return;
    }

    if( effect_id == "next_weather" ) {
        set_effect( talk_effect_fun_t( talk_effect_fun::f_next_weather() ) );
        return;
    }

    if( effect_id == "npc_gets_item" || effect_id == "npc_gets_item_to_use" ) {
        bool to_use = effect_id == "npc_gets_item_to_use";
        set_effect( talk_effect_fun_t( talk_effect_fun::f_npc_gets_item( to_use ) ) );
        return;
    }

    if( effect_id == "u_ranged_attack" ) {
        set_effect( talk_effect_fun_t( talk_effect_fun::f_ranged_attack( false ) ) );
        return;
    }

    if( effect_id == "npc_ranged_attack" ) {
        set_effect( talk_effect_fun_t( talk_effect_fun::f_ranged_attack( true ) ) );
        return;
    }

    if( effect_id == "open_dialogue" ) {
        set_effect( talk_effect_fun_t( talk_effect_fun::f_open_dialogue( jo, "", src ) ) );
        return;
    }
    if( effect_id == "take_control" ) {
        set_effect( talk_effect_fun_t( talk_effect_fun::f_take_control( jo, "", src ) ) );
        return;
    }
    if( effect_id == "take_control_menu" ) {
        set_effect( talk_effect_fun_t( talk_effect_fun::f_take_control_menu() ) );
        return;
    }
    if( effect_id == "u_wants_to_talk" ) {
        set_effect( talk_effect_fun_t( talk_effect_fun::f_wants_to_talk( false ) ) );
        return;
    }
    if( effect_id == "npc_wants_to_talk" ) {
        set_effect( talk_effect_fun_t( talk_effect_fun::f_wants_to_talk( true ) ) );
        return;
    }
    if( effect_id == "npc_rules_menu" ) {
        set_effect( talk_effect_fun_t( talk_effect_fun::f_npc_rules_menu() ) );
        return;
    }
    jo.throw_error_at( effect_id, "unknown effect string" );
}

void talk_effect_t::load_effect( const JsonObject &jo, const std::string &member_name,
                                 std::string_view src )
{
    if( jo.has_member( "opinion" ) ) {
        JsonValue jv = jo.get_member( "opinion" );
        // Same format as when saving a game (-:
        opinion.deserialize( jv );
    }
    if( jo.has_member( "mission_opinion" ) ) {
        JsonValue jv = jo.get_member( "mission_opinion" );
        // Same format as when saving a game (-:
        mission_opinion.deserialize( jv );
    }
    if( !jo.has_member( member_name ) ) {
        return;
    } else if( jo.has_string( member_name ) ) {
        const std::string type = jo.get_string( member_name );
        parse_string_effect( type, jo, src );
    } else if( jo.has_object( member_name ) ) {
        JsonObject sub_effect = jo.get_object( member_name );
        parse_sub_effect( sub_effect, src );
    } else if( jo.has_array( member_name ) ) {
        for( const JsonValue entry : jo.get_array( member_name ) ) {
            if( entry.test_string() ) {
                const std::string type = entry.get_string();
                parse_string_effect( type, jo, src );
            } else if( entry.test_object() ) {
                JsonObject sub_effect = entry.get_object();
                parse_sub_effect( sub_effect, src );
            } else {
                jo.throw_error_at( member_name, "invalid effect array syntax" );
            }
        }
    } else {
        jo.throw_error_at( member_name, "invalid effect syntax" );
    }
}

talk_response::talk_response()
{
    truefalse_condition = []( const_dialogue const & ) {
        return true;
    };
    mission_selected = nullptr;
    // Why aren't these null ids? Well, it turns out most responses give
    // empty ids, so things like the training code check for these empty ids
    // and when it's given a null id, it breaks
    // FIXME: Use null ids
    skill = skill_id();
    style = matype_id();
    proficiency = proficiency_id();
    dialogue_spell = spell_id();
}

talk_response::talk_response( const JsonObject &jo, std::string_view src )
{
    if( jo.has_member( "truefalsetext" ) ) {
        JsonObject truefalse_jo = jo.get_object( "truefalsetext" );
        read_condition( truefalse_jo, "condition", truefalse_condition, true );
        truefalse_jo.read( "true", truetext );
        truefalse_jo.read( "false", falsetext );
    } else {
        jo.read( "text", truetext );
        truefalse_condition = []( const_dialogue const & ) {
            return true;
        };
    }
    if( jo.has_member( "trial" ) ) {
        JsonObject trial_obj = jo.get_object( "trial" );
        trial = talk_trial( trial_obj );
    }
    if( jo.has_member( "success" ) ) {
        JsonObject success_obj = jo.get_object( "success" );
        success = talk_effect_t( success_obj, "effect", src );
    } else if( jo.has_string( "topic" ) ) {
        // This is for simple topic switching without a possible failure
        success.next_topic = talk_topic( jo.get_string( "topic" ) );
        success.load_effect( jo, "effect", src );
    } else if( jo.has_object( "topic" ) ) {
        success.next_topic = load_inline_topic( jo.get_object( "topic" ), src );
    }
    if( trial && !jo.has_member( "failure" ) ) {
        jo.throw_error( "the failure effect is mandatory if a talk_trial has been defined" );
    }
    if( jo.has_member( "failure" ) ) {
        JsonObject failure_obj = jo.get_object( "failure" );
        failure = talk_effect_t( failure_obj, "effect", src );
    }
    if( jo.has_member( "show_always" ) ) {
        show_always = jo.get_bool( "show_always" );
    }
    if( jo.has_member( "show_reason" ) ) {
        show_reason = jo.get_string( "show_reason" );
    }
    if( jo.has_member( "show_condition" ) ) {
        if( show_always ) {
            jo.throw_error( "show_always and show_condition cannot both be defined" );
        }
        read_condition( jo, "show_condition", show_condition, false );
    }
    // TODO: mission_selected
    // TODO: skill
    // TODO: style
}

json_talk_repeat_response::json_talk_repeat_response( const JsonObject &jo,
        std::string_view src )
{
    if( jo.has_bool( "is_npc" ) ) {
        is_npc = true;
    }
    if( jo.has_bool( "include_containers" ) ) {
        include_containers = true;
    }
    if( jo.has_string( "for_item" ) ) {
        for_item.emplace_back( jo.get_string( "for_item" ) );
    } else if( jo.has_array( "for_item" ) ) {
        for( const std::string line : jo.get_array( "for_item" ) ) {
            for_item.emplace_back( line );
        }
    } else if( jo.has_string( "for_category" ) ) {
        for_category.emplace_back( jo.get_string( "for_category" ) );
    } else if( jo.has_array( "for_category" ) ) {
        for( const std::string line : jo.get_array( "for_category" ) ) {
            for_category.emplace_back( line );
        }
    } else {
        jo.throw_error( "Repeat response with no repeat information!" );
    }
    if( for_item.empty() && for_category.empty() ) {
        jo.throw_error( "Repeat response with empty repeat information!" );
    }
    if( jo.has_object( "response" ) ) {
        JsonObject response_obj = jo.get_object( "response" );
        response = json_talk_response( response_obj, src );
    } else {
        jo.throw_error( "Repeat response with no response!" );
    }
}

json_talk_response::json_talk_response( const JsonObject &jo, std::string_view src )
    : actual_response( jo, src )
{
    load_condition( jo, src );
    get_raw_debug_fields( jo, actual_response.debug_info );
    actual_response.condition = condition;
}

void json_talk_response::load_condition( const JsonObject &jo, std::string_view )
{
    has_condition_ = jo.has_member( "condition" );
    is_switch = jo.get_bool( "switch", false );
    is_default = jo.get_bool( "default", false );
    read_condition( jo, "condition", condition, true );

    optional( jo, true, "failure_explanation", failure_explanation );
    optional( jo, true, "failure_topic", failure_topic );
}

bool json_talk_response::test_condition( dialogue &d ) const
{
    if( condition ) {
        return condition( d );
    }
    //if condition doesn't exist, it evaluates to true if tested
    return true;
}

const talk_response &json_talk_response::get_actual_response() const
{
    return actual_response;
}

bool json_talk_response::gen_responses( dialogue &d, bool switch_done )
{
    bool condition_result = test_condition( d );

    if( !is_switch || !switch_done || d.debug_ignore_conditionals ) {
        if( condition_result || show_anyways( d ) || d.debug_ignore_conditionals ) {
            actual_response.ignore_conditionals = !condition_result && ( show_anyways( d ) ||
                                                  d.debug_ignore_conditionals );
            d.add_gen_response( actual_response, false, !!condition, condition_result );
            return is_switch && !is_default;
        } else if( !failure_explanation.empty() || !failure_topic.empty() ) {
            // build additional talk responses for failed options with an explanation if details are given
            talk_response tr = talk_response();
            tr.truetext = no_translation(
                              string_format( pgettext( "failure_explanation: actual_response", "*%s: %s" ),
                                             failure_explanation.translated(), actual_response.truetext.translated() ) );
            if( !failure_topic.empty() ) {
                // Default is TALK_NONE otherwise go to the failure topic provided
                tr.success.next_topic = talk_topic( failure_topic );
            }
            d.add_gen_response( tr, false, !!condition, condition_result );
        }
    }
    return false;
}

// repeat responses always go in front
bool json_talk_response::gen_repeat_response( dialogue &d, const itype_id &item_id,
        bool switch_done ) const
{
    if( !is_switch || !switch_done ) {
        if( test_condition( d ) ) {
            talk_response result = actual_response;
            result.success.next_topic.item_type = item_id;
            result.failure.next_topic.item_type = item_id;
            d.add_gen_response( result, !!condition, true, true );
            return is_switch && !is_default;
        }
    }
    return false;
}

static std::string translate_gendered_line(
    const std::string &line,
    const std::vector<std::string> &relevant_genders,
    const dialogue &d
)
{
    GenderMap gender_map;
    for( const std::string &subject : relevant_genders ) {
        if( subject == "npc" ) {
            gender_map[subject] = d.actor( true )->get_grammatical_genders();
        } else if( subject == "u" ) {
            gender_map[subject] = d.actor( false )->get_grammatical_genders();
        } else {
            debugmsg( "Unsupported subject '%s' for grammatical gender in dialogue", subject );
        }
    }
    return gettext_gendered( gender_map, line );
}

dynamic_line_t dynamic_line_t::from_member( const JsonObject &jo,
        std::string_view member_name )
{
    if( jo.has_array( member_name ) ) {
        return dynamic_line_t( jo.get_array( member_name ) );
    } else if( jo.has_object( member_name ) ) {
        JsonObject dl = jo.get_object( member_name );
        if( dl.has_member( "str" ) ) {
            dl.allow_omitted_members();
            translation line;
            jo.read( member_name, line );
            return dynamic_line_t( line );
        } else {
            return dynamic_line_t( dl );
        }
    } else if( jo.has_string( member_name ) ) {
        translation line;
        jo.read( member_name, line );
        return dynamic_line_t( line );
    } else {
        return dynamic_line_t{};
    }
}

dynamic_line_t::dynamic_line_t( const translation &line )
{
    function = [line]( const dialogue & ) {
        return line.translated();
    };
}

dynamic_line_t::dynamic_line_t( const JsonObject &jo )
{
    if( jo.has_member( "concatenate" ) ) {
        std::vector<dynamic_line_t> lines;
        for( const JsonValue entry : jo.get_array( "concatenate" ) ) {
            if( entry.test_string() ) {
                translation line;
                entry.read( line );
                lines.emplace_back( line );
            } else if( entry.test_array() ) {
                lines.emplace_back( entry.get_array() );
            } else if( entry.test_object() ) {
                JsonObject dl = entry.get_object();
                if( dl.has_member( "str" ) ) {
                    dl.allow_omitted_members();
                    translation line;
                    entry.read( line );
                    lines.emplace_back( line );
                } else {
                    lines.emplace_back( dl );
                }
            } else {
                entry.throw_error( "invalid format: must be string, array or object" );
            }
        }
        function = [lines]( dialogue & d ) {
            std::string all_lines;
            for( const dynamic_line_t &line : lines ) {
                all_lines += line( d );
            }
            return all_lines;
        };
    } else if( jo.get_bool( "give_hint", false ) ) {
        function = [&]( const dialogue & ) {
            return get_hint();
        };
    } else if( jo.get_bool( "use_reason", false ) ) {
        function = [&]( const dialogue & d ) {
            std::string tmp = d.reason;
            d.reason.clear();
            return tmp;
        };
    } else if( jo.get_bool( "list_faction_camp_sites", false ) ) {
        function = [&]( const dialogue & ) {
            const auto &sites = recipe_group::get_recipes_by_id( "all_faction_base_types" );
            if( sites.empty() ) {
                return std::string( _( "I can't think of a single place I can build a camp." ) );
            }
            std::string tmp = "I can start a new camp as a ";
            tmp += enumerate_as_string( sites.begin(), sites.end(),
            []( const std::pair<recipe_id, translation> &site ) {
                return site.second.translated();
            },
            enumeration_conjunction::or_ );
            return tmp;
        };
    } else if( jo.has_string( "gendered_line" ) ) {
        std::string line;
        mandatory( jo, false, "gendered_line", line, text_style_check_reader() );
        if( !jo.has_array( "relevant_genders" ) ) {
            jo.throw_error(
                R"(dynamic line with "gendered_line" must also have "relevant_genders")" );
        }
        std::vector<std::string> relevant_genders;
        for( const std::string gender : jo.get_array( "relevant_genders" ) ) {
            relevant_genders.push_back( gender );
            if( gender != "npc" && gender != "u" ) {
                jo.throw_error( "Unexpected subject in relevant_genders; expected 'npc' or 'u'" );
            }
        }
        function = [line, relevant_genders]( const dialogue & d ) {
            return translate_gendered_line( line, relevant_genders, d );
        };
    } else {
        conditional_t dcondition;
        const dynamic_line_t yes = from_member( jo, "yes" );
        const dynamic_line_t no = from_member( jo, "no" );
        for( const std::string &sub_member : dialogue_data::simple_string_conds() ) {
            if( jo.has_bool( sub_member ) ) {
                // This also marks the member as visited.
                if( !jo.get_bool( sub_member ) ) {
                    jo.throw_error_at( sub_member, "value must be true" );
                }
                dcondition = conditional_t( sub_member );
                function = [dcondition, yes, no]( dialogue & d ) {
                    return ( dcondition( d ) ? yes : no )( d );
                };
                return;
            } else if( jo.has_member( sub_member ) ) {
                dcondition = conditional_t( sub_member );
                const dynamic_line_t yes_member = from_member( jo, sub_member );
                function = [dcondition, yes_member, no]( dialogue & d ) {
                    return ( dcondition( d ) ? yes_member : no )( d );
                };
                return;
            }
        }
        for( const std::string &sub_member : dialogue_data::complex_conds() ) {
            if( jo.has_member( sub_member ) ) {
                dcondition = conditional_t( jo );
                function = [dcondition, yes, no]( dialogue & d ) {
                    return ( dcondition( d ) ? yes : no )( d );
                };
                return;
            }
        }
        jo.throw_error( "dynamic line not supported" );
    }
}

dynamic_line_t::dynamic_line_t( const JsonArray &ja )
{
    std::vector<dynamic_line_t> lines;
    for( const JsonValue entry : ja ) {
        if( entry.test_string() ) {
            translation line;
            entry.read( line );
            lines.emplace_back( line );
        } else if( entry.test_array() ) {
            lines.emplace_back( entry.get_array() );
        } else if( entry.test_object() ) {
            JsonObject dl = entry.get_object();
            if( dl.has_member( "str" ) ) {
                dl.allow_omitted_members();
                translation line;
                entry.read( line );
                lines.emplace_back( line );
            } else {
                lines.emplace_back( dl );
            }
        } else {
            entry.throw_error( "invalid format: must be string, array or object" );
        }
    }
    function = [lines]( dialogue & d ) {
        const dynamic_line_t &line = random_entry_ref( lines );
        return line( d );
    };
}

json_dynamic_line_effect::json_dynamic_line_effect( const JsonObject &jo,
        const std::string &id, std::string_view src )
{
    std::function<bool( const_dialogue const & )> tmp_condition;
    read_condition( jo, "condition", tmp_condition, true );
    talk_effect_t tmp_effect = talk_effect_t( jo, "effect", src );
    get_raw_debug_fields( jo, debug_info );
    // if the topic has a sentinel, it means implicitly add a check for the sentinel value
    // and do not run the effects if it is set.  if it is not set, run the effects and
    // set the sentinel
    if( jo.has_string( "sentinel" ) ) {
        const std::string sentinel = jo.get_string( "sentinel" );
        const std::string varname = "sentinel_" + id + "_" + sentinel;
        condition = [varname, tmp_condition]( dialogue & d ) {
            return d.actor( false )->get_value( varname ).str() != "yes" && tmp_condition( d );
        };
        std::function<void( const dialogue &d )> function = [varname]( const dialogue & d ) {
            d.actor( false )->set_value( varname, "yes" );
        };
        tmp_effect.effects.emplace_back( function );
    } else {
        condition = tmp_condition;
    }
    effect = tmp_effect;
}

bool json_dynamic_line_effect::test_condition( dialogue &d ) const
{
    return condition( d );
}

void json_dynamic_line_effect::apply( dialogue &d ) const
{
    effect.apply( d );
}

void json_talk_topic::load( const JsonObject &jo, std::string_view src )
{
    if( jo.has_member( "dynamic_line" ) ) {
        dynamic_line = dynamic_line_t::from_member( jo, "dynamic_line" );
    }
    if( jo.has_member( "speaker_effect" ) ) {
        std::string id = "no_id";
        if( jo.has_string( "id" ) ) {
            id = jo.get_string( "id" );
        } else if( jo.has_array( "id" ) ) {
            id = jo.get_array( "id" ).next_string();
        }
        if( jo.has_object( "speaker_effect" ) ) {
            JsonObject speaker_effect = jo.get_object( "speaker_effect" );
            speaker_effects.emplace_back( speaker_effect, id, src );
        } else if( jo.has_array( "speaker_effect" ) ) {
            for( JsonObject speaker_effect : jo.get_array( "speaker_effect" ) ) {
                speaker_effects.emplace_back( speaker_effect, id, src );
            }
        }
    }
    bool insert_above_bottom = false;
    if( jo.has_bool( "insert_before_standard_exits" ) ) {
        insert_above_bottom = jo.get_bool( "insert_before_standard_exits" );
    }
    if( !insert_above_bottom || responses.empty() ) {
        for( JsonObject response : jo.get_array( "responses" ) ) {
            responses.emplace_back( response, src );
        }
    } else {
        int dec_count = 0;
        if( !responses.empty() &&
            responses.back().get_actual_response().success.next_topic.id == "TALK_DONE" ) {
            dec_count = 1;
        }
        if( responses.size() >= 2 &&
            responses[ responses.size() - 2].get_actual_response().success.next_topic.id == "TALK_NONE" ) {
            dec_count = 2;
        }
        for( JsonObject response : jo.get_array( "responses" ) ) {
            responses.emplace( responses.end() - dec_count, response, src );
        }
    }
    if( jo.has_object( "repeat_responses" ) ) {
        repeat_responses.emplace_back( jo.get_object( "repeat_responses" ), src );
    } else if( jo.has_array( "repeat_responses" ) ) {
        for( JsonObject elem : jo.get_array( "repeat_responses" ) ) {
            repeat_responses.emplace_back( elem, src );
        }
    }
    if( responses.empty() ) {
        jo.throw_error_at( "responses", "no responses for talk topic defined" );
    }
    replace_built_in_responses = jo.get_bool( "replace_built_in_responses",
                                 replace_built_in_responses );
}

bool json_talk_topic::gen_responses( dialogue &d )
{
    d.responses.reserve( responses.size() ); // A wild guess, can actually be more or less

    bool switch_done = false;
    for( json_talk_response &r : responses ) {
        switch_done |= r.gen_responses( d, switch_done );
    }
    for( const json_talk_repeat_response &repeat : repeat_responses ) {
        talker *actor =  d.actor( repeat.is_npc );
        std::function<bool( const item & )> filter = return_true<item>;
        for( const itype_id &item_id : repeat.for_item ) {
            if( actor->charges_of( item_id ) > 0 || actor->has_amount( item_id, 1 ) ) {
                switch_done |= repeat.response.gen_repeat_response( d, item_id, switch_done );
            }
        }
        for( const item_category_id &category_id : repeat.for_category ) {
            const bool include_containers = repeat.include_containers;
            const auto items_with = actor->items_with( [category_id,
            include_containers]( const item & it ) {
                if( include_containers ) {
                    return it.get_category_of_contents().get_id() == category_id;
                }
                return it.type && it.type->category_force == category_id;
            } );
            for( item * const &it : items_with ) {
                switch_done |= repeat.response.gen_repeat_response( d, it->typeId(), switch_done );
            }
        }
    }

    return replace_built_in_responses;
}

cata::flat_set<std::string> json_talk_topic::get_directly_reachable_topics(
    bool only_unconditional ) const
{
    std::vector<std::string> result;

    auto add_reachable_for_response = [&]( const json_talk_response & json_response ) {
        const talk_response &response = json_response.get_actual_response();
        if( !only_unconditional || !json_response.has_condition() ) {
            result.push_back( response.success.next_topic.id );
            result.push_back( response.failure.next_topic.id );
        }
    };

    for( const json_talk_response &r : responses ) {
        add_reachable_for_response( r );
    }
    for( const json_talk_repeat_response &r : repeat_responses ) {
        add_reachable_for_response( r.response );
    }

    return cata::flat_set<std::string>( result.begin(), result.end() );
}

std::vector<json_talk_response> json_talk_topic::get_responses()
{
    return responses;
}

std::string json_talk_topic::get_dynamic_line( dialogue &d ) const
{
    return dynamic_line( d );
}

std::vector<json_dynamic_line_effect> json_talk_topic::get_speaker_effects() const
{
    return speaker_effects;
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

void load_talk_topic( const JsonObject &jo, std::string_view src )
{
    if( jo.has_array( "id" ) ) {
        for( auto &id : jo.get_string_array( "id" ) ) {
            json_talk_topics[id].load( jo, src );
        }
    } else {
        const std::string id = jo.get_string( "id" );
        json_talk_topics[id].load( jo, src );
    }
}

std::string npc::pick_talk_topic( const Character &/*u*/ )
{
    if( personality.aggression > 0 ) {
        if( op_of_u.fear * 2 < personality.bravery && personality.altruism < 0 ) {
            set_attitude( NPCATT_MUG );
            return chatbin.talk_mug ;
        }

        if( personality.aggression + personality.bravery - op_of_u.fear > 0 ) {
            return chatbin.talk_stranger_aggressive ;
        }
    }

    if( op_of_u.fear * 2 > personality.altruism + personality.bravery ) {
        return chatbin.talk_stranger_scared;
    }

    if( op_of_u.fear * 2 > personality.bravery + op_of_u.trust ) {
        return chatbin.talk_stranger_wary;
    }

    if( op_of_u.trust - op_of_u.fear +
        ( personality.bravery + personality.altruism ) / 2 > 0 ) {
        return chatbin.talk_stranger_friendly;
    }

    set_attitude( NPCATT_NULL );
    return chatbin.talk_stranger_neutral;
}

std::string const &npc::get_specified_talk_topic( std::string const &topic_id )
{
    static const dialogue_chatbin default_chatbin;
    std::vector<std::pair<std::string const &, std::string const &>> const talk_topics = {
        {default_chatbin.first_topic, chatbin.first_topic},
        {default_chatbin.talk_radio, chatbin.talk_radio},
        {default_chatbin.talk_leader, chatbin.talk_leader},
        {default_chatbin.talk_friend, chatbin.talk_friend},
        {default_chatbin.talk_stole_item, chatbin.talk_stole_item},
        {default_chatbin.talk_wake_up, chatbin.talk_wake_up},
        {default_chatbin.talk_mug, chatbin.talk_mug},
        {default_chatbin.talk_stranger_aggressive, chatbin.talk_stranger_aggressive},
        {default_chatbin.talk_stranger_scared, chatbin.talk_stranger_scared},
        {default_chatbin.talk_stranger_wary, chatbin.talk_stranger_wary},
        {default_chatbin.talk_stranger_friendly, chatbin.talk_stranger_friendly},
        {default_chatbin.talk_stranger_neutral, chatbin.talk_stranger_neutral},
        {default_chatbin.talk_friend_guard, chatbin.talk_friend_guard}
    };

    const auto iter = std::find_if( talk_topics.begin(), talk_topics.end(),
    [&topic_id]( const std::pair<std::string, std::string> &pair ) {
        return pair.first == topic_id;
    } );
    if( iter != talk_topics.end() ) {
        return iter->second;
    }

    return topic_id;
}

bool npc::has_item_whitelist() const
{
    return is_player_ally() && !rules.pickup_whitelist->empty();
}

bool npc::item_name_whitelisted( const std::string &to_match )
{
    if( !has_item_whitelist() ) {
        return true;
    }

    auto_pickup::npc_settings &wlist = *rules.pickup_whitelist;
    const rule_state rule = wlist.check_item( to_match );
    if( rule == rule_state::WHITELISTED ) {
        return true;
    }

    if( rule == rule_state::BLACKLISTED ) {
        return false;
    }

    wlist.create_rule( to_match );
    return wlist.check_item( to_match ) == rule_state::WHITELISTED;
}

bool npc::item_whitelisted( const item &it )
{
    if( !has_item_whitelist() ) {
        return true;
    }

    const std::string to_match = it.tname( 1, false );
    return item_name_whitelisted( to_match );
}

const json_talk_topic *get_talk_topic( const std::string &id )
{
    auto it = json_talk_topics.find( id );
    if( it == json_talk_topics.end() ) {
        return nullptr;
    }
    return &it->second;
}

std::vector<std::string> get_all_talk_topic_ids()
{
    std::vector<std::string> dialogue_ids;
    dialogue_ids.reserve( json_talk_topics.size() );
    for( auto &elem : json_talk_topics ) {
        dialogue_ids.push_back( elem.first );
    }
    return dialogue_ids;
}

