#ifndef AMMO_H
#define AMMO_H

#include <string>

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
    private:
        // Localized name
        std::string name_;
        std::string default_ammotype_;

        static void load_ammunition_type( JsonObject &jsobj );
        static void reset();
        static void check_consistency();
};

#endif
