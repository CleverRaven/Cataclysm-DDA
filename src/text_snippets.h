#pragma once
#ifndef CATA_SRC_TEXT_SNIPPETS_H
#define CATA_SRC_TEXT_SNIPPETS_H

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "optional.h"
#include "translations.h"
#include "type_id.h"

class JsonArray;
class JsonObject;

class snippet_library
{
    public:
        /**
         * Load snippet from the standalone entry, used by the @ref DynamicDataLoader.
         */
        void load_snippet( const JsonObject &jsobj );
        /**
         * Load all snippet definitions in the json array into given category.
         * Entries in the array can be simple strings, or json objects (for the
         * later see add_snippet_from_json).
         */
        void add_snippets_from_json( const std::string &category, const JsonArray &jarr );
        /**
         * Load a single snippet text from the json object. The object should have
         * a "text" member with the text of the snippet.
         * A "id" member is optional and if present gives the snippet text a id,
         * stored in snippets_by_id.
         */
        void add_snippet_from_json( const std::string &category, const JsonObject &jo );
        void clear_snippets();

        bool has_category( const std::string &category ) const;
        /**
         * Returns the snippet referenced by the id, or cata::nullopt if there
         * is no snippet with such id.
         * The tags in the snippet are not automatically expanded - you need to
         * call `expand()` to do that.
         */
        cata::optional<translation> get_snippet_by_id( const snippet_id &id ) const;
        /**
         * Returns a reference to the snippet with the id, or a reference to an
         * empty translation object if no such snippet exist.
         */
        const translation &get_snippet_ref_by_id( const snippet_id &id ) const;
        /**
         * Returns true if there exists a snippet with the id
         */
        bool has_snippet_with_id( const snippet_id &id ) const;
        /**
         * Expand the string by recursively replacing tags in angle brackets (<>)
         * with random snippets from the tags' respective category. For example,
         * if the string is "<okay><punc>", the function then replaces "<okay>"
         * with a snippet from the "<okay>" category, and "<punc>" with a snippet
         * from the "<punc>" category, and returns, for example, "okay!". If a tag
         * does not have a corresponding category, it is copied to the final string,
         * for example, "drop <yrwp>, <okay>?" may expand to "drop <yrwp>, okay?"
         * because "<yrwp>" is a special tag that does not have a category defined
         * in the JSON files.
         */
        std::string expand( const std::string &str ) const;
        /**
         * Returns the id of a random snippet out of the given category.
         * Snippets without an id will NOT be returned by this function.
         * If there isn't any snippet in the category, or if none of the snippets
         * in the category has an id, snippet_id::NULL_ID() is returned.
         */
        snippet_id random_id_from_category( const std::string &cat ) const;
        /**
         * Returns a random snippet out of the given category. Both snippets with
         * or without an id may be returned.
         * If there isn't any snippet in the category, cata::nullopt is returned.
         * The tags in the snippet are not automatically expanded - you need to
         * call `expand()` to do that.
         */
        cata::optional<translation> random_from_category( const std::string &cat ) const;
        /**
         * Use the given seed to select a random snippet from the category.
         * For the same seed, the returned snippet stays the same *in a single
         * game session*, but may change after reloading or restarting a game,
         * due to layout changes in the JSON files, for example. If there isn't
         * any snippet in the category, cata::nullopt is returned.
         * The tags in the snippet are not automatically expanded - you need to
         * call `expand()` to do that.
         *
         * TODO: make the result stay the same through different game sessions
         */
        cata::optional<translation> random_from_category( const std::string &cat, unsigned int seed ) const;
        /**
         * Used only for legacy compatibility. `hash_to_id_migration` will be
         * initialized if it hasn't been yet, and the snippet with the given
         * hash is looked up and returned. If there's no longer any snippet with
         * the given hash, snippet_id::NULL_ID() is returned instead.
         */
        snippet_id migrate_hash_to_id( int hash );

    private:
        std::unordered_map<snippet_id, translation> snippets_by_id;

        struct category_snippets {
            std::vector<snippet_id> ids;
            std::vector<translation> no_id;
        };
        std::unordered_map<std::string, category_snippets> snippets_by_category;

        cata::optional<std::map<int, snippet_id>> hash_to_id_migration;
};

extern snippet_library SNIPPET;

#endif // CATA_SRC_TEXT_SNIPPETS_H
