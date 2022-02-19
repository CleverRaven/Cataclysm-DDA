#include "clause.h"
#include "color.h"
#include "condition.h"
#include "dialogue.h"
#include "generic_factory.h"

cata::optional<translation> clause_t::get_text() const
{
    return !!values.text ? values.text() : cata::optional<translation>();
}

cata::optional<std::string> clause_t::get_sym() const
{
    return !!values.sym ? values.sym() : cata::optional<std::string>();
}

cata::optional<nc_color> clause_t::get_color() const
{
    return !!values.color ? values.color() : cata::optional<nc_color>();
}

cata::optional<int> clause_t::get_value() const
{
    return !!values.value ? values.value() : cata::optional<int>();
}

cata::optional<int> clause_t::get_val_min() const
{
    return !!values.val_min ? values.val_min() : cata::optional<int>();
}

cata::optional<int> clause_t::get_val_max() const
{
    return !!values.val_max ? values.val_max() : cata::optional<int>();
}

std::pair<cata::optional<int>, cata::optional<int>> clause_t::get_val_norm() const
{
    cata::optional<int> v1 = !!values.val_norm.first ? values.val_norm.first() : cata::optional<int>();
    cata::optional<int> v2 = !!values.val_norm.second ? values.val_norm.second() :
                             cata::optional<int>();
    return { v1, v2 };
}

static void load_text_func( const JsonObject &/*jo*/,
                            std::function<cata::optional<translation>()> &/*var*/ )
{
    // { "text": { extraction parameters } }
}

static void load_sym_func( const JsonObject &/*jo*/,
                           std::function<cata::optional<std::string>()> &/*var*/ )
{
    // { "sym": { extraction parameters } }
}

static void load_color_func( const JsonObject &/*jo*/,
                             std::function<cata::optional<nc_color>()> &/*var*/ )
{
    // { "color": { extraction parameters } }
}

static void load_value_func( const JsonObject &jo, std::function<cata::optional<int>()> &var )
{
    if( jo.has_array( "arithmetic" ) ) {
        std::function<int( const dialogue & )> get_int_func = conditional_t<dialogue>::get_arithmetic( jo,
                "arithmetic" );
        if( !!get_int_func ) {
            var = [get_int_func]() -> int {
                dialogue d( get_talker_for( get_player_character() ), get_talker_for( get_player_character() ) );
                return get_int_func( d );
            };
        }
    } else {
        std::function<int( const dialogue & )> get_int_func = conditional_t<dialogue>::get_get_int( jo );
        var = [get_int_func]() -> int {
            dialogue d( get_talker_for( get_player_character() ), get_talker_for( get_player_character() ) );
            return get_int_func( d );
        };
    }
}

template<typename T>
static void load_const_val( const T &val, std::function<cata::optional<T>()> &var )
{
    var = [val]() -> T {
        return val;
    };
}

static void load_value_pair_func( const JsonArray &jo,
                                  std::pair<std::function<cata::optional<int>()>, std::function<cata::optional<int>()>> &var )
{
    var = std::pair<std::function<cata::optional<int>()>, std::function<cata::optional<int>()>>();
    if( jo.has_int( 0 ) ) {
        load_const_val( jo.get_int( 0 ), var.first );
    } else if( jo.has_object( 0 ) ) {
        load_value_func( jo.get_object( 0 ), var.first );
    }
    if( jo.has_int( 1 ) ) {
        load_const_val( jo.get_int( 1 ), var.second );
    } else if( jo.has_object( 1 ) ) {
        load_value_func( jo.get_object( 1 ), var.second );
    }
}

clause_t::clause_t( const JsonObject &jo )
{
    if( jo.has_member( "text" ) ) {
        if( jo.has_string( "text" ) ) {
            translation t;
            mandatory( jo, false, "text", t );
            load_const_val( t, values.text );
        } else if( jo.has_object( "text" ) ) {
            load_text_func( jo.get_object( "text" ), values.text );
        }
    }
    if( jo.has_member( "sym" ) ) {
        if( jo.has_string( "sym" ) ) {
            load_const_val( jo.get_string( "sym" ), values.sym );
        } else if( jo.has_object( "sym" ) ) {
            load_sym_func( jo.get_object( "sym" ), values.sym );
        }
    }
    if( jo.has_member( "color" ) ) {
        if( jo.has_string( "color" ) ) {
            nc_color clr = color_from_string( jo.get_string( "color" ) );
            load_const_val( clr, values.color );
        } else if( jo.has_object( "color" ) ) {
            load_color_func( jo.get_object( "color" ), values.color );
        }
    }
    if( jo.has_member( "value" ) ) {
        if( jo.has_object( "value" ) ) {
            load_value_func( jo.get_object( "value" ), values.value );
        } else if( jo.has_int( "value" ) ) {
            load_const_val( jo.get_int( "value" ), values.value );
        }
    }
    if( jo.has_member( "val_min" ) ) {
        if( jo.has_object( "val_min" ) ) {
            load_value_func( jo.get_object( "val_min" ), values.val_min );
        } else if( jo.has_int( "val_min" ) ) {
            load_const_val( jo.get_int( "val_min" ), values.val_min );
        }
    }
    if( jo.has_member( "val_max" ) ) {
        if( jo.has_object( "val_max" ) ) {
            load_value_func( jo.get_object( "val_max" ), values.val_max );
        } else if( jo.has_int( "val_max" ) ) {
            load_const_val( jo.get_int( "val_max" ), values.val_max );
        }
    }
    if( jo.has_member( "val_norm" ) ) {
        load_value_pair_func( jo.get_array( "val_norm" ), values.val_norm );
    }
}