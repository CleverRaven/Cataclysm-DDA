#ifndef INIT_H
#define INIT_H

#include "json.h"

#include <string>
#include <vector>
#include <list>
#include <memory>
#include <functional>

/**
 * This class is used to load (and unload) the dynamic
 * (and modable) data from json files.
 * There exists only one instance of this class, which
 * can be accessed with @ref get_instance
 *
 * Usage is basically this:
 * - Let user decide which world to play in.
 * - Call @ref unload_data (to unload data from a
 * previously loaded world, if any)
 * - Call @ref load_data_from_path(...) repeatedly with
 * different pathes for the core data and all the mods
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
        friend class Item_factory;
        friend class vpart_info;
        friend class recipe_dictionary;

    public:
        typedef std::string type_string;
        typedef std::map<type_string, std::function<void( JsonObject &, const std::string & )>>
                t_type_function_map;
        typedef std::vector<std::string> str_vec;

        /**
         * JSON data dependent upon as-yet unparsed definitions
         * first: JSON data, second: source identifier
         */
        typedef std::list<std::pair<std::string, std::string>> deferred_json;

    protected:
        /**
         * Maps the type string (comming from json) to the
         * functor that loads that kind of object from json.
         */
        t_type_function_map type_function_map;
        void add( const std::string &type, std::function<void( JsonObject & )> f );
        void add( const std::string &type, std::function<void( JsonObject &, const std::string & )> f );
        /**
         * Load all the types from that json data.
         * @param jsin Might contain single object,
         * or an array of objects. Each object must have a
         * "type", that is part of the @ref type_function_map
         * @throws std::exception on all kind of errors.
         */
        void load_all_from_json( JsonIn &jsin, const std::string &src );
        /**
         * Load a single object from a json object.
         * @param jo The json object to load the C++-object from.
         * @throws std::exception on all kind of errors.
         */
        void load_object( JsonObject &jo, const std::string &src );

        /**
         * Loads and then removes entries from @param data
         * @return whether all entries were sucessfully loaded
         */
        bool load_deferred( deferred_json &data );

        DynamicDataLoader();
        ~DynamicDataLoader();
        /**
         * Initializes @ref type_function_map
         */
        void initialize();
        /**
         * Check the consistency of all the loaded data.
         * May print a debugmsg if something seems wrong.
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
         * @throws std::exception on all kind of errors.
         */
        void load_data_from_path( const std::string &path, const std::string &src );
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
         */
        void finalize_loaded_data();
};

void init_names();

#endif
