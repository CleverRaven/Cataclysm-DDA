#include "mondeath.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iosfwd>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "calendar.h"
#include "character.h"
#include "colony.h"
#include "creature.h"
#include "debug.h"
#include "enums.h"
#include "explosion.h"
#include "field_type.h"
#include "fungal_effects.h"
#include "game.h"
#include "harvest.h"
#include "item_stack.h"
#include "itype.h"
#include "kill_tracker.h"
#include "line.h"
#include "make_static.h"
#include "map.h"
#include "map_iterator.h"
#include "mattack_actors.h"
#include "mattack_common.h"
#include "messages.h"
#include "monster.h"
#include "morale_types.h"
#include "mtype.h"
#include "point.h"
#include "rng.h"
#include "sounds.h"
#include "string_formatter.h"
#include "timed_event.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"
#include "viewer.h"

static const efftype_id effect_critter_underfed( "critter_underfed" );
static const efftype_id effect_no_ammo( "no_ammo" );

static const harvest_drop_type_id harvest_drop_bone( "bone" );
static const harvest_drop_type_id harvest_drop_flesh( "flesh" );

static const species_id species_ZOMBIE( "ZOMBIE" );

item_location mdeath::normal( monster &z )
{
    if( z.no_corpse_quiet ) {
        return {};
    }

    if( !z.quiet_death && !z.has_flag( mon_flag_QUIETDEATH ) ) {
        if( z.type->in_species( species_ZOMBIE ) ) {
            sfx::play_variant_sound( "mon_death", "zombie_death", sfx::get_heard_volume( z.pos() ) );
        }

        //Currently it is possible to get multiple messages that a monster died.
        add_msg_if_player_sees( z, m_good, _( "The %s dies!" ), z.name() );
    }

    if( z.death_drops ) {
        const int max_hp = std::max( z.get_hp_max(), 1 );
        const float overflow_damage = std::max( -z.get_hp(), 0 );
        const float corpse_damage = 2.5 * overflow_damage / max_hp;
        const bool pulverized = corpse_damage > 5 && overflow_damage > z.get_hp_max();

        z.bleed(); // leave some blood if we have to

        if( pulverized ) {
            return splatter( z );
        } else {
            const float damage = std::floor( corpse_damage * itype::damage_scale );
            return make_mon_corpse( z, static_cast<int>( damage ) );
        }
    }
    return {};
}

static void scatter_chunks( const itype_id &chunk_name, int chunk_amt, monster &z, int distance,
                            int pile_size = 1 )
{
    // can't have less than one item in a pile or it would cause an infinite loop
    pile_size = std::max( pile_size, 1 );
    // can't have more items in a pile than total items
    pile_size = std::min( chunk_amt, pile_size );
    distance = std::abs( distance );
    const item chunk( chunk_name, calendar::turn, pile_size );
    map &here = get_map();
    for( int i = 0; i < chunk_amt; i += pile_size ) {
        bool drop_chunks = true;
        tripoint tarp( z.pos() + point( rng( -distance, distance ), rng( -distance, distance ) ) );
        const auto traj = line_to( z.pos(), tarp );

        for( size_t j = 0; j < traj.size(); j++ ) {
            tarp = traj[j];
            if( one_in( 2 ) && z.bloodType().id() ) {
                here.add_splatter( z.bloodType(), tarp );
            } else {
                here.add_splatter( z.gibType(), tarp, rng( 1, j + 1 ) );
            }
            if( here.impassable( tarp ) ) {
                here.bash( tarp, distance );
                if( here.impassable( tarp ) ) {
                    // Target is obstacle, not destroyed by bashing,
                    // stop trajectory in front of it, if this is the first
                    // point (e.g. wall adjacent to monster), don't drop anything on it
                    if( j > 0 ) {
                        tarp = traj[j - 1];
                    } else {
                        drop_chunks = false;
                    }
                    break;
                }
            }
        }
        if( drop_chunks ) {
            here.add_item_or_charges( tarp, chunk );
        }
    }
}

item_location mdeath::splatter( monster &z )
{
    const bool gibbable = !z.type->has_flag( mon_flag_NOGIB );

    const int max_hp = std::max( z.get_hp_max(), 1 );
    const float overflow_damage = std::max( -z.get_hp(), 0 );
    const float corpse_damage = 2.5 * overflow_damage / max_hp;

    const field_type_id type_blood = z.bloodType();
    const field_type_id type_gib = z.gibType();

    map &here = get_map();
    if( gibbable ) {
        const auto area = here.points_in_radius( z.pos(), 1 );
        int number_of_gibs = std::min( std::floor( corpse_damage ) - 1, 1 + max_hp / 5.0f );

        if( z.type->size >= creature_size::medium ) {
            number_of_gibs += rng( 1, 6 );
            sfx::play_variant_sound( "mon_death", "zombie_gibbed", sfx::get_heard_volume( z.pos() ) );
        }

        for( int i = 0; i < number_of_gibs; ++i ) {
            here.add_splatter( type_gib, random_entry( area ), rng( 1, i + 1 ) );
            here.add_splatter( type_blood, random_entry( area ) );
        }
    }
    // 1% of the weight of the monster is the base, with overflow damage as a multiplier
    int gibbed_weight = rng( 0, std::round( to_gram( z.get_weight() ) / 100.0 *
                                            ( overflow_damage / max_hp + 1 ) ) );
    const int z_weight = to_gram( z.get_weight() );
    // limit gibbing to 15%
    gibbed_weight = std::min( gibbed_weight, z_weight * 15 / 100 );

    if( gibbable ) {
        float overflow_ratio = overflow_damage / max_hp + 1.0f;
        int gib_distance = std::round( rng( 2, 4 ) );
        for( const harvest_entry &entry : *z.type->harvest ) {
            // only flesh and bones survive.
            if( entry.type == harvest_drop_flesh || entry.type == harvest_drop_bone ) {
                // the larger the overflow damage, the less you get
                const int chunk_amt =
                    entry.mass_ratio / overflow_ratio / 10 *
                    z.get_weight() / item::find_type( itype_id( entry.drop ) )->weight;
                scatter_chunks( itype_id( entry.drop ), chunk_amt, z, gib_distance,
                                chunk_amt / ( gib_distance - 1 ) );
                gibbed_weight -= entry.mass_ratio / overflow_ratio / 20 * to_gram( z.get_weight() );
            }
        }
        if( gibbed_weight > 0 ) {
            const itype_id &leftover_id = z.type->id->harvest->leftovers;
            const int chunk_amount =
                gibbed_weight / to_gram( item::find_type( leftover_id )->weight );
            scatter_chunks( leftover_id, chunk_amount, z, gib_distance,
                            chunk_amount / ( gib_distance + 1 ) );
        }
        // add corpse with gib flag
        item corpse = item::make_corpse( z.type->id, calendar::turn, z.unique_name, z.get_upgrade_time() );
        if( corpse.is_null() ) {
            return {};
        }
        // Set corpse to damage that aligns with being pulped
        corpse.set_damage( 4000 );
        corpse.set_flag( STATIC( flag_id( "GIBBED" ) ) );
        if( z.has_effect( effect_no_ammo ) ) {
            corpse.set_var( "no_ammo", "no_ammo" );
        }
        if( z.has_effect( effect_critter_underfed ) ) {
            corpse.set_flag( STATIC( flag_id( "UNDERFED" ) ) );
        }
        return here.add_item_ret_loc( z.pos(), corpse );
    }
    return {};
}

void mdeath::disappear( monster &z )
{
    if( !z.type->has_flag( mon_flag_SILENT_DISAPPEAR ) ) {
        add_msg_if_player_sees( z.pos(), m_good, _( "The %s disappears." ), z.name() );
    }
}

void mdeath::broken( monster &z )
{
    // Bail out if flagged (simulates eyebot flying away)
    if( z.no_corpse_quiet ) {
        return;
    }
    std::string item_id = z.type->id.str();
    if( item_id.compare( 0, 4, "mon_" ) == 0 ) {
        item_id.erase( 0, 4 );
    }
    // make "broken_manhack", or "broken_eyebot", ...
    item_id.insert( 0, "broken_" );
    item broken_mon( item_id, calendar::turn );
    const int max_hp = std::max( z.get_hp_max(), 1 );
    const float overflow_damage = std::max( -z.get_hp(), 0 );
    const float corpse_damage = 2.5 * overflow_damage / max_hp;
    broken_mon.set_damage( static_cast<int>( std::floor( corpse_damage * itype::damage_scale ) ) );

    map &here = get_map();
    here.add_item_or_charges( z.pos(), broken_mon );

    if( z.type->has_flag( mon_flag_DROPS_AMMO ) ) {
        for( const std::pair<const itype_id, int> &ammo_entry : z.ammo ) {
            if( ammo_entry.second > 0 ) {
                bool spawned = false;
                for( const std::pair<const std::string, mtype_special_attack> &attack : z.type->special_attacks ) {
                    if( attack.second->id == "gun" ) {
                        item gun = item( dynamic_cast<const gun_actor *>( attack.second.get() )->gun_type );
                        bool same_ammo = false;
                        if( gun.typeId()->magazine_default.count( item( ammo_entry.first ).ammo_type() ) ) {
                            same_ammo = true;
                        }
                        if( same_ammo && gun.uses_magazine() ) {
                            std::vector<item> mags;
                            int ammo_count = ammo_entry.second;
                            while( ammo_count > 0 ) {
                                item mag = item( gun.type->magazine_default.find( item( ammo_entry.first ).ammo_type() )->second );
                                mag.ammo_set( ammo_entry.first,
                                              std::min( ammo_count, mag.type->magazine->capacity ) );
                                mags.insert( mags.end(), mag );
                                ammo_count -= mag.type->magazine->capacity;
                            }
                            here.spawn_items( z.pos(), mags );
                            spawned = true;
                            break;
                        }
                    }
                }
                if( !spawned ) {
                    here.spawn_item( z.pos(), ammo_entry.first, ammo_entry.second, 1,
                                     calendar::turn );
                }
            }
        }
    }

    // TODO: make mdeath::splatter work for robots
    if( broken_mon.damage() >= broken_mon.max_damage() ) {
        add_msg_if_player_sees( z.pos(), m_good, _( "The %s is destroyed!" ), z.name() );
    } else {
        add_msg_if_player_sees( z.pos(), m_good, _( "The %s collapses!" ), z.name() );
    }
}

item_location make_mon_corpse( monster &z, int damageLvl )
{
    item corpse = item::make_corpse( z.type->id, calendar::turn, z.unique_name, z.get_upgrade_time() );
    // All corpses are at 37 C at time of death
    // This may not be true but anything better would be way too complicated
    if( z.is_warm() ) {
        corpse.set_item_temperature( units::from_celsius( 37 ) );
    }
    corpse.set_damage( damageLvl );
    if( z.has_effect( effect_no_ammo ) ) {
        corpse.set_var( "no_ammo", "no_ammo" );
    }
    if( z.has_effect( effect_critter_underfed ) ) {
        corpse.set_flag( STATIC( flag_id( "UNDERFED" ) ) );
    }
    return get_map().add_item_ret_loc( z.pos(), corpse );
}
