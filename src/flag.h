#pragma once
#ifndef CATA_SRC_FLAG_H
#define CATA_SRC_FLAG_H

#include <set>
#include <string>

#include "translations.h"
#include "type_id.h"

class JsonObject;

extern const flag_str_id flag_NULL;

class json_flag
{
        friend class DynamicDataLoader;
        friend class generic_factory<json_flag>;

    public:
        // used by generic_factory
        flag_str_id id = flag_NULL;
        bool was_loaded = false;

        json_flag() = default;

        /** Fetches flag definition (or null flag if not found) */
        static const json_flag &get( const std::string &id );

        /** Get informative text for display in UI */
        std::string info() const {
            return info_.translated();
        }

        /** Get "restriction" phrase, saying what items with this flag must be able to do */
        std::string restriction() const {
            return restriction_.translated();
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
        operator bool() const;

        void check() const;

        /** true, if flags were loaded */
        static bool is_ready();

        static const std::vector<json_flag> &get_all();

    private:
        translation info_;
        translation restriction_;
        std::set<std::string> conflicts_;
        bool inherit_ = true;
        bool craft_inherit_ = false;
        std::string requires_flag_;
        int taste_mod_ = 0;

        /** Load flag definition from JSON */
        void load( const JsonObject &jo, const std::string &src );

        /** Load all flags from JSON */
        static void load_all( const JsonObject &jo, const std::string &src );

        /** finalize */
        static void finalize_all( );

        /** Check consistency of all loaded flags */
        static void check_consistency();

        /** Clear all loaded flags (invalidating any pointers) */
        static void reset();
};

#endif // CATA_SRC_FLAG_H
