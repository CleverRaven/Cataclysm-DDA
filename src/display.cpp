#include "display.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <tuple>
#include <type_traits>
#include <vector>

#include "avatar.h"
#include "bodygraph.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character.h"
#include "creature.h"
#include "debug.h"
#include "effect.h"
#include "game.h"
#include "game_constants.h"
#include "make_static.h"
#include "map.h"
#include "mood_face.h"
#include "move_mode.h"
#include "mtype.h"
#include "npc.h"
#include "omdata.h"
#include "options.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "string_formatter.h"
#include "subbodypart.h"
#include "tileray.h"
#include "timed_event.h"
#include "translation.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"
#include "units_utility.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "weather.h"
#include "weather_type.h"

static const efftype_id effect_bite( "bite" );
static const efftype_id effect_bleed( "bleed" );
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

static const flag_id json_flag_THERMOMETER( "THERMOMETER" );

static const itype_id fuel_type_muscle( "muscle" );

// Cache for the overmap widget string
static disp_overmap_cache disp_om_cache;
// Cache for the bodygraph widget string
static std::array<disp_bodygraph_cache, 5> disp_bg_cache = { {
        disp_bodygraph_cache( bodygraph_var::hp ),
        disp_bodygraph_cache( bodygraph_var::temp ),
        disp_bodygraph_cache( bodygraph_var::encumb ),
        disp_bodygraph_cache( bodygraph_var::status ),
        disp_bodygraph_cache( bodygraph_var::wet )
    }
};

disp_overmap_cache::disp_overmap_cache()
{
    _center = overmap::invalid_tripoint;
    _mission = overmap::invalid_tripoint;
    _width = 0;
}

disp_bodygraph_cache::disp_bodygraph_cache( bodygraph_var var )
{
    _var = var;
    _bp_cur_max.clear();
    _graph_id.clear();
}

bool disp_bodygraph_cache::is_valid_for( const Character &u, const std::string &graph_id ) const
{
    if( graph_id != _graph_id ) {
        return false;
    }
    std::vector<bodypart_id> cur_parts = u.get_all_body_parts( get_body_part_flags::none );
    for( const auto &bp : _bp_cur_max ) {
        if( std::find( cur_parts.begin(), cur_parts.end(), bp.first ) == cur_parts.end() ) {
            // cached bodypart no longer on character
            return false;
        }
    }
    for( const bodypart_id &bp : cur_parts ) {
        auto prev_color = _bp_cur_max.find( bp );
        if( prev_color == _bp_cur_max.end() ) {
            // uncached bodypart
            return false;
        }
        if( prev_color->second != display::get_bodygraph_bp_color( u, bp, _var ).to_int() ) {
            // values differ
            return false;
        }
    }
    return true;
}

void disp_bodygraph_cache::rebuild( const Character &u, const std::string &graph_id,
                                    const std::string &bg_wgt_str )
{
    _bp_cur_max.clear();
    _graph_id = graph_id;
    for( const bodypart_id &bp : u.get_all_body_parts( get_body_part_flags::none ) ) {
        _bp_cur_max.emplace( bp, display::get_bodygraph_bp_color( u, bp, _var ).to_int() );
    }
    _graph_wgt_str = bg_wgt_str;
}

// Get remotely controlled vehicle, or vehicle character is inside of
vehicle *display::vehicle_driven( const Character &u )
{
    vehicle *veh = g->remoteveh();
    if( veh == nullptr && u.in_vehicle ) {
        veh = veh_pointer_or_null( get_map().veh_at( u.pos() ) );
    }
    return veh;
}

std::string display::get_temp( const Character &u )
{
    std::string temp;
    if( u.cache_has_item_with( json_flag_THERMOMETER ) ||
        u.has_flag( STATIC( json_character_flag( "THERMOMETER" ) ) ) ) {
        temp = print_temperature( get_weather().get_temperature( u.pos() ) );
    }
    if( temp.empty() ) {
        return "-";
    }
    return temp;
}

std::string display::time_approx( const time_point &turn )
{
    const int iHour = hour_of_day<int>( turn );
    if( iHour >= 23 || iHour == 0 ) {
        return _( "Around midnight" );
    } else if( is_dawn( turn ) ) {
        return _( "Around dawn" );
    } else if( is_dusk( turn ) ) {
        return _( "Around dusk" );
    } else if( iHour <= 3 && is_night( turn ) ) {
        return _( "Dead of night" );
    } else if( is_night( turn ) ) {
        return _( "Night" );
    } else if( iHour <= 7 ) {
        return _( "Early morning" );
    } else if( iHour <= 10 ) {
        return _( "Morning" );
    } else if( iHour <= 12 ) {
        return _( "Around noon" );
    } else if( iHour <= 16 ) {
        return _( "Afternoon" );
    } else if( iHour <= 18 ) {
        return _( "Early evening" );
    } else if( iHour <= 20 ) {
        return _( "Evening" );
    }
    return _( "Night" );
}

std::string display::time_approx()
{
    return time_approx( calendar::turn );
}

std::string display::date_string()
{
    const std::string season = calendar::name_season( season_of_year( calendar::turn ) );
    const int day_num = day_of_season<int>( calendar::turn ) + 1;
    return string_format( _( "%s, day %d" ), season, day_num );
}

std::string display::time_string( const Character &u )
{
    // Return exact time if character has a watch, or approximate time if can see the sky
    if( u.has_watch() ) {
        return to_string_time_of_day( calendar::turn );
    } else if( is_creature_outside( u ) ) {
        return display::time_approx();
    } else {
        // NOLINTNEXTLINE(cata-text-style): the question mark does not end a sentence
        return _( "???" );
    }
}

std::string display::sundial_time_text_color( const Character &u, int width )
{
    // Return exact time if character has a watch, or approximate time if can see the sky
    if( u.has_watch() ) {
        return to_string_time_of_day( calendar::turn );
    } else if( is_creature_outside( u ) ) {
        return display::sundial_text_color( u, width );
    } else {
        // NOLINTNEXTLINE(cata-text-style): the question mark does not end a sentence
        return _( "???" );
    }
}

std::string display::sundial_text_color( const Character &u, int width )
{
    const std::vector<std::pair<std::string, nc_color> > d_glyphs {
        { "*", c_yellow },
        { "+", c_yellow },
        { ".", c_brown },
        { "_", c_red }
    };
    const std::vector<std::pair<std::string, nc_color> > n_glyphs {
        { "C", c_white },
        { "c", c_light_blue },
        { ",", c_blue },
        { "_", c_cyan }
    };

    auto get_glyph = []( int x, int w, int num_glyphs ) {
        int hw = ( w / 2 ) > 0 ? w / 2 : 1;
        return clamp<int>( ( std::abs( x - hw ) * num_glyphs ) / hw, 0, num_glyphs - 1 );
    };

    std::pair<units::angle, units::angle> sun_pos = sun_azimuth_altitude( calendar::turn );
    const int h = hour_of_day<int>( calendar::turn );
    const int h_dawn = hour_of_day<int>( sunset( calendar::turn ) ) - 12;
    const float light = sun_light_at( calendar::turn );
    float azm = to_degrees( normalize( sun_pos.first + 90_degrees ) );
    if( azm > 270.f ) {
        azm -= 360.f;
    }

    width -= 2;
    const float scale = 180.f / width;
    const int azm_pos = static_cast<int>( std::round( azm / scale ) ) - 1;
    const int night_h = h >= h_dawn + 12 ? h - ( h_dawn + 12 ) : h + ( 12 - h_dawn );
    std::string ret = "[";
    if( g->is_sheltered( u.pos() ) ) {
        ret += ( width > 0 ? std::string( width, '?' ) : "" );
    } else {
        for( int i = 0; i < width; i++ ) {
            std::string ch = " ";
            nc_color clr = c_white;
            int i_dist = std::abs( i - azm_pos );
            float f_dist = ( i_dist * 2 ) / static_cast<float>( width );
            float l_dist = ( f_dist * f_dist * 80.f ) + 30.f;
            if( h >= h_dawn && h < h_dawn + 12 ) {
                // day
                if( i_dist == 0 ) {
                    int glyph = get_glyph( i, width, d_glyphs.size() );
                    ch = d_glyphs[glyph].first;
                    clr = d_glyphs[glyph].second;
                }
            } else {
                // night
                int n_dist = std::abs( ( night_h * width ) / 12 - i );
                if( n_dist == 0 ) {
                    int glyph = get_glyph( i, width, n_glyphs.size() );
                    ch = n_glyphs[glyph].first;
                    clr = n_glyphs[glyph].second;
                }
            }
            if( light > l_dist ) {
                clr = hilite( clr );
            }
            ret += colorize( ch, clr );
        }
    }
    return ret + "]";
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
        if( units::abs( u.get_part_temp_cur( bp ) - BODYTEMP_NORM ) >
            units::abs( u.get_part_temp_cur( current_bp_extreme ) - BODYTEMP_NORM ) ) {
            current_bp_extreme = bp;
        }
        if( units::abs( u.get_part_temp_conv( bp ) - BODYTEMP_NORM ) >
            units::abs( u.get_part_temp_conv( conv_bp_extreme ) - BODYTEMP_NORM ) ) {
            conv_bp_extreme = bp;
        }
    }
    return std::make_pair( current_bp_extreme, conv_bp_extreme );
}

std::pair<std::string, nc_color> display::temp_text_color( const Character &u,
        const bodypart_str_id &bp )
{
    /// Find hottest/coldest bodypart
    // Calculate the most extreme body temperatures
    const bodypart_id current_bp_extreme = bp.is_null() ? temp_delta( u ).first : bp;

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
            c_pwr = c_light_red;
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

std::pair<std::string, nc_color> display::power_balance_text_color( const avatar &u )
{
    nc_color c_pwr = c_red;
    std::string s_pwr;

    units::energy balance = u.power_balance;
    units::energy abs_balance = units::from_millijoule( std::abs( balance.value() ) );

    if( balance < -1_kJ ) {
        c_pwr = c_red;
    } else if( balance <= -10_J ) {
        c_pwr = c_light_red;
    } else if( balance <= -1_mJ ) {
        c_pwr = c_yellow;
    } else if( balance <= 1_mJ ) {
        c_pwr = c_white;
    } else if( balance < 10_J ) {
        c_pwr = c_light_green;
    } else if( balance < 1_kJ ) {
        c_pwr = c_green;
    } else {
        c_pwr = c_light_blue;
    }

    std::string suffix;
    if( balance > 0_kJ ) {
        suffix = "+";
    }

    if( abs_balance < 1_J ) {
        s_pwr = suffix + std::to_string( units::to_millijoule( balance ) ) +
                pgettext( "energy unit: millijoule", "mJ" );
    } else if( abs_balance < 1_kJ ) {
        s_pwr = suffix + std::to_string( units::to_joule( balance ) ) +
                pgettext( "energy unit: joule", "J" );
    } else {
        s_pwr = suffix + std::to_string( units::to_kilojoule( balance ) ) +
                pgettext( "energy unit: kilojoule", "kJ" );
    }

    return std::make_pair( s_pwr, c_pwr );
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
    int thirst = u.get_instant_thirst();
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
            std::forward_as_tuple( effect_hunger_hungry, translate_marker( "Peckish" ), c_light_gray ),
            std::forward_as_tuple( effect_hunger_very_hungry, translate_marker( "Hungry" ), c_yellow ),
            std::forward_as_tuple( effect_hunger_near_starving, translate_marker( "Near starving" ), c_red ),
            std::forward_as_tuple( effect_hunger_starving, translate_marker( "Starving!" ), c_red ),
            std::forward_as_tuple( effect_hunger_famished, translate_marker( "Famished" ), c_light_red )
        }
    };
    for( const auto &hunger_state : hunger_states ) {
        if( u.has_effect( std::get<0>( hunger_state ) ) ) {
            return std::make_pair( _( std::get<1>( hunger_state ) ), std::get<2>( hunger_state ) );
        }
    }
    return std::make_pair( _( "ERROR!" ), c_light_red );
}

std::pair<std::string, nc_color> display::weight_text_color( const Character &u )
{
    const float bmi = u.get_bmi_fat();
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
            weight_color = c_light_gray;
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

std::pair<std::string, nc_color> display::health_text_color( const Character &u )
{
    const int health = u.get_lifestyle();
    std::string health_string;
    nc_color health_color = c_light_gray;

    if( health > character_health_category::great ) {
        health_string = translate_marker( "Feel Great" );
        health_color = c_green;
    } else if( health > character_health_category::very_good ) {
        health_string = translate_marker( "Feel Very Good" );
        health_color = c_green;
    } else if( health > character_health_category::good ) {
        health_string = translate_marker( "Feel Good" );
        health_color = c_green;
    } else if( health > character_health_category::fine ) {
        health_string = translate_marker( "Feel Fine" );
        health_color = c_light_gray;
    } else if( health > character_health_category::bad ) {
        health_string = translate_marker( "Feel Bad" );
        health_color = c_red;
    } else if( health > character_health_category::very_bad ) {
        health_string = translate_marker( "Feel Very Bad" );
        health_color = c_red;
    } else {
        health_string = translate_marker( "Feel Awful" );
        health_color = c_red;
    }
    return std::make_pair( _( health_string ), health_color );
}

std::string display::weight_long_description( const Character &u )
{
    const float bmi = u.get_bmi_fat();
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

std::string display::health_string( const Character &u )
{
    std::pair<std::string, nc_color> health_pair = display::health_text_color( u );
    return colorize( health_pair.first, health_pair.second );
}

std::pair<std::string, nc_color> display::sleepiness_text_color( const Character &u )
{
    int sleepiness = u.get_sleepiness();
    std::string sleepiness_string;
    nc_color sleepiness_color = c_white;
    if( sleepiness >= sleepiness_levels::EXHAUSTED ) {
        sleepiness_color = c_red;
        sleepiness_string = translate_marker( "Exhausted" );
    } else if( sleepiness >= sleepiness_levels::DEAD_TIRED ) {
        sleepiness_color = c_light_red;
        sleepiness_string = translate_marker( "Dead Tired" );
    } else if( sleepiness >= sleepiness_levels::TIRED ) {
        sleepiness_color = c_yellow;
        sleepiness_string = translate_marker( "Tired" );
    }
    return std::make_pair( _( sleepiness_string ), sleepiness_color );
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
    const units::temperature temp_conv = u.get_part_temp_conv( bp );
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
    if( veh ) {
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
    if( veh ) {
        itype_id fuel_type = itype_id::NULL_ID();
        // FIXME: Move this to a vehicle helper function like get_active_engine
        for( const int p : veh->engines ) {
            const vehicle_part &vp = veh->part( p );
            if( veh->is_engine_on( vp )
                && !veh->is_perpetual_type( vp )
                && !veh->is_engine_type( vp, fuel_type_muscle ) ) {
                // Get the fuel type of the first engine that is turned on
                fuel_type = vp.fuel_current();
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

// Single-letter move mode (W, R, C, P)
std::pair<std::string, nc_color> display::move_mode_letter_color( const Character &u )
{
    const std::string mm_text = std::string( 1, u.current_movement_mode()->panel_letter() );
    const nc_color mm_color = u.current_movement_mode()->panel_color();
    return std::make_pair( mm_text, mm_color );
}

// Movement counter and mode letter, like "50(R)" or "100(W)"
std::pair<std::string, nc_color> display::move_count_and_mode_text_color( const avatar &u )
{
    std::pair<std::string, nc_color> mode_pair = display::move_mode_letter_color( u );
    std::string count_and_mode = string_format( "%d(%s)", u.movecounter, mode_pair.first );
    return std::make_pair( count_and_mode, mode_pair.second );
}

// Weight carried, relative to capacity, in %, like "90%".
std::pair<std::string, nc_color> display::carry_weight_text_color( const avatar &u )
{
    int carry_wt = ( 100 * u.weight_carried() ) / u.weight_capacity();
    std::string weight_text = string_format( "%d%%", carry_wt );

    nc_color weight_color = c_green;
    if( carry_wt > 100 ) {
        weight_color = c_white_red;
    } else if( carry_wt > 75 ) {
        weight_color = c_light_red;
    } else if( carry_wt > 50 ) {
        weight_color = c_yellow;
    } else if( carry_wt > 25 ) {
        weight_color = c_light_green;
    } else {
        weight_color = c_green;
    }

    return std::make_pair( weight_text, weight_color );
}

std::pair<std::string, nc_color> display::overmap_note_symbol_color( const std::string_view
        note_text )
{
    std::string ter_sym = "N";
    nc_color ter_color = c_yellow;

    // Parse note text for : (symbol) and ; (color) separators
    int symbolIndex = note_text.find( ':' );
    int colorIndex = note_text.find( ';' );

    bool symbolFirst = symbolIndex < colorIndex;

    // Both color and symbol
    if( colorIndex > -1 && symbolIndex > -1 ) {
        if( symbolFirst ) {
            if( colorIndex > 4 ) {
                colorIndex = -1;
            }
            if( symbolIndex > 1 ) {
                symbolIndex = -1;
                colorIndex = -1;
            }
        } else {
            if( symbolIndex > 4 ) {
                symbolIndex = -1;
            }
            if( colorIndex > 2 ) {
                colorIndex = -1;
            }
        }
    } else if( colorIndex > 2 ) {
        colorIndex = -1;
    } else if( symbolIndex > 1 ) {
        symbolIndex = -1;
    }

    if( symbolIndex > -1 ) {
        int symbolStart = 0;
        if( colorIndex > -1 && !symbolFirst ) {
            symbolStart = colorIndex + 1;
        }
        ter_sym = note_text.substr( symbolStart, symbolIndex - symbolStart );
    }

    if( colorIndex > -1 ) {

        int colorStart = 0;

        if( symbolIndex > -1 && symbolFirst ) {
            colorStart = symbolIndex + 1;
        }

        std::string_view sym = note_text.substr( colorStart, colorIndex - colorStart );

        if( sym.length() == 2 ) {
            if( sym == "br" ) {
                ter_color = c_brown;
            } else if( sym == "lg" ) {
                ter_color = c_light_gray;
            } else if( sym == "dg" ) {
                ter_color = c_dark_gray;
            }
        } else {
            char colorID = sym[0];
            if( colorID == 'r' ) {
                ter_color = c_light_red;
            } else if( colorID == 'R' ) {
                ter_color = c_red;
            } else if( colorID == 'g' ) {
                ter_color = c_light_green;
            } else if( colorID == 'G' ) {
                ter_color = c_green;
            } else if( colorID == 'b' ) {
                ter_color = c_light_blue;
            } else if( colorID == 'B' ) {
                ter_color = c_blue;
            } else if( colorID == 'W' ) {
                ter_color = c_white;
            } else if( colorID == 'C' ) {
                ter_color = c_cyan;
            } else if( colorID == 'c' ) {
                ter_color = c_light_cyan;
            } else if( colorID == 'P' ) {
                ter_color = c_pink;
            } else if( colorID == 'm' ) {
                ter_color = c_magenta;
            }
        }
    }
    return std::make_pair( ter_sym, ter_color );
}

// Return an overmap tile symbol and color for an omt relatively near the avatar's position.
// The edge_tile flag says this omt is at the edge of the map and may point to an off-map mission.
// The found_mi (reference) is set to true to tell the calling function if a mission marker was found.
std::pair<std::string, nc_color> display::overmap_tile_symbol_color( const avatar &u,
        const tripoint_abs_omt &omt, const bool edge_tile, bool &found_mi )
{
    std::string ter_sym;
    nc_color ter_color = c_light_gray;

    // Terrain color and symbol to use for this point
    const bool seen = overmap_buffer.seen( omt );
    if( overmap_buffer.has_note( omt ) ) {
        const std::string &note_text = overmap_buffer.note( omt );
        std::pair<std::string, nc_color> sym_color = display::overmap_note_symbol_color( note_text );
        ter_sym = sym_color.first;
        ter_color = sym_color.second;
    } else if( !seen ) {
        // Always gray # for unseen
        ter_sym = "#";
        ter_color = c_dark_gray;
    } else if( overmap_buffer.has_vehicle( omt ) ) {
        ter_color = c_cyan;
        ter_sym = overmap_buffer.get_vehicle_ter_sym( omt );
    } else {
        // Otherwise, get symbol and color appropriate for the terrain
        const oter_id &cur_ter = overmap_buffer.ter( omt );
        ter_sym = cur_ter->get_symbol();
        if( overmap_buffer.is_explored( omt ) ) {
            ter_color = c_dark_gray;
        } else {
            ter_color = cur_ter->get_color();
        }
    }
    const tripoint_abs_omt target = u.get_active_mission_target();
    const tripoint_abs_omt u_loc = u.global_omt_location();

    // Check if there is a valid mission target, and avatar is not there already
    if( target != overmap::invalid_tripoint && target.xy() != u_loc.xy() ) {
        // highlight it with a red background (if on-map)
        // or point towards it with a red asterisk (if off-map)
        if( target.xy() == omt.xy() ) {
            ter_color = red_background( ter_color );
            found_mi = true;
        } else if( edge_tile ) {
            std::vector<tripoint_abs_omt> plist = line_to( u_loc, target );
            if( std::find( plist.begin(), plist.end(), omt ) != plist.end() ) {
                ter_color = c_red;
                ter_sym = "*";
                found_mi = true;
            }
        }
    }

    // Show hordes on minimap, leaving a one-tile space around the player
    if( std::abs( u_loc.x() - omt.x() ) > 1 || std::abs( u_loc.y() - omt.y() ) > 1 ) {
        const int horde_size = overmap_buffer.get_horde_size( omt );
        const int sight_points = u.overmap_sight_range( g->light_level( u.posz() ) );
        if( horde_size >= HORDE_VISIBILITY_SIZE && overmap_buffer.seen( omt ) &&
            u.overmap_los( omt, sight_points ) ) {
            // Draw green Z or z
            ter_sym = horde_size > HORDE_VISIBILITY_SIZE * 2 ? 'Z' : 'z';
            ter_color = c_green;
        }
    }

    return std::make_pair( ter_sym, ter_color );
}

std::string display::colorized_overmap_text( const avatar &u, const int width, const int height )
{
    std::string overmap_text;
    map &here = get_map();

    // Map is roughly centered around this point
    const tripoint_abs_omt &center_xyz = u.global_omt_location();
    const tripoint_abs_omt &mission_xyz = u.get_active_mission_target();
    // Retrieve cached string instead of constantly rebuilding it
    if( disp_om_cache.is_valid_for( center_xyz, mission_xyz, width ) ) {
        return disp_om_cache.get_val();
    }

    // Remember when mission indicator is found, so we don't draw it more than once
    bool found_mi = false;
    // Figure out extents of the map area, so we know where the edges are
    const int left = -( width / 2 );
    const int right = width + left - 1;
    const int top = -( height / 2 );
    const int bottom = height + top - 1;
    // Scan each row of overmap tiles
    for( int row = top; row <= bottom; row++ ) {
        // Scan across the width of the row
        for( int col = left; col <= right; col++ ) {
            // Is this point along the border of the overmap text area we have to work with?
            // If so, overmap_tile_symbol_color may draw a mission indicator at this point.
            const bool edge = !found_mi && !( mission_xyz.x() >= center_xyz.x() + left &&
                                              mission_xyz.x() <= center_xyz.x() + right &&
                                              mission_xyz.y() >= center_xyz.y() + top &&
                                              mission_xyz.y() <= center_xyz.y() + bottom ) &&
                              ( row == top || row == bottom || col == left || col == right );
            // Get colorized symbol for this point
            const tripoint_abs_omt omt( center_xyz.xy() + point( col, row ), here.get_abs_sub().z() );
            std::pair<std::string, nc_color> sym_color = display::overmap_tile_symbol_color( u, omt, edge,
                    found_mi );

            // Highlight player character location in the center
            if( row == 0 && col == 0 ) {
                sym_color.second = hilite( sym_color.second );
            }

            // Append the colorized symbol for this point to the map
            overmap_text += colorize( sym_color.first, sym_color.second );
        }
        overmap_text += "\n";
    }

    // Rebuild the cache so we can reuse it if nothing changes
    disp_om_cache.rebuild( center_xyz, mission_xyz, width, overmap_text );

    return overmap_text;
}

std::string display::overmap_position_text( const tripoint_abs_omt &loc )
{
    point_abs_omt abs_omt = loc.xy();
    point_abs_om om;
    point_om_omt omt;
    std::tie( om, omt ) = project_remain<coords::om>( abs_omt );
    return string_format( _( "LEVEL %i, %d'%d, %d'%d" ), loc.z(), om.x(), omt.x(), om.y(), omt.y() );
}

std::string display::current_position_text( const tripoint_abs_omt &loc )
{
    if( const timed_event *e = get_timed_events().get( timed_event_type::OVERRIDE_PLACE ) ) {
        return e->string_id;
    }
    return overmap_buffer.ter( loc )->get_name();
}

// Return (x, y) position of mission target, relative to avatar location, within an overmap of the
// given width and height.
point display::mission_arrow_offset( const avatar &you, int width, int height )
{
    // FIXME: Use tripoint for curs
    const point_abs_omt curs = you.global_omt_location().xy();
    const tripoint_abs_omt targ = you.get_active_mission_target();
    const point mid( width / 2, height / 2 );

    // If x-coordinates are the same, mission is either due north or due south
    // Use an extreme slope rather than dividing by zero
    double slope = curs.x() == targ.x() ? 1000 :
                   static_cast<double>( targ.y() - curs.y() ) / ( targ.x() - curs.x() );

    if( std::fabs( slope ) > 12 ) {
        // For any near-vertical slope, center the marker
        if( targ.y() > curs.y() ) {
            // Target is due south
            return point( mid.x, height - 1 );
            //mvwputch( w_minimap, point( mid.x + start_x, height - 1 + start_y ), c_red, '*' );
        } else if( targ.y() < curs.y() ) {
            // Target is due north
            return point( mid.x, 1 );
            //mvwputch( w_minimap, point( mid.x + start_x, 1 + start_y ), c_red, '*' );
        } else {
            // Target is right here
            return mid;
        }
    } else {
        // For non-vertical slope, calculate where it intersects the edge of the map
        point arrow( point_north_west );
        if( std::fabs( slope ) >= 1. ) {
            // If target to the north or south, arrow on top or bottom edge of minimap
            if( targ.y() > curs.y() ) {
                arrow.x = static_cast<int>( ( 1. + ( 1. / slope ) ) * mid.x );
                arrow.y = height - 1;
            } else {
                arrow.x = static_cast<int>( ( 1. - ( 1. / slope ) ) * mid.x );
                arrow.y = 0;
            }
            // Clip to left/right edges
            arrow.x = std::max( arrow.x, 0 );
            arrow.x = std::min( arrow.x, width - 1 );
        } else {
            // If target to the east or west, arrow on left or right edge of minimap
            if( targ.x() > curs.x() ) {
                arrow.x = width - 1;
                arrow.y = static_cast<int>( ( 1. + slope ) * mid.y );
            } else {
                arrow.x = 0;
                arrow.y = static_cast<int>( ( 1. - slope ) * mid.y );
            }
            // Clip to top/bottom edges
            arrow.y = std::max( arrow.y, 0 );
            arrow.y = std::min( arrow.y, height - 1 );
        }
        return arrow;
        //mvwputch( w_minimap, arrow + point( start_x, start_y ), c_red, glyph );
    }
}

static std::string get_compass_for_direction( const cardinal_direction dir, int max_width )
{
    const int d = static_cast<int>( dir );
    const monster_visible_info &mon_visible = get_avatar().get_mon_visible();
    std::vector<std::pair<std::string, nc_color>> syms;
    for( npc *n : mon_visible.unique_types[d] ) {
        switch( n->get_attitude() ) {
            case NPCATT_KILL:
                syms.emplace_back( "@", c_red );
                break;
            case NPCATT_FOLLOW:
                syms.emplace_back( "@", c_light_green );
                break;
            default:
                syms.emplace_back( "@", c_pink );
                break;
        }
    }
    for( const std::pair<const mtype *, int> &m : mon_visible.unique_mons[d] ) {
        syms.emplace_back( m.first->sym, m.first->color );
    }

    std::string ret;
    for( int i = 0; i < static_cast<int>( syms.size() ); i++ ) {
        if( i >= max_width - 1 ) {
            ret += colorize( "+", c_white );
            break;
        }
        ret += colorize( syms[i].first, syms[i].second );
    }
    return ret;
}

std::string display::colorized_compass_text( const cardinal_direction dir, int width )
{
    if( dir == cardinal_direction::num_cardinal_directions ) {
        return "";
    }
    return get_compass_for_direction( dir, width );
}

std::string display::colorized_compass_legend_text( int width, int max_height, int &height )
{
    const monster_visible_info &mon_visible = get_avatar().get_mon_visible();
    std::vector<std::string> names;
    for( const std::vector<npc *> &nv : mon_visible.unique_types ) {
        for( const npc *n : nv ) {
            std::string name;
            switch( n->get_attitude() ) {
                case NPCATT_KILL:
                    name = colorize( "@", c_red );
                    break;
                case NPCATT_FOLLOW:
                    name = colorize( "@", c_light_green );
                    break;
                default:
                    name = colorize( "@", c_pink );
                    break;
            }
            name = string_format( "%s %s", name, n->name );
            names.emplace_back( name );
        }
    }
    std::map<const mtype *, int> mlist;
    for( const auto &mv : mon_visible.unique_mons ) {
        for( const std::pair<const mtype *, int> &m : mv ) {
            mlist[m.first] += m.second;
        }
    }
    for( const auto &m : mlist ) {
        nc_color danger = c_dark_gray;
        if( m.first->difficulty >= 30 ) {
            danger = c_red;
        } else if( m.first->difficulty >= 16 ) {
            danger = c_light_red;
        } else if( m.first->difficulty >= 8 ) {
            danger = c_white;
        } else if( m.first->agro > 0 ) {
            danger = c_light_gray;
        }
        std::string name = m.second > 1 ? string_format( "%d ", m.second ) : "";
        name += m.first->nname( m.second );
        name = string_format( "%s %s", colorize( m.first->sym, m.first->color ), colorize( name, danger ) );
        names.emplace_back( name );
    }
    return format_widget_multiline( names, max_height, width, height );
}

static std::pair<std::string, nc_color> get_bodygraph_bp_sym_color( const Character &u,
        const bodygraph_part &bgp, const bodygraph_var var )
{
    const bodypart_id &bid = bgp.sub_bodyparts.empty() ?
                             bgp.bodyparts.front() : bgp.sub_bodyparts.front()->parent.id();
    return { bgp.sym, display::get_bodygraph_bp_color( u, bid, var ) };
}

nc_color display::get_bodygraph_bp_color( const Character &u, const bodypart_id &bid,
        const bodygraph_var var )
{
    if( !u.has_part( bid, body_part_filter::equivalent ) ) {
        return c_black; // character is missing this part
    }

    switch( var ) {
        case bodygraph_var::hp: {
            const int cur_hp = u.get_part_hp_cur( bid );
            const int max_hp = u.get_part_hp_max( bid );
            const float cur_hp_pcnt = cur_hp / static_cast<float>( max_hp );
            if( cur_hp_pcnt < 0.125f ) {
                return c_red;
            } else if( cur_hp_pcnt < 0.375f ) {
                return c_light_red;
            } else if( cur_hp_pcnt < 0.625f ) {
                return c_yellow;
            } else if( cur_hp_pcnt < 0.875f ) {
                return c_light_green;
            }
            return c_green;
        }
        case bodygraph_var::temp: {
            return display::bodytemp_color( u, bid );
        }
        case bodygraph_var::encumb: {
            int level = u.get_part_encumbrance_data( bid ).encumbrance;
            return display::encumb_color( level );
        }
        case bodygraph_var::status: {
            return display::limb_color( u, bid, true, true, true );
        }
        case bodygraph_var::wet: {
            const int cur_wet = u.get_part_wetness( bid );
            if( cur_wet == 0 ) {
                return c_light_gray; // dry
            } else {
                const float cur_wet = u.get_part_wetness_percentage( bid );
                if( cur_wet < BODYWET_PERCENT_WET ) {
                    return c_light_cyan;
                } else if( cur_wet < BODYWET_PERCENT_SOAKED ) {
                    return c_light_blue;
                } else {
                    return c_blue; // maximum wetness
                }
            }
        }
        // Fall-through - invalid
        case bodygraph_var::last:
            break;
    }
    cata_fatal( "Invalid widget_var" );
}

std::string display::colorized_bodygraph_text( const Character &u, const std::string &graph_id,
        const bodygraph_var var, int width, int max_height, int &height )
{
    int var_idx = int( var );

    if( disp_bg_cache[var_idx].is_valid_for( u, graph_id ) ) {
        // Nothing changed, just retrieve from cache
        return disp_bg_cache[var_idx].get_val();
    }

    bodygraph_id graph( graph_id );
    if( graph.is_null() || !graph.is_valid() || graph->rows.empty() ) {
        height = 1;
        return "";
    }

    auto process_sym = [&u, var]( const bodygraph_part * bgp, const std::string & sym ) {
        if( !bgp ) {
            return sym;
        }
        std::pair<std::string, nc_color> sym_col = get_bodygraph_bp_sym_color( u, *bgp, var );
        return colorize( sym, sym_col.second );
    };

    std::vector<std::string> rows = get_bodygraph_lines( u, process_sym, graph, width, max_height );
    height = rows.size();

    std::string ret;
    std::string sep;
    for( const std::string &row : rows ) {
        ret.append( sep );
        ret.append( row );
        sep = "\n";
    }

    // Rebuild bodygraph text cache
    disp_bg_cache[var_idx].rebuild( u, graph_id, ret );

    return ret;
}

std::pair<std::string, nc_color> display::weather_text_color( const Character &u )
{
    if( u.pos().z < 0 ) {
        return std::make_pair( _( "Underground" ), c_light_gray );
    } else {
        weather_manager &weather = get_weather();
        std::string weather_text = weather.weather_id->name.translated();
        nc_color weather_color = weather.weather_id->color;
        return std::make_pair( weather_text, weather_color );
    }
}

std::pair<std::string, nc_color> display::wind_text_color( const Character &u )
{
    const oter_id &cur_om_ter = overmap_buffer.ter( u.global_omt_location() );
    weather_manager &weather = get_weather();
    double windpower = get_local_windpower( weather.windspeed, cur_om_ter,
                                            u.get_location(), weather.winddirection, g->is_sheltered( u.pos() ) );

    // Wind descriptor followed by a directional arrow
    const std::string wind_text = get_wind_desc( windpower ) + " " + get_wind_arrow(
                                      weather.winddirection );

    return std::make_pair( wind_text, get_wind_color( windpower ) );
}

