#include "requirements.h"

#include <limits.h>
#include <cstdlib>
#include <algorithm>
#include <limits>
#include <sstream>
#include <iterator>
#include <list>
#include <memory>
#include <set>
#include <unordered_map>

#include "avatar.h"
#include "cata_utility.h"
#include "debug.h"
#include "game.h"
#include "generic_factory.h"
#include "inventory.h"
#include "item_factory.h"
#include "itype.h"
#include "json.h"
#include "output.h"
#include "string_formatter.h"
#include "translations.h"
#include "color.h"
#include "item.h"
#include "visitable.h"
#include "point.h"

static const trait_id trait_DEBUG_HS( "DEBUG_HS" );

static std::map<requirement_id, requirement_data> requirements_all;

/** @relates string_id */
template<>
bool string_id<requirement_data>::is_valid() const
{
    return requirements_all.count( *this );
}

/** @relates string_id */
template<>
const requirement_data &string_id<requirement_data>::obj() const
{
    const auto found = requirements_all.find( *this );
    if( found == requirements_all.end() ) {
        debugmsg( "Tried to get invalid requirements: %s", c_str() );
        static const requirement_data null_requirement{};
        return null_requirement;
    }
    return found->second;
}

namespace
{
generic_factory<quality> quality_factory( "tool quality" );
} // namespace

void quality::reset()
{
    quality_factory.reset();
}

void quality::load_static( JsonObject &jo, const std::string &src )
{
    quality_factory.load( jo, src );
}

void quality::load( JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "name", name );

    JsonArray arr = jo.get_array( "usages" );
    while( arr.has_more() ) {
        auto lvl = arr.next_array();
        auto funcs = lvl.get_array( 1 );
        while( funcs.has_more() ) {
            usages.emplace_back( lvl.get_int( 0 ), funcs.next_string() );
        }
    }
}

/** @relates string_id */
template<>
const quality &string_id<quality>::obj() const
{
    return quality_factory.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<quality>::is_valid() const
{
    return quality_factory.is_valid( *this );
}

std::string quality_requirement::to_string( const int, const int ) const
{
    //~ %1$d: tool count, %2$s: quality requirement name, %3$d: quality level requirement
    return string_format( ngettext( "%1$d tool with %2$s of %3$d or more.",
                                    "%1$d tools with %2$s of %3$d or more.", count ),
                          count, type.obj().name, level );
}

bool tool_comp::by_charges() const
{
    return count > 0;
}

std::string tool_comp::to_string( const int batch, const int ) const
{
    if( by_charges() ) {
        //~ %1$s: tool name, %2$d: charge requirement
        return string_format( npgettext( "requirement", "%1$s (%2$d charge)", "%1$s (%2$d charges)",
                                         count * batch ),
                              item::nname( type ), count * batch );
    } else {
        return item::nname( type, abs( count ) );
    }
}

std::string item_comp::to_string( const int batch, const int avail ) const
{
    const int c = std::abs( count ) * batch;
    const auto type_ptr = item::find_type( type );
    if( type_ptr->count_by_charges() ) {
        if( avail == 2147483647/* int32 max */ ) {
            //~ %1$s: item name, %2$d: charge requirement
            return string_format( npgettext( "requirement", "%1$s (%2$d of infinite)",
                                             "%1$s (%2$d of infinite)",
                                             c ),
                                  type_ptr->nname( 1 ), c );
        } else if( avail > 0 ) {
            //~ %1$s: item name, %2$d: charge requirement, %3%d: available charges
            return string_format( npgettext( "requirement", "%1$s (%2$d of %3$d)", "%1$s (%2$d of %3$d)", c ),
                                  type_ptr->nname( 1 ), c, avail );
        } else {
            //~ %1$s: item name, %2$d: charge requirement
            return string_format( npgettext( "requirement", "%1$s (%2$d)", "%1$s (%2$d)", c ),
                                  type_ptr->nname( 1 ), c );
        }
    } else {
        //~ %1$s: item name, %2$d: charge requirement
        if( avail == 2147483647/* int32 max */ ) {
            return string_format( npgettext( "requirement", "%2$d %1$s of infinite", "%2$d %1$s of infinite",
                                             c ),
                                  type_ptr->nname( c ), c );
        } else if( avail > 0 ) {
            //~ %1$s: item name, %2$d: required count, %3%d: available count
            return string_format( npgettext( "requirement", "%2$d %1$s of %3$d", "%2$d %1$s of %3$d", c ),
                                  type_ptr->nname( c ), c, avail );
        } else {
            //~ %1$s: item name, %2$d: required count
            return string_format( npgettext( "requirement", "%2$d %1$s", "%2$d %1$s", c ),
                                  type_ptr->nname( c ), c );
        }
    }
}

void quality_requirement::load( JsonArray &jsarr )
{
    JsonObject quality_data = jsarr.next_object();
    type = quality_id( quality_data.get_string( "id" ) );
    level = quality_data.get_int( "level", 1 );
    count = quality_data.get_int( "amount", 1 );
    if( count <= 0 ) {
        quality_data.throw_error( "quality amount must be a positive number", "amount" );
    }
    // Note: level is not checked, negative values and 0 are allow, see butchering quality.
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
        requirement = comp.size() > 2 && comp.get_string( 2 ) == "LIST";
    }
    if( count == 0 ) {
        ja.throw_error( "tool count must not be 0" );
    }
    // Note: negative count means charges (of the tool) should be consumed
}

void item_comp::load( JsonArray &ja )
{
    JsonArray comp = ja.next_array();
    type = comp.get_string( 0 );
    count = comp.get_int( 1 );
    size_t handled = 2;
    while( comp.size() > handled ) {
        const std::string &flag = comp.get_string( handled++ );
        if( flag == "NO_RECOVER" ) {
            recoverable = false;
        } else if( flag == "LIST" ) {
            requirement = true;
        }
    }
    if( count <= 0 ) {
        ja.throw_error( "item count must be a positive number" );
    }
}

template<typename T>
void requirement_data::load_obj_list( JsonArray &jsarr, std::vector< std::vector<T> > &objs )
{
    while( jsarr.has_more() ) {
        if( jsarr.test_array() ) {
            std::vector<T> choices;
            JsonArray ja = jsarr.next_array();
            while( ja.has_more() ) {
                choices.push_back( T() );
                choices.back().load( ja );
            }
            if( !choices.empty() ) {
                objs.push_back( choices );
            }
        } else {
            // tool qualities don't normally use a list of alternatives
            // each quality is mandatory.
            objs.push_back( std::vector<T>( 1 ) );
            objs.back().back().load( jsarr );
        }
    }
}

requirement_data requirement_data::operator*( unsigned scalar ) const
{
    requirement_data res = *this;
    for( auto &group : res.components ) {
        for( auto &e : group ) {
            e.count = std::max( e.count * static_cast<int>( scalar ), -1 );
        }
    }
    for( auto &group : res.tools ) {
        for( auto &e : group ) {
            e.count = std::max( e.count * static_cast<int>( scalar ), -1 );
        }
    }

    return res;
}

requirement_data requirement_data::operator+( const requirement_data &rhs ) const
{
    requirement_data res = *this;

    res.components.insert( res.components.end(), rhs.components.begin(), rhs.components.end() );
    res.tools.insert( res.tools.end(), rhs.tools.begin(), rhs.tools.end() );
    res.qualities.insert( res.qualities.end(), rhs.qualities.begin(), rhs.qualities.end() );

    // combined result is temporary which caller could store via @ref save_requirement
    res.id_ = requirement_id::NULL_ID();

    // TODO: deduplicate qualities and combine other requirements

    // if either operand was blacklisted then their summation should also be
    res.blacklisted |= rhs.blacklisted;

    return res;
}

void requirement_data::load_requirement( JsonObject &jsobj, const requirement_id &id )
{
    requirement_data req;

    JsonArray jsarr = jsobj.get_array( "components" );
    requirement_data::load_obj_list( jsarr, req.components );
    jsarr = jsobj.get_array( "qualities" );
    requirement_data::load_obj_list( jsarr, req.qualities );
    jsarr = jsobj.get_array( "tools" );
    requirement_data::load_obj_list( jsarr, req.tools );

    if( !id.is_null() ) {
        req.id_ = id;
    } else if( jsobj.has_string( "id" ) ) {
        req.id_ = requirement_id( jsobj.get_string( "id" ) );
    } else {
        jsobj.throw_error( "id was not specified for requirement" );
    }

    save_requirement( req );
}

void requirement_data::save_requirement( const requirement_data &req, const requirement_id &id )
{
    auto dup = req;
    if( !id.is_null() ) {
        dup.id_ = id;
    }

    requirements_all[ dup.id_ ] = dup;
}

template<typename T>
bool requirement_data::any_marked_available( const std::vector<T> &comps )
{
    for( const auto &comp : comps ) {
        if( comp.available == a_true ) {
            return true;
        }
    }
    return false;
}

template<typename T>
std::string requirement_data::print_all_objs( const std::string &header,
        const std::vector< std::vector<T> > &objs )
{
    std::ostringstream buffer;
    for( const auto &list : objs ) {
        if( !buffer.str().empty() ) {
            buffer << "\n" << _( "and " );
        }
        for( auto it = list.begin(); it != list.end(); ++it ) {
            if( it != list.begin() ) {
                buffer << _( " or " );
            }
            buffer << it->to_string();
        }
    }
    if( buffer.str().empty() ) {
        return std::string();
    }
    return header + "\n" + buffer.str() + "\n";
}

std::string requirement_data::list_all() const
{
    std::ostringstream buffer;
    buffer << print_all_objs( _( "These tools are required:" ), tools );
    buffer << print_all_objs( _( "These tools are required:" ), qualities );
    buffer << print_all_objs( _( "These components are required:" ), components );
    return buffer.str();
}

template<typename T>
std::string requirement_data::print_missing_objs( const std::string &header,
        const std::vector< std::vector<T> > &objs )
{
    std::ostringstream buffer;
    for( const auto &list : objs ) {
        if( any_marked_available( list ) ) {
            continue;
        }
        if( !buffer.str().empty() ) {
            buffer << "\n" << _( "and " );
        }
        for( auto it = list.begin(); it != list.end(); ++it ) {
            if( it != list.begin() ) {
                buffer << _( " or " );
            }
            buffer << it->to_string();
        }
    }
    if( buffer.str().empty() ) {
        return std::string();
    }
    return header + "\n" + buffer.str() + "\n";
}

std::string requirement_data::list_missing() const
{
    std::ostringstream buffer;
    buffer << print_missing_objs( _( "These tools are missing:" ), tools );
    buffer << print_missing_objs( _( "These tools are missing:" ), qualities );
    buffer << print_missing_objs( _( "These components are missing:" ), components );
    return buffer.str();
}

void quality_requirement::check_consistency( const std::string &display_name ) const
{
    if( !type.is_valid() ) {
        debugmsg( "Unknown quality %s in %s", type.c_str(), display_name );
    }
}

void component::check_consistency( const std::string &display_name ) const
{
    if( !item::type_is_defined( type ) ) {
        debugmsg( "%s in %s is not a valid item template", type.c_str(), display_name );
    }
}

template<typename T>
void requirement_data::check_consistency( const std::vector< std::vector<T> > &vec,
        const std::string &display_name )
{
    for( const auto &list : vec ) {
        for( const auto &comp : list ) {
            if( comp.requirement ) {
                debugmsg( "Finalization failed to inline %s in %s", comp.type.c_str(), display_name );
            }

            comp.check_consistency( display_name );
        }
    }
}

const std::map<requirement_id, requirement_data> &requirement_data::all()
{
    return requirements_all;
}

void requirement_data::check_consistency()
{
    for( const auto &r : all() ) {
        check_consistency( r.second.tools, r.first.str() );
        check_consistency( r.second.components, r.first.str() );
        check_consistency( r.second.qualities, r.first.str() );
    }
}

template <typename T, typename Getter>
void inline_requirements( std::vector< std::vector<T> > &list, Getter getter )
{
    std::set<requirement_id> already_nested;
    for( auto &vec : list ) {
        // We always need to restart from the beginning in case of vector relocation
        while( true ) {
            auto iter = std::find_if( vec.begin(), vec.end(), []( const T & req ) {
                return req.requirement;
            } );
            if( iter == vec.end() ) {
                break;
            }

            const auto req_id = requirement_id( iter->type );
            if( !req_id.is_valid() ) {
                debugmsg( "Tried to inline unknown requirement %s", req_id.c_str() );
                return;
            }

            if( already_nested.count( req_id ) > 0 ) {
                debugmsg( "Tried to inline requirement %s which was inlined before in the same pass (infinite loop?)",
                          req_id.c_str() );
                return;
            }

            already_nested.insert( req_id );
            const auto &req = req_id.obj();
            const requirement_data multiplied = req * iter->count;
            iter = vec.erase( iter );

            const auto &to_inline = getter( multiplied );
            vec.insert( iter, to_inline.front().begin(), to_inline.front().end() );
        }
    }
}

void requirement_data::finalize()
{
    for( auto &r : const_cast<std::map<requirement_id, requirement_data> &>( all() ) ) {
        inline_requirements( r.second.tools, []( const requirement_data & d ) {
            return d.get_tools();
        } );
        inline_requirements( r.second.components, []( const requirement_data & d ) {
            return d.get_components();
        } );
        auto &vec = r.second.tools;
        for( auto &list : vec ) {
            std::vector<tool_comp> new_list;
            for( auto &comp : list ) {
                const auto replacements = item_controller->subtype_replacement( comp.type );
                for( const auto &replaced_type : replacements ) {
                    new_list.emplace_back( replaced_type, comp.count );
                }
            }

            list = new_list;
        }
    }
}

void requirement_data::reset()
{
    requirements_all.clear();
}

std::vector<std::string> requirement_data::get_folded_components_list( int width, nc_color col,
        const inventory &crafting_inv, const std::function<bool( const item & )> &filter, int batch,
        std::string hilite ) const
{
    std::vector<std::string> out_buffer;
    if( components.empty() ) {
        return out_buffer;
    }
    out_buffer.push_back( colorize( _( "Components required:" ), col ) );

    std::vector<std::string> folded_buffer =
        get_folded_list( width, crafting_inv, filter, components, batch, hilite );
    out_buffer.insert( out_buffer.end(), folded_buffer.begin(), folded_buffer.end() );

    return out_buffer;
}

template<typename T>
std::vector<std::string> requirement_data::get_folded_list( int width,
        const inventory &crafting_inv, const std::function<bool( const item & )> &filter,
        const std::vector< std::vector<T> > &objs, int batch, const std::string &hilite ) const
{
    // hack: ensure 'cached' availability is up to date
    can_make_with_inventory( crafting_inv, filter );

    std::vector<std::string> out_buffer;
    for( const auto &comp_list : objs ) {
        const bool has_one = any_marked_available( comp_list );
        std::vector<std::string> list_as_string;
        std::vector<std::string> buffer_has;
        for( const T &component : comp_list ) {
            nc_color color = component.get_color( has_one, crafting_inv, filter, batch );
            const std::string color_tag = get_tag_from_color( color );
            int qty = 0;
            if( component.get_component_type() == COMPONENT_ITEM ) {
                const itype_id item_id = static_cast<itype_id>( component.type );
                if( item::count_by_charges( item_id ) ) {
                    qty = crafting_inv.charges_of( item_id, INT_MAX, filter );
                } else {
                    qty = crafting_inv.amount_of( item_id, false, INT_MAX, filter );
                }
            }
            const std::string text = component.to_string( batch, qty );

            if( std::find( buffer_has.begin(), buffer_has.end(), text + color_tag ) != buffer_has.end() ) {
                continue;
            }

            if( !hilite.empty() && lcmatch( text, hilite ) ) {
                color = yellow_background( color );
            }

            list_as_string.push_back( colorize( text, color ) );
            buffer_has.push_back( text + color_tag );
        }
        std::sort( list_as_string.begin(), list_as_string.end() );

        const std::string separator = colorize( _( " OR " ), c_white );
        const std::string unfolded = join( list_as_string, separator );
        std::vector<std::string> folded = foldstring( unfolded, width - 2 );

        for( size_t i = 0; i < folded.size(); i++ ) {
            if( i == 0 ) {
                out_buffer.push_back( std::string( "> " ).append( folded[i] ) );
            } else {
                out_buffer.push_back( std::string( "  " ).append( folded[i] ) );
            }
        }
    }
    return out_buffer;
}

std::vector<std::string> requirement_data::get_folded_tools_list( int width, nc_color col,
        const inventory &crafting_inv, int batch ) const
{
    std::vector<std::string> output_buffer;
    output_buffer.push_back( colorize( _( "Tools required:" ), col ) );
    if( tools.empty() && qualities.empty() ) {
        output_buffer.push_back( colorize( "> ", col ) + colorize( _( "NONE" ), c_green ) );
        return output_buffer;
    }

    std::vector<std::string> folded_qualities = get_folded_list( width, crafting_inv, return_true<item>,
            qualities );
    output_buffer.insert( output_buffer.end(), folded_qualities.begin(), folded_qualities.end() );

    std::vector<std::string> folded_tools = get_folded_list( width, crafting_inv, return_true<item>,
                                            tools,
                                            batch );
    output_buffer.insert( output_buffer.end(), folded_tools.begin(), folded_tools.end() );
    return output_buffer;
}

bool requirement_data::can_make_with_inventory( const inventory &crafting_inv,
        const std::function<bool( const item & )> &filter, int batch ) const
{
    if( g->u.has_trait( trait_DEBUG_HS ) ) {
        return true;
    }

    bool retval = true;
    // All functions must be called to update the available settings in the components.
    if( !has_comps( crafting_inv, qualities, return_true<item> ) ) {
        retval = false;
    }
    if( !has_comps( crafting_inv, tools, return_true<item>, batch ) ) {
        retval = false;
    }
    if( !has_comps( crafting_inv, components, filter, batch ) ) {
        retval = false;
    }
    if( !check_enough_materials( crafting_inv, filter, batch ) ) {
        retval = false;
    }
    return retval;
}

template<typename T>
bool requirement_data::has_comps( const inventory &crafting_inv,
                                  const std::vector< std::vector<T> > &vec,
                                  const std::function<bool( const item & )> &filter,
                                  int batch )
{
    bool retval = true;
    int total_UPS_charges_used = 0;
    for( const auto &set_of_tools : vec ) {
        bool has_tool_in_set = false;
        int UPS_charges_used = std::numeric_limits<int>::max();
        for( const auto &tool : set_of_tools ) {
            if( tool.has( crafting_inv, filter, batch, [ &UPS_charges_used ]( int charges ) {
            UPS_charges_used = std::min( UPS_charges_used, charges );
            } ) ) {
                tool.available = a_true;
            } else {
                tool.available = a_false;
            }
            has_tool_in_set = has_tool_in_set || tool.available == a_true;
        }
        if( !has_tool_in_set ) {
            retval = false;
        }
        if( UPS_charges_used != std::numeric_limits<int>::max() ) {
            total_UPS_charges_used += UPS_charges_used;
        }
    }

    if( total_UPS_charges_used > 0 &&
        total_UPS_charges_used > crafting_inv.charges_of( "UPS" ) ) {
        return false;
    }
    return retval;
}

bool quality_requirement::has( const inventory &crafting_inv,
                               const std::function<bool( const item & )> &, int, std::function<void( int )> ) const
{
    if( g->u.has_trait( trait_DEBUG_HS ) ) {
        return true;
    }
    return crafting_inv.has_quality( type, level, count );
}

nc_color quality_requirement::get_color( bool has_one, const inventory &,
        const std::function<bool( const item & )> &, int ) const
{
    if( available == a_true ) {
        return c_green;
    }
    return has_one ? c_dark_gray : c_red;
}

bool tool_comp::has( const inventory &crafting_inv,
                     const std::function<bool( const item & )> &filter, int batch,
                     std::function<void( int )> visitor ) const
{
    if( g->u.has_trait( trait_DEBUG_HS ) ) {
        return true;
    }
    if( !by_charges() ) {
        return crafting_inv.has_tools( type, std::abs( count ), filter );
    } else {
        const int charges_required = count * batch * item::find_type( type )->charge_factor();

        int charges_found = crafting_inv.charges_of( type, charges_required, filter, visitor );
        return charges_found == charges_required;
    }
}

nc_color tool_comp::get_color( bool has_one, const inventory &crafting_inv,
                               const std::function<bool( const item & )> &filter, int batch ) const
{
    if( available == a_insufficent ) {
        return c_brown;
    } else if( has( crafting_inv, filter, batch ) ) {
        return c_green;
    }
    return has_one ? c_dark_gray : c_red;
}

bool item_comp::has( const inventory &crafting_inv,
                     const std::function<bool( const item & )> &filter, int batch,
                     std::function<void( int )> ) const
{
    if( g->u.has_trait( trait_DEBUG_HS ) ) {
        return true;
    }
    const int cnt = std::abs( count ) * batch;
    if( item::count_by_charges( type ) ) {
        return crafting_inv.has_charges( type, cnt, filter );
    } else {
        return crafting_inv.has_components( type, cnt, filter );
    }
}

nc_color item_comp::get_color( bool has_one, const inventory &crafting_inv,
                               const std::function<bool( const item & )> &filter, int batch ) const
{
    if( available == a_insufficent ) {
        return c_brown;
    } else if( has( crafting_inv, filter, batch ) ) {
        return c_green;
    }
    return has_one ? c_dark_gray  : c_red;
}

template<typename T, typename ID>
const T *requirement_data::find_by_type( const std::vector< std::vector<T> > &vec,
        const ID &type )
{
    for( const auto &list : vec ) {
        for( const auto &comp : list ) {
            if( comp.type == type ) {
                return &comp;
            }
        }
    }
    return nullptr;
}

bool requirement_data::check_enough_materials( const inventory &crafting_inv,
        const std::function<bool( const item & )> &filter, int batch ) const
{
    bool retval = true;
    for( const auto &component_choices : components ) {
        bool atleast_one_available = false;
        for( const auto &comp : component_choices ) {
            if( check_enough_materials( comp, crafting_inv, filter, batch ) ) {
                atleast_one_available = true;
            }
        }
        if( !atleast_one_available ) {
            retval = false;
        }
    }
    return retval;
}

bool requirement_data::check_enough_materials( const item_comp &comp, const inventory &crafting_inv,
        const std::function<bool( const item & )> &filter, int batch ) const
{
    if( comp.available != a_true ) {
        return false;
    }
    const int cnt = std::abs( comp.count ) * batch;
    const tool_comp *tq = find_by_type( tools, comp.type );
    if( tq != nullptr && tq->available == a_true ) {
        // The very same item type is also needed as tool!
        // Use charges of it, or use it by count?
        const int tc = tq->by_charges() ? 1 : std::abs( tq->count );
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
        const item_comp i_tmp( comp.type, cnt + tc );
        const tool_comp t_tmp( comp.type, -( cnt + tc ) ); // not by charges!
        // batch factor is explicitly 1, because it's already included in the count.
        if( !i_tmp.has( crafting_inv, filter, 1 ) && !t_tmp.has( crafting_inv, filter, 1 ) ) {
            comp.available = a_insufficent;
        }
    }
    const itype *it = item::find_type( comp.type );
    for( const auto &ql : it->qualities ) {
        const quality_requirement *qr = find_by_type( qualities, ql.first );
        if( qr == nullptr || qr->level > ql.second ) {
            continue;
        }
        // This item can be used for the quality requirement, same as above for specific
        // tools applies.
        if( !crafting_inv.has_quality( qr->type, qr->level, qr->count + abs( comp.count ) ) ) {
            comp.available = a_insufficent;
        }
    }
    return comp.available == a_true;
}

template <typename T>
static bool apply_blacklist( std::vector<std::vector<T>> &vec, const std::string &id )
{
    // remove all instances of @id type from each of the options
    for( auto &opts : vec ) {
        opts.erase( std::remove_if( opts.begin(), opts.end(), [&id]( const T & e ) {
            return e.type == id;
        } ), opts.end() );
    }

    // did we remove the last instance of an option group?
    const bool blacklisted = std::any_of( vec.begin(), vec.end(), []( const std::vector<T> &e ) {
        return e.empty();
    } );

    // if an option group is left empty then it can be removed
    vec.erase( std::remove_if( vec.begin(), vec.end(), []( const std::vector<T> &e ) {
        return e.empty();
    } ), vec.end() );

    return blacklisted;
}

void requirement_data::blacklist_item( const std::string &id )
{
    blacklisted |= apply_blacklist( tools, id );
    blacklisted |= apply_blacklist( components, id );
}

const requirement_data::alter_tool_comp_vector &requirement_data::get_tools() const
{
    return tools;
}

const requirement_data::alter_quali_req_vector &requirement_data::get_qualities() const
{
    return qualities;
}

const requirement_data::alter_item_comp_vector &requirement_data::get_components() const
{
    return components;
}

requirement_data::alter_item_comp_vector &requirement_data::get_components()
{
    return components;
}

requirement_data requirement_data::disassembly_requirements() const
{
    // TODO:
    // Allow jsonizing those tool replacements

    // Make a copy
    // Maybe TODO: Cache it somewhere and return a reference instead
    requirement_data ret = *this;
    auto new_qualities = std::vector<quality_requirement>();
    bool remove_fire = false;
    for( auto &it : ret.tools ) {
        bool replaced = false;
        for( const auto &tool : it ) {
            const itype_id &type = tool.type;

            // If crafting required a welder or forge then disassembly requires metal sawing
            if( type == "welder" || type == "welder_crude" || type == "oxy_torch" ||
                type == "forge" || type == "char_forge" ) {
                new_qualities.emplace_back( quality_id( "SAW_M_FINE" ), 1, 1 );
                replaced = true;
                break;
            }
            //This only catches instances where the two tools are explicitly stated, and not just the required sewing quality
            if( type == "sewing_kit" ||
                type == "mold_plastic" ) {
                new_qualities.emplace_back( quality_id( "CUT" ), 1, 1 );
                replaced = true;
                break;
            }

            if( type == "crucible" ) {
                replaced = true;
                break;
            }
            //This ensures that you don't need a hand press to break down reloaded ammo.
            if( type == "press" ) {
                replaced = true;
                remove_fire = true;
                new_qualities.emplace_back( quality_id( "PULL" ), 1, 1 );
                break;
            }
            if( type == "fire" && remove_fire ) {
                replaced = true;
                break;
            }
        }

        if( replaced ) {
            // Replace the entire block of variants
            // This avoids the pesky integrated toolset
            it.clear();
        }
    }

    // Warning: This depends on the fact that tool qualities
    // are all mandatory (don't use variants)
    // If that ever changes, this will be wrong!
    if( ret.qualities.empty() ) {
        ret.qualities.resize( 1 );
    } else {
        //If the required quality level is not empty, iterate through and replace or remove
        //qualities with deconstruction equivalents
        for( auto &it : ret.qualities ) {
            bool replaced = false;
            for( const auto &quality : it ) {
                if( quality.type == quality_id( "SEW" ) ) {
                    replaced = true;
                    new_qualities.emplace_back( quality_id( "CUT" ), 1, quality.level );
                    break;
                }
                if( quality.type == quality_id( "GLARE" ) ) {
                    replaced = true;
                    //Just remove the glare protection requirement from deconstruction
                    //This only happens in case of a reversible recipe, an explicit
                    //deconstruction recipe can still specify glare protection
                    break;
                }
                if( quality.type == quality_id( "KNIT" ) ) {
                    replaced = true;
                    //Ditto for knitting needles
                    break;
                }
            }
            if( replaced ) {
                it.clear();
            }
        }
    }

    auto &qualities = ret.qualities[0];
    qualities.insert( qualities.end(), new_qualities.begin(), new_qualities.end() );
    // Remove duplicate qualities
    {
        const auto itr = std::unique( qualities.begin(), qualities.end(),
        []( const quality_requirement & a, const quality_requirement & b ) {
            return a.type == b.type;
        } );
        qualities.resize( std::distance( qualities.begin(), itr ) );
    }

    // Remove empty variant sections
    ret.tools.erase( std::remove_if( ret.tools.begin(), ret.tools.end(),
    []( const std::vector<tool_comp> &tcv ) {
        return tcv.empty();
    } ), ret.tools.end() );
    // Remove unrecoverable components
    ret.components.erase( std::remove_if( ret.components.begin(), ret.components.end(),
    []( std::vector<item_comp> &cov ) {
        cov.erase( std::remove_if( cov.begin(), cov.end(),
        []( const item_comp & comp ) {
            return !comp.recoverable || item( comp.type ).has_flag( "UNRECOVERABLE" );
        } ), cov.end() );
        return cov.empty();
    } ), ret.components.end() );

    return ret;
}

requirement_data requirement_data::continue_requirements( const std::vector<item_comp>
        &required_comps, const std::list<item> &remaining_comps )
{
    // Create an empty requirement_data
    requirement_data ret;

    // For items we cant change what alternative we selected half way through
    for( const item_comp &it : required_comps ) {
        ret.components.emplace_back( std::vector<item_comp>( {it} ) );
    }

    inventory craft_components;
    craft_components += remaining_comps;

    // Remove requirements that are completely fulfilled by current craft components
    // For each requirement that isn't completely fulfilled, reduce the requirement by the amount
    // that we still have
    // We also need to consume whatever charges we use in case two requirements share a common type
    ret.components.erase( std::remove_if( ret.components.begin(), ret.components.end(),
    [&craft_components]( std::vector<item_comp> &comps ) {
        item_comp &comp = comps.front();
        if( item::count_by_charges( comp.type ) && comp.count > 0 ) {
            int qty = craft_components.charges_of( comp.type, comp.count );
            comp.count -= qty;
            // This is terrible but inventory doesn't have a use_charges() function so...
            std::vector<item *> del;
            craft_components.visit_items( [&comp, &qty, &del]( item * e ) {
                std::list<item> used;
                if( e->use_charges( comp.type, qty, used, tripoint_zero ) ) {
                    del.push_back( e );
                }
                return qty > 0 ? VisitResponse::SKIP : VisitResponse::ABORT;
            } );
            craft_components.remove_items_with( [&del]( const item & e ) {
                for( const item *it : del ) {
                    if( it == &e ) {
                        return true;
                    }
                }
                return false;
            } );
        } else {
            int amount = craft_components.amount_of( comp.type, comp.count );
            comp.count -= amount;
            craft_components.use_amount( comp.type, amount );
        }
        return comp.count <= 0;
    } ), ret.components.end() );

    return ret;
}

void requirement_data::consolidate()
{
    std::map<quality_id, quality_requirement> all_quals;
    for( const std::vector<quality_requirement> &qual_vector : qualities ) {
        for( const quality_requirement &qual_data : qual_vector ) {
            if( all_quals.find( qual_data.type ) == all_quals.end() ) {
                all_quals[qual_data.type] = qual_data;
            } else {
                all_quals[qual_data.type].count = std::max( all_quals[qual_data.type].count,
                                                  qual_data.count );
                all_quals[qual_data.type].level = std::max( all_quals[qual_data.type].level,
                                                  qual_data.level );
            }
        }
    }
    qualities.clear();
    std::transform( all_quals.begin(), all_quals.end(), std::back_inserter( qualities ),
    []( auto & qual_data ) {
        return std::vector<quality_requirement>( { qual_data.second } );
    } );

    // elegance?  I've heard of it
    std::vector<std::vector<tool_comp>> all_tools;
    for( const std::vector<tool_comp> &old_tool_vector : tools ) {
        bool match = false;
        for( std::vector<tool_comp> &con_tool_vector : all_tools ) {
            size_t need_matches = con_tool_vector.size();
            size_t has_matches = 0;
            for( const tool_comp &old_tool : old_tool_vector ) {
                for( const tool_comp &con_tool : con_tool_vector ) {
                    if( old_tool.type == con_tool.type ) {
                        has_matches += 1;
                        break;
                    }
                }
            }
            if( has_matches == need_matches ) {
                match = true;
                for( const tool_comp &old_tool : old_tool_vector ) {
                    for( tool_comp &con_tool : con_tool_vector ) {
                        if( old_tool.type == con_tool.type ) {
                            con_tool.count += old_tool.count;
                            break;
                        }
                    }
                }
                break;
            }
        }
        if( !match ) {
            all_tools.emplace_back( old_tool_vector );
        }
    }
    tools = std::move( all_tools );

    std::vector<std::vector<item_comp>> all_comps;
    for( const std::vector<item_comp> &old_item_vector : components ) {
        bool match = false;
        for( auto &con_item_vector : all_comps ) {
            size_t need_matches = con_item_vector.size();
            size_t has_matches = 0;
            for( const item_comp &old_item : old_item_vector ) {
                for( const item_comp &con_item : con_item_vector ) {
                    if( old_item.type == con_item.type ) {
                        has_matches += 1;
                        break;
                    }
                }
            }
            if( has_matches == need_matches ) {
                match = true;
                for( const item_comp &old_item : old_item_vector ) {
                    for( item_comp &con_item : con_item_vector ) {
                        if( old_item.type == con_item.type ) {
                            con_item.count += old_item.count;
                            break;
                        }
                    }
                }
                break;
            }
        }
        if( !match ) {
            all_comps.emplace_back( old_item_vector );
        }
    }
    components = std::move( all_comps );
}
