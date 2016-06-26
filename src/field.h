#ifndef FIELD_H
#define FIELD_H

#include "game_constants.h"
#include "color.h"
#include "json.h"
#include "string_id.h"

#include <vector>
#include <string>
#include <map>
#include <iosfwd>

struct field_t;
using field_id = string_id<field_t>;

extern const field_id fd_null;
extern const field_id fd_acid;
extern const field_id fd_acid_vent;
extern const field_id fd_bees;
extern const field_id fd_bile;
extern const field_id fd_blood;
extern const field_id fd_blood_veggy;
extern const field_id fd_blood_insect;
extern const field_id fd_blood_invertebrate;
extern const field_id fd_cigsmoke;
extern const field_id fd_cracksmoke;
extern const field_id fd_dazzling;
extern const field_id fd_electricity;
extern const field_id fd_fatigue;
extern const field_id fd_fire;
extern const field_id fd_fire_vent;
extern const field_id fd_flame_burst;
extern const field_id fd_fungal_haze;
extern const field_id fd_fungicidal_gas;
extern const field_id fd_gas_vent;
extern const field_id fd_gibs_flesh;
extern const field_id fd_gibs_invertebrate;
extern const field_id fd_hot_air1;
extern const field_id fd_hot_air2;
extern const field_id fd_hot_air3;
extern const field_id fd_hot_air4;
extern const field_id fd_incendiary;
extern const field_id fd_laser;
extern const field_id fd_methsmoke;
extern const field_id fd_nuke_gas;
extern const field_id fd_plasma;
extern const field_id fd_push_items;
extern const field_id fd_relax_gas;
extern const field_id fd_rubble;
extern const field_id fd_shock_vent;
extern const field_id fd_slime;
extern const field_id fd_smoke;
extern const field_id fd_spotlight;
extern const field_id fd_tear_gas;
extern const field_id fd_toxic_gas;
extern const field_id fd_sap;
extern const field_id fd_sludge;
extern const field_id fd_web;
extern const field_id fd_weedsmoke;

struct field_t {
    public:
        field_t() : id_( field_id( "null" ) ) {}

        const field_id &id() const {
            return id_;
        }

        bool is_null() const {
            return id_ == field_id( "null" );
        }

     /** Display name for field at given density (eg. light smoke, smoke, heavy smoke) */
     std::string name[ MAX_FIELD_DENSITY ];

 char sym; //The symbol to draw for this field. Note that some are reserved like * and %. You will have to check the draw function for specifics.
 int priority; //Inferior numbers have lower priority. 0 is "ground" (splatter), 2 is "on the ground", 4 is "above the ground" (fire), 6 is reserved for furniture, and 8 is "in the air" (smoke).

     /** Color the field will be drawn as on the screen at a given density */
     nc_color color[ MAX_FIELD_DENSITY ];

     /**
      * If false this field may block line of sight.
      * @warning this does nothing by itself. You must go to the code block in lightmap.cpp and modify
      * transparancy code there with a case statement as well!
     **/
     bool transparent[ MAX_FIELD_DENSITY ];

     /** Where tile is dangerous (prompt before moving into) at given density */
     bool dangerous[ MAX_FIELD_DENSITY ];

 //Controls, albeit randomly, how long a field of a given type will last before going down in density.
 int halflife; // In turns

     /** cost of moving into and out of this field at given density */
    int move_cost[ MAX_FIELD_DENSITY ];

        /** Load field type from JSON definition */
        static void load_field( JsonObject &jo );

        /** Get all currently loaded field types */
        static const std::map<field_id, field_t> &all();

        /** Check consistency of all loaded field types */
        static void check_consistency();

        /** Clear all loaded field types (invalidating any pointers) */
        static void reset();

    private:
        field_id id_;
};

/**
 * Returns if the field has at least one intensity for which dangerous[intensity] is true.
 */
bool field_type_dangerous( field_id id );

/**
 * An active or passive effect existing on a tile.
 * Each effect can vary in intensity (density) and age (usually used as a time to live).
 */
class field_entry {
public:
    field_entry() {
      type = field_id( "null" );
      density = 1;
      age = 0;
      is_alive = false;
    };

    field_entry(field_id t, int d, int a) {
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
    int getFieldDensity() const;

    //Returns the age (usually turns to live) of the current field entry.
    int getFieldAge() const;

    //Allows you to modify the field_id of the current field entry.
    //This probably shouldn't be called outside of field::replaceField, as it
    //breaks the field drawing code and field lookup
    field_id setFieldType(const field_id new_field_id);

    //Allows you to modify the density of the current field entry.
    int setFieldDensity(const int new_density);

    //Allows you to modify the age of the current field entry.
    int setFieldAge(const int new_age);

    //Returns if the current field is dangerous or not.
    bool is_dangerous() const
    {
        return type->dangerous[ density - 1 ];
    }

    //Returns the display name of the current field given its current density.
    //IE: light smoke, smoke, heavy smoke
    std::string name() const
    {
        return type->name[ density - 1 ];
    }

    //Returns true if this is an active field, false if it should be removed.
    bool isAlive(){
        return is_alive;
    }

private:
    field_id type; //The field identifier.
    int density; //The density, or intensity (higher is stronger), of the field entry.
    int age; //The age, or time to live, of the field effect. 0 is permanent.
    bool is_alive; //True if this is an active field, false if it should be destroyed next check.
};

/**
 * A variable sized collection of field entries on a given map square.
 * It contains one (at most) entry of each field type (e. g. one smoke entry and one
 * fire entry, but not two fire entries).
 * Use @ref findField to get the field entry of a specific type, or iterate over
 * all entries via @ref begin and @ref end (allows range based iteration).
 * There is @ref fieldSymbol to specific which field should be drawn on the map.
*/
class field{
public:
    field();
    ~field();

    /**
     * Returns a field entry corresponding to the field_id parameter passed in.
     * If no fields are found then nullptr is returned.
     */
    field_entry* findField(const field_id field_to_find);
    /**
     * Returns a field entry corresponding to the field_id parameter passed in.
     * If no fields are found then nullptr is returned.
     */
    const field_entry* findFieldc(const field_id field_to_find) const;
    /**
     * Returns a field entry corresponding to the field_id parameter passed in.
     * If no fields are found then nullptr is returned.
     */
    const field_entry* findField(const field_id field_to_find) const;

    /**
     * Inserts the given field_id into the field list for a given tile if it does not already exist.
     * If you wish to modify an already existing field use findField and modify the result.
     * Density defaults to 1, and age to 0 (permanent) if not specified.
     * The density is added to an existing field entry, but the age is only used for newly added entries.
     * @return false if the field_id already exists, true otherwise.
     */
    bool addField(const field_id field_to_add,const int new_density = 1, const int new_age = 0);

    /**
     * Removes the field entry with a type equal to the field_id parameter.
     * Make sure to decrement the field counter in the submap if (and only if) the
     * function returns true.
     * @return True if the field was removed, false if it did not exist in the first place.
     */
    bool removeField( field_id field_to_remove );
    /**
     * Make sure to decrement the field counter in the submap.
     * Removes the field entry, the iterator must point into @ref field_list and must be valid.
     */
    void removeField( std::map<field_id, field_entry>::iterator );

    //Returns the number of fields existing on the current tile.
    unsigned int fieldCount() const;

    /**
     * Returns the id of the field that should be drawn.
     */
    field_id fieldSymbol() const;

    //Returns the vector iterator to begin searching through the list.
    std::map<field_id, field_entry>::iterator begin();
    std::map<field_id, field_entry>::const_iterator begin() const;

    //Returns the vector iterator to end searching through the list.
    std::map<field_id, field_entry>::iterator end();
    std::map<field_id, field_entry>::const_iterator end() const;

    /**
     * Returns the total move cost from all fields.
     */
    int move_cost() const;

private:
    std::map<field_id, field_entry> field_list; //A pointer lookup table of all field effects on the current tile.    //Draw_symbol currently is equal to the last field added to the square. You can modify this behavior in the class functions if you wish.
    field_id draw_symbol;
};

#endif
