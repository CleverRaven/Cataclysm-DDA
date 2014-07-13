#include "requirements.h"
#include "json.h"
#include "translations.h"
#include "item_factory.h"
#include "output.h"
#include "itype.h"
#include <sstream>

quality::quality_map quality::qualities;

void quality::reset()
{
    qualities.clear();
}

void quality::load( JsonObject &jo )
{
    quality qual;
    qual.id = jo.get_string( "id" );
    qual.name = _( jo.get_string( "name" ).c_str() );
    qualities[qual.id] = qual;
}

std::string quality::get_name( const std::string &id )
{
    const auto a = qualities.find( id );
    if( a != qualities.end() ) {
        return a->second.name;
    }
    return id;
}

bool quality::has( const std::string &id )
{
    return qualities.count( id ) > 0;
}

void requirements::load_obj_list( JsonArray &jsarr, alter_comp_vector &objs )
{
    while( jsarr.has_more() ) {
        std::vector<component> choices;
        JsonArray ja = jsarr.next_array();
        while( ja.has_more() ) {
            if( ja.test_array() ) {
                // crafting uses this format: [ ["tool", count], ... ]
                JsonArray comp = ja.next_array();
                const itype_id name = comp.get_string( 0 );
                const int quant = comp.get_int( 1 );
                choices.push_back( component( name, quant ) );
            } else {
                // constructions uses this format: [ "tool", ... ]
                const itype_id name = ja.next_string( );
                choices.push_back( component( name, -1 ) );
            }
        }
        if( !choices.empty() ) {
            objs.push_back( choices );
        }
    }
}

void requirements::load_obj_list( JsonArray &jsarr, quality_vector &objs )
{
    while( jsarr.has_more() ) {
        JsonObject quality_data = jsarr.next_object();
        const quality_id ident = quality_data.get_string( "id" );
        const int level = quality_data.get_int( "level", 1 );
        const int amount = quality_data.get_int( "amount", 1 );
        objs.push_back( quality_requirement( ident, amount, level ) );
    }
}

void requirements::load( JsonObject &jsobj )
{
    JsonArray jsarr;
    jsarr = jsobj.get_array( "components" );
    load_obj_list( jsarr, components );
    jsarr = jsobj.get_array( "qualities" );
    load_obj_list( jsarr, qualities );
    jsarr = jsobj.get_array( "tools" );
    load_obj_list( jsarr, tools );
}

bool requirements::any_marked_available( const comp_vector &comps )
{
    for( const auto & comp : comps ) {
        if( comp.available == 1 ) {
            return true;
        }
    }
    return false;
}

std::string requirements::print_missing_objs( const alter_comp_vector &objs, bool is_tools )
{
    std::ostringstream buffer;
    for( const auto & list : objs ) {
        if( any_marked_available( list ) ) {
            continue;
        }
        if( !buffer.str().empty() ) {
            buffer << _( "\nand " );
        }
        for( auto it = list.begin(); it != list.end(); ++it ) {
            const component &comp = *it;
            const itype *type = item_controller->find_template( comp.type );
            if( it != list.begin() ) {
                buffer << _( " or " );
            }
            if( !is_tools ) {
                //~ <item-count> <item-name>
                buffer << string_format( _( "%d %s" ), abs( comp.count ),
                                         type->nname( abs( comp.count ) ).c_str() );
            } else if( comp.count > 0 ) {
                //~ <tool-name> (<numer-of-charges> charges)
                buffer << string_format( ngettext( "%s (%d charge)", "%s (%d charges)", comp.count ),
                                         type->nname( 1 ).c_str(), comp.count );
            } else {
                buffer << type->nname( abs( comp.count ) );
            }
        }
    }
    return buffer.str();
}

std::string requirements::print_missing_objs( const quality_vector &objs )
{
    std::ostringstream buffer;
    for( auto it = objs.begin(); it != objs.end(); ++it ) {
        if( it->available ) {
            continue;
        }
        if( it != objs.begin() ) {
            buffer << _( "\nand " );
        }
        buffer << string_format( ngettext( "%d tool with %s of %d or more.",
                                           "%d tools with %s of %d or more.", it->count ),
                                 it->count, quality::get_name( it->type ).c_str(), it->level );
    }
    return buffer.str();
}

std::string requirements::list_missing() const
{
    std::ostringstream buffer;

    const std::string missing_tools = print_missing_objs( tools, true );
    if( !missing_tools.empty() ) {
        buffer << _( "\nThese tools are missing:\n" ) << missing_tools;
    }
    const std::string missing_quali = print_missing_objs( qualities );
    if( !missing_quali.empty() ) {
        if( missing_tools.empty() ) {
            buffer << _( "\nThese tools are missing:" );
        }
        buffer << "\n" << missing_quali;
    }
    const std::string missing_comps = print_missing_objs( components, false );
    if( !missing_comps.empty() ) {
        buffer << _( "\nThose components are missing:\n" ) << missing_comps;
    }
    return buffer.str();
}

void requirements::check_objs( const alter_comp_vector &objs, const std::string &display_name )
{
    for( const auto & list : objs ) {
        for( const auto & comp : list ) {
            if( !item_controller->has_template( comp.type ) ) {
                debugmsg( "%s in %s is not a valid item template", comp.type.c_str(), display_name.c_str() );
            }
        }
    }
}

void requirements::check_objs( const quality_vector &objs, const std::string &display_name )
{
    for( const auto & q : objs ) {
        if( !quality::has( q.type ) ) {
            debugmsg( "Unknown quality %s in %s", q.type.c_str(), display_name.c_str() );
        }
    }
}

void requirements::check_consistency( const std::string &display_name ) const
{
    check_objs( tools, display_name );
    check_objs( components, display_name );
    check_objs( qualities, display_name );
}
