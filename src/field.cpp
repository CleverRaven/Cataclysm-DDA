#include "rng.h"
#include "map.h"
#include "debug.h"
#include "field.h"
#include "game.h"
#include "messages.h"
#include "translations.h"
#include "material.h"
#include "monster.h"
#include "npc.h"
#include "trap.h"
#include "itype.h"
#include "vehicle.h"
#include "submap.h"
#include "mapdata.h"
#include "mtype.h"

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

#define INBOUNDS(x, y) \
 (x >= 0 && x < SEEX * my_MAPSIZE && y >= 0 && y < SEEY * my_MAPSIZE)

field_t fieldlist[num_fields];

void game::init_fields()
{
    field_t tmp_fields[num_fields] =
    {
        {
            "fd_null",
            {"", "", ""}, '%', 0,
            {c_white, c_white, c_white}, {true, true, true}, {false, false, false}, 0,
            {0,0,0}
        },
        {
            "fd_blood",
            {_("blood splatter"), _("blood stain"), _("puddle of blood")}, '%', 0,
            {c_red, c_red, c_red}, {true, true, true}, {false, false, false}, HOURS(48),
            {0,0,0}
        },
        {
            "fd_bile",
            {_("bile splatter"), _("bile stain"), _("puddle of bile")}, '%', 0,
            {c_pink, c_pink, c_pink}, {true, true, true}, {false, false, false}, HOURS(24),
            {0,0,0}
        },

        {
            "fd_gibs_flesh",
            {_("scraps of flesh"), _("bloody meat chunks"), _("heap of gore")}, '~', 0,
            {c_brown, c_ltred, c_red}, {true, true, true}, {false, false, false}, HOURS(48),
            {0,0,0}
        },

        {
            "fd_gibs_veggy",
            {_("shredded leaves and twigs"), _("shattered branches and leaves"), _("broken vegetation tangle")}, '~', 0,
            {c_ltgreen, c_ltgreen, c_green}, {true, true, true}, {false, false, false}, HOURS(48),
            {0,0,0}
        },

        {
            "fd_web",
            {_("cobwebs"),_("webs"), _("thick webs")}, '}', 2,
            {c_white, c_white, c_white}, {true, true, false},{false, false, false}, 0,
            {0,0,0}
        },

        {
            "fd_slime",
            {_("slime trail"), _("slime stain"), _("puddle of slime")}, '%', 0,
            {c_ltgreen, c_ltgreen, c_green},{true, true, true},{false, false, false}, HOURS(24),
            {0,0,0}
        },

        {
            "fd_acid",
            {_("acid splatter"), _("acid streak"), _("pool of acid")}, '5', 2,
            {c_ltgreen, c_green, c_green}, {true, true, true}, {true, true, true}, MINUTES(2),
            {0,0,0}
        },

        {
            "fd_sap",
            {_("sap splatter"), _("glob of sap"), _("pool of sap")}, '5', 2,
            {c_yellow, c_brown, c_brown}, {true, true, true}, {true, true, true}, MINUTES(2),
            {0,0,0}
        },

        {
            "fd_sludge",
            {_("thin sludge trail"), _("sludge trail"), _("thick sludge trail")}, '5', 2,
            {c_ltgray, c_dkgray, c_black}, {true, true, true}, {false, false, false}, HOURS(6),
            {0,0,0}
        },

        {
            "fd_fire",
            {_("small fire"), _("fire"), _("raging fire")}, '4', 4,
            {c_yellow, c_ltred, c_red}, {true, true, true}, {true, true, true}, MINUTES(30),
            {0,0,0}
        },

       {
           "fd_rubble",
           {_("legacy rubble"), _("legacy rubble"), _("legacy rubble")}, '#', 0,
           {c_dkgray, c_dkgray, c_dkgray}, {true, true, true},{false, false, false},  1,
           {0,0,0}
       },

        {
            "fd_smoke",
            {_("thin smoke"), _("smoke"), _("thick smoke")}, '8', 8,
            {c_white, c_ltgray, c_dkgray}, {true, false, false},{false, true, true}, MINUTES(3),
            {0,0,0}
        },
        {
            "fd_toxic_gas",
            {_("hazy cloud"),_("toxic gas"),_("thick toxic gas")}, '8', 8,
            {c_white, c_ltgreen, c_green}, {true, false, false},{false, true, true}, MINUTES(90),
            {0,0,0}
        },

        {
            "fd_tear_gas",
            {_("hazy cloud"),_("tear gas"),_("thick tear gas")}, '8', 8,
            {c_white, c_yellow, c_brown}, {true, false, false},{true, true, true}, MINUTES(60),
            {0,0,0}
        },

        {
            "fd_nuke_gas",
            {_("hazy cloud"),_("radioactive gas"), _("thick radioactive gas")}, '8', 8,
            {c_white, c_ltgreen, c_green}, {true, true, false}, {true, true, true}, MINUTES(100),
            {0,0,0}
        },

        {
            "fd_gas_vent",
            {_("gas vent"), _("gas vent"), _("gas vent")}, '%', 0,
            {c_white, c_white, c_white}, {true, true, true}, {false, false, false}, 0,
            {0,0,0}
        },

        { // Fire Vents
            "fd_fire_vent",
            {"", "", ""}, '&', -1,
            {c_white, c_white, c_white}, {true, true, true}, {false, false, false}, 0,
            {0,0,0}
        },

        {
            "fd_flame_burst",
            {_("fire"), _("fire"), _("fire")}, '5', 4,
            {c_red, c_red, c_red}, {true, true, true}, {true, true, true}, 0,
            {0,0,0}
        },

        {
            "fd_electricity",
            {_("sparks"), _("electric crackle"), _("electric cloud")}, '9', 4,
            {c_white, c_cyan, c_blue}, {true, true, true}, {true, true, true}, 2,
            {0,0,0}
        },

        {
            "fd_fatigue",
            {_("odd ripple"), _("swirling air"), _("tear in reality")}, '*', 8,
            {c_ltgray, c_dkgray, c_magenta},{true, true, false},{false, false, false},  0,
            {0,0,0}
        },

        { //Push Items
            "fd_push_items",
            {"", "", ""}, '&', -1,
            {c_white, c_white, c_white}, {true, true, true}, {false, false, false}, 0,
            {0,0,0}
        },

        { // shock vents
            "fd_shock_vent",
            {"", "", ""}, '&', -1,
            {c_white, c_white, c_white}, {true, true, true}, {false, false, false}, 0,
            {0,0,0}
        },

        { // acid vents
            "fd_acid_vent",
            {"", "", ""}, '&', -1,
            {c_white, c_white, c_white}, {true, true, true}, {false, false, false}, 0,
            {0,0,0}
        },

        { // plasma glow (for plasma weapons)
            "fd_plasma",
            {_("faint plasma"), _("glowing plasma"), _("glaring plasma")}, '9', 4,
            {c_magenta, c_pink, c_white}, {true, true, true}, {false, false, false}, 2,
            {0,0,0}
        },

        { // laser beam (for laser weapons)
            "fd_laser",
            {_("faint glimmer"), _("beam of light"), _("intense beam of light")}, '#', 4,
            {c_blue, c_ltblue, c_white}, {true, true, true}, {false, false, false}, 1,
            {0,0,0}
        },
        {
            "fd_spotlight",
            { _("spotlight"), _("spotlight"), _("spotlight") }, '&', 1,
            {c_white, c_white, c_white}, { true, true, true }, { false, false, false }, 1,
            {0,0,0}
        },
        {
            "fd_dazzling",
            { _("dazzling"), _("dazzling"), _("dazzling") }, '#', 4,
            {c_ltred_yellow, c_ltred_yellow, c_ltred_yellow}, { true, true, true }, { false, false, false }, 1,
            { 0, 0, 0 }
        },
        {
            "fd_blood_veggy",
            {_("plant sap splatter"), _("plant sap stain"), _("puddle of resin")}, '%', 0,
            {c_ltgreen, c_ltgreen, c_ltgreen}, {true, true, true}, {false, false, false}, HOURS(48),
            {0,0,0}
        },
        {
            "fd_blood_insect",
            {_("bug blood splatter"), _("bug blood stain"), _("puddle of bug blood")}, '%', 0,
            {c_green, c_green, c_green}, {true, true, true}, {false, false, false}, HOURS(48),
            {0,0,0}
        },
        {
            "fd_blood_invertebrate",
            {_("hemolymph splatter"), _("hemolymph stain"), _("puddle of hemolymph")}, '%', 0,
            {c_ltgray, c_ltgray, c_ltgray}, {true, true, true}, {false, false, false}, HOURS(48),
            {0,0,0}
        },
        {
            "fd_gibs_insect",
            {_("shards of chitin"), _("shattered bug leg"), _("torn insect organs")}, '~', 0,
            {c_ltgreen, c_green, c_yellow}, {true, true, true}, {false, false, false}, HOURS(48),
            {0,0,0}
        },
        {
            "fd_gibs_invertebrate",
            {_("gooey scraps"), _("icky mess"), _("heap of squishy gore")}, '~', 0,
            {c_ltgray, c_ltgray, c_dkgray}, {true, true, true}, {false, false, false}, HOURS(48),
            {0,0,0}
        },
        {
            "fd_cigsmoke",
            {_("swirl of tobacco smoke"), _("tobacco smoke"), _("thick tobacco smoke")}, '%', 8,
            {c_white, c_ltgray, c_dkgray}, {true, true, true},{false, false, false}, MINUTES(35),
            {0,0,0}
        },
        {
            "fd_weedsmoke",
            {_("swirl of pot smoke"), _("pot smoke"), _("thick pot smoke")}, '%', 8,
            {c_white, c_ltgray, c_dkgray}, {true, true, true},{false, false, false},  325,
            {0,0,0}
        },

        {
            "fd_cracksmoke",
            {_("swirl of crack smoke"), _("crack smoke"), _("thick crack smoke")}, '%', 8,
            {c_white, c_ltgray, c_dkgray}, {true, true, true},{false, false, false},  225,
            {0,0,0}
        },
        {
            "fd_methsmoke",
            {_("swirl of meth smoke"), _("meth smoke"), _("thick meth smoke")}, '%', 8,
            {c_white, c_ltgray, c_dkgray}, {true, true, true},{false, false, false},  275,
            {0,0,0}
        },
        {
            "fd_bees",
            {_("some bees"), _("swarm of bees"), _("angry swarm of bees")}, '8', 8,
            {c_white, c_ltgray, c_dkgray}, {true, true, true},{true, true, true}, MINUTES(100),
            {0,0,0}
        },

        {
            "fd_incendiary",
            {_("smoke"),_("airborne incendiary"), _("airborne incendiary")}, '8', 8,
            {c_white, c_ltred, c_ltred_red}, {true, true, false}, {true, true, true}, MINUTES(50),
            {0,0,0}
        },

        {
            "fd_relax_gas",
            {_("hazy cloud"),_("sedative gas"),_("relaxation gas")}, '.', 8,
            { c_white, c_pink, c_cyan }, { true, true, true }, { false, true, true }, MINUTES(50),
            {0,0,0}
        },

        {
            "fd_fungal_haze",
            {_("hazy cloud"),_("fungal haze"),_("thick fungal haze")}, '.', 8,
            { c_white, c_cyan, c_cyan }, { true, true, false }, { true, true, true }, MINUTES(4),
            {0,0,0}
        },

        {
            "fd_hot_air1",
            {"", "", ""}, '&', -1,
            {c_white, c_yellow, c_red}, {true, true, true}, {false, false, false}, MINUTES(50),
            {0,0,0}
        },

        {
            "fd_hot_air2",
            {"", "", ""}, '&', -1,
            {c_white, c_yellow, c_red}, {true, true, true}, {false, false, false}, MINUTES(50),
            {0,0,0}
        },

        {
            "fd_hot_air3",
            {"", "", ""}, '&', -1,
            {c_white, c_yellow, c_red}, {true, true, true}, {false, false, false}, MINUTES(50),
            {0,0,0}
        },

        {
            "fd_hot_air4",
            {"", "", ""}, '&', -1,
            {c_white, c_yellow, c_red}, {true, true, true}, {false, false, false}, MINUTES(50),
            {0,0,0}
        },

        {
            "fd_fungicidal_gas",
            {_("hazy cloud"),_("fungicidal gas"),_("thick fungicidal gas")}, '8', 8,
            {c_white, c_ltgray, c_dkgray}, {true, true, false}, {false, true, true}, MINUTES(90),
            {0,0,0}
        }

    };
    for(int i = 0; i < num_fields; i++) {
        fieldlist[i] = tmp_fields[i];
    }
}

field_id field_from_ident(const std::string &field_ident)
{
    for( size_t i = 0; i < num_fields; i++) {
        if( fieldlist[i].id == field_ident ) {
            return static_cast<field_id>( i );
        }
    }
    debugmsg( "unknown field ident %s", field_ident.c_str() );
    return fd_null;
}

bool map::process_fields()
{
    bool dirty_transparency_cache = false;
    const int minz = zlevels ? -OVERMAP_DEPTH : abs_sub.z;
    const int maxz = zlevels ? OVERMAP_HEIGHT : abs_sub.z;
    bool zlev_dirty;
    for( int z = minz; z <= maxz; z++ ) {
        zlev_dirty = false;
        for( int x = 0; x < my_MAPSIZE; x++ ) {
            for( int y = 0; y < my_MAPSIZE; y++ ) {
                submap * const current_submap = get_submap_at_grid( x, y, z );
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

// A helper to turn neighbor index back into a tripoint
// Ugly, but can save time where it matters
tripoint offset_by_index( const size_t index, const tripoint &base )
{
    switch( index ) {
    case 0:
        return tripoint( base.x - 1, base.y - 1, base.z );
    case 1:
        return tripoint( base.x, base.y - 1, base.z );
    case 2:
        return tripoint( base.x + 1, base.y - 1, base.z );
    case 3:
        return tripoint( base.x - 1, base.y, base.z );
    case 4:
        return tripoint( base.x + 1, base.y, base.z );
    case 5:
        return tripoint( base.x - 1, base.y + 1, base.z );
    case 6:
        return tripoint( base.x - 1, base.y + 1, base.z );
    case 7:
        return tripoint( base.x - 1, base.y + 1, base.z );
    default:
        debugmsg( "offset_by_index got invalid index: %d", index );
        return tripoint_min;
    }
}

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
    const auto get_neighbors = [this]( const tripoint &pt ) {
        // Wrapper to allow skipping bound checks except at the edges of the map
        const auto maptile_has_bounds = [this]( const tripoint &pt, const bool bounds_checked ) {
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
            maptile_has_bounds( {pt.x - 1, pt.y - 1, pt.z}  , west && north ),
            maptile_has_bounds( {pt.x, pt.y - 1, pt.z}      , north ),
            maptile_has_bounds( {pt.x + 1, pt.y - 1, pt.z}  , east && north ),
            maptile_has_bounds( {pt.x - 1, pt.y, pt.z}      , west ),
            maptile_has_bounds( {pt.x + 1, pt.y, pt.z}      , east ),
            maptile_has_bounds( {pt.x - 1, pt.y + 1, pt.z}  , west && south ),
            maptile_has_bounds( {pt.x, pt.y + 1, pt.z}      , south ),
            maptile_has_bounds( {pt.x + 1, pt.y + 1, pt.z}  , east && south ),
        } };
    };

    const auto spread_gas = [this, &get_neighbors] (
        field_entry *cur, const tripoint &p, field_id curtype,
        int percent_spread, int outdoor_age_speedup ) {
        // Reset nearby scents to zero
        tripoint tmp;
        tmp.z = p.z;
        for( tmp.x = p.x - 1; tmp.x <= p.x + 1; tmp.x++ ) {
            for( tmp.y = p.y - 1; tmp.y <= p.y + 1; tmp.y++ ) {
                g->scent( tmp ) = 0;
            }
        }

        const int current_density = cur->getFieldDensity();
        const int current_age = cur->getFieldAge();
        // Dissipate faster outdoors.
        if( is_outside( p ) ) {
            cur->setFieldAge( current_age + outdoor_age_speedup );
        }

        // Bail out if we don't meet the spread chance or required density.
        if( current_density <= 1 || rng( 1, 100 ) > percent_spread ) {
            return;
        }

        const auto can_spread_to = [&]( const maptile &dst, field_id curtype ) {
            const field_entry* tmpfld = dst.get_field().findField( curtype );
            const auto &ter = dst.get_ter_t();
            const auto &frn = dst.get_furn_t();
            // Candidates are existing weaker fields or navigable/flagged tiles with no field.
            return ( ter_furn_movecost( ter, frn ) > 0 || ter_furn_has_flag( ter, frn, TFLAG_PERMEABLE ) ) &&
                ( tmpfld == nullptr || tmpfld->getFieldDensity() < cur->getFieldDensity() );
        };

        const auto spread_to = [&]( maptile &dst ) {
            field_entry *candidate_field = dst.find_field( curtype );
            // Nearby gas grows thicker, and ages are shared.
            int age_fraction = 0.5 + current_age / current_density;
            if ( candidate_field != nullptr ) {
                candidate_field->setFieldDensity( candidate_field->getFieldDensity() + 1 );
                cur->setFieldDensity( current_density - 1 );
                candidate_field->setFieldAge(candidate_field->getFieldAge() + age_fraction);
                cur->setFieldAge(current_age - age_fraction);
            // Or, just create a new field.
            } else if( dst.add_field( curtype, 1, 0 ) ) {
                dst.find_field( curtype )->setFieldAge(age_fraction);
                cur->setFieldDensity( current_density - 1 );
                cur->setFieldAge(current_age - age_fraction);
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
        const size_t end_it = (size_t)rng( 0, neighs.size() - 1 );
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
    const auto create_hot_air = [this] ( const tripoint &p, int density )
    {
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
            add_field( dst, hot_air, 1, 0 );
        }
    };

    // This should be true only when the field changes transparency
    // More correctly: not just when the field is opaque, but when it changes state
    // to a more/less transparent one, or creates a non-transparent field nearby
    bool dirty_transparency_cache = false;
    //Holds m.field_at(x,y).findField(fd_some_field) type returns.
    // Just to avoid typing that long string for a temp value.
    field_entry *tmpfld = nullptr;
    field_id curtype; //Holds cur->getFieldType() as thats what the old system used before rewrite.

    tripoint thep;
    thep.z = submap_z;

    // Initialize the map tile wrapper
    maptile map_tile( current_submap, 0, 0 );
    size_t &locx = map_tile.x;
    size_t &locy = map_tile.y;
    //Loop through all tiles in this submap indicated by current_submap
    for( locx = 0; locx < SEEX; locx++ ) {
        for( locy = 0; locy < SEEY; locy++ ) {
            // This is a translation from local coordinates to submap coords.
            // All submaps are in one long 1d array.
            thep.x = locx + submap_x * SEEX;
            thep.y = locy + submap_y * SEEY;
            // A const reference to the tripoint above, so that the code below doesn't accidentaly change it
            const tripoint &p = thep;
            // Get a reference to the field variable from the submap;
            // contains all the pointers to the real field effects.
            field &curfield = current_submap->fld[locx][locy];
            for( auto it = curfield.begin(); it != curfield.end();) {
                //Iterating through all field effects in the submap's field.
                field_entry * cur = &it->second;
                // The field might have been killed by processing a neighbour field
                if( !cur->isAlive() ) {
                    if( !fieldlist[cur->getFieldType()].transparent[cur->getFieldDensity() - 1] ) {
                        dirty_transparency_cache = true;
                    }
                    current_submap->field_count--;
                    curfield.removeField( it++ );
                    continue;
                }

                curtype = cur->getFieldType();
                // Again, legacy support in the event someone Mods setFieldDensity to allow more values.
                if (cur->getFieldDensity() > 3 || cur->getFieldDensity() < 1) {
                    debugmsg("Whoooooa density of %d", cur->getFieldDensity());
                }

                // Don't process "newborn" fields. This gives the player time to run if they need to.
                if( cur->getFieldAge() == 0 ) {
                    curtype = fd_null;
                }

                int part;
                vehicle *veh;
                switch (curtype) {
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
                            cur->setFieldAge( cur->getFieldAge() + 250 );
                        }
                        break;

                    case fd_acid:
                    {
                        const auto &ter = map_tile.get_ter_t();
                        if( ter.has_flag( TFLAG_SWIMMABLE ) ) { // Dissipate faster in water
                            cur->setFieldAge( cur->getFieldAge() + 20 );
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
                                dst_tile.add_field( fd_acid, cur->getFieldDensity(), cur->getFieldAge() );
                            } else {
                                // Math can be a bit off,
                                // but "boiling" falling acid can be allowed to be stronger
                                // than acid that just lies there
                                const int sum_density = cur->getFieldDensity() + acid_there->getFieldDensity();
                                const int new_density = std::min( 3, sum_density );
                                // No way to get precise elapsed time, let's always reset
                                // Allow falling acid to last longer than regular acid to show it off
                                const int new_age = -MINUTES( sum_density - new_density );
                                acid_there->setFieldDensity( new_density );
                                acid_there->setFieldAge( new_age );
                            }

                            // Set ourselves up for removal
                            cur->setFieldDensity( 0 );
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
                        if( g->scent( p ) < cur->getFieldDensity() * 10 ) {
                            g->scent( p ) = cur->getFieldDensity() * 10;
                        }
                        break;
                    case fd_plasma:
                        dirty_transparency_cache = true;
                        break;
                    case fd_laser:
                        dirty_transparency_cache = true;
                        break;

                        // TODO-MATERIALS: use fire resistance
                    case fd_fire:
                    {
                        // Entire objects for ter/frn for flags, but only id for trp
                        // because the only trap we're checking for is brazier
                        const auto &ter = map_tile.get_ter_t();
                        const auto &frn = map_tile.get_furn_t();

                        const auto &trp = map_tile.get_trap();
                        // We've got ter/furn cached, so let's use that
                        const bool is_sealed = ter_furn_has_flag( ter, frn, TFLAG_SEALED ) &&
                                               !ter_furn_has_flag( ter, frn, TFLAG_ALLOW_FIELD_EFFECT );
                        // Volume, Smoke generation probability, consumed items count
                        int vol = 0, smoke = 0, consumed = 0;
                        // How much time to add to the fire's life due to burned items/terrain/furniture
                        int time_added = 0;
                        // The huge indent below should probably be somehow moved away from here
                        // without forcing the function to use i_at( p ) for fires without items
                        if( !is_sealed && map_tile.get_item_count() > 0 ) {
                            auto items_here = i_at( p );
                            // explosions will destroy items on this square, iterating
                            // backwards makes sure that every item is visited.
                            for( auto explosive = items_here.begin(); explosive != items_here.end(); ) {
                                if( explosive->type->explode_in_fire() ) {
                                    // Make a copy and let the copy explode.
                                    item tmp = *explosive;
                                    i_rem( p, explosive );
                                    tmp.detonate( p );
                                    // Just restart from the beginning.
                                    explosive = items_here.begin();
                                } else {
                                    ++explosive;
                                }
                            }

                            std::vector<item> new_content;
                            // Consume items as fuel to help us grow/last longer.
                            bool destroyed = false; //Is the item destroyed?
                            // The highest # of items this fire can remove in one turn
                            int max_consume = cur->getFieldDensity() * 2;
                            for( auto fuel = items_here.begin(); fuel != items_here.end() && consumed < max_consume; ) {
                                // Stop when we hit the end of the item buffer OR we consumed
                                // more than max_consume items
                                destroyed = false;
                                // Used to feed the fire based on volume of item burnt.
                                vol = fuel->volume();
                                const islot_ammo *ammo_type = nullptr; //Special case if its ammo.

                                if( fuel->is_ammo() ) {
                                    ammo_type = fuel->type->ammo.get();
                                }
                                // Types of ammo with special effects.
                                bool cookoff = false;
                                bool special = false;
                                //Flame type ammo removed so gasoline isn't explosive, it just burns.
                                if( ammo_type != nullptr &&
                                    ( !fuel->made_of("hydrocarbons") && !fuel->made_of("oil") ) ) {
                                    cookoff = ammo_type->ammo_effects.count("INCENDIARY") ||
                                              ammo_type->ammo_effects.count("COOKOFF");
                                    special = ammo_type->ammo_effects.count("FRAG") ||
                                              ammo_type->ammo_effects.count("NAPALM") ||
                                              ammo_type->ammo_effects.count("NAPALM_BIG") ||
                                              ammo_type->ammo_effects.count("EXPLOSIVE_SMALL") ||
                                              ammo_type->ammo_effects.count("EXPLOSIVE") ||
                                              ammo_type->ammo_effects.count("EXPLOSIVE_BIG") ||
                                              ammo_type->ammo_effects.count("EXPLOSIVE_HUGE") ||
                                              ammo_type->ammo_effects.count("TOXICGAS") ||
                                              ammo_type->ammo_effects.count("TEARGAS") ||
                                              ammo_type->ammo_effects.count("SMOKE") ||
                                              ammo_type->ammo_effects.count("SMOKE_BIG") ||
                                              ammo_type->ammo_effects.count("FLASHBANG");
                                }

                                // How much more burnt the item will be,
                                // should be a multiple of 'base_burn_amt'.
                                int burn_amt = 0;
                                // 'burn_amt' / 'base_burn_amt' == 1 to 'consumed',
                                // Right now all materials are 1, except paper, which is 3
                                // This means paper is consumed 3x as fast
                                int base_burn_amt = 1;

                                if( special || cookoff ) {
                                    int charges_remaining = fuel->charges;
                                    const long rounds_exploded = rng( 1, charges_remaining );
                                    // Yank the exploding item off the map for the duration of the explosion
                                    // so it doesn't blow itself up.
                                    item temp_item = *fuel;
                                    items_here.erase( fuel );
                                    // cook off ammo instead of just burning it.
                                    for(int j = 0; j < (rounds_exploded / 10) + 1; j++) {
                                        if( cookoff ) {
                                            // Ammo that cooks off, but doesn't have a
                                            // large intrinsic effect blows up with half
                                            // the ammos damage in force, for each bullet,
                                            // just creating shrapnel.
                                            g->explosion( p, ammo_type->damage / 2, 0.5f, 1 );
                                        } else if( special ) {
                                            // If it has a special effect just trigger it.
                                            ammo_effects( p, ammo_type->ammo_effects );
                                        }
                                    }
                                    charges_remaining -= rounds_exploded;
                                    if( charges_remaining > 0 ) {
                                        temp_item.charges = charges_remaining;
                                        items_here.push_back( temp_item );
                                    }
                                    // Can't find an easy way to handle reinserting the ammo into a potentially
                                    // invalidated list and continuing iteration, so just bail out.
                                    break;
                                } else if( fuel->made_of("paper") ) {
                                    //paper items feed the fire moderately.
                                    base_burn_amt = 3;
                                    burn_amt = base_burn_amt * (max_consume - consumed);
                                    if (cur->getFieldDensity() == 1) {
                                        time_added += vol * 10;
                                        time_added += (vol * 10) * (burn_amt / base_burn_amt);
                                    }
                                    if( vol >= 4 ) {
                                        smoke++;    //Large paper items give chance to smoke.
                                    }

                                } else if( fuel->made_of("wood") || fuel->made_of("veggy") ) {
                                    //Wood or vegy items burn slowly.
                                    if (vol <= cur->getFieldDensity() * 10 ||
                                        cur->getFieldDensity() == 3) {
                                        // A single wood item will just maintain at the current level.
                                        time_added += 1;
                                        // ammo has more surface area, and burns quicker
                                        if (one_in( (ammo_type != NULL) ? 25 : 50 )) {
                                            burn_amt = cur->getFieldDensity();
                                        }
                                    } else if( fuel->burnt < cur->getFieldDensity() ) {
                                        burn_amt = 1;
                                    }
                                    smoke++;

                                } else if( (fuel->made_of("cotton") || fuel->made_of("wool")) &&
                                           !fuel->made_of("nomex") ) {
                                    //Cotton and wool moderately quickly but don't feed the fire much.
                                    if( vol <= 5 || cur->getFieldDensity() > 1 ) {
                                        time_added += 1;
                                        burn_amt = cur->getFieldDensity();
                                    } else if( x_in_y( cur->getFieldDensity(), fuel->burnt ) ) {
                                        burn_amt = 1;
                                    }
                                    smoke++;

                                } else if( fuel->made_of("flesh") || fuel->made_of("hflesh") ||
                                           fuel->made_of("iflesh") ) {
                                    // Slow and smokey
                                    if( one_in( vol / 50 / cur->getFieldDensity() ) ) {
                                        time_added += 1;
                                        burn_amt = cur->getFieldDensity();
                                        smoke += 3 * cur->getFieldDensity();
                                    } else if( x_in_y( cur->getFieldDensity(), fuel->burnt ) ) {
                                        time_added += 1;
                                        burn_amt = 1;
                                        smoke++;
                                    }

                                } else if( fuel->made_of(LIQUID) ) {
                                    // Lots of smoke if alcohol, and LOTS of fire fueling power
                                    if( fuel->made_of("hydrocarbons") ) {
                                        time_added += 300;
                                        smoke += 6;
                                    } else if( fuel->made_of("alcohol") && fuel->made_of().size() == 1 ) {
                                        // Only strong alcohol for now
                                        time_added += 250;
                                        smoke += 1;
                                    } else if( fuel->type->id == "lamp_oil" ) {
                                        time_added += 300;
                                        smoke += 3;
                                    } else {
                                        // kills a fire otherwise.
                                        time_added += -rng(80 * vol, 300 * vol);
                                        smoke++;
                                    }
                                    // burn_amt will get multiplied by stack size in item::burn
                                    burn_amt = cur->getFieldDensity();

                                } else if( fuel->made_of("powder") ) {
                                    // Any powder will fuel the fire as 100 times much as its volume
                                    // but be immediately destroyed.
                                    time_added += vol * 100;
                                    destroyed = true;
                                    smoke += 2;

                                } else if( fuel->made_of("plastic") && !fuel->made_of("nomex") ) {
                                    //Smokey material, doesn't fuel well.
                                    smoke += 3;
                                    if( fuel->burnt <= cur->getFieldDensity() * 2 ||
                                        (cur->getFieldDensity() == 3 && one_in(vol)) ) {
                                        burn_amt = cur->getFieldDensity();
                                        if( one_in( fuel->burnt ) ) {
                                            time_added += 1;
                                        }
                                    }
                                } else if( !fuel->made_of("nomex") ) {
                                    // Generic materials, like bone, wheat or fruit
                                    // Just damage and smoke, don't feed the fire
                                    int best_res = 0;
                                    for( auto mat : fuel->made_of_types() ) {
                                        best_res = std::max( best_res, mat->fire_resist() );
                                    }
                                    if( best_res < cur->getFieldDensity() && one_in( fuel->volume() ) ) {
                                        smoke++;
                                        burn_amt = cur->getFieldDensity() - best_res;
                                    }
                                }
                                if( !destroyed ) {
                                    destroyed = fuel->burn( burn_amt );
                                }

                                if( destroyed ) {
                                    //If we decided the item was destroyed by fire, remove it.
                                    new_content.insert( new_content.end(),
                                                        fuel->contents.begin(), fuel->contents.end() );
                                    fuel = items_here.erase( fuel );
                                } else {
                                    ++fuel;
                                }
                            }

                            spawn_items( p, new_content );
                        }

                        //Get the part of the vehicle in the fire.
                        veh = veh_at_internal( p, part ); // _internal skips the boundary check
                        if( veh != nullptr ) {
                            veh->damage(part, cur->getFieldDensity() * 10, DT_HEAT, false);
                            //Damage the vehicle in the fire.
                        }
                        // If the flames are in a brazier, they're fully contained,
                        // so skip consuming terrain
                        if( tr_brazier != trp &&
                            !ter_furn_has_flag( ter, frn, TFLAG_FIRE_CONTAINER ) ) {

                            if( ter.has_flag( TFLAG_SWIMMABLE ) ) {
                                // Flames die quickly on water
                                cur->setFieldAge( cur->getFieldAge() + MINUTES(4) );
                            }

                            // Consume the terrain we're on
                            if( ter_furn_has_flag( ter, frn, TFLAG_FLAMMABLE ) ) {
                                // The fire feeds on the ground itself until max density.
                                time_added += 5 - cur->getFieldDensity();
                                smoke += 2;
                                if( cur->getFieldDensity() > 1 &&
                                    one_in( 200 - cur->getFieldDensity() * 50 ) ) {
                                    destroy( p, false );
                                }

                            } else if( ter_furn_has_flag( ter, frn, TFLAG_FLAMMABLE_HARD ) &&
                                       one_in( 3 ) ) {
                                // The fire feeds on the ground itself until max density.
                                time_added += 4 - cur->getFieldDensity();
                                smoke += 2;
                                if( cur->getFieldDensity() > 1 &&
                                    one_in( 200 - cur->getFieldDensity() * 50 ) ) {
                                    destroy( p, false );
                                }

                            } else if( ter_furn_has_flag( ter, frn, TFLAG_FLAMMABLE_ASH ) ) {
                                // The fire feeds on the ground itself until max density.
                                time_added += 5 - cur->getFieldDensity();
                                smoke += 2;
                                if( cur->getFieldDensity() > 1 &&
                                    one_in( 200 - cur->getFieldDensity() * 50 ) ) {
                                    ter_set( p, t_dirt );
                                    furn_set( p, f_ash );
                                }
                            } else if( ter.has_flag( TFLAG_NO_FLOOR ) && zlevels && p.z > -OVERMAP_DEPTH ) {
                                // We're hanging in the air - let's fall down
                                tripoint dst{p.x, p.y, p.z - 1};
                                if( valid_move( p, dst, true, true ) ) {
                                    maptile dst_tile = maptile_at_internal( dst );
                                    field_entry *fire_there = dst_tile.find_field( fd_fire );
                                    if( fire_there == nullptr ) {
                                        dst_tile.add_field( fd_fire, 1, 0 );
                                        cur->setFieldDensity( cur->getFieldDensity() - 1 );
                                    } else {
                                        // Don't fuel raging fires or they'll burn forever
                                        // as they can produce small fires above themselves
                                        int new_density = std::max( cur->getFieldDensity(),
                                                                    fire_there->getFieldDensity() );
                                        // Allow smaller fires to combine
                                        if( new_density < 3 &&
                                            cur->getFieldDensity() == fire_there->getFieldDensity() ) {
                                            new_density++;
                                        }
                                        fire_there->setFieldDensity( new_density );
                                        // A raging fire below us can support us for a while
                                        // Otherwise decay and decay fast
                                        if( new_density < 3 || one_in( 10 ) ) {
                                            cur->setFieldDensity( cur->getFieldDensity() - 1 );
                                        }
                                    }

                                    break;
                                }
                            }
                        }

                        // Lower age is a longer lasting fire
                        if( time_added != 0 ) {
                            cur->setFieldAge( cur->getFieldAge() - time_added );
                        } else {
                            // Nothing to burn = fire should be dying out faster
                            // Drain more power from big fires, so that they stop raging over nothing
                            cur->setFieldAge( cur->getFieldAge() + 2 * cur->getFieldDensity() );
                        }

                        // Below we will access our nearest 8 neighbors, so let's cache them now
                        // This should probably be done more globally, because large fires will re-do it a lot
                        auto neighs = get_neighbors( p );

                        // If the flames are in a pit, it can't spread to non-pit
                        const bool in_pit = ter.loadid == t_pit;

                        // Count adjacent fires, to optimize out needless smoke and hot air
                        int adjacent_fires = 0;

                        // If the flames are big, they contribute to adjacent flames
                        if( tr_brazier != trp &&
                            !ter_furn_has_flag( ter, frn, TFLAG_FIRE_CONTAINER ) ) {
                            if( cur->getFieldDensity() > 1 && one_in( 3 ) ) {
                                // Basically: Scan around for a spot,
                                // if there is more fire there, make it bigger and give it some fuel.
                                // This is how big fires spend their excess age:
                                // making other fires bigger. Flashpoint.
                                const size_t end_it = (size_t)rng( 0, neighs.size() - 1 );
                                for( size_t i = ( end_it + 1 ) % neighs.size();
                                     i != end_it && cur->getFieldAge() < 0;
                                     i = ( i + 1 ) % neighs.size() ) {
                                    maptile &dst = neighs[i];
                                    auto dstfld = dst.find_field( fd_fire );
                                    // If the fire exists and is weaker than ours, boost it
                                    if( dstfld != nullptr &&
                                        ( dstfld->getFieldDensity() <= cur->getFieldDensity() ||
                                          dstfld->getFieldAge() > cur->getFieldAge() ) &&
                                        ( in_pit == ( dst.get_ter() == t_pit) ) ) {
                                        if( dstfld->getFieldDensity() < 2 ) {
                                            dstfld->setFieldDensity(dstfld->getFieldDensity() + 1);
                                        }

                                        dstfld->setFieldAge( dstfld->getFieldAge() - MINUTES(5) );
                                        cur->setFieldAge( cur->getFieldAge() + MINUTES(5) );
                                    }

                                    if( dstfld != nullptr ) {
                                        adjacent_fires++;
                                    }
                                }
                            } else if( cur->getFieldAge() < 0 && cur->getFieldDensity() < 3 ) {
                                // See if we can grow into a stage 2/3 fire, for this
                                // burning neighbours are necessary in addition to
                                // field age < 0, or alternatively, a LOT of fuel.

                                // The maximum fire density is 1 for a lone fire, 2 for at least 1 neighbour,
                                // 3 for at least 2 neighbours.
                                int maximum_density =  1;

                                // The following logic looks a bit complex due to optimization concerns, so here are the semantics:
                                // 1. Calculate maximum field density based on fuel, -50 minutes is 2(medium), -500 minutes is 3(raging)
                                // 2. Calculate maximum field density based on neighbours, 3 neighbours is 2(medium), 7 or more neighbours is 3(raging)
                                // 3. Pick the higher maximum between 1. and 2.
                                if( cur->getFieldAge() < -MINUTES(500) ) {
                                    maximum_density = 3;
                                } else {
                                    for( size_t i = 0; i < neighs.size(); i++ ) {
                                        if( neighs[i].get_field().findField( fd_fire ) != nullptr ) {
                                            adjacent_fires++;
                                        }
                                    }
                                    maximum_density = 1 + (adjacent_fires >= 3) + (adjacent_fires >= 7);

                                    if( maximum_density < 2 && cur->getFieldAge() < -MINUTES(50) ) {
                                        maximum_density = 2;
                                    }
                                }

                                // If we consumed a lot, the flames grow higher
                                if( cur->getFieldDensity() < maximum_density && cur->getFieldAge() < 0 ) {
                                    // Fires under 0 age grow in size. Level 3 fires under 0 spread later on.
                                    // Weaken the newly-grown fire
                                    cur->setFieldDensity( cur->getFieldDensity() + 1 );
                                    cur->setFieldAge( cur->getFieldAge() + MINUTES( cur->getFieldDensity() * 10 ) );
                                }
                            }
                        }

                        // Consume adjacent fuel / terrain / webs to spread.
                        // Allow raging fires (and only raging fires) to spread up
                        // Spreading down is achieved by wrecking the walls/floor and then falling
                        if( zlevels && cur->getFieldDensity() == 3 && p.z < OVERMAP_HEIGHT ) {
                            // Let it burn through the floor
                            maptile dst = maptile_at_internal( {p.x, p.y, p.z + 1} );
                            const auto &dst_ter = dst.get_ter_t();
                            if( dst_ter.has_flag( TFLAG_NO_FLOOR ) ||
                                dst_ter.has_flag( TFLAG_FLAMMABLE ) ||
                                dst_ter.has_flag( TFLAG_FLAMMABLE_ASH ) ||
                                dst_ter.has_flag( TFLAG_FLAMMABLE_HARD ) ) {
                                field_entry *nearfire = dst.find_field( fd_fire );
                                if( nearfire != nullptr ) {
                                    nearfire->setFieldAge( nearfire->getFieldAge() - MINUTES(2) );
                                } else {
                                    dst.add_field( fd_fire, 1, 0 );
                                }
                                // Fueling fires above doesn't cost fuel
                            }
                        }

                        // Our iterator will start at end_i + 1 and increment from there and then wrap around.
                        // This guarantees it will check all neighbors, starting from a random one
                        const size_t end_i = (size_t)rng( 0, neighs.size() - 1 );
                        for( size_t i = ( end_i + 1 ) % neighs.size();
                             i != end_i; i = ( i + 1 ) % neighs.size() ) {
                            if( one_in( cur->getFieldDensity() * 2 ) ) {
                                // Skip some processing to save on CPU
                                continue;
                            }

                            maptile &dst = neighs[i];
                            // No bounds checking here: we'll treat the invalid neighbors as valid.
                            // We're using the maptile wrapper, so we can treat invalid tiles as sentinels.
                            // This will create small oddities on map edges, but nothing more noticeable than
                            // "cut-off" that happenes with bounds checks.

                            field_entry *nearfire = dst.find_field(fd_fire);
                            if( nearfire != nullptr ) {
                                // We handled supporting fires in the section above, no need to do it here
                                continue;
                            }

                            field_entry *nearwebfld = dst.find_field(fd_web);
                            int spread_chance = 25 * (cur->getFieldDensity() - 1);
                            if( nearwebfld != nullptr ) {
                                spread_chance = 50 + spread_chance / 2;
                            }

                            const auto &dster = dst.get_ter_t();
                            const auto &dsfrn = dst.get_furn_t();
                            // Allow weaker fires to spread occasionally
                            const int power = cur->getFieldDensity() + one_in( 5 );
                            if( rng(1, 100) < spread_chance && tr_brazier != trp &&
                                  !ter_furn_has_flag( ter, frn, TFLAG_FIRE_CONTAINER ) &&
                                  (in_pit == (dster.loadid == t_pit)) &&
                                  (
                                    (power >= 3 && cur->getFieldAge() < 0 && one_in( 20 ) ) ||
                                    (power >= 2 && ( ter_furn_has_flag( dster, dsfrn, TFLAG_FLAMMABLE ) && one_in(2) ) ) ||
                                    (power >= 2 && ( ter_furn_has_flag( dster, dsfrn, TFLAG_FLAMMABLE_ASH ) && one_in(2) ) ) ||
                                    (power >= 3 && ( ter_furn_has_flag( dster, dsfrn, TFLAG_FLAMMABLE_HARD ) && one_in(5) ) ) ||
                                    nearwebfld || ( dst.get_item_count() > 0 && flammable_items_at( offset_by_index( i, p ) ) && one_in(5) )
                                  ) ) {
                                dst.add_field( fd_fire, 1, 0 ); // Nearby open flammable ground? Set it on fire.
                                tmpfld = dst.find_field(fd_fire);
                                if( tmpfld != nullptr ) {
                                    // Make the new fire quite weak, so that it doesn't start jumping around instantly
                                    tmpfld->setFieldAge( MINUTES(2) );
                                    // Consume a bit of our fuel
                                    cur->setFieldAge( cur->getFieldAge() + MINUTES(1) );
                                }
                                if( nearwebfld ) {
                                    nearwebfld->setFieldDensity( 0 );
                                }
                            }
                        }

                        // Create smoke once - above us if possible, at us otherwise
                        if( !ter_furn_has_flag( ter, frn, TFLAG_SUPPRESS_SMOKE ) &&
                            rng(0, 100) <= smoke &&
                            rng(3, 35) < cur->getFieldDensity() * 10 ) {
                                bool smoke_up = zlevels && p.z < OVERMAP_HEIGHT;
                                if( smoke_up ) {
                                    tripoint up{p.x, p.y, p.z + 1};
                                    maptile dst = maptile_at_internal( up );
                                    const auto &dst_ter = dst.get_ter_t();
                                    if( dst_ter.has_flag( TFLAG_NO_FLOOR ) ) {
                                        dst.add_field( fd_smoke, rng( 1, cur->getFieldDensity() ), 0 );
                                    } else {
                                        // Can't create smoke above
                                        smoke_up = false;
                                    }
                                }

                                if( !smoke_up ) {
                                    maptile dst = maptile_at_internal( p );
                                    // Create thicker smoke
                                    dst.add_field( fd_smoke, cur->getFieldDensity(), 0 );
                                }

                                dirty_transparency_cache = true; // Smoke affects transparency
                            }

                        // Hot air is a heavy load on the CPU and it doesn't do much
                        // Don't produce too much of it if we have a lot fires nearby, they produce
                        // radiant heat which does what hot air would do anyway
                        if( rng( 0, adjacent_fires ) > 2 ) {
                            create_hot_air( p, cur->getFieldDensity() );
                        }
                    }
                    break;

                    case fd_smoke:
                        dirty_transparency_cache = true;
                        spread_gas( cur, p, curtype, 80, 50 );
                        break;

                    case fd_tear_gas:
                        dirty_transparency_cache = true;
                        spread_gas( cur, p, curtype, 33, 30 );
                        break;

                    case fd_relax_gas:
                        dirty_transparency_cache = true;
                        spread_gas( cur, p, curtype, 25, 50 );
                        break;

                    case fd_fungal_haze:
                        dirty_transparency_cache = true;
                        spread_gas( cur, p, curtype, 33,  5);
                        if( one_in( 10 - 2 * cur->getFieldDensity() ) ) {
                            g->spread_fungus( p ); //Haze'd terrain
                        }

                        break;

                    case fd_toxic_gas:
                        dirty_transparency_cache = true;
                        spread_gas( cur, p, curtype, 50, 30 );
                        break;

                    case fd_cigsmoke:
                        dirty_transparency_cache = true;
                        spread_gas( cur, p, curtype, 250, 65 );
                        break;

                    case fd_weedsmoke:
                    {
                        dirty_transparency_cache = true;
                        spread_gas( cur, p, curtype, 200, 60 );

                        if(one_in(20)) {
                            int npcdex = g->npc_at( p );
                            if (npcdex != -1) {
                                npc *p = g->active_npc[npcdex];
                                if(p->is_friend()) {
                                    p->say(one_in(10) ? _("Whew... smells like skunk!") : _("Man, that smells like some good shit!"));
                                }
                            }
                        }

                    }
                        break;

                    case fd_methsmoke:
                    {
                        dirty_transparency_cache = true;
                        spread_gas( cur, p, curtype, 175, 70 );

                        if(one_in(20)) {
                            int npcdex = g->npc_at( p );
                            if (npcdex != -1) {
                                npc *p = g->active_npc[npcdex];
                                if(p->is_friend()) {
                                    p->say(_("I don't know... should you really be smoking that stuff?"));
                                }
                            }
                        }
                    }
                        break;

                    case fd_cracksmoke:
                    {
                        dirty_transparency_cache = true;
                        spread_gas( cur, p, curtype, 175, 80 );

                        if(one_in(20)) {
                            int npcdex = g->npc_at( p );
                            if (npcdex != -1) {
                                npc *p = g->active_npc[npcdex];
                                if(p->is_friend()) {
                                    p->say(one_in(2) ? _("Ew, smells like burning rubber!") : _("Ugh, that smells rancid!"));
                                }
                            }
                        }
                    }
                        break;

                    case fd_nuke_gas:
                    {
                        dirty_transparency_cache = true;
                        int extra_radiation = rng(0, cur->getFieldDensity());
                        adjust_radiation( p, extra_radiation );
                        spread_gas( cur, p, curtype, 50, 10 );
                        break;
                    }

                    case fd_hot_air1:
                    case fd_hot_air2:
                    case fd_hot_air3:
                    case fd_hot_air4:
                        // No transparency cache wrecking here!
                        spread_gas( cur, p, curtype, 100, 1000 );
                        break;

                    case fd_gas_vent:
                    {
                        dirty_transparency_cache = true;
                        for( int i = -1; i <= 1; i++ ) {
                            for( int j = -1; j <= 1; j++ ) {
                                const tripoint pnt( p.x + i, p.y + j, p.z );
                                field &wandering_field = get_field( pnt );
                                tmpfld = wandering_field.findField(fd_toxic_gas);
                                if (tmpfld && tmpfld->getFieldDensity() < 3) {
                                    tmpfld->setFieldDensity(tmpfld->getFieldDensity() + 1);
                                } else {
                                    add_field( pnt, fd_toxic_gas, 3, 0 );
                                }
                            }
                        }
                    }
                        break;

                    case fd_fire_vent:
                        if (cur->getFieldDensity() > 1) {
                            if (one_in(3)) {
                                cur->setFieldDensity(cur->getFieldDensity() - 1);
                            }
                            create_hot_air( p, cur->getFieldDensity());
                        } else {
                            dirty_transparency_cache = true;
                            add_field( p, fd_flame_burst, 3, cur->getFieldAge() );
                            cur->setFieldDensity( 0 );
                        }
                        break;

                    case fd_flame_burst:
                        if (cur->getFieldDensity() > 1) {
                            cur->setFieldDensity(cur->getFieldDensity() - 1);
                            create_hot_air( p, cur->getFieldDensity());
                        } else {
                            dirty_transparency_cache = true;
                            add_field( p, fd_fire_vent, 3, cur->getFieldAge() );
                            cur->setFieldDensity( 0 );
                        }
                        break;

                    case fd_electricity:
                        if (!one_in(5)) {   // 4 in 5 chance to spread
                            std::vector<tripoint> valid;
                            if (impassable( p ) && cur->getFieldDensity() > 1) { // We're grounded
                                int tries = 0;
                                tripoint pnt;
                                pnt.z = p.z;
                                while (tries < 10 && cur->getFieldAge() < 50 && cur->getFieldDensity() > 1) {
                                    pnt.x = p.x + rng(-1, 1);
                                    pnt.y = p.y + rng(-1, 1);
                                    if( passable( pnt ) ) {
                                        add_field( pnt, fd_electricity, 1, cur->getFieldAge() + 1);
                                        cur->setFieldDensity(cur->getFieldDensity() - 1);
                                        tries = 0;
                                    } else {
                                        tries++;
                                    }
                                }
                            } else {    // We're not grounded; attempt to ground
                                for (int a = -1; a <= 1; a++) {
                                    for (int b = -1; b <= 1; b++) {
                                        tripoint dst( p.x + a, p.y + b, p.z );
                                        if( impassable( dst ) ) // Grounded tiles first

                                        {
                                            valid.push_back( dst );
                                        }
                                    }
                                }
                                if( valid.empty() ) {    // Spread to adjacent space, then
                                    tripoint dst( p.x + rng(-1, 1), p.y + rng(-1, 1), p.z );
                                    field_entry *elec = get_field( dst ).findField( fd_electricity );
                                    if( passable( dst ) && elec != nullptr &&
                                        elec->getFieldDensity() < 3) {
                                        elec->setFieldDensity( elec->getFieldDensity() + 1 );
                                        cur->setFieldDensity(cur->getFieldDensity() - 1);
                                    } else if( passable( dst ) ) {
                                        add_field( dst, fd_electricity, 1, cur->getFieldAge() + 1 );
                                    }
                                    cur->setFieldDensity(cur->getFieldDensity() - 1);
                                }
                                while( !valid.empty() && cur->getFieldDensity() > 1 ) {
                                    const tripoint target = random_entry_removed( valid );
                                    add_field(target, fd_electricity, 1, cur->getFieldAge() + 1);
                                    cur->setFieldDensity(cur->getFieldDensity() - 1);
                                }
                            }
                        }
                        break;

                    case fd_fatigue:
                    {
                        static const std::array<mtype_id, 9> monids = { {
                            mtype_id( "mon_flying_polyp" ), mtype_id( "mon_hunting_horror" ),
                            mtype_id( "mon_mi_go" ), mtype_id( "mon_yugg" ), mtype_id( "mon_gelatin" ),
                            mtype_id( "mon_flaming_eye" ), mtype_id( "mon_kreck" ), mtype_id( "mon_gracke" ),
                            mtype_id( "mon_blank" ),
                        } };
                        if (cur->getFieldDensity() < 3 && calendar::once_every(HOURS(6)) && one_in(10)) {
                            cur->setFieldDensity(cur->getFieldDensity() + 1);
                        } else if (cur->getFieldDensity() == 3 && one_in(600)) { // Spawn nether creature!
                            g->summon_mon( random_entry( monids ), p);
                        }
                    }
                        break;

                    case fd_push_items: {
                        auto items = i_at( p );
                        for( auto pushee = items.begin(); pushee != items.end(); ) {
                            if( pushee->type->id != "rock" ||
                                pushee->bday >= int(calendar::turn) - 1 ) {
                                pushee++;
                            } else {
                                item tmp = *pushee;
                                tmp.bday = int(calendar::turn);
                                pushee = items.erase( pushee );
                                std::vector<tripoint> valid;
                                tripoint dst;
                                dst.z = p.z;
                                int &xx = dst.x;
                                int &yy = dst.y;
                                for( xx = p.x - 1; xx <= p.x + 1; xx++ ) {
                                    for( yy = p.y - 1; yy <= p.y + 1; yy++ ) {
                                        if( get_field( dst, fd_push_items ) != nullptr ) {
                                            valid.push_back( dst );
                                        }
                                    }
                                }
                                if (!valid.empty()) {
                                    tripoint newp = random_entry( valid );
                                    add_item_or_charges( newp, tmp );
                                    if( g->u.pos() == newp ) {
                                        add_msg(m_bad, _("A %s hits you!"), tmp.tname().c_str());
                                        body_part hit = random_body_part();
                                        g->u.deal_damage( nullptr, hit, damage_instance( DT_BASH, 6 ) );
                                        g->u.check_dead_state();
                                    }
                                    int npcdex = g->npc_at( newp );
                                    int mondex = g->mon_at( newp );

                                    if( npcdex != -1 ) {
                                        // TODO: combine with player character code above
                                        npc *p = g->active_npc[npcdex];
                                        body_part hit = random_body_part();
                                        p->deal_damage( nullptr, hit, damage_instance( DT_BASH, 6 ) );
                                        if (g->u.sees( newp )) {
                                            add_msg(_("A %1$s hits %2$s!"), tmp.tname().c_str(), p->name.c_str());
                                        }
                                        p->check_dead_state();
                                    }

                                    if( mondex != -1 ) {
                                        monster *mon = &(g->zombie(mondex));
                                        mon->apply_damage( nullptr, bp_torso, 6 - mon->get_armor_bash( bp_torso ) );
                                        if (g->u.sees( newp ))
                                            add_msg(_("A %1$s hits the %2$s!"), tmp.tname().c_str(),
                                                       mon->name().c_str());
                                        mon->check_dead_state();
                                    }
                                }
                            }
                        }
                    }
                    break;

                    case fd_shock_vent:
                        if (cur->getFieldDensity() > 1) {
                            if (one_in(5)) {
                                cur->setFieldDensity(cur->getFieldDensity() - 1);
                            }
                        } else {
                            cur->setFieldDensity(3);
                            int num_bolts = rng(3, 6);
                            for (int i = 0; i < num_bolts; i++) {
                                int xdir = 0, ydir = 0;
                                while (xdir == 0 && ydir == 0) {
                                    xdir = rng(-1, 1);
                                    ydir = rng(-1, 1);
                                }
                                int dist = rng(4, 12);
                                int boltx = p.x, bolty = p.y;
                                for (int n = 0; n < dist; n++) {
                                    boltx += xdir;
                                    bolty += ydir;
                                    add_field( tripoint( boltx, bolty, p.z ), fd_electricity, rng(2, 3), 0 );
                                    if (one_in(4)) {
                                        if (xdir == 0) {
                                            xdir = rng(0, 1) * 2 - 1;
                                        } else {
                                            xdir = 0;
                                        }
                                    }
                                    if (one_in(4)) {
                                        if (ydir == 0) {
                                            ydir = rng(0, 1) * 2 - 1;
                                        } else {
                                            ydir = 0;
                                        }
                                    }
                                }
                            }
                        }
                        break;

                    case fd_acid_vent:
                        if (cur->getFieldDensity() > 1) {
                            if (cur->getFieldAge() >= 10) {
                                cur->setFieldDensity(cur->getFieldDensity() - 1);
                                cur->setFieldAge(0);
                            }
                        } else {
                            cur->setFieldDensity(3);
                            for( int i = p.x - 5; i <= p.x + 5; i++ ) {
                                for( int j = p.y - 5; j <= p.y + 5; j++ ) {
                                    const field_entry *acid = get_field( tripoint( i, j, p.z ), fd_acid );
                                    if( acid != nullptr && acid->getFieldDensity() == 0 ) {
                                            int newdens = 3 - (rl_dist( p.x, p.y, i, j) / 2) + (one_in(3) ? 1 : 0);
                                            if (newdens > 3) {
                                                newdens = 3;
                                            }
                                            if (newdens > 0) {
                                                add_field( tripoint( i, j, p.z ), fd_acid, newdens, 0 );
                                            }
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
                            curfield.findField( fd_fungicidal_gas ) ||
                            curfield.findField( fd_fire_vent ) ||
                            curfield.findField( fd_flame_burst ) ||
                            curfield.findField( fd_electricity ) ||
                            curfield.findField( fd_fatigue ) ||
                            curfield.findField( fd_shock_vent ) ||
                            curfield.findField( fd_plasma ) ||
                            curfield.findField( fd_laser ) ||
                            curfield.findField( fd_dazzling) ||
                            curfield.findField( fd_electricity ) ||
                            curfield.findField( fd_incendiary ) ) {
                            // Kill them at the end of processing.
                            cur->setFieldDensity( 0 );
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
                                                   cur->getFieldDensity(), cur->getFieldAge() );
                                        cur->setFieldDensity( 0 );
                                        break;
                                    }
                                }
                            } else {
                                spread_gas( cur, p, curtype, 5, 0 );
                            }
                        }
                        break;

                    case fd_incendiary:
                        {
                            //Needed for variable scope
                            dirty_transparency_cache = true;
                            tripoint dst( p.x + rng( -1, 1 ), p.y + rng( -1, 1 ), p.z );
                            if( has_flag( TFLAG_FLAMMABLE, dst ) ||
                                has_flag( TFLAG_FLAMMABLE_ASH, dst ) ||
                                has_flag( TFLAG_FLAMMABLE_HARD, dst ) ) {
                                add_field( dst, fd_fire, 1, 0 );
                            }

                            //check piles for flammable items and set those on fire
                            if( flammable_items_at( dst ) ) {
                                add_field( dst, fd_fire, 1, 0 );
                            }

                            spread_gas( cur, p, curtype, 66, 40 );
                            create_hot_air( p, cur->getFieldDensity());
                        }
                        break;

                    //Legacy Stuff
                    case fd_rubble:
                        make_rubble( p );
                        break;

                    case fd_fungicidal_gas:
                        {
                            dirty_transparency_cache = true;
                            spread_gas( cur, p, curtype, 120, 10 );
                            //check the terrain and replace it accordingly to simulate the fungus dieing off
                            const auto &ter = map_tile.get_ter_t();
                            const auto &frn = map_tile.get_furn_t();
                            const int density = cur->getFieldDensity();
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

                cur->setFieldAge(cur->getFieldAge() + 1);
                auto &fdata = fieldlist[cur->getFieldType()];
                if( fdata.halflife > 0 && cur->getFieldAge() > 0 &&
                    dice( 2, cur->getFieldAge() ) > fdata.halflife ) {
                    cur->setFieldAge( 0 );
                    cur->setFieldDensity( cur->getFieldDensity() - 1 );
                }
                if( !cur->isAlive() ) {
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
    int veh_part; // vehicle part existing on this tile.
    vehicle *veh = NULL; // Vehicle reference if there is one.
    bool inside = false; // Are we inside?
    //to modify power of a field based on... whatever is relevant for the effect.
    int adjusted_intensity;

    //If we are in a vehicle figure out if we are inside (reduces effects usually)
    // and what part of the vehicle we need to deal with.
    if (u.in_vehicle) {
        veh = veh_at( u.pos(), veh_part );
        inside = (veh && veh->is_inside(veh_part));
    }

    // Iterate through all field effects on this tile.
    // Do not remove the field with removeField, instead set it's density to 0. It will be removed
    // later by the field processing, which will also adjust field_count accordingly.
    for( auto field_list_it = curfield.begin(); field_list_it != curfield.end(); ++field_list_it){
        field_entry * cur = &field_list_it->second;
        if( !cur->isAlive() ) {
            continue;
        }

        //Do things based on what field effect we are currently in.
        switch (cur->getFieldType()) {
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
            if (!u.has_trait("WEB_WALKER") && !u.in_vehicle) {
                //between 5 and 15 minus your current web level.
                u.add_effect( effect_webbed, 1, num_bp, true, cur->getFieldDensity());
                cur->setFieldDensity( 0 ); //Its spent.
                continue;
                //If you are in a vehicle destroy the web.
                //It should of been destroyed when you ran over it anyway.
            } else if (u.in_vehicle) {
                cur->setFieldDensity( 0 );
                continue;
            }
        } break;

        case fd_acid:
        {
            // Assume vehicles block acid damage entirely,
            // you're certainly not standing in it.
            if( u.in_vehicle ) {
                break;
            }

            const int density = cur->getFieldDensity();
            int total_damage = 0;
            // Use a helper for a bit less boilerplate
            const auto burn_part = [&]( body_part bp, const int scale ) {
                const int damage = std::max<int>( density, rng( 1, scale ) );
                auto ddi = u.deal_damage( nullptr, bp, damage_instance( DT_ACID, damage ) );
                total_damage += ddi.total_damage();
                // Represents acid seeping in rather than being splashed on
                u.add_env_effect( effect_corroding, bp, 3, rng( 1, density ), bp, false, 0 );
            };

            const bool on_ground = u.is_on_ground();
            burn_part( bp_foot_l, 5 );
            burn_part( bp_foot_r, 5 );
            burn_part( bp_leg_l,  4 );
            burn_part( bp_leg_r,  4 );
            if( on_ground ) {
                // Before, it would just break the legs and leave the survivor alone
                burn_part( bp_hand_l, 3 );
                burn_part( bp_hand_r, 3 );
                burn_part( bp_torso,  3 );
                // Less arms = less ability to keep upright
                if( ( u.has_two_arms() && one_in( 4 ) ) || one_in( 2 ) ) {
                    burn_part( bp_arm_l, 2 );
                    burn_part( bp_arm_r, 2 );
                    burn_part( bp_head,  2 );
                }
            }

            if( on_ground && total_damage > 0 ) {
                u.add_msg_player_or_npc( m_bad, _("The acid burns your body!"),
                                                _("The acid burns <npcname>s body!") );
            } else if( total_damage > 0 ) {
                u.add_msg_player_or_npc( m_bad, _("The acid burns your legs and feet!"),
                                                _("The acid burns <npcname>s legs and feet!") );
            } else if( on_ground ) {
                u.add_msg_if_player( m_warning, _("You're lying in a pool of acid") );
            } else {
                u.add_msg_if_player( m_warning, _("You're standing in a pool of acid") );
            }

            u.check_dead_state();
        }
            break;

        case fd_sap:
            //Sap causes the player to get sap disease, slowing them down.
            if ( u.in_vehicle ) {
                break; //sap does nothing to cars.
            }
            u.add_msg_player_or_npc(m_bad, _("The sap sticks to you!"), _("The sap sticks to <npcname>!"));
            u.add_effect( effect_sap, cur->getFieldDensity() * 2);
            cur->setFieldDensity(cur->getFieldDensity() - 1); //Use up sap.
            break;

        case fd_sludge:
            u.add_msg_if_player(m_bad, _("The sludge is thick and sticky. You struggle to pull free."));
            u.moves -= cur->getFieldDensity() * 300;
            cur->setFieldDensity( 0 );
            break;

        case fd_fire:
            if( u.has_active_bionic("bio_heatsink") || u.is_wearing("rm13_armor_on") ||
                u.has_trait("M_SKIN2") ) {
                //heatsink, suit, or internal restructuring prevents ALL fire damage.
                break;
            }
            //Burn the player. Less so if you are in a car or ON a car.
            adjusted_intensity = cur->getFieldDensity();
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
                    _("You burn your legs and feet!"),
                    _("You're burning up!"),
                    _("You're set ablaze!"),
                    _("Your whole body is burning!")
                }};
                static const std::array<std::string, 4> npc_burn_msg = {{
                    _("<npcname> burns their legs and feet!"),
                    _("<npcname> is burning up!"),
                    _("<npcname> is set ablaze!"),
                    _("<npcname>s whole body is burning!")
                }};
                static const std::array<std::string, 4> player_warn_msg = {{
                    _("You're standing in a fire!"),
                    _("You're waist-deep in a fire!"),
                    _("You're surrounded by raging fire!"),
                    _("You're lying in fire!")
                }};

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
                            // Fallthrough intentional.
                        case 2:
                            parts_burned.push_back( bp_torso );
                            // Fallthrough intentional.
                        case 1:
                            parts_burned.push_back( bp_foot_l );
                            parts_burned.push_back( bp_foot_r );
                            parts_burned.push_back( bp_leg_l );
                            parts_burned.push_back( bp_leg_r );
                    }
                } else {
                    // Lying in the fire is BAAAD news, hits every body part.
                    msg_num = 3;
                    for( int i = 0; i < num_bp; ++i ) {
                        parts_burned.push_back( (body_part)i );
                    }
                }

                int total_damage = 0;
                for( auto part_burned : parts_burned ) {
                    const auto dealt = u.deal_damage( nullptr, part_burned,
                        damage_instance( DT_HEAT, rng( burn_min, burn_max ) ) );
                    total_damage += dealt.type_damage( DT_HEAT );
                }
                if( total_damage > 10 ) {
                    u.add_effect( effect_onfire, 2 + adjusted_intensity );
                }
                if( total_damage > 0 ) {
                    u.add_msg_player_or_npc( m_bad,
                        player_burn_msg[msg_num].c_str(),
                        npc_burn_msg[msg_num].c_str() );
                } else {
                    u.add_msg_if_player( m_warning,
                        player_warn_msg[msg_num].c_str() );
                }
                u.check_dead_state();
            }
            break;

        case fd_smoke:
            {
                if (!inside) {
                    //Get smoke disease from standing in smoke.
                    int density = cur->getFieldDensity();
                    int coughStr;
                    int coughDur;
                    if (density >= 3) {   // thick smoke
                        coughStr = 4;
                        coughDur = 15;
                    } else if (density == 2) {  // smoke
                        coughStr = 2;
                        coughDur = 7;
                    } else {    // density 1, thin smoke
                        coughStr = 1;
                        coughDur = 2;
                    }
                    u.add_env_effect( effect_smoke, bp_mouth, coughStr, coughDur );
                }
            }
            break;

        case fd_tear_gas:
            //Tear gas will both give you teargas disease and/or blind you.
            if ((cur->getFieldDensity() > 1 || !one_in(3)) && (!inside || (inside && one_in(3))))
            {
                u.add_env_effect( effect_teargas, bp_mouth, 5, 20 );
            }
            if (cur->getFieldDensity() > 1 && (!inside || (inside && one_in(3))))
            {
                u.add_env_effect( effect_blind, bp_eyes, cur->getFieldDensity() * 2, 10 );
            }
            break;

        case fd_relax_gas:
            if ((cur->getFieldDensity() > 1 || !one_in(3)) && (!inside || (inside && one_in(3))))
            {
                u.add_env_effect( effect_relax_gas, bp_mouth, cur->getFieldDensity() * 2, 3 );
            }
            break;

        case fd_fungal_haze:
            if (!u.has_trait("M_IMMUNE") && (!inside || (inside && one_in(4))) ) {
                u.add_env_effect( effect_fungus, bp_mouth, 4, 100, num_bp, true );
                u.add_env_effect( effect_fungus, bp_eyes, 4, 100, num_bp, true );
            }
            break;

        case fd_dazzling:
            if (cur->getFieldDensity() > 1 || one_in(5)){
                u.add_env_effect( effect_blind, bp_eyes, 10, 10 );
            } else{
                u.add_env_effect( effect_blind, bp_eyes, 2, 2 );
            }
            break;

        case fd_toxic_gas:
            // Toxic gas at low levels poisons you.
            // Toxic gas at high levels will cause very nasty poison.
            {
                bool inhaled = false;
                if( cur->getFieldDensity() == 2 &&
                    (!inside || (cur->getFieldDensity() == 3 && inside)) ) {
                    inhaled = u.add_env_effect( effect_poison, bp_mouth, 5, 30 );
                } else if( cur->getFieldDensity() == 3 && !inside ) {
                    inhaled = u.add_env_effect( effect_badpoison, bp_mouth, 5, 30 );
                } else if( cur->getFieldDensity() == 1 && (!inside) ) {
                    inhaled = u.add_env_effect( effect_poison, bp_mouth, 2, 20 );
                }
                if( inhaled ) {
                    // player does not know how the npc feels, so no message.
                    u.add_msg_if_player(m_bad, _("You feel sick from inhaling the %s"), cur->name().c_str());
                }
            }
            break;

        case fd_nuke_gas:
            // Get irradiated by the nuclear fallout.
            // Changed to min of density, not 0.
            u.radiation += rng(cur->getFieldDensity(),
                                  cur->getFieldDensity() * (cur->getFieldDensity() + 1));
            if (cur->getFieldDensity() == 3) {
                u.add_msg_if_player(m_bad, _("This radioactive gas burns!"));
                u.hurtall(rng(1, 3), nullptr);
            }
            break;

        case fd_flame_burst:
            //A burst of flame? Only hits the legs and torso.
            if (inside) break; //fireballs can't touch you inside a car.
            if (!u.has_active_bionic("bio_heatsink") && !u.is_wearing("rm13_armor_on") &&
                !u.has_trait("M_SKIN2")) { //heatsink, suit, or Mycus fireproofing stops fire.
                u.add_msg_player_or_npc(m_bad, _("You're torched by flames!"), _("<npcname> is torched by flames!"));
                u.deal_damage( nullptr, bp_leg_l, damage_instance( DT_HEAT, rng( 2, 6 ) ) );
                u.deal_damage( nullptr, bp_leg_r, damage_instance( DT_HEAT, rng( 2, 6 ) ) );
                u.deal_damage( nullptr, bp_torso, damage_instance( DT_HEAT, rng( 4, 9 ) ) );
                u.check_dead_state();
            } else
                u.add_msg_player_or_npc(_("These flames do not burn you."), _("Those flames do not burn <npcname>."));
            break;

        case fd_electricity:
        {
            // Small universal damage based on density.
            int total_damage = 0;
            for( size_t i = 0; i < num_hp_parts; i++ ) {
                const body_part bp = player::hp_to_bp( static_cast<hp_part>( i ) );
                const int dmg = rng( 1, cur->getFieldDensity() );
                total_damage += u.deal_damage( nullptr, bp, damage_instance( DT_ELECTRIC, dmg ) ).total_damage();
            }

            if( total_damage > 0 ) {
                u.add_msg_player_or_npc(m_bad, _("You're electrocuted!"), _("<npcname> is electrocuted!"));
            } else {
                u.add_msg_player_or_npc( _("The electric cloud doesn't affect you."),
                                     _("The electric cloud doesn't seem to affect <npcname>.") );
            }
        }

            break;

        case fd_fatigue:
            //Teleports you... somewhere.
            if (rng(0, 2) < cur->getFieldDensity() && u.is_player() ) {
                // TODO: allow teleporting for npcs
                add_msg(m_bad, _("You're violently teleported!"));
                u.hurtall(cur->getFieldDensity(), nullptr);
                g->teleport();
            }
            break;

            //Why do these get removed???
        case fd_shock_vent:
            //Stepping on a shock vent shuts it down.
            cur->setFieldDensity( 0 );
            continue;

        case fd_acid_vent:
            //Stepping on an acid vent shuts it down.
            cur->setFieldDensity( 0 );
            continue;

        case fd_bees:
            // Player is immune to bees while underwater.
            if( !u.is_underwater() ) {
                int times_stung = 0;
                int density = cur->getFieldDensity();
                // If the bees can get at you, they cause steadily increasing pain.
                // TODO: Specific stinging messages.
                times_stung += one_in(4) &&
                    u.add_env_effect( effect_stung, bp_torso, density, 90 );
                times_stung += one_in(4) &&
                    u.add_env_effect( effect_stung, bp_torso, density, 90 );
                times_stung += one_in(4) &&
                    u.add_env_effect( effect_stung, bp_torso, density, 90 );
                times_stung += one_in(4) &&
                    u.add_env_effect( effect_stung, bp_torso, density, 90 );
                times_stung += one_in(4) &&
                    u.add_env_effect( effect_stung, bp_torso, density, 90 );
                times_stung += one_in(4) &&
                    u.add_env_effect( effect_stung, bp_torso, density, 90 );
                times_stung += one_in(4) &&
                    u.add_env_effect( effect_stung, bp_torso, density, 90 );
                times_stung += one_in(4) &&
                    u.add_env_effect( effect_stung, bp_torso, density, 90 );
                switch( times_stung ) {
                case 0:
                    // Woo, unscathed!
                    break;
                case 1:
                    u.add_msg_if_player( m_bad, _("The bees sting you!") );
                    break;
                case 2:
                case 3:
                    u.add_msg_if_player( m_bad, _("The bees sting you several times!") );
                    break;
                case 4:
                case 5:
                    u.add_msg_if_player( m_bad, _("The bees sting you many times!") );
                    break;
                case 6:
                case 7:
                case 8:
                default:
                    u.add_msg_if_player( m_bad, _("The bees sting you all over your body!") );
                    break;
                }
            }
            break;

        case fd_incendiary:
            // Mysterious incendiary substance melts you horribly.
            if (u.has_trait("M_SKIN2") || cur->getFieldDensity() == 1) {
                u.add_msg_player_or_npc(m_bad, _("The incendiary burns you!"), _("The incendiary burns <npcname>!"));
                u.hurtall(rng(1, 3), nullptr);
            } else {
                u.add_msg_player_or_npc(m_bad, _("The incendiary melts into your skin!"), _("The incendiary melts into <npcname>s skin!"));
                u.add_effect( effect_onfire, 8);
                u.hurtall(rng(2, 6), nullptr);
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
                if( u.worn_with_flag("GAS_PROOF") ) {
                    break;
                }
                bool inhaled = false;
                const int density = cur->getFieldDensity();
                inhaled = u.add_env_effect( effect_poison, bp_mouth, 5, density * 10 );
                if( u.has_trait("THRESH_MYCUS") || u.has_trait("THRESH_MARLOSS") ) {
                    inhaled |= u.add_env_effect( effect_badpoison, bp_mouth, 5, density * 10 );
                    u.hurtall( rng( density, density * 2 ), nullptr );
                    u.add_msg_if_player( m_bad, _("The %s burns your skin."), cur->name().c_str() );
                }

                if( inhaled ) {
                    u.add_msg_if_player( m_bad, _("The %s makes you feel sick."), cur->name().c_str() );
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
    if (z.digging()) {
        return; // Digging monsters are immune to fields
    }
    field &curfield = get_field( z.pos() );

    int dam = 0;
    // Iterate through all field effects on this tile.
    // Do not remove the field with removeField, instead set it's density to 0. It will be removed
    // later by the field processing, which will also adjust field_count accordingly.
    for( auto field_list_it = curfield.begin(); field_list_it != curfield.end(); ++field_list_it ) {
        field_entry * cur = &field_list_it->second;
        if( !cur->isAlive() ) {
            continue;
        }

        switch (cur->getFieldType()) {
        case fd_null:
        case fd_blood: // It doesn't actually do anything
        case fd_bile:  // Ditto
            break;

        case fd_web:
            if (!z.has_flag(MF_WEBWALK)) {
                z.add_effect( effect_webbed, 1, num_bp, true, cur->getFieldDensity());
                cur->setFieldDensity( 0 );
            }
            break;

        case fd_acid:
            if( !z.has_flag( MF_FLIES ) ) {
                const int d = rng( cur->getFieldDensity(), cur->getFieldDensity() * 3 );
                z.deal_damage( nullptr, bp_torso, damage_instance( DT_ACID, d ) );
                z.check_dead_state();
            }
            break;

        case fd_sap:
            z.moves -= cur->getFieldDensity() * 5;
            cur->setFieldDensity(cur->getFieldDensity() - 1);
            break;

        case fd_sludge:
            if (!z.has_flag(MF_DIGS) && !z.has_flag(MF_FLIES) &&
                !z.has_flag(MF_SLUDGEPROOF)) {
              z.moves -= cur->getFieldDensity() * 300;
              cur->setFieldDensity( 0 );
            }
            break;

            // MATERIALS-TODO: Use fire resistance
        case fd_fire:
            if (z.has_flag(MF_FIREPROOF)){
                return;
            }
            // TODO: Replace the section below with proper json values
            if ( z.made_of("flesh") || z.made_of("hflesh") || z.made_of("iflesh") ) {
                dam += 3;
            }
            if (z.made_of("veggy")) {
                dam += 12;
            }
            if (z.made_of("paper") || z.made_of(LIQUID) || z.made_of("powder") ||
                z.made_of("wood")  || z.made_of("cotton") || z.made_of("wool")) {
                dam += 20;
            }
            if (z.made_of("stone") || z.made_of("kevlar") || z.made_of("steel")) {
                dam += -20;
            }
            if (z.has_flag(MF_FLIES)) {
                dam -= 15;
            }
            dam -= z.get_armor_type( DT_HEAT, bp_torso );

            if (cur->getFieldDensity() == 1) {
                dam += rng(2, 6);
            } else if (cur->getFieldDensity() == 2) {
                dam += rng(6, 12);
                if (!z.has_flag(MF_FLIES)) {
                    z.moves -= 20;
                    if( dam > 0 ) {
                        z.add_effect( effect_onfire, rng(dam / 2, dam * 2));
                    }
                }
            } else if (cur->getFieldDensity() == 3) {
                dam += rng(10, 20);
                if (!z.has_flag(MF_FLIES) || one_in(3)) {
                    z.moves -= 40;
                    if( dam > 0 ) {
                        z.add_effect( effect_onfire, rng(dam / 2, dam * 2));
                    }
                }
            }
            // Drop through to smoke no longer needed as smoke will exist in the same square now,
            // this would double apply otherwise.
            break;

        case fd_smoke:
            if (!z.has_flag(MF_NO_BREATHE)) {
                if (cur->getFieldDensity() == 3) {
                    z.moves -= rng(10, 20);
                }
                if (z.made_of("veggy")) { // Plants suffer from smoke even worse
                    z.moves -= rng(1, cur->getFieldDensity() * 12);
                }
            }
            break;

        case fd_tear_gas:
            if ((z.made_of("flesh") || z.made_of("hflesh") || z.made_of("veggy") || z.made_of("iflesh")) &&
                !z.has_flag(MF_NO_BREATHE)) {
                if (cur->getFieldDensity() == 3) {
                    z.add_effect( effect_stunned, rng(10, 20));
                    dam += rng(4, 10);
                } else if (cur->getFieldDensity() == 2) {
                    z.add_effect( effect_stunned, rng(5, 10));
                    dam += rng(2, 5);
                } else {
                    z.add_effect( effect_stunned, rng(1, 5));
                }
                if (z.made_of("veggy")) {
                    z.moves -= rng(cur->getFieldDensity() * 5, cur->getFieldDensity() * 12);
                    dam += cur->getFieldDensity() * rng(8, 14);
                }
                if (z.has_flag(MF_SEES)) {
                     z.add_effect( effect_blind, cur->getFieldDensity() * 8);
                }
            }
            break;

        case fd_relax_gas:
            if ((z.made_of("flesh") || z.made_of("hflesh") || z.made_of("veggy") || z.made_of("iflesh")) &&
                !z.has_flag(MF_NO_BREATHE)) {
                z.add_effect( effect_stunned, rng(cur->getFieldDensity() * 4, cur->getFieldDensity() * 8));
            }
            break;

        case fd_dazzling:
            if (z.has_flag(MF_SEES)) {
                z.add_effect( effect_blind, cur->getFieldDensity() * 12);
                z.add_effect( effect_stunned, cur->getFieldDensity() * rng(5, 12));
            }
            break;

        case fd_toxic_gas:
            if(!z.has_flag(MF_NO_BREATHE)) {
                dam += cur->getFieldDensity();
                z.moves -= cur->getFieldDensity();
            }
            break;

        case fd_nuke_gas:
            if(!z.has_flag(MF_NO_BREATHE)) {
                if (cur->getFieldDensity() == 3) {
                    z.moves -= rng(60, 120);
                    dam += rng(30, 50);
                } else if (cur->getFieldDensity() == 2) {
                    z.moves -= rng(20, 50);
                    dam += rng(10, 25);
                } else {
                    z.moves -= rng(0, 15);
                    dam += rng(0, 12);
                }
                if (z.made_of("veggy")) {
                    z.moves -= rng(cur->getFieldDensity() * 5, cur->getFieldDensity() * 12);
                    dam *= cur->getFieldDensity();
                }
            }
            break;

            // MATERIALS-TODO: Use fire resistance
        case fd_flame_burst:
            if (z.made_of("flesh") || z.made_of("hflesh") || z.made_of("iflesh")) {
                dam += 3;
            }
            if (z.made_of("veggy")) {
                dam += 12;
            }
            if (z.made_of("paper") || z.made_of(LIQUID) || z.made_of("powder") ||
                z.made_of("wood")  || z.made_of("cotton") || z.made_of("wool")) {
                dam += 50;
            }
            if (z.made_of("stone") || z.made_of("kevlar") || z.made_of("steel")) {
                dam += -25;
            }
            dam += rng(0, 8);
            z.moves -= 20;
            break;

        case fd_electricity:
            // We don't want to increase dam, but deal a separate hit so that it can apply effects
            z.deal_damage( nullptr, bp_torso,
                           damage_instance( DT_ELECTRIC, rng( 1, cur->getFieldDensity() * 3 ) ) );
            break;

        case fd_fatigue:
            if (rng(0, 2) < cur->getFieldDensity()) {
                dam += cur->getFieldDensity();
                int tries = 0;
                tripoint newpos = z.pos();
                do {
                    newpos.x = rng(z.posx() - SEEX, z.posx() + SEEX);
                    newpos.y = rng(z.posy() - SEEY, z.posy() + SEEY);
                    tries++;
                } while (impassable(newpos) && tries != 10);

                if (tries == 10) {
                    z.die_in_explosion( nullptr );
                } else {
                    int mon_hit = g->mon_at(newpos);
                    if (mon_hit != -1) {
                        if (g->u.sees(z)) {
                            add_msg(_("The %1$s teleports into a %2$s, killing them both!"),
                                       z.name().c_str(), g->zombie(mon_hit).name().c_str());
                        }
                        g->zombie( mon_hit ).die_in_explosion( &z );
                    } else {
                        z.setpos(newpos);
                    }
                }
            }
            break;

        case fd_incendiary:
            // MATERIALS-TODO: Use fire resistance
            if ( z.made_of("flesh") || z.made_of("hflesh") || z.made_of("iflesh") ) {
                dam += 3;
            }
            if (z.made_of("veggy")) {
                dam += 12;
            }
            if (z.made_of("paper") || z.made_of(LIQUID) || z.made_of("powder") ||
                z.made_of("wood")  || z.made_of("cotton") || z.made_of("wool")) {
                dam += 20;
            }
            if (z.made_of("stone") || z.made_of("kevlar") || z.made_of("steel")) {
                dam += -5;
            }

            if (cur->getFieldDensity() == 1) {
                dam += rng(2, 6);
            } else if (cur->getFieldDensity() == 2) {
                dam += rng(6, 12);
                z.moves -= 20;
                if (!z.made_of(LIQUID) && !z.made_of("stone") && !z.made_of("kevlar") &&
                !z.made_of("steel") && !z.has_flag(MF_FIREY)) {
                    z.add_effect( effect_onfire, rng(8, 12));
                }
            } else if (cur->getFieldDensity() == 3) {
                dam += rng(10, 20);
                z.moves -= 40;
                if (!z.made_of(LIQUID) && !z.made_of("stone") && !z.made_of("kevlar") &&
                !z.made_of("steel") && !z.has_flag(MF_FIREY)) {
                        z.add_effect( effect_onfire, rng(12, 16));
                }
            }
            break;

        case fd_fungal_haze:
            if( !z.type->in_species( FUNGUS ) &&
                !z.type->has_flag("NO_BREATHE") &&
                !z.make_fungus() ) {
                // Don't insta-kill jabberwocks, that's silly
                const int density = cur->getFieldDensity();
                z.moves -= rng( 10 * density, 30 * density );
                dam += rng( 0, 10 * density );
            }
            break;

        case fd_fungicidal_gas:
            if( z.type->in_species( FUNGUS ) ) {
                const int density = cur->getFieldDensity();
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
        z.apply_damage( nullptr, bp_torso, dam );
        z.check_dead_state();
    }
}

int field_entry::move_cost() const{
  return fieldlist[type].move_cost[ getFieldDensity() - 1 ];
}

field_id field_entry::getFieldType() const{
    return type;
}


int field_entry::getFieldDensity() const{
    return density;
}


int field_entry::getFieldAge() const{
    return age;
}

field_id field_entry::setFieldType(const field_id new_field_id){

    //TODO: Better bounds checking.
    if(new_field_id >= 0 && new_field_id < num_fields){
        type = new_field_id;
    } else {
        type = fd_null;
    }

    return type;

}

int field_entry::setFieldDensity(const int new_density){

    if(new_density > 3) {
        density = 3;
    } else if (new_density < 1) {
        density = 1;
        is_alive = false;
    } else {
        density = new_density;
    }

    return density;

}

int field_entry::setFieldAge(const int new_age){

    age = new_age;

    return age;
}

field::field()
    : field_list()
    , draw_symbol( fd_null )
{
}

field::~field()
{
}

/*
Function: findField
Returns a field entry corresponding to the field_id parameter passed in. If no fields are found then returns NULL.
Good for checking for exitence of a field: if(myfield.findField(fd_fire)) would tell you if the field is on fire.
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
bool field::addField(const field_id field_to_add, const int new_density, const int new_age){
    auto it = field_list.find(field_to_add);
    if (fieldlist[field_to_add].priority >= fieldlist[draw_symbol].priority)
        draw_symbol = field_to_add;
    if(it != field_list.end()) {
        //Already exists, but lets update it. This is tentative.
        it->second.setFieldDensity(it->second.getFieldDensity() + new_density);
        return false;
    }
    field_list[field_to_add] = field_entry(field_to_add, new_density, new_age);
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
                if (fieldlist[fld.first].priority >= fieldlist[draw_symbol].priority) {
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
    for( auto & fld : field_list ) {
        current_cost += fld.second.move_cost();
    }
    return current_cost;
}
