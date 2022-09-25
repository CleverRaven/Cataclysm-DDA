#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>

#include "addiction.h"
#include "avatar.h"
#include "bionics.h"
#include "bodygraph.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "character.h"
#include "character_modifier.h"
#include "color.h"
#include "cursesdef.h"
#include "debug.h"
#include "display.h"
#include "effect.h"
#include "enum_conversions.h"
#include "game.h"
#include "input.h"
#include "localized_comparator.h"
#include "mutation.h"
#include "options.h"
#include "output.h"
#include "pimpl.h"
#include "profession.h"
#include "proficiency.h"
#include "skill.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "ui_manager.h"
#include "units.h"
#include "units_utility.h"
#include "weather.h"
#include "weather_type.h"

static const bionic_id bio_cqb( "bio_cqb" );

static const json_character_flag json_flag_ECTOTHERM( "ECTOTHERM" );

static const skill_id skill_bashing( "bashing" );
static const skill_id skill_cutting( "cutting" );
static const skill_id skill_dodge( "dodge" );
static const skill_id skill_melee( "melee" );
static const skill_id skill_stabbing( "stabbing" );
static const skill_id skill_unarmed( "unarmed" );

static const trait_id trait_SUNLIGHT_DEPENDENT( "SUNLIGHT_DEPENDENT" );
static const trait_id trait_TROGLO( "TROGLO" );
static const trait_id trait_TROGLO2( "TROGLO2" );
static const trait_id trait_TROGLO3( "TROGLO3" );

static const std::string title_STATS = translate_marker( "STATS" );
static const std::string title_ENCUMB = translate_marker( "ENCUMBRANCE AND WARMTH" );
static const std::string title_EFFECTS = translate_marker( "EFFECTS" );
static const std::string title_SPEED = translate_marker( "SPEED" );
static const std::string title_SKILLS = translate_marker( "SKILLS" );
static const std::string title_BIONICS = translate_marker( "BIONICS" );
static const std::string title_TRAITS = translate_marker( "TRAITS" );
static const std::string title_PROFICIENCIES = translate_marker( "PROFICIENCIES" );

// use this instead of having to type out 26 spaces like before
static const std::string header_spaces( 26, ' ' );
static const unsigned int grid_width = 26;

// Rescale temperature value to one that the player sees
static int temperature_print_rescaling( int temp )
{
    return ( temp / 100.0 ) * 2 - 100;
}

static bool should_combine_bps( const Character &p, const bodypart_id &l, const bodypart_id &r,
                                const item *selected_clothing )
{
    const encumbrance_data &enc_l = p.get_part_encumbrance_data( l );
    const encumbrance_data &enc_r = p.get_part_encumbrance_data( r );

    return l != r && // are different parts
           l ==  r->opposite_part && r == l->opposite_part && // are complementary parts
           // same encumbrance & temperature
           enc_l == enc_r &&
           temperature_print_rescaling( p.get_part_temp_conv( l ) ) == temperature_print_rescaling(
               p.get_part_temp_conv( r ) ) &&
           // selected_clothing covers both or neither parts
           ( !selected_clothing || ( selected_clothing->covers( l ) == selected_clothing->covers( r ) ) ) &&
           // they have the same HP
           p.get_part_hp_cur( l ) == p.get_part_hp_cur( r );

}

static std::vector<std::pair<bodypart_id, bool>> list_and_combine_bps( const Character &p,
        const item *selected_clothing )
{
    // bool represents whether the part has been combined with its other half
    std::vector<std::pair<bodypart_id, bool>> bps;
    for( const bodypart_id &bp : p.get_all_body_parts( get_body_part_flags::sorted ) ) {
        // assuming that a body part has at most one other half
        if( should_combine_bps( p, bp, bp->opposite_part.id(), selected_clothing ) ) {
            if( std::find( bps.begin(), bps.end(), std::pair<bodypart_id, bool>( bp->opposite_part.id(),
                           true ) ) == bps.end() ) {
                // only add one
                bps.emplace_back( bp, true );
            }
        } else {
            bps.emplace_back( bp, false );
        }
    }
    return bps;
}

static std::pair<int, int> subindex_around_cursor(
    const int num_entries, const int available_space, const int cursor_pos, const bool focused )
/**
 * Return indexes [start, end) that should be displayed from list long `num_entries`,
 * given that cursor is at position `cursor_pos` and we have `available_space` spaces.
 *
 * Example:
 * num_entries = 6, available_space = 3, cursor_pos = 2, focused = true;
 * so choose 3 from indexes [0, 1, 2, 3, 4, 5]
 * return {1, 4}
 */
{
    if( !focused || num_entries <= available_space ) {
        return std::make_pair( 0, std::min( available_space, num_entries ) );
    }
    int slice_start = std::min( std::max( 0, cursor_pos - available_space / 2 ),
                                num_entries - available_space );
    return std::make_pair( slice_start, slice_start + available_space );
}

void Character::print_encumbrance( ui_adaptor &ui, const catacurses::window &win,
                                   const int line, const item *const selected_clothing ) const
{
    // bool represents whether the part has been combined with its other half
    const std::vector<std::pair<bodypart_id, bool>> bps = list_and_combine_bps( *this,
            selected_clothing );

    // width/height excluding title & scrollbar
    const int height = getmaxy( win ) - 1;
    const bool do_draw_scrollbar = height < static_cast<int>( bps.size() );
    const int width = getmaxx( win ) - ( do_draw_scrollbar ? 1 : 0 );
    // index of the first printed bodypart from `bps`
    const int firstline = clamp( line - height / 2, 0, std::max( 0,
                                 static_cast<int>( bps.size() ) - height ) );

    /*** I chose to instead only display X+Y instead of X+Y=Z. More room was needed ***
     *** for displaying triple digit encumbrance, due to new encumbrance system.    ***
     *** If the player wants to see the total without having to do them maths, the  ***
     *** armor layers ui shows everything they want :-) -Davek                      ***/
    for( int i = 0; i < height; ++i ) {
        int thisline = firstline + i;
        if( thisline < 0 ) {
            continue;
        }
        if( static_cast<size_t>( thisline ) >= bps.size() ) {
            break;
        }

        const bodypart_id &bp = bps[thisline].first;
        const bool combine = bps[thisline].second;
        const encumbrance_data &e = get_part_encumbrance_data( bp );

        const bool highlighted = selected_clothing ? selected_clothing->covers( bp ) : false;
        std::string out = body_part_name_as_heading( bp, combine ? 2 : 1 );
        if( utf8_width( out ) > 7 ) {
            out = utf8_truncate( out, 7 );
        }

        // Two different highlighting schemes, highlight if the line is selected as per line being set.
        // Make the text green if this part is covered by the passed in item.
        const bool highlight_line = thisline == line;
        nc_color limb_color = highlight_line ?
                              ( highlighted ? h_green : h_light_gray ) :
                              ( highlighted ? c_green : c_light_gray );
        const int y_pos = 1 + i;
        if( highlight_line ) {
            ui.set_cursor( win, point( 1, y_pos ) );
        }
        mvwprintz( win, point( 1, y_pos ), limb_color, "%s", out );
        // accumulated encumbrance from clothing, plus extra encumbrance from layering
        int column = std::max( 10, ( width / 2 ) - 3 ); //Ideally the encumbrance data is centred
        mvwprintz( win, point( column, y_pos ), display::encumb_color( e.encumbrance ), "%3d",
                   e.encumbrance - e.layer_penalty );
        // separator in low toned color
        column += 3; //Prepared for 3-digit encumbrance
        mvwprintz( win, point( column, y_pos ), c_light_gray, "+" );
        column += 1; // "+"
        // take into account the new encumbrance system for layers
        mvwprintz( win, point( column, y_pos ), display::encumb_color( e.encumbrance ), "%-3d",
                   e.layer_penalty );
        // print warmth, tethered to right hand side of the window
        mvwprintz( win, point( width - 6, y_pos ), display::bodytemp_color( *this, bp ), "(% 3d)",
                   temperature_print_rescaling( get_part_temp_conv( bp ) ) );
    }

    if( do_draw_scrollbar ) {
        draw_scrollbar( win, firstline, height, bps.size(), point( width, 1 ), c_white, true );
    }
}

static nc_color limb_score_current_color( float cur_score, float bp_score )
{
    if( cur_score < bp_score * 0.25 ) {
        return c_brown;
    }
    if( cur_score < bp_score * 0.5 ) {
        return c_light_red;
    }
    if( cur_score < bp_score * 0.8 ) {
        return c_yellow;
    }
    return c_white;
}

static std::string get_score_text( const std::string &sc_name, float cur_score, float bp_score )
{
    if( std::abs( bp_score ) <= std::numeric_limits<float>::epsilon() ) {
        return std::string();
    }

    nc_color score_c = limb_score_current_color( cur_score, bp_score );
    std::string sc_txt = colorize( string_format( "%.2f (%.f%%)", cur_score,
                                   cur_score * 100.f / bp_score ), score_c );
    //~ 1$ = name of the limb score (ex: Balance), 2$ = current score value (colored)
    return string_format( _( "%1$s score: %2$s" ), sc_name, sc_txt );
}

static std::vector<std::string> get_encumbrance_description( const Character &you,
        const bodypart_id &bp )
{
    std::vector<std::string> s;
    const bodypart *part = you.get_part( bp );
    if( !bp->encumb_text.empty() ) {
        s.emplace_back( colorize( string_format( _( "Encumbrance effects: %s" ), bp->encumb_text ),
                                  c_magenta ) );
    }
    if( bp->encumb_impacts_dodge && you.is_avatar() ) {
        int dodge = get_avatar().limb_dodge_encumbrance();
        nc_color dodge_c = dodge > 10 ? c_light_red : dodge > 5 ? c_yellow : c_white;
        std::string dodge_str = colorize( string_format( "-%d", dodge ), dodge_c );
        s.emplace_back( string_format( _( "Encumbrance dodge modifier: %s" ), dodge_str ) );
    }
    for( const limb_score &sc : limb_score::get_all() ) {
        if( !bp->has_limb_score( sc.getId() ) ) {
            continue;
        }
        float cur_score = part->get_limb_score( sc.getId() );
        float bp_score = bp->get_limb_score( sc.getId() );
        s.emplace_back( get_score_text( sc.name().translated(), cur_score, bp_score ) );
    }
    for( const character_modifier &mod : character_modifier::get_all() ) {
        for( const auto &sc : mod.use_limb_scores() ) {
            if( sc.second == 0.0f || sc.first.is_null() || !bp->has_limb_score( sc.first ) ) {
                continue;
            }
            std::string desc = mod.description().translated();
            std::string valstr = colorize( string_format( "%.2f", mod.modifier( you ) ),
                                           limb_score_current_color( part->get_limb_score( sc.first ) * sc.second,
                                                   bp->get_limb_score( sc.first ) * sc.second ) );
            s.emplace_back( string_format( "%s: %s%s", desc, mod.mod_type_str(), valstr ) );
        }
    }
    return s;
}

static bool is_cqb_skill( const skill_id &id )
{
    // TODO: this skill list here is used in other places as well. Useless redundancy and
    // dependency. Maybe change it into a flag of the skill that indicates it's a skill used
    // by the bionic?
    static const std::array<skill_id, 5> cqb_skills = { {
            skill_melee, skill_unarmed, skill_cutting,
            skill_bashing, skill_stabbing,
        }
    };
    return std::find( cqb_skills.begin(), cqb_skills.end(), id ) != cqb_skills.end();
}

namespace
{
enum class player_display_tab : int {
    stats,
    encumbrance,
    skills,
    traits,
    bionics,
    effects,
    proficiencies,
    num_tabs,
};
} // namespace

static player_display_tab next_tab( const player_display_tab tab )
{
    if( static_cast<int>( tab ) + 1 < static_cast<int>( player_display_tab::num_tabs ) ) {
        return static_cast<player_display_tab>( static_cast<int>( tab ) + 1 );
    } else {
        return static_cast<player_display_tab>( 0 );
    }
}

static player_display_tab prev_tab( const player_display_tab tab )
{
    if( static_cast<int>( tab ) > 0 ) {
        return static_cast<player_display_tab>( static_cast<int>( tab ) - 1 );
    } else {
        return static_cast<player_display_tab>( static_cast<int>( player_display_tab::num_tabs ) - 1 );
    }
}

static void draw_proficiencies_tab( ui_adaptor &ui, const catacurses::window &win,
                                    const unsigned line, const Character &guy,
                                    const player_display_tab curtab, const input_context &ctxt )
{
    werase( win );
    const std::vector<display_proficiency> profs = guy.display_proficiencies();
    const bool focused = curtab == player_display_tab::proficiencies;
    const nc_color title_color = focused ? h_light_gray : c_light_gray;
    if( focused ) {
        ui.set_cursor( win, point_zero );
    }
    center_print( win, 0, title_color, string_format( "[<color_yellow>%s</color>] %s",
                  ctxt.get_desc( "VIEW_PROFICIENCIES" ), _( title_PROFICIENCIES ) ) );

    const int height = getmaxy( win ) - 1;
    const bool do_draw_scrollbar = height < static_cast<int>( profs.size() );
    const int width = getmaxx( win ) - 1 - ( do_draw_scrollbar ? 1 : 0 );  // -1 for beginning space

    const std::pair<const int, const int> range = subindex_around_cursor( profs.size(), height, line,
            focused );
    for( size_t i = range.first; i < static_cast<size_t>( range.second ); ++i ) {
        std::string name;
        const display_proficiency &cur = profs[i];
        if( !cur.known && cur.id->can_learn() ) {
            name = string_format( "%s %2.0f%%",
                                  left_justify( trim_by_length( cur.id->name(), width - 4 ), width - 4 ),
                                  std::floor( cur.practice * 100 ) );
        } else {
            name = trim_by_length( cur.id->name(), width );
        }
        const bool highlight_line = focused && i == line;
        const nc_color col = highlight_line ? hilite( cur.color ) : cur.color;
        nc_color col_cur = col;  // make non const copy
        const point pos( 1, 1 + i - range.first );
        if( highlight_line ) {
            ui.set_cursor( win, pos );
        }
        print_colored_text( win, pos, col_cur, col, name );
    }
    if( do_draw_scrollbar ) {
        draw_scrollbar( win, range.first, height, profs.size(), point( width + 1, 1 ), c_white, true );
    }
    wnoutrefresh( win );
}

static void draw_proficiencies_info( const catacurses::window &w_info, const unsigned line,
                                     const Character &guy )
{
    werase( w_info );
    const std::vector<display_proficiency> profs = guy.display_proficiencies();
    if( line < profs.size() ) {
        const display_proficiency &cur = profs[line];
        std::string progress;
        if( cur.known ) {
            progress = _( "You know this proficiency." );
        } else {
            progress = string_format( _( "You are %2.1f%% of the way towards learning this proficiency." ),
                                      cur.practice * 100 );
            if( debug_mode ) {
                progress += string_format( "\nYou have spent %s practicing this proficiency.",
                                           to_string( cur.spent ) );
            }
        }
        int y = 0;
        y += fold_and_print( w_info, point( 1, y ), getmaxx( w_info ) - 1, cur.color, cur.id->name() );
        y += fold_and_print( w_info, point( 1, y ), getmaxx( w_info ) - 1, c_cyan, progress );
        fold_and_print( w_info, point( 1, y ), getmaxx( w_info ) - 1, c_white, cur.id->description() );
    }
    wnoutrefresh( w_info );
}

static void draw_stats_tab( ui_adaptor &ui, const catacurses::window &w_stats, const Character &you,
                            const unsigned line, const player_display_tab curtab, const input_context &ctxt )
{
    werase( w_stats );
    const bool is_current_tab = curtab == player_display_tab::stats;
    const nc_color title_col = is_current_tab ? h_light_gray : c_light_gray;
    if( is_current_tab ) {
        ui.set_cursor( w_stats, point_zero );
    }
    center_print( w_stats, 0, title_col,
                  string_format( "[<color_yellow>%s</color>] %s",
                                 ctxt.get_desc( "VIEW_BODYSTAT" ), _( title_STATS ) ) );

    const auto highlight_line = [is_current_tab, line]( const unsigned line_to_draw ) {
        return is_current_tab && line == line_to_draw;
    };

    const auto line_color = [&highlight_line]( const unsigned line_to_draw ) {
        if( highlight_line( line_to_draw ) ) {
            return h_light_gray;
        } else {
            return c_light_gray;
        }
    };

    const auto set_highlight_cursor = [&highlight_line, &ui, &w_stats]
    ( const unsigned line_to_draw ) {
        if( highlight_line( line_to_draw ) ) {
            ui.set_cursor( w_stats, point( 1, line_to_draw + 1 ) );
        }
    };

    // Stats
    const auto display_stat = [&line_color, &set_highlight_cursor, &w_stats]
    ( const char *name, const int cur, const int max, const unsigned line_to_draw ) {
        nc_color cstatus;
        if( cur <= 0 ) {
            cstatus = c_dark_gray;
        } else if( cur < max / 2 ) {
            cstatus = c_red;
        } else if( cur < max ) {
            cstatus = c_light_red;
        } else if( cur == max ) {
            cstatus = c_white;
        } else if( cur < max * 1.5 ) {
            cstatus = c_light_green;
        } else {
            cstatus = c_green;
        }

        set_highlight_cursor( line_to_draw );
        mvwprintz( w_stats, point( 1, line_to_draw + 1 ), line_color( line_to_draw ), name );
        mvwprintz( w_stats, point( 18, line_to_draw + 1 ), cstatus, "%2d", cur );
        mvwprintz( w_stats, point( 21, line_to_draw + 1 ), c_light_gray, "(%2d)", max );
    };

    display_stat( _( "Strength:" ), you.get_str(), you.get_str_base(), 0 );
    display_stat( _( "Dexterity:" ), you.get_dex(), you.get_dex_base(), 1 );
    display_stat( _( "Intelligence:" ), you.get_int(), you.get_int_base(), 2 );
    display_stat( _( "Perception:" ), you.get_per(), you.get_per_base(), 3 );

    set_highlight_cursor( 4 );
    mvwprintz( w_stats, point( 1, 5 ), line_color( 4 ), _( "Weight:" ) );
    right_print( w_stats, 5, 1, c_light_gray, display::weight_string( you ) );

    set_highlight_cursor( 5 );
    mvwprintz( w_stats, point( 1, 6 ), line_color( 5 ), _( "Lifestyle:" ) );
    right_print( w_stats, 6, 1, c_light_gray, display::health_string( you ) );

    set_highlight_cursor( 6 );
    mvwprintz( w_stats, point( 1, 7 ), line_color( 6 ), _( "Height:" ) );
    mvwprintz( w_stats, point( 25 - utf8_width( you.height_string() ), 7 ), c_light_gray,
               you.height_string() );

    set_highlight_cursor( 7 );
    mvwprintz( w_stats, point( 1, 8 ), line_color( 7 ), _( "Age:" ) );
    mvwprintz( w_stats, point( 25 - utf8_width( you.age_string() ), 8 ), c_light_gray,
               you.age_string() );

    set_highlight_cursor( 8 );
    mvwprintz( w_stats, point( 1, 9 ), line_color( 8 ), _( "Blood type:" ) );
    mvwprintz( w_stats, point( 25 - utf8_width( io::enum_to_string( you.my_blood_type ) +
                               ( you.blood_rh_factor ? "+" : "-" ) ), 9 ),
               c_light_gray,
               io::enum_to_string( you.my_blood_type ) + ( you.blood_rh_factor ? "+" : "-" ) );

    wnoutrefresh( w_stats );
}

static void draw_stats_info( const catacurses::window &w_info, const Character &you,
                             const unsigned line )
{
    werase( w_info );
    nc_color col_temp = c_light_gray;

    if( line == 0 ) {
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        fold_and_print( w_info, point( 1, 0 ), FULL_SCREEN_WIDTH - 2, c_magenta,
                        _( "Strength affects your melee damage, the amount of weight you can carry, your total HP, "
                           "your resistance to many diseases, and the effectiveness of actions which require brute force." ) );
        print_colored_text( w_info, point( 1, 3 ), col_temp, c_light_gray,
                            string_format( _( "Base HP: <color_white>%d</color>" ),
                                           you.get_part_hp_max( you.get_root_body_part() ) ) );
        print_colored_text( w_info, point( 1, 4 ), col_temp, c_light_gray,
                            string_format( _( "Carry weight (%s): <color_white>%.1f</color>" ), weight_units(),
                                           convert_weight( you.weight_capacity() ) ) );
        print_colored_text( w_info, point( 1, 5 ), col_temp, c_light_gray,
                            string_format( _( "Bash damage: <color_white>%.1f</color>" ), you.bonus_damage( false ) ) );
    } else if( line == 1 ) {
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        fold_and_print( w_info, point( 1, 0 ), FULL_SCREEN_WIDTH - 2, c_magenta,
                        _( "Dexterity affects your chance to hit in melee combat, helps you steady your "
                           "gun for ranged combat, and enhances many actions that require finesse." ) );
        print_colored_text( w_info, point( 1, 3 ), col_temp, c_light_gray,
                            string_format( _( "Melee to-hit bonus: <color_white>%+.1lf</color>" ), you.get_melee_hit_base() ) );
        print_colored_text( w_info, point( 1, 4 ), col_temp, c_light_gray,
                            string_format( _( "Ranged penalty: <color_white>%+d</color>" ),
                                           -std::abs( you.ranged_dex_mod() ) ) );
        print_colored_text( w_info, point( 1, 5 ), col_temp, c_light_gray,
                            string_format( _( "Throwing penalty per target's dodge: <color_white>%+d</color>" ),
                                           you.throw_dispersion_per_dodge( false ) ) );
    } else if( line == 2 ) {
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        fold_and_print( w_info, point( 1, 0 ), FULL_SCREEN_WIDTH - 2, c_magenta,
                        _( "Intelligence is less important in most situations, but it is vital for more complex tasks like "
                           "electronics crafting.  It also affects how much skill you can pick up from reading a book." ) );
        print_colored_text( w_info, point( 1, 4 ), col_temp, c_light_gray,
                            string_format( _( "Read times: <color_white>%d%%</color>" ), you.read_speed() ) );
        print_colored_text( w_info, point( 1, 5 ), col_temp, c_light_gray,
                            string_format( _( "Crafting bonus: <color_white>%d%%</color>" ), you.get_int() ) );
    } else if( line == 3 ) {
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        fold_and_print( w_info, point( 1, 0 ), FULL_SCREEN_WIDTH - 2, c_magenta,
                        _( "Perception is the most important stat for ranged combat.  It's also used for "
                           "detecting traps and other things of interest." ) );
        print_colored_text( w_info, point( 1, 4 ), col_temp, c_light_gray,
                            string_format( _( "Trap detection level: <color_white>%d</color>" ), you.get_per() ) );
        if( you.ranged_per_mod() > 0 ) {
            print_colored_text( w_info, point( 1, 5 ), col_temp, c_light_gray,
                                string_format( _( "Aiming penalty: <color_white>%+d</color>" ), -you.ranged_per_mod() ) );
        }
    } else if( line == 4 ) {
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        const int lines = fold_and_print( w_info, point( 1, 0 ), FULL_SCREEN_WIDTH - 2, c_magenta,
                                          _( "Your weight is a general indicator of how much fat your body has stored up,"
                                             " which in turn shows how prepared you are to survive for a time without food."
                                             "  Having too much, or too little, can be unhealthy." ) );
        fold_and_print( w_info, point( 1, 1 + lines ), FULL_SCREEN_WIDTH - 2, c_light_gray,
                        display::weight_long_description( you ) );
    } else if( line == 5 ) {
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        fold_and_print( w_info, point( 1, 0 ), FULL_SCREEN_WIDTH - 2, c_magenta,
                        _( "How healthy you feel.  Excercise, vitamins, sleep and not ingesting poison will increase this overtime." ) );
    } else if( line == 6 ) {
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        const int lines = fold_and_print( w_info, point( 1, 0 ), FULL_SCREEN_WIDTH - 2, c_magenta,
                                          _( "Your height.  Simply how tall you are." ) );
        fold_and_print( w_info, point( 1, 1 + lines ), FULL_SCREEN_WIDTH - 2, c_light_gray,
                        you.height_string() );
    } else if( line == 7 ) {
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        const int lines = fold_and_print( w_info, point( 1, 0 ), FULL_SCREEN_WIDTH - 2, c_magenta,
                                          _( "This is how old you are." ) );
        fold_and_print( w_info, point( 1, 1 + lines ), FULL_SCREEN_WIDTH - 2, c_light_gray,
                        you.age_string() );
    } else if( line == 8 ) {
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        const int lines = fold_and_print( w_info, point( 1, 0 ), FULL_SCREEN_WIDTH - 2, c_magenta,
                                          _( "This is your blood type and Rh factor." ) );
        fold_and_print( w_info, point( 1, 1 + lines ), FULL_SCREEN_WIDTH - 2, c_light_gray,
                        string_format( _( "Blood type: %s" ), io::enum_to_string( you.my_blood_type ) ) );
        fold_and_print( w_info, point( 1, 2 + lines ), FULL_SCREEN_WIDTH - 2, c_light_gray,
                        string_format( _( "Rh factor: %s" ),
                                       you.blood_rh_factor ? _( "positive (+)" ) : _( "negative (-)" ) ) );
    }
    wnoutrefresh( w_info );
}

static void draw_encumbrance_tab( ui_adaptor &ui, const catacurses::window &w_encumb,
                                  const Character &you, const unsigned line,
                                  const player_display_tab curtab )
{
    werase( w_encumb );
    const bool is_current_tab = curtab == player_display_tab::encumbrance;
    const nc_color title_col = is_current_tab ? h_light_gray : c_light_gray;
    center_print( w_encumb, 0, title_col, _( title_ENCUMB ) );
    if( is_current_tab ) {
        ui.set_cursor( w_encumb, point_zero );
        you.print_encumbrance( ui, w_encumb, line );
    } else {
        you.print_encumbrance( ui, w_encumb );
    }
    wnoutrefresh( w_encumb );
}

static void draw_encumbrance_info( const catacurses::window &w_info, const Character &you,
                                   const unsigned line, const unsigned info_line )
{
    const std::vector<std::pair<bodypart_id, bool>> bps = list_and_combine_bps( you, nullptr );

    werase( w_info );
    bodypart_id bp;
    if( line < bps.size() ) {
        bp = bps[line].first;
    }
    const std::vector<std::string> s = get_encumbrance_description( you, bp );
    const int winh = catacurses::getmaxy( w_info );
    const bool do_scroll = s.size() > static_cast<unsigned>( std::abs( winh ) );
    const int winw = FULL_SCREEN_WIDTH - ( do_scroll ? 3 : 2 );
    const int fline = do_scroll ? info_line % ( s.size() + 1 - winh ) : 0;
    const int lline = do_scroll ? fline + winh : s.size();
    for( int i = fline; i < lline; i++ ) {
        trim_and_print( w_info, point( 1, i - fline ), winw, c_light_gray, s[i] );
    }
    if( do_scroll ) {
        draw_scrollbar( w_info, fline, winh, s.size(), point( winw, 0 ), c_white, true );
    }
    wnoutrefresh( w_info );
}

static void draw_traits_tab( ui_adaptor &ui, const catacurses::window &w_traits,
                             const unsigned line, const player_display_tab curtab,
                             const std::vector<trait_and_var> &traitslist )
{
    werase( w_traits );
    const bool is_current_tab = curtab == player_display_tab::traits;
    const nc_color title_col = is_current_tab ? h_light_gray : c_light_gray;
    if( is_current_tab ) {
        ui.set_cursor( w_traits, point_zero );
    }
    center_print( w_traits, 0, title_col, _( title_TRAITS ) );

    const int height = getmaxy( w_traits ) - 1;
    const bool do_draw_scrollbar = height < static_cast<int>( traitslist.size() );
    const int width = getmaxx( w_traits ) - 1 - ( do_draw_scrollbar ? 1 : 0 );
    const std::pair<const int, const int> range = subindex_around_cursor( traitslist.size(), height,
            line, is_current_tab );

    for( size_t i = range.first; i < static_cast<size_t>( range.second ); ++i ) {
        const trait_and_var &cur = traitslist[i];
        const nc_color color = cur.trait->get_display_color();
        const bool highlight_line = is_current_tab && i == line;
        const point pos( 1, 1 + i - range.first );
        if( highlight_line ) {
            ui.set_cursor( w_traits, pos );
        }
        trim_and_print( w_traits, pos, width,
                        highlight_line ? hilite( color ) : color, cur.name() );
    }
    if( do_draw_scrollbar ) {
        draw_scrollbar( w_traits, range.first, height, traitslist.size(), point( width + 1, 1 ), c_white,
                        true );
    }
    wnoutrefresh( w_traits );
}

static void draw_traits_info( const catacurses::window &w_info, const unsigned line,
                              const std::vector<trait_and_var> &traitslist )
{
    werase( w_info );
    if( line < traitslist.size() ) {
        const trait_and_var &cur = traitslist[line];
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        fold_and_print( w_info, point( 1, 0 ), FULL_SCREEN_WIDTH - 2, c_light_gray, string_format( "%s: %s",
                        colorize( cur.name(), cur.trait->get_display_color() ), cur.desc() ) );
    }
    wnoutrefresh( w_info );
}

struct bionic_grouping {
    const translation name;
    const translation description;
    const int installed_count;
};

static void draw_bionics_tab( ui_adaptor &ui, const catacurses::window &w_bionics,
                              const Character &you, const unsigned line,
                              const player_display_tab curtab,
                              const std::vector<bionic_grouping> &bionicslist )
{
    werase( w_bionics );
    const bool is_current_tab = curtab == player_display_tab::bionics;
    const nc_color title_col = is_current_tab ? h_light_gray : c_light_gray;
    center_print( w_bionics, 0, title_col, _( title_BIONICS ) );
    int power_amount;
    std::string power_unit;
    if( you.get_power_level() < 1_J ) {
        power_amount = units::to_millijoule( you.get_power_level() );
        power_unit = pgettext( "energy unit: millijoule", "mJ" );
    } else if( you.get_power_level() < 1_kJ ) {
        power_amount = units::to_joule( you.get_power_level() );
        power_unit = pgettext( "energy unit: joule", "J" );
    } else {
        power_amount = units::to_kilojoule( you.get_power_level() );
        power_unit = pgettext( "energy unit: kilojoule", "kJ" );
    }
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    const point pow_pos( 1, 1 );
    if( is_current_tab ) {
        ui.set_cursor( w_bionics, pow_pos );
    }
    trim_and_print( w_bionics, pow_pos, getmaxx( w_bionics ) - 1, c_white,
                    string_format( _( "Power: <color_light_blue>%1$d %2$s</color>"
                                      " / <color_light_blue>%3$d kJ</color>" ),
                                   power_amount, power_unit, units::to_kilojoule( you.get_max_power_level() ) ) );
    const int height = getmaxy( w_bionics ) - 2;  // -2 for headline and power_level
    const bool do_draw_scrollbar = height < static_cast<int>( bionicslist.size() );
    const int width = getmaxx( w_bionics ) - 1 - ( do_draw_scrollbar ? 1 : 0 );
    const std::pair<const int, const int> range = subindex_around_cursor( bionicslist.size(), height,
            line, is_current_tab );

    for( size_t i = range.first; i < static_cast<size_t>( range.second ); ++i ) {
        const bool highlight_line = is_current_tab && i == line;
        const point pos( 1, 2 + i - range.first );
        if( highlight_line ) {
            ui.set_cursor( w_bionics, pos );
        }
        const bionic_grouping &bio_group = bionicslist[i];
        const std::string bio_name = bio_group.name.translated( bio_group.installed_count );
        nc_color col = highlight_line ? hilite( c_white ) : c_white;
        if( bio_group.installed_count <= 1 ) {
            trim_and_print( w_bionics, pos, width - 1, col, bio_name );
        } else {
            const std::string bio_count = string_format( " (%d)", bio_group.installed_count );
            const std::string bio_trunc = trim_by_length( bio_name, width - utf8_width( bio_count ) - 1 );
            print_colored_text( w_bionics, pos, col, col, bio_trunc );
            print_colored_text( w_bionics, pos + point( utf8_width( bio_trunc ), 0 ), col, col, bio_count );
        }
    }
    if( do_draw_scrollbar ) {
        draw_scrollbar( w_bionics, range.first, height, bionicslist.size(), point( width + 1, 2 ), c_white,
                        true );
    }
    wnoutrefresh( w_bionics );
}

static void draw_bionics_info( const catacurses::window &w_info, const unsigned line,
                               const std::vector<bionic_grouping> &bionicslist )
{
    werase( w_info );
    if( line < bionicslist.size() ) {
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        fold_and_print( w_info, point( 1, 0 ), FULL_SCREEN_WIDTH - 2, c_light_gray, "%s",
                        bionicslist[line].description );
    }
    wnoutrefresh( w_info );
}

static void draw_effects_tab( ui_adaptor &ui, const catacurses::window &w_effects,
                              const unsigned line, const player_display_tab curtab,
                              const std::vector<std::pair<std::string, std::string>> &effect_name_and_text )
{
    werase( w_effects );
    const bool is_current_tab = curtab == player_display_tab::effects;
    const nc_color title_col = is_current_tab ? h_light_gray : c_light_gray;
    if( is_current_tab ) {
        ui.set_cursor( w_effects, point_zero );
    }
    center_print( w_effects, 0, title_col, _( title_EFFECTS ) );

    const int height = getmaxy( w_effects ) - 1;
    const bool do_draw_scrollbar = height < static_cast<int>( effect_name_and_text.size() );
    const int width = getmaxx( w_effects ) - 1 - ( do_draw_scrollbar ? 1 : 0 );
    const std::pair<const int, const int> range = subindex_around_cursor( effect_name_and_text.size(),
            height, line, is_current_tab );

    for( size_t i = range.first; i < static_cast<size_t>( range.second ); ++i ) {
        const bool highlight_line = is_current_tab && i == line;
        const point pos( 1, 1 + i - range.first );
        if( highlight_line ) {
            ui.set_cursor( w_effects, pos );
        }
        trim_and_print( w_effects, pos, width,
                        highlight_line ? h_light_gray : c_light_gray, effect_name_and_text[i].first );
    }
    if( do_draw_scrollbar ) {
        draw_scrollbar( w_effects, range.first, height, effect_name_and_text.size(), point( width + 1, 1 ),
                        c_white, true );
    }
    wnoutrefresh( w_effects );
}

static void draw_effects_info( const catacurses::window &w_info, const unsigned line,
                               const std::vector<std::pair<std::string, std::string>> &effect_name_and_text )
{
    werase( w_info );
    const size_t actual_size = effect_name_and_text.size();
    if( line < actual_size ) {
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        fold_and_print( w_info, point( 1, 0 ), FULL_SCREEN_WIDTH - 2, c_light_gray,
                        effect_name_and_text[line].second );
    }
    wnoutrefresh( w_info );
}

struct HeaderSkill {
    const Skill *skill;
    bool is_header;
    HeaderSkill( const Skill *skill, bool is_header ): skill( skill ), is_header( is_header ) {
    }
};

static void draw_skills_tab( ui_adaptor &ui, const catacurses::window &w_skills,
                             Character &you, unsigned int line, const player_display_tab curtab,
                             std::vector<HeaderSkill> &skillslist,
                             const size_t skill_win_size_y )
{
    const int col_width = getmaxx( w_skills ) - 1;
    if( line == 0 ) { //can't point to a header;
        line = 1;
    }

    werase( w_skills );
    const bool is_current_tab = curtab == player_display_tab::skills;
    nc_color cstatus = is_current_tab ? h_light_gray : c_light_gray;
    if( is_current_tab ) {
        ui.set_cursor( w_skills, point_zero );
    }
    center_print( w_skills, 0, cstatus, _( title_SKILLS ) );

    size_t min = 0;
    size_t max = 0;

    const size_t half_y = ( skill_win_size_y - 1 ) / 2;

    if( !is_current_tab || line <= half_y ) {
        min = 0;
    } else if( line >= skillslist.size() - half_y ) {
        min = ( skillslist.size() < skill_win_size_y - 1 ? 0 : skillslist.size() -
                skill_win_size_y + 1 );
    } else {
        min = line - half_y;
    }
    max = std::min( min + skill_win_size_y - 1, skillslist.size() );

    int y_pos = 1;
    for( size_t i = min; i < max; ++i, ++y_pos ) {
        const Skill *aSkill = skillslist[i].skill;
        const SkillLevel &level = you.get_skill_level_object( aSkill->ident() );

        if( skillslist[i].is_header ) {
            const SkillDisplayType t = SkillDisplayType::get_skill_type( aSkill->display_category() );
            std::string type_name = t.display_string();
            mvwprintz( w_skills, point( 0, y_pos ), c_light_gray, header_spaces );
            center_print( w_skills, y_pos, c_yellow, type_name );
        } else {
            const bool can_train = level.can_train();
            const bool training = level.isTraining();
            const bool rusty = level.isRusty();
            int exercise = level.exercise();
            int level_num = level.level();
            bool locked = false;
            if( you.has_active_bionic( bio_cqb ) && is_cqb_skill( aSkill->ident() ) ) {
                level_num = 5;
                exercise = 0;
                locked = true;
            }
            if( is_current_tab && i == line ) {
                ui.set_cursor( w_skills, point( 1, y_pos ) );
                if( locked ) {
                    cstatus = h_yellow;
                } else if( !can_train ) {
                    cstatus = rusty ? h_light_red : h_white;
                } else if( exercise >= 100 ) {
                    cstatus = training ? h_pink : h_magenta;
                } else if( rusty ) {
                    cstatus = training ? h_light_red : h_red;
                } else {
                    cstatus = training ? h_light_blue : h_blue;
                }
                mvwprintz( w_skills, point( 1, y_pos ), cstatus, std::string( col_width, ' ' ) );
            } else {
                if( locked ) {
                    cstatus = c_yellow;
                } else if( rusty ) {
                    cstatus = training ? c_light_red : c_red;
                } else if( !can_train ) {
                    cstatus = c_white;
                } else {
                    cstatus = training ? c_light_blue : c_blue;
                }
                mvwprintz( w_skills, point( 1, y_pos ), c_light_gray, std::string( col_width, ' ' ) );
            }
            mvwprintz( w_skills, point( 1, y_pos ), cstatus, "%s:", aSkill->name() );
            if( aSkill->ident() == skill_dodge ) {
                mvwprintz( w_skills, point( 14, y_pos ), cstatus, "%4.1f/%-2d(%2d%%)",
                           you.get_dodge(),
                           level_num,
                           ( exercise < 0 ? 0 : exercise ) );
            } else {
                mvwprintz( w_skills, point( 19, y_pos ), cstatus, "%-2d(%2d%%)",
                           level_num,
                           ( exercise < 0 ? 0 : exercise ) );
            }
        }
    }

    if( is_current_tab && skillslist.size() > skill_win_size_y - 1 ) {
        draw_scrollbar( w_skills, line - 1, skill_win_size_y - 1, skillslist.size() - 1,
                        // NOLINTNEXTLINE(cata-use-named-point-constants)
                        point( 0, 1 ) );
    }
    wnoutrefresh( w_skills );
}

static void draw_skills_info( const catacurses::window &w_info, const Character &you,
                              unsigned int line,
                              const std::vector<HeaderSkill> &skillslist )
{
    werase( w_info );
    if( line < 1 ) {
        line = 1;
    }
    const Skill *selectedSkill = nullptr;
    if( line < skillslist.size() && !skillslist[line].is_header ) {
        selectedSkill = skillslist[line].skill;
    }

    werase( w_info );

    if( selectedSkill ) {
        const SkillLevel &level = you.get_skill_level_object( selectedSkill->ident() );
        std::string info_text = selectedSkill->description();
        if( level.isRusty() ) {
            info_text = string_format( _( "%s\n\nKnowledge level: %d (%d%%)" ), info_text,
                                       level.knowledgeLevel(), level.knowledgeExperience() );
        }
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        fold_and_print( w_info, point( 1, 0 ), FULL_SCREEN_WIDTH - 2, c_light_gray, info_text );
    }
    wnoutrefresh( w_info );
}

static void draw_speed_tab( const catacurses::window &w_speed,
                            const Character &you,
                            const std::map<std::string, int> &speed_effects )
{
    werase( w_speed );
    // Finally, draw speed.
    center_print( w_speed, 0, c_light_gray, _( title_SPEED ) );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w_speed, point( 1, 1 ), c_light_gray, _( "Base Move Cost:" ) );
    mvwprintz( w_speed, point( 1, 2 ), c_light_gray, _( "Current Speed:" ) );
    int newmoves = you.get_speed();
    int pen = 0;
    unsigned int line = 3;
    if( you.weight_carried() > you.weight_capacity() ) {
        pen = 25 * ( you.weight_carried() - you.weight_capacity() ) / you.weight_capacity();
        mvwprintz( w_speed, point( 1, line ), c_red,
                   pgettext( "speed penalty", "Overburdened        -%2d%%" ), pen );
        ++line;
    }
    pen = you.get_pain_penalty().speed;
    if( pen >= 1 ) {
        mvwprintz( w_speed, point( 1, line ), c_red,
                   pgettext( "speed penalty", "Pain                -%2d%%" ), pen );
        ++line;
    }
    if( you.get_thirst() > 40 ) {
        pen = std::abs( Character::thirst_speed_penalty( you.get_thirst() ) );
        mvwprintz( w_speed, point( 1, line ), c_red,
                   pgettext( "speed penalty", "Thirst              -%2d%%" ), pen );
        ++line;
    }
    if( you.kcal_speed_penalty() < 0 ) {
        pen = std::abs( you.kcal_speed_penalty() );
        const std::string inanition = you.get_bmi() < character_weight_category::underweight ?
                                      _( "Starving" ) : _( "Underfed" );
        //~ %s: Starving/Underfed (already left-justified), %2d: speed penalty
        mvwprintz( w_speed, point( 1, line ), c_red, pgettext( "speed penalty", "%s-%2d%%" ),
                   left_justify( inanition, 20 ), pen );
        ++line;
    }
    if( you.has_trait( trait_SUNLIGHT_DEPENDENT ) && !g->is_in_sunlight( you.pos() ) ) {
        pen = ( g->light_level( you.posz() ) >= 12 ? 5 : 10 );
        mvwprintz( w_speed, point( 1, line ), c_red,
                   pgettext( "speed penalty", "Out of Sunlight     -%2d%%" ), pen );
        ++line;
    }

    const float temperature_speed_modifier = you.mutation_value( "temperature_speed_modifier" );
    if( temperature_speed_modifier != 0 ) {
        nc_color pen_color;
        std::string pen_sign;
        const units::temperature player_local_temp = get_weather().get_temperature( you.pos() );
        if( you.has_flag( json_flag_ECTOTHERM ) && player_local_temp > units::from_fahrenheit( 65 ) ) {
            pen_color = c_green;
            pen_sign = "+";
        } else if( player_local_temp < units::from_fahrenheit( 65 ) ) {
            pen_color = c_red;
            pen_sign = "-";
        }
        if( !pen_sign.empty() ) {
            pen = ( units::to_fahrenheit( player_local_temp ) - 65 ) * temperature_speed_modifier;
            mvwprintz( w_speed, point( 1, line ), pen_color,
                       //~ %s: sign of bonus/penalty, %2d: speed bonus/penalty
                       pgettext( "speed modifier", "Cold-Blooded        %s%2d%%" ), pen_sign, std::abs( pen ) );
            ++line;
        }
    }

    const int speed_modifier = you.get_enchantment_speed_bonus();

    if( speed_modifier != 0 ) {
        mvwprintz( w_speed, point( 1, line ), c_green,
                   pgettext( "speed bonus", "Bio/Mut/Effects     +%2d" ), speed_modifier );
        ++line;
    }

    for( const std::pair<const std::string, int> &speed_effect : speed_effects ) {
        nc_color col = ( speed_effect.second > 0 ? c_green : c_red );
        mvwprintz( w_speed, point( 1, line ), col, "%s", speed_effect.first );
        mvwprintz( w_speed, point( 21, line ), col, ( speed_effect.second > 0 ? "+" : "-" ) );
        mvwprintz( w_speed, point( std::abs( speed_effect.second ) >= 10 ? 22 : 23, line ), col, "%d%%",
                   std::abs( speed_effect.second ) );
        ++line;
    }

    int runcost = you.run_cost( 100 );
    nc_color col = ( runcost <= 100 ? c_green : c_red );
    mvwprintz( w_speed, point( 21 + ( runcost >= 100 ? 0 : ( runcost < 10 ? 2 : 1 ) ), 1 ), col,
               "%d", runcost );
    col = ( newmoves >= 100 ? c_green : c_red );
    mvwprintz( w_speed, point( 21 + ( newmoves >= 100 ? 0 : ( newmoves < 10 ? 2 : 1 ) ), 2 ), col,
               "%d", newmoves );
    wnoutrefresh( w_speed );
}

static void draw_info_window( const catacurses::window &w_info, const Character &you,
                              const unsigned line, const unsigned info_line, const player_display_tab curtab,
                              const std::vector<trait_and_var> &traitslist,
                              const std::vector<bionic_grouping> &bionicslist,
                              const std::vector<std::pair<std::string, std::string>> &effect_name_and_text,
                              const std::vector<HeaderSkill> &skillslist )
{
    switch( curtab ) {
        case player_display_tab::stats:
            draw_stats_info( w_info, you, line );
            break;
        case player_display_tab::encumbrance:
            draw_encumbrance_info( w_info, you, line, info_line );
            break;
        case player_display_tab::skills:
            draw_skills_info( w_info, you, line, skillslist );
            break;
        case player_display_tab::traits:
            draw_traits_info( w_info, line, traitslist );
            break;
        case player_display_tab::bionics:
            draw_bionics_info( w_info, line, bionicslist );
            break;
        case player_display_tab::effects:
            draw_effects_info( w_info, line, effect_name_and_text );
            break;
        case player_display_tab::proficiencies:
            draw_proficiencies_info( w_info, line, you );
            break;
        case player_display_tab::num_tabs:
            cata_fatal( "Invalid curtab" );
    }
}

static void draw_tip( const catacurses::window &w_tip, const Character &you,
                      const std::string &race, const input_context &ctxt, bool customize_character )
{
    werase( w_tip );

    // Print name and header
    if( you.custom_profession.empty() ) {
        if( you.crossed_threshold() ) {
            //~ player info window: 1s - name, 2s - gender, 3s - Prof or Mutation name
            mvwprintz( w_tip, point_zero, c_white, _( " %1$s | %2$s | %3$s" ), you.get_name(),
                       you.male ? _( "Male" ) : _( "Female" ), race );
        } else if( you.prof == nullptr || you.prof == profession::generic() ) {
            // Regular person. Nothing interesting.
            //~ player info window: 1s - name, 2s - gender '|' - field separator.
            mvwprintz( w_tip, point_zero, c_white, _( " %1$s | %2$s" ), you.get_name(),
                       you.male ? _( "Male" ) : _( "Female" ) );
        } else {
            mvwprintz( w_tip, point_zero, c_white, _( " %1$s | %2$s | %3$s" ), you.get_name(),
                       you.male ? _( "Male" ) : _( "Female" ),
                       you.prof->gender_appropriate_name( you.male ) );
        }
    } else {
        mvwprintz( w_tip, point_zero, c_white, _( " %1$s | %2$s | %3$s" ), you.get_name(),
                   you.male ? _( "Male" ) : _( "Female" ), you.custom_profession );
    }

    if( customize_character ) {
        right_print( w_tip, 0, 8, c_light_gray, string_format(
                         _( "[<color_yellow>%s</color>] Customize character" ),
                         ctxt.get_desc( "SWITCH_GENDER" ) ) );
    }

    right_print( w_tip, 0, 1, c_light_gray, string_format(
                     _( "[<color_yellow>%s</color>]" ),
                     ctxt.get_desc( "HELP_KEYBINDINGS" ) ) );

    right_print( w_tip, 0, 0, c_light_gray, string_format( "%s", LINE_XOXO_S ) );

    wnoutrefresh( w_tip );
}

static bool handle_player_display_action( Character &you, unsigned int &line,
        unsigned int &info_line,
        player_display_tab &curtab, input_context &ctxt, const ui_adaptor &ui_tip,
        const ui_adaptor &ui_info, const ui_adaptor &ui_stats, const ui_adaptor &ui_encumb,
        const ui_adaptor &ui_traits, const ui_adaptor &ui_bionics, const ui_adaptor &ui_effects,
        const ui_adaptor &ui_skills, const ui_adaptor &ui_proficiencies,
        std::vector<trait_and_var> &traitslist,
        const std::vector<bionic_grouping> &bionicslist,
        const std::vector<std::pair<std::string, std::string>> &effect_name_and_text,
        const std::vector<HeaderSkill> &skillslist, bool customize_character )
{
    const auto invalidate_tab = [&]( const player_display_tab tab ) {
        switch( tab ) {
            case player_display_tab::stats:
                ui_stats.invalidate_ui();
                break;
            case player_display_tab::encumbrance:
                ui_encumb.invalidate_ui();
                break;
            case player_display_tab::traits:
                ui_traits.invalidate_ui();
                break;
            case player_display_tab::bionics:
                ui_bionics.invalidate_ui();
                break;
            case player_display_tab::effects:
                ui_effects.invalidate_ui();
                break;
            case player_display_tab::skills:
                ui_skills.invalidate_ui();
                break;
            case player_display_tab::proficiencies:
                ui_proficiencies.invalidate_ui();
                break;
            case player_display_tab::num_tabs:
                cata_fatal( "Invalid tab" );
        }
    };

    unsigned int line_beg = 0;
    unsigned int line_end = 0;
    switch( curtab ) {
        case player_display_tab::stats:
            line_end = 8;
            break;
        case player_display_tab::encumbrance: {
            const std::vector<std::pair<bodypart_id, bool>> bps = list_and_combine_bps( you, nullptr );
            line_end = bps.size();
            break;
        }
        case player_display_tab::traits:
            line_end = traitslist.size();
            break;
        case player_display_tab::bionics:
            line_end = bionicslist.size();
            break;
        case player_display_tab::effects:
            line_end = effect_name_and_text.size();
            break;
        case player_display_tab::skills:
            line_beg = 1; // skip first header
            line_end = skillslist.size();
            break;
        case player_display_tab::proficiencies:
            line_end = you.display_proficiencies().size();
            break;
        case player_display_tab::num_tabs:
            cata_fatal( "Invalid curtab" );
    }
    if( line_beg >= line_end || line < line_beg ) {
        line = line_beg;
    } else if( line > line_end - 1 ) {
        line = line_end - 1;
    }

    bool done = false;
    std::string action = ctxt.handle_input();

    if( action == "UP" ) {
        if( line > line_beg ) {
            --line;
        } else {
            line = line_end - 1;
        }
        if( curtab == player_display_tab::skills && skillslist[line].is_header ) {
            --line;
        }
        info_line = 0;
        invalidate_tab( curtab );
        ui_info.invalidate_ui();
    } else if( action == "DOWN" ) {
        if( line + 1 < line_end ) {
            ++line;
        } else {
            line = line_beg;
        }
        if( curtab == player_display_tab::skills && skillslist[line].is_header ) {
            ++line;
        }
        info_line = 0;
        invalidate_tab( curtab );
        ui_info.invalidate_ui();
    } else if( action == "NEXT_TAB" || action == "PREV_TAB" ) {
        line = 0;
        invalidate_tab( curtab );
        curtab = action == "NEXT_TAB" ? next_tab( curtab ) : prev_tab( curtab );
        invalidate_tab( curtab );
        info_line = 0;
        ui_info.invalidate_ui();
    } else if( action == "SELECT_TRAIT_VARIANT" ) {
        if( curtab == player_display_tab::traits ) {
            const mutation_variant *var = traitslist[line].trait->pick_variant_menu();
            you.set_mut_variant( traitslist[line].trait, var );
            const std::string &varid = var == nullptr ? "" : var->id;
            traitslist[line].variant = varid;
        }
    } else if( action == "QUIT" ) {
        done = true;
    } else if( action == "CONFIRM" ) {
        switch( curtab ) {
            default:
                break;
            case player_display_tab::stats:
                if( line < 4 && get_option<bool>( "STATS_THROUGH_KILLS" ) && you.is_avatar() ) {
                    you.as_avatar()->upgrade_stat_prompt( static_cast<character_stat>( line ) );
                }
                invalidate_tab( curtab );
                break;
            case player_display_tab::skills: {
                const Skill *selectedSkill = nullptr;
                if( line < skillslist.size() && !skillslist[line].is_header ) {
                    selectedSkill = skillslist[line].skill;
                }
                if( selectedSkill ) {
                    you.get_skill_level_object( selectedSkill->ident() ).toggleTraining();
                }
                invalidate_tab( curtab );
                break;
            }
            case player_display_tab::proficiencies:
                show_proficiencies_window( you );
                break;
        }
    } else if( action == "CHANGE_PROFESSION_NAME" ) {
        string_input_popup popup;
        popup.title( _( "Profession Name: " ) )
        .width( 25 )
        .text( "" )
        .max_length( 25 )
        .query();

        you.custom_profession = popup.text();
        ui_tip.invalidate_ui();
    } else if( action == "VIEW_PROFICIENCIES" ) {
        show_proficiencies_window( you );
    } else if( action == "VIEW_BODYSTAT" ) {
        display_bodygraph( you );
    } else if( customize_character && action == "SWITCH_GENDER" ) {
        uilist cmenu;
        cmenu.title = _( "Customize Character" );
        cmenu.addentry( 1, true, 'y', _( "Change gender" ) );
        cmenu.addentry( 2, true, 'n', _( "Change name" ) );

        cmenu.query();
        if( cmenu.ret == 1 ) {
            you.male = !you.male;
            popup( _( "Gender set to %s." ), you.male ? _( "Male" ) : _( "Female" ) );
        } else if( cmenu.ret == 2 ) {
            std::string filterstring = you.play_name.value_or( std::string() );
            string_input_popup popup;
            popup
            .title( _( "New name ( leave empty to reset ):" ) )
            .width( 85 )
            .edit( filterstring );
            if( popup.confirmed() ) {
                if( filterstring.empty() ) {
                    you.play_name.reset();
                } else {
                    you.play_name = filterstring;
                }
            }
        }
    } else if( action == "SCROLL_INFOBOX_UP" ) {
        if( info_line > 0 ) {
            --info_line;
            ui_info.invalidate_ui();
        }
    } else if( action == "SCROLL_INFOBOX_DOWN" ) {
        ++info_line;
        ui_info.invalidate_ui();
    } else if( action == "MEDICAL_MENU" ) {
        you.as_avatar()->disp_medical();
    }
    return done;
}

static std::pair<unsigned, unsigned> calculate_shared_column_win_height(
    const unsigned available_height, unsigned first_win_size_y_max, unsigned second_win_size_y_max )
/**
 * Calculate max allowed height of two windows sharing column space.
 */
{
    if( ( second_win_size_y_max + 1 + first_win_size_y_max ) > available_height ) {
        // maximum space for either window if they're both the same size
        unsigned max_shared_y = ( available_height - 1 ) / 2;
        if( std::min( second_win_size_y_max, first_win_size_y_max ) > max_shared_y ) {
            // both are larger than the shared size
            second_win_size_y_max = max_shared_y;
            first_win_size_y_max = available_height - 1 - second_win_size_y_max;
        } else if( first_win_size_y_max <= max_shared_y ) {
            // first window is less than the shared size, so give space to second window
            second_win_size_y_max = available_height - 1 - first_win_size_y_max;
        } else {
            // second window is less than the shared size
            first_win_size_y_max = available_height - 1 - second_win_size_y_max;
        }
    }
    return std::make_pair( first_win_size_y_max, second_win_size_y_max );
}

void Character::disp_info( bool customize_character )
{
    // Customizing any character is always enabled in debug mode
    customize_character |= debug_mode;

    std::vector<std::pair<std::string, std::string>> effect_name_and_text;
    for( auto &elem : *effects ) {
        for( auto &_effect_it : elem.second ) {
            const std::string name = _effect_it.second.disp_name();
            if( name.empty() ) {
                continue;
            }
            effect_name_and_text.emplace_back( name, _effect_it.second.disp_desc() );
        }
    }
    if( get_perceived_pain() > 0 ) {
        const stat_mod ppen = get_pain_penalty();
        std::pair<std::string, nc_color> pain_desc = display::pain_text_color( *this );
        std::string pain_text;
        pain_desc.first = string_format( _( "You are in %s\n" ), pain_desc.first );
        pain_text += colorize( pain_desc.first, pain_desc.second );
        const auto add_if = [&]( const int amount, const char *const name ) {
            if( amount > 0 ) {
                pain_text += string_format( name, amount ) + "   ";
            }
        };
        add_if( ppen.strength, _( "Strength -%d" ) );
        add_if( ppen.dexterity, _( "Dexterity -%d" ) );
        add_if( ppen.intelligence, _( "Intelligence -%d" ) );
        add_if( ppen.perception, _( "Perception -%d" ) );
        add_if( ppen.speed, _( "Speed -%d %%" ) );
        effect_name_and_text.emplace_back( _( "Pain" ), pain_text );
    }

    const float bmi = get_bmi();

    if( bmi < character_weight_category::underweight ) {
        std::string starvation_name;
        std::string starvation_text;

        if( bmi < character_weight_category::emaciated ) {
            starvation_name = _( "Severely Malnourished" );
            starvation_text =
                _( "Your body is severely weakened by starvation.  You might die if you don't start eating regular meals!\n\n" );
        } else {
            starvation_name = _( "Malnourished" );
            starvation_text =
                _( "Your body is weakened by starvation.  Only time and regular meals will help you recover.\n\n" );
        }

        if( bmi < character_weight_category::underweight ) {
            const float str_penalty = 1.0f - ( ( bmi - 13.0f ) / 3.0f );
            starvation_text += std::string( _( "Strength" ) ) + " -" + string_format( "%2.0f%%\n",
                               str_penalty * 100.0f );
            starvation_text += std::string( _( "Dexterity" ) ) + " -" + string_format( "%2.0f%%\n",
                               str_penalty * 50.0f );
            starvation_text += std::string( _( "Intelligence" ) ) + " -" + string_format( "%2.0f%%",
                               str_penalty * 50.0f );
        }

        effect_name_and_text.emplace_back( starvation_name, starvation_text );
    }

    if( has_trait( trait_TROGLO3 ) && g->is_in_sunlight( pos() ) ) {
        effect_name_and_text.emplace_back( _( "In Sunlight" ),
                                           _( "The sunlight irritates you terribly.\n"
                                              "Strength - 4;    Dexterity - 4;    Intelligence - 4;    Perception - 4" )
                                         );
    } else  if( has_trait( trait_TROGLO2 ) && g->is_in_sunlight( pos() ) ) {
        if( incident_sun_irradiance( get_weather().weather_id, calendar::turn ) > irradiance::moderate ) {
            effect_name_and_text.emplace_back( _( "In Sunlight" ),
                                               _( "The sunlight irritates you badly.\n"
                                                  "Strength - 2;    Dexterity - 2;    Intelligence - 2;    Perception - 2" ) );
        } else if( incident_sun_irradiance( get_weather().weather_id, calendar::turn ) > irradiance::low ) {
            effect_name_and_text.emplace_back( _( "In Sunlight" ),
                                               _( "The sunlight irritates you badly.\n"
                                                  "Strength - 1;    Dexterity - 1;    Intelligence - 1;    Perception - 1" ) );
        }


    } else if( has_trait( trait_TROGLO ) && g->is_in_sunlight( pos() ) &&
               incident_sun_irradiance( get_weather().weather_id, calendar::turn ) > irradiance::moderate ) {
        effect_name_and_text.emplace_back( _( "In Sunlight" ),
                                           _( "The sunlight irritates you.\n"
                                              "Strength - 1;    Dexterity - 1;    Intelligence - 1;    Perception - 1" )
                                         );
    }

    for( addiction &elem : addictions ) {
        if( elem.sated < 0_turns && elem.intensity >= MIN_ADDICTION_LEVEL ) {
            effect_name_and_text.emplace_back( elem.type->get_name().translated(),
                                               elem.type->get_description().translated() );
        }
    }

    const unsigned int effect_win_size_y_max = 1 + static_cast<unsigned>( effect_name_and_text.size() );
    const unsigned int proficiency_win_size_y_max = 1 + static_cast<unsigned>
            ( display_proficiencies().size() );

    std::vector<trait_and_var> traitslist = get_mutations_variants( false );
    std::sort( traitslist.begin(), traitslist.end(), trait_display_sort );
    const unsigned int trait_win_size_y_max = 1 + static_cast<unsigned>( traitslist.size() );

    std::vector<bionic_grouping> bionicslist;
    {
        // count installed bionics by type and sort by localized name
        const auto bio_comp = []( const bionic_data & lhs, const bionic_data & rhs ) {
            return lhs.name.translated_lt( rhs.name );
        };
        std::map<const bionic_data, int, decltype( bio_comp )> bionics_map( bio_comp );
        for( const bionic &bio : *my_bionics ) {
            bionics_map[bio.info()]++;
        }
        for( const auto &pair : bionics_map ) {
            bionicslist.push_back( { pair.first.name, pair.first.description, pair.second } );
        }
    }
    const unsigned int bionics_win_size_y_max = 2 + bionicslist.size();

    const std::vector<const Skill *> player_skill = Skill::get_skills_sorted_by(
    [&]( const Skill & a, const Skill & b ) {
        skill_displayType_id type_a = a.display_category();
        skill_displayType_id type_b = b.display_category();

        return localized_compare( std::make_pair( type_a, a.name() ),
                                  std::make_pair( type_b, b.name() ) );
    } );

    std::vector<HeaderSkill> skillslist;
    skill_displayType_id prev_type = skill_displayType_id::NULL_ID();
    for( const Skill * const &s : player_skill ) {
        if( s->display_category() != prev_type ) {
            prev_type = s->display_category();
            skillslist.emplace_back( s, true );
        }
        skillslist.emplace_back( s, false );
    }
    const unsigned int skill_win_size_y_max = 1 + skillslist.size();
    const unsigned int info_win_size_y = 6;

    const unsigned int grid_height = 10;

    const unsigned int infooffsetytop = grid_height + 2;
    unsigned int infooffsetybottom = infooffsetytop + 1 + info_win_size_y;

    // Print name and header
    // Post-humanity trumps your pre-Cataclysm life
    // Unless you have a custom profession.
    std::string race;
    if( custom_profession.empty() && crossed_threshold() ) {
        for( const trait_and_var &mut : get_mutations_variants() ) {
            if( mut.trait->threshold ) {
                race = mut.name();
                break;
            }
        }
    }

    input_context ctxt( "PLAYER_INFO" );
    ctxt.register_updown();
    ctxt.register_action( "NEXT_TAB", to_translation( "Cycle to next category" ) );
    ctxt.register_action( "PREV_TAB", to_translation( "Cycle to previous category" ) );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "CONFIRM", to_translation( "Toggle skill training / Upgrade stat" ) );
    ctxt.register_action( "CHANGE_PROFESSION_NAME", to_translation( "Change profession name" ) );
    ctxt.register_action( "SWITCH_GENDER", to_translation( "Customize base appearance and name" ) );
    ctxt.register_action( "VIEW_PROFICIENCIES", to_translation( "View character proficiencies" ) );
    ctxt.register_action( "VIEW_BODYSTAT", to_translation( "View character's body status" ) );
    ctxt.register_action( "SCROLL_INFOBOX_UP", to_translation( "Scroll information box up" ) );
    ctxt.register_action( "SCROLL_INFOBOX_DOWN", to_translation( "Scroll information box down" ) );
    ctxt.register_action( "SELECT_TRAIT_VARIANT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "MEDICAL_MENU" );

    std::map<std::string, int> speed_effects;
    for( auto &elem : *effects ) {
        for( std::pair<const bodypart_id, effect> &_effect_it : elem.second ) {
            effect &it = _effect_it.second;
            bool reduced = resists_effect( it );
            int move_adjust = it.get_mod( "SPEED", reduced );
            if( move_adjust != 0 ) {
                const std::string dis_text = it.get_speed_name();
                speed_effects[dis_text] += move_adjust;
            }
        }
    }

    border_helper borders;

    player_display_tab curtab = player_display_tab::stats;
    unsigned int line = 0;
    unsigned int info_line = 0;

    catacurses::window w_tip;
    ui_adaptor ui_tip;
    ui_tip.on_screen_resize( [&]( ui_adaptor & ui_tip ) {
        w_tip = catacurses::newwin( 1, FULL_SCREEN_WIDTH + 1, point_zero );
        ui_tip.position_from_window( w_tip );
    } );
    ui_tip.mark_resize();
    ui_tip.on_redraw( [&]( ui_adaptor & ui_tip ) {
        ui_tip.disable_cursor();
        draw_tip( w_tip, *this, race, ctxt, customize_character );
    } );

    // STATS
    catacurses::window w_stats;
    catacurses::window w_stats_border;
    border_helper::border_info &border_stats = borders.add_border();
    ui_adaptor ui_stats;
    ui_stats.on_screen_resize( [&]( ui_adaptor & ui_stats ) {
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        w_stats = catacurses::newwin( grid_height, grid_width, point( 0, 1 ) );
        // Every grid draws the bottom and right borders. The top and left borders
        // are either not displayed or drawn by another grid.
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        w_stats_border = catacurses::newwin( grid_height + 1, grid_width + 1, point( 0, 1 ) );
        // But we need to specify the full border for border_helper to calculate the
        // border connection.
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        border_stats.set( point( -1, 0 ), point( grid_width + 2, grid_height + 2 ) );
        ui_stats.position_from_window( w_stats_border );
    } );
    ui_stats.mark_resize();
    ui_stats.on_redraw( [&]( ui_adaptor & ui_stats ) {
        borders.draw_border( w_stats_border );
        wnoutrefresh( w_stats_border );
        ui_stats.disable_cursor();
        draw_stats_tab( ui_stats, w_stats, *this, line, curtab, ctxt );
    } );

    // TRAITS & BIONICS
    unsigned trait_win_size_y;
    unsigned bionics_win_size_y;
    // TRAITS
    catacurses::window w_traits;
    catacurses::window w_traits_border;
    border_helper::border_info &border_traits = borders.add_border();
    ui_adaptor ui_traits;
    ui_traits.on_screen_resize( [&]( ui_adaptor & ui_traits ) {
        std::tie( trait_win_size_y, bionics_win_size_y ) = calculate_shared_column_win_height(
                    static_cast<unsigned>( TERMY ) - infooffsetybottom, trait_win_size_y_max, bionics_win_size_y_max );
        w_traits = catacurses::newwin( trait_win_size_y, grid_width,
                                       point( grid_width + 1, infooffsetybottom ) );
        w_traits_border = catacurses::newwin( trait_win_size_y + 1, grid_width + 2,
                                              point( grid_width, infooffsetybottom ) );
        border_traits.set( point( grid_width, infooffsetybottom - 1 ),
                           point( grid_width + 2, trait_win_size_y + 2 ) );
        ui_traits.position_from_window( w_traits_border );
    } );
    ui_traits.mark_resize();
    ui_traits.on_redraw( [&]( ui_adaptor & ui_traits ) {
        borders.draw_border( w_traits_border );
        wnoutrefresh( w_traits_border );
        ui_traits.disable_cursor();
        draw_traits_tab( ui_traits, w_traits, line, curtab, traitslist );
    } );

    // BIONICS
    catacurses::window w_bionics;
    catacurses::window w_bionics_border;
    border_helper::border_info &border_bionics = borders.add_border();
    ui_adaptor ui_bionics;
    ui_bionics.on_screen_resize( [&]( ui_adaptor & ui_bionics ) {
        std::tie( trait_win_size_y, bionics_win_size_y ) = calculate_shared_column_win_height(
                    static_cast<unsigned>( TERMY ) - infooffsetybottom, trait_win_size_y_max, bionics_win_size_y_max );
        w_bionics = catacurses::newwin( bionics_win_size_y, grid_width,
                                        point( grid_width + 1,
                                               infooffsetybottom + trait_win_size_y + 1 ) );
        w_bionics_border = catacurses::newwin( bionics_win_size_y + 1, grid_width + 2,
                                               point( grid_width, infooffsetybottom + trait_win_size_y + 1 ) );
        border_bionics.set( point( grid_width, infooffsetybottom + trait_win_size_y ),
                            point( grid_width + 2, bionics_win_size_y + 2 ) );
        ui_bionics.position_from_window( w_bionics_border );
    } );
    ui_bionics.mark_resize();
    ui_bionics.on_redraw( [&]( ui_adaptor & ui_bionics ) {
        borders.draw_border( w_bionics_border );
        wnoutrefresh( w_bionics_border );
        ui_bionics.disable_cursor();
        draw_bionics_tab( ui_bionics, w_bionics, *this, line, curtab, bionicslist );
    } );

    // ENCUMBRANCE
    catacurses::window w_encumb;
    catacurses::window w_encumb_border;
    border_helper::border_info &border_encumb = borders.add_border();
    ui_adaptor ui_encumb;
    ui_encumb.on_screen_resize( [&]( ui_adaptor & ui_encumb ) {
        w_encumb = catacurses::newwin( grid_height, grid_width, point( grid_width + 1, 1 ) );
        w_encumb_border = catacurses::newwin( grid_height + 1, grid_width + 1, point( grid_width + 1, 1 ) );
        border_encumb.set( point( grid_width, 0 ), point( grid_width + 2, grid_height + 2 ) );
        ui_encumb.position_from_window( w_encumb_border );
    } );
    ui_encumb.mark_resize();
    ui_encumb.on_redraw( [&]( ui_adaptor & ui_encumb ) {
        borders.draw_border( w_encumb_border );
        wnoutrefresh( w_encumb_border );
        ui_encumb.disable_cursor();
        draw_encumbrance_tab( ui_encumb, w_encumb, *this, line, curtab );
    } );

    // EFFECTS & PROFICIENCIES
    unsigned effect_win_size_y;
    unsigned proficiency_win_size_y;
    // EFFECTS
    catacurses::window w_effects;
    catacurses::window w_effects_border;
    border_helper::border_info &border_effects = borders.add_border();
    ui_adaptor ui_effects;
    ui_effects.on_screen_resize( [&]( ui_adaptor & ui_effects ) {
        std::tie( effect_win_size_y, proficiency_win_size_y ) = calculate_shared_column_win_height(
                    static_cast<unsigned>( TERMY ) - infooffsetybottom, effect_win_size_y_max,
                    proficiency_win_size_y_max );
        w_effects = catacurses::newwin( effect_win_size_y, grid_width,
                                        point( grid_width * 2 + 2, infooffsetybottom ) );
        w_effects_border = catacurses::newwin( effect_win_size_y + 1, grid_width + 2,
                                               point( grid_width * 2 + 1, infooffsetybottom ) );
        border_effects.set( point( grid_width * 2 + 1, infooffsetybottom - 1 ),
                            point( grid_width + 2, effect_win_size_y + 2 ) );
        ui_effects.position_from_window( w_effects_border );
    } );
    ui_effects.mark_resize();
    ui_effects.on_redraw( [&]( ui_adaptor & ui_effects ) {
        borders.draw_border( w_effects_border );
        wnoutrefresh( w_effects_border );
        ui_effects.disable_cursor();
        draw_effects_tab( ui_effects, w_effects, line, curtab, effect_name_and_text );
    } );

    // PROFICIENCIES
    catacurses::window w_proficiencies;
    catacurses::window w_proficiencies_border;
    border_helper::border_info &border_proficiencies = borders.add_border();
    ui_adaptor ui_proficiencies;
    ui_proficiencies.on_screen_resize( [&]( ui_adaptor & ui_proficiencies ) {
        std::tie( effect_win_size_y, proficiency_win_size_y ) = calculate_shared_column_win_height(
                    static_cast<unsigned>( TERMY ) - infooffsetybottom, effect_win_size_y_max,
                    proficiency_win_size_y_max );
        const point profstart = point( grid_width * 2 + 2, infooffsetybottom + effect_win_size_y + 1 );
        w_proficiencies = catacurses::newwin( proficiency_win_size_y, grid_width,
                                              profstart );
        w_proficiencies_border = catacurses::newwin( proficiency_win_size_y + 1, grid_width + 2,
                                 profstart + point_west );
        border_proficiencies.set( profstart + point_north_west, point( grid_width + 2,
                                  proficiency_win_size_y + 2 ) );
        ui_proficiencies.position_from_window( w_proficiencies_border );
    } );
    ui_proficiencies.mark_resize();
    ui_proficiencies.on_redraw( [&]( ui_adaptor & ui_proficiencies ) {
        borders.draw_border( w_proficiencies_border );
        wnoutrefresh( w_proficiencies_border );
        ui_proficiencies.disable_cursor();
        draw_proficiencies_tab( ui_proficiencies, w_proficiencies, line, *this, curtab, ctxt );
    } );

    // SKILLS
    unsigned int skill_win_size_y = 0;
    catacurses::window w_skills;
    catacurses::window w_skills_border;
    border_helper::border_info &border_skills = borders.add_border();
    ui_adaptor ui_skills;
    ui_skills.on_screen_resize( [&]( ui_adaptor & ui_skills ) {
        const unsigned int maxy = static_cast<unsigned>( TERMY );
        skill_win_size_y = skill_win_size_y_max;
        if( skill_win_size_y + infooffsetybottom > maxy ) {
            skill_win_size_y = maxy - infooffsetybottom;
        }
        w_skills = catacurses::newwin( skill_win_size_y, grid_width,
                                       point( 0, infooffsetybottom ) );
        w_skills_border = catacurses::newwin( skill_win_size_y + 1, grid_width + 1,
                                              point( 0, infooffsetybottom ) );
        border_skills.set( point( -1, infooffsetybottom - 1 ),
                           point( grid_width + 2, skill_win_size_y + 2 ) );
        ui_skills.position_from_window( w_skills_border );
    } );
    ui_skills.mark_resize();
    ui_skills.on_redraw( [&]( ui_adaptor & ui_skills ) {
        borders.draw_border( w_skills_border );
        wnoutrefresh( w_skills_border );
        ui_skills.disable_cursor();
        draw_skills_tab( ui_skills, w_skills, *this, line, curtab, skillslist, skill_win_size_y );
    } );

    // info panel
    catacurses::window w_info;
    catacurses::window w_info_border;
    border_helper::border_info &border_info = borders.add_border();
    ui_adaptor ui_info;
    ui_info.on_screen_resize( [&]( ui_adaptor & ui_info ) {
        w_info = catacurses::newwin( info_win_size_y, FULL_SCREEN_WIDTH,
                                     point( 0, infooffsetytop ) );
        w_info_border = catacurses::newwin( info_win_size_y + 1, FULL_SCREEN_WIDTH + 1,
                                            point( 0, infooffsetytop ) );
        border_info.set( point( -1, infooffsetytop - 1 ),
                         point( FULL_SCREEN_WIDTH + 2, info_win_size_y + 2 ) );
        ui_info.position_from_window( w_info_border );
    } );
    ui_info.mark_resize();
    ui_info.on_redraw( [&]( ui_adaptor & ui_info ) {
        borders.draw_border( w_info_border );
        wnoutrefresh( w_info_border );
        ui_info.disable_cursor();
        draw_info_window( w_info, *this, line, info_line, curtab,
                          traitslist, bionicslist, effect_name_and_text, skillslist );
    } );

    // SPEED
    catacurses::window w_speed;
    catacurses::window w_speed_border;
    border_helper::border_info &border_speed = borders.add_border();
    ui_adaptor ui_speed;
    ui_speed.on_screen_resize( [&]( ui_adaptor & ui_speed ) {
        w_speed = catacurses::newwin( grid_height, grid_width, point( grid_width * 2 + 2, 1 ) );
        w_speed_border = catacurses::newwin( grid_height + 1, grid_width + 1,
                                             point( grid_width * 2 + 2, 1 ) );
        border_speed.set( point( grid_width * 2 + 1, 0 ),
                          point( grid_width + 2, grid_height + 2 ) );
        ui_speed.position_from_window( w_speed_border );
    } );
    ui_speed.mark_resize();
    ui_speed.on_redraw( [&]( ui_adaptor & ui_speed ) {
        borders.draw_border( w_speed_border );
        wnoutrefresh( w_speed_border );
        ui_speed.disable_cursor();
        draw_speed_tab( w_speed, *this, speed_effects );
    } );

    bool done = false;

    do {
        ui_manager::redraw_invalidated();

        done = handle_player_display_action( *this, line, info_line, curtab, ctxt, ui_tip, ui_info,
                                             ui_stats, ui_encumb, ui_traits, ui_bionics, ui_effects,
                                             ui_skills, ui_proficiencies, traitslist, bionicslist,
                                             effect_name_and_text, skillslist, customize_character );
    } while( !done );
}
