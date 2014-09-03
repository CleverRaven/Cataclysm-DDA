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

std::string quality_requirement::to_string(int batch) const
{
    return string_format( ngettext( "%d tool with %s of %d or more.",
                                    "%d tools with %s of %d or more.", count ),
                          count, quality::get_name( type ).c_str(), level );
}

std::string tool_comp::to_string(int batch) const
{
    const itype *it = item_controller->find_template( type );
    if( count > 0 ) {
        //~ <tool-name> (<numer-of-charges> charges)
        return string_format( ngettext( "%s (%d charge)", "%s (%d charges)", count * batch ),
                              it->nname( 1 ).c_str(), count * batch );
    } else {
        return it->nname( abs( count ) );
    }
}

std::string item_comp::to_string(int batch) const
{
    const itype *it = item_controller->find_template( type );
    const int c = (count > 0) ? count * batch : abs( count );
    //~ <item-count> <item-name>
    return string_format( ngettext( "%d %s", "%d %s", c ), c, it->nname( c ).c_str() );
}

void quality_requirement::load( JsonArray &jsarr )
{
    JsonObject quality_data = jsarr.next_object();
    type = quality_data.get_string( "id" );
    level = quality_data.get_int( "level", 1 );
    count = quality_data.get_int( "amount", 1 );
}

void tool_comp::load( JsonArray &ja )
{
    if( ja.test_string() ) {
        // constructions uses this format: [ "tool", ... ]
        type = ja.next_string();
        count = -1;
    } else {
        JsonArray comp = ja.next_array();
        type = comp.get_string( 0 );
        count = comp.get_int( 1 );
    }
}

void item_comp::load( JsonArray &ja )
{
    JsonArray comp = ja.next_array();
    type = comp.get_string( 0 );
    count = comp.get_int( 1 );
}

template<typename T>
void requirements::load_obj_list(JsonArray &jsarr, std::vector< std::vector<T> > &objs)
{
    while (jsarr.has_more()) {
        if(jsarr.test_array()) {
            std::vector<T> choices;
            JsonArray ja = jsarr.next_array();
            while (ja.has_more()) {
                choices.push_back(T());
                choices.back().load(ja);
            }
            if( !choices.empty() ) {
                objs.push_back( choices );
            }
        } else {
            // tool qualities don't normally use a list of alternatives
            // each quality is mandatory.
            objs.push_back(std::vector<T>(1));
            objs.back().back().load(jsarr);
        }
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

template<typename T>
bool requirements::any_marked_available( const std::vector<T> &comps )
{
    for( const auto &comp : comps ) {
        if( comp.available == a_true ) {
            return true;
        }
    }
    return false;
}

template<typename T>
std::string requirements::print_missing_objs(const std::string &header,
        const std::vector< std::vector<T> > &objs)
{
    std::ostringstream buffer;
    for( const auto &list : objs ) {
        if( any_marked_available( list ) ) {
            continue;
        }
        if( !buffer.str().empty() ) {
            buffer << "\n" << _("and ");
        }
        for( auto it = list.begin(); it != list.end(); ++it ) {
            if( it != list.begin() ) {
                buffer << _(" or ");
            }
            buffer << it->to_string();
        }
    }
    if (buffer.str().empty()) {
        return std::string();
    }
    return header + "\n" + buffer.str() + "\n";
}

std::string requirements::list_missing() const
{
    std::ostringstream buffer;
    buffer << print_missing_objs(_("These tools are missing:"), tools);
    buffer << print_missing_objs(_("These tools are missing:"), qualities);
    buffer << print_missing_objs(_("Those components are missing:"), components);
    return buffer.str();
}

void quality_requirement::check_consistency( const std::string &display_name ) const
{
    if( !quality::has( type ) ) {
        debugmsg( "Unknown quality %s in %s", type.c_str(), display_name.c_str() );
    }
}

void component::check_consistency( const std::string &display_name ) const
{
    if( !item_controller->has_template( type ) ) {
        debugmsg( "%s in %s is not a valid item template", type.c_str(), display_name.c_str() );
    }
}

template<typename T>
void requirements::check_consistency( const std::vector< std::vector<T> > &vec,
                                      const std::string &display_name )
{
    for( const auto &list : vec ) {
        for( const auto &comp : list ) {
            comp.check_consistency( display_name );
        }
    }
}

void requirements::check_consistency( const std::string &display_name ) const
{
    check_consistency(tools, display_name);
    check_consistency(components, display_name);
    check_consistency(qualities, display_name);
}

int requirements::print_components( WINDOW *w, int ypos, int xpos, int width, nc_color col,
                                    const inventory &crafting_inv, int batch ) const
{
    if( components.empty() ) {
        return 0;
    }
    mvwprintz( w, ypos, xpos, col, _( "Components required:" ) );
    return print_list( w, ypos + 1, xpos, width, col, crafting_inv, components, batch ) + 1;
}

template<typename T>
int requirements::print_list( WINDOW *w, int ypos, int xpos, int width, nc_color col,
                              const inventory &crafting_inv, const std::vector< std::vector<T> > &objs,
                              int batch )
{
    const int oldy = ypos;
    for( const auto &comp_list : objs ) {
        const bool has_one = any_marked_available( comp_list );
        std::ostringstream buffer;
        for( auto a = comp_list.begin(); a != comp_list.end(); ++a ) {
            if( a != comp_list.begin() ) {
                buffer << "<color_white> " << _( "OR" ) << "</color> ";
            }
            const std::string col = a->get_color( has_one, crafting_inv, batch );
            buffer << "<color_" << col << ">" << a->to_string(batch) << "</color>";
        }
        mvwprintz( w, ypos, xpos, col, "> " );
        ypos += fold_and_print( w, ypos, xpos + 2, width - 2, col, buffer.str() );
    }
    return ypos - oldy;
}

int requirements::print_tools( WINDOW *w, int ypos, int xpos, int width, nc_color col,
                               const inventory &crafting_inv, int batch ) const
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
    ypos += print_list(w, ypos, xpos, width, col, crafting_inv, qualities, batch);
    ypos += print_list(w, ypos, xpos, width, col, crafting_inv, tools, batch);
    return ypos - oldy;
}

int requirements::print_time( WINDOW *w, int ypos, int xpos, int width, nc_color col, int batch ) const
{
    const int turns = time * batch / 100;
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

bool requirements::can_make_with_inventory( const inventory &crafting_inv, int batch ) const
{
    bool retval = true;
    // Doing this in several steps avoids C++ Short-circuit evaluation
    // and makes sure that the available value is set for every entry
    retval &= has_comps(crafting_inv, qualities, batch);
    retval &= has_comps(crafting_inv, tools, batch);
    retval &= has_comps(crafting_inv, components, batch);
    retval &= check_enough_materials(crafting_inv, batch);
    return retval;
}

template<typename T>
bool requirements::has_comps( const inventory &crafting_inv,
                              const std::vector< std::vector<T> > &vec,
                              int batch )
{
    bool retval = true;
    for( const auto &set_of_tools : vec ) {
        bool has_tool_in_set = false;
        for( const auto &tool : set_of_tools ) {
            if( tool.has( crafting_inv, batch ) ) {
                tool.available = a_true;
            } else {
                tool.available = a_false;
            }
            has_tool_in_set = has_tool_in_set || tool.available == a_true;
        }
        if( !has_tool_in_set ) {
            retval = false;
        }
    }
    return retval;
}

bool quality_requirement::has( const inventory &crafting_inv, int batch ) const
{
    return crafting_inv.has_items_with_quality( type, level, count );
}

std::string quality_requirement::get_color( bool, const inventory &, int batch) const
{
    return available == a_true ? "green" : "red";
}

bool tool_comp::has( const inventory &crafting_inv, int batch ) const
{
    if( type == "goggles_welding" ) {
        if( g->u.has_bionic( "bio_sunglasses" ) || g->u.is_wearing( "rm13_armor_on" ) ) {
            return true;
        }
    }
    if( count <= 0 ) {
        return crafting_inv.has_tools( type, std::abs( count ) );
    } else {
        return crafting_inv.has_charges( type, count * batch );
    }
}

std::string tool_comp::get_color( bool has_one, const inventory &crafting_inv, int batch ) const
{
    if( type == "goggles_welding" ) {
        if( g->u.has_bionic( "bio_sunglasses" ) || g->u.is_wearing( "rm13_armor_on" ) ) {
            return "cyan";
        }
    }
    if( available == a_insufficent ) {
        return "brown";
    } else if( count < 0 && crafting_inv.has_tools( type, std::abs( count ) ) ) {
        return "green";
    } else if( count > 0 && crafting_inv.has_charges( type, count * batch ) ) {
        return "green";
    }
    return has_one ? "dkgray" : "red";
}

bool item_comp::has( const inventory &crafting_inv, int batch ) const
{
    // If you've Rope Webs, you can spin up the webbing to replace any amount of
    // rope your projects may require.  But you need to be somewhat nourished:
    // Famished or worse stops it.
    if( type == "rope_30" || type == "rope_6" ) {
        // NPC don't craft?
        // TODO: what about the amount of ropes vs the hunger?
        if( g->u.has_trait( "WEB_ROPE" ) && g->u.hunger <= 300 ) {
            return true;
        }
    }
    const itype *it = item_controller->find_template( type );
    if( it->count_by_charges() && count > 0 ) {
        return crafting_inv.has_charges( type, count * batch );
    } else {
        return crafting_inv.has_components( type, (count > 0) ? count * batch : abs( count ) );
    }
}

std::string item_comp::get_color( bool has_one, const inventory &crafting_inv, int batch ) const
{
    if( type == "rope_30" || type == "rope_6" ) {
        if( g->u.has_trait( "WEB_ROPE" ) && g->u.hunger <= 300 ) {
            return "ltgreen"; // Show that WEB_ROPE is on the job!
        }
    }
    const itype *it = item_controller->find_template( type );
    if( available == a_insufficent ) {
        return "brown";
    } else if( it->count_by_charges() && count > 0 ) {
        if( crafting_inv.has_charges( type, count * batch ) ) {
            return "green";
        }
    } else if( crafting_inv.has_components( type, (count > 0) ? count * batch : abs( count ) ) ) {
        return "green";
    }
    return has_one ? "dkgray" : "red";
}

template<typename T>
const T *requirements::find_by_type(const std::vector< std::vector<T> > &vec,
                                    const std::string &type)
{
    for( const auto &list : vec) {
        for( const auto &comp : list) {
            if( comp.type == type ) {
                return &comp;
            }
        }
    }
    return nullptr;
}

bool requirements::check_enough_materials( const inventory &crafting_inv, int batch ) const
{
    bool retval = true;
    for( const auto &component_choices : components ) {
        bool atleast_one_available = false;
        for( const auto &comp : component_choices ) {
            if( check_enough_materials( comp, crafting_inv, batch ) ) {
                atleast_one_available = true;
            }
        }
        if( !atleast_one_available ) {
            retval = false;
        }
    }
    return retval;
}

bool requirements::check_enough_materials( const item_comp &comp,
        const inventory &crafting_inv, int batch ) const
{
    if( comp.available != a_true ) {
        return false;
    }
    const itype *it = item_controller->find_template( comp.type );
    const tool_comp *tq = find_by_type( tools, comp.type );
    if( tq != nullptr ) {
        // The very same item type is also needed as tool!
        // Use charges of it, or use it by count?
        const int tc = tq->count < 0 ? std::abs( tq->count ) : 1;
        // Check for components + tool count. Check item amount (excludes
        // pseudo items) and tool amount (includes pseudo items)
        // Imagine: required = 1 welder (component) + 1 welder (tool),
        // available = 1 welder (real item), 1 welding rig (creates
        // a pseudo welder item). has_components(welder,2) returns false
        // as there is only one real welder available, but has_tools(welder,2)
        // returns true.
        // Keep in mind that both requirements (tool+component) are checked
        // before this. That assures that one real item is actually available,
        // two welding rigs (and no real welder) would make this component
        // non-available even before this function is called.
        // Only ammo and (some) food is counted by charges, both are unlikely
        // to appear as tool, but it's possible /-:
        bool has_comps;
        if( it->count_by_charges() && comp.count > 0 ) {
            has_comps = crafting_inv.has_charges( comp.type, comp.count * batch + tc );
        } else {
            has_comps = crafting_inv.has_components( comp.type, ((comp.count > 0) ? comp.count * batch : abs( comp.count )) + tc );
        }
        if( !has_comps && !crafting_inv.has_tools( comp.type, comp.count + tc ) ) {
            comp.available = a_insufficent;
        }
    }
    for( const auto &ql : it->qualities ) {
        const quality_requirement *qr = find_by_type( qualities, ql.first );
        if( qr == nullptr || qr->level > ql.second ) {
            continue;
        }
        // This item can be used for the quality requirement, same as above for specific
        // tools applies.
        if( !crafting_inv.has_items_with_quality( qr->type, qr->level, qr->count + abs(comp.count) ) ) {
            comp.available = a_insufficent;
        }
    }
    return comp.available == a_true;
}

template<typename T>
bool requirements::remove_item( const std::string &type, std::vector< std::vector<T> > &vec )
{
    for( auto b = vec.begin(); b != vec.end(); b++ ) {
        for( auto c = b->begin(); c != b->end(); ) {
            if( c->type == type ) {
                if( b->size() == 1 ) {
                    return true;
                }
                c = b->erase( c );
            } else {
                ++c;
            }
        }
    }
    return false;
}

bool requirements::remove_item( const std::string &type )
{
    return remove_item( type, tools ) || remove_item( type, components );
}
