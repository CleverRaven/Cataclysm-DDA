#ifndef AMMO_H
#define AMMO_H

#include <string>
#include <unordered_map>
#include <functional>

#include "string_id.h"

class ammunition_type;
using ammotype = string_id<ammunition_type>;

class JsonObject;

class ammunition_type
{
        friend class DynamicDataLoader;

    public:
        ammunition_type() = default;
        explicit ammunition_type( std::string name ) : name_( std::move( name ) ) { }

        std::string const &name() const {
            return name_;
        }

        std::string const &default_ammotype() const {
            return default_ammotype_;
        }

        /** Get all currently loaded ammunition types */
        static const std::unordered_map<ammotype, ammunition_type> &all();

        /**
         * Remove all ammunition types matching the predicate
         * @warning must not be called after finalize()
         */
        static void delete_if( const std::function<bool( const ammunition_type & )> &pred );

    private:
        std::string name_;
        std::string default_ammotype_;

        static void load_ammunition_type( JsonObject &jsobj );
        static void reset();
        static void check_consistency();
};

#endif
