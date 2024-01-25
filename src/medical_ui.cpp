#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>
#include <utility>

#include "addiction.h"
#include "avatar_action.h"
#include "creature.h"
#include "character.h"
#include "character_modifier.h"
#include "display.h"
#include "effect.h"
#include "flag.h"
#include "game.h"
#include "input_context.h"
#include "output.h"
#include "ui_manager.h"
#include "weather.h"

static const efftype_id effect_bite( "bite" );
static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_infected( "infected" );
static const efftype_id effect_mending( "mending" );

static const json_character_flag json_flag_ECTOTHERM( "ECTOTHERM" );
static const json_character_flag json_flag_PAIN_IMMUNE( "PAIN_IMMUNE" );

static const trait_id trait_SUNLIGHT_DEPENDENT( "SUNLIGHT_DEPENDENT" );
static const trait_id trait_TROGLO( "TROGLO" );
static const trait_id trait_TROGLO2( "TROGLO2" );
static const trait_id trait_TROGLO3( "TROGLO3" );

enum class medical_tab_mode {
    TAB_SUMMARY
};

class selection_line
{
    public:
        selection_line() = default;
        selection_line( const std::string &text, const std::string &desc_str, const int max_width )
            : desc_str( desc_str ) {
            std::vector<std::string> textformatted = foldstring( text, max_width,
                    ']' );
            row_count = textformatted.size();
            if( row_count > 1 ) {
                //If there are too many tags, display them neatly on a new line.
                std::string print_line = string_format( "%s\n", textformatted[0] );
                for( int i = 1; i < row_count; i++ ) {
                    if( i != row_count ) {
                        print_line += string_format( "->%s\n", textformatted[i] );
                    } else {
                        print_line += string_format( "->%s", textformatted[i] );
                    }
                }
                header_str = print_line;
            } else {
                header_str = text;
            }
        }

        std::string print() {
            if( highlight_line ) {
                header_str = colorize( ">", h_white ) + header_str;
            }
            return header_str;
        }

        std::string description() {
            return desc_str;
        }

        void set_detail( const std::string &header, const std::string &detail ) {
            detail_str = std::pair<std::string, std::string>( header, detail );
        }

        std::pair<std::string, std::string> get_detail() {
            return detail_str;
        }

        int get_row_count() const {
            return row_count;
        }

        void set_highlight() {
            highlight_line = true;
        }

        bool highlighted() const {
            return highlight_line;
        }

    private:
        std::string header_str;
        std::string desc_str;
        std::pair<std::string, std::string> detail_str;
        int row_count;
        bool highlight_line = false;
};

class medical_column
{
    public:
        medical_column() = default;
        medical_column( const int column_id, const point &COLUMN_START,
                        const std::pair<int, int> &COLUMN_BOUNDS )
            : column_id( column_id ), COLUMN_BOUNDS( COLUMN_BOUNDS ), COLUMN_START( COLUMN_START ) {}

        void draw_column( const catacurses::window &window, const int BORDER_START,
                          const int BORDER_END ) const {
            mvwvline( window, point( COLUMN_START.x, BORDER_START ), LINE_XOXO,
                      BORDER_END - 4 ); // |
            mvwputch( window, point( COLUMN_START.x, BORDER_END - 1 ), BORDER_COLOR,
                      LINE_XXOX ); // _|_
        }

        void print_column( const catacurses::window &window, const int LINE_START, const int MAX_HEIGHT ) {
            int linerow = 0;
            int selectionrow = 0;

            for( selection_line &line : column_lines ) {
                const point row_start(
                    line.highlighted() ?
                    COLUMN_START.x + left_padding - 1 :
                    COLUMN_START.x + left_padding,
                    COLUMN_START.y + linerow
                );

                if( row_start.y - LINE_START >= MAX_HEIGHT ) {
                    break;
                }

                if( linerow >= LINE_START ) {
                    fold_and_print( window, row_start - point( 0, LINE_START ), max_width(),
                                    c_light_gray, line.print() );
                    linerow += line.get_row_count();
                } else {
                    linerow++;
                }
                ++selectionrow;
            }
            COLUMN_ROWS = { selectionrow, linerow };
        }

        void add_column_line( const selection_line &line ) {
            column_lines.emplace_back( line );
            COLUMN_ROWS.second += line.get_row_count();
        }

        selection_line set_highlight( int y ) {
            int offset = y % column_lines.size();
            column_lines[offset].set_highlight();
            return column_lines[offset];
        }

        int selection_count() const {
            return COLUMN_ROWS.first;
        }

        int row_count() const {
            return COLUMN_ROWS.second;
        }

        int max_width() const {
            return COLUMN_BOUNDS.first - COLUMN_START.x - left_padding;
        }

        bool empty() {
            return column_lines.empty();
        }

        bool current_column( const int selected_id ) const {
            return column_id == selected_id;
        }

        std::pair<std::string, std::string> detail_str( int y ) {
            std::pair<std::string, std::string> ret;
            if( y < static_cast<int>( column_lines.size() ) ) {
                int offset = y % column_lines.size();
                ret = column_lines[offset].get_detail();
            }
            return ret;
        }

    private:
        int column_id;
        const int left_padding = 2;
        std::pair<int, int> COLUMN_ROWS = { 0, 0 }; // Selection Lines - Print Lines
        std::pair<int, int> COLUMN_BOUNDS; // Left Bound - Right Bound
        point COLUMN_START;
        std::vector<selection_line>column_lines;
        std::string column_title;
};

static std::string coloured_stat_display( int statCur, int statMax )
{
    nc_color cstatus;
    if( statCur <= 0 ) {
        cstatus = c_dark_gray;
    } else if( statCur < statMax / 2 ) {
        cstatus = c_red;
    } else if( statCur < statMax ) {
        cstatus = c_light_red;
    } else if( statCur == statMax ) {
        cstatus = c_white;
    } else if( statCur < statMax * 1.5 ) {
        cstatus = c_light_green;
    } else {
        cstatus = c_green;
    }
    std::string cur = colorize( string_format( _( "%2d" ), statCur ), cstatus );
    return string_format( _( "%s (%s)" ), cur, statMax );
}

static void draw_medical_titlebar( const catacurses::window &window, avatar *player,
                                   const int WIDTH )
{
    input_context ctxt( "MEDICAL", keyboard_mode::keychar );
    const Character &you = *player->as_character();

    werase( window );
    draw_border( window, BORDER_COLOR, _( " MEDICAL " ) );

    // Tabs
    const std::vector<std::pair<medical_tab_mode, std::string>> tabs = {
        { medical_tab_mode::TAB_SUMMARY, string_format( _( "SUMMARY" ) ) }
    };
    draw_tabs( window, tabs, medical_tab_mode::TAB_SUMMARY );

    const int TAB_WIDTH = 12;

    // Draw symbols to connect additional lines to border

    int width = getmaxx( window );
    int height = getmaxy( window );
    for( int i = 1; i < height - 1; ++i ) {
        // |
        mvwputch( window, point( 0, i ), BORDER_COLOR, LINE_XOXO );
        // |
        mvwputch( window, point( width - 1, i ), BORDER_COLOR, LINE_XOXO );
    }
    // |-
    mvwputch( window, point( 0, height - 1 ), BORDER_COLOR, LINE_XXXO );
    // -|
    mvwputch( window, point( width - 1, height - 1 ), BORDER_COLOR, LINE_XOXX );

    int right_indent = 2;
    int cur_str_pos = 0;

    // Pain Indicator
    auto pain_descriptor = display::pain_text_color( *player );
    if( !pain_descriptor.first.empty() ) {
        const std::string pain_str = string_format( _( "In %s" ), pain_descriptor.first );

        cur_str_pos = right_print( window, 1, right_indent, pain_descriptor.second, pain_str );

        // Borders
        for( int i = 1; i < getmaxy( window ) - 1; i++ ) {
            mvwputch( window, point( cur_str_pos - 2, i ), BORDER_COLOR, LINE_XOXO ); // |
        }
        mvwputch( window, point( cur_str_pos - 2, 0 ), BORDER_COLOR, LINE_OXXX ); // ^|^
        mvwputch( window, point( cur_str_pos - 2, 2 ), BORDER_COLOR, LINE_XXOX ); // _|_
        right_indent += utf8_width( remove_color_tags( pain_str ) ) + 3;
    }

    const std::pair<std::string, nc_color> hunger_pair = display::hunger_text_color( you );
    const std::pair<std::string, nc_color> thirst_pair = display::thirst_text_color( you );
    const std::pair<std::string, nc_color> fatigue_pair = display::fatigue_text_color( you );

    // Hunger
    if( !hunger_pair.first.empty() ) {
        cur_str_pos = right_print( window, 1, right_indent, hunger_pair.second, hunger_pair.first );

        // Borders
        for( int i = 1; i < getmaxy( window ) - 1; i++ ) {
            mvwputch( window, point( cur_str_pos - 2, i ), BORDER_COLOR, LINE_XOXO ); // |
        }
        mvwputch( window, point( cur_str_pos - 2, 0 ), BORDER_COLOR, LINE_OXXX ); // ^|^
        mvwputch( window, point( cur_str_pos - 2, 2 ), BORDER_COLOR, LINE_XXOX ); // _|_

        right_indent += utf8_width( hunger_pair.first ) + 3;
    }

    // Thirst
    if( !thirst_pair.first.empty() ) {
        cur_str_pos = right_print( window, 1, right_indent, thirst_pair.second, thirst_pair.first );

        // Borders
        for( int i = 1; i < getmaxy( window ) - 1; i++ ) {
            mvwputch( window, point( cur_str_pos - 2, i ), BORDER_COLOR, LINE_XOXO ); // |
        }
        mvwputch( window, point( cur_str_pos - 2, 0 ), BORDER_COLOR, LINE_OXXX ); // ^|^
        mvwputch( window, point( cur_str_pos - 2, 2 ), BORDER_COLOR, LINE_XXOX ); // _|_

        right_indent += utf8_width( thirst_pair.first ) + 3;
    }

    // Fatigue
    if( !fatigue_pair.first.empty() ) {
        cur_str_pos = right_print( window, 1, right_indent, fatigue_pair.second, fatigue_pair.first );

        // Borders
        for( int i = 1; i < getmaxy( window ) - 1; i++ ) {
            mvwputch( window, point( cur_str_pos - 2, i ), BORDER_COLOR, LINE_XOXO ); // |
        }
        mvwputch( window, point( cur_str_pos - 2, 0 ), BORDER_COLOR, LINE_OXXX ); // ^|^
        mvwputch( window, point( cur_str_pos - 2, 2 ), BORDER_COLOR, LINE_XXOX ); // _|_

        right_indent += utf8_width( fatigue_pair.first ) + 3;
    }

    // Hotkey Helper
    std::string desc;
    desc = string_format( _(
                              "[<color_yellow>%s/%s</color>] Scroll info [<color_yellow>%s</color>] Use item [<color_yellow>%s</color>] Keybindings" ),
                          ctxt.get_desc( "SCROLL_INFOBOX_UP" ), ctxt.get_desc( "SCROLL_INFOBOX_DOWN" ),
                          ctxt.get_desc( "APPLY" ), ctxt.get_desc( "HELP_KEYBINDINGS" ) );

    const int details_width = utf8_width( remove_color_tags( desc ) ) + 3;
    const int max_width = right_indent + TAB_WIDTH;
    if( WIDTH - max_width > details_width ) {
        // If the window runs out of room, we won't print keybindings.
        right_print( window, 1, right_indent, c_white, desc );
    }

    const std::string TITLE_STR = "Medical";

    // Window Title
    if( WIDTH - details_width - utf8_width( TITLE_STR ) > WIDTH / 2 ) {
        center_print( window, 0, c_blue, _( TITLE_STR ) );
    }
}

// Displays a summary of each bodypart's health, including a display for a few 'statuses'
static medical_column draw_health_summary( const int column_count, avatar *player,
        const point &COLUMN_START,
        const std::pair<int, int> &COLUMN_BOUNDS )
{
    medical_column health_column = medical_column( column_count, COLUMN_START, COLUMN_BOUNDS );
    const int max_width = health_column.max_width();

    for( const bodypart_id &part : player->get_all_body_parts( get_body_part_flags::sorted ) ) {
        std::string header; // Bodypart Title
        std::string hp_str; // Bodypart HP
        std::string detail;
        std::string description;

        const int bleed_intensity = player->get_effect_int( effect_bleed, part );
        const bool bleeding = bleed_intensity > 0;
        const bool bitten = player->has_effect( effect_bite, part.id() );
        const bool infected = player->has_effect( effect_infected, part.id() );
        const bool no_feeling = player->has_flag( json_flag_PAIN_IMMUNE );
        const int maximal_hp = player->get_part_hp_max( part );
        const int current_hp = player->get_part_hp_cur( part );
        const bool limb_is_broken = player->is_limb_broken( part );
        const bool limb_is_mending = player->worn_with_flag( flag_SPLINT, part );

        if( limb_is_mending ) {
            detail += string_format( _( "[ %s ]" ), colorize( _( "SPLINTED" ), c_yellow ) );
            if( no_feeling ) {
                hp_str = colorize( "==%==", c_blue );
            } else {
                const effect &eff = player->get_effect( effect_mending, part );
                const int mend_perc = eff.is_null() ? 0.0 : 100 * eff.get_duration() / eff.get_max_duration();

                const int num = mend_perc / 20;
                hp_str = colorize( std::string( num, '#' ) + std::string( 5 - num, '=' ), c_blue );
            }
        } else if( limb_is_broken ) {
            detail += string_format( _( "[ %s ]" ), colorize( _( "BROKEN" ), c_red ) );
            hp_str = "==%==";
        } else if( no_feeling ) {
            const float cur_hp_pcnt = current_hp / static_cast<float>( maximal_hp );
            if( cur_hp_pcnt < 0.125f ) {
                hp_str = colorize( _( "Very Bad" ), c_red );
            } else if( cur_hp_pcnt < 0.375f ) {
                hp_str = colorize( _( "Bad" ), c_light_red );
            } else if( cur_hp_pcnt < 0.625f ) {
                hp_str = colorize( _( "So-so" ), c_yellow );
            } else if( cur_hp_pcnt < 0.875f ) {
                hp_str = colorize( _( "Okay" ), c_light_green );
            } else {
                hp_str = colorize( _( "Good" ), c_green );
            }
        } else {
            std::pair<std::string, nc_color> h_bar = get_hp_bar( current_hp, maximal_hp, false );
            hp_str = colorize( h_bar.first, h_bar.second ) +
                     colorize( std::string( 5 - utf8_width( h_bar.first ), '.' ), c_white );
        }
        const std::string bp_name = uppercase_first_letter( body_part_name( part, 1 ) );
        header += colorize( bp_name,
                            display::limb_color( *player,
                                    part, true, true, true ) ) + " " + hp_str;

        // BLEEDING block
        if( bleeding ) {
            const effect bleed_effect = player->get_effect( effect_bleed, part );
            const nc_color bleeding_color = colorize_bleeding_intensity( bleed_intensity );
            detail += string_format( _( "[ %s ]" ), colorize( _( "BLEEDING" ), bleeding_color ) );
            description += string_format( "[ %s ] - %s\n",
                                          colorize( bleed_effect.get_speed_name(),  bleeding_color ),
                                          bleed_effect.disp_short_desc() );
        }

        // BITTEN block
        if( bitten ) {
            const effect bite_effect = player->get_effect( effect_bite, part );
            detail += string_format( _( "[ %s ]" ), colorize( _( "BITTEN" ), c_yellow ) );
            description += string_format( "[ %s ] - %s\n",
                                          colorize( bite_effect.get_speed_name(), c_yellow ),
                                          bite_effect.disp_short_desc() );
        }

        // INFECTED block
        if( infected ) {
            const effect infected_effect = player->get_effect( effect_infected, part );
            detail += string_format( _( "[ %s ]" ), colorize( _( "INFECTED" ), c_pink ) );
            description += string_format( "[ %s ] - %s\n",
                                          colorize( infected_effect.get_speed_name(), c_pink ),
                                          infected_effect.disp_short_desc() );
        }

        selection_line line;
        if( !detail.empty() ) {
            line = selection_line( string_format( "[%s] - %s", header, detail ), description, max_width );
        } else {
            line = selection_line( string_format( "[%s]", header ), description, max_width );
        }

        const bodypart *bp = player->get_part( part );
        std::string detail_str;
        for( const limb_score &sc : limb_score::get_all() ) {
            if( !part->has_limb_score( sc.getId() ) ) {
                continue;
            }

            float injury_score = bp->get_limb_score( sc.getId(), 0, 0, 1 );
            float max_score = part->get_limb_score( sc.getId() );

            if( injury_score < max_score ) {
                const float injury_modifier = 100 * ( max_score - injury_score ) / max_score;
                std::pair<std::string, nc_color> score_c;
                if( injury_score < max_score * 0.4f ) {
                    score_c.first = string_format( _( "Crippled (-%.f%%)" ), injury_modifier );
                    score_c.second = c_red;
                } else if( injury_score < max_score * 0.6f ) {
                    score_c.first = string_format( _( "Impaired (-%.f%%)" ), injury_modifier );
                    score_c.second = c_light_red;
                } else if( injury_score < max_score * 0.75f ) {
                    score_c.first = string_format( _( "Weakened (-%.f%%)" ), injury_modifier );
                    score_c.second = c_yellow;
                } else if( injury_score < max_score * 0.9f ) {
                    score_c.first = string_format( _( "Weakened (-%.f%%)" ), injury_modifier );
                    score_c.second = c_dark_gray;
                } else {
                    score_c.first = string_format( _( "OK (-%.f%%)" ), injury_modifier );
                    score_c.second = c_dark_gray;
                }
                detail_str += string_format( _( "%s: %s\n" ), sc.name().translated(), colorize( score_c.first,
                                             score_c.second ) );
            } else {
                detail_str += string_format( _( "%s: %s\n" ), sc.name().translated(), colorize( "OK", c_green ) );
            }
        }

        for( const character_modifier &mod : character_modifier::get_all() ) {
            for( const auto &sc : mod.use_limb_scores() ) {
                if( sc.first.is_null() || !part->has_limb_score( sc.first ) ) {
                    continue;
                }
                std::string desc = mod.description().translated();
                float injury_score = bp->get_limb_score( sc.first, 0, 0, 1 );
                float max_score = part->get_limb_score( sc.first );
                nc_color score_c;

                if( injury_score < max_score * 0.4f ) {
                    score_c = c_red;
                } else if( injury_score < max_score * 0.6f ) {
                    score_c = c_light_red;
                } else if( injury_score < max_score * 0.75f ) {
                    score_c = c_yellow;
                } else {
                    score_c = c_white;
                }

                std::string valstr = colorize( string_format( "%.2f", mod.modifier( *player->as_character() ) ),
                                               score_c );
                detail_str += string_format( "%s: %s%s\n", desc, mod.mod_type_str(), valstr );
            }
        }

        line.set_detail( string_format( _( "%s STATS" ), to_upper_case( bp_name ) ), detail_str );
        health_column.add_column_line( line );
    }
    return health_column;
}

// Displays a summary list of all visible effects.
static medical_column draw_effects_summary( const int column_count, avatar *player,
        const point &COLUMN_START,
        const std::pair<int, int> &COLUMN_BOUNDS )
{
    medical_column effects_column = medical_column( column_count, COLUMN_START, COLUMN_BOUNDS );
    const int max_width = effects_column.max_width();

    for( const effect &eff : player->get_effects() ) {
        const std::string name = eff.disp_name();
        if( name.empty() ) {
            continue;
        }
        effects_column.add_column_line( selection_line( name, eff.disp_desc(), max_width ) );
    }

    const float bmi = player->get_bmi_fat();

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

        effects_column.add_column_line( selection_line( starvation_name, starvation_text, max_width ) );
    }

    if( player->has_trait( trait_TROGLO3 ) && g->is_in_sunlight( player->pos() ) ) {
        effects_column.add_column_line( selection_line( "In Sunlight",
                                        "The sunlight irritates you terribly.\n", max_width ) );
    } else if( player->has_trait( trait_TROGLO2 ) && g->is_in_sunlight( player->pos() ) &&
               incident_sun_irradiance( get_weather().weather_id, calendar::turn ) > irradiance::low ) {
        effects_column.add_column_line( selection_line( "In Sunlight",
                                        "The sunlight irritates you badly.\n", max_width ) );
    } else if( ( player->has_trait( trait_TROGLO ) || player->has_trait( trait_TROGLO2 ) ) &&
               g->is_in_sunlight( player->pos() ) &&
               incident_sun_irradiance( get_weather().weather_id, calendar::turn ) > irradiance::moderate ) {
        effects_column.add_column_line( selection_line( "In Sunlight", "The sunlight irritates you.\n",
                                        max_width ) );
    }

    for( addiction &elem : player->addictions ) {
        if( elem.sated < 0_turns && elem.intensity >= MIN_ADDICTION_LEVEL ) {
            effects_column.add_column_line( selection_line( elem.type->get_name().translated(),
                                            elem.type->get_description().translated(), max_width ) );
        }
    }

    if( effects_column.empty() ) {
        effects_column.add_column_line( selection_line( colorize( "None", c_dark_gray ), "", max_width ) );
    }

    return effects_column;
}

// Displays a summary list of the player's statistics.
static medical_column draw_stats_summary( const int column_count, avatar *player,
        const point &COLUMN_START,
        const std::pair<int, int> &COLUMN_BOUNDS )
{
    medical_column stats_column = medical_column( column_count, COLUMN_START, COLUMN_BOUNDS );
    const int max_width = stats_column.max_width();

    std::string speed_detail_str;
    int runcost = player->run_cost( 100 );
    int newmoves = player->get_speed();

    std::string coloured_str = colorize( string_format( _( "%d" ), runcost ),
                                         ( runcost <= 100 ? c_green : c_red ) );
    selection_line runcost_line = selection_line( string_format( _( "Base Move Cost: %s" ),
                                  coloured_str ),
                                  colorize( _( "Base move cost is the final modified movement cost taken to traverse flat ground." ),
                                            c_light_blue ),
                                  max_width );

    coloured_str = colorize( string_format( _( "%d" ), newmoves ),
                             ( newmoves >= 100 ? c_green : c_red ) );
    selection_line movecost_line = selection_line( string_format( _( "Current Speed: %s" ),
                                   coloured_str ),
                                   colorize( _( "Speed determines the amount of actions or movement points you can perform in a turn." ),
                                           c_light_blue ),
                                   max_width );

    const int speed_modifier = player->get_enchantment_speed_bonus();

    std::string pge_str;
    if( speed_modifier != 0 ) {
        pge_str = pgettext( "speed bonus", "Bio/Mut/Effects " );
        speed_detail_str += colorize( string_format( _( "%s    -%2d%%\n" ), pge_str, speed_modifier ),
                                      c_green );
    }

    int pen = 0;

    if( player->weight_carried() > player->weight_capacity() ) {
        pen = 25 * ( player->weight_carried() - player->weight_capacity() ) / player->weight_capacity();
        pge_str = pgettext( "speed penalty", "Overburdened " );
        speed_detail_str += colorize( string_format( _( "%s    -%2d%%\n" ), pge_str, pen ), c_red );
    }

    pen = player->get_pain_penalty().speed;
    if( pen >= 1 ) {
        pge_str = pgettext( "speed penalty", "Pain " );
        speed_detail_str += colorize( string_format( _( "%s    -%2d%%\n" ), pge_str, pen ), c_red );
    }
    if( player->get_thirst() > 40 ) {
        pen = std::abs( Character::thirst_speed_penalty( player->get_thirst() ) );
        pge_str = pgettext( "speed penalty", "Thirst " );
        speed_detail_str += colorize( string_format( _( "%s    -%2d%%\n" ), pge_str, pen ), c_red );
    }
    if( player->kcal_speed_penalty() < 0 ) {
        pen = std::abs( player->kcal_speed_penalty() );
        pge_str = pgettext( "speed penalty", player->get_bmi() < character_weight_category::underweight ?
                            "Starving" : "Underfed" );
        speed_detail_str += colorize( string_format( _( "%s    -%2d%%\n" ), pge_str, pen ), c_red );
    }
    if( player->has_trait( trait_SUNLIGHT_DEPENDENT ) && !g->is_in_sunlight( player->pos() ) ) {
        pen = ( g->light_level( player->posz() ) >= 12 ? 5 : 10 );
        pge_str = pgettext( "speed penalty", "Out of Sunlight " );
        speed_detail_str += colorize( string_format( _( "%s     -%2d%%\n" ), pge_str, pen ), c_red );
    }

    const float temperature_speed_modifier = player->mutation_value( "temperature_speed_modifier" );
    if( temperature_speed_modifier != 0 ) {
        nc_color pen_color;
        std::string pen_sign;
        const units::temperature player_local_temp = get_weather().get_temperature( player->pos() );
        if( player->has_flag( json_flag_ECTOTHERM ) && player_local_temp > units::from_fahrenheit( 65 ) ) {
            pen_color = c_green;
            pen_sign = "+";
        } else if( player_local_temp < units::from_fahrenheit( 65 ) ) {
            pen_color = c_red;
            pen_sign = "-";
        }
        if( !pen_sign.empty() ) {
            pen = ( units::to_fahrenheit( player_local_temp ) - 65 ) * temperature_speed_modifier;
            pge_str = pgettext( "speed modifier", "Cold-Blooded " );
            speed_detail_str += colorize( string_format( _( "%s     %s%2d%%\n" ), pge_str, pen_sign,
                                          std::abs( pen ) ), pen_color );
        }
    }

    std::map<std::string, int> speed_effects;
    for( const effect &elem : player->get_effects() ) {
        bool reduced = player->resists_effect( elem );
        int move_adjust = elem.get_mod( "SPEED", reduced );
        if( move_adjust != 0 ) {
            const std::string dis_text = elem.get_speed_name();
            speed_effects[dis_text] += move_adjust;
        }
    }

    for( const std::pair<const std::string, int> &speed_effect : speed_effects ) {
        nc_color col = speed_effect.second > 0 ? c_green : c_red;
        speed_detail_str += colorize( string_format( _( "%s    %s%d%%\n" ), speed_effect.first,
                                      ( speed_effect.second > 0 ? "+" : "-" ),
                                      std::abs( speed_effect.second ) ), col );
    }

    runcost_line.set_detail( _( "SPEED" ), speed_detail_str );
    movecost_line.set_detail( _( "SPEED" ), speed_detail_str );

    stats_column.add_column_line( runcost_line );
    stats_column.add_column_line( movecost_line );

    std::string strength_str = coloured_stat_display( player->get_str(), player->get_str_base() );
    stats_column.add_column_line(
        selection_line( string_format( _( "Strength: %s" ), strength_str ),
                        _( "Strength affects your melee damage, the amount of weight you can carry, your total HP, "
                           "your resistance to many diseases, and the effectiveness of actions which require brute force." ),
                        max_width ) );

    std::string dexterity_str = coloured_stat_display( player->get_dex(), player->get_dex_base() );
    stats_column.add_column_line(
        selection_line( string_format( _( "Dexterity: %s" ), dexterity_str ),
                        _( "Dexterity affects your chance to hit in melee combat, helps you steady your "
                           "gun for ranged combat, and enhances many actions that require finesse." ),
                        max_width ) );

    std::string intelligence_str = coloured_stat_display( player->get_int(), player->get_int_base() );
    stats_column.add_column_line(
        selection_line( string_format( _( "Intelligence: %s" ), intelligence_str ),
                        _( "Intelligence is less important in most situations, but it is vital for more complex tasks like "
                           "electronics crafting.  It also affects how much skill you can pick up from reading a book." ),
                        max_width ) );

    std::string perception_str = coloured_stat_display( player->get_per(), player->get_per_base() );
    stats_column.add_column_line(
        selection_line( string_format( _( "Perception: %s" ), perception_str ),
                        _( "Perception is the most important stat for ranged combat.  It's also used for "
                           "detecting traps and other things of interest." ),
                        max_width ) );

    return stats_column;
}

void avatar::disp_medical()
{
    // Windows
    catacurses::window w_title; // Title Bar - Tabs, Pain Indicator & Blood Indicator
    catacurses::window wMedical; // Primary Window
    catacurses::window w_description; // Bottom Detail Bar

    // Window Definitions
    const int TITLE_W_HEIGHT = 3;
    const int DESC_W_HEIGHT = 6; // Consistent with Player Info (@) Menu
    const int HEADER_Y = TITLE_W_HEIGHT;
    const int TEXT_START_Y = HEADER_Y + 2;
    const int INFO_START_Y = HEADER_Y + 8;
    int DESC_W_BEGIN;
    int HEIGHT;
    int WIDTH;

    // Column Definitions
    int second_column_x = 0;
    int third_column_x = 0;

    // Scrolling
    int SCROLL_POINT; // The number of printed rows at which to enable scrolling
    int scroll_position = 0;
    int INFO_SCROLL_POINT;
    int info_scroll_position = 0;

    int info_lines = 0;

    // Cursor
    std::array<int, 3> cursor_bounds; // Number of selectable rows in each column
    point cursor;

    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        const int WIDTH_OFFSET = ( TERMX - FULL_SCREEN_WIDTH ) / 4;
        HEIGHT = std::min( TERMY, FULL_SCREEN_HEIGHT );
        WIDTH = FULL_SCREEN_WIDTH + WIDTH_OFFSET;

        const point win( ( TERMX - WIDTH ) / 2, ( TERMY - HEIGHT ) / 2 );

        wMedical = catacurses::newwin( HEIGHT, WIDTH, win );

        w_title = catacurses::newwin( TITLE_W_HEIGHT, WIDTH, win );

        DESC_W_BEGIN = HEIGHT - DESC_W_HEIGHT - 1;
        w_description = catacurses::newwin( DESC_W_HEIGHT, WIDTH - 2,
                                            win + point( 1, DESC_W_BEGIN ) );

        //40% - 30% - 30%
        second_column_x = WIDTH / 2.5f;
        third_column_x = second_column_x + WIDTH / 3.3f;

        ui.position_from_window( wMedical );
    } );

    ui.mark_resize();

    ui.on_redraw( [&]( const ui_adaptor & ) {
        werase( wMedical );

        draw_border( wMedical, BORDER_COLOR, _( " MEDICAL " ) );
        mvwputch( wMedical, point( getmaxx( wMedical ) - 1, 2 ), BORDER_COLOR, LINE_XOXX ); // -|

        wnoutrefresh( wMedical );

        draw_medical_titlebar( w_title, this, WIDTH );
        mvwputch( w_title, point( second_column_x, HEADER_Y - 1 ), BORDER_COLOR, LINE_OXXX ); // ^|^
        mvwputch( w_title, point( third_column_x, HEADER_Y - 1 ), BORDER_COLOR, LINE_OXXX ); // ^|^
        wnoutrefresh( w_title );

        SCROLL_POINT = HEIGHT - TEXT_START_Y - DESC_W_HEIGHT - 3;

        // Columns

        int column_id = 0;

        // Health Summary
        fold_and_print( wMedical, point( 2, HEADER_Y ), WIDTH - 2, c_light_blue, _( "HEALTH" ) );
        medical_column health_column = draw_health_summary( column_id++, this, point( 0, TEXT_START_Y ),
                                       std::pair<int, int>( second_column_x, HEIGHT ) );

        // Effects Summary
        mvwprintz( wMedical, point( second_column_x + 2, HEADER_Y ), c_light_blue, _( "EFFECTS" ) );
        medical_column effects_column = draw_effects_summary( column_id++, this, point( second_column_x,
                                        TEXT_START_Y ), std::pair<int, int>( third_column_x, HEIGHT ) );

        // Stats Summary
        mvwprintz( wMedical, point( third_column_x + 2, HEADER_Y ), c_light_blue, _( "STATS" ) );
        medical_column stats_column = draw_stats_summary( column_id++, this, point( third_column_x,
                                      TEXT_START_Y ), std::pair<int, int>( WIDTH - 2, 5 ) );

        // Description Text
        std::string desc_str;
        std::pair<std::string, std::string> detail_str;
        switch( cursor.x ) {
            case 0:
                desc_str = health_column.set_highlight( cursor.y ).description();
                detail_str = health_column.detail_str( cursor.y );
                break;
            case 1:
                desc_str = effects_column.set_highlight( cursor.y ).description();
                detail_str = effects_column.detail_str( cursor.y );
                break;
            case 2:
                desc_str = stats_column.set_highlight( cursor.y ).description();
                detail_str = stats_column.detail_str( cursor.y );
                break;
            default:
                break;
        }

        // Description Bar
        werase( w_description );

        int DESCRIPTION_WIN_OFFSET; // Y Position of the start of the description bar (wMedical)

        if( !desc_str.empty() ) {
            // Number of display rows required by the highlighted line.
            std::vector<std::string> textformatted = foldstring( desc_str, WIDTH - 2, ' ' );

            // Beginning row of description text [0-6] (w_description)
            const int DESCRIPTION_TEXT_Y = DESC_W_HEIGHT - std::min( DESC_W_HEIGHT,
                                           static_cast<int>( textformatted.size() ) );

            DESCRIPTION_WIN_OFFSET = DESC_W_BEGIN + DESCRIPTION_TEXT_Y;
            mvwputch( wMedical, point( 0, DESCRIPTION_WIN_OFFSET - 1 ), BORDER_COLOR, LINE_XXXO );
            mvwhline( wMedical, point( 1, DESCRIPTION_WIN_OFFSET - 1 ), LINE_OXOX, getmaxx( wMedical ) - 2 );
            mvwputch( wMedical, point( getmaxx( wMedical ) - 1, DESCRIPTION_WIN_OFFSET - 1 ), BORDER_COLOR,
                      LINE_XOXX );
            fold_and_print( w_description, point( 1, DESCRIPTION_TEXT_Y ), WIDTH - 2, c_light_gray,
                            desc_str );
        } else {
            DESCRIPTION_WIN_OFFSET = getmaxy( wMedical );
        }

        wnoutrefresh( w_description );

        // Info Menu

        INFO_SCROLL_POINT = HEIGHT - INFO_START_Y - DESC_W_HEIGHT - 3;
        const int INFO_CUTOFF = DESCRIPTION_WIN_OFFSET - 1 - ( INFO_START_Y + 3 );

        if( !detail_str.first.empty() ) {
            mvwprintz( wMedical, point( third_column_x + 2, INFO_START_Y + 1 ), c_light_blue,
                       detail_str.first );

            mvwputch( wMedical, point( third_column_x, INFO_START_Y ), BORDER_COLOR, LINE_XXXO ); // |-
            mvwhline( wMedical, point( third_column_x + 1, INFO_START_Y ), LINE_OXOX,
                      getmaxx( wMedical ) - 2 ); // -
            mvwputch( wMedical, point( getmaxx( wMedical ) - 1, INFO_START_Y ), BORDER_COLOR, // -|
                      LINE_XOXX );

            const int info_width = WIDTH - third_column_x - 3;
            std::vector<std::string> textformatted = foldstring( detail_str.second, info_width,
                    ' ' );
            info_lines = textformatted.size();
            int i = 0;
            for( std::string &line : textformatted ) {
                if( i - info_scroll_position >= INFO_CUTOFF ) {
                    break;
                }
                if( i++ >= info_scroll_position ) {
                    trim_and_print( wMedical, point( third_column_x + 2, INFO_START_Y + 2 + i - info_scroll_position ),
                                    info_width, c_light_gray, line );
                }
            }
        } else {
            info_lines = 0;
        }

        // Draw Selection Lines For Each Column

        const int MAX_COLUMN_HEIGHT = DESCRIPTION_WIN_OFFSET - 1;

        health_column.print_column( wMedical,
                                    health_column.current_column( cursor.x ) ? scroll_position : 0, MAX_COLUMN_HEIGHT );
        effects_column.print_column( wMedical,
                                     effects_column.current_column( cursor.x ) ? scroll_position : 0, MAX_COLUMN_HEIGHT );
        stats_column.print_column( wMedical, stats_column.current_column( cursor.x ) ? scroll_position : 0,
                                   MAX_COLUMN_HEIGHT );

        // Update Cursor Boundaries to number of selectable rows

        cursor_bounds[0] = health_column.selection_count();
        cursor_bounds[1] = effects_column.selection_count();
        cursor_bounds[2] = stats_column.selection_count();

        // Draw Column Borders

        effects_column.draw_column( wMedical, HEADER_Y, DESCRIPTION_WIN_OFFSET );
        stats_column.draw_column( wMedical, HEADER_Y, DESCRIPTION_WIN_OFFSET );

        // Draw Scrollbars

        const int content_size = ( cursor_bounds[cursor.x] - 1 ) * 2;

        scrollbar()
        .offset_x( 0 )
        .offset_y( HEADER_Y )
        .content_size( content_size )
        .viewport_pos( cursor.y * 2 )
        .viewport_size( DESC_W_BEGIN - 3 )
        .scroll_to_last( true )
        .apply( wMedical );

        scrollbar()
        .offset_x( third_column_x )
        .offset_y( INFO_START_Y + 1 )
        .content_size( info_lines + INFO_SCROLL_POINT )
        .viewport_pos( info_scroll_position )
        .viewport_size( INFO_SCROLL_POINT + 1 )
        .scroll_to_last( true )
        .apply( wMedical );

        wnoutrefresh( wMedical );
    } );

    input_context ctxt( "MEDICAL" );
    ctxt.register_action( "UP" );
    ctxt.register_action( "DOWN" );
    ctxt.register_action( "LEFT" );
    ctxt.register_action( "RIGHT" );
    ctxt.register_action( "APPLY" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "SCROLL_INFOBOX_UP", to_translation( "Scroll up" ) );
    ctxt.register_action( "SCROLL_INFOBOX_DOWN", to_translation( "Scroll down" ) );
    ctxt.register_action( "QUIT" );

    for( ;; ) {
        ui_manager::redraw();
        const std::string action = ctxt.handle_input();

        if( action == "DOWN" || action == "UP" ) {
            const int step = cursor.y + ( action == "DOWN" ? 1 : -1 );
            const int limit = cursor_bounds[cursor.x] - 1;

            if( step == -1 ) {
                cursor.y = limit;
            } else if( step > limit ) {
                cursor.y = 0;
            } else {
                cursor.y = step;
            }

            const int scroll_overflow = limit - SCROLL_POINT;
            if( scroll_overflow > 0 ) {
                const int half_list = SCROLL_POINT / 2;
                scroll_position = std::max( 0, std::min( scroll_overflow, cursor.y - half_list ) );
            }
            info_scroll_position = 0;
        } else if( action == "RIGHT" || action == "LEFT" ) {
            const int step = cursor.x + ( action == "RIGHT" ? 1 : -1 );
            const int limit = 2;
            if( step == -1 ) {
                cursor.x = limit;
            } else if( step > limit ) {
                cursor.x = 0;
            } else {
                cursor.x = step;
            }

            // Match the cursor to the nearest row on the next column.
            const int y_limit = cursor_bounds[cursor.x] - 1;
            cursor.y = std::min( cursor.y - scroll_position, y_limit );

            const int scroll_overflow = y_limit - SCROLL_POINT;
            if( scroll_overflow > 0 ) {
                const int half_list = SCROLL_POINT / 2;
                scroll_position = std::max( 0, std::min( scroll_overflow, cursor.y - half_list ) );
            } else {
                scroll_position = 0;
            }
            info_scroll_position = 0;
        } else if( action == "APPLY" ) {
            avatar_action::use_item( *this );
        } else if( action == "SCROLL_INFOBOX_UP" || action == "SCROLL_INFOBOX_DOWN" ) {
            const int scroll_overflow = info_lines - INFO_SCROLL_POINT - 1;
            if( scroll_overflow > 0 ) {
                const int step = info_scroll_position + ( action == "SCROLL_INFOBOX_DOWN" ? 1 : -1 );

                if( step == -1 ) {
                    info_scroll_position = scroll_overflow;
                } else if( step > scroll_overflow ) {
                    info_scroll_position = 0;
                } else {
                    info_scroll_position = step;
                }
            } else {
                info_scroll_position = 0;
            }
        } else if( action == "QUIT" ) {
            break;
        }
    }
}
