#pragma once
#ifndef CATA_SRC_FLAG_H
#define CATA_SRC_FLAG_H

#include <set>
#include <string>

class JsonObject;

class json_flag
{
        friend class DynamicDataLoader;

    public:
        /** Fetches flag definition (or null flag if not found) */
        static const json_flag &get( const std::string &id );

        /** Get identifier of flag as specified in JSON */
        const std::string &id() const {
            return id_;
        }

        /** Get informative text for display in UI */
        const std::string &info() const {
            return info_;
        }

        /** Is flag inherited by base items from any attached items? */
        bool inherit() const {
            return inherit_;
        }

        /** Is flag inherited by crafted items from any component items? */
        bool craft_inherit() const {
            return craft_inherit_;
        }

        /** Requires this flag to be installed on vehicle */
        std::string requires_flag() const {
            return requires_flag_;
        }

        /** The flag's modifier on the fun value of comestibles */
        int taste_mod() const {
            return taste_mod_;
        }

        /** Is this a valid (non-null) flag */
        operator bool() const {
            return !id_.empty();
        }

    private:
        const std::string id_;
        std::string info_;
        std::set<std::string> conflicts_;
        bool inherit_ = true;
        bool craft_inherit_ = false;
        std::string requires_flag_;
        int taste_mod_ = 0;

        json_flag( const std::string &id = std::string() ) : id_( id ) {}

        /** Load flag definition from JSON */
        static void load( const JsonObject &jo );

        /** Check consistency of all loaded flags */
        static void check_consistency();

        /** Clear all loaded flags (invalidating any pointers) */
        static void reset();
};

#endif // CATA_SRC_FLAG_H
