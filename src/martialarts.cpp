#include "martialarts.h"

#include <imgui/imgui.h>
#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "bodypart.h"
#include "cata_imgui.h"
#include "cata_utility.h"
#include "character.h"
#include "character_attire.h"
#include "character_martial_arts.h"
#include "color.h"
#include "condition.h"
#include "creature.h"
#include "damage.h"
#include "debug.h"
#include "dialogue.h"
#include "effect.h"
#include "effect_on_condition.h"
#include "enums.h"
#include "flexbuffer_json.h"
#include "game_constants.h"
#include "generic_factory.h"
#include "input_context.h"
#include "item.h"
#include "item_factory.h"
#include "item_location.h"
#include "itype.h"
#include "localized_comparator.h"
#include "messages.h"
#include "output.h"
#include "pimpl.h"
#include "skill.h"
#include "string_formatter.h"
#include "subbodypart.h"
#include "talker.h"
#include "text.h"
#include "translations.h"
#include "ui_manager.h"
#include "value_ptr.h"
#include "weighted_list.h"

class json_flag;

static const bionic_id bio_armor_arms( "bio_armor_arms" );
static const bionic_id bio_armor_legs( "bio_armor_legs" );
static const bionic_id bio_cqb( "bio_cqb" );

static const flag_id json_flag_PROVIDES_TECHNIQUES( "PROVIDES_TECHNIQUES" );

static const json_character_flag json_flag_ALWAYS_BLOCK( "ALWAYS_BLOCK" );
static const json_character_flag json_flag_NONSTANDARD_BLOCK( "NONSTANDARD_BLOCK" );

static const limb_score_id limb_score_block( "block" );

static const skill_id skill_unarmed( "unarmed" );

static const weapon_category_id weapon_category_OTHER_INVALID_WEAP_CAT( "OTHER_INVALID_WEAP_CAT" );


namespace
{
generic_factory<weapon_category> weapon_category_factory( "weapon category" );
generic_factory<ma_technique> ma_techniques( "martial art technique" );
generic_factory<martialart> martialarts( "martial art style" );
generic_factory<ma_buff> ma_buffs( "martial art buff" );
generic_factory<attack_vector> attack_vector_factory( "attack vector" );
} // namespace

/** @relates string_id */
template<>
const attack_vector &string_id<attack_vector>::obj() const
{
    return attack_vector_factory.obj( *this );
}

template<>
bool attack_vector_id::is_valid() const
{
    return attack_vector_factory.is_valid( *this );
}

void attack_vector::load_attack_vectors( const JsonObject &jo, const std::string &src )
{
    attack_vector_factory.load( jo, src );
}

void attack_vector::reset()
{
    attack_vector_factory.reset();
}

void attack_vector::load( const JsonObject &jo, std::string_view )
{
    mandatory( jo, was_loaded, "id", id );
    optional( jo, was_loaded, "weapon", weapon, false );
    optional( jo, was_loaded, "limbs", limbs );
    optional( jo, was_loaded, "strict_limb_definition", strict_limb_definition, false );
    optional( jo, was_loaded, "contact_area", contact_area );
    optional( jo, was_loaded, "limb_req", limb_req );
    optional( jo, was_loaded, "armor_bonus", armor_bonus, true );
    optional( jo, was_loaded, "encumbrance_limit", encumbrance_limit, 100 );
    optional( jo, was_loaded, "bp_hp_limit", bp_hp_limit, 10 );
    optional( jo, was_loaded, "required_limb_flags", required_limb_flags );
    optional( jo, was_loaded, "forbidden_limb_flags", forbidden_limb_flags );
}

template<>
const weapon_category &weapon_category_id::obj() const
{
    return weapon_category_factory.obj( *this );
}

/** @relates string_id */
template<>
bool weapon_category_id::is_valid() const
{
    return weapon_category_factory.is_valid( *this );
}

void weapon_category::load_weapon_categories( const JsonObject &jo, const std::string &src )
{
    weapon_category_factory.load( jo, src );
}

void weapon_category::reset()
{
    weapon_category_factory.reset();
}

void weapon_category::load( const JsonObject &jo, std::string_view )
{
    mandatory( jo, was_loaded, "name", name_ );
    optional( jo, was_loaded, "proficiencies", proficiencies_ );
}

void weapon_category::verify_weapon_categories()
{
    weapon_category_factory.check();
}

void weapon_category::check() const
{
    for( const proficiency_id &prof : proficiencies_ ) {
        if( !prof.is_valid() ) {
            debugmsg( "Proficiency %s does not exist in weapon category %s", prof.str(), id.str() );
        }
    }
}

const std::vector<weapon_category> &weapon_category::get_all()
{
    return weapon_category_factory.get_all();
}

matype_id martial_art_learned_from( const itype &type )
{
    if( !type.can_use( "MA_MANUAL" ) ) {
        return {};
    }

    if( !type.book || type.book->martial_art.is_null() ) {
        debugmsg( "Item '%s' which claims to teach a martial art is missing martial_art",
                  type.get_id().str() );
        return {};
    }

    return type.book->martial_art;
}

void load_technique( const JsonObject &jo, const std::string &src )
{
    ma_techniques.load( jo, src );
}

// To avoid adding empty entries
template <typename Container>
void add_if_exists( const JsonObject &jo, Container &cont, bool was_loaded,
                    const std::string &json_key, const typename Container::key_type &id )
{
    if( jo.has_member( json_key ) ) {
        mandatory( jo, was_loaded, json_key, cont[id] );
    }
}

class ma_skill_reader : public generic_typed_reader<ma_skill_reader>
{
    public:
        std::pair<skill_id, int> get_next( const JsonObject &jo ) const {
            return std::pair<skill_id, int>( skill_id( jo.get_string( "name" ) ), jo.get_int( "level" ) );
        }
        template<typename C>
        void erase_next( std::string &&skill_id_str, C &container ) const {
            const skill_id id = skill_id( std::move( skill_id_str ) );
            reader_detail::handler<C>().erase_if( container, [&id]( const std::pair<skill_id, int> &e ) {
                return e.first == id;
            } );
        }
};

class ma_weapon_damage_reader : public generic_typed_reader<ma_weapon_damage_reader>
{
    public:
        std::pair<damage_type_id, int> get_next( const JsonObject &jo ) const {
            return std::pair<damage_type_id, int>( jo.get_string( "type" ), jo.get_int( "min" ) );
        }
        template<typename C>
        void erase_next( const JsonObject &jo, C &container ) const {
            damage_type_id id( jo.get_string( "type" ) );
            reader_detail::handler<C>().erase_if( container,
            [&id]( const std::pair<const damage_type_id, int> &e ) {
                return e.first == id;
            } );
        }
};

tech_effect_data load_tech_effect_data( const JsonObject &e )
{
    return tech_effect_data( efftype_id( e.get_string( "id" ) ), e.get_int( "duration", 0 ),
                             e.get_bool( "permanent", false ), e.get_bool( "on_damage", true ),
                             e.get_int( "chance", 100 ), e.get_string( "message", "" ),
                             json_character_flag( e.get_string( "req_flag", "NULL" ) ) );
}

class tech_effect_reader : public generic_typed_reader<tech_effect_reader>
{
    public:
        tech_effect_data get_next( JsonValue &jv ) const {
            JsonObject e = jv.get_object();
            return load_tech_effect_data( e );
        }
        template<typename C>
        void erase_next( std::string &&eff_str, C &container ) const {
            const efftype_id id = efftype_id( std::move( eff_str ) );
            reader_detail::handler<C>().erase_if( container, [&id]( const tech_effect_data & e ) {
                return e.id == id;
            } );
        }
};

void ma_requirements::load( const JsonObject &jo, std::string_view )
{
    optional( jo, was_loaded, "unarmed_allowed", unarmed_allowed, false );
    optional( jo, was_loaded, "melee_allowed", melee_allowed, false );
    if( jo.has_string( "weapon_categories_allowed" ) ) {
        weapon_category_id tmp_id;
        mandatory( jo, was_loaded, "weapon_categories_allowed", tmp_id );
        weapon_categories_allowed.emplace_back( tmp_id );
    } else {
        optional( jo, was_loaded, "weapon_categories_allowed", weapon_categories_allowed );
    }
    optional( jo, was_loaded, "strictly_unarmed", strictly_unarmed, false );
    optional( jo, was_loaded, "wall_adjacent", wall_adjacent, false );

    optional( jo, was_loaded, "required_buffs_all", req_buffs_all, string_id_reader<::ma_buff> {} );
    optional( jo, was_loaded, "required_buffs_any", req_buffs_any, string_id_reader<::ma_buff> {} );
    optional( jo, was_loaded, "forbidden_buffs_all", forbid_buffs_all, string_id_reader<::ma_buff> {} );
    optional( jo, was_loaded, "forbidden_buffs_any", forbid_buffs_any, string_id_reader<::ma_buff> {} );

    optional( jo, was_loaded, "req_flags", req_flags, string_id_reader<::json_flag> {} );
    optional( jo, was_loaded, "required_char_flags", req_char_flags );
    optional( jo, was_loaded, "required_char_flags_all", req_char_flags_all );
    optional( jo, was_loaded, "forbidden_char_flags", forbidden_char_flags );

    optional( jo, was_loaded, "skill_requirements", min_skill, ma_skill_reader {} );
    optional( jo, was_loaded, "weapon_damage_requirements", min_damage, ma_weapon_damage_reader {} );
}

void ma_technique::load( const JsonObject &jo, std::string_view src )
{
    mandatory( jo, was_loaded, "name", name );
    optional( jo, was_loaded, "description", description, translation() );

    if( jo.has_member( "messages" ) ) {
        JsonArray jsarr = jo.get_array( "messages" );
        jsarr.read( 0, avatar_message );
        jsarr.read( 1, npc_message );
    }

    optional( jo, was_loaded, "crit_tec", crit_tec, false );
    optional( jo, was_loaded, "crit_ok", crit_ok, false );
    optional( jo, was_loaded, "wall_adjacent", wall_adjacent, false );
    optional( jo, was_loaded, "reach_tec", reach_tec, false );
    optional( jo, was_loaded, "reach_ok", reach_ok, false );

    optional( jo, was_loaded, "needs_ammo", needs_ammo, false );

    optional( jo, was_loaded, "defensive", defensive, false );
    optional( jo, was_loaded, "disarms", disarms, false );
    optional( jo, was_loaded, "take_weapon", take_weapon, false );
    optional( jo, was_loaded, "side_switch", side_switch, false );
    optional( jo, was_loaded, "dummy", dummy, false );
    optional( jo, was_loaded, "dodge_counter", dodge_counter, false );
    optional( jo, was_loaded, "block_counter", block_counter, false );
    optional( jo, was_loaded, "miss_recovery", miss_recovery, false );
    optional( jo, was_loaded, "grab_break", grab_break, false );

    optional( jo, was_loaded, "weighting", weighting, 1 );

    optional( jo, was_loaded, "repeat_min", repeat_min, 1 );
    optional( jo, was_loaded, "repeat_max", repeat_max, 1 );
    optional( jo, was_loaded, "down_dur", down_dur, 0 );
    optional( jo, was_loaded, "stun_dur", stun_dur, 0 );
    optional( jo, was_loaded, "knockback_dist", knockback_dist, 0 );
    optional( jo, was_loaded, "knockback_spread", knockback_spread, 0 );
    optional( jo, was_loaded, "knockback_follow", knockback_follow, false );

    optional( jo, was_loaded, "aoe", aoe, "" );
    optional( jo, was_loaded, "flags", flags, auto_flags_reader<> {} );
    optional( jo, was_loaded, "tech_effects", tech_effects, tech_effect_reader{} );

    for( JsonValue jv : jo.get_array( "eocs" ) ) {
        eocs.push_back( effect_on_conditions::load_inline_eoc( jv, src ) );
    }

    if( jo.has_member( "condition" ) ) {
        read_condition( jo, "condition", condition, false );
        optional( jo, was_loaded, "condition_desc", condition_desc );
        has_condition = true;
    }

    optional( jo, was_loaded, "attack_vectors", attack_vectors );
    reqs.load( jo, src );
    bonuses.load( jo );
}

void ma_technique::verify_ma_techniques()
{
    ma_techniques.check();
}

void ma_technique::check() const
{
    if( attack_vectors.empty() && !dummy && !defensive && !grab_break && !miss_recovery ) {
        debugmsg( "MA technique %s is missing an attack vector", id.c_str() );
    }
}

// Not implemented on purpose (martialart objects have no integer id)
// int_id<T> string_id<mabuff>::id() const;

/** @relates string_id */
template<>
const ma_technique &string_id<ma_technique>::obj() const
{
    return ma_techniques.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<ma_technique>::is_valid() const
{
    return ma_techniques.is_valid( *this );
}

void ma_buff::load( const JsonObject &jo, std::string_view src )
{
    mandatory( jo, was_loaded, "name", name );
    mandatory( jo, was_loaded, "description", description );

    optional( jo, was_loaded, "buff_duration", buff_duration, 2_turns );
    optional( jo, was_loaded, "max_stacks", max_stacks, 1 );
    optional( jo, was_loaded, "persists", persists, false );

    optional( jo, was_loaded, "bonus_dodges", dodges_bonus, 0 );
    optional( jo, was_loaded, "bonus_blocks", blocks_bonus, 0 );

    optional( jo, was_loaded, "quiet", quiet, false );
    optional( jo, was_loaded, "stealthy", stealthy, false );
    optional( jo, was_loaded, "melee_bash_damage_cap_bonus", melee_bash_damage_cap_bonus, false );

    optional( jo, was_loaded, "flags", flags );

    reqs.load( jo, src );
    bonuses.load( jo );
}

// Not implemented on purpose (martialart objects have no integer id)
// int_id<T> string_id<mabuff>::id() const;

/** @relates string_id */
template<>
const ma_buff &string_id<ma_buff>::obj() const
{
    return ma_buffs.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<ma_buff>::is_valid() const
{
    return ma_buffs.is_valid( *this );
}

void load_martial_art( const JsonObject &jo, const std::string &src )
{
    martialarts.load( jo, src );
}

class ma_buff_reader : public generic_typed_reader<ma_buff_reader>
{
    public:
        mabuff_id get_next( const JsonValue &jin ) const {
            if( jin.test_string() ) {
                return mabuff_id( jin.get_string() );
            }
            JsonObject jsobj = jin.get_object();
            ma_buffs.load( jsobj, "" );
            return mabuff_id( jsobj.get_string( "id" ) );
        }
};

void martialart::load( const JsonObject &jo, std::string_view src )
{
    mandatory( jo, was_loaded, "name", name );
    mandatory( jo, was_loaded, "description", description );
    mandatory( jo, was_loaded, "initiate", initiate );
    for( JsonArray skillArray : jo.get_array( "autolearn" ) ) {
        std::string skill_name = skillArray.get_string( 0 );
        int skill_level = skillArray.get_int( 1 );
        autolearn_skills.emplace_back( skill_name, skill_level );
    }
    optional( jo, was_loaded, "priority", priority, 0 );
    optional( jo, was_loaded, "primary_skill", primary_skill, skill_unarmed );
    optional( jo, was_loaded, "learn_difficulty", learn_difficulty );
    optional( jo, was_loaded, "teachable", teachable, true );

    optional( jo, was_loaded, "static_buffs", static_buffs, ma_buff_reader{} );
    optional( jo, was_loaded, "onmove_buffs", onmove_buffs, ma_buff_reader{} );
    optional( jo, was_loaded, "onpause_buffs", onpause_buffs, ma_buff_reader{} );
    optional( jo, was_loaded, "onhit_buffs", onhit_buffs, ma_buff_reader{} );
    optional( jo, was_loaded, "onattack_buffs", onattack_buffs, ma_buff_reader{} );
    optional( jo, was_loaded, "ondodge_buffs", ondodge_buffs, ma_buff_reader{} );
    optional( jo, was_loaded, "onblock_buffs", onblock_buffs, ma_buff_reader{} );
    optional( jo, was_loaded, "ongethit_buffs", ongethit_buffs, ma_buff_reader{} );
    optional( jo, was_loaded, "onmiss_buffs", onmiss_buffs, ma_buff_reader{} );
    optional( jo, was_loaded, "oncrit_buffs", oncrit_buffs, ma_buff_reader{} );
    optional( jo, was_loaded, "onkill_buffs", onkill_buffs, ma_buff_reader{} );

    for( JsonValue jv : jo.get_array( "static_eocs" ) ) {
        static_eocs.push_back( effect_on_conditions::load_inline_eoc( jv, src ) );
    }
    for( JsonValue jv : jo.get_array( "onmove_eocs" ) ) {
        onmove_eocs.push_back( effect_on_conditions::load_inline_eoc( jv, src ) );
    }
    for( JsonValue jv : jo.get_array( "onpause_eocs" ) ) {
        onpause_eocs.push_back( effect_on_conditions::load_inline_eoc( jv, src ) );
    }
    for( JsonValue jv : jo.get_array( "onhit_eocs" ) ) {
        onhit_eocs.push_back( effect_on_conditions::load_inline_eoc( jv, src ) );
    }
    for( JsonValue jv : jo.get_array( "onattack_eocs" ) ) {
        onattack_eocs.push_back( effect_on_conditions::load_inline_eoc( jv, src ) );
    }
    for( JsonValue jv : jo.get_array( "ondodge_eocs" ) ) {
        ondodge_eocs.push_back( effect_on_conditions::load_inline_eoc( jv, src ) );
    }
    for( JsonValue jv : jo.get_array( "onblock_eocs" ) ) {
        onblock_eocs.push_back( effect_on_conditions::load_inline_eoc( jv, src ) );
    }
    for( JsonValue jv : jo.get_array( "ongethit_eocs" ) ) {
        ongethit_eocs.push_back( effect_on_conditions::load_inline_eoc( jv, src ) );
    }
    for( JsonValue jv : jo.get_array( "onmiss_eocs" ) ) {
        onmiss_eocs.push_back( effect_on_conditions::load_inline_eoc( jv, src ) );
    }
    for( JsonValue jv : jo.get_array( "oncrit_eocs" ) ) {
        oncrit_eocs.push_back( effect_on_conditions::load_inline_eoc( jv, src ) );
    }
    for( JsonValue jv : jo.get_array( "onkill_eocs" ) ) {
        onkill_eocs.push_back( effect_on_conditions::load_inline_eoc( jv, src ) );
    }

    optional( jo, was_loaded, "techniques", techniques, string_id_reader<::ma_technique> {} );
    optional( jo, was_loaded, "weapons", weapons, string_id_reader<::itype> {} );
    optional( jo, was_loaded, "weapon_category", weapon_category, auto_flags_reader<weapon_category_id> {} );

    optional( jo, was_loaded, "strictly_melee", strictly_melee, false );
    optional( jo, was_loaded, "strictly_unarmed", strictly_unarmed, false );
    optional( jo, was_loaded, "allow_all_weapons", allow_all_weapons, false );
    optional( jo, was_loaded, "force_unarmed", force_unarmed, false );
    optional( jo, was_loaded, "prevent_weapon_blocking", prevent_weapon_blocking, false );

    optional( jo, was_loaded, "leg_block", leg_block, 99 );
    optional( jo, was_loaded, "arm_block", arm_block, 99 );
    optional( jo, was_loaded, "nonstandard_block", nonstandard_block, 99 );

    optional( jo, was_loaded, "arm_block_with_bio_armor_arms", arm_block_with_bio_armor_arms, false );
    optional( jo, was_loaded, "leg_block_with_bio_armor_legs", leg_block_with_bio_armor_legs, false );
}

// Not implemented on purpose (martialart objects have no integer id)
// int_id<T> string_id<martialart>::id() const;

/** @relates string_id */
template<>
const martialart &string_id<martialart>::obj() const
{
    return martialarts.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<martialart>::is_valid() const
{
    return martialarts.is_valid( *this );
}

std::vector<matype_id> all_martialart_types()
{
    std::vector<matype_id> result;
    std::vector<martialart> martial_arts = martialarts.get_all();
    result.reserve( martial_arts.size() );
    for( const martialart &ma : martial_arts ) {
        result.push_back( ma.id );
    }
    return result;
}

std::vector<matype_id> autolearn_martialart_types()
{
    std::vector<matype_id> result;
    for( const martialart &ma : martialarts.get_all() ) {
        if( ma.autolearn_skills.empty() ) {
            continue;
        }
        result.push_back( ma.id );
    }
    return result;
}

static void check( const ma_requirements &req, const std::string &display_text )
{
    for( const mabuff_id &r : req.req_buffs_all ) {
        if( !r.is_valid() ) {
            debugmsg( "ma buff %s of %s does not exist", r.c_str(), display_text );
        }
    }

    for( const mabuff_id &r : req.req_buffs_any ) {
        if( !r.is_valid() ) {
            debugmsg( "ma buff %s of %s does not exist", r.c_str(), display_text );
        }
    }

    for( const mabuff_id &r : req.forbid_buffs_all ) {
        if( !r.is_valid() ) {
            debugmsg( "ma buff %s of %s does not exist", r.c_str(), display_text );
        }
    }

    for( const mabuff_id &r : req.forbid_buffs_any ) {
        if( !r.is_valid() ) {
            debugmsg( "ma buff %s of %s does not exist", r.c_str(), display_text );
        }
    }
}

void check_martialarts()
{
    for( const martialart &ma : martialarts.get_all() ) {
        for( auto technique = ma.techniques.cbegin();
             technique != ma.techniques.cend(); ++technique ) {
            if( !technique->is_valid() ) {
                debugmsg( "Technique with id %s in style %s doesn't exist.",
                          technique->c_str(), ma.name );
            }
        }
        for( auto weapon = ma.weapons.cbegin();
             weapon != ma.weapons.cend(); ++weapon ) {
            if( !item::type_is_defined( *weapon ) ) {
                debugmsg( "Weapon %s in style %s doesn't exist.",
                          weapon->c_str(), ma.name );
            }
        }
    }
    for( const ma_technique &t : ma_techniques.get_all() ) {
        ::check( t.reqs, string_format( "technique %s", t.id.c_str() ) );
    }
    for( const ma_buff &b : ma_buffs.get_all() ) {
        ::check( b.reqs, string_format( "buff %s", b.id.c_str() ) );
    }
}

/**
 * This is a wrapper class to get access to the protected members of effect_type, it creates
 * the @ref effect_type that is used to store a @ref ma_buff in the creatures effect map.
 * Note: this class must not contain any new members, it will be converted to a plain
 * effect_type later and that would slice the new members of.
 */
class ma_buff_effect_type : public effect_type
{
    public:
        explicit ma_buff_effect_type( const ma_buff &buff ) {
            id = buff.get_effect_id();
            max_intensity = buff.max_stacks;
            // add_effect add the duration to an existing effect, but it must never be
            // above buff_duration, this keeps the old ma_buff behavior
            max_duration = buff.buff_duration;
            dur_add_perc = 100;
            show_intensity = true;
            // each add_effect call increases the intensity by 1
            int_add_val = 1;
            // effect intensity increases by -1 each turn.
            int_decay_step = -1;
            int_decay_tick = 1;
            int_dur_factor = 0_turns;
            int_decay_remove = false;
            name.push_back( buff.name );
            desc.push_back( buff.description );
            apply_msgs.emplace_back( no_translation( "" ), m_good );
        }
};

void finalize_martial_arts()
{
    // This adds an effect type for each ma_buff, so we can later refer to it and don't need a
    // redundant definition of those effects in json.
    for( const ma_buff &buff : ma_buffs.get_all() ) {
        const ma_buff_effect_type new_eff( buff );
        // Note the slicing here: new_eff is converted to a plain effect_type, but this doesn't
        // bother us because ma_buff_effect_type does not have any members that can be sliced.
        effect_type::register_ma_buff_effect( new_eff );
    }
    attack_vector_factory.finalize();
    for( const attack_vector &vector : attack_vector_factory.get_all() ) {
        // Check if this vector allows substitutions in the first place
        if( vector.strict_limb_definition ) {
            continue;
        }
        // Add similar parts
        // The vector needs both a limb and a contact area, so we can substitute safely
        std::vector<bodypart_str_id> similar_bp;
        for( const bodypart_str_id &bp : vector.limbs ) {
            for( const bodypart_str_id &similar : bp->similar_bodyparts ) {
                similar_bp.emplace_back( similar );
            }
        }
        const_cast<attack_vector &>( vector ).limbs.insert( vector.limbs.end(), similar_bp.begin(),
                similar_bp.end() );

        std::vector<sub_bodypart_str_id> similar_sbp;
        for( const sub_bodypart_str_id &sbp : vector.contact_area ) {
            for( const sub_bodypart_str_id &similar : sbp->similar_bodyparts ) {
                similar_sbp.emplace_back( similar );
            }
        }

        const_cast<attack_vector &>( vector ).contact_area.insert( vector.contact_area.end(),
                similar_sbp.begin(), similar_sbp.end() );
    }
}

std::string martialart_difficulty( const matype_id &mstyle )
{
    std::string diff;
    if( mstyle->learn_difficulty <= 2 ) {
        diff = _( "easy" );
    } else if( mstyle->learn_difficulty <= 4 ) {
        diff = _( "moderately hard" );
    } else if( mstyle->learn_difficulty <= 6 ) {
        diff = _( "hard" );
    } else if( mstyle->learn_difficulty <= 8 ) {
        diff = _( "very hard" );
    } else {
        diff = _( "extremely hard" );
    }
    return diff;
}

void clear_techniques_and_martial_arts()
{
    martialarts.reset();
    ma_buffs.reset();
    ma_techniques.reset();
}

bool ma_requirements::buff_requirements_satisfied( const Character &u ) const
{
    const auto having_buff = [&u]( const mabuff_id & buff_id ) {
        return u.has_mabuff( buff_id );
    };

    if( std::any_of( forbid_buffs_any.begin(), forbid_buffs_any.end(), having_buff ) ) {
        return false;
    }

    if( !forbid_buffs_all.empty() ) {
        if( std::all_of( forbid_buffs_all.begin(), forbid_buffs_all.end(), having_buff ) ) {
            return false;
        }
    }

    return std::all_of( req_buffs_all.begin(), req_buffs_all.end(), having_buff ) &&
           ( req_buffs_any.empty() || std::any_of( req_buffs_any.begin(), req_buffs_any.end(), having_buff ) );
}

bool ma_requirements::is_valid_character( const Character &u ) const
{
    if( !buff_requirements_satisfied( u ) ) {
        add_msg_debug( debugmode::DF_MELEE, "Buff requirements not satisfied, attack discarded" );
        return false;
    }

    //A technique is valid if it applies to unarmed strikes, if it applies generally
    //to all weapons (such as Ninjutsu sneak attacks or innate weapon techniques like RAPID)
    //or if the weapon is flagged as being compatible with the style. Some techniques have
    //further restrictions on required weapon properties (is_valid_weapon).
    bool cqb = u.has_active_bionic( bio_cqb );
    // There are 4 different cases of "armedness":
    // Truly unarmed, unarmed weapon, style-allowed weapon, generic weapon
    const item_location weapon = u.get_wielded_item();
    bool melee_style = u.martial_arts_data->selected_strictly_melee();
    bool is_armed = u.is_armed();
    bool forced_unarmed = u.martial_arts_data->selected_force_unarmed();
    bool weapon_ok = melee_allowed && weapon && is_valid_weapon( *weapon );
    bool style_weapon = weapon && u.martial_arts_data->selected_has_weapon( weapon->typeId() );
    bool all_weapons = u.martial_arts_data->selected_allow_all_weapons();

    bool melee_ok = weapon_ok && ( style_weapon || all_weapons );

    bool valid_unarmed = !melee_style && unarmed_allowed && !is_armed;
    bool valid_melee = !strictly_unarmed && ( forced_unarmed || melee_ok );

    if( !valid_unarmed && !valid_melee ) {
        add_msg_debug( debugmode::DF_MELEE, "Weapon/technique conflict, attack discarded" );
        return false;
    }

    for( const auto &pr : min_skill ) {
        if( ( cqb ? 5 : static_cast<int>( u.get_skill_level( pr.first ) ) ) < pr.second ) {
            add_msg_debug( debugmode::DF_MELEE, "Skill level requirement %d not satisfied, attack discarded",
                           pr.second );
            return false;
        }
    }

    if( !req_char_flags.empty() ) {
        bool has_flag = false;
        for( const json_character_flag &flag : req_char_flags ) {
            if( u.has_flag( flag ) ) {
                has_flag = true;
            }
        }
        if( !has_flag ) {
            add_msg_debug( debugmode::DF_MELEE, "Required flag(any) not found, attack discarded" );
            return false;
        }
    }

    for( const json_character_flag &flag : req_char_flags_all ) {
        if( !u.has_flag( flag ) ) {
            add_msg_debug( debugmode::DF_MELEE, "Required flags(all) not found, attack discarded" );
            return false;
        }
    }

    if( !forbidden_char_flags.empty() ) {
        bool has_flag = false;
        for( const json_character_flag &flag : forbidden_char_flags ) {
            if( u.has_flag( flag ) ) {
                has_flag = true;
            }
        }
        if( has_flag ) {
            add_msg_debug( debugmode::DF_MELEE, "Forbidden flag found, attack discarded" );
            return false;
        }
    }

    if( !weapon_categories_allowed.empty() ) {
        bool valid_weap_cat = false;
        for( const weapon_category_id &w_cat : weapon_categories_allowed ) {
            if( u.used_weapon() && u.used_weapon()->typeId()->weapon_category.count( w_cat ) > 0 ) {
                valid_weap_cat = true;
            }
        }
        if( !valid_weap_cat ) {
            add_msg_debug( debugmode::DF_MELEE, "Not using weapon from a valid category, attack discarded" );
            return false;
        }
    }

    return true;
}

bool ma_requirements::is_valid_weapon( const item &i ) const
{
    for( const flag_id &flag : req_flags ) {
        if( !i.has_flag( flag ) ) {
            return false;
        }
    }
    for( const auto &pr : min_damage ) {
        if( i.damage_melee( pr.first ) < pr.second ) {
            return false;
        }
    }

    return true;
}

static std::string required_skill_as_string( const skill_id &skill, const int required_skill,
        const int player_skill )
{
    std::string difficulty_tag;
    if( required_skill <= player_skill ) {
        difficulty_tag = "good";
    } else {
        difficulty_tag = "bad";
    }
    return string_format( "<info>%s</info> <%s>(%d/%d)</%s>", skill->name(), difficulty_tag,
                          player_skill, required_skill, difficulty_tag );
}

std::string ma_requirements::get_description( bool buff ) const
{
    std::string dump;

    if( std::any_of( min_skill.begin(), min_skill.end(), []( const std::pair<skill_id, int> &pr ) {
    return pr.second > 0;
} ) ) {
        dump += n_gettext( "<bold>Skill required: </bold>",
                           "<bold>Skills required: </bold>", min_skill.size() );

        dump += enumerate_as_string( min_skill.begin(),
        min_skill.end(), []( const std::pair<skill_id, int>  &pr ) {
            Character &u = get_player_character();
            int player_skill = u.get_skill_level( skill_id( pr.first ) );
            if( u.has_active_bionic( bio_cqb ) ) {
                player_skill = BIO_CQB_LEVEL;
            }
            return required_skill_as_string( pr.first, pr.second, player_skill );
        }, enumeration_conjunction::none ) + "\n";
    }

    if( std::any_of( min_damage.begin(),
    min_damage.end(), []( const std::pair<const damage_type_id, int>  &pr ) {
    return pr.second > 0;
} ) ) {
        dump += n_gettext( "<bold>Damage type required: </bold>",
                           "<bold>Damage types required: </bold>", min_damage.size() );

        dump += enumerate_as_string( min_damage.begin(),
        min_damage.end(), []( const std::pair<const damage_type_id, int>  &pr ) {
            return string_format( _( "%s: <stat>%d</stat>" ), pr.first->name.translated(), pr.second );
        }, enumeration_conjunction::none ) + "\n";
    }

    if( !weapon_categories_allowed.empty() ) {
        dump += n_gettext( "<bold>Weapon category required: </bold>",
                           "<bold>Weapon categories required: </bold>", weapon_categories_allowed.size() );
        dump += enumerate_as_string( weapon_categories_allowed.begin(),
        weapon_categories_allowed.end(), []( const weapon_category_id & w_cat ) {
            if( !w_cat.is_valid() ) {
                return w_cat.str();
            }
            return w_cat->name().translated();
        } ) + "\n";
    }

    if( !req_buffs_all.empty() ) {
        dump += _( "<bold>Requires (all):</bold> " );

        dump += enumerate_as_string( req_buffs_all.begin(),
        req_buffs_all.end(), []( const mabuff_id & bid ) {
            return bid->name.translated();
        }, enumeration_conjunction::none ) + "\n";
    }

    if( !req_buffs_any.empty() ) {
        dump += _( "<bold>Requires (any):</bold> " );

        dump += enumerate_as_string( req_buffs_any.begin(),
        req_buffs_any.end(), []( const mabuff_id & bid ) {
            return bid->name.translated();
        }, enumeration_conjunction::none ) + "\n";
    }

    if( !forbid_buffs_all.empty() ) {
        dump += _( "<bold>Forbidden (all):</bold> " );

        dump += enumerate_as_string( forbid_buffs_all.begin(),
        forbid_buffs_all.end(), []( const mabuff_id & bid ) {
            return bid->name.translated();
        }, enumeration_conjunction::none ) + "\n";
    }

    if( !forbid_buffs_any.empty() ) {
        dump += _( "<bold>Forbidden (any):</bold> " );

        dump += enumerate_as_string( forbid_buffs_any.begin(),
        forbid_buffs_any.end(), []( const mabuff_id & bid ) {
            return bid->name.translated();
        }, enumeration_conjunction::none ) + "\n";
    }

    const std::string type = buff ? _( "activate" ) : _( "be used" );

    if( unarmed_allowed && melee_allowed ) {
        dump += string_format( _( "* Can %s while <info>armed</info> or <info>unarmed</info>" ),
                               type ) + "\n";
    } else if( unarmed_allowed ) {
        dump += string_format( _( "* Can <info>only</info> %s while <info>unarmed</info>" ),
                               type ) + "\n";
    } else if( melee_allowed ) {
        dump += string_format( _( "* Can <info>only</info> %s while <info>armed</info>" ),
                               type ) + "\n";
    }

    if( wall_adjacent ) {
        dump += string_format( _( "* Can %s while <info>near</info> to a <info>wall</info>" ),
                               type ) + "\n";
    }

    return dump;
}

ma_technique::ma_technique()
{
    crit_tec = false;
    crit_ok = false;
    defensive = false;
    side_switch = false; // moves the target behind user
    dummy = false;

    down_dur = 0;
    stun_dur = 0;
    knockback_dist = 0;
    knockback_spread = 0; // adding randomness to knockback, like tec_throw
    knockback_follow = false; // player follows the knocked-back party into their former tile

    // offensive
    disarms = false; // like tec_disarm
    take_weapon = false; // disarms and equips weapon if hands are free
    dodge_counter = false; // like tec_grab
    block_counter = false; // like tec_counter

    // conditional
    wall_adjacent = false;    // only works near a wall

    needs_ammo = false;       // technique only works if the item is loaded with ammo

    miss_recovery = false; // reduces the total move cost of a miss by 50%, post stumble modifier
    grab_break = false; // allows grab_breaks, like tec_break
}

bool ma_technique::is_valid_character( const Character &u ) const
{
    return reqs.is_valid_character( u );
}

ma_buff::ma_buff()
    : buff_duration( 1_turns )
{
    max_stacks = 1; // total number of stacks this buff can have

    dodges_bonus = 0; // extra dodges, like karate
    blocks_bonus = 0; // extra blocks, like karate

}

efftype_id ma_buff::get_effect_id() const
{
    return efftype_id( std::string( "mabuff:" ) + id.str() );
}

const ma_buff *ma_buff::from_effect( const effect &eff )
{
    const std::string &id = eff.get_effect_type()->id.str();
    // Same as in get_effect_id!
    if( id.compare( 0, 7, "mabuff:" ) != 0 ) {
        return nullptr;
    }
    return &mabuff_id( id.substr( 7 ) ).obj();
}

void ma_buff::apply_buff( Character &u ) const
{
    u.add_effect( get_effect_id(), time_duration::from_turns( buff_duration ) );
}

bool ma_buff::is_valid_character( const Character &u ) const
{
    return reqs.is_valid_character( u );
}

void ma_buff::apply_character( Character &u ) const
{
    u.mod_num_dodges_bonus( dodges_bonus );
    u.set_num_blocks_bonus( u.get_num_blocks_bonus() + blocks_bonus );
}

int ma_buff::hit_bonus( const Character &u ) const
{
    return bonuses.get_flat( u, affected_stat::HIT );
}
int ma_buff::critical_hit_chance_bonus( const Character &u ) const
{
    return bonuses.get_flat( u, affected_stat::CRITICAL_HIT_CHANCE );
}
int ma_buff::dodge_bonus( const Character &u ) const
{
    return bonuses.get_flat( u, affected_stat::DODGE );
}
int ma_buff::block_bonus( const Character &u ) const
{
    return bonuses.get_flat( u, affected_stat::BLOCK );
}
int ma_buff::arpen_bonus( const Character &u, const damage_type_id &dt ) const
{
    return bonuses.get_flat( u, affected_stat::ARMOR_PENETRATION, dt );
}
int ma_buff::block_effectiveness_bonus( const Character &u ) const
{
    return bonuses.get_flat( u, affected_stat::BLOCK_EFFECTIVENESS );
}
int ma_buff::speed_bonus( const Character &u ) const
{
    return bonuses.get_flat( u, affected_stat::SPEED );
}
int ma_buff::armor_bonus( const Character &guy, const damage_type_id &dt ) const
{
    return bonuses.get_flat( guy, affected_stat::ARMOR, dt );
}
float ma_buff::damage_bonus( const Character &u, const damage_type_id &dt ) const
{
    return bonuses.get_flat( u, affected_stat::DAMAGE, dt );
}
float ma_buff::damage_mult( const Character &u, const damage_type_id &dt ) const
{
    return bonuses.get_mult( u, affected_stat::DAMAGE, dt );
}
bool ma_buff::is_melee_bash_damage_cap_bonus() const
{
    return melee_bash_damage_cap_bonus;
}
bool ma_buff::is_quiet() const
{
    return quiet;
}
bool ma_buff::is_stealthy() const
{
    return stealthy;
}

bool ma_buff::has_flag( const json_character_flag &flag ) const
{
    for( const json_character_flag &q : flags ) {
        if( q == flag ) {
            return true;
        }
    }
    return false;
}

bool ma_buff::can_melee() const
{
    return melee_allowed;
}

std::string ma_buff::get_description( bool passive ) const
{
    std::string dump;
    dump += string_format( _( "<bold>Buff technique:</bold> %s" ), name ) + "\n";

    std::string temp = bonuses.get_description();
    if( !temp.empty() ) {
        dump += std::string( npgettext( "martial arts buff desc", "<bold>Bonus:</bold> ",
                                        "<bold>Bonus/stack:</bold> ", max_stacks ) ) + "\n" + temp;
    }

    dump += reqs.get_description( true );

    if( max_stacks > 1 ) {
        dump += string_format( _( "* Will <info>stack</info> up to <stat>%d</stat> times" ),
                               max_stacks ) + "\n";
    }

    const int turns = to_turns<int>( buff_duration );
    if( !passive && turns ) {
        dump += string_format( n_gettext( "* Will <info>last</info> for <stat>%d turn</stat>",
                                          "* Will <info>last</info> for <stat>%d turns</stat>",
                                          turns ),
                               turns ) + "\n";
    }

    if( dodges_bonus > 0 ) {
        dump += string_format(
                    n_gettext( "* Will give a <good>+%s</good> bonus to <info>dodge</info> for the stack",
                               "* Will give a <good>+%s</good> bonus to <info>dodge</info> per stack",
                               max_stacks ),
                    dodges_bonus ) + "\n";
    } else if( dodges_bonus < 0 ) {
        dump += string_format(
                    n_gettext( "* Will give a <bad>%s</bad> penalty to <info>dodge</info> for the stack",
                               "* Will give a <bad>%s</bad> penalty to <info>dodge</info> per stack",
                               max_stacks ),
                    dodges_bonus ) + "\n";
    }

    if( blocks_bonus > 0 ) {
        dump += string_format(
                    n_gettext( "* Will give a <good>+%s</good> bonus to <info>block</info> for the stack",
                               "* Will give a <good>+%s</good> bonus to <info>block</info> per stack",
                               max_stacks ),
                    blocks_bonus ) + "\n";
    } else if( blocks_bonus < 0 ) {
        dump += string_format(
                    n_gettext( "* Will give a <bad>%s</bad> penalty to <info>block</info> for the stack",
                               "* Will give a <bad>%s</bad> penalty to <info>block</info> per stack",
                               max_stacks ),
                    blocks_bonus ) + "\n";
    }

    if( quiet ) {
        dump += _( "* Attacks will be completely <info>silent</info>" ) + std::string( "\n" );
    }

    if( stealthy ) {
        dump += _( "* Movement will make <info>less noise</info>" ) + std::string( "\n" );
    }

    return dump;
}

martialart::martialart()
{
    leg_block = -1;
    arm_block = -1;
}

// simultaneously check and add all buffs. this is so that buffs that have
// buff dependencies added by the same event trigger correctly
static void simultaneous_add( Character &u, const std::vector<mabuff_id> &buffs )
{
    std::vector<const ma_buff *> buffer; // hey get it because it's for buffs????
    for( const mabuff_id &buffid : buffs ) {
        const ma_buff &buff = buffid.obj();
        if( buff.is_valid_character( u ) ) {
            buffer.push_back( &buff );
        }
    }
    for( const ma_buff *&elem : buffer ) {
        elem->apply_buff( u );
    }
}

void martialart::remove_all_buffs( Character &u ) const
{
    // Remove static buffs
    for( const auto &elem : static_buffs ) {
        const efftype_id eff_id = elem->get_effect_id();
        if( u.has_effect( eff_id ) && !elem->persists ) {
            u.remove_effect( eff_id );
        }
    }

    // Remove onmove buffs
    for( const auto &elem : onmove_buffs ) {
        const efftype_id eff_id = elem->get_effect_id();
        if( u.has_effect( eff_id ) && !elem->persists ) {
            u.remove_effect( eff_id );
        }
    }

    // Remove onpause buffs
    for( const auto &elem : onpause_buffs ) {
        const efftype_id eff_id = elem->get_effect_id();
        if( u.has_effect( eff_id ) && !elem->persists ) {
            u.remove_effect( eff_id );
        }
    }

    // Remove onhit buffs
    for( const auto &elem : onhit_buffs ) {
        const efftype_id eff_id = elem->get_effect_id();
        if( u.has_effect( eff_id ) && !elem->persists ) {
            u.remove_effect( eff_id );
        }
    }

    // Remove onattack buffs
    for( const auto &elem : onattack_buffs ) {
        const efftype_id eff_id = elem->get_effect_id();
        if( u.has_effect( eff_id ) && !elem->persists ) {
            u.remove_effect( eff_id );
        }
    }

    // Remove ondodge buffs
    for( const auto &elem : ondodge_buffs ) {
        const efftype_id eff_id = elem->get_effect_id();
        if( u.has_effect( eff_id ) && !elem->persists ) {
            u.remove_effect( eff_id );
        }
    }

    // Remove onblock buffs
    for( const auto &elem : onblock_buffs ) {
        const efftype_id eff_id = elem->get_effect_id();
        if( u.has_effect( eff_id ) && !elem->persists ) {
            u.remove_effect( eff_id );
        }
    }

    // Remove ongethit buffs
    for( const auto &elem : ongethit_buffs ) {
        const efftype_id eff_id = elem->get_effect_id();
        if( u.has_effect( eff_id ) && !elem->persists ) {
            u.remove_effect( eff_id );
        }
    }

    // Remove onmiss buffs
    for( const auto &elem : onmiss_buffs ) {
        const efftype_id eff_id = elem->get_effect_id();
        if( u.has_effect( eff_id ) && !elem->persists ) {
            u.remove_effect( eff_id );
        }
    }

    // Remove oncrit buffs
    for( const auto &elem : oncrit_buffs ) {
        const efftype_id eff_id = elem->get_effect_id();
        if( u.has_effect( eff_id ) && !elem->persists ) {
            u.remove_effect( eff_id );
        }
    }

    // Remove onkill buffs
    for( const auto &elem : onkill_buffs ) {
        const efftype_id eff_id = elem->get_effect_id();
        if( u.has_effect( eff_id ) && !elem->persists ) {
            u.remove_effect( eff_id );
        }
    }
}

void martialart::apply_static_buffs( Character &u ) const
{
    simultaneous_add( u, static_buffs );
}

void martialart::apply_onmove_buffs( Character &u ) const
{
    simultaneous_add( u, onmove_buffs );
}

void martialart::apply_onpause_buffs( Character &u ) const
{
    simultaneous_add( u, onpause_buffs );
}

void martialart::apply_onhit_buffs( Character &u ) const
{
    simultaneous_add( u, onhit_buffs );
}

void martialart::apply_onattack_buffs( Character &u ) const
{
    simultaneous_add( u, onattack_buffs );
}

void martialart::apply_ondodge_buffs( Character &u ) const
{
    simultaneous_add( u, ondodge_buffs );
}

void martialart::apply_onblock_buffs( Character &u ) const
{
    simultaneous_add( u, onblock_buffs );
}

void martialart::apply_ongethit_buffs( Character &u ) const
{
    simultaneous_add( u, ongethit_buffs );
}

void martialart::apply_onmiss_buffs( Character &u ) const
{
    simultaneous_add( u, onmiss_buffs );
}

void martialart::apply_oncrit_buffs( Character &u ) const
{
    simultaneous_add( u, oncrit_buffs );
}

void martialart::apply_onkill_buffs( Character &u ) const
{
    simultaneous_add( u, onkill_buffs );
}

void martialart::activate_eocs( Character &u,
                                const std::vector<effect_on_condition_id> &eocs ) const
{
    for( const effect_on_condition_id &eoc : eocs ) {
        dialogue d( get_talker_for( u ), nullptr );
        if( eoc->type == eoc_type::ACTIVATION ) {
            eoc->activate( d );
        } else {
            debugmsg( "Must use an activation eoc for a martial art activation.  If you don't want the effect_on_condition to happen on its own (without the martial art being activated), remove the recurrence min and max.  Otherwise, create a non-recurring effect_on_condition for this martial art with its condition and effects, then have a recurring one queue it." );
        }
    }
}

void martialart::apply_static_eocs( Character &u ) const
{
    activate_eocs( u, static_eocs );
}

void martialart::apply_onmove_eocs( Character &u ) const
{
    activate_eocs( u, onmove_eocs );
}

void martialart::apply_onpause_eocs( Character &u ) const
{
    activate_eocs( u, onpause_eocs );
}

void martialart::apply_onhit_eocs( Character &u ) const
{
    activate_eocs( u, onhit_eocs );
}

void martialart::apply_onattack_eocs( Character &u ) const
{
    activate_eocs( u, onattack_eocs );
}

void martialart::apply_ondodge_eocs( Character &u ) const
{
    activate_eocs( u, ondodge_eocs );
}

void martialart::apply_onblock_eocs( Character &u ) const
{
    activate_eocs( u, onblock_eocs );
}

void martialart::apply_ongethit_eocs( Character &u ) const
{
    activate_eocs( u, ongethit_eocs );
}

void martialart::apply_onmiss_eocs( Character &u ) const
{
    activate_eocs( u, onmiss_eocs );
}

void martialart::apply_oncrit_eocs( Character &u ) const
{
    activate_eocs( u, oncrit_eocs );
}

void martialart::apply_onkill_eocs( Character &u ) const
{
    activate_eocs( u, onkill_eocs );
}

bool martialart::has_technique( const Character &u, const matec_id &tec_id ) const
{
    for( const matec_id &elem : techniques ) {
        const ma_technique &tec = elem.obj();
        if( tec.is_valid_character( u ) && tec.id == tec_id ) {
            return true;
        }
    }
    return false;
}

bool martialart::has_weapon( const itype_id &itt ) const
{
    return weapons.count( itt ) > 0 ||
           std::any_of( itt->weapon_category.begin(), itt->weapon_category.end(),
    [&]( const weapon_category_id & weap ) {
        return weapon_category.count( weap ) > 0;
    } );
}

bool martialart::weapon_valid( const item_location &it ) const
{
    if( allow_all_weapons ) {
        return true;
    }

    if( !it && !strictly_melee ) {
        return true;
    }

    if( it && has_weapon( it->typeId() ) ) {
        return true;
    }

    return false;
}

std::string martialart::get_initiate_avatar_message() const
{
    return initiate[0].translated();
}

std::string martialart::get_initiate_npc_message() const
{
    return initiate[1].translated();
}
// Player stuff

// technique
std::vector<matec_id> character_martial_arts::get_all_techniques( const item_location &weap,
        const Character &u ) const
{
    std::vector<matec_id> tecs;
    const martialart &style = style_selected.obj();

    // Grab individual item techniques if the style allows them
    if( weap && !style.force_unarmed ) {
        const auto &weapon_techs = weap->get_techniques();
        tecs.insert( tecs.end(), weapon_techs.begin(), weapon_techs.end() );
    }
    // If we have any items that also provide techniques
    const std::vector<const item *> tech_providing_items = u.cache_get_items_with(
                json_flag_PROVIDES_TECHNIQUES );
    for( const item *it : tech_providing_items ) {
        const std::set<matec_id> &item_techs = it->get_techniques();
        tecs.insert( tecs.end(), item_techs.begin(), item_techs.end() );
    }
    // and martial art techniques
    tecs.insert( tecs.end(), style.techniques.begin(), style.techniques.end() );
    // And limb techniques
    const auto &limb_techs = u.get_limb_techs();
    tecs.insert( tecs.end(), limb_techs.begin(), limb_techs.end() );

    return tecs;
}

// defensive technique-related

static ma_technique get_valid_technique( const Character &owner, bool ma_technique::*  purpose )
{
    const auto &ma_data = owner.martial_arts_data;

    for( const matec_id &candidate_id : ma_data->get_all_techniques( owner.get_wielded_item(),
            owner ) ) {
        ma_technique candidate = candidate_id.obj();

        if( candidate.*purpose && candidate.is_valid_character( owner ) ) {
            return candidate;
        }
    }

    return tec_none.obj();
}

ma_technique character_martial_arts::get_grab_break( const Character &owner ) const
{
    return get_valid_technique( owner, &ma_technique::grab_break );
}

ma_technique character_martial_arts::get_miss_recovery( const Character &owner ) const
{
    return get_valid_technique( owner, &ma_technique::miss_recovery );
}

std::optional<std::pair<attack_vector_id, sub_bodypart_str_id>>
        character_martial_arts::choose_attack_vector( const Character &user,
                const matec_id &tech ) const
{
    // Use the simple weighted list to handle picking semi-randomly
    attack_vector_id ret;
    weighted_float_list<attack_vector_id> list;
    std::pair<attack_vector_id, sub_bodypart_str_id> return_set;
    std::vector<std::pair<attack_vector_id, sub_bodypart_str_id>> storage;
    const std::vector<bodypart_id> anat = user.get_all_body_parts();
    const bool armed = user.is_armed();
    martialart ma = style_selected.obj();
    bool valid_weapon = ma.weapon_valid( user.get_wielded_item() );
    for( const attack_vector_id &vec : tech.obj().attack_vectors ) {
        add_msg_debug( debugmode::DF_MELEE, "Evaluating vector %s for tech %s", vec.c_str(), tech.c_str() );
        float weight = 0.0f;
        // Early break for armed vectors
        if( vec->weapon && armed && valid_weapon ) {
            item *weapon = user.get_wielded_item().get_item();
            // Calculate weapon damage for weighting
            // Store a dummy sublimb to show we're attacking with a weapon
            weight = weapon->base_damage_melee().total_damage();
            list.add_or_replace( vec, weight );
            storage.emplace_back( vec, sub_bodypart_str_id::NULL_ID() );
            add_msg_debug( debugmode::DF_MELEE, "Weapon %s eligable for attack vector %s with weight %.1f",
                           weapon->display_name(),
                           vec.c_str(), weight );
            continue;
        }
        // Check if we have the required limbs
        bool reqs = true;
        for( const std::pair<body_part_type::type, int> &req : vec->limb_req ) {
            int count = 0;
            for( const bodypart_id &bp : user.get_all_body_parts_of_type( req.first ) ) {
                if( user.get_part_hp_cur( bp ) > bp->health_limit ) {
                    count++;
                }
            }
            if( count < req.second ) {
                add_msg_debug( debugmode::DF_MELEE,
                               "Limb type requirements: %d matching limbs found from %d req, vector discarded", count,
                               req.second );
                reqs = false;
                break;
            }
        }
        if( !reqs ) {
            continue;
        }

        // Smilar bodyparts get appended to the vector limb list in the finalization step
        // So we just need to check if we have a limb, a contact area sublimb and tally up the damages
        std::vector<std::pair<sub_bodypart_str_id, float>> calc_vector;
        for( const bodypart_str_id &bp : vec->limbs ) {
            //const bodypart_str_id &bp = bp_id.id();
            if( std::find( anat.begin(), anat.end(), bp.id() ) != anat.end() ) {
                add_msg_debug( debugmode::DF_MELEE, "Evaluating limb %s for vector %s", bp->name, vec.c_str() );
                // Filter on limb flags early
                bool allowed = true;
                for( const json_character_flag &req : vec->required_limb_flags ) {
                    if( !bp->has_flag( req ) ) {
                        add_msg_debug( debugmode::DF_MELEE, "Required limb flag %s not found on limb %s, limb discarded",
                                       req.c_str(),
                                       bp->name );
                        allowed = false;
                        break;
                    }
                }
                for( const json_character_flag &forb : vec->forbidden_limb_flags ) {
                    if( bp->has_flag( forb ) ) {
                        add_msg_debug( debugmode::DF_MELEE, "Forbidden limb flag %s found on limb %s, limb discarded",
                                       forb.c_str(),
                                       bp->name );
                        allowed = false;
                        break;
                    }
                }
                if( !allowed ) {
                    continue;
                }

                // TODO: move this from being a special case to the default
                int bp_hp_cur = bp->main_part == bp ? user.get_part_hp_cur( bp ) : user.get_part_hp_cur(
                                    bp->main_part );
                int bp_hp_max = bp->main_part == bp ? user.get_part_hp_max( bp ) : user.get_part_hp_max(
                                    bp->main_part );
                if( ( 100 * bp_hp_cur / bp_hp_max ) > vec->bp_hp_limit &&
                    user.get_part_encumbrance( bp ) < vec->encumbrance_limit ) {
                    sub_bodypart_str_id current_contact;
                    for( const sub_bodypart_str_id &sbp : bp->sub_parts ) {
                        if( std::find( vec->contact_area.begin(), vec->contact_area.end(),
                                       sbp ) != vec->contact_area.end() ) {
                            current_contact = sbp;
                            break;
                        }
                    }

                    float unarmed_damage = calculate_vector_damage( user, vec, current_contact ).total_damage();
                    if( unarmed_damage <= 0.0f ) {
                        // Give extra/damage-less vectors a base chance to be chosen
                        unarmed_damage = 1.0f;
                    }
                    calc_vector.emplace_back( current_contact, unarmed_damage );
                    add_msg_debug( debugmode::DF_MELEE,
                                   "Bodypart %s eligable for attack vector %s weight %.1f (contact area %s)",
                                   bp.c_str(),
                                   vec.c_str(), unarmed_damage, current_contact->name );
                }
            }
        }
        if( calc_vector.empty() ) {
            add_msg_debug( debugmode::DF_MELEE, "Vector %s found no eligable bodyparts, discarding",
                           vec.c_str() );
            continue;
        }
        // Sort our calc_vector of sublimb/damage pairs
        std::sort( calc_vector.begin(),
                   calc_vector.end(), []( const std::pair<sub_bodypart_str_id, float> &a,
        const std::pair<sub_bodypart_str_id, float> &b ) {
            return a.second < b.second;
        } );
        list.add( vec, calc_vector.rbegin()->second );
        storage.emplace_back( vec, calc_vector.rbegin()->first );
        add_msg_debug( debugmode::DF_MELEE,
                       "Chose contact sublimb %s for vector %s with weight %.1f; %d stored vectors",
                       calc_vector.rbegin()->first->name, vec.c_str(), calc_vector.rbegin()->second, storage.size() );
    }
    if( !list.empty() ) {
        ret = *list.pick();
        add_msg_debug( debugmode::DF_MELEE, "Picked vector %s for technique %s", ret.c_str(),
                       tech.c_str() );
        // Now find the contact data matching the winning vector
        for( auto &iterate : storage ) {
            if( iterate.first == ret ) {
                return_set = iterate;
                break;
            }
        }
        return return_set;
    }
    return std::nullopt;
}

damage_instance character_martial_arts::calculate_vector_damage( const Character &user,
        const attack_vector_id &vec, const sub_bodypart_str_id &contact_area ) const
{
    // Calculate unarmed damage for the given sublimb(s)
    damage_instance ret;
    // If we got this far the limb is not overencumbered
    // But do filter on the flag to bring it in line with the actual damage calc
    if( !user.natural_attack_restricted_on( contact_area->parent ) ) {
        ret.add( contact_area->parent->unarmed_damage_instance() );
        add_msg_debug( debugmode::DF_MELEE,
                       "Unarmed damage of bodypart %s %.1f", contact_area->parent->name,
                       ret.total_damage() );
    }
    if( !user.natural_attack_restricted_on( contact_area ) ) {
        ret.add( contact_area->unarmed_damage );
        add_msg_debug( debugmode::DF_MELEE,
                       "Unarmed damage of subpart %s %.1f, total damage %.1f", contact_area->parent->name,
                       contact_area->unarmed_damage.total_damage(),
                       ret.total_damage() );
    }
    // Add any bonus from worn armor if the vector allows it
    if( vec->armor_bonus ) {
        outfit current_worn = user.worn;
        item *unarmed_weapon = current_worn.current_unarmed_weapon( contact_area );
        if( unarmed_weapon != nullptr ) {
            ret.add( unarmed_weapon->base_damage_melee() );
            add_msg_debug( debugmode::DF_MELEE,
                           "Unarmed weapon %s found, melee damage %.1f, new total damage %.1f",
                           unarmed_weapon->display_name(), unarmed_weapon->base_damage_melee().total_damage(),
                           ret.total_damage() );
        }
    }
    return ret;
}

bool character_martial_arts::can_leg_block( const Character &owner ) const
{
    const martialart &ma = style_selected.obj();
    ///\EFFECT_UNARMED increases ability to perform leg block
    const int unarmed_skill = owner.has_active_bionic( bio_cqb ) ? 5 : owner.get_skill_level(
                                  skill_unarmed );

    // Before we check our legs, can you block at all?
    const bool block_with_skill = unarmed_skill >= ma.leg_block;
    const bool block_with_bio_armor = ma.leg_block_with_bio_armor_legs &&
                                      owner.has_bionic( bio_armor_legs );
    if( !( block_with_skill || block_with_bio_armor ) ) {
        return false;
    }

    // Success conditions.
    // Do we have boring human anatomy? Use the basic calculation
    // Legs are harder to block with, so the score thresholds stay the same
    if( !owner.has_flag( json_flag_NONSTANDARD_BLOCK ) ) {
        return owner.get_limb_score( limb_score_block, body_part_type::type::leg ) >= 0.5f;
    } else {
        // Check all standard legs for the score threshold
        for( const bodypart_id &bp : owner.get_all_body_parts_of_type( body_part_type::type::leg ) ) {
            if( !bp->has_flag( json_flag_NONSTANDARD_BLOCK ) &&
                owner.get_part( bp )->get_limb_score( owner, limb_score_block ) * bp->limbtypes.at(
                    body_part_type::type::leg ) >= 0.25f ) {
                return true;
            }
        }
    }

    // if not above, can't block.
    return false;
}

bool character_martial_arts::can_arm_block( const Character &owner ) const
{
    const martialart &ma = style_selected.obj();
    ///\EFFECT_UNARMED increases ability to perform arm block
    const int unarmed_skill = owner.has_active_bionic( bio_cqb ) ? 5 : owner.get_skill_level(
                                  skill_unarmed );

    // before we check our arms, can you block at all?
    const bool block_with_skill = unarmed_skill >= ma.arm_block;
    const bool block_with_bio_armor = ma.arm_block_with_bio_armor_arms &&
                                      owner.has_bionic( bio_armor_arms );
    if( !( block_with_skill || block_with_bio_armor ) ) {
        return false;
    }

    // Success conditions.
    // Do we have boring human anatomy? Use the basic calculation
    if( !owner.has_flag( json_flag_NONSTANDARD_BLOCK ) ) {
        return owner.get_limb_score( limb_score_block, body_part_type::type::arm ) >= 0.5f;
    } else {
        // Check all standard arms for the score threshold
        for( const bodypart_id &bp : owner.get_all_body_parts_of_type( body_part_type::type::arm ) ) {
            if( !bp->has_flag( json_flag_NONSTANDARD_BLOCK ) &&
                owner.get_part( bp )->get_limb_score( owner, limb_score_block ) * bp->limbtypes.at(
                    body_part_type::type::arm ) >= 0.25f ) {
                return true;
            }
        }
    }
    // if not above, can't block.
    return false;
}

bool character_martial_arts::can_nonstandard_block( const Character &owner ) const
{
    // No nonstandard limb that blocks
    if( !owner.has_flag( json_flag_NONSTANDARD_BLOCK ) && !owner.has_flag( json_flag_ALWAYS_BLOCK ) ) {
        return false;
    }

    const martialart &ma = style_selected.obj();
    // Bionic combatives won't help with nonstandard anatomy
    const int unarmed_skill = owner.get_skill_level(
                                  skill_unarmed );
    const bool block_with_skill = unarmed_skill >= ma.nonstandard_block;

    // Filter out the case where the flagged BP is overencumbered/broken but blocking score is contributed by arms/legs

    // Success conditions
    // Return true if the limbs which would always block can block
    if( owner.has_flag( json_flag_ALWAYS_BLOCK ) ) {
        for( const bodypart_id &bp : owner.get_all_body_parts_with_flag( json_flag_ALWAYS_BLOCK ) ) {
            if( owner.get_part( bp )->get_limb_score( owner, limb_score_block ) >= 0.25f ) {
                return true;
            }
        }
    }
    // Return true if we're skilled enough to block and we have at least one limb ready to block
    if( block_with_skill ) {
        for( const bodypart_id &bp : owner.get_all_body_parts_with_flag( json_flag_NONSTANDARD_BLOCK ) ) {
            if( owner.get_part( bp )->get_limb_score( owner, limb_score_block ) >= 0.25f ) {
                return true;
            }
        }
    }

    return false;
}

bool character_martial_arts::is_force_unarmed() const
{
    return style_selected->force_unarmed;
}

bool character_martial_arts::can_weapon_block() const
{
    return !style_selected->prevent_weapon_blocking;
}

void character_martial_arts::clear_all_effects( Character &owner )
{
    style_selected->remove_all_buffs( owner );
}

// event handlers
void character_martial_arts::ma_static_effects( Character &owner )
{
    style_selected->apply_static_buffs( owner );
    style_selected->apply_static_eocs( owner );
}
void character_martial_arts::ma_onmove_effects( Character &owner )
{
    style_selected->apply_onmove_buffs( owner );
    style_selected->apply_onmove_eocs( owner );
}
void character_martial_arts::ma_onpause_effects( Character &owner )
{
    style_selected->apply_onpause_buffs( owner );
    style_selected->apply_onpause_eocs( owner );
}
void character_martial_arts::ma_onhit_effects( Character &owner )
{
    style_selected->apply_onhit_buffs( owner );
    style_selected->apply_onhit_eocs( owner );
}
void character_martial_arts::ma_onattack_effects( Character &owner )
{
    style_selected->apply_onattack_buffs( owner );
    style_selected->apply_onattack_eocs( owner );
}
void character_martial_arts::ma_ondodge_effects( Character &owner )
{
    style_selected->apply_ondodge_buffs( owner );
    style_selected->apply_ondodge_eocs( owner );
}
void character_martial_arts::ma_onblock_effects( Character &owner )
{
    style_selected->apply_onblock_buffs( owner );
    style_selected->apply_onblock_eocs( owner );
}
void character_martial_arts::ma_ongethit_effects( Character &owner )
{
    style_selected->apply_ongethit_buffs( owner );
    style_selected->apply_ongethit_eocs( owner );
}
void character_martial_arts::ma_onmiss_effects( Character &owner )
{
    style_selected->apply_onmiss_buffs( owner );
    style_selected->apply_onmiss_eocs( owner );
}
void character_martial_arts::ma_oncrit_effects( Character &owner )
{
    style_selected->apply_oncrit_buffs( owner );
    style_selected->apply_oncrit_eocs( owner );
}
void character_martial_arts::ma_onkill_effects( Character &owner )
{
    style_selected->apply_onkill_buffs( owner );
    style_selected->apply_onkill_eocs( owner );
}

template<typename C, typename F>
static void accumulate_ma_buff_effects( const C &container, F f )
{
    for( auto &elem : container ) {
        for( auto &eff : elem.second ) {
            if( auto buff = ma_buff::from_effect( eff.second ) ) {
                f( *buff, eff.second );
            }
        }
    }
}

template<typename C, typename F>
static bool search_ma_buff_effect( const C &container, F f )
{
    for( auto &elem : container ) {
        for( auto &eff : elem.second ) {
            if( auto buff = ma_buff::from_effect( eff.second ) ) {
                if( f( *buff, eff.second ) ) {
                    return true;
                }
            }
        }
    }
    return false;
}

// bonuses
float Character::mabuff_tohit_bonus() const
{
    float ret = 0.0f;
    accumulate_ma_buff_effects( *effects, [&ret, this]( const ma_buff & b, const effect & ) {
        ret += b.hit_bonus( *this );
    } );
    return ret;
}
float Character::mabuff_critical_hit_chance_bonus() const
{
    float ret = 0.0f;
    accumulate_ma_buff_effects( *effects, [&ret, this]( const ma_buff & b, const effect & d ) {
        ret += d.get_intensity() * b.critical_hit_chance_bonus( *this );
    } );
    return ret;
}
float Character::mabuff_dodge_bonus() const
{
    float ret = 0.0f;
    accumulate_ma_buff_effects( *effects, [&ret, this]( const ma_buff & b, const effect & d ) {
        ret += d.get_intensity() * b.dodge_bonus( *this );
    } );
    return ret;
}
int Character::mabuff_block_bonus() const
{
    int ret = 0;
    accumulate_ma_buff_effects( *effects, [&ret, this]( const ma_buff & b, const effect & d ) {
        ret += d.get_intensity() * b.block_bonus( *this );
    } );
    return ret;
}
int Character::mabuff_block_effectiveness_bonus( ) const
{
    int ret = 0;
    accumulate_ma_buff_effects( *effects, [&ret, this]( const ma_buff & b, const effect & d ) {
        ret += d.get_intensity() * b.block_effectiveness_bonus( *this );
    } );
    return ret;
}
int Character::mabuff_speed_bonus() const
{
    int ret = 0;
    accumulate_ma_buff_effects( *effects, [&ret, this]( const ma_buff & b, const effect & d ) {
        ret += d.get_intensity() * b.speed_bonus( *this );
    } );
    return ret;
}
int Character::mabuff_arpen_bonus( const damage_type_id &type ) const
{
    int ret = 0;
    accumulate_ma_buff_effects( *effects, [&ret, type, this]( const ma_buff & b, const effect & d ) {
        ret += d.get_intensity() * b.arpen_bonus( *this, type );
    } );
    return ret;
}
int Character::mabuff_armor_bonus( const damage_type_id &type ) const
{
    int ret = 0;
    accumulate_ma_buff_effects( *effects, [&ret, type, this]( const ma_buff & b, const effect & d ) {
        ret += d.get_intensity() * b.armor_bonus( *this, type );
    } );
    return ret;
}
float Character::mabuff_damage_mult( const damage_type_id &type ) const
{
    float ret = 1.f;
    accumulate_ma_buff_effects( *effects, [&ret, type, this]( const ma_buff & b, const effect & d ) {
        // This is correct, so that a 20% buff (1.2) plus a 20% buff (1.2)
        // becomes 1.4 instead of 2.4 (which would be a 240% buff)
        ret *= d.get_intensity() * ( b.damage_mult( *this, type ) - 1 ) + 1;
    } );
    return ret;
}
int Character::mabuff_damage_bonus( const damage_type_id &type ) const
{
    int ret = 0;
    accumulate_ma_buff_effects( *effects, [&ret, type, this]( const ma_buff & b, const effect & d ) {
        ret += d.get_intensity() * b.damage_bonus( *this, type );
    } );
    return ret;
}
int Character::mabuff_attack_cost_penalty() const
{
    int ret = 0;
    accumulate_ma_buff_effects( *effects, [&ret, this]( const ma_buff & b, const effect & d ) {
        ret += d.get_intensity() * b.bonuses.get_flat( *this, affected_stat::MOVE_COST );
    } );
    return ret;
}
float Character::mabuff_attack_cost_mult() const
{
    float ret = 1.0f;
    accumulate_ma_buff_effects( *effects, [&ret, this]( const ma_buff & b, const effect & d ) {
        // This is correct, so that a 20% buff (1.2) plus a 20% buff (1.2)
        // becomes 1.4 instead of 2.4 (which would be a 240% buff)
        ret *= d.get_intensity() * ( b.bonuses.get_mult( *this,
                                     affected_stat::MOVE_COST ) - 1 ) + 1;
    } );
    return ret;
}

bool Character::has_mabuff_flag( const json_character_flag &flag ) const
{
    return search_ma_buff_effect( *effects, [flag]( const ma_buff & b, const effect & ) {
        return b.has_flag( flag );
    } );
}

int Character::count_mabuff_flag( const json_character_flag &flag ) const
{
    int ret = 0;
    accumulate_ma_buff_effects( *effects, [&ret, flag]( const ma_buff & b, const effect & ) {
        ret += b.has_flag( flag );
    } );
    return ret;
}

bool Character::is_melee_bash_damage_cap_bonus() const
{
    return search_ma_buff_effect( *effects, []( const ma_buff & b, const effect & ) {
        return b.is_melee_bash_damage_cap_bonus();
    } );
}
bool Character::is_quiet() const
{
    return search_ma_buff_effect( *effects, []( const ma_buff & b, const effect & ) {
        return b.is_quiet();
    } );
}
bool Character::is_stealthy() const
{
    return search_ma_buff_effect( *effects, []( const ma_buff & b, const effect & ) {
        return b.is_stealthy();
    } );
}

bool Character::can_melee() const
{
    return search_ma_buff_effect( *effects, []( const ma_buff & b, const effect & ) {
        return b.can_melee();
    } );
}

bool Character::has_mabuff( const mabuff_id &id ) const
{
    return search_ma_buff_effect( *effects, [&id]( const ma_buff & b, const effect & ) {
        return b.id == id;
    } );
}

bool Character::has_grab_break_tec() const
{
    return martial_arts_data->get_grab_break( *this ).id != tec_none;
}

bool character_martial_arts::has_martialart( const matype_id &ma ) const
{
    return std::find( ma_styles.begin(), ma_styles.end(), ma ) != ma_styles.end();
}

void character_martial_arts::add_martialart( const matype_id &ma_id )
{
    if( has_martialart( ma_id ) ) {
        return;
    }
    ma_styles.emplace_back( ma_id );
}

bool Character::can_autolearn( const matype_id &ma_id ) const
{
    if( ma_id.obj().autolearn_skills.empty() ) {
        return false;
    }

    for( const std::pair<std::string, int> &elem : ma_id.obj().autolearn_skills ) {
        const skill_id skill_req( elem.first );
        const int required_level = elem.second;

        if( required_level > static_cast<int>( get_skill_level( skill_req ) ) ) {
            return false;
        }
    }

    return true;
}

void character_martial_arts::martialart_use_message( const Character &owner ) const
{
    martialart ma = style_selected.obj();
    if( ma.force_unarmed || ma.weapon_valid( owner.get_wielded_item() ) ) {
        owner.add_msg_if_player( m_info, "%s", ma.get_initiate_avatar_message() );
    } else if( ma.strictly_melee && !owner.is_armed() ) {
        owner.add_msg_if_player( m_bad, _( "%s cannot be used unarmed." ), ma.name );
    } else if( ma.strictly_unarmed && owner.is_armed() ) {
        owner.add_msg_if_player( m_bad, _( "%s cannot be used with weapons." ), ma.name );
    } else {
        owner.add_msg_if_player( m_bad, _( "The %1$s is not a valid %2$s weapon." ),
                                 owner.get_wielded_item()->tname( 1, false ), ma.name );
    }
}

float ma_technique::damage_bonus( const Character &u, const damage_type_id &type ) const
{
    return bonuses.get_flat( u, affected_stat::DAMAGE, type );
}

float ma_technique::damage_multiplier( const Character &u, const damage_type_id &type ) const
{
    return bonuses.get_mult( u, affected_stat::DAMAGE, type );
}

float ma_technique::move_cost_multiplier( const Character &u ) const
{
    return bonuses.get_mult( u, affected_stat::MOVE_COST );
}

float ma_technique::move_cost_penalty( const Character &u ) const
{
    return bonuses.get_flat( u, affected_stat::MOVE_COST );
}

float ma_technique::armor_penetration( const Character &u, const damage_type_id &type ) const
{
    return bonuses.get_flat( u, affected_stat::ARMOR_PENETRATION, type );
}

std::string ma_technique::get_description() const
{
    std::string dump;
    std::string type;

    if( block_counter ) {
        type = _( "Block Counter" );
    } else if( dodge_counter ) {
        type = _( "Dodge Counter" );
    } else if( miss_recovery ) {
        type = _( "Miss Recovery" );
    } else if( grab_break ) {
        type = _( "Grab Break" );
    } else if( defensive ) {
        type = _( "Defensive" );
    } else {
        type = _( "Offensive" );
    }

    dump += string_format( _( "<bold>Type:</bold> %s" ), type ) + "\n";

    std::string temp = bonuses.get_description();
    if( !temp.empty() ) {
        dump += _( "<bold>Bonus:</bold> " ) + std::string( "\n" ) + temp;
    }

    dump += reqs.get_description();

    dump += string_format( _( condition_desc ) ) + "\n";

    if( weighting > 1 ) {
        dump += string_format( _( "* <info>Greater chance</info> to activate: <stat>+%s%%</stat>" ),
                               ( 100 * ( weighting - 1 ) ) ) + "\n";
    } else if( weighting < -1 ) {
        dump += string_format( _( "* <info>Lower chance</info> to activate: <stat>1/%s</stat>" ),
                               std::abs( weighting ) ) + "\n";
    }

    if( crit_ok ) {
        dump += _( "* Can activate on a <info>normal</info> or a <info>crit</info> hit" ) +
                std::string( "\n" );
    } else if( crit_tec ) {
        dump += _( "* Will only activate on a <info>crit</info>" ) + std::string( "\n" );
    }

    if( reach_ok ) {
        dump += _( "* Can activate on a <info>normal</info> or a <info>reach attack</info> hit" ) +
                std::string( "\n" );
    } else if( reach_tec ) {
        dump += _( "* Will only activate on a <info>reach attack</info>" ) + std::string( "\n" );
    }

    if( side_switch ) {
        dump += _( "* Moves target <info>behind</info> you" ) + std::string( "\n" );
    }

    if( wall_adjacent ) {
        dump += _( "* Will only activate while <info>near</info> to a <info>wall</info>" ) +
                std::string( "\n" );
    }

    if( dodge_counter ) {
        dump += _( "* Will <info>counterattack</info> when you <info>dodge</info>" ) + std::string( "\n" );
    }

    if( block_counter ) {
        dump += _( "* Will <info>counterattack</info> when you <info>block</info>" ) + std::string( "\n" );
    }

    if( miss_recovery ) {
        dump += _( "* Reduces the time of a <info>miss</info> by <info>half</info>" ) +
                std::string( "\n" );
    }

    if( grab_break ) {
        dump += _( "* Will <info>break</info> a <info>grab</info>" ) + std::string( "\n" );
    }

    if( aoe == "wide" ) {
        dump += _( "* Will attack in a <info>wide arc</info> in front of you" ) + std::string( "\n" );

    } else if( aoe == "spin" ) {
        dump += _( "* Will attack <info>adjacent</info> enemies around you" ) + std::string( "\n" );

    } else if( aoe == "impale" ) {
        dump += _( "* Will <info>attack</info> your target and another <info>one behind</info> it" ) +
                std::string( "\n" );
    }

    if( knockback_dist ) {
        dump += string_format( n_gettext( "* Will <info>knock back</info> enemies <stat>%d tile</stat>",
                                          "* Will <info>knock back</info> enemies <stat>%d tiles</stat>",
                                          knockback_dist ),
                               knockback_dist ) + "\n";
    }

    if( knockback_follow ) {
        dump += _( "* Will <info>follow</info> enemies after knockback." ) + std::string( "\n" );
    }

    if( down_dur ) {
        dump += string_format( n_gettext( "* Will <info>down</info> enemies for <stat>%d turn</stat>",
                                          "* Will <info>down</info> enemies for <stat>%d turns</stat>",
                                          down_dur ),
                               down_dur ) + "\n";
    }

    if( stun_dur ) {
        dump += string_format( n_gettext( "* Will <info>stun</info> target for <stat>%d turn</stat>",
                                          "* Will <info>stun</info> target for <stat>%d turns</stat>",
                                          stun_dur ),
                               stun_dur ) + "\n";
    }

    if( disarms ) {
        dump += _( "* Will <info>disarm</info> the target" ) + std::string( "\n" );
    }

    if( take_weapon ) {
        dump += _( "* Will <info>disarm</info> the target and <info>take their weapon</info>" ) +
                std::string( "\n" );
    }

    return dump;
}

class ma_details_ui
{
        friend class ma_details_ui_impl;
    public:
        void draw_ma_details_ui( const matype_id &style_selected );
};

class ma_details_ui_impl : public cataimgui::window
{
    public:
        std::string last_action;
        explicit ma_details_ui_impl( const matype_id &style_selected ) : cataimgui::window(
                string_format( _( "Martial art style details - %s" ),
                               style_selected.obj().name.translated().c_str() ),
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav ) {
            ma_style = style_selected;
        };
        void init_data();

    private:
        void draw_ma_details_text();

        size_t window_width = ImGui::GetMainViewport()->Size.x * 8 / 9;
        size_t window_height = ImGui::GetMainViewport()->Size.y * 8 / 9;

        bool general_info_group_collapsed = false;
        bool buffs_group_collapsed = false;
        bool techniques_group_collapsed = false;
        bool weapons_group_collapsed = false;

        matype_id ma_style;
        std::vector<std::string> general_info_text;
        std::map<std::string, std::vector<std::string>> buffs_text;
        std::map<std::string, std::vector<std::string>> techniques_text;
        std::map<std::string, std::string> weapons_text;
        int buffs_total = 0;
        int weapons_total = 0;

        cataimgui::scroll s = cataimgui::scroll::none;

    protected:
        void draw_controls() override;
};

void ma_details_ui::draw_ma_details_ui( const matype_id &style_selected )
{
    input_context ctxt( "MA_DETAILS_UI" );
    ma_details_ui_impl p_impl( style_selected );

    p_impl.init_data();

    ctxt.register_navigate_ui_list();
    ctxt.register_leftright();
    ctxt.register_action( "TOGGLE_GENERAL_INFO_GROUP" );
    ctxt.register_action( "TOGGLE_BUFFS_GROUP" );
    ctxt.register_action( "TOGGLE_TECHNIQUES_GROUP" );
    ctxt.register_action( "TOGGLE_WEAPONS_GROUP" );
    ctxt.register_action( "SELECT" );
    ctxt.register_action( "MOUSE_MOVE" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );
    // Smooths out our handling, makes tabs load immediately after input instead of waiting for next.
    ctxt.set_timeout( 10 );

    while( true ) {
        ui_manager::redraw_invalidated();

        p_impl.last_action = ctxt.handle_input();

        if( p_impl.last_action == "QUIT" || !p_impl.get_is_open() ) {
            break;
        }
    }
}

void ma_details_ui_impl::init_data()
{
    general_info_text.clear();
    buffs_text.clear();
    techniques_text.clear();
    weapons_text.clear();

    buffs_total = 0;
    weapons_total = 0;

    if( !ma_style.str().empty() ) {

        const martialart &ma = ma_style.obj();

        if( ma.force_unarmed ) {
            general_info_text.emplace_back(
                _( "This style <bold>forces you to use unarmed strikes</bold>, even if wielding a weapon." ) );
        } else if( ma.allow_all_weapons ) {
            general_info_text.emplace_back( _( "This style can be used with <bold>all</bold> weapons." ) );
        } else if( ma.strictly_melee ) {
            general_info_text.emplace_back( _( "This is an <bold>armed</bold> combat style." ) );
        }

        if( ma.arm_block_with_bio_armor_arms || ma.arm_block != 99 ||
            ma.leg_block_with_bio_armor_legs || ma.leg_block != 99 ||
            ma.nonstandard_block != 99 ) {
            Character &u = get_player_character();
            int unarmed_skill = u.get_skill_level( skill_unarmed );
            if( u.has_active_bionic( bio_cqb ) ) {
                unarmed_skill = BIO_CQB_LEVEL;
            }
            if( ma.arm_block_with_bio_armor_arms ) {
                general_info_text.emplace_back(
                    _( "You can <info>arm block</info> by installing the <info>Arms Alloy Plating CBM</info>" ) );
            } else if( ma.arm_block != 99 ) {
                general_info_text.emplace_back( string_format(
                                                    _( "You can <info>arm block</info> at %s" ),
                                                    required_skill_as_string( skill_unarmed, ma.arm_block, unarmed_skill ) ) );
            }

            if( ma.leg_block_with_bio_armor_legs ) {
                general_info_text.emplace_back(
                    _( "You can <info>leg block</info> by installing the <info>Legs Alloy Plating CBM</info>" ) );
            } else if( ma.leg_block != 99 ) {
                general_info_text.emplace_back( string_format(
                                                    _( "You can <info>leg block</info> at %s" ),
                                                    required_skill_as_string( skill_unarmed, ma.leg_block, unarmed_skill ) ) );
                if( ma.nonstandard_block != 99 ) {
                    general_info_text.emplace_back( string_format(
                                                        _( "You can <info>block with mutated limbs</info> at %s" ),
                                                        required_skill_as_string( skill_unarmed, ma.nonstandard_block, unarmed_skill ) ) );
                }
            }
        }
        for( std::string &entry : general_info_text ) {
            entry = replace_colors( entry );
        }

        auto buff_desc = [&]( const std::string & title, const std::vector<mabuff_id> &buffs,
        bool passive = false ) {
            if( !buffs.empty() ) {
                for( const auto &buff : buffs ) {
                    buffs_total++;
                    std::vector<std::string> buff_lines =
                        string_split( replace_colors( buff->get_description( passive ) ), '\n' );
                    buffs_text[title] = buff_lines;
                }
            }
        };

        buff_desc( _( "Passive buffs" ), ma.static_buffs, true );
        buff_desc( _( "Move buffs" ), ma.onmove_buffs );
        buff_desc( _( "Pause buffs" ), ma.onpause_buffs );
        buff_desc( _( "Hit buffs" ), ma.onhit_buffs );
        buff_desc( _( "Miss buffs" ), ma.onmiss_buffs );
        buff_desc( _( "Attack buffs" ), ma.onattack_buffs );
        buff_desc( _( "Crit buffs" ), ma.oncrit_buffs );
        buff_desc( _( "Kill buffs" ), ma.onkill_buffs );
        buff_desc( _( "Dodge buffs" ), ma.ondodge_buffs );
        buff_desc( _( "Block buffs" ), ma.onblock_buffs );
        buff_desc( _( "Get hit buffs" ), ma.ongethit_buffs );

        for( const auto &tech : ma.techniques ) {
            std::vector<std::string> tehcnique_lines =
                string_split( replace_colors( tech.obj().get_description() ), '\n' );
            techniques_text[tech.obj().name.translated() ] = tehcnique_lines;
        }

        // Copy set to vector for sorting
        std::vector<itype_id> valid_ma_weapons;
        std::copy( ma.weapons.begin(), ma.weapons.end(), std::back_inserter( valid_ma_weapons ) );
        for( const itype *itp : item_controller->all() ) {
            const itype_id &weap_id = itp->get_id();
            if( ma.has_weapon( weap_id ) ) {
                valid_ma_weapons.emplace_back( weap_id );
            }
        }

        if( !valid_ma_weapons.empty() ) {
            Character &player = get_player_character();
            std::map<weapon_category_id, std::vector<std::string>> weaps_by_cat;
            std::sort( valid_ma_weapons.begin(), valid_ma_weapons.end(),
            []( const itype_id & w1, const itype_id & w2 ) {
                return localized_compare( item::nname( w1 ), item::nname( w2 ) );
            } );
            for( const itype_id &w : valid_ma_weapons ) {
                bool carrying = player.has_item_with( [&w]( const item & it ) {
                    return it.typeId() == w;
                } );
                // Wielded weapon in cyan, weapons in player inventory in yellow
                std::string wname = player.get_wielded_item() && player.get_wielded_item()->typeId() == w ?
                                    colorize( item::nname( w ) + _( " (wielded)" ), c_light_cyan ) :
                                    carrying ? colorize( item::nname( w ), c_yellow ) : item::nname( w );
                bool cat_found = false;
                for( const weapon_category_id &w_cat : w->weapon_category ) {
                    // If martial art does not define a weapon category, include all valid categories
                    // If martial art defines one or more weapon categories, only include those categories
                    if( ma.weapon_category.empty() || ma.weapon_category.count( w_cat ) > 0 ) {
                        weaps_by_cat[w_cat].push_back( wname );
                        cat_found = true;
                    }
                }
                if( !cat_found ) {
                    // Weapons that are uncategorized or not in the martial art's weapon categories
                    weaps_by_cat[weapon_category_OTHER_INVALID_WEAP_CAT].push_back( wname );
                }
                weapons_total++;
            }

            bool has_other_cat = false;
            for( auto &weaps : weaps_by_cat ) {
                if( weaps.first == weapon_category_OTHER_INVALID_WEAP_CAT ) {
                    // Print "OTHER" category at the end
                    has_other_cat = true;
                    continue;
                }
                weaps.second.erase( std::unique( weaps.second.begin(), weaps.second.end() ), weaps.second.end() );
                std::string w_cat;
                if( weaps.first.is_valid() ) {
                    w_cat = weaps.first->name().translated();
                } else {
                    // MISSING JSON DEFINITION intentionally not translated
                    w_cat = weaps.first.str() + " - MISSING JSON DEFINITION";
                }

                weapons_text.emplace( w_cat, enumerate_as_string( weaps.second ) );
            }
            if( has_other_cat ) {
                std::vector<std::string> &weaps = weaps_by_cat[weapon_category_OTHER_INVALID_WEAP_CAT];
                weaps.erase( std::unique( weaps.begin(), weaps.end() ), weaps.end() );
                weapons_text.emplace( _( "OTHER" ), enumerate_as_string( weaps ) );
            }
        }
    }
}

void ma_details_ui_impl::draw_ma_details_text()
{

    if( !general_info_text.empty() &&
        ImGui::CollapsingHeader( _( "General info" ),
                                 general_info_group_collapsed ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_DefaultOpen ) ) {
        for( const auto &entry : general_info_text ) {
            cataimgui::TextColoredParagraph( c_light_gray, entry );
            ImGui::NewLine();
        }
    }

    if( !buffs_text.empty() &&
        ImGui::CollapsingHeader( string_format( _( "Buffs (%d in %d categories)" ),
                                 buffs_total, buffs_text.size() ).c_str(),
                                 buffs_group_collapsed ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_DefaultOpen ) ) {
        for( const auto &entry : buffs_text ) {
            cataimgui::TextColoredParagraph( c_header, entry.first );
            ImGui::NewLine();
            for( const auto &buff_line : entry.second ) {
                cataimgui::TextColoredParagraph( c_light_gray, buff_line );
                ImGui::NewLine();
            }
            ImGui::Separator();
        }
    }

    if( !techniques_text.empty() &&
        ImGui::CollapsingHeader( string_format( _( "Techniques (%d)" ), techniques_text.size() ).c_str(),
                                 techniques_group_collapsed ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_DefaultOpen ) ) {
        for( const auto &entry : techniques_text ) {
            cataimgui::TextColoredParagraph( c_header, _( "Technique: " ) );
            ImGui::SameLine();
            cataimgui::TextColoredParagraph( c_bold, entry.first );
            ImGui::NewLine();
            for( const auto &technique_line : entry.second ) {
                cataimgui::TextColoredParagraph( c_light_gray, technique_line );
                ImGui::NewLine();
            }
            ImGui::Separator();
        }
    }

    if( !weapons_text.empty() &&
        ImGui::CollapsingHeader( string_format( _( "Weapons (%d in %d categories)" ),
                                 weapons_total, weapons_text.size() ).c_str(),
                                 weapons_group_collapsed ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_DefaultOpen ) ) {
        for( const auto &entry : weapons_text ) {
            cataimgui::TextColoredParagraph( c_header, entry.first );
            ImGui::NewLine();
            cataimgui::TextColoredParagraph( c_white, entry.second );
            ImGui::NewLine();
            ImGui::Separator();
        }
    }

    cataimgui::set_scroll( s );
}

void ma_details_ui_impl::draw_controls()
{
    ImGui::SetWindowSize( ImVec2( window_width, window_height ), ImGuiCond_Once );

    if( last_action == "QUIT" ) {
        return;
    } else if( last_action == "TOGGLE_GENERAL_INFO_GROUP" ) {
        general_info_group_collapsed = !general_info_group_collapsed;
    } else if( last_action == "TOGGLE_BUFFS_GROUP" ) {
        buffs_group_collapsed = !buffs_group_collapsed;
    } else if( last_action == "TOGGLE_TECHNIQUES_GROUP" ) {
        techniques_group_collapsed = !techniques_group_collapsed;
    } else if( last_action == "TOGGLE_WEAPONS_GROUP" ) {
        weapons_group_collapsed = !weapons_group_collapsed;
    } else if( last_action == "UP" ) {
        s = cataimgui::scroll::line_up;
    } else if( last_action == "DOWN" ) {
        s = cataimgui::scroll::line_down;
    } else if( last_action == "LEFT" ) {
        ImGui::SetScrollX( ImGui::GetScrollX() - ImGui::CalcTextSize( "x" ).x );
    } else if( last_action == "RIGHT" ) {
        ImGui::SetScrollX( ImGui::GetScrollX() + ImGui::CalcTextSize( "x" ).x );
    } else if( last_action == "PAGE_UP" ) {
        s = cataimgui::scroll::page_up;
    } else if( last_action == "PAGE_DOWN" ) {
        s = cataimgui::scroll::page_down;
    } else if( last_action == "HOME" ) {
        s = cataimgui::scroll::begin;
    } else if( last_action == "END" ) {
        s = cataimgui::scroll::end;
    }

    draw_ma_details_text();
}

static void show_ma_details_ui( const matype_id &style_selected )
{
    ma_details_ui new_instance;
    new_instance.draw_ma_details_ui( style_selected );
}

bool ma_style_callback::key( const input_context &ctxt, const input_event &event, int entnum,
                             uilist * )
{
    const std::string &action = ctxt.input_to_action( event );
    if( entnum < static_cast<int>( offset ) || action != "SHOW_DESCRIPTION" )  {
        return false;
    }

    matype_id style_selected;
    const size_t index = entnum;
    if( index >= offset && index - offset < styles.size() ) {
        style_selected = styles[index - offset];
    }

    show_ma_details_ui( style_selected );

    return true;
}
