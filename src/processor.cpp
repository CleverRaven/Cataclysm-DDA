#include "game.h"
#include "map.h"
#include "mapdata.h"
#include "generic_factory.h"
#include "messages.h"
#include "json.h"
#include "vehicle.h"
#include "processor.h"

#include <string>

//process namespace

namespace
{
    struct process_data;
    using process_id = string_id<process_data>;

    struct process_data {

        process_data() :
            was_loaded(false) {};

        process_id id;
        std::string name;
        std::vector<itype_id> components;
        std::string output;
        int fuel_intake;
        int duration;
        //TODO: Skill used

        bool was_loaded;
        void load(JsonObject &jo, const std::string &src);
        void check() const;

    };

    process_id get_process_id( std::string id )
    {
        return process_id( id );
    }

    generic_factory<process_data> processes_data("process type", "id");

}

void process_data::load(JsonObject &jo, const std::string &)
{
    mandatory(jo, was_loaded, "name", name);
    mandatory(jo, was_loaded, "components", components);
    mandatory(jo, was_loaded, "output", output);
    mandatory(jo, was_loaded, "fuel_intake", fuel_intake);
    mandatory(jo, was_loaded, "duration", duration);
}

void process_data::check() const
{
    for (const itype_id &elem : components) {
        if (!item::type_is_defined( elem )) {
            debugmsg("Invalid components \"%s\" in \"%s\".", elem.c_str(), id.c_str());
        }
    }
    if (!item::type_is_defined( output )) {
        debugmsg("Invalid output \"%s\" in \"%s\".", output.c_str(), id.c_str());
    }
    if ( !(fuel_intake >= 0) ) {
        debugmsg("Invalid fuel intake \"%d\" in \"%s\".", fuel_intake, id.c_str());
    }
    if (!(duration > 0)) {
        debugmsg("Invalid duration \"%d\" in \"%s\".", duration, id.c_str());
    }
}

void processes::load(JsonObject &jo, const std::string &src)
{
    processes_data.load(jo, src);
}

void processes::check()
{
    processes_data.check();
}

void processes::reset()
{
    processes_data.reset();
}

// Processor namespace

namespace
{
struct processor_data;
using processor_id = string_id<processor_data>;

struct processor_data {

    processor_data() :
        was_loaded( false ) {};

    processor_id id; //furniture
    furn_str_id next_type;
    itype_id fuel_type;


    std::vector<process_id> processes;

    std::string full_message;
    std::string empty_message;
    std::string start_message;
    std::string finish_message;
    std::string processing_message;
    std::string fail_message;

    bool was_loaded;

    void load( JsonObject &jo, const std::string &src );
    void check() const;

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
    mandatory( jo, was_loaded, "processes", processes, string_id_reader<process_id>() );

    if( !was_loaded || jo.has_member( "messages" ) ) {
        JsonObject messages_obj = jo.get_object( "messages" );

        optional( messages_obj, was_loaded, "empty", empty_message, translated_string_reader );
        optional( messages_obj, was_loaded, "full", full_message, translated_string_reader );
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
                  processor_id.c_str(), id.c_str() );
    }
    for( const process_id &elem : processes ) {
        if( !elem.is_valid() ) {
            debugmsg( "Invalid process \"%s\" in \"%s\".", elem.c_str(), id.c_str() );
        }
    }
}

void processors::load( JsonObject &jo, const std::string &src )
{
    processors_data.load( jo, src );
}

void processors::check()
{
    processors_data.check();
}

void processors::reset()
{
    processors_data.reset();
}

template <typename T>
bool IsSubset(std::set<T> A, std::vector<T> B)
{
    std::sort(B.begin(), B.end());
    return std::includes(A.begin(), A.end(), B.begin(), B.end());
}

const process_data& select_active(const tripoint &examp )
{
    processor_id pid = get_processor_id(examp);
    const processor_data &current_processor = processors_data.obj(pid);
    //Make an object list of processes
    std::vector<process_data> *possible_processes = new std::vector<process_data>();
    for (const process_id &p : current_processor.processes) {
        possible_processes->push_back( processes_data.obj( p ));
    }
    //Figure out active process
    const process_data *active_process = nullptr;
    if (possible_processes->size() == 1) {
        return possible_processes->at(0);
    }
    const map_stack items = g->m.i_at( examp );
    if (items.empty()) {
        //Temporary solution, should be expanded into player being able to depsit directly form inventory after choosing a process
        return possible_processes->at(0);
    }
    //Build a set of item_id's present
    std::set<itype_id> present_items;
    for (const item &i : items) {
            present_items.insert(i.typeId());
    }
    //Based on the items present, find the process which has all components
    //available and the largest number of required components
    for (const process_data &data : (*possible_processes) ) {
        if (IsSubset(present_items, data.components)) {
            if (!active_process) {
                //if nullpointer, set it as active
                active_process = &data;
            }
            else if (active_process->components.size() < data.components.size()) {
                active_process = &data;
            }
        }
    }
    if (!active_process) {
        //If we can't decide based on the items, make the first process active
        active_process = &possible_processes->at(0);
    }
    return *active_process;
}

void processors::interact_with_processor(const tripoint &examp, player &p)
{
    processor_id pid = get_processor_id(examp);

    if (!processors_data.is_valid( pid )) {
        //TODO EDIT: Verify in mapdata.cpp verification functions instead of on examine
        debugmsg("Examined %s has action interact_with_processor, but has no processor data associated", g->m.furn(examp).id().c_str());
        return;
    }

    const processor_data &current_processor = processors_data.obj(pid);
    //Figure out active process
    process_data active_process = select_active( examp );

    //Msg to tell you what you need for a process
    add_msg(_("You start on %s."), active_process.name.c_str());
    for (const itype_id &i : active_process.components) {
        add_msg(_("You need some %s for each."), item::nname(i, 1).c_str());
    }
    map_stack items = g->m.i_at(examp);
    std::map<itype_id, long> charges;
    item fuel = item::item(current_processor.fuel_type, int(calendar::turn), 0);
    for ( item &i : items) {
        //Please check to make sure this one is working as intended
        auto index = std::find( active_process.components.begin(), active_process.components.end(), i.typeId() );
        if (index == active_process.components.end() && i.typeId() != current_processor.fuel_type) {
            add_msg(m_bad, _("You find some %s, which can't be used in this process."), i.tname(1, false).c_str());
            add_msg(_("You must remove it before you proceed."));
            return;
        }
        else if (i.typeId() != current_processor.fuel_type) {
            charges.insert( std::make_pair( i.typeId() , charges.at( i.typeId() ) + i.charges ) );
        }
        else {
            fuel.charges += i.charges;
        }
    }
    //At this point we only have components and fuel on tile
    //And we know exact numbers
    long minimum = LONG_MAX;
    for( std::pair<itype_id, long> m : charges ){
        if( m.second == 0 ){
            add_msg(m_info, _("There's no %s which is needed for the process"), item::nname( m.first, 1 ).c_str() );
            return;
        } else if( m.second < minimum ){
            minimum = m.second;
        }
    }
    //Need to check if the next line performs integer division
    minimum = std::min( minimum, fuel.charges / active_process.fuel_intake );
    if ( minimum <= 0 ) {
        add_msg(_("The current materials present would go to waste without producing any %s, you need more reagents"), item::nname(active_process.output, 1).c_str() );
        return;
    }
    add_msg( _("This process is ready to be started, and it will produce %d %s"), minimum, item::nname(active_process.output, minimum).c_str() );
    if( !p.has_charges( "fire" , 1 ) ) {
        add_msg( _("This process is ready to be started, but you have no fire source to start it.") );
        return;
    } else if( !query_yn( _("This process is ready to be started, and it will produce %d %s . %s"),
                minimum, item::nname(active_process.output, minimum).c_str(), current_processor.full_message.c_str() ) ) {
        return;
    }
    add_msg( _( current_processor.start_message.c_str() ) );
    p.use_charges( "fire", 1 );

    //TODO: skill

    for( item &i : items ){
        i.bday = calendar::turn;
    }
    g->m.furn_set(examp, current_processor.next_type );
}

void processors::interact_with_working_processor( const tripoint &examp, player &p )
{
    processor_id pid = get_processor_id( examp );

    if (!processors_data.is_valid(pid)) {
        //TODO EDIT: Verify in mapdata.cpp verification functions instead of on examine
        debugmsg("Examined %s has action interact_with_processor, but has no processor data associated", g->m.furn(examp).id().c_str());
        return;
    }
    const processor_data &current_processor = processors_data.obj(pid);
    //Figure out active process. In this case, being a processor currently processing, it shouldn't opt for the menu
    process_data active_process = select_active( examp );

    map_stack items = g->m.i_at(examp);
    if (items.empty()) {
        debugmsg(current_processor.empty_message.c_str());
        g->m.furn_set(examp, current_processor.next_type);
        return;
    }
    int last_bday = items[0].bday;
    for (size_t i = 0; i < items.size(); i++) {
        if (std::find(active_process.components.begin(), active_process.components.end(), items[i].typeId()) == active_process.components.end()) {
            add_msg(_("You remove %s from the %s."), item::nname(items[i].typeId() , items[i].charges ).c_str() , g->m.furn( examp ).obj().name.c_str() );
            g->m.add_item_or_charges(p.pos(), items[i] );
            g->m.i_rem(examp, i);
            i--;
        } else if (items[i].bday > last_bday ) {
            last_bday = items[i].bday;
        }
    }
    int time_left = active_process.duration - calendar::turn.get_turn() + items[0].bday;
    if (time_left > 0) {
        const auto time = calendar(time_left).textify_period();
        add_msg(_("It will be done in %s."), time.c_str());
        return;
    }
    //Processing time is done, tally up components and figure out the output
    items = g->m.i_at(examp);
    std::map<itype_id, long> charges;
    item fuel = item::item(current_processor.fuel_type, int(calendar::turn), 0);
    for (item &i : items) {
        auto index = std::find(active_process.components.begin(), active_process.components.end(), i.typeId());
        //This handles unusable items found on tile at the finish time, could be more complicated, as items will be cleared
        if (index == active_process.components.end()) {
            continue;
        }
        if (i.typeId() != current_processor.fuel_type) {
            charges.insert(std::make_pair( i.typeId() , charges.at(i.typeId()) + i.charges) );
        } else {
            fuel.charges += i.charges;
        }
    }
    //And we know exact numbers
    long minimum = LONG_MAX;
    for ( std::pair<const itype_id, long> &m : charges) {
        if (m.second < minimum) {
            minimum = m.second;
        }
    }
    minimum = std::min( minimum, fuel.charges / active_process.fuel_intake );
    item result = item::item( active_process.output , int(calendar::turn), minimum );
    g->m.i_clear(examp);
    g->m.add_item(examp, result);
    p.moves -= 500;
    g->m.furn_set(examp, current_processor.next_type );

}
