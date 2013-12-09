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
 int halflife; // In turns

 //cost of moving into and out of this field
 int move_cost[3];
};

/*
  On altering any entries in this enum please add or remove the appropriate entry to the field_names array in tile_id_data.h
*/
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
 fd_plasma,
 fd_laser,
 fd_ice_mist,
 fd_ice_floor,
 fd_frost,
 fd_snow,
 num_fields
};

/*
Controls the master listing of all possible field effects, indexed by a field_id. Does not store active fields, just metadata.
*/
extern field_t fieldlist[num_fields];

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
    //This probably shouldn't be called outside of field::replaceField, as it
    //breaks the field drawing code and field lookup
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
    std::map<field_id, field_entry*>::iterator removeField(const field_id field_to_remove);

    //Returns the number of fields existing on the current tile.
    unsigned int fieldCount() const;

    //Returns the last added field from the tile for drawing purposes.
    //This can be changed to return whatever you think the most important field to draw is.
    field_id fieldSymbol() const;

    std::map<field_id, field_entry*>::iterator replaceField(field_id old_field, field_id new_field);

    //Returns the vector iterator to begin searching through the list.
    //Note: If you are using "field_at" function, set the return to a temporary field variable! If you somehow
    //query an out of bounds field location it returns a different field every inquery. This means that
    //the start and end iterators won't match up and will crash the system.
    std::map<field_id, field_entry*>::iterator getFieldStart();

    //Returns the vector iterator to end searching through the list.
    std::map<field_id, field_entry*>::iterator getFieldEnd();

    //Returns the total move cost from all fields
    int move_cost() const;

    std::map<field_id, field_entry*>& getEntries();
    std::map<field_id, field_entry*> field_list; //A pointer lookup table of all field effects on the current tile.
private:
    //Draw_symbol currently is equal to the last field added to the square. You can modify this behavior in the class functions if you wish.
    field_id draw_symbol;
    bool dirty; //true if this is a copy of the class, false otherwise.
};
#endif
