#include "player.h"
#include "game.h"
#include "mutation.h"
#include "options.h"
#include "weather.h"
#include "profession.h"
#include "input.h"
#include "addiction.h"
#include <algorithm>

const skill_id skill_swimming( "swimming" );

// use this instead of having to type out 26 spaces like before
static const std::string header_spaces( 26, ' ' );

// Rescale temperature value to one that the player sees
int temperature_print_rescaling( int temp )
{
    return ( temp / 100.0 ) * 2 - 100;
}

bool should_combine_bps( const player &p, size_t l, size_t r )
{
    const auto enc_data = p.get_encumbrance();
    return enc_data[l] == enc_data[r] &&
           temperature_print_rescaling( p.temp_conv[l] ) == temperature_print_rescaling( p.temp_conv[r] );
}

void player::print_encumbrance( WINDOW *win, int line, item *selected_clothing ) const
{
    int height, width;
    getmaxyx( win, height, width );
    int orig_line = line;

    // fill a set with the indices of the body parts to display
    line = std::max( 0, line );
    std::set<int> parts;
    // check and optionally enqueue line+0, -1, +1, -2, +2, ...
    int off = 0; // offset from line
    int skip[2] = {}; // how far to skip on next neg/pos jump
    do {
        if( !skip[off > 0] && line + off >= 0 && line + off < num_bp ) { // line+off is in bounds
            parts.insert( line + off );
            if( line + off != ( int )bp_aiOther[line + off] &&
                should_combine_bps( *this, line + off, bp_aiOther[line + off] ) ) { // part of a pair
                skip[( int )bp_aiOther[line + off] > line + off ] = 1; // skip the next candidate in this direction
            }
        } else {
            skip[off > 0] = 0;
        }
        if( off < 0 ) {
            off = -off;
        } else {
            off = -off - 1;
        }
    } while( off > -num_bp && ( int )parts.size() < height - 1 );

    std::string out;
    /*** I chose to instead only display X+Y instead of X+Y=Z. More room was needed ***
     *** for displaying triple digit encumbrance, due to new encumbrance system.    ***
     *** If the player wants to see the total without having to do them maths, the  ***
     *** armor layers ui shows everything they want :-) -Davek                      ***/
    int row = 1;
    const auto enc_data = get_encumbrance();
    for( auto bp : parts ) {
        const encumbrance_data &e = enc_data[bp];
        bool highlighted = ( selected_clothing == nullptr ) ? false :
                           ( selected_clothing->covers( static_cast<body_part>( bp ) ) );
        bool combine = should_combine_bps( *this, bp, bp_aiOther[bp] );
        out.clear();
        // limb, and possible color highlighting
        out = string_format( "%-7s", body_part_name_as_heading( bp_aBodyPart[bp],
                             combine ? 2 : 1 ).c_str() );
        // Two different highlighting schemes, highlight if the line is selected as per line being set.
        // Make the text green if this part is covered by the passed in item.
        int limb_color = ( orig_line == bp ) ?
                         ( highlighted ? h_green : h_ltgray ) :
                         ( highlighted ? c_green : c_ltgray );
        mvwprintz( win, row, 1, limb_color, out.c_str() );
        // take into account the new encumbrance system for layers
        out = string_format( "(%1d) ", static_cast<int>( e.layer_penalty / 10.0 ) );
        wprintz( win, c_ltgray, out.c_str() );
        // accumulated encumbrance from clothing, plus extra encumbrance from layering
        wprintz( win, encumb_color( e.encumbrance ), string_format( "%3d", e.armor_encumbrance ).c_str() );
        // seperator in low toned color
        wprintz( win, c_ltgray, "+" );
        wprintz( win, encumb_color( e.encumbrance ), string_format( "%-3d",
                 e.encumbrance - e.armor_encumbrance ).c_str() );
        // print warmth, tethered to right hand side of the window
        out = string_format( "(% 3d)", temperature_print_rescaling( temp_conv[bp] ) );
        mvwprintz( win, row, getmaxx( win ) - 6, bodytemp_color( bp ), out.c_str() );
        row++;
    }

    if( off > -num_bp ) { // not every body part fit in the window
        //TODO: account for skipped paired body parts in scrollbar math
        draw_scrollbar( win, std::max( orig_line, 0 ), height - 1, num_bp, 1 );
    }

}


std::string swim_cost_text( int moves )
{
    return string_format( ngettext( "Swimming costs %+d movement point. ",
                                    "Swimming costs %+d movement points. ",
                                    moves ),
                          moves );
}

std::string run_cost_text( int moves )
{
    return string_format( ngettext( "Running costs %+d movement point. ",
                                    "Running costs %+d movement points. ",
                                    moves ),
                          moves );
}

std::string reload_cost_text( int moves )
{
    return string_format( ngettext( "Reloading costs %+d movement point. ",
                                    "Reloading costs %+d movement points. ",
                                    moves ),
                          moves );
}

std::string melee_cost_text( int moves )
{
    return string_format( ngettext( "Melee and thrown attacks cost %+d movement point. ",
                                    "Melee and thrown attacks cost %+d movement points. ",
                                    moves ),
                          moves );
}

std::string dodge_skill_text( double mod )
{
    return string_format( _( "Dodge skill %+.1f. " ), mod );
}

std::string get_encumbrance_description( const player &p, body_part bp, bool combine )
{
    std::string s;
    // Body parts that can't combine with anything shouldn't print double values on combine
    // This shouldn't happen, but handle this, just in case
    bool combines_with_other = ( int )bp_aiOther[bp] != bp;
    int eff_encumbrance = p.encumb( bp ) * ( ( combine && combines_with_other ) ? 2 : 1 );
    switch( bp ) {
        case bp_torso: {
            const int melee_roll_pen = std::max( -eff_encumbrance, -80 );
            s += string_format( _( "Melee attack rolls %+d%%; " ), melee_roll_pen );
            s += dodge_skill_text( - ( eff_encumbrance / 10 ) );
            s += swim_cost_text( ( eff_encumbrance / 10 ) * ( 80 - p.get_skill_level( skill_swimming ) * 3 ) );
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
            break;
        case bp_leg_l:
        case bp_leg_r:
            s += run_cost_text( int( eff_encumbrance * 0.15 ) );
            s += swim_cost_text( ( eff_encumbrance / 10 ) * ( 50 - p.get_skill_level(
                                     skill_swimming ) * 2 ) / 2 );
            s += dodge_skill_text( -eff_encumbrance / 10.0 / 4.0 );
            break;
        case bp_foot_l:
        case bp_foot_r:
            s += run_cost_text( int( eff_encumbrance * 0.25 ) );
            break;
        case num_bp:
            break;
    }

    return s;
}

void player::disp_info()
{
    unsigned line;
    std::vector<std::string> effect_name;
    std::vector<std::string> effect_text;
    std::string tmp = "";
    for( auto &elem : effects ) {
        for( auto &_effect_it : elem.second ) {
            tmp = _effect_it.second.disp_name();
            if( tmp != "" ) {
                effect_name.push_back( tmp );
                effect_text.push_back( _effect_it.second.disp_desc() );
            }
        }
    }
    if( abs( get_morale_level() ) >= 100 ) {
        bool pos = ( get_morale_level() > 0 );
        effect_name.push_back( pos ? _( "Elated" ) : _( "Depressed" ) );
        std::stringstream morale_text;
        if( abs( get_morale_level() ) >= 200 ) {
            morale_text << _( "Dexterity" ) << ( pos ? " +" : " " ) <<
                        get_morale_level() / 200 << "   ";
        }
        if( abs( get_morale_level() ) >= 180 ) {
            morale_text << _( "Strength" ) << ( pos ? " +" : " " ) <<
                        get_morale_level() / 180 << "   ";
        }
        if( abs( get_morale_level() ) >= 125 ) {
            morale_text << _( "Perception" ) << ( pos ? " +" : " " ) <<
                        get_morale_level() / 125 << "   ";
        }
        morale_text << _( "Intelligence" ) << ( pos ? " +" : " " ) <<
                    get_morale_level() / 100 << "   ";
        effect_text.push_back( morale_text.str() );
    }
    if( get_perceived_pain() > 0 ) {
        effect_name.push_back( _( "Pain" ) );
        const auto ppen = get_pain_penalty();
        std::stringstream pain_text;
        if( ppen.strength > 0 ) {
            pain_text << _( "Strength" ) << " -" << ppen.strength << "   ";
        }
        if( ppen.dexterity > 0 ) {
            pain_text << _( "Dexterity" ) << " -" << ppen.dexterity << "   ";
        }
        if( ppen.intelligence > 0 ) {
            pain_text << _( "Intelligence" ) << " -" << ppen.intelligence << "   ";
        }
        if( ppen.perception > 0 ) {
            pain_text << _( "Perception" ) << " -" << ppen.perception << "   ";
        }
        if( ppen.speed > 0 ) {
            pain_text << _( "Speed" ) << " -" << ppen.speed << "%   ";
        }
        effect_text.push_back( pain_text.str() );
    }
    if( stim > 0 ) {
        int dexbonus = stim / 10;
        int perbonus = stim /  7;
        int intbonus = stim /  6;
        if( abs( stim ) >= 30 ) {
            dexbonus -= abs( stim - 15 ) /  8;
            perbonus -= abs( stim - 15 ) / 12;
            intbonus -= abs( stim - 15 ) / 14;
        }

        if( dexbonus < 0 ) {
            effect_name.push_back( _( "Stimulant Overdose" ) );
        } else {
            effect_name.push_back( _( "Stimulant" ) );
        }
        std::stringstream stim_text;
        stim_text << _( "Speed" ) << " +" << stim << "   " << _( "Intelligence" ) <<
                  ( intbonus > 0 ? " + " : " " ) << intbonus << "   " << _( "Perception" ) <<
                  ( perbonus > 0 ? " + " : " " ) << perbonus << "   " << _( "Dexterity" )  <<
                  ( dexbonus > 0 ? " + " : " " ) << dexbonus;
        effect_text.push_back( stim_text.str() );
    } else if( stim < 0 ) {
        effect_name.push_back( _( "Depressants" ) );
        std::stringstream stim_text;
        int dexpen = stim / 10;
        int perpen = stim /  7;
        int intpen = stim /  6;
        // Since dexpen etc. are always less than 0, no need for + signs
        stim_text << _( "Speed" ) << " " << stim / 4 << "   " << _( "Intelligence" ) << " " << intpen <<
                  "   " << _( "Perception" ) << " " << perpen << "   " << _( "Dexterity" ) << " " << dexpen;
        effect_text.push_back( stim_text.str() );
    }

    if( ( has_trait( "TROGLO" ) && g->is_in_sunlight( pos() ) &&
          g->weather == WEATHER_SUNNY ) ||
        ( has_trait( "TROGLO2" ) && g->is_in_sunlight( pos() ) &&
          g->weather != WEATHER_SUNNY ) ) {
        effect_name.push_back( _( "In Sunlight" ) );
        effect_text.push_back( _( "The sunlight irritates you.\n\
Strength - 1;    Dexterity - 1;    Intelligence - 1;    Perception - 1" ) );
    } else if( has_trait( "TROGLO2" ) && g->is_in_sunlight( pos() ) ) {
        effect_name.push_back( _( "In Sunlight" ) );
        effect_text.push_back( _( "The sunlight irritates you badly.\n\
Strength - 2;    Dexterity - 2;    Intelligence - 2;    Perception - 2" ) );
    } else if( has_trait( "TROGLO3" ) && g->is_in_sunlight( pos() ) ) {
        effect_name.push_back( _( "In Sunlight" ) );
        effect_text.push_back( _( "The sunlight irritates you terribly.\n\
Strength - 4;    Dexterity - 4;    Intelligence - 4;    Perception - 4" ) );
    }

    for( auto &elem : addictions ) {
        if( elem.sated < 0 && elem.intensity >= MIN_ADDICTION_LEVEL ) {
            effect_name.push_back( addiction_name( elem ) );
            effect_text.push_back( addiction_text( elem ) );
        }
    }

    unsigned maxy = unsigned( TERMY );

    unsigned infooffsetytop = 11;
    unsigned infooffsetybottom = 15;
    std::vector<std::string> traitslist = get_mutations();

    unsigned effect_win_size_y = 1 + unsigned( effect_name.size() );
    unsigned trait_win_size_y = 1 + unsigned( traitslist.size() );
    unsigned skill_win_size_y = 1 + unsigned( Skill::skill_count() );

    if( trait_win_size_y + infooffsetybottom > maxy ) {
        trait_win_size_y = maxy - infooffsetybottom;
    }

    if( skill_win_size_y + infooffsetybottom > maxy ) {
        skill_win_size_y = maxy - infooffsetybottom;
    }

    WINDOW *w_grid_top    = newwin( infooffsetybottom, FULL_SCREEN_WIDTH + 1, VIEW_OFFSET_Y,
                                    VIEW_OFFSET_X );
    WINDOW *w_grid_skill  = newwin( skill_win_size_y + 1, 27, infooffsetybottom + VIEW_OFFSET_Y,
                                    0 + VIEW_OFFSET_X );
    WINDOW *w_grid_trait  = newwin( trait_win_size_y + 1, 27, infooffsetybottom + VIEW_OFFSET_Y,
                                    27 + VIEW_OFFSET_X );
    WINDOW *w_grid_effect = newwin( effect_win_size_y + 1, 28, infooffsetybottom + VIEW_OFFSET_Y,
                                    53 + VIEW_OFFSET_X );

    WINDOW *w_tip     = newwin( 1, FULL_SCREEN_WIDTH,  VIEW_OFFSET_Y,  0 + VIEW_OFFSET_X );
    WINDOW *w_stats   = newwin( 9, 26,  1 + VIEW_OFFSET_Y,  0 + VIEW_OFFSET_X );
    WINDOW *w_traits  = newwin( trait_win_size_y, 26, infooffsetybottom + VIEW_OFFSET_Y,
                                27 + VIEW_OFFSET_X );
    WINDOW *w_encumb  = newwin( 9, 26,  1 + VIEW_OFFSET_Y, 27 + VIEW_OFFSET_X );
    WINDOW *w_effects = newwin( effect_win_size_y, 26, infooffsetybottom + VIEW_OFFSET_Y,
                                54 + VIEW_OFFSET_X );
    WINDOW *w_speed   = newwin( 9, 26,  1 + VIEW_OFFSET_Y, 54 + VIEW_OFFSET_X );
    WINDOW *w_skills  = newwin( skill_win_size_y, 26, infooffsetybottom + VIEW_OFFSET_Y,
                                0 + VIEW_OFFSET_X );
    WINDOW *w_info    = newwin( 3, FULL_SCREEN_WIDTH, infooffsetytop + VIEW_OFFSET_Y,
                                0 + VIEW_OFFSET_X );

    for( unsigned i = 0; i < unsigned( FULL_SCREEN_WIDTH + 1 ); i++ ) {
        //Horizontal line top grid
        mvwputch( w_grid_top, 10, i, BORDER_COLOR, LINE_OXOX );
        mvwputch( w_grid_top, 14, i, BORDER_COLOR, LINE_OXOX );

        //Vertical line top grid
        if( i <= infooffsetybottom ) {
            mvwputch( w_grid_top, i, 26, BORDER_COLOR, LINE_XOXO );
            mvwputch( w_grid_top, i, 53, BORDER_COLOR, LINE_XOXO );
            mvwputch( w_grid_top, i, FULL_SCREEN_WIDTH, BORDER_COLOR, LINE_XOXO );
        }

        //Horizontal line skills
        if( i <= 26 ) {
            mvwputch( w_grid_skill, skill_win_size_y, i, BORDER_COLOR, LINE_OXOX );
        }

        //Vertical line skills
        if( i <= skill_win_size_y ) {
            mvwputch( w_grid_skill, i, 26, BORDER_COLOR, LINE_XOXO );
        }

        //Horizontal line traits
        if( i <= 26 ) {
            mvwputch( w_grid_trait, trait_win_size_y, i, BORDER_COLOR, LINE_OXOX );
        }

        //Vertical line traits
        if( i <= trait_win_size_y ) {
            mvwputch( w_grid_trait, i, 26, BORDER_COLOR, LINE_XOXO );
        }

        //Horizontal line effects
        if( i <= 27 ) {
            mvwputch( w_grid_effect, effect_win_size_y, i, BORDER_COLOR, LINE_OXOX );
        }

        //Vertical line effects
        if( i <= effect_win_size_y ) {
            mvwputch( w_grid_effect, i, 0, BORDER_COLOR, LINE_XOXO );
            mvwputch( w_grid_effect, i, 27, BORDER_COLOR, LINE_XOXO );
        }
    }

    //Intersections top grid
    mvwputch( w_grid_top, 14, 26, BORDER_COLOR, LINE_OXXX ); // T
    mvwputch( w_grid_top, 14, 53, BORDER_COLOR, LINE_OXXX ); // T
    mvwputch( w_grid_top, 10, 26, BORDER_COLOR, LINE_XXOX ); // _|_
    mvwputch( w_grid_top, 10, 53, BORDER_COLOR, LINE_XXOX ); // _|_
    mvwputch( w_grid_top, 10, FULL_SCREEN_WIDTH, BORDER_COLOR, LINE_XOXX ); // -|
    mvwputch( w_grid_top, 14, FULL_SCREEN_WIDTH, BORDER_COLOR, LINE_XOXX ); // -|
    wrefresh( w_grid_top );

    mvwputch( w_grid_skill, skill_win_size_y, 26, BORDER_COLOR, LINE_XOOX ); // _|

    if( skill_win_size_y > trait_win_size_y ) {
        mvwputch( w_grid_skill, trait_win_size_y, 26, BORDER_COLOR, LINE_XXXO );    // |-
    } else if( skill_win_size_y == trait_win_size_y ) {
        mvwputch( w_grid_skill, trait_win_size_y, 26, BORDER_COLOR, LINE_XXOX );    // _|_
    }

    mvwputch( w_grid_trait, trait_win_size_y, 26, BORDER_COLOR, LINE_XOOX ); // _|

    if( trait_win_size_y > effect_win_size_y ) {
        mvwputch( w_grid_trait, effect_win_size_y, 26, BORDER_COLOR, LINE_XXXO ); // |-
    } else if( trait_win_size_y == effect_win_size_y ) {
        mvwputch( w_grid_trait, effect_win_size_y, 26, BORDER_COLOR, LINE_XXOX ); // _|_
    } else if( trait_win_size_y < effect_win_size_y ) {
        mvwputch( w_grid_trait, trait_win_size_y, 26, BORDER_COLOR, LINE_XOXX ); // -|
        mvwputch( w_grid_trait, effect_win_size_y, 26, BORDER_COLOR, LINE_XXOO ); // |_
    }

    mvwputch( w_grid_effect, effect_win_size_y, 0, BORDER_COLOR, LINE_XXOO ); // |_
    mvwputch( w_grid_effect, effect_win_size_y, 27, BORDER_COLOR, LINE_XOOX ); // _|

    wrefresh( w_grid_skill );
    wrefresh( w_grid_effect );
    wrefresh( w_grid_trait );

    //-1 for header
    trait_win_size_y--;
    skill_win_size_y--;
    effect_win_size_y--;

    // Print name and header
    // Post-humanity trumps your pre-Cataclysm life.
    if( crossed_threshold() ) {
        std::string race;
        for( auto &mut : my_mutations ) {
            const auto &mdata = mutation_branch::get( mut.first );
            if( mdata.threshold ) {
                race = mdata.name;
                break;
            }
        }
        //~ player info window: 1s - name, 2s - gender, 3s - Prof or Mutation name
        mvwprintw( w_tip, 0, 0, _( "%1$s | %2$s | %3$s" ), name.c_str(),
                   male ? _( "Male" ) : _( "Female" ), race.c_str() );
    } else if( prof == NULL || prof == prof->generic() ) {
        // Regular person. Nothing interesting.
        //~ player info window: 1s - name, 2s - gender, '|' - field separator.
        mvwprintw( w_tip, 0, 0, _( "%1$s | %2$s" ), name.c_str(),
                   male ? _( "Male" ) : _( "Female" ) );
    } else {
        mvwprintw( w_tip, 0, 0, _( "%1$s | %2$s | %3$s" ), name.c_str(),
                   male ? _( "Male" ) : _( "Female" ), prof->gender_appropriate_name( male ).c_str() );
    }

    input_context ctxt( "PLAYER_INFO" );
    ctxt.register_updown();
    ctxt.register_action( "NEXT_TAB", _( "Cycle to next category" ) );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "CONFIRM", _( "Toggle skill training" ) );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    std::string action;

    std::string help_msg = string_format( _( "Press %s for help." ),
                                          ctxt.get_desc( "HELP_KEYBINDINGS" ).c_str() );
    mvwprintz( w_tip, 0, FULL_SCREEN_WIDTH - utf8_width( help_msg ), c_ltred, help_msg.c_str() );
    help_msg.clear();
    wrefresh( w_tip );

    // First!  Default STATS screen.
    const char *title_STATS = _( "STATS" );
    mvwprintz( w_stats, 0, 13 - utf8_width( title_STATS ) / 2, c_ltgray, title_STATS );

    // Stats
    const auto display_stat = [&w_stats]( const char *name, int cur, int max, int line_n ) {
        nc_color cstatus;
        if( cur <= 0 ) {
            cstatus = c_dkgray;
        } else if( cur < max / 2 ) {
            cstatus = c_red;
        } else if( cur < max ) {
            cstatus = c_ltred;
        } else if( cur == max ) {
            cstatus = c_white;
        } else if( cur < max * 1.5 ) {
            cstatus = c_ltgreen;
        } else {
            cstatus = c_green;
        }

        mvwprintz( w_stats, line_n, 1, c_ltgray, name );
        mvwprintz( w_stats, line_n, 18, cstatus, "%2d", cur );
        mvwprintz( w_stats, line_n, 21, c_ltgray, "(%2d)", max );
    };

    display_stat( _( "Strength:" ),     str_cur, str_max, 2 );
    display_stat( _( "Dexterity:" ),    dex_cur, dex_max, 3 );
    display_stat( _( "Intelligence:" ), int_cur, int_max, 4 );
    display_stat( _( "Perception:" ),   per_cur, per_max, 5 );

    wrefresh( w_stats );

    // Next, draw encumberment.
    const char *title_ENCUMB = _( "ENCUMBRANCE AND WARMTH" );
    mvwprintz( w_encumb, 0, 13 - utf8_width( title_ENCUMB ) / 2, c_ltgray, title_ENCUMB );
    print_encumbrance( w_encumb );
    wrefresh( w_encumb );

    // Next, draw traits.
    const char *title_TRAITS = _( "TRAITS" );
    mvwprintz( w_traits, 0, 13 - utf8_width( title_TRAITS ) / 2, c_ltgray, title_TRAITS );
    std::sort( traitslist.begin(), traitslist.end(), trait_display_sort );
    for( size_t i = 0; i < traitslist.size() && i < trait_win_size_y; i++ ) {
        const auto &mdata = mutation_branch::get( traitslist[i] );
        const auto color = mdata.get_display_color();
        mvwprintz( w_traits, int( i ) + 1, 1, color, mdata.name.c_str() );
    }
    wrefresh( w_traits );

    // Next, draw effects.
    const char *title_EFFECTS = _( "EFFECTS" );
    mvwprintz( w_effects, 0, 13 - utf8_width( title_EFFECTS ) / 2, c_ltgray, title_EFFECTS );
    for( size_t i = 0; i < effect_name.size() && i < effect_win_size_y; i++ ) {
        mvwprintz( w_effects, int( i ) + 1, 0, c_ltgray, "%s", effect_name[i].c_str() );
    }
    wrefresh( w_effects );

    // Next, draw skills.
    line = 1;

    const char *title_SKILLS = _( "SKILLS" );
    mvwprintz( w_skills, 0, 13 - utf8_width( title_SKILLS ) / 2, c_ltgray, title_SKILLS );

    auto skillslist = Skill::get_skills_sorted_by( [&]( Skill const & a, Skill const & b ) {
        int const level_a = get_skill_level( a.ident() ).exercised_level();
        int const level_b = get_skill_level( b.ident() ).exercised_level();
        return level_a > level_b || ( level_a == level_b && a.name() < b.name() );
    } );

    for( auto &elem : skillslist ) {
        SkillLevel level = get_skill_level( elem->ident() );

        // Default to not training and not rusting
        nc_color text_color = c_blue;
        bool not_capped = level.can_train();
        bool training = level.isTraining();
        bool rusting = level.isRusting();

        if( training && rusting ) {
            text_color = c_ltred;
        } else if( training && not_capped ) {
            text_color = c_ltblue;
        } else if( rusting ) {
            text_color = c_red;
        } else if( !not_capped ) {
            text_color = c_white;
        }

        int level_num = ( int )level;
        int exercise = level.exercise();

        // TODO: this skill list here is used in other places as well. Useless redundancy and
        // dependency. Maybe change it into a flag of the skill that indicates it's a skill used
        // by the bionic?
        static const std::array<skill_id, 5> cqb_skills = { {
                skill_id( "melee" ), skill_id( "unarmed" ), skill_id( "cutting" ),
                skill_id( "bashing" ), skill_id( "stabbing" ),
            }
        };
        if( has_active_bionic( "bio_cqb" ) &&
            std::find( cqb_skills.begin(), cqb_skills.end(), elem->ident() ) != cqb_skills.end() ) {
            level_num = 5;
            exercise = 0;
            text_color = c_yellow;
        }

        if( line < skill_win_size_y + 1 ) {
            mvwprintz( w_skills, line, 1, text_color, "%s:", ( elem )->name().c_str() );
            mvwprintz( w_skills, line, 19, text_color, "%-2d(%2d%%)", level_num,
                       ( exercise <  0 ? 0 : exercise ) );
            line++;
        }
    }
    wrefresh( w_skills );

    // Finally, draw speed.
    const char *title_SPEED = _( "SPEED" );
    mvwprintz( w_speed, 0, 13 - utf8_width( title_SPEED ) / 2, c_ltgray, title_SPEED );
    mvwprintz( w_speed, 1,  1, c_ltgray, _( "Base Move Cost:" ) );
    mvwprintz( w_speed, 2,  1, c_ltgray, _( "Current Speed:" ) );
    int newmoves = get_speed();
    int pen = 0;
    line = 3;
    if( weight_carried() > weight_capacity() ) {
        pen = 25 * ( weight_carried() - weight_capacity() ) / ( weight_capacity() );
        mvwprintz( w_speed, line, 1, c_red, _( "Overburdened        -%s%d%%" ),
                   ( pen < 10 ? " " : "" ), pen );
        line++;
    }
    pen = get_morale_level() / 25;
    if( abs( pen ) >= 4 ) {
        if( pen > 10 ) {
            pen = 10;
        } else if( pen < -10 ) {
            pen = -10;
        }
        if( pen > 0 )
            mvwprintz( w_speed, line, 1, c_green, _( "Good mood           +%s%d%%" ),
                       ( pen < 10 ? " " : "" ), pen );
        else
            mvwprintz( w_speed, line, 1, c_red, _( "Depressed           -%s%d%%" ),
                       ( abs( pen ) < 10 ? " " : "" ), abs( pen ) );
        line++;
    }
    pen = get_pain_penalty().speed;
    if( pen >= 1 ) {
        mvwprintz( w_speed, line, 1, c_red, _( "Pain                -%s%d%%" ),
                   ( pen < 10 ? " " : "" ), pen );
        line++;
    }
    if( get_painkiller() >= 10 ) {
        pen = int( get_painkiller() * .1 );
        mvwprintz( w_speed, line, 1, c_red, _( "Painkillers         -%s%d%%" ),
                   ( pen < 10 ? " " : "" ), pen );
        line++;
    }
    if( stim != 0 ) {
        pen = std::min( 10, stim / 4 );
        if( pen > 0 )
            mvwprintz( w_speed, line, 1, c_green, _( "Stimulants          +%s%d%%" ),
                       ( pen < 10 ? " " : "" ), pen );
        else
            mvwprintz( w_speed, line, 1, c_red, _( "Depressants         -%s%d%%" ),
                       ( abs( pen ) < 10 ? " " : "" ), abs( pen ) );
        line++;
    }
    if( get_thirst() > 40 ) {
        pen = abs( thirst_speed_penalty( get_thirst() ) );
        mvwprintz( w_speed, line, 1, c_red, _( "Thirst              -%s%d%%" ),
                   ( pen < 10 ? " " : "" ), pen );
        line++;
    }
    if( get_hunger() > 100 ) {
        pen = abs( hunger_speed_penalty( get_hunger() ) );
        mvwprintz( w_speed, line, 1, c_red, _( "Hunger              -%s%d%%" ),
                   ( pen < 10 ? " " : "" ), pen );
        line++;
    }
    if( has_trait( "SUNLIGHT_DEPENDENT" ) && !g->is_in_sunlight( pos() ) ) {
        pen = ( g->light_level( posz() ) >= 12 ? 5 : 10 );
        mvwprintz( w_speed, line, 1, c_red, _( "Out of Sunlight     -%s%d%%" ),
                   ( pen < 10 ? " " : "" ), pen );
        line++;
    }
    if( has_trait( "COLDBLOOD4" ) && g->get_temperature() > 65 ) {
        pen = ( g->get_temperature() - 65 ) / 2;
        mvwprintz( w_speed, line, 1, c_green, _( "Cold-Blooded        +%s%d%%" ),
                   ( pen < 10 ? " " : "" ), pen );
        line++;
    }
    if( ( has_trait( "COLDBLOOD" ) || has_trait( "COLDBLOOD2" ) ||
          has_trait( "COLDBLOOD3" ) || has_trait( "COLDBLOOD4" ) ) &&
        g->get_temperature() < 65 ) {
        if( has_trait( "COLDBLOOD3" ) || has_trait( "COLDBLOOD4" ) ) {
            pen = ( 65 - g->get_temperature() ) / 2;
        } else if( has_trait( "COLDBLOOD2" ) ) {
            pen = ( 65 - g->get_temperature() ) / 3;
        } else {
            pen = ( 65 - g->get_temperature() ) / 5;
        }
        mvwprintz( w_speed, line, 1, c_red, _( "Cold-Blooded        -%s%d%%" ),
                   ( pen < 10 ? " " : "" ), pen );
        line++;
    }

    std::map<std::string, int> speed_effects;
    std::string dis_text = "";
    for( auto &elem : effects ) {
        for( auto &_effect_it : elem.second ) {
            auto &it = _effect_it.second;
            bool reduced = resists_effect( it );
            int move_adjust = it.get_mod( "SPEED", reduced );
            if( move_adjust != 0 ) {
                dis_text = it.get_speed_name();
                speed_effects[dis_text] += move_adjust;
            }
        }
    }

    for( auto &speed_effect : speed_effects ) {
        nc_color col = ( speed_effect.second > 0 ? c_green : c_red );
        mvwprintz( w_speed, line, 1, col, "%s", _( speed_effect.first.c_str() ) );
        mvwprintz( w_speed, line, 21, col, ( speed_effect.second > 0 ? "+" : "-" ) );
        mvwprintz( w_speed, line, ( abs( speed_effect.second ) >= 10 ? 22 : 23 ), col, "%d%%",
                   abs( speed_effect.second ) );
        line++;
    }

    int quick_bonus = int( newmoves - ( newmoves / 1.1 ) );
    int bio_speed_bonus = quick_bonus;
    if( has_trait( "QUICK" ) && has_bionic( "bio_speed" ) ) {
        bio_speed_bonus = int( newmoves / 1.1 - ( newmoves / 1.1 / 1.1 ) );
        std::swap( quick_bonus, bio_speed_bonus );
    }
    if( has_trait( "QUICK" ) ) {
        mvwprintz( w_speed, line, 1, c_green, _( "Quick               +%s%d%%" ),
                   ( quick_bonus < 10 ? " " : "" ), quick_bonus );
        line++;
    }
    if( has_bionic( "bio_speed" ) ) {
        mvwprintz( w_speed, line, 1, c_green, _( "Bionic Speed        +%s%d%%" ),
                   ( bio_speed_bonus < 10 ? " " : "" ), bio_speed_bonus );
    }

    int runcost = run_cost( 100 );
    nc_color col = ( runcost <= 100 ? c_green : c_red );
    mvwprintz( w_speed, 1, ( runcost  >= 100 ? 21 : ( runcost  < 10 ? 23 : 22 ) ), col,
               "%d", runcost );
    col = ( newmoves >= 100 ? c_green : c_red );
    mvwprintz( w_speed, 2, ( newmoves >= 100 ? 21 : ( newmoves < 10 ? 23 : 22 ) ), col,
               "%d", newmoves );
    wrefresh( w_speed );

    refresh();

    int curtab = 1;
    size_t min, max;
    line = 0;
    bool done = false;
    size_t half_y = 0;

    // Initial printing is DONE.  Now we give the player a chance to scroll around
    // and "hover" over different items for more info.
    do {
        werase( w_info );
        switch( curtab ) {
            case 1: // Stats tab
                mvwprintz( w_stats, 0, 0, h_ltgray, header_spaces.c_str() );
                mvwprintz( w_stats, 0, 13 - utf8_width( title_STATS ) / 2, h_ltgray, title_STATS );

                // Clear bonus/penalty menu.
                mvwprintz( w_stats, 6, 0, c_ltgray, "%26s", "" );
                mvwprintz( w_stats, 7, 0, c_ltgray, "%26s", "" );
                mvwprintz( w_stats, 8, 0, c_ltgray, "%26s", "" );

                if( line == 0 ) {
                    // Display player current strength effects
                    mvwprintz( w_stats, 2, 1, h_ltgray, _( "Strength:" ) );
                    mvwprintz( w_stats, 6, 1, c_magenta, _( "Base HP:" ) );
                    mvwprintz( w_stats, 6, 22, c_magenta, "%3d", hp_max[1] );
                    if( get_option<std::string>( "USE_METRIC_WEIGHTS" ) == "kg" ) {
                        mvwprintz( w_stats, 7, 1, c_magenta, _( "Carry weight(kg):" ) );
                    } else {
                        mvwprintz( w_stats, 7, 1, c_magenta, _( "Carry weight(lbs):" ) );
                    }
                    mvwprintz( w_stats, 7, 21, c_magenta, "%4.1f", convert_weight( weight_capacity() ) );
                    mvwprintz( w_stats, 8, 1, c_magenta, _( "Melee damage:" ) );
                    mvwprintz( w_stats, 8, 22, c_magenta, "%3.1f", bonus_damage( false ) );

                    fold_and_print( w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta,
                                    _( "Strength affects your melee damage, the amount of weight you can carry, your total HP, "
                                       "your resistance to many diseases, and the effectiveness of actions which require brute force." ) );
                } else if( line == 1 ) {
                    // Display player current dexterity effects
                    mvwprintz( w_stats, 3, 1, h_ltgray, _( "Dexterity:" ) );

                    mvwprintz( w_stats, 6, 1, c_magenta, _( "Melee to-hit bonus:" ) );
                    mvwprintz( w_stats, 6, 22, c_magenta, "%+.2lf", get_hit_base() );
                    mvwprintz( w_stats, 8, 1, c_magenta, _( "Throwing penalty per target's dodge: +%d" ) );
                    mvwprintz( w_stats, 8, 22, c_magenta, "%3d", throw_dispersion_per_dodge( false ) );

                    fold_and_print( w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta,
                                    _( "Dexterity affects your chance to hit in melee combat, helps you steady your "
                                       "gun for ranged combat, and enhances many actions that require finesse." ) );
                } else if( line == 2 ) {
                    // Display player current intelligence effects
                    mvwprintz( w_stats, 4, 1, h_ltgray, _( "Intelligence:" ) );
                    mvwprintz( w_stats, 6, 1, c_magenta, _( "Read times:" ) );
                    mvwprintz( w_stats, 6, 21, c_magenta, "%3d%%", read_speed( false ) );
                    mvwprintz( w_stats, 7, 1, c_magenta, _( "Skill rust:" ) );
                    mvwprintz( w_stats, 7, 22, c_magenta, "%2d%%", rust_rate( false ) );
                    mvwprintz( w_stats, 8, 1, c_magenta, _( "Crafting Bonus:" ) );
                    mvwprintz( w_stats, 8, 22, c_magenta, "%2d%%", get_int() );

                    fold_and_print( w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta,
                                    _( "Intelligence is less important in most situations, but it is vital for more complex tasks like "
                                       "electronics crafting.  It also affects how much skill you can pick up from reading a book." ) );
                } else if( line == 3 ) {
                    // Display player current perception effects
                    mvwprintz( w_stats, 5, 1, h_ltgray, _( "Perception:" ) );
                    mvwprintz( w_stats, 7, 1, c_magenta, _( "Trap detection level:" ) );
                    mvwprintz( w_stats, 7, 23, c_magenta, "%2d", get_per() );

                    fold_and_print( w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta,
                                    _( "Perception is the most important stat for ranged combat.  It's also used for "
                                       "detecting traps and other things of interest." ) );
                }
                wrefresh( w_stats );
                wrefresh( w_info );

                action = ctxt.handle_input();
                if( action == "DOWN" ) {
                    line++;
                    if( line == 4 ) {
                        line = 0;
                    }
                } else if( action == "UP" ) {
                    if( line == 0 ) {
                        line = 3;
                    } else {
                        line--;
                    }
                } else if( action == "NEXT_TAB" ) {
                    mvwprintz( w_stats, 0, 0, c_ltgray, header_spaces.c_str() );
                    mvwprintz( w_stats, 0, 13 - utf8_width( title_STATS ) / 2, c_ltgray, title_STATS );
                    wrefresh( w_stats );
                    line = 0;
                    curtab++;
                } else if( action == "QUIT" ) {
                    done = true;
                }
                mvwprintz( w_stats, 2, 1, c_ltgray, _( "Strength:" ) );
                mvwprintz( w_stats, 3, 1, c_ltgray, _( "Dexterity:" ) );
                mvwprintz( w_stats, 4, 1, c_ltgray, _( "Intelligence:" ) );
                mvwprintz( w_stats, 5, 1, c_ltgray, _( "Perception:" ) );
                wrefresh( w_stats );
                break;
            case 2: { // Encumberment tab
                werase( w_encumb );
                mvwprintz( w_encumb, 0, 13 - utf8_width( title_ENCUMB ) / 2, h_ltgray, title_ENCUMB );
                print_encumbrance( w_encumb, line );
                wrefresh( w_encumb );

                werase( w_info );
                std::string s;

                body_part bp = line <= 11 ? bp_aBodyPart[line] : num_bp;
                bool combined_here = ( bp_aiOther[line] == line + 1 ||
                                       bp_aiOther[line] == line - 1 ) && // first of a pair
                                     should_combine_bps( *this, line, bp_aiOther[line] );
                s += get_encumbrance_description( *this, bp, combined_here );
                fold_and_print( w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta, s );
                wrefresh( w_info );

                action = ctxt.handle_input();
                if( action == "DOWN" ) {
                    if( line < num_bp - 1 ) {
                        if( combined_here ) {
                            line += ( line < num_bp - 2 ) ? 2 : 0; // skip a line if we aren't at the last pair
                        } else {
                            line++; // unpaired or unequal
                        }
                    }
                } else if( action == "UP" ) {
                    if( line > 0 ) {
                        if( bp_aiOther[line] == line - 1 && // second of a pair
                            should_combine_bps( *this, line, bp_aiOther[line] ) ) {
                            line -= ( line > 1 ) ? 2 : 0; // skip a line if we aren't at the first pair
                        } else {
                            line--; // unpaired or unequal
                        }
                    }
                } else if( action == "NEXT_TAB" ) {
                    mvwprintz( w_encumb, 0, 0, c_ltgray, header_spaces.c_str() );
                    mvwprintz( w_encumb, 0, 13 - utf8_width( title_ENCUMB ) / 2, c_ltgray, title_ENCUMB );
                    wrefresh( w_encumb );
                    line = 0;
                    curtab++;
                } else if( action == "QUIT" ) {
                    done = true;
                }
                break;
            }
            case 4: // Traits tab
                mvwprintz( w_traits, 0, 0, h_ltgray, header_spaces.c_str() );
                mvwprintz( w_traits, 0, 13 - utf8_width( title_TRAITS ) / 2, h_ltgray, title_TRAITS );
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
                    max = line + ( trait_win_size_y + 1 ) / 2;
                    if( traitslist.size() < max ) {
                        max = traitslist.size();
                    }
                }

                for( size_t i = min; i < max; i++ ) {
                    const auto &mdata = mutation_branch::get( traitslist[i] );
                    mvwprintz( w_traits, int( 1 + i - min ), 1, c_ltgray, "                         " );
                    const auto color = mdata.get_display_color();
                    if( i == line ) {
                        mvwprintz( w_traits, int( 1 + i - min ), 1, hilite( color ), "%s",
                                   mdata.name.c_str() );
                    } else {
                        mvwprintz( w_traits, int( 1 + i - min ), 1, color, "%s",
                                   mdata.name.c_str() );
                    }
                }
                if( line < traitslist.size() ) {
                    const auto &mdata = mutation_branch::get( traitslist[line] );
                    fold_and_print( w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta,
                                    mdata.description );
                }
                wrefresh( w_traits );
                wrefresh( w_info );

                action = ctxt.handle_input();
                if( action == "DOWN" ) {
                    if( line < traitslist.size() - 1 ) {
                        line++;
                    }
                    break;
                } else if( action == "UP" ) {
                    if( line > 0 ) {
                        line--;
                    }
                } else if( action == "NEXT_TAB" ) {
                    mvwprintz( w_traits, 0, 0, c_ltgray, header_spaces.c_str() );
                    mvwprintz( w_traits, 0, 13 - utf8_width( title_TRAITS ) / 2, c_ltgray, title_TRAITS );
                    for( size_t i = 0; i < traitslist.size() && i < trait_win_size_y; i++ ) {
                        const auto &mdata = mutation_branch::get( traitslist[i] );
                        mvwprintz( w_traits, int( i + 1 ), 1, c_black, "                         " );
                        const auto color = mdata.get_display_color();
                        mvwprintz( w_traits, int( i + 1 ), 1, color, "%s", mdata.name.c_str() );
                    }
                    wrefresh( w_traits );
                    line = 0;
                    curtab++;
                } else if( action == "QUIT" ) {
                    done = true;
                }
                break;

            case 5: // Effects tab
                mvwprintz( w_effects, 0, 0, h_ltgray, header_spaces.c_str() );
                mvwprintz( w_effects, 0, 13 - utf8_width( title_EFFECTS ) / 2, h_ltgray, title_EFFECTS );
                half_y = effect_win_size_y / 2;
                if( line <= half_y ) {
                    min = 0;
                    max = effect_win_size_y;
                    if( effect_name.size() < max ) {
                        max = effect_name.size();
                    }
                } else if( line >= effect_name.size() - half_y ) {
                    min = ( effect_name.size() < effect_win_size_y ? 0 : effect_name.size() - effect_win_size_y );
                    max = effect_name.size();
                } else {
                    min = line - half_y;
                    max = line - half_y + effect_win_size_y;
                    if( effect_name.size() < max ) {
                        max = effect_name.size();
                    }
                }

                for( size_t i = min; i < max; i++ ) {
                    if( i == line ) {
                        mvwprintz( w_effects, int( 1 + i - min ), 0, h_ltgray, "%s", effect_name[i].c_str() );
                    } else {
                        mvwprintz( w_effects, int( 1 + i - min ), 0, c_ltgray, "%s", effect_name[i].c_str() );
                    }
                }
                if( line < effect_text.size() ) {
                    fold_and_print( w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta, effect_text[line] );
                }
                wrefresh( w_effects );
                wrefresh( w_info );

                action = ctxt.handle_input();
                if( action == "DOWN" ) {
                    if( line < effect_name.size() - 1 ) {
                        line++;
                    }
                    break;
                } else if( action == "UP" ) {
                    if( line > 0 ) {
                        line--;
                    }
                } else if( action == "NEXT_TAB" ) {
                    mvwprintz( w_effects, 0, 0, c_ltgray, header_spaces.c_str() );
                    mvwprintz( w_effects, 0, 13 - utf8_width( title_EFFECTS ) / 2, c_ltgray, title_EFFECTS );
                    for( size_t i = 0; i < effect_name.size() && i < 7; i++ ) {
                        mvwprintz( w_effects, int( i + 1 ), 0, c_ltgray, "%s", effect_name[i].c_str() );
                    }
                    wrefresh( w_effects );
                    line = 0;
                    curtab = 1;
                } else if( action == "QUIT" ) {
                    done = true;
                }
                break;

            case 3: // Skills tab
                mvwprintz( w_skills, 0, 0, h_ltgray, header_spaces.c_str() );
                mvwprintz( w_skills, 0, 13 - utf8_width( title_SKILLS ) / 2, h_ltgray, title_SKILLS );
                half_y = skill_win_size_y / 2;
                if( line <= half_y ) {
                    min = 0;
                    max = skill_win_size_y;
                    if( skillslist.size() < max ) {
                        max = skillslist.size();
                    }
                } else if( line >= skillslist.size() - half_y ) {
                    min = ( skillslist.size() < size_t( skill_win_size_y ) ? 0 : skillslist.size() - skill_win_size_y );
                    max = skillslist.size();
                } else {
                    min = line - half_y;
                    max = line - half_y + skill_win_size_y;
                    if( skillslist.size() < max ) {
                        max = skillslist.size();
                    }
                }

                const Skill *selectedSkill = NULL;

                for( size_t i = min; i < max; i++ ) {
                    const Skill *aSkill = skillslist[i];
                    SkillLevel level = get_skill_level( aSkill->ident() );

                    const bool can_train = level.can_train();
                    const bool training = level.isTraining();
                    const bool rusting = level.isRusting();
                    const int exercise = level.exercise();

                    nc_color cstatus;
                    if( i == line ) {
                        selectedSkill = aSkill;
                        if( !can_train ) {
                            cstatus = rusting ? h_ltred : h_white;
                        } else if( exercise >= 100 ) {
                            cstatus = training ? h_pink : h_magenta;
                        } else if( rusting ) {
                            cstatus = training ? h_ltred : h_red;
                        } else {
                            cstatus = training ? h_ltblue : h_blue;
                        }
                    } else {
                        if( rusting ) {
                            cstatus = training ? c_ltred : c_red;
                        } else if( !can_train ) {
                            cstatus = c_white;
                        } else {
                            cstatus = training ? c_ltblue : c_blue;
                        }
                    }
                    mvwprintz( w_skills, int( 1 + i - min ), 1, c_ltgray, "                         " );
                    mvwprintz( w_skills, int( 1 + i - min ), 1, cstatus, "%s:", aSkill->name().c_str() );
                    mvwprintz( w_skills, int( 1 + i - min ), 19, cstatus, "%-2d(%2d%%)", ( int )level,
                               ( exercise <  0 ? 0 : exercise ) );
                }

                draw_scrollbar( w_skills, line, skill_win_size_y, int( skillslist.size() ), 1 );
                wrefresh( w_skills );

                werase( w_info );

                if( line < skillslist.size() ) {
                    fold_and_print( w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta, selectedSkill->description() );
                }
                wrefresh( w_info );

                action = ctxt.handle_input();
                if( action == "DOWN" ) {
                    if( size_t( line ) < skillslist.size() - 1 ) {
                        line++;
                    }
                } else if( action == "UP" ) {
                    if( line > 0 ) {
                        line--;
                    }
                } else if( action == "NEXT_TAB" ) {
                    werase( w_skills );
                    mvwprintz( w_skills, 0, 0, c_ltgray, header_spaces.c_str() );
                    mvwprintz( w_skills, 0, 13 - utf8_width( title_SKILLS ) / 2, c_ltgray, title_SKILLS );
                    for( size_t i = 0; i < skillslist.size() && i < size_t( skill_win_size_y ); i++ ) {
                        const Skill *thisSkill = skillslist[i];
                        SkillLevel level = get_skill_level( thisSkill->ident() );
                        bool can_train = level.can_train();
                        bool isLearning = level.isTraining();
                        bool rusting = level.isRusting();

                        nc_color cstatus;
                        if( rusting ) {
                            cstatus = isLearning ? c_ltred : c_red;
                        } else if( !can_train ) {
                            cstatus = c_white;
                        } else {
                            cstatus = isLearning ? c_ltblue : c_blue;
                        }

                        mvwprintz( w_skills, i + 1,  1, cstatus, "%s:", thisSkill->name().c_str() );
                        mvwprintz( w_skills, i + 1, 19, cstatus, "%-2d(%2d%%)", ( int )level,
                                   ( level.exercise() <  0 ? 0 : level.exercise() ) );
                    }
                    wrefresh( w_skills );
                    line = 0;
                    curtab++;
                } else if( action == "CONFIRM" ) {
                    get_skill_level( selectedSkill->ident() ).toggleTraining();
                } else if( action == "QUIT" ) {
                    done = true;
                }
        }
    } while( !done );

    werase( w_info );
    werase( w_tip );
    werase( w_stats );
    werase( w_encumb );
    werase( w_traits );
    werase( w_effects );
    werase( w_skills );
    werase( w_speed );
    werase( w_info );
    werase( w_grid_top );
    werase( w_grid_effect );
    werase( w_grid_skill );
    werase( w_grid_trait );

    delwin( w_info );
    delwin( w_tip );
    delwin( w_stats );
    delwin( w_encumb );
    delwin( w_traits );
    delwin( w_effects );
    delwin( w_skills );
    delwin( w_speed );
    delwin( w_grid_top );
    delwin( w_grid_effect );
    delwin( w_grid_skill );
    delwin( w_grid_trait );

    g->refresh_all();
}
