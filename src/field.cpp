#include "field.h"

#include <algorithm>
#include <queue>

#include "calendar.h"
#include "cata_utility.h"
#include "debug.h"
#include "emit.h"
#include "enums.h"
#include "fire.h"
#include "fungal_effects.h"
#include "game.h"
#include "itype.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "material.h"
#include "messages.h"
#include "monster.h"
#include "mtype.h"
#include "npc.h"
#include "output.h"
#include "rng.h"
#include "scent_map.h"
#include "submap.h"
#include "translations.h"
#include "vehicle.h"
#include "vpart_position.h"

const species_id FUNGUS( "FUNGUS" );

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

/** ID, {name}, symbol, priority, {color}, {transparency}, {dangerous}, half-life, {move_cost}, phase_id (of matter), accelerated_decay (decay outside of reality bubble) **/
const std::array<field_t, num_fields> fieldlist = { {
        {
            "fd_null",
            {"", "", ""}, '%', 0,
            {def_c_white, def_c_white, def_c_white}, {true, true, true}, {false, false, false}, 0_turns,
            {0, 0, 0},
            PNULL,
            false
        },
        {
            "fd_blood",
            {translate_marker( "blood splatter" ), translate_marker( "blood stain" ), translate_marker( "puddle of blood" )}, '%', 0,
            {def_c_red, def_c_red, def_c_red}, {true, true, true}, {false, false, false}, 2_days,
            {0, 0, 0},
            LIQUID,
            true
        },
        {
            "fd_bile",
            {translate_marker( "bile splatter" ), translate_marker( "bile stain" ), translate_marker( "puddle of bile" )}, '%', 0,
            {def_c_pink, def_c_pink, def_c_pink}, {true, true, true}, {false, false, false}, 1_days,
            {0, 0, 0},
            LIQUID,
            true
        },

        {
            "fd_gibs_flesh",
            {translate_marker( "scraps of flesh" ), translate_marker( "bloody meat chunks" ), translate_marker( "heap of gore" )}, '~', 0,
            {def_c_brown, def_c_light_red, def_c_red}, {true, true, true}, {false, false, false}, 2_days,
            {0, 0, 0},
            SOLID,
            true
        },

        {
            "fd_gibs_veggy",
            {translate_marker( "shredded leaves and twigs" ), translate_marker( "shattered branches and leaves" ), translate_marker( "broken vegetation tangle" )}, '~', 0,
            {def_c_light_green, def_c_light_green, def_c_green}, {true, true, true}, {false, false, false}, 2_days,
            {0, 0, 0},
            SOLID,
            true
        },

        {
            "fd_web",
            {translate_marker( "cobwebs" ), translate_marker( "webs" ), translate_marker( "thick webs" )}, '}', 2,
            {def_c_white, def_c_white, def_c_white}, {true, true, false}, {true, true, true}, 0_turns,
            {0, 0, 0},
            SOLID,
            false
        },

        {
            "fd_slime",
            {translate_marker( "slime trail" ), translate_marker( "slime stain" ), translate_marker( "puddle of slime" )}, '%', 0,
            {def_c_light_green, def_c_light_green, def_c_green}, {true, true, true}, {false, false, false}, 1_days,
            {0, 0, 0},
            LIQUID,
            true
        },

        {
            "fd_acid",
            {translate_marker( "acid splatter" ), translate_marker( "acid streak" ), translate_marker( "pool of acid" )}, '5', 2,
            {def_c_light_green, def_c_green, def_c_green}, {true, true, true}, {true, true, true}, 2_minutes,
            {0, 0, 0},
            LIQUID,
            false
        },

        {
            "fd_sap",
            {translate_marker( "sap splatter" ), translate_marker( "glob of sap" ), translate_marker( "pool of sap" )}, '5', 2,
            {def_c_yellow, def_c_brown, def_c_brown}, {true, true, true}, {true, true, true}, 2_minutes,
            {0, 0, 0},
            LIQUID,
            false
        },

        {
            "fd_sludge",
            {translate_marker( "thin sludge trail" ), translate_marker( "sludge trail" ), translate_marker( "thick sludge trail" )}, '5', 2,
            {def_c_light_gray, def_c_dark_gray, def_c_dark_gray}, {true, true, true}, {true, true, true}, 6_hours,
            {0, 0, 0},
            LIQUID,
            false
        },

        {
            "fd_fire",
            {translate_marker( "small fire" ), translate_marker( "fire" ), translate_marker( "raging fire" )}, '4', 4,
            {def_c_yellow, def_c_light_red, def_c_red}, {true, true, true}, {true, true, true}, 30_minutes,
            {0, 0, 0},
            PLASMA,
            false
        },

        {
            "fd_rubble",
            {translate_marker( "legacy rubble" ), translate_marker( "legacy rubble" ), translate_marker( "legacy rubble" )}, '#', 0,
            {def_c_dark_gray, def_c_dark_gray, def_c_dark_gray}, {true, true, true}, {false, false, false}, 1_turns,
            {0, 0, 0},
            SOLID,
            false
        },

        {
            "fd_smoke",
            {translate_marker( "thin smoke" ), translate_marker( "smoke" ), translate_marker( "thick smoke" )}, '8', 8,
            {def_c_white, def_c_light_gray, def_c_dark_gray}, {true, false, false}, {true, true, true}, 2_minutes,
            {0, 0, 0},
            GAS,
            true
        },
        {
            "fd_toxic_gas",
            {translate_marker( "hazy cloud" ), translate_marker( "toxic gas" ), translate_marker( "thick toxic gas" )}, '8', 8,
            {def_c_white, def_c_light_green, def_c_green}, {true, false, false}, {true, true, true}, 10_minutes,
            {0, 0, 0},
            GAS,
            false
        },

        {
            "fd_tear_gas",
            {translate_marker( "hazy cloud" ), translate_marker( "tear gas" ), translate_marker( "thick tear gas" )}, '8', 8,
            {def_c_white, def_c_yellow, def_c_brown}, {true, false, false}, {true, true, true}, 5_minutes,
            {0, 0, 0},
            GAS,
            false
        },

        {
            "fd_nuke_gas",
            {translate_marker( "hazy cloud" ), translate_marker( "radioactive gas" ), translate_marker( "thick radioactive gas" )}, '8', 8,
            {def_c_white, def_c_light_green, def_c_green}, {true, true, false}, {true, true, true}, 100_minutes,
            {0, 0, 0},
            GAS,
            false
        },

        {
            "fd_gas_vent",
            {translate_marker( "gas vent" ), translate_marker( "gas vent" ), translate_marker( "gas vent" )}, '%', 0,
            {def_c_white, def_c_white, def_c_white}, {true, true, true}, {false, false, false}, 0_turns,
            {0, 0, 0},
            GAS,
            false
        },

        {
            // Fire Vents
            "fd_fire_vent",
            {"", "", ""}, '&', -1,
            {def_c_white, def_c_white, def_c_white}, {true, true, true}, {false, false, false}, 0_turns,
            {0, 0, 0},
            GAS,
            false
        },

        {
            "fd_flame_burst",
            {translate_marker( "fire" ), translate_marker( "fire" ), translate_marker( "fire" )}, '5', 4,
            {def_c_red, def_c_red, def_c_red}, {true, true, true}, {true, true, true}, 0_turns,
            {0, 0, 0},
            GAS,
            false
        },

        {
            "fd_electricity",
            {translate_marker( "sparks" ), translate_marker( "electric crackle" ), translate_marker( "electric cloud" )}, '9', 4,
            {def_c_white, def_c_cyan, def_c_blue}, {true, true, true}, {true, true, true}, 2_turns,
            {0, 0, 0},
            PLASMA,
            false
        },

        {
            "fd_fatigue",
            {translate_marker( "odd ripple" ), translate_marker( "swirling air" ), translate_marker( "tear in reality" )}, '*', 8,
            {def_c_light_gray, def_c_dark_gray, def_c_magenta}, {true, true, false}, {true, true, true}, 0_turns,
            {0, 0, 0},
            PNULL,
            false
        },

        {
            //Push Items
            "fd_push_items",
            {"", "", ""}, '&', -1,
            {def_c_white, def_c_white, def_c_white}, {true, true, true}, {false, false, false}, 0_turns,
            {0, 0, 0},
            PNULL,
            false
        },

        {
            // shock vents
            "fd_shock_vent",
            {"", "", ""}, '&', -1,
            {def_c_white, def_c_white, def_c_white}, {true, true, true}, {false, false, false}, 0_turns,
            {0, 0, 0},
            PLASMA,
            false
        },

        {
            // acid vents
            "fd_acid_vent",
            {"", "", ""}, '&', -1,
            {def_c_white, def_c_white, def_c_white}, {true, true, true}, {false, false, false}, 0_turns,
            {0, 0, 0},
            LIQUID,
            false
        },

        {
            // plasma glow ( for plasma weapons )
            "fd_plasma",
            {translate_marker( "faint plasma" ), translate_marker( "glowing plasma" ), translate_marker( "glaring plasma" )}, '9', 4,
            {def_c_magenta, def_c_pink, def_c_white}, {true, true, true}, {false, false, false}, 2_turns,
            {0, 0, 0},
            PLASMA,
            false
        },

        {
            // laser beam ( for laser weapons )
            "fd_laser",
            {translate_marker( "faint glimmer" ), translate_marker( "beam of light" ), translate_marker( "intense beam of light" )}, '#', 4,
            {def_c_blue, def_c_light_blue, def_c_white}, {true, true, true}, {false, false, false}, 1_turns,
            {0, 0, 0},
            PLASMA,
            false
        },
        {
            "fd_spotlight",
            { translate_marker( "spotlight" ), translate_marker( "spotlight" ), translate_marker( "spotlight" ) }, '&', 1,
            {def_c_white, def_c_white, def_c_white}, { true, true, true }, { false, false, false }, 1_turns,
            {0, 0, 0},
            PNULL,
            false
        },
        {
            "fd_dazzling",
            { translate_marker( "dazzling" ), translate_marker( "dazzling" ), translate_marker( "dazzling" ) }, '#', 4,
            {def_c_light_red_yellow, def_c_light_red_yellow, def_c_light_red_yellow}, { true, true, true }, { false, false, false }, 1_turns,
            { 0, 0, 0 },
            PLASMA,
            false
        },
        {
            "fd_blood_veggy",
            {translate_marker( "plant sap splatter" ), translate_marker( "plant sap stain" ), translate_marker( "puddle of resin" )}, '%', 0,
            {def_c_light_green, def_c_light_green, def_c_light_green}, {true, true, true}, {false, false, false}, 2_days,
            {0, 0, 0},
            LIQUID,
            true
        },
        {
            "fd_blood_insect",
            {translate_marker( "bug blood splatter" ), translate_marker( "bug blood stain" ), translate_marker( "puddle of bug blood" )}, '%', 0,
            {def_c_green, def_c_green, def_c_green}, {true, true, true}, {false, false, false}, 2_days,
            {0, 0, 0},
            LIQUID,
            true
        },
        {
            "fd_blood_invertebrate",
            {translate_marker( "hemolymph splatter" ), translate_marker( "hemolymph stain" ), translate_marker( "puddle of hemolymph" )}, '%', 0,
            {def_c_light_gray, def_c_light_gray, def_c_light_gray}, {true, true, true}, {false, false, false}, 2_days,
            {0, 0, 0},
            LIQUID,
            true
        },
        {
            "fd_gibs_insect",
            {translate_marker( "shards of chitin" ), translate_marker( "shattered bug leg" ), translate_marker( "torn insect organs" )}, '~', 0,
            {def_c_light_green, def_c_green, def_c_yellow}, {true, true, true}, {false, false, false}, 2_days,
            {0, 0, 0},
            SOLID,
            true
        },
        {
            "fd_gibs_invertebrate",
            {translate_marker( "gooey scraps" ), translate_marker( "icky mess" ), translate_marker( "heap of squishy gore" )}, '~', 0,
            {def_c_light_gray, def_c_light_gray, def_c_dark_gray}, {true, true, true}, {false, false, false}, 2_days,
            {0, 0, 0},
            SOLID,
            true
        },
        {
            "fd_cigsmoke",
            {translate_marker( "swirl of tobacco smoke" ), translate_marker( "tobacco smoke" ), translate_marker( "thick tobacco smoke" )}, '%', 8,
            {def_c_white, def_c_light_gray, def_c_dark_gray}, {true, true, true}, {false, false, false}, 35_minutes,
            {0, 0, 0},
            GAS,
            true
        },
        {
            "fd_weedsmoke",
            {translate_marker( "swirl of pot smoke" ), translate_marker( "pot smoke" ), translate_marker( "thick pot smoke" )}, '%', 8,
            {def_c_white, def_c_light_gray, def_c_dark_gray}, {true, true, true}, {false, false, false}, 325_turns,
            {0, 0, 0},
            GAS,
            true
        },

        {
            "fd_cracksmoke",
            {translate_marker( "swirl of crack smoke" ), translate_marker( "crack smoke" ), translate_marker( "thick crack smoke" )}, '%', 8,
            {def_c_white, def_c_light_gray, def_c_dark_gray}, {true, true, true}, {false, false, false}, 225_turns,
            {0, 0, 0},
            GAS,
            true
        },
        {
            "fd_methsmoke",
            {translate_marker( "swirl of meth smoke" ), translate_marker( "meth smoke" ), translate_marker( "thick meth smoke" )}, '%', 8,
            {def_c_white, def_c_light_gray, def_c_dark_gray}, {true, true, true}, {false, false, false}, 275_turns,
            {0, 0, 0},
            GAS,
            true
        },
        {
            "fd_bees",
            {translate_marker( "some bees" ), translate_marker( "swarm of bees" ), translate_marker( "angry swarm of bees" )}, '8', 8,
            {def_c_white, def_c_light_gray, def_c_dark_gray}, {true, true, true}, {true, true, true}, 100_minutes,
            {0, 0, 0},
            PNULL,
            false
        },

        {
            "fd_incendiary",
            {translate_marker( "smoke" ), translate_marker( "airborne incendiary" ), translate_marker( "airborne incendiary" )}, '8', 8,
            {def_c_white, def_c_light_red, def_c_light_red_red}, {true, true, false}, {true, true, true}, 50_minutes,
            {0, 0, 0},
            GAS,
            false
        },

        {
            "fd_relax_gas",
            {translate_marker( "hazy cloud" ), translate_marker( "sedative gas" ), translate_marker( "relaxation gas" )}, '.', 8,
            {def_c_white, def_c_pink, def_c_cyan }, { true, true, true }, { true, true, true }, 50_minutes,
            {0, 0, 0},
            GAS,
            false
        },

        {
            "fd_fungal_haze",
            {translate_marker( "hazy cloud" ), translate_marker( "fungal haze" ), translate_marker( "thick fungal haze" )}, '.', 8,
            {def_c_white, def_c_cyan, def_c_cyan }, { true, true, false }, { true, true, true }, 4_minutes,
            {0, 0, 0},
            GAS,
            false
        },

        {
            "fd_hot_air1",
            {"", "", ""}, '&', -1,
            {def_c_white, def_c_yellow, def_c_red}, {true, true, true}, {false, false, false}, 50_minutes,
            {0, 0, 0},
            GAS,
            false
        },

        {
            "fd_hot_air2",
            {"", "", ""}, '&', -1,
            {def_c_white, def_c_yellow, def_c_red}, {true, true, true}, {false, false, false}, 50_minutes,
            {0, 0, 0},
            GAS,
            false
        },

        {
            "fd_hot_air3",
            {"", "", ""}, '&', -1,
            {def_c_white, def_c_yellow, def_c_red}, {true, true, true}, {false, false, false}, 50_minutes,
            {0, 0, 0},
            GAS,
            false
        },

        {
            "fd_hot_air4",
            {"", "", ""}, '&', -1,
            {def_c_white, def_c_yellow, def_c_red}, {true, true, true}, {false, false, false}, 50_minutes,
            {0, 0, 0},
            GAS,
            false
        },

        {
            "fd_fungicidal_gas",
            {translate_marker( "hazy cloud" ), translate_marker( "fungicidal gas" ), translate_marker( "thick fungicidal gas" )}, '8', 8,
            {def_c_white, def_c_light_gray, def_c_dark_gray}, {true, true, false}, {true, true, true}, 90_minutes,
            {0, 0, 0},
            GAS,
            false
        },
        {
            "fd_smoke_vent",
            {translate_marker( "smoke vent" ), translate_marker( "smoke vent" ), translate_marker( "smoke vent" )}, '%', 0,
            {def_c_white, def_c_white, def_c_white}, {true, true, true}, {false, false, false}, 0_turns,
            {0, 0, 0},
            GAS,
            false
        }
    }
};

field_id field_from_ident( const std::string &field_ident )
{
    for( size_t i = 0; i < num_fields; i++ ) {
        if( fieldlist[i].id == field_ident ) {
            return static_cast<field_id>( i );
        }
    }
    debugmsg( "unknown field ident %s", field_ident.c_str() );
    return fd_null;
}

void map::create_burnproducts( const tripoint &p, const item &fuel, const units::mass &burned_mass )
{
    std::vector<material_id> all_mats = fuel.made_of();
    if( all_mats.empty() ) {
        return;
    }
    //Items that are multiple materials are assumed to be equal parts each.
    units::mass by_weight = burned_mass / all_mats.size();
    for( auto &mat : all_mats ) {
        for( auto &bp : mat->burn_products() ) {
            itype_id id = bp.first;
            // Spawning the same item as the one that was just burned is pointless
            // and leads to infinite recursion.
            if( fuel.typeId() == id ) {
                continue;
            }
            float eff = bp.second;
            int n = floor( eff * ( by_weight / item::find_type( id )->weight ) );

            if( n <= 0 ) {
                continue;
            }
            spawn_item( p, id, n, 1, calendar::turn );
        }
    }
}

bool map::process_fields()
{
    bool dirty_transparency_cache = false;
    const int minz = zlevels ? -OVERMAP_DEPTH : abs_sub.z;
    const int maxz = zlevels ? OVERMAP_HEIGHT : abs_sub.z;
    for( int z = minz; z <= maxz; z++ ) {
        bool zlev_dirty = false;
        for( int x = 0; x < my_MAPSIZE; x++ ) {
            for( int y = 0; y < my_MAPSIZE; y++ ) {
                submap *const current_submap = get_submap_at_grid( { x, y, z } );
                if( current_submap->field_count > 0 ) {
                    const bool cur_dirty = process_fields_in_submap( current_submap, x, y, z );
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

int ter_furn_movecost( const ter_t &ter, const furn_t &furn )
{
    if( ter.movecost == 0 ) {
        return 0;
    }

    if( furn.movecost < 0 ) {
        return 0;
    }

    return ter.movecost + furn.movecost;
}

static const std::array<tripoint, 8> eight_horizontal_neighbors = { {
        { -1, -1, 0 },
        {  0, -1, 0 },
        { +1, -1, 0 },
        { -1,  0, 0 },
        { +1,  0, 0 },
        { -1, +1, 0 },
        {  0, +1, 0 },
        { +1, +1, 0 },
    }
};

bool at_edge( const size_t x, const size_t y )
{
    return x == 0 || x == SEEX || y == 0 || y == SEEY;
}

/*
Function: process_fields_in_submap
Iterates over every field on every tile of the given submap given as parameter.
This is the general update function for field effects. This should only be called once per game turn.
If you need to insert a new field behavior per unit time add a case statement in the switch below.
*/
bool map::process_fields_in_submap( submap *const current_submap,
                                    const int submap_x, const int submap_y, const int submap_z )
{
    const auto get_neighbors = [this]( const tripoint & pt ) {
        // Wrapper to allow skipping bound checks except at the edges of the map
        const auto maptile_has_bounds = [this]( const tripoint & pt, const bool bounds_checked ) {
            if( bounds_checked ) {
                // We know that the point is in bounds
                return maptile_at_internal( pt );
            }

            return maptile_at( pt );
        };

        // Find out which edges are in the bubble
        // Where possible, do just one bounds check for all the neighbors
        const bool west = pt.x > 0;
        const bool north = pt.y > 0;
        const bool east = pt.x < SEEX * my_MAPSIZE - 1;
        const bool south = pt.y < SEEY * my_MAPSIZE - 1;
        return std::array< maptile, 8 > { {
                maptile_has_bounds( pt + eight_horizontal_neighbors[0], west &&north ),
                maptile_has_bounds( pt + eight_horizontal_neighbors[1], north ),
                maptile_has_bounds( pt + eight_horizontal_neighbors[2], east &&north ),
                maptile_has_bounds( pt + eight_horizontal_neighbors[3], west ),
                maptile_has_bounds( pt + eight_horizontal_neighbors[4], east ),
                maptile_has_bounds( pt + eight_horizontal_neighbors[5], west &&south ),
                maptile_has_bounds( pt + eight_horizontal_neighbors[6], south ),
                maptile_has_bounds( pt + eight_horizontal_neighbors[7], east &&south ),
            }
        };
    };

    const auto spread_gas = [this, &get_neighbors](
                                field_entry & cur, const tripoint & p, field_id curtype,
    int percent_spread, const time_duration & outdoor_age_speedup ) {
        // Reset nearby scents to zero
        for( const tripoint &tmp : points_in_radius( p, 1 ) ) {
            g->scent.set( tmp, 0 );
        }

        const int current_density = cur.getFieldDensity();
        const time_duration current_age = cur.getFieldAge();
        // Dissipate faster outdoors.
        if( is_outside( p ) ) {
            cur.setFieldAge( current_age + outdoor_age_speedup );
        }

        // Bail out if we don't meet the spread chance or required density.
        if( current_density <= 1 || rng( 1, 100 ) > percent_spread ) {
            return;
        }

        const auto can_spread_to = [&]( const maptile & dst, field_id curtype ) {
            const field_entry *tmpfld = dst.get_field().findField( curtype );
            const auto &ter = dst.get_ter_t();
            const auto &frn = dst.get_furn_t();
            // Candidates are existing weaker fields or navigable/flagged tiles with no field.
            return ( ter_furn_movecost( ter, frn ) > 0 || ter_furn_has_flag( ter, frn, TFLAG_PERMEABLE ) ) &&
                   ( tmpfld == nullptr || tmpfld->getFieldDensity() < cur.getFieldDensity() );
        };

        const auto spread_to = [&]( maptile & dst ) {
            field_entry *candidate_field = dst.find_field( curtype );
            // Nearby gas grows thicker, and ages are shared.
            const time_duration age_fraction = current_age / current_density ;
            if( candidate_field != nullptr ) {
                candidate_field->setFieldDensity( candidate_field->getFieldDensity() + 1 );
                cur.setFieldDensity( current_density - 1 );
                candidate_field->setFieldAge( candidate_field->getFieldAge() + age_fraction );
                cur.setFieldAge( current_age - age_fraction );
                // Or, just create a new field.
            } else if( dst.add_field( curtype, 1, 0_turns ) ) {
                dst.find_field( curtype )->setFieldAge( age_fraction );
                cur.setFieldDensity( current_density - 1 );
                cur.setFieldAge( current_age - age_fraction );
            }
        };

        // First check if we can fall
        // TODO: Make fall and rise chances parameters to enable heavy/light gas
        if( zlevels && p.z > -OVERMAP_DEPTH ) {
            tripoint down{p.x, p.y, p.z - 1};
            maptile down_tile = maptile_at_internal( down );
            if( can_spread_to( down_tile, curtype ) && valid_move( p, down, true, true ) ) {
                spread_to( down_tile );
                return;
            }
        }

        auto neighs = get_neighbors( p );
        const size_t end_it = static_cast<size_t>( rng( 0, neighs.size() - 1 ) );
        std::vector<size_t> spread;
        spread.reserve( 8 );
        // Start at end_it + 1, then wrap around until i == end_it
        for( size_t i = ( end_it + 1 ) % neighs.size() ;
             i != end_it;
             i = ( i + 1 ) % neighs.size() ) {
            const auto &neigh = neighs[i];
            if( can_spread_to( neigh, curtype ) ) {
                spread.push_back( i );
            }
        }

        // Then, spread to a nearby point.
        // If not possible (or randomly), try to spread up
        if( !spread.empty() && ( !zlevels || one_in( spread.size() ) ) ) {
            // Construct the destination from offset and p
            spread_to( neighs[ random_entry( spread ) ] );
        } else if( zlevels && p.z < OVERMAP_HEIGHT ) {
            tripoint up{p.x, p.y, p.z + 1};
            maptile up_tile = maptile_at_internal( up );
            if( can_spread_to( up_tile, curtype ) && valid_move( p, up, true, true ) ) {
                spread_to( up_tile );
            }
        }
    };

    /*
    Function: create_hot_air
    Helper function that encapsulates the logic involved in creating hot air.
    */
    const auto create_hot_air = [this]( const tripoint & p, int density ) {
        field_id hot_air;
        switch( density ) {
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
                debugmsg( "Tried to spread hot air with density %d", density );
                return;
        }

        for( int counter = 0; counter < 5; counter++ ) {
            tripoint dst( p.x + rng( -1, 1 ), p.y + rng( -1, 1 ), p.z );
            add_field( dst, hot_air, 1 );
        }
    };

    // This should be true only when the field changes transparency
    // More correctly: not just when the field is opaque, but when it changes state
    // to a more/less transparent one, or creates a non-transparent field nearby
    bool dirty_transparency_cache = false;
    //Holds m.field_at(x,y).findField(fd_some_field) type returns.
    // Just to avoid typing that long string for a temp value.
    field_entry *tmpfld = nullptr;

    tripoint thep;
    thep.z = submap_z;

    // Initialize the map tile wrapper
    maptile map_tile( current_submap, 0, 0 );
    size_t &locx = map_tile.x;
    size_t &locy = map_tile.y;
    //Loop through all tiles in this submap indicated by current_submap
    for( locx = 0; locx < SEEX; locx++ ) {
        for( locy = 0; locy < SEEY; locy++ ) {
            // This is a translation from local coordinates to submap coordinates.
            // All submaps are in one long 1d array.
            thep.x = locx + submap_x * SEEX;
            thep.y = locy + submap_y * SEEY;
            // A const reference to the tripoint above, so that the code below doesn't accidentally change it
            const tripoint &p = thep;
            // Get a reference to the field variable from the submap;
            // contains all the pointers to the real field effects.
            field &curfield = current_submap->fld[locx][locy];
            for( auto it = curfield.begin(); it != curfield.end(); ) {
                //Iterating through all field effects in the submap's field.
                field_entry &cur = it->second;
                // The field might have been killed by processing a neighbor field
                if( !cur.isAlive() ) {
                    if( !fieldlist[cur.getFieldType()].transparent[cur.getFieldDensity() - 1] ) {
                        dirty_transparency_cache = true;
                    }
                    current_submap->field_count--;
                    curfield.removeField( it++ );
                    continue;
                }

                //Holds cur.getFieldType() as that is what the old system used before rewrite.
                field_id curtype = cur.getFieldType();
                // Again, legacy support in the event someone Mods setFieldDensity to allow more values.
                if( cur.getFieldDensity() > 3 || cur.getFieldDensity() < 1 ) {
                    debugmsg( "Whoooooa density of %d", cur.getFieldDensity() );
                }

                // Don't process "newborn" fields. This gives the player time to run if they need to.
                if( cur.getFieldAge() == 0_turns ) {
                    curtype = fd_null;
                }

                int part;
                switch( curtype ) {
                    case fd_null:
                    case num_fields:
                        break;  // Do nothing, obviously.  OBVIOUSLY.

                    case fd_blood:
                    case fd_blood_veggy:
                    case fd_blood_insect:
                    case fd_blood_invertebrate:
                    case fd_bile:
                    case fd_gibs_flesh:
                    case fd_gibs_veggy:
                    case fd_gibs_insect:
                    case fd_gibs_invertebrate:
                        // Dissipate faster in water
                        if( map_tile.get_ter_t().has_flag( TFLAG_SWIMMABLE ) ) {
                            cur.setFieldAge( cur.getFieldAge() + 25_minutes );
                        }
                        break;

                    case fd_acid: {
                        const auto &ter = map_tile.get_ter_t();
                        if( ter.has_flag( TFLAG_SWIMMABLE ) ) { // Dissipate faster in water
                            cur.setFieldAge( cur.getFieldAge() + 2_minutes );
                        }

                        // Try to fall by a z-level
                        if( !zlevels || p.z <= -OVERMAP_DEPTH ) {
                            break;
                        }

                        tripoint dst{p.x, p.y, p.z - 1};
                        if( valid_move( p, dst, true, true ) ) {
                            maptile dst_tile = maptile_at_internal( dst );
                            field_entry *acid_there = dst_tile.find_field( fd_acid );
                            if( acid_there == nullptr ) {
                                dst_tile.add_field( fd_acid, cur.getFieldDensity(), cur.getFieldAge() );
                            } else {
                                // Math can be a bit off,
                                // but "boiling" falling acid can be allowed to be stronger
                                // than acid that just lies there
                                const int sum_density = cur.getFieldDensity() + acid_there->getFieldDensity();
                                const int new_density = std::min( 3, sum_density );
                                // No way to get precise elapsed time, let's always reset
                                // Allow falling acid to last longer than regular acid to show it off
                                const time_duration new_age = -1_minutes * ( sum_density - new_density );
                                acid_there->setFieldDensity( new_density );
                                acid_there->setFieldAge( new_age );
                            }

                            // Set ourselves up for removal
                            cur.setFieldDensity( 0 );
                        }

                        // TODO: Allow spreading to the sides if age < 0 && density == 3
                    }
                    break;

                    // Use the normal aging logic below this switch
                    case fd_web:
                        break;
                    case fd_sap:
                        break;
                    case fd_sludge:
                        break;
                    case fd_slime:
                        if( g->scent.get( p ) < cur.getFieldDensity() * 10 ) {
                            g->scent.set( p, cur.getFieldDensity() * 10 );
                        }
                        break;
                    case fd_plasma:
                        dirty_transparency_cache = true;
                        break;
                    case fd_laser:
                        dirty_transparency_cache = true;
                        break;

                    // TODO-MATERIALS: use fire resistance
                    case fd_fire: {
                        // Entire objects for ter/frn for flags
                        const auto &ter = map_tile.get_ter_t();
                        const auto &frn = map_tile.get_furn_t();

                        // We've got ter/furn cached, so let's use that
                        const bool is_sealed = ter_furn_has_flag( ter, frn, TFLAG_SEALED ) &&
                                               !ter_furn_has_flag( ter, frn, TFLAG_ALLOW_FIELD_EFFECT );
                        // Smoke generation probability, consumed items count
                        int smoke = 0;
                        int consumed = 0;
                        // How much time to add to the fire's life due to burned items/terrain/furniture
                        time_duration time_added = 0_turns;
                        // Checks if the fire can spread
                        // If the flames are in furniture with fire_container flag like brazier or oven,
                        // they're fully contained, so skip consuming terrain
                        const bool can_spread = !ter_furn_has_flag( ter, frn, TFLAG_FIRE_CONTAINER );
                        // The huge indent below should probably be somehow moved away from here
                        // without forcing the function to use i_at( p ) for fires without items
                        if( !is_sealed && map_tile.get_item_count() > 0 ) {
                            auto items_here = i_at( p );
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

                            fire_data frd( cur.getFieldDensity(), !can_spread );
                            // The highest # of items this fire can remove in one turn
                            int max_consume = cur.getFieldDensity() * 2;

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

                        //Get the part of the vehicle in the fire.
                        vehicle *veh = veh_at_internal( p, part ); // _internal skips the boundary check
                        if( veh != nullptr ) {
                            veh->damage( part, cur.getFieldDensity() * 10, DT_HEAT, true );
                            //Damage the vehicle in the fire.
                        }
                        if( can_spread ) {
                            if( ter.has_flag( TFLAG_SWIMMABLE ) ) {
                                // Flames die quickly on water
                                cur.setFieldAge( cur.getFieldAge() + 4_minutes );
                            }

                            // Consume the terrain we're on
                            if( ter_furn_has_flag( ter, frn, TFLAG_FLAMMABLE ) ) {
                                // The fire feeds on the ground itself until max density.
                                time_added += 1_turns * ( 5 - cur.getFieldDensity() );
                                smoke += 2;
                                if( cur.getFieldDensity() > 1 &&
                                    one_in( 200 - cur.getFieldDensity() * 50 ) ) {
                                    destroy( p, false );
                                }

                            } else if( ter_furn_has_flag( ter, frn, TFLAG_FLAMMABLE_HARD ) &&
                                       one_in( 3 ) ) {
                                // The fire feeds on the ground itself until max density.
                                time_added += 1_turns * ( 4 - cur.getFieldDensity() );
                                smoke += 2;
                                if( cur.getFieldDensity() > 1 &&
                                    one_in( 200 - cur.getFieldDensity() * 50 ) ) {
                                    destroy( p, false );
                                }

                            } else if( ter.has_flag( TFLAG_FLAMMABLE_ASH ) ) {
                                // The fire feeds on the ground itself until max density.
                                time_added += 1_turns * ( 5 - cur.getFieldDensity() );
                                smoke += 2;
                                if( cur.getFieldDensity() > 1 &&
                                    one_in( 200 - cur.getFieldDensity() * 50 ) ) {
                                    ter_set( p, t_dirt );
                                }

                            } else if( frn.has_flag( TFLAG_FLAMMABLE_ASH ) ) {
                                // The fire feeds on the ground itself until max density.
                                time_added += 1_turns * ( 5 - cur.getFieldDensity() );
                                smoke += 2;
                                if( cur.getFieldDensity() > 1 &&
                                    one_in( 200 - cur.getFieldDensity() * 50 ) ) {
                                    furn_set( p, f_ash );
                                    add_item_or_charges( p, item( "ash" ) );
                                }

                            } else if( ter.has_flag( TFLAG_NO_FLOOR ) && zlevels && p.z > -OVERMAP_DEPTH ) {
                                // We're hanging in the air - let's fall down
                                tripoint dst{p.x, p.y, p.z - 1};
                                if( valid_move( p, dst, true, true ) ) {
                                    maptile dst_tile = maptile_at_internal( dst );
                                    field_entry *fire_there = dst_tile.find_field( fd_fire );
                                    if( fire_there == nullptr ) {
                                        dst_tile.add_field( fd_fire, 1, 0_turns );
                                        cur.setFieldDensity( cur.getFieldDensity() - 1 );
                                    } else {
                                        // Don't fuel raging fires or they'll burn forever
                                        // as they can produce small fires above themselves
                                        int new_density = std::max( cur.getFieldDensity(),
                                                                    fire_there->getFieldDensity() );
                                        // Allow smaller fires to combine
                                        if( new_density < 3 &&
                                            cur.getFieldDensity() == fire_there->getFieldDensity() ) {
                                            new_density++;
                                        }
                                        fire_there->setFieldDensity( new_density );
                                        // A raging fire below us can support us for a while
                                        // Otherwise decay and decay fast
                                        if( new_density < 3 || one_in( 10 ) ) {
                                            cur.setFieldDensity( cur.getFieldDensity() - 1 );
                                        }
                                    }

                                    break;
                                }
                            }
                        }

                        // Lower age is a longer lasting fire
                        if( time_added != 0_turns ) {
                            cur.setFieldAge( cur.getFieldAge() - time_added );
                        } else if( can_spread || !ter_furn_has_flag( ter, frn, TFLAG_FIRE_CONTAINER ) ) {
                            // Nothing to burn = fire should be dying out faster
                            // Drain more power from big fires, so that they stop raging over nothing
                            // Except for fires on stoves and fireplaces, those are made to keep the fire alive
                            cur.setFieldAge( cur.getFieldAge() + 2_turns * cur.getFieldDensity() );
                        }

                        // Below we will access our nearest 8 neighbors, so let's cache them now
                        // This should probably be done more globally, because large fires will re-do it a lot
                        auto neighs = get_neighbors( p );

                        // If the flames are in a pit, it can't spread to non-pit
                        const bool in_pit = ter.id.id() == t_pit;

                        // Count adjacent fires, to optimize out needless smoke and hot air
                        int adjacent_fires = 0;

                        // If the flames are big, they contribute to adjacent flames
                        if( can_spread ) {
                            if( cur.getFieldDensity() > 1 && one_in( 3 ) ) {
                                // Basically: Scan around for a spot,
                                // if there is more fire there, make it bigger and give it some fuel.
                                // This is how big fires spend their excess age:
                                // making other fires bigger. Flashpoint.
                                const size_t end_it = static_cast<size_t>( rng( 0, neighs.size() - 1 ) );
                                for( size_t i = ( end_it + 1 ) % neighs.size();
                                     i != end_it && cur.getFieldAge() < 0_turns;
                                     i = ( i + 1 ) % neighs.size() ) {
                                    maptile &dst = neighs[i];
                                    auto dstfld = dst.find_field( fd_fire );
                                    // If the fire exists and is weaker than ours, boost it
                                    if( dstfld != nullptr &&
                                        ( dstfld->getFieldDensity() <= cur.getFieldDensity() ||
                                          dstfld->getFieldAge() > cur.getFieldAge() ) &&
                                        ( in_pit == ( dst.get_ter() == t_pit ) ) ) {
                                        if( dstfld->getFieldDensity() < 2 ) {
                                            dstfld->setFieldDensity( dstfld->getFieldDensity() + 1 );
                                        }

                                        dstfld->setFieldAge( dstfld->getFieldAge() - 5_minutes );
                                        cur.setFieldAge( cur.getFieldAge() + 5_minutes );
                                    }

                                    if( dstfld != nullptr ) {
                                        adjacent_fires++;
                                    }
                                }
                            } else if( cur.getFieldAge() < 0_turns && cur.getFieldDensity() < 3 ) {
                                // See if we can grow into a stage 2/3 fire, for this
                                // burning neighbors are necessary in addition to
                                // field age < 0, or alternatively, a LOT of fuel.

                                // The maximum fire density is 1 for a lone fire, 2 for at least 1 neighbor,
                                // 3 for at least 2 neighbors.
                                int maximum_density =  1;

                                // The following logic looks a bit complex due to optimization concerns, so here are the semantics:
                                // 1. Calculate maximum field density based on fuel, -50 minutes is 2(medium), -500 minutes is 3(raging)
                                // 2. Calculate maximum field density based on neighbors, 3 neighbors is 2(medium), 7 or more neighbors is 3(raging)
                                // 3. Pick the higher maximum between 1. and 2.
                                if( cur.getFieldAge() < -500_minutes ) {
                                    maximum_density = 3;
                                } else {
                                    for( auto &neigh : neighs ) {
                                        if( neigh.get_field().findField( fd_fire ) != nullptr ) {
                                            adjacent_fires++;
                                        }
                                    }
                                    maximum_density = 1 + ( adjacent_fires >= 3 ) + ( adjacent_fires >= 7 );

                                    if( maximum_density < 2 && cur.getFieldAge() < -50_minutes ) {
                                        maximum_density = 2;
                                    }
                                }

                                // If we consumed a lot, the flames grow higher
                                if( cur.getFieldDensity() < maximum_density && cur.getFieldAge() < 0_turns ) {
                                    // Fires under 0 age grow in size. Level 3 fires under 0 spread later on.
                                    // Weaken the newly-grown fire
                                    cur.setFieldDensity( cur.getFieldDensity() + 1 );
                                    cur.setFieldAge( cur.getFieldAge() + 10_minutes * cur.getFieldDensity() );
                                }
                            }
                        }

                        // Consume adjacent fuel / terrain / webs to spread.
                        // Allow raging fires (and only raging fires) to spread up
                        // Spreading down is achieved by wrecking the walls/floor and then falling
                        if( zlevels && cur.getFieldDensity() == 3 && p.z < OVERMAP_HEIGHT ) {
                            // Let it burn through the floor
                            maptile dst = maptile_at_internal( {p.x, p.y, p.z + 1} );
                            const auto &dst_ter = dst.get_ter_t();
                            if( dst_ter.has_flag( TFLAG_NO_FLOOR ) ||
                                dst_ter.has_flag( TFLAG_FLAMMABLE ) ||
                                dst_ter.has_flag( TFLAG_FLAMMABLE_ASH ) ||
                                dst_ter.has_flag( TFLAG_FLAMMABLE_HARD ) ) {
                                field_entry *nearfire = dst.find_field( fd_fire );
                                if( nearfire != nullptr ) {
                                    nearfire->setFieldAge( nearfire->getFieldAge() - 2_minutes );
                                } else {
                                    dst.add_field( fd_fire, 1, 0_turns );
                                }
                                // Fueling fires above doesn't cost fuel
                            }
                        }

                        // Our iterator will start at end_i + 1 and increment from there and then wrap around.
                        // This guarantees it will check all neighbors, starting from a random one
                        const size_t end_i = static_cast<size_t>( rng( 0, neighs.size() - 1 ) );
                        for( size_t i = ( end_i + 1 ) % neighs.size();
                             i != end_i; i = ( i + 1 ) % neighs.size() ) {
                            if( one_in( cur.getFieldDensity() * 2 ) ) {
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
                            int spread_chance = 25 * ( cur.getFieldDensity() - 1 );
                            if( nearwebfld != nullptr ) {
                                spread_chance = 50 + spread_chance / 2;
                            }

                            const auto &dster = dst.get_ter_t();
                            const auto &dsfrn = dst.get_furn_t();
                            // Allow weaker fires to spread occasionally
                            const int power = cur.getFieldDensity() + one_in( 5 );
                            if( can_spread && rng( 1, 100 ) < spread_chance &&
                                ( in_pit == ( dster.id.id() == t_pit ) ) &&
                                (
                                    ( power >= 3 && cur.getFieldAge() < 0_turns && one_in( 20 ) ) ||
                                    ( power >= 2 && ( ter_furn_has_flag( dster, dsfrn, TFLAG_FLAMMABLE ) && one_in( 2 ) ) ) ||
                                    ( power >= 2 && ( ter_furn_has_flag( dster, dsfrn, TFLAG_FLAMMABLE_ASH ) && one_in( 2 ) ) ) ||
                                    ( power >= 3 && ( ter_furn_has_flag( dster, dsfrn, TFLAG_FLAMMABLE_HARD ) && one_in( 5 ) ) ) ||
                                    nearwebfld || ( dst.get_item_count() > 0 &&
                                                    flammable_items_at( p + eight_horizontal_neighbors[i] ) &&
                                                    one_in( 5 ) )
                                ) ) {
                                dst.add_field( fd_fire, 1, 0_turns ); // Nearby open flammable ground? Set it on fire.
                                tmpfld = dst.find_field( fd_fire );
                                if( tmpfld != nullptr ) {
                                    // Make the new fire quite weak, so that it doesn't start jumping around instantly
                                    tmpfld->setFieldAge( 2_minutes );
                                    // Consume a bit of our fuel
                                    cur.setFieldAge( cur.getFieldAge() + 1_minutes );
                                }
                                if( nearwebfld ) {
                                    nearwebfld->setFieldDensity( 0 );
                                }
                            }
                        }

                        // Create smoke once - above us if possible, at us otherwise
                        if( !ter_furn_has_flag( ter, frn, TFLAG_SUPPRESS_SMOKE ) &&
                            rng( 0, 100 ) <= smoke &&
                            rng( 3, 35 ) < cur.getFieldDensity() * 10 ) {
                            bool smoke_up = zlevels && p.z < OVERMAP_HEIGHT;
                            if( smoke_up ) {
                                tripoint up{p.x, p.y, p.z + 1};
                                maptile dst = maptile_at_internal( up );
                                const auto &dst_ter = dst.get_ter_t();
                                if( dst_ter.has_flag( TFLAG_NO_FLOOR ) ) {
                                    dst.add_field( fd_smoke, rng( 1, cur.getFieldDensity() ), 0_turns );
                                } else {
                                    // Can't create smoke above
                                    smoke_up = false;
                                }
                            }

                            if( !smoke_up ) {
                                maptile dst = maptile_at_internal( p );
                                // Create thicker smoke
                                dst.add_field( fd_smoke, cur.getFieldDensity(), 0_turns );
                            }

                            dirty_transparency_cache = true; // Smoke affects transparency
                        }

                        // Hot air is a load on the CPU
                        // Don't produce too much of it if we have a lot fires nearby, they produce
                        // radiant heat which does what hot air would do anyway
                        if( adjacent_fires < 5 && rng( 0, 4 - adjacent_fires ) ) {
                            create_hot_air( p, cur.getFieldDensity() );
                        }
                    }
                    break;

                    case fd_smoke:
                        dirty_transparency_cache = true;
                        spread_gas( cur, p, curtype, 50, 0_turns );
                        break;

                    case fd_tear_gas:
                        dirty_transparency_cache = true;
                        spread_gas( cur, p, curtype, 30, 0_turns );
                        break;

                    case fd_relax_gas:
                        dirty_transparency_cache = true;
                        spread_gas( cur, p, curtype, 25, 5_minutes );
                        break;

                    case fd_fungal_haze:
                        dirty_transparency_cache = true;
                        spread_gas( cur, p, curtype, 33, 5_turns );
                        if( one_in( 10 - 2 * cur.getFieldDensity() ) ) {
                            // Haze'd terrain
                            fungal_effects( *g, g->m ).spread_fungus( p );
                        }

                        break;

                    case fd_toxic_gas:
                        dirty_transparency_cache = true;
                        spread_gas( cur, p, curtype, 50, 3_minutes );
                        break;

                    case fd_cigsmoke:
                        dirty_transparency_cache = true;
                        spread_gas( cur, p, curtype, 250, 65_turns );
                        break;

                    case fd_weedsmoke: {
                        dirty_transparency_cache = true;
                        spread_gas( cur, p, curtype, 200, 6_minutes );

                        if( one_in( 20 ) ) {
                            if( npc *const np = g->critter_at<npc>( p ) ) {
                                if( np->is_friend() ) {
                                    np->say( one_in( 10 ) ? _( "Whew... smells like skunk!" ) :
                                             _( "Man, that smells like some good shit!" ) );
                                }
                            }
                        }

                    }
                    break;

                    case fd_methsmoke: {
                        dirty_transparency_cache = true;
                        spread_gas( cur, p, curtype, 175, 7_minutes );

                        if( one_in( 20 ) ) {
                            if( npc *const np = g->critter_at<npc>( p ) ) {
                                if( np->is_friend() ) {
                                    np->say( _( "I don't know... should you really be smoking that stuff?" ) );
                                }
                            }
                        }
                    }
                    break;

                    case fd_cracksmoke: {
                        dirty_transparency_cache = true;
                        spread_gas( cur, p, curtype, 175, 8_minutes );

                        if( one_in( 20 ) ) {
                            if( npc *const np = g->critter_at<npc>( p ) ) {
                                if( np->is_friend() ) {
                                    np->say( one_in( 2 ) ? _( "Ew, smells like burning rubber!" ) : _( "Ugh, that smells rancid!" ) );
                                }
                            }
                        }
                    }
                    break;

                    case fd_nuke_gas: {
                        dirty_transparency_cache = true;
                        int extra_radiation = rng( 0, cur.getFieldDensity() );
                        adjust_radiation( p, extra_radiation );
                        spread_gas( cur, p, curtype, 50, 1_minutes );
                        break;
                    }

                    case fd_hot_air1:
                    case fd_hot_air2:
                    case fd_hot_air3:
                    case fd_hot_air4:
                        // No transparency cache wrecking here!
                        spread_gas( cur, p, curtype, 100, 100_minutes );
                        break;

                    case fd_gas_vent: {
                        dirty_transparency_cache = true;
                        for( const tripoint &pnt : points_in_radius( p, cur.getFieldDensity() - 1 ) ) {
                            field &wandering_field = get_field( pnt );
                            tmpfld = wandering_field.findField( fd_toxic_gas );
                            if( tmpfld && tmpfld->getFieldDensity() < cur.getFieldDensity() ) {
                                tmpfld->setFieldDensity( tmpfld->getFieldDensity() + 1 );
                            } else {
                                add_field( pnt, fd_toxic_gas, cur.getFieldDensity() );
                            }
                        }
                    }
                    break;

                    case fd_smoke_vent: {
                        dirty_transparency_cache = true;
                        for( const tripoint &pnt : points_in_radius( p, cur.getFieldDensity() - 1 ) ) {
                            field &wandering_field = get_field( pnt );
                            tmpfld = wandering_field.findField( fd_smoke );
                            if( tmpfld && tmpfld->getFieldDensity() < cur.getFieldDensity() ) {
                                tmpfld->setFieldDensity( tmpfld->getFieldDensity() + 1 );
                            } else {
                                add_field( pnt, fd_smoke, cur.getFieldDensity() );
                            }
                        }
                    }
                    break;

                    case fd_fire_vent:
                        if( cur.getFieldDensity() > 1 ) {
                            if( one_in( 3 ) ) {
                                cur.setFieldDensity( cur.getFieldDensity() - 1 );
                            }
                            create_hot_air( p, cur.getFieldDensity() );
                        } else {
                            dirty_transparency_cache = true;
                            add_field( p, fd_flame_burst, 3, cur.getFieldAge() );
                            cur.setFieldDensity( 0 );
                        }
                        break;

                    case fd_flame_burst:
                        if( cur.getFieldDensity() > 1 ) {
                            cur.setFieldDensity( cur.getFieldDensity() - 1 );
                            create_hot_air( p, cur.getFieldDensity() );
                        } else {
                            dirty_transparency_cache = true;
                            add_field( p, fd_fire_vent, 3, cur.getFieldAge() );
                            cur.setFieldDensity( 0 );
                        }
                        break;

                    case fd_electricity:
                        if( !one_in( 5 ) ) { // 4 in 5 chance to spread
                            std::vector<tripoint> valid;
                            if( impassable( p ) && cur.getFieldDensity() > 1 ) { // We're grounded
                                int tries = 0;
                                tripoint pnt;
                                pnt.z = p.z;
                                while( tries < 10 && cur.getFieldAge() < 5_minutes && cur.getFieldDensity() > 1 ) {
                                    pnt.x = p.x + rng( -1, 1 );
                                    pnt.y = p.y + rng( -1, 1 );
                                    if( passable( pnt ) ) {
                                        add_field( pnt, fd_electricity, 1, cur.getFieldAge() + 1_turns );
                                        cur.setFieldDensity( cur.getFieldDensity() - 1 );
                                        tries = 0;
                                    } else {
                                        tries++;
                                    }
                                }
                            } else {    // We're not grounded; attempt to ground
                                for( const tripoint &dst : points_in_radius( p, 1 ) ) {
                                    if( impassable( dst ) ) { // Grounded tiles first
                                        valid.push_back( dst );
                                    }
                                }
                                if( valid.empty() ) {    // Spread to adjacent space, then
                                    tripoint dst( p.x + rng( -1, 1 ), p.y + rng( -1, 1 ), p.z );
                                    field_entry *elec = get_field( dst ).findField( fd_electricity );
                                    if( passable( dst ) && elec != nullptr &&
                                        elec->getFieldDensity() < 3 ) {
                                        elec->setFieldDensity( elec->getFieldDensity() + 1 );
                                        cur.setFieldDensity( cur.getFieldDensity() - 1 );
                                    } else if( passable( dst ) ) {
                                        add_field( dst, fd_electricity, 1, cur.getFieldAge() + 1_turns );
                                    }
                                    cur.setFieldDensity( cur.getFieldDensity() - 1 );
                                }
                                while( !valid.empty() && cur.getFieldDensity() > 1 ) {
                                    const tripoint target = random_entry_removed( valid );
                                    add_field( target, fd_electricity, 1, cur.getFieldAge() + 1_turns );
                                    cur.setFieldDensity( cur.getFieldDensity() - 1 );
                                }
                            }
                        }
                        break;

                    case fd_fatigue: {
                        static const std::array<mtype_id, 9> monids = { {
                                mtype_id( "mon_flying_polyp" ), mtype_id( "mon_hunting_horror" ),
                                mtype_id( "mon_mi_go" ), mtype_id( "mon_yugg" ), mtype_id( "mon_gelatin" ),
                                mtype_id( "mon_flaming_eye" ), mtype_id( "mon_kreck" ), mtype_id( "mon_gracke" ),
                                mtype_id( "mon_blank" ),
                            }
                        };
                        if( cur.getFieldDensity() < 3 && calendar::once_every( 6_hours ) && one_in( 10 ) ) {
                            cur.setFieldDensity( cur.getFieldDensity() + 1 );
                        } else if( cur.getFieldDensity() == 3 && one_in( 600 ) ) { // Spawn nether creature!
                            g->summon_mon( random_entry( monids ), p );
                        }
                    }
                    break;

                    case fd_push_items: {
                        auto items = i_at( p );
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
                                        add_msg( m_bad, _( "A %s hits you!" ), tmp.tname().c_str() );
                                        body_part hit = random_body_part();
                                        g->u.deal_damage( nullptr, hit, damage_instance( DT_BASH, 6 ) );
                                        g->u.check_dead_state();
                                    }

                                    if( npc *const p = g->critter_at<npc>( newp ) ) {
                                        // TODO: combine with player character code above
                                        body_part hit = random_body_part();
                                        p->deal_damage( nullptr, hit, damage_instance( DT_BASH, 6 ) );
                                        if( g->u.sees( newp ) ) {
                                            add_msg( _( "A %1$s hits %2$s!" ), tmp.tname().c_str(), p->name.c_str() );
                                        }
                                        p->check_dead_state();
                                    } else if( monster *const mon = g->critter_at<monster>( newp ) ) {
                                        mon->apply_damage( nullptr, bp_torso, 6 - mon->get_armor_bash( bp_torso ) );
                                        if( g->u.sees( newp ) )
                                            add_msg( _( "A %1$s hits the %2$s!" ), tmp.tname().c_str(),
                                                     mon->name().c_str() );
                                        mon->check_dead_state();
                                    }
                                }
                            }
                        }
                    }
                    break;

                    case fd_shock_vent:
                        if( cur.getFieldDensity() > 1 ) {
                            if( one_in( 5 ) ) {
                                cur.setFieldDensity( cur.getFieldDensity() - 1 );
                            }
                        } else {
                            cur.setFieldDensity( 3 );
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
                        break;

                    case fd_acid_vent:
                        if( cur.getFieldDensity() > 1 ) {
                            if( cur.getFieldAge() >= 1_minutes ) {
                                cur.setFieldDensity( cur.getFieldDensity() - 1 );
                                cur.setFieldAge( 0_turns );
                            }
                        } else {
                            cur.setFieldDensity( 3 );
                            for( const tripoint &t : points_in_radius( p, 5 ) ) {
                                const field_entry *acid = get_field( t, fd_acid );
                                if( acid != nullptr && acid->getFieldDensity() == 0 ) {
                                    int newdens = 3 - ( rl_dist( p, t ) / 2 ) + ( one_in( 3 ) ? 1 : 0 );
                                    if( newdens > 3 ) {
                                        newdens = 3;
                                    }
                                    if( newdens > 0 ) {
                                        add_field( t, fd_acid, newdens );
                                    }
                                }
                            }
                        }
                        break;

                    case fd_bees:
                        dirty_transparency_cache = true;
                        // Poor bees are vulnerable to so many other fields.
                        // TODO: maybe adjust effects based on different fields.
                        if( curfield.findField( fd_web ) ||
                            curfield.findField( fd_fire ) ||
                            curfield.findField( fd_smoke ) ||
                            curfield.findField( fd_toxic_gas ) ||
                            curfield.findField( fd_tear_gas ) ||
                            curfield.findField( fd_relax_gas ) ||
                            curfield.findField( fd_nuke_gas ) ||
                            curfield.findField( fd_gas_vent ) ||
                            curfield.findField( fd_smoke_vent ) ||
                            curfield.findField( fd_fungicidal_gas ) ||
                            curfield.findField( fd_fire_vent ) ||
                            curfield.findField( fd_flame_burst ) ||
                            curfield.findField( fd_electricity ) ||
                            curfield.findField( fd_fatigue ) ||
                            curfield.findField( fd_shock_vent ) ||
                            curfield.findField( fd_plasma ) ||
                            curfield.findField( fd_laser ) ||
                            curfield.findField( fd_dazzling ) ||
                            curfield.findField( fd_electricity ) ||
                            curfield.findField( fd_incendiary ) ) {
                            // Kill them at the end of processing.
                            cur.setFieldDensity( 0 );
                        } else {
                            // Bees chase the player if in range, wander randomly otherwise.
                            if( !g->u.is_underwater() &&
                                rl_dist( p, g->u.pos() ) < 10 &&
                                clear_path( p, g->u.pos(), 10, 0, 100 ) ) {

                                std::vector<point> candidate_positions =
                                    squares_in_direction( p.x, p.y, g->u.posx(), g->u.posy() );
                                for( auto &candidate_position : candidate_positions ) {
                                    field &target_field =
                                        get_field( tripoint( candidate_position, p.z ) );
                                    // Only shift if there are no bees already there.
                                    // TODO: Figure out a way to merge bee fields without allowing
                                    // Them to effectively move several times in a turn depending
                                    // on iteration direction.
                                    if( !target_field.findField( fd_bees ) ) {
                                        add_field( tripoint( candidate_position, p.z ), fd_bees,
                                                   cur.getFieldDensity(), cur.getFieldAge() );
                                        cur.setFieldDensity( 0 );
                                        break;
                                    }
                                }
                            } else {
                                spread_gas( cur, p, curtype, 5, 0_turns );
                            }
                        }
                        break;

                    case fd_incendiary: {
                        //Needed for variable scope
                        dirty_transparency_cache = true;
                        tripoint dst( p.x + rng( -1, 1 ), p.y + rng( -1, 1 ), p.z );
                        if( has_flag( TFLAG_FLAMMABLE, dst ) ||
                            has_flag( TFLAG_FLAMMABLE_ASH, dst ) ||
                            has_flag( TFLAG_FLAMMABLE_HARD, dst ) ) {
                            add_field( dst, fd_fire, 1 );
                        }

                        //check piles for flammable items and set those on fire
                        if( flammable_items_at( dst ) ) {
                            add_field( dst, fd_fire, 1 );
                        }

                        spread_gas( cur, p, curtype, 66, 4_minutes );
                        create_hot_air( p, cur.getFieldDensity() );
                    }
                    break;

                    //Legacy Stuff
                    case fd_rubble:
                        make_rubble( p );
                        break;

                    case fd_fungicidal_gas: {
                        dirty_transparency_cache = true;
                        spread_gas( cur, p, curtype, 120, 1_minutes );
                        //check the terrain and replace it accordingly to simulate the fungus dieing off
                        const auto &ter = map_tile.get_ter_t();
                        const auto &frn = map_tile.get_furn_t();
                        const int density = cur.getFieldDensity();
                        if( ter.has_flag( "FUNGUS" ) && one_in( 10 / density ) ) {
                            ter_set( p, t_dirt );
                        }
                        if( frn.has_flag( "FUNGUS" ) && one_in( 10 / density ) ) {
                            furn_set( p, f_null );
                        }
                    }
                    break;

                    default:
                        //Suppress warnings
                        break;

                } // switch (curtype)

                cur.setFieldAge( cur.getFieldAge() + 1_turns );
                auto &fdata = fieldlist[cur.getFieldType()];
                if( fdata.halflife > 0_turns && cur.getFieldAge() > 0_turns &&
                    dice( 2, to_turns<int>( cur.getFieldAge() ) ) > to_turns<int>( fdata.halflife ) ) {
                    cur.setFieldAge( 0_turns );
                    cur.setFieldDensity( cur.getFieldDensity() - 1 );
                }
                if( !cur.isAlive() ) {
                    current_submap->field_count--;
                    curfield.removeField( it++ );
                } else {
                    ++it;
                }
            }
        }
    }
    return dirty_transparency_cache;
}

//This entire function makes very little sense. Why are the rules the way they are? Why does walking into some things destroy them but not others?

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
    bool inside = false; // Are we inside?
    //to modify power of a field based on... whatever is relevant for the effect.
    int adjusted_intensity;

    //If we are in a vehicle figure out if we are inside (reduces effects usually)
    // and what part of the vehicle we need to deal with.
    if( u.in_vehicle ) {
        if( const optional_vpart_position vp = veh_at( u.pos() ) ) {
            inside = vp->is_inside();
        }
    }

    // Iterate through all field effects on this tile.
    // Do not remove the field with removeField, instead set it's density to 0. It will be removed
    // later by the field processing, which will also adjust field_count accordingly.
    for( auto &field_list_it : curfield ) {
        field_entry &cur = field_list_it.second;
        if( !cur.isAlive() ) {
            continue;
        }

        //Do things based on what field effect we are currently in.
        switch( cur.getFieldType() ) {
            case fd_null:
            case fd_blood: // It doesn't actually do anything //necessary to add other types of blood?
            case fd_bile:  // Ditto
            case fd_cigsmoke:
            case fd_weedsmoke:
            case fd_methsmoke:
            case fd_cracksmoke:
                //break instead of return in the event of post-processing in the future;
                // also we're in a loop now!
                break;

            case fd_web: {
                //If we are in a web, can't walk in webs or are in a vehicle, get webbed maybe.
                //Moving through multiple webs stacks the effect.
                if( !u.has_trait( trait_id( "WEB_WALKER" ) ) && !u.in_vehicle ) {
                    //between 5 and 15 minus your current web level.
                    u.add_effect( effect_webbed, 1_turns, num_bp, true, cur.getFieldDensity() );
                    cur.setFieldDensity( 0 ); //Its spent.
                    continue;
                    //If you are in a vehicle destroy the web.
                    //It should of been destroyed when you ran over it anyway.
                } else if( u.in_vehicle ) {
                    cur.setFieldDensity( 0 );
                    continue;
                }
            }
            break;

            case fd_acid: {
                // Assume vehicles block acid damage entirely,
                // you're certainly not standing in it.
                if( u.in_vehicle ) {
                    break;
                }

                if( u.has_trait( trait_id( "ACIDPROOF" ) ) ) {
                    // No need for warnings
                    break;
                }

                const int density = cur.getFieldDensity();
                int total_damage = 0;
                // Use a helper for a bit less boilerplate
                const auto burn_part = [&]( body_part bp, const int scale ) {
                    const int damage = rng( 1, scale + density );
                    // A bit ugly, but better than being annoyed by acid when in hazmat
                    if( u.get_armor_type( DT_ACID, bp ) < damage ) {
                        auto ddi = u.deal_damage( nullptr, bp, damage_instance( DT_ACID, damage ) );
                        total_damage += ddi.total_damage();
                    }
                    // Represents acid seeping in rather than being splashed on
                    u.add_env_effect( effect_corroding, bp, 2 + density, time_duration::from_turns( rng( 2,
                                      1 + density ) ), bp, false, 0 );
                };

                // 1-3 at density, 1-4 at 2, 1-5 at 3
                burn_part( bp_foot_l, 2 );
                burn_part( bp_foot_r, 2 );
                // 1 dmg at 1 density, 1-3 at 2, 1-5 at 3
                burn_part( bp_leg_l,  density - 1 );
                burn_part( bp_leg_r,  density - 1 );
                const bool on_ground = u.is_on_ground();
                if( on_ground ) {
                    // Before, it would just break the legs and leave the survivor alone
                    burn_part( bp_hand_l, 2 );
                    burn_part( bp_hand_r, 2 );
                    burn_part( bp_torso,  2 );
                    // Less arms = less ability to keep upright
                    if( ( !u.has_two_arms() && one_in( 4 ) ) || one_in( 2 ) ) {
                        burn_part( bp_arm_l, 1 );
                        burn_part( bp_arm_r, 1 );
                        burn_part( bp_head,  1 );
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
            break;

            case fd_sap:
                //Sap causes the player to get sap disease, slowing them down.
                if( u.in_vehicle ) {
                    break; //sap does nothing to cars.
                }
                u.add_msg_player_or_npc( m_bad, _( "The sap sticks to you!" ),
                                         _( "The sap sticks to <npcname>!" ) );
                u.add_effect( effect_sap, cur.getFieldDensity() * 2_turns );
                cur.setFieldDensity( cur.getFieldDensity() - 1 ); //Use up sap.
                break;

            case fd_sludge:
                //sludge is on the ground, but you are above the ground when boarded on a vehicle
                if( !u.in_vehicle ) {
                    u.add_msg_if_player( m_bad, _( "The sludge is thick and sticky. You struggle to pull free." ) );
                    u.moves -= cur.getFieldDensity() * 300;
                    cur.setFieldDensity( 0 );
                }
                break;

            case fd_fire:
                if( u.has_active_bionic( bionic_id( "bio_heatsink" ) ) || u.is_wearing( "rm13_armor_on" ) ) {
                    //heatsink or suit prevents ALL fire damage.
                    break;
                }
                //Burn the player. Less so if you are in a car or ON a car.
                adjusted_intensity = cur.getFieldDensity();
                if( u.in_vehicle ) {
                    if( inside ) {
                        adjusted_intensity -= 2;
                    } else {
                        adjusted_intensity -= 1;
                    }
                }

                if( adjusted_intensity < 1 ) {
                    break;
                }
                {
                    // Burn message by intensity
                    static const std::array<std::string, 4> player_burn_msg = {{
                            translate_marker( "You burn your legs and feet!" ),
                            translate_marker( "You're burning up!" ),
                            translate_marker( "You're set ablaze!" ),
                            translate_marker( "Your whole body is burning!" )
                        }
                    };
                    static const std::array<std::string, 4> npc_burn_msg = {{
                            translate_marker( "<npcname> burns their legs and feet!" ),
                            translate_marker( "<npcname> is burning up!" ),
                            translate_marker( "<npcname> is set ablaze!" ),
                            translate_marker( "<npcname>s whole body is burning!" )
                        }
                    };
                    static const std::array<std::string, 4> player_warn_msg = {{
                            translate_marker( "You're standing in a fire!" ),
                            translate_marker( "You're waist-deep in a fire!" ),
                            translate_marker( "You're surrounded by raging fire!" ),
                            translate_marker( "You're lying in fire!" )
                        }
                    };

                    int burn_min = adjusted_intensity;
                    int burn_max = 3 * adjusted_intensity + 3;
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
                    for( auto part_burned : parts_burned ) {
                        const auto dealt = u.deal_damage( nullptr, part_burned,
                                                          damage_instance( DT_HEAT, rng( burn_min, burn_max ) ) );
                        total_damage += dealt.type_damage( DT_HEAT );
                    }
                    if( total_damage > 0 ) {
                        u.add_msg_player_or_npc( m_bad,
                                                 _( player_burn_msg[msg_num].c_str() ),
                                                 _( npc_burn_msg[msg_num].c_str() ) );
                    } else {
                        u.add_msg_if_player( m_warning,
                                             _( player_warn_msg[msg_num].c_str() ) );
                    }
                    u.check_dead_state();
                }
                break;

            case fd_smoke: {
                if( !inside ) {
                    //Get smoke disease from standing in smoke.
                    int density = cur.getFieldDensity();
                    int coughStr;
                    time_duration coughDur = 0_turns;
                    if( density >= 3 ) {  // thick smoke
                        coughStr = 4;
                        coughDur = 15_turns;
                    } else if( density == 2 ) { // smoke
                        coughStr = 2;
                        coughDur = 7_turns;
                    } else {    // density 1, thin smoke
                        coughStr = 1;
                        coughDur = 2_turns;
                    }
                    u.add_env_effect( effect_smoke, bp_mouth, coughStr, coughDur );
                }
            }
            break;

            case fd_tear_gas:
                //Tear gas will both give you teargas disease and/or blind you.
                if( ( cur.getFieldDensity() > 1 || !one_in( 3 ) ) && ( !inside || one_in( 3 ) ) ) {
                    u.add_env_effect( effect_teargas, bp_mouth, 5, 2_minutes );
                }
                if( cur.getFieldDensity() > 1 && ( !inside || one_in( 3 ) ) ) {
                    u.add_env_effect( effect_blind, bp_eyes, cur.getFieldDensity() * 2, 1_minutes );
                }
                break;

            case fd_relax_gas:
                if( ( cur.getFieldDensity() > 1 || !one_in( 3 ) ) && ( !inside || one_in( 3 ) ) ) {
                    u.add_env_effect( effect_relax_gas, bp_mouth, cur.getFieldDensity() * 2, 3_turns );
                }
                break;

            case fd_fungal_haze:
                if( !u.has_trait( trait_id( "M_IMMUNE" ) ) && ( !inside || one_in( 4 ) ) ) {
                    u.add_env_effect( effect_fungus, bp_mouth, 4, 10_minutes, num_bp, true );
                    u.add_env_effect( effect_fungus, bp_eyes, 4, 10_minutes, num_bp, true );
                }
                break;

            case fd_dazzling:
                if( cur.getFieldDensity() > 1 || one_in( 5 ) ) {
                    u.add_env_effect( effect_blind, bp_eyes, 10, 10_turns );
                } else {
                    u.add_env_effect( effect_blind, bp_eyes, 2, 2_turns );
                }
                break;

            case fd_toxic_gas:
                // Toxic gas at low levels poisons you.
                // Toxic gas at high levels will cause very nasty poison.
            {
                bool inhaled = false;
                if( ( cur.getFieldDensity() == 2 && !inside ) ||
                    ( cur.getFieldDensity() == 3 && inside ) ) {
                    inhaled = u.add_env_effect( effect_poison, bp_mouth, 5, 3_minutes );
                } else if( cur.getFieldDensity() == 3 && !inside ) {
                    inhaled = u.add_env_effect( effect_badpoison, bp_mouth, 5, 3_minutes );
                } else if( cur.getFieldDensity() == 1 && ( !inside ) ) {
                    inhaled = u.add_env_effect( effect_poison, bp_mouth, 2, 2_minutes );
                }
                if( inhaled ) {
                    // player does not know how the npc feels, so no message.
                    u.add_msg_if_player( m_bad, _( "You feel sick from inhaling the %s" ), cur.name().c_str() );
                }
            }
            break;

            case fd_nuke_gas: {
                // Get irradiated by the nuclear fallout.
                // Changed to min of density, not 0.
                float rads = rng( cur.getFieldDensity(),
                                  cur.getFieldDensity() * ( cur.getFieldDensity() + 1 ) );
                bool rad_proof = !u.irradiate( rads );
                //TODO: Reduce damage for rad resistant?
                if( cur.getFieldDensity() == 3 && !rad_proof ) {
                    u.add_msg_if_player( m_bad, _( "This radioactive gas burns!" ) );
                    u.hurtall( rng( 1, 3 ), nullptr );
                }
            }
            break;

            case fd_flame_burst:
                //A burst of flame? Only hits the legs and torso.
                if( inside ) {
                    break;    //fireballs can't touch you inside a car.
                }
                if( !u.has_active_bionic( bionic_id( "bio_heatsink" ) ) &&
                    !u.is_wearing( "rm13_armor_on" ) ) { //heatsink or suit stops fire.
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
                break;

            case fd_electricity: {
                // Small universal damage based on density, only if not electroproofed.
                if( u.is_elec_immune() ) {
                    break;
                }
                int total_damage = 0;
                for( size_t i = 0; i < num_hp_parts; i++ ) {
                    const body_part bp = player::hp_to_bp( static_cast<hp_part>( i ) );
                    const int dmg = rng( 1, cur.getFieldDensity() );
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

            break;

            case fd_fatigue:
                //Teleports you... somewhere.
                if( rng( 0, 2 ) < cur.getFieldDensity() && u.is_player() ) {
                    // TODO: allow teleporting for npcs
                    add_msg( m_bad, _( "You're violently teleported!" ) );
                    u.hurtall( cur.getFieldDensity(), nullptr );
                    g->teleport();
                }
                break;

            //Why do these get removed???
            case fd_shock_vent:
                //Stepping on a shock vent shuts it down.
                cur.setFieldDensity( 0 );
                continue;

            case fd_acid_vent:
                //Stepping on an acid vent shuts it down.
                cur.setFieldDensity( 0 );
                continue;

            case fd_bees:
                // Player is immune to bees while underwater.
                if( !u.is_underwater() ) {
                    int times_stung = 0;
                    int density = cur.getFieldDensity();
                    // If the bees can get at you, they cause steadily increasing pain.
                    // TODO: Specific stinging messages.
                    times_stung += one_in( 4 ) &&
                                   u.add_env_effect( effect_stung, bp_torso, density, 9_minutes );
                    times_stung += one_in( 4 ) &&
                                   u.add_env_effect( effect_stung, bp_torso, density, 9_minutes );
                    times_stung += one_in( 4 ) &&
                                   u.add_env_effect( effect_stung, bp_torso, density, 9_minutes );
                    times_stung += one_in( 4 ) &&
                                   u.add_env_effect( effect_stung, bp_torso, density, 9_minutes );
                    times_stung += one_in( 4 ) &&
                                   u.add_env_effect( effect_stung, bp_torso, density, 9_minutes );
                    times_stung += one_in( 4 ) &&
                                   u.add_env_effect( effect_stung, bp_torso, density, 9_minutes );
                    times_stung += one_in( 4 ) &&
                                   u.add_env_effect( effect_stung, bp_torso, density, 9_minutes );
                    times_stung += one_in( 4 ) &&
                                   u.add_env_effect( effect_stung, bp_torso, density, 9_minutes );
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
                break;

            case fd_incendiary:
                // Mysterious incendiary substance melts you horribly.
                if( u.has_trait( trait_M_SKIN2 ) ||
                    u.has_trait( trait_M_SKIN3 ) ||
                    cur.getFieldDensity() == 1 ) {
                    u.add_msg_player_or_npc( m_bad, _( "The incendiary burns you!" ),
                                             _( "The incendiary burns <npcname>!" ) );
                    u.hurtall( rng( 1, 3 ), nullptr );
                } else {
                    u.add_msg_player_or_npc( m_bad, _( "The incendiary melts into your skin!" ),
                                             _( "The incendiary melts into <npcname>s skin!" ) );
                    u.add_effect( effect_onfire, 8_turns, bp_torso );
                    u.hurtall( rng( 2, 6 ), nullptr );
                }
                break;

            case fd_fungicidal_gas:
                // Fungicidal gas is unhealthy and becomes deadly if you cross a related threshold.
            {
                // The gas won't harm you inside a vehicle.
                if( inside ) {
                    break;
                }
                // Full body suits protect you from the effects of the gas.
                if( u.worn_with_flag( "GAS_PROOF" ) && u.get_env_resist( bp_mouth ) >= 15 &&
                    u.get_env_resist( bp_eyes ) >= 15 ) {
                    break;
                }
                bool inhaled = false;
                const int density = cur.getFieldDensity();
                inhaled = u.add_env_effect( effect_poison, bp_mouth, 5, density * 1_minutes );
                if( u.has_trait( trait_id( "THRESH_MYCUS" ) ) || u.has_trait( trait_id( "THRESH_MARLOSS" ) ) ) {
                    inhaled |= u.add_env_effect( effect_badpoison, bp_mouth, 5, density * 1_minutes );
                    u.hurtall( rng( density, density * 2 ), nullptr );
                    u.add_msg_if_player( m_bad, _( "The %s burns your skin." ), cur.name().c_str() );
                }

                if( inhaled ) {
                    u.add_msg_if_player( m_bad, _( "The %s makes you feel sick." ), cur.name().c_str() );
                }
            }
            break;

            default:
                //Suppress warnings
                break;
        }
    }

}

void map::creature_in_field( Creature &critter )
{
    auto m = dynamic_cast<monster *>( &critter );
    auto p = dynamic_cast<player *>( &critter );
    if( m != nullptr ) {
        monster_in_field( *m );
    } else if( p != nullptr ) {
        player_in_field( *p );
    }
}

void map::monster_in_field( monster &z )
{
    if( z.digging() ) {
        return; // Digging monsters are immune to fields
    }
    field &curfield = get_field( z.pos() );

    int dam = 0;
    // Iterate through all field effects on this tile.
    // Do not remove the field with removeField, instead set it's density to 0. It will be removed
    // later by the field processing, which will also adjust field_count accordingly.
    for( auto &field_list_it : curfield ) {
        field_entry &cur = field_list_it.second;
        if( !cur.isAlive() ) {
            continue;
        }

        switch( cur.getFieldType() ) {
            case fd_null:
            case fd_blood: // It doesn't actually do anything
            case fd_bile:  // Ditto
                break;

            case fd_web:
                if( !z.has_flag( MF_WEBWALK ) ) {
                    z.add_effect( effect_webbed, 1_turns, num_bp, true, cur.getFieldDensity() );
                    cur.setFieldDensity( 0 );
                }
                break;

            case fd_acid:
                if( !z.has_flag( MF_FLIES ) ) {
                    const int d = rng( cur.getFieldDensity(), cur.getFieldDensity() * 3 );
                    z.deal_damage( nullptr, bp_torso, damage_instance( DT_ACID, d ) );
                    z.check_dead_state();
                }
                break;

            case fd_sap:
                z.moves -= cur.getFieldDensity() * 5;
                cur.setFieldDensity( cur.getFieldDensity() - 1 );
                break;

            case fd_sludge:
                if( !z.has_flag( MF_DIGS ) && !z.has_flag( MF_FLIES ) &&
                    !z.has_flag( MF_SLUDGEPROOF ) ) {
                    z.moves -= cur.getFieldDensity() * 300;
                    cur.setFieldDensity( 0 );
                }
                break;

            // MATERIALS-TODO: Use fire resistance
            case fd_fire:
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

                if( cur.getFieldDensity() == 1 ) {
                    dam += rng( 2, 6 );
                } else if( cur.getFieldDensity() == 2 ) {
                    dam += rng( 6, 12 );
                    if( !z.has_flag( MF_FLIES ) ) {
                        z.moves -= 20;
                        if( dam > 0 ) {
                            z.add_effect( effect_onfire, 1_turns * rng( dam / 2, dam * 2 ) );
                        }
                    }
                } else if( cur.getFieldDensity() == 3 ) {
                    dam += rng( 10, 20 );
                    if( !z.has_flag( MF_FLIES ) || one_in( 3 ) ) {
                        z.moves -= 40;
                        if( dam > 0 ) {
                            z.add_effect( effect_onfire, 1_turns * rng( dam / 2, dam * 2 ) );
                        }
                    }
                }
                // Drop through to smoke no longer needed as smoke will exist in the same square now,
                // this would double apply otherwise.
                break;

            case fd_smoke:
                if( !z.has_flag( MF_NO_BREATHE ) ) {
                    if( cur.getFieldDensity() == 3 ) {
                        z.moves -= rng( 10, 20 );
                    }
                    if( z.made_of( material_id( "veggy" ) ) ) { // Plants suffer from smoke even worse
                        z.moves -= rng( 1, cur.getFieldDensity() * 12 );
                    }
                }
                break;

            case fd_tear_gas:
                if( z.made_of_any( Creature::cmat_fleshnveg ) && !z.has_flag( MF_NO_BREATHE ) ) {
                    if( cur.getFieldDensity() == 3 ) {
                        z.add_effect( effect_stunned, rng( 1_minutes, 2_minutes ) );
                        dam += rng( 4, 10 );
                    } else if( cur.getFieldDensity() == 2 ) {
                        z.add_effect( effect_stunned, rng( 5_turns, 10_turns ) );
                        dam += rng( 2, 5 );
                    } else {
                        z.add_effect( effect_stunned, rng( 1_turns, 5_turns ) );
                    }
                    if( z.made_of( material_id( "veggy" ) ) ) {
                        z.moves -= rng( cur.getFieldDensity() * 5, cur.getFieldDensity() * 12 );
                        dam += cur.getFieldDensity() * rng( 8, 14 );
                    }
                    if( z.has_flag( MF_SEES ) ) {
                        z.add_effect( effect_blind, cur.getFieldDensity() * 8_turns );
                    }
                }
                break;

            case fd_relax_gas:
                if( z.made_of_any( Creature::cmat_fleshnveg ) && !z.has_flag( MF_NO_BREATHE ) ) {
                    z.add_effect( effect_stunned, rng( cur.getFieldDensity() * 4_turns,
                                                       cur.getFieldDensity() * 8_turns ) );
                }
                break;

            case fd_dazzling:
                if( z.has_flag( MF_SEES ) && !z.has_flag( MF_ELECTRONIC ) ) {
                    z.add_effect( effect_blind, cur.getFieldDensity() * 12_turns );
                    z.add_effect( effect_stunned, cur.getFieldDensity() * rng( 5_turns, 12_turns ) );
                }
                break;

            case fd_toxic_gas:
                if( !z.has_flag( MF_NO_BREATHE ) ) {
                    dam += cur.getFieldDensity();
                    z.moves -= cur.getFieldDensity();
                }
                break;

            case fd_nuke_gas:
                if( !z.has_flag( MF_NO_BREATHE ) ) {
                    if( cur.getFieldDensity() == 3 ) {
                        z.moves -= rng( 60, 120 );
                        dam += rng( 30, 50 );
                    } else if( cur.getFieldDensity() == 2 ) {
                        z.moves -= rng( 20, 50 );
                        dam += rng( 10, 25 );
                    } else {
                        z.moves -= rng( 0, 15 );
                        dam += rng( 0, 12 );
                    }
                    if( z.made_of( material_id( "veggy" ) ) ) {
                        z.moves -= rng( cur.getFieldDensity() * 5, cur.getFieldDensity() * 12 );
                        dam *= cur.getFieldDensity();
                    }
                }
                break;

            // MATERIALS-TODO: Use fire resistance
            case fd_flame_burst:
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
                break;

            case fd_electricity:
                // We don't want to increase dam, but deal a separate hit so that it can apply effects
                z.deal_damage( nullptr, bp_torso,
                               damage_instance( DT_ELECTRIC, rng( 1, cur.getFieldDensity() * 3 ) ) );
                break;

            case fd_fatigue:
                if( rng( 0, 2 ) < cur.getFieldDensity() ) {
                    dam += cur.getFieldDensity();
                    int tries = 0;
                    tripoint newpos = z.pos();
                    do {
                        newpos.x = rng( z.posx() - SEEX, z.posx() + SEEX );
                        newpos.y = rng( z.posy() - SEEY, z.posy() + SEEY );
                        tries++;
                    } while( impassable( newpos ) && tries != 10 );

                    if( tries == 10 ) {
                        z.die_in_explosion( nullptr );
                    } else if( monster *const other = g->critter_at<monster>( newpos ) ) {
                        if( g->u.sees( z ) ) {
                            add_msg( _( "The %1$s teleports into a %2$s, killing them both!" ),
                                     z.name().c_str(), other->name().c_str() );
                        }
                        other->die_in_explosion( &z );
                    } else {
                        z.setpos( newpos );
                    }
                }
                break;

            case fd_incendiary:
                // MATERIALS-TODO: Use fire resistance
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

                if( cur.getFieldDensity() == 1 ) {
                    dam += rng( 2, 6 );
                } else if( cur.getFieldDensity() == 2 ) {
                    dam += rng( 6, 12 );
                    z.moves -= 20;
                    if( !z.made_of( LIQUID ) && !z.made_of_any( Creature::cmat_flameres ) ) {
                        z.add_effect( effect_onfire, rng( 8_turns, 12_turns ) );
                    }
                } else if( cur.getFieldDensity() == 3 ) {
                    dam += rng( 10, 20 );
                    z.moves -= 40;
                    if( !z.made_of( LIQUID ) && !z.made_of_any( Creature::cmat_flameres ) ) {
                        z.add_effect( effect_onfire, rng( 12_turns, 16_turns ) );
                    }
                }
                break;

            case fd_fungal_haze:
                if( !z.type->in_species( FUNGUS ) &&
                    !z.type->has_flag( "NO_BREATHE" ) &&
                    !z.make_fungus() ) {
                    // Don't insta-kill jabberwocks, that's silly
                    const int density = cur.getFieldDensity();
                    z.moves -= rng( 10 * density, 30 * density );
                    dam += rng( 0, 10 * density );
                }
                break;

            case fd_fungicidal_gas:
                if( z.type->in_species( FUNGUS ) ) {
                    const int density = cur.getFieldDensity();
                    z.moves -= rng( 10 * density, 30 * density );
                    dam += rng( 4, 7 * density );
                }
                break;

            default:
                //Suppress warnings
                break;
        }
    }

    if( dam > 0 ) {
        z.apply_damage( nullptr, bp_torso, dam, true );
        z.check_dead_state();
    }
}

int field_entry::move_cost() const
{
    return fieldlist[type].move_cost[ getFieldDensity() - 1 ];
}

nc_color field_entry::color() const
{
    return fieldlist[type].color[density - 1];
}

char field_entry::symbol() const
{
    return fieldlist[type].sym;
}

field_id field_entry::getFieldType() const
{
    return type;
}

int field_entry::getFieldDensity() const
{
    return density;
}

time_duration field_entry::getFieldAge() const
{
    return age;
}

field_id field_entry::setFieldType( const field_id new_field_id )
{

    //TODO: Better bounds checking.
    if( new_field_id >= 0 && new_field_id < num_fields ) {
        type = new_field_id;
    } else {
        type = fd_null;
    }

    return type;

}

int field_entry::setFieldDensity( const int new_density )
{
    is_alive = new_density > 0;
    return density = std::max( std::min( new_density, MAX_FIELD_DENSITY ), 1 );
}

time_duration field_entry::setFieldAge( const time_duration &new_age )
{
    return age = new_age;
}

field::field()
    : draw_symbol( fd_null )
{
}

/*
Function: findField
Returns a field entry corresponding to the field_id parameter passed in. If no fields are found then returns NULL.
Good for checking for existence of a field: if(myfield.findField(fd_fire)) would tell you if the field is on fire.
*/
field_entry *field::findField( const field_id field_to_find )
{
    const auto it = field_list.find( field_to_find );
    if( it != field_list.end() ) {
        return &it->second;
    }
    return nullptr;
}

const field_entry *field::findFieldc( const field_id field_to_find ) const
{
    const auto it = field_list.find( field_to_find );
    if( it != field_list.end() ) {
        return &it->second;
    }
    return nullptr;
}

const field_entry *field::findField( const field_id field_to_find ) const
{
    return findFieldc( field_to_find );
}

/*
Function: addfield
Inserts the given field_id into the field list for a given tile if it does not already exist.
Returns false if the field_id already exists, true otherwise.
If the field already exists, it will return false BUT it will add the density/age to the current values for upkeep.
If you wish to modify an already existing field use findField and modify the result.
Density defaults to 1, and age to 0 (permanent) if not specified.
*/
bool field::addField( const field_id field_to_add, const int new_density,
                      const time_duration &new_age )
{
    auto it = field_list.find( field_to_add );
    if( fieldlist[field_to_add].priority >= fieldlist[draw_symbol].priority ) {
        draw_symbol = field_to_add;
    }
    if( it != field_list.end() ) {
        //Already exists, but lets update it. This is tentative.
        it->second.setFieldDensity( it->second.getFieldDensity() + new_density );
        return false;
    }
    field_list[field_to_add] = field_entry( field_to_add, new_density, new_age );
    return true;
}

bool field::removeField( field_id const field_to_remove )
{
    const auto it = field_list.find( field_to_remove );
    if( it == field_list.end() ) {
        return false;
    }
    removeField( it );
    return true;
}

void field::removeField( std::map<field_id, field_entry>::iterator const it )
{
    field_list.erase( it );
    if( field_list.empty() ) {
        draw_symbol = fd_null;
    } else {
        draw_symbol = fd_null;
        for( auto &fld : field_list ) {
            if( fieldlist[fld.first].priority >= fieldlist[draw_symbol].priority ) {
                draw_symbol = fld.first;
            }
        }
    }
}

/*
Function: fieldCount
Returns the number of fields existing on the current tile.
*/
unsigned int field::fieldCount() const
{
    return field_list.size();
}

std::map<field_id, field_entry>::iterator field::begin()
{
    return field_list.begin();
}

std::map<field_id, field_entry>::const_iterator field::begin() const
{
    return field_list.begin();
}

std::map<field_id, field_entry>::iterator field::end()
{
    return field_list.end();
}

std::map<field_id, field_entry>::const_iterator field::end() const
{
    return field_list.end();
}

std::string field_t::name( const int density ) const
{
    const std::string &n = untranslated_name[std::min( std::max( 0, density ), MAX_FIELD_DENSITY - 1 )];
    return n.empty() ? n : _( n.c_str() );
}

/*
Function: fieldSymbol
Returns the last added field from the tile for drawing purposes.
*/
field_id field::fieldSymbol() const
{
    return draw_symbol;
}

int field::move_cost() const
{
    int current_cost = 0;
    for( auto &fld : field_list ) {
        current_cost += fld.second.move_cost();
    }
    return current_cost;
}

bool field_type_dangerous( field_id id )
{
    const field_t &ft = fieldlist[id];
    return ft.dangerous[0] || ft.dangerous[1] || ft.dangerous[2];
}

void map::emit_field( const tripoint &pos, const emit_id &src, float mul )
{
    if( !src.is_valid() ) {
        return;
    }

    float chance = src->chance() * mul;
    if( src.is_valid() &&  x_in_y( chance, 100 ) ) {
        int qty = chance > 100.0f ? roll_remainder( src->qty() * chance / 100.0f ) : src->qty();
        propagate_field( pos, src->field(), qty, src->density() );
    }
}

void map::propagate_field( const tripoint &center, const field_id type, int amount,
                           int max_density )
{
    using gas_blast = std::pair<float, tripoint>;
    std::priority_queue<gas_blast, std::vector<gas_blast>, pair_greater_cmp_first> open;
    std::set<tripoint> closed;
    open.push( { 0.0f, center } );

    const bool not_gas = fieldlist[ type ].phase != GAS;

    while( amount > 0 && !open.empty() ) {
        if( closed.count( open.top().second ) ) {
            open.pop();
            continue;
        }

        // All points with equal gas density should propagate at the same time
        std::list<gas_blast> gas_front;
        gas_front.push_back( open.top() );
        int cur_intensity = get_field_strength( open.top().second, type );
        open.pop();
        while( !open.empty() && get_field_strength( open.top().second, type ) == cur_intensity ) {
            if( closed.count( open.top().second ) == 0 ) {
                gas_front.push_back( open.top() );
            }

            open.pop();
        }

        int increment = std::max<int>( 1, amount / gas_front.size() );

        while( amount > 0 && !gas_front.empty() ) {
            auto gp = random_entry_removed( gas_front );
            closed.insert( gp.second );
            int cur_strength = get_field_strength( gp.second, type );
            if( cur_strength < max_density ) {
                int bonus = std::min( max_density - cur_strength, increment );
                adjust_field_strength( gp.second, type, bonus );
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
