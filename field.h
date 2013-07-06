#ifndef _FIELD_H_
#define _FIELD_H_

#include <vector>
#include <string>
#include "color.h"
#include "item.h"
#include "trap.h"
#include "monster.h"
#include "enums.h"
#include "computer.h"
#include "vehicle.h"
#include "graffiti.h"
#include "basecamp.h"
#include "iexamine.h"
#include <iosfwd>

/*
struct field_t
Used to store the master field effects list metadata. Not used to store a field, just queried to find out specifics
of an existing field.
*/
struct field_t {
 std::string name[3]; //The display name of the given density (ie: light smoke, smoke, heavy smoke)
 char sym; //The symbol to draw for this field. Note that some are reserved like * and %. You will have to check the draw function for specifics.
 nc_color color[3]; //The color the field will be drawn as on the screen, by density.

 /*
 If true, does not invoke a check to block line of sight. If false, may block line of sight.
 Note that this does nothing by itself. You must go to the code block in lightmap.cpp and modify
 transparancy code there with a case statement as well!
 */
 bool transparent[3];

 //Dangerous tiles ask you before you step in them.
 bool dangerous[3];

 //Controls, albeit randomly, how long a field of a given type will last before going down in density.
 int halflife;	// In turns

 //cost of moving into and out of this field
 int move_cost[3];
};

//The master list of id's for a field, corresponding to the fieldlist array.
enum field_id {
 fd_null = 0,
 fd_blood,
 fd_bile,
 fd_gibs_flesh,
 fd_gibs_veggy, 
 fd_web,
 fd_slime,
 fd_acid,
 fd_sap,
 fd_sludge,
 fd_fire,
 fd_rubble,
 fd_smoke,
 fd_toxic_gas,
 fd_tear_gas,
 fd_nuke_gas,
 fd_gas_vent,
 fd_fire_vent,
 fd_flame_burst,
 fd_electricity,
 fd_fatigue,
 fd_push_items,
 fd_shock_vent,
 fd_acid_vent,
 num_fields
};

/*
Controls the master listing of all possible field effects, indexed by a field_id. Does not store active fields, just metadata.
*/
const field_t fieldlist[] = {
    {
        {"", "", ""}, '%',
        {c_white, c_white, c_white}, {true, true, true}, {false, false, false}, 0,
        {0,0,0}
    },

    {
        {"blood splatter", "blood stain", "puddle of blood"}, '%',
        {c_red, c_red, c_red}, {true, true, true}, {false, false, false}, 2500,
        {0,0,0}
    },
    {
        {"bile splatter", "bile stain", "puddle of bile"},	'%',
        {c_pink, c_pink, c_pink},	{true, true, true}, {false, false, false},2500,
        {0,0,0}
    },

    {
        {"scraps of flesh",	"bloody meat chunks",	"heap of gore"},			'~',
        {c_brown, c_ltred, c_red},	{true, true, true}, {false, false, false},	   2500,
        {0,0,0}
    },
     
    {
        {"shredded leaves and twigs",	"shattered branches and leaves",	"broken vegetation tangle"},			'~',
        {c_ltgreen, c_ltgreen, c_green},	{true, true, true}, {false, false, false},	   2500,
        {0,0,0}
    },
     
    {
        {"cobwebs","webs", "thick webs"},			'}',
        {c_white, c_white, c_white},	{true, true, false},{false, false, false},   0,
        {0,0,0}
    },

    {
        {"slime trail", "slime stain", "puddle of slime"},	'%',
        {c_ltgreen, c_ltgreen, c_green},{true, true, true},{false, false, false},2500,
        {0,0,0}
    },

    {
        {"acid splatter", "acid streak", "pool of acid"},	'5',
        {c_ltgreen, c_green, c_green},	{true, true, true}, {true, true, true},	    10,
        {0,0,0}
    },

    {
        {"sap splatter", "glob of sap", "pool of sap"},	'5',
        {c_yellow, c_brown, c_brown},	{true, true, true}, {true, true, true},     20,
        {0,0,0}
    },

    {
        {"thin sludge trail", "sludge trail", "thick sludge trail"},	'5',
        {c_ltgray, c_dkgray, c_black},	{true, true, true}, {false, false, false}, 900,
        {0,0,0}
    },

    {
        {"small fire",	"fire",	"raging fire"},			'4',
        {c_yellow, c_ltred, c_red},	{true, true, true}, {true, true, true},	   800,
        {0,0,0}
    },

    {
        {"rubble heap",	"rubble pile", "mountain of rubble"},		'#',
        {c_dkgray, c_dkgray, c_dkgray},	{true, true, false},{false, false, false},  0,
        {0,0,0}
    },

    {
        {"thin smoke",	"smoke", "thick smoke"},		'8',
        {c_white, c_ltgray, c_dkgray},	{true, false, false},{false, true, true},  300,
        {0,0,0}
    },

    {
        {"hazy cloud","toxic gas","thick toxic gas"},		'8',
        {c_white, c_ltgreen, c_green}, {true, false, false},{false, true, true},  900,
        {0,0,0}
    },

    {
        {"hazy cloud","tear gas","thick tear gas"},		'8',
        {c_white, c_yellow, c_brown},	{true, false, false},{true, true, true},   600,
        {0,0,0}
    },

    {
        {"hazy cloud","radioactive gas", "thick radioactive gas"}, '8',
        {c_white, c_ltgreen, c_green},	{true, true, false}, {true, true, true},  1000,
        {0,0,0}
    },

    {
        {"gas vent", "gas vent", "gas vent"}, '%',
        {c_white, c_white, c_white}, {true, true, true}, {false, false, false}, 0,
        {0,0,0}
    },

    { // Fire Vents
        {"", "", ""}, '&',
        {c_white, c_white, c_white}, {true, true, true}, {false, false, false}, 0,
        {0,0,0}
    },

    {
        {"fire", "fire", "fire"}, '5',
        {c_red, c_red, c_red}, {true, true, true}, {true, true, true}, 0,
        {0,0,0}
    },

    {
        {"sparks", "electric crackle", "electric cloud"},	'9',
        {c_white, c_cyan, c_blue},	{true, true, true}, {true, true, true},	     2,
        {0,0,0}
    },

    {
        {"odd ripple", "swirling air", "tear in reality"},	'*',
        {c_ltgray, c_dkgray, c_magenta},{true, true, false},{false, false, false},  0,
        {0,0,0}
    },

    { //Push Items
        {"", "", ""}, '&',
        {c_white, c_white, c_white}, {true, true, true}, {false, false, false}, 0,
        {0,0,0}
    },

    { // shock vents
        {"", "", ""}, '&',
        {c_white, c_white, c_white}, {true, true, true}, {false, false, false}, 0,
        {0,0,0}
    },

    { // acid vents
        {"", "", ""}, '&',
        {c_white, c_white, c_white}, {true, true, true}, {false, false, false}, 0,
        {0,0,0}
    }
};

/*
Class: field_entry
An active or passive effect existing on a tile. Multiple different types can exist on one tile
but there can be only one of each type (IE: one fire, one smoke cloud, etc). Each effect
can vary in intensity (density) and age (usually used as a time to live).
*/
class field_entry {
public:
    field_entry() { 
      type = fd_null; 
      density = 1; 
      age = 0; 
      is_alive = false;
    };

    field_entry(field_id t, unsigned char d, unsigned int a) {
        type = t;
        density = d;
        age = a;
        is_alive = true;
    }

    //returns the move cost of this field
    int move_cost() const;

    //Returns the field_id of the current field entry.
    field_id getFieldType() const;

    //Returns the current density (aka intensity) of the current field entry.
    signed char getFieldDensity() const;

    //Returns the age (usually turns to live) of the current field entry.
    int getFieldAge() const;

    //Allows you to modify the field_id of the current field entry.
    field_id setFieldType(const field_id new_field_id);

    //Allows you to modify the density of the current field entry.
    signed char setFieldDensity(const signed char new_density);

    //Allows you to modify the age of the current field entry.
    int setFieldAge(const int new_age);

    //Get the move cost for this field
    int getFieldMoveCost();

    //Returns if the current field is dangerous or not.
    bool is_dangerous() const
    {
        return fieldlist[type].dangerous[density - 1];
    }

    //Returns the display name of the current field given its current density.
    //IE: light smoke, smoke, heavy smoke
    std::string name() const
    {
        return fieldlist[type].name[density - 1];
    }

    //Returns true if this is an active field, false if it should be removed.
    bool isAlive(){
        return is_alive;
    }

private:
    field_id type; //The field identifier.
    signed char density; //The density, or intensity (higher is stronger), of the field entry.
    int age; //The age, or time to live, of the field effect. 0 is permanent.
    bool is_alive; //True if this is an active field, false if it should be destroyed next check.
};

//Represents a variable sized collection of field entries on a given map square.
class field{
public:
    //Field constructor
    field();
    //Frees all memory assigned to the field's field_entry vector and general cleanup.
    ~field();

    //Returns a field entry corresponding to the field_id parameter passed in. 
    //If no fields are found then a field_entry with type fd_null is returned.
    field_entry* findField(const field_id field_to_find);
    const field_entry* findFieldc(const field_id field_to_find); //for when you want a const field_entry.

    //Inserts the given field_id into the field list for a given tile if it does not already exist.
    //Returns false if the field_id already exists, true otherwise.
    //If you wish to modify an already existing field use findField and modify the result.
    //Density defaults to 1, and age to 0 (permanent) if not specified.
    bool addField(const field_id field_to_add,const unsigned char new_density=1, const int new_age=0);

    //Removes the field entry with a type equal to the field_id parameter. Returns true if removed, false otherwise.
    bool removeField(const field_id field_to_remove);

    //Returns the number of fields existing on the current tile.
    unsigned int fieldCount() const;

    //Returns the last added field from the tile for drawing purposes.
    //This can be changed to return whatever you think the most important field to draw is.
    field_id fieldSymbol() const;

    //Returns the vector iterator to begin searching through the list.
    //Note: If you are using "field_at" function, set the return to a temporary field variable! If you somehow
    //query an out of bounds field location it returns a different field every inquery. This means that
    //the start and end iterators won't match up and will crash the system.
    std::vector<field_entry*>::iterator getFieldStart();

    //Returns the vector iterator to end searching through the list.
    std::vector<field_entry*>::iterator getFieldEnd();

    //Returns the total move cost from all fields
    int move_cost() const;

private:
    std::vector<field_entry*> field_list; //A listing of all field effects on the current tile.
    //Draw_symbol currently is equal to the last field added to the square. You can modify this behavior in the class functions if you wish.
    field_id draw_symbol;
    bool dirty; //true if this is a copy of the class, false otherwise.
};
#endif
