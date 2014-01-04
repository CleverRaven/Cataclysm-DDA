#ifndef _INIT_H_
#define _INIT_H_

#include "json.h"

#include <string>
#include <vector>

//********** Functor Base, Static and Class member accessors
class TFunctor
{
public:
    virtual void operator ()(JsonObject &jo) = 0; // virtual () operator
    virtual void Call(JsonObject &jo) = 0; // what will be getting called
    virtual ~TFunctor() {};
};

class StaticFunctionAccessor : public TFunctor
{
private:
    void (*_fptr)(JsonObject &jo);

public:
    virtual void operator()(JsonObject &jo)
    {
        (*_fptr)(jo);
    }
    virtual void Call(JsonObject &jo)
    {
        (*_fptr)(jo);
    }

    StaticFunctionAccessor(void (*fptr)(JsonObject &jo))
    {
        _fptr = fptr;
    }

    ~StaticFunctionAccessor()
    {
        _fptr = NULL;
    }
};
template <class TClass> class ClassFunctionAccessor : public TFunctor
{
private:
    void (TClass::*_fptr)(JsonObject &jo);
    TClass *ptr_to_obj;

public:
    virtual void operator()(JsonObject &jo)
    {
        (*ptr_to_obj.*_fptr)(jo);
    }
    virtual void Call(JsonObject &jo)
    {
        (*ptr_to_obj.*_fptr)(jo);
    }

    ClassFunctionAccessor(TClass *ptr2obj, void (TClass::*fptr)(JsonObject &jo))
    {
        ptr_to_obj = ptr2obj;
        _fptr = fptr;
    }

    ~ClassFunctionAccessor()
    {
        _fptr = NULL;
        ptr_to_obj = NULL;
    }
};
//********** END - Functor Base, Static and Class member accessors

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
 * - Call @ref load_data_from_dir(...) repeatedly with
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
 * Delayed loading works like this:
 * All types in delay_order[delay_order_state] up to
 * delay_order[delay_order.size()-1] are delayed.
 * We start with delay_order_state=0, which means all
 * types in any of the delay_order set are ignored.
 * Than we increase delay_order_state, and reload everything again.
 * This is done as long as delay_order_state<= delay_order.size()
 *
 *
 * Porting stuff to json works like this:
 * - create a function
 *       void load_my_object(JsonObject &jo);
 * - Or a class member function:
 *       TMyClass::load_my_object(JsonObject &jo);
 * - Or create a new class derived from @ref TFunctor
 * - Add a pointer to this function to @ref type_function_map
 * in the function @ref initialize (see there).
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
        typedef std::string type_string;
        typedef std::map<type_string, TFunctor *> t_type_function_map;
        typedef std::set<type_string> delay_type_set;
        typedef std::vector<delay_type_set> delay_order_vector;
        typedef std::vector<std::string> str_vec;

    protected:
        /**
         * Maps the type string (comming from json) to the
         * functor that loads that kind of object from json.
         */
        t_type_function_map type_function_map;
        /**
         * Contains type names (keys into the @ref type_function_map),
         * of types that get delayed while loaded.
         * Types not in here are loaded first.
         * The next pass loads all in types in delay_order[0] and so on.
         * Which pass we're in is defined by @ref delay_order_state,
         * first pass means @ref delay_order_state == 0
         */
        delay_order_vector delay_order;
        /**
         * For delayed loading, @see delay_order
         */
        int delay_order_state;
        /**
         * Load all the types from that json data.
         * @param jsin Might contain single object,
         * or an array of objects. Each object must have a
         * "type", that is part of the @ref type_function_map
         * @throws std::string on all kind of errors. The string
         * contains the error message.
         */
        void load_all_from_json(JsonIn &jsin);
        /**
         * Determines if loading the object of the given type
         * should be delayed in current state (delay_order_state).
         * @param type The type (key into @ref type_function_map)
         * of the object that is currently atttempted to load.
         * @return true if the type should not be loaded right now.
         */
        bool should_loading_delay(const type_string &type) const;
        /**
         * Load a single object from a json object.
         * @param jo The json object to load the C++-object from.
         * @throws std::string on all kind of errors. The string
         * contains the error message.
         */
        void load_object(JsonObject &jo);

        DynamicDataLoader();
        ~DynamicDataLoader();
        /**
         * Initializes @ref type_function_map and @ref delay_order
         */
        void initialize();
        /**
         * Clears and deletes the contents of @ref delay_order and
         * @ref type_function_map
         */
        void reset();
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
         * @throws std::string on all kind of errors. The string
         * contains the error message.
         */
        void load_data_from_dir(const std::string &path);
        /**
         * Deletes and unloads all the data previously loaded with
         * @ref load_data_from_dir
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

#endif // _INIT_H_
