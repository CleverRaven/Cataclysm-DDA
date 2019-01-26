#pragma once
#ifndef FIELD_H
#define FIELD_H

#include <array>
#include <map>
#include <string>

#include "calendar.h"
#include "color.h"
#include "game_constants.h"

enum phase_id : int;

/*
struct field_t
Used to store the master field effects list metadata. Not used to store a field, just queried to find out specifics
of an existing field.
*/
struct field_t {
    // internal ident, used for tileset and for serializing,
    // should be the same as the entry in field_id below (e.g. "fd_fire").
    std::string id;

    /** Display name for field at given density (e.g. light smoke, smoke, heavy smoke) */
    std::string untranslated_name[ MAX_FIELD_DENSITY ];
    /// Can be empty! \p density must be in the range [0, MAX_FIELD_DENSITY - 1].
    std::string name( int density ) const;

    char sym; //The symbol to draw for this field. Note that some are reserved like * and %. You will have to check the draw function for specifics.
    int priority; //Inferior numbers have lower priority. 0 is "ground" (splatter), 2 is "on the ground", 4 is "above the ground" (fire), 6 is reserved for furniture, and 8 is "in the air" (smoke).

    /** Color the field will be drawn as on the screen at a given density */
    deferred_color color[ MAX_FIELD_DENSITY ];

    /**
     * If false this field may block line of sight.
     * @warning this does nothing by itself. You must go to the code block in lightmap.cpp and modify
     * transparancy code there with a case statement as well!
    **/
    bool transparent[ MAX_FIELD_DENSITY ];

    /** Where tile is dangerous (prompt before moving into) at given density */
    bool dangerous[ MAX_FIELD_DENSITY ];

    //Controls, albeit randomly, how long a field of a given type will last before going down in density.
    time_duration halflife;

    /** cost of moving into and out of this field at given density */
    int move_cost[ MAX_FIELD_DENSITY ];

    /** Does it penetrate obstacles like gas, spread like liquid or just lie there like solid? */
    phase_id phase;

    /** Should it decay with out-of-bubble time too? */
    bool accelerated_decay;
};

//The master list of id's for a field, corresponding to the fieldlist array.
enum field_id : int {
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
    fd_spotlight,
    fd_dazzling,
    fd_blood_veggy,
    fd_blood_insect,
    fd_blood_invertebrate,
    fd_gibs_insect,
    fd_gibs_invertebrate,
    fd_cigsmoke,
    fd_weedsmoke,
    fd_cracksmoke,
    fd_methsmoke,
    fd_bees,
    fd_incendiary,
    fd_relax_gas,
    fd_fungal_haze,
    fd_hot_air1,
    fd_hot_air2,
    fd_hot_air3,
    fd_hot_air4,
    fd_fungicidal_gas,
    fd_smoke_vent,
    num_fields
};

/*
Controls the master listing of all possible field effects, indexed by a field_id. Does not store active fields, just metadata.
*/
extern const std::array<field_t, num_fields> fieldlist;
/**
 * Returns the field_id of the field whose ident (field::id) matches the given ident.
 * Returns fd_null (and prints a debug message!) if the field ident is unknown.
 * Never returns num_fields.
 */
extern field_id field_from_ident( const std::string &field_ident );

/**
 * Returns if the field has at least one intensity for which dangerous[intensity] is true.
 */
bool field_type_dangerous( field_id id );

/**
 * An active or passive effect existing on a tile.
 * Each effect can vary in intensity (density) and age (usually used as a time to live).
 */
class field_entry
{
    public:
        field_entry() : type( fd_null ), density( 1 ), age( 0_turns ), is_alive( false ) { }
        field_entry( const field_id t, const int d, const time_duration &a ) : type( t ), density( d ),
            age( a ), is_alive( true ) { }

        nc_color color() const;

        char symbol() const;

        //returns the move cost of this field
        int move_cost() const;

        //Returns the field_id of the current field entry.
        field_id getFieldType() const;

        //Returns the current density (aka intensity) of the current field entry.
        int getFieldDensity() const;

        //Allows you to modify the field_id of the current field entry.
        //This probably shouldn't be called outside of field::replaceField, as it
        //breaks the field drawing code and field lookup
        field_id setFieldType( const field_id new_field_id );

        //Allows you to modify the density of the current field entry.
        int setFieldDensity( const int new_density );

        /// @returns @ref age.
        time_duration getFieldAge() const;
        /// Sets @ref age to the given value.
        /// @returns New value of @ref age.
        time_duration setFieldAge( const time_duration &new_age );
        /// Adds given value to @ref age.
        /// @returns New value of @ref age.
        time_duration mod_age( const time_duration &mod ) {
            return setFieldAge( getFieldAge() + mod );
        }

        //Returns if the current field is dangerous or not.
        bool is_dangerous() const {
            return fieldlist[type].dangerous[density - 1];
        }

        //Returns the display name of the current field given its current density.
        //IE: light smoke, smoke, heavy smoke
        std::string name() const {
            return fieldlist[type].name( density - 1 );
        }

        //Returns true if this is an active field, false if it should be removed.
        bool isAlive() {
            return is_alive;
        }

        bool decays_on_actualize() const {
            return fieldlist[type].accelerated_decay;
        }

    private:
        field_id type; //The field identifier.
        int density; //The density, or intensity (higher is stronger), of the field entry.
        time_duration age; //The age, of the field effect. 0 is permanent.
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
class field
{
    public:
        field();

        /**
         * Returns a field entry corresponding to the field_id parameter passed in.
         * If no fields are found then nullptr is returned.
         */
        field_entry *findField( const field_id field_to_find );
        /**
         * Returns a field entry corresponding to the field_id parameter passed in.
         * If no fields are found then nullptr is returned.
         */
        const field_entry *findFieldc( const field_id field_to_find ) const;
        /**
         * Returns a field entry corresponding to the field_id parameter passed in.
         * If no fields are found then nullptr is returned.
         */
        const field_entry *findField( const field_id field_to_find ) const;

        /**
         * Inserts the given field_id into the field list for a given tile if it does not already exist.
         * If you wish to modify an already existing field use findField and modify the result.
         * Density defaults to 1, and age to 0 (permanent) if not specified.
         * The density is added to an existing field entry, but the age is only used for newly added entries.
         * @return false if the field_id already exists, true otherwise.
         */
        bool addField( field_id field_to_add, int new_density = 1, const time_duration &new_age = 0_turns );

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
        std::map<field_id, field_entry>
        field_list; //A pointer lookup table of all field effects on the current tile.    //Draw_symbol currently is equal to the last field added to the square. You can modify this behavior in the class functions if you wish.
        field_id draw_symbol;
};

#endif
