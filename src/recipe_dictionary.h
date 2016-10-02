#ifndef RECIPE_DICTIONARY_H
#define RECIPE_DICTIONARY_H

#include <string>
#include <map>
#include <functional>
#include <set>
#include <vector>

class JsonObject;
struct recipe;
typedef std::string itype_id;

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

        /** Returns all recipes that can be automatically learned */
        const std::set<const recipe *> &all_autolearn() const {
            return autolearn;
        }

        size_t size() const;
        std::map<std::string, recipe>::const_iterator begin() const;
        std::map<std::string, recipe>::const_iterator end() const;

        /** Returns disassembly recipe (or null recipe if no match) */
        static const recipe &get_uncraft( const itype_id &id );

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
        std::set<const recipe *> autolearn;

        static void finalize_internal( std::map<std::string, recipe> &obj );
};

extern recipe_dictionary recipe_dict;

class recipe_subset
{
    public:
        void add( const recipe *r );

        bool has( const std::string &id ) const {
            return recipes.find( id ) != recipes.end();
        }

        /** Get all recipes in given category (optionally restricted to subcategory) */
        std::vector<const recipe *> in_category(
            const std::string &cat,
            const std::string &subcat = std::string() ) const;

        /** Returns all recipes which could use component */
        const std::set<const recipe *> &of_component( const itype_id &id ) const;

        /** Find recipe by result name (left anchored partial matches are supported) */
        std::vector<const recipe *> search( const std::string &txt ) const;

        size_t size() const {
            return recipes.size();
        }

        void clear() {
            component.clear();
            category.clear();
            recipes.clear();
        }

        std::map<std::string, const recipe *>::const_iterator begin() const {
            return recipes.begin();
        }

        std::map<std::string, const recipe *>::const_iterator end() const {
            return recipes.end();
        }

    private:
        std::map<std::string, const recipe *> recipes;
        std::map<std::string, std::set<const recipe *>> category;
        std::map<itype_id, std::set<const recipe *>> component;
};

#endif
