#include "requirements.h"
#include "json.h"
#include "translations.h"
#include "item_factory.h"
#include "game.h"
#include "inventory.h"
#include "output.h"
#include "itype.h"
#include <sstream>
#include "calendar.h"

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
    time = jsobj.get_int( "time" );
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

int requirements::print_components( WINDOW *w, int ypos, int xpos, int width, nc_color col,
                                    const inventory &crafting_inv ) const
{
    if( components.empty() ) {
        return 0;
    }
    const int oldy = ypos;
    mvwprintz( w, ypos, xpos, col, _( "Components required:" ) );
    ypos++;
    for( const auto & comp_list : components ) {
        const bool has_one = any_marked_available( comp_list );
        mvwprintz( w, ypos, xpos, col, "> " );
        std::ostringstream buffer;
        for( auto a = comp_list.begin(); a != comp_list.end(); ++a ) {
            const component &comp = *a;
            if( a != comp_list.begin() ) {
                buffer << "<color_white> " << _( "OR" ) << "</color> ";
            }
            std::string compcol = has_one ? "dkgray" : "red";
            const itype *type = item_controller->find_template( comp.type );
            const int count = comp.count;
            if( comp.available == 0 ) {
                compcol = "brown";
            } else if( type->count_by_charges() && count > 0 ) {
                if( crafting_inv.has_charges( comp.type, count ) ) {
                    compcol = "green";
                }
            } else if( crafting_inv.has_components( comp.type, abs( count ) ) ) {
                compcol = "green";
            }
            if( ( comp.type == "rope_30" || comp.type == "rope_6" ) &&
                ( g->u.has_trait( "WEB_ROPE" ) && g->u.hunger <= 300 ) ) {
                compcol = "ltgreen"; // Show that WEB_ROPE is on the job!
            }
            buffer << "<color_" << compcol << ">";
            buffer << abs( count ) << " " << type->nname( abs( count ) );
            buffer << "</color>";
        }
        ypos += fold_and_print( w, ypos, xpos + 2, width - 2, col, buffer.str() );
    }
    return ypos - oldy;
}

int requirements::print_tools( WINDOW *w, int ypos, int xpos, int width, nc_color col,
                               const inventory &crafting_inv ) const
{
    const int oldy = ypos;
    mvwprintz( w, ypos, xpos, col, _( "Tools required:" ) );
    ypos++;
    if( tools.empty() && qualities.empty() ) {
        mvwprintz( w, ypos, xpos, col, "> " );
        mvwprintz( w, ypos, xpos + 2, c_green, _( "NONE" ) );
        ypos++;
        return ypos - oldy;
    }
    for( auto a = qualities.begin(); a != qualities.end(); ++a ) {
        mvwprintz( w, ypos, xpos, col, "> " );
        std::ostringstream buffer;
        nc_color toolcol = c_red;
        if( a->available ) {
            toolcol = c_green;
        }
        buffer << string_format( ngettext( "Requires %d tool with %s quality of %d or more.",
                                           "Requires %d tools with %s quality of %d or more.",
                                           a->count ),
                                 a->count, quality::get_name( a->type ).c_str(), a->level );
        ypos += fold_and_print( w, ypos, xpos + 2, width - 2, toolcol, buffer.str() );
    }
    for( const auto & tool_list : tools ) {
        const bool has_one = any_marked_available( tool_list );
        mvwprintz( w, ypos, xpos, col, "> " );
        std::ostringstream buffer;
        for( auto a = tool_list.begin(); a != tool_list.end(); ++a ) {
            const component &comp = *a;
            if( a != tool_list.begin() ) {
                buffer << "<color_white> " << _( "OR" ) << "</color> ";
            }
            const long charges = comp.count;
            const itype *type = item_controller->find_template( comp.type );
            std::string toolcol = has_one ? "dkgray" : "red";

            if( comp.available == 0 ) {
                toolcol = "brown";
            } else if( charges < 0 && crafting_inv.has_tools( comp.type, 1 ) ) {
                toolcol = "green";
            } else if( charges > 0 && crafting_inv.has_charges( comp.type, charges ) ) {
                toolcol = "green";
            } else if( ( comp.type == "goggles_welding" ) && ( g->u.has_bionic( "bio_sunglasses" ) ||
                       g->u.is_wearing( "rm13_armor_on" ) ) ) {
                toolcol = "cyan";
            }

            buffer << "<color_" << toolcol << ">";
            buffer << type->nname( 1 );
            if( charges > 0 ) {
                buffer << " " << string_format( ngettext( "(%d charge)", "(%d charges)", charges ), charges );
            }
            buffer << "</color>";
        }
        ypos += fold_and_print( w, ypos, xpos + 2, width - 2, col, buffer.str() );
    }
    ypos++;
    return ypos - oldy;
}

int requirements::print_time( WINDOW *w, int ypos, int xpos, int width, nc_color col ) const
{
    const int turns = time / 100;
    std::string text;
    if( turns < MINUTES( 1 ) ) {
        const int seconds = std::max( 1, turns * 6 );
        text = string_format( ngettext( "%d second", "%d seconds", seconds ), seconds );
    } else {
        const int minutes = ( turns % HOURS( 1 ) ) / MINUTES( 1 );
        const int hours = turns / HOURS( 1 );
        if( hours == 0 ) {
            text = string_format( ngettext( "%d minute", "%d minutes", minutes ), minutes );
        } else if( minutes == 0 ) {
            text = string_format( ngettext( "%d hour", "%d hours", hours ), hours );
        } else {
            const std::string h = string_format( ngettext( "%d hour", "%d hours", hours ), hours );
            const std::string m = string_format( ngettext( "%d minute", "%d minutes", minutes ), minutes );
            //~ A time duration: first is hours, second is minutes, e.g. "4 hours" "6 minutes"
            text = string_format( _( "%s and %s" ), h.c_str(), m.c_str() );
        }
    }
    text = string_format( _( "Time to complete: %s" ), text.c_str() );
    return fold_and_print( w, ypos, xpos, width, col, text );
}

bool requirements::can_make_with_inventory( const inventory &crafting_inv ) const
{
    bool retval = true;
    // check all tool_quality requirements
    // this is an alternate method of checking for tools by using the tools qualities instead of the specific tool
    // You can specify the amount of tools with this quality required, but it does not work for consumed charges.
    for( const auto & qual_req : qualities ) {
        qual_req.available = crafting_inv.has_items_with_quality( qual_req.type, qual_req.level,
                             qual_req.count );
        if( !qual_req.available ) {
            retval = false;
        }
    }

    // check all tools
    for( const auto & set_of_tools : tools ) {
        bool has_tool_in_set = false;
        for( const auto & tool : set_of_tools ) {
            const itype_id &type = tool.type;
            int req = tool.count;
            if( req <= 0 && crafting_inv.has_amount( type, 1 ) ) {
                tool.available = 1;
            } else if( req <= 0 && type == ( "goggles_welding" ) && ( g->u.has_bionic( "bio_sunglasses" ) ||
                       g->u.is_wearing( "rm13_armor_on" ) ) ) {
                tool.available = 1;
            } else if( req > 0 && crafting_inv.has_charges( type, req ) ) {
                tool.available = 1;
            } else {
                tool.available = -1;
            }
            has_tool_in_set = has_tool_in_set || tool.available == 1;
        }
        if( !has_tool_in_set ) {
            retval = false;
        }
    }
    // check all components
    for( const auto & component_choices : components ) {
        bool has_comp_in_set = false;
        for( const auto & comp : component_choices ) {
            const itype_id &type = comp.type;
            int req = comp.count;
            // If you've Rope Webs, you can spin up the webbing to replace any amount of
            // rope your projects may require.  But you need to be somewhat nourished:
            // Famished or worse stops it.
            if( ( type == "rope_30" ||
                  type == "rope_6" ) &&
                g->u.has_trait( "WEB_ROPE" ) && g->u.hunger <= 300 ) {
                comp.available = 1;
            } else if( req > 0 && item_controller->find_template( type )->count_by_charges() ) {
                if( crafting_inv.has_charges( type, req ) ) {
                    comp.available = 1;
                } else {
                    comp.available = -1;
                }
            } else if( crafting_inv.has_components( type, abs( req ) ) ) {
                comp.available = 1;
            } else {
                comp.available = -1;
            }
            has_comp_in_set = has_comp_in_set || comp.available == 1;
        }
        if( !has_comp_in_set ) {
            retval = false;
        }
    }
    return check_enough_materials( crafting_inv ) && retval;
}

bool requirements::check_enough_materials( const inventory &crafting_inv ) const
{
    bool retval = true;
    for( const auto & component_choices : components ) {
        bool atleast_one_available = false;
        for( const auto & comp : component_choices ) {
            if( comp.available == 1 ) {
                bool have_enough_in_set = true;
                for( const auto & set_of_tools : tools ) {
                    bool have_enough = false;
                    bool found_same_type = false;
                    for( const auto & tool : set_of_tools ) {
                        if( tool.available == 1 ) {
                            if( comp.type == tool.type ) {
                                found_same_type = true;
                                bool count_by_charges =
                                    item_controller->find_template( comp.type )->count_by_charges();
                                if( count_by_charges ) {
                                    int req = comp.count;
                                    if( tool.count > 0 ) {
                                        req += tool.count;
                                    } else  {
                                        ++req;
                                    }
                                    if( crafting_inv.has_charges( comp.type, req ) ) {
                                        have_enough = true;
                                    }
                                } else {
                                    int req = comp.count + 1;
                                    if( crafting_inv.has_components( comp.type, req ) ) {
                                        have_enough = true;
                                    }
                                }
                            } else {
                                have_enough = true;
                            }
                        }
                    }
                    if( found_same_type ) {
                        have_enough_in_set &= have_enough;
                    }
                }
                if( !have_enough_in_set ) {
                    // This component can't be used with any tools
                    // from one of the sets of tools, which means
                    // its availability should be set to 0 (in inventory,
                    // but not enough for both tool and components).
                    comp.available = 0;
                }
            }
            //Flag that at least one of the components in the set is available
            if( comp.available == 1 ) {
                atleast_one_available = true;
            }
        }

        if( !atleast_one_available ) {
            // this set doesn't have any components available,
            // so the recipe can't be crafted
            retval = false;
        }
    }

    for( const auto & set_of_tools : tools ) {
        bool atleast_one_available = false;
        for( const auto & tool : set_of_tools ) {
            if( tool.available == 1 ) {
                bool have_enough_in_set = true;
                for( const auto & component_choices : components ) {
                    bool have_enough = false, conflict = false;
                    for( const auto & comp : component_choices ) {
                        if( tool.type == comp.type ) {
                            if( tool.count > 0 ) {
                                int req = comp.count + tool.count;
                                if( !crafting_inv.has_charges( comp.type, req ) ) {
                                    conflict = true;
                                    have_enough = have_enough || false;
                                }
                            } else {
                                int req = comp.count + 1;
                                if( !crafting_inv.has_components( comp.type, req ) ) {
                                    conflict = true;
                                    have_enough = have_enough || false;
                                }
                            }
                        } else if( comp.available == 1 ) {
                            have_enough = true;
                        }
                    }
                    if( conflict ) {
                        have_enough_in_set = have_enough_in_set && have_enough;
                    }
                }
                if( !have_enough_in_set ) {
                    // This component can't be used with any components
                    // from one of the sets of components, which means
                    // its availability should be set to 0 (in inventory,
                    // but not enough for both tool and components).
                    tool.available = 0;
                }
            }
            //Flag that at least one of the tools in the set is available
            if( tool.available == 1 ) {
                atleast_one_available = true;
            }
        }

        if( !atleast_one_available ) {
            // this set doesn't have any tools available,
            // so the recipe can't be crafted
            retval = false;
        }
    }

    return retval;
}
