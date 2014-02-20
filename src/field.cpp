#include "rng.h"
#include "map.h"
#include "field.h"
#include "game.h"
#include "monstergenerator.h"

#define INBOUNDS(x, y) \
 (x >= 0 && x < SEEX * my_MAPSIZE && y >= 0 && y < SEEY * my_MAPSIZE)

bool vector_has(std::vector <item> vec, itype_id type);

field_t fieldlist[num_fields];

void game::init_fields()
{
    field_t tmp_fields[num_fields] =
    {
        {
            {"", "", ""}, '%', 0,
            {c_white, c_white, c_white}, {true, true, true}, {false, false, false}, 0,
            {0,0,0}
        },
        {
            {_("blood splatter"), _("blood stain"), _("puddle of blood")}, '%', 0,
            {c_red, c_red, c_red}, {true, true, true}, {false, false, false}, 2500,
            {0,0,0}
        },
        {
            {_("bile splatter"), _("bile stain"), _("puddle of bile")}, '%', 0,
            {c_pink, c_pink, c_pink}, {true, true, true}, {false, false, false}, 2500,
            {0,0,0}
        },

        {
            {_("scraps of flesh"), _("bloody meat chunks"), _("heap of gore")}, '~', 0,
            {c_brown, c_ltred, c_red}, {true, true, true}, {false, false, false}, 2500,
            {0,0,0}
        },

        {
            {_("shredded leaves and twigs"), _("shattered branches and leaves"), _("broken vegetation tangle")}, '~', 0,
            {c_ltgreen, c_ltgreen, c_green}, {true, true, true}, {false, false, false}, 2500,
            {0,0,0}
        },

        {
            {_("cobwebs"),_("webs"), _("thick webs")}, '}', 2,
            {c_white, c_white, c_white}, {true, true, false},{false, false, false}, 0,
            {0,0,0}
        },

        {
            {_("slime trail"), _("slime stain"), _("puddle of slime")}, '%', 0,
            {c_ltgreen, c_ltgreen, c_green},{true, true, true},{false, false, false}, 2500,
            {0,0,0}
        },

        {
            {_("acid splatter"), _("acid streak"), _("pool of acid")}, '5', 2,
            {c_ltgreen, c_green, c_green}, {true, true, true}, {true, true, true}, 10,
            {0,0,0}
        },

        {
            {_("sap splatter"), _("glob of sap"), _("pool of sap")}, '5', 2,
            {c_yellow, c_brown, c_brown}, {true, true, true}, {true, true, true}, 20,
            {0,0,0}
        },

        {
            {_("thin sludge trail"), _("sludge trail"), _("thick sludge trail")}, '5', 2,
            {c_ltgray, c_dkgray, c_black}, {true, true, true}, {false, false, false}, 900,
            {0,0,0}
        },

        {
            {_("small fire"), _("fire"), _("raging fire")}, '4', 4,
            {c_yellow, c_ltred, c_red}, {true, true, true}, {true, true, true}, 800,
            {0,0,0}
        },

        {
            {_("rubble heap"), _("rubble pile"), _("mountain of rubble")}, '#', 2,
            {c_dkgray, c_dkgray, c_dkgray}, {true, true, false},{false, false, false},  0,
            {0,0,0}
        },

        {
            {_("thin smoke"), _("smoke"), _("thick smoke")}, '8', 8,
            {c_white, c_ltgray, c_dkgray}, {true, false, false},{false, true, true},  300,
            {0,0,0}
        },

        {
            {_("hazy cloud"),_("toxic gas"),_("thick toxic gas")}, '8', 8,
            {c_white, c_ltgreen, c_green}, {true, false, false},{false, true, true},  900,
            {0,0,0}
        },

        {
            {_("hazy cloud"),_("tear gas"),_("thick tear gas")}, '8', 8,
            {c_white, c_yellow, c_brown}, {true, false, false},{true, true, true},   600,
            {0,0,0}
        },

        {
            {_("hazy cloud"),_("radioactive gas"), _("thick radioactive gas")}, '8', 8,
            {c_white, c_ltgreen, c_green}, {true, true, false}, {true, true, true},  1000,
            {0,0,0}
        },

        {
            {_("gas vent"), _("gas vent"), _("gas vent")}, '%', 0,
            {c_white, c_white, c_white}, {true, true, true}, {false, false, false}, 0,
            {0,0,0}
        },

        { // Fire Vents
            {"", "", ""}, '&', 0,
            {c_white, c_white, c_white}, {true, true, true}, {false, false, false}, 0,
            {0,0,0}
        },

        {
            {_("fire"), _("fire"), _("fire")}, '5', 4,
            {c_red, c_red, c_red}, {true, true, true}, {true, true, true}, 0,
            {0,0,0}
        },

        {
            {_("sparks"), _("electric crackle"), _("electric cloud")}, '9', 4,
            {c_white, c_cyan, c_blue}, {true, true, true}, {true, true, true}, 2,
            {0,0,0}
        },

        {
            {_("odd ripple"), _("swirling air"), _("tear in reality")}, '*', 8,
            {c_ltgray, c_dkgray, c_magenta},{true, true, false},{false, false, false},  0,
            {0,0,0}
        },

        { //Push Items
            {"", "", ""}, '&', 0,
            {c_white, c_white, c_white}, {true, true, true}, {false, false, false}, 0,
            {0,0,0}
        },

        { // shock vents
            {"", "", ""}, '&', 0,
            {c_white, c_white, c_white}, {true, true, true}, {false, false, false}, 0,
            {0,0,0}
        },

        { // acid vents
            {"", "", ""}, '&', 0,
            {c_white, c_white, c_white}, {true, true, true}, {false, false, false}, 0,
            {0,0,0}
        },

        { // plasma glow (for plasma weapons)
            {_("faint plasma"), _("glowing plasma"), _("glaring plasma")}, '9', 4,
            {c_magenta, c_pink, c_white}, {true, true, true}, {false, false, false}, 2,
            {0,0,0}
        },

        { // laser beam (for laser weapons)
            {_("faint glimmer"), _("beam of light"), _("intense beam of light")}, '#', 4,
            {c_blue, c_ltblue, c_white}, {true, true, true}, {false, false, false}, 1,
            {0,0,0}
        },
        {
            {_("plant sap splatter"), _("plant sap stain"), _("puddle of resin")}, '%', 0,
            {c_ltgreen, c_ltgreen, c_ltgreen}, {true, true, true}, {false, false, false}, 2500,
            {0,0,0}
        },
        {
            {_("bug blood splatter"), _("bug blood stain"), _("puddle of bug blood")}, '%', 0,
            {c_green, c_green, c_green}, {true, true, true}, {false, false, false}, 2500,
            {0,0,0}
        },
        {
            {_("hemolymph splatter"), _("hemolymph stain"), _("puddle of hemolymph")}, '%', 0,
            {c_ltgray, c_ltgray, c_ltgray}, {true, true, true}, {false, false, false}, 2500,
            {0,0,0}
        },
        {
            {_("shards of chitin"), _("shattered bug leg"), _("torn insect organs")}, '~', 0,
            {c_ltgreen, c_green, c_yellow}, {true, true, true}, {false, false, false}, 2500,
            {0,0,0}
        },
        {
            {_("gooey scraps"), _("icky mess"), _("heap of squishy gore")}, '~', 0,
            {c_ltgray, c_ltgray, c_dkgray}, {true, true, true}, {false, false, false}, 2500,
            {0,0,0}
        }
    };
    for(int i=0; i<num_fields; i++) {
        fieldlist[i] = tmp_fields[i];
    }
}


/*
Function: spread_gas
Helper function that encapsulates the logic involved in gas spread.
*/
static void spread_gas( map *m, field_entry *cur, int x, int y, field_id curtype,
                        int percent_spread, int outdoor_age_speedup )
{
    // Reset nearby scents to zero
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            g->scent( x + i, y + j ) = 0;
        }
    }
    // Dissapate faster outdoors.
    if (m->is_outside(x, y)) { cur->setFieldAge( cur->getFieldAge() + outdoor_age_speedup ); }

    // Bail out if we don't meet the spread chance.
    if( rng(1, 100) > percent_spread ) { return; }

    std::vector <point> spread;
    // Pick all eligible points to spread to.
    for( int a = -1; a <= 1; a++ ) {
        for( int b = -1; b <= 1; b++ ) {
            // Current field not a candidate.
            if( !(a || b) ) { continue; }
            field_entry* tmpfld = m->field_at( x + a, y + b ).findField( curtype );
            // Candidates are existing non-max-strength fields or navigable tiles with no field.
            if( ( tmpfld && tmpfld->getFieldDensity() < 3 ) ||
                ( !tmpfld && m->move_cost( x + a, y + b ) > 0 ) ) {
                spread.push_back( point( x + a, y + b ) );
            }
        }
    }
    // Then, spread to a nearby point.
    int current_density = cur->getFieldDensity();
    if (current_density > 1 && cur->getFieldAge() > 0 && !spread.empty()) {
        point p = spread[ rng( 0, spread.size() - 1 ) ];
        field_entry *candidate_field = m->field_at(p.x, p.y).findField( curtype );
        int candidate_density = candidate_field ? candidate_field->getFieldDensity() : 0;
        // Nearby gas grows thicker.
        if ( candidate_field ) {
            candidate_field->setFieldDensity(candidate_density + 1);
            cur->setFieldDensity(current_density - 1);
        // Or, just create a new field.
        } else if ( m->add_field( p.x, p.y, curtype, 1 ) ) {
            cur->setFieldDensity( current_density - 1 );
        }
    }
}


bool map::process_fields()
{
 bool found_field = false;
 for (int x = 0; x < my_MAPSIZE; x++) {
  for (int y = 0; y < my_MAPSIZE; y++) {
   if (grid[x + y * my_MAPSIZE]->field_count > 0)
    found_field |= process_fields_in_submap(x + y * my_MAPSIZE);
  }
 }
 return found_field;
}

/*
Function: process_fields_in_submap
Iterates over every field on every tile of the given submap indicated by NONANT parameter gridn.
This is the general update function for field effects. This should only be called once per game turn.
If you need to insert a new field behavior per unit time add a case statement in the switch below.
*/
bool map::process_fields_in_submap(int gridn)
{
    // Realistically this is always true, this function only gets called if fields exist.
    bool found_field = false;
    // A pointer to the current field effect.
    // Used to modify or otherwise get information on the field effect to update.
    field_entry *cur;
    //Holds m.field_at(x,y).findField(fd_some_field) type returns.
    // Just to avoid typing that long string for a temp value.
    field_entry *tmpfld = NULL;
    field_id curtype; //Holds cur->getFieldType() as thats what the old system used before rewrite.

    bool skipIterIncr = false; // keep track on when not to increment it[erator]

    //Loop through all tiles in this submap indicated by gridn
    for (int locx = 0; locx < SEEX; locx++) {
        for (int locy = 0; locy < SEEY; locy++) {
            // This is a translation from local coordinates to submap coords.
            // All submaps are in one long 1d array.
            int x = locx + SEEX * (gridn % my_MAPSIZE);
            int y = locy + SEEY * int(gridn / my_MAPSIZE);
            // get a copy of the field variable from the submap;
            // contains all the pointers to the real field effects.
            field &curfield = grid[gridn]->fld[locx][locy];
            for(std::map<field_id, field_entry *>::iterator it = curfield.getFieldStart();
                it != curfield.getFieldEnd();) {
                //Iterating through all field effects in the submap's field.
                cur = it->second;
                if(cur == NULL) {
                    continue;    //This shouldn't happen ever, but pointer safety is number one.
                }

                curtype = cur->getFieldType();
                //Setting our return value. fd_null really doesn't exist anymore, its there for legacy support.
                if (!found_field && curtype != fd_null) {
                    found_field = true;
                }
                // Again, legacy support in the event someone Mods setFieldDensity to allow more values.
                if (cur->getFieldDensity() > 3 || cur->getFieldDensity() < 1) {
                    debugmsg("Whoooooa density of %d", cur->getFieldDensity());
                }

                // Don't process "newborn" fields. This gives the player time to run if they need to.
                if (cur->getFieldAge() == 0) {
                    curtype = fd_null;
                }

                int part;
                vehicle *veh;
                switch (curtype) {

                    case fd_null:
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
                        if (has_flag("SWIMMABLE", x, y)) { // Dissipate faster in water
                            cur->setFieldAge(cur->getFieldAge() + 250);
                        }
                        break;

                    case fd_acid:
                        if (has_flag("SWIMMABLE", x, y)) { // Dissipate faster in water
                            cur->setFieldAge(cur->getFieldAge() + 20);
                        }
                        for (int i = 0; i < i_at(x, y).size(); i++) {
                            item *melting = &(i_at(x, y)[i]); //For each item on the tile...

                            // see DEVELOPER_FAQ.txt for how acid resistance is calculated

                            int chance = melting->acid_resist();
                            if (chance == 0) {
                                melting->damage++;
                            } else if (chance > 0 && chance < 9) {
                                if (one_in(chance)) {
                                    melting->damage++;
                                }
                            }
                            if (melting->damage >= 5) {
                                //Destroy the object, age the field.
                                cur->setFieldAge(cur->getFieldAge() + melting->volume());
                                for (int m = 0; m < i_at(x, y)[i].contents.size(); m++) {
                                    i_at(x, y).push_back( i_at(x, y)[i].contents[m] );
                                }
                                i_at(x, y).erase(i_at(x, y).begin() + i);
                                i--;
                            }
                        }
                        break;

                    case fd_sap:
                        break;

                    case fd_sludge:
                        break;

                        // TODO-MATERIALS: use fire resistance
                    case fd_fire: {
                        // Consume items as fuel to help us grow/last longer.
                        bool destroyed = false; //Is the item destroyed?
                        // Volume, Smoke generation probability, consumed items count
                        int vol = 0, smoke = 0, consumed = 0;
                        for (int i = 0; i < i_at(x, y).size() && consumed < cur->getFieldDensity() * 2; i++) {
                            //Stop when we hit the end of the item buffer OR we consumed enough items given our fire size.
                            destroyed = false;
                            item *it = &(i_at(x, y)[i]); //Pointer to the item we are dealing with.
                            vol = it->volume(); //Used to feed the fire based on volume of item burnt.
                            it_ammo *ammo_type = NULL; //Special case if its ammo.

                            if (it->is_ammo()) {
                                ammo_type = dynamic_cast<it_ammo *>(it->type);
                            }
                            //Flame type ammo removed so gasoline isn't explosive, it just burns.
                            if(ammo_type != NULL &&
                               (ammo_type->ammo_effects.count("INCENDIARY") ||
                                ammo_type->ammo_effects.count("EXPLOSIVE") ||
                                ammo_type->ammo_effects.count("FRAG") ||
                                ammo_type->ammo_effects.count("NAPALM") ||
                                ammo_type->ammo_effects.count("NAPALM_BIG") ||
                                ammo_type->ammo_effects.count("EXPLOSIVE_BIG") ||
                                ammo_type->ammo_effects.count("EXPLOSIVE_HUGE") ||
                                ammo_type->ammo_effects.count("TEARGAS") ||
                                ammo_type->ammo_effects.count("SMOKE") ||
                                ammo_type->ammo_effects.count("SMOKE_BIG") ||
                                ammo_type->ammo_effects.count("FLASHBANG") ||
                                ammo_type->ammo_effects.count("COOKOFF"))) {
                                //Any kind of explosive ammo (IE: not arrows and pebbles and such)
                                const long rounds_exploded = rng(1, it->charges);
                                // TODO: Vary the effect based on the ammo flag instead of just exploding them all.
                                // cook off ammo instead of just burning it.
                                for(int j = 0; j < (rounds_exploded / 10) + 1; j++) {
                                    //Blow up with half the ammos damage in force, for each bullet.
                                    g->explosion(x, y, ammo_type->damage / 2, true, false, false);
                                }
                                it->charges -= rounds_exploded; //Get rid of the spent ammo.
                                if(it->charges == 0) {
                                    destroyed = true;    //No more ammo, item should be removed.
                                }
                            } else if (it->made_of("paper")) {
                                //paper items feed the fire moderatly.
                                destroyed = it->burn(cur->getFieldDensity() * 3);
                                consumed++;
                                if (cur->getFieldDensity() == 1) {
                                    cur->setFieldAge(cur->getFieldAge() - vol * 10);    //lower age is a longer lasting fire
                                }
                                if (vol >= 4) {
                                    smoke++;    //Large paper items give chance to smoke.
                                }

                            } else if ((it->made_of("wood") || it->made_of("veggy"))) {
                                //Wood or vegy items burn slowly.
                                if (vol <= cur->getFieldDensity() * 10 ||
                                    cur->getFieldDensity() == 3) {
                                    // A single wood item will just maintain at the current level.
                                    cur->setFieldAge(cur->getFieldAge() - 1);
                                    if( one_in(50) ) {
                                        destroyed = it->burn(cur->getFieldDensity());
                                    }
                                    smoke++;
                                    consumed++;
                                } else if (it->burnt < cur->getFieldDensity()) {
                                    destroyed = it->burn(1);
                                    smoke++;
                                }

                            } else if ((it->made_of("cotton") || it->made_of("wool"))) {
                                //Cotton and Wool burn slowly but don't feed the fire much.
                                if (vol <= cur->getFieldDensity() * 5 || cur->getFieldDensity() == 3) {
                                    cur->setFieldAge(cur->getFieldAge() - 1);
                                    destroyed = it->burn(cur->getFieldDensity());
                                    smoke++;
                                    consumed++;
                                } else if (it->burnt < cur->getFieldDensity()) {
                                    destroyed = it->burn(1);
                                    smoke++;
                                }

                            } else if ((it->made_of("flesh")) || (it->made_of("hflesh")) || (it->made_of("iflesh"))) {
                                //Same as cotton/wool really but more smokey.
                                if (vol <= cur->getFieldDensity() * 5 || (cur->getFieldDensity() == 3 && one_in(vol / 20))) {
                                    cur->setFieldAge(cur->getFieldAge() - 1);
                                    destroyed = it->burn(cur->getFieldDensity());
                                    smoke += 3;
                                    consumed++;
                                } else if (it->burnt < cur->getFieldDensity() * 5 || cur->getFieldDensity() >= 2) {
                                    destroyed = it->burn(1);
                                    smoke++;
                                }

                            } else if (it->made_of(LIQUID)) {
                                //Lots of smoke if alcohol, and LOTS of fire fueling power, kills a fire otherwise.
                                if(it->type->id == "tequila" || it->type->id == "whiskey" ||
                                   it->type->id == "vodka" || it->type->id == "rum" || it->type->id == "gasoline") {
                                    cur->setFieldAge(cur->getFieldAge() - 300);
                                    smoke += 6;
                                } else {
                                    cur->setFieldAge(cur->getFieldAge() + rng(80 * vol, 300 * vol));
                                    smoke++;
                                }
                                it->charges -= cur->getFieldDensity();
                                if(it->charges <= 0) {
                                    destroyed = true;
                                }
                                consumed++;
                            } else if (it->made_of("powder")) {
                                //Any powder will fuel the fire as much as its volume but be immediately destroyed.
                                cur->setFieldAge(cur->getFieldAge() - vol);
                                destroyed = true;
                                smoke += 2;

                            } else if (it->made_of("plastic")) {
                                //Smokey material, doesn't fuel well.
                                smoke += 3;
                                if (it->burnt <= cur->getFieldDensity() * 2 || (cur->getFieldDensity() == 3 && one_in(vol))) {
                                    destroyed = it->burn(cur->getFieldDensity());
                                    if (one_in(vol + it->burnt)) {
                                        cur->setFieldAge(cur->getFieldAge() - 1);
                                    }
                                }
                            }

                            if (destroyed) {
                                //If we decided the item was destroyed by fire, remove it.
                                for (int m = 0; m < i_at(x, y)[i].contents.size(); m++) {
                                    i_at(x, y).push_back( i_at(x, y)[i].contents[m] );
                                }
                                i_at(x, y).erase(i_at(x, y).begin() + i);
                                i--;
                            }
                        }

                        veh = veh_at(x, y, part); //Get the part of the vehicle in the fire.
                        if (veh) {
                            veh->damage (part, cur->getFieldDensity() * 10, false);    //Damage the vehicle in the fire.
                        }
                        // If the flames are in a brazier, they're fully contained, so skip consuming terrain
                        if((tr_brazier != tr_at(x, y)) && (has_flag("FIRE_CONTAINER", x, y) != true )) {
                            // Consume the terrain we're on
                            if (has_flag("EXPLODES", x, y)) {
                                //This is what destroys houses so fast.
                                ter_set(x, y, ter_id(int(ter(x, y)) + 1));
                                cur->setFieldAge(0); //Fresh level 3 fire.
                                cur->setFieldDensity(3);
                                g->explosion(x, y, 40, 0, true); //Boom.

                            } else if (has_flag("FLAMMABLE", x, y) && one_in(32 - cur->getFieldDensity() * 10)) {
                                //The fire feeds on the ground itself until max density.
                                cur->setFieldAge(cur->getFieldAge() - cur->getFieldDensity() * cur->getFieldDensity() * 40);
                                smoke += 15;
                                if (cur->getFieldDensity() == 3) {
                                    g->m.destroy(x, y, false);
                                }

                            } else if (has_flag("FLAMMABLE_ASH", x, y) && one_in(32 - cur->getFieldDensity() * 10)) {
                                //The fire feeds on the ground itself until max density.
                                cur->setFieldAge(cur->getFieldAge() - cur->getFieldDensity() * cur->getFieldDensity() * 40);
                                smoke += 15;
                                if (cur->getFieldDensity() == 3 || cur->getFieldAge() < -600) {
                                    ter_set(x, y, t_ash);
                                    if(has_furn(x, y)) {
                                        furn_set(x, y, f_null);
                                    }
                                }

                            } else if (has_flag("FLAMMABLE_HARD", x, y) && one_in(62 - cur->getFieldDensity() * 10)) {
                                //The fire feeds on the ground itself until max density.
                                cur->setFieldAge(cur->getFieldAge() - cur->getFieldDensity() * cur->getFieldDensity() * 30);
                                smoke += 10;
                                if (cur->getFieldDensity() == 3 || cur->getFieldAge() < -600) {
                                    g->m.destroy(x, y, false);
                                }

                            } else if (terlist[ter(x, y)].has_flag("SWIMMABLE")) {
                                cur->setFieldAge(cur->getFieldAge() + 800);    // Flames die quickly on water
                            }
                        }

                        // If the flames are in a pit, it can't spread to non-pit
                        bool in_pit = (ter(x, y) == t_pit);

                        // If the flames are REALLY big, they contribute to adjacent flames
                        if (cur->getFieldAge() < 0 && tr_brazier != tr_at(x, y) &&
                            (has_flag("FIRE_CONTAINER", x, y) != true  ) ) {
                            if(cur->getFieldDensity() == 3) {
                                // Randomly offset our x/y shifts by 0-2, to randomly pick a square to spread to
                                int starti = rng(0, 2);
                                int startj = rng(0, 2);
                                tmpfld = NULL;
                                // Basically: Scan around for a spot,
                                // if there is more fire there, make it bigger and both flames renew in power
                                // This is how level 3 fires spend their excess age: making other fires bigger. Flashpoint.
                                for (int i = 0; i < 3 && cur->getFieldAge() < 0; i++) {
                                    for (int j = 0; j < 3 && cur->getFieldAge() < 0; j++) {
                                        int fx = x + ((i + starti) % 3) - 1, fy = y + ((j + startj) % 3) - 1;
                                        tmpfld = field_at(fx, fy).findField(fd_fire);
                                        if (tmpfld && tmpfld != cur && cur->getFieldAge() < 0 && tmpfld->getFieldDensity() < 3 &&
                                            (in_pit == (ter(fx, fy) == t_pit))) {
                                            tmpfld->setFieldDensity(tmpfld->getFieldDensity() + 1);
                                            cur->setFieldAge(cur->getFieldAge() + 150);
                                        }
                                    }
                                }
                            } else {
                                // See if we can grow into a stage 2/3 fire, for this
                                // burning neighbours are necessary in addition to
                                // field age < 0, or alternatively, a LOT of fuel.

                                // The maximum fire density is 1 for a lone fire, 2 for at least 1 neighbour,
                                // 3 for at least 2 neighbours.
                                int maximum_density =  1;

                                // The following logic looks a bit complex due to optimization concerns, so here are the semantics:
                                // 1. Calculate maximum field density based on fuel, -500 is 2(raging), -1000 is 3(inferno)
                                // 2. Calculate maximum field density based on neighbours, 1 neighbours is 2(raging), 2 or more neighbours is 3(inferno)
                                // 3. Pick the higher maximum between 1. and 2.
                                // We don't just calculate both maximums and pick the higher because the adjacent field lookup is quite expensive and should be avoided if possible.
                                if(cur->getFieldAge() < -1500) {
                                    maximum_density = 3;
                                } else {
                                    int adjacent_fires = 0;

                                    for (int i = 0; i < 3; i++) {
                                        for (int j = 0; j < 3; j++) {
                                            int fx = x + (i % 3) - 1, fy = y + (j % 3) - 1;
                                            tmpfld = field_at(fx, fy).findField(fd_fire);
                                            if (tmpfld && tmpfld != cur) {
                                                adjacent_fires++;
                                            }
                                        }
                                    }
                                    maximum_density = 1 + (adjacent_fires >= 1) + (adjacent_fires >= 2);

                                    if(maximum_density < 2 && cur->getFieldAge() < -500) {
                                        maximum_density = 2;
                                    }
                                }

                                // If we consumed a lot, the flames grow higher
                                while (cur->getFieldDensity() < maximum_density && cur->getFieldAge() < 0) {
                                    //Fires under 0 age grow in size. Level 3 fires under 0 spread later on.
                                    cur->setFieldAge(cur->getFieldAge() + 300);
                                    cur->setFieldDensity(cur->getFieldDensity() + 1);
                                }
                            }
                        }

                        // Consume adjacent fuel / terrain / webs to spread.
                        // Randomly offset our x/y shifts by 0-2, to randomly pick a square to spread to
                        //Fires can only spread under 30 age. This is arbitrary but seems to work well.
                        //The reason is to differentiate a fire that spawned vs one created really.
                        //Fires spawned by fire in a new square START at 30 age, so if its a square with no fuel on it
                        //the fire won't keep crawling endlessly across the map.
                        int starti = rng(0, 2);
                        int startj = rng(0, 2);
                        for (int i = 0; i < 3; i++) {
                            for (int j = 0; j < 3; j++) {
                                int fx = x + ((i + starti) % 3) - 1, fy = y + ((j + startj) % 3) - 1;
                                if (INBOUNDS(fx, fy)) {
                                    field &nearby_field = g->m.field_at(fx, fy);
                                    field_entry *nearwebfld = nearby_field.findField(fd_web);
                                    int spread_chance = 25 * (cur->getFieldDensity() - 1);
                                    if (nearwebfld) {
                                        spread_chance = 50 + spread_chance / 2;
                                    }
                                    if (has_flag("EXPLODES", fx, fy) && one_in(8 - cur->getFieldDensity()) &&
                                        tr_brazier != tr_at(x, y) && (has_flag("FIRE_CONTAINER", x, y) != true ) ) {
                                        ter_set(fx, fy, ter_id(int(ter(fx, fy)) + 1));
                                        g->explosion(fx, fy, 40, 0, true); //Nearby explodables? blow em up.
                                    } else if ((i != 0 || j != 0) && rng(1, 100) < spread_chance && cur->getFieldAge() < 200 &&
                                               tr_brazier != tr_at(x, y) &&
                                               (has_flag("FIRE_CONTAINER", x, y) != true ) &&
                                               (in_pit == (ter(fx, fy) == t_pit)) &&
                                               (
                                                   (cur->getFieldDensity() >= 2 &&
                                                    (has_flag("FLAMMABLE", fx, fy) && one_in(20))) ||
                                                   (cur->getFieldDensity() >= 2  &&
                                                    (has_flag("FLAMMABLE_ASH", fx, fy) && one_in(10))) ||
                                                   (cur->getFieldDensity() == 3  &&
                                                    (has_flag("FLAMMABLE_HARD", fx, fy) && one_in(10))) ||
                                                   flammable_items_at(fx, fy) ||
                                                   nearwebfld )) {
                                        add_field(fx, fy, fd_fire, 1); //Nearby open flammable ground? Set it on fire.
                                        tmpfld = nearby_field.findField(fd_fire);
                                        if(tmpfld) {
                                            tmpfld->setFieldAge(100);
                                            cur->setFieldAge(cur->getFieldAge() + 50);
                                        }
                                        if(nearwebfld) {
                                            if (fx == x && fy == y){
                                                it = nearby_field.removeField(fd_web);
                                                skipIterIncr = true;
                                            } else {
                                                nearby_field.removeField(fd_web);
                                            }
                                        }
                                    } else {
                                        bool nosmoke = true;
                                        for (int ii = -1; ii <= 1; ii++) {
                                            for (int jj = -1; jj <= 1; jj++) {
                                                field &spreading_field = g->m.field_at(x + ii, y + jj);

                                                tmpfld = spreading_field.findField(fd_fire);
                                                int tmpflddens = ( tmpfld ? tmpfld->getFieldDensity() : 0 );
                                                if ( ( tmpflddens == 3 ) || ( tmpflddens == 2 && one_in(4) ) ) {
                                                    smoke++; //The higher this gets, the more likely for smoke.
                                                } else if (spreading_field.findField(fd_smoke)) {
                                                    nosmoke = false; //slightly, slightly, less likely to make smoke if there is already smoke
                                                }
                                            }
                                        }
                                        // If we're not spreading, maybe we'll stick out some smoke, huh?
                                        if(!(is_outside(fx, fy))) {
                                            //Lets make more smoke indoors since it doesn't dissipate
                                            smoke += 10; //10 is just a magic number. To much smoke indoors? Lower it. To little, raise it.
                                        }
                                        if (move_cost(fx, fy) > 0 &&
                                            (rng(0, 100) <= smoke || (nosmoke && one_in(40))) &&
                                            rng(3, 35) < cur->getFieldDensity() * 5 && cur->getFieldAge() < 1000 &&
                                            (has_flag("SUPPRESS_SMOKE", x, y) != true )) {
                                            smoke--;
                                            add_field(fx, fy, fd_smoke, rng(1, cur->getFieldDensity())); //Add smoke!
                                        }
                                    }
                                }
                            }
                        }
                    }
                    break;

                    case fd_smoke:
                        spread_gas( this, cur, x, y, curtype, 80, 50 );
                        break;

                    case fd_tear_gas:
                        spread_gas( this, cur, x, y, curtype, 33, 30 );
                        break;

                    case fd_toxic_gas:
                        spread_gas( this, cur, x, y, curtype, 50, 30 );
                        break;


                    case fd_nuke_gas:
                        radiation(x, y) += rng(0, cur->getFieldDensity());
                        spread_gas( this, cur, x, y, curtype, 50, 10 );
                        break;

                    case fd_gas_vent:
                        for (int i = x - 1; i <= x + 1; i++) {
                            for (int j = y - 1; j <= y + 1; j++) {
                                field &wandering_field = field_at(i, j);
                                tmpfld = NULL;
                                tmpfld = wandering_field.findField(fd_toxic_gas);
                                if (tmpfld && tmpfld->getFieldDensity() < 3) {
                                    tmpfld->setFieldDensity(tmpfld->getFieldDensity() + 1);
                                } else {
                                    add_field(i, j, fd_toxic_gas, 3);
                                }
                            }
                        }
                        break;

                    case fd_fire_vent:
                        if (cur->getFieldDensity() > 1) {
                            if (one_in(3)) {
                                cur->setFieldDensity(cur->getFieldDensity() - 1);
                            }
                        } else {
                            it = curfield.replaceField(fd_fire_vent, fd_flame_burst);
                            cur->setFieldDensity(3);
                            continue;
                        }
                        break;

                    case fd_flame_burst:
                        if (cur->getFieldDensity() > 1) {
                            cur->setFieldDensity(cur->getFieldDensity() - 1);
                        } else {
                            it = curfield.replaceField(fd_flame_burst, fd_fire_vent);
                            cur->setFieldDensity(3);
                            continue;
                        }
                        break;

                    case fd_electricity:

                        if (!one_in(5)) {   // 4 in 5 chance to spread
                            std::vector<point> valid;
                            if (move_cost(x, y) == 0 && cur->getFieldDensity() > 1) { // We're grounded
                                int tries = 0;
                                while (tries < 10 && cur->getFieldAge() < 50 && cur->getFieldDensity() > 1) {
                                    int cx = x + rng(-1, 1), cy = y + rng(-1, 1);
                                    if (move_cost(cx, cy) != 0) {
                                        add_field(point(cx, cy), fd_electricity, 1, cur->getFieldAge() + 1);
                                        cur->setFieldDensity(cur->getFieldDensity() - 1);
                                        tries = 0;
                                    } else {
                                        tries++;
                                    }
                                }
                            } else {    // We're not grounded; attempt to ground
                                for (int a = -1; a <= 1; a++) {
                                    for (int b = -1; b <= 1; b++) {
                                        if (move_cost(x + a, y + b) == 0) // Grounded tiles first

                                        {
                                            valid.push_back(point(x + a, y + b));
                                        }
                                    }
                                }
                                if (valid.empty()) {    // Spread to adjacent space, then
                                    int px = x + rng(-1, 1), py = y + rng(-1, 1);
                                    if (move_cost(px, py) > 0 && field_at(px, py).findField(fd_electricity) &&
                                        field_at(px, py).findField(fd_electricity)->getFieldDensity() < 3) {
                                        field_at(px, py).findField(fd_electricity)->setFieldDensity(field_at(px,
                                                py).findField(fd_electricity)->getFieldDensity() + 1);
                                        cur->setFieldDensity(cur->getFieldDensity() - 1);
                                    } else if (move_cost(px, py) > 0) {
                                        add_field(point(px, py), fd_electricity, 1, cur->getFieldAge() + 1);
                                    }
                                    cur->setFieldDensity(cur->getFieldDensity() - 1);
                                }
                                while (!valid.empty() && cur->getFieldDensity() > 1) {
                                    int index = rng(0, valid.size() - 1);
                                    add_field(valid[index], fd_electricity, 1, cur->getFieldAge() + 1);
                                    cur->setFieldDensity(cur->getFieldDensity() - 1);
                                    valid.erase(valid.begin() + index);
                                }
                            }
                        }
                        break;

                    case fd_fatigue:{
                        std::string monids[9] = {"mon_flying_polyp", "mon_hunting_horror", "mon_mi_go", "mon_yugg", "mon_gelatin", "mon_flaming_eye", "mon_kreck", "mon_gracke", "mon_blank"};
                        if (cur->getFieldDensity() < 3 && int(g->turn) % 3600 == 0 && one_in(10)) {
                            cur->setFieldDensity(cur->getFieldDensity() + 1);
                        } else if (cur->getFieldDensity() == 3 && one_in(600)) { // Spawn nether creature!
                            std::string type = monids[(rng(0, 9))];
                            monster creature(GetMType(type));
                            creature.spawn(x + rng(-3, 3), y + rng(-3, 3));
                            g->add_zombie(creature);
                        }
                    }
                        break;

                    case fd_push_items: {
                        std::vector<item> *it = &(i_at(x, y));
                        for (int i = 0; i < it->size(); i++) {
                            if ((*it)[i].type->id != "rock" || (*it)[i].bday >= int(g->turn) - 1) {
                                i++;
                            } else {
                                item tmp = (*it)[i];
                                tmp.bday = int(g->turn);
                                it->erase(it->begin() + i);
                                i--;
                                std::vector<point> valid;
                                for (int xx = x - 1; xx <= x + 1; xx++) {
                                    for (int yy = y - 1; yy <= y + 1; yy++) {
                                        if (field_at(xx, yy).findField(fd_push_items)) {
                                            valid.push_back( point(xx, yy) );
                                        }
                                    }
                                }
                                if (!valid.empty()) {
                                    point newp = valid[rng(0, valid.size() - 1)];
                                    add_item_or_charges(newp.x, newp.y, tmp);
                                    if (g->u.posx == newp.x && g->u.posy == newp.y) {
                                        g->add_msg(_("A %s hits you!"), tmp.tname().c_str());
                                        body_part hit = random_body_part();
                                        int side = random_side(hit);
                                        g->u.hit(NULL, hit, side, 6, 0);
                                    }
                                    int npcdex = g->npc_at(newp.x, newp.y),
                                        mondex = g->mon_at(newp.x, newp.y);

                                    if (npcdex != -1) {
                                        npc *p = g->active_npc[npcdex];
                                        body_part hit = random_body_part();
                                        int side = random_side(hit);
                                        p->hit(NULL, hit, side, 6, 0);
                                        if (g->u_see(newp.x, newp.y)) {
                                            g->add_msg(_("A %s hits %s!"), tmp.tname().c_str(), p->name.c_str());
                                        }
                                    }

                                    if (mondex != -1) {
                                        monster *mon = &(g->zombie(mondex));
                                        mon->hurt(6 - mon->get_armor_bash(bp_torso));
                                        if (g->u_see(newp.x, newp.y))
                                            g->add_msg(_("A %s hits the %s!"), tmp.tname().c_str(),
                                                       mon->name().c_str());
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
                                int boltx = x, bolty = y;
                                for (int n = 0; n < dist; n++) {
                                    boltx += xdir;
                                    bolty += ydir;
                                    add_field(boltx, bolty, fd_electricity, rng(2, 3));
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
                            for (int i = x - 5; i <= x + 5; i++) {
                                for (int j = y - 5; j <= y + 5; j++) {
                                    field &wandering_field = g->m.field_at(i, j);
                                    if (wandering_field.findField(fd_acid)) {
                                        if (wandering_field.findField(fd_acid)->getFieldDensity() == 0) {
                                            int newdens = 3 - (rl_dist(x, y, i, j) / 2) + (one_in(3) ? 1 : 0);
                                            if (newdens > 3) {
                                                newdens = 3;
                                            }
                                            if (newdens > 0) {
                                                add_field(i, j, fd_acid, newdens);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        break;

                } // switch (curtype)

                cur->setFieldAge(cur->getFieldAge() + 1);
                if (fieldlist[cur->getFieldType()].halflife > 0) {
                    bool should_dissipate = false;
                    if (cur->getFieldAge() > 0 &&
                        dice(2, cur->getFieldAge()) > fieldlist[cur->getFieldType()].halflife) {
                        cur->setFieldAge(0);
                        if(cur->getFieldDensity() == 1 || !cur->isAlive()) {
                            should_dissipate = true;
                        }
                        cur->setFieldDensity(cur->getFieldDensity() - 1);
                    }
                    if (should_dissipate == true || !cur->isAlive()) { // Totally dissapated.
                        grid[gridn]->field_count--;
                        it = grid[gridn]->fld[locx][locy].removeField(cur->getFieldType());
                        continue;
                    }
                }
                if (!skipIterIncr)
                    ++it;
                skipIterIncr = false;
            }
        }
    }
    return found_field;
}

//This entire function makes very little sense. Why are the rules the way they are? Why does walking into some things destroy them but not others?

/*
Function: step_in_field
Triggers any active abilities a field effect would have. Fire burns you, acid melts you, etc.
If you add a field effect that interacts with the player place a case statement in the switch here.
If you wish for a field effect to do something over time (propagate, interact with terrain, etc) place it in process_subfields
*/
void map::step_in_field(int x, int y)
{
    // A copy of the current field for reference. Do not add fields to it, use map::add_field
    field &curfield = field_at(x, y);
    field_entry *cur = NULL; // The current field effect.
    int veh_part; // vehicle part existing on this tile.
    vehicle *veh = NULL; // Vehicle reference if there is one.
    bool inside = false; // Are we inside?
    // For use in determining if we are in a rubble square or not, for the disease effect
    bool no_rubble = true;
    //to modify power of a field based on... whatever is relevant for the effect.
    int adjusted_intensity;

    //If we are in a vehicle figure out if we are inside (reduces effects usually)
    // and what part of the vehicle we need to deal with.
    if (g->u.in_vehicle) {
        veh = g->m.veh_at(x, y, veh_part);
        inside = (veh && veh->is_inside(veh_part));
    }

    // Iterate through all field effects on this tile.
    // When removing a field, do field_list_it = curfield.removeField(type) and continue
    // This ensures proper iteration through the fields.
    for(std::map<field_id, field_entry*>::iterator field_list_it = curfield.getFieldStart();
        field_list_it != curfield.getFieldEnd();){
        cur = field_list_it->second;
        // Shouldn't happen unless you free memory of field entries manually
        // (hint: don't do that)... Pointer safety.
        if(cur == NULL) continue;

        if (cur->getFieldType() == fd_rubble) {
             //We found rubble, don't remove the players rubble disease at the end of function.
            no_rubble = false;
        }

        //Do things based on what field effect we are currently in.
        switch (cur->getFieldType()) {
        case fd_null:
        case fd_blood: // It doesn't actually do anything //necessary to add other types of blood?
        case fd_bile:  // Ditto
            //break instead of return in the event of post-processing in the future;
            // also we're in a loop now!
            break;

        case fd_web: {
            //If we are in a web, can't walk in webs or are in a vehicle, get webbed maybe.
            //Moving through multiple webs stacks the effect.
            if (!g->u.has_trait("WEB_WALKER") && !g->u.in_vehicle) {
                //between 5 and 15 minus your current web level.
                int web = cur->getFieldDensity() * 5 - g->u.disease_duration("webbed");
                if (web > 0) { g->u.add_disease("webbed", web); }
                field_list_it = curfield.removeField( fd_web ); //Its spent.
                continue;
                //If you are in a vehicle destroy the web.
                //It should of been destroyed when you ran over it anyway.
            } else if (g->u.in_vehicle) {
                field_list_it = curfield.removeField( fd_web );
                continue;
            }
        } break;

        case fd_acid:
            //Acid deals damage at all levels now; the inside refers to inside a vehicle.
            //TODO: Add resistance to this with rubber shoes or something?
            if (cur->getFieldDensity() == 3 && !inside) {
                g->add_msg(_("The acid burns your legs and feet!"));
                g->u.hit(NULL, bp_feet, 0, 0, rng(4, 10));
                g->u.hit(NULL, bp_feet, 1, 0, rng(4, 10));
                g->u.hit(NULL, bp_legs, 0, 0, rng(2,  8));
                g->u.hit(NULL, bp_legs, 1, 0, rng(2,  8));
            } else if (cur->getFieldDensity() == 2 && !inside) {
                g->u.hit(NULL, bp_feet, 0, 0, rng(2, 5));
                g->u.hit(NULL, bp_feet, 1, 0, rng(2, 5));
                g->u.hit(NULL, bp_legs, 0, 0, rng(1,  4));
                g->u.hit(NULL, bp_legs, 1, 0, rng(1,  4));
            } else if (!inside) {
                g->u.hit(NULL, bp_feet, 0, 0, rng(1, 3));
                g->u.hit(NULL, bp_feet, 1, 0, rng(1, 3));
                g->u.hit(NULL, bp_legs, 0, 0, rng(0,  2));
                g->u.hit(NULL, bp_legs, 1, 0, rng(0,  2));
            }
            break;

        case fd_sap:
            //Sap causes the player to get sap disease, slowing them down.
            if( g->u.in_vehicle ) break; //sap does nothing to cars.
            g->add_msg(_("The sap sticks to you!"));
            g->u.add_disease("sap", cur->getFieldDensity() * 2);
            if (cur->getFieldDensity() == 1) {
                field_list_it = curfield.removeField( fd_sap );
                continue;
            } else {
                cur->setFieldDensity(cur->getFieldDensity() - 1); //Use up sap.
            }
            break;

        case fd_sludge:
            g->add_msg(_("The sludge is thick and sticky. You struggle to pull free."));
            g->u.moves -= cur->getFieldDensity() * 300;
            curfield.removeField( fd_sludge );
            break;

        case fd_fire:
            //Burn the player. Less so if you are in a car or ON a car.
            adjusted_intensity = cur->getFieldDensity();
            if( g->u.in_vehicle ) {
                if( inside ) {
                    adjusted_intensity -= 2;
                } else {
                    adjusted_intensity -= 1;
                }
            }
            if (!g->u.has_active_bionic("bio_heatsink")) { //heatsink prevents ALL fire damage.
                if (adjusted_intensity == 1) {
                    g->add_msg(_("You burn your legs and feet!"));
                    g->u.hit(NULL, bp_feet, 0, 0, rng(2, 6));
                    g->u.hit(NULL, bp_feet, 1, 0, rng(2, 6));
                    g->u.hit(NULL, bp_legs, 0, 0, rng(1, 4));
                    g->u.hit(NULL, bp_legs, 1, 0, rng(1, 4));
                } else if (adjusted_intensity == 2) {
                    g->add_msg(_("You're burning up!"));
                    g->u.hit(NULL, bp_legs, 0, 0,  rng(2, 6));
                    g->u.hit(NULL, bp_legs, 1, 0,  rng(2, 6));
                    g->u.hit(NULL, bp_torso, -1, 4, rng(4, 9));
                } else if (adjusted_intensity == 3) {
                    g->add_msg(_("You're set ablaze!"));
                    g->u.hit(NULL, bp_legs, 0, 0, rng(2, 6));
                    g->u.hit(NULL, bp_legs, 1, 0, rng(2, 6));
                    g->u.hit(NULL, bp_torso, -1, 4, rng(4, 9));
                    g->u.add_effect("onfire", 5); //lasting fire damage only from the strongest fires.
                }
            }
            break;

        case fd_rubble:
            //You are walking on rubble. Slow down.
            g->u.add_disease("bouldering", 0, false, cur->getFieldDensity(), 3);
            break;

        case fd_smoke:
            {
                if (!inside) {
                    //Get smoke disease from standing in smoke.
                    signed char density = cur->getFieldDensity();
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
                    g->u.add_env_effect("smoke", bp_mouth, coughStr, coughDur);
                }
            }
            break;

        case fd_tear_gas:
            //Tear gas will both give you teargas disease and/or blind you.
            if ((cur->getFieldDensity() > 1 || !one_in(3)) && (!inside || (inside && one_in(3))))
            {
                g->u.add_env_effect("teargas", bp_mouth, 5, 20);
            }
            if (cur->getFieldDensity() > 1 && (!inside || (inside && one_in(3))))
            {
                g->u.add_env_effect("blind", bp_eyes, cur->getFieldDensity() * 2, 10);
            }
            break;

        case fd_toxic_gas:
            // Toxic gas at low levels poisons you.
            // Toxic gas at high levels will cause very nasty poison.
            if (cur->getFieldDensity() == 2 && (!inside || (cur->getFieldDensity() == 3 && inside))) {
                g->u.add_env_effect("poison", bp_mouth, 5, 30);
            }
            else if (cur->getFieldDensity() == 3 && !inside)
            {
                g->u.infect("badpoison", bp_mouth, 5, 30);
            } else if (cur->getFieldDensity() == 1 && (!inside)) {
                g->u.add_env_effect("poison", bp_mouth, 2, 20);
            }
            break;

        case fd_nuke_gas:
            // Get irradiated by the nuclear fallout.
            // Changed to min of density, not 0.
            g->u.radiation += rng(cur->getFieldDensity(),
                                  cur->getFieldDensity() * (cur->getFieldDensity() + 1));
            if (cur->getFieldDensity() == 3) {
                g->add_msg(_("This radioactive gas burns!"));
                g->u.hurtall(rng(1, 3));
            }
            break;

        case fd_flame_burst:
            //A burst of flame? Only hits the legs and torso.
            if (inside) break; //fireballs can't touch you inside a car.
            if (!g->u.has_active_bionic("bio_heatsink")) { //heatsink stops fire.
                g->add_msg(_("You're torched by flames!"));
                g->u.hit(NULL, bp_legs, 0, 0,  rng(2, 6));
                g->u.hit(NULL, bp_legs, 1, 0,  rng(2, 6));
                g->u.hit(NULL, bp_torso, -1, 4, rng(4, 9));
            } else
                g->add_msg(_("These flames do not burn you."));
            break;

        case fd_electricity:
            if (g->u.has_artifact_with(AEP_RESIST_ELECTRICITY) || g->u.has_active_bionic("bio_faraday")) //Artifact or bionic stops electricity.
                g->add_msg(_("The electricity flows around you."));
            else {
                g->add_msg(_("You're electrocuted!"));
                //small universal damage based on density.
                g->u.hurtall(rng(1, cur->getFieldDensity()));
                if (one_in(8 - cur->getFieldDensity()) && !one_in(30 - g->u.str_cur)) {
                    //str of 30 stops this from happening.
                    g->add_msg(_("You're paralyzed!"));
                    g->u.moves -= rng(cur->getFieldDensity() * 150, cur->getFieldDensity() * 200);
                }
            }
            break;

        case fd_fatigue:
            //Teleports you... somewhere.
            if (rng(0, 2) < cur->getFieldDensity()) {
                g->add_msg(_("You're violently teleported!"));
                g->u.hurtall(cur->getFieldDensity());
                g->teleport();
            }
            break;

            //Why do these get removed???
        case fd_shock_vent:
            //Stepping on a shock vent shuts it down.
            field_list_it = curfield.removeField( fd_shock_vent );
            continue;

        case fd_acid_vent:
            //Stepping on an acid vent shuts it down.
            field_list_it = curfield.removeField( fd_acid_vent );
            continue;
        }
        ++field_list_it;
    }

    if(no_rubble) {
        //After iterating through all fields, if we found no rubble, remove the rubble disease.
        g->u.rem_disease("bouldering");
    }
}

void map::mon_in_field(int x, int y, monster *z)
{
    if (z->digging()) {
        return; // Digging monsters are immune to fields
    }
    field &curfield = field_at(x, y);
    field_entry *cur = NULL;

    int dam = 0;
    for( std::map<field_id, field_entry*>::iterator field_list_it = curfield.getFieldStart();
         field_list_it != curfield.getFieldEnd(); ) {
        cur = field_list_it->second;
        //shouldn't happen unless you free memory of field entries manually (hint: don't do that)
        if(cur == NULL) continue;

        switch (cur->getFieldType()) {
        case fd_null:
        case fd_blood: // It doesn't actually do anything
        case fd_bile:  // Ditto
            break;

        case fd_web:
            if (!z->has_flag(MF_WEBWALK)) {
                z->moves *= .8;
                field_list_it = curfield.removeField( fd_web );
                continue;
            }
            break;

 // TODO: Use acid resistance
        case fd_acid:
            if (!z->has_flag(MF_FLIES) && !z->has_flag(MF_ACIDPROOF)) {
                if (cur->getFieldDensity() == 3) {
                    dam += rng(4, 10) + rng(2, 8);
                } else {
                    dam += rng(cur->getFieldDensity(), cur->getFieldDensity() * 4);
                }
            }
            break;

        case fd_sap:
            z->moves -= cur->getFieldDensity() * 5;
            if (cur->getFieldDensity() == 1) {
                field_list_it = curfield.removeField( fd_sap );
                continue;
            } else {
                cur->setFieldDensity(cur->getFieldDensity() - 1);
            }
            break;

        case fd_sludge:
            if (!z->has_flag(MF_DIGS) && !z->has_flag(MF_FLIES) &&
                !z->has_flag(MF_SLUDGEPROOF)) {
              z->moves -= cur->getFieldDensity() * 300;
              curfield.removeField( fd_sludge );
            }
            break;

            // MATERIALS-TODO: Use fire resistance
        case fd_fire:
            if ( z->made_of("flesh") || z->made_of("hflesh") || z->made_of("iflesh") ) {
                dam += 3;
            }
            if (z->made_of("veggy")) {
                dam += 12;
            }
            if (z->made_of("paper") || z->made_of(LIQUID) || z->made_of("powder") ||
                z->made_of("wood")  || z->made_of("cotton") || z->made_of("wool")) {
                dam += 20;
            }
            if (z->made_of("stone") || z->made_of("kevlar") || z->made_of("steel")) {
                dam += -20;
            }
            if (z->has_flag(MF_FLIES)) {
                dam -= 15;
            }

            if (cur->getFieldDensity() == 1) {
                dam += rng(2, 6);
            } else if (cur->getFieldDensity() == 2) {
                dam += rng(6, 12);
                if (!z->has_flag(MF_FLIES)) {
                    z->moves -= 20;
                    if (!z->made_of(LIQUID) && !z->made_of("stone") && !z->made_of("kevlar") &&
                        !z->made_of("steel") && !z->has_flag(MF_FIREY)) {
                        z->add_effect("onfire", rng(3, 8));
                    }
                }
            } else if (cur->getFieldDensity() == 3) {
                dam += rng(10, 20);
                if (!z->has_flag(MF_FLIES) || one_in(3)) {
                    z->moves -= 40;
                    if (!z->made_of(LIQUID) && !z->made_of("stone") && !z->made_of("kevlar") &&
                        !z->made_of("steel") && !z->has_flag(MF_FIREY)) {
                        z->add_effect("onfire", rng(8, 12));
                    }
                }
            }
            // Drop through to smoke no longer needed as smoke will exist in the same square now,
            // this would double apply otherwise.
            break;

        case fd_rubble:
            if (!z->has_flag(MF_FLIES) && !z->has_flag(MF_AQUATIC)) {
                z->add_effect("bouldering", 1);
            }
            break;

        case fd_smoke:
            if (!z->has_flag(MF_NO_BREATHE)) {
                if (cur->getFieldDensity() == 3) {
                    z->moves -= rng(10, 20);
                }
                if (z->made_of("veggy")) { // Plants suffer from smoke even worse
                    z->moves -= rng(1, cur->getFieldDensity() * 12);
                }
            }
            break;

        case fd_tear_gas:
            if ((z->made_of("flesh") || z->made_of("hflesh") || z->made_of("veggy") || z->made_of("iflesh")) &&
                !z->has_flag(MF_NO_BREATHE)) {
                if (cur->getFieldDensity() == 3) {
                    z->add_effect("stunned", rng(10, 20));
                    dam += rng(4, 10);
                } else if (cur->getFieldDensity() == 2) {
                    z->add_effect("stunned", rng(5, 10));
                    dam += rng(2, 5);
                } else {
                    z->add_effect("stunned", rng(1, 5));
                }
                if (z->made_of("veggy")) {
                    z->moves -= rng(cur->getFieldDensity() * 5, cur->getFieldDensity() * 12);
                    dam += cur->getFieldDensity() * rng(8, 14);
                }
                if (z->has_flag(MF_SEES)) {
                     z->add_effect("blind", cur->getFieldDensity() * 8);
                }
            }
            break;

        case fd_toxic_gas:
            if(!z->has_flag(MF_NO_BREATHE)) {
                dam += cur->getFieldDensity();
                z->moves -= cur->getFieldDensity();
            }
            break;

        case fd_nuke_gas:
            if(!z->has_flag(MF_NO_BREATHE)) {
                if (cur->getFieldDensity() == 3) {
                    z->moves -= rng(60, 120);
                    dam += rng(30, 50);
                } else if (cur->getFieldDensity() == 2) {
                    z->moves -= rng(20, 50);
                    dam += rng(10, 25);
                } else {
                    z->moves -= rng(0, 15);
                    dam += rng(0, 12);
                }
                if (z->made_of("veggy")) {
                    z->moves -= rng(cur->getFieldDensity() * 5, cur->getFieldDensity() * 12);
                    dam *= cur->getFieldDensity();
                }
            }
            break;

            // MATERIALS-TODO: Use fire resistance
        case fd_flame_burst:
            if (z->made_of("flesh") || z->made_of("hflesh") || z->made_of("iflesh")) {
                dam += 3;
            }
            if (z->made_of("veggy")) {
                dam += 12;
            }
            if (z->made_of("paper") || z->made_of(LIQUID) || z->made_of("powder") ||
                z->made_of("wood")  || z->made_of("cotton") || z->made_of("wool")) {
                dam += 50;
            }
            if (z->made_of("stone") || z->made_of("kevlar") || z->made_of("steel")) {
                dam += -25;
            }
            dam += rng(0, 8);
            z->moves -= 20;
            break;

        case fd_electricity:
            dam += rng(1, cur->getFieldDensity());
            if (one_in(8 - cur->getFieldDensity())) {
                z->moves -= cur->getFieldDensity() * 150;
            }
            break;

        case fd_fatigue:
            if (rng(0, 2) < cur->getFieldDensity()) {
                dam += cur->getFieldDensity();
                int tries = 0;
                int newposx, newposy;
                do {
                    newposx = rng(z->posx() - SEEX, z->posx() + SEEX);
                    newposy = rng(z->posy() - SEEY, z->posy() + SEEY);
                    tries++;
                } while (g->m.move_cost(newposx, newposy) == 0 && tries != 10);

                if (tries == 10) {
                    g->explode_mon(g->mon_at(z->posx(), z->posy()));
                } else {
                    int mon_hit = g->mon_at(newposx, newposy);
                    if (mon_hit != -1) {
                        if (g->u_see(z)) {
                            g->add_msg(_("The %s teleports into a %s, killing them both!"),
                                       z->name().c_str(), g->zombie(mon_hit).name().c_str());
                        }
                        g->explode_mon(mon_hit);
                    } else {
                        z->setpos(newposx, newposy);
                    }
                }
            }
            break;

        }
        ++field_list_it;
    }
    if (dam > 0) {
        z->hurt(dam);
    }
}

bool vector_has(std::vector <item> vec, itype_id type)
{
 for (int i = 0; i < vec.size(); i++) {
  if (vec[i].type->id == type)
   return true;
 }
 return false;
}

// TODO FIXME XXX: oh god the horror
void map::field_effect(int x, int y) //Applies effect of field immediately
{
 field_entry *cur = NULL;
 field &curfield = field_at(x, y);
 for(std::map<field_id, field_entry*>::iterator field_list_it = curfield.getFieldStart(); field_list_it != curfield.getFieldEnd(); ++field_list_it){
 cur = field_list_it->second;
 if(cur == NULL) continue;

 switch (cur->getFieldType()) {                        //Can add independent code for different types of fields to apply different effects
  case fd_rubble:
   int hit_chance = 10;
   int fdmon = g->mon_at(x, y);              //The index of the monster at (x,y), or -1 if there isn't one
   int fdnpc = g->npc_at(x, y);              //The index of the NPC at (x,y), or -1 if there isn't one
   npc *me = NULL;
   if (fdnpc != -1)
    me = g->active_npc[fdnpc];
   int veh_part;
   bool pc_inside = false;
   bool npc_inside = false;

   if (g->u.in_vehicle) {
    vehicle *veh = g->m.veh_at(x, y, veh_part);
    pc_inside = (veh && veh->is_inside(veh_part));
   }
   if (me && me->in_vehicle) {
    vehicle *veh = g->m.veh_at(x, y, veh_part);
    npc_inside = (veh && veh->is_inside(veh_part));
   }
   if (g->u.posx == x && g->u.posy == y && !pc_inside) {            //If there's a PC at (x,y) and he's not in a covered vehicle...
    if (g->u.get_dodge() < rng(1, hit_chance) || one_in(g->u.get_dodge())) {
     int how_many_limbs_hit = rng(0, num_hp_parts);
     for ( int i = 0 ; i < how_many_limbs_hit ; i++ ) {
      g->u.hp_cur[rng(0, num_hp_parts)] -= rng(0, 10);
      g->add_msg(_("You are hit by the falling debris!"));
     }
     if ((one_in(g->u.dex_cur)) && (((!(g->u.has_trait("LEG_TENT_BRACE")))) || (g->u.wearing_something_on(bp_feet))) ) {
      g->u.add_effect("downed", 2);
     }
     if (one_in(g->u.str_cur)) {
      g->u.add_effect("stunned", 2);
     }
    }
    else if ((one_in(g->u.str_cur)) && ((!(g->u.has_trait("LEG_TENT_BRACE"))) || (g->u.wearing_something_on(bp_feet))) ) {
     g->add_msg(_("You trip as you evade the falling debris!"));
     g->u.add_effect("downed", 1);
    }
                        //Avoiding disease system for the moment, since I was having trouble with it.
//    g->u.add_disease("crushed", 42, g);    //Using a disease allows for easy modification without messing with field code
 //   g->u.rem_disease("crushed");           //For instance, if we wanted to easily add a chance of limb mangling or a stun effect later
   }
   if (fdmon != -1 && fdmon < g->num_zombies()) {  //If there's a monster at (x,y)...
    monster* monhit = &(g->zombie(fdmon));
    int dam = 10;                             //This is a simplistic damage implementation. It can be improved, for instance to account for armor
    if (monhit->hurt(dam))                    //Ideally an external disease-like system would handle this to make it easier to modify later
     g->kill_mon(fdmon, false);
   }
   if (fdnpc != -1) {
    if (fdnpc < g->active_npc.size() && !npc_inside) { //If there's an NPC at (x,y) and he's not in a covered vehicle...
    if (me && (me->get_dodge() < rng(1, hit_chance) || one_in(me->get_dodge()))) {
      int how_many_limbs_hit = rng(0, num_hp_parts);
      for ( int i = 0 ; i < how_many_limbs_hit ; i++ ) {
       me->hp_cur[rng(0, num_hp_parts)] -= rng(0, 10);
      }
      // Not sure how to track what NPCs are wearing, and they're under revision anyway so leaving it checking player. :-/
      if ((one_in(me->dex_cur)) && ( ((!(g->u.has_trait("LEG_TENT_BRACE")))) || (g->u.wearing_something_on(bp_feet)) ) ) {
       me->add_effect("downed", 2);
      }
      if (one_in(me->str_cur)) {
       me->add_effect("stunned", 2);
      }
     }
     else if (me && (one_in(me->str_cur)) && ( ((!(g->u.has_trait("LEG_TENT_BRACE")))) || (g->u.wearing_something_on(bp_feet)) ) ) {
      me->add_effect("downed", 1);
     }
    }
    if (me && (me->hp_cur[hp_head]  <= 0 || me->hp_cur[hp_torso] <= 0)) {
     me->die(false);        //Right now cave-ins are treated as not the player's fault. This should be iterated on.
     g->active_npc.erase(g->active_npc.begin() + fdnpc);
    }                                       //Still need to add vehicle damage, but I'm ignoring that for now.
   }
    vehicle *veh = veh_at(x, y, veh_part);
    if (veh) {
     veh->damage(veh_part, ceil(veh->parts[veh_part].hp/3.0 * cur->getFieldDensity()), 1, false);
    }
 }
 }
}

int field_entry::move_cost() const{
  return fieldlist[type].move_cost[getFieldDensity()-1];
}

field_id field_entry::getFieldType() const{
    return type;
}


signed char field_entry::getFieldDensity() const{
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

signed char field_entry::setFieldDensity(const signed char new_density){

    if(new_density > 3)
        density = 3;
    else if (new_density < 1){
        density = 1;
        is_alive = false;
    }
    else
        density = new_density;

    return density;

}

int field_entry::setFieldAge(const int new_age){

    age = new_age;

    return age;
}

field::field(){
    draw_symbol = fd_null;
    dirty = false;
};

field::~field(){
    if(dirty) return;
};

/*
Function: findField
Returns a field entry corresponding to the field_id parameter passed in. If no fields are found then returns NULL.
Good for checking for exitence of a field: if(myfield.findField(fd_fire)) would tell you if the field is on fire.
*/
field_entry* field::findField(const field_id field_to_find){
    field_entry* tmp = NULL;
    std::map<field_id, field_entry*>::iterator it = field_list.find(field_to_find);
    if(it != field_list.end()) {
        if(it->second == NULL){
            //In the event someone deleted the field_entry memory somewhere else clean up the list.
            field_list.erase(it);
        } else {
            return it->second;
        }
    }
    return tmp;
};

const field_entry* field::findFieldc(const field_id field_to_find){
    const field_entry* tmp = NULL;
    std::map<field_id, field_entry*>::iterator it = field_list.find(field_to_find);
    if(it != field_list.end()) {
        if(it->second == NULL){
            //In the event someone deleted the field_entry memory somewhere else clean up the list.
            field_list.erase(it);
        } else {
            return it->second;
        }
    }
    return tmp;
};

/*
Function: addfield
Inserts the given field_id into the field list for a given tile if it does not already exist.
Returns false if the field_id already exists, true otherwise.
If the field already exists, it will return false BUT it will add the density/age to the current values for upkeep.
If you wish to modify an already existing field use findField and modify the result.
Density defaults to 1, and age to 0 (permanent) if not specified.
*/
bool field::addField(const field_id field_to_add, const unsigned char new_density, const int new_age){
    std::map<field_id, field_entry*>::iterator it = field_list.find(field_to_add);
    if (fieldlist[field_to_add].priority >= fieldlist[draw_symbol].priority)
        draw_symbol = field_to_add;
    if(it != field_list.end()) {
        //Already exists, but lets update it. This is tentative.
        it->second->setFieldDensity(it->second->getFieldDensity() + new_density);
        return false;
    }
    field_list[field_to_add]=new field_entry(field_to_add, new_density, new_age);
    return true;
};

/*
Function: removeField
Removes the field entry with a type equal to the field_id parameter.
Returns the next iterator or field_list.end().
*/
std::map<field_id, field_entry*>::iterator field::removeField(const field_id field_to_remove){
    std::map<field_id, field_entry*>::iterator it = field_list.find(field_to_remove);
    std::map<field_id, field_entry*>::iterator next = it;
    if(it != field_list.end()) {
        ++next;
        field_entry* tmp = it->second;
        delete tmp;
        field_list.erase(it);
        it = next;
        if (field_list.empty()) {
            draw_symbol = fd_null;
        } else {
            draw_symbol = fd_null;
            for(std::map<field_id, field_entry*>::iterator it2 = field_list.begin(); it2 != field_list.end(); ++it2) {
                if (fieldlist[it2->first].priority >= fieldlist[draw_symbol].priority) {
                    draw_symbol = it2->first;
                }
            }
        }
    };
    return it;
};

/*
Function: fieldCount
Returns the number of fields existing on the current tile.
*/
unsigned int field::fieldCount() const{
    return field_list.size();
};

std::map<field_id, field_entry*>& field::getEntries() {
    return field_list;
}

std::map<field_id, field_entry*>::iterator field::getFieldStart(){

    return field_list.begin();

};

std::map<field_id, field_entry*>::iterator field::getFieldEnd(){

    return field_list.end();

};

/*
Function: fieldSymbol
Returns the last added field from the tile for drawing purposes.
*/
field_id field::fieldSymbol() const
{
    return draw_symbol;
}

std::map<field_id, field_entry*>::iterator field::replaceField(field_id old_field, field_id new_field)
{
    std::map<field_id, field_entry*>::iterator it = field_list.find(old_field);
    std::map<field_id, field_entry*>::iterator next = it;
    ++next;

    if(it != field_list.end()) {
        field_entry* tmp = it->second;
        tmp->setFieldType(new_field);
        field_list.erase(it);
        it = next;
        field_list[new_field] = tmp;
        if (draw_symbol == old_field)
            draw_symbol = new_field;
    }
    return it;
}

int field::move_cost() const{
    if(fieldCount() < 1){
        return 0;
    }
    int current_cost = 0;
    for( std::map<field_id, field_entry*>::const_iterator current_field = field_list.begin();
         current_field != field_list.end();
         ++current_field){
        current_cost += current_field->second->move_cost();
    }
    return current_cost;
}
