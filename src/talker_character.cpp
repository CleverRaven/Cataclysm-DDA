#include <memory>

#include "character_id.h"
#include "item.h"
#include "magic.h"
#include "npc.h"
#include "pimpl.h"
#include "player.h"
#include "player_activity.h"
#include "point.h"
#include "talker_character.h"
#include "vehicle.h"

class time_duration;
static const trait_id trait_SEESLEEP( "SEESLEEP" );

std::string talker_character::disp_name() const
{
    return me_chr->disp_name();
}

character_id talker_character::getID() const
{
    return me_chr->getID();
}

bool talker_character::is_male() const
{
    return me_chr->male;
}

std::vector<std::string> talker_character::get_grammatical_genders() const
{
    return me_chr->get_grammatical_genders();
}

int talker_character::posx() const
{
    return me_chr->posx();
}

int talker_character::posy() const
{
    return me_chr->posy();
}

int talker_character::posz() const
{
    return me_chr->posz();
}

tripoint talker_character::pos() const
{
    return me_chr->pos();
}

tripoint_abs_omt talker_character::global_omt_location() const
{
    return me_chr->global_omt_location();
}

void talker_character::set_pos( tripoint new_pos )
{
    me_chr->setpos( new_pos );
}

int talker_character::str_cur() const
{
    return me_chr->str_cur;
}

int talker_character::dex_cur() const
{
    return me_chr->dex_cur;
}

int talker_character::int_cur() const
{
    return me_chr->int_cur;
}

int talker_character::per_cur() const
{
    return me_chr->per_cur;
}

void talker_character::set_str_max( int value )
{
    me_chr->str_max = value;
}

void talker_character::set_dex_max( int value )
{
    me_chr->dex_max = value;
}

void talker_character::set_int_max( int value )
{
    me_chr->int_max = value;
}

void talker_character::set_per_max( int value )
{
    me_chr->per_max = value;
}

int talker_character::get_str_max()
{
    return me_chr->str_max;
}

int talker_character::get_dex_max()
{
    return me_chr->dex_max;
}

int talker_character::get_int_max()
{
    return me_chr->int_max;
}

int talker_character::get_per_max()
{
    return me_chr->per_max;
}

bool talker_character::has_trait( const trait_id &trait_to_check ) const
{
    return me_chr->has_trait( trait_to_check );
}

bool talker_character::is_deaf() const
{
    return me_chr->is_deaf();
}

bool talker_character::is_mute() const
{
    return me_chr->is_mute();
}

void talker_character::set_mutation( const trait_id &new_trait )
{
    me_chr->set_mutation( new_trait );
}

void talker_character::unset_mutation( const trait_id &old_trait )
{
    me_chr->unset_mutation( old_trait );
}

bool talker_character::has_trait_flag( const json_character_flag &trait_flag_to_check ) const
{
    return me_chr->has_trait_flag( trait_flag_to_check );
}

bool talker_character::crossed_threshold() const
{
    return me_chr->crossed_threshold();
}

int talker_character::num_bionics() const
{
    return me_chr->num_bionics();
}

bool talker_character::has_max_power() const
{
    return me_chr->has_max_power();
}

void talker_character::set_power_cur( units::energy value )
{
    me_chr->set_power_level( value );
}

bool talker_character::has_bionic( const bionic_id &bionics_id ) const
{
    return me_chr->has_bionic( bionics_id );
}

bool talker_character::knows_spell( const spell_id &sp ) const
{
    return me_chr->magic->knows_spell( sp );
}

int talker_character::get_skill_level( const skill_id &skill ) const
{
    return me_chr->get_skill_level( skill );
}

void talker_character::set_skill_level( const skill_id &skill, int value )
{
    me_chr->set_skill_level( skill, value );
}

bool talker_character::knows_proficiency( const proficiency_id &proficiency ) const
{
    return me_chr->has_proficiency( proficiency );
}

bool talker_character::has_effect( const efftype_id &effect_id ) const
{
    return me_chr->has_effect( effect_id );
}

void talker_character::add_effect( const efftype_id &new_effect, const time_duration &dur,
                                   std::string bp, bool permanent, bool force, int intensity )
{
    bodypart_id target_part;
    if( "RANDOM" == bp ) {
        target_part = get_player_character().random_body_part( true );
    } else {
        target_part = bodypart_str_id( bp );
    }

    me_chr->add_effect( new_effect, dur, target_part, permanent, intensity, force );
}

void talker_character::remove_effect( const efftype_id &old_effect )
{
    me_chr->remove_effect( old_effect );
}

std::string talker_character::get_value( const std::string &var_name ) const
{
    return me_chr->get_value( var_name );
}

void talker_character::set_value( const std::string &var_name, const std::string &value )
{
    me_chr->set_value( var_name, value );
}

void talker_character::remove_value( const std::string &var_name )
{
    me_chr->remove_value( var_name );
}

bool talker_character::is_wearing( const itype_id &item_id ) const
{
    return me_chr->is_wearing( item_id );
}

int talker_character::charges_of( const itype_id &item_id ) const
{
    return me_chr->charges_of( item_id );
}

bool talker_character::has_charges( const itype_id &item_id, int count ) const
{
    return me_chr->has_charges( item_id, count );
}

std::list<item> talker_character::use_charges( const itype_id &item_name, const int count )
{
    return me_chr->use_charges( item_name, count );
}

std::list<item> talker_character::use_amount( const itype_id &item_name, const int count )
{
    return me_chr->use_amount( item_name, count );
}

bool talker_character::has_amount( const itype_id &item_id, int count ) const
{
    return me_chr->has_amount( item_id, count );
}

int talker_character::get_amount( const itype_id &item_id ) const
{
    return me_chr->amount_of( item_id );
}

int talker_character::cash() const
{
    return me_chr->cash;
}

std::vector<item *> talker_character::items_with( const std::function<bool( const item & )>
        &filter ) const
{
    return me_chr->items_with( filter );
}

void talker_character::i_add( const item &new_item )
{
    me_chr->i_add( new_item );
}

void talker_character::remove_items_with( const std::function<bool( const item & )> &filter )
{
    me_chr->remove_items_with( filter );
}

bool talker_character::unarmed_attack() const
{
    return me_chr->unarmed_attack();
}

bool talker_character::can_stash_weapon() const
{
    return me_chr->can_pickVolume( *me_chr->get_wielded_item() );
}

bool talker_character::has_stolen_item( const talker &guy ) const
{
    const player *owner = guy.get_character();
    if( owner ) {
        for( auto &elem : me_chr->inv_dump() ) {
            if( elem->is_old_owner( *owner, true ) ) {
                return true;
            }
        }
    }
    return false;
}

faction *talker_character::get_faction() const
{
    return me_chr->get_faction();
}

std::string talker_character::short_description() const
{
    return me_chr->short_description();
}

bool talker_character::has_activity() const
{
    return !me_chr->activity.is_null();
}

bool talker_character::is_mounted() const
{
    return me_chr->is_mounted();
}

int talker_character::get_fatigue() const
{
    return me_chr->get_fatigue();
}

int talker_character::get_hunger() const
{
    return me_chr->get_hunger();
}

int talker_character::get_thirst() const
{
    return me_chr->get_thirst();
}

int talker_character::get_stored_kcal() const
{
    return me_chr->get_stored_kcal();
}

void talker_character::set_stored_kcal( int value )
{
    me_chr->set_stored_kcal( value );
}
void talker_character::set_thirst( int value )
{
    me_chr->set_thirst( value );
}

bool talker_character::is_in_control_of( const vehicle &veh ) const
{
    return veh.player_in_control( *me_chr );
}

void talker_character::shout( const std::string &speech, bool order )
{
    me_chr->shout( speech, order );
}

int talker_character::pain_cur() const
{
    return me_chr->get_pain();
}

void talker_character::mod_pain( int amount )
{
    me_chr->mod_pain( amount );
}

bool talker_character::worn_with_flag( const flag_id &flag ) const
{
    return me_chr->worn_with_flag( flag );
}

bool talker_character::wielded_with_flag( const flag_id &flag ) const
{
    return me_chr->get_wielded_item()->has_flag( flag );
}

units::energy talker_character::power_cur() const
{
    return me_chr->get_power_level();
}

units::energy talker_character::power_max() const
{
    return me_chr->get_max_power_level();
}

int talker_character::mana_cur() const
{
    return me_chr->magic->available_mana();
}

int talker_character::mana_max() const
{
    return me_chr->magic->max_mana( *me_chr );
}

void talker_character::set_mana_cur( int value )
{
    me_chr->magic->set_mana( value );
}

bool talker_character::can_see() const
{
    return !me_chr->is_blind() && ( !me_chr->in_sleep_state() || me_chr->has_trait( trait_SEESLEEP ) );
}

void talker_character::mod_fatigue( int amount )
{
    me_chr->mod_fatigue( amount );
}

void talker_character::mod_healthy_mod( int amount, int cap )
{
    me_chr->mod_healthy_mod( amount, cap );
}

int talker_character::morale_cur() const
{
    return me_chr->get_morale_level();
}

void talker_character::add_morale( const morale_type &new_morale, int bonus, int max_bonus,
                                   time_duration duration, time_duration decay_start, bool capped )
{
    me_chr->add_morale( new_morale, bonus, max_bonus, duration, decay_start, capped );
}

void talker_character::remove_morale( const morale_type &old_morale )
{
    me_chr->rem_morale( old_morale );
}

int talker_character::focus_cur() const
{
    return me_chr->get_focus();
}

void talker_character::mod_focus( int amount )
{
    me_chr->mod_focus( amount );
}

void talker_character::mod_rad( int amount )
{
    me_chr->mod_rad( amount );
}
