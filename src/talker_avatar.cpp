#include <memory>
#include <string>

#include "avatar.h"
#include "calendar.h"
#include "character.h"
#include "game.h"
#include "messages.h"
#include "monster.h"
#include "mtype.h"
#include "npc.h"
#include "npctrade.h"
#include "output.h"
#include "skill.h"
#include "talker.h"
#include "talker_avatar.h"
#include "translations.h"

static const efftype_id effect_pacified( "pacified" );
static const efftype_id effect_pet( "pet" );

static const itype_id itype_foodperson_mask( "foodperson_mask" );
static const itype_id itype_foodperson_mask_on( "foodperson_mask_on" );

static const trait_id trait_PROF_FOODP( "PROF_FOODP" );

talker_avatar::talker_avatar( avatar *new_me )
{
    me_chr = new_me;
    me_chr_const = new_me;
}

std::vector<std::string> talker_avatar::get_topics( bool )
{
    std::vector<std::string> add_topics;
    if( has_trait( trait_PROF_FOODP ) && !( is_wearing( itype_foodperson_mask ) ||
                                            is_wearing( itype_foodperson_mask_on ) ) ) {
        add_topics.emplace_back( "TALK_NOFACE" );
    }
    return add_topics;
}

int talker_avatar::parse_mod( const std::string &attribute, const int factor ) const
{
    int modifier = 0;
    if( attribute == "U_INTIMIDATE" ) {
        modifier = me_chr->intimidation();
    }
    modifier *= factor;
    return modifier;
}

int talker_avatar::trial_chance_mod( const std::string &trial_type ) const
{
    int chance = 0;
    const social_modifiers &me_mods = me_chr->get_mutation_bionic_social_mods();
    if( trial_type == "lie" ) {
        chance += me_chr->lie_skill() + me_mods.lie;
    } else if( trial_type == "persuade" ) {
        chance += me_chr->persuade_skill() + me_mods.persuade;
    } else if( trial_type == "intimidate" ) {
        chance += me_chr->intimidation() + me_mods.intimidate;
    }
    return chance;
}

std::vector<skill_id> talker_avatar::skills_offered_to( const talker &student ) const
{
    if( !student.get_character() ) {
        return {};
    }
    const Character &c = *student.get_character();
    std::vector<skill_id> ret;
    for( const auto &pair : *me_chr->_skills ) {
        const skill_id &id = pair.first;
        if( c.get_knowledge_level( id ) < pair.second.level() ) {
            ret.push_back( id );
        }
    }
    return ret;
}

bool talker_avatar::buy_monster( talker &seller, const mtype_id &mtype, int cost,
                                 int count, bool pacified, const translation &name )
{
    npc *seller_guy = seller.get_npc();
    if( !seller_guy ) {
        popup( _( "%s can't sell you any %s" ), seller.disp_name(), mtype.obj().nname( 2 ) );
        return false;
    }
    if( cost > 0 && !npc_trading::pay_npc( *seller_guy, cost ) ) {
        popup( _( "You can't afford it!" ) );
        return false;
    }

    for( int i = 0; i < count; i++ ) {
        monster *const mon_ptr = g->place_critter_around( mtype, me_chr->pos(), 3 );
        if( !mon_ptr ) {
            add_msg_debug( debugmode::DF_TALKER, "Cannot place u_buy_monster, no valid placement locations." );
            break;
        }
        monster &tmp = *mon_ptr;
        // Our monster is always a pet.
        tmp.friendly = -1;
        tmp.add_effect( effect_pet, 1_turns, true );

        if( pacified ) {
            tmp.add_effect( effect_pacified, 1_turns, true );
        }

        if( !name.empty() ) {
            tmp.unique_name = name.translated();
        }

    }

    if( name.empty() ) {
        popup( _( "%1$s gives you %2$d %3$s." ), seller_guy->get_name(),
               count, mtype.obj().nname( count ) );
    } else {
        popup( _( "%1$s gives you %2$s." ), seller_guy->get_name(), name );
    }
    return true;
}
