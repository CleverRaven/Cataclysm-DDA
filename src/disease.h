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
        efftype_id symptoms;

};
#endif

