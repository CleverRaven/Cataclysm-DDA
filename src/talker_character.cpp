#include <algorithm>
#include <bitset>
#include <cmath>
#include <cstddef>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <tuple>
#include <unordered_set>
#include <utility>

#include "addiction.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character_attire.h"
#include "character_id.h"
#include "character_martial_arts.h"
#include "coordinates.h"
#include "creature.h"
#include "damage.h"
#include "debug.h"
#include "effect.h"
#include "faction.h"
#include "item.h"
#include "item_location.h"
#include "itype.h"
#include "magic.h"
#include "map.h"
#include "martialarts.h"
#include "math_parser_diag_value.h"
#include "messages.h"
#include "npc.h"
#include "npctalk.h"
#include "output.h"
#include "pimpl.h"
#include "player_activity.h"
#include "proficiency.h"
#include "ret_val.h"
#include "skill.h"
#include "string_formatter.h"
#include "talker_character.h"
#include "translation.h"
#include "translations.h"
#include "units.h"
#include "vehicle.h"
#include "weather.h"

struct bionic;

static const flag_id json_flag_FIT( "FIT" );
static const json_character_flag json_flag_SEESLEEP( "SEESLEEP" );

std::string talker_character_const::disp_name() const
{
    return me_chr_const->disp_name();
}

std::string talker_character_const::get_name() const
{
    return me_chr_const->get_name();
}

character_id talker_character_const::getID() const
{
    return me_chr_const->getID();
}

bool talker_character_const::is_male() const
{
    return me_chr_const->male;
}

std::vector<std::string> talker_character_const::get_grammatical_genders() const
{
    return me_chr_const->get_grammatical_genders();
}

int talker_character_const::posx( const map &here ) const
{
    return me_chr_const->posx( here );
}

int talker_character_const::posy( const map &here ) const
{
    return me_chr_const->posy( here );
}

int talker_character_const::posz() const
{
    return me_chr_const->posz();
}

tripoint_bub_ms talker_character_const::pos_bub( const map &here ) const
{
    return me_chr_const->pos_bub( here );
}

tripoint_abs_ms talker_character_const::pos_abs() const
{
    return me_chr_const->pos_abs();
}

tripoint_abs_omt talker_character_const::pos_abs_omt() const
{
    return me_chr_const->pos_abs_omt();
}

int talker_character_const::get_cur_hp( const bodypart_id &bp ) const
{
    return me_chr_const->get_hp( bp );
}

int talker_character_const::get_hp_max( const bodypart_id &bp ) const
{
    return me_chr_const->get_hp_max( bp );
}

units::temperature talker_character_const::get_cur_part_temp( const bodypart_id &bp ) const
{
    return me_chr_const->get_part_temp_conv( bp );
}

int talker_character_const::str_cur() const
{
    return me_chr_const->str_cur;
}

int talker_character_const::dex_cur() const
{
    return me_chr_const->dex_cur;
}

int talker_character_const::int_cur() const
{
    return me_chr_const->int_cur;
}

int talker_character_const::per_cur() const
{
    return me_chr_const->per_cur;
}

int talker_character_const::attack_speed() const
{
    item_location cur_weapon = me_chr_const->used_weapon();
    item cur_weap = cur_weapon ? *cur_weapon : null_item_reference();
    return me_chr_const->attack_speed( cur_weap );
}

int talker_character_const::get_speed() const
{
    return me_chr_const->get_speed();
}

dealt_damage_instance talker_character_const::deal_damage( Creature *source, bodypart_id bp,
        const damage_instance &dam ) const
{
    return source->deal_damage( source, bp, dam );
}

void talker_character::set_pos( tripoint_bub_ms new_pos )
{
    map &here = get_map();

    me_chr->setpos( here, new_pos );
}

void talker_character::set_pos( tripoint_abs_ms new_pos )
{
    me_chr->setpos( new_pos );
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

void talker_character::set_str_bonus( int value )
{
    me_chr->mod_str_bonus( value );
}

void talker_character::set_dex_bonus( int value )
{
    me_chr->mod_dex_bonus( value );
}

void talker_character::set_int_bonus( int value )
{
    me_chr->mod_int_bonus( value );
}

void talker_character::set_per_bonus( int value )
{
    me_chr->mod_per_bonus( value );
}

void talker_character::set_cash( int value )
{
    me_chr->cash = value;
}

int talker_character_const::get_str_max() const
{
    return me_chr_const->str_max;
}

int talker_character_const::get_dex_max() const
{
    return me_chr_const->dex_max;
}

int talker_character_const::get_int_max() const
{
    return me_chr_const->int_max;
}

int talker_character_const::get_per_max() const
{
    return me_chr_const->per_max;
}

int talker_character_const::get_str_bonus() const
{
    return me_chr_const->get_str_bonus();
}

int talker_character_const::get_dex_bonus() const
{
    return me_chr_const->get_dex_bonus();
}

int talker_character_const::get_int_bonus() const
{
    return me_chr_const->get_int_bonus();
}

int talker_character_const::get_per_bonus() const
{
    return me_chr_const->get_per_bonus();
}

bool talker_character_const::has_trait( const trait_id &trait_to_check ) const
{
    return me_chr_const->has_trait( trait_to_check );
}

int talker_character_const::get_total_in_category( const mutation_category_id &categ,
        mut_count_type count_type ) const
{
    return me_chr_const->get_total_in_category( categ, count_type );
}

int talker_character_const::get_total_in_category_char_has( const mutation_category_id &categ,
        mut_count_type count_type ) const
{
    return me_chr_const->get_total_in_category_char_has( categ, count_type );
}

bool talker_character_const::is_trait_purifiable( const trait_id &trait_to_check ) const
{
    return me_chr_const->purifiable( trait_to_check );
}

bool talker_character_const::has_recipe( const recipe_id &recipe_to_check ) const
{
    return me_chr_const->knows_recipe( &*recipe_to_check );
}

void talker_character::learn_recipe( const recipe_id &recipe_to_learn )
{
    me_chr->learn_recipe( &*recipe_to_learn );
}

void talker_character::forget_recipe( const recipe_id &recipe_to_forget )
{
    me_chr->forget_recipe( &*recipe_to_forget );
}

bool talker_character_const::is_deaf() const
{
    return me_chr_const->is_deaf();
}

bool talker_character_const::is_mute() const
{
    return me_chr_const->is_mute();
}

void talker_character::mutate( const int &highest_cat_chance, const bool &use_vitamins )
{
    me_chr->mutate( highest_cat_chance, use_vitamins );
}

void talker_character::mutate_category( const mutation_category_id &mut_cat,
                                        const bool &use_vitamins, const bool &true_random )
{
    me_chr->mutate_category( mut_cat, use_vitamins, true_random );
}

void talker_character::mutate_towards( const trait_id &trait, const mutation_category_id &mut_cat,
                                       const bool &use_vitamins )
{
    me_chr->mutate_towards( trait, mut_cat, nullptr, use_vitamins );
}

void talker_character::set_trait_purifiability( const trait_id &trait, const bool &purifiable )
{
    // If we want to set it non-purifiable and we didn't already do that and we really do have the trait
    if( me_chr->has_trait( trait ) ) {
        if( !purifiable && !me_chr->my_intrinsic_mutations.count( trait ) ) {
            me_chr->my_intrinsic_mutations.insert( trait );
            add_msg_debug( debugmode::DF_MUTATION, "Setting trait %s unpurifiable", trait.c_str() );
        };
        // If we want to set it purifiable
        if( purifiable && me_chr->my_intrinsic_mutations.count( trait ) ) {
            me_chr->my_intrinsic_mutations.erase( trait );
            add_msg_debug( debugmode::DF_MUTATION, "Setting trait %s purifiable", trait.c_str() );
        }
    }
}

void talker_character::set_mutation( const trait_id &new_trait, const mutation_variant *variant )
{
    me_chr->set_mutation( new_trait, variant );
}

void talker_character::unset_mutation( const trait_id &old_trait )
{
    me_chr->unset_mutation( old_trait );
}

void talker_character::activate_mutation( const trait_id &trait )
{
    me_chr->activate_mutation( trait );
}

void talker_character::deactivate_mutation( const trait_id &trait )
{
    me_chr->deactivate_mutation( trait );
}

bool talker_character_const::has_flag( const json_character_flag &trait_flag_to_check ) const
{
    return me_chr_const->has_flag( trait_flag_to_check );
}

bool talker_character_const::has_species( const species_id &species ) const
{
    add_msg_debug( debugmode::DF_TALKER, "Character %s checked for species %s", me_chr_const->name,
                   species.c_str() );
    return me_chr_const->in_species( species );
}

bool talker_character_const::bodytype( const bodytype_id &bt ) const
{
    add_msg_debug( debugmode::DF_TALKER, "Character %s checked for bodytype %s", me_chr_const->name,
                   bt );
    // All characters are human-bodytyped for now
    // TODO: Change that for very limby characters
    return bt == "human";
}

bool talker_character_const::crossed_threshold() const
{
    return me_chr_const->crossed_threshold();
}

int talker_character_const::num_bionics() const
{
    return me_chr_const->num_bionics();
}

bool talker_character_const::has_max_power() const
{
    return me_chr_const->has_max_power();
}

void talker_character::set_power_cur( units::energy value )
{
    me_chr->set_power_level( value );
}

bool talker_character_const::has_bionic( const bionic_id &bionics_id ) const
{
    return me_chr_const->has_bionic( bionics_id );
}

bool talker_character_const::knows_spell( const spell_id &sp ) const
{
    return me_chr_const->magic->knows_spell( sp );
}

int talker_character_const::get_skill_level( const skill_id &skill ) const
{
    return me_chr_const->get_skill_level( skill );
}

void talker_character::set_skill_level( const skill_id &skill, int value )
{
    me_chr->set_skill_level( skill, value );
}

int talker_character_const::get_skill_exp( const skill_id &skill, bool raw ) const
{
    return me_chr_const->get_skill_level_object( skill ).exercise( raw );
}

void talker_character::set_skill_exp( const skill_id &skill, int value, bool raw )
{
    me_chr->get_skill_level_object( skill ).set_exercise( value, raw );
}

int talker_character_const::get_spell_level( const trait_id &spell_school ) const
{
    int spell_level = -1;
    for( const spell &sp : me_chr_const->spells_known_of_class( spell_school ) ) {
        spell_level = std::max( sp.get_effective_level(), spell_level );
    }
    return spell_level;
}

int talker_character_const::get_spell_level( const spell_id &spell_name ) const
{
    if( !me_chr_const->magic->knows_spell( spell_name ) ) {
        return -1;
    }
    return me_chr_const->magic->get_spell( spell_name ).get_effective_level();
}

int talker_character_const::get_highest_spell_level() const
{
    int spell_level = -1;
    for( const spell *sp : me_chr_const->magic->get_spells() ) {
        spell_level = std::max( sp->get_effective_level(), spell_level );
    }
    return spell_level;
}

int talker_character_const::get_spell_exp( const spell_id &spell_name ) const
{
    if( !me_chr_const->magic->knows_spell( spell_name ) ) {
        return -1;
    }
    return me_chr_const->magic->get_spell( spell_name ).xp();
}

int talker_character_const::get_spell_difficulty( const spell_id &spell_name ) const
{
    if( !me_chr_const->magic->knows_spell( spell_name ) ) {
        return spell_name->get_difficulty( *me_chr_const );
    }
    return me_chr_const->magic->get_spell( spell_name ).get_difficulty( *me_chr_const );
}

int talker_character_const::get_spell_count( const trait_id &school ) const
{
    int count = 0;
    for( const spell *sp : me_chr_const->magic->get_spells() ) {
        if( school.is_null() || sp->spell_class() == school ) {
            count++;
        }
    }
    return count;
}

int talker_character_const::get_spell_sum( const trait_id &school, int min_level ) const
{
    int count = 0;

    for( const spell *sp : me_chr_const->magic->get_spells() ) {
        if( school.is_null() || ( sp->spell_class() == school &&
                                  sp->get_effective_level() >= min_level ) ) {
            count = count + sp->get_effective_level() ;
        }
    }
    return count;
}

void talker_character::set_spell_level( const spell_id &sp, int new_level )
{
    me_chr->magic->set_spell_level( sp, new_level, me_chr );
}

void talker_character::set_spell_exp( const spell_id &sp, int new_level )
{
    me_chr->magic->set_spell_exp( sp, new_level, me_chr );
}

bool talker_character_const::knows_proficiency( const proficiency_id &proficiency ) const
{
    return me_chr_const->has_proficiency( proficiency );
}

time_duration talker_character_const::proficiency_practiced_time( const proficiency_id &prof ) const
{
    return me_chr_const->get_proficiency_practiced_time( prof );
}

void talker_character::set_proficiency_practiced_time( const proficiency_id &prof, int turns )
{
    me_chr->set_proficiency_practiced_time( prof, turns );
}

void talker_character::train_proficiency_for( const proficiency_id &prof, int turns )
{
    me_chr->practice_proficiency( prof, time_duration::from_seconds<int>( turns ) );
}

bool talker_character_const::has_effect( const efftype_id &effect_id, const bodypart_id &bp ) const
{
    return me_chr_const->has_effect( effect_id, bp );
}

effect talker_character_const::get_effect( const efftype_id &effect_id,
        const bodypart_id &bp ) const
{
    return me_chr_const->get_effect( effect_id, bp );
}

void talker_character::add_effect( const efftype_id &new_effect, const time_duration &dur,
                                   const std::string &bp, bool permanent, bool force,
                                   int intensity )
{
    bodypart_id target_part;
    if( "RANDOM" == bp ) {
        target_part = get_player_character().random_body_part( true );
    } else {
        target_part = bodypart_str_id( bp );
    }

    me_chr->add_effect( new_effect, dur, target_part, permanent, intensity, force );
}

void talker_character::remove_effect( const efftype_id &old_effect, const std::string &bp )
{
    bodypart_id target_part;
    if( "RANDOM" == bp ) {
        target_part = get_player_character().random_body_part( true );
    } else {
        target_part = bodypart_str_id( bp );
    }
    me_chr->remove_effect( old_effect, target_part );
}

diag_value const *talker_character_const::maybe_get_value( const std::string &var_name ) const
{
    return me_chr_const->maybe_get_value( var_name );
}

void talker_character::set_value( const std::string &var_name, diag_value const &value )
{
    me_chr->set_value( var_name, value );
}

void talker_character::remove_value( const std::string &var_name )
{
    me_chr->remove_value( var_name );
}

bool talker_character_const::is_wearing( const itype_id &item_id ) const
{
    return me_chr_const->is_wearing( item_id );
}

int talker_character_const::charges_of( const itype_id &item_id ) const
{
    return me_chr_const->charges_of( item_id );
}

bool talker_character_const::has_charges( const itype_id &item_id, int count ) const
{
    return me_chr_const->has_charges( item_id, count );
}

bool talker_character_const::has_charges( const itype_id &item_id, int count, bool in_tools ) const
{
    if( !in_tools ) {
        return me_chr_const->has_charges( item_id, count );
    } else {
        return me_chr_const->charges_of( item_id, count, return_true<item>, nullptr, in_tools ) >= count;
    }
}

std::list<item> talker_character::use_charges( const itype_id &item_name, const int count )
{
    return me_chr->use_charges( item_name, count );
}

std::list<item> talker_character::use_charges( const itype_id &item_name, const int count,
        bool in_tools )
{
    if( !in_tools ) {
        return me_chr->use_charges( item_name, count );
    } else {
        return me_chr->use_charges( item_name, count, -1, return_true<item>, in_tools );
    }
}

std::list<item> talker_character::use_amount( const itype_id &item_name, const int count )
{
    return me_chr->use_amount( item_name, count );
}

bool talker_character_const::has_amount( const itype_id &item_id, int count ) const
{
    return me_chr_const->has_amount( item_id, count );
}

int talker_character_const::get_amount( const itype_id &item_id ) const
{
    return me_chr_const->amount_of( item_id );
}

int talker_character_const::cash() const
{
    return me_chr_const->cash;
}

std::vector<const item *> talker_character_const::const_items_with( const
        std::function<bool( const item & )>
        &filter ) const
{
    return me_chr_const->items_with( filter );
}

std::vector<item *> talker_character::items_with( const std::function<bool( const item & )>
        &filter )
{
    return me_chr->items_with( filter );
}

void talker_character::i_add( const item &new_item )
{
    me_chr->i_add( new_item );
}

void talker_character::i_add_or_drop( item &new_item, bool force_equip )
{
    if( force_equip ) {
        if( me_chr->can_wear( new_item ).success() ) {
            new_item.set_flag( json_flag_FIT );
            me_chr->wear_item( new_item, false );
            return;
        } else if( !me_chr->has_wield_conflicts( new_item ) &&
                   !me_chr->martial_arts_data->keep_hands_free && //No wield if hands free
                   me_chr->wield( new_item ) ) {
            return;
        }
    }
    me_chr->i_add_or_drop( new_item );
}

void talker_character::remove_items_with( const std::function<bool( const item & )> &filter )
{
    me_chr->remove_items_with( filter );
}

bool talker_character_const::unarmed_attack() const
{
    return me_chr_const->unarmed_attack();
}

bool talker_character_const::can_stash_weapon() const
{
    std::optional<bionic *> bionic_weapon = me_chr_const->find_bionic_by_uid(
            me_chr_const->get_weapon_bionic_uid() );
    if( bionic_weapon && me_chr_const->can_deactivate_bionic( **bionic_weapon ).success() ) {
        return true;
    }

    return me_chr_const->can_pickVolume( *me_chr_const->get_wielded_item() );
}

bool talker_character_const::has_stolen_item( const_talker const &guy ) const
{
    const Character *owner = guy.get_const_character();
    if( owner ) {
        for( const item *&elem : me_chr_const->inv_dump() ) {
            if( elem->is_old_owner( *owner, true ) ) {
                return true;
            }
        }
    }
    return false;
}

faction *talker_character_const::get_faction() const
{
    return me_chr_const->get_faction();
}

std::string talker_character_const::short_description() const
{
    return me_chr_const->short_description();
}

bool talker_character_const::has_activity() const
{
    return !me_chr_const->activity.is_null();
}

bool talker_character_const::is_mounted() const
{
    return me_chr_const->is_mounted();
}

int talker_character_const::get_activity_level() const
{
    return me_chr_const->activity_level_index();
}

int talker_character_const::get_sleepiness() const
{
    return me_chr_const->get_sleepiness();
}

int talker_character_const::get_hunger() const
{
    return me_chr_const->get_hunger();
}

int talker_character_const::get_thirst() const
{
    return me_chr_const->get_thirst();
}

int talker_character_const::get_instant_thirst() const
{
    return me_chr_const->get_instant_thirst();
}

int talker_character_const::get_stored_kcal() const
{
    return me_chr_const->get_stored_kcal();
}

int talker_character_const::get_healthy_kcal() const
{
    return me_chr_const->get_healthy_kcal();
}

int talker_character_const::get_size() const
{
    add_msg_debug( debugmode::DF_TALKER, "Size category of character %s = %d", me_chr_const->name,
                   me_chr_const->get_size() - 0 );
    return me_chr_const->get_size() - 0;
}

void talker_character::set_stored_kcal( int value )
{
    me_chr->set_stored_kcal( value );
}
void talker_character::mod_stored_kcal( int value, bool ignore_weariness )
{
    me_chr->mod_stored_kcal( value, ignore_weariness );
}
void talker_character::set_thirst( int value )
{
    me_chr->set_thirst( value );
}

bool talker_character_const::is_in_control_of( const vehicle &veh ) const
{
    map &here = get_map();

    return veh.player_in_control( here, *me_chr_const );
}

void talker_character::shout( const std::string &speech, bool order )
{
    me_chr->shout( speech, order );
}

int talker_character_const::pain_cur() const
{
    return me_chr_const->get_pain();
}

int talker_character_const::perceived_pain_cur() const
{
    return me_chr_const->get_perceived_pain();
}

double talker_character_const::armor_at( damage_type_id &dt, bodypart_id &bp ) const
{
    return me_chr_const->worn.damage_resist( dt, bp );
}

int talker_character_const::coverage_at( bodypart_id &id ) const
{
    return me_chr_const->worn.get_coverage( id );
}

int talker_character_const::encumbrance_at( bodypart_id &id ) const
{
    return me_chr_const->encumb( id );
}

void talker_character::mod_pain( int amount )
{
    me_chr->mod_pain( amount );
}

void talker_character::set_pain( int amount )
{
    me_chr->set_pain( amount );
}

bool talker_character_const::worn_with_flag( const flag_id &flag, const bodypart_id &bp ) const
{
    return me_chr_const->worn_with_flag( flag, bp );
}

bool talker_character_const::wielded_with_flag( const flag_id &flag ) const
{
    return me_chr_const->get_wielded_item() && me_chr_const->get_wielded_item()->has_flag( flag );
}

bool talker_character_const::wielded_with_weapon_category( const weapon_category_id &w_cat ) const
{
    return me_chr_const->get_wielded_item() &&
           me_chr_const->get_wielded_item()->typeId()->weapon_category.count( w_cat ) > 0;
}

bool talker_character_const::wielded_with_weapon_skill( const skill_id &w_skill ) const
{
    if( me_chr_const->get_wielded_item() ) {
        item *it = me_chr_const->get_wielded_item().get_item();
        skill_id it_skill = it->is_gun() ? it->gun_skill() : it->melee_skill();
        return it_skill == w_skill;
    } else {
        return false;
    }
}

bool talker_character_const::wielded_with_item_ammotype( const ammotype &w_ammotype ) const
{
    if( !me_chr_const->get_wielded_item() ) {
        return false;
    }
    item *it = me_chr_const->get_wielded_item().get_item();
    if( it->ammo_types().empty() ) {
        return false;
    }
    std::set<ammotype> it_ammotype = it->ammo_types();
    bool match = false;

    for( ammotype ammo : it_ammotype ) {
        if( ammo == w_ammotype ) {
            match = true;
        }
    }
    return match;
}

bool talker_character_const::has_item_with_flag( const flag_id &flag ) const
{
    return me_chr_const->cache_has_item_with( flag );
}

int talker_character_const::item_rads( const flag_id &flag, aggregate_type agg_func ) const
{
    std::vector<int> rad_vals;
    me_chr_const->cache_visit_items_with( flag, [&]( const item & it ) {
        if( me_chr_const->is_worn( it ) || me_chr_const->is_wielding( it ) ) {
            rad_vals.emplace_back( it.irradiation );
        }
    } );
    return aggregate( rad_vals, agg_func );
}

units::energy talker_character_const::power_cur() const
{
    return me_chr_const->get_power_level();
}

units::energy talker_character_const::power_max() const
{
    return me_chr_const->get_max_power_level();
}

int talker_character_const::mana_cur() const
{
    return me_chr_const->magic->available_mana();
}

int talker_character_const::mana_max() const
{
    return me_chr_const->magic->max_mana( *me_chr_const );
}

void talker_character::set_mana_cur( int value )
{
    me_chr->magic->set_mana( value );
}

bool talker_character_const::can_see() const
{
    return !me_chr_const->is_blind() && ( !me_chr_const->in_sleep_state() ||
                                          me_chr_const->has_flag( json_flag_SEESLEEP ) );
}

bool talker_character_const::can_see_location( const tripoint_bub_ms &pos ) const
{
    const map &here = get_map();

    return me_chr_const->sees( here, pos );
}

void talker_character::set_sleepiness( int amount )
{
    me_chr->set_sleepiness( amount );
}

void talker_character::mod_daily_health( int amount, int cap )
{
    me_chr->mod_daily_health( amount, cap );
}

void talker_character::mod_livestyle( int amount )
{
    me_chr->mod_livestyle( amount );
}

int talker_character_const::morale_cur() const
{
    return me_chr_const->get_morale_level();
}

void talker_character::set_fac_relation( const Character *guy, npc_factions::relationship rule,
        bool should_set_value )
{
    if( !guy || !me_chr ) {
        debugmsg( "Missing character to set new faction relationship" );
        return;
    }

    faction *u_fac = me_chr->get_faction();
    faction *npc_fac = guy->get_faction();

    npc_fac->relations[u_fac->id.c_str()].set( static_cast<size_t>( rule ), should_set_value );

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

int talker_character_const::focus_cur() const
{
    return me_chr_const->get_focus();
}

int talker_character_const::focus_effective_cur() const
{
    return me_chr_const->get_effective_focus();
}

void talker_character::mod_focus( int amount )
{
    me_chr->mod_focus( amount );
}

void talker_character::set_rad( int amount )
{
    me_chr->set_rad( amount );
}

int talker_character_const::get_rad() const
{
    return me_chr_const->get_rad();
}

int talker_character_const::get_stim() const
{
    return me_chr_const->get_stim();
}

int talker_character_const::get_addiction_intensity( const addiction_id &add_id ) const
{
    return me_chr_const->addiction_level( add_id );
}

int talker_character_const::get_addiction_turns( const addiction_id &add_id ) const
{
    for( const addiction &add : me_chr_const->addictions ) {
        if( add.type == add_id ) {
            return to_turns<int>( add.sated );
        }
    }
    return 0;
}

void talker_character::set_addiction_turns( const addiction_id &add_id, int amount )
{
    for( addiction &add : me_chr->addictions ) {
        if( add.type == add_id ) {
            add.sated += time_duration::from_turns( amount );
        }
    }
}

void talker_character::set_stim( int amount )
{
    me_chr->set_stim( amount );
}

int talker_character_const::get_pkill() const
{
    return me_chr_const->get_painkiller();
}

void talker_character::set_pkill( int amount )
{
    me_chr->set_painkiller( amount );
}

int talker_character_const::get_stamina() const
{
    return me_chr_const->get_stamina();
}

void talker_character::set_stamina( int amount )
{
    me_chr->set_stamina( amount );
}

int talker_character_const::get_sleep_deprivation() const
{
    return me_chr_const->get_sleep_deprivation();
}

void talker_character::set_sleep_deprivation( int amount )
{
    me_chr->set_sleep_deprivation( amount );
}

void talker_character::set_kill_xp( int amount )
{
    me_chr->kill_xp = amount;
}

int talker_character_const::get_kill_xp() const
{
    return me_chr_const->kill_xp;
}

void talker_character::set_age( int amount )
{
    int years_since_cataclysm = to_turns<int>( calendar::turn - calendar::turn_zero ) /
                                to_turns<int>( calendar::year_length() );
    me_chr->set_base_age( amount + years_since_cataclysm );
}

int talker_character_const::get_age() const
{
    return me_chr_const->age();
}

int talker_character_const::get_ugliness() const
{
    return me_chr_const->ugliness();
}

int talker_character_const::get_bmi_permil() const
{
    return std::round( me_chr_const->get_bmi_fat() * 1000.0f );
}

int talker_character_const::get_weight() const
{
    return units::to_milligram( me_chr_const->get_weight() );
}

int talker_character_const::get_volume() const
{
    return units::to_milliliter( me_chr_const->get_total_volume() );
}

void talker_character::set_height( int amount )
{
    me_chr->set_base_height( amount );
}

int talker_character_const::get_height() const
{
    return me_chr_const->height();
}

const move_mode_id &talker_character_const::get_move_mode() const
{
    return me_chr_const->move_mode;
}

int talker_character_const::get_fine_detail_vision_mod() const
{
    return std::ceil( me_chr_const->fine_detail_vision_mod() );
}

int talker_character_const::get_health() const
{
    return me_chr_const->get_lifestyle();
}

static std::pair<bodypart_id, bodypart_id> temp_delta( const Character *u )
{
    bodypart_id current_bp_extreme = u->get_all_body_parts().front();
    bodypart_id conv_bp_extreme = current_bp_extreme;
    for( const bodypart_id &bp : u->get_all_body_parts() ) {
        if( units::abs( u->get_part_temp_cur( bp ) - BODYTEMP_NORM ) >
            units::abs( u->get_part_temp_cur( current_bp_extreme ) - BODYTEMP_NORM ) ) {
            current_bp_extreme = bp;
        }
        if( units::abs( u->get_part_temp_conv( bp ) - BODYTEMP_NORM ) >
            units::abs( u->get_part_temp_conv( conv_bp_extreme ) - BODYTEMP_NORM ) ) {
            conv_bp_extreme = bp;
        }
    }
    return std::make_pair( current_bp_extreme, conv_bp_extreme );
}

units::temperature talker_character_const::get_body_temp() const
{
    return me_chr_const->get_part_temp_cur( temp_delta( me_chr_const ).first );
}

units::temperature_delta talker_character_const::get_body_temp_delta() const
{
    return me_chr_const->get_part_temp_conv( temp_delta( me_chr_const ).second ) -
           me_chr_const->get_part_temp_cur( temp_delta( me_chr_const ).first );
}

bool talker_character_const::knows_martial_art( const matype_id &id ) const
{
    return me_chr_const->martial_arts_data->has_martialart( id );
}

bool talker_character_const::using_martial_art( const matype_id &id ) const
{
    return me_chr_const->martial_arts_data->selected_style() == id;
}

void talker_character::add_bionic( const bionic_id &new_bionic )
{
    me_chr->add_bionic( new_bionic );
}

void talker_character::remove_bionic( const bionic_id &old_bionic )
{
    if( std::optional<bionic *> bio = me_chr->find_bionic_by_type( old_bionic ) ) {
        me_chr->remove_bionic( **bio );
    }
}

std::vector<skill_id> talker_character_const::skills_teacheable() const
{
    return me_chr_const->skills_offered_to( nullptr );
}

std::vector<proficiency_id> talker_character_const::proficiencies_teacheable() const
{
    return me_chr_const->proficiencies_offered_to( nullptr );
}

std::vector<matype_id> talker_character_const::styles_teacheable() const
{
    return me_chr_const->styles_offered_to( nullptr );
}

std::vector<spell_id> talker_character_const::spells_teacheable() const
{
    return me_chr_const->spells_offered_to( nullptr );
}

std::vector<skill_id> talker_character_const::skills_offered_to( const_talker const &student ) const
{
    if( student.get_const_character() ) {
        return me_chr_const->skills_offered_to( student.get_const_character() );
    } else {
        return {};
    }
}

std::string talker_character_const::skill_training_text( const_talker const &student,
        const skill_id &skill ) const
{
    Character const *pupil = student.get_const_character();
    if( !pupil ) {
        return "";
    }
    const int cost = calc_skill_training_cost_char( *me_chr_const, *pupil, skill );
    SkillLevel skill_level_obj = pupil->get_skill_level_object( skill );
    SkillLevel teacher_skill_level = me_chr_const->get_skill_level_object( skill );
    const int cur_level = skill_level_obj.knowledgeLevel();
    const int cur_level_exercise = skill_level_obj.knowledgeExperience();
    // knowledge_train will adjust level xp based on the difference between your understanding and the NPC's.
    skill_level_obj.knowledge_train( 10000, teacher_skill_level.knowledgeLevel() );
    const int next_level = skill_level_obj.knowledgeLevel();
    const int next_level_exercise = skill_level_obj.knowledgeExperience();

    //~Skill name: current level (experience) -> next level (experience) (cost in dollars)
    return string_format( cost > 0 ?  _( "%s: %d (%d%%) -> %d (%d%%) (cost $%d)" ) :
                          _( "%s: %d (%d%%) -> %d (%d%%)" ), skill.obj().name(), cur_level,
                          cur_level_exercise, next_level, next_level_exercise, cost / 100 );
}

std::vector<proficiency_id> talker_character_const::proficiencies_offered_to(
    const_talker const &student ) const
{
    if( student.get_const_character() ) {
        return me_chr_const->proficiencies_offered_to( student.get_const_character() );
    } else {
        return {};
    }
}

std::string talker_character_const::proficiency_training_text( const_talker const &student,
        const proficiency_id &proficiency ) const
{
    Character const *pupil = student.get_const_character();
    if( !pupil ) {
        return "";
    }
    const time_duration time_needed = proficiency->time_to_learn();
    const time_duration current_time = time_needed - pupil->proficiency_training_needed( proficiency );

    const int cost = calc_proficiency_training_cost( *me_chr_const, *pupil, proficiency );
    const std::string name = proficiency->name();
    const float pct_before = current_time * 100.0f / time_needed;
    const float pct_after = ( current_time + 15_minutes ) * 100.0f / time_needed;
    const std::string after_str = pct_after >= 100.0f ? pgettext( "NPC training: proficiency learned",
                                  "done" ) : string_format( "%2.0f%%", pct_after );

    if( cost > 0 ) {
        //~ Proficiency name: (current_practice) -> (next_practice) (cost in dollars)
        return string_format( _( "%s: (%2.0f%%) -> (%s) (cost $%d)" ), name, pct_before, after_str,
                              cost / 100 );
    }
    //~ Proficiency name: (current_practice) -> (next_practice)
    return string_format( _( "%s: (%2.0f%%) -> (%s)" ), name, pct_before, after_str );
}

std::vector<matype_id> talker_character_const::styles_offered_to( const_talker const &student )
const
{
    if( student.get_const_character() ) {
        return me_chr_const->styles_offered_to( student.get_const_character() );
    } else {
        return {};
    }
}

std::string talker_character_const::style_training_text( const_talker const &student,
        const matype_id &style ) const
{
    if( !student.get_const_character() ) {
        return "";
    } else if( !me_chr_const->is_npc() ||
               me_chr_const->as_npc()->is_ally( *student.get_const_character() ) ) {
        return string_format( "%s", style.obj().name );
    } else {
        return string_format( _( "%s ( cost $%d )" ), style.obj().name, 8 );
    }
}

std::vector<spell_id> talker_character_const::spells_offered_to( const_talker const &student ) const
{
    if( student.get_const_character() ) {
        return me_chr_const->spells_offered_to( student.get_const_character() );
    } else {
        return {};
    }
}

std::string talker_character_const::spell_training_text( const_talker const &student,
        const spell_id &sp ) const
{
    Character const *pupil = student.get_const_character();
    if( !pupil ) {
        return "";
    }
    const spell &temp_spell = me_chr_const->magic->get_spell( sp );
    const bool knows = pupil->magic->knows_spell( sp );
    const int cost = me_chr_const->calc_spell_training_cost( knows, temp_spell.get_difficulty( *pupil ),
                     temp_spell.get_level() );
    std::string text;
    if( cost == 0 && ( !me_chr_const->is_npc() || me_chr_const->as_npc()->is_ally( *pupil ) ) ) {
        text = temp_spell.name();
    } else if( knows ) {
        text = string_format( _( "%s: 1 hour lesson (cost %s)" ), temp_spell.name(),
                              format_money( cost ) );
    } else {
        text = string_format( _( "%s: teaching spell knowledge (cost %s)" ),
                              temp_spell.name(), format_money( cost ) );
    }
    return text;
}

std::string talker_character_const::skill_seminar_text( const skill_id &s ) const
{
    int lvl = me_chr_const->get_skill_level( s );
    return string_format( "%s (%d)", s.obj().name(), lvl );
}

std::string talker_character_const::proficiency_seminar_text( const proficiency_id &p ) const
{
    return p->name();
}

std::string talker_character_const::style_seminar_text( const matype_id &m ) const
{
    return m->name.translated();
}

std::string talker_character_const::spell_seminar_text( const spell_id &s ) const
{
    return s->name.translated();
}

std::vector<bodypart_id> talker_character_const::get_all_body_parts( get_body_part_flags flags )
const
{
    return me_chr_const->get_all_body_parts( flags );
}

int talker_character_const::get_part_hp_cur( const bodypart_id &id ) const
{
    return me_chr_const->get_part_hp_cur( id );
}

int talker_character_const::get_part_hp_max( const bodypart_id &id ) const
{
    return me_chr_const->get_part_hp_max( id );
}

void talker_character::set_part_hp_cur( const bodypart_id &id, int set )
{
    me_chr->set_part_hp_cur( id, set );
}

void talker_character::set_all_parts_hp_cur( int set )
{
    me_chr->set_all_parts_hp_cur( set );
}

bool talker_character_const::get_is_alive() const
{
    return !me_chr_const->is_dead_state();
}

bool talker_character_const::is_warm() const
{
    return me_chr_const->is_warm();
}

void talker_character::die( map *here )
{
    me_chr->die( here, nullptr );
}

matec_id talker_character_const::get_random_technique( Creature const &t, bool crit,
        bool dodge_counter, bool block_counter, const std::vector<matec_id> &blacklist ) const
{
    return std::get<0>( me_chr_const->pick_technique( t, me_chr_const->used_weapon(), crit,
                        dodge_counter,
                        block_counter,
                        blacklist ) );
}

void talker_character::attack_target( Creature &t, bool allow_special,
                                      const matec_id &force_technique, bool allow_unarmed, int forced_movecost )
{
    me_chr->melee_attack( t, allow_special, force_technique, allow_unarmed, forced_movecost );
}

void talker_character::learn_martial_art( const matype_id &id )
{
    me_chr->martial_arts_data->add_martialart( id );
}

void talker_character::forget_martial_art( const matype_id &id )
{
    me_chr->martial_arts_data->clear_style( id );
}

int talker_character_const::climate_control_str_heat() const
{
    return me_chr_const->climate_control_strength().first;
}

int talker_character_const::climate_control_str_chill() const
{
    return me_chr_const->climate_control_strength().second;
}
