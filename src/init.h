#pragma once
#ifndef CATA_SRC_INIT_H
#define CATA_SRC_INIT_H

#include <functional>
#include <iosfwd>
#include <list>
#include <map>
#include <memory>
#include <string> // IWYU pragma: keep
#include <utility>
#include <vector>

#include "cata_path.h"
#include "memory_fast.h"

class JsonObject;
class JsonValue;

/**
 * This class is used to load (and unload) the dynamic
 * (and moddable) data from json files.
 * There exists only one instance of this class, which
 * can be accessed with @ref get_instance
 *
 * Usage is basically this:
 * - Let user decide which world to play in.
 * - Call @ref unload_data (to unload data from a
 * previously loaded world, if any)
 * - Call @ref load_data_from_path(...) repeatedly with
 * different paths for the core data and all the mods
 * of the current world.
 * - Call @ref finalize_loaded_data when all mods have been
 * loaded.
 * - Play.
 *
 * The object initializes itself upon first usage.
 * It also unloads everything when the program ends.
 *
 *
 *
 * Porting stuff to json works like this:
 * - create a function
 *       void load_my_object(JsonObject &jo);
 * - Add an entry to @ref type_function_map (inside of @ref initialize)
 *   that calls the new function.
 * - Inside that function load the data from the json object.
 * You must also provide a reset function and add a call to
 * that function in @ref unload_data
 * - Optional: create a finalize function and call it from
 * @ref finalize_loaded_data
 * - Optional: create a function to check the consistency of
 * the loaded data and call this function from @ref check_consistency
 * - Than create json files.
 */
class DynamicDataLoader
{
    public:
        /**
         * JSON data dependent upon as-yet unparsed definitions
         * first: JSON source location, second: source identifier
         */
        using deferred_json = std::list<std::pair<JsonObject, std::string>>;

    private:
        bool finalized = false;

        struct cached_streams;

        std::unique_ptr<cached_streams> stream_cache;

    protected:
        /**
         * Maps the type string (coming from json) to the
         * functor that loads that kind of object from json.
         */
        using type_string = std::string;
        using t_type_function_map =
            std::map<type_string, std::function<void( const JsonObject &, const std::string &, const cata_path &, const cata_path & )>>;
        using str_vec = std::vector<std::string>;
        t_type_function_map type_function_map;
        void add( const std::string &type, const std::function<void( const JsonObject & )> &f );
        void add( const std::string &type,
                  const std::function<void( const JsonObject &, const std::string & )> &f );
        void add( const std::string &type,
                  const std::function<void( const JsonObject &, const std::string &, const std::string &, const std::string & )>
                  &f );
        void add( const std::string &type,
                  const std::function<void( const JsonObject &, const std::string &, const cata_path &, const cata_path & )>
                  &f );
        /**
         * Load all the types from that json data.
         * @param jsin Might contain single object,
         * or an array of objects. Each object must have a
         * "type", that is part of the @ref type_function_map
         * @param src String identifier for mod this data comes from
         * @param ui Finalization status display.
         * @throws std::exception on all kind of errors.
         */
        void load_all_from_json( const JsonValue &jsin, const std::string &src,
                                 const cata_path &base_path, const cata_path &full_path );
        /**
         * Load a single object from a json object.
         * @param jo The json object to load the C++-object from.
         * @param src String identifier for mod this data comes from
         * @throws std::exception on all kind of errors.
         */
        void load_object( const JsonObject &jo, const std::string &src,
                          const cata_path &base_path = cata_path{},
                          const cata_path &full_path = cata_path{} );

        DynamicDataLoader();
        ~DynamicDataLoader();
        /**
         * Initializes @ref type_function_map
         */
        void initialize();
        /**
         * Check the consistency of all the loaded data.
         * May print a debugmsg if something seems wrong.
         * @param ui Finalization status display.
         */
        void check_consistency();

    public:
        /**
         * Returns the single instance of this class.
         */
        static DynamicDataLoader &get_instance();
        /**
         * Load all data from json files located in
         * the path (recursive).
         * @param path Either a folder (recursively load all
         * files with the extension .json), or a file (load only
         * that file, don't check extension).
         * @param src String identifier for mod this data comes from
         * @param ui Finalization status display.
         * @throws std::exception on all kind of errors.
         */
        /*@{*/
        void load_data_from_path( const cata_path &path, const std::string &src );
        /**
         * Load all data from json files located in
         * the path (recursive) except for those within the mod_interactions folder.
         * @param path Either a folder (recursively load all
         * files with the extension .json), or a file (load only
         * that file, don't check extension).
         * @param src String identifier for mod this data comes from.
         * @throws std::exception on all kind of errors.
         */
        /*@{*/
        void load_mod_data_from_path( const cata_path &path, const std::string &src );
        /**
         * Load directories located within the given path if they are named after a currently loaded mod id.
         * @param path a folder.
         * @param src String identifier for mod this data comes from.
         * @throws std::exception on all kind of errors.
         */
        /*@{*/
        void load_mod_interaction_files_from_path( const cata_path &path, const std::string &src );
        /*@}*/
        /**
         * Deletes and unloads all the data previously loaded with
         * @ref load_data_from_path
         */
        void unload_data();
        /**
         * Called to finalize the loaded data. This should be called
         * after all the mods have been loaded.
         * It must be called once after loading all data.
         * It also checks the consistency of the loaded data with
         * @ref check_consistency
         * @param ui Finalization status display.
         * @throw std::exception if the loaded data is not valid. The
         * game should *not* proceed in that case.
         */
        /*@{*/
        void finalize_loaded_data();
        /*@}*/

        /**
         * Loads and then removes entries from @param data
         */
        void load_deferred( deferred_json &data );

        /**
         * Returns whether the data is finalized and ready to be utilized.
         */
        bool is_data_finalized() const {
            return finalized;
        }

        /**
         * Get a possibly cached stream for deferred data loading. If the cached
         * stream is still in use by outside code, this returns a new stream to
         * avoid conflict of stream cursor. The stream cursor is not reset if a
         * cached stream is returned.
         */
        shared_ptr_fast<std::istream> get_cached_stream( const std::string &path );
};

#endif // CATA_SRC_INIT_H
