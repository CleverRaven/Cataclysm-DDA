#pragma once
#ifndef CATA_SRC_DISEASE_H
#define CATA_SRC_DISEASE_H

#include <set>
#include <string>
#include <vector>

#include "bodypart.h"
#include "calendar.h"
#include "json.h"
#include "optional.h"
#include "type_id.h"

class disease_type
{
    public:
        static void load_disease_type( const JsonObject &jo, const std::string &src );
        void load( const JsonObject &jo, const std::string & );
        static const std::vector<disease_type> &get_all();
        static void check_disease_consistency();
        bool was_loaded;

        diseasetype_id id;
        time_duration min_duration = 1_turns;
        time_duration max_duration = 1_turns;
        int min_intensity = 1;
        int max_intensity = 1;
        /**Affected body parts*/
        std::set<body_part> affected_bodyparts;
        /**If not empty this sets the health threshold above which you're immune to the disease*/
        cata::optional<int> health_threshold;
        /**effect applied by this disease*/
        efftype_id symptoms;

};
#endif // CATA_SRC_DISEASE_H

