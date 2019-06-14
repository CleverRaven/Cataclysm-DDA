#include "field.h"

#include <cstddef>
#include <algorithm>
#include <utility>

#include "calendar.h"
#include "debug.h"
#include "emit.h"
#include "enums.h"
#include "itype.h"
#include "translations.h"
#include "optional.h"

/** ID, {name}, symbol, priority, {color}, {transparency}, {dangerous}, half-life, {move_cost}, phase_id (of matter), accelerated_decay (decay outside of reality bubble) **/
const std::array<field_t, num_fields> all_field_types_enum_list = { {
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
            {def_c_white, def_c_light_gray, def_c_dark_gray}, {true, true, true}, {false, false, false}, 35_minutes,
            {0, 0, 0},
            GAS,
            true
        },

        {
            "fd_cracksmoke",
            {translate_marker( "swirl of crack smoke" ), translate_marker( "crack smoke" ), translate_marker( "thick crack smoke" )}, '%', 8,
            {def_c_white, def_c_light_gray, def_c_dark_gray}, {true, true, true}, {false, false, false}, 25_minutes,
            {0, 0, 0},
            GAS,
            true
        },
        {
            "fd_methsmoke",
            {translate_marker( "swirl of meth smoke" ), translate_marker( "meth smoke" ), translate_marker( "thick meth smoke" )}, '%', 8,
            {def_c_white, def_c_light_gray, def_c_dark_gray}, {true, true, true}, {false, false, false}, 30_minutes,
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
            "fd_cold_air1",
            {"", "", ""}, '&', -1,
            {def_c_white, def_c_blue, def_c_blue}, {true, true, true}, {false, false, false}, 50_minutes,
            {0, 0, 0},
            GAS,
            false
        },

        {
            "fd_cold_air2",
            {"", "", ""}, '&', -1,
            {def_c_white, def_c_blue, def_c_blue}, {true, true, true}, {false, false, false}, 50_minutes,
            {0, 0, 0},
            GAS,
            false
        },

        {
            "fd_cold_air3",
            {"", "", ""}, '&', -1,
            {def_c_white, def_c_blue, def_c_blue}, {true, true, true}, {false, false, false}, 50_minutes,
            {0, 0, 0},
            GAS,
            false
        },

        {
            "fd_cold_air4",
            {"", "", ""}, '&', -1,
            {def_c_white, def_c_blue, def_c_blue}, {true, true, true}, {false, false, false}, 50_minutes,
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
        if( all_field_types_enum_list[i].id == field_ident ) {
            return static_cast<field_id>( i );
        }
    }
    debugmsg( "unknown field ident %s", field_ident.c_str() );
    return fd_null;
}

int field_entry::move_cost() const
{
    return all_field_types_enum_list[type].move_cost[ get_field_intensity() - 1 ];
}

nc_color field_entry::color() const
{
    return all_field_types_enum_list[type].color[density - 1];
}

char field_entry::symbol() const
{
    return all_field_types_enum_list[type].sym;
}

field_id field_entry::get_field_type() const
{
    return type;
}

int field_entry::get_field_intensity() const
{
    return density;
}

time_duration field_entry::get_field_age() const
{
    return age;
}

field_id field_entry::set_field_type( const field_id new_field_id )
{

    // TODO: Better bounds checking.
    if( new_field_id >= 0 && new_field_id < num_fields ) {
        type = new_field_id;
    } else {
        type = fd_null;
    }

    return type;

}

int field_entry::set_field_density( const int new_density )
{
    is_alive = new_density > 0;
    return density = std::max( std::min( new_density, MAX_FIELD_DENSITY ), 1 );
}

time_duration field_entry::set_field_age( const time_duration &new_age )
{
    return age = new_age;
}

field::field()
    : draw_symbol( fd_null )
{
}

/*
Function: find_field
Returns a field entry corresponding to the field_id parameter passed in. If no fields are found then returns NULL.
Good for checking for existence of a field: if(myfield.find_field(fd_fire)) would tell you if the field is on fire.
*/
field_entry *field::find_field( const field_id field_to_find )
{
    const auto it = field_list.find( field_to_find );
    if( it != field_list.end() ) {
        return &it->second;
    }
    return nullptr;
}

const field_entry *field::find_field_c( const field_id field_to_find ) const
{
    const auto it = field_list.find( field_to_find );
    if( it != field_list.end() ) {
        return &it->second;
    }
    return nullptr;
}

const field_entry *field::find_field( const field_id field_to_find ) const
{
    return find_field_c( field_to_find );
}

/*
Function: addfield
Inserts the given field_id into the field list for a given tile if it does not already exist.
Returns false if the field_id already exists, true otherwise.
If the field already exists, it will return false BUT it will add the density/age to the current values for upkeep.
If you wish to modify an already existing field use find_field and modify the result.
Density defaults to 1, and age to 0 (permanent) if not specified.
*/
bool field::add_field( const field_id field_to_add, const int new_density,
                       const time_duration &new_age )
{
    auto it = field_list.find( field_to_add );
    if( all_field_types_enum_list[field_to_add].priority >=
        all_field_types_enum_list[draw_symbol].priority ) {
        draw_symbol = field_to_add;
    }
    if( it != field_list.end() ) {
        //Already exists, but lets update it. This is tentative.
        it->second.set_field_density( it->second.get_field_intensity() + new_density );
        return false;
    }
    field_list[field_to_add] = field_entry( field_to_add, new_density, new_age );
    return true;
}

bool field::remove_field( field_id const field_to_remove )
{
    const auto it = field_list.find( field_to_remove );
    if( it == field_list.end() ) {
        return false;
    }
    remove_field( it );
    return true;
}

void field::remove_field( std::map<field_id, field_entry>::iterator const it )
{
    field_list.erase( it );
    if( field_list.empty() ) {
        draw_symbol = fd_null;
    } else {
        draw_symbol = fd_null;
        for( auto &fld : field_list ) {
            if( all_field_types_enum_list[fld.first].priority >=
                all_field_types_enum_list[draw_symbol].priority ) {
                draw_symbol = fld.first;
            }
        }
    }
}

/*
Function: field_count
Returns the number of fields existing on the current tile.
*/
unsigned int field::field_count() const
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
    return n.empty() ? n : _( n );
}

/*
Function: field_symbol
Returns the last added field from the tile for drawing purposes.
*/
field_id field::field_symbol() const
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
    const field_t &ft = all_field_types_enum_list[id];
    return ft.dangerous[0] || ft.dangerous[1] || ft.dangerous[2];
}
