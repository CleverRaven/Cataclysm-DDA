#pragma once
#ifndef CATA_SRC_PROFICIENCY_H
#define CATA_SRC_PROFICIENCY_H

#include "json.h"
#include "translations.h"
#include "type_id.h"

template<typename T>
class generic_factory;

class proficiency
{
        friend class generic_factory<proficiency>;

        proficiency_id id;
        bool was_loaded = false;

        translation _name;

    public:
        static void load_proficiencies( const JsonObject &jo, const std::string &src );
        static void reset();
        void load( const JsonObject &jo, const std::string &src );

        std::string name() const;
};

#endif // CATA_SRC_PROFICIENCY_H
