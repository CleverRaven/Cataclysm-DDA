#ifndef CONSTRUCTION_H
#define CONSTRUCTION_H

#include "requirements.h"
#include "cursesdef.h" // WINDOW
#include "enums.h" // point
#include "string_id.h"

#include <string>
#include <set>
#include <functional>

class JsonObject;
typedef int nc_color;
class Skill;
using skill_id = string_id<Skill>;

struct construction {
        std::string category; //Construction type category
        std::string description; // how the action is displayed to the player
        skill_id skill;
        std::string pre_terrain; // beginning terrain for construction
        std::string post_terrain;// final terrain after construction

        std::set<std::string> pre_flags; // flags beginning terrain must have

        requirement_data requirements;

        int id; // arbitrary internal identifier
        int time;
        int difficulty;

        bool ( *pre_special )( point ); // custom constructability check
        void ( *post_special )( point ); // custom after-effects

        bool pre_is_furniture; // whether it's furniture or terrain
        bool post_is_furniture; // whether it's furniture or terrain

        int adjusted_time() const; // NPC assistance adjusted
        int print_time( WINDOW *w, int ypos, int xpos, int width, nc_color col ) const;
        std::vector<std::string> get_folded_time_string( int width ) const;
        float time_scale() const; //result of construction scaling option
    private:
        std::string get_time_string() const;
};

//! Set all constructions to take the specified time.
void standardize_construction_times( int time );

//! Remove all constructions matching the predicate.
void remove_construction_if( std::function<bool ( construction & )> pred );

void load_construction( JsonObject &jsobj );
void reset_constructions();
void construction_menu();
void complete_construction();
void check_constructions();

#endif
