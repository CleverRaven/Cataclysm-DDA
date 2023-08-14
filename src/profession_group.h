#pragma once
#ifndef CATA_SRCPROFESSION_GROUP_H
#define CATA_SRCPROFESSION_GROUP_H

#include "type_id.h"
#include "json.h"

class profession_group
{
    public:
        static void load_profession_group( const JsonObject &jo, const std::string &src );
        void load( const JsonObject &jo, const std::string & );
        static const std::vector<profession_group> &get_all();
        static void check_profession_group_consistency();
        bool was_loaded;

        profession_group_id id;
        std::vector<profession_id> professions;

};
#endif
