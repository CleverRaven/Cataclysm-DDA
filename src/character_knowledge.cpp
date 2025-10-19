#include "character.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "bionics.h"
#include "calendar.h"
#include "character_martial_arts.h"
#include "construction.h"
#include "coordinates.h"
#include "creature.h"
#include "debug.h"
#include "enum_traits.h"
#include "enums.h"
#include "flag.h"
#include "item.h"
#include "itype.h"
#include "magic.h"
#include "magic_enchantment.h"
#include "map.h"
#include "messages.h"
#include "monster.h"
#include "mtype.h"
#include "pimpl.h"
#include "rng.h"
#include "skill.h"
#include "string_formatter.h"
#include "translations.h"
#include "trap.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"
#include "vehicle.h"
#include "vpart_position.h"

static const bionic_id bio_memory( "bio_memory" );

static const efftype_id effect_contacts( "contacts" );
static const efftype_id effect_transition_contacts( "transition_contacts" );

static const flag_id json_flag_FIX_FARSIGHT( "FIX_FARSIGHT" );

static const itype_id itype_cookbook_human( "cookbook_human" );

static const json_character_flag json_flag_ENHANCED_VISION( "ENHANCED_VISION" );
static const json_character_flag json_flag_HYPEROPIC( "HYPEROPIC" );
static const json_character_flag json_flag_PRED2( "PRED2" );
static const json_character_flag json_flag_PRED3( "PRED3" );
static const json_character_flag json_flag_PRED4( "PRED4" );
static const json_character_flag json_flag_READ_IN_DARKNESS( "READ_IN_DARKNESS" );

static const skill_id skill_speech( "speech" );

static const trait_id trait_CANNIBAL( "CANNIBAL" );
static const trait_id trait_HATES_BOOKS( "HATES_BOOKS" );
static const trait_id trait_ILLITERATE( "ILLITERATE" );
static const trait_id trait_LOVES_BOOKS( "LOVES_BOOKS" );
static const trait_id trait_PROF_DICEMASTER( "PROF_DICEMASTER" );
static const trait_id trait_PSYCHOPATH( "PSYCHOPATH" );
static const trait_id trait_SAPIOVORE( "SAPIOVORE" );
static const trait_id trait_SPIRITUAL( "SPIRITUAL" );

std::vector<matype_id> Character::known_styles( bool teachable_only ) const
{
    return martial_arts_data->get_known_styles( teachable_only );
}

bool Character::has_martialart( const matype_id &m ) const
{
    return martial_arts_data->has_martialart( m );
}

void Character::handle_skill_warning( const skill_id &id, bool force_warning )
{
    //remind the player intermittently that no skill gain takes place
    if( is_avatar() && ( force_warning || one_in( 5 ) ) ) {
        SkillLevel &level = get_skill_level_object( id );

        const Skill &skill = id.obj();
        std::string skill_name = skill.name();
        int curLevel = level.level();

        add_msg( m_info, _( "This task is too simple to train your %s beyond %d." ),
                 skill_name, curLevel );
    }
}

std::vector<skill_id> Character::skills_offered_to( const Character *you ) const
{
    std::vector<skill_id> ret;
    for( const auto &pair : *_skills ) {
        const skill_id &id = pair.first;
        if( id->is_teachable() && pair.second.level() > 0 &&
            ( !you || you->get_knowledge_level( id ) < pair.second.level() ) ) {
            ret.push_back( id );
        }
    }
    return ret;
}

std::vector<matype_id> Character::styles_offered_to( const Character *you ) const
{
    if( you ) {
        return you->martial_arts_data->get_unknown_styles( *martial_arts_data, true );
    }
    return martial_arts_data->get_known_styles( true );
}

std::vector<spell_id> Character::spells_offered_to( const Character *you ) const
{
    std::vector<spell_id> teachable;
    for( const spell_id &sp : magic->spells() ) {
        const spell &teacher_spell = magic->get_spell( sp );
        if( sp->teachable && ( !you || you->magic->can_learn_spell( *you, sp ) ) ) {
            if( you && you->magic->knows_spell( sp ) ) {
                const spell &student_spell = you->magic->get_spell( sp );
                if( student_spell.is_max_level( *you ) ||
                    student_spell.get_level() >= teacher_spell.get_level() ) {
                    continue;
                }
            }
            teachable.emplace_back( sp );
        }
    }
    return teachable;
}

void Character::zero_all_skills()
{
    for( Skill &skill : Skill::skills ) {
        set_skill_level( skill.ident(), 0 );
    }
}

SkillLevelMap Character::get_all_skills() const
{
    SkillLevelMap skills = *_skills;
    for( std::pair<const skill_id, SkillLevel> &sk : skills ) {
        sk.second.level( std::round( enchantment_cache->modify_value( sk.first, sk.second.level() ) ) );
    }
    return skills;
}

const SkillLevel &Character::get_skill_level_object( const skill_id &ident ) const
{
    return _skills->get_skill_level_object( ident );
}

SkillLevel &Character::get_skill_level_object( const skill_id &ident )
{
    return _skills->get_skill_level_object( ident );
}

float Character::get_skill_level( const skill_id &ident ) const
{
    return enchantment_cache->modify_value( ident,
                                            _skills->get_skill_level( ident ) + _skills->get_progress_level( ident ) );
}

float Character::get_skill_level( const skill_id &ident, const item &context ) const
{
    return enchantment_cache->modify_value( ident, _skills->get_skill_level( ident,
                                            context ) + _skills->get_progress_level( ident,
                                                    context ) );
}

int Character::get_knowledge_level( const skill_id &ident ) const
{
    return _skills->get_knowledge_level( ident );
}

float Character::get_knowledge_plus_progress( const skill_id &ident ) const
{
    return _skills->get_knowledge_level( ident ) + _skills->get_knowledge_progress_level( ident );
}

int Character::get_knowledge_level( const skill_id &ident, const item &context ) const
{
    return _skills->get_knowledge_level( ident, context );
}

float Character::get_average_skill_level( const skill_id &ident ) const
{
    return ( get_skill_level( ident ) + get_knowledge_plus_progress( ident ) ) / 2;
}

float Character::get_greater_skill_or_knowledge_level( const skill_id &ident ) const
{
    return get_skill_level( ident ) > get_knowledge_plus_progress( ident ) ? get_skill_level(
               ident ) : get_knowledge_plus_progress( ident );
}

void Character::set_skill_level( const skill_id &ident, const int level )
{
    get_skill_level_object( ident ).level( level );
}

void Character::mod_skill_level( const skill_id &ident, const int delta )
{
    _skills->mod_skill_level( ident, delta );
}
void Character::set_knowledge_level( const skill_id &ident, const int level )
{
    get_skill_level_object( ident ).knowledgeLevel( level );
}

void Character::mod_knowledge_level( const skill_id &ident, const int delta )
{
    _skills->mod_knowledge_level( ident, delta );
}

int Character::read_speed() const
{
    // Stat window shows stat effects on based on current stat
    const float intel = 2.0f + get_int() / 8.0f;
    // 4 int = 72 seconds, 8 int = 60 seconds, 12 int = 51 seconds, 16 int = 45 seconds, 20 int = 40 seconds
    /** @EFFECT_INT affects reading speed by an decreasing amount the higher intelligence goes, intially about 9% per point at 4 int to lower than 4% at 20+ int */
    time_duration ret = 180_seconds / intel;

    ret = enchantment_cache->modify_value( enchant_vals::mod::READING_SPEED_MULTIPLIER, ret );
    if( ret < 1_seconds ) {
        ret = 1_seconds;
    }
    return ret * 100 / 1_minutes;
}

bool Character::meets_skill_requirements( const std::map<skill_id, int> &req,
        const item &context ) const
{
    return _skills->meets_skill_requirements( req, context );
}

bool Character::meets_skill_requirements( const construction &con ) const
{
    return std::all_of( con.required_skills.begin(), con.required_skills.end(),
    [&]( const std::pair<skill_id, int> &pr ) {
        return get_knowledge_level( pr.first ) >= pr.second;
    } );
}

void Character::do_skill_rust()
{
    for( std::pair<const skill_id, SkillLevel> &pair : *_skills ) {
        const Skill &aSkill = *pair.first;
        SkillLevel &skill_level_obj = pair.second;

        if( aSkill.is_combat_skill() &&
            ( ( has_flag( json_flag_PRED2 ) && calendar::once_every( 8_hours ) ) ||
              ( has_flag( json_flag_PRED3 ) && calendar::once_every( 4_hours ) ) ||
              ( has_flag( json_flag_PRED4 ) && calendar::once_every( 3_hours ) ) ) ) {
            // Their brain is optimized to remember this
            if( one_in( 13 ) ) {
                // They've already passed the roll to avoid rust at
                // this point, but print a message about it now and
                // then.
                //
                // 13 combat skills.
                // This means PRED2/PRED3/PRED4 think of hunting on
                // average every 8/4/3 hours, enough for immersion
                // without becoming an annoyance.
                //
                add_msg_if_player( _( "Your heart races as you recall your most recent hunt." ) );
                mod_stim( 1 );
            }
            continue;
        }

        const int rust_resist = enchantment_cache->get_value_add( enchant_vals::mod::SKILL_RUST_RESIST );
        const float rust_resist_mult = 1.0f + enchantment_cache->get_value_multiply(
                                           enchant_vals::mod::SKILL_RUST_RESIST );
        if( skill_level_obj.rust( rust_resist, rust_resist_mult ) ) {
            mod_power_level( -bio_memory->power_trigger );
        }
    }
}

bool Character::knows_creature_type( const Creature *c ) const
{
    if( const monster *mon = dynamic_cast<const monster *>( c ) ) {
        return knows_creature_type( mon->type->id );
    }
    return false;
}

bool Character::knows_creature_type( const mtype_id &c ) const
{
    return known_monsters.count( c ) > 0;
}

void Character::set_knows_creature_type( const Creature *c )
{
    if( const monster *mon = dynamic_cast<const monster *>( c ) ) {
        set_knows_creature_type( mon->type->id );
    }
}

void Character::set_knows_creature_type( const mtype_id &c )
{
    known_monsters.emplace( c );
}

bool Character::knows_trap( const tripoint_bub_ms &pos ) const
{
    const tripoint_abs_ms p = get_map().get_abs( pos );
    return known_traps.count( p ) > 0;
}

void Character::add_known_trap( const tripoint_bub_ms &pos, const trap &t )
{
    const tripoint_abs_ms p = get_map().get_abs( pos );
    if( t.is_null() ) {
        known_traps.erase( p );
    } else {
        // TODO: known_traps should map to a trap_str_id
        known_traps[p] = t.id.str();
    }
}

int Character::persuade_skill() const
{
    /** @EFFECT_INT slightly increases talking skill */
    /** @EFFECT_PER slightly increases talking skill */
    /** @EFFECT_SPEECH increases talking skill */
    float ret = get_int() + get_per() + get_skill_level( skill_speech ) * 3;
    ret = enchantment_cache->modify_value( enchant_vals::mod::SOCIAL_PERSUADE, ret );
    return round( ret );
}

int Character::lie_skill() const
{
    /** @EFFECT_INT slightly increases talking skill */
    /** @EFFECT_PER slightly increases talking skill */
    /** @EFFECT_SPEECH increases talking skill */
    float ret = get_int() + get_per() + get_skill_level( skill_speech ) * 3;
    ret = enchantment_cache->modify_value( enchant_vals::mod::SOCIAL_LIE, ret );
    return round( ret );
}

book_mastery Character::get_book_mastery( const item &book ) const
{
    if( !book.is_book() || !has_identified( book.typeId() ) ) {
        return book_mastery::CANT_DETERMINE;
    }
    // TODO: add illiterate check?

    const cata::value_ptr<islot_book> &type = book.type->book;
    const skill_id &skill = type->skill;

    if( !skill ) {
        // book gives no skill
        return book_mastery::MASTERED;
    }

    const int skill_level = get_knowledge_level( skill );
    const int skill_requirement = type->req;
    const int max_skill_learnable = type->level;

    if( skill_requirement > skill_level ) {
        return book_mastery::CANT_UNDERSTAND;
    }
    if( skill_level >= max_skill_learnable ) {
        return book_mastery::MASTERED;
    }
    return book_mastery::LEARNING;
}

bool Character::fun_to_read( const item &book ) const
{
    return book_fun_for( book, *this ) > 0;
}

int Character::book_fun_for( const item &book, const Character &p ) const
{
    int fun_bonus = book.type->book->fun;
    if( !book.is_book() ) {
        debugmsg( "called avatar::book_fun_for with non-book" );
        return 0;
    }

    // If you don't have a problem with eating humans, To Serve Man becomes rewarding
    if( ( p.has_trait( trait_CANNIBAL ) || p.has_trait( trait_PSYCHOPATH ) ||
          p.has_trait( trait_SAPIOVORE ) ) &&
        book.typeId() == itype_cookbook_human ) {
        fun_bonus = std::abs( fun_bonus );
    } else if( p.has_trait( trait_SPIRITUAL ) && book.has_flag( flag_INSPIRATIONAL ) ) {
        fun_bonus = std::abs( fun_bonus * 3 );
    }

    if( has_trait( trait_LOVES_BOOKS ) ) {
        fun_bonus++;
    } else if( has_trait( trait_HATES_BOOKS ) ) {
        if( book.type->book->fun > 0 ) {
            fun_bonus = 0;
        } else {
            fun_bonus--;
        }
    }

    if( fun_bonus > 1 && book.get_chapters() > 0 && book.get_remaining_chapters( p ) == 0 ) {
        fun_bonus /= 2;
    }

    return fun_bonus;
}

read_condition_result Character::check_read_condition( const item &book ) const
{
    map &here = get_map();

    read_condition_result result = read_condition_result::SUCCESS;
    if( !book.is_book() ) {
        result |= read_condition_result::NOT_BOOK;
    } else {
        const optional_vpart_position vp = here.veh_at( pos_bub() );
        if( vp && vp->vehicle().player_in_control( here, *this ) ) {
            result |= read_condition_result::DRIVING;
        }

        if( !fun_to_read( book ) && !has_morale_to_read() && has_identified( book.typeId() ) ) {
            result |= read_condition_result::MORALE_LOW;
        }

        const book_mastery mastery = get_book_mastery( book );
        if( mastery == book_mastery::CANT_UNDERSTAND ) {
            result |= read_condition_result::CANT_UNDERSTAND;
        }
        if( mastery == book_mastery::MASTERED ) {
            result |= read_condition_result::MASTERED;
        }

        const bool book_requires_intelligence = book.type->book->intel > 0;
        if( book_requires_intelligence && has_trait( trait_ILLITERATE ) ) {
            result |= read_condition_result::ILLITERATE;
        }
        if( has_flag( json_flag_HYPEROPIC ) &&
            !worn_with_flag( json_flag_FIX_FARSIGHT ) &&
            !has_effect( effect_contacts ) &&
            !has_effect( effect_transition_contacts ) &&
            !has_flag( json_flag_ENHANCED_VISION ) ) {
            result |= read_condition_result::NEED_GLASSES;
        }
        if( fine_detail_vision_mod() > 4 && !book.has_flag( flag_CAN_USE_IN_DARK ) ) {
            result |= read_condition_result::TOO_DARK;
        }
        if( is_blind() ) {
            result |= read_condition_result::BLIND;
        }
    }
    return result;
}

const Character *Character::get_book_reader( const item &book,
        std::vector<std::string> &reasons ) const
{
    const map &here = get_map();

    const Character *reader = nullptr;

    if( !book.is_book() ) {
        reasons.push_back( is_avatar() ? string_format( _( "Your %s is not good reading material." ),
                           book.tname() ) :
                           string_format( _( "The %s is not good reading material." ), book.tname() )
                         );
        return nullptr;
    }

    const cata::value_ptr<islot_book> &type = book.type->book;
    const skill_id &book_skill = type->skill;
    const int book_skill_requirement = type->req;

    // Check for conditions that immediately disqualify the player from reading:
    read_condition_result condition = check_read_condition( book );
    if( condition & read_condition_result::DRIVING ) {
        reasons.emplace_back( _( "It's a bad idea to read while driving!" ) );
        return nullptr;
    }
    if( condition & read_condition_result::MORALE_LOW ) {
        // Low morale still permits skimming
        reasons.emplace_back( is_avatar() ?
                              _( "What's the point of studying?  (Your morale is too low!)" )  :
                              string_format( _( "What's the point of studying?  (%s's morale is too low!)" ), disp_name() ) );
        return nullptr;
    }
    if( condition & read_condition_result::CANT_UNDERSTAND ) {
        reasons.push_back( is_avatar() ? string_format( _( "%s %d needed to understand.  You have %d" ),
                           book_skill->name(), book_skill_requirement, get_knowledge_level( book_skill ) ) :
                           string_format( _( "%s %d needed to understand.  %s has %d" ), book_skill->name(),
                                          book_skill_requirement, disp_name(), get_knowledge_level( book_skill ) ) );
        return nullptr;
    }

    // Check for conditions that disqualify us only if no other Characters can read to us
    if( condition & read_condition_result::ILLITERATE ) {
        reasons.emplace_back( is_avatar() ? _( "You're illiterate!" ) : string_format(
                                  _( "%s is illiterate!" ), disp_name() ) );
    } else if( condition & read_condition_result::NEED_GLASSES ) {
        reasons.emplace_back( is_avatar() ? _( "Your eyes won't focus without reading glasses." ) :
                              string_format( _( "%s's eyes won't focus without reading glasses." ), disp_name() ) );
    } else if( condition & read_condition_result::TOO_DARK &&
               !has_flag( json_flag_READ_IN_DARKNESS ) && !book.has_flag( flag_CAN_USE_IN_DARK ) ) {
        // Too dark to read only applies if the player can read to himself
        reasons.emplace_back( _( "It's too dark to read!" ) );
        return nullptr;
    } else {
        return this;
    }

    if( ! is_avatar() ) {
        // NPCs are too proud to ask for help, perhaps someday they will not be
        return nullptr;
    }

    //Check for other Characters to read for you, negates Illiterate and Far Sighted
    //The fastest-reading Character is chosen
    if( is_deaf() ) {
        reasons.emplace_back( _( "Maybe someone could read that to you, but you're deaf!" ) );
        return nullptr;
    }

    time_duration time_taken = time_duration::from_turns( INT_MAX );
    std::vector<Character *> candidates = get_crafting_helpers();

    for( const Character *elem : candidates ) {
        // Check for disqualifying factors:
        condition = elem->check_read_condition( book );
        if( condition & read_condition_result::ILLITERATE ) {
            reasons.push_back( string_format( _( "%s is illiterate!" ),
                                              elem->disp_name() ) );
        } else if( condition & read_condition_result::CANT_UNDERSTAND ) {
            reasons.push_back( string_format( _( "%s %d needed to understand.  %s has %d" ),
                                              book_skill->name(), book_skill_requirement, elem->disp_name(),
                                              elem->get_knowledge_level( book_skill ) ) );
        } else if( condition & read_condition_result::NEED_GLASSES ) {
            reasons.push_back( string_format( _( "%s needs reading glasses!" ),
                                              elem->disp_name() ) );
        } else if( condition & read_condition_result::TOO_DARK ) {
            reasons.push_back( string_format(
                                   _( "It's too dark for %s to read!" ),
                                   elem->disp_name() ) );
        } else if( !elem->sees( here, *this ) ) {
            reasons.push_back( string_format( _( "%s could read that to you, but they can't see you." ),
                                              elem->disp_name() ) );
        } else if( condition & read_condition_result::MORALE_LOW ) {
            // Low morale still permits skimming
            reasons.push_back( string_format( _( "%s morale is too low!" ), elem->disp_name( true ) ) );
        } else if( condition & read_condition_result::BLIND ) {
            reasons.push_back( string_format( _( "%s is blind." ), elem->disp_name() ) );
        } else {
            time_duration proj_time = time_to_read( book, *elem );
            if( proj_time < time_taken ) {
                reader = elem;
                time_taken = proj_time;
            }
        }
    }
    //end for all candidates
    return reader;
}

time_duration Character::time_to_read( const item &book, const Character &reader,
                                       const Character *learner ) const
{
    const auto &type = book.type->book;
    const skill_id &skill = type->skill;
    // The reader's reading speed has an effect only if they're trying to understand the book as they read it
    // Reading speed is assumed to be how well you learn from books (as opposed to hands-on experience)
    const bool try_understand = reader.fun_to_read( book ) ||
                                reader.get_knowledge_level( skill ) < type->level;
    int reading_speed = try_understand ? std::max( reader.read_speed(), read_speed() ) : read_speed();
    if( learner ) {
        reading_speed = std::max( reading_speed, learner->read_speed() );
    }

    time_duration retval = type->time * reading_speed / 100;
    retval *= std::min( fine_detail_vision_mod(), reader.fine_detail_vision_mod() );

    const int effective_int = std::min( { get_int(), reader.get_int(), learner ? learner->get_int() : INT_MAX } );
    if( type->intel > effective_int && !reader.has_trait( trait_PROF_DICEMASTER ) ) {
        retval += type->time * ( time_duration::from_seconds( type->intel - effective_int ) / 1_minutes );
    }
    if( !has_identified( book.typeId() ) ) {
        //skimming
        retval /= 10;
    }
    return retval;
}
