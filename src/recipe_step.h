#pragma once



#ifndef CATA_SRC_RECIPE_STEP_H
#define CATA_SRC_RECIPE_STEP_H

#include <map>

#include "flexbuffer_json.h"
#include "requirements.h"
#include "type_id.h"

// class character;


struct recipe_step_data {
    public:
        // factory
        static void load_recipe_step( const JsonObject &jo, const std::string &src );
        void load( const JsonObject &jo, const std::string &src );
        void deserialize( const JsonObject &jo );
        bool was_loaded = false;

        recipe_step_id id;
        skill_id skill_used;


        const requirement_data &simple_requirements() const {
            return requirements_;
        }

        const deduped_requirement_data &deduped_requirements() const {
            return deduped_requirements_;
        }


        // do step
        void start();
        void do_turn();
        void finish();

    private:
        /** Deduped version constructed from the above requirements_ */
        deduped_requirement_data deduped_requirements_;

        /** Combined requirements cached when recipe finalized */
        requirement_data requirements_;

        /** External requirements (via "using" syntax) where second field is multiplier */
        std::vector<std::pair<requirement_id, int>> reqs_external;

        /** Requires specified inline with the recipe (and replaced upon inheritance) */
        std::vector<std::pair<requirement_id, int>> reqs_internal;

};



struct rs_gather_components: recipe_step_data {

};


#endif // CATA_SRC_RECIPE_STEP_H