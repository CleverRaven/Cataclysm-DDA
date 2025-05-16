#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

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
#include "coordinates.h"
#include "creature.h"
#include "cursesdef.h"
#include "debug.h"
#include "display.h"
#include "effect.h"
#include "enum_conversions.h"
#include "flag.h"
#include "game.h"
#include "game_constants.h"
#include "game_inventory.h"
#include "input_context.h"
#include "item.h"
#include "item_location.h"
#include "itype.h"
#include "magic_enchantment.h"
#include "mutation.h"
#include "options.h"
#include "output.h"
#include "pimpl.h"
#include "point.h"
#include "proficiency.h"
#include "sdltiles.h"
#include "skill.h"
#include "skill_ui.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translation.h"
#include "translations.h"
#include "type_id.h"
#include "uilist.h"
#include "ui_manager.h"
#include "units.h"
#include "units_utility.h"
#include "weather.h"

static const bionic_id bio_cqb( "bio_cqb" );

static const skill_id skill_bashing( "bashing" );
static const skill_id skill_cutting( "cutting" );
static const skill_id skill_dodge( "dodge" );
static const skill_id skill_melee( "melee" );
static const skill_id skill_stabbing( "stabbing" );
static const skill_id skill_unarmed( "unarmed" );

static const trait_id trait_TROGLO( "TROGLO" );
static const trait_id trait_TROGLO2( "TROGLO2" );
static const trait_id trait_TROGLO3( "TROGLO3" );

static const std::string title_STATS = translate_marker( "STATS" );
static const std::string title_ENCUMB = translate_marker( "ENCUMBRANCE AND WARMTH" );
static const std::string title_EFFECTS = translate_marker( "EFFECTS" );
static const std::string title_SPEED = translate_marker( "SPEED" );
static const std::string title_MOVE_COST = translate_marker( "MOVE COST" );
static const std::string title_SKILLS = translate_marker( "SKILLS" );
static const std::string title_BIONICS = translate_marker( "BIONICS" );
static const std::string title_TRAITS = translate_marker( "TRAITS" );
static const std::string title_PROFICIENCIES = translate_marker( "PROFICIENCIES" );

static const unsigned int grid_width = 26;

// Rescale temperature value to one that the player sees
static int temperature_print_rescaling( units::temperature temp )
{
    return ( units::to_legacy_bodypart_temp( temp ) / 100.0 ) * 2 - 100;
}

static bool should_combine_bps( const Character &p, const bodypart_id &l, const bodypart_id &r,
                                const item *selected_clothing )
{
    return l != r && // are different parts
           l ==  r->opposite_part && r == l->opposite_part && // are complementary parts
           // same encumbrance & temperature
           p.compare_encumbrance_data( l, r ) &&
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
        int encumbrance = get_part_encumbrance( bp );
        int layer_penalty = get_part_layer_penalty( bp );

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
        mvwprintz( win, point( column, y_pos ), display::encumb_color( encumbrance ), "%3d",
                   encumbrance - layer_penalty );
        // separator in low toned color
        column += 3; //Prepared for 3-digit encumbrance
        mvwprintz( win, point( column, y_pos ), c_light_gray, "+" );
        column += 1; // "+"
        // take into account the new encumbrance system for layers
        mvwprintz( win, point( column, y_pos ), display::encumb_color( encumbrance ), "%-3d",
                   layer_penalty );
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
    for( const limb_score &sc : limb_score::get_all() ) {
        if( !bp->has_limb_score( sc.getId() ) ) {
            continue;
        }
        float cur_score = part->get_limb_score( you, sc.getId() );
        float bp_score = bp->get_limb_score( sc.getId() );
        // Check for any global limb score modifiers
        for( const effect &eff : you.get_effects_with_flag( flag_EFFECT_LIMB_SCORE_MOD ) ) {
            cur_score *= eff.get_limb_score_mod( sc.getId(), you.resists_effect( eff ) );
        }
        // Check for local score-modifying effects only if we have any
        if( you.has_flag( flag_EFFECT_LIMB_SCORE_MOD_LOCAL ) ) {
            for( const effect &local : you.get_effects_from_bp( part->get_id() ) ) {
                float local_mul = 1.0f;
                if( local.has_flag( flag_EFFECT_LIMB_SCORE_MOD_LOCAL ) ) {
                    local_mul = local.get_limb_score_mod( sc.getId(), you.resists_effect( local ) );
                    cur_score *= local_mul;
                }
            }
        }
        s.emplace_back( get_score_text( sc.name().translated(), cur_score, bp_score ) );
    }
    for( const character_modifier &mod : character_modifier::get_all() ) {
        for( const auto &sc : mod.use_limb_scores() ) {
            if( sc.second == 0.0f || sc.first.is_null() || !bp->has_limb_score( sc.first ) ) {
                continue;
            }
            std::string desc = mod.description().translated();
            std::string valstr = colorize( string_format( "%.2f", mod.modifier( you ) ),
                                           limb_score_current_color( part->get_limb_score( you, sc.first ) * sc.second,
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
    speed,
    skills,
    traits,
    bionics,
    effects,
    proficiencies,
    num_tabs,
};
} // namespace

static void draw_x_info( const catacurses::window &w_info, const std::string &description,
                         const unsigned info_line )
{
    std::vector<std::string> text_lines = foldstring( description, FULL_SCREEN_WIDTH - 3 );
    const int winh = catacurses::getmaxy( w_info );
    const bool do_scroll = text_lines.size() > static_cast<unsigned>( std::abs( winh ) );
    const int first_line = do_scroll ? info_line % ( text_lines.size() + 1 - winh ) : 0;
    const int last_line = do_scroll ? first_line + winh : text_lines.size();
    for( int i = first_line; i < last_line; i++ ) {
        trim_and_print( w_info, point( 1, i - first_line ), FULL_SCREEN_WIDTH - ( do_scroll ? 3 : 2 ),
                        c_light_gray, text_lines[i] );
    }
    if( do_scroll ) {
        draw_scrollbar( w_info, first_line, winh, text_lines.size(), point( FULL_SCREEN_WIDTH - 1, 0 ),
                        c_white, true );
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
        ui.set_cursor( win, point::zero );
    }
    center_print( win, 0, title_color, string_format( "[<color_yellow>%s</color>] %s",
                  ctxt.get_desc( "VIEW_PROFICIENCIES" ), _( title_PROFICIENCIES ) ) );
    right_print( win, 0, 0, title_color, string_format( "[<color_yellow>%s</color>]",
                 ctxt.get_desc( "SELECT_PROFICIENCIES_TAB" ) ) );

    const int height = getmaxy( win ) - 1;
    const bool do_draw_scrollbar = height < static_cast<int>( profs.size() );
    const int width = getmaxx( win ) - 1 - ( do_draw_scrollbar ? 1 : 0 );  // -1 for beginning space

    const auto& [i_start, i_end] = subindex_around_cursor( profs.size(), height, line, focused );
    for( size_t i = i_start; i < static_cast<size_t>( i_end ); ++i ) {
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
        const point pos( 1, 1 + i - i_start );
        if( highlight_line ) {
            ui.set_cursor( win, pos );
        }
        print_colored_text( win, pos, col_cur, col, name );
    }
    if( do_draw_scrollbar ) {
        draw_scrollbar( win, i_start, height, profs.size(), point( width + 1, 1 ), c_white, true );
    }
    wnoutrefresh( win );
}

static void draw_proficiencies_info( const catacurses::window &w_info, const unsigned line,
                                     const Character &guy, const unsigned info_line )
{
    werase( w_info );
    const std::vector<display_proficiency> profs = guy.display_proficiencies();
    if( line < profs.size() ) {
        const display_proficiency &cur = profs[line];
        std::string progress;
        if( cur.known ) {
            progress = _( "You know this proficiency." );
        } else {
            progress = string_format( _( "You are %.2f%% of the way towards learning this proficiency." ),
                                      cur.practice * 100 );
            if( debug_mode ) {
                progress += string_format( "\nYou have spent %s practicing this proficiency.",
                                           to_string( cur.spent ) );
            }
        }
        std::string desc = colorize( cur.id->name(), cur.color )
                           + "\n" + colorize( progress, c_cyan )
                           + "\n" + colorize( cur.id->description(), c_white );
        draw_x_info( w_info, desc, info_line );
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
        ui.set_cursor( w_stats, point::zero );
    }
    center_print( w_stats, 0, title_col,
                  string_format( "[<color_yellow>%s</color>] %s",
                                 ctxt.get_desc( "VIEW_BODYSTAT" ), _( title_STATS ) ) );
    right_print( w_stats, 0, 0, title_col, string_format( "[<color_yellow>%s</color>]",
                 ctxt.get_desc( "SELECT_STATS_TAB" ) ) );

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
                        _( "How healthy you feel.  Exercise, vitamins, sleep and not ingesting poison will increase this over time." ) );
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
                                  const player_display_tab curtab, const input_context &ctxt )
{
    werase( w_encumb );
    const bool is_current_tab = curtab == player_display_tab::encumbrance;
    const nc_color title_col = is_current_tab ? h_light_gray : c_light_gray;
    center_print( w_encumb, 0, title_col, _( title_ENCUMB ) );
    right_print( w_encumb, 0, 0, title_col, string_format( "[<color_yellow>%s</color>]",
                 ctxt.get_desc( "SELECT_ENCUMBRANCE_TAB" ) ) );
    if( is_current_tab ) {
        ui.set_cursor( w_encumb, point::zero );
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
    const std::string desc = string_join( s, "\n" );
    draw_x_info( w_info, desc, info_line );
    wnoutrefresh( w_info );
}

static void draw_traits_tab( ui_adaptor &ui, const catacurses::window &w_traits,
                             const unsigned line, const player_display_tab curtab,
                             const std::vector<trait_and_var> &traitslist, const input_context &ctxt )
{
    werase( w_traits );
    const bool is_current_tab = curtab == player_display_tab::traits;
    const nc_color title_col = is_current_tab ? h_light_gray : c_light_gray;
    if( is_current_tab ) {
        ui.set_cursor( w_traits, point::zero );
    }
    center_print( w_traits, 0, title_col, _( title_TRAITS ) );
    right_print( w_traits, 0, 0, title_col, string_format( "[<color_yellow>%s</color>]",
                 ctxt.get_desc( "SELECT_TRAITS_TAB" ) ) );

    const int height = getmaxy( w_traits ) - 1;
    const bool do_draw_scrollbar = height < static_cast<int>( traitslist.size() );
    const int width = getmaxx( w_traits ) - 1 - ( do_draw_scrollbar ? 1 : 0 );
    const auto& [i_start, i_end] = subindex_around_cursor( traitslist.size(), height, line,
                                   is_current_tab );

    for( size_t i = i_start; i < static_cast<size_t>( i_end ); ++i ) {
        const trait_and_var &cur = traitslist[i];
        const nc_color color = cur.trait->get_display_color();
        const bool highlight_line = is_current_tab && i == line;
        const point pos( 1, 1 + i - i_start );
        if( highlight_line ) {
            ui.set_cursor( w_traits, pos );
        }
        trim_and_print( w_traits, pos, width,
                        highlight_line ? hilite( color ) : color, cur.name() );
    }
    if( do_draw_scrollbar ) {
        draw_scrollbar( w_traits, i_start, height, traitslist.size(), point( width + 1, 1 ), c_white,
                        true );
    }
    wnoutrefresh( w_traits );
}

static void draw_traits_info( const catacurses::window &w_info, const Character &you,
                              const unsigned line,
                              const std::vector<trait_and_var> &traitslist, const unsigned info_line )
{
    werase( w_info );
    if( line < traitslist.size() ) {
        const trait_and_var &cur = traitslist[line];
        std::string trait_desc = you.mutation_desc( cur.trait );
        if( !you.purifiable( cur.trait ) ) {
            trait_desc +=
                _( "\n<color_yellow>This trait is an intrinsic part of you now, purifier won't be able to remove it.</color>" );
        }
        const std::string desc =
            string_format( "%s: %s", colorize( cur.name(), cur.trait->get_display_color() ), trait_desc );
        draw_x_info( w_info, desc, info_line );
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
                              const std::vector<bionic_grouping> &bionicslist, const input_context &ctxt )
{
    werase( w_bionics );
    const bool is_current_tab = curtab == player_display_tab::bionics;
    const nc_color title_col = is_current_tab ? h_light_gray : c_light_gray;
    center_print( w_bionics, 0, title_col, _( title_BIONICS ) );
    right_print( w_bionics, 0, 0, title_col, string_format( "[<color_yellow>%s</color>]",
                 ctxt.get_desc( "SELECT_BIONICS_TAB" ) ) );
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
    const auto& [i_start, i_end] = subindex_around_cursor( bionicslist.size(), height, line,
                                   is_current_tab );

    for( size_t i = i_start; i < static_cast<size_t>( i_end ); ++i ) {
        const bool highlight_line = is_current_tab && i == line;
        const point pos( 1, 2 + i - i_start );
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
        draw_scrollbar( w_bionics, i_start, height, bionicslist.size(), point( width + 1, 2 ), c_white,
                        true );
    }
    wnoutrefresh( w_bionics );
}

static void draw_bionics_info( const catacurses::window &w_info, const unsigned line,
                               const std::vector<bionic_grouping> &bionicslist, const unsigned info_line )
{
    werase( w_info );
    if( line < bionicslist.size() ) {
        draw_x_info( w_info, bionicslist[line].description.translated(), info_line );
    }
    wnoutrefresh( w_info );
}

static void draw_effects_tab( ui_adaptor &ui, const catacurses::window &w_effects,
                              const unsigned line, const player_display_tab curtab,
                              const std::vector<std::pair<std::string, std::string>> &effect_name_and_text,
                              const input_context &ctxt )
{
    werase( w_effects );
    const bool is_current_tab = curtab == player_display_tab::effects;
    const nc_color title_col = is_current_tab ? h_light_gray : c_light_gray;
    if( is_current_tab ) {
        ui.set_cursor( w_effects, point::zero );
    }
    center_print( w_effects, 0, title_col, _( title_EFFECTS ) );
    right_print( w_effects, 0, 0, title_col, string_format( "[<color_yellow>%s</color>]",
                 ctxt.get_desc( "SELECT_EFFECTS_TAB" ) ) );
    const int height = getmaxy( w_effects ) - 1;
    const bool do_draw_scrollbar = height < static_cast<int>( effect_name_and_text.size() );
    const int width = getmaxx( w_effects ) - 1 - ( do_draw_scrollbar ? 1 : 0 );
    const auto& [i_start, i_end] = subindex_around_cursor( effect_name_and_text.size(), height, line,
                                   is_current_tab );

    for( size_t i = i_start; i < static_cast<size_t>( i_end ); ++i ) {
        const bool highlight_line = is_current_tab && i == line;
        const point pos( 1, 1 + i - i_start );
        if( highlight_line ) {
            ui.set_cursor( w_effects, pos );
        }
        trim_and_print( w_effects, pos, width,
                        highlight_line ? h_light_gray : c_light_gray, effect_name_and_text[i].first );
    }
    if( do_draw_scrollbar ) {
        draw_scrollbar( w_effects, i_start, height, effect_name_and_text.size(), point( width + 1, 1 ),
                        c_white, true );
    }
    wnoutrefresh( w_effects );
}

static void draw_effects_info( const catacurses::window &w_info, const unsigned line,
                               const std::vector<std::pair<std::string, std::string>> &effect_name_and_text,
                               const unsigned info_line )
{
    werase( w_info );
    const size_t actual_size = effect_name_and_text.size();
    if( line < actual_size ) {
        draw_x_info( w_info, effect_name_and_text[line].second, info_line );
    }
    wnoutrefresh( w_info );
}

static void draw_skills_tab( ui_adaptor &ui, const catacurses::window &w_skills,
                             Character &you, unsigned int line, const player_display_tab curtab,
                             std::vector<HeaderSkill> &skillslist, const input_context &ctxt )
{
    werase( w_skills );
    const bool is_current_tab = curtab == player_display_tab::skills;
    nc_color cstatus = is_current_tab ? h_light_gray : c_light_gray;
    if( is_current_tab ) {
        ui.set_cursor( w_skills, point::zero );
    }
    center_print( w_skills, 0, cstatus, _( title_SKILLS ) );
    right_print( w_skills, 0, 0, c_light_gray, string_format( "[<color_yellow>%s</color>]",
                 ctxt.get_desc( "SELECT_SKILLS_TAB" ) ) );

    const int height = getmaxy( w_skills ) - 1;
    const bool do_draw_scrollbar = height < static_cast<int>( skillslist.size() );
    const int width = getmaxx( w_skills ) - 1 - ( do_draw_scrollbar ? 1 : 0 );

    const auto& [i_start, i_end] = subindex_around_cursor( skillslist.size(), height, line,
                                   is_current_tab );

    int y_pos = 1;
    for( int i = i_start; i < i_end; ++i, ++y_pos ) {
        const Skill *aSkill = skillslist[i].skill;
        if( skillslist[i].is_header ) {
            const SkillDisplayType t = SkillDisplayType::get_skill_type( aSkill->display_category() );
            std::string type_name = t.display_string();
            center_print( w_skills, y_pos, c_yellow, type_name );
        } else {
            const SkillLevel &level = you.get_skill_level_object( aSkill->ident() );
            int exercise = level.knowledgeExperience();
            int level_num = level.knowledgeLevel();
            const bool can_train = level.can_train();
            const bool training = level.isTraining();
            const bool skill_gap = level_num > level.level();
            const bool skill_small_gap = exercise > level.exercise();
            bool locked = false;
            if( you.has_active_bionic( bio_cqb ) && is_cqb_skill( aSkill->ident() ) ) {
                level_num = 5;
                exercise = 0;
                locked = true;
            }
            level_num = you.enchantment_cache->modify_value( aSkill->ident(), level_num );
            if( is_current_tab && i == static_cast<int>( line ) ) {
                ui.set_cursor( w_skills, point( 1, y_pos ) );
                if( locked ) {
                    cstatus = h_yellow;
                } else if( !can_train ) {
                    cstatus = h_white;
                } else if( exercise >= 100 ) {
                    cstatus = training ? h_pink : h_magenta;
                } else if( skill_gap || skill_small_gap ) {
                    cstatus = training ? h_light_cyan : h_cyan;
                } else {
                    cstatus = training ? h_light_blue : h_blue;
                }
            } else {
                if( locked ) {
                    cstatus = c_yellow;
                } else if( skill_gap || skill_small_gap ) {
                    cstatus = training ? c_light_cyan : c_cyan;
                } else if( !can_train ) {
                    cstatus = c_white;
                } else {
                    cstatus = training ? c_light_blue : c_blue;
                }
            }
            mvwprintz( w_skills, point( 1, y_pos ), cstatus, "%s:", aSkill->name() );
            if( aSkill->ident() == skill_dodge ) {
                mvwprintz( w_skills, point( width - 11, y_pos ), cstatus, "%4.1f/%-2d(%2d%%)", you.get_dodge(),
                           level_num, ( exercise < 0 ? 0 : exercise ) );
            } else {
                mvwprintz( w_skills, point( width - 6, y_pos ), cstatus, "%-2d(%2d%%)", level_num,
                           ( exercise < 0 ? 0 : exercise ) );
            }
        }
    }

    if( do_draw_scrollbar ) {
        draw_scrollbar( w_skills, i_start, height, skillslist.size(), point( width + 1, 1 ), c_white,
                        true );
    }
    wnoutrefresh( w_skills );
}

struct speedlist_entry {
    bool is_speed;
    std::string description;
    int val;
    bool percent;
};

static void draw_speed_info( const catacurses::window &w_info,
                             unsigned int line,
                             const std::vector<speedlist_entry> &speedlist )
{
    werase( w_info );

    if( line >= speedlist.size() ) {
        wnoutrefresh( w_info );
        return;
    }

    if( speedlist[line].val != 0 ) {
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        mvwprintz( w_info, point( 1, 0 ), c_light_gray,
                   //~ One of a list of reasons speed or movecost are being changed
                   _( "Cause: %s" ), speedlist[line].description );
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        mvwprintz( w_info, point( 1, 1 ), c_light_gray,
                   //~ speed or movecost reduced by a constant or percentage
                   speedlist[line].val < 0 ? _( "Effect: %1$s reduced by %2$d%3$c" ) :
                   //~ speed or movecost increased by a constant or percentage
                   _( "Effect: %1$s increased by %2$d%3$c" ),
                   speedlist[line].is_speed ? _( "Speed" ) : _( "Move Cost" ),
                   std::abs( speedlist[line].val ),
                   speedlist[line].percent ? '%' : ' ' );
    } else {
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        mvwprintz( w_info, point( 1, 0 ), c_light_gray, speedlist[line].description );
    }

    wnoutrefresh( w_info );
}

static void draw_skills_info( const catacurses::window &w_info, const Character &you,
                              unsigned int line,
                              const std::vector<HeaderSkill> &skillslist, const unsigned info_line )
{
    werase( w_info );

    const Skill *selectedSkill = nullptr;
    if( line < skillslist.size() && !skillslist[line].is_header ) {
        selectedSkill = skillslist[line].skill;
    }

    if( selectedSkill ) {
        const SkillLevel &level = you.get_skill_level_object( selectedSkill->ident() );
        std::string info_text = selectedSkill->description();
        info_text = string_format( _( "%s\n%s\n%s" ), info_text,
                                   selectedSkill->get_level_description( level.knowledgeLevel(), false ),
                                   selectedSkill->get_level_description( level.level(), true ) );
        float level_gap = 100.0f * std::max( level.knowledgeLevel(), 1 ) / std::max( level.level(), 1 );
        if( level.knowledgeLevel() == 1 && level.level() == 0 ) {
            level_gap = 150.0f;
        }
        float learning_bonus = 100.0f * std::max( ( 1.0f + you.get_int() / 40.0f ) - 0.1f *
                               level.exercise() / level.knowledgeExperience(), 1.0f );
        if( level.isRusty() ) {
            info_text = string_format( _( "%s\n\nPractical level: %d (%d%%) " ), info_text,
                                       level.level(), level.exercise() );
            if( level.knowledgeLevel() > level.level() ) {
                info_text = string_format( _( "%s| Learning bonus: %.0f%%" ), info_text,
                                           level_gap );
            } else {
                info_text = string_format( _( "%s| Learning bonus: %.0f%%" ), info_text,
                                           learning_bonus );
            }
            if( !level.isTraining() ) {
                info_text = string_format( "%s | %s", info_text,
                                           _( "<color_yellow>Learning is disabled.</color>" ) );
            }
        } else {
            if( !level.isTraining() ) {
                info_text = string_format( "%s\n\n%s", info_text,
                                           _( "<color_yellow>Learning is disabled.</color>" ) );
            }
        }
        draw_x_info( w_info, info_text, info_line );
    }

    wnoutrefresh( w_info );
}

static std::vector<speedlist_entry> get_speedlist_entries( const Character &you, int &move_cost,
        const std::map<std::string, int> &speed_effects )
{
    std::vector<speedlist_entry> entries;

    for( const std::pair<const std::string, int> &speed_effect : speed_effects ) {
        if( speed_effect.second != 0 ) {
            const speedlist_entry entry { true, speed_effect.first, speed_effect.second, false };
            entries.push_back( entry );
        }
    }

    for( const speed_bonus_effect &effect : you.get_speed_bonus_effects() ) {
        if( effect.bonus != 0 && speed_effects.end() == speed_effects.find( effect.description ) ) {
            const speedlist_entry entry { true, effect.description, effect.bonus, false };
            entries.push_back( entry );
        }
    }


    float movecost = 100;
    for( const run_cost_effect &effect : you.run_cost_effects( movecost ) ) {
        const int percent = std::trunc( effect.times * 100 - 100 ); // truncate toward 0%
        const int constant = std::trunc( effect.plus ); // truncate toward 0
        if( ( effect.times != 1.0 && percent == 0 ) || ( effect.plus != 0 && constant == 0 ) ) {
            continue; // negligible effect
        }
        const speedlist_entry entry { false, effect.description, percent != 0 ? percent : constant, percent != 0 };
        entries.push_back( entry );
    }
    move_cost = static_cast<int>( movecost );

    return entries;
}

static void draw_speed_tab( ui_adaptor &ui, const catacurses::window &w_speed,
                            const unsigned line, const player_display_tab curtab,
                            const Character &you,
                            const std::vector<speedlist_entry> &speedlist, const int move_cost )
{
    werase( w_speed );
    const bool is_current_tab = curtab == player_display_tab::speed;
    if( is_current_tab ) {
        ui.set_cursor( w_speed, point( 0, 2 ) );
    }

    const int speed = you.get_speed();
    center_print( w_speed, 0, c_light_gray, string_format( "%s: <color_%s>%d</color>",
                  _( title_SPEED ), speed >= 100 ? "light_green" : "red", speed ) );
    center_print( w_speed, 1, c_light_gray, string_format( "%s: <color_%s>%d</color>",
                  _( title_MOVE_COST ), move_cost <= 100 ? "light_green" : "red", move_cost ) );

    const int height = getmaxy( w_speed ) - 2;
    const bool do_draw_scrollbar = height < static_cast<int>( speedlist.size() );
    const int width = getmaxx( w_speed ) - 1 - ( do_draw_scrollbar ? 1 : 0 );
    const auto& [i_start, i_end] = subindex_around_cursor( speedlist.size(), height, line,
                                   is_current_tab );

    for( size_t i = i_start; i < static_cast<size_t>( i_end ); ++i ) {
        const bool highlight_line = is_current_tab && i == line;
        const point pos( 1, 2 + i - i_start );
        if( highlight_line ) {
            ui.set_cursor( w_speed, pos );
        }
        const speedlist_entry &entry = speedlist[i];

        // +speed is good, -movecost is good
        const nc_color col = entry.is_speed == ( entry.val > 0 ) ? c_light_green : c_red;
        const nc_color hcol = highlight_line ? hilite( col ) : col;
        mvwprintz( w_speed, pos, hcol, "%s", entry.description );
        //~ single character abbreviations for Speed and Move Cost
        mvwprintz( w_speed, pos + point( 18, 0 ), hcol, "%s", entry.is_speed ? _( "S" ) : _( "M" ) );
        if( entry.val != 0 ) {
            const char prefix = entry.val < 0 ? '-' : '+';
            const char suffix = entry.percent ? '%' : ' ';
            if( std::abs( entry.val ) < 1000 ) {
                mvwprintz( w_speed, pos + point( 19, 0 ), hcol, "%c%3d%c", prefix, std::abs( entry.val ), suffix );
            } else {
                // broken limbs or very big numbers can cause this
                mvwprintz( w_speed, pos + point( 19, 0 ), hcol, "%c!!!%c", prefix, suffix );
            }
        }
    }

    if( do_draw_scrollbar ) {
        draw_scrollbar( w_speed, i_start, height, speedlist.size(), point( width + 1, 2 ),
                        c_white, true );
    }

    wnoutrefresh( w_speed );
}

static void draw_info_window( const catacurses::window &w_info, const Character &you,
                              const unsigned line, const unsigned info_line, const player_display_tab curtab,
                              const std::vector<speedlist_entry> &speedlist,
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
        case player_display_tab::speed:
            draw_speed_info( w_info, line, speedlist );
            break;
        case player_display_tab::skills:
            draw_skills_info( w_info, you, line, skillslist, info_line );
            break;
        case player_display_tab::traits:
            draw_traits_info( w_info, you, line, traitslist, info_line );
            break;
        case player_display_tab::bionics:
            draw_bionics_info( w_info, line, bionicslist, info_line );
            break;
        case player_display_tab::effects:
            draw_effects_info( w_info, line, effect_name_and_text, info_line );
            break;
        case player_display_tab::proficiencies:
            draw_proficiencies_info( w_info, line, you, info_line );
            break;
        case player_display_tab::num_tabs:
            cata_fatal( "Invalid curtab" );
    }
}

static void draw_tip( const catacurses::window &w_tip, const Character &you,
                      const std::string &race, const input_context &ctxt, bool customize_character,
                      int &tip_btn_highlight )
{
    werase( w_tip );

    // Print name and header
    if( you.crossed_threshold() ) {
        //~ player info window: 1s - name, 2s - gender, 3s - Prof or Mutation name
        mvwprintz( w_tip, point::zero, c_white, _( " %1$s | %2$s | %3$s" ), you.get_name(),
                   you.male ? _( "Male" ) : _( "Female" ), race );
    } else {
        std::string profession = you.disp_profession();
        if( !profession.empty() ) {
            mvwprintz( w_tip, point::zero, c_white, _( " %1$s | %2$s | %3$s " ), you.get_name(),
                       you.male ? _( "Male" ) : _( "Female" ), profession );
        } else {
            mvwprintz( w_tip, point::zero, c_white, _( " %1$s | %2$s " ), you.get_name(),
                       you.male ? _( "Male" ) : _( "Female" ) );
        }
    }

    const auto btn_color = [&tip_btn_highlight]( const int btn_to_draw ) {
        if( tip_btn_highlight == btn_to_draw ) {
            return h_light_gray;
        } else {
            return c_light_gray;
        }
    };

    if( customize_character ) {
        right_print( w_tip, 0, 16, btn_color( 0 ), string_format(
                         _( "[<color_yellow>%s</color>] Customize " ),
                         ctxt.get_desc( "SWITCH_GENDER" ) ) );
    }

    right_print( w_tip, 0, 6, btn_color( 1 ), string_format(
                     _( "[<color_yellow>%s</color>] Morale" ),
                     ctxt.get_desc( "morale" ) ) );

    right_print( w_tip, 0, 1, btn_color( 2 ), string_format(
                     _( "[<color_yellow>%s</color>]" ),
                     ctxt.get_desc( "HELP_KEYBINDINGS" ) ) );

    right_print( w_tip, 0, 0, c_light_gray, string_format( "%s", LINE_XOXO_S ) );

    wnoutrefresh( w_tip );
}

static void on_customize_character( Character &you )
{
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
}

static void change_armor_sprite( Character &you )
{
    item_location target_loc;
    target_loc = game_menus::inv::change_sprite( you );
    if( target_loc && target_loc.get_item() ) {
        item *target_item = target_loc.get_item();

        uilist menu;
        menu.title = _( "Change sprite" );
        menu.addentry( 0, true, MENU_AUTOASSIGN, _( "Select sprite from items" ) );
        menu.addentry( 1, true, MENU_AUTOASSIGN, _( "Restore default sprite" ) );
        menu.addentry( 2, true, MENU_AUTOASSIGN, _( "Cancel" ) );

        menu.query();
        if( menu.ret == 0 ) {
            item_location sprite_loc;
            auto armor_filter = [&]( const item & i ) {
                return i.is_armor();
            };
            sprite_loc = game_menus::inv::titled_filter_menu( armor_filter,
                         you,
                         _( "Select appearance of this armor:" ),
                         -1,
                         _( "You have nothing to wear." ) );
            if( sprite_loc && sprite_loc.get_item() ) {
                const item *sprite_item = sprite_loc.get_item();
                const std::string variant = sprite_item->has_itype_variant() ? sprite_item->itype_variant().id : "";
                target_item->set_var( "sprite_override", sprite_item->typeId().str() );
                target_item->set_var( "sprite_override_variant", variant );
            }
        } else if( menu.ret == 1 ) {
            target_item->erase_var( "sprite_override" );
            target_item->erase_var( "sprite_override_variant" );
        }
    }
}

static int get_line_count( player_display_tab curtab,
                           Character &you,
                           const std::vector<speedlist_entry> &speedlist,
                           std::vector<trait_and_var> &traitslist,
                           const std::vector<bionic_grouping> &bionicslist,
                           const std::vector<std::pair<std::string,
                           std::string>> &effect_name_and_text,
                           const std::vector<HeaderSkill> &skillslist )
{
    switch( curtab ) {
        case player_display_tab::stats:
            return 9;
        case player_display_tab::encumbrance: {
            const std::vector<std::pair<bodypart_id, bool>> bps = list_and_combine_bps( you, nullptr );
            return bps.size();
        }
        case player_display_tab::speed:
            return speedlist.size();
        case player_display_tab::traits:
            return traitslist.size();
        case player_display_tab::bionics:
            return bionicslist.size();
        case player_display_tab::effects:
            return effect_name_and_text.size();
        case player_display_tab::skills:
            return skillslist.size();
        case player_display_tab::proficiencies:
            return you.display_proficiencies().size();
        case player_display_tab::num_tabs:
            cata_fatal( "Invalid curtab" );
    }
    return 0;
}

static bool handle_player_display_action( Character &you, unsigned int &line,
        unsigned int &info_line,
        player_display_tab &curtab, input_context &ctxt, const ui_adaptor &ui_tip,
        const ui_adaptor &ui_info, const ui_adaptor &ui_stats, const ui_adaptor &ui_encumb,
        const ui_adaptor &ui_speed, const ui_adaptor &ui_traits, const ui_adaptor &ui_bionics,
        const ui_adaptor &ui_effects, const ui_adaptor &ui_skills, const ui_adaptor &ui_proficiencies,
        const std::vector<speedlist_entry> &speedlist,
        std::vector<trait_and_var> &traitslist,
        const std::vector<bionic_grouping> &bionicslist,
        const std::vector<std::pair<std::string, std::string>> &effect_name_and_text,
        const std::vector<HeaderSkill> &skillslist, bool customize_character,
        const std::vector<catacurses::window *> &windows, const catacurses::window &w_tip,
        int &tip_btn_selected )
{
    const auto invalidate_tab = [&]( const player_display_tab tab ) {
        switch( tab ) {
            case player_display_tab::stats:
                ui_stats.invalidate_ui();
                break;
            case player_display_tab::encumbrance:
                ui_encumb.invalidate_ui();
                break;
            case player_display_tab::speed:
                ui_speed.invalidate_ui();
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

    unsigned int line_count = get_line_count( curtab, you, speedlist, traitslist, bionicslist,
                              effect_name_and_text, skillslist );

    if( line_count > 0 ) {
        line = std::clamp( line, 0U, line_count - 1 );
        if( curtab == player_display_tab::skills ) {
            skip_skill_headers( skillslist, line, true );
        }
    } else {
        line = 0;
    }

    bool done = false;
    const std::string action = ctxt.handle_input();

    if( navigate_ui_list( action, line, 3, line_count, true ) ) {
        if( tip_btn_selected != -1 ) {
            tip_btn_selected = -1;
            ui_tip.invalidate_ui();
        }
        if( curtab == player_display_tab::skills ) {
            // including actios "PAGE_UP", and "HOME", because these shouldn't wrap
            const bool inc = action == "DOWN" || action == "PAGE_DOWN" || action == "PAGE_UP" ||
                             action == "SCROLL_DOWN" || action == "HOME";
            skip_skill_headers( skillslist, line, inc );
        }
        info_line = 0;
        invalidate_tab( curtab );
        ui_info.invalidate_ui();
    } else if( action == "MOUSE_MOVE" ) {
        int orig_tip_btn = tip_btn_selected;
        tip_btn_selected = -1;
        std::optional<point> p;
        for( size_t i = 0; i < windows.size(); i++ ) {
            p = ctxt.get_coordinates_text( *windows[i] );
            if( p.has_value() && window_contains_point_relative( *windows[i], p.value() ) ) {
                invalidate_tab( curtab );
                curtab = player_display_tab( i );
                invalidate_tab( curtab );
                line_count = get_line_count( curtab, you, speedlist, traitslist, bionicslist, effect_name_and_text,
                                             skillslist );
                line = std::clamp( p.value().y - 1, 0, int( line_count ) );
                ui_info.invalidate_ui();
                break;
            }
        }
        p = ctxt.get_coordinates_text( w_tip );
        if( p.has_value() && window_contains_point_relative( w_tip, p.value() ) ) {
            int rightX = catacurses::getmaxx( w_tip ) - p.value().x;
            if( rightX  > 1 && rightX < 5 ) {
                tip_btn_selected = 2;
            } else if( rightX >= 7 && rightX < 17 ) {
                tip_btn_selected = 1;
            } else if( customize_character && rightX > 17  && rightX <= 30 ) {
                tip_btn_selected = 0;
            }
        }
        if( orig_tip_btn != tip_btn_selected ) {
            ui_tip.invalidate_ui();
        }
    } else if( action == "LEFT" || action == "PREV_TAB" || action == "RIGHT" || action == "NEXT_TAB" ) {
        if( tip_btn_selected != -1 ) {
            tip_btn_selected = -1;
            ui_tip.invalidate_ui();
        }
        invalidate_tab( curtab );
        curtab = inc_clamp_wrap( curtab, action == "RIGHT" || action == "NEXT_TAB",
                                 player_display_tab::num_tabs );
        invalidate_tab( curtab );
        line = curtab == player_display_tab::skills ? 1 : 0; // avoid a call to skip_skill_headers
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
    } else if( action == "CONFIRM" || action == "SELECT" ) {
        bool header_clicked = false;
        if( action == "SELECT" ) {
            header_clicked = false;
            int mouse_line = -1;
            if( curtab < player_display_tab::num_tabs ) {
                std::optional<point> p = ctxt.get_coordinates_text( *windows[int( curtab )] );
                if( p.has_value() && window_contains_point_relative( *windows[int( curtab )], p.value() ) ) {
                    mouse_line = std::clamp( p.value().y - 1, 0, int( line_count ) );
                    header_clicked = p.value().y == 0;
                    if( curtab == player_display_tab::skills && skillslist[mouse_line].is_header ) {
                        mouse_line = -1;
                    }
                }
            }
            if( tip_btn_selected != -1 ) {
                switch( tip_btn_selected ) {
                    case 0:
                        if( customize_character ) {
                            on_customize_character( you );
                        }
                        break;
                    case 1:
                        you.disp_morale();
                        break;
                    case 2:
                        ctxt.display_menu();
                        break;
                }
                return done;
            } else if( mouse_line == -1 ) {
                return done;
            }
        }
        switch( curtab ) {
            default:
                break;
            case player_display_tab::stats:
                if( header_clicked ) {
                    display_bodygraph( you );
                } else if( line < 4 && get_option<bool>( "STATS_THROUGH_KILLS" ) && you.is_avatar() ) {
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
                ui_info.invalidate_ui();
                break;
            }
            case player_display_tab::proficiencies:
                const std::vector<display_proficiency> profs = you.display_proficiencies();
                if( !profs.empty() ) {
                    show_proficiencies_window( you, profs[line].id );
                }
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

    } else if( action == "morale" ) {
        you.disp_morale( );
    } else if( action == "VIEW_BODYSTAT" ) {
        display_bodygraph( you );
    } else if( customize_character && action == "SWITCH_GENDER" ) {
        on_customize_character( you );
    } else if( action == "SCROLL_INFOBOX_UP" ) {
        if( info_line > 0 ) {
            --info_line;
            ui_info.invalidate_ui();
        }
    } else if( action == "SCROLL_INFOBOX_DOWN" ) {
        ++info_line;
        ui_info.invalidate_ui();
    } else if( action == "MEDICAL_MENU" ) {
        you.disp_medical();
    } else if( action == "SELECT_STATS_TAB" ) {
        invalidate_tab( curtab );
        curtab = player_display_tab::stats;
        invalidate_tab( curtab );
        line = 0;
        info_line = 0;
        ui_info.invalidate_ui();
    } else if( action == "SELECT_ENCUMBRANCE_TAB" ) {
        invalidate_tab( curtab );
        curtab = player_display_tab::encumbrance;
        invalidate_tab( curtab );
        line = 0;
        info_line = 0;
        ui_info.invalidate_ui();
    } else if( action == "SELECT_SKILLS_TAB" ) {
        invalidate_tab( curtab );
        curtab = player_display_tab::skills;
        invalidate_tab( curtab );
        line = 1; // avoid a call to skip_skill_headers
        info_line = 0;
        ui_info.invalidate_ui();
    } else if( action == "SELECT_TRAITS_TAB" ) {
        invalidate_tab( curtab );
        curtab = player_display_tab::traits;
        invalidate_tab( curtab );
        line = 0;
        info_line = 0;
        ui_info.invalidate_ui();
    } else if( action == "SELECT_BIONICS_TAB" ) {
        invalidate_tab( curtab );
        curtab = player_display_tab::bionics;
        invalidate_tab( curtab );
        line = 0;
        info_line = 0;
        ui_info.invalidate_ui();
    } else if( action == "SELECT_EFFECTS_TAB" ) {
        invalidate_tab( curtab );
        curtab = player_display_tab::effects;
        invalidate_tab( curtab );
        line = 0;
        info_line = 0;
        ui_info.invalidate_ui();
    } else if( action == "SELECT_PROFICIENCIES_TAB" ) {
        invalidate_tab( curtab );
        curtab = player_display_tab::proficiencies;
        invalidate_tab( curtab );
        line = 0;
        info_line = 0;
        ui_info.invalidate_ui();
    } else if( action == "CHANGE_ARMOR_SPRITE" ) {
        change_armor_sprite( you );
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
    return { first_win_size_y_max, second_win_size_y_max };
}

void Character::disp_info( bool customize_character )
{
    // Customizing any character is always enabled in debug mode
    customize_character |= debug_mode;

    std::vector<std::pair<std::string, std::string>> effect_name_and_text;
    for( auto &elem : *effects ) {
        for( auto &_effect_it : elem.second ) {
            const std::string name = _effect_it.second.disp_name();
            effect_name_and_text.emplace_back( name,
                                               _effect_it.second.disp_desc() + '\n' + _effect_it.second.disp_mod_source_info() );
        }
    }
    if( get_perceived_pain() > 0 ) {
        const stat_mod ppen = read_pain_penalty();
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

    const float bmi = get_bmi_fat();

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

        if( bmi < character_weight_category::normal ) {
            const int str_penalty = std::floor( ( 1.0f - ( get_bmi_fat() /
                                                  character_weight_category::normal ) ) * str_max );
            const int dexint_penalty = std::floor( ( character_weight_category::normal - bmi ) * 3.0f );
            starvation_text += std::string( _( "Strength" ) ) + " -" + string_format( "%d\n",
                               str_penalty );
            starvation_text += std::string( _( "Dexterity" ) ) + " -" + string_format( "%d\n",
                               dexint_penalty );
            starvation_text += std::string( _( "Intelligence" ) ) + " -" + string_format( "%d",
                               dexint_penalty );
        }

        effect_name_and_text.emplace_back( starvation_name, starvation_text );
    }

    if( has_trait( trait_TROGLO3 ) && g->is_in_sunlight( pos_bub() ) ) {
        effect_name_and_text.emplace_back( _( "In Sunlight" ),
                                           _( "The sunlight irritates you terribly.\n"
                                              "Strength - 4;    Dexterity - 4;    Intelligence - 4;    Perception - 4" )
                                         );
    } else  if( has_trait( trait_TROGLO2 ) && g->is_in_sunlight( pos_bub() ) ) {
        if( incident_sun_irradiance( get_weather().weather_id, calendar::turn ) > irradiance::moderate ) {
            effect_name_and_text.emplace_back( _( "In Sunlight" ),
                                               _( "The sunlight irritates you badly.\n"
                                                  "Strength - 2;    Dexterity - 2;    Intelligence - 2;    Perception - 2" ) );
        } else if( incident_sun_irradiance( get_weather().weather_id, calendar::turn ) > irradiance::low ) {
            effect_name_and_text.emplace_back( _( "In Sunlight" ),
                                               _( "The sunlight irritates you badly.\n"
                                                  "Strength - 1;    Dexterity - 1;    Intelligence - 1;    Perception - 1" ) );
        }

    } else if( has_trait( trait_TROGLO ) && g->is_in_sunlight( pos_bub() ) &&
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

    for( const std::pair<std::string, std::string> &detail : enchantment_cache->details ) {
        effect_name_and_text.emplace_back( detail );
    }

    // If any effects with no names have cropped up, let's remove them.
    //   NB: This can happen if an effect no longer exists.
    effect_name_and_text.erase( std::remove_if( effect_name_and_text.begin(),
    effect_name_and_text.end(), []( const std::pair<std::string, std::string> &e ) {
        return e.first.empty();
    } ), effect_name_and_text.end() );

    const unsigned int effect_win_size_y_max = 1 + effect_name_and_text.size();
    const unsigned int proficiency_win_size_y_max = 1 + display_proficiencies().size();

    std::vector<trait_and_var> traitslist = get_mutations_variants( false );
    std::sort( traitslist.begin(), traitslist.end(), trait_var_display_sort );
    const unsigned int trait_win_size_y_max = 1 + traitslist.size();

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
        return a.get_sort_rank() < b.get_sort_rank();
    } );

    std::vector<HeaderSkill> skillslist = get_HeaderSkills( player_skill );
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
    ctxt.register_navigate_ui_list();
    ctxt.register_action( "COORDINATE" );
    ctxt.register_action( "SELECT" );
    ctxt.register_action( "MOUSE_MOVE" );
    ctxt.register_action( "LEFT", to_translation( "Cycle to next category" ) );
    ctxt.register_action( "RIGHT", to_translation( "Cycle to previous category" ) );
    ctxt.register_action( "NEXT_TAB", to_translation( "Cycle to next category" ) );
    ctxt.register_action( "PREV_TAB", to_translation( "Cycle to previous category" ) );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "CONFIRM", to_translation( "Toggle skill training / Upgrade stat" ) );
    ctxt.register_action( "CHANGE_PROFESSION_NAME", to_translation( "Change profession name" ) );
    ctxt.register_action( "SWITCH_GENDER", to_translation( "Customize base appearance and name" ) );
    ctxt.register_action( "VIEW_PROFICIENCIES", to_translation( "View character proficiencies" ) );
    ctxt.register_action( "VIEW_BODYSTAT", to_translation( "View character's body status" ) );
    ctxt.register_action( "morale" );
    ctxt.register_action( "SCROLL_INFOBOX_UP", to_translation( "Scroll information box up" ) );
    ctxt.register_action( "SCROLL_INFOBOX_DOWN", to_translation( "Scroll information box down" ) );
    ctxt.register_action( "SELECT_TRAIT_VARIANT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "MEDICAL_MENU" );
    ctxt.register_action( "CHANGE_ARMOR_SPRITE" );
    ctxt.register_action( "SELECT_STATS_TAB" );
    ctxt.register_action( "SELECT_ENCUMBRANCE_TAB" );
    ctxt.register_action( "SELECT_SKILLS_TAB" );
    ctxt.register_action( "SELECT_TRAITS_TAB" );
    ctxt.register_action( "SELECT_BIONICS_TAB" );
    ctxt.register_action( "SELECT_EFFECTS_TAB" );
    ctxt.register_action( "SELECT_PROFICIENCIES_TAB" );

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

    int move_cost;
    std::vector<speedlist_entry> speedlist = get_speedlist_entries( *this, move_cost, speed_effects );

    border_helper borders;

    player_display_tab curtab = player_display_tab::stats;
    // line marks at which line is selection in tab window, could be named tab_line
    unsigned int line = 0;
    // info_line marks at which line is selection in info window
    unsigned int info_line = 0;
    int tip_btn_highlight = -1;

    catacurses::window w_tip;
    ui_adaptor ui_tip;
    ui_tip.on_screen_resize( [&]( ui_adaptor & ui_tip ) {
        w_tip = catacurses::newwin( 1, FULL_SCREEN_WIDTH + 1, point::zero );
        ui_tip.position_from_window( w_tip );
    } );
    ui_tip.mark_resize();
    ui_tip.on_redraw( [&]( ui_adaptor & ui_tip ) {
        ui_tip.disable_cursor();
        draw_tip( w_tip, *this, race, ctxt, customize_character, tip_btn_highlight );
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
                    TERMY - infooffsetybottom, trait_win_size_y_max, bionics_win_size_y_max );
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
        draw_traits_tab( ui_traits, w_traits, line, curtab, traitslist, ctxt );
    } );

    // BIONICS
    catacurses::window w_bionics;
    catacurses::window w_bionics_border;
    border_helper::border_info &border_bionics = borders.add_border();
    ui_adaptor ui_bionics;
    ui_bionics.on_screen_resize( [&]( ui_adaptor & ui_bionics ) {
        std::tie( trait_win_size_y, bionics_win_size_y ) = calculate_shared_column_win_height(
                    TERMY - infooffsetybottom, trait_win_size_y_max, bionics_win_size_y_max );
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
        draw_bionics_tab( ui_bionics, w_bionics, *this, line, curtab, bionicslist, ctxt );
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
        draw_encumbrance_tab( ui_encumb, w_encumb, *this, line, curtab, ctxt );
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
                    TERMY - infooffsetybottom, effect_win_size_y_max, proficiency_win_size_y_max );
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
        draw_effects_tab( ui_effects, w_effects, line, curtab, effect_name_and_text, ctxt );
    } );

    // PROFICIENCIES
    catacurses::window w_proficiencies;
    catacurses::window w_proficiencies_border;
    border_helper::border_info &border_proficiencies = borders.add_border();
    ui_adaptor ui_proficiencies;
    ui_proficiencies.on_screen_resize( [&]( ui_adaptor & ui_proficiencies ) {
        std::tie( effect_win_size_y, proficiency_win_size_y ) = calculate_shared_column_win_height(
                    TERMY - infooffsetybottom, effect_win_size_y_max, proficiency_win_size_y_max );
        const point profstart = point( grid_width * 2 + 2, infooffsetybottom + effect_win_size_y + 1 );
        w_proficiencies = catacurses::newwin( proficiency_win_size_y, grid_width,
                                              profstart );
        w_proficiencies_border = catacurses::newwin( proficiency_win_size_y + 1, grid_width + 2,
                                 profstart + point::west );
        border_proficiencies.set( profstart + point::north_west, point( grid_width + 2,
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
        draw_skills_tab( ui_skills, w_skills, *this, line, curtab, skillslist, ctxt );
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
                          speedlist, traitslist, bionicslist, effect_name_and_text, skillslist );
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
        draw_speed_tab( ui_speed, w_speed, line, curtab, *this, speedlist, move_cost );
    } );

    bool done = false;

    // needs to have the same windows in the same order as player_display_tab so that mouse scrolling can work
    std::vector<catacurses::window *> windows{ &w_stats, &w_encumb, &w_speed, &w_skills, &w_traits, &w_bionics, &w_effects, &w_proficiencies };

    do {
        ui_manager::redraw_invalidated();

        done = handle_player_display_action( *this, line, info_line, curtab, ctxt, ui_tip, ui_info,
                                             ui_stats, ui_encumb, ui_speed, ui_traits, ui_bionics, ui_effects,
                                             ui_skills, ui_proficiencies, speedlist, traitslist, bionicslist,
                                             effect_name_and_text, skillslist, customize_character,
                                             windows, w_tip, tip_btn_highlight );
    } while( !done );
}
