#include "clause.h"
#include "condition.h"
#include "dialogue.h"

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

void clause_t::load_text_func( const JsonObject &/*jo*/ )
{
    // { "text": { extraction parameters } }
}

void clause_t::load_sym_func( const JsonObject &/*jo*/ )
{
    // { "sym": { extraction parameters } }
}

void clause_t::load_color_func( const JsonObject &/*jo*/ )
{
    // { "color": { extraction parameters } }
}

void clause_t::load_value_func( const JsonObject &jo )
{
    if( jo.has_array( "arithmetic" ) ) {
        std::function<int( const dialogue & )> get_int_func = conditional_t<dialogue>::get_arithmetic( jo,
                "arithmetic" );
        if( !!get_int_func ) {
            values.value = [get_int_func]() -> int {
                dialogue d( get_talker_for( get_player_character() ), get_talker_for( get_player_character() ) );
                return get_int_func( d );
            };
        }
    } else {
        std::function<int( const dialogue & )> get_int_func = conditional_t<dialogue>::get_get_int( jo );
        values.value = [get_int_func]() -> int {
            dialogue d( get_talker_for( get_player_character() ), get_talker_for( get_player_character() ) );
            return get_int_func( d );
        };
    }
}

clause_t::clause_t( const JsonObject &jo )
{
    if( jo.has_member( "text" ) ) {
        load_text_func( jo.get_object( "text" ) );
    }
    if( jo.has_member( "sym" ) ) {
        load_sym_func( jo.get_object( "sym" ) );
    }
    if( jo.has_member( "color" ) ) {
        load_color_func( jo.get_object( "color" ) );
    }
    if( jo.has_member( "value" ) ) {
        load_value_func( jo.get_object( "value" ) );
    }
}