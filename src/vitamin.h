#ifndef VITAMIN_H
#define VITAMIN_H

#include "json.h"
#include "string_id.h"

class vitamin;
using vitamin_id = string_id<vitamin>;

class vitamin
{
    public:
        vitamin() : id_( vitamin_id( "null" ) ) {}

        const vitamin_id &id() const {
            return id_;
        }

        bool is_null() const {
            return id_ == vitamin_id( "null" );
        }

        const std::string &name() const {
            return name_;
        }

        /** Load vitamin from JSON definition */
        static void load_vitamin( JsonObject &jo );

        /** Get all currently loaded vitamins */
        static const std::map<vitamin_id, vitamin> &all();

        /** Clear all loaded vitamins (invalidating any pointers) */
        static void reset();

    private:
        vitamin_id id_;
        std::string name_;
};

#endif
