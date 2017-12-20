#pragma once
#ifndef FLAG_H
#define FLAG_H

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

        /** Is this a valid (non-null) flag */
        operator bool() const {
            return !id_.empty();
        }

    private:
        const std::string id_;
        std::string info_;
        std::set<std::string> conflicts_;
        bool inherit_ = true;

        json_flag( const std::string &id = std::string() ) : id_( id ) {}

        /** Load flag definition from JSON */
        static void load( JsonObject &jo );

        /** Check consistency of all loaded flags */
        static void check_consistency();

        /** Clear all loaded flags (invalidating any pointers) */
        static void reset();
};

#endif
