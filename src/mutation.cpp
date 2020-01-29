#include "mutation.h"

#include <cstddef>
#include <algorithm>
#include <list>
#include <unordered_set>

#include "avatar_action.h"
#include "event_bus.h"
#include "field.h"
#include "game.h"
#include "item.h"
#include "itype.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "memorial_logger.h"
#include "monster.h"
#include "output.h"
#include "overmapbuffer.h"
#include "player.h"
#include "translations.h"
#include "omdata.h"
#include "player_activity.h"
#include "rng.h"
#include "string_id.h"
#include "enums.h"
#include "cata_string_consts.h"

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
    debugmsg( "Invalid mutagen_technique" );
    abort();
}

} // namespace io

bool Character::has_trait( const trait_id &b ) const
{
    return my_mutations.count( b ) > 0;
}

bool Character::has_trait_flag( const std::string &b ) const
{
    // UGLY, SLOW, should be cached as my_mutation_flags or something
    for( const auto &mut : my_mutations ) {
        auto &mut_data = mut.first.obj();
        if( mut_data.flags.count( b ) > 0 ) {
            return true;
        }
    }

    return false;
}

bool Character::has_base_trait( const trait_id &b ) const
{
    // Look only at base traits
    return my_traits.find( b ) != my_traits.end();
}

void Character::toggle_trait( const trait_id &flag )
{
    const auto titer = my_traits.find( flag );
    if( titer == my_traits.end() ) {
        my_traits.insert( flag );
    } else {
        my_traits.erase( titer );
    }
    const auto miter = my_mutations.find( flag );
    if( miter == my_mutations.end() ) {
        set_mutation( flag );
        mutation_effect( flag );
    } else {
        unset_mutation( flag );
        mutation_loss_effect( flag );
    }
}

void Character::set_mutation( const trait_id &flag )
{
    const auto iter = my_mutations.find( flag );
    if( iter == my_mutations.end() ) {
        my_mutations[flag]; // Creates a new entry with default values
        cached_mutations.push_back( &flag.obj() );
    } else {
        return;
    }
    recalc_sight_limits();
    reset_encumbrance();
}

void Character::unset_mutation( const trait_id &flag )
{
    const auto iter = my_mutations.find( flag );
    if( iter != my_mutations.end() ) {
        my_mutations.erase( iter );
        const mutation_branch &mut = *flag;
        cached_mutations.erase( std::remove( cached_mutations.begin(), cached_mutations.end(), &mut ),
                                cached_mutations.end() );
    } else {
        return;
    }
    recalc_sight_limits();
    reset_encumbrance();
}

int Character::get_mod( const trait_id &mut, const std::string &arg ) const
{
    auto &mod_data = mut->mods;
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

    if( str_change != 0 ) {
        recalc_hp();
    }
}

bool mutation_branch::conflicts_with_item( const item &it ) const
{
    if( allow_soft_gear && it.is_soft() ) {
        return false;
    }

    for( body_part bp : restricts_gear ) {
        if( it.covers( bp ) ) {
            return true;
        }
    }

    return false;
}

const resistances &mutation_branch::damage_resistance( body_part bp ) const
{
    const auto iter = armor.find( bp );
    if( iter == armor.end() ) {
        static const resistances nulres;
        return nulres;
    }

    return iter->second;
}

void Character::mutation_effect( const trait_id &mut )
{
    if( mut == "GLASSJAW" ) {
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

    const auto &branch = mut.obj();
    if( branch.hp_modifier != 0.0f || branch.hp_modifier_secondary != 0.0f ||
        branch.hp_adjustment != 0.0f ) {
        recalc_hp();
    }

    remove_worn_items_with( [&]( item & armor ) {
        static const std::string mutation_safe = "OVERSIZE";
        if( armor.has_flag( mutation_safe ) ) {
            return false;
        }
        if( !branch.conflicts_with_item( armor ) ) {
            return false;
        }
        if( branch.destroys_gear ) {
            add_msg_player_or_npc( m_bad,
                                   _( "Your %s is destroyed!" ),
                                   _( "<npcname>'s %s is destroyed!" ),
                                   armor.tname() );
            for( item &remain : armor.contents ) {
                g->m.add_item_or_charges( pos(), remain );
            }
        } else {
            add_msg_player_or_npc( m_bad,
                                   _( "Your %s is pushed off!" ),
                                   _( "<npcname>'s %s is pushed off!" ),
                                   armor.tname() );
            g->m.add_item_or_charges( pos(), armor );
        }
        return true;
    } );

    if( branch.starts_active ) {
        my_mutations[mut].powered = true;
    }

    on_mutation_gain( mut );
}

void Character::mutation_loss_effect( const trait_id &mut )
{
    if( mut == "GLASSJAW" ) {
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

    const auto &branch = mut.obj();
    if( branch.hp_modifier != 0.0f || branch.hp_modifier_secondary != 0.0f ||
        branch.hp_adjustment != 0.0f ) {
        recalc_hp();
    }

    on_mutation_loss( mut );
}

bool Character::has_active_mutation( const trait_id &b ) const
{
    const auto iter = my_mutations.find( b );
    return iter != my_mutations.end() && iter->second.powered;
}

bool Character::is_category_allowed( const std::vector<std::string> &category ) const
{
    bool allowed = false;
    bool restricted = false;
    for( const trait_id &mut : get_mutations() ) {
        if( !mut.obj().allowed_category.empty() ) {
            restricted = true;
        }
        for( const std::string &Mu_cat : category ) {
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

bool Character::is_category_allowed( const std::string &category ) const
{
    bool allowed = false;
    bool restricted = false;
    for( const trait_id &mut : get_mutations() ) {
        for( const std::string &Ch_cat : mut.obj().allowed_category ) {
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
    for( const auto &mut : my_mutations ) {
        if( mut.first.obj().weakness_to_water > 0 ) {
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
        can_use = !med.has_flag( "CANT_HEAL_EVERYONE" );
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

bool Character::can_install_cbm_on_bp( const std::vector<body_part> &bps ) const
{
    bool can_install = true;
    for( const trait_id &mut : get_mutations() ) {
        for( const body_part bp : bps ) {
            if( mut.obj().no_cbm_on_bp.count( bp ) ) {
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
    // Preserve the fake weapon used to initiate ranged mutation firing
    static item mut_ranged( weapon );
    // You can take yourself halfway to Near Death levels of hunger/thirst.
    // Fatigue can go to Exhausted.
    if( ( mdata.hunger && get_kcal_percent() < 0.5f ) || ( mdata.thirst &&
            get_thirst() >= 260 ) ||
        ( mdata.fatigue && get_fatigue() >= EXHAUSTED ) ) {
        // Insufficient Foo to *maintain* operation is handled in player::suffer
        add_msg_if_player( m_warning, _( "You feel like using your %s would kill you!" ),
                           mdata.name() );
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
            mod_stored_nutr( cost );
        }
        if( mdata.thirst ) {
            mod_thirst( cost );
        }
        if( mdata.fatigue ) {
            mod_fatigue( cost );
        }
        tdata.powered = true;

        // Handle stat changes from activation
        apply_mods( mut, true );
        recalc_sight_limits();
    }

    if( mut == trait_WEB_WEAVER ) {
        g->m.add_field( pos(), fd_web, 1 );
        add_msg_if_player( _( "You start spinning web with your spinnerets!" ) );
    } else if( mut == "BURROW" ) {
        tdata.powered = false;
        item burrowing_item( itype_id( "fake_burrowing" ) );
        invoke_item( &burrowing_item );
        return;  // handled when the activity finishes
    } else if( mut == trait_SLIMESPAWNER ) {
        monster *const slime = g->place_critter_around( mtype_id( "mon_player_blob" ), pos(), 1 );
        if( !slime ) {
            // Oops, no room to divide!
            add_msg_if_player( m_bad, _( "You focus, but are too hemmed in to birth a new slimespring!" ) );
            tdata.powered = false;
            return;
        }
        add_msg_if_player( m_good,
                           _( "You focus, and with a pleasant splitting feeling, birth a new slimespring!" ) );
        slime->friendly = -1;
        if( one_in( 3 ) ) {
            add_msg_if_player( m_good,
                               //~ Usual enthusiastic slimespring small voices! :D
                               _( "wow!  you look just like me!  we should look out for each other!" ) );
        } else if( one_in( 2 ) ) {
            //~ Usual enthusiastic slimespring small voices! :D
            add_msg_if_player( m_good, _( "come on, big me, let's go!" ) );
        } else {
            //~ Usual enthusiastic slimespring small voices! :D
            add_msg_if_player( m_good, _( "we're a team, we've got this!" ) );
        }
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
    } else if( mut == trait_SELFAWARE ) {
        print_health();
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
        for( const tripoint &p2 : g->m.points_in_radius( pos(), 1 ) ) {
            if( g->m.has_flag( "TREE", p2 ) ) {
                adjacent_tree = true;
            }
        }
        if( !adjacent_tree ) {
            add_msg_if_player( m_info, _( "You can only do that next to a tree." ) );
            return;
        }

        if( has_trait( trait_ROOTS2 ) || has_trait( trait_ROOTS3 ) ) {
            add_msg_if_player( _( "You reach out to the trees with your roots." ) );
        } else {
            add_msg_if_player(
                _( "You lay next to the trees letting your hair roots tangle with the trees." ) );
        }

        assign_activity( ACT_TREE_COMMUNION );

        if( has_trait( trait_ROOTS2 ) || has_trait( trait_ROOTS3 ) ) {
            const time_duration startup_time = has_trait( trait_ROOTS3 ) ? rng( 15_minutes,
                                               30_minutes ) : rng( 60_minutes, 90_minutes );
            activity.values.push_back( to_turns<int>( startup_time ) );
            return;
        } else {
            const time_duration startup_time = rng( 120_minutes, 180_minutes );
            activity.values.push_back( to_turns<int>( startup_time ) );
            return;
        }
    } else if( mut == trait_DEBUG_BIONIC_POWER ) {
        mod_max_power_level( 100_kJ );
        add_msg_if_player( m_good, _( "Bionic power storage increased by 100." ) );
        tdata.powered = false;
        return;
    } else if( mut == trait_DEBUG_BIONIC_POWERGEN ) {
        int npower;
        if( query_int( npower, "Modify bionic power by how much?  (Values are in millijoules)" ) ) {
            mod_power_level( units::from_millijoule( npower ) );
            add_msg_if_player( m_good, "Bionic power increased by %dmJ.", npower );
            tdata.powered = false;
        }
        return;
    } else if( !mdata.spawn_item.empty() ) {
        item tmpitem( mdata.spawn_item );
        i_add_or_drop( tmpitem );
        add_msg_if_player( mdata.spawn_item_message() );
        tdata.powered = false;
        return;
    } else if( !mdata.ranged_mutation.empty() ) {
        mut_ranged = item( mdata.ranged_mutation );
        add_msg_if_player( mdata.ranged_mutation_message() );
        g->refresh_all();
        avatar_action::fire( g->u, g->m, mut_ranged );
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
}

trait_id Character::trait_by_invlet( const int ch ) const
{
    for( auto &mut : my_mutations ) {
        if( mut.second.key == ch ) {
            return mut.first;
        }
    }
    return trait_id::NULL_ID();
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

void Character::mutate()
{
    bool force_bad = one_in( 3 );
    bool force_good = false;
    if( has_trait( trait_ROBUST ) && force_bad ) {
        // Robust Genetics gives you a 33% chance for a good mutation,
        // instead of the 33% chance of a bad one.
        force_bad = false;
        force_good = true;
    }
    if( has_trait( trait_CHAOTIC_BAD ) ) {
        force_bad = true;
        force_good = false;
    }

    // Determine the highest mutation category
    std::string cat = get_highest_category();

    if( !is_category_allowed( cat ) ) {
        cat.clear();
    }

    // See if we should upgrade/extend an existing mutation...
    std::vector<trait_id> upgrades;

    // ... or remove one that is not in our highest category
    std::vector<trait_id> downgrades;

    // For each mutation...
    for( const mutation_branch &traits_iter : mutation_branch::get_all() ) {
        const trait_id &base_mutation = traits_iter.id;
        const mutation_branch &base_mdata = traits_iter;
        bool thresh_save = base_mdata.threshold;
        bool prof_save = base_mdata.profession;
        // are we unpurifiable? (saved from mutating away)
        bool purify_save = !base_mdata.purifiable;

        // ...that we have...
        if( has_trait( base_mutation ) ) {
            // ...consider the mutations that replace it.
            for( const trait_id &mutation : base_mdata.replacements ) {
                bool valid_ok = mutation->valid;

                if( ( mutation_ok( mutation, force_good, force_bad ) ) &&
                    ( valid_ok ) ) {
                    upgrades.push_back( mutation );
                }
            }

            // ...consider the mutations that add to it.
            for( const trait_id &mutation : base_mdata.additions ) {
                bool valid_ok = mutation->valid;

                if( ( mutation_ok( mutation, force_good, force_bad ) ) &&
                    ( valid_ok ) ) {
                    upgrades.push_back( mutation );
                }
            }

            // ...consider whether its in our highest category
            if( has_trait( base_mutation ) && !has_base_trait( base_mutation ) ) {
                // Starting traits don't count toward categories
                std::vector<trait_id> group = mutations_category[cat];
                bool in_cat = false;
                for( const trait_id &elem : group ) {
                    if( elem == base_mutation ) {
                        in_cat = true;
                        break;
                    }
                }

                // mark for removal
                // no removing Thresholds/Professions this way!
                // unpurifiable traits also cannot be purified
                if( !in_cat && !thresh_save && !prof_save && !purify_save ) {
                    if( one_in( 4 ) ) {
                        downgrades.push_back( base_mutation );
                    }
                }
            }
        }
    }

    // Preliminary round to either upgrade or remove existing mutations
    if( one_in( 2 ) ) {
        if( !upgrades.empty() ) {
            // (upgrade count) chances to pick an upgrade, 4 chances to pick something else.
            size_t roll = rng( 0, upgrades.size() + 4 );
            if( roll < upgrades.size() ) {
                // We got a valid upgrade index, so use it and return.
                mutate_towards( upgrades[roll] );
                return;
            }
        }
    } else {
        // Remove existing mutations that don't fit into our category
        if( !downgrades.empty() && !cat.empty() ) {
            size_t roll = rng( 0, downgrades.size() + 4 );
            if( roll < downgrades.size() ) {
                remove_mutation( downgrades[roll] );
                return;
            }
        }
    }

    std::vector<trait_id> valid; // Valid mutations
    bool first_pass = true;

    do {
        // If we tried once with a non-NULL category, and couldn't find anything valid
        // there, try again with empty category
        // CHAOTIC_BAD lets the game pull from any category by default
        if( !first_pass || has_trait( trait_CHAOTIC_BAD ) ) {
            cat.clear();
        }

        if( cat.empty() ) {
            // Pull the full list
            for( const mutation_branch &traits_iter : mutation_branch::get_all() ) {
                if( traits_iter.valid && is_category_allowed( traits_iter.category ) ) {
                    valid.push_back( traits_iter.id );
                }
            }
        } else {
            // Pull the category's list
            valid = mutations_category[cat];
        }

        // Remove anything we already have, that we have a child of, or that
        // goes against our intention of a good/bad mutation
        for( size_t i = 0; i < valid.size(); i++ ) {
            if( ( !mutation_ok( valid[i], force_good, force_bad ) ) ||
                ( !valid[i]->valid ) ) {
                valid.erase( valid.begin() + i );
                i--;
            }
        }

        if( valid.empty() ) {
            // So we won't repeat endlessly
            first_pass = false;
        }
    } while( valid.empty() && !cat.empty() );

    if( valid.empty() ) {
        // Couldn't find anything at all!
        return;
    }

    if( mutate_towards( random_entry( valid ) ) ) {
        return;
    } else {
        // if mutation failed (errors, post-threshold pick), try again once.
        mutate_towards( random_entry( valid ) );
    }
}

void Character::mutate_category( const std::string &cat )
{
    // Hacky ID comparison is better than separate hardcoded branch used before
    // TODO: Turn it into the null id
    if( cat == "ANY" ) {
        mutate();
        return;
    }

    bool force_bad = one_in( 3 );
    bool force_good = false;
    if( has_trait( trait_ROBUST ) && force_bad ) {
        // Robust Genetics gives you a 33% chance for a good mutation,
        // instead of the 33% chance of a bad one.
        force_bad = false;
        force_good = true;
    }

    // Pull the category's list for valid mutations
    std::vector<trait_id> valid = mutations_category[cat];

    // Remove anything we already have, that we have a child of, or that
    // goes against our intention of a good/bad mutation
    for( size_t i = 0; i < valid.size(); i++ ) {
        if( !mutation_ok( valid[i], force_good, force_bad ) ) {
            valid.erase( valid.begin() + i );
            i--;
        }
    }

    mutate_towards( valid, 2 );
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

bool Character::mutate_towards( std::vector<trait_id> muts, int num_tries )
{
    while( !muts.empty() && num_tries > 0 ) {
        int i = rng( 0, muts.size() - 1 );

        if( mutate_towards( muts[i] ) ) {
            return true;
        }

        muts.erase( muts.begin() + i );
        --num_tries;
    }

    return false;
}

bool Character::mutate_towards( const trait_id &mut )
{
    if( has_child_flag( mut ) ) {
        remove_child_flag( mut );
        return true;
    }
    const mutation_branch &mdata = mut.obj();

    bool has_prereqs = false;
    bool prereq1 = false;
    bool prereq2 = false;
    std::vector<trait_id> canceltrait;
    std::vector<trait_id> prereq = mdata.prereqs;
    std::vector<trait_id> prereqs2 = mdata.prereqs2;
    std::vector<trait_id> cancel = mdata.cancels;
    std::vector<trait_id> same_type = get_mutations_in_types( mdata.types );
    std::vector<trait_id> all_prereqs = get_all_mutation_prereqs( mut );

    // Check mutations of the same type - except for the ones we might need for pre-reqs
    for( const auto &consider : same_type ) {
        if( std::find( all_prereqs.begin(), all_prereqs.end(), consider ) == all_prereqs.end() ) {
            cancel.push_back( consider );
        }
    }

    for( size_t i = 0; i < cancel.size(); i++ ) {
        if( !has_trait( cancel[i] ) ) {
            cancel.erase( cancel.begin() + i );
            i--;
        } else if( has_base_trait( cancel[i] ) ) {
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
            // This checks for cases where one trait knocks out several others
            // Probably a better way, but gets it Fixed Now--KA101
            return mutate_towards( mut );
        }
    }

    for( size_t i = 0; ( !prereq1 ) && i < prereq.size(); i++ ) {
        if( has_trait( prereq[i] ) ) {
            prereq1 = true;
        }
    }

    for( size_t i = 0; ( !prereq2 ) && i < prereqs2.size(); i++ ) {
        if( has_trait( prereqs2[i] ) ) {
            prereq2 = true;
        }
    }

    if( prereq1 && prereq2 ) {
        has_prereqs = true;
    }

    if( !has_prereqs && ( !prereq.empty() || !prereqs2.empty() ) ) {
        if( !prereq1 && !prereq.empty() ) {
            return mutate_towards( prereq );
        } else if( !prereq2 && !prereqs2.empty() ) {
            return mutate_towards( prereqs2 );
        }
    }

    // Check for threshold mutation, if needed
    bool threshold = mdata.threshold;
    bool profession = mdata.profession;
    bool has_threshreq = false;
    std::vector<trait_id> threshreq = mdata.threshreq;

    // It shouldn't pick a Threshold anyway--they're supposed to be non-Valid
    // and aren't categorized. This can happen if someone makes a threshold mutation into a prerequisite.
    if( threshold ) {
        add_msg_if_player( _( "You feel something straining deep inside you, yearning to be free…" ) );
        return false;
    }
    if( profession ) {
        // Profession picks fail silently
        return false;
    }

    for( size_t i = 0; !has_threshreq && i < threshreq.size(); i++ ) {
        if( has_trait( threshreq[i] ) ) {
            has_threshreq = true;
        }
    }

    // No crossing The Threshold by simply not having it
    if( !has_threshreq && !threshreq.empty() ) {
        add_msg_if_player( _( "You feel something straining deep inside you, yearning to be free…" ) );
        return false;
    }

    // Check if one of the prerequisites that we have TURNS INTO this one
    trait_id replacing = trait_id::NULL_ID();
    prereq = mdata.prereqs; // Reset it
    for( auto &elem : prereq ) {
        if( has_trait( elem ) ) {
            const trait_id &pre = elem;
            const auto &p = pre.obj();
            for( size_t j = 0; !replacing && j < p.replacements.size(); j++ ) {
                if( p.replacements[j] == mut ) {
                    replacing = pre;
                }
            }
        }
    }

    // Loop through again for prereqs2
    trait_id replacing2 = trait_id::NULL_ID();
    prereq = mdata.prereqs2; // Reset it
    for( auto &elem : prereq ) {
        if( has_trait( elem ) ) {
            const trait_id &pre2 = elem;
            const auto &p = pre2.obj();
            for( size_t j = 0; !replacing2 && j < p.replacements.size(); j++ ) {
                if( p.replacements[j] == mut ) {
                    replacing2 = pre2;
                }
            }
        }
    }

    set_mutation( mut );

    bool mutation_replaced = false;

    game_message_type rating;

    if( replacing ) {
        const auto &replace_mdata = replacing.obj();
        if( mdata.mixed_effect || replace_mdata.mixed_effect ) {
            rating = m_mixed;
        } else if( replace_mdata.points - mdata.points < 0 ) {
            rating = m_good;
        } else if( mdata.points - replace_mdata.points < 0 ) {
            rating = m_bad;
        } else {
            rating = m_neutral;
        }
        //  TODO: Limit this to visible mutations
        // TODO: In case invisible mutation turns into visible or vice versa
        //  print only the visible mutation appearing/disappearing
        add_msg_player_or_npc( rating,
                               _( "Your %1$s mutation turns into %2$s!" ),
                               _( "<npcname>'s %1$s mutation turns into %2$s!" ),
                               replace_mdata.name(), mdata.name() );

        g->events().send<event_type::evolves_mutation>( getID(), replace_mdata.id, mdata.id );
        unset_mutation( replacing );
        mutation_loss_effect( replacing );
        mutation_effect( mut );
        mutation_replaced = true;
    }
    if( replacing2 ) {
        const auto &replace_mdata = replacing2.obj();
        if( mdata.mixed_effect || replace_mdata.mixed_effect ) {
            rating = m_mixed;
        } else if( replace_mdata.points - mdata.points < 0 ) {
            rating = m_good;
        } else if( mdata.points - replace_mdata.points < 0 ) {
            rating = m_bad;
        } else {
            rating = m_neutral;
        }
        add_msg_player_or_npc( rating,
                               _( "Your %1$s mutation turns into %2$s!" ),
                               _( "<npcname>'s %1$s mutation turns into %2$s!" ),
                               replace_mdata.name(), mdata.name() );
        g->events().send<event_type::evolves_mutation>( getID(), replace_mdata.id, mdata.id );
        unset_mutation( replacing2 );
        mutation_loss_effect( replacing2 );
        mutation_effect( mut );
        mutation_replaced = true;
    }
    for( const auto &i : canceltrait ) {
        const auto &cancel_mdata = i.obj();
        if( mdata.mixed_effect || cancel_mdata.mixed_effect ) {
            rating = m_mixed;
        } else if( mdata.points < cancel_mdata.points ) {
            rating = m_bad;
        } else if( mdata.points > cancel_mdata.points ) {
            rating = m_good;
        } else if( mdata.points == cancel_mdata.points ) {
            rating = m_neutral;
        } else {
            rating = m_mixed;
        }
        // If this new mutation cancels a base trait, remove it and add the mutation at the same time
        add_msg_player_or_npc( rating,
                               _( "Your innate %1$s trait turns into %2$s!" ),
                               _( "<npcname>'s innate %1$s trait turns into %2$s!" ),
                               cancel_mdata.name(), mdata.name() );
        g->events().send<event_type::evolves_mutation>( getID(), cancel_mdata.id, mdata.id );
        unset_mutation( i );
        mutation_loss_effect( i );
        mutation_effect( mut );
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
        // TODO: Limit to visible mutations
        add_msg_player_or_npc( rating,
                               _( "You gain a mutation called %s!" ),
                               _( "<npcname> gains a mutation called %s!" ),
                               mdata.name() );
        g->events().send<event_type::gains_mutation>( getID(), mdata.id );
        mutation_effect( mut );
    }

    set_highest_cat_level();
    drench_mut_calc();
    return true;
}

void Character::remove_mutation( const trait_id &mut, bool silent )
{
    const auto &mdata = mut.obj();
    // Check if there's a prerequisite we should shrink back into
    trait_id replacing = trait_id::NULL_ID();
    std::vector<trait_id> originals = mdata.prereqs;
    for( size_t i = 0; !replacing && i < originals.size(); i++ ) {
        trait_id pre = originals[i];
        const auto &p = pre.obj();
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
        const auto &p = pre2.obj();
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
        for( auto &iter : mutation_branch::get_all() ) {
            //See if it's in our list of base traits but not active
            if( has_base_trait( iter.id ) && !has_trait( iter.id ) ) {
                //See if that base trait cancels the mutation we are using
                std::vector<trait_id> traitcheck = iter.cancels;
                if( !traitcheck.empty() ) {
                    for( size_t j = 0; !replacing && j < traitcheck.size(); j++ ) {
                        if( traitcheck[j] == mut ) {
                            replacing = ( iter.id );
                        }
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
        for( auto &iter : mutation_branch::get_all() ) {
            //See if it's in our list of base traits but not active
            if( has_base_trait( iter.id ) && !has_trait( iter.id ) ) {
                //See if that base trait cancels the mutation we are using
                std::vector<trait_id> traitcheck = iter.cancels;
                if( !traitcheck.empty() ) {
                    for( size_t j = 0; !replacing2 && j < traitcheck.size(); j++ ) {
                        if( traitcheck[j] == mut && ( iter.id ) != replacing ) {
                            replacing2 = ( iter.id );
                        }
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

    if( replacing ) {
        const auto &replace_mdata = replacing.obj();
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
            add_msg_player_or_npc( rating,
                                   _( "Your %1$s mutation turns into %2$s." ),
                                   _( "<npcname>'s %1$s mutation turns into %2$s." ),
                                   mdata.name(), replace_mdata.name() );
        }
        set_mutation( replacing );
        mutation_loss_effect( mut );
        mutation_effect( replacing );
        mutation_replaced = true;
    }
    if( replacing2 ) {
        const auto &replace_mdata = replacing2.obj();
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
            add_msg_player_or_npc( rating,
                                   _( "Your %1$s mutation turns into %2$s." ),
                                   _( "<npcname>'s %1$s mutation turns into %2$s." ),
                                   mdata.name(), replace_mdata.name() );
        }
        set_mutation( replacing2 );
        mutation_loss_effect( mut );
        mutation_effect( replacing2 );
        mutation_replaced = true;
    }
    if( !mutation_replaced ) {
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
                                   mdata.name() );
        }
        mutation_loss_effect( mut );
    }

    set_highest_cat_level();
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
    for( auto &elem : flag->replacements ) {
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

static mutagen_rejection try_reject_mutagen( player &p, const item &it, bool strong )
{
    if( p.has_trait( trait_MUTAGEN_AVOID ) ) {
        p.add_msg_if_player( m_warning,
                             //~ "Uh-uh" is a sound used for "nope", "no", etc.
                             _( "After what happened that last time?  uh-uh.  You're not drinking that chemical stuff." ) );
        return mutagen_rejection::rejected;
    }

    static const std::vector<std::string> safe = {{
            "MYCUS", "MARLOSS", "MARLOSS_SEED", "MARLOSS_GEL"
        }
    };
    if( std::any_of( safe.begin(), safe.end(), [it]( const std::string & flag ) {
    return it.type->can_use( flag );
    } ) ) {
        return mutagen_rejection::accepted;
    }

    if( p.has_trait( trait_THRESH_MYCUS ) ) {
        p.add_msg_if_player( m_info, _( "This is a contaminant.  We reject it from the Mycus." ) );
        if( p.has_trait( trait_M_SPORES ) || p.has_trait( trait_M_FERTILE ) ||
            p.has_trait( trait_M_BLOSSOMS ) || p.has_trait( trait_M_BLOOM ) ) {
            p.add_msg_if_player( m_good, _( "We decontaminate it with spores." ) );
            g->m.ter_set( p.pos(), t_fungus );
            if( p.is_avatar() ) {
                g->memorial().add(
                    pgettext( "memorial_male", "Destroyed a harmful invader." ),
                    pgettext( "memorial_female", "Destroyed a harmful invader." ) );
            }
            return mutagen_rejection::destroyed;
        } else {
            p.add_msg_if_player( m_bad,
                                 _( "We must eliminate this contaminant at the earliest opportunity." ) );
            return mutagen_rejection::rejected;
        }
    }

    if( p.has_trait( trait_THRESH_MARLOSS ) ) {
        p.add_msg_player_or_npc( m_warning,
                                 _( "The %s sears your insides white-hot, and you collapse to the ground!" ),
                                 _( "<npcname> writhes in agony and collapses to the ground!" ),
                                 it.tname() );
        p.vomit();
        p.mod_pain( 35 + ( strong ? 20 : 0 ) );
        // Lose a significant amount of HP, probably about 25-33%
        p.hurtall( rng( 20, 35 ) + ( strong ? 10 : 0 ), nullptr );
        // Hope you were eating someplace safe.  Mycus v. Goo in your guts is no joke.
        p.fall_asleep( 5_hours - 1_minutes * ( p.int_cur + ( strong ? 100 : 0 ) ) );
        p.set_mutation( trait_MUTAGEN_AVOID );
        // Injected mutagen purges marloss, ingested doesn't
        if( strong ) {
            p.unset_mutation( trait_THRESH_MARLOSS );
            p.add_msg_if_player( m_warning,
                                 _( "It was probably that marloss -- how did you know to call it \"marloss\" anyway?" ) );
            p.add_msg_if_player( m_warning, _( "Best to stay clear of that alien crap in future." ) );
            if( p.is_avatar() ) {
                g->memorial().add(
                    pgettext( "memorial_male",
                              "Burned out a particularly nasty fungal infestation." ),
                    pgettext( "memorial_female",
                              "Burned out a particularly nasty fungal infestation." ) );
            }
        } else {
            p.add_msg_if_player( m_warning,
                                 _( "That was some toxic %s!  Let's stick with Marloss next time, that's safe." ),
                                 it.tname() );
            if( p.is_avatar() ) {
                g->memorial().add(
                    pgettext( "memorial_male", "Suffered a toxic marloss/mutagen reaction." ),
                    pgettext( "memorial_female", "Suffered a toxic marloss/mutagen reaction." ) );
            }
        }

        return mutagen_rejection::destroyed;
    }

    return mutagen_rejection::accepted;
}

mutagen_attempt mutagen_common_checks( player &p, const item &it, bool strong,
                                       const mutagen_technique technique )
{
    g->events().send<event_type::administers_mutagen>( p.getID(), technique );
    mutagen_rejection status = try_reject_mutagen( p, it, strong );
    if( status == mutagen_rejection::rejected ) {
        return mutagen_attempt( false, 0 );
    }
    if( status == mutagen_rejection::destroyed ) {
        return mutagen_attempt( false, it.type->charges_to_use() );
    }

    return mutagen_attempt( true, 0 );
}

void test_crossing_threshold( Character &guy, const mutation_category_trait &m_category )
{
    // Threshold-check.  You only get to cross once!
    if( guy.crossed_threshold() ) {
        return;
    }

    // If there is no threshold for this category, don't check it
    const trait_id &mutation_thresh = m_category.threshold_mut;
    if( mutation_thresh.is_empty() ) {
        return;
    }

    std::string mutation_category = m_category.id;
    int total = 0;
    for( const auto &iter : mutation_category_trait::get_all() ) {
        total += guy.mutation_category_level[ iter.first ];
    }
    // Threshold-breaching
    const std::string &primary = guy.get_highest_category();
    int breach_power = guy.mutation_category_level[primary];
    // Only if you were pushing for more in your primary category.
    // You wanted to be more like it and less human.
    // That said, you're required to have hit third-stage dreams first.
    if( ( mutation_category == primary ) && ( breach_power > 50 ) ) {
        // Little help for the categories that have a lot of crossover.
        // Starting with Ursine as that's... a bear to get.  8-)
        // Alpha is similarly eclipsed by other mutation categories.
        // Will add others if there's serious/demonstrable need.
        int booster = 0;
        if( mutation_category == "URSINE"  || mutation_category == "ALPHA" ) {
            booster = 50;
        }
        int breacher = breach_power + booster;
        if( x_in_y( breacher, total ) ) {
            guy.add_msg_if_player( m_good,
                                   _( "Something strains mightily for a moment… and then… you're… FREE!" ) );
            guy.set_mutation( mutation_thresh );
            g->events().send<event_type::crosses_mutation_threshold>( guy.getID(), m_category.id );
            // Manually removing Carnivore, since it tends to creep in
            // This is because carnivore is a prerequisite for the
            // predator-style post-threshold mutations.
            if( mutation_category == "URSINE" && guy.has_trait( trait_CARNIVORE ) ) {
                guy.unset_mutation( trait_CARNIVORE );
                guy.add_msg_if_player( _( "Your appetite for blood fades." ) );
            }
        }
    } else if( guy.has_trait( trait_NOPAIN ) ) {
        //~NOPAIN is a post-Threshold trait, so you shouldn't
        //~legitimately have it and get here!
        guy.add_msg_if_player( m_bad, _( "You feel extremely Bugged." ) );
    } else if( breach_power > 100 ) {
        guy.add_msg_if_player( m_bad, _( "You stagger with a piercing headache!" ) );
        guy.mod_pain_noresist( 8 );
        guy.add_effect( effect_stunned, rng( 3_turns, 5_turns ) );
    } else if( breach_power > 80 ) {
        guy.add_msg_if_player( m_bad,
                               _( "Your head throbs with memories of your life, before all this…" ) );
        guy.mod_pain_noresist( 6 );
        guy.add_effect( effect_stunned, rng( 2_turns, 4_turns ) );
    } else if( breach_power > 60 ) {
        guy.add_msg_if_player( m_bad, _( "Images of your past life flash before you." ) );
        guy.add_effect( effect_stunned, rng( 2_turns, 3_turns ) );
    }
}
