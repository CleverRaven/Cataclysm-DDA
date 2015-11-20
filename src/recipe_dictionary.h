#ifndef RECIPE_DICTIONARY_H
#define RECIPE_DICTIONARY_H

#include <string>
#include <vector>
#include <map>
#include <list>
#include <functional>

struct recipe;
using itype_id = std::string; // From itype.h

/**
*   Repository class for recipes.
*
*   This class is aimed at making (fast) recipe lookups easier from the outside.
*/
class recipe_dictionary
{
    public:
        void add( recipe *rec );
        void remove( recipe *rec );
        void clear();

        /** Returns a list of recipes in the 'cat' category */
        const std::vector<recipe *> &in_category( const std::string &cat );
        /** Returns a list of recipes in which the component with itype_id 'id' can be used */
        const std::vector<recipe *> &of_component( const itype_id &id );

        /** Allows for lookup like: 'recipe_dict[name]'. */
        recipe *operator[]( const std::string &rec_name ) {
            return by_name[rec_name];
        }

        /** Allows for lookup like: 'recipe_dict[id]'. */
        recipe *operator[]( int rec_id ) {
            return by_index[rec_id];
        }

        size_t size() const {
            return recipes.size();
        }

        /** Allows for iteration over all recipes like: 'for( recipe &r : recipe_dict )'. */
        std::list<recipe *>::const_iterator begin() const {
            return recipes.begin();
        }
        std::list<recipe *>::const_iterator end() const {
            return recipes.end();
        }

        /**
         * Goes over all recipes and calls the predicate, if it returns true, the recipe
         * is removed *and* deleted.
         */
        void delete_if( const std::function<bool(recipe &)> &pred );

    private:
        std::list<recipe *> recipes;

        std::map<const std::string, std::vector<recipe *>> by_category;
        std::map<const itype_id, std::vector<recipe *>> by_component;

        std::map<const std::string, recipe *> by_name;
        std::map<int, recipe *> by_index;

        /** Maps a component to a list of recipes. So we can look up what we can make with an item */
        void add_to_component_lookup( recipe *r );
        void remove_from_component_lookup( recipe *r );
};

extern recipe_dictionary recipe_dict;

#endif
