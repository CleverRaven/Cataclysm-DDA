#include "processor.h"
#include "game.h" // TODO: This is a circular dependency //it was for gates, probably will be for this too
#include "map.h"
#include "mapdata.h"
#include "generic_factory.h"
#include "messages.h"
#include "json.h"
#include "vehicle.h"

#include <string>

// Processor namespace

namespace
{

struct processor_data;
using processor_id = string_id<processor_data>;

struct processor_data {

    processor_data() :
        is_processing( false ),
        was_loaded( false ) {};

    processor_id id; //furniture
    furn_id next_type;
    auto fuel_type;
    bool is_processing;


    std::vector<process_id> processes;

    std::string full_message;
    std::string empty_message;
    std::string start_message;
    std::string processing_message;
    std::string fail_message;

    bool was_loaded;

    void load( JsonObject &jo, const std::string &src );
    void check() const;
    void reset();

};

processor_id get_processor_id( const tripoint &pos )
{
    return processor_id( g->m.furn( pos ).id().str() );
}

generic_factory<processor_data> processors_data( "processor type", "id" );

}

void processor_data::load( JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "next_type", next_type );
    mandatory( jo, was_loaded, "is_processing", is_processing );
    mandatory( jo, was_loaded, "fuel_type", fuel_type );
    mandatory( jo, was_loaded, "processes", processes, string_id_reader<process_id> {} );

    if( !was_loaded || jo.has_member( "messages" ) ) {
        JsonObject messages_obj = jo.get_object( "messages" );

        optional( messages_obj, was_loaded, "empty", empty_message, translated_string_reader );
        optional( messages_obj, was_loaded, "full", full_message, translated_string_reader );
        //optional( messages_obj, was_loaded, "prompt", prompt_message, translated_string_reader );
        optional( messages_obj, was_loaded, "start", start_message, translated_string_reader );
        optional( messages_obj, was_loaded, "processing", processing_message, translated_string_reader );
        optional( messages_obj, was_loaded, "finish", finish_message, translated_string_reader );
        optional( messages_obj, was_loaded, "fail", fail_message, translated_string_reader );
    }
}

void processor_data::check() const
{
    static const iexamine_function converter( iexamine_function_from_string( "converter" ) );
    const furn_str_id processor_id( id.str() );

    if( !processor_id.is_valid() ) {
        debugmsg( "Furniture \"%s\" have no processor of the same name.", id.c_str() );
    } else if( processor_id->examine != converter ) {
        debugmsg( "Furniture \"%s\" can't act as a processor because the assigned iexamine function is not converter, but it has \"%s\" associated.",
                  processor_tid.c_str(), id.c_str() );
    }
    for( const auto &elem : processes ) {
        if( !elem.is_valid() ) {
            debugmsg( "Invalid process \"%s\" in \"%s\".", elem.c_str(), id.c_str() );
        }
    }
}

void processor_data::load( JsonObject &jo, const std::string &src )
{
    processors_data.load( jo, src );
}

void processor_data::check()
{
    processors_data.check();
}

void processor_data::reset()
{
    processors_data.reset();
}

//process namespace

namespace
{

struct process_data;
using process_id = string_id<process_data>;

struct process_data {
    process_data():
        was_loaded( false ) {};

    process_id id;
    std::string name;
    auto reagent;
    auto output;
    int fuelintake;
    int duration;

    void load( JsonObject &jo, const std::string &src );
    void check() const;

};

process_id get_process_id( std::string id )
{
    return process_id( id );
}

generic_factory<process_data> processes_data( "process type", "id" );

}

void process_data::load( JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "name", name );
    mandatory( jo, was_loaded, "reagent", reagent );
    mandatory( jo, was_loaded, "output", output );
    mandatory( jo, was_loaded, "fuel_intake", fuel_intake );
    mandatory( jo, was_loaded, "duration", duration );
}

void process_data::check() const
{
    const process_id proc_id( id.str() );

    if( !proc_id.is_valid() ) {
        debugmsg( "There is no process with the name %s", id.c_str() );
    if( !reagent.is_valid() ) {
        debugmsg( "Invalid reagent \"%s\" in \"%s\".", reagent.c_str(), id.c_str() );
    }
    if( !output.is_valid() ) {
        debugmsg( "Invalid output \"%s\" in \"%s\".", output.c_str(), id.c_str() );
    }
    if( !fuel_intake.is_valid() ) {
        debugmsg( "Invalid fuel intake \"%s\" in \"%s\".", fuel_intake.c_str(), id.c_str() );
    }
    if( !duration.is_valid() ) {
        debugmsg( "Invalid duration \"%s\" in \"%s\".", duration.c_str(), id.c_str() );
    }
}

void process_data::load( JsonObject &jo, const std::string &src )
{
    processes_data.load( jo, src );
}

void process_data::check()
{
    processes_data.check();
}

void process_data::reset()
{
    processes_data.reset();
}

void interact_with_processor( const tripoint &examp, player &p )
{
    //Get processor id at examp
    processor_id pid = get_processor_id( examp );
    //Do I need the debugmsg-es here or de they get checked on load?
    if( !processors_data.is_valid( pid ) ) {
        debugmsg( "Examined %s has action interact_with_processor, but has no processor data associated", g->m.furn( examp ).id().c_str() );
        return;
    }
    //Get data associated with said processor id
    const processor_data &processor = processors_data.obj( pid );
    std::vector<auto> possible_processes;
    for( auto &p : processor.processes() ) {
        if( p.is_valid() ){
            possible_processes.push_back( process_data.obj( process_id( p ) ) );
        } else {
            //Debugmsg probably not exactly right,
            //I'm trying to return the processor name and the problematic process name too
            debugmsg( "Examined %s has non-valid process data %s2 associated", processor.id().c_str(), get_process_id( p ).c_str() );
            return;
        }
    }
    //You did say one function to handle all
    //processors not in the middle of a process
    //Basically, this furniture type denotes the type
    //where it is not in the middle of making something
    if(!processor.is_processing){
        //  Figure out which process to run, easier to work with than
        //  trying to figure out desired process from items on tile,
        //  except we'll have to do just that on a running processor
        std::vector<std::string> p_names;
        for( auto p : possible_processes ){
            p_names.push_back( p.name );
        }
        int p_index = 0;
        if( p_names.size() > 1 ){
            p_names.push_back(_("Cancel"));
            p_index = menu_vec(true, _("Possible processes"), p_names);
            if (p_index == (int)p_names.size() - 1) {
                p_index = -1;
            }
        } else {
            // Might not need confirmation if there's only one
            if (!query_yn(_("Use the %s" + "process?"), p_names[0].c_str())) {
                p_index = -1;
            }
        }
        if ( p_index < 0 ) {
            return;
        }
        active_process = possible_processes[p_index];
        //Collect possible processed item id-s and fuels into a vector
        //TODO: Handle processes with multiple possible inputs, see kiln
        //TODO: Handle processes requiring multiple input materials
        std::vector<itype_id> processed_materials;
        for( auto &i : active_process.reagent ) {
                //Not sure about this line
                //I want to collect the possible reagent item id's into the vector
                processed_materials.push_back( itype_id( i ) );
        }
        if( processed_materials.empty() ){
            debugmsg( "Examined %s has no reagents associated", active_process.id().c_str() );
            return;
        }
        //Use said vector to find out if we have usable items on the tile
        bool to_deposit = false;
        bool processed_is_present = false;
        // Seen the code in keg, I could do interesting stuff by manipulating
        // which reagent is at which position in the item stack, could make
        // multi-input processes more bearable
        auto items = g->m.i_at( examp );
            //TODO : figure out which loop to use
            //Use said items to figure out if we have usable items
            //Option 1
            for( auto item_it = items.begin(); item_it != items.end(); ) {
                //TODO Here, processor.fuel_type should return the item_id of the fuel
                //TODO Make it possible to use different types of fuels
                if( processed_materials.find( item_it->id ) == container.end() || item_it->id != processor.fuel_type ) {
                    //This is not one of the reagents
                    items.push_back( *item_it );
                    // ? next comment is from vat code, i'm not sure about this
                    // This will add items to a space near the processor, if it's flagged as NOITEM.
                    item_it = items.erase( item_it );
                } else {
                    item_it++;
                    processed_is_present = true;
                }
            }
            //Option 2
            processed_is_present = true;
            for( auto i : items ) {
                if( processed_materials.find( i->id ) == container.end() && i.id  != processor.fuel_type ) {
                    add_msg( m_bad, _("You find some %s, which can't be processed here."), i.tname( 1, false ).c_str() );
                    add_msg( _("You must remove it before you proceed.") );
                    return;
            }
        //TODO: If there's nothing on the tile, check player inventory if they have anything
        if( !processed_is_present ){
            add_msg(m_info, _("There's nothing that can be processed."));
            return;
        }
        //At this point, we should have only usable reagents on the tile,
        // Currently in a very unordered fashion (keg code could help)
        std::vector<itype_id> materials;
        std::vector<integer> charges;
        int fuel = 0;
        items = g->m.i_at( examp );
        for( auto item_it = items.begin(); item_it != items.end(); ) {
            if (item_it.typeId() == processor.fuel_type ){
                fuel = item_it.charges;
                continue;
            }
            for( auto reagent_it = processed_materials.begin(); reagent_it != processed_materials.end(); ){
                //If we found which reagent it belongs to
                if( item_it.typeId() == reagent_it.typeId() ) {
                    materials.push_back( item_it.typeId() );
                    charges.push_back( item_it.charges );
                    break;
                }
                reagent_it++;
            }
            item_it++;
        }
        //  Now we know how many charges of each reagent do we have, along with how much fuel
        //  Current implementation assumes use of one on each reagent and one fuel_intake number of fuel charges
        //  per charge of output produced

        //  Should it be volume based?
        //TODO: calculate skill gain
        //TODO: calculate output here, or in second phase, based on charges and fuel present at start

        //TODO: make it skill use dependent? Would be silly to ask for fire on a CNC machine down the road
        if( !p.has_charges( "fire" , 1 ) ) {
            add_msg( _("This process is ready to be started, but you have no fire source.") );
            return;
        } else if( !query_yn( _( processor.full_message ) ) ) {
            return;
        }
        add_msg( _( processor.start_message ) );
        p.use_charges( "fire", 1 );

        g->m.i_at( examp ).front().bday = calendar::turn;

    } else {
        //processor doing something

    }
}

