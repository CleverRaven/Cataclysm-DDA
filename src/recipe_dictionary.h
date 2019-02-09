#pragma once
#ifndef RECIPE_DICTIONARY_H
#define RECIPE_DICTIONARY_H

#include <algorithm>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "recipe.h"
#include "string_id.h"

class JsonIn;
class JsonOut;
class JsonObject;
typedef std::string itype_id;
class recipe;
using recipe_id = string_id<recipe>;

class recipe_dictionary
{
        friend class Item_factory; // allow removal of blacklisted recipes
        friend recipe_id;

    public:
        /** Returns all recipes that can be automatically learned */
        const std::set<const recipe *> &all_autolearn() const {
            return autolearn;
        }

        size_t size() const;
        std::map<recipe_id, recipe>::const_iterator begin() const;
        std::map<recipe_id, recipe>::const_iterator end() const;

        /** Returns disassembly recipe (or null recipe if no match) */
        static const recipe &get_uncraft( const itype_id &id );

        static void load_recipe( JsonObject &jo, const std::string &src );
        static void load_uncraft( JsonObject &jo, const std::string &src );

        static void finalize();
        static void reset();

    protected:
        /**
         * Remove all recipes matching the predicate
         * @warning must not be called after finalize()
         */
        static void delete_if( const std::function<bool( const recipe & )> &pred );

        static recipe &load( JsonObject &jo, const std::string &src,
                             std::map<recipe_id, recipe> &out );

    private:
        std::map<recipe_id, recipe> recipes;
        std::map<recipe_id, recipe> uncraft;
        std::set<const recipe *> autolearn;

        static void finalize_internal( std::map<recipe_id, recipe> &obj );
};

extern recipe_dictionary recipe_dict;

class recipe_subset
{
    public:
        recipe_subset() = default;
        recipe_subset( const recipe_subset &src, const std::vector<const recipe *> &recipes );
        /**
         * Include a recipe to the subset.
         * @param r recipe to include
         * @param custom_difficulty If specified, it defines custom difficulty for the recipe
         */
        void include( const recipe *r, int custom_difficulty = -1 );
        void include( const recipe_subset &subset );
        /**
         * Include a recipe to the subset. Based on the condition.
         * @param subset Where to included the recipe
         * @param pred Unary predicate that accepts a @ref recipe.
         */
        template<class Predicate>
        void include_if( const recipe_subset &subset, Predicate pred ) {
            for( const auto &elem : subset ) {
                if( pred( *elem ) ) {
                    include( elem );
                }
            }
        }

        /** Check if the subset contains a recipe with the specified id. */
        bool contains( const recipe *r ) const {
            return std::any_of( recipes.begin(), recipes.end(), [r]( const recipe * elem ) {
                return elem->ident() == r->ident();
            } );
        }

        /**
         * Get custom difficulty for the recipe.
         * @return Either custom difficulty if it was specified, or recipe default difficulty.
         */
        int get_custom_difficulty( const recipe *r ) const;

        /** Check if there is any recipes in given category (optionally restricted to subcategory) */
        bool empty_category(
            const std::string &cat,
            const std::string &subcat = std::string() ) const;

        /** Get all recipes in given category (optionally restricted to subcategory) */
        std::vector<const recipe *> in_category(
            const std::string &cat,
            const std::string &subcat = std::string() ) const;

        /** Returns all recipes which could use component */
        const std::set<const recipe *> &of_component( const itype_id &id ) const;

        enum class search_type {
            name,
            skill,
            primary_skill,
            component,
            tool,
            quality,
            quality_result,
            description_result
        };

        /** Find marked favorite recipes */
        std::vector<const recipe *> favorite() const;

        /** Find recently used recipes */
        std::vector<const recipe *> recent() const;

        /** Find recipes matching query (left anchored partial matches are supported) */
        std::vector<const recipe *> search( const std::string &txt,
                                            const search_type key = search_type::name ) const;
        /** Find recipes matching query and return a new recipe_subset */
        recipe_subset reduce( const std::string &txt, const search_type key = search_type::name ) const;
        /** Set intersection between recipe_subsets */
        recipe_subset intersection( const recipe_subset &subset ) const;
        /** Set difference between recipe_subsets */
        recipe_subset difference( const recipe_subset &subset ) const;
        /** Find recipes producing the item */
        std::vector<const recipe *> search_result( const itype_id &item ) const;

        size_t size() const {
            return recipes.size();
        }

        void clear() {
            component.clear();
            category.clear();
            recipes.clear();
        }

        std::set<const recipe *>::const_iterator begin() const {
            return recipes.begin();
        }

        std::set<const recipe *>::const_iterator end() const {
            return recipes.end();
        }

    private:
        std::set<const recipe *> recipes;
        std::map<const recipe *, int> difficulties;
        std::map<std::string, std::set<const recipe *>> category;
        std::map<itype_id, std::set<const recipe *>> component;
};

void serialize( const recipe_subset &value, JsonOut &jsout );
void deserialize( recipe_subset &value, JsonIn &jsin );

#endif
