#include "requirements.h"

#include <algorithm>
#include <climits>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <list>
#include <memory>
#include <set>
#include <stack>
#include <unordered_map>
#include <unordered_set>

#include "cata_assert.h"
#include "cata_utility.h"
#include "character.h"
#include "color.h"
#include "debug.h"
#include "debug_menu.h"
#include "enum_traits.h"
#include "generic_factory.h"
#include "inventory.h"
#include "item.h"
#include "item_factory.h"
#include "itype.h"
#include "json.h"
#include "localized_comparator.h"
#include "make_static.h"
#include "output.h"
#include "point.h"
#include "string_formatter.h"
#include "translations.h"
#include "value_ptr.h"
#include "visitable.h"

static const itype_id itype_UPS( "UPS" );
static const itype_id itype_char_forge( "char_forge" );
static const itype_id itype_crucible( "crucible" );
static const itype_id itype_fire( "fire" );
static const itype_id itype_forge( "forge" );
static const itype_id itype_mold_plastic( "mold_plastic" );
static const itype_id itype_oxy_torch( "oxy_torch" );
static const itype_id itype_press( "press" );
static const itype_id itype_sewing_kit( "sewing_kit" );
static const itype_id itype_welder( "welder" );
static const itype_id itype_welder_crude( "welder_crude" );

static const quality_id qual_CUT( "CUT" );
static const quality_id qual_GLARE( "GLARE" );
static const quality_id qual_KNIT( "KNIT" );
static const quality_id qual_PULL( "PULL" );
static const quality_id qual_SAW_M_FINE( "SAW_M_FINE" );
static const quality_id qual_SEW( "SEW" );

static const trait_id trait_DEBUG_HS( "DEBUG_HS" );

static std::map<requirement_id, requirement_data> requirements_all;

static bool a_satisfies_b( const quality_requirement &a, const quality_requirement &b );
static bool a_satisfies_b( const std::vector<quality_requirement> &a,
                           const std::vector<quality_requirement> &b );

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

std::vector<requirement_data> requirement_data::get_all()
{
    std::vector<requirement_data> ret;
    ret.reserve( requirements_all.size() );
    for( const std::pair<const requirement_id, requirement_data> &pair : requirements_all ) {
        ret.push_back( pair.second );
    }
    return ret;
}

namespace
{
generic_factory<quality> quality_factory( "tool quality" );
} // namespace

void quality::reset()
{
    quality_factory.reset();
}

void quality::load_static( const JsonObject &jo, const std::string &src )
{
    quality_factory.load( jo, src );
}

void quality::load( const JsonObject &jo, const std::string_view )
{
    mandatory( jo, was_loaded, "name", name );

    for( JsonArray levels : jo.get_array( "usages" ) ) {
        const int level = levels.get_int( 0 );
        for( const std::string line : levels.get_array( 1 ) ) {
            usages.emplace_back( level, line );
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
    return string_format( n_gettext( "%1$d tool with %2$s of %3$d or more.",
                                     "%1$d tools with %2$s of %3$d or more.", count ),
                          count, type.obj().name, level );
}

std::string quality_requirement::to_colored_string() const
{
    //~ %1$d: tool count, %2$s: quality requirement name, %3$d: quality level requirement
    return string_format( n_gettext( "%1$d tool with <info>%2$s of %3$d</info> or more",
                                     "%1$d tools with <info>%2$s of %3$d</info> or more", count ),
                          count, type.obj().name, level );
}

bool tool_comp::by_charges() const
{
    return count > 0;
}

std::string tool_comp::to_string( const int batch, const int ) const
{
    if( by_charges() ) {
        int charge_total = count * batch;
        //~ %1$s: tool name, %2$d: charge requirement
        return string_format( npgettext( "requirement", "%1$s (%2$d charge)", "%1$s (%2$d charges)",
                                         charge_total ),
                              item::nname( type ), charge_total );
    } else {
        return item::nname( type, std::abs( count ) );
    }
}

std::string item_comp::to_string( const int batch, const int avail ) const
{
    const int c = std::abs( count ) * batch;
    const itype *type_ptr = item::find_type( type );
    if( type_ptr->count_by_charges() ) {
        // Count-by-charge

        if( avail == item::INFINITE_CHARGES ) {
            //~ %1$s: item name, %2$d: charge requirement
            return string_format( npgettext( "requirement", "%2$d %1$s (have infinite)",
                                             "%2$d %1$s (have infinite)",
                                             c ),
                                  type_ptr->nname( 1 ), c );
        } else if( avail > 0 ) {
            //~ %1$s: item name, %2$d: charge requirement, %3%d: available charges
            return string_format( npgettext( "requirement", "%2$d %1$s (have %3$d)",
                                             "%2$d %1$s (have %3$d)", c ),
                                  type_ptr->nname( 1 ), c, avail );
        } else {
            //~ %1$s: item name, %2$d: charge requirement
            return string_format( npgettext( "requirement", "%2$d %1$s", "%2$d %1$s", c ),
                                  type_ptr->nname( 1 ), c );
        }
    } else {
        if( avail == item::INFINITE_CHARGES ) {
            //~ %1$s: item name, %2$d: required count
            return string_format( npgettext( "requirement", "%2$d %1$s (have infinite)",
                                             "%2$d %1$s (have infinite)",
                                             c ),
                                  type_ptr->nname( c ), c );
        } else if( avail > 0 ) {
            //~ %1$s: item name, %2$d: required count, %3%d: available count
            return string_format( npgettext( "requirement", "%2$d %1$s (have %3$d)",
                                             "%2$d %1$s (have %3$d)", c ),
                                  type_ptr->nname( c ), c, avail );
        } else {
            //~ %1$s: item name, %2$d: required count
            return string_format( npgettext( "requirement", "%2$d %1$s", "%2$d %1$s", c ),
                                  type_ptr->nname( c ), c );
        }
    }
}

void quality_requirement::load( const JsonValue &value )
{
    const JsonObject quality_data = value.get_object();
    type = quality_id( quality_data.get_string( "id" ) );
    level = quality_data.get_int( "level", 1 );
    count = quality_data.get_int( "amount", 1 );
    if( count <= 0 ) {
        quality_data.throw_error_at( "amount", "quality amount must be a positive number" );
    }
    // Note: level is not checked, negative values and 0 are allow, see butchering quality.
}

void quality_requirement::dump( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "id", type );
    if( level != 1 ) {
        jsout.member( "level", level );
    }
    if( count != 1 ) {
        jsout.member( "amount", count );
    }
    jsout.end_object();
}

void tool_comp::load( const JsonValue &value )
{
    if( value.test_string() ) {
        // constructions uses this format: [ "tool", ... ]
        value.read( type, true );
        count = -1;
    } else {
        JsonArray comp = value.get_array();
        comp.read( 0, type, true );
        count = comp.get_int( 1 );
        requirement = comp.size() > 2 && comp.get_string( 2 ) == "LIST";
    }
    if( count == 0 ) {
        value.throw_error( "tool count must not be 0" );
    }
    // Note: negative count means charges (of the tool) should be consumed
}

void tool_comp::dump( JsonOut &jsout ) const
{
    jsout.start_array();
    jsout.write( type );
    jsout.write( count );
    if( requirement ) {
        jsout.write( "LIST" );
    }
    jsout.end_array();
}

void item_comp::load( const JsonValue &value )
{
    JsonArray comp = value.get_array();
    comp.read( 0, type, true );
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
        value.throw_error( "item count must be a positive number" );
    }
}

void item_comp::dump( JsonOut &jsout ) const
{
    jsout.start_array();
    jsout.write( type );
    jsout.write( count );
    if( !recoverable ) {
        jsout.write( "NO_RECOVER" );
    }
    if( requirement ) {
        jsout.write( "LIST" );
    }
    jsout.end_array();
}

template<typename T>
void requirement_data::load_obj_list( const JsonArray &jsarr, std::vector< std::vector<T> > &objs )
{
    for( const JsonValue entry : jsarr ) {
        if( entry.test_array() ) {
            std::vector<T> choices;
            for( const JsonValue subentry : entry.get_array() ) {
                choices.push_back( T() );
                choices.back().load( subentry );
            }
            if( !choices.empty() ) {
                objs.push_back( choices );
            }
        } else {
            // tool qualities don't normally use a list of alternatives
            // each quality is mandatory.
            objs.push_back( std::vector<T>( 1 ) );
            objs.back().back().load( entry );
        }
    }
}

requirement_data requirement_data::operator*( unsigned scalar ) const
{
    requirement_data res = *this;
    for( auto &group : res.components ) {
        for( item_comp &e : group ) {
            e.count = std::max( e.count * static_cast<int>( scalar ), -1 );
        }
    }
    for( auto &group : res.tools ) {
        for( tool_comp &e : group ) {
            e.count = std::max( e.count * static_cast<int>( scalar ), -1 );
        }
    }

    return res;
}

static bool a_satisfies_b( const quality_requirement &a, const quality_requirement &b )
{
    return a.type == b.type && a.level >= b.level
           && a.requirement == b.requirement && a.count == b.count;
}

static bool a_satisfies_b( const std::vector<quality_requirement> &a,
                           const std::vector<quality_requirement> &b )
{
    // every b_x is satisfied by some a_x
    for( const quality_requirement &b_x : b ) {
        bool satisfied = false;
        for( const quality_requirement &a_x : a ) {
            if( a_satisfies_b( a_x, b_x ) ) {
                satisfied = true;
                break;
            }
        }
        if( !satisfied ) {
            return false;
        }
    }

    return true;
}

requirement_data requirement_data::operator+( const requirement_data &rhs ) const
{
    requirement_data res = *this;

    res.components.insert( res.components.end(), rhs.components.begin(), rhs.components.end() );
    res.tools.insert( res.tools.end(), rhs.tools.begin(), rhs.tools.end() );
    /*
    TODO: what is / isn't implemented:
    I will represent type by letter and level by number, always true: count = 1 && requirement = false
    A1 then has type A with level 1, (count = 1, requirement = false)
    X) [implemented ? "x" else " "] What's required -> How should it be displayed
    1) [x] A1 && A1 -> A1
    2) [x] A1 && A2 -> A2
    3) [x] (A1 || B1) && A1 -> A1
    4) [ ] A1 || A2 -> A1
    5) [ ] (A1 || B1) && A2 -> A2
    6) [ ] (A1 || B1 || C1) || (A1 || B1) -> A1 || B1 || C1
    Note: (1) covers most cases, (2) probably the rest, (3..6) probably isn't anywhere?

    It all takes O(n^2), but that's acceptable since n is very small.
    @Brambor
    */
    for( const std::vector<quality_requirement> &new_quality : rhs.qualities ) {
        bool add = true;
        for( std::vector<quality_requirement> &old_quality : res.qualities ) {
            if( a_satisfies_b( new_quality, old_quality ) ) {
                add = false;
                old_quality = new_quality;
                break;
            } else if( a_satisfies_b( old_quality, new_quality ) ) {
                add = false;
                break;
            }
        }
        if( add ) {
            res.qualities.emplace_back( new_quality );
        }
    }

    // combined result is temporary which caller could store via @ref save_requirement
    res.id_ = requirement_id::NULL_ID();

    // TODO: combine other requirements

    // if either operand was blacklisted then their summation should also be
    res.blacklisted |= rhs.blacklisted;

    return res;
}

requirement_data requirement_data::operator+(
    const std::pair<const requirement_id, int> &rhs ) const
{
    return *this + *rhs.first * rhs.second;
}

requirement_data requirement_data::operator+( const std::pair<requirement_id, int> &rhs ) const
{
    return *this + *rhs.first * rhs.second;
}

void requirement_data::load_requirement( const JsonObject &jsobj, const requirement_id &id,
        const bool check_extend )
{
    requirement_data req;
    requirement_data ext;

    if( check_extend && jsobj.has_object( "extend" ) ) {
        JsonObject jext = jsobj.get_object( "extend" );
        if( jext.has_member( "components" ) ) {
            load_obj_list( jext.get_array( "components" ), ext.components );
        }
        if( jext.has_member( "qualities" ) ) {
            load_obj_list( jext.get_array( "qualities" ), ext.qualities );
        }
        if( jext.has_member( "tools" ) ) {
            load_obj_list( jext.get_array( "tools" ), ext.tools );
        }
    }

    if( ext.components.empty() || jsobj.has_member( "components" ) ) {
        load_obj_list( jsobj.get_array( "components" ), req.components );
    }
    if( ext.qualities.empty() || jsobj.has_member( "qualities" ) ) {
        load_obj_list( jsobj.get_array( "qualities" ), req.qualities );
    }
    if( ext.tools.empty() || jsobj.has_member( "tools" ) ) {
        load_obj_list( jsobj.get_array( "tools" ), req.tools );
    }

    if( !id.is_null() ) {
        req.id_ = id;
    } else if( jsobj.has_string( "id" ) ) {
        req.id_ = requirement_id( jsobj.get_string( "id" ) );
    } else {
        jsobj.throw_error( "id was not specified for requirement" );
    }

    save_requirement( req, string_id<requirement_data>::NULL_ID(), &ext );
}

void requirement_data::save_requirement( const requirement_data &req, const requirement_id &id,
        const requirement_data *extend )
{
    requirement_data dup = req;
    if( !id.is_null() ) {
        dup.id_ = id;
    }

    if( requirements_all.count( dup.id_ ) == 0 ) {
        requirements_all[ dup.id_ ] = dup;
    }

    requirement_data &r = requirements_all[ dup.id_ ];
    if( !dup.components.empty() ) {
        r.components.clear();
        r.components.insert( r.components.end(), dup.components.begin(), dup.components.end() );
    }
    if( !dup.tools.empty() ) {
        r.tools.clear();
        r.tools.insert( r.tools.end(), dup.tools.begin(), dup.tools.end() );
    }
    if( !dup.qualities.empty() ) {
        r.qualities.clear();
        r.qualities.insert( r.qualities.end(), dup.qualities.begin(), dup.qualities.end() );
    }

    if( !!extend ) {
        for( unsigned i = 0; i < r.components.size() && i < extend->components.size(); i++ ) {
            r.components[i].insert( r.components[i].end(), extend->components[i].begin(),
                                    extend->components[i].end() );
        }
        for( unsigned i = 0; i < r.tools.size() && i < extend->tools.size(); i++ ) {
            r.tools[i].insert( r.tools[i].end(), extend->tools[i].begin(), extend->tools[i].end() );
        }
        for( unsigned i = 0; i < r.qualities.size() && i < extend->qualities.size(); i++ ) {
            r.qualities[i].insert( r.qualities[i].end(), extend->qualities[i].begin(),
                                   extend->qualities[i].end() );
        }
    }
}

template<typename T>
bool requirement_data::any_marked_available( const std::vector<T> &comps )
{
    for( const auto &comp : comps ) {
        if( comp.available == available_status::a_true ) {
            return true;
        }
    }
    return false;
}

template<typename T>
std::string requirement_data::print_all_objs( const std::string &header,
        const std::vector< std::vector<T> > &objs )
{
    std::string buffer;
    for( const auto &list : objs ) {
        if( !buffer.empty() ) {
            buffer += std::string( "\n" ) + _( "and " );
        }
        std::vector<std::string> alternatives;
        std::transform( list.begin(), list.end(), std::back_inserter( alternatives ),
        []( const T & t ) {
            return t.to_string();
        } );
        std::sort( alternatives.begin(), alternatives.end(), localized_compare );
        buffer += string_join( alternatives, _( " or " ) );
    }
    if( buffer.empty() ) {
        return std::string();
    }
    return header + "\n" + buffer + "\n";
}

std::string requirement_data::list_all() const
{
    std::string buffer;
    buffer += print_all_objs( _( "These tools are required:" ), tools );
    buffer += print_all_objs( _( "These tools are required:" ), qualities );
    buffer += print_all_objs( _( "These components are required:" ), components );
    return buffer;
}

template<typename T>
std::string requirement_data::print_missing_objs( const std::string &header,
        const std::vector< std::vector<T> > &objs )
{
    std::string separator_and = _( "and " );
    std::string separator_or = _( " or " );
    std::string buffer;
    for( const auto &list : objs ) {
        if( any_marked_available( list ) ) {
            continue;
        }
        if( !buffer.empty() ) {
            buffer += std::string( "\n" ) + separator_and;
        }
        for( auto it = list.begin(); it != list.end(); ++it ) {
            if( it != list.begin() ) {
                buffer += separator_or;
            }
            buffer += it->to_string();
        }
    }
    if( buffer.empty() ) {
        return std::string();
    }
    return header + "\n" + buffer + "\n";
}

std::string requirement_data::list_missing() const
{
    std::string buffer;
    buffer += print_missing_objs( _( "These tools are missing:" ), tools );
    buffer += print_missing_objs( _( "These tools are missing:" ), qualities );
    buffer += print_missing_objs( _( "These components are missing:" ), components );
    return buffer;
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

template <typename T>
void inline_requirements( std::vector<std::vector<T>> &list,
                          const std::function<const std::vector<std::vector<T>> & ( const requirement_data & )> &getter )
{
    // add a single component to the vector. If component already exists, chooses min count
    const auto add_component = []( const T & comp, std::vector<T> &accum ) {
        auto iter = std::find_if( accum.begin(), accum.end(), [&]( const T & req ) {
            return !req.requirement && req.type == comp.type;
        } );
        if( iter == accum.end() ) {
            accum.push_back( comp ); // component doesn't exist yet, adding it
        } else {
            // this is the list of alternatives (a OR b OR c), so if there are two identical components
            // with the different counts as an alternatives, the one with the lowest count is selected
            iter->count = std::min( comp.count, iter->count );
        }
    };

    // if `comp` is a component, adds it to the `accum`
    // if it's a requirement, recursively expands it into simple components and adds them to the vector
    const auto inline_one = [&]( const T & comp, std::vector<T> &accum ) {
        std::set<requirement_id> already_nested;
        std::list<requirement_id> stack;

        std::function<void( const T &comp )> rec;
        rec = [&]( const T & comp ) {
            // add simple component to the vector
            if( !comp.requirement ) {
                add_component( comp, accum );
                return;
            }
            // otherwise expand component as requirement
            const requirement_id r( comp.type.str() );
            if( !r.is_valid() ) {
                debugmsg( "Tried to inline unknown requirement %s", r.c_str() );
                return;
            }
            // stack just holds the current path of inlining for debug purposes
            stack.push_back( r );
            if( already_nested.count( r ) ) {
                // print debug msg and skip just this one requirement
                debugmsg( "Tried to inline requirement %s which forms a cycle: %s",
                r.c_str(),  debug_menu::iterable_to_string( stack, " -> ", []( const requirement_id & r ) {
                    return r.str();
                } ) );
                stack.pop_back();
                return;
            }
            already_nested.insert( r );

            const requirement_data &req = r.obj();
            const requirement_data multiplied = req * comp.count;

            const std::vector<std::vector<T>> &to_inline = getter( multiplied );
            if( !to_inline.empty() ) {
                const std::vector<T> &nested = to_inline.front();
                for( const T &nested_comp : nested ) {
                    rec( nested_comp );
                }
            }
            already_nested.erase( r );
            stack.pop_back();
        };

        rec( comp );
    };

    for( std::vector<T> &vec : list ) {
        const std::vector<T> vec_copy = vec;
        vec.clear();
        for( const T &comp  : vec_copy ) {
            inline_one( comp, vec );
        }
    }
}

void requirement_data::finalize()
{
    for( auto &r : const_cast<std::map<requirement_id, requirement_data> &>( all() ) ) {
        inline_requirements<tool_comp>( r.second.tools, []( const requirement_data & d ) -> const auto & {
            return d.get_tools();
        } );
        inline_requirements<item_comp>( r.second.components,
        []( const requirement_data & d ) -> const auto & {
            return d.get_components();
        } );
        requirement_data::alter_tool_comp_vector &vec = r.second.tools;
        for( auto &list : vec ) {
            std::vector<tool_comp> new_list;
            for( tool_comp &comp : list ) {
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
        const read_only_visitable &crafting_inv, const std::function<bool( const item & )> &filter,
        int batch,
        const std::string_view hilite, requirement_display_flags flags ) const
{
    std::vector<std::string> out_buffer;
    if( components.empty() ) {
        return out_buffer;
    }
    out_buffer.push_back( colorize( _( "Components required:" ), col ) );

    std::vector<std::string> folded_buffer =
        get_folded_list( width, crafting_inv, filter, components, batch, hilite, flags );
    out_buffer.insert( out_buffer.end(), folded_buffer.begin(), folded_buffer.end() );

    return out_buffer;
}

template<typename T>
std::vector<std::string> requirement_data::get_folded_list( int width,
        const read_only_visitable &crafting_inv, const std::function<bool( const item & )> &filter,
        const std::vector< std::vector<T> > &objs, int batch, const std::string_view hilite,
        requirement_display_flags flags ) const
{
    // hack: ensure 'cached' availability is up to date
    can_make_with_inventory( crafting_inv, filter );

    const bool no_unavailable =
        static_cast<bool>( flags & requirement_display_flags::no_unavailable );

    std::vector<std::string> out_buffer;
    for( const auto &comp_list : objs ) {
        const bool has_one = any_marked_available( comp_list );
        std::vector<std::string> list_as_string;
        std::vector<std::string> list_as_string_unavailable;
        std::vector<std::string> buffer_has;
        for( const T &component : comp_list ) {
            nc_color color = component.get_color( has_one, crafting_inv, filter, batch );
            const std::string color_tag = get_tag_from_color( color );
            int qty = 0;
            if( component.get_component_type() == component_type::ITEM ) {
                const itype_id item_id = itype_id( component.type.str() );
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

            if( component.has( crafting_inv, filter, batch ) ) {
                list_as_string.push_back( colorize( text, color ) );
            } else if( !no_unavailable ) {
                list_as_string_unavailable.push_back( colorize( text, color ) );
            }
            buffer_has.push_back( text + color_tag );
        }

        std::sort( list_as_string.begin(), list_as_string.end(), localized_compare );
        std::sort( list_as_string_unavailable.begin(), list_as_string_unavailable.end(),
                   localized_compare );
        list_as_string.insert( list_as_string.end(), list_as_string_unavailable.begin(),
                               list_as_string_unavailable.end() );

        const std::string separator = colorize( _( " OR " ), c_white );
        const std::string unfolded = string_join( list_as_string, separator );
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
        const read_only_visitable &crafting_inv, int batch ) const
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

bool requirement_data::can_make_with_inventory( const read_only_visitable &crafting_inv,
        const std::function<bool( const item & )> &filter, int batch, craft_flags flags ) const
{
    if( get_player_character().has_trait( trait_DEBUG_HS ) ) {
        return true;
    }

    bool retval = true;
    // All functions must be called to update the available settings in the components.
    if( !has_comps( crafting_inv, qualities, return_true<item> ) ) {
        retval = false;
    }
    if( !has_comps( crafting_inv, tools, return_true<item>, batch, flags ) ) {
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
bool requirement_data::has_comps( const read_only_visitable &crafting_inv,
                                  const std::vector< std::vector<T> > &vec,
                                  const std::function<bool( const item & )> &filter,
                                  int batch, craft_flags flags )
{
    bool retval = true;
    int total_UPS_charges_used = 0;
    for( const std::vector<T> &set_of_tools : vec ) {
        bool has_tool_in_set = false;
        int UPS_charges_used = std::numeric_limits<int>::max();
        const std::function<void( int )> use_ups = [ &UPS_charges_used ]( int charges ) {
            UPS_charges_used = std::min( UPS_charges_used, charges );
        };

        for( const T &tool : set_of_tools ) {
            if( tool.has( crafting_inv, filter, batch, flags, use_ups ) ) {
                tool.available = available_status::a_true;
            } else {
                // Trying to track down why the crafting tests are failing?
                // Uncomment the below to see the group of requirements that are lacking satisfaction
                // Add a printf("\n") to the loop above this to separate different groups onto a separate line
                // printf( "T: %s ", tool.type.str().c_str() );
                tool.available = available_status::a_false;
            }
            has_tool_in_set = has_tool_in_set || tool.available == available_status::a_true;
        }
        if( !has_tool_in_set ) {
            retval = false;
        }
        if( UPS_charges_used != std::numeric_limits<int>::max() ) {
            total_UPS_charges_used += UPS_charges_used;
        }
    }

    if( total_UPS_charges_used > 0 &&
        total_UPS_charges_used > crafting_inv.charges_of( itype_UPS ) ) {
        return false;
    }
    return retval;
}

bool quality_requirement::has(
    const read_only_visitable &crafting_inv, const std::function<bool( const item & )> &, int,
    craft_flags, const std::function<void( int )> & ) const
{
    if( get_player_character().has_trait( trait_DEBUG_HS ) ) {
        return true;
    }
    return crafting_inv.has_quality( type, level, count );
}

nc_color quality_requirement::get_color( bool has_one, const read_only_visitable &,
        const std::function<bool( const item & )> &, int ) const
{
    if( get_player_character().has_trait( trait_DEBUG_HS ) ||
        available == available_status::a_true ) {
        return c_green;
    }
    return has_one ? c_dark_gray : c_red;
}

bool tool_comp::has(
    const read_only_visitable &crafting_inv, const std::function<bool( const item & )> &filter,
    int batch,
    craft_flags flags, const std::function<void( int )> &visitor ) const
{
    if( get_player_character().has_trait( trait_DEBUG_HS ) ) {
        return true;
    }
    if( !by_charges() ) {
        return crafting_inv.has_tools( type, std::abs( count ), filter );
    } else {
        int charges_required = count * batch * item::find_type( type )->charge_factor();

        // The `type->tool` check excludes items counted by charge used as tools,
        // such as water purification tablets.
        if( ( flags & craft_flags::start_only ) != craft_flags::none && type->tool ) {
            // See Character::craft_consume_tools. In theory only
            // `charges_required / 20 + charges_required % 20` charges are
            // consumed during the first 5% progress, however that equation
            // sometimes decreases when the batch size increases, so we take
            // the largest remainder value 19 to make this function return
            // false consistently for large batch sizes.
            charges_required = std::min( charges_required, charges_required / 20 + 19 );
        }

        int charges_found = crafting_inv.charges_of( type, charges_required, filter, visitor );
        return charges_found == charges_required;
    }
}

nc_color tool_comp::get_color( bool has_one, const read_only_visitable &crafting_inv,
                               const std::function<bool( const item & )> &filter, int batch ) const
{
    if( available == available_status::a_insufficient ) {
        return c_brown;
    } else if( has( crafting_inv, filter, batch ) ) {
        return c_green;
    }
    return has_one ? c_dark_gray : c_red;
}

bool item_comp::has(
    const read_only_visitable &crafting_inv, const std::function<bool( const item & )> &filter,
    int batch,
    craft_flags, const std::function<void( int )> & ) const
{
    if( get_player_character().has_trait( trait_DEBUG_HS ) ) {
        return true;
    }
    const int cnt = std::abs( count ) * batch;
    if( item::count_by_charges( type ) ) {
        return crafting_inv.has_charges( type, cnt, filter );
    } else {
        return crafting_inv.has_components( type, cnt, filter );
    }
}

nc_color item_comp::get_color( bool has_one, const read_only_visitable &crafting_inv,
                               const std::function<bool( const item & )> &filter, int batch ) const
{
    if( available == available_status::a_insufficient ) {
        return c_brown;
    } else if( has( crafting_inv, filter, batch ) ) {
        const inventory *inv = static_cast<const inventory *>( &crafting_inv );
        // Will use non-empty liquid container
        if( std::any_of( type->pockets.begin(), type->pockets.end(), []( const pocket_data & d ) {
        return d.type == pocket_type::CONTAINER && d.watertight;
    } ) && inv != nullptr && inv->must_use_liq_container( type, count * batch ) ) {
            return c_magenta;
        }
        if( inv != nullptr && inv->must_use_hallu_poison( type, count * batch ) ) {
            return c_magenta;
        }
        // Will use favorited component
        if( !has( crafting_inv, [&filter]( const item & it ) {
        return filter( it ) && !it.is_favorite;
        }, batch ) ) {
            return c_pink;
        }
        // Component is OK
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

bool requirement_data::check_enough_materials( const read_only_visitable &crafting_inv,
        const std::function<bool( const item & )> &filter, int batch ) const
{
    bool retval = true;
    for( const auto &component_choices : components ) {
        bool atleast_one_available = false;
        for( const item_comp &comp : component_choices ) {
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

bool requirement_data::check_enough_materials( const item_comp &comp,
        const read_only_visitable &crafting_inv,
        const std::function<bool( const item & )> &filter, int batch ) const
{
    if( comp.available != available_status::a_true ) {
        return false;
    }
    const int cnt = std::abs( comp.count ) * batch;
    const tool_comp *tq = find_by_type( tools, comp.type );
    if( tq != nullptr && tq->available == available_status::a_true ) {
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
            comp.available = available_status::a_insufficient;
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
        if( !crafting_inv.has_quality( qr->type, qr->level, qr->count + std::abs( comp.count ) ) ) {
            comp.available = available_status::a_insufficient;
        }
    }
    return comp.available == available_status::a_true;
}

template <typename T>
static bool apply_blacklist( std::vector<std::vector<T>> &vec, const itype_id &id )
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

void requirement_data::blacklist_item( const itype_id &id )
{
    blacklisted |= apply_blacklist( tools, id );
    blacklisted |= apply_blacklist( components, id );
}

template <typename T>
static void apply_replacements( std::vector<std::vector<T>> &vec,
                                const std::unordered_map<itype_id, itype_id> &replacements )
{
    for( std::vector<T> &opts : vec ) {
        for( typename std::vector<T>::iterator iiter = opts.begin(); iiter != opts.end(); ) {
            auto riter = replacements.find( iiter->type );
            if( riter != replacements.end() ) {
                itype_id to_id = riter->second;
                // Replace an item or outright remove it if its replacement
                // is already present in the vector, to prevent duplicates
                bool has_duplicates = std::count_if( opts.cbegin(), opts.cend(), [&to_id]( const T & item ) {
                    return item.type == to_id;
                } ) != 0;
                if( has_duplicates ) {
                    iiter = opts.erase( iiter );
                    continue;
                } else {
                    iiter->type = to_id;
                }
            }
            ++iiter;
        }
    }
}

void requirement_data::replace_items( const std::unordered_map<itype_id, itype_id> &replacements )
{
    apply_replacements( tools, replacements );
    apply_replacements( components, replacements );
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
        for( const tool_comp &tool : it ) {
            const itype_id &type = tool.type;

            // If crafting required a welder or forge then disassembly requires metal sawing
            if( type == itype_welder || type == itype_welder_crude || type == itype_oxy_torch ||
                type == itype_forge || type == itype_char_forge ) {
                new_qualities.emplace_back( qual_SAW_M_FINE, 1, 1 );
                replaced = true;
                break;
            }
            //This only catches instances where the two tools are explicitly stated, and not just the required sewing quality
            if( type == itype_sewing_kit ||
                type == itype_mold_plastic ) {
                new_qualities.emplace_back( qual_CUT, 1, 1 );
                replaced = true;
                break;
            }

            if( type == itype_crucible ) {
                replaced = true;
                break;
            }
            //This ensures that you don't need a hand press to break down reloaded ammo.
            if( type == itype_press ) {
                replaced = true;
                remove_fire = true;
                new_qualities.emplace_back( qual_PULL, 1, 1 );
                break;
            }
            if( type == itype_fire && remove_fire ) {
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
            for( const quality_requirement &quality : it ) {
                if( quality.type == qual_SEW ) {
                    replaced = true;
                    new_qualities.emplace_back( qual_CUT, 1, quality.level );
                    break;
                }
                if( quality.type == qual_GLARE ) {
                    replaced = true;
                    //Just remove the glare protection requirement from deconstruction
                    //This only happens in case of a reversible recipe, an explicit
                    //deconstruction recipe can still specify glare protection
                    break;
                }
                if( quality.type == qual_KNIT ) {
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
            return !comp.recoverable || item( comp.type ).has_flag( STATIC( flag_id( "UNRECOVERABLE" ) ) );
        } ), cov.end() );
        return cov.empty();
    } ), ret.components.end() );

    return ret;
}

requirement_data requirement_data::continue_requirements( const std::vector<item_comp>
        &required_comps, const item_components &remaining_comps )
{
    // Create an empty requirement_data
    requirement_data ret;

    // For items we can't change what alternative we selected half way through
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
            craft_components.visit_items( [&comp, &qty, &del]( item * e, item * ) {
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

template<typename T, typename Accum>
static std::vector<std::vector<T>> consolidate( std::vector<std::vector<T>> old_vec,
                                const Accum &accum )
{
    const auto type_lt = []( const T & lhs, const T & rhs ) -> bool {
        //TODO change to use localized sorting
        return std::forward_as_tuple( lhs.type.str(), lhs.requirement )
        < std::forward_as_tuple( rhs.type.str(), rhs.requirement );
    };
    // in order to simplify blueprint requirements, we merge a longer requirement
    // list into a shorter requirement list whose types are a subsequence of the
    // longer list's types. However, this operation is not symmetric and depends
    // on the order of the requirement lists. Thus we sort the lists first, to
    // ensure consistent results when the order of construction requirements changes.
    for( std::vector<T> &old_inner : old_vec ) {
        std::sort( old_inner.begin(), old_inner.end(), type_lt );
    }
    std::sort( old_vec.begin(), old_vec.end(),
    [&type_lt]( const std::vector<T> &lhs, const std::vector<T> &rhs ) -> bool {
        return std::lexicographical_compare( lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
                                             type_lt );
    } );

    std::vector<std::vector<T>> new_vec;
    for( std::vector<T> &old_inner : old_vec ) {
        bool match = false;
        for( std::vector<T> &new_inner : new_vec ) {
            // in order to simplify blueprint requirements, we merge a longer
            // requirement list into a shorter requirement list whose types are
            // a subsequence of the longer list's types.
            //
            // note this actually may make a requirement stricter.
            // for example, if the item requirement was
            //   [ [ [ "a", 1 ], [ "b", 1 ], [ "c", 1 ] ],
            //     [ [ "a", 1 ], [ "b", 1 ] ] ]
            // then you could satisfy it by having one "a" and one "b", one
            // "c" and one "a", two "a", or two "b", etc.
            //
            // but after consolidation, it becomes
            //   [ [ [ "a", 2 ], [ "b", 2 ] ] ]
            // then you can only satisfy it by having either 2 "a" or 2 "b"
            if( std::includes( new_inner.begin(), new_inner.end(),
                               old_inner.begin(), old_inner.end(),
                               type_lt ) ) {
                // old_inner is a subsequence of new_inner
                match = true;
                std::swap( old_inner, new_inner );
            } else if( std::includes( old_inner.begin(), old_inner.end(),
                                      new_inner.begin(), new_inner.end(),
                                      type_lt ) ) {
                // new_inner is a subsequence of old_inner
                match = true;
            }
            if( match ) {
                for( auto it1 = new_inner.begin(), it2 = old_inner.begin();
                     it1 < new_inner.end(); ++it2 ) {
                    if( !type_lt( *it2, *it1 ) ) {
                        // which means *it2 and *it1 have the same type, since
                        // we know new_inner is a subsequence of old_inner
                        *it1 = accum( *it1, *it2 );
                        ++it1;
                    }
                }
                break;
            }
        }
        if( !match ) {
            new_vec.emplace_back( old_inner );
        }
    }
    return new_vec;
}

void requirement_data::consolidate()
{
    qualities = ::consolidate( qualities,
    []( const quality_requirement & lhs, const quality_requirement & rhs ) {
        quality_requirement ret = lhs;
        ret.count = std::max( ret.count, rhs.count );
        ret.level = std::max( ret.level, rhs.level );
        return ret;
    } );

    tools = ::consolidate( tools,
    []( const tool_comp & lhs, const tool_comp & rhs ) {
        tool_comp ret = lhs;
        if( ret.count < 0 && rhs.count < 0 ) {
            ret.count = std::min( ret.count, rhs.count );
        } else if( ret.count > 0 && rhs.count > 0 ) {
            ret.count += rhs.count;
        } else {
            debugmsg( "required counts of the same tool have different signs" );
        }
        return ret;
    } );

    components = ::consolidate( components,
    []( const item_comp & lhs, const item_comp & rhs ) {
        item_comp ret = lhs;
        ret.count += rhs.count;
        return ret;
    } );
}

template<typename T>
static bool sorted_equal( std::vector<std::vector<T>> lhs, std::vector<std::vector<T>> rhs )
{
    if( lhs.size() != rhs.size() ) {
        return false;
    }
    for( auto &inner : lhs ) {
        std::sort( inner.begin(), inner.end() );
    }
    for( auto &inner : rhs ) {
        std::sort( inner.begin(), inner.end() );
    }
    std::sort( lhs.begin(), lhs.end() );
    std::sort( rhs.begin(), rhs.end() );
    return lhs == rhs;
}

bool requirement_data::has_same_requirements_as( const requirement_data &that ) const
{
    return sorted_equal( tools, that.tools ) && sorted_equal( qualities, that.qualities )
           && sorted_equal( components, that.components );
}

template<typename T>
static void dump_req_vec( const std::vector<std::vector<T>> &vec, JsonOut &jsout )
{
    jsout.start_array( /*wrap=*/!vec.empty() );
    for( const auto &inner : vec ) {
        jsout.start_array();
        for( const auto &val : inner ) {
            val.dump( jsout );
        }
        jsout.end_array();
    }
    jsout.end_array();
}

void requirement_data::dump( JsonOut &jsout ) const
{
    jsout.start_object( /*wrap=*/true );

    jsout.member( "tools" );
    dump_req_vec( tools, jsout );

    jsout.member( "qualities" );
    dump_req_vec( qualities, jsout );

    jsout.member( "components" );
    dump_req_vec( components, jsout );

    jsout.end_object();
}

uint64_t requirement_data::make_hash() const
{
    std::ostringstream stream;
    JsonOut json( stream );
    dump( json );
    std::hash<std::string> hasher;
    return hasher( stream.str() );
}

/// Helper function for deduped_requirement_data constructor below.
///
/// The goal of this function is to consolidate a particular item_comp that
/// would otherwise be duplicated between two requirements.
///
/// It operates recursively (increasing @p index with the depth of recursion),
/// searching for another item_comp to merge @p leftover with.  For each
/// compatible item_comp found it performs that merger and writes out a
/// suitably updated form of the overall requirements to @p result.
///
/// If it chooses *not* to merge with any particular item_comp, then it deletes
/// that item_comp from the options, to avoid the duplication.
///
/// Lastly, it also outputs a version of the requirements where @p leftover
/// remains where it was, and all other compatible item_comp entries have been
/// deleted.
///
/// @param leftover The item_comp needing to be dealt with.
/// @param req_prefix The requirements considered so far; more will be appended
/// to this.
/// @param to_expand The original requirements we are working through to look
/// for a duplicate.
/// @param orig_index The index into the alter_item_comp_vector where @p
/// leftover was originally to be found.  If it isn't merged with another item,
/// then it will be re-inserted at this position.
/// @param index The position within @p to_expand where we will next look for
/// duplicates of @p leftover to merge with.
/// @param result The finished requirements should be appended to this.
static void expand_item_in_reqs(
    const item_comp &leftover, requirement_data::alter_item_comp_vector req_prefix,
    const requirement_data::alter_item_comp_vector &to_expand, size_t orig_index, size_t index,
    std::vector<requirement_data::alter_item_comp_vector> &result )
{
    cata_assert( req_prefix.size() >= orig_index );
    cata_assert( orig_index < index );

    if( index == to_expand.size() ) {
        // We reached the end without using the leftovers.  So need to add them
        // as their own requirement, separate from everything else.
        req_prefix.insert( req_prefix.begin() + orig_index, { leftover } );
        result.push_back( req_prefix );
        return;
    }

    std::vector<item_comp> this_requirement = to_expand[index];
    auto duplicate = std::find_if( this_requirement.begin(), this_requirement.end(),
    [&]( const item_comp & c ) {
        return c.type == leftover.type;
    } );
    if( duplicate == this_requirement.end() ) {
        // No match in this one; proceed to next
        req_prefix.push_back( this_requirement );
        expand_item_in_reqs( leftover, req_prefix, to_expand, orig_index, index + 1, result );
        return;
    }
    // First option: amalgamate the leftovers into this requirement, which
    // forces us to pick that specific option:
    requirement_data::alter_item_comp_vector req = req_prefix;
    req.push_back( { item_comp( leftover.type, leftover.count + duplicate->count ) } );
    req.insert( req.end(), to_expand.begin() + index + 1, to_expand.end() );
    result.push_back( req );

    // Second option: use a separate option for this requirement, which means
    // we need to recurse further to find something into which to amalgamate
    // the original requirement
    this_requirement.erase( duplicate );
    if( !this_requirement.empty() ) {
        req_prefix.push_back( this_requirement );
        expand_item_in_reqs( leftover, req_prefix, to_expand, orig_index, index + 1, result );
    }
}

deduped_requirement_data::deduped_requirement_data( const requirement_data &in,
        const recipe_id &context )
{
    // This constructor works through a requirement_data, converting it into an
    // equivalent set of requirement_data alternatives, where each alternative
    // has the property that no item type appears more than once.
    //
    // We maintain a queue of requirement_data component info to be split.
    // Each to_check struct has a vector of component requirements, and an
    // index.  The index is the position within the vector to be checked next.
    struct to_check {
        alter_item_comp_vector components;
        size_t index;
    };
    std::stack<to_check, std::vector<to_check>> pending;
    pending.push( { in.get_components(), 0 } );

    // Make sure the tools are not duplicated.
    requirement_data::alter_tool_comp_vector tools_unique;
    for( auto tools : in.get_tools() ) {
        std::sort( tools.begin(), tools.end() );
        tools.erase( std::unique( tools.begin(), tools.end() ), tools.end() );
        tools_unique.push_back( tools );
    }

    while( !pending.empty() ) {
        to_check next = pending.top();
        pending.pop();

        if( next.index == next.components.size() ) {
            alternatives_.emplace_back( tools_unique, in.get_qualities(), next.components );
            continue;
        }

        // Build a set of all the itypes used in later stages of this set of
        // requirements.
        std::unordered_set<itype_id> later_itypes;
        for( size_t i = next.index + 1; i != next.components.size(); ++i ) {
            std::transform( next.components[i].begin(), next.components[i].end(),
                            std::inserter( later_itypes, later_itypes.end() ),
            []( const item_comp & c ) {
                return c.type;
            } );
        }

        std::vector<item_comp> this_requirement = next.components[next.index];

        auto first_duplicated = std::stable_partition(
                                    this_requirement.begin(), this_requirement.end(),
        [&]( const item_comp & c ) {
            return !later_itypes.count( c.type );
        }
                                );

        for( auto comp_it = first_duplicated; comp_it != this_requirement.end(); ++comp_it ) {
            // Factor this requirement out into its own separate case

            alter_item_comp_vector req_prefix( next.components.begin(),
                                               next.components.begin() + next.index );
            std::vector<alter_item_comp_vector> result;
            expand_item_in_reqs( *comp_it, req_prefix, next.components, next.index, next.index + 1,
                                 result );
            for( const alter_item_comp_vector &v : result ) {
                // When v is smaller, that means the current requirement was
                // deleted, in which case we don't advance index.
                size_t index_inc = v.size() == next.components.size() ? 1 : 0;
                pending.push( { v, next.index + index_inc } );
            }
        }

        // Deal with all the remaining, non-duplicated ones
        this_requirement.erase( first_duplicated, this_requirement.end() );
        if( !this_requirement.empty() ) {
            alter_item_comp_vector without_dupes = next.components;
            without_dupes[next.index] = this_requirement;
            pending.push( { without_dupes, next.index + 1 } );
        }

        // Because this algorithm is super-exponential in the worst case, add a
        // sanity check to prevent things getting too far out of control.
        // The worst case in the core game currently is boots_fur
        // with 104 alternatives.
        static constexpr size_t max_alternatives = 105;
        if( alternatives_.size() + pending.size() > max_alternatives ) {
            debugmsg( "Construction of deduped_requirement_data generated too many alternatives.  "
                      "The recipe %1s should be simplified.  See the Recipe section in "
                      "doc/JSON_INFO.md for more details.  It has %2s alternatives.", context.str(),
                      alternatives_.size() + pending.size() );
            is_too_complex_ = true;
            alternatives_ = { in };
            return;
        }
    }
}

bool deduped_requirement_data::can_make_with_inventory(
    const read_only_visitable &crafting_inv, const std::function<bool( const item & )> &filter,
    int batch, craft_flags flags ) const
{
    return std::any_of( alternatives().begin(), alternatives().end(),
    [&]( const requirement_data & alt ) {
        return alt.can_make_with_inventory( crafting_inv, filter, batch, flags );
    } );
}

std::vector<const requirement_data *> deduped_requirement_data::feasible_alternatives(
    const read_only_visitable &crafting_inv, const std::function<bool( const item & )> &filter,
    int batch, craft_flags flags ) const
{
    std::vector<const requirement_data *> result;
    for( const requirement_data &req : alternatives() ) {
        if( req.can_make_with_inventory( crafting_inv, filter, batch, flags ) ) {
            result.push_back( &req );
        }
    }
    return result;
}

const requirement_data *deduped_requirement_data::select_alternative(
    Character &crafter, const std::function<bool( const item & )> &filter, int batch,
    craft_flags flags ) const
{
    return select_alternative( crafter, crafter.crafting_inventory(), filter, batch, flags );
}

const requirement_data *deduped_requirement_data::select_alternative(
    Character &crafter, const read_only_visitable &inv,
    const std::function<bool( const item & )> &filter,
    int batch, craft_flags flags ) const
{
    const std::vector<const requirement_data *> all_reqs =
        feasible_alternatives( inv, filter, batch, flags );
    return crafter.select_requirements( all_reqs, 1, inv, filter );
}
