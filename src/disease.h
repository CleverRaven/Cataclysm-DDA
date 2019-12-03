#pragma once
#ifndef DISEASE_H
#define DISEASE_H

#include "effect.h"
#include "type_id.h"
#include "json.h"

class disease_type
{
    public:
        static void load_disease_type( JsonObject &jo, const std::string &src );
        void load( JsonObject &jo, const std::string & );
        static const std::vector<disease_type> &get_all();
        static void check_disease_consistency();
        bool was_loaded;

        diseasetype_id id;
        time_duration min_duration;
        time_duration max_duration;
        int min_intensity;
        int max_intensity;
        /**If not empty this sets the health threshold above which you're immune to the disease*/
        cata::optional<int> health_threshold;
        /**effects applied by this disease*/
        std::set<efftype_id> symptoms;

};
#endif

