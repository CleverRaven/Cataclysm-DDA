#include "player.h" // IWYU pragma: associated

#include <cstdlib>
#include <algorithm>
#include <cstddef>

#include "addiction.h"
#include "avatar.h"
#include "bionics.h"
#include "cata_utility.h"
#include "effect.h"
#include "game.h"
#include "input.h"
#include "mutation.h"
#include "options.h"
#include "output.h"
#include "profession.h"
#include "skill.h"
#include "string_formatter.h"
#include "units.h"
#include "weather.h"
#include "catacharset.h"
#include "translations.h"
#include "string_id.h"
#include "enums.h"

const skill_id skill_swimming( "swimming" );

static const std::string title_STATS = translate_marker( "STATS" );
static const std::string title_ENCUMB = translate_marker( "ENCUMBRANCE AND WARMTH" );
static const std::string title_EFFECTS = translate_marker( "EFFECTS" );
static const std::string title_SPEED = translate_marker( "SPEED" );
static const std::string title_SKILLS = translate_marker( "SKILLS" );
static const std::string title_BIONICS = translate_marker( "BIONICS" );
static const std::string title_TRAITS = translate_marker( "TRAITS" );

// use this instead of having to type out 26 spaces like before
static const std::string header_spaces( 26, ' ' );

// Rescale temperature value to one that the player sees
static int temperature_print_rescaling( int temp )
{
    return ( temp / 100.0 ) * 2 - 100;
}

static body_part other_part( body_part bp )
{
    return static_cast<body_part>( bp_aiOther[bp] );
}

static bool should_combine_bps( const player &p, body_part l, body_part r,
                                const item *selected_clothing )
{
    const auto enc_data = p.get_encumbrance();
    return l != r && // are different parts
           l == other_part( r ) && r == other_part( l ) && // are complementary parts
           // same encumberance & temperature
           enc_data[l] == enc_data[r] &&
           temperature_print_rescaling( p.temp_conv[l] ) == temperature_print_rescaling( p.temp_conv[r] ) &&
           // selected_clothing covers both or neither parts
           ( !selected_clothing || ( selected_clothing->covers( l ) == selected_clothing->covers( r ) ) );
}

static std::vector<std::pair<body_part, bool>> list_and_combine_bps( const player &p,
        const item *selected_clothing )
{
    // bool represents whether the part has been combined with its other half
    std::vector<std::pair<body_part, bool>> bps;
    for( auto bp : all_body_parts ) {
        // assuming that a body part has at most one other half
        if( other_part( other_part( bp ) ) != bp ) {
            debugmsg( "Bodypart %d has more than one other half!", bp );
        }
        if( should_combine_bps( p, bp, other_part( bp ), selected_clothing ) ) {
            if( bp < other_part( bp ) ) {
                // only add the earlier one
                bps.emplace_back( bp, true );
            }
        } else {
            bps.emplace_back( bp, false );
        }
    }
    return bps;
}

void player::print_encumbrance( const catacurses::window &win, const int line,
                                const item *const selected_clothing ) const
{
    // bool represents whether the part has been combined with its other half
    const std::vector<std::pair<body_part, bool>> bps = list_and_combine_bps( *this,
            selected_clothing );

    // width/height excluding title & scrollbar
    const int height = getmaxy( win ) - 1;
    bool draw_scrollbar = height < static_cast<int>( bps.size() );
    const int width = getmaxx( win ) - ( draw_scrollbar ? 1 : 0 );
    // index of the first printed bodypart from `bps`
    const int firstline = clamp( line - height / 2, 0, std::max( 0,
                                 static_cast<int>( bps.size() ) - height ) );

    /*** I chose to instead only display X+Y instead of X+Y=Z. More room was needed ***
     *** for displaying triple digit encumbrance, due to new encumbrance system.    ***
     *** If the player wants to see the total without having to do them maths, the  ***
     *** armor layers ui shows everything they want :-) -Davek                      ***/
    const auto enc_data = get_encumbrance();
    for( int i = 0; i < height; ++i ) {
        int thisline = firstline + i;
        if( thisline < 0 ) {
            continue;
        }
        if( static_cast<size_t>( thisline ) >= bps.size() ) {
            break;
        }
        const body_part bp = bps[thisline].first;
        const bool combine = bps[thisline].second;
        const encumbrance_data &e = enc_data[bp];
        const bool highlighted = selected_clothing ? selected_clothing->covers( bp ) : false;
        std::string out = body_part_name_as_heading( bp, combine ? 2 : 1 );
        if( utf8_width( out ) > 7 ) {
            out = utf8_truncate( out, 7 );
        }

        // Two different highlighting schemes, highlight if the line is selected as per line being set.
        // Make the text green if this part is covered by the passed in item.
        nc_color limb_color = thisline == line ?
                              ( highlighted ? h_green : h_light_gray ) :
                              ( highlighted ? c_green : c_light_gray );
        mvwprintz( win, point( 1, 1 + i ), limb_color, "%s", out );
        // accumulated encumbrance from clothing, plus extra encumbrance from layering
        mvwprintz( win, point( 8, 1 + i ), encumb_color( e.encumbrance ), "%3d", e.armor_encumbrance );
        // separator in low toned color
        mvwprintz( win, point( 11, 1 + i ), c_light_gray, "+" );
        // take into account the new encumbrance system for layers
        mvwprintz( win, point( 12, 1 + i ), encumb_color( e.encumbrance ), "%-3d", e.layer_penalty );
        // print warmth, tethered to right hand side of the window
        mvwprintz( win, point( width - 6, 1 + i ), bodytemp_color( bp ), "(% 3d)",
                   temperature_print_rescaling( temp_conv[bp] ) );
    }

    if( draw_scrollbar ) {
        scrollbar().
        offset_x( width ).
        offset_y( 1 ).
        content_size( bps.size() ).
        viewport_pos( firstline ).
        viewport_size( height ).
        scroll_to_last( false ).
        apply( win );
    }
}

static std::string swim_cost_text( int moves )
{
    return string_format( ngettext( "Swimming costs %+d movement point. ",
                                    "Swimming costs %+d movement points. ",
                                    moves ),
                          moves );
}

static std::string run_cost_text( int moves )
{
    return string_format( ngettext( "Running costs %+d movement point. ",
                                    "Running costs %+d movement points. ",
                                    moves ),
                          moves );
}

static std::string reload_cost_text( int moves )
{
    return string_format( ngettext( "Reloading costs %+d movement point. ",
                                    "Reloading costs %+d movement points. ",
                                    moves ),
                          moves );
}

static std::string melee_cost_text( int moves )
{
    return string_format( ngettext( "Melee and thrown attacks cost %+d movement point. ",
                                    "Melee and thrown attacks cost %+d movement points. ",
                                    moves ),
                          moves );
}

static std::string dodge_skill_text( double mod )
{
    return string_format( _( "Dodge skill %+.1f. " ), mod );
}

static int get_encumbrance( const player &p, body_part bp, bool combine )
{
    // Body parts that can't combine with anything shouldn't print double values on combine
    // This shouldn't happen, but handle this, just in case
    const bool combines_with_other = static_cast<int>( bp_aiOther[bp] ) != bp;
    return p.encumb( bp ) * ( ( combine && combines_with_other ) ? 2 : 1 );
}

static std::string get_encumbrance_description( const player &p, body_part bp, bool combine )
{
    std::string s;

    const int eff_encumbrance = get_encumbrance( p, bp, combine );

    switch( bp ) {
        case bp_torso: {
            const int melee_roll_pen = std::max( -eff_encumbrance, -80 );
            s += string_format( _( "Melee attack rolls %+d%%; " ), melee_roll_pen );
            s += dodge_skill_text( -( eff_encumbrance / 10.0 ) );
            s += swim_cost_text( ( eff_encumbrance / 10.0 ) * ( 80 - p.get_skill_level(
                                     skill_swimming ) * 3 ) );
            s += melee_cost_text( eff_encumbrance );
            break;
        }
        case bp_head:
            s += _( "Head encumbrance has no effect; it simply limits how much you can put on." );
            break;
        case bp_eyes:
            s += string_format( _( "Perception %+d when checking traps or firing ranged weapons;\n"
                                   "Dispersion %+d when throwing items." ),
                                -( eff_encumbrance / 10 ),
                                eff_encumbrance * 10 );
            break;
        case bp_mouth:
            s += _( "Covering your mouth will make it more difficult to breathe and catch your breath." );
            break;
        case bp_arm_l:
        case bp_arm_r:
            s += _( "Arm encumbrance affects stamina cost of melee attacks and accuracy with ranged weapons." );
            break;
        case bp_hand_l:
        case bp_hand_r:
            s += _( "Reduces the speed at which you can handle or manipulate items\n" );
            s += reload_cost_text( ( eff_encumbrance / 10 ) * 15 );
            s += string_format( _( "Dexterity %+.1f when throwing items;\n" ), -( eff_encumbrance / 10.0f ) );
            s += melee_cost_text( eff_encumbrance / 2 );
            s += "\n";
            s += string_format( _( "Reduces aim speed of guns by %.1f." ), p.aim_speed_encumbrance_modifier() );
            break;
        case bp_leg_l:
        case bp_leg_r:
            s += run_cost_text( static_cast<int>( eff_encumbrance * 0.15 ) );
            s += swim_cost_text( ( eff_encumbrance / 10 ) * ( 50 - p.get_skill_level(
                                     skill_swimming ) * 2 ) / 2 );
            s += dodge_skill_text( -eff_encumbrance / 10.0 / 4.0 );
            break;
        case bp_foot_l:
        case bp_foot_r:
            s += run_cost_text( static_cast<int>( eff_encumbrance * 0.25 ) );
            break;
        case num_bp:
            break;
    }

    return s;
}

static bool is_cqb_skill( const skill_id &id )
{
    // TODO: this skill list here is used in other places as well. Useless redundancy and
    // dependency. Maybe change it into a flag of the skill that indicates it's a skill used
    // by the bionic?
    static const std::array<skill_id, 5> cqb_skills = { {
            skill_id( "melee" ), skill_id( "unarmed" ), skill_id( "cutting" ),
            skill_id( "bashing" ), skill_id( "stabbing" ),
        }
    };
    return std::find( cqb_skills.begin(), cqb_skills.end(), id ) != cqb_skills.end();
}

static void draw_stats_tab( const catacurses::window &w_stats, const catacurses::window &w_info,
                            player &you, unsigned int &line, int &curtab, input_context &ctxt, bool &done,
                            std::string &action )
{

    mvwprintz( w_stats, point_zero, h_light_gray, header_spaces );
    center_print( w_stats, 0, h_light_gray, _( title_STATS ) );

    // Clear bonus/penalty menu.
    mvwprintz( w_stats, point( 0, 7 ), c_light_gray, "%26s", "" );
    mvwprintz( w_stats, point( 0, 8 ), c_light_gray, "%26s", "" );

    nc_color col_temp = c_light_gray;

    if( line == 0 ) {
        // Display information on player strength in appropriate window
        mvwprintz( w_stats, point( 1, 2 ), h_light_gray, _( "Strength:" ) );
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        fold_and_print( w_info, point( 1, 0 ), FULL_SCREEN_WIDTH - 2, c_magenta,
                        _( "Strength affects your melee damage, the amount of weight you can carry, your total HP, "
                           "your resistance to many diseases, and the effectiveness of actions which require brute force." ) );
        print_colored_text( w_info, point( 1, 3 ), col_temp, c_light_gray,
                            string_format( _( "Base HP: <color_white>%d</color>" ), you.hp_max[1] ) );
        print_colored_text( w_info, point( 1, 4 ), col_temp, c_light_gray,
                            string_format( _( "Carry weight (%s): <color_white>%.1f</color>" ), weight_units(),
                                           convert_weight( you.weight_capacity() ) ) );
        print_colored_text( w_info, point( 1, 5 ), col_temp, c_light_gray,
                            string_format( _( "Melee damage: <color_white>%.1f</color>" ), you.bonus_damage( false ) ) );
    } else if( line == 1 ) {
        // Display information on player dexterity in appropriate window
        mvwprintz( w_stats, point( 1, 3 ), h_light_gray, _( "Dexterity:" ) );
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        fold_and_print( w_info, point( 1, 0 ), FULL_SCREEN_WIDTH - 2, c_magenta,
                        _( "Dexterity affects your chance to hit in melee combat, helps you steady your "
                           "gun for ranged combat, and enhances many actions that require finesse." ) );
        print_colored_text( w_info, point( 1, 3 ), col_temp, c_light_gray,
                            string_format( _( "Melee to-hit bonus: <color_white>%+.1lf</color>" ), you.get_hit_base() ) );
        print_colored_text( w_info, point( 1, 4 ), col_temp, c_light_gray,
                            string_format( _( "Ranged penalty: <color_white>%+d</color>" ),
                                           -abs( you.ranged_dex_mod() ) ) );
        print_colored_text( w_info, point( 1, 5 ), col_temp, c_light_gray,
                            string_format( _( "Throwing penalty per target's dodge: <color_white>%+d</color>" ),
                                           you.throw_dispersion_per_dodge( false ) ) );
    } else if( line == 2 ) {
        // Display information on player intelligence in appropriate window
        mvwprintz( w_stats, point( 1, 4 ), h_light_gray, _( "Intelligence:" ) );
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        fold_and_print( w_info, point( 1, 0 ), FULL_SCREEN_WIDTH - 2, c_magenta,
                        _( "Intelligence is less important in most situations, but it is vital for more complex tasks like "
                           "electronics crafting.  It also affects how much skill you can pick up from reading a book." ) );
        print_colored_text( w_info, point( 1, 3 ), col_temp, c_light_gray,
                            string_format( _( "Read times: <color_white>%d%%</color>" ), you.read_speed( false ) ) );
        print_colored_text( w_info, point( 1, 4 ), col_temp, c_light_gray,
                            string_format( _( "Crafting bonus: <color_white>%d%%</color>" ), you.get_int() ) );
        if( you.rust_rate() ) {
            print_colored_text( w_info, point( 1, 5 ), col_temp, c_light_gray,
                                string_format( _( "Skill rust: <color_white>%d%%</color>" ), you.rust_rate( false ) ) );
        }
    } else if( line == 3 ) {
        // Display information on player perception in appropriate window
        mvwprintz( w_stats, point( 1, 5 ), h_light_gray, _( "Perception:" ) );
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
        mvwprintz( w_stats, point( 1, 6 ), h_light_gray, _( "Weight:" ) );
        mvwprintz( w_stats, point( 25 - you.get_weight_string().size(), 6 ), h_light_gray,
                   you.get_weight_string() );
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        const int lines = fold_and_print( w_info, point( 1, 0 ), FULL_SCREEN_WIDTH - 2, c_magenta,
                                          _( "Your weight is a general indicator of how much fat your body has stored up,"
                                             " which in turn shows how prepared you are to survive for a time without food."
                                             "Having too much, or too little, can be unhealthy." ) );
        fold_and_print( w_info, point( 1, 1 + lines ), FULL_SCREEN_WIDTH - 2, c_magenta,
                        you.get_weight_description() );
    }
    wrefresh( w_stats );
    wrefresh( w_info );

    action = ctxt.handle_input();
    if( action == "DOWN" ) {
        line++;
        if( line == 5 ) {
            line = 0;
        }
    } else if( action == "UP" ) {
        if( line == 0 ) {
            line = 4;
        } else {
            line--;
        }
    } else if( action == "NEXT_TAB" || action == "PREV_TAB" ) {
        mvwprintz( w_stats, point_zero, c_light_gray, header_spaces );
        center_print( w_stats, 0, c_light_gray, _( title_STATS ) );
        wrefresh( w_stats );
        line = 0;
        curtab = action == "NEXT_TAB" ? curtab + 1 : 6;
    } else if( action == "QUIT" ) {
        done = true;
    } else if( action == "CONFIRM" && line < 4 && get_option<bool>( "STATS_THROUGH_KILLS" ) &&
               you.is_player() ) {
        g->u.upgrade_stat_prompt( static_cast<Character::stat>( line ) );
    }
    mvwprintz( w_stats, point( 1, 2 ), c_light_gray, _( "Strength:" ) );
    mvwprintz( w_stats, point( 1, 3 ), c_light_gray, _( "Dexterity:" ) );
    mvwprintz( w_stats, point( 1, 4 ), c_light_gray, _( "Intelligence:" ) );
    mvwprintz( w_stats, point( 1, 5 ), c_light_gray, _( "Perception:" ) );
    mvwprintz( w_stats, point( 1, 6 ), c_light_gray, _( "Weight:" ) );
    mvwprintz( w_stats, point( 25 - you.get_weight_string().size(), 6 ), c_light_gray,
               you.get_weight_string() );
    wrefresh( w_stats );
}

static void draw_encumbrance_tab( const catacurses::window &w_encumb,
                                  const catacurses::window &w_info, player &you, unsigned int &line, int &curtab, input_context &ctxt,
                                  bool &done, std::string &action )
{
    const std::vector<std::pair<body_part, bool>> bps = list_and_combine_bps( you, nullptr );

    werase( w_encumb );
    center_print( w_encumb, 0, h_light_gray, _( title_ENCUMB ) );
    you.print_encumbrance( w_encumb, line );
    wrefresh( w_encumb );

    werase( w_info );
    body_part bp = num_bp;
    bool combined_here = false;
    if( line < bps.size() ) {
        bp = bps[line].first;
        combined_here = bps[line].second;
    }
    const std::string s = get_encumbrance_description( you, bp, combined_here );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    fold_and_print( w_info, point( 1, 0 ), FULL_SCREEN_WIDTH - 2, c_magenta, s );
    wrefresh( w_info );

    action = ctxt.handle_input();
    if( action == "DOWN" ) {
        if( line + 1 < bps.size() ) {
            ++line;
        }
    } else if( action == "UP" ) {
        if( line > 0 ) {
            --line;
        }
    } else if( action == "NEXT_TAB" || action == "PREV_TAB" ) {
        mvwprintz( w_encumb, point_zero, c_light_gray, header_spaces );
        center_print( w_encumb, 0, c_light_gray, _( title_ENCUMB ) );
        wrefresh( w_encumb );
        line = 0;
        curtab = action == "NEXT_TAB" ? curtab + 1 : curtab - 1;
    } else if( action == "QUIT" ) {
        done = true;
    }
}

static void draw_traits_tab( const catacurses::window &w_traits, const catacurses::window &w_info,
                             unsigned int &line, int &curtab, input_context &ctxt, bool &done,
                             std::string &action, const std::vector<trait_id> &traitslist,
                             const size_t trait_win_size_y )
{
    werase( w_traits );
    mvwprintz( w_traits, point_zero, h_light_gray, header_spaces );
    center_print( w_traits, 0, h_light_gray, _( title_TRAITS ) );

    size_t min = 0;
    size_t max = 0;

    if( line <= ( trait_win_size_y - 1 ) / 2 ) {
        min = 0;
        max = trait_win_size_y;
        if( traitslist.size() < max ) {
            max = traitslist.size();
        }
    } else if( line >= traitslist.size() - ( trait_win_size_y + 1 ) / 2 ) {
        min = ( traitslist.size() < trait_win_size_y ? 0 : traitslist.size() - trait_win_size_y );
        max = traitslist.size();
    } else {
        min = line - ( trait_win_size_y - 1 ) / 2;
        max = line + trait_win_size_y / 2 + 1;
        if( traitslist.size() < max ) {
            max = traitslist.size();
        }
    }

    for( size_t i = min; i < max; i++ ) {
        const auto &mdata = traitslist[i].obj();
        const auto color = mdata.get_display_color();
        trim_and_print( w_traits, point( 1, static_cast<int>( 1 + i - min ) ), getmaxx( w_traits ) - 1,
                        i == line ? hilite( color ) : color, mdata.name() );
    }
    if( line < traitslist.size() ) {
        const auto &mdata = traitslist[line].obj();
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        fold_and_print( w_info, point( 1, 0 ), FULL_SCREEN_WIDTH - 2, c_magenta, string_format(
                            "%s: %s", colorize( mdata.name(), mdata.get_display_color() ), traitslist[line]->desc() ) );
    }
    wrefresh( w_traits );
    wrefresh( w_info );

    action = ctxt.handle_input();
    if( action == "DOWN" ) {
        if( line < traitslist.size() - 1 ) {
            line++;
        }
        return;
    } else if( action == "UP" ) {
        if( line > 0 ) {
            line--;
        }
    } else if( action == "NEXT_TAB" || action == "PREV_TAB" ) {
        mvwprintz( w_traits, point_zero, c_light_gray, header_spaces );
        center_print( w_traits, 0, c_light_gray, _( title_TRAITS ) );
        for( size_t i = 0; i < traitslist.size() && i < trait_win_size_y; i++ ) {
            const auto &mdata = traitslist[i].obj();
            mvwprintz( w_traits, point( 1, static_cast<int>( i + 1 ) ), c_black, "                         " );
            const auto color = mdata.get_display_color();
            trim_and_print( w_traits, point( 1, static_cast<int>( i + 1 ) ), getmaxx( w_traits ) - 1,
                            color, mdata.name() );
        }
        wrefresh( w_traits );
        line = 0;
        curtab = action == "NEXT_TAB" ? curtab + 1 : curtab - 1;
    } else if( action == "QUIT" ) {
        done = true;
    }
}

static void draw_bionics_tab( const catacurses::window &w_bionics, const catacurses::window &w_info,
                              player &you, unsigned int &line, int &curtab, input_context &ctxt, bool &done,
                              std::string &action, std::vector<bionic> &bionicslist,
                              const size_t bionics_win_size_y )
{
    werase( w_bionics );
    mvwprintz( w_bionics, point_zero, h_light_gray, header_spaces );
    center_print( w_bionics, 0, h_light_gray, _( title_BIONICS ) );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    trim_and_print( w_bionics, point( 1, 1 ), getmaxx( w_bionics ) - 1, c_white,
                    string_format( _( "Bionic Power: <color_light_blue>%1$d / %2$d</color>" ),
                                   units::to_kilojoule( you.get_power_level() ), units::to_kilojoule( you.get_max_power_level() ) ) );

    const size_t useful_y = bionics_win_size_y - 1;
    const size_t half_y = useful_y / 2;

    size_t min = 0;
    size_t max = 0;

    if( line <= half_y ) { // near the top
        min = 0;
        max = std::min( bionicslist.size(), useful_y );
    } else if( line >= bionicslist.size() - half_y ) { // near the bottom
        min = ( bionicslist.size() <= useful_y ? 0 : bionicslist.size() - useful_y );
        max = bionicslist.size();
    } else { // scrolling
        min = line - half_y;
        max = std::min( bionicslist.size(), line + useful_y - half_y );
    }

    for( size_t i = min; i < max; i++ ) {
        trim_and_print( w_bionics, point( 1, static_cast<int>( 2 + i - min ) ), getmaxx( w_bionics ) - 1,
                        i == line ? hilite( c_white ) : c_white, "%s", bionicslist[i].info().name );
    }
    if( line < bionicslist.size() ) {
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        fold_and_print( w_info, point( 1, 0 ), FULL_SCREEN_WIDTH - 2, c_white, "%s",
                        bionicslist[line].info().description );
    }
    wrefresh( w_bionics );
    wrefresh( w_info );

    action = ctxt.handle_input();
    if( action == "DOWN" ) {
        if( line < bionicslist.size() - 1 ) {
            line++;
        }
        return;
    } else if( action == "UP" ) {
        if( line > 0 ) {
            line--;
        }
    } else if( action == "NEXT_TAB" || action == "PREV_TAB" ) {
        mvwprintz( w_bionics, point_zero, c_light_gray, header_spaces );
        center_print( w_bionics, 0, c_light_gray, _( title_BIONICS ) );
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        trim_and_print( w_bionics, point( 1, 1 ), getmaxx( w_bionics ) - 1, c_white,
                        string_format( _( "Bionic Power: <color_light_blue>%1$d / %2$d</color>" ),
                                       units::to_kilojoule( you.get_power_level() ), units::to_kilojoule( you.get_max_power_level() ) ) );
        for( size_t i = 0; i < bionicslist.size() && i < bionics_win_size_y - 1; i++ ) {
            mvwprintz( w_bionics, point( 1, static_cast<int>( i + 2 ) ), c_black, "                         " );
            trim_and_print( w_bionics, point( 1, static_cast<int>( i + 2 ) ), getmaxx( w_bionics ) - 1,
                            c_white, "%s", bionicslist[i].info().name );
        }
        wrefresh( w_bionics );
        line = 0;
        curtab = action == "NEXT_TAB" ? curtab + 1 : curtab - 1;
    } else if( action == "QUIT" ) {
        done = true;
    }
}

static void draw_effects_tab( const catacurses::window &w_effects, const catacurses::window &w_info,
                              unsigned int &line, int &curtab, input_context &ctxt, bool &done,
                              std::string &action, std::vector<std::pair<std::string, std::string>> &effect_name_and_text,
                              const size_t effect_win_size_y )
{
    mvwprintz( w_effects, point_zero, h_light_gray, header_spaces );
    center_print( w_effects, 0, h_light_gray, _( title_EFFECTS ) );

    const size_t half_y = effect_win_size_y / 2;

    size_t min = 0;
    size_t max = 0;

    const size_t actual_size = effect_name_and_text.size();

    if( line <= half_y ) {
        min = 0;
        max = effect_win_size_y;
        if( actual_size < max ) {
            max = actual_size;
        }
    } else if( line >= actual_size - half_y ) {
        min = ( actual_size < effect_win_size_y ? 0 : actual_size - effect_win_size_y );
        max = actual_size;
    } else {
        min = line - half_y;
        max = line - half_y + effect_win_size_y;
        if( actual_size < max ) {
            max = actual_size;
        }
    }

    for( size_t i = min; i < max; i++ ) {
        trim_and_print( w_effects, point( 0, static_cast<int>( 1 + i - min ) ), getmaxx( w_effects ) - 1,
                        i == line ? h_light_gray : c_light_gray, effect_name_and_text[i].first );
    }
    if( line < actual_size ) {
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        fold_and_print( w_info, point( 1, 0 ), FULL_SCREEN_WIDTH - 2, c_magenta,
                        effect_name_and_text[line].second );
    }
    wrefresh( w_effects );
    wrefresh( w_info );

    action = ctxt.handle_input();
    if( action == "DOWN" ) {
        if( line < actual_size - 1 ) {
            line++;
        }
        return;
    } else if( action == "UP" ) {
        if( line > 0 ) {
            line--;
        }
    } else if( action == "NEXT_TAB" || action == "PREV_TAB" ) {
        mvwprintz( w_effects, point_zero, c_light_gray, header_spaces );
        center_print( w_effects, 0, c_light_gray, _( title_EFFECTS ) );
        for( size_t i = 0; i < actual_size && i < 7; i++ ) {
            trim_and_print( w_effects, point( 0, static_cast<int>( i ) + 1 ), getmaxx( w_effects ) - 1,
                            c_light_gray,
                            effect_name_and_text[i].first );
        }
        wrefresh( w_effects );
        line = 0;
        curtab = action == "NEXT_TAB" ? 1 : curtab - 1;
    } else if( action == "QUIT" ) {
        done = true;
    }
}

struct HeaderSkill {
    const Skill *skill;
    bool is_header;
    HeaderSkill( const Skill *skill, bool is_header ): skill( skill ), is_header( is_header ) {
    }
};

static const Skill *draw_skills_list( const catacurses::window &w_skills,
                                      player &you, unsigned int &line,
                                      std::vector<HeaderSkill> &skillslist,
                                      const size_t skill_win_size_y )
{
    const int col_width = 25;
    if( line == 0 ) { //can't point to a header;
        line = 1;
    }

    nc_color cstatus = c_light_gray;
    if( line < 100 ) {
        mvwprintz( w_skills, point_zero, h_light_gray, header_spaces );
        cstatus = hilite( cstatus );
    }
    center_print( w_skills, 0, cstatus, _( title_SKILLS ) );

    size_t min = 0;
    size_t max = 0;

    const size_t half_y = skill_win_size_y / 2;

    if( line <= half_y || line > 100 ) {
        min = 0;
    } else if( line >= skillslist.size() - half_y ) {
        min = ( skillslist.size() < skill_win_size_y ? 0 : skillslist.size() -
                skill_win_size_y );
    } else {
        min = line - half_y;
    }
    max = std::min( min + skill_win_size_y, skillslist.size() );

    const Skill *selectedSkill = nullptr;
    int y_pos = 1;
    for( size_t i = min; i < max; i++, y_pos++ ) {
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
            const bool rusting = level.isRusting();
            int exercise = level.exercise();
            int level_num = level.level();
            bool locked = false;
            if( you.has_active_bionic( bionic_id( "bio_cqb" ) ) && is_cqb_skill( aSkill->ident() ) ) {
                level_num = 5;
                exercise = 0;
                locked = true;
            }
            if( i == line ) {
                selectedSkill = aSkill;
                if( locked ) {
                    cstatus = h_yellow;
                } else if( !can_train ) {
                    cstatus = rusting ? h_light_red : h_white;
                } else if( exercise >= 100 ) {
                    cstatus = training ? h_pink : h_magenta;
                } else if( rusting ) {
                    cstatus = training ? h_light_red : h_red;
                } else {
                    cstatus = training ? h_light_blue : h_blue;
                }
                mvwprintz( w_skills, point( 1, y_pos ), cstatus, "%*s", col_width, "" );
            } else {
                if( locked ) {
                    cstatus = c_yellow;
                } else if( rusting ) {
                    cstatus = training ? c_light_red : c_red;
                } else if( !can_train ) {
                    cstatus = c_white;
                } else {
                    cstatus = training ? c_light_blue : c_blue;
                }
                mvwprintz( w_skills, point( 1, y_pos ), c_light_gray, "%*s", col_width, "" );
            }
            mvwprintz( w_skills, point( 1, y_pos ), cstatus, "%s:", aSkill->name() );
            if( aSkill->ident() == skill_id( "dodge" ) ) {
                mvwprintz( w_skills, point( 14, y_pos ), cstatus, "%4.1f/%-2d(%2d%%)",
                           you.get_dodge(), level_num, exercise < 0 ? 0 : exercise );
            } else {
                mvwprintz( w_skills, point( 19, y_pos ), cstatus, "%-2d(%2d%%)",
                           level_num,
                           ( exercise < 0 ? 0 : exercise ) );
            }
        }
    }

    return selectedSkill;
}

static void draw_skills_tab( const catacurses::window &w_skills, const catacurses::window &w_info,
                             player &you, unsigned int &line, int &curtab, input_context &ctxt, bool &done,
                             std::string &action, std::vector<HeaderSkill> &skillslist,
                             const size_t skill_win_size_y )
{

    const Skill *selectedSkill = draw_skills_list( w_skills, you, line, skillslist, skill_win_size_y );
    int list_size = skillslist.size();
    if( skillslist.size() > skill_win_size_y ) {
        draw_scrollbar( w_skills, line, skill_win_size_y, list_size,
                        point_south );
    }
    wrefresh( w_skills );

    werase( w_info );

    if( selectedSkill ) {
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        fold_and_print( w_info, point( 1, 0 ), FULL_SCREEN_WIDTH - 2, c_magenta,
                        selectedSkill->description() );
    }
    wrefresh( w_info );

    action = ctxt.handle_input();
    if( action == "DOWN" ) {
        line++;
        if( static_cast<size_t>( line ) >= skillslist.size() ) {
            line %= skillslist.size();
        }
        if( skillslist[line].is_header ) { //ok not to check for wraparound because header can't be at the end of the list
            line++;
        }
    } else if( action == "UP" ) {
        //ok not to check for -1 because header is always first in the list
        line--;
        if( skillslist[line].is_header ) {
            if( line > 0 ) {
                line--;
            } else {
                line = skillslist.size() - 1;
            }
        }
    } else if( action == "NEXT_TAB" || action == "PREV_TAB" ) {
        werase( w_skills );
        line = 1000;
        draw_skills_list( w_skills, you, line, skillslist, skill_win_size_y );
        wrefresh( w_skills );
        line = 0;
        curtab = action == "NEXT_TAB" ? curtab + 1 : curtab - 1;
    } else if( action == "CONFIRM" ) {
        if( selectedSkill ) {
            you.get_skill_level_object( selectedSkill->ident() ).toggleTraining();
        }
    } else if( action == "QUIT" ) {
        done = true;
    }
}

static void draw_grid_borders( const catacurses::window &w_grid_top,
                               const catacurses::window &w_grid_skill, const catacurses::window &w_grid_trait,
                               const catacurses::window &w_grid_bionics, const catacurses::window &w_grid_effect,
                               const unsigned int &info_win_size_y, const unsigned int &infooffsetybottom,
                               const unsigned int &skill_win_size_y, const unsigned int &trait_win_size_y,
                               const unsigned int &bionics_win_size_y, const unsigned int &effect_win_size_y )
{
    unsigned upper_info_border = 10;
    unsigned lower_info_border = 1 + upper_info_border + info_win_size_y;
    for( unsigned i = 0; i < static_cast<unsigned>( FULL_SCREEN_WIDTH + 1 ); i++ ) {
        //Horizontal line top grid
        mvwputch( w_grid_top, point( i, upper_info_border ), BORDER_COLOR, LINE_OXOX );
        mvwputch( w_grid_top, point( i, lower_info_border ), BORDER_COLOR, LINE_OXOX );

        //Vertical line top grid
        if( i <= infooffsetybottom ) {
            mvwputch( w_grid_top, point( 26, i ), BORDER_COLOR, LINE_XOXO );
            mvwputch( w_grid_top, point( 53, i ), BORDER_COLOR, LINE_XOXO );
            mvwputch( w_grid_top, point( FULL_SCREEN_WIDTH, i ), BORDER_COLOR, LINE_XOXO );
        }

        //Horizontal line skills
        if( i <= 26 ) {
            mvwputch( w_grid_skill, point( i, skill_win_size_y ), BORDER_COLOR, LINE_OXOX );
        }

        //Vertical line skills
        if( i <= skill_win_size_y ) {
            mvwputch( w_grid_skill, point( 26, i ), BORDER_COLOR, LINE_XOXO );
        }

        //Horizontal line traits
        if( i <= 26 ) {
            mvwputch( w_grid_trait, point( i, trait_win_size_y ), BORDER_COLOR, LINE_OXOX );
        }

        //Vertical line traits
        if( i <= trait_win_size_y ) {
            mvwputch( w_grid_trait, point( 26, i ), BORDER_COLOR, LINE_XOXO );
        }

        //Horizontal line bionics
        if( i <= 26 ) {
            mvwputch( w_grid_bionics, point( i, bionics_win_size_y ), BORDER_COLOR, LINE_OXOX );
        }

        //Vertical line bionics
        if( i <= bionics_win_size_y ) {
            mvwputch( w_grid_bionics, point( 26, i ), BORDER_COLOR, LINE_XOXO );
        }

        //Horizontal line effects
        if( i <= 27 ) {
            mvwputch( w_grid_effect, point( i, effect_win_size_y ), BORDER_COLOR, LINE_OXOX );
        }

        //Vertical line effects
        if( i <= effect_win_size_y ) {
            mvwputch( w_grid_effect, point( 0, i ), BORDER_COLOR, LINE_XOXO );
            mvwputch( w_grid_effect, point( 27, i ), BORDER_COLOR, LINE_XOXO );
        }
    }

    //Intersections top grid
    mvwputch( w_grid_top, point( 26, lower_info_border ), BORDER_COLOR, LINE_OXXX ); // T
    mvwputch( w_grid_top, point( 53, lower_info_border ), BORDER_COLOR, LINE_OXXX ); // T
    mvwputch( w_grid_top, point( 26, upper_info_border ), BORDER_COLOR, LINE_XXOX ); // _|_
    mvwputch( w_grid_top, point( 53, upper_info_border ), BORDER_COLOR, LINE_XXOX ); // _|_
    mvwputch( w_grid_top, point( FULL_SCREEN_WIDTH, upper_info_border ), BORDER_COLOR,
              LINE_XOXX ); // -|
    mvwputch( w_grid_top, point( FULL_SCREEN_WIDTH, lower_info_border ), BORDER_COLOR,
              LINE_XOXX ); // -|
    wrefresh( w_grid_top );

    mvwputch( w_grid_skill, point( 26, skill_win_size_y ), BORDER_COLOR, LINE_XOOX ); // _|

    if( skill_win_size_y > trait_win_size_y ) {
        mvwputch( w_grid_skill, point( 26, trait_win_size_y ), BORDER_COLOR, LINE_XXXO );    // |-
    } else if( skill_win_size_y == trait_win_size_y ) {
        mvwputch( w_grid_skill, point( 26, trait_win_size_y ), BORDER_COLOR, LINE_XXOX );    // _|_
    }

    mvwputch( w_grid_trait, point( 26, trait_win_size_y ), BORDER_COLOR, LINE_XOXX ); // -|

    if( trait_win_size_y > effect_win_size_y ) {
        mvwputch( w_grid_trait, point( 26, effect_win_size_y ), BORDER_COLOR, LINE_XXXO ); // |-
    } else if( trait_win_size_y == effect_win_size_y ) {
        mvwputch( w_grid_trait, point( 26, effect_win_size_y ), BORDER_COLOR, LINE_XXOX ); // _|_
    } else if( trait_win_size_y < effect_win_size_y ) {
        mvwputch( w_grid_trait, point( 26, trait_win_size_y ), BORDER_COLOR, LINE_XOXX ); // -|
        mvwputch( w_grid_trait, point( 26, effect_win_size_y ), BORDER_COLOR, LINE_XXOO ); // |_
    }

    if( ( trait_win_size_y + bionics_win_size_y ) > effect_win_size_y ) {
        mvwputch( w_grid_bionics, point( 26, bionics_win_size_y ), BORDER_COLOR, LINE_XOOX ); // _|
    } else if( ( trait_win_size_y + bionics_win_size_y ) == effect_win_size_y ) {
        mvwputch( w_grid_bionics, point( 26, effect_win_size_y ), BORDER_COLOR, LINE_XXOX ); // _|_
    } else if( ( trait_win_size_y + bionics_win_size_y ) < effect_win_size_y ) {
        mvwputch( w_grid_bionics, point( 26, bionics_win_size_y ), BORDER_COLOR, LINE_XOXX ); // -|
        mvwputch( w_grid_bionics, point( 26, effect_win_size_y ), BORDER_COLOR, LINE_XXOO ); // |_
    }

    mvwputch( w_grid_effect, point( 0, effect_win_size_y ), BORDER_COLOR, LINE_XXOO ); // |_
    mvwputch( w_grid_effect, point( 27, effect_win_size_y ), BORDER_COLOR, LINE_XOOX ); // _|

    wrefresh( w_grid_skill );
    wrefresh( w_grid_effect );
    wrefresh( w_grid_trait );
    wrefresh( w_grid_bionics );

}

static void draw_initial_windows( const catacurses::window &w_stats,
                                  const catacurses::window &w_encumb, const catacurses::window &w_traits,
                                  const catacurses::window &w_bionics, const catacurses::window &w_effects,
                                  const catacurses::window &w_skills, const catacurses::window &w_speed, player &you,
                                  unsigned int &line, const std::vector<trait_id> &traitslist, std::vector<bionic> &bionicslist,
                                  std::vector<std::pair<std::string, std::string>> &effect_name_and_text,
                                  std::vector<HeaderSkill> &skillslist,
                                  const size_t bionics_win_size_y, const size_t effect_win_size_y, const size_t trait_win_size_y,
                                  const size_t skill_win_size_y )
{
    // First!  Default STATS screen.
    center_print( w_stats, 0, c_light_gray, _( title_STATS ) );

    // Stats
    const auto display_stat = [&w_stats]( const char *name, int cur, int max, int line_n ) {
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

        mvwprintz( w_stats, point( 1, line_n ), c_light_gray, name );
        mvwprintz( w_stats, point( 18, line_n ), cstatus, "%2d", cur );
        mvwprintz( w_stats, point( 21, line_n ), c_light_gray, "(%2d)", max );
    };

    display_stat( _( "Strength:" ), you.get_str(), you.get_str_base(), 2 );
    display_stat( _( "Dexterity:" ), you.get_dex(), you.get_dex_base(), 3 );
    display_stat( _( "Intelligence:" ), you.get_int(), you.get_int_base(), 4 );
    display_stat( _( "Perception:" ), you.get_per(), you.get_per_base(), 5 );
    mvwprintz( w_stats, point( 1, 6 ), c_light_gray, _( "Weight:" ) );
    mvwprintz( w_stats, point( 25 - you.get_weight_string().size(), 6 ), c_light_gray,
               you.get_weight_string() );

    wrefresh( w_stats );

    // Next, draw encumbrance.
    center_print( w_encumb, 0, c_light_gray, _( title_ENCUMB ) );
    you.print_encumbrance( w_encumb );
    wrefresh( w_encumb );

    // Next, draw traits.
    center_print( w_traits, 0, c_light_gray, _( title_TRAITS ) );
    std::sort( traitslist.begin(), traitslist.end(), trait_display_sort );
    for( size_t i = 0; i < traitslist.size() && i < trait_win_size_y; i++ ) {
        const auto &mdata = traitslist[i].obj();
        const auto color = mdata.get_display_color();
        trim_and_print( w_traits, point( 1, static_cast<int>( i ) + 1 ), getmaxx( w_traits ) - 1, color,
                        mdata.name() );
    }
    wrefresh( w_traits );

    // Next, draw bionics
    center_print( w_bionics, 0, c_light_gray, _( title_BIONICS ) );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    trim_and_print( w_bionics, point( 1, 1 ), getmaxx( w_bionics ) - 1, c_white,
                    string_format( _( "Bionic Power: <color_light_blue>%1$d / %2$d</color>" ),
                                   units::to_kilojoule( you.get_power_level() ), units::to_kilojoule( you.get_max_power_level() ) ) );
    for( size_t i = 0; i < bionicslist.size() && i < bionics_win_size_y - 1; i++ ) {
        trim_and_print( w_bionics, point( 1, static_cast<int>( i ) + 2 ), getmaxx( w_bionics ) - 1, c_white,
                        "%s", bionicslist[i].info().name );
    }
    wrefresh( w_bionics );

    // Next, draw effects.
    center_print( w_effects, 0, c_light_gray, _( title_EFFECTS ) );
    for( size_t i = 0; i < effect_name_and_text.size() && i < effect_win_size_y; i++ ) {
        trim_and_print( w_effects, point( 0, static_cast<int>( i ) + 1 ), getmaxx( w_effects ) - 1,
                        c_light_gray,
                        effect_name_and_text[i].first );
    }
    wrefresh( w_effects );

    // Next, draw skills.
    line = 1000;
    draw_skills_list( w_skills, you, line, skillslist, skill_win_size_y );
    wrefresh( w_skills );

    // Finally, draw speed.
    center_print( w_speed, 0, c_light_gray, _( title_SPEED ) );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w_speed, point( 1, 1 ), c_light_gray, _( "Base Move Cost:" ) );
    mvwprintz( w_speed, point( 1, 2 ), c_light_gray, _( "Current Speed:" ) );
    int newmoves = you.get_speed();
    int pen = 0;
    line = 3;
    if( you.weight_carried() > you.weight_capacity() ) {
        pen = 25 * ( you.weight_carried() - you.weight_capacity() ) / ( you.weight_capacity() );
        mvwprintz( w_speed, point( 1, line ), c_red, _( "Overburdened        -%s%d%%" ),
                   ( pen < 10 ? " " : "" ), pen );
        line++;
    }
    pen = you.get_pain_penalty().speed;
    if( pen >= 1 ) {
        mvwprintz( w_speed, point( 1, line ), c_red, _( "Pain                -%s%d%%" ),
                   ( pen < 10 ? " " : "" ), pen );
        line++;
    }
    if( you.get_thirst() > 40 ) {
        pen = abs( player::thirst_speed_penalty( you.get_thirst() ) );
        mvwprintz( w_speed, point( 1, line ), c_red, _( "Thirst              -%s%d%%" ),
                   ( pen < 10 ? " " : "" ), pen );
        line++;
    }
    if( you.kcal_speed_penalty() < 0 ) {
        pen = abs( you.kcal_speed_penalty() );
        const std::string inanition = you.get_bmi() < character_weight_category::underweight ?
                                      _( "Starving" ) : _( "Underfed" );
        mvwprintz( w_speed, point( 1, line ), c_red, _( "%-20s-%s%d%%" ), inanition,
                   ( pen < 10 ? " " : "" ), pen );
        line++;
    }
    if( you.mutations.has_trait( trait_id( "SUNLIGHT_DEPENDENT" ) ) && !g->is_in_sunlight( you.pos() ) ) {
        pen = ( g->light_level( you.posz() ) >= 12 ? 5 : 10 );
        mvwprintz( w_speed, point( 1, line ), c_red, _( "Out of Sunlight     -%s%d%%" ),
                   ( pen < 10 ? " " : "" ), pen );
        line++;
    }

    const float temperature_speed_modifier = you.mutations.mutation_value( "temperature_speed_modifier" );
    if( temperature_speed_modifier != 0 ) {
        nc_color pen_color;
        std::string pen_sign;
        const auto player_local_temp = g->weather.get_temperature( you.pos() );
        if( you.mutations.has_trait( trait_id( "COLDBLOOD4" ) ) && player_local_temp > 65 ) {
            pen_color = c_green;
            pen_sign = "+";
        } else if( player_local_temp < 65 ) {
            pen_color = c_red;
            pen_sign = "-";
        }
        if( !pen_sign.empty() ) {
            pen = ( player_local_temp - 65 ) * temperature_speed_modifier;
            mvwprintz( w_speed, point( 1, line ), pen_color, _( "Cold-Blooded        %s%s%d%%" ), pen_sign,
                       ( pen < 10 ? " " : "" ), pen );
            line++;
        }
    }

    int quick_bonus = static_cast<int>( newmoves - ( newmoves / 1.1 ) );
    int bio_speed_bonus = quick_bonus;
    if( you.mutations.has_trait( trait_id( "QUICK" ) ) && you.has_bionic( bionic_id( "bio_speed" ) ) ) {
        bio_speed_bonus = static_cast<int>( newmoves / 1.1 - ( newmoves / 1.1 / 1.1 ) );
        std::swap( quick_bonus, bio_speed_bonus );
    }
    if( you.mutations.has_trait( trait_id( "QUICK" ) ) ) {
        mvwprintz( w_speed, point( 1, line ), c_green, _( "Quick               +%s%d%%" ),
                   ( quick_bonus < 10 ? " " : "" ), quick_bonus );
        line++;
    }
    if( you.has_bionic( bionic_id( "bio_speed" ) ) ) {
        mvwprintz( w_speed, point( 1, line ), c_green, _( "Bionic Speed        +%s%d%%" ),
                   ( bio_speed_bonus < 10 ? " " : "" ), bio_speed_bonus );
    }

    int runcost = you.run_cost( 100 );
    nc_color col = ( runcost <= 100 ? c_green : c_red );
    mvwprintz( w_speed, point( 21 + ( runcost >= 100 ? 0 : ( runcost < 10 ? 2 : 1 ) ), 1 ), col,
               "%d", runcost );
    col = ( newmoves >= 100 ? c_green : c_red );
    mvwprintz( w_speed, point( 21 + ( newmoves >= 100 ? 0 : ( newmoves < 10 ? 2 : 1 ) ), 2 ), col,
               "%d", newmoves );
    wrefresh( w_speed );
}

void player::disp_info()
{
    unsigned int line = 0;
    std::vector<std::pair<std::string, std::string>> effect_name_and_text;
    for( auto &elem : *effects ) {
        for( auto &_effect_it : elem.second ) {
            const std::string tmp = _effect_it.second.disp_name();
            if( tmp.empty() ) {
                continue;
            }
            effect_name_and_text.push_back( { tmp, _effect_it.second.disp_desc() } );
        }
    }
    if( get_perceived_pain() > 0 ) {
        const auto ppen = get_pain_penalty();
        std::string pain_text;
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
        effect_name_and_text.push_back( { _( "Pain" ), pain_text } );
    }

    const float bmi = get_bmi();

    if( bmi < character_weight_category::underweight ) {
        std::string starvation_name;
        std::stringstream starvation_text;

        if( bmi < character_weight_category::emaciated ) {
            starvation_name = _( "Severely Malnourished" );
            starvation_text <<
                            _( "Your body is severely weakened by starvation.  You might die if you don't start eating regular meals!\n\n" );
        } else {
            starvation_name = _( "Malnourished" );
            starvation_text <<
                            _( "Your body is weakened by starvation.  Only time and regular meals will help you recover.\n\n" );
        }

        if( bmi < character_weight_category::underweight ) {
            const float str_penalty = 1.0f - ( ( bmi - 13.0f ) / 3.0f );
            starvation_text << _( "Strength" ) << " -" << string_format( "%2.0f%%\n", str_penalty * 100.0f );
            starvation_text << _( "Dexterity" ) << " -" << string_format( "%2.0f%%\n", str_penalty * 50.0f );
            starvation_text << _( "Intelligence" ) << " -" << string_format( "%2.0f%%", str_penalty * 50.0f );
        }

        effect_name_and_text.push_back( { starvation_name, starvation_text.str() } );
    }

    if( ( mutations.has_trait( trait_id( "TROGLO" ) ) && g->is_in_sunlight( pos() ) &&
          g->weather.weather == WEATHER_SUNNY ) ||
        ( mutations.has_trait( trait_id( "TROGLO2" ) ) && g->is_in_sunlight( pos() ) &&
          g->weather.weather != WEATHER_SUNNY ) ) {
        effect_name_and_text.push_back( { _( "In Sunlight" ),
                                          _( "The sunlight irritates you.\n"
                                             "Strength - 1;    Dexterity - 1;    Intelligence - 1;    Perception - 1" )
                                        } );
    } else if( mutations.has_trait( trait_id( "TROGLO2" ) ) && g->is_in_sunlight( pos() ) ) {
        effect_name_and_text.push_back( { _( "In Sunlight" ),
                                          _( "The sunlight irritates you badly.\n"
                                             "Strength - 2;    Dexterity - 2;    Intelligence - 2;    Perception - 2" )
                                        } );
    } else if( mutations.has_trait( trait_id( "TROGLO3" ) ) && g->is_in_sunlight( pos() ) ) {
        effect_name_and_text.push_back( { _( "In Sunlight" ),
                                          _( "The sunlight irritates you terribly.\n"
                                             "Strength - 4;    Dexterity - 4;    Intelligence - 4;    Perception - 4" )
                                        } );
    }

    for( auto &elem : addictions ) {
        if( elem.sated < 0_turns && elem.intensity >= MIN_ADDICTION_LEVEL ) {
            effect_name_and_text.push_back( { addiction_name( elem ), addiction_text( elem ) } );
        }
    }

    unsigned int maxy = static_cast<unsigned>( TERMY );

    unsigned int effect_win_size_y = 1 + static_cast<unsigned>( effect_name_and_text.size() );

    const std::vector<trait_id> traitslist = mutations.get_mutations( false );
    unsigned int trait_win_size_y = 1 + static_cast<unsigned>( traitslist.size() );

    std::vector<bionic> bionicslist = *my_bionics;
    unsigned int bionics_win_size_y = 2 + bionicslist.size();

    const std::vector<const Skill *> player_skill = Skill::get_skills_sorted_by( [&]( const Skill & a,
    const Skill & b ) {
        skill_displayType_id type_a = a.display_category();
        skill_displayType_id type_b = b.display_category();

        if( type_a != type_b ) {
            return type_a < type_b;
        } else {
            return a.name() < b.name();
        }
    } );

    std::vector<HeaderSkill> skillslist;
    skill_displayType_id prev_type = skill_displayType_id::NULL_ID();
    for( auto &s : player_skill ) {
        if( s->display_category() != prev_type ) {
            prev_type = s->display_category();
            skillslist.emplace_back( s, true );
        }
        skillslist.emplace_back( s, false );
    }
    unsigned int skill_win_size_y = 1 + skillslist.size();
    unsigned int info_win_size_y = 6;

    unsigned int infooffsetytop = 11;
    unsigned int infooffsetybottom = infooffsetytop + 1 + info_win_size_y;

    if( ( bionics_win_size_y + trait_win_size_y + infooffsetybottom ) > maxy ) {
        // maximum space for either window if they're both the same size
        unsigned max_shared_y = ( maxy - infooffsetybottom ) / 2;
        // both are larger than the shared size
        if( std::min( bionics_win_size_y, trait_win_size_y ) > max_shared_y ) {
            bionics_win_size_y = max_shared_y;
            // trait window is less than the shared size, so give space to bionics
        } else if( trait_win_size_y <= max_shared_y ) {
            bionics_win_size_y = maxy - infooffsetybottom - trait_win_size_y;
        }
        // fall through if bionics is smaller
        trait_win_size_y = maxy - infooffsetybottom - bionics_win_size_y;

        bionics_win_size_y--;
    }

    if( skill_win_size_y + infooffsetybottom > maxy ) {
        skill_win_size_y = maxy - infooffsetybottom;
    }

    catacurses::window w_grid_top =
        catacurses::newwin( infooffsetybottom, FULL_SCREEN_WIDTH + 1,
                            point( VIEW_OFFSET_X, VIEW_OFFSET_Y ) );
    catacurses::window w_grid_skill =
        catacurses::newwin( skill_win_size_y + 1, 27,
                            point( 0 + VIEW_OFFSET_X, infooffsetybottom + VIEW_OFFSET_Y ) );
    catacurses::window w_grid_trait =
        catacurses::newwin( trait_win_size_y + 1, 27,
                            point( 27 + VIEW_OFFSET_X, infooffsetybottom + VIEW_OFFSET_Y ) );
    catacurses::window w_grid_bionics =
        catacurses::newwin( bionics_win_size_y + 1, 27,
                            point( 27 + VIEW_OFFSET_X,
                                   infooffsetybottom + VIEW_OFFSET_Y + trait_win_size_y + 1 ) );
    catacurses::window w_grid_effect =
        catacurses::newwin( effect_win_size_y + 1, 28,
                            point( 53 + VIEW_OFFSET_X, infooffsetybottom + VIEW_OFFSET_Y ) );

    catacurses::window w_tip =
        catacurses::newwin( 1, FULL_SCREEN_WIDTH,  point( 0 + VIEW_OFFSET_X, VIEW_OFFSET_Y ) );
    catacurses::window w_stats =
        catacurses::newwin( 9, 26,  point( 0 + VIEW_OFFSET_X, 1 + VIEW_OFFSET_Y ) );
    catacurses::window w_traits =
        catacurses::newwin( trait_win_size_y, 26,
                            point( 27 + VIEW_OFFSET_X, infooffsetybottom + VIEW_OFFSET_Y ) );
    catacurses::window w_bionics =
        catacurses::newwin( bionics_win_size_y, 26,
                            point( 27 + VIEW_OFFSET_X,
                                   infooffsetybottom + VIEW_OFFSET_Y + trait_win_size_y + 1 ) );
    catacurses::window w_encumb =
        catacurses::newwin( 9, 26, point( 27 + VIEW_OFFSET_X, 1 + VIEW_OFFSET_Y ) );
    catacurses::window w_effects =
        catacurses::newwin( effect_win_size_y, 26,
                            point( 54 + VIEW_OFFSET_X, infooffsetybottom + VIEW_OFFSET_Y ) );
    catacurses::window w_speed =
        catacurses::newwin( 9, 26,  point( 54 + VIEW_OFFSET_X, 1 + VIEW_OFFSET_Y ) );
    catacurses::window w_skills =
        catacurses::newwin( skill_win_size_y, 26,
                            point( 0 + VIEW_OFFSET_X, infooffsetybottom + VIEW_OFFSET_Y ) );
    catacurses::window w_info =
        catacurses::newwin( info_win_size_y, FULL_SCREEN_WIDTH,
                            point( 0 + VIEW_OFFSET_X, infooffsetytop + VIEW_OFFSET_Y ) );

    draw_grid_borders( w_grid_top, w_grid_skill, w_grid_trait, w_grid_bionics, w_grid_effect,
                       info_win_size_y, infooffsetybottom, skill_win_size_y, trait_win_size_y,
                       bionics_win_size_y, effect_win_size_y );
    //-1 for header
    trait_win_size_y--;
    bionics_win_size_y--;
    skill_win_size_y--;
    effect_win_size_y--;

    // Print name and header
    // Post-humanity trumps your pre-Cataclysm life.
    if( crossed_threshold() ) {
        std::string race;
        for( const trait_id &mut : mutations.get_mutations() ) {
            if( mut->threshold ) {
                race = mut->name();
                break;
            }
        }
        //~ player info window: 1s - name, 2s - gender, 3s - Prof or Mutation name
        mvwprintw( w_tip, point_zero, _( "%1$s | %2$s | %3$s" ), name,
                   male ? _( "Male" ) : _( "Female" ), race );
    } else if( prof == nullptr || prof == profession::generic() ) {
        // Regular person. Nothing interesting.
        //~ player info window: 1s - name, 2s - gender, '|' - field separator.
        mvwprintw( w_tip, point_zero, _( "%1$s | %2$s" ), name, male ? _( "Male" ) : _( "Female" ) );
    } else {
        mvwprintw( w_tip, point_zero, _( "%1$s | %2$s | %3$s" ), name,
                   male ? _( "Male" ) : _( "Female" ), prof->gender_appropriate_name( male ) );
    }

    input_context ctxt( "PLAYER_INFO" );
    ctxt.register_updown();
    ctxt.register_action( "NEXT_TAB", translate_marker( "Cycle to next category" ) );
    ctxt.register_action( "PREV_TAB", translate_marker( "Cycle to previous category" ) );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "CONFIRM", translate_marker( "Toggle skill training" ) );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    std::string action;

    std::string help_msg = string_format( _( "Press %s for help." ),
                                          ctxt.get_desc( "HELP_KEYBINDINGS" ) );
    mvwprintz( w_tip, point( FULL_SCREEN_WIDTH - utf8_width( help_msg ), 0 ), c_light_red, help_msg );
    help_msg.clear();
    wrefresh( w_tip );

    draw_initial_windows( w_stats, w_encumb, w_traits, w_bionics, w_effects, w_skills, w_speed, *this,
                          line, traitslist, bionicslist, effect_name_and_text, skillslist, bionics_win_size_y,
                          effect_win_size_y, trait_win_size_y, skill_win_size_y );

    std::map<std::string, int> speed_effects;
    for( auto &elem : *effects ) {
        for( std::pair<const body_part, effect> &_effect_it : elem.second ) {
            effect &it = _effect_it.second;
            bool reduced = resists_effect( it );
            int move_adjust = it.get_mod( "SPEED", reduced );
            if( move_adjust != 0 ) {
                const std::string dis_text = it.get_speed_name();
                speed_effects[dis_text] += move_adjust;
            }
        }
    }

    for( std::pair<const std::string, int> &speed_effect : speed_effects ) {
        nc_color col = ( speed_effect.second > 0 ? c_green : c_red );
        mvwprintz( w_speed, point( 1, line ), col, "%s", speed_effect.first );
        mvwprintz( w_speed, point( 21, line ), col, ( speed_effect.second > 0 ? "+" : "-" ) );
        mvwprintz( w_speed, point( abs( speed_effect.second ) >= 10 ? 22 : 23, line ), col, "%d%%",
                   abs( speed_effect.second ) );
        line++;
    }

    catacurses::refresh();

    int curtab = 1;
    line = 0;
    bool done = false;

    // Initial printing is DONE.  Now we give the player a chance to scroll around
    // and "hover" over different items for more info.
    do {
        werase( w_info );
        switch( curtab ) {
            case 1: // Stats tab
                draw_stats_tab( w_stats, w_info, *this, line, curtab, ctxt, done, action );
                break;
            case 2: // Encumbrance tab
                draw_encumbrance_tab( w_encumb, w_info, *this, line, curtab, ctxt, done, action );
                break;

            case 4: // Traits tab
                draw_traits_tab( w_traits, w_info, line, curtab, ctxt, done, action,
                                 traitslist, trait_win_size_y );
                break;

            case 5: // Bionics tab
                draw_bionics_tab( w_bionics, w_info, *this, line, curtab, ctxt, done, action,
                                  bionicslist, bionics_win_size_y );
                break;

            case 6: // Effects tab
                draw_effects_tab( w_effects, w_info, line, curtab, ctxt, done, action,
                                  effect_name_and_text, effect_win_size_y );
                break;

            case 3: // Skills tab
                draw_skills_tab( w_skills, w_info, *this, line, curtab, ctxt, done, action,
                                 skillslist, skill_win_size_y );

        }
    } while( !done );

    g->refresh_all();
}
