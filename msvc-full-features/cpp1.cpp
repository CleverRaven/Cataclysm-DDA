talk_effect_fun_t::func f_get_distance_value( const JsonObject &jo, std::string_view member )
{
    JsonArray objects = jo.get_array( member );
    if( objects.size() != 3 )
    {
        objects.throw_error( "get distance value requires an array with 3 elements." );
    }
    std::string first = get_string_from_input( objects, 0 );
    std::string second = get_string_from_input( objects, 1 );
    std::string third = get_string_from_input( objects, 2 );
    return [first, second, third]( dialogue & d )
    {
        tripoint_abs_ms first_point = get_tripoint_from_string( first, d );
        tripoint_abs_ms second_point = get_tripoint_from_string( second, d );
        var_info var = get_varinfo_from_string( third );
        int distance_is = rl_dist( first_point, second_point );
        var_type type = var.type;
        std::string var_name = var.name;
        write_var_value( type, var_name, d.actor( type == var_type::npc ), &d,
                         std::to_string( distance_is ) );
    };
}



std::string get_string_from_input( const JsonArray &objects, int index )
{
    if( objects.has_string( index ) )
    {
        std::string type = objects.get_string( index );
        if( type == "u" || type == "npc" )
        {
            return type;
        }
    }
    dbl_or_var empty;
    JsonObject object = objects.get_object( index );
    if( object.has_string( "u_val" ) )
    {
        return "u_" + get_talk_varname( object, "u_val", false, empty );
    }
    else if( object.has_string( "npc_val" ) )
    {
        return "npc_" + get_talk_varname( object, "npc_val", false, empty );
    }
    else if( object.has_string( "global_val" ) )
    {
        return "global_" + get_talk_varname( object, "global_val", false, empty );
    }
    else if( object.has_string( "context_val" ) )
    {
        return "context_" + get_talk_varname( object, "context_val", false, empty );
    }
    else if( object.has_string( "faction_val" ) )
    {
        return "faction_" + get_talk_varname( object, "faction_val", false, empty );
    }
    else if( object.has_string( "party_val" ) )
    {
        return "party_" + get_talk_varname( object, "party_val", false, empty );
    }
    object.throw_error( "Invalid input type." );
    return "";
}

var_info get_varinfo_from_string( const std::string &type )
{
    if( type.find( "u_" ) == 0 )
    {
        var_info var = var_info( var_type::u, type.substr( 2, type.size() - 2 ) );
        return var;
    }
    else if( type.find( "npc_" ) == 0 )
    {
        var_info var = var_info( var_type::npc, type.substr( 4, type.size() - 4 ) );
        return var;
    }
    else if( type.find( "global_" ) == 0 )
    {
        var_info var = var_info( var_type::global, type.substr( 7, type.size() - 7 ) );
        return var;
    }
    else if( type.find( "faction_" ) == 0 )
    {
        var_info var = var_info( var_type::faction, type.substr( 8, type.size() - 8 ) );
        return var;
    }
    else if( type.find( "party_" ) == 0 )
    {
        var_info var = var_info( var_type::party, type.substr( 6, type.size() - 6 ) );
        return var;
    }
    else if( type.find( "context_" ) == 0 )
    {
        var_info var = var_info( var_type::context, type.substr( 8, type.size() - 8 ) );
        return var;
    }
    return var_info();
}

tripoint_abs_ms get_tripoint_from_string( const std::string &type, dialogue const &d )
{
    if( type == "u" )
    {
        return d.actor( false )->global_pos();
    }
    else if( type == "npc" )
    {
        return d.actor( true )->global_pos();
    }
    else if( type.find( "u_" ) == 0 )
    {
        var_info var = var_info( var_type::u, type.substr( 2, type.size() - 2 ) );
        return get_tripoint_from_var( var, d );
    }
    else if( type.find( "npc_" ) == 0 )
    {
        var_info var = var_info( var_type::npc, type.substr( 4, type.size() - 4 ) );
        return get_tripoint_from_var( var, d );
    }
    else if( type.find( "global_" ) == 0 )
    {
        var_info var = var_info( var_type::global, type.substr( 7, type.size() - 7 ) );
        return get_tripoint_from_var( var, d );
    }
    else if( type.find( "faction_" ) == 0 )
    {
        var_info var = var_info( var_type::faction, type.substr( 8, type.size() - 8 ) );
        return get_tripoint_from_var( var, d );
    }
    else if( type.find( "party_" ) == 0 )
    {
        var_info var = var_info( var_type::party, type.substr( 6, type.size() - 6 ) );
        return get_tripoint_from_var( var, d );
    }
    else if( type.find( "context_" ) == 0 )
    {
        var_info var = var_info( var_type::context, type.substr( 8, type.size() - 8 ) );
        return get_tripoint_from_var( var, d );
    }
    return tripoint_abs_ms();
}

tripoint_abs_ms get_tripoint_from_string( const std::string &type, dialogue const &d );
var_info read_var_info( const JsonObject &jo );
var_info get_varinfo_from_string( const std::string &type );
void write_var_value( var_type type, const std::string &name, talker *talk, dialogue *d,
                      const std::string &value );
void write_var_value( var_type type, const std::string &name, talker *talk, dialogue *d,
                      double value );
std::string get_talk_varname( const JsonObject &jo, std::string_view member,
                              bool check_value, dbl_or_var &default_val );
std::string get_talk_var_basename( const JsonObject &jo, std::string_view member,
                                   bool check_value );
std::string get_string_from_input( const JsonArray &objects, int index );
