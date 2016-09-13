#ifndef RECIPE_DICTIONARY_H
#define RECIPE_DICTIONARY_H

#include <string>
#include <map>
#include <functional>

#include "crafting.h"

extern recipe_dictionary recipe_dict;

class recipe_dictionary
{
        friend class Item_factory; // allow removal of blacklisted recipes

    public:
        /**
         * Look up a recipe by qualified identifier
         * @warning this is not always the same as the result
         * @return matching recipe or null recipe if none found
         */
        const recipe &operator[]( const std::string &id ) const;

        /** Returns all recipes in a given category */
        const std::set<const recipe *> &in_category( const std::string &cat ) const;

        /** Returns all recipes which could use component */
        const std::set<const recipe *> &of_component( const itype_id &id ) const;

        /** Returns disassembly recipe (or null recipe if no match) */
        static const recipe &get_uncraft( const itype_id &id );

        size_t size() const {
            return recipes.size();
        }

        std::map<std::string, recipe>::const_iterator begin() const {
            return recipes.begin();
        }

        std::map<std::string, recipe>::const_iterator end() const {
            return recipes.end();
        }

        static void load( JsonObject &jo, const std::string &src, bool uncraft );

        static void finalize();
        static void reset();

    protected:
        /**
         * Remove all recipes matching the predicate
         * @warning must not be called after finalize()
         */
        static void delete_if( const std::function<bool( const recipe & )> &pred );

    private:
        std::map<std::string, recipe> recipes;
        std::map<std::string, recipe> uncraft;
        std::map<std::string, std::set<const recipe *>> category;
        std::map<itype_id, std::set<const recipe *>> component;

        static void finalize_internal( std::map<std::string, recipe> &obj );
};

#endif
