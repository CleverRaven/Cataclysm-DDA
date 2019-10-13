#include <array>
#include <cmath>
#include <cstddef>
#include <algorithm>
#include <queue>
#include <tuple>
#include <iterator>
#include <list>
#include <memory>
#include <utility>
#include <vector>
#include <array>
#include <bitset>
#include <string>

#include "field.h"
#include "avatar.h"
#include "calendar.h"
#include "coordinate_conversions.h"
#include "debug.h"
#include "effect.h"
#include "emit.h"
#include "enums.h"
#include "fire.h"
#include "fungal_effects.h"
#include "game.h"
#include "itype.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "messages.h"
#include "monster.h"
#include "mtype.h"
#include "npc.h"
#include "overmapbuffer.h"
#include "rng.h"
#include "scent_map.h"
#include "submap.h"
#include "translations.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "weather.h"
#include "bodypart.h"
#include "creature.h"
#include "damage.h"
#include "int_id.h"
#include "item.h"
#include "line.h"
#include "optional.h"
#include "player.h"
#include "pldata.h"
#include "string_id.h"
#include "units.h"
#include "type_id.h"
#include "basecamp.h"
#include "colony.h"
#include "game_constants.h"
#include "point.h"
#include "scent_block.h"
#include "mongroup.h"
#include "teleport.h"

const species_id FUNGUS( "FUNGUS" );
const species_id INSECT( "INSECT" );
const species_id SPIDER( "SPIDER" );

const efftype_id effect_badpoison( "badpoison" );
const efftype_id effect_blind( "blind" );
const efftype_id effect_corroding( "corroding" );
const efftype_id effect_fungus( "fungus" );
const efftype_id effect_onfire( "onfire" );
const efftype_id effect_poison( "poison" );
const efftype_id effect_relax_gas( "relax_gas" );
const efftype_id effect_sap( "sap" );
const efftype_id effect_smoke( "smoke" );
const efftype_id effect_stung( "stung" );
const efftype_id effect_stunned( "stunned" );
const efftype_id effect_teargas( "teargas" );
const efftype_id effect_webbed( "webbed" );

static const trait_id trait_ELECTRORECEPTORS( "ELECTRORECEPTORS" );
static const trait_id trait_M_SKIN2( "M_SKIN2" );
static const trait_id trait_M_SKIN3( "M_SKIN3" );

void map::create_burnproducts( const tripoint &p, const item &fuel, const units::mass &burned_mass )
{
    std::vector<material_id> all_mats = fuel.made_of();
    if( all_mats.empty() ) {
        return;
    }
    // Items that are multiple materials are assumed to be equal parts each.
    const units::mass by_weight = burned_mass / all_mats.size();
    for( material_id &mat : all_mats ) {
        for( auto &bp : mat->burn_products() ) {
            itype_id id = bp.first;
            // Spawning the same item as the one that was just burned is pointless
            // and leads to infinite recursion.
            if( fuel.typeId() == id ) {
                continue;
            }
            const float eff = bp.second;
            const int n = floor( eff * ( by_weight / item::find_type( id )->weight ) );

            if( n <= 0 ) {
                continue;
            }
            spawn_item( p, id, n, 1, calendar::turn );
        }
    }
}

// Use a helper for a bit less boilerplate
int map::burn_body_part( player &u, field_entry &cur, body_part bp, const int scale )
{
    int total_damage = 0;
    const int intensity = cur.get_field_intensity();
    const int damage = rng( 1, scale + intensity );
    // A bit ugly, but better than being annoyed by acid when in hazmat
    if( u.get_armor_type( DT_ACID, bp ) < damage ) {
        const dealt_damage_instance ddi = u.deal_damage( nullptr, bp, damage_instance( DT_ACID, damage ) );
        total_damage += ddi.total_damage();
    }
    // Represents acid seeping in rather than being splashed on
    u.add_env_effect( effect_corroding, bp, 2 + intensity, time_duration::from_turns( rng( 2,
                      1 + intensity ) ), bp, false, 0 );
    return total_damage;
}

bool map::process_fields()
{
    bool dirty_transparency_cache = false;
    const int minz = zlevels ? -OVERMAP_DEPTH : abs_sub.z;
    const int maxz = zlevels ? OVERMAP_HEIGHT : abs_sub.z;
    for( int z = minz; z <= maxz; z++ ) {
        bool zlev_dirty = false;
        auto &field_cache = get_cache( z ).field_cache;
        for( int x = 0; x < my_MAPSIZE; x++ ) {
            for( int y = 0; y < my_MAPSIZE; y++ ) {
                if( field_cache[ x + y * MAPSIZE ] ) {
                    submap *const current_submap = get_submap_at_grid( { x, y, z } );
                    const bool cur_dirty = process_fields_in_submap( current_submap, tripoint( x, y, z ) );
                    zlev_dirty |= cur_dirty;
                }
            }
        }

        if( zlev_dirty ) {
            // For now, just always dirty the transparency cache
            // when a field might possibly be changed.
            // TODO: check if there are any fields(mostly fire)
            //       that frequently change, if so set the dirty
            //       flag, otherwise only set the dirty flag if
            //       something actually changed
            set_transparency_cache_dirty( z );
            dirty_transparency_cache = true;
        }
    }

    return dirty_transparency_cache;
}

bool ter_furn_has_flag( const ter_t &ter, const furn_t &furn, const ter_bitflags flag )
{
    return ter.has_flag( flag ) || furn.has_flag( flag );
}

static int ter_furn_movecost( const ter_t &ter, const furn_t &furn )
{
    if( ter.movecost == 0 ) {
        return 0;
    }

    if( furn.movecost < 0 ) {
        return 0;
    }

    return ter.movecost + furn.movecost;
}

// Wrapper to allow skipping bound checks except at the edges of the map
maptile map::maptile_has_bounds( const tripoint &p, const bool bounds_checked )
{
    if( bounds_checked ) {
        // We know that the point is in bounds
        return maptile_at_internal( p );
    }

    return maptile_at( p );
}

std::array<maptile, 8> map::get_neighbors( const tripoint &p )
{
    // Find out which edges are in the bubble
    // Where possible, do just one bounds check for all the neighbors
    const bool west = p.x > 0;
    const bool north = p.y > 0;
    const bool east = p.x < SEEX * my_MAPSIZE - 1;
    const bool south = p.y < SEEY * my_MAPSIZE - 1;
    return std::array< maptile, 8 > { {
            maptile_has_bounds( p + eight_horizontal_neighbors[0], west &&north ),
            maptile_has_bounds( p + eight_horizontal_neighbors[1], north ),
            maptile_has_bounds( p + eight_horizontal_neighbors[2], east &&north ),
            maptile_has_bounds( p + eight_horizontal_neighbors[3], west ),
            maptile_has_bounds( p + eight_horizontal_neighbors[4], east ),
            maptile_has_bounds( p + eight_horizontal_neighbors[5], west &&south ),
            maptile_has_bounds( p + eight_horizontal_neighbors[6], south ),
            maptile_has_bounds( p + eight_horizontal_neighbors[7], east &&south ),
        }
    };
}

bool map::gas_can_spread_to( field_entry &cur, const maptile &dst )
{
    const field_entry *tmpfld = dst.get_field().find_field( cur.get_field_type() );
    const ter_t &ter = dst.get_ter_t();
    const furn_t &frn = dst.get_furn_t();
    // Candidates are existing weaker fields or navigable/flagged tiles with no field.
    return ( ter_furn_movecost( ter, frn ) > 0 || ter_furn_has_flag( ter, frn, TFLAG_PERMEABLE ) ) &&
           ( tmpfld == nullptr || tmpfld->get_field_intensity() < cur.get_field_intensity() );
}

void map::gas_spread_to( field_entry &cur, maptile &dst )
{
    const field_type_id current_type = cur.get_field_type();
    const time_duration current_age = cur.get_field_age();
    const int current_intensity = cur.get_field_intensity();
    field_entry *candidate_field = dst.find_field( current_type );
    // Nearby gas grows thicker, and ages are shared.
    const time_duration age_fraction = current_age / current_intensity ;
    if( candidate_field != nullptr ) {
        candidate_field->set_field_intensity( candidate_field->get_field_intensity() + 1 );
        cur.set_field_intensity( current_intensity - 1 );
        candidate_field->set_field_age( candidate_field->get_field_age() + age_fraction );
        cur.set_field_age( current_age - age_fraction );
        // Or, just create a new field.
    } else if( dst.add_field( current_type, 1, 0_turns ) ) {
        dst.find_field( current_type )->set_field_age( age_fraction );
        cur.set_field_intensity( current_intensity - 1 );
        cur.set_field_age( current_age - age_fraction );
    }
}

void map::spread_gas( field_entry &cur, const tripoint &p, int percent_spread,
                      const time_duration &outdoor_age_speedup, scent_block &sblk )
{
    const oter_id &cur_om_ter = overmap_buffer.ter( ms_to_omt_copy( g->m.getabs( p ) ) );
    const bool sheltered = g->is_sheltered( p );
    const int winddirection = g->weather.winddirection;
    const int windpower = get_local_windpower( g->weather.windspeed, cur_om_ter, p, winddirection,
                          sheltered );

    const int current_intensity = cur.get_field_intensity();
    const field_type_id ft_id = cur.get_field_type();

    const int scent_neutralize = ft_id->get_intensity_level( current_intensity -
                                 1 ).scent_neutralization;

    if( scent_neutralize > 0 ) {
        // modify scents by neutralization value (minus)
        for( const tripoint &tmp : points_in_radius( p, 1 ) ) {
            sblk.apply_gas( tmp, scent_neutralize );
        }
    }

    // Dissipate faster outdoors.
    if( is_outside( p ) ) {
        const time_duration current_age = cur.get_field_age();
        cur.set_field_age( current_age + outdoor_age_speedup );
    }

    // Bail out if we don't meet the spread chance or required intensity.
    if( current_intensity <= 1 || rng( 1, 100 - windpower ) > percent_spread ) {
        return;
    }

    // First check if we can fall
    // TODO: Make fall and rise chances parameters to enable heavy/light gas
    if( zlevels && p.z > -OVERMAP_DEPTH ) {
        const tripoint down{ p.xy(), p.z - 1 };
        maptile down_tile = maptile_at_internal( down );
        if( gas_can_spread_to( cur, down_tile ) && valid_move( p, down, true, true ) ) {
            gas_spread_to( cur, down_tile );
            return;
        }
    }

    auto neighs = get_neighbors( p );
    size_t end_it = static_cast<size_t>( rng( 0, neighs.size() - 1 ) );
    std::vector<size_t> spread;
    std::vector<maptile> neighbour_vec;
    // Then, spread to a nearby point.
    // If not possible (or randomly), try to spread up
    // Wind direction will block the field spreading into the wind.
    // Start at end_it + 1, then wrap around until all elements have been processed.
    for( size_t i = ( end_it + 1 ) % neighs.size(), count = 0 ;
         count != neighs.size();
         i = ( i + 1 ) % neighs.size(), count++ ) {
        const auto &neigh = neighs[i];
        if( gas_can_spread_to( cur, neigh ) ) {
            spread.push_back( i );
        }
    }
    auto maptiles = get_wind_blockers( winddirection, p );
    // Three map tiles that are facing the wind direction.
    const maptile remove_tile = std::get<0>( maptiles );
    const maptile remove_tile2 = std::get<1>( maptiles );
    const maptile remove_tile3 = std::get<2>( maptiles );
    if( !spread.empty() && ( !zlevels || one_in( spread.size() ) ) ) {
        // Construct the destination from offset and p
        if( g->is_sheltered( p ) || windpower < 5 ) {
            gas_spread_to( cur, neighs[ random_entry( spread ) ] );
        } else {
            end_it = static_cast<size_t>( rng( 0, neighs.size() - 1 ) );
            // Start at end_it + 1, then wrap around until all elements have been processed.
            for( size_t i = ( end_it + 1 ) % neighs.size(), count = 0 ;
                 count != neighs.size();
                 i = ( i + 1 ) % neighs.size(), count++ ) {
                const auto &neigh = neighs[i];
                if( ( neigh.x != remove_tile.x && neigh.y != remove_tile.y ) ||
                    ( neigh.x != remove_tile2.x && neigh.y != remove_tile2.y ) ||
                    ( neigh.x != remove_tile3.x && neigh.y != remove_tile3.y ) ) {
                    neighbour_vec.push_back( neigh );
                } else if( x_in_y( 1, std::max( 2, windpower ) ) ) {
                    neighbour_vec.push_back( neigh );
                }
            }
            if( !neighbour_vec.empty() ) {
                gas_spread_to( cur, neighbour_vec[rng( 0, neighbour_vec.size() - 1 )] );
            }
        }
    } else if( zlevels && p.z < OVERMAP_HEIGHT ) {
        const tripoint up{ p.xy(), p.z + 1 };
        maptile up_tile = maptile_at_internal( up );
        if( gas_can_spread_to( cur, up_tile ) && valid_move( p, up, true, true ) ) {
            gas_spread_to( cur, up_tile );
        }
    }
}

/*
Helper function that encapsulates the logic involved in creating hot air.
*/
void map::create_hot_air( const tripoint &p, int intensity )
{
    field_type_id hot_air;
    switch( intensity ) {
        case 1:
            hot_air = fd_hot_air1;
            break;
        case 2:
            hot_air = fd_hot_air2;
            break;
        case 3:
            hot_air = fd_hot_air3;
            break;
        case 4:
            hot_air = fd_hot_air4;
            break;
        default:
            debugmsg( "Tried to spread hot air with intensity %d", intensity );
            return;
    }

    for( int counter = 0; counter < 5; counter++ ) {
        tripoint dst( p + point( rng( -1, 1 ), rng( -1, 1 ) ) );
        add_field( dst, hot_air, 1 );
    }
}

/*
Function: process_fields_in_submap
Iterates over every field on every tile of the given submap given as parameter.
This is the general update function for field effects. This should only be called once per game turn.
If you need to insert a new field behavior per unit time add a case statement in the switch below.
*/
bool map::process_fields_in_submap( submap *const current_submap,
                                    const tripoint &submap )
{
    scent_block sblk( submap.x, submap.y, submap.z, g->scent );

    // This should be true only when the field changes transparency
    // More correctly: not just when the field is opaque, but when it changes state
    // to a more/less transparent one, or creates a non-transparent field nearby
    bool dirty_transparency_cache = false;
    // Holds m.field_at(x,y).find_field(fd_some_field) type returns.
    // Just to avoid typing that long string for a temp value.
    field_entry *tmpfld = nullptr;

    tripoint thep;
    thep.z = submap.z;

    // Initialize the map tile wrapper
    maptile map_tile( current_submap, 0, 0 );
    size_t &locx = map_tile.x;
    size_t &locy = map_tile.y;
    // Loop through all tiles in this submap indicated by current_submap
    for( locx = 0; locx < SEEX; locx++ ) {
        for( locy = 0; locy < SEEY; locy++ ) {
            // This is a translation from local coordinates to submap coordinates.
            // All submaps are in one long 1d array.
            thep.x = locx + submap.x * SEEX;
            thep.y = locy + submap.y * SEEY;
            // A const reference to the tripoint above, so that the code below doesn't accidentally change it
            const tripoint &p = thep;
            // Get a reference to the field variable from the submap;
            // contains all the pointers to the real field effects.
            field &curfield = current_submap->fld[locx][locy];
            for( auto it = curfield.begin(); it != curfield.end(); ) {
                // Iterating through all field effects in the submap's field.
                field_entry &cur = it->second;
                // The field might have been killed by processing a neighbor field
                if( !cur.is_field_alive() ) {
                    if( !cur.get_field_type().obj().get_transparent( cur.get_field_intensity() - 1 ) ) {
                        dirty_transparency_cache = true;
                    }
                    --current_submap->field_count;
                    curfield.remove_field( it++ );
                    continue;
                }

                // Holds cur.get_field_type() as that is what the old system used before rewrite.
                field_type_id curtype = cur.get_field_type();
                // Again, legacy support in the event someone Mods set_field_intensity to allow more values.
                if( cur.get_field_intensity() > 3 || cur.get_field_intensity() < 1 ) {
                    // TODO: Remove this eventually as we would suppoort more than 3 field intensity levels
                    debugmsg( "Whoooooa intensity of %d", cur.get_field_intensity() );
                }

                dirty_transparency_cache = curtype.obj().dirty_transparency_cache;

                // Don't process "newborn" fields. This gives the player time to run if they need to.
                if( cur.get_field_age() == 0_turns ) {
                    curtype = fd_null;
                }

                // Upgrade field intensity
                if( cur.intensity_upgrade_chance() > 0 &&
                    one_in( cur.intensity_upgrade_chance() ) &&
                    cur.intensity_upgrade_duration() > 0_turns &&
                    calendar::once_every( cur.intensity_upgrade_duration() ) ) {
                    cur.set_field_intensity( cur.get_field_intensity() + 1 );
                }

                int part;
                const ter_t &ter = map_tile.get_ter_t();
                // Dissipate faster in water
                if( ter.has_flag( TFLAG_SWIMMABLE ) ) {
                    cur.mod_field_age( cur.get_underwater_age_speedup() );
                }
                if( curtype == fd_acid ) {
                    // Try to fall by a z-level
                    if( zlevels && p.z > -OVERMAP_DEPTH ) {
                        tripoint dst{ p.xy(), p.z - 1 };
                        if( valid_move( p, dst, true, true ) ) {
                            maptile dst_tile = maptile_at_internal( dst );
                            field_entry *acid_there = dst_tile.find_field( fd_acid );
                            if( acid_there == nullptr ) {
                                dst_tile.add_field( fd_acid, cur.get_field_intensity(), cur.get_field_age() );
                            } else {
                                // Math can be a bit off,
                                // but "boiling" falling acid can be allowed to be stronger
                                // than acid that just lies there
                                const int sum_intensity = cur.get_field_intensity() + acid_there->get_field_intensity();
                                const int new_intensity = std::min( 3, sum_intensity );
                                // No way to get precise elapsed time, let's always reset
                                // Allow falling acid to last longer than regular acid to show it off
                                const time_duration new_age = -1_minutes * ( sum_intensity - new_intensity );
                                acid_there->set_field_intensity( new_intensity );
                                acid_there->set_field_age( new_age );
                            }

                            // Set ourselves up for removal
                            cur.set_field_intensity( 0 );
                        }
                    }
                    // TODO: Allow spreading to the sides if age < 0 && intensity == 3
                }
                if( curtype.obj().apply_slime_factor > 0 ) {
                    sblk.apply_slime( p, cur.get_field_intensity() * curtype.obj().apply_slime_factor );
                }
                if( curtype == fd_fire ) {
                    // Entire objects for ter/frn for flags
                    const oter_id &cur_om_ter = overmap_buffer.ter( ms_to_omt_copy( g->m.getabs( p ) ) );
                    bool sheltered = g->is_sheltered( p );
                    int winddirection = g->weather.winddirection;
                    int windpower = get_local_windpower( g->weather.windspeed, cur_om_ter, p, winddirection,
                                                         sheltered );
                    const ter_t &ter = map_tile.get_ter_t();
                    const furn_t &frn = map_tile.get_furn_t();

                    // We've got ter/furn cached, so let's use that
                    const bool is_sealed = ter_furn_has_flag( ter, frn, TFLAG_SEALED ) &&
                                           !ter_furn_has_flag( ter, frn, TFLAG_ALLOW_FIELD_EFFECT );
                    // Smoke generation probability, consumed items count
                    int smoke = 0;
                    int consumed = 0;
                    // How much time to add to the fire's life due to burned items/terrain/furniture
                    time_duration time_added = 0_turns;
                    // Checks if the fire can spread
                    const bool can_spread = !ter_furn_has_flag( ter, frn, TFLAG_FIRE_CONTAINER );
                    // If the flames are in furniture with fire_container flag like brazier or oven,
                    // they're fully contained, so skip consuming terrain
                    const bool can_burn = ( ter.is_flammable() || frn.is_flammable() ) &&
                                          !ter_furn_has_flag( ter, frn, TFLAG_FIRE_CONTAINER );
                    // The huge indent below should probably be somehow moved away from here
                    // without forcing the function to use i_at( p ) for fires without items
                    if( !is_sealed && map_tile.get_item_count() > 0 ) {
                        map_stack items_here = i_at( p );
                        std::vector<item> new_content;
                        for( auto explosive = items_here.begin(); explosive != items_here.end(); ) {
                            if( explosive->will_explode_in_fire() ) {
                                // We need to make a copy because the iterator validity is not predictable
                                item copy = *explosive;
                                explosive = items_here.erase( explosive );
                                if( copy.detonate( p, new_content ) ) {
                                    // Need to restart, iterators may not be valid
                                    explosive = items_here.begin();
                                }
                            } else {
                                ++explosive;
                            }
                        }

                        fire_data frd( cur.get_field_intensity(), !can_spread );
                        // The highest # of items this fire can remove in one turn
                        int max_consume = cur.get_field_intensity() * 2;

                        for( auto fuel = items_here.begin(); fuel != items_here.end() && consumed < max_consume; ) {
                            // `item::burn` modifies the charges in order to simulate some of them getting
                            // destroyed by the fire, this changes the item weight, but may not actually
                            // destroy it. We need to spawn products anyway.
                            const units::mass old_weight = fuel->weight( false );
                            bool destroyed = fuel->burn( frd );
                            // If the item is considered destroyed, it may have negative charge count,
                            // see `item::burn?. This in turn means `item::weight` returns a negative value,
                            // which we can not use, so only call `weight` when it's still an existing item.
                            const units::mass new_weight = destroyed ? 0_gram : fuel->weight( false );
                            if( old_weight != new_weight ) {
                                create_burnproducts( p, *fuel, old_weight - new_weight );
                            }

                            if( destroyed ) {
                                // If we decided the item was destroyed by fire, remove it.
                                // But remember its contents, except for irremovable mods, if any
                                std::copy( fuel->contents.begin(), fuel->contents.end(),
                                           std::back_inserter( new_content ) );
                                new_content.erase( std::remove_if( new_content.begin(), new_content.end(), [&]( const item & i ) {
                                    return i.is_irremovable();
                                } ), new_content.end() );
                                fuel = items_here.erase( fuel );
                                consumed++;
                            } else {
                                ++fuel;
                            }
                        }

                        spawn_items( p, new_content );
                        smoke = roll_remainder( frd.smoke_produced );
                        time_added = 1_turns * roll_remainder( frd.fuel_produced );
                    }

                    // Get the part of the vehicle in the fire (_internal skips the boundary check)
                    vehicle *veh = veh_at_internal( p, part );
                    if( veh != nullptr ) {
                        veh->damage( part, cur.get_field_intensity() * 10, DT_HEAT, true );
                        // Damage the vehicle in the fire.
                    }
                    if( can_burn ) {
                        if( ter.has_flag( TFLAG_SWIMMABLE ) ) {
                            // Flames die quickly on water
                            cur.set_field_age( cur.get_field_age() + 4_minutes );
                        }

                        // Consume the terrain we're on
                        if( ter_furn_has_flag( ter, frn, TFLAG_FLAMMABLE ) ) {
                            // The fire feeds on the ground itself until max intensity.
                            time_added += 1_turns * ( 5 - cur.get_field_intensity() );
                            smoke += 2;
                            smoke += static_cast<int>( windpower / 5 );
                            if( cur.get_field_intensity() > 1 &&
                                one_in( 200 - cur.get_field_intensity() * 50 ) ) {
                                destroy( p, false );
                            }

                        } else if( ter_furn_has_flag( ter, frn, TFLAG_FLAMMABLE_HARD ) &&
                                   one_in( 3 ) ) {
                            // The fire feeds on the ground itself until max intensity.
                            time_added += 1_turns * ( 4 - cur.get_field_intensity() );
                            smoke += 2;
                            smoke += static_cast<int>( windpower / 5 );
                            if( cur.get_field_intensity() > 1 &&
                                one_in( 200 - cur.get_field_intensity() * 50 ) ) {
                                destroy( p, false );
                            }

                        } else if( ter.has_flag( TFLAG_FLAMMABLE_ASH ) ) {
                            // The fire feeds on the ground itself until max intensity.
                            time_added += 1_turns * ( 5 - cur.get_field_intensity() );
                            smoke += 2;
                            smoke += static_cast<int>( windpower / 5 );
                            if( cur.get_field_intensity() > 1 &&
                                one_in( 200 - cur.get_field_intensity() * 50 ) ) {
                                if( p.z > 0 ) {
                                    // We're in the air
                                    ter_set( p, t_open_air );
                                } else {
                                    ter_set( p, t_dirt );
                                }
                            }

                        } else if( frn.has_flag( TFLAG_FLAMMABLE_ASH ) ) {
                            // The fire feeds on the ground itself until max intensity.
                            time_added += 1_turns * ( 5 - cur.get_field_intensity() );
                            smoke += 2;
                            smoke += static_cast<int>( windpower / 5 );
                            if( cur.get_field_intensity() > 1 &&
                                one_in( 200 - cur.get_field_intensity() * 50 ) ) {
                                furn_set( p, f_ash );
                                add_item_or_charges( p, item( "ash" ) );
                            }

                        } else if( ter.has_flag( TFLAG_NO_FLOOR ) && zlevels && p.z > -OVERMAP_DEPTH ) {
                            // We're hanging in the air - let's fall down
                            tripoint dst{ p.xy(), p.z - 1 };
                            if( valid_move( p, dst, true, true ) ) {
                                maptile dst_tile = maptile_at_internal( dst );
                                field_entry *fire_there = dst_tile.find_field( fd_fire );
                                if( fire_there == nullptr ) {
                                    dst_tile.add_field( fd_fire, 1, 0_turns );
                                    cur.set_field_intensity( cur.get_field_intensity() - 1 );
                                } else {
                                    // Don't fuel raging fires or they'll burn forever
                                    // as they can produce small fires above themselves
                                    int new_intensity = std::max( cur.get_field_intensity(),
                                                                  fire_there->get_field_intensity() );
                                    // Allow smaller fires to combine
                                    if( new_intensity < 3 &&
                                        cur.get_field_intensity() == fire_there->get_field_intensity() ) {
                                        new_intensity++;
                                    }
                                    // A raging fire below us can support us for a while
                                    // Otherwise decay and decay fast
                                    if( fire_there->get_field_intensity() < 3 || one_in( 10 ) ) {
                                        cur.set_field_intensity( cur.get_field_intensity() - 1 );
                                    }
                                    fire_there->set_field_intensity( new_intensity );
                                }
                                break;
                            }
                        }
                    }
                    // Lower age is a longer lasting fire
                    if( time_added != 0_turns ) {
                        cur.set_field_age( cur.get_field_age() - time_added );
                    } else if( can_burn ) {
                        // Nothing to burn = fire should be dying out faster
                        // Drain more power from big fires, so that they stop raging over nothing
                        // Except for fires on stoves and fireplaces, those are made to keep the fire alive
                        cur.mod_field_age( 10_seconds * cur.get_field_intensity() );
                    }

                    // Below we will access our nearest 8 neighbors, so let's cache them now
                    // This should probably be done more globally, because large fires will re-do it a lot
                    auto neighs = get_neighbors( p );
                    // Get the neighbours that are allowed due to wind direction
                    auto maptiles = get_wind_blockers( winddirection, p );
                    maptile remove_tile = std::get<0>( maptiles );
                    maptile remove_tile2 = std::get<1>( maptiles );
                    maptile remove_tile3 = std::get<2>( maptiles );
                    std::vector<maptile> neighbour_vec;
                    size_t end_it = static_cast<size_t>( rng( 0, neighs.size() - 1 ) );
                    // Start at end_it + 1, then wrap around until all elements have been processed
                    for( size_t i = ( end_it + 1 ) % neighs.size(), count = 0;
                         count != neighs.size();
                         i = ( i + 1 ) % neighs.size(), count++ ) {
                        const auto &neigh = neighs[i];
                        if( ( neigh.x != remove_tile.x && neigh.y != remove_tile.y ) ||
                            ( neigh.x != remove_tile2.x && neigh.y != remove_tile2.y ) ||
                            ( neigh.x != remove_tile3.x && neigh.y != remove_tile3.y ) ) {
                            neighbour_vec.push_back( neigh );
                        } else if( x_in_y( 1, std::max( 2, windpower ) ) ) {
                            neighbour_vec.push_back( neigh );
                        }
                    }
                    // If the flames are in a pit, it can't spread to non-pit
                    const bool in_pit = ter.id.id() == t_pit;

                    // Count adjacent fires, to optimize out needless smoke and hot air
                    int adjacent_fires = 0;

                    // If the flames are big, they contribute to adjacent flames
                    if( can_spread ) {
                        if( cur.get_field_intensity() > 1 && one_in( 3 ) ) {
                            // Basically: Scan around for a spot,
                            // if there is more fire there, make it bigger and give it some fuel.
                            // This is how big fires spend their excess age:
                            // making other fires bigger. Flashpoint.
                            if( sheltered || windpower < 5 ) {
                                end_it = static_cast<size_t>( rng( 0, neighs.size() - 1 ) );
                                for( size_t i = ( end_it + 1 ) % neighs.size(), count = 0;
                                     count != neighs.size() && cur.get_field_age() < 0_turns;
                                     i = ( i + 1 ) % neighs.size(), count++ ) {
                                    maptile &dst = neighs[i];
                                    auto dstfld = dst.find_field( fd_fire );
                                    // If the fire exists and is weaker than ours, boost it
                                    if( dstfld != nullptr &&
                                        ( dstfld->get_field_intensity() <= cur.get_field_intensity() ||
                                          dstfld->get_field_age() > cur.get_field_age() ) &&
                                        ( in_pit == ( dst.get_ter() == t_pit ) ) ) {
                                        if( dstfld->get_field_intensity() < 2 ) {
                                            dstfld->set_field_intensity( dstfld->get_field_intensity() + 1 );
                                        }

                                        dstfld->set_field_age( dstfld->get_field_age() - 5_minutes );
                                        cur.set_field_age( cur.get_field_age() + 5_minutes );
                                    }
                                    if( dstfld != nullptr ) {
                                        adjacent_fires++;
                                    }
                                }
                            } else {
                                end_it = static_cast<size_t>( rng( 0, neighbour_vec.size() - 1 ) );
                                for( size_t i = ( end_it + 1 ) % neighbour_vec.size(), count = 0;
                                     count != neighbour_vec.size() && cur.get_field_age() < 0_turns;
                                     i = ( i + 1 ) % neighbour_vec.size(), count++ ) {
                                    maptile &dst = neighbour_vec[i];
                                    field_entry *dstfld = dst.find_field( fd_fire );
                                    // If the fire exists and is weaker than ours, boost it
                                    if( dstfld != nullptr &&
                                        ( dstfld->get_field_intensity() <= cur.get_field_intensity() ||
                                          dstfld->get_field_age() > cur.get_field_age() ) &&
                                        ( in_pit == ( dst.get_ter() == t_pit ) ) ) {
                                        if( dstfld->get_field_intensity() < 2 ) {
                                            dstfld->set_field_intensity( dstfld->get_field_intensity() + 1 );
                                        }

                                        dstfld->set_field_age( dstfld->get_field_age() - 5_minutes );
                                        cur.set_field_age( cur.get_field_age() + 5_minutes );
                                    }

                                    if( dstfld != nullptr ) {
                                        adjacent_fires++;
                                    }
                                }
                            }
                        } else if( cur.get_field_age() < 0_turns && cur.get_field_intensity() < 3 ) {
                            // See if we can grow into a stage 2/3 fire, for this
                            // burning neighbors are necessary in addition to
                            // field age < 0, or alternatively, a LOT of fuel.

                            // The maximum fire intensity is 1 for a lone fire, 2 for at least 1 neighbor,
                            // 3 for at least 2 neighbors.
                            int maximum_intensity = 1;

                            // The following logic looks a bit complex due to optimization concerns, so here are the semantics:
                            // 1. Calculate maximum field intensity based on fuel, -50 minutes is 2(medium), -500 minutes is 3(raging)
                            // 2. Calculate maximum field intensity based on neighbors, 3 neighbors is 2(medium), 7 or more neighbors is 3(raging)
                            // 3. Pick the higher maximum between 1. and 2.
                            if( cur.get_field_age() < -500_minutes ) {
                                maximum_intensity = 3;
                            } else {
                                for( auto &neigh : neighs ) {
                                    if( neigh.get_field().find_field( fd_fire ) != nullptr ) {
                                        adjacent_fires++;
                                    }
                                }
                                maximum_intensity = 1 + ( adjacent_fires >= 3 ) + ( adjacent_fires >= 7 );

                                if( maximum_intensity < 2 && cur.get_field_age() < -50_minutes ) {
                                    maximum_intensity = 2;
                                }
                            }

                            // If we consumed a lot, the flames grow higher
                            if( cur.get_field_intensity() < maximum_intensity && cur.get_field_age() < 0_turns ) {
                                // Fires under 0 age grow in size. Level 3 fires under 0 spread later on.
                                // Weaken the newly-grown fire
                                cur.set_field_intensity( cur.get_field_intensity() + 1 );
                                cur.set_field_age( cur.get_field_age() + 10_minutes * cur.get_field_intensity() );
                            }
                        }
                    }
                    // Consume adjacent fuel / terrain / webs to spread.
                    // Allow raging fires (and only raging fires) to spread up
                    // Spreading down is achieved by wrecking the walls/floor and then falling
                    if( zlevels && cur.get_field_intensity() == 3 && p.z < OVERMAP_HEIGHT ) {
                        // Let it burn through the floor
                        maptile dst = maptile_at_internal( {p.xy(), p.z + 1} );
                        const auto &dst_ter = dst.get_ter_t();
                        if( dst_ter.has_flag( TFLAG_NO_FLOOR ) ||
                            dst_ter.has_flag( TFLAG_FLAMMABLE ) ||
                            dst_ter.has_flag( TFLAG_FLAMMABLE_ASH ) ||
                            dst_ter.has_flag( TFLAG_FLAMMABLE_HARD ) ) {
                            field_entry *nearfire = dst.find_field( fd_fire );
                            if( nearfire != nullptr ) {
                                nearfire->mod_field_age( -2_turns );
                            } else {
                                dst.add_field( fd_fire, 1, 0_turns );
                            }
                            // Fueling fires above doesn't cost fuel
                        }
                    }
                    // Our iterator will start at end_i + 1 and increment from there and then wrap around.
                    // This guarantees it will check all neighbors, starting from a random one
                    if( sheltered || windpower < 5 ) {
                        const size_t end_i = static_cast<size_t>( rng( 0, neighs.size() - 1 ) );
                        for( size_t i = ( end_i + 1 ) % neighs.size(), count = 0;
                             count != neighs.size();
                             i = ( i + 1 ) % neighs.size(), count++ ) {
                            if( one_in( cur.get_field_intensity() * 2 ) ) {
                                // Skip some processing to save on CPU
                                continue;
                            }

                            maptile &dst = neighs[i];
                            // No bounds checking here: we'll treat the invalid neighbors as valid.
                            // We're using the map tile wrapper, so we can treat invalid tiles as sentinels.
                            // This will create small oddities on map edges, but nothing more noticeable than
                            // "cut-off" that happens with bounds checks.

                            field_entry *nearfire = dst.find_field( fd_fire );
                            if( nearfire != nullptr ) {
                                // We handled supporting fires in the section above, no need to do it here
                                continue;
                            }

                            field_entry *nearwebfld = dst.find_field( fd_web );
                            int spread_chance = 25 * ( cur.get_field_intensity() - 1 );
                            if( nearwebfld != nullptr ) {
                                spread_chance = 50 + spread_chance / 2;
                            }

                            const ter_t &dster = dst.get_ter_t();
                            const furn_t &dsfrn = dst.get_furn_t();
                            // Allow weaker fires to spread occasionally
                            const int power = cur.get_field_intensity() + one_in( 5 );
                            if( can_spread && rng( 1, 100 ) < spread_chance &&
                                ( dster.is_flammable() || dsfrn.is_flammable() ) &&
                                ( in_pit == ( dster.id.id() == t_pit ) ) &&
                                (
                                    ( power >= 3 && cur.get_field_age() < 0_turns && one_in( 20 ) ) ||
                                    ( power >= 2 && ( ter_furn_has_flag( dster, dsfrn, TFLAG_FLAMMABLE ) && one_in( 2 ) ) ) ||
                                    ( power >= 2 && ( ter_furn_has_flag( dster, dsfrn, TFLAG_FLAMMABLE_ASH ) && one_in( 2 ) ) ) ||
                                    ( power >= 3 && ( ter_furn_has_flag( dster, dsfrn, TFLAG_FLAMMABLE_HARD ) && one_in( 5 ) ) ) ||
                                    nearwebfld || ( dst.get_item_count() > 0 &&
                                                    flammable_items_at( p + eight_horizontal_neighbors[i] ) &&
                                                    one_in( 5 ) )
                                ) ) {
                                // Nearby open flammable ground? Set it on fire.
                                dst.add_field( fd_fire, 1, 0_turns );
                                tmpfld = dst.find_field( fd_fire );
                                if( tmpfld != nullptr ) {
                                    // Make the new fire quite weak, so that it doesn't start jumping around instantly
                                    tmpfld->set_field_age( 2_minutes );
                                    // Consume a bit of our fuel
                                    cur.set_field_age( cur.get_field_age() + 1_minutes );
                                }
                                if( nearwebfld ) {
                                    nearwebfld->set_field_intensity( 0 );
                                }
                            }
                        }
                    } else {
                        const size_t end_i = static_cast<size_t>( rng( 0, neighbour_vec.size() - 1 ) );
                        for( size_t i = ( end_i + 1 ) % neighbour_vec.size(), count = 0;
                             count != neighbour_vec.size();
                             i = ( i + 1 ) % neighbour_vec.size(), count++ ) {
                            if( one_in( cur.get_field_intensity() * 2 ) ) {
                                // Skip some processing to save on CPU
                                continue;
                            }

                            if( neighbour_vec.empty() ) {
                                continue;
                            }

                            maptile &dst = neighbour_vec[i];
                            // No bounds checking here: we'll treat the invalid neighbors as valid.
                            // We're using the map tile wrapper, so we can treat invalid tiles as sentinels.
                            // This will create small oddities on map edges, but nothing more noticeable than
                            // "cut-off" that happens with bounds checks.

                            field_entry *nearfire = dst.find_field( fd_fire );
                            if( nearfire != nullptr ) {
                                // We handled supporting fires in the section above, no need to do it here
                                continue;
                            }

                            field_entry *nearwebfld = dst.find_field( fd_web );
                            int spread_chance = 25 * ( cur.get_field_intensity() - 1 );
                            if( nearwebfld != nullptr ) {
                                spread_chance = 50 + spread_chance / 2;
                            }

                            const ter_t &dster = dst.get_ter_t();
                            const furn_t &dsfrn = dst.get_furn_t();
                            // Allow weaker fires to spread occasionally
                            const int power = cur.get_field_intensity() + one_in( 5 );
                            if( can_spread && rng( 1, 100 - windpower ) < spread_chance &&
                                ( dster.is_flammable() || dsfrn.is_flammable() ) &&
                                ( in_pit == ( dster.id.id() == t_pit ) ) &&
                                (
                                    ( power >= 3 && cur.get_field_age() < 0_turns && one_in( 20 ) ) ||
                                    ( power >= 2 && ( ter_furn_has_flag( dster, dsfrn, TFLAG_FLAMMABLE ) && one_in( 2 ) ) ) ||
                                    ( power >= 2 && ( ter_furn_has_flag( dster, dsfrn, TFLAG_FLAMMABLE_ASH ) && one_in( 2 ) ) ) ||
                                    ( power >= 3 && ( ter_furn_has_flag( dster, dsfrn, TFLAG_FLAMMABLE_HARD ) && one_in( 5 ) ) ) ||
                                    nearwebfld || ( dst.get_item_count() > 0 &&
                                                    flammable_items_at( p + eight_horizontal_neighbors[i] ) &&
                                                    one_in( 5 ) )
                                ) ) {
                                // Nearby open flammable ground? Set it on fire.
                                dst.add_field( fd_fire, 1, 0_turns );
                                tmpfld = dst.find_field( fd_fire );
                                if( tmpfld != nullptr ) {
                                    // Make the new fire quite weak, so that it doesn't start jumping around instantly
                                    tmpfld->set_field_age( 2_minutes );
                                    // Consume a bit of our fuel
                                    cur.set_field_age( cur.get_field_age() + 1_minutes );
                                }
                                if( nearwebfld ) {
                                    nearwebfld->set_field_intensity( 0 );
                                }
                            }
                        }
                    }
                    // Create smoke once - above us if possible, at us otherwise
                    if( !ter_furn_has_flag( ter, frn, TFLAG_SUPPRESS_SMOKE ) &&
                        rng( 0, 100 - windpower ) <= smoke &&
                        rng( 3, 35 ) < cur.get_field_intensity() * 10 ) {
                        bool smoke_up = zlevels && p.z < OVERMAP_HEIGHT;
                        if( smoke_up ) {
                            tripoint up{p.xy(), p.z + 1};
                            maptile dst = maptile_at_internal( up );
                            const ter_t &dst_ter = dst.get_ter_t();
                            if( dst_ter.has_flag( TFLAG_NO_FLOOR ) ) {
                                dst.add_field( fd_smoke, rng( 1, cur.get_field_intensity() ), 0_turns );
                            } else {
                                // Can't create smoke above
                                smoke_up = false;
                            }
                        }

                        if( !smoke_up ) {
                            maptile dst = maptile_at_internal( p );
                            // Create thicker smoke
                            dst.add_field( fd_smoke, cur.get_field_intensity(), 0_turns );
                        }

                        // Smoke affects transparency
                        dirty_transparency_cache = true;
                    }

                    // Hot air is a load on the CPU
                    // Don't produce too much of it if we have a lot fires nearby, they produce
                    // radiant heat which does what hot air would do anyway
                    if( adjacent_fires < 5 && rng( 0, 4 - adjacent_fires ) ) {
                        create_hot_air( p, cur.get_field_intensity() );
                    }
                }

                // Spread gaseous fields
                if( cur.gas_can_spread() ) {
                    const int gas_percent_spread = curtype.obj().percent_spread;
                    if( gas_percent_spread > 0 ) {
                        const time_duration outdoor_age_speedup = curtype.obj().outdoor_age_speedup;
                        spread_gas( cur, p, gas_percent_spread, outdoor_age_speedup, sblk );
                    }
                }

                if( curtype == fd_fungal_haze ) {
                    if( one_in( 10 - 2 * cur.get_field_intensity() ) ) {
                        // Haze'd terrain
                        fungal_effects( *g, g->m ).spread_fungus( p );
                    }
                }

                // Process npc complaints
                const std::tuple<int, std::string, time_duration, std::string> &npc_complain_data =
                    curtype.obj().npc_complain_data;
                const int chance = std::get<0>( npc_complain_data );
                if( chance > 0 && one_in( chance ) ) {
                    if( npc *const np = g->critter_at<npc>( p, false ) ) {
                        np->complain_about( std::get<1>( npc_complain_data ),
                                            std::get<2>( npc_complain_data ),
                                            std::get<3>( npc_complain_data ) );
                    }
                }

                // Apply radiation
                if( cur.extra_radiation_max() > 0 ) {
                    int extra_radiation = rng( cur.extra_radiation_min(), cur.extra_radiation_max() );
                    adjust_radiation( p, extra_radiation );
                }

                // Apply wandering fields from vents
                if( curtype.obj().wandering_field.is_valid() ) {
                    for( const tripoint &pnt : points_in_radius( p, cur.get_field_intensity() - 1 ) ) {
                        field &wandering_field = get_field( pnt );
                        tmpfld = wandering_field.find_field( curtype.obj().wandering_field );
                        if( tmpfld && tmpfld->get_field_intensity() < cur.get_field_intensity() ) {
                            tmpfld->set_field_intensity( tmpfld->get_field_intensity() + 1 );
                        } else {
                            add_field( pnt, curtype.obj().wandering_field, cur.get_field_intensity() );
                        }
                    }
                }

                if( curtype == fd_fire_vent ) {

                    if( cur.get_field_intensity() > 1 ) {
                        if( one_in( 3 ) ) {
                            cur.set_field_intensity( cur.get_field_intensity() - 1 );
                        }
                        create_hot_air( p, cur.get_field_intensity() );
                    } else {
                        dirty_transparency_cache = true;
                        add_field( p, fd_flame_burst, 3, cur.get_field_age() );
                        cur.set_field_intensity( 0 );
                    }
                }
                if( curtype == fd_flame_burst ) {
                    if( cur.get_field_intensity() > 1 ) {
                        cur.set_field_intensity( cur.get_field_intensity() - 1 );
                        create_hot_air( p, cur.get_field_intensity() );
                    } else {
                        dirty_transparency_cache = true;
                        add_field( p, fd_fire_vent, 3, cur.get_field_age() );
                        cur.set_field_intensity( 0 );
                    }
                }
                if( curtype == fd_electricity ) {
                    // 4 in 5 chance to spread
                    if( !one_in( 5 ) ) {
                        std::vector<tripoint> valid;
                        // We're grounded
                        if( impassable( p ) && cur.get_field_intensity() > 1 ) {
                            int tries = 0;
                            tripoint pnt;
                            pnt.z = p.z;
                            while( tries < 10 && cur.get_field_age() < 5_minutes && cur.get_field_intensity() > 1 ) {
                                pnt.x = p.x + rng( -1, 1 );
                                pnt.y = p.y + rng( -1, 1 );
                                if( passable( pnt ) ) {
                                    add_field( pnt, fd_electricity, 1, cur.get_field_age() + 1_turns );
                                    cur.set_field_intensity( cur.get_field_intensity() - 1 );
                                    tries = 0;
                                } else {
                                    tries++;
                                }
                            }
                            // We're not grounded; attempt to ground
                        } else {
                            for( const tripoint &dst : points_in_radius( p, 1 ) ) {
                                // Grounded tiles first
                                if( impassable( dst ) ) {
                                    valid.push_back( dst );
                                }
                            }
                            // Spread to adjacent space, then
                            if( valid.empty() ) {
                                tripoint dst( p + point( rng( -1, 1 ), rng( -1, 1 ) ) );
                                field_entry *elec = get_field( dst ).find_field( fd_electricity );
                                if( passable( dst ) && elec != nullptr &&
                                    elec->get_field_intensity() < 3 ) {
                                    elec->set_field_intensity( elec->get_field_intensity() + 1 );
                                    cur.set_field_intensity( cur.get_field_intensity() - 1 );
                                } else if( passable( dst ) ) {
                                    add_field( dst, fd_electricity, 1, cur.get_field_age() + 1_turns );
                                }
                                cur.set_field_intensity( cur.get_field_intensity() - 1 );
                            }
                            while( !valid.empty() && cur.get_field_intensity() > 1 ) {
                                const tripoint target = random_entry_removed( valid );
                                add_field( target, fd_electricity, 1, cur.get_field_age() + 1_turns );
                                cur.set_field_intensity( cur.get_field_intensity() - 1 );
                            }
                        }
                    }
                }

                int monster_spawn_chance = cur.monster_spawn_chance();
                int monster_spawn_count = cur.monster_spawn_count();
                if( monster_spawn_count > 0 && monster_spawn_chance > 0 && one_in( monster_spawn_chance ) ) {
                    for( ; monster_spawn_count > 0; monster_spawn_count-- ) {
                        MonsterGroupResult spawn_details = MonsterGroupManager::GetResultFromGroup(
                                                               cur.monster_spawn_group(), &monster_spawn_count );
                        if( !spawn_details.name ) {
                            continue;
                        }
                        if( const auto spawn_point = random_point( points_in_radius( p,
                        cur.monster_spawn_radius() ), [this]( const tripoint & n ) {
                        return passable( n );
                        } ) ) {
                            add_spawn( spawn_details.name, spawn_details.pack_size, spawn_point->xy() );
                        }
                    }
                }

                if( curtype == fd_push_items ) {
                    map_stack items = i_at( p );
                    for( auto pushee = items.begin(); pushee != items.end(); ) {
                        if( pushee->typeId() != "rock" ||
                            pushee->age() < 1_turns ) {
                            pushee++;
                        } else {
                            item tmp = *pushee;
                            tmp.set_age( 0_turns );
                            pushee = items.erase( pushee );
                            std::vector<tripoint> valid;
                            for( const tripoint &dst : points_in_radius( p, 1 ) ) {
                                if( get_field( dst, fd_push_items ) != nullptr ) {
                                    valid.push_back( dst );
                                }
                            }
                            if( !valid.empty() ) {
                                tripoint newp = random_entry( valid );
                                add_item_or_charges( newp, tmp );
                                if( g->u.pos() == newp ) {
                                    add_msg( m_bad, _( "A %s hits you!" ), tmp.tname() );
                                    body_part hit = random_body_part();
                                    g->u.deal_damage( nullptr, hit, damage_instance( DT_BASH, 6 ) );
                                    g->u.check_dead_state();
                                }

                                if( npc *const p = g->critter_at<npc>( newp ) ) {
                                    // TODO: combine with player character code above
                                    body_part hit = random_body_part();
                                    p->deal_damage( nullptr, hit, damage_instance( DT_BASH, 6 ) );
                                    if( g->u.sees( newp ) ) {
                                        add_msg( _( "A %1$s hits %2$s!" ), tmp.tname(), p->name );
                                    }
                                    p->check_dead_state();
                                } else if( monster *const mon = g->critter_at<monster>( newp ) ) {
                                    mon->apply_damage( nullptr, bp_torso, 6 - mon->get_armor_bash( bp_torso ) );
                                    if( g->u.sees( newp ) ) {
                                        add_msg( _( "A %1$s hits the %2$s!" ), tmp.tname(), mon->name() );
                                    }
                                    mon->check_dead_state();
                                }
                            }
                        }
                    }
                }
                if( curtype == fd_shock_vent ) {
                    if( cur.get_field_intensity() > 1 ) {
                        if( one_in( 5 ) ) {
                            cur.set_field_intensity( cur.get_field_intensity() - 1 );
                        }
                    } else {
                        cur.set_field_intensity( 3 );
                        int num_bolts = rng( 3, 6 );
                        for( int i = 0; i < num_bolts; i++ ) {
                            int xdir = 0;
                            int ydir = 0;
                            while( xdir == 0 && ydir == 0 ) {
                                xdir = rng( -1, 1 );
                                ydir = rng( -1, 1 );
                            }
                            int dist = rng( 4, 12 );
                            int boltx = p.x;
                            int bolty = p.y;
                            for( int n = 0; n < dist; n++ ) {
                                boltx += xdir;
                                bolty += ydir;
                                add_field( tripoint( boltx, bolty, p.z ), fd_electricity, rng( 2, 3 ) );
                                if( one_in( 4 ) ) {
                                    if( xdir == 0 ) {
                                        xdir = rng( 0, 1 ) * 2 - 1;
                                    } else {
                                        xdir = 0;
                                    }
                                }
                                if( one_in( 4 ) ) {
                                    if( ydir == 0 ) {
                                        ydir = rng( 0, 1 ) * 2 - 1;
                                    } else {
                                        ydir = 0;
                                    }
                                }
                            }
                        }
                    }
                }
                if( curtype == fd_acid_vent ) {

                    if( cur.get_field_intensity() > 1 ) {
                        if( cur.get_field_age() >= 1_minutes ) {
                            cur.set_field_intensity( cur.get_field_intensity() - 1 );
                            cur.set_field_age( 0_turns );
                        }
                    } else {
                        cur.set_field_intensity( 3 );
                        for( const tripoint &t : points_in_radius( p, 5 ) ) {
                            const field_entry *acid = get_field( t, fd_acid );
                            if( acid != nullptr && acid->get_field_intensity() == 0 ) {
                                int new_intensity = 3 - rl_dist( p, t ) / 2 + ( one_in( 3 ) ? 1 : 0 );
                                if( new_intensity > 3 ) {
                                    new_intensity = 3;
                                }
                                if( new_intensity > 0 ) {
                                    add_field( t, fd_acid, new_intensity );
                                }
                            }
                        }
                    }
                }
                if( curtype == fd_bees ) {
                    // Poor bees are vulnerable to so many other fields.
                    // TODO: maybe adjust effects based on different fields.
                    if( curfield.find_field( fd_web ) ||
                        curfield.find_field( fd_fire ) ||
                        curfield.find_field( fd_smoke ) ||
                        curfield.find_field( fd_toxic_gas ) ||
                        curfield.find_field( fd_tear_gas ) ||
                        curfield.find_field( fd_relax_gas ) ||
                        curfield.find_field( fd_nuke_gas ) ||
                        curfield.find_field( fd_gas_vent ) ||
                        curfield.find_field( fd_smoke_vent ) ||
                        curfield.find_field( fd_fungicidal_gas ) ||
                        curfield.find_field( fd_insecticidal_gas ) ||
                        curfield.find_field( fd_fire_vent ) ||
                        curfield.find_field( fd_flame_burst ) ||
                        curfield.find_field( fd_electricity ) ||
                        curfield.find_field( fd_fatigue ) ||
                        curfield.find_field( fd_shock_vent ) ||
                        curfield.find_field( fd_plasma ) ||
                        curfield.find_field( fd_laser ) ||
                        curfield.find_field( fd_dazzling ) ||
                        curfield.find_field( fd_electricity ) ||
                        curfield.find_field( fd_incendiary ) ) {
                        // Kill them at the end of processing.
                        cur.set_field_intensity( 0 );
                    } else {
                        // Bees chase the player if in range, wander randomly otherwise.
                        if( !g->u.is_underwater() &&
                            rl_dist( p, g->u.pos() ) < 10 &&
                            clear_path( p, g->u.pos(), 10, 0, 100 ) ) {

                            std::vector<point> candidate_positions =
                                squares_in_direction( p.xy(), point( g->u.posx(), g->u.posy() ) );
                            for( point &candidate_position : candidate_positions ) {
                                field &target_field =
                                    get_field( tripoint( candidate_position, p.z ) );
                                // Only shift if there are no bees already there.
                                // TODO: Figure out a way to merge bee fields without allowing
                                // Them to effectively move several times in a turn depending
                                // on iteration direction.
                                if( !target_field.find_field( fd_bees ) ) {
                                    add_field( tripoint( candidate_position, p.z ), fd_bees,
                                               cur.get_field_intensity(), cur.get_field_age() );
                                    cur.set_field_intensity( 0 );
                                    break;
                                }
                            }
                        } else {
                            spread_gas( cur, p, 5, 0_turns, sblk );
                        }
                    }
                }
                if( curtype == fd_incendiary ) {
                    // Needed for variable scope
                    tripoint dst( p + point( rng( -1, 1 ), rng( -1, 1 ) ) );
                    if( has_flag( TFLAG_FLAMMABLE, dst ) ||
                        has_flag( TFLAG_FLAMMABLE_ASH, dst ) ||
                        has_flag( TFLAG_FLAMMABLE_HARD, dst ) ) {
                        add_field( dst, fd_fire, 1 );
                    }

                    // Check piles for flammable items and set those on fire
                    if( flammable_items_at( dst ) ) {
                        add_field( dst, fd_fire, 1 );
                    }

                    create_hot_air( p, cur.get_field_intensity() );
                }
                if( curtype == fd_rubble ) {
                    // Legacy Stuff
                    make_rubble( p );
                }
                if( curtype == fd_fungicidal_gas ) {
                    // Check the terrain and replace it accordingly to simulate the fungus dieing off
                    const ter_t &ter = map_tile.get_ter_t();
                    const furn_t &frn = map_tile.get_furn_t();
                    const int intensity = cur.get_field_intensity();
                    if( ter.has_flag( "FUNGUS" ) && one_in( 10 / intensity ) ) {
                        ter_set( p, t_dirt );
                    }
                    if( frn.has_flag( "FUNGUS" ) && one_in( 10 / intensity ) ) {
                        furn_set( p, f_null );
                    }
                }

                cur.set_field_age( cur.get_field_age() + 1_turns );
                auto &fdata = cur.get_field_type().obj();
                if( fdata.half_life > 0_turns && cur.get_field_age() > 0_turns &&
                    dice( 2, to_turns<int>( cur.get_field_age() ) ) > to_turns<int>( fdata.half_life ) ) {
                    cur.set_field_age( 0_turns );
                    cur.set_field_intensity( cur.get_field_intensity() - 1 );
                }
                if( !cur.is_field_alive() ) {
                    --current_submap->field_count;
                    curfield.remove_field( it++ );
                } else {
                    ++it;
                }
            }
        }
    }
    const int minz = zlevels ? -OVERMAP_DEPTH : abs_sub.z;
    const int maxz = zlevels ? OVERMAP_HEIGHT : abs_sub.z;
    for( int z = std::max( submap.z - 1, minz ); z <= std::min( submap.z + 1, maxz ); ++z ) {
        auto &field_cache = get_cache( z ).field_cache;
        for( int y = std::max( submap.y - 1, 0 ); y <= std::min( submap.y + 1, MAPSIZE - 1 ); ++y ) {
            for( int x = std::max( submap.x - 1, 0 ); x <= std::min( submap.x + 1, MAPSIZE - 1 ); ++x ) {
                if( get_submap_at_grid( { x, y, z } )->field_count > 0 ) {
                    field_cache.set( x + y * MAPSIZE );
                } else {
                    field_cache.reset( x + y * MAPSIZE );
                }
            }
        }
    }
    sblk.commit_modifications();
    return dirty_transparency_cache;
}

// This entire function makes very little sense. Why are the rules the way they are? Why does walking into some things destroy them but not others?

/*
Function: step_in_field
Triggers any active abilities a field effect would have. Fire burns you, acid melts you, etc.
If you add a field effect that interacts with the player place a case statement in the switch here.
If you wish for a field effect to do something over time (propagate, interact with terrain, etc) place it in process_subfields
*/
void map::player_in_field( player &u )
{
    // A copy of the current field for reference. Do not add fields to it, use map::add_field
    field &curfield = get_field( u.pos() );
    // Are we inside?
    bool inside = false;
    // If we are in a vehicle figure out if we are inside (reduces effects usually)
    // and what part of the vehicle we need to deal with.
    if( u.in_vehicle ) {
        if( const optional_vpart_position vp = veh_at( u.pos() ) ) {
            inside = vp->is_inside();
        }
    }

    // Iterate through all field effects on this tile.
    // Do not remove the field with remove_field, instead set it's intensity to 0. It will be removed
    // later by the field processing, which will also adjust field_count accordingly.
    for( auto &field_list_it : curfield ) {
        field_entry &cur = field_list_it.second;
        if( !cur.is_field_alive() ) {
            continue;
        }

        // Do things based on what field effect we are currently in.
        const field_type_id ft = cur.get_field_type();
        if( ft == fd_web ) {
            // If we are in a web, can't walk in webs or are in a vehicle, get webbed maybe.
            // Moving through multiple webs stacks the effect.
            if( !u.has_trait( trait_id( "WEB_WALKER" ) ) && !u.in_vehicle ) {
                // Between 5 and 15 minus your current web level.
                u.add_effect( effect_webbed, 1_turns, num_bp, true, cur.get_field_intensity() );
                // It is spent.
                cur.set_field_intensity( 0 );
                continue;
                // If you are in a vehicle destroy the web.
                // It should of been destroyed when you ran over it anyway.
            } else if( u.in_vehicle ) {
                cur.set_field_intensity( 0 );
                continue;
            }
        }
        if( ft == fd_acid ) {
            // Assume vehicles block acid damage entirely,
            // you're certainly not standing in it.
            if( !u.in_vehicle && !u.has_trait( trait_id( "ACIDPROOF" ) ) ) {
                int total_damage = 0;
                const int intensity = cur.get_field_intensity();
                // 1-3 at intensity, 1-4 at 2, 1-5 at 3
                total_damage += burn_body_part( u, cur, bp_foot_l, 2 );
                total_damage += burn_body_part( u, cur, bp_foot_r, 2 );
                // 1 dmg at 1 intensity, 1-3 at 2, 1-5 at 3
                total_damage += burn_body_part( u, cur, bp_leg_l, intensity - 1 );
                total_damage += burn_body_part( u, cur, bp_leg_r, intensity - 1 );
                const bool on_ground = u.is_on_ground();
                if( on_ground ) {
                    // Before, it would just break the legs and leave the survivor alone
                    total_damage += burn_body_part( u, cur, bp_hand_l, 2 );
                    total_damage += burn_body_part( u, cur, bp_hand_r, 2 );
                    total_damage += burn_body_part( u, cur, bp_torso, 2 );
                    // Less arms = less ability to keep upright
                    if( ( !u.has_two_arms() && one_in( 4 ) ) || one_in( 2 ) ) {
                        total_damage += burn_body_part( u, cur, bp_arm_l, 1 );
                        total_damage += burn_body_part( u, cur, bp_arm_r, 1 );
                        total_damage += burn_body_part( u, cur, bp_head, 1 );
                    }
                }

                if( on_ground && total_damage > 0 ) {
                    u.add_msg_player_or_npc( m_bad, _( "The acid burns your body!" ),
                                             _( "The acid burns <npcname>s body!" ) );
                } else if( total_damage > 0 ) {
                    u.add_msg_player_or_npc( m_bad, _( "The acid burns your legs and feet!" ),
                                             _( "The acid burns <npcname>s legs and feet!" ) );
                } else if( on_ground ) {
                    u.add_msg_if_player( m_warning, _( "You're lying in a pool of acid" ) );
                } else {
                    u.add_msg_if_player( m_warning, _( "You're standing in a pool of acid" ) );
                }

                u.check_dead_state();
            }
        }
        if( ft == fd_sap ) {
            // Sap causes the player to get sap disease, slowing them down.
            // Sap does nothing to cars.
            if( !u.in_vehicle ) {
                u.add_msg_player_or_npc( m_bad, _( "The sap sticks to you!" ),
                                         _( "The sap sticks to <npcname>!" ) );
                u.add_effect( effect_sap, cur.get_field_intensity() * 2_turns );
                // Use up sap.
                cur.set_field_intensity( cur.get_field_intensity() - 1 );
            }
        }
        if( ft == fd_sludge ) {
            // Sludge is on the ground, but you are above the ground when boarded on a vehicle
            if( !u.in_vehicle ) {
                u.add_msg_if_player( m_bad, _( "The sludge is thick and sticky. You struggle to pull free." ) );
                u.moves -= cur.get_field_intensity() * 300;
                cur.set_field_intensity( 0 );
            }
        }
        if( ft == fd_fire ) {
            // Heatsink or suit prevents ALL fire damage.
            if( !u.has_active_bionic( bionic_id( "bio_heatsink" ) ) && !u.is_wearing( "rm13_armor_on" ) ) {

                // To modify power of a field based on... whatever is relevant for the effect.
                int adjusted_intensity = cur.get_field_intensity();
                // Burn the player. Less so if you are in a car or ON a car.
                if( u.in_vehicle ) {
                    if( inside ) {
                        adjusted_intensity -= 2;
                    } else {
                        adjusted_intensity -= 1;
                    }
                }

                if( adjusted_intensity >= 1 ) {
                    // Burn message by intensity
                    static const std::array<std::string, 4> player_burn_msg = { {
                            translate_marker( "You burn your legs and feet!" ),
                            translate_marker( "You're burning up!" ),
                            translate_marker( "You're set ablaze!" ),
                            translate_marker( "Your whole body is burning!" )
                        }
                    };
                    static const std::array<std::string, 4> npc_burn_msg = { {
                            translate_marker( "<npcname> burns their legs and feet!" ),
                            translate_marker( "<npcname> is burning up!" ),
                            translate_marker( "<npcname> is set ablaze!" ),
                            translate_marker( "<npcname>s whole body is burning!" )
                        }
                    };
                    static const std::array<std::string, 4> player_warn_msg = { {
                            translate_marker( "You're standing in a fire!" ),
                            translate_marker( "You're waist-deep in a fire!" ),
                            translate_marker( "You're surrounded by raging fire!" ),
                            translate_marker( "You're lying in fire!" )
                        }
                    };

                    const int burn_min = adjusted_intensity;
                    const int burn_max = 3 * adjusted_intensity + 3;
                    std::list<body_part> parts_burned;
                    int msg_num = adjusted_intensity - 1;
                    if( !u.is_on_ground() ) {
                        switch( adjusted_intensity ) {
                            case 3:
                                parts_burned.push_back( bp_hand_l );
                                parts_burned.push_back( bp_hand_r );
                                parts_burned.push_back( bp_arm_l );
                                parts_burned.push_back( bp_arm_r );
                            /* fallthrough */
                            case 2:
                                parts_burned.push_back( bp_torso );
                            /* fallthrough */
                            case 1:
                                parts_burned.push_back( bp_foot_l );
                                parts_burned.push_back( bp_foot_r );
                                parts_burned.push_back( bp_leg_l );
                                parts_burned.push_back( bp_leg_r );
                        }
                    } else {
                        // Lying in the fire is BAAAD news, hits every body part.
                        msg_num = 3;
                        parts_burned.assign( all_body_parts.begin(), all_body_parts.end() );
                    }

                    int total_damage = 0;
                    for( body_part part_burned : parts_burned ) {
                        const dealt_damage_instance dealt = u.deal_damage( nullptr, part_burned,
                                                            damage_instance( DT_HEAT, rng( burn_min, burn_max ) ) );
                        total_damage += dealt.type_damage( DT_HEAT );
                    }
                    if( total_damage > 0 ) {
                        u.add_msg_player_or_npc( m_bad, _( player_burn_msg[msg_num] ), _( npc_burn_msg[msg_num] ) );
                    } else {
                        u.add_msg_if_player( m_warning, _( player_warn_msg[msg_num] ) );
                    }
                    u.check_dead_state();
                }
            }

        }
        if( ft == fd_tear_gas ) {
            // Tear gas will both give you teargas disease and/or blind you.
            if( ( cur.get_field_intensity() > 1 || !one_in( 3 ) ) && ( !inside || one_in( 3 ) ) ) {
                u.add_env_effect( effect_teargas, bp_mouth, 5, 20_seconds );
            }
            if( cur.get_field_intensity() > 1 && ( !inside || one_in( 3 ) ) ) {
                u.add_env_effect( effect_blind, bp_eyes, cur.get_field_intensity() * 2, 10_seconds );
            }
        }
        if( ft == fd_relax_gas ) {
            if( ( cur.get_field_intensity() > 1 || !one_in( 3 ) ) && ( !inside || one_in( 3 ) ) ) {
                u.add_env_effect( effect_relax_gas, bp_mouth, cur.get_field_intensity() * 2, 3_turns );
            }
        }
        if( ft == fd_fungal_haze ) {
            if( !u.has_trait( trait_id( "M_IMMUNE" ) ) && ( !inside || one_in( 4 ) ) ) {
                u.add_env_effect( effect_fungus, bp_mouth, 4, 10_minutes, num_bp, true );
                u.add_env_effect( effect_fungus, bp_eyes, 4, 10_minutes, num_bp, true );
            }
        }
        if( ft == fd_dazzling ) {
            if( cur.get_field_intensity() > 1 || one_in( 5 ) ) {
                u.add_env_effect( effect_blind, bp_eyes, 10, 10_turns );
            } else {
                u.add_env_effect( effect_blind, bp_eyes, 2, 2_turns );
            }
        }
        if( ft == fd_toxic_gas ) {
            // Toxic gas at low levels poisons you.
            // Toxic gas at high levels will cause very nasty poison.
            {
                bool inhaled = false;
                if( ( cur.get_field_intensity() == 2 && !inside ) ||
                    ( cur.get_field_intensity() == 3 && inside ) ) {
                    inhaled = u.add_env_effect( effect_poison, bp_mouth, 5, 3_minutes );
                } else if( cur.get_field_intensity() == 3 && !inside ) {
                    inhaled = u.add_env_effect( effect_badpoison, bp_mouth, 5, 3_minutes );
                } else if( cur.get_field_intensity() == 1 && !inside ) {
                    inhaled = u.add_env_effect( effect_poison, bp_mouth, 2, 2_minutes );
                }
                if( inhaled ) {
                    // Player does not know how the npc feels, so no message.
                    u.add_msg_if_player( m_bad, _( "You feel sick from inhaling the %s" ), cur.name() );
                }
            }
        }

        if( cur.extra_radiation_min() > 0 ) {
            // Get irradiated by the nuclear fallout.
            const float rads = rng( cur.extra_radiation_min() + 1,
                                    cur.extra_radiation_max() * ( cur.extra_radiation_max() + 1 ) );
            const bool rad_proof = !u.irradiate( rads );
            // TODO: Reduce damage for rad resistant?
            if( cur.radiation_hurt_damage_min() > 0 && !rad_proof ) {
                u.add_msg_if_player( m_bad, cur.radiation_hurt_message() );
                u.hurtall( rng( cur.radiation_hurt_damage_min(), cur.radiation_hurt_damage_max() ), nullptr );
            }
        }
        if( ft == fd_flame_burst ) {
            // A burst of flame? Only hits the legs and torso.
            if( !inside ) {
                // Fireballs can't touch you inside a car.
                // Heatsink or suit stops fire.
                if( !u.has_active_bionic( bionic_id( "bio_heatsink" ) ) &&
                    !u.is_wearing( "rm13_armor_on" ) ) {
                    u.add_msg_player_or_npc( m_bad, _( "You're torched by flames!" ),
                                             _( "<npcname> is torched by flames!" ) );
                    u.deal_damage( nullptr, bp_leg_l, damage_instance( DT_HEAT, rng( 2, 6 ) ) );
                    u.deal_damage( nullptr, bp_leg_r, damage_instance( DT_HEAT, rng( 2, 6 ) ) );
                    u.deal_damage( nullptr, bp_torso, damage_instance( DT_HEAT, rng( 4, 9 ) ) );
                    u.check_dead_state();
                } else {
                    u.add_msg_player_or_npc( _( "These flames do not burn you." ),
                                             _( "Those flames do not burn <npcname>." ) );
                }
            }
        }
        if( ft == fd_electricity ) {
            // Small universal damage based on intensity, only if not electroproofed.
            if( !u.is_elec_immune() ) {
                int total_damage = 0;
                for( size_t i = 0; i < num_hp_parts; i++ ) {
                    const body_part bp = player::hp_to_bp( static_cast<hp_part>( i ) );
                    const int dmg = rng( 1, cur.get_field_intensity() );
                    total_damage += u.deal_damage( nullptr, bp, damage_instance( DT_ELECTRIC, dmg ) ).total_damage();
                }

                if( total_damage > 0 ) {
                    if( u.has_trait( trait_ELECTRORECEPTORS ) ) {
                        u.add_msg_player_or_npc( m_bad, _( "You're painfully electrocuted!" ),
                                                 _( "<npcname> is shocked!" ) );
                        u.mod_pain( total_damage / 2 );
                    } else {
                        u.add_msg_player_or_npc( m_bad, _( "You're shocked!" ), _( "<npcname> is shocked!" ) );
                    }
                } else {
                    u.add_msg_player_or_npc( _( "The electric cloud doesn't affect you." ),
                                             _( "The electric cloud doesn't seem to affect <npcname>." ) );
                }
            }
        }
        if( ft == fd_fatigue ) {
            // Teleports you... somewhere.
            if( rng( 0, 2 ) < cur.get_field_intensity() && u.is_player() ) {
                add_msg( m_bad, _( "You're violently teleported!" ) );
                u.hurtall( cur.get_field_intensity(), nullptr );
                teleport::teleport( u );
            }
        }
        // Why do these get removed???
        // Stepping on a shock vent shuts it down.
        if( ft == fd_shock_vent || ft == fd_acid_vent ) {
            cur.set_field_intensity( 0 );
        }
        if( ft == fd_bees ) {
            // Player is immune to bees while underwater.
            if( !u.is_underwater() ) {
                int times_stung = 0;
                const int intensity = cur.get_field_intensity();
                // If the bees can get at you, they cause steadily increasing pain.
                // TODO: Specific stinging messages.
                times_stung += one_in( 4 ) &&
                               u.add_env_effect( effect_stung, bp_torso, intensity, 9_minutes );
                times_stung += one_in( 4 ) &&
                               u.add_env_effect( effect_stung, bp_torso, intensity, 9_minutes );
                times_stung += one_in( 4 ) &&
                               u.add_env_effect( effect_stung, bp_torso, intensity, 9_minutes );
                times_stung += one_in( 4 ) &&
                               u.add_env_effect( effect_stung, bp_torso, intensity, 9_minutes );
                times_stung += one_in( 4 ) &&
                               u.add_env_effect( effect_stung, bp_torso, intensity, 9_minutes );
                times_stung += one_in( 4 ) &&
                               u.add_env_effect( effect_stung, bp_torso, intensity, 9_minutes );
                times_stung += one_in( 4 ) &&
                               u.add_env_effect( effect_stung, bp_torso, intensity, 9_minutes );
                times_stung += one_in( 4 ) &&
                               u.add_env_effect( effect_stung, bp_torso, intensity, 9_minutes );
                switch( times_stung ) {
                    case 0:
                        // Woo, unscathed!
                        break;
                    case 1:
                        u.add_msg_if_player( m_bad, _( "The bees sting you!" ) );
                        break;
                    case 2:
                    case 3:
                        u.add_msg_if_player( m_bad, _( "The bees sting you several times!" ) );
                        break;
                    case 4:
                    case 5:
                        u.add_msg_if_player( m_bad, _( "The bees sting you many times!" ) );
                        break;
                    case 6:
                    case 7:
                    case 8:
                    default:
                        u.add_msg_if_player( m_bad, _( "The bees sting you all over your body!" ) );
                        break;
                }
            }
        }
        if( ft == fd_incendiary ) {
            // Mysterious incendiary substance melts you horribly.
            if( u.has_trait( trait_M_SKIN2 ) ||
                u.has_trait( trait_M_SKIN3 ) ||
                cur.get_field_intensity() == 1 ) {
                u.add_msg_player_or_npc( m_bad, _( "The incendiary burns you!" ),
                                         _( "The incendiary burns <npcname>!" ) );
                u.hurtall( rng( 1, 3 ), nullptr );
            } else {
                u.add_msg_player_or_npc( m_bad, _( "The incendiary melts into your skin!" ),
                                         _( "The incendiary melts into <npcname>s skin!" ) );
                u.add_effect( effect_onfire, 8_turns, bp_torso );
                u.hurtall( rng( 2, 6 ), nullptr );
            }
        }
        // Both gases are unhealthy and become deadly if you cross a related threshold.
        if( ft == fd_fungicidal_gas || ft == fd_insecticidal_gas ) {
            // The gas won't harm you inside a vehicle.
            if( !inside ) {
                // Full body suits protect you from the effects of the gas.
                if( !( u.worn_with_flag( "GAS_PROOF" ) && u.get_env_resist( bp_mouth ) >= 15 &&
                       u.get_env_resist( bp_eyes ) >= 15 ) ) {
                    const int intensity = cur.get_field_intensity();
                    bool inhaled = u.add_env_effect( effect_poison, bp_mouth, 5, intensity * 1_minutes );
                    if( u.has_trait( trait_id( "THRESH_MYCUS" ) ) || u.has_trait( trait_id( "THRESH_MARLOSS" ) ) ||
                        ( ft == fd_insecticidal_gas && ( u.get_highest_category() == "INSECT" ||
                                                         u.get_highest_category() == "SPIDER" ) ) ) {
                        inhaled |= u.add_env_effect( effect_badpoison, bp_mouth, 5, intensity * 1_minutes );
                        u.hurtall( rng( intensity, intensity * 2 ), nullptr );
                        u.add_msg_if_player( m_bad, _( "The %s burns your skin." ), cur.name() );
                    }

                    if( inhaled ) {
                        u.add_msg_if_player( m_bad, _( "The %s makes you feel sick." ), cur.name() );
                    }
                }
            }
        }
    }
}

void map::creature_in_field( Creature &critter )
{
    if( critter.is_monster() ) {
        monster_in_field( *static_cast<monster *>( &critter ) );
    } else if( player *p = critter.as_player() ) {
        player_in_field( *p );
    }

    field &curfield = get_field( critter.pos() );
    for( auto &field_entry_it : curfield ) {
        field_entry &cur_field_entry = field_entry_it.second;
        if( !cur_field_entry.is_field_alive() ) {
            continue;
        }
        const field_type_id cur_field_id = cur_field_entry.get_field_type();
        const effect field_fx = cur_field_entry.field_effect();
        if( field_fx.is_null() ||
            critter.is_immune_field( cur_field_id ) || field_fx.get_id().is_empty() ||
            critter.is_immune_effect( field_fx.get_id() ) ) {
            continue;
        }
        player *u = critter.as_player();
        if( u && cur_field_entry.inside_immune() ) {
            // If we are in a vehicle figure out if we are inside (reduces effects usually)
            // and what part of the vehicle we need to deal with.
            if( u->in_vehicle ) {
                if( const optional_vpart_position vp = veh_at( u->pos() ) ) {
                    if( vp->is_inside() ) {
                        continue;
                    }
                }
            }
        }
        critter.add_effect( field_fx );
    }
}

void map::monster_in_field( monster &z )
{
    if( z.digging() ) {
        // Digging monsters are immune to fields
        return;
    }
    field &curfield = get_field( z.pos() );

    int dam = 0;
    // Iterate through all field effects on this tile.
    // Do not remove the field with remove_field, instead set it's intensity to 0. It will be removed
    // later by the field processing, which will also adjust field_count accordingly.
    for( auto &field_list_it : curfield ) {
        field_entry &cur = field_list_it.second;
        if( !cur.is_field_alive() ) {
            continue;
        }
        const field_type_id cur_field_type = cur.get_field_type();
        if( cur_field_type == fd_web ) {
            if( !z.has_flag( MF_WEBWALK ) ) {
                z.add_effect( effect_webbed, 1_turns, num_bp, true, cur.get_field_intensity() );
                cur.set_field_intensity( 0 );
            }
        }
        if( cur_field_type == fd_acid ) {
            if( !z.has_flag( MF_FLIES ) ) {
                const int d = rng( cur.get_field_intensity(), cur.get_field_intensity() * 3 );
                z.deal_damage( nullptr, bp_torso, damage_instance( DT_ACID, d ) );
                z.check_dead_state();
            }

        }
        if( cur_field_type == fd_sap ) {
            z.moves -= cur.get_field_intensity() * 5;
            cur.set_field_intensity( cur.get_field_intensity() - 1 );
        }
        if( cur_field_type == fd_sludge ) {
            if( !z.has_flag( MF_DIGS ) && !z.has_flag( MF_FLIES ) &&
                !z.has_flag( MF_SLUDGEPROOF ) ) {
                z.moves -= cur.get_field_intensity() * 300;
                cur.set_field_intensity( 0 );
            }
        }
        if( cur_field_type == fd_fire ) {
            // TODO: MATERIALS Use fire resistance
            if( z.has_flag( MF_FIREPROOF ) || z.has_flag( MF_FIREY ) ) {
                return;
            }
            // TODO: Replace the section below with proper json values
            if( z.made_of_any( Creature::cmat_flesh ) ) {
                dam += 3;
            }
            if( z.made_of( material_id( "veggy" ) ) ) {
                dam += 12;
            }
            if( z.made_of( LIQUID ) || z.made_of_any( Creature::cmat_flammable ) ) {
                dam += 20;
            }
            if( z.made_of_any( Creature::cmat_flameres ) ) {
                dam += -20;
            }
            if( z.has_flag( MF_FLIES ) ) {
                dam -= 15;
            }
            dam -= z.get_armor_type( DT_HEAT, bp_torso );

            if( cur.get_field_intensity() == 1 ) {
                dam += rng( 2, 6 );
            } else if( cur.get_field_intensity() == 2 ) {
                dam += rng( 6, 12 );
                if( !z.has_flag( MF_FLIES ) ) {
                    z.moves -= 20;
                    if( dam > 0 ) {
                        z.add_effect( effect_onfire, 1_turns * rng( dam / 2, dam * 2 ) );
                    }
                }
            } else if( cur.get_field_intensity() == 3 ) {
                dam += rng( 10, 20 );
                if( !z.has_flag( MF_FLIES ) || one_in( 3 ) ) {
                    z.moves -= 40;
                    if( dam > 0 ) {
                        z.add_effect( effect_onfire, 1_turns * rng( dam / 2, dam * 2 ) );
                    }
                }
            }
        }
        if( cur_field_type == fd_smoke ) {
            if( !z.has_flag( MF_NO_BREATHE ) ) {
                if( cur.get_field_intensity() == 3 ) {
                    z.moves -= rng( 10, 20 );
                }
                // Plants suffer from smoke even worse
                if( z.made_of( material_id( "veggy" ) ) ) {
                    z.moves -= rng( 1, cur.get_field_intensity() * 12 );
                }
            }

        }
        if( cur_field_type == fd_tear_gas ) {
            if( z.made_of_any( Creature::cmat_fleshnveg ) && !z.has_flag( MF_NO_BREATHE ) ) {
                if( cur.get_field_intensity() == 3 ) {
                    z.add_effect( effect_stunned, rng( 1_minutes, 2_minutes ) );
                    dam += rng( 4, 10 );
                } else if( cur.get_field_intensity() == 2 ) {
                    z.add_effect( effect_stunned, rng( 5_turns, 10_turns ) );
                    dam += rng( 2, 5 );
                } else {
                    z.add_effect( effect_stunned, rng( 1_turns, 5_turns ) );
                }
                if( z.made_of( material_id( "veggy" ) ) ) {
                    z.moves -= rng( cur.get_field_intensity() * 5, cur.get_field_intensity() * 12 );
                    dam += cur.get_field_intensity() * rng( 8, 14 );
                }
                if( z.has_flag( MF_SEES ) ) {
                    z.add_effect( effect_blind, cur.get_field_intensity() * 8_turns );
                }
            }

        }
        if( cur_field_type == fd_relax_gas ) {
            if( z.made_of_any( Creature::cmat_fleshnveg ) && !z.has_flag( MF_NO_BREATHE ) ) {
                z.add_effect( effect_stunned, rng( cur.get_field_intensity() * 4_turns,
                                                   cur.get_field_intensity() * 8_turns ) );
            }
        }
        if( cur_field_type == fd_dazzling ) {
            if( z.has_flag( MF_SEES ) && !z.has_flag( MF_ELECTRONIC ) ) {
                z.add_effect( effect_blind, cur.get_field_intensity() * 12_turns );
                z.add_effect( effect_stunned, cur.get_field_intensity() * rng( 5_turns, 12_turns ) );
            }

        }
        if( cur_field_type == fd_toxic_gas ) {
            if( !z.has_flag( MF_NO_BREATHE ) ) {
                dam += cur.get_field_intensity();
                z.moves -= cur.get_field_intensity();
            }

        }
        if( cur_field_type == fd_nuke_gas ) {
            if( !z.has_flag( MF_NO_BREATHE ) ) {
                if( cur.get_field_intensity() == 3 ) {
                    z.moves -= rng( 60, 120 );
                    dam += rng( 30, 50 );
                } else if( cur.get_field_intensity() == 2 ) {
                    z.moves -= rng( 20, 50 );
                    dam += rng( 10, 25 );
                } else {
                    z.moves -= rng( 0, 15 );
                    dam += rng( 0, 12 );
                }
                if( z.made_of( material_id( "veggy" ) ) ) {
                    z.moves -= rng( cur.get_field_intensity() * 5, cur.get_field_intensity() * 12 );
                    dam *= cur.get_field_intensity();
                }
            }

        }
        if( cur_field_type == fd_flame_burst ) {
            // TODO: MATERIALS Use fire resistance
            if( z.has_flag( MF_FIREPROOF ) || z.has_flag( MF_FIREY ) ) {
                return;
            }
            if( z.made_of_any( Creature::cmat_flesh ) ) {
                dam += 3;
            }
            if( z.made_of( material_id( "veggy" ) ) ) {
                dam += 12;
            }
            if( z.made_of( LIQUID ) || z.made_of_any( Creature::cmat_flammable ) ) {
                dam += 50;
            }
            if( z.made_of_any( Creature::cmat_flameres ) ) {
                dam += -25;
            }
            dam += rng( 0, 8 );
            z.moves -= 20;
        }
        if( cur_field_type == fd_electricity ) {
            // We don't want to increase dam, but deal a separate hit so that it can apply effects
            z.deal_damage( nullptr, bp_torso,
                           damage_instance( DT_ELECTRIC, rng( 1, cur.get_field_intensity() * 3 ) ) );
        }
        if( cur_field_type == fd_fatigue ) {
            if( rng( 0, 2 ) < cur.get_field_intensity() ) {
                dam += cur.get_field_intensity();
                teleport::teleport( z );
            }
        }
        if( cur_field_type == fd_incendiary ) {
            // TODO: MATERIALS Use fire resistance
            if( z.has_flag( MF_FIREPROOF ) || z.has_flag( MF_FIREY ) ) {
                return;
            }
            if( z.made_of_any( Creature::cmat_flesh ) ) {
                dam += 3;
            }
            if( z.made_of( material_id( "veggy" ) ) ) {
                dam += 12;
            }
            if( z.made_of( LIQUID ) || z.made_of_any( Creature::cmat_flammable ) ) {
                dam += 20;
            }
            if( z.made_of_any( Creature::cmat_flameres ) ) {
                dam += -5;
            }

            if( cur.get_field_intensity() == 1 ) {
                dam += rng( 2, 6 );
            } else if( cur.get_field_intensity() == 2 ) {
                dam += rng( 6, 12 );
                z.moves -= 20;
                if( !z.made_of( LIQUID ) && !z.made_of_any( Creature::cmat_flameres ) ) {
                    z.add_effect( effect_onfire, rng( 8_turns, 12_turns ) );
                }
            } else if( cur.get_field_intensity() == 3 ) {
                dam += rng( 10, 20 );
                z.moves -= 40;
                if( !z.made_of( LIQUID ) && !z.made_of_any( Creature::cmat_flameres ) ) {
                    z.add_effect( effect_onfire, rng( 12_turns, 16_turns ) );
                }
            }
        }
        if( cur_field_type == fd_fungal_haze ) {
            if( !z.type->in_species( FUNGUS ) &&
                !z.type->has_flag( MF_NO_BREATHE ) &&
                !z.make_fungus() ) {
                // Don't insta-kill jabberwocks, that's silly
                const int intensity = cur.get_field_intensity();
                z.moves -= rng( 10 * intensity, 30 * intensity );
                dam += rng( 0, 10 * intensity );
            }
        }
        if( cur_field_type == fd_fungicidal_gas ) {
            if( z.type->in_species( FUNGUS ) ) {
                const int intensity = cur.get_field_intensity();
                z.moves -= rng( 10 * intensity, 30 * intensity );
                dam += rng( 4, 7 * intensity );
            }
        }
        if( cur_field_type == fd_insecticidal_gas ) {
            if( z.type->in_species( INSECT ) || z.type->in_species( SPIDER ) ) {
                const int intensity = cur.get_field_intensity();
                z.moves -= rng( 10 * intensity, 30 * intensity );
                dam += rng( 4, 7 * intensity );
            }
        }
    }

    if( dam > 0 ) {
        z.apply_damage( nullptr, bp_torso, dam, true );
        z.check_dead_state();
    }
}

std::tuple<maptile, maptile, maptile> map::get_wind_blockers( const int &winddirection,
        const tripoint &pos )
{
    static const std::array<std::pair<int, std::tuple< point, point, point >>, 9> outputs = {{
            { 330, std::make_tuple( point_east, point_north_east, point_south_east ) },
            { 301, std::make_tuple( point_south_east, point_east, point_south ) },
            { 240, std::make_tuple( point_south, point_south_west, point_south_east ) },
            { 211, std::make_tuple( point_south_west, point_west, point_south ) },
            { 150, std::make_tuple( point_west, point_north_west, point_south_west ) },
            { 121, std::make_tuple( point_north_west, point_north, point_west ) },
            { 60, std::make_tuple( point_north, point_north_west, point_north_east ) },
            { 31, std::make_tuple( point_north_east, point_east, point_north ) },
            { 0, std::make_tuple( point_east, point_north_east, point_south_east ) }
        }
    };

    tripoint removepoint;
    tripoint removepoint2;
    tripoint removepoint3;
    for( const std::pair<int, std::tuple< point, point, point >> &val : outputs ) {
        if( winddirection >= val.first ) {
            removepoint = pos + std::get<0>( val.second );
            removepoint2 = pos + std::get<1>( val.second );
            removepoint3 = pos + std::get<2>( val.second );
            break;
        }
    }

    const maptile remove_tile = maptile_at( removepoint );
    const maptile remove_tile2 = maptile_at( removepoint2 );
    const maptile remove_tile3 = maptile_at( removepoint3 );
    return std::make_tuple( remove_tile, remove_tile2, remove_tile3 );
}

void map::emit_field( const tripoint &pos, const emit_id &src, float mul )
{
    if( !src.is_valid() ) {
        return;
    }

    const float chance = src->chance() * mul;
    if( src.is_valid() &&  x_in_y( chance, 100 ) ) {
        const int qty = chance > 100.0f ? roll_remainder( src->qty() * chance / 100.0f ) : src->qty();
        propagate_field( pos, src->field(), qty, src->intensity() );
    }
}

void map::propagate_field( const tripoint &center, const field_type_id type, int amount,
                           int max_intensity )
{
    using gas_blast = std::pair<float, tripoint>;
    std::priority_queue<gas_blast, std::vector<gas_blast>, pair_greater_cmp_first> open;
    std::set<tripoint> closed;
    open.push( { 0.0f, center } );

    const bool not_gas = type.obj().phase != GAS;

    while( amount > 0 && !open.empty() ) {
        if( closed.count( open.top().second ) ) {
            open.pop();
            continue;
        }

        // All points with equal gas intensity should propagate at the same time
        std::list<gas_blast> gas_front;
        gas_front.push_back( open.top() );
        const int cur_intensity = get_field_intensity( open.top().second, type );
        open.pop();
        while( !open.empty() && get_field_intensity( open.top().second, type ) == cur_intensity ) {
            if( closed.count( open.top().second ) == 0 ) {
                gas_front.push_back( open.top() );
            }

            open.pop();
        }

        int increment = std::max<int>( 1, amount / gas_front.size() );

        while( amount > 0 && !gas_front.empty() ) {
            gas_blast gp = random_entry_removed( gas_front );
            closed.insert( gp.second );
            const int cur_intensity = get_field_intensity( gp.second, type );
            if( cur_intensity < max_intensity ) {
                const int bonus = std::min( max_intensity - cur_intensity, increment );
                mod_field_intensity( gp.second, type, bonus );
                amount -= bonus;
            } else {
                amount--;
            }

            if( amount <= 0 ) {
                return;
            }

            static const std::array<int, 8> x_offset = {{ -1, 1,  0, 0,  1, -1, -1, 1  }};
            static const std::array<int, 8> y_offset = {{  0, 0, -1, 1, -1,  1, -1, 1  }};
            for( size_t i = 0; i < 8; i++ ) {
                tripoint pt = gp.second + point( x_offset[ i ], y_offset[ i ] );
                if( closed.count( pt ) > 0 ) {
                    continue;
                }

                if( impassable( pt ) && ( not_gas || !has_flag( TFLAG_PERMEABLE, pt ) ) ) {
                    closed.insert( pt );
                    continue;
                }

                open.push( { static_cast<float>( rl_dist( center, pt ) ), pt } );
            }
        }
    }
}
