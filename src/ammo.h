#pragma once
#ifndef CATA_SRC_AMMO_H
#define CATA_SRC_AMMO_H

#include <string>
#include <utility>

class JsonObject;

using itype_id = std::string;

class ammunition_type
{
        friend class DynamicDataLoader;
    public:
        ammunition_type() = default;
        explicit ammunition_type( std::string name ) : name_( std::move( name ) ) { }

        std::string name() const;

        const itype_id &default_ammotype() const {
            return default_ammotype_;
        }

    private:
        std::string name_;
        itype_id default_ammotype_;

        static void load_ammunition_type( const JsonObject &jsobj );
        static void reset();
        static void check_consistency();
};

#endif // CATA_SRC_AMMO_H
