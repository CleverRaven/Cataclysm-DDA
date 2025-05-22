#pragma once

#ifndef CATA_SRC_RECIPE_STEP_H
#define CATA_SRC_RECIPE_STEP_H

#include <map>


#include "calendar.h"
#include "flexbuffer_json.h"
#include "requirements.h"
#include "type_id.h"

// class character;
struct recipe_proficiency {
    proficiency_id id;
    bool _skill_penalty_assigned = false;
    bool required = false;
    float time_multiplier = 0.0f;
    float skill_penalty = 0.0f;
    float learning_time_mult = 1.0f;
    std::optional<time_duration> max_experience = std::nullopt;

    void load( const JsonObject &jo );
    void deserialize( const JsonObject &jo );
};

struct recipe_step_data {
    public:
        // factory
        // static void load_recipe_step( const JsonObject &jo, const std::string &src );
        void load( const JsonObject &jo );
        void deserialize( const JsonObject &jo );
        bool was_loaded = false;

        recipe_step_id id;
        skill_id skill_used;
        std::string description;
        std::string start_msg;
        std::string end_msg;

        double batch_rscale = 0.0;
        int batch_rsize = 0; // minimum batch size to needed to reach batch_rscale

        std::vector<recipe_proficiency> proficiencies;
        std::map<skill_id, int> required_skills;
        int difficulty = 0;
        int64_t time = 0; // in movement points (100 per turn)
        bool unattended = false;



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