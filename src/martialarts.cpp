#include "martialarts.h"

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "avatar.h"
#include "character.h"
#include "character_martial_arts.h"
#include "color.h"
#include "cursesdef.h"
#include "damage.h"
#include "debug.h"
#include "effect.h"
#include "enums.h"
#include "generic_factory.h"
#include "input.h"
#include "item.h"
#include "itype.h"
#include "json.h"
#include "map.h"
#include "output.h"
#include "pimpl.h"
#include "player.h"
#include "pldata.h"
#include "point.h"
#include "skill.h"
#include "string_formatter.h"
#include "string_id.h"
#include "translations.h"
#include "ui_manager.h"
#include "value_ptr.h"

static const skill_id skill_unarmed( "unarmed" );

static const bionic_id bio_armor_arms( "bio_armor_arms" );
static const bionic_id bio_armor_legs( "bio_armor_legs" );
static const bionic_id bio_cqb( "bio_cqb" );

static const std::string flag_UNARMED_WEAPON( "UNARMED_WEAPON" );

namespace
{
generic_factory<ma_technique> ma_techniques( "martial art technique" );
generic_factory<martialart> martialarts( "martial art style" );
generic_factory<ma_buff> ma_buffs( "martial art buff" );
} // namespace

matype_id martial_art_learned_from( const itype &type )
{
    if( !type.can_use( "MA_MANUAL" ) ) {
        return {};
    }

    if( !type.book || type.book->martial_art.is_null() ) {
        debugmsg( "Item '%s' which claims to teach a martial art is missing martial_art",
                  type.get_id() );
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
        std::pair<skill_id, int> get_next( JsonIn &jin ) const {
            JsonObject jo = jin.get_object();
            return std::pair<skill_id, int>( skill_id( jo.get_string( "name" ) ), jo.get_int( "level" ) );
        }
        template<typename C>
        void erase_next( JsonIn &jin, C &container ) const {
            const skill_id id = skill_id( jin.get_string() );
            reader_detail::handler<C>().erase_if( container, [&id]( const std::pair<skill_id, int> &e ) {
                return e.first == id;
            } );
        }
};

class ma_weapon_damage_reader : public generic_typed_reader<ma_weapon_damage_reader>
{
    public:
        std::map<std::string, damage_type> dt_map = get_dt_map();

        std::pair<damage_type, int> get_next( JsonIn &jin ) const {
            JsonObject jo = jin.get_object();
            std::string type = jo.get_string( "type" );
            const auto iter = get_dt_map().find( type );
            if( iter == get_dt_map().end() ) {
                jo.throw_error( "Invalid damage type" );
            }
            const damage_type dt = iter->second;
            return std::pair<damage_type, int>( dt, jo.get_int( "min" ) );
        }
        template<typename C>
        void erase_next( JsonIn &jin, C &container ) const {
            JsonObject jo = jin.get_object();
            std::string type = jo.get_string( "type" );
            const auto iter = get_dt_map().find( type );
            if( iter == get_dt_map().end() ) {
                jo.throw_error( "Invalid damage type" );
            }
            damage_type id = iter->second;
            reader_detail::handler<C>().erase_if( container, [&id]( const std::pair<damage_type, int> &e ) {
                return e.first == id;
            } );
        }
};

void ma_requirements::load( const JsonObject &jo, const std::string & )
{
    optional( jo, was_loaded, "unarmed_allowed", unarmed_allowed, false );
    optional( jo, was_loaded, "melee_allowed", melee_allowed, false );
    optional( jo, was_loaded, "unarmed_weapons_allowed", unarmed_weapons_allowed, true );
    optional( jo, was_loaded, "strictly_unarmed", strictly_unarmed, false );
    optional( jo, was_loaded, "wall_adjacent", wall_adjacent, false );

    optional( jo, was_loaded, "req_buffs", req_buffs, auto_flags_reader<mabuff_id> {} );
    optional( jo, was_loaded, "req_flags", req_flags, auto_flags_reader<> {} );

    optional( jo, was_loaded, "skill_requirements", min_skill, ma_skill_reader {} );
    optional( jo, was_loaded, "weapon_damage_requirements", min_damage, ma_weapon_damage_reader {} );
}

void ma_technique::load( const JsonObject &jo, const std::string &src )
{
    mandatory( jo, was_loaded, "name", name );
    optional( jo, was_loaded, "description", description, "" );

    if( jo.has_member( "messages" ) ) {
        JsonArray jsarr = jo.get_array( "messages" );
        avatar_message = jsarr.get_string( 0 );
        npc_message = jsarr.get_string( 1 );
    }

    optional( jo, was_loaded, "crit_tec", crit_tec, false );
    optional( jo, was_loaded, "crit_ok", crit_ok, false );
    optional( jo, was_loaded, "downed_target", downed_target, false );
    optional( jo, was_loaded, "stunned_target", stunned_target, false );
    optional( jo, was_loaded, "wall_adjacent", wall_adjacent, false );
    optional( jo, was_loaded, "human_target", human_target, false );

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

    optional( jo, was_loaded, "down_dur", down_dur, 0 );
    optional( jo, was_loaded, "stun_dur", stun_dur, 0 );
    optional( jo, was_loaded, "knockback_dist", knockback_dist, 0 );
    optional( jo, was_loaded, "knockback_spread", knockback_spread, 0 );
    optional( jo, was_loaded, "powerful_knockback", powerful_knockback, false );
    optional( jo, was_loaded, "knockback_follow", knockback_follow, false );

    optional( jo, was_loaded, "aoe", aoe, "" );
    optional( jo, was_loaded, "flags", flags, auto_flags_reader<> {} );

    reqs.load( jo, src );
    bonuses.load( jo );
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

void ma_buff::load( const JsonObject &jo, const std::string &src )
{
    mandatory( jo, was_loaded, "name", name );
    mandatory( jo, was_loaded, "description", description );

    optional( jo, was_loaded, "buff_duration", buff_duration, 2_turns );
    optional( jo, was_loaded, "max_stacks", max_stacks, 1 );

    optional( jo, was_loaded, "bonus_dodges", dodges_bonus, 0 );
    optional( jo, was_loaded, "bonus_blocks", blocks_bonus, 0 );

    optional( jo, was_loaded, "quiet", quiet, false );
    optional( jo, was_loaded, "throw_immune", throw_immune, false );
    optional( jo, was_loaded, "stealthy", stealthy, false );

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
        mabuff_id get_next( JsonIn &jin ) const {
            if( jin.test_string() ) {
                return mabuff_id( jin.get_string() );
            }
            JsonObject jsobj = jin.get_object();
            ma_buffs.load( jsobj, "" );
            return mabuff_id( jsobj.get_string( "id" ) );
        }
};

void martialart::load( const JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "name", name );
    mandatory( jo, was_loaded, "description", description );
    mandatory( jo, was_loaded, "initiate", initiate );
    for( JsonArray skillArray : jo.get_array( "autolearn" ) ) {
        std::string skill_name = skillArray.get_string( 0 );
        int skill_level = 0;
        std::string skill_level_string = skillArray.get_string( 1 );
        skill_level = stoi( skill_level_string );
        autolearn_skills.emplace_back( skill_name, skill_level );
    }
    optional( jo, was_loaded, "primary_skill", primary_skill, skill_id( "unarmed" ) );
    optional( jo, was_loaded, "learn_difficulty", learn_difficulty );

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

    optional( jo, was_loaded, "techniques", techniques, auto_flags_reader<matec_id> {} );
    optional( jo, was_loaded, "weapons", weapons, auto_flags_reader<itype_id> {} );

    optional( jo, was_loaded, "strictly_melee", strictly_melee, false );
    optional( jo, was_loaded, "strictly_unarmed", strictly_unarmed, false );
    optional( jo, was_loaded, "allow_melee", allow_melee, false );
    optional( jo, was_loaded, "force_unarmed", force_unarmed, false );

    optional( jo, was_loaded, "leg_block", leg_block, 99 );
    optional( jo, was_loaded, "arm_block", arm_block, 99 );

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
    for( const auto &ma : martialarts.get_all() ) {
        result.push_back( ma.id );
    }
    return result;
}

std::vector<matype_id> autolearn_martialart_types()
{
    std::vector<matype_id> result;
    for( const auto &ma : martialarts.get_all() ) {
        if( ma.autolearn_skills.empty() ) {
            continue;
        }
        result.push_back( ma.id );
    }
    return result;
}

static void check( const ma_requirements &req, const std::string &display_text )
{
    for( auto &r : req.req_buffs ) {
        if( !r.is_valid() ) {
            debugmsg( "ma buff %s of %s does not exist", r.c_str(), display_text );
        }
    }
}

void check_martialarts()
{
    for( const auto &ma : martialarts.get_all() ) {
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
    for( const auto &t : ma_techniques.get_all() ) {
        ::check( t.reqs, string_format( "technique %s", t.id.c_str() ) );
    }
    for( const auto &b : ma_buffs.get_all() ) {
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
        ma_buff_effect_type( const ma_buff &buff ) {
            id = buff.get_effect_id();
            max_intensity = buff.max_stacks;
            // add_effect add the duration to an existing effect, but it must never be
            // above buff_duration, this keeps the old ma_buff behavior
            max_duration = buff.buff_duration;
            dur_add_perc = 100;
            // each add_effect call increases the intensity by 1
            int_add_val = 1;
            // effect intensity increases by -1 each turn.
            int_decay_step = -1;
            int_decay_tick = 1;
            int_dur_factor = 0_turns;
            name.push_back( to_translation( buff.name ) );
            desc.push_back( buff.description );
            rating = e_good;
        }
};

void finialize_martial_arts()
{
    // This adds an effect type for each ma_buff, so we can later refer to it and don't need a
    // redundant definition of those effects in json.
    for( const auto &buff : ma_buffs.get_all() ) {
        const ma_buff_effect_type new_eff( buff );
        // Note the slicing here: new_eff is converted to a plain effect_type, but this doesn't
        // bother us because ma_buff_effect_type does not have any members that can be sliced.
        effect_type::register_ma_buff_effect( new_eff );
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

bool ma_requirements::is_valid_character( const Character &u ) const
{
    for( const mabuff_id &buff_id : req_buffs ) {
        if( !u.has_mabuff( buff_id ) ) {
            return false;
        }
    }

    //A technique is valid if it applies to unarmed strikes, if it applies generally
    //to all weapons (such as Ninjutsu sneak attacks or innate weapon techniques like RAPID)
    //or if the weapon is flagged as being compatible with the style. Some techniques have
    //further restrictions on required weapon properties (is_valid_weapon).
    bool cqb = u.has_active_bionic( bio_cqb );
    // There are 4 different cases of "armedness":
    // Truly unarmed, unarmed weapon, style-allowed weapon, generic weapon
    bool melee_style = u.martial_arts_data.selected_strictly_melee();
    bool is_armed = u.is_armed();
    bool unarmed_weapon = is_armed && u.used_weapon().has_flag( flag_UNARMED_WEAPON );
    bool forced_unarmed = u.martial_arts_data.selected_force_unarmed();
    bool weapon_ok = is_valid_weapon( u.weapon );
    bool style_weapon = u.martial_arts_data.selected_has_weapon( u.weapon.typeId() );
    bool all_weapons = u.martial_arts_data.selected_allow_melee();

    bool unarmed_ok = !is_armed || ( unarmed_weapon && unarmed_weapons_allowed );
    bool melee_ok = melee_allowed && weapon_ok && ( style_weapon || all_weapons );

    bool valid_unarmed = !melee_style && unarmed_allowed && unarmed_ok;
    bool valid_melee = !strictly_unarmed && ( forced_unarmed || melee_ok );

    if( !valid_unarmed && !valid_melee ) {
        return false;
    }

    if( wall_adjacent && !get_map().is_wall_adjacent( u.pos() ) ) {
        return false;
    }

    for( const auto &pr : min_skill ) {
        if( ( cqb ? 5 : u.get_skill_level( pr.first ) ) < pr.second ) {
            return false;
        }
    }

    return true;
}

bool ma_requirements::is_valid_weapon( const item &i ) const
{
    for( const std::string &flag : req_flags ) {
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

std::string ma_requirements::get_description( bool buff ) const
{
    std::string dump;

    if( std::any_of( min_skill.begin(), min_skill.end(), []( const std::pair<skill_id, int> &pr ) {
    return pr.second > 0;
} ) ) {
        dump += string_format( _( "<bold>%s required: </bold>" ),
                               vgettext( "Skill", "Skills", min_skill.size() ) );

        dump += enumerate_as_string( min_skill.begin(),
        min_skill.end(), []( const std::pair<skill_id, int>  &pr ) {
            int player_skill = get_player_character().get_skill_level( skill_id( pr.first ) );
            if( get_player_character().has_active_bionic( bio_cqb ) ) {
                player_skill = BIO_CQB_LEVEL;
            }
            return string_format( "%s: <stat>%d</stat>/<stat>%d</stat>", pr.first->name(), player_skill,
                                  pr.second );
        }, enumeration_conjunction::none ) + "\n";
    }

    if( std::any_of( min_damage.begin(),
    min_damage.end(), []( const std::pair<damage_type, int>  &pr ) {
    return pr.second > 0;
} ) ) {
        dump += vgettext( "<bold>Damage type required: </bold>",
                          "<bold>Damage types required: </bold>", min_damage.size() );

        dump += enumerate_as_string( min_damage.begin(),
        min_damage.end(), []( const std::pair<damage_type, int>  &pr ) {
            return string_format( _( "%s: <stat>%d</stat>" ), name_by_dt( pr.first ), pr.second );
        }, enumeration_conjunction::none ) + "\n";
    }

    if( !req_buffs.empty() ) {
        dump += _( "<bold>Requires:</bold> " );

        dump += enumerate_as_string( req_buffs.begin(), req_buffs.end(), []( const mabuff_id & bid ) {
            return _( bid->name );
        }, enumeration_conjunction::none ) + "\n";
    }

    const std::string type = buff ? _( "activate" ) : _( "be used" );

    if( unarmed_allowed && melee_allowed ) {
        dump += string_format( _( "* Can %s while <info>armed</info> or <info>unarmed</info>" ),
                               type ) + "\n";
        if( unarmed_weapons_allowed ) {
            dump += string_format( _( "* Can %s while using <info>any unarmed weapon</info>" ),
                                   type ) + "\n";
        }
    } else if( unarmed_allowed ) {
        dump += string_format( _( "* Can <info>only</info> %s while <info>unarmed</info>" ),
                               type ) + "\n";
        if( unarmed_weapons_allowed ) {
            dump += string_format( _( "* Can %s while using <info>any unarmed weapon</info>" ),
                                   type ) + "\n";
        }
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
    powerful_knockback = false;
    knockback_follow = false; // player follows the knocked-back party into their former tile

    // offensive
    disarms = false; // like tec_disarm
    take_weapon = false; // disarms and equips weapon if hands are free
    dodge_counter = false; // like tec_grab
    block_counter = false; // like tec_counter

    // conditional
    downed_target = false;    // only works on downed enemies
    stunned_target = false;   // only works on stunned enemies
    wall_adjacent = false;    // only works near a wall
    human_target = false;     // only works on humanoid enemies

    miss_recovery = false; // allows free recovery from misses, like tec_feint
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

    throw_immune = false;

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
int ma_buff::dodge_bonus( const Character &u ) const
{
    return bonuses.get_flat( u, affected_stat::DODGE );
}
int ma_buff::block_bonus( const Character &u ) const
{
    return bonuses.get_flat( u, affected_stat::BLOCK );
}
int ma_buff::speed_bonus( const Character &u ) const
{
    return bonuses.get_flat( u, affected_stat::SPEED );
}
int ma_buff::armor_bonus( const Character &guy, damage_type dt ) const
{
    return bonuses.get_flat( guy, affected_stat::ARMOR, dt );
}
float ma_buff::damage_bonus( const Character &u, damage_type dt ) const
{
    return bonuses.get_flat( u, affected_stat::DAMAGE, dt );
}
float ma_buff::damage_mult( const Character &u, damage_type dt ) const
{
    return bonuses.get_mult( u, affected_stat::DAMAGE, dt );
}
bool ma_buff::is_throw_immune() const
{
    return throw_immune;
}
bool ma_buff::is_quiet() const
{
    return quiet;
}
bool ma_buff::is_stealthy() const
{
    return stealthy;
}

bool ma_buff::can_melee() const
{
    return melee_allowed;
}

std::string ma_buff::get_description( bool passive ) const
{
    std::string dump;
    dump += string_format( _( "<bold>Buff technique:</bold> %s" ), _( name ) ) + "\n";

    std::string temp = bonuses.get_description();
    if( !temp.empty() ) {
        dump += string_format( _( "<bold>%s:</bold> " ),
                               vgettext( "Bonus", "Bonus/stack", max_stacks ) ) + "\n" + temp;
    }

    dump += reqs.get_description( true );

    if( max_stacks > 1 ) {
        dump += string_format( _( "* Will <info>stack</info> up to <stat>%d</stat> times" ),
                               max_stacks ) + "\n";
    }

    const int turns = to_turns<int>( buff_duration );
    if( !passive && turns ) {
        dump += string_format( _( "* Will <info>last</info> for <stat>%d %s</stat>" ),
                               turns, vgettext( "turn", "turns", turns ) ) + "\n";
    }

    if( dodges_bonus > 0 ) {
        dump += string_format( _( "* Will give a <good>+%s</good> bonus to <info>dodge</info>%s" ),
                               dodges_bonus, vgettext( " for the stack", " per stack", max_stacks ) ) + "\n";
    } else if( dodges_bonus < 0 ) {
        dump += string_format( _( "* Will give a <bad>%s</bad> penalty to <info>dodge</info>%s" ),
                               dodges_bonus, vgettext( " for the stack", " per stack", max_stacks ) ) + "\n";
    }

    if( blocks_bonus > 0 ) {
        dump += string_format( _( "* Will give a <good>+%s</good> bonus to <info>block</info>%s" ),
                               blocks_bonus, vgettext( " for the stack", " per stack", max_stacks ) ) + "\n";
    } else if( blocks_bonus < 0 ) {
        dump += string_format( _( "* Will give a <bad>%s</bad> penalty to <info>block</info>%s" ),
                               blocks_bonus, vgettext( " for the stack", " per stack", max_stacks ) ) + "\n";
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
    for( auto &elem : buffer ) {
        elem->apply_buff( u );
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
    return weapons.count( itt ) > 0;
}

bool martialart::weapon_valid( const item &it ) const
{
    if( allow_melee ) {
        return true;
    }

    if( it.is_null() && !strictly_melee ) {
        return true;
    }

    if( has_weapon( it.typeId() ) ) {
        return true;
    }

    if( !strictly_unarmed && !strictly_melee && !it.is_null() && it.has_flag( "UNARMED_WEAPON" ) ) {
        return true;
    }

    return false;
}

std::string martialart::get_initiate_avatar_message() const
{
    return initiate[0];
}

std::string martialart::get_initiate_npc_message() const
{
    return initiate[1];
}
// Player stuff

// technique
std::vector<matec_id> character_martial_arts::get_all_techniques( const item &weap ) const
{
    std::vector<matec_id> tecs;
    // Grab individual item techniques
    const auto &weapon_techs = weap.get_techniques();
    tecs.insert( tecs.end(), weapon_techs.begin(), weapon_techs.end() );
    // and martial art techniques
    const auto &style = style_selected.obj();
    tecs.insert( tecs.end(), style.techniques.begin(), style.techniques.end() );

    return tecs;
}

// defensive technique-related
bool character_martial_arts::has_miss_recovery_tec( const item &weap ) const
{
    for( const matec_id &technique : get_all_techniques( weap ) ) {
        if( technique->miss_recovery ) {
            return true;
        }
    }
    return false;
}

ma_technique character_martial_arts::get_miss_recovery_tec( const item &weap ) const
{
    ma_technique tech;
    for( const matec_id &technique : get_all_techniques( weap ) ) {
        if( technique->miss_recovery ) {
            tech = technique.obj();
            break;
        }
    }

    return tech;
}

// This one isn't used with a weapon
bool character_martial_arts::has_grab_break_tec() const
{
    for( const matec_id &technique : get_all_techniques( item() ) ) {
        if( technique->grab_break ) {
            return true;
        }
    }
    return false;
}

ma_technique character_martial_arts::get_grab_break_tec( const item &weap ) const
{
    ma_technique tec;
    for( const matec_id &technique : get_all_techniques( weap ) ) {
        if( technique->grab_break ) {
            tec = technique.obj();
            break;
        }
    }
    return tec;
}

bool player::can_grab_break( const item &weap ) const
{
    if( !has_grab_break_tec() ) {
        return false;
    }

    ma_technique tec = martial_arts_data.get_grab_break_tec( weap );

    return tec.is_valid_character( *this );
}

bool Character::can_miss_recovery( const item &weap ) const
{
    if( !martial_arts_data.has_miss_recovery_tec( weap ) ) {
        return false;
    }

    ma_technique tec = martial_arts_data.get_miss_recovery_tec( weap );

    return tec.is_valid_character( *this );
}

bool character_martial_arts::can_leg_block( const Character &owner ) const
{
    const martialart &ma = style_selected.obj();
    ///\EFFECT_UNARMED increases ability to perform leg block
    int unarmed_skill = owner.has_active_bionic( bio_cqb ) ? 5 : owner.get_skill_level(
                            skill_unarmed );

    // Success conditions.
    if( owner.get_working_leg_count() >= 1 ) {
        if( unarmed_skill >= ma.leg_block ) {
            return true;
        } else if( ma.leg_block_with_bio_armor_legs && owner.has_bionic( bio_armor_legs ) ) {
            return true;
        }
    }
    // if not above, can't block.
    return false;
}

bool character_martial_arts::can_arm_block( const Character &owner ) const
{
    const martialart &ma = style_selected.obj();
    ///\EFFECT_UNARMED increases ability to perform arm block
    int unarmed_skill = owner.has_active_bionic( bio_cqb ) ? 5 : owner.get_skill_level(
                            skill_unarmed );

    // Success conditions.
    if( !owner.is_limb_broken( hp_arm_l ) || !owner.is_limb_broken( hp_arm_r ) ) {
        if( unarmed_skill >= ma.arm_block ) {
            return true;
        } else if( ma.arm_block_with_bio_armor_arms && owner.has_bionic( bio_armor_arms ) ) {
            return true;
        }
    }
    // if not above, can't block.
    return false;
}

bool character_martial_arts::can_limb_block( const Character &owner ) const
{
    return can_arm_block( owner ) || can_leg_block( owner );
}

bool character_martial_arts::is_force_unarmed() const
{
    return style_selected->force_unarmed;
}

// event handlers
void character_martial_arts::ma_static_effects( Character &owner )
{
    style_selected->apply_static_buffs( owner );
}
void character_martial_arts::ma_onmove_effects( Character &owner )
{
    style_selected->apply_onmove_buffs( owner );
}
void character_martial_arts::ma_onpause_effects( Character &owner )
{
    style_selected->apply_onpause_buffs( owner );
}
void character_martial_arts::ma_onhit_effects( Character &owner )
{
    style_selected->apply_onhit_buffs( owner );
}
void character_martial_arts::ma_onattack_effects( Character &owner )
{
    style_selected->apply_onattack_buffs( owner );
}
void character_martial_arts::ma_ondodge_effects( Character &owner )
{
    style_selected->apply_ondodge_buffs( owner );
}
void character_martial_arts::ma_onblock_effects( Character &owner )
{
    style_selected->apply_onblock_buffs( owner );
}
void character_martial_arts::ma_ongethit_effects( Character &owner )
{
    style_selected->apply_ongethit_buffs( owner );
}
void character_martial_arts::ma_onmiss_effects( Character &owner )
{
    style_selected->apply_onmiss_buffs( owner );
}
void character_martial_arts::ma_oncrit_effects( Character &owner )
{
    style_selected->apply_oncrit_buffs( owner );
}
void character_martial_arts::ma_onkill_effects( Character &owner )
{
    style_selected->apply_onkill_buffs( owner );
}

template<typename C, typename F>
static void accumulate_ma_buff_effects( const C &container, F f )
{
    for( auto &elem : container ) {
        for( auto &eff : elem.second ) {
            const effect &e = eff.second;
            if( e.is_removed() ) {
                continue;
            }
            if( auto buff = ma_buff::from_effect( e ) ) {
                f( *buff, e );
            }
        }
    }
}

template<typename C, typename F>
static bool search_ma_buff_effect( const C &container, F f )
{
    for( auto &elem : container ) {
        for( auto &eff : elem.second ) {
            const effect &e = eff.second;
            if( e.is_removed() ) {
                continue;
            }
            if( auto buff = ma_buff::from_effect( e ) ) {
                if( f( *buff, e ) ) {
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
    float ret = 0;
    accumulate_ma_buff_effects( *effects, [&ret, this]( const ma_buff & b, const effect & ) {
        ret += b.hit_bonus( *this );
    } );
    return ret;
}
float Character::mabuff_dodge_bonus() const
{
    float ret = 0;
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
int Character::mabuff_speed_bonus() const
{
    int ret = 0;
    accumulate_ma_buff_effects( *effects, [&ret, this]( const ma_buff & b, const effect & d ) {
        ret += d.get_intensity() * b.speed_bonus( *this );
    } );
    return ret;
}
int Character::mabuff_armor_bonus( damage_type type ) const
{
    int ret = 0;
    accumulate_ma_buff_effects( *effects, [&ret, type, this]( const ma_buff & b, const effect & d ) {
        ret += d.get_intensity() * b.armor_bonus( *this, type );
    } );
    return ret;
}
float Character::mabuff_damage_mult( damage_type type ) const
{
    float ret = 1.f;
    accumulate_ma_buff_effects( *effects, [&ret, type, this]( const ma_buff & b, const effect & d ) {
        // This is correct, so that a 20% buff (1.2) plus a 20% buff (1.2)
        // becomes 1.4 instead of 2.4 (which would be a 240% buff)
        ret *= d.get_intensity() * ( b.damage_mult( *this, type ) - 1 ) + 1;
    } );
    return ret;
}
int Character::mabuff_damage_bonus( damage_type type ) const
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

bool Character::is_throw_immune() const
{
    return search_ma_buff_effect( *effects, []( const ma_buff & b, const effect & ) {
        return b.is_throw_immune();
    } );
}
bool Character::is_quiet() const
{
    return search_ma_buff_effect( *effects, []( const ma_buff & b, const effect & ) {
        return b.is_quiet();
    } );
}
bool player::is_stealthy() const
{
    return search_ma_buff_effect( *effects, []( const ma_buff & b, const effect & ) {
        return b.is_stealthy();
    } );
}

bool player::can_melee() const
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

bool player::can_autolearn( const matype_id &ma_id ) const
{
    if( ma_id.obj().autolearn_skills.empty() ) {
        return false;
    }

    for( const std::pair<std::string, int> &elem : ma_id.obj().autolearn_skills ) {
        const skill_id skill_req( elem.first );
        const int required_level = elem.second;

        if( required_level > get_skill_level( skill_req ) ) {
            return false;
        }
    }

    return true;
}

void character_martial_arts::martialart_use_message( const Character &owner ) const
{
    martialart ma = style_selected.obj();
    if( ma.force_unarmed || ma.weapon_valid( owner.weapon ) ) {
        owner.add_msg_if_player( m_info, _( ma.get_initiate_avatar_message() ) );
    } else if( ma.strictly_melee && !owner.is_armed() ) {
        owner.add_msg_if_player( m_bad, _( "%s cannot be used unarmed." ), ma.name );
    } else if( ma.strictly_unarmed && owner.is_armed() ) {
        owner.add_msg_if_player( m_bad, _( "%s cannot be used with weapons." ), ma.name );
    } else {
        owner.add_msg_if_player( m_bad, _( "The %1$s is not a valid %2$s weapon." ), owner.weapon.tname( 1,
                                 false ), ma.name );
    }
}

float ma_technique::damage_bonus( const Character &u, damage_type type ) const
{
    return bonuses.get_flat( u, affected_stat::DAMAGE, type );
}

float ma_technique::damage_multiplier( const Character &u, damage_type type ) const
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

float ma_technique::armor_penetration( const Character &u, damage_type type ) const
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

    if( side_switch ) {
        dump += _( "* Moves target <info>behind</info> you" ) + std::string( "\n" );
    }

    if( wall_adjacent ) {
        dump += _( "* Will only activate while <info>near</info> to a <info>wall</info>" ) +
                std::string( "\n" );
    }

    if( downed_target ) {
        dump += _( "* Only works on a <info>downed</info> target" ) + std::string( "\n" );
    }

    if( stunned_target ) {
        dump += _( "* Only works on a <info>stunned</info> target" ) + std::string( "\n" );
    }

    if( human_target ) {
        dump += _( "* Only works on a <info>humanoid</info> target" ) + std::string( "\n" );
    }

    if( powerful_knockback ) {
        dump += _( "* Causes extra damage on <info>knockback collision</info>." ) + std::string( "\n" );
    }

    if( dodge_counter ) {
        dump += _( "* Will <info>counterattack</info> when you <info>dodge</info>" ) + std::string( "\n" );
    }

    if( block_counter ) {
        dump += _( "* Will <info>counterattack</info> when you <info>block</info>" ) + std::string( "\n" );
    }

    if( miss_recovery ) {
        dump += _( "* Will grant <info>free recovery</info> from a <info>miss</info>" ) +
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
        dump += string_format( _( "* Will <info>knock back</info> enemies <stat>%d %s</stat>" ),
                               knockback_dist, vgettext( "tile", "tiles", knockback_dist ) ) + "\n";
    }

    if( knockback_follow ) {
        dump += _( "* Will <info>follow</info> enemies after knockback." ) + std::string( "\n" );
    }

    if( down_dur ) {
        dump += string_format( _( "* Will <info>down</info> enemies for <stat>%d %s</stat>" ),
                               down_dur, vgettext( "turn", "turns", down_dur ) ) + "\n";
    }

    if( stun_dur ) {
        dump += string_format( _( "* Will <info>stun</info> target for <stat>%d %s</stat>" ),
                               stun_dur, vgettext( "turn", "turns", stun_dur ) ) + "\n";
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

bool ma_style_callback::key( const input_context &ctxt, const input_event &event, int entnum,
                             uilist * )
{
    const std::string &action = ctxt.input_to_action( event );
    if( action != "SHOW_DESCRIPTION" ) {
        return false;
    }
    matype_id style_selected;
    const size_t index = entnum;
    if( index >= offset && index - offset < styles.size() ) {
        style_selected = styles[index - offset];
    }
    if( !style_selected.str().empty() ) {
        const martialart &ma = style_selected.obj();

        std::string buffer;

        if( ma.force_unarmed ) {
            buffer += _( "<bold>This style forces you to use unarmed strikes, even if wielding a weapon.</bold>" );
            buffer += "\n";
        } else if( ma.allow_melee ) {
            buffer += _( "<bold>This style can be used with all weapons.</bold>" );
            buffer += "\n";
        } else if( ma.strictly_melee ) {
            buffer += _( "<bold>This is an armed combat style.</bold>" );
            buffer += "\n";
        }

        buffer += "--\n";

        if( ma.arm_block_with_bio_armor_arms || ma.arm_block != 99 ||
            ma.leg_block_with_bio_armor_legs || ma.leg_block != 99 ) {
            int unarmed_skill =  get_player_character().get_skill_level( skill_unarmed );
            if( get_player_character().has_active_bionic( bio_cqb ) ) {
                unarmed_skill = BIO_CQB_LEVEL;
            }
            if( ma.arm_block_with_bio_armor_arms ) {
                buffer += _( "You can <info>arm block</info> by installing the <info>Arms Alloy Plating CBM</info>" );
                buffer += "\n";
            } else if( ma.arm_block != 99 ) {
                buffer += string_format(
                              _( "You can <info>arm block</info> at <info>unarmed combat:</info> <stat>%s</stat>/<stat>%s</stat>" ),
                              unarmed_skill, ma.arm_block ) + "\n";
            }

            if( ma.leg_block_with_bio_armor_legs ) {
                buffer += _( "You can <info>leg block</info> by installing the <info>Legs Alloy Plating CBM</info>" );
                buffer += "\n";
            } else if( ma.leg_block != 99 ) {
                buffer += string_format(
                              _( "You can <info>leg block</info> at <info>unarmed combat:</info> <stat>%s</stat>/<stat>%s</stat>" ),
                              unarmed_skill, ma.leg_block );
                buffer += "\n";
            }
            buffer += "--\n";
        }

        auto buff_desc = [&]( const std::string & title, const std::vector<mabuff_id> &buffs,
        bool passive = false ) {
            if( !buffs.empty() ) {
                buffer += string_format( _( "<header>%s buffs:</header>" ), title );
                for( const auto &buff : buffs ) {
                    buffer += "\n" + buff->get_description( passive );
                }
                buffer += "--\n";
            }
        };

        buff_desc( _( "Passive" ), ma.static_buffs, true );
        buff_desc( _( "Move" ), ma.onmove_buffs );
        buff_desc( _( "Pause" ), ma.onpause_buffs );
        buff_desc( _( "Hit" ), ma.onhit_buffs );
        buff_desc( _( "Miss" ), ma.onmiss_buffs );
        buff_desc( _( "Attack" ), ma.onattack_buffs );
        buff_desc( _( "Crit" ), ma.oncrit_buffs );
        buff_desc( _( "Kill" ), ma.onkill_buffs );
        buff_desc( _( "Dodge" ), ma.ondodge_buffs );
        buff_desc( _( "Block" ), ma.onblock_buffs );
        buff_desc( _( "Get hit" ), ma.ongethit_buffs );

        for( const auto &tech : ma.techniques ) {
            buffer += string_format( _( "<header>Technique:</header> <bold>%s</bold>   " ),
                                     _( tech.obj().name ) ) + "\n";
            buffer += tech.obj().get_description() + "--\n";
        }

        if( !ma.weapons.empty() ) {
            std::vector<std::string> weapons;
            std::transform( ma.weapons.begin(), ma.weapons.end(),
            std::back_inserter( weapons ), []( const itype_id & wid )-> std::string {
                if( item::nname( wid ) == get_player_character().weapon.display_name() )
                {
                    return colorize( item::nname( wid ) + _( " (wielded)" ), c_light_cyan );
                } else
                {
                    return item::nname( wid );
                } } );
            // Sorting alphabetically makes it easier to find a specific weapon
            std::sort( weapons.begin(), weapons.end(), localized_compare );
            // This removes duplicate names (e.g. a real weapon and a replica sharing the same name) from the weapon list.
            auto last = std::unique( weapons.begin(), weapons.end() );
            weapons.erase( last, weapons.end() );

            buffer += vgettext( "<bold>Weapon:</bold>", "<bold>Weapons:</bold>",
                                weapons.size() ) + std::string( " " );
            buffer += enumerate_as_string( weapons );
        }

        catacurses::window w;

        const std::string text = replace_colors( buffer );
        int width = 0;
        int height = 0;
        int iLines = 0;
        int selected = 0;

        ui_adaptor ui;
        ui.on_screen_resize( [&]( ui_adaptor & ui ) {
            w = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                    point( TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0,
                                           TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0 ) );

            width = FULL_SCREEN_WIDTH - 4;
            height = FULL_SCREEN_HEIGHT - 2;

            const auto vFolded = foldstring( text, width );
            iLines = vFolded.size();

            if( iLines < height ) {
                selected = 0;
            } else if( selected >= iLines - height ) {
                selected = iLines - height;
            }

            ui.position_from_window( w );
        } );
        ui.mark_resize();

        input_context ict;
        ict.register_action( "UP" );
        ict.register_action( "DOWN" );
        ict.register_action( "PAGE_UP" );
        ict.register_action( "PAGE_DOWN" );
        ict.register_action( "QUIT" );
        ict.register_action( "HELP_KEYBINDINGS" );

        ui.on_redraw( [&]( const ui_adaptor & ) {
            werase( w );
            fold_and_print_from( w, point( 2, 1 ), width, selected, c_light_gray, text );
            draw_border( w, BORDER_COLOR, string_format( _( " Style: %s " ), ma.name ) );
            draw_scrollbar( w, selected, height, iLines, point_south, BORDER_COLOR, true );
            wnoutrefresh( w );
        } );

        do {
            if( selected < 0 ) {
                selected = 0;
            } else if( iLines < height ) {
                selected = 0;
            } else if( selected >= iLines - height ) {
                selected = iLines - height;
            }

            ui_manager::redraw();
            const int scroll_lines = catacurses::getmaxy( w ) - 4;
            std::string action = ict.handle_input();

            if( action == "QUIT" ) {
                break;
            } else if( action == "DOWN" ) {
                selected++;
            } else if( action == "UP" ) {
                selected--;
            } else if( action == "PAGE_DOWN" ) {
                selected += scroll_lines;
            } else if( action == "PAGE_UP" ) {
                selected -= scroll_lines;
            }
        } while( true );
    }
    return true;
}
