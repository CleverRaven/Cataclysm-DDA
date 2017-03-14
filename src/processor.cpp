#include "processor.h"
#include "game.h"
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
    itype_id fuel_type;


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

    bool is_subset

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
    for( const process_id &elem : processes ) {
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
    std::vector<itype_id> components;
    itype_id output;
    int fuel_intake;
    int duration;
    //TODO: Skill used

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
    //TODO: modify to allow lists of components
    mandatory( jo, was_loaded, "components", components,  string_id_reader<itype_id> {} );
    mandatory( jo, was_loaded, "output", output, string_id_reader<itype_id> {} );
    mandatory( jo, was_loaded, "fuel_intake", fuel_intake );
    mandatory( jo, was_loaded, "duration", duration );
}

void process_data::check() const
{
    const process_id proc_id( id.str() );

    if( !proc_id.is_valid() ) {
        debugmsg( "There is no process with the name %s", id.c_str() );
    for( const auto &elem : components ) {
        if( !elem.is_valid() ) {
            debugmsg( "Invalid components \"%s\" in \"%s\".", elem.c_str(), id.c_str() );
        }
    }
    if( !output.is_valid() ) {
        debugmsg( "Invalid output \"%s\" in \"%s\".", output.c_str(), id.c_str() );
    }
    if( !fuel_intake >= 0 ) {
        debugmsg( "Invalid fuel intake \"%s\" in \"%s\".", fuel_intake.c_str(), id.c_str() );
    }
    if( !duration > 0 ) {
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

template <typename T>
bool IsSubset(std::vector<T> A, std::vector<T> B)
{
    std::sort(A.begin(), A.end());
    std::sort(B.begin(), B.end());
    return std::includes(A.begin(), A.end(), B.begin(), B.end());
}

void interact_with_processor( const tripoint &examp, player &p )
{
    processor_id pid = get_processor_id( examp );
    //Do I need the debugmsg-es here or do they get checked in process_data::load?
    if( !processors_data.is_valid( pid ) ) {
        //TODO EDIT: Verify in mapdata.cpp verification functions instead of on examine
        debugmsg( "Examined %s has action interact_with_processor, but has no processor data associated", g->m.furn( examp ).id().c_str() );
        return;
    }
    const processor_data &processor = processors_data.obj( pid );
    //Make an object list of processes
    std::vector<process_data> possible_processes;
    for( process_id &p : processor.processes ) {
        possible_processes.push_back( processes_data.obj( p ) );
    }
    //Figure out active process
    process_data *active_process;
    if( possible_processes.size() == 1 ){
        active_process = possible_processes[0] );
    } else {
        map_stack items = g->m.i_at();
        if( !items.empty() ){
            //Build a set of item_id's present
            std::vector<itype_id> present_items;
            for( item i : items ){
                if( std::find( present_items.begin(), present_items.end(), i.typeId() ) == present_items.end() ){
                present_items.insert( i.typeId() );
                }
            }
            //Based on the items present, find the process which has all components
            //available and the largest number of required components
            for( process_data data : possible_processes){
                if( IsSubset( present_items, data.components) ){
                    if ( !active_process ){
                        //if nullpointer, set it as active
                        active_process = data;
                    } else if( active_process.components.size() < data.components.size() ) {
                        active_process = data;
                    }
                }
            }
            if( !active_process ){
                //If we can't decide based on the items, make the first process active
                active_process = possible_processes[0];
            }
        } else {
            //No items present, we can default to asking the player what they want
            //and then looking at their inventory
            std::vector<std::string> p_names;
            for( process_data p : possible_processes ){
                p_names.push_back( p.name );
            }
            p_names.push_back(_("Cancel"));
            int p_index = menu_vec(true, _("Possible processes"), p_names);
            if (p_index == (int)p_names.size() - 1 || p_index < 0) {
                //In case cancel, return
                return;
            } else {
                active_process = possible_processes[p_index];
            }
        }
    }
    //Msg to tell you what you need for a process
    add_msg( _("You start on %s.") , active_process.name.c_str() );
    for( itype_id i : active_process.components ){
        add_msg( _("You need some %s for each.") , item::nname( i, 1 ).c_str() );
    }
    //At this point, we know which process is active, but we don't know whether
    //we have the fuel and components available,

    //Use component vector to find out if we have usable items on the tile
    //TODO: Handle processes with multiple possible inputs, see kiln
    //TODO: Handle processes requiring multiple input materials

    // Seen the code in keg, I could do interesting stuff by manipulating
    // which components is at which position in the item stack, could make
    // multi-input processes more bearable
    map_stack items = g->m.i_at( examp );
    bool processed_is_present = false;
    std::map<itype_id, long> charges;
    item fuel = item::item( processor.fuel_type, int( calendar::turn ), 0 );
    for( item i : items ) {
        //Please check to make sure this one is working as intended
        iterator index = active_process.components.find( i->id );
        if(  index == active_process.components.end() && i.id  != processor.fuel_type ) {
            add_msg( m_bad, _("You find some %s, which can't be used in this process."), i.tname( 1, false ).c_str() );
            add_msg( _("You must remove it before you proceed.") );
            return;
        } else if ( i.id  != processor.fuel_type ){
            charges.insert(active_process.components[index], charges[active_process.components[index]] + item.charges );
        } else {
            fuel.charges += i.charges;
        }
    }
    //At this point we only have components and fuel on tile
    //And we know exact numbers
    //Can someone check if I'm using iterators right in this case?
    long minimum = LONG_LONG_MAX;
    for( std::map::iterator it = charges.begin(); it != charges.end(); it++ ){
        if( charges[it] == 0 ){
            add_msg(m_info, _("There's no %s which is needed for the process"), item::nname( it, 1 ).c_str() );
            return;
        } else if( charges[it] < minimum ){
            minimum = charges[it];
        }
    }
    minimum = min( minimum, fuel.charges );
    add_msg( _("This process is ready to be started, and it will produce %d %s"), item::nname(active_process.output, minimum).c_str(), minimum );
    if( !p.has_charges( "fire" , 1 ) ) {
        add_msg( _("This process is ready to be started, but you have no fire source to start it.") );
        return;
    } else if( !query_yn( _("This process is ready to be started, and it will produce %d %s . %s"),
                        item::nname(active_process.output, minimum).c_str(), minimum, processor.full_message.c_str() ) ) {
        return;
    }
    add_msg( _( processor.start_message.c_str() ) );
    p.use_charges( "fire", 1 );

    //TODO: skill

    for( item item_it = items.begin(); item_it != items.end(); ){
        item_it.bday = calendar::turn;
    }
    g->m.furn_set(examp, processor.next_type );
}

// Below this is still WIP

    //You did say one function to handle all
    //processors not in the middle of a process
    //Basically, this furniture type denotes the type
    //where it is not in the middle of making something
    if(!processor.is_processing){

            //TODO : figure out which loop to use
            //Use said items to figure out if we have usable items
            //Option 1
            for( item item_it = items.begin(); item_it != items.end(); ) {
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

        //TODO: If there's nothing on the tile, check player inventory
        // to see if they have anything to process
        if( !processed_is_present ){
            add_msg(m_info, _("There's nothing that can be processed."));
            return;
        }
        //  At this point, we should have only usable reagents on the tile,
        //  Currently in a very unordered fashion (keg code could help)
        //  This next loop is currently not used, could help sort items into a
        //  predefined order (based on order in process.json maybe?)
        std::vector<item> materials;
        std::vector<integer> charges;
        item &fuel;
        items = g->m.i_at( examp );
        for( item item_it = items.begin(); item_it != items.end(); ) {
            if (item_it.typeId() == processor.fuel_type ){
                item_it.i
                fuel = item_it;
                continue;
            }
            for( itype_id component_it = processed_materials.begin(); component_it != processed_materials.end(); ){
                //If we found which components it belongs to
                if( item_it.typeId() == component_it.typeId() ) {
                    materials.push_back( item_it.typeId() );
                    charges.push_back( item_it.charges );
                    break;
                }
                component_it++;
            }
            item_it++;
        }
        //  Now we know how many charges of each components do we have, along with how much fuel
        //  Current implementation plan is to assume use of one on each components and one fuel_intake number of fuel charges
        //  per charge of output produced

        //  Should it be volume based?
        //TODO: calculate skill gain
        //TODO: calculate output here, or in second phase, based on charges and fuel present at start

        //TODO: make it skill use dependent? Would be silly to ask for fire on a CNC machine down the road
        if( !p.has_charges( "fire" , 1 ) ) {
            add_msg( _("This process is ready to be started, but you have no fire source.") );
            return;
        } else if( !query_yn( _( processor.full_message.c_str() ) ) ) {
            return;
        }
        add_msg( _( processor.start_message.c_str() ) );
        p.use_charges( "fire", 1 );
        //TODO: skill xp gain


    } else {
        //processor processing away
        auto items = g->m.i_at( examp );
        if( items.empty() ) {
            debugmsg( processor.empty_message.c_str() );
            g->m.furn_set( examp, processor.next_type );
            return;
        }
        int last_bday = items[0].bday;
        for( auto i : items ) {
            if( processed_materials.find( item_it->id ) != container.end() && i.bday > last_bday ) {
                last_bday = i.bday;
            }
        }
        int process_time = active_process.duration;
        int time_left = process_time - calendar::turn.get_turn() + items[0].bday;
        if( time_left > 0 ) {
            add_msg( _("It should take %d minutes to finish."), time_left / MINUTES(1) + 1 );
            return;
        }

        //  Currently in a very unordered fashion (keg code could help)
        //  This next loop is currently not used, could help sort items into a
        //  predefined order (based on order in process.json maybe?)
        std::vector<item> materials;
        std::vector<integer> charges;
        item &fuel;
        items = g->m.i_at( examp );
        fuel = items.find( processor.fuel_type );
        for( auto component_it = processed_materials.begin(); component_it != processed_materials.end(); ){
            materials.push_back( component_it.typeId() );
            charges.push_back( 0 );
            for( auto item_it = items.begin(); item_it != items.end(); ) {
                //If we found which components it belongs to
                if( item_it.typeId() == component_it.typeId() ) {
                    charges[charges.size()-1] = item_it.charges;
                    break;
                }
                item_it++;
            }
            component_it++;
        }
        int charge = 0;
        bool have_enough = true;
        for (int i : charges ){
            if ( i == 0){
                have_enough = false;
            }
        }
        //Calculate output
        while ( have_enough ) {
            for (int i : charges ){
                i--
            }
            charge++;
            for (int i : charges ){
            if ( i == 0){
                have_enough = false;
            }
            }
        }
        item result( process.output , calendar::turn.get_turn() );
        result.charge = charge;
        g->m.add_item( examp, result );
        g->m.furn_set( examp, processor.next_type);
    }
}

void interact_with_working_processor( const tripoint &pos, player &p )
{

}



