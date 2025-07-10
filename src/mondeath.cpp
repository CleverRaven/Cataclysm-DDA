#include "mondeath.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "calendar.h"
#include "coordinates.h"
#include "creature.h"
#include "enums.h"
#include "harvest.h"
#include "item.h"
#include "itype.h"
#include "make_static.h"
#include "map.h"
#include "map_iterator.h"
#include "mattack_actors.h"
#include "mattack_common.h"
#include "messages.h"
#include "monster.h"
#include "mtype.h"
#include "point.h"
#include "rng.h"
#include "sounds.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"

static const efftype_id effect_no_ammo( "no_ammo" );

static const harvest_drop_type_id harvest_drop_bone( "bone" );
static const harvest_drop_type_id harvest_drop_flesh( "flesh" );

static const species_id species_ZOMBIE( "ZOMBIE" );

item_location mdeath::normal( map *here, monster &z )
{
    if( z.no_corpse_quiet ) {
        return {};
    }

    if( !z.quiet_death && !z.has_flag( mon_flag_QUIETDEATH ) ) {
        if( z.type->in_species( species_ZOMBIE ) && get_map().inbounds( z.pos_abs() ) ) {
            sfx::play_variant_sound( "mon_death", "zombie_death", sfx::get_heard_volume( z.pos_bub() ) );
        }

        //Currently it is possible to get multiple messages that a monster died.
        add_msg_if_player_sees( z, m_good, _( "The %s dies!" ), z.name() );
    }

    if( z.death_drops ) {
        const int max_hp = std::max( z.get_hp_max(), 1 );
        const float overflow_damage = std::max( -z.get_hp(), 0 );
        const float corpse_damage = 2.5 * overflow_damage / max_hp;
        const bool pulverized = corpse_damage > 5 && overflow_damage > z.get_hp_max();

        z.bleed( *here ); // leave some blood if we have to

        if( pulverized ) {
            return splatter( here, z );
        } else {
            const float damage = std::floor( corpse_damage * itype::damage_scale );
            return make_mon_corpse( here,  z, static_cast<int>( damage ) );
        }
    }
    return {};
}

static void scatter_chunks( map *here, const itype_id &chunk_name, int chunk_amt, monster &z,
                            int distance,
                            int pile_size = 1 )
{
    // can't have less than one item in a pile or it would cause an infinite loop
    pile_size = std::max( pile_size, 1 );
    // can't have more items in a pile than total items
    pile_size = std::min( chunk_amt, pile_size );
    distance = std::abs( distance );
    const item chunk( chunk_name, calendar::turn );
    int placed_chunks = 0;
    while( placed_chunks < chunk_amt ) {
        bool drop_chunks = true;
        tripoint_bub_ms tarp( z.pos_bub( *here ) + point( rng( -distance, distance ),
                              rng( -distance,
                                   distance ) ) );
        const std::vector<tripoint_bub_ms> traj = line_to( z.pos_bub( *here ), tarp );

        for( size_t j = 0; j < traj.size(); j++ ) {
            tarp = traj[j];
            if( one_in( 2 ) && z.bloodType().id() ) {
                here->add_splatter( z.bloodType(), tarp );
            } else {
                here->add_splatter( z.gibType(), tarp, rng( 1, j + 1 ) );
            }
            if( here->impassable( tarp ) ) {
                here->bash( tarp, distance );
                if( here->impassable( tarp ) ) {
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
            for( int i = placed_chunks; i < chunk_amt && i < placed_chunks + pile_size; i++ ) {
                here->add_item_or_charges( tarp, chunk );
            }
        }
        placed_chunks += pile_size;
    }
}

item_location mdeath::splatter( map *here, monster &z )
{
    const bool gibbable = !z.type->has_flag( mon_flag_NOGIB );

    const int max_hp = std::max( z.get_hp_max(), 1 );
    const float overflow_damage = std::max( -z.get_hp(), 0 );
    const float corpse_damage = 2.5 * overflow_damage / max_hp;

    const field_type_id type_blood = z.bloodType();
    const field_type_id type_gib = z.gibType();

    if( gibbable ) {
        const tripoint_range<tripoint_bub_ms> area = here->points_in_radius( z.pos_bub( *here ),
                1 );
        int number_of_gibs = std::min( std::floor( corpse_damage ) - 1, 1 + max_hp / 5.0f );

        if( z.type->size >= creature_size::medium ) {
            number_of_gibs += rng( 1, 6 );
            if( reality_bubble().inbounds( z.pos_abs() ) ) {
                sfx::play_variant_sound( "mon_death", "zombie_gibbed", sfx::get_heard_volume( z.pos_bub() ) );
            }
        }

        for( int i = 0; i < number_of_gibs; ++i ) {
            here->add_splatter( type_gib, random_entry( area ), rng( 1, i + 1 ) );
            here->add_splatter( type_blood, random_entry( area ) );
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
                scatter_chunks( here, itype_id( entry.drop ), chunk_amt, z, gib_distance,
                                chunk_amt / ( gib_distance - 1 ) );
                gibbed_weight -= entry.mass_ratio / overflow_ratio / 20 * to_gram( z.get_weight() );
            }
        }
        if( gibbed_weight > 0 ) {
            const itype_id &leftover_id = z.type->id->harvest->leftovers;
            const int chunk_amount =
                gibbed_weight / to_gram( item::find_type( leftover_id )->weight );
            scatter_chunks( here, leftover_id, chunk_amount, z, gib_distance,
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
        if( !z.has_eaten_enough() ) {
            corpse.set_flag( STATIC( flag_id( "UNDERFED" ) ) );
        }
        return here->add_item_or_charges_ret_loc( z.pos_bub( *here ), corpse );
    }
    return {};
}

void mdeath::disappear( monster &z )
{
    if( !z.type->has_flag( mon_flag_SILENT_DISAPPEAR ) ) {
        add_msg_if_player_sees( z.pos_bub(), m_good, _( "The %s disappears." ), z.name() );
    }
}

void mdeath::broken( map *here, monster &z )
{
    // Bail out if flagged (simulates eyebot flying away)
    if( z.no_corpse_quiet ) {
        return;
    }
    std::string item_id;

    if( !z.type->broken_itype.is_empty() ) {
        item_id = z.type->broken_itype.str();
    } else {
        item_id = z.type->id.str();
        //FIXME Remove hardcoded id manipulation
        if( item_id.compare( 0, 4, "mon_" ) == 0 ) {
            item_id.erase( 0, 4 );
        }
        // make "broken_manhack", or "broken_eyebot", ...
        item_id.insert( 0, "broken_" );
    }
    item broken_mon( itype_id( item_id ), calendar::turn );
    const int max_hp = std::max( z.get_hp_max(), 1 );
    const float overflow_damage = std::max( -z.get_hp(), 0 );
    const float corpse_damage = 2.5 * overflow_damage / max_hp;
    broken_mon.set_damage( static_cast<int>( std::floor( corpse_damage * itype::damage_scale ) ) );

    here->add_item_or_charges( z.pos_bub( *here ), broken_mon );

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
                            here->spawn_items( z.pos_bub( *here ), mags );
                            spawned = true;
                            break;
                        }
                    }
                }
                if( !spawned ) {
                    here->spawn_item( z.pos_bub( *here ), ammo_entry.first, ammo_entry.second, 1,
                                      calendar::turn );
                }
            }
        }
    }

    // TODO: make mdeath::splatter work for robots
    if( broken_mon.damage() >= broken_mon.max_damage() ) {
        add_msg_if_player_sees( z.pos_bub(), m_good, _( "The %s is destroyed!" ), z.name() );
    } else {
        add_msg_if_player_sees( z.pos_bub(), m_good, _( "The %s collapses!" ), z.name() );
    }
}

item_location make_mon_corpse( map *here, monster &z, int damageLvl )
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
    if( !z.has_eaten_enough() ) {
        corpse.set_flag( STATIC( flag_id( "UNDERFED" ) ) );
    }
    return here->add_item_or_charges_ret_loc( z.pos_bub( *here ), corpse );
}
