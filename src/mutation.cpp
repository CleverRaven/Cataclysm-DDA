#include "mutation.h"

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <memory>
#include <unordered_set>

#include "activity_type.h"
#include "avatar_action.h"
#include "avatar.h"
#include "bionics.h"
#include "character_attire.h"
#include "character.h"
#include "color.h"
#include "condition.h"
#include "creature.h"
#include "debug.h"
#include "effect_on_condition.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "field_type.h"
#include "game.h"
#include "item.h"
#include "itype.h"
#include "magic_enchantment.h"
#include "make_static.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "memorial_logger.h"
#include "messages.h"
#include "monster.h"
#include "omdata.h"
#include "output.h"
#include "overmapbuffer.h"
#include "pimpl.h"
#include "player_activity.h"
#include "rng.h"
#include "text_snippets.h"
#include "translations.h"
#include "units.h"

static const activity_id ACT_PULL_CREATURE( "ACT_PULL_CREATURE" );
static const activity_id ACT_TREE_COMMUNION( "ACT_TREE_COMMUNION" );

static const itype_id itype_fake_burrowing( "fake_burrowing" );

static const json_character_flag json_flag_CHLOROMORPH( "CHLOROMORPH" );
static const json_character_flag json_flag_HUGE( "HUGE" );
static const json_character_flag json_flag_LARGE( "LARGE" );
static const json_character_flag json_flag_ROOTS2( "ROOTS2" );
static const json_character_flag json_flag_ROOTS3( "ROOTS3" );
static const json_character_flag json_flag_SMALL( "SMALL" );
static const json_character_flag json_flag_TINY( "TINY" );

static const mtype_id mon_player_blob( "mon_player_blob" );

static const mutation_category_id mutation_category_ANY( "ANY" );

static const trait_id trait_BURROW( "BURROW" );
static const trait_id trait_BURROWLARGE( "BURROWLARGE" );
static const trait_id trait_DEBUG_BIONIC_POWER( "DEBUG_BIONIC_POWER" );
static const trait_id trait_DEBUG_BIONIC_POWERGEN( "DEBUG_BIONIC_POWERGEN" );
static const trait_id trait_DEX_ALPHA( "DEX_ALPHA" );
static const trait_id trait_GASTROPOD_EXTREMITY2( "GASTROPOD_EXTREMITY2" );
static const trait_id trait_GASTROPOD_EXTREMITY3( "GASTROPOD_EXTREMITY3" );
static const trait_id trait_GLASSJAW( "GLASSJAW" );
static const trait_id trait_INT_ALPHA( "INT_ALPHA" );
static const trait_id trait_INT_SLIME( "INT_SLIME" );
static const trait_id trait_LONG_TONGUE2( "LONG_TONGUE2" );
static const trait_id trait_M_BLOOM( "M_BLOOM" );
static const trait_id trait_M_FERTILE( "M_FERTILE" );
static const trait_id trait_M_PROVENANCE( "M_PROVENANCE" );
static const trait_id trait_NAUSEA( "NAUSEA" );
static const trait_id trait_PER_ALPHA( "PER_ALPHA" );
static const trait_id trait_SLIMESPAWNER( "SLIMESPAWNER" );
static const trait_id trait_SNAIL_TRAIL( "SNAIL_TRAIL" );
static const trait_id trait_STR_ALPHA( "STR_ALPHA" );
static const trait_id trait_TREE_COMMUNION( "TREE_COMMUNION" );
static const trait_id trait_VOMITOUS( "VOMITOUS" );
static const trait_id trait_WEB_WEAVER( "WEB_WEAVER" );

static const vitamin_id vitamin_instability( "instability" );

namespace io
{

template<>
std::string enum_to_string<mutagen_technique>( mutagen_technique data )
{
    switch( data ) {
        // *INDENT-OFF*
        case mutagen_technique::consumed_mutagen: return "consumed_mutagen";
        case mutagen_technique::injected_mutagen: return "injected_mutagen";
        case mutagen_technique::consumed_purifier: return "consumed_purifier";
        case mutagen_technique::injected_purifier: return "injected_purifier";
        case mutagen_technique::injected_smart_purifier: return "injected_smart_purifier";
        // *INDENT-ON*
        case mutagen_technique::num_mutagen_techniques:
            break;
    }
    cata_fatal( "Invalid mutagen_technique" );
}

} // namespace io

bool Character::has_trait( const trait_id &b ) const
{
    return my_mutations.count( b ) || enchantment_cache->get_mutations().count( b );
}

bool Character::has_trait_variant( const trait_and_var &test ) const
{
    auto mutit = my_mutations.find( test.trait );
    if( mutit != my_mutations.end() ) {
        if( mutit->second.variant != nullptr ) {
            return mutit->second.variant->id == test.variant;
        } else {
            return test.variant.empty();
        }
    }

    return false;
}

int Character::count_trait_flag( const json_character_flag &b ) const
{
    int ret = 0;
    // UGLY, SLOW, should be cached as my_mutation_flags or something
    for( const trait_id &mut : get_mutations() ) {
        const mutation_branch &mut_data = mut.obj();
        if( mut_data.flags.count( b ) > 0 ) {
            ret++;
        } else if( mut_data.activated ) {
            Character &player = get_player_character();
            if( ( mut_data.active_flags.count( b ) > 0 && player.has_active_mutation( mut ) ) ||
                ( mut_data.inactive_flags.count( b ) > 0 && !player.has_active_mutation( mut ) ) ) {
                ret++;
            }
        }
    }

    return ret;
}

bool Character::has_base_trait( const trait_id &b ) const
{
    // Look only at base traits
    return my_traits.find( b ) != my_traits.end();
}

void Character::toggle_trait( const trait_id &trait_, const std::string &var_ )
{
    // Take copy of argument because it might be a reference into a container
    // we're about to erase from.
    // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
    const trait_id trait = trait_;
    const auto titer = my_traits.find( trait );
    const auto miter = my_mutations.find( trait );
    const bool not_found_in_traits = titer == my_traits.end();
    const bool not_found_in_mutations = miter == my_mutations.end();
    if( not_found_in_traits ) {
        my_traits.insert( trait );
    } else {
        my_traits.erase( titer );
    }
    // Checking this after toggling my_traits, if we exit the two are now consistent.
    if( not_found_in_traits != not_found_in_mutations ) {
        debugmsg( "my_traits and my_mutations were out of sync for %s\n", trait.str() );
        return;
    }
    if( not_found_in_mutations ) {
        set_mutation( trait, trait->variant( var_ ) );
    } else {
        unset_mutation( trait );
    }
}

void Character::set_mutation_unsafe( const trait_id &trait, const mutation_variant *variant )
{
    const auto iter = my_mutations.find( trait );
    if( iter != my_mutations.end() ) {
        return;
    }
    if( variant == nullptr ) {
        variant = trait->pick_variant();
    }
    my_mutations.emplace( trait, trait_data{variant} );
    cached_mutations.push_back( &trait.obj() );
    mutation_effect( trait, false );
}

void Character::do_mutation_updates()
{
    recalc_sight_limits();
    calc_encumbrance();

    // If the stamina is higher than the max (Languorous), set it back to max
    if( get_stamina() > get_stamina_max() ) {
        set_stamina( get_stamina_max() );
    }
}

void Character::set_mutations( const std::vector<trait_id> &traits )
{
    for( const trait_id &trait : traits ) {
        set_mutation_unsafe( trait );
    }
    do_mutation_updates();
}

void Character::set_mutation( const trait_id &trait, const mutation_variant *variant )
{
    set_mutation_unsafe( trait, variant );
    do_mutation_updates();
}

void Character::set_mut_variant( const trait_id &trait, const mutation_variant *variant )
{
    auto mutit = my_mutations.find( trait );
    if( mutit != my_mutations.end() ) {
        mutit->second.variant = variant;
    }
}

void Character::unset_mutation( const trait_id &trait_ )
{
    // Take copy of argument because it might be a reference into a container
    // we're about to erase from.
    // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
    const trait_id trait = trait_;
    const auto iter = my_mutations.find( trait );
    if( iter == my_mutations.end() ) {
        return;
    }
    const mutation_branch &mut = *trait;
    cached_mutations.erase( std::remove( cached_mutations.begin(), cached_mutations.end(), &mut ),
                            cached_mutations.end() );
    my_mutations.erase( iter );
    mutation_loss_effect( trait );
    do_mutation_updates();
}

void Character::switch_mutations( const trait_id &switched, const trait_id &target,
                                  bool start_powered )
{
    unset_mutation( switched );

    set_mutation( target );
    my_mutations[target].powered = start_powered;
}

bool Character::can_power_mutation( const trait_id &mut )
{
    bool hunger = mut->hunger && get_kcal_percent() < 0.5f;
    bool thirst = mut->thirst && get_thirst() >= 260;
    bool fatigue = mut->fatigue && get_fatigue() >= fatigue_levels::EXHAUSTED;

    return !hunger && !fatigue && !thirst;
}

void Character::mutation_reflex_trigger( const trait_id &mut )
{
    if( mut->trigger_list.empty() || !can_power_mutation( mut ) ) {
        return;
    }

    bool activate = false;

    std::pair<translation, game_message_type> msg_on;
    std::pair<translation, game_message_type> msg_off;

    for( const std::vector<reflex_activation_data> &vect_rdata : mut->trigger_list ) {
        activate = false;
        // OR conditions: if any trigger is true then this condition is true
        for( const reflex_activation_data &rdata : vect_rdata ) {
            if( rdata.is_trigger_true( *this ) ) {
                activate = true;
                msg_on = rdata.msg_on;
                break;
            }
            msg_off = rdata.msg_off;
        }
        // AND conditions: if any OR condition is false then this is false
        if( !activate ) {
            break;
        }
    }

    if( activate && !has_active_mutation( mut ) ) {
        activate_mutation( mut );
        if( !msg_on.first.empty() ) {
            add_msg_if_player( msg_on.second, msg_on.first );
        }
    } else if( !activate && has_active_mutation( mut ) ) {
        deactivate_mutation( mut );
        if( !msg_off.first.empty() ) {
            add_msg_if_player( msg_off.second, msg_off.first );
        }
    }
}

bool reflex_activation_data::is_trigger_true( Character &guy ) const
{
    dialogue d( get_talker_for( guy ), nullptr );
    return trigger( d );
}

int Character::get_mod( const trait_id &mut, const std::string &arg ) const
{
    const auto &mod_data = mut->mods;
    int ret = 0;
    auto found = mod_data.find( std::make_pair( false, arg ) );
    if( found != mod_data.end() ) {
        ret += found->second;
    }
    return ret;
}

void Character::apply_mods( const trait_id &mut, bool add_remove )
{
    int sign = add_remove ? 1 : -1;
    int str_change = get_mod( mut, "STR" );
    str_max += sign * str_change;
    per_max += sign * get_mod( mut, "PER" );
    dex_max += sign * get_mod( mut, "DEX" );
    int_max += sign * get_mod( mut, "INT" );

    reset_stats();
    if( str_change != 0 ) {
        recalc_hp();
    }
}

bool mutation_branch::conflicts_with_item( const item &it ) const
{
    if( allow_soft_gear && it.is_soft() ) {
        return false;
    }

    for( const flag_id &allowed : allowed_items ) {
        if( it.has_flag( allowed ) ) {
            return false;
        }
    }

    for( const bodypart_str_id &bp : restricts_gear ) {
        if( it.covers( bp.id() ) ) {
            return true;
        }
    }

    return false;
}

const resistances &mutation_branch::damage_resistance( const bodypart_id &bp ) const
{
    const auto iter = armor.find( bp.id() );
    if( iter == armor.end() ) {
        static const resistances nulres;
        return nulres;
    }

    return iter->second;
}

void Character::recalculate_size()
{
    if( has_flag( json_flag_TINY ) ) {
        size_class = creature_size::tiny;
    } else if( has_flag( json_flag_SMALL ) ) {
        size_class = creature_size::small;
    } else if( has_flag( json_flag_LARGE ) ) {
        size_class = creature_size::large;
    } else if( has_flag( json_flag_HUGE ) ) {
        size_class = creature_size::huge;
    } else {
        size_class = creature_size::medium;
    }
}

void Character::mutation_effect( const trait_id &mut, const bool worn_destroyed_override )
{
    if( mut == trait_GLASSJAW ) {
        recalc_hp();

    } else if( mut == trait_STR_ALPHA ) {
        if( str_max < 16 ) {
            str_max = 8 + str_max / 2;
        }
        apply_mods( mut, true );
        recalc_hp();
    } else if( mut == trait_DEX_ALPHA ) {
        if( dex_max < 16 ) {
            dex_max = 8 + dex_max / 2;
        }
        apply_mods( mut, true );
    } else if( mut == trait_INT_ALPHA ) {
        if( int_max < 16 ) {
            int_max = 8 + int_max / 2;
        }
        apply_mods( mut, true );
    } else if( mut == trait_INT_SLIME ) {
        int_max *= 2; // Now, can you keep it? :-)

    } else if( mut == trait_PER_ALPHA ) {
        if( per_max < 16 ) {
            per_max = 8 + per_max / 2;
        }
        apply_mods( mut, true );
    } else {
        apply_mods( mut, true );
    }

    recalculate_size();

    const mutation_branch &branch = mut.obj();
    if( branch.hp_modifier.has_value() || branch.hp_modifier_secondary.has_value() ||
        branch.hp_adjustment.has_value() ) {
        recalc_hp();
    }

    for( const itype_id &armor : branch.integrated_armor ) {
        item tmparmor( armor );
        wear_item( tmparmor, false );
    }

    remove_worn_items_with( [&]( item & armor ) {
        if( armor.has_flag( STATIC( flag_id( "OVERSIZE" ) ) ) ) {
            return false;
        }
        if( armor.has_flag( STATIC( flag_id( "INTEGRATED" ) ) ) ) {
            return false;
        }
        if( !branch.conflicts_with_item( armor ) ) {
            return false;
        }
        // if an item gives an enchantment it shouldn't break or be shoved off
        for( const enchantment &ench : armor.get_enchantments() ) {
            for( const trait_id &inner_mut : ench.get_mutations() ) {
                if( mut == inner_mut ) {
                    return false;
                }
            }
        }
        if( !worn_destroyed_override && branch.destroys_gear ) {
            add_msg_player_or_npc( m_bad,
                                   _( "Your %s is destroyed!" ),
                                   _( "<npcname>'s %s is destroyed!" ),
                                   armor.tname() );
            armor.spill_contents( pos() );
        } else {
            add_msg_player_or_npc( m_bad,
                                   _( "Your %s is pushed off!" ),
                                   _( "<npcname>'s %s is pushed off!" ),
                                   armor.tname() );
            get_map().add_item_or_charges( pos(), armor );
        }
        return true;
    } );

    for( std::pair<mtype_id, int> moncam : branch.moncams ) {
        add_moncam( moncam );
    }

    if( branch.starts_active ) {
        my_mutations[mut].powered = true;
    }

    on_mutation_gain( mut );
}

void Character::mutation_loss_effect( const trait_id &mut )
{
    if( mut == trait_GLASSJAW ) {
        recalc_hp();

    } else if( mut == trait_STR_ALPHA ) {
        apply_mods( mut, false );
        if( str_max < 16 ) {
            str_max = 2 * ( str_max - 8 );
        }
        recalc_hp();
    } else if( mut == trait_DEX_ALPHA ) {
        apply_mods( mut, false );
        if( dex_max < 16 ) {
            dex_max = 2 * ( dex_max - 8 );
        }
    } else if( mut == trait_INT_ALPHA ) {
        apply_mods( mut, false );
        if( int_max < 16 ) {
            int_max = 2 * ( int_max - 8 );
        }
    } else if( mut == trait_INT_SLIME ) {
        int_max /= 2; // In case you have a freak accident with the debug menu ;-)

    } else if( mut == trait_PER_ALPHA ) {
        apply_mods( mut, false );
        if( per_max < 16 ) {
            per_max = 2 * ( per_max - 8 );
        }
    } else {
        apply_mods( mut, false );
    }

    recalculate_size();

    const mutation_branch &branch = mut.obj();
    if( branch.hp_modifier.has_value() || branch.hp_modifier_secondary.has_value() ||
        branch.hp_adjustment.has_value() ) {
        recalc_hp();
    }

    for( const itype_id &popped_armor : branch.integrated_armor ) {
        remove_worn_items_with( [&]( item & armor ) {
            return armor.typeId() == popped_armor;
        } );
    }

    if( !branch.enchantments.empty() ) {
        recalculate_enchantment_cache();
        recalculate_bodyparts();
    }

    for( std::pair<mtype_id, int> moncam : branch.moncams ) {
        remove_moncam( moncam.first );
    }

    on_mutation_loss( mut );
}

bool Character::has_active_mutation( const trait_id &b ) const
{
    const auto iter = my_mutations.find( b );
    return iter != my_mutations.end() && iter->second.powered;
}

int Character::get_cost_timer( const trait_id &mut ) const
{
    const auto iter = my_mutations.find( mut );
    if( iter != my_mutations.end() ) {
        return iter->second.charge;
    } else {
        debugmsg( "Tried to get cost timer of %s but doesn't have this mutation.", mut.c_str() );
    }
    return 0;
}

void Character::set_cost_timer( const trait_id &mut, int set )
{
    const auto iter = my_mutations.find( mut );
    if( iter != my_mutations.end() ) {
        iter->second.charge = set;
    } else {
        debugmsg( "Tried to set cost timer of %s but doesn't have this mutation.", mut.c_str() );
    }
}

void Character::mod_cost_timer( const trait_id &mut, int mod )
{
    set_cost_timer( mut, get_cost_timer( mut ) + mod );
}

bool Character::is_category_allowed( const std::vector<mutation_category_id> &category ) const
{
    bool allowed = false;
    bool restricted = false;
    for( const trait_id &mut : get_mutations() ) {
        if( !mut.obj().allowed_category.empty() ) {
            restricted = true;
        }
        for( const mutation_category_id &Mu_cat : category ) {
            if( mut.obj().allowed_category.count( Mu_cat ) ) {
                allowed = true;
                break;
            }
        }

    }
    if( !restricted ) {
        allowed = true;
    }
    return allowed;

}

bool Character::is_category_allowed( const mutation_category_id &category ) const
{
    bool allowed = false;
    bool restricted = false;
    for( const trait_id &mut : get_mutations() ) {
        for( const mutation_category_id &Ch_cat : mut.obj().allowed_category ) {
            restricted = true;
            if( Ch_cat == category ) {
                allowed = true;
            }
        }
    }
    if( !restricted ) {
        allowed = true;
    }
    return allowed;
}

bool Character::is_weak_to_water() const
{
    for( const trait_id &mut : get_mutations() ) {
        if( mut.obj().weakness_to_water > 0 ) {
            return true;
        }
    }
    return false;
}

bool Character::can_use_heal_item( const item &med ) const
{
    const itype_id heal_id = med.typeId();

    bool can_use = false;
    bool got_restriction = false;

    for( const trait_id &mut : get_mutations() ) {
        if( !mut.obj().can_only_heal_with.empty() ) {
            got_restriction = true;
        }
        if( mut.obj().can_only_heal_with.count( heal_id ) ) {
            can_use = true;
            break;
        }
    }
    if( !got_restriction ) {
        can_use = !med.has_flag( STATIC( flag_id( "CANT_HEAL_EVERYONE" ) ) );
    }

    if( !can_use ) {
        for( const trait_id &mut : get_mutations() ) {
            if( mut.obj().can_heal_with.count( heal_id ) ) {
                can_use = true;
                break;
            }
        }
    }

    return can_use;
}

bool Character::can_install_cbm_on_bp( const std::vector<bodypart_id> &bps ) const
{
    bool can_install = true;
    for( const trait_id &mut : get_mutations() ) {
        for( const bodypart_id &bp : bps ) {
            if( mut.obj().no_cbm_on_bp.count( bp.id() ) ) {
                can_install = false;
                break;
            }
        }
    }
    return can_install;
}

void Character::activate_mutation( const trait_id &mut )
{
    const mutation_branch &mdata = mut.obj();
    trait_data &tdata = my_mutations[mut];
    int cost = mdata.cost;
    // You can take yourself halfway to Near Death levels of hunger/thirst.
    // Fatigue can go to Exhausted.
    if( !can_power_mutation( mut ) ) {
        // Insufficient Foo to *maintain* operation is handled in player::suffer
        add_msg_if_player( m_warning, _( "You feel like using your %s would kill you!" ),
                           mutation_name( mut ) );
        return;
    }
    if( tdata.powered && tdata.charge > 0 ) {
        // Already-on units just lose a bit of charge
        tdata.charge--;
    } else {
        // Not-on units, or those with zero charge, have to pay the power cost
        if( mdata.cooldown > 0 ) {
            tdata.charge = mdata.cooldown - 1;
        }
        if( mdata.hunger ) {
            // burn some energy
            mod_stored_kcal( -cost );
        }
        if( mdata.thirst ) {
            mod_thirst( cost );
        }
        if( mdata.fatigue ) {
            mod_fatigue( cost );
        }
        tdata.powered = true;
        recalc_sight_limits();
    }

    if( !mut->enchantments.empty() ) {
        recalculate_enchantment_cache();
    }

    if( !mut->activated_eocs.empty() ) {
        for( const effect_on_condition_id &eoc : mut->activated_eocs ) {
            dialogue d( get_talker_for( *this ), nullptr );
            if( eoc->type == eoc_type::ACTIVATION ) {
                eoc->activate( d );
            } else {
                debugmsg( "Must use an activation eoc for a mutation activation.  If you don't want the effect_on_condition to happen on its own (without the mutation being activated), remove the recurrence min and max.  Otherwise, create a non-recurring effect_on_condition for this mutation with its condition and effects, then have a recurring one queue it." );
            }
        }
        tdata.powered = false;
    }

    if( mdata.transform ) {
        const cata::value_ptr<mut_transform> trans = mdata.transform;
        mod_moves( - trans->moves );
        switch_mutations( mut, trans->target, trans->active );

        if( !mdata.transform->msg_transform.empty() ) {
            add_msg_if_player( m_neutral, mdata.transform->msg_transform );
        }
        return;
    }

    if( mut == trait_WEB_WEAVER ) {
        get_map().add_field( pos(), fd_web, 1 );
        add_msg_if_player( _( "You start spinning web with your spinnerets!" ) );
    } else if( mut == trait_LONG_TONGUE2 ||
               mut == trait_GASTROPOD_EXTREMITY2 ||
               mut == trait_GASTROPOD_EXTREMITY3 ) {
        tdata.powered = false;
        assign_activity( ACT_PULL_CREATURE, to_moves<int>( 1_seconds ), 0, 0, mutation_name( mut ) );
        return;
    } else if( mut == trait_SNAIL_TRAIL ) {
        get_map().add_field( pos(), fd_sludge, 1 );
        add_msg_if_player( _( "You start leaving a trail of sludge as you go." ) );
    } else if( mut == trait_BURROW || mut == trait_BURROWLARGE ) {
        tdata.powered = false;
        item burrowing_item( itype_fake_burrowing );
        invoke_item( &burrowing_item );
        return;  // handled when the activity finishes
    } else if( mut == trait_SLIMESPAWNER ) {
        monster *const slime = g->place_critter_around( mon_player_blob, pos(), 1 );
        if( !slime ) {
            // Oops, no room to divide!
            add_msg_if_player( m_bad, _( "You focus, but are too hemmed in to birth a new slime microbian!" ) );
            tdata.powered = false;
            return;
        }
        add_msg_if_player( m_good,
                           _( "You focus, and with a pleasant splitting feeling, birth a new slime microbian!" ) );
        slime->friendly = -1;
        add_msg_if_player( m_good, SNIPPET.random_from_category( "slime_generate" ).value_or(
                               translation() ).translated() );
        tdata.powered = false;
        return;
    } else if( mut == trait_NAUSEA || mut == trait_VOMITOUS ) {
        vomit();
        tdata.powered = false;
        return;
    } else if( mut == trait_M_FERTILE ) {
        spores();
        tdata.powered = false;
        return;
    } else if( mut == trait_M_BLOOM ) {
        blossoms();
        tdata.powered = false;
        return;
    } else if( mut == trait_M_PROVENANCE ) {
        spores(); // double trouble!
        blossoms();
        tdata.powered = false;
        return;
    } else if( mut == trait_TREE_COMMUNION ) {
        tdata.powered = false;
        if( !overmap_buffer.ter( global_omt_location() ).obj().is_wooded() ) {
            add_msg_if_player( m_info, _( "You can only do that in a wooded area." ) );
            return;
        }
        // Check for adjacent trees.
        bool adjacent_tree = false;
        map &here = get_map();
        for( const tripoint &p2 : here.points_in_radius( pos(), 1 ) ) {
            if( here.has_flag( ter_furn_flag::TFLAG_TREE, p2 ) ) {
                adjacent_tree = true;
            }
        }
        if( !adjacent_tree ) {
            add_msg_if_player( m_info, _( "You can only do that next to a tree." ) );
            return;
        }

        if( has_flag( json_flag_ROOTS2 ) || has_flag( json_flag_ROOTS3 ) ||
            has_flag( json_flag_CHLOROMORPH ) ) {
            add_msg_if_player( _( "You reach out to the trees with your roots." ) );
        } else {
            add_msg_if_player(
                _( "You lay next to the trees letting your hair roots tangle with the trees." ) );
        }

        assign_activity( ACT_TREE_COMMUNION );

        if( has_flag( json_flag_ROOTS2 ) || has_flag( json_flag_ROOTS3 ) ||
            has_flag( json_flag_CHLOROMORPH ) ) {
            const time_duration startup_time = ( has_flag( json_flag_ROOTS3 ) ||
                                                 has_flag( json_flag_CHLOROMORPH ) ) ? rng( 15_minutes,
                                                         30_minutes ) : rng( 60_minutes, 90_minutes );
            activity.values.push_back( to_turns<int>( startup_time ) );
            return;
        } else {
            const time_duration startup_time = rng( 120_minutes, 180_minutes );
            activity.values.push_back( to_turns<int>( startup_time ) );
            return;
        }
    } else if( mut == trait_DEBUG_BIONIC_POWER ) {
        mod_max_power_level_modifier( 100_kJ );
        add_msg_if_player( m_good, _( "Bionic power storage increased by 100." ) );
        tdata.powered = false;
        return;
    } else if( mut == trait_DEBUG_BIONIC_POWERGEN ) {
        int npower;
        if( query_int( npower, "Modify bionic power by how much?  (Values are in millijoules)" ) ) {
            mod_power_level( units::from_millijoule( npower ) );
            add_msg_if_player( m_good, _( "Bionic power increased by %dmJ." ), npower );
            tdata.powered = false;
        }
        return;
    } else if( !mdata.spawn_item.is_empty() ) {
        item tmpitem( mdata.spawn_item );
        i_add_or_drop( tmpitem );
        add_msg_if_player( mdata.spawn_item_message() );
        tdata.powered = false;
        return;
    } else if( !mdata.ranged_mutation.is_empty() ) {
        add_msg_if_player( mdata.ranged_mutation_message() );
        avatar_action::fire_ranged_mutation( *this, item( mdata.ranged_mutation ) );
        tdata.powered = false;
        return;
    }
}

void Character::deactivate_mutation( const trait_id &mut )
{
    my_mutations[mut].powered = false;

    // Handle stat changes from deactivation
    apply_mods( mut, false );
    recalc_sight_limits();
    const mutation_branch &mdata = mut.obj();
    if( mdata.transform ) {
        const cata::value_ptr<mut_transform> trans = mdata.transform;
        mod_moves( -trans->moves );
        switch_mutations( mut, trans->target, trans->active );
    }

    if( !mut->enchantments.empty() ) {
        recalculate_enchantment_cache();
    }

    for( const effect_on_condition_id &eoc : mut->deactivated_eocs ) {
        dialogue d( get_talker_for( *this ), nullptr );
        if( eoc->type == eoc_type::ACTIVATION ) {
            eoc->activate( d );
        } else {
            debugmsg( "Must use an activation eoc for a mutation deactivation.  If you don't want the effect_on_condition to happen on its own (without the mutation being activated), remove the recurrence min and max.  Otherwise, create a non-recurring effect_on_condition for this mutation with its condition and effects, then have a recurring one queue it." );
        }
    }

    if( mdata.transform && !mdata.transform->msg_transform.empty() ) {
        add_msg_if_player( m_neutral, mdata.transform->msg_transform );
    }

}

trait_id Character::trait_by_invlet( const int ch ) const
{
    for( const std::pair<const trait_id, trait_data> &mut : my_mutations ) {
        if( mut.second.key == ch ) {
            return mut.first;
        }
    }
    return trait_id::NULL_ID();
}

bool Character::mutation_ok( const trait_id &mutation, bool force_good, bool force_bad,
                             const vitamin_id &mut_vit, const bool &terminal ) const
{
    if( mut_vit != vitamin_id::NULL_ID() && vitamin_get( mut_vit ) < mutation->vitamin_cost ) {
        // We don't have the required mutagen vitamins
        return false;
    }
    if( !terminal && mutation->terminus ) {
        return false;
    }
    return mutation_ok( mutation, force_good, force_bad );
}

bool Character::mutation_ok( const trait_id &mutation, bool force_good, bool force_bad ) const
{
    if( !is_category_allowed( mutation->category ) ) {
        return false;
    }
    if( mutation_branch::trait_is_blacklisted( mutation ) ) {
        return false;
    }
    if( has_trait( mutation ) || has_child_flag( mutation ) ) {
        // We already have this mutation or something that replaces it.
        return false;
    }

    for( const bionic_id &bid : get_bionics() ) {
        for( const trait_id &mid : bid->canceled_mutations ) {
            if( mid == mutation ) {
                return false;
            }
        }

        if( bid->mutation_conflicts.count( mutation ) != 0 ) {
            return false;
        }
    }

    const mutation_branch &mdata = mutation.obj();
    if( force_bad && mdata.points > 0 ) {
        // This is a good mutation, and we're due for a bad one.
        return false;
    }

    if( force_good && mdata.points < 0 ) {
        // This is a bad mutation, and we're due for a good one.
        return false;
    }

    return true;
}

void Character::mutate( const int &true_random_chance, const bool use_vitamins )
{
    // Determine the highest mutation category
    mutation_category_id cat;
    weighted_int_list<mutation_category_id> cat_list = get_vitamin_weighted_categories();
    const float instability = vitamin_get( vitamin_instability );
    const bool terminal = instability >= 9500;
    const int flaw = rng( 0, ( cat_list.size() == 1 ? 0 : 10 ) + sqrt( instability ) ) +
                     ( instability / 900 );
    bool force_good = flaw < 10;
    bool force_bad = flaw >= 30;

    if( true_random_chance > 0 && one_in( true_random_chance ) ) {
        cat = mutation_category_ANY;
    } else if( cat_list.get_weight() > 0 ) {
        cat = *cat_list.pick();
        cat_list.add_or_replace( cat, 0 );
    } else {
        add_msg_if_player( m_bad,
                           _( "Your body tries to shift, tries to change, but only contorts for a moment.  You crave a more exotic mutagen." ) );
        return;
    }

    std::vector<trait_id> valid; // Valid mutations

    do {
        // See if we should upgrade/extend an existing mutation...
        std::vector<trait_id> upgrades;

        // ... or remove one that is not in our highest category
        std::vector<trait_id> downgrades;

        const vitamin_id mut_vit = use_vitamins ? mutation_category_trait::get_category(
                                       cat ).vitamin : vitamin_id::NULL_ID();

        // For each mutation in category...
        for( const trait_id &traits_iter : mutations_category[cat] ) {
            const trait_id &base_mutation = traits_iter;
            const mutation_branch &base_mdata = traits_iter.obj();

            // ...those we don't have are valid.
            if( base_mdata.valid && is_category_allowed( base_mdata.category ) ) {
                valid.push_back( base_mdata.id );
            }

            // ...for those that we have...
            if( has_trait( base_mutation ) ) {
                // ...consider the mutations that replace it.

                for( const trait_id &mutation : base_mdata.replacements ) {
                    if( mutation->valid && mutation_ok( mutation, force_good, force_bad, mut_vit, terminal ) &&
                        mutation_is_in_category( mutation, cat ) ) {
                        upgrades.push_back( mutation );
                    }
                }

                // ...consider the mutations that add to it.
                for( const trait_id &mutation : base_mdata.additions ) {
                    if( mutation->valid && mutation_ok( mutation, force_good, force_bad, mut_vit, terminal ) &&
                        mutation_is_in_category( mutation, cat ) ) {
                        upgrades.push_back( mutation );
                    }
                }
            }
        }

        // Check whether any of our current mutations are candidates for
        // removal. If the mutation doesn't belong to the current category and
        // can be removed there is 1/4 chance of it being added to the removal
        // candidates list.
        for( const auto &mutations_iter : my_mutations ) {
            const trait_id &mutation_id = mutations_iter.first;
            const mutation_branch &base_mdata = mutation_id.obj();
            if( has_base_trait( mutation_id ) || find( base_mdata.category.begin(), base_mdata.category.end(),
                    cat ) != base_mdata.category.end() ) {
                continue;
            }

            // mark for removal
            // no removing Thresholds/Professions this way!
            // unpurifiable traits also cannot be purified
            // removing a Terminus trait is a definite no
            if( !base_mdata.threshold && !base_mdata.terminus && !base_mdata.profession &&
                base_mdata.purifiable ) {
                if( one_in( 4 ) ) {
                    downgrades.push_back( mutation_id );
                }
            }
        }

        // Prioritize upgrading existing mutations
        if( one_in( 2 ) ) {
            if( !upgrades.empty() ) {
                // (upgrade count) chances to pick an upgrade, 4 chances to pick something else.
                size_t roll = rng( 0, upgrades.size() + 4 );
                if( roll < upgrades.size() ) {
                    // We got a valid upgrade index, so use it and return.
                    mutate_towards( upgrades[roll], cat );
                    return;
                }
            }
        } else {
            // Remove existing mutations that don't fit into our category
            if( !downgrades.empty() ) {
                size_t roll = rng( 0, downgrades.size() + 4 );
                if( roll < downgrades.size() ) {
                    remove_mutation( downgrades[roll] );
                    return;
                }
            }
        }

        // Remove anything we already have, that we have a child of, that
        // goes against our intention of a good/bad mutation, or that we lack resources for
        for( size_t i = 0; i < valid.size(); i++ ) {
            if( ( !mutation_ok( valid[i], force_good, force_bad, mut_vit, terminal ) ) ||
                ( !valid[i]->valid ) ) {
                valid.erase( valid.begin() + i );
                i--;
            }
        }

        if( valid.empty() ) {
            if( cat_list.get_weight() > 0 ) {
                // try to pick again
                cat = *cat_list.pick();
                cat_list.add_or_replace( cat, 0 );
            } else {
                // every option we have vitamins for is invalid
                add_msg_if_player( m_bad,
                                   _( "Your body tries to shift, tries to change, but only contorts for a moment.  You crave a more exotic mutagen." ) );
                return;
            }
        } else {
            if( mut_vit != vitamin_id::NULL_ID() && vitamin_get( mut_vit ) >= 2200 ) {
                test_crossing_threshold( cat );
            }
            if( mutate_towards( valid, cat, 2 ) ) {
                add_msg_if_player( m_mixed, mutation_category_trait::get_category( cat ).mutagen_message() );
            }
            return;
        }
    } while( valid.empty() );
}

void Character::mutate( )
{
    mutate( 1, false );
}

void Character::mutate_category( const mutation_category_id &cat, const bool use_vitamins )
{
    // Hacky ID comparison is better than separate hardcoded branch used before
    // TODO: Turn it into the null id
    if( cat == mutation_category_ANY ) {
        mutate( 0, use_vitamins );
        return;
    }

    const float instability = vitamin_get( vitamin_instability );
    const bool terminal = instability >= 9500;
    const int flaw = rng( 0, sqrt( instability ) ) + ( instability / 900 );
    bool force_good = flaw < 10;
    bool force_bad = flaw >= 30;

    // Pull the category's list for valid mutations
    std::vector<trait_id> valid = mutations_category[cat];

    const vitamin_id mut_vit = use_vitamins ? mutation_category_trait::get_category(
                                   cat ).vitamin : vitamin_id::NULL_ID();

    // Remove anything we already have, that we have a child of, or that
    // goes against our intention of a good/bad mutation
    for( size_t i = 0; i < valid.size(); i++ ) {
        if( !mutation_ok( valid[i], force_good, force_bad, mut_vit, terminal ) ) {
            valid.erase( valid.begin() + i );
            i--;
        }
    }

    mutate_towards( valid, cat, 2 );
}

void Character::mutate_category( const mutation_category_id &cat )
{
    mutate_category( cat, false );
}

static std::vector<trait_id> get_all_mutation_prereqs( const trait_id &id )
{
    std::vector<trait_id> ret;
    for( const trait_id &it : id->prereqs ) {
        ret.push_back( it );
        std::vector<trait_id> these_prereqs = get_all_mutation_prereqs( it );
        ret.insert( ret.end(), these_prereqs.begin(), these_prereqs.end() );
    }
    for( const trait_id &it : id->prereqs2 ) {
        ret.push_back( it );
        std::vector<trait_id> these_prereqs = get_all_mutation_prereqs( it );
        ret.insert( ret.end(), these_prereqs.begin(), these_prereqs.end() );
    }
    return ret;
}

bool Character::mutate_towards( std::vector<trait_id> muts, const mutation_category_id &mut_cat,
                                int num_tries )
{
    while( !muts.empty() && num_tries > 0 ) {
        int i = rng( 0, muts.size() - 1 );

        if( mutate_towards( muts[i], mut_cat ) ) {
            return true;
        }

        muts.erase( muts.begin() + i );
        --num_tries;
    }

    return false;
}

bool Character::mutate_towards( const trait_id &mut, const mutation_category_id &mut_cat,
                                const mutation_variant *chosen_var )
{
    if( has_child_flag( mut ) ) {
        remove_child_flag( mut );
        return true;
    }
    const mutation_branch &mdata = mut.obj();
    // character has...
    bool c_has_both_prereqs = false;
    bool c_has_prereq1 = false;
    bool c_has_prereq2 = false;
    std::vector<trait_id> canceltrait;
    std::vector<trait_id> prereqs1 = mdata.prereqs;
    std::vector<trait_id> prereqs2 = mdata.prereqs2;
    std::vector<trait_id> cancel = mdata.cancels;
    std::vector<trait_id> same_type = get_mutations_in_types( mdata.types );
    std::vector<trait_id> all_prereqs = get_all_mutation_prereqs( mut );
    // mutation has...
    bool mut_has_prereq1 = !mdata.prereqs.empty();
    bool mut_has_prereq2 = !mdata.prereqs2.empty();


    // Check mutations of the same type - except for the ones we might need for pre-reqs
    for( const auto &consider : same_type ) {
        if( std::find( all_prereqs.begin(), all_prereqs.end(), consider ) == all_prereqs.end() ) {
            cancel.push_back( consider );
        }
    }

    std::vector<trait_id> cancel_recheck = cancel;

    for( size_t i = 0; i < cancel.size(); i++ ) {
        if( !has_trait( cancel[i] ) ) {
            cancel.erase( cancel.begin() + i );
            i--;
        } else if( has_base_trait( cancel[i] ) || !purifiable( cancel[i] ) ) {
            //If we have the trait, but it's a base trait, don't allow it to be removed normally
            canceltrait.push_back( cancel[i] );
            cancel.erase( cancel.begin() + i );
            i--;
        }
    }

    for( size_t i = 0; i < cancel.size(); i++ ) {
        if( !cancel.empty() ) {
            trait_id removed = cancel[i];
            remove_mutation( removed );
            cancel.erase( cancel.begin() + i );
            i--;
            //We intentionally don't cancel entire trees, we prune the current top
        }
    }

    //If we still have anything to cancel, we aren't done pruning, but should stop for now
    for( const trait_id &trait : cancel_recheck ) {
        if( has_trait( trait ) ) {
            return true;
        }
    }

    for( auto &m : canceltrait ) {
        if( !purifiable( m ) ) {
            // We can't cancel unpurifiable mutations
            return false;
        }
    }

    for( size_t i = 0; ( !c_has_prereq1 ) && i < prereqs1.size(); i++ ) {
        if( has_trait( prereqs1[i] ) ) {
            c_has_prereq1 = true;
        }
    }

    for( size_t i = 0; ( !c_has_prereq2 ) && i < prereqs2.size(); i++ ) {
        if( has_trait( prereqs2[i] ) ) {
            c_has_prereq2 = true;
        }
    }

    if( c_has_prereq1 && c_has_prereq2 ) {
        c_has_both_prereqs = true;
    }

    // Only mutate in-category prerequisites
    if( mut_cat != mutation_category_ANY ) {
        auto is_not_in_mut_cat = [&mut_cat]( const trait_id & p ) {
            return !mutation_is_in_category( p, mut_cat );
        };
        prereqs1.erase( std::remove_if( prereqs1.begin(), prereqs1.end(), is_not_in_mut_cat ),
                        prereqs1.end() );
        prereqs2.erase( std::remove_if( prereqs2.begin(), prereqs2.end(), is_not_in_mut_cat ),
                        prereqs2.end() );
    }

    // Mutation consistency check should prevent this from ever happening, but raise an error
    // if it somehow does, because otherwise nobody will ever notice
    if( ( mut_has_prereq1 && !c_has_prereq1 && prereqs1.empty() ) ||
        ( mut_has_prereq2 && !c_has_prereq2 && prereqs2.empty() ) ) {
        debugmsg( "Failed to mutate towards %s because a prerequisite is needed but none are available in category %s",
                  mdata.id.c_str(), mut_cat.c_str() );
        return false;
    }

    if( !c_has_both_prereqs && ( !prereqs1.empty() || !prereqs2.empty() ) ) {
        if( !c_has_prereq1 && !prereqs1.empty() ) {
            return mutate_towards( prereqs1, mut_cat );
        } else if( !c_has_prereq2 && !prereqs2.empty() ) {
            return mutate_towards( prereqs2, mut_cat );
        }
    }

    // Check for threshold mutation, if needed
    bool mut_is_threshold = mdata.threshold;
    bool c_has_threshreq = false;
    bool mut_is_profession = mdata.profession;
    std::vector<trait_id> threshreq = mdata.threshreq;

    // It shouldn't pick a Threshold anyway--they're supposed to be non-Valid
    // and aren't categorized. This can happen if someone makes a threshold mutation into a prerequisite.
    if( mut_is_threshold ) {
        add_msg_if_player( _( "You feel something straining deep inside you, yearning to be free…" ) );
        return false;
    }
    if( mut_is_profession ) {
        // Profession picks fail silently
        return false;
    }

    // Just prevent it when it conflicts with a CBM, for now
    // TODO: Consequences?
    for( const bionic_id &bid : get_bionics() ) {
        if( bid->mutation_conflicts.count( mut ) != 0 ) {
            return false;
        }
    }

    for( size_t i = 0; !c_has_threshreq && i < threshreq.size(); i++ ) {
        if( has_trait( threshreq[i] ) ) {
            c_has_threshreq = true;
        }
    }

    // No crossing The Threshold by simply not having it
    if( !c_has_threshreq && !threshreq.empty() ) {
        add_msg_if_player( _( "You feel something straining deep inside you, yearning to be free…" ) );
        return false;
    }

    const vitamin_id mut_vit = mut_cat == mutation_category_ANY ? vitamin_id::NULL_ID() :
                               mutation_category_trait::get_category( mut_cat ).vitamin;

    if( mut_vit != vitamin_id::NULL_ID() ) {
        if( vitamin_get( mut_vit ) >= mdata.vitamin_cost ) {
            vitamin_mod( mut_vit, -mdata.vitamin_cost );
            vitamin_mod( vitamin_instability, mdata.vitamin_cost );
        } else {
            return false;
        }
    }

    // Check if one of the prerequisites that we have TURNS INTO this one
    trait_id replacing = trait_id::NULL_ID();
    prereqs1 = mdata.prereqs; // Reset it
    for( auto &elem : prereqs1 ) {
        if( has_trait( elem ) ) {
            const trait_id &pre = elem;
            const mutation_branch &p = pre.obj();
            for( size_t j = 0; !replacing && j < p.replacements.size(); j++ ) {
                if( p.replacements[j] == mut ) {
                    replacing = pre;
                }
            }
        }
    }

    // Loop through again for prereqs2
    trait_id replacing2 = trait_id::NULL_ID();
    prereqs1 = mdata.prereqs2; // Reset it
    for( auto &elem : prereqs1 ) {
        if( has_trait( elem ) ) {
            const trait_id &pre2 = elem;
            const mutation_branch &p = pre2.obj();
            for( size_t j = 0; !replacing2 && j < p.replacements.size(); j++ ) {
                if( p.replacements[j] == mut ) {
                    replacing2 = pre2;
                }
            }
        }
    }

    bool mutation_replaced = false;

    game_message_type rating;

    if( chosen_var == nullptr ) {
        chosen_var = mut->pick_variant();
    }
    const std::string &variant_id = chosen_var != nullptr ? chosen_var->id : "";
    const std::string gained_name = mdata.name( variant_id );

    if( replacing ) {
        const mutation_branch &replace_mdata = replacing.obj();
        const std::string lost_name = mutation_name( replacing );
        if( mdata.mixed_effect || replace_mdata.mixed_effect ) {
            rating = m_mixed;
        } else if( replace_mdata.points - mdata.points < 0 ) {
            rating = m_good;
        } else if( mdata.points - replace_mdata.points < 0 ) {
            rating = m_bad;
        } else {
            rating = m_neutral;
        }
        // Both new and old mutation visible
        if( mdata.player_display && replace_mdata.player_display ) {
            add_msg_player_or_npc( rating,
                                   _( "Your %1$s mutation turns into %2$s!" ),
                                   _( "<npcname>'s %1$s mutation turns into %2$s!" ),
                                   lost_name, gained_name );
        }
        // New mutation visible, precursor invisible
        if( mdata.player_display && !replace_mdata.player_display ) {
            add_msg_player_or_npc( rating,
                                   _( "You gain a mutation called %s!" ),
                                   _( "<npcname> gains a mutation called %s!" ),
                                   gained_name );
        }
        // Precursor visible, new mutation invisible
        if( !mdata.player_display && replace_mdata.player_display ) {
            add_msg_player_or_npc( rating, _( "You lose your %s mutation." ),
                                   _( "<npcname> loses their %s mutation." ), lost_name );
        }
        get_event_bus().send<event_type::evolves_mutation>( getID(), replace_mdata.id, mdata.id );
        unset_mutation( replacing );
        mutation_replaced = true;
    }
    if( replacing2 ) {
        const mutation_branch &replace_mdata = replacing2.obj();
        const std::string lost_name = mutation_name( replacing2 );
        if( mdata.mixed_effect || replace_mdata.mixed_effect ) {
            rating = m_mixed;
        } else if( replace_mdata.points - mdata.points < 0 ) {
            rating = m_good;
        } else if( mdata.points - replace_mdata.points < 0 ) {
            rating = m_bad;
        } else {
            rating = m_neutral;
        }
        // Both new and old mutation visible
        if( mdata.player_display && replace_mdata.player_display ) {
            add_msg_player_or_npc( rating,
                                   _( "Your %1$s mutation turns into %2$s!" ),
                                   _( "<npcname>'s %1$s mutation turns into %2$s!" ),
                                   lost_name, gained_name );
        }
        // New mutation visible, precursor invisible
        if( mdata.player_display && !replace_mdata.player_display ) {
            add_msg_player_or_npc( rating,
                                   _( "You gain a mutation called %s!" ),
                                   _( "<npcname> gains a mutation called %s!" ),
                                   gained_name );
        }
        // Precursor visible, new mutation invisible
        if( !mdata.player_display && replace_mdata.player_display ) {
            add_msg_player_or_npc( rating,
                                   _( "You lose your %s mutation." ),
                                   _( "<npcname> loses their %s mutation." ), lost_name );
        }
        get_event_bus().send<event_type::evolves_mutation>( getID(), replace_mdata.id, mdata.id );
        unset_mutation( replacing2 );
        mutation_replaced = true;
    }
    for( const auto &i : canceltrait ) {
        const mutation_branch &cancel_mdata = i.obj();
        const std::string lost_name = mutation_name( i );
        if( mdata.mixed_effect || cancel_mdata.mixed_effect ) {
            rating = m_mixed;
        } else if( mdata.points < cancel_mdata.points ) {
            rating = m_bad;
        } else if( mdata.points > cancel_mdata.points ) {
            rating = m_good;
        } else {
            rating = m_neutral;
        }
        // If this new mutation cancels a base trait, remove it and add the mutation at the same time
        if( mdata.player_display ) {
            add_msg_player_or_npc( rating,
                                   _( "Your innate %1$s trait turns into %2$s!" ),
                                   _( "<npcname>'s innate %1$s trait turns into %2$s!" ),
                                   lost_name, gained_name );
        } else {
            add_msg_player_or_npc( rating,
                                   _( "You lose your innate %1$s trait!" ),
                                   _( "<npcname> loses their innate %1$s trait!" ),
                                   lost_name );
        }
        get_event_bus().send<event_type::evolves_mutation>( getID(), cancel_mdata.id, mdata.id );
        unset_mutation( i );
        mutation_replaced = true;
    }
    if( !mutation_replaced ) {
        if( mdata.mixed_effect ) {
            rating = m_mixed;
        } else if( mdata.points > 0 ) {
            rating = m_good;
        } else if( mdata.points < 0 ) {
            rating = m_bad;
        } else {
            rating = m_neutral;
        }
        // Only print a message on visible mutations
        if( mdata.player_display ) {
            add_msg_player_or_npc( rating,
                                   _( "You gain a mutation called %s!" ),
                                   _( "<npcname> gains a mutation called %s!" ),
                                   gained_name );
        }
        get_event_bus().send<event_type::gains_mutation>( getID(), mdata.id );
    }

    set_mutation( mut, chosen_var );

    calc_mutation_levels();
    drench_mut_calc();
    return true;
}

bool Character::mutate_towards( const trait_id &mut, const mutation_variant *chosen_var )
{
    return mutate_towards( mut, mutation_category_ANY, chosen_var );
}

bool Character::has_conflicting_trait( const trait_id &flag ) const
{
    return has_opposite_trait( flag ) || has_lower_trait( flag ) || has_replacement_trait( flag ) ||
           has_same_type_trait( flag );
}

std::unordered_set<trait_id> Character::get_conflicting_traits( const trait_id &flag ) const
{
    std::unordered_set<trait_id> traits;
    return traits
           << get_opposite_traits( flag )
           << get_lower_traits( flag )
           << get_replacement_traits( flag )
           << get_same_type_traits( flag );
}

bool Character::has_lower_trait( const trait_id &flag ) const
{
    return !get_lower_traits( flag ).empty();
}

std::unordered_set<trait_id> Character::get_lower_traits( const trait_id &flag ) const
{
    std::unordered_set<trait_id> traits;
    for( const trait_id &i : flag->prereqs ) {
        if( has_trait( i ) ) {
            traits.insert( i );
        }
        traits = traits << ( get_lower_traits( i ) );
    }
    for( const trait_id &i : flag->prereqs2 ) {
        if( has_trait( i ) ) {
            traits.insert( i );
        }
        traits = traits << ( get_lower_traits( i ) );
    }
    return traits;
}

bool Character::has_replacement_trait( const trait_id &flag ) const
{
    return !get_replacement_traits( flag ).empty();
}

std::unordered_set<trait_id> Character::get_replacement_traits( const trait_id &flag ) const
{
    std::unordered_set<trait_id> traits;
    for( const trait_id &i : flag->replacements ) {
        if( has_trait( i ) ) {
            traits.insert( i );
        } else {
            traits = traits << ( get_replacement_traits( i ) );
        }
    }
    return traits;
}

bool Character::has_addition_trait( const trait_id &flag ) const
{
    return !get_addition_traits( flag ).empty();
}

std::unordered_set<trait_id> Character::get_addition_traits( const trait_id &flag ) const
{
    std::unordered_set<trait_id> traits;
    for( const trait_id &i : flag->additions ) {
        if( has_trait( i ) ) {
            traits.insert( i );
        } else {
            traits = traits << ( get_addition_traits( i ) );
        }
    }
    return traits;
}

bool Character::has_same_type_trait( const trait_id &flag ) const
{
    return !get_same_type_traits( flag ).empty();
}

std::unordered_set<trait_id> Character::get_same_type_traits( const trait_id &flag ) const
{
    std::unordered_set<trait_id> traits;
    for( auto &i : get_mutations_in_types( flag->types ) ) {
        if( has_trait( i ) && flag != i ) {
            traits.insert( i );
        }
    }
    return traits;
}

bool Character::purifiable( const trait_id &flag ) const
{
    return flag->purifiable;
}

/// Returns a randomly selected dream
std::string Character::get_category_dream( const mutation_category_id &cat,
        int strength ) const
{
    std::vector<dream> valid_dreams;
    //Pull the list of dreams
    for( dream &i : dreams ) {
        //Pick only the ones matching our desired category and strength
        if( ( i.category == cat ) && ( i.strength == strength ) ) {
            // Put the valid ones into our list
            valid_dreams.push_back( i );
        }
    }
    if( valid_dreams.empty() ) {
        return "";
    }
    const dream &selected_dream = random_entry( valid_dreams );
    return random_entry( selected_dream.messages() );
}

void Character::remove_mutation( const trait_id &mut, bool silent )
{
    const mutation_branch &mdata = mut.obj();
    // Check if there's a prerequisite we should shrink back into
    trait_id replacing = trait_id::NULL_ID();
    std::vector<trait_id> originals = mdata.prereqs;
    for( size_t i = 0; !replacing && i < originals.size(); i++ ) {
        trait_id pre = originals[i];
        const mutation_branch &p = pre.obj();
        for( size_t j = 0; !replacing && j < p.replacements.size(); j++ ) {
            if( p.replacements[j] == mut ) {
                replacing = pre;
            }
        }
    }

    trait_id replacing2 = trait_id::NULL_ID();
    std::vector<trait_id> originals2 = mdata.prereqs2;
    for( size_t i = 0; !replacing2 && i < originals2.size(); i++ ) {
        trait_id pre2 = originals2[i];
        const mutation_branch &p = pre2.obj();
        for( size_t j = 0; !replacing2 && j < p.replacements.size(); j++ ) {
            if( p.replacements[j] == mut ) {
                replacing2 = pre2;
            }
        }
    }

    // See if this mutation is canceled by a base trait
    //Only if there's no prerequisite to shrink to, thus we're at the bottom of the trait line
    if( !replacing ) {
        //Check each mutation until we reach the end or find a trait to revert to
        for( const mutation_branch &iter : mutation_branch::get_all() ) {
            //See if it's in our list of base traits but not active
            if( has_base_trait( iter.id ) && !has_trait( iter.id ) ) {
                //See if that base trait cancels the mutation we are using
                std::vector<trait_id> traitcheck = iter.cancels;
                for( size_t j = 0; !replacing && j < traitcheck.size(); j++ ) {
                    if( traitcheck[j] == mut ) {
                        replacing = ( iter.id );
                    }
                }
            }
            if( replacing ) {
                break;
            }
        }
    }

    // Duplicated for prereq2
    if( !replacing2 ) {
        //Check each mutation until we reach the end or find a trait to revert to
        for( const mutation_branch &iter : mutation_branch::get_all() ) {
            //See if it's in our list of base traits but not active
            if( has_base_trait( iter.id ) && !has_trait( iter.id ) ) {
                //See if that base trait cancels the mutation we are using
                std::vector<trait_id> traitcheck = iter.cancels;
                for( size_t j = 0; !replacing2 && j < traitcheck.size(); j++ ) {
                    if( traitcheck[j] == mut && ( iter.id ) != replacing ) {
                        replacing2 = ( iter.id );
                    }
                }
            }
            if( replacing2 ) {
                break;
            }
        }
    }

    // make sure we don't toggle a mutation or trait twice, or it will cancel itself out.
    if( replacing == replacing2 ) {
        replacing2 = trait_id::NULL_ID();
    }

    // This should revert back to a removed base trait rather than simply removing the mutation
    unset_mutation( mut );

    bool mutation_replaced = false;

    game_message_type rating;

    const std::string lost_name = mutation_name( mut );

    if( replacing ) {
        const mutation_variant *chosen_var = replacing->pick_variant();
        const std::string &variant_id = chosen_var != nullptr ? chosen_var->id : "";

        const mutation_branch &replace_mdata = replacing.obj();
        const std::string replace_name = replace_mdata.name( variant_id );
        // Don't print a message if both are invisible
        if( !mdata.player_display && !replace_mdata.player_display ) {
            silent = true;
        }
        if( mdata.mixed_effect || replace_mdata.mixed_effect ) {
            rating = m_mixed;
        } else if( replace_mdata.points - mdata.points > 0 ) {
            rating = m_good;
        } else if( mdata.points - replace_mdata.points > 0 ) {
            rating = m_bad;
        } else {
            rating = m_neutral;
        }
        if( !silent ) {
            // Both visible
            if( mdata.player_display && replace_mdata.player_display ) {
                add_msg_player_or_npc( rating,
                                       _( "Your %1$s mutation turns into %2$s." ),
                                       _( "<npcname>'s %1$s mutation turns into %2$s." ),
                                       lost_name, replace_name );
            }
            // Old trait invisible, new visible
            if( !mdata.player_display && replace_mdata.player_display ) {
                add_msg_player_or_npc( rating,
                                       _( "You gain a mutation called %s!" ),
                                       _( "<npcname> gains a mutation called %s!" ),
                                       replace_name );
            }
            // Old trait visible, new invisible
            if( mdata.player_display && !replace_mdata.player_display ) {
                add_msg_player_or_npc( rating,
                                       _( "You lose your %s mutation." ),
                                       _( "<npcname> loses their %s mutation." ),
                                       lost_name );
            }
        }
        set_mutation( replacing, chosen_var );
        mutation_replaced = true;
    }
    if( replacing2 ) {
        const mutation_variant *chosen_var = replacing2->pick_variant();
        const std::string &variant_id = chosen_var != nullptr ? chosen_var->id : "";

        const mutation_branch &replace_mdata = replacing2.obj();
        const std::string replace_name = replace_mdata.name( variant_id );
        // Don't print a message if both are invisible
        if( !mdata.player_display && !replace_mdata.player_display ) {
            silent = true;
        }
        if( mdata.mixed_effect || replace_mdata.mixed_effect ) {
            rating = m_mixed;
        } else if( replace_mdata.points - mdata.points > 0 ) {
            rating = m_good;
        } else if( mdata.points - replace_mdata.points > 0 ) {
            rating = m_bad;
        } else {
            rating = m_neutral;
        }
        if( !silent ) {
            // Both visible
            if( mdata.player_display && replace_mdata.player_display ) {
                add_msg_player_or_npc( rating,
                                       _( "Your %1$s mutation turns into %2$s." ),
                                       _( "<npcname>'s %1$s mutation turns into %2$s." ),
                                       lost_name, replace_name );
            }
            // Old trait invisible, new visible
            if( !mdata.player_display && replace_mdata.player_display ) {
                add_msg_player_or_npc( rating,
                                       _( "You gain a mutation called %s!" ),
                                       _( "<npcname> gains a mutation called %s!" ),
                                       replace_name );
            }
            // Old trait visible, new invisible
            if( mdata.player_display && !replace_mdata.player_display ) {
                add_msg_player_or_npc( rating,
                                       _( "You lose your %s mutation." ),
                                       _( "<npcname> loses their %s mutation." ),
                                       lost_name );
            }
        }
        set_mutation( replacing2, chosen_var );
        mutation_replaced = true;
    }
    if( !mutation_replaced ) {
        if( !mdata.player_display ) {
            silent = true;
        }
        if( mdata.mixed_effect ) {
            rating = m_mixed;
        } else if( mdata.points > 0 ) {
            rating = m_bad;
        } else if( mdata.points < 0 ) {
            rating = m_good;
        } else {
            rating = m_neutral;
        }
        if( !silent ) {
            add_msg_player_or_npc( rating,
                                   _( "You lose your %s mutation." ),
                                   _( "<npcname> loses their %s mutation." ),
                                   lost_name );
        }
    }

    calc_mutation_levels();
    drench_mut_calc();
}

bool Character::has_child_flag( const trait_id &flag ) const
{
    for( const trait_id &elem : flag->replacements ) {
        const trait_id &tmp = elem;
        if( has_trait( tmp ) || has_child_flag( tmp ) ) {
            return true;
        }
    }
    return false;
}

void Character::remove_child_flag( const trait_id &flag )
{
    for( const auto &elem : flag->replacements ) {
        const trait_id &tmp = elem;
        if( has_trait( tmp ) ) {
            remove_mutation( tmp );
            return;
        } else if( has_child_flag( tmp ) ) {
            remove_child_flag( tmp );
            return;
        }
    }
}

void Character::test_crossing_threshold( const mutation_category_id &mutation_category )
{
    // Threshold-check.  You only get to cross once!
    if( crossed_threshold() ) {
        return;
    }

    const mutation_category_trait &m_category = mutation_category_trait::get_category(
                mutation_category );

    // If there is no threshold for this category, don't check it
    const trait_id &mutation_thresh = m_category.threshold_mut;
    if( mutation_thresh.is_empty() ) {
        return;
    }

    // Threshold-breaching
    int breach_power = mutation_category_level[mutation_category];
    // You're required to have hit third-stage dreams first.
    if( breach_power > 30 ) {
        if( breach_power >= 100 || x_in_y( breach_power, 100 ) ) {
            const mutation_branch &thrdata = mutation_thresh.obj();
            if( vitamin_get( m_category.vitamin ) >= thrdata.vitamin_cost ) {
                vitamin_mod( m_category.vitamin, -thrdata.vitamin_cost );
                add_msg_if_player( m_good,
                                   _( "Something strains mightily for a moment… and then… you're… FREE!" ) );
                // Thresholds can cancel unpurifiable traits
                for( const trait_id &canceled : thrdata.cancels ) {
                    unset_mutation( canceled );
                }
                set_mutation( mutation_thresh );
                get_event_bus().send<event_type::crosses_mutation_threshold>( getID(), m_category.id );
            }
        }
    }
}

bool are_conflicting_traits( const trait_id &trait_a, const trait_id &trait_b )
{
    return are_opposite_traits( trait_a, trait_b ) || b_is_lower_trait_of_a( trait_a, trait_b )
           || b_is_higher_trait_of_a( trait_a, trait_b ) || are_same_type_traits( trait_a, trait_b );
}

bool are_opposite_traits( const trait_id &trait_a, const trait_id &trait_b )
{
    return contains_trait( trait_a->cancels, trait_b );
}

bool b_is_lower_trait_of_a( const trait_id &trait_a, const trait_id &trait_b )
{
    return contains_trait( trait_a->prereqs, trait_b );
}

bool b_is_higher_trait_of_a( const trait_id &trait_a, const trait_id &trait_b )
{
    return contains_trait( trait_a->replacements, trait_b );
}

bool are_same_type_traits( const trait_id &trait_a, const trait_id &trait_b )
{
    return contains_trait( get_mutations_in_types( trait_a->types ), trait_b );
}

bool contains_trait( std::vector<string_id<mutation_branch>> traits, const trait_id &trait )
{
    return std::find( traits.begin(), traits.end(), trait ) != traits.end();
}

void Character::give_all_mutations( const mutation_category_trait &category,
                                    const bool include_postthresh )
{
    const std::vector<trait_id> category_mutations = mutations_category[category.id];

    // Add the threshold mutation first
    if( include_postthresh && !category.threshold_mut.is_empty() ) {
        set_mutation( category.threshold_mut );
    }

    // Store current vitamin level, we fake it high to make sure we have enough, restore later
    int v_store = vitamin_get( category.vitamin );

    for( const trait_id &mut : category_mutations ) {
        const mutation_branch &mut_data = *mut;
        if( !mut_data.threshold ) {
            vitamin_set( category.vitamin, INT_MAX );

            // Try up to 10 times to mutate towards this trait
            int mutation_attempts = 10;
            while( mutation_attempts > 0 && mutation_ok( mut, false, false ) ) {
                mutate_towards( mut, category.id );
                --mutation_attempts;
            }
        }
    }
    vitamin_set( category.vitamin, v_store );
}

void Character::unset_all_mutations()
{
    for( const trait_id &mut : get_mutations() ) {
        unset_mutation( mut );
    }
}

std::string Character::mutation_name( const trait_id &mut ) const
{
    auto it = my_mutations.find( mut );
    if( it != my_mutations.end() && it->second.variant != nullptr ) {
        return mut->name( it->second.variant->id );
    }

    return mut->name();
}

std::string Character::mutation_desc( const trait_id &mut ) const
{
    auto it = my_mutations.find( mut );
    if( it != my_mutations.end() && it->second.variant != nullptr ) {
        return mut->desc( it->second.variant->id );
    }

    return mut->desc();
}


void Character::customize_appearance( customize_appearance_choice choice )
{
    uilist amenu;
    trait_id current_trait;

    auto make_entries = [this, &amenu, &current_trait]( const std::vector<trait_id> &traits ) {
        const size_t iterations = traits.size();
        for( int i = 0; i < static_cast<int>( iterations ); ++i ) {
            const trait_id &trait = traits[i];
            bool char_has_trait = false;
            if( this->has_trait( trait ) ) {
                current_trait = trait;
                char_has_trait = true;
            }

            const std::string &entry_name = mutation_name( trait );

            amenu.addentry(
                i, true, MENU_AUTOASSIGN,
                char_has_trait ? entry_name + " *" : entry_name
            );
        }
    };

    std::vector<trait_id> traits;
    std::string end_message;
    switch( choice ) {
        case customize_appearance_choice::EYES:
            amenu.text = _( "Choose a new eye color" );
            traits = get_mutations_in_type( STATIC( "eye_color" ) );
            end_message = _( "Maybe things will be better by seeing it with new eyes." );
            break;
        case customize_appearance_choice::HAIR:
            amenu.text = _( "Choose a new hairstyle" );
            traits = get_mutations_in_type( STATIC( "hair_style" ) );
            end_message = _( "A change in hairstyle will freshen up the mood." );
            break;
        case customize_appearance_choice::HAIR_F:
            amenu.text = _( "Choose a new facial hairstyle" );
            traits = get_mutations_in_type( STATIC( "facial_hair" ) );
            end_message = _( "Surviving the end with style." );
            break;
        case customize_appearance_choice::SKIN:
            amenu.text = _( "Choose a new skin color" );
            traits = get_mutations_in_type( STATIC( "skin_tone" ) );
            end_message = _( "Life in the Cataclysm seems to have changed you." );
            break;
    }

    traits.erase(
    std::remove_if( traits.begin(), traits.end(), []( const trait_id & traitid ) {
        return !traitid->vanity;
    } ), traits.end() );

    if( traits.empty() ) {
        popup( _( "No traits found." ) );
    }

    make_entries( traits );

    amenu.query();
    if( amenu.ret >= 0 ) {
        const trait_id &trait_selected = traits[amenu.ret];
        if( has_trait( current_trait ) ) {
            remove_mutation( current_trait );
        }
        set_mutation( trait_selected );
        if( one_in( 3 ) ) {
            add_msg( m_neutral, end_message );
        }
    }
}

std::string Character::visible_mutations( const int visibility_cap ) const
{
    const std::vector<trait_id> &my_muts = get_mutations();
    const std::string trait_str = enumerate_as_string( my_muts.begin(), my_muts.end(),
    [this, visibility_cap ]( const trait_id & pr ) -> std::string {
        const mutation_branch &mut_branch = pr.obj();
        // Finally some use for visibility trait of mutations
        if( mut_branch.visibility > 0 && mut_branch.visibility >= visibility_cap )
        {
            return colorize( mutation_name( pr ), mut_branch.get_display_color() );
        }

        return std::string();
    } );
    return trait_str;
}

