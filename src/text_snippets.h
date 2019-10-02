#pragma once
#ifndef TEXT_SNIPPETS_H
#define TEXT_SNIPPETS_H

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

class JsonObject;
class JsonArray;

class snippet_library
{
    public:
        snippet_library();

        /**
         * Load snippet from the standalone entry, used by the @ref DynamicDataLoader.
         */
        void load_snippet( JsonObject &jsobj );
        /**
         * Returns the id of a random snippet out of the given category.
         */
        int assign( const std::string &category ) const;
        /**
         * Like @ref assign, but with an explicit seed to the RNG. The seed decides which
         * snippet out of the category is returned (same seed yields same snippet).
         */
        int assign( const std::string &category, unsigned seed ) const;
        std::string get( int index ) const;
        std::string expand( const std::string &str ) const;
        bool has_category( const std::string &category ) const;
        int get_snippet_by_id( const std::string &id ) const;
        std::string random_from_category( const std::string &cat ) const;
        std::vector<int> all_ids_from_category( const std::string &cat ) const;
        /**
         * Load a single snippet text from the json object. The object should have
         * a "text" member with the text of the snippet.
         * A "id" member is optional and if present gives the snippet text a id,
         * stored in snippets_by_id.
         */
        void add_snippets_from_json( const std::string &category, JsonArray &jarr );

        void clear_snippets();
    private:
        /**
         * Adds the snippet text, puts it into @ref categories and @ref snippets.
         * Snippet texts must already be translated!
         * @return the hash of the snippet.
         */
        int add_snippet( const std::string &category, const std::string &text );
        void add_snippet_from_json( const std::string &category, JsonObject &jo );
        /**
         * Load all snippet definitions in the json array into given category.
         * Entries in the array can be simple strings, or json objects (for the
         * later see add_snippet_from_json).
         */
        // Snippets holds a map from the strings hash to the string.
        // The strings stored and used for hashing are untranslated in case the language changes.
        // This is so the reference to the string remains stable across
        // changes to the layout and contents of the snippets json file.
        std::map<int, std::string> snippets;
        // Key is an arbitrary id string (from json), value is the hash of the untranslated snippet.
        std::unordered_map<std::string, int> snippets_by_id;
        // Categories groups snippets by well, category.
        std::multimap<std::string, int> categories;
};

extern snippet_library SNIPPET;

#endif
