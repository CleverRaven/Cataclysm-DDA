#include "avatar.h"
#include "character.h"
#include "display.h"
#include "game.h"
#include "options.h"
#include "make_static.h"
#include "map.h"
#include "mood_face.h"
#include "move_mode.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "weather.h"

static const efftype_id effect_bandaged( "bandaged" );
static const efftype_id effect_bite( "bite" );
static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_disinfected( "disinfected" );
static const efftype_id effect_got_checked( "got_checked" );
static const efftype_id effect_hunger_blank( "hunger_blank" );
static const efftype_id effect_hunger_engorged( "hunger_engorged" );
static const efftype_id effect_hunger_famished( "hunger_famished" );
static const efftype_id effect_hunger_full( "hunger_full" );
static const efftype_id effect_hunger_hungry( "hunger_hungry" );
static const efftype_id effect_hunger_near_starving( "hunger_near_starving" );
static const efftype_id effect_hunger_satisfied( "hunger_satisfied" );
static const efftype_id effect_hunger_starving( "hunger_starving" );
static const efftype_id effect_hunger_very_hungry( "hunger_very_hungry" );
static const efftype_id effect_infected( "infected" );

static const flag_id json_flag_RAD_DETECT( "RAD_DETECT" );
static const flag_id json_flag_SPLINT( "SPLINT" );
static const flag_id json_flag_THERMOMETER( "THERMOMETER" );

static const itype_id fuel_type_muscle( "muscle" );

// Get remotely controlled vehicle, or vehicle character is inside of
vehicle *display::vehicle_driven( const Character &u )
{
    vehicle *veh = g->remoteveh();
    if( veh == nullptr && u.in_vehicle ) {
        veh = veh_pointer_or_null( get_map().veh_at( u.pos() ) );
    }
    return veh;
}

std::pair<std::string, nc_color> display::str_text_color( const Character &p )
{
    nc_color clr;

    if( p.get_str() == p.get_str_base() ) {
        clr = c_white;
    } else if( p.get_str() > p.get_str_base() ) {
        clr = c_green;
    } else if( p.get_str() < p.get_str_base() ) {
        clr = c_red;
    }
    return std::make_pair( _( "Str " ) + ( p.get_str() < 100 ? std::to_string(
            p.get_str() ) : "++" ), clr );
}

std::pair<std::string, nc_color> display::dex_text_color( const Character &p )
{
    nc_color clr;

    if( p.get_dex() == p.get_dex_base() ) {
        clr = c_white;
    } else if( p.get_dex() > p.get_dex_base() ) {
        clr = c_green;
    } else if( p.get_dex() < p.get_dex_base() ) {
        clr = c_red;
    }
    return std::make_pair( _( "Dex " ) + ( p.get_dex() < 100 ? std::to_string(
            p.get_dex() ) : "++" ), clr );
}

std::pair<std::string, nc_color> display::int_text_color( const Character &p )
{
    nc_color clr;

    if( p.get_int() == p.get_int_base() ) {
        clr = c_white;
    } else if( p.get_int() > p.get_int_base() ) {
        clr = c_green;
    } else if( p.get_int() < p.get_int_base() ) {
        clr = c_red;
    }
    return std::make_pair( _( "Int " ) + ( p.get_int() < 100 ? std::to_string(
            p.get_int() ) : "++" ), clr );
}

std::pair<std::string, nc_color> display::per_text_color( const Character &p )
{
    nc_color clr;

    if( p.get_per() == p.get_per_base() ) {
        clr = c_white;
    } else if( p.get_per() > p.get_per_base() ) {
        clr = c_green;
    } else if( p.get_per() < p.get_per_base() ) {
        clr = c_red;
    }
    return std::make_pair( _( "Per " ) + ( p.get_per() < 100 ? std::to_string(
            p.get_per() ) : "++" ), clr );
}

std::string display::get_temp( const Character &u )
{
    std::string temp;
    if( u.has_item_with_flag( json_flag_THERMOMETER ) ||
        u.has_flag( STATIC( json_character_flag( "THERMOMETER" ) ) ) ) {
        temp = print_temperature( get_weather().get_temperature( u.pos() ) );
    }
    if( temp.empty() ) {
        return "-";
    }
    return temp;
}

std::string display::get_moon_graphic()
{
    //moon phase display
    static std::vector<std::string> vMoonPhase = { "(   )", "(  ))", "( | )", "((  )" };

    const int iPhase = static_cast<int>( get_moon_phase( calendar::turn ) );
    std::string sPhase = vMoonPhase[iPhase % 4];

    if( iPhase > 0 ) {
        sPhase.insert( 5 - ( ( iPhase > 4 ) ? iPhase % 4 : 0 ), "</color>" );
        sPhase.insert( 5 - ( ( iPhase < 4 ) ? iPhase + 1 : 5 ),
                       "<color_" + string_from_color( i_black ) + ">" );
    }
    return sPhase;
}

std::string display::get_moon()
{
    const int iPhase = static_cast<int>( get_moon_phase( calendar::turn ) );
    switch( iPhase ) {
        case 0:
            return _( "New moon" );
        case 1:
            return _( "Waxing crescent" );
        case 2:
            return _( "Half moon" );
        case 3:
            return _( "Waxing gibbous" );
        case 4:
            return _( "Full moon" );
        case 5:
            return _( "Waning gibbous" );
        case 6:
            return _( "Half moon" );
        case 7:
            return _( "Waning crescent" );
        case 8:
            return _( "Dark moon" );
        default:
            return "";
    }
}

std::string display::time_approx()
{
    const int iHour = hour_of_day<int>( calendar::turn );
    if( iHour >= 23 || iHour <= 1 ) {
        return _( "Around midnight" );
    } else if( iHour <= 4 ) {
        return _( "Dead of night" );
    } else if( iHour <= 6 ) {
        return _( "Around dawn" );
    } else if( iHour <= 8 ) {
        return _( "Early morning" );
    } else if( iHour <= 10 ) {
        return _( "Morning" );
    } else if( iHour <= 13 ) {
        return _( "Around noon" );
    } else if( iHour <= 16 ) {
        return _( "Afternoon" );
    } else if( iHour <= 18 ) {
        return _( "Early evening" );
    } else if( iHour <= 20 ) {
        return _( "Around dusk" );
    }
    return _( "Night" );
}

std::string display::date_string()
{
    const std::string season = calendar::name_season( season_of_year( calendar::turn ) );
    const int day_num = day_of_season<int>( calendar::turn ) + 1;
    return string_format( _( "%s, day %d" ), season, day_num );
}

std::string display::time_string( const Character &u )
{
    // Return exact time if character has a watch, or approximate time if aboveground
    if( u.has_watch() ) {
        return to_string_time_of_day( calendar::turn );
    } else if( get_map().get_abs_sub().z >= 0 ) {
        return display::time_approx();
    } else {
        // NOLINTNEXTLINE(cata-text-style): the question mark does not end a sentence
        return _( "???" );
    }
}

std::pair<std::string, nc_color> display::morale_face_color( const avatar &u )
{
    const mood_face_id &face = u.character_mood_face();
    if( face.is_null() ) {
        return std::make_pair( "ERR", c_white );
    }

    const int morale_int = u.get_morale_level();
    return morale_emotion( morale_int, face.obj() );
}

static std::pair<bodypart_id, bodypart_id> temp_delta( const Character &u )
{
    bodypart_id current_bp_extreme = u.get_all_body_parts().front();
    bodypart_id conv_bp_extreme = current_bp_extreme;
    for( const bodypart_id &bp : u.get_all_body_parts() ) {
        if( std::abs( u.get_part_temp_cur( bp ) - BODYTEMP_NORM ) >
            std::abs( u.get_part_temp_cur( current_bp_extreme ) - BODYTEMP_NORM ) ) {
            current_bp_extreme = bp;
        }
        if( std::abs( u.get_part_temp_conv( bp ) - BODYTEMP_NORM ) >
            std::abs( u.get_part_temp_conv( conv_bp_extreme ) - BODYTEMP_NORM ) ) {
            conv_bp_extreme = bp;
        }
    }
    return std::make_pair( current_bp_extreme, conv_bp_extreme );
}

static int define_temp_level( const int lvl )
{
    if( lvl > BODYTEMP_SCORCHING ) {
        return 7;
    } else if( lvl > BODYTEMP_VERY_HOT ) {
        return 6;
    } else if( lvl > BODYTEMP_HOT ) {
        return 5;
    } else if( lvl > BODYTEMP_COLD ) {
        return 4;
    } else if( lvl > BODYTEMP_VERY_COLD ) {
        return 3;
    } else if( lvl > BODYTEMP_FREEZING ) {
        return 2;
    }
    return 1;
}

std::string display::temp_delta_string( const Character &u )
{
    std::string temp_message;
    std::pair<bodypart_id, bodypart_id> temp_pair = temp_delta( u );
    // Assign zones for comparisons
    const int cur_zone = define_temp_level( u.get_part_temp_cur( temp_pair.first ) );
    const int conv_zone = define_temp_level( u.get_part_temp_conv( temp_pair.second ) );

    // delta will be positive if temp_cur is rising
    const int delta = conv_zone - cur_zone;
    // Decide if temp_cur is rising or falling
    if( delta > 2 ) {
        temp_message = _( " (Rising!!)" );
    } else if( delta == 2 ) {
        temp_message = _( " (Rising!)" );
    } else if( delta == 1 ) {
        temp_message = _( " (Rising)" );
    } else if( delta == 0 ) {
        temp_message.clear();
    } else if( delta == -1 ) {
        temp_message = _( " (Falling)" );
    } else if( delta == -2 ) {
        temp_message = _( " (Falling!)" );
    } else {
        temp_message = _( " (Falling!!)" );
    }
    return temp_message;
}

std::pair<std::string, nc_color> display::temp_delta_arrows( const Character &u )
{
    std::string temp_message;
    nc_color temp_color = c_white;
    std::pair<bodypart_id, bodypart_id> temp_pair = temp_delta( u );
    // Assign zones for comparisons
    const int cur_zone = define_temp_level( u.get_part_temp_cur( temp_pair.first ) );
    const int conv_zone = define_temp_level( u.get_part_temp_conv( temp_pair.second ) );

    // delta will be positive if temp_cur is rising
    const int delta = conv_zone - cur_zone;
    // Decide if temp_cur is rising or falling
    if( delta > 2 ) {
        temp_message = " ↑↑↑";
        temp_color = c_red;
    } else if( delta == 2 ) {
        temp_message = " ↑↑";
        temp_color = c_light_red;
    } else if( delta == 1 ) {
        temp_message = " ↑";
        temp_color = c_yellow;
    } else if( delta == 0 ) {
        temp_message = "-";
        temp_color = c_green;
    } else if( delta == -1 ) {
        temp_message = " ↓";
        temp_color = c_light_blue;
    } else if( delta == -2 ) {
        temp_message = " ↓↓";
        temp_color = c_cyan;
    } else {
        temp_message = " ↓↓↓";
        temp_color = c_blue;
    }
    return std::make_pair( temp_message, temp_color );
}

std::pair<std::string, nc_color> display::temp_text_color( const Character &u )
{
    /// Find hottest/coldest bodypart
    // Calculate the most extreme body temperatures
    const bodypart_id current_bp_extreme = temp_delta( u ).first;

    // printCur the hottest/coldest bodypart
    std::string temp_string;
    nc_color temp_color = c_yellow;
    if( u.get_part_temp_cur( current_bp_extreme ) > BODYTEMP_SCORCHING ) {
        temp_color = c_red;
        temp_string = _( "Scorching!" );
    } else if( u.get_part_temp_cur( current_bp_extreme ) > BODYTEMP_VERY_HOT ) {
        temp_color = c_light_red;
        temp_string = _( "Very hot!" );
    } else if( u.get_part_temp_cur( current_bp_extreme ) > BODYTEMP_HOT ) {
        temp_color = c_yellow;
        temp_string = _( "Warm" );
    } else if( u.get_part_temp_cur( current_bp_extreme ) > BODYTEMP_COLD ) {
        temp_color = c_green;
        temp_string = _( "Comfortable" );
    } else if( u.get_part_temp_cur( current_bp_extreme ) > BODYTEMP_VERY_COLD ) {
        temp_color = c_light_blue;
        temp_string = _( "Chilly" );
    } else if( u.get_part_temp_cur( current_bp_extreme ) > BODYTEMP_FREEZING ) {
        temp_color = c_cyan;
        temp_string = _( "Very cold!" );
    } else if( u.get_part_temp_cur( current_bp_extreme ) <= BODYTEMP_FREEZING ) {
        temp_color = c_blue;
        temp_string = _( "Freezing!" );
    }
    return std::make_pair( temp_string, temp_color );
}

std::pair<std::string, nc_color> display::morale_emotion( const int morale_cur,
        const mood_face &face )
{
    std::string current_face;
    nc_color current_color;

    for( const mood_face_value &face_value : face.values() ) {
        current_face = remove_color_tags( face_value.face() );
        current_color = get_color_from_tag( face_value.face() ).color;
        if( face_value.value() <= morale_cur ) {
            return std::make_pair( current_face, current_color );
        }
    }

    // Return the last value found
    if( !current_face.empty() ) {
        return std::make_pair( current_face, current_color );
    }

    debugmsg( "morale_emotion no matching face found for: %s", face.getId().str() );
    return std::make_pair( "ERR", c_light_gray );
}

std::pair<std::string, nc_color> display::power_text_color( const Character &u )
{
    nc_color c_pwr = c_red;
    std::string s_pwr;
    if( !u.has_max_power() ) {
        s_pwr = "--";
        c_pwr = c_light_gray;
    } else {
        if( u.get_power_level() >= u.get_max_power_level() / 2 ) {
            c_pwr = c_light_blue;
        } else if( u.get_power_level() >= u.get_max_power_level() / 3 ) {
            c_pwr = c_yellow;
        } else if( u.get_power_level() >= u.get_max_power_level() / 4 ) {
            c_pwr = c_red;
        }

        if( u.get_power_level() < 1_J ) {
            s_pwr = std::to_string( units::to_millijoule( u.get_power_level() ) ) +
                    pgettext( "energy unit: millijoule", "mJ" );
        } else if( u.get_power_level() < 1_kJ ) {
            s_pwr = std::to_string( units::to_joule( u.get_power_level() ) ) +
                    pgettext( "energy unit: joule", "J" );
        } else {
            s_pwr = std::to_string( units::to_kilojoule( u.get_power_level() ) ) +
                    pgettext( "energy unit: kilojoule", "kJ" );
        }
    }
    return std::make_pair( s_pwr, c_pwr );
}

std::pair<std::string, nc_color> display::mana_text_color( const Character &you )
{
    nc_color c_mana = c_red;
    std::string s_mana;
    if( you.magic->max_mana( you ) <= 0 ) {
        s_mana = "--";
        c_mana = c_light_gray;
    } else {
        if( you.magic->available_mana() >= you.magic->max_mana( you ) / 2 ) {
            c_mana = c_light_blue;
        } else if( you.magic->available_mana() >= you.magic->max_mana( you ) / 3 ) {
            c_mana = c_yellow;
        }
        s_mana = std::to_string( you.magic->available_mana() );
    }
    return std::make_pair( s_mana, c_mana );
}

std::pair<std::string, nc_color> display::safe_mode_text_color( const bool classic_mode )
{
    // "classic" mode used by draw_limb2 and draw_health_classic are "SAFE" or (empty)
    // draw_stat_narrow and draw_stat_wide display "On" or "Off" value
    std::string s_text;
    if( classic_mode ) {
        if( g->safe_mode || get_option<bool>( "AUTOSAFEMODE" ) ) {
            s_text = _( "SAFE" );
        }
    } else {
        s_text = g->safe_mode ? _( "On" ) : _( "Off" );
    }

    // By default, color is green if safe, red otherwise
    nc_color s_color = g->safe_mode ? c_green : c_red;
    // If auto-safe-mode is enabled, go to light red, yellow, and green as the turn limit approaches
    if( g->safe_mode == SAFE_MODE_OFF && get_option<bool>( "AUTOSAFEMODE" ) ) {
        time_duration s_return = time_duration::from_turns( get_option<int>( "AUTOSAFEMODETURNS" ) );
        int iPercent = g->turnssincelastmon * 100 / s_return;
        if( iPercent >= 100 ) {
            s_color = c_green;
        } else if( iPercent >= 75 ) {
            s_color = c_yellow;
        } else if( iPercent >= 50 ) {
            s_color = c_light_red;
        } else if( iPercent >= 25 ) {
            s_color = c_red;
        }
    }

    return std::make_pair( s_text, s_color );
}

std::pair<std::string, nc_color> display::weariness_text_color( const Character &u )
{
    std::pair<translation, nc_color> trans_color = display::weariness_text_color( u.weariness_level() );
    return std::make_pair( trans_color.first.translated(), trans_color.second );
}

std::pair<translation, nc_color> display::weariness_text_color( size_t weariness )
{
    static const std::array<std::pair<translation, nc_color>, 6> weary_descriptions { {
            {to_translation( "weariness description", "Fresh" ), c_green},
            {to_translation( "weariness description", "Light" ), c_yellow },
            {to_translation( "weariness description", "Moderate" ), c_light_red},
            {to_translation( "weariness description", "Weary" ), c_light_red },
            {to_translation( "weariness description", "Very" ), c_red },
            {to_translation( "weariness description", "Extreme" ), c_red }
        } };
    // We can have more than level 5, but we don't really care about it
    if( weariness >= weary_descriptions.size() ) {
        weariness = 5;
    }
    return weary_descriptions[weariness];
}

std::pair<std::string, nc_color> display::weary_malus_text_color( const Character &u )
{
    const float act_level = u.instantaneous_activity_level();
    nc_color act_color;
    if( u.exertion_adjusted_move_multiplier( act_level ) < 1.0 ) {
        act_color = c_red;
    } else {
        act_color = c_light_gray;
    }
    return std::make_pair( display::activity_malus_str( u ), act_color );
}

std::string display::activity_level_str( float level )
{
    static const std::array<translation, 6> activity_descriptions { {
            to_translation( "activity description", "None" ),
            to_translation( "activity description", "Light" ),
            to_translation( "activity description", "Moderate" ),
            to_translation( "activity description", "Brisk" ),
            to_translation( "activity description", "Active" ),
            to_translation( "activity description", "Extreme" ),
        } };
    // Activity levels are 1, 2, 4, 6, 8, 10
    // So we can easily cut them in half and round down for an index
    int idx = std::floor( level / 2 );

    return activity_descriptions[idx].translated();
}

std::string display::activity_malus_str( const Character &u )
{
    const float act_level = u.instantaneous_activity_level();
    const float exertion_mult = u.exertion_adjusted_move_multiplier( act_level ) ;
    const int malus_value = ( 1 / exertion_mult ) * 100 - 100;
    return string_format( "+%3d%%", malus_value );
}

std::pair<std::string, nc_color> display::activity_text_color( const Character &u )
{
    const float act_level = u.instantaneous_activity_level();
    const std::string act_text = display::activity_level_str( act_level );

    nc_color act_color;
    if( u.exertion_adjusted_move_multiplier( act_level ) < 1.0 ) {
        act_color = c_red;
    } else {
        act_color = c_light_gray;
    }

    return std::make_pair( _( act_text ), act_color );
}

std::pair<std::string, nc_color> display::thirst_text_color( const Character &u )
{
    // some delay from water in stomach is desired, but there needs to be some visceral response
    int thirst = u.get_thirst() - std::max( units::to_milliliter<int>( u.stomach.get_water() ) / 10,
                                            0 );
    std::string hydration_string;
    nc_color hydration_color = c_white;
    if( thirst > 520 ) {
        hydration_color = c_light_red;
        hydration_string = translate_marker( "Parched" );
    } else if( thirst > 240 ) {
        hydration_color = c_light_red;
        hydration_string = translate_marker( "Dehydrated" );
    } else if( thirst > 80 ) {
        hydration_color = c_yellow;
        hydration_string = translate_marker( "Very thirsty" );
    } else if( thirst > 40 ) {
        hydration_color = c_yellow;
        hydration_string = translate_marker( "Thirsty" );
    } else if( thirst < -60 ) {
        hydration_color = c_green;
        hydration_string = translate_marker( "Turgid" );
    } else if( thirst < -20 ) {
        hydration_color = c_green;
        hydration_string = translate_marker( "Hydrated" );
    } else if( thirst < 0 ) {
        hydration_color = c_green;
        hydration_string = translate_marker( "Slaked" );
    }
    return std::make_pair( _( hydration_string ), hydration_color );
}

std::pair<std::string, nc_color> display::hunger_text_color( const Character &u )
{
    // NPCs who do not need food have no hunger
    if( !u.needs_food() ) {
        return std::make_pair( _( "Without Hunger" ), c_white );
    }
    // clang 3.8 has some sort of issue where if the initializer list contains const arguments,
    // like all of the effect_* string_id variables which are const string_id, then it fails to
    // initialize the array with tuples successfully complaining that
    // "chosen constructor is explicit in copy-initialization". Using std::forward_as_tuple
    // returns a tuple consisting of correctly implcitly copyable types.
    static const std::array<std::tuple<const efftype_id &, const char *, nc_color>, 9> hunger_states{ {
            std::forward_as_tuple( effect_hunger_engorged, translate_marker( "Engorged" ), c_red ),
            std::forward_as_tuple( effect_hunger_full, translate_marker( "Full" ), c_yellow ),
            std::forward_as_tuple( effect_hunger_satisfied, translate_marker( "Satisfied" ), c_green ),
            std::forward_as_tuple( effect_hunger_blank, "", c_white ),
            std::forward_as_tuple( effect_hunger_hungry, translate_marker( "Hungry" ), c_yellow ),
            std::forward_as_tuple( effect_hunger_very_hungry, translate_marker( "Very hungry" ), c_yellow ),
            std::forward_as_tuple( effect_hunger_near_starving, translate_marker( "Near starving" ), c_red ),
            std::forward_as_tuple( effect_hunger_starving, translate_marker( "Starving!" ), c_red ),
            std::forward_as_tuple( effect_hunger_famished, translate_marker( "Famished" ), c_light_red )
        }
    };
    for( auto &hunger_state : hunger_states ) {
        if( u.has_effect( std::get<0>( hunger_state ) ) ) {
            return std::make_pair( _( std::get<1>( hunger_state ) ), std::get<2>( hunger_state ) );
        }
    }
    return std::make_pair( _( "ERROR!" ), c_light_red );
}

std::pair<std::string, nc_color> display::weight_text_color( const Character &u )
{
    const float bmi = u.get_bmi();
    std::string weight_string;
    nc_color weight_color = c_light_gray;
    if( get_option<bool>( "CRAZY" ) ) {
        if( bmi > character_weight_category::morbidly_obese + 10.0f ) {
            weight_string = translate_marker( "AW HELL NAH" );
            weight_color = c_red;
        } else if( bmi > character_weight_category::morbidly_obese + 5.0f ) {
            weight_string = translate_marker( "DAYUM" );
            weight_color = c_red;
        } else if( bmi > character_weight_category::morbidly_obese ) {
            weight_string = translate_marker( "Fluffy" );
            weight_color = c_red;
        } else if( bmi > character_weight_category::very_obese ) {
            weight_string = translate_marker( "Husky" );
            weight_color = c_red;
        } else if( bmi > character_weight_category::obese ) {
            weight_string = translate_marker( "Healthy" );
            weight_color = c_light_red;
        } else if( bmi > character_weight_category::overweight ) {
            weight_string = translate_marker( "Big" );
            weight_color = c_yellow;
        } else if( bmi > character_weight_category::normal ) {
            weight_string = translate_marker( "Normal" );
            weight_color = c_light_gray;
        } else if( bmi > character_weight_category::underweight ) {
            weight_string = translate_marker( "Bean Pole" );
            weight_color = c_yellow;
        } else if( bmi > character_weight_category::emaciated ) {
            weight_string = translate_marker( "Emaciated" );
            weight_color = c_light_red;
        } else {
            weight_string = translate_marker( "Spooky Scary Skeleton" );
            weight_color = c_red;
        }
    } else {
        if( bmi > character_weight_category::morbidly_obese ) {
            weight_string = translate_marker( "Morbidly Obese" );
            weight_color = c_red;
        } else if( bmi > character_weight_category::very_obese ) {
            weight_string = translate_marker( "Very Obese" );
            weight_color = c_red;
        } else if( bmi > character_weight_category::obese ) {
            weight_string = translate_marker( "Obese" );
            weight_color = c_light_red;
        } else if( bmi > character_weight_category::overweight ) {
            weight_string = translate_marker( "Overweight" );
            weight_color = c_yellow;
        } else if( bmi > character_weight_category::normal ) {
            weight_string = translate_marker( "Normal" );
            weight_color = c_light_gray;
        } else if( bmi > character_weight_category::underweight ) {
            weight_string = translate_marker( "Underweight" );
            weight_color = c_yellow;
        } else if( bmi > character_weight_category::emaciated ) {
            weight_string = translate_marker( "Emaciated" );
            weight_color = c_light_red;
        } else {
            weight_string = translate_marker( "Skeletal" );
            weight_color = c_red;
        }
    }
    return std::make_pair( _( weight_string ), weight_color );
}

std::string display::weight_long_description( const Character &u )
{
    const float bmi = u.get_bmi();
    if( bmi > character_weight_category::morbidly_obese ) {
        return _( "You have far more fat than is healthy or useful.  It is causing you major problems." );
    } else if( bmi > character_weight_category::very_obese ) {
        return _( "You have too much fat.  It impacts your day-to-day health and wellness." );
    } else if( bmi > character_weight_category::obese ) {
        return _( "You've definitely put on a lot of extra weight.  Although helpful in times of famine, this is too much and is impacting your health." );
    } else if( bmi > character_weight_category::overweight ) {
        return _( "You've put on some extra pounds.  Nothing too excessive, but it's starting to impact your health and waistline a bit." );
    } else if( bmi > character_weight_category::normal ) {
        return _( "You look to be a pretty healthy weight, with some fat to last you through the winter, but nothing excessive." );
    } else if( bmi > character_weight_category::underweight ) {
        return _( "You are thin, thinner than is healthy.  You are less resilient to going without food." );
    } else if( bmi > character_weight_category::emaciated ) {
        return _( "You are very unhealthily underweight, nearing starvation." );
    } else {
        return _( "You have very little meat left on your bones.  You appear to be starving." );
    }
}

std::string display::weight_string( const Character &u )
{
    std::pair<std::string, nc_color> weight_pair = display::weight_text_color( u );
    return colorize( weight_pair.first, weight_pair.second );
}

std::pair<std::string, nc_color> display::fatigue_text_color( const Character &u )
{
    int fatigue = u.get_fatigue();
    std::string fatigue_string;
    nc_color fatigue_color = c_white;
    if( fatigue > fatigue_levels::EXHAUSTED ) {
        fatigue_color = c_red;
        fatigue_string = translate_marker( "Exhausted" );
    } else if( fatigue > fatigue_levels::DEAD_TIRED ) {
        fatigue_color = c_light_red;
        fatigue_string = translate_marker( "Dead Tired" );
    } else if( fatigue > fatigue_levels::TIRED ) {
        fatigue_color = c_yellow;
        fatigue_string = translate_marker( "Tired" );
    }
    return std::make_pair( _( fatigue_string ), fatigue_color );
}

std::pair<std::string, nc_color> display::health_text_color( const Character &u )
{
    std::string h_string;
    nc_color h_color = c_light_gray;

    int current_health = u.get_healthy();
    if( current_health < -100 ) {
        h_string = "Horrible";
        h_color = c_red;
    } else if( current_health < -50 ) {
        h_string = "Very bad";
        h_color = c_light_red;
    } else if( current_health < -10 ) {
        h_string = "Bad";
        h_color = c_yellow;
    } else if( current_health < 10 ) {
        h_string = "OK";
        h_color = c_light_gray;
    } else if( current_health < 50 ) {
        h_string = "Good";
        h_color = c_white;
    } else if( current_health < 100 ) {
        h_string = "Very good";
        h_color = c_green;
    } else {
        h_string = "Excellent";
        h_color = c_light_green;
    }
    return std::make_pair( _( h_string ), h_color );
}

std::pair<std::string, nc_color> display::pain_text_color( const Creature &c )
{
    float scale = c.get_perceived_pain() / 10.f;
    std::string pain_string;
    nc_color pain_color = c_yellow;
    if( scale > 7 ) {
        pain_string = _( "Severe pain" );
    } else if( scale > 6 ) {
        pain_string = _( "Intense pain" );
    } else if( scale > 5 ) {
        pain_string = _( "Unmanageable pain" );
    } else if( scale > 4 ) {
        pain_string = _( "Distressing pain" );
    } else if( scale > 3 ) {
        pain_string = _( "Distracting pain" );
    } else if( scale > 2 ) {
        pain_string = _( "Moderate pain" );
    } else if( scale > 1 ) {
        pain_string = _( "Mild pain" );
    } else if( scale > 0 ) {
        pain_string = _( "Minimal pain" );
    } else {
        pain_string = _( "No pain" );
        pain_color = c_white;
    }
    return std::make_pair( pain_string, pain_color );
}

std::pair<std::string, nc_color> display::pain_text_color( const Character &u )
{
    // Get base Creature pain text to start with
    const std::pair<std::string, nc_color> pain = display::pain_text_color(
                static_cast<const Creature &>( u ) );
    nc_color pain_color = pain.second;
    std::string pain_string;
    // get pain color
    int perceived_pain = u.get_perceived_pain();
    if( perceived_pain >= 60 ) {
        pain_color = c_red;
    } else if( perceived_pain >= 40 ) {
        pain_color = c_light_red;
    }
    // get pain string
    if( u.has_effect( effect_got_checked ) && perceived_pain > 0 ) {
        pain_string = string_format( "%s %d", _( "Pain " ), perceived_pain );
    } else if( perceived_pain > 0 ) {
        pain_string = pain.first;
    }
    return std::make_pair( pain_string, pain_color );
}

nc_color display::bodytemp_color( const Character &u, const bodypart_id &bp )
{
    nc_color color = c_light_gray; // default
    const int temp_conv = u.get_part_temp_conv( bp );
    if( bp == body_part_eyes ) {
        color = c_light_gray;    // Eyes don't count towards warmth
    } else if( temp_conv  > BODYTEMP_SCORCHING ) {
        color = c_red;
    } else if( temp_conv  > BODYTEMP_VERY_HOT ) {
        color = c_light_red;
    } else if( temp_conv  > BODYTEMP_HOT ) {
        color = c_yellow;
    } else if( temp_conv  > BODYTEMP_COLD ) {
        color = c_green;
    } else if( temp_conv  > BODYTEMP_VERY_COLD ) {
        color = c_light_blue;
    } else if( temp_conv  > BODYTEMP_FREEZING ) {
        color = c_cyan;
    } else {
        color = c_blue;
    }
    return color;
}

nc_color display::encumb_color( const int level )
{
    if( level < 0 ) {
        return c_green;
    }
    if( level < 10 ) {
        return c_light_gray;
    }
    if( level < 40 ) {
        return c_yellow;
    }
    if( level < 70 ) {
        return c_light_red;
    }
    return c_red;
}

nc_color display::limb_color( const Character &u, const bodypart_id &bp, bool bleed, bool bite,
                              bool infect )
{
    if( bp == bodypart_str_id::NULL_ID() ) {
        return c_light_gray;
    }
    nc_color i_color = c_light_gray;
    const int intense = u.get_effect_int( effect_bleed, bp );
    const bool bleeding = bleed && intense > 0;
    const bool bitten = bite && u.has_effect( effect_bite, bp.id() );
    const bool infected = infect && u.has_effect( effect_infected, bp.id() );

    // Handle worst cases first
    if( bleeding && infected ) {
        // Red and green make yellow
        if( intense < 21 ) {
            i_color = c_yellow;
        } else {
            i_color = c_yellow_red;
        }
    } else if( bleeding && bitten ) {
        // Red and blue make magenta
        if( intense < 21 ) {
            i_color = c_magenta;
        } else {
            i_color = c_magenta_red;
        }
    } else if( infected ) {
        i_color = c_green; // Green is very bad
    } else if( bitten ) {
        i_color = c_blue; // Blue is also bad
    } else if( bleeding ) {
        // Blood is some shade of red, naturally
        i_color = colorize_bleeding_intensity( intense );
    }

    return i_color;
}

std::string display::rad_badge_color_name( const int rad )
{
    using pair_t = std::pair<const int, const translation>;

    static const std::array<pair_t, 6> values = {{
            pair_t {  0, to_translation( "color", "green" ) },
            pair_t { 30, to_translation( "color", "blue" )  },
            pair_t { 60, to_translation( "color", "yellow" )},
            pair_t {120, to_translation( "color", "orange" )},
            pair_t {240, to_translation( "color", "red" )   },
            pair_t {500, to_translation( "color", "black" ) },
        }
    };

    for( const auto &i : values ) {
        if( rad <= i.first ) {
            return i.second.translated();
        }
    }

    return values.back().second.translated();
}

nc_color display::rad_badge_color( const int rad )
{
    using pair_t = std::pair<const int, nc_color>;

    // Map radiation to a displayable color, using background color if needed
    static const std::array<pair_t, 6> values = {{
            pair_t {  0, c_white_green },   // white on green (for green)
            pair_t { 30, h_white },         // white on blue (for blue)
            pair_t { 60, i_yellow },        // black on yellow (for yellow)
            pair_t {120, c_red_yellow },    // red on brown (for orange)
            pair_t {240, c_red_red },       // black on red (for red)
            pair_t {500, c_pink },          // pink on black (for black)
        }
    };

    for( const auto &i : values ) {
        if( rad <= i.first ) {
            return i.second;
        }
    }

    return values.back().second;
}

std::pair<std::string, nc_color> display::rad_badge_text_color( const Character &u )
{
    // Default - no radiation badge
    std::string rad_text = "Unknown";
    nc_color rad_color = c_light_gray;
    // Get all items that can detect radiation
    for( const item *it : u.all_items_with_flag( json_flag_RAD_DETECT ) ) {
        // Radiation badges only work if they're exposed (worn or wielded)
        if( u.is_worn( *it ) || u.is_wielding( *it ) ) {
            rad_text = string_format( " %s ", rad_badge_color_name( it->irradiation ) );
            rad_color = rad_badge_color( it->irradiation );
            break; // Quit after the first one
        }
    }
    return std::make_pair( rad_text, rad_color );
}

std::string display::vehicle_azimuth_text( const Character &u )
{
    vehicle *veh = display::vehicle_driven( u );
    if( veh ) {
        return veh->face.to_string_azimuth_from_north();
    }
    return "";
}

std::pair<std::string, nc_color> display::vehicle_cruise_text_color( const Character &u )
{
    // Defaults in case no vehicle is found
    std::string vel_text;
    nc_color vel_color = c_light_gray;

    // Show target velocity and current velocity, with units.
    // For example:
    //     25 < 10 mph : accelerating towards 25 mph
    //     25 < 25 mph : cruising at 25 mph
    //     10 < 25 mph : decelerating toward 10
    // Text color indicates how much the engine is straining beyond its safe velocity.
    vehicle *veh = display::vehicle_driven( u );
    if( veh && veh->cruise_on ) {
        int target = static_cast<int>( convert_velocity( veh->cruise_velocity, VU_VEHICLE ) );
        int current = static_cast<int>( convert_velocity( veh->velocity, VU_VEHICLE ) );
        const std::string units = get_option<std::string> ( "USE_METRIC_SPEEDS" );
        vel_text = string_format( "%d < %d %s", target, current, units );

        const float strain = veh->strain();
        if( strain <= 0 ) {
            vel_color = c_light_blue;
        } else if( strain <= 0.2 ) {
            vel_color = c_yellow;
        } else if( strain <= 0.4 ) {
            vel_color = c_light_red;
        } else {
            vel_color = c_red;
        }
    }
    return std::make_pair( vel_text, vel_color );
}

std::pair<std::string, nc_color> display::vehicle_fuel_percent_text_color( const Character &u )
{
    // Defaults in case no vehicle is found
    std::string fuel_text;
    nc_color fuel_color = c_light_gray;

    vehicle *veh = display::vehicle_driven( u );
    if( veh && veh->cruise_on ) {
        itype_id fuel_type = itype_id::NULL_ID();
        // FIXME: Move this to a vehicle helper function like get_active_engine
        for( size_t e = 0; e < veh->engines.size(); e++ ) {
            if( veh->is_engine_on( e ) &&
                !( veh->is_perpetual_type( e ) || veh->is_engine_type( e, fuel_type_muscle ) ) ) {
                // Get the fuel type of the first engine that is turned on
                fuel_type = veh->engine_fuel_current( e );
            }
        }
        int max_fuel = veh->fuel_capacity( fuel_type );
        int cur_fuel = veh->fuel_left( fuel_type );
        if( max_fuel != 0 ) {
            int percent = cur_fuel * 100 / max_fuel;
            // Simple percent indicator, yellow under 25%, red under 10%
            fuel_text = string_format( "%d %%", percent );
            fuel_color = percent < 10 ? c_red : ( percent < 25 ? c_yellow : c_green );
        }
    }

    return std::make_pair( fuel_text, fuel_color );
}

std::vector<std::pair<std::string, nc_color>> display::bodypart_status_colors( const Character &u,
        const bodypart_id &bp )
{
    // List of status strings and colors
    std::vector<std::pair<std::string, nc_color>> ret;
    // Empty if no bodypart given
    if( bp == bodypart_str_id::NULL_ID() ) {
        return ret;
    }

    const int bleed_intensity = u.get_effect_int( effect_bleed, bp );
    const bool bleeding = bleed_intensity > 0;
    const bool bitten = u.has_effect( effect_bite, bp.id() );
    const bool infected = u.has_effect( effect_infected, bp.id() );
    const bool broken = u.is_limb_broken( bp ) && bp->is_limb;
    const bool splinted = u.worn_with_flag( json_flag_SPLINT,  bp );
    const bool bandaged = u.has_effect( effect_bandaged,  bp.id() );
    const bool disinfected = u.has_effect( effect_disinfected,  bp.id() );

    // Ailments
    if( broken ) {
        ret.emplace_back( std::make_pair( "broken", c_magenta ) );
    }
    if( bitten ) {
        ret.emplace_back( std::make_pair( "bitten", c_yellow ) );
    }
    if( bleeding ) {
        ret.emplace_back( std::make_pair( "bleeding",
                                          colorize_bleeding_intensity( bleed_intensity ) ) );
    }
    if( infected ) {
        ret.emplace_back( std::make_pair( "infected", c_pink ) );
    }
    // Treatments
    if( splinted ) {
        ret.emplace_back( std::make_pair( "splinted", c_light_gray ) );
    }
    if( bandaged ) {
        ret.emplace_back( std::make_pair( "bandaged", c_white ) );
    }
    if( disinfected ) {
        ret.emplace_back( std::make_pair( "disinfected", c_light_green ) );
    }

    return ret;
}

std::string display::colorized_bodypart_status_text( const Character &u, const bodypart_id &bp )
{
    // Colorized strings for each status
    std::vector<std::string> color_strings;
    // Get all status strings and colorize them
    for( const std::pair<std::string, nc_color> &sc : display::bodypart_status_colors( u, bp ) ) {
        color_strings.emplace_back( colorize( sc.first, sc.second ) );
    }
    // Join with commas, or return "--" if no statuses
    return color_strings.empty() ? "--" : join( color_strings, ", " );
}

// Single-letter move mode (W, R, C, P)
std::pair<std::string, nc_color> display::move_mode_letter_color( const Character &u )
{
    const std::string mm_text = std::string( 1, u.current_movement_mode()->panel_letter() );
    const nc_color mm_color = u.current_movement_mode()->panel_color();
    return std::make_pair( mm_text, mm_color );
}

// Full name of move mode (walking, running, crouching, prone)
std::pair<std::string, nc_color> display::move_mode_text_color( const Character &u )
{
    const std::string mm_text = u.current_movement_mode()->type_name();
    const nc_color mm_color = u.current_movement_mode()->panel_color();
    return std::make_pair( mm_text, mm_color );
}

