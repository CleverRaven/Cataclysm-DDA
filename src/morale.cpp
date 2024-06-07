#include "morale.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <numeric>
#include <set>
#include <string>
#include <utility>

#include "bodypart.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "character.h"
#include "color.h"
#include "cursesdef.h"
#include "debug.h"
#include "enums.h"
#include "input_context.h"
#include "item.h"
#include "localized_comparator.h"
#include "make_static.h"
#include "morale_types.h"
#include "output.h"
#include "point.h"
#include "string_formatter.h"
#include "translations.h"
#include "ui_manager.h"

static const efftype_id effect_cold( "cold" );
static const efftype_id effect_hot( "hot" );
static const efftype_id effect_took_prozac( "took_prozac" );
static const efftype_id effect_took_prozac_bad( "took_prozac_bad" );

static const trait_id trait_BADTEMPER( "BADTEMPER" );
static const trait_id trait_CENOBITE( "CENOBITE" );
static const trait_id trait_CHLOROMORPH( "CHLOROMORPH" );
static const trait_id trait_FLOWERS( "FLOWERS" );
static const trait_id trait_LEAVES2( "LEAVES2" );
static const trait_id trait_LEAVES3( "LEAVES3" );
static const trait_id trait_MASOCHIST( "MASOCHIST" );
static const trait_id trait_MASOCHIST_MED( "MASOCHIST_MED" );
static const trait_id trait_NUMB( "NUMB" );
static const trait_id trait_OPTIMISTIC( "OPTIMISTIC" );
static const trait_id trait_RADIOPHILE( "RADIOPHILE" );
static const trait_id trait_ROOTS1( "ROOTS1" );
static const trait_id trait_ROOTS2( "ROOTS2" );
static const trait_id trait_ROOTS3( "ROOTS3" );
static const trait_id trait_STYLISH( "STYLISH" );
static const trait_id trait_VANITY( "VANITY" );

namespace
{

bool is_permanent_morale( const morale_type &id )
{
    static const std::set<morale_type> permanent_morale = {{
            MORALE_PERM_OPTIMIST,
            MORALE_PERM_BADTEMPER,
            MORALE_PERM_NUMB,
            MORALE_PERM_FANCY,
            MORALE_PERM_MASOCHIST,
            MORALE_PERM_CONSTRAINED,
            MORALE_PERM_FILTHY,
            MORALE_PERM_DEBUG,
            MORALE_PERM_RADIOPHILE
        }
    };

    return permanent_morale.count( id ) != 0;
}

} // namespace

// Morale multiplier
struct morale_mult {
    morale_mult(): good( 1.0 ), bad( 1.0 ) {}
    morale_mult( double good, double bad ): good( good ), bad( bad ) {}
    explicit morale_mult( double both ): good( both ), bad( both ) {}

    double good;    // For good morale
    double bad;     // For bad morale

    morale_mult operator * ( const morale_mult &rhs ) const {
        return morale_mult( *this ) *= rhs;
    }

    morale_mult &operator *= ( const morale_mult &rhs ) {
        good *= rhs.good;
        bad *= rhs.bad;
        return *this;
    }
};

static double operator * ( double morale, const morale_mult &mult )
{
    return morale * ( ( morale >= 0.0 ) ? mult.good : mult.bad );
}

static int operator *= ( int &morale, const morale_mult &mult )
{
    morale = morale * mult;
    return morale;
}

// Commonly used morale multipliers
namespace morale_mults
{
// Optimistic characters focus on the good things in life,
// and downplay the bad things.
static const morale_mult optimist( 1.2, 0.8 );
// Again, those grouchy Bad-Tempered folks always focus on the negative.
// They can't handle positive things as well.  They're No Fun.  D:
static const morale_mult badtemper( 0.8, 1.2 );
// Numb characters have trouble feeling anything
static const morale_mult numb( 0.25, 0.25 );
// Prozac reduces overall negative morale by 75%.
static const morale_mult prozac( 1.0, 0.25 );
// The bad prozac effect reduces good morale by 75%.
static const morale_mult prozac_bad( 0.25, 1.0 );
} // namespace morale_mults

std::string player_morale::morale_point::get_name() const
{
    return type.obj().describe( item_type );
}

int player_morale::morale_point::get_net_bonus() const
{
    return bonus * ( ( !is_permanent() && age > decay_start ) ?
                     logarithmic_range( to_turns<int>( decay_start ), to_turns<int>( duration ),
                                        to_turns<int>( age ) ) : 1 );
}

int player_morale::morale_point::get_net_bonus( const morale_mult &mult ) const
{
    return get_net_bonus() * mult;
}

bool player_morale::morale_point::is_expired() const
{
    // Zero morale bonuses will be shown occasionally anyway
    return ( !is_permanent() && age >= duration ) || bonus == 0;
}

bool player_morale::morale_point::is_permanent() const
{
    return ( duration == 0_turns );
}

bool player_morale::morale_point::matches( const morale_type &_type, const itype *_item_type ) const
{
    return ( _type == type ) && ( _item_type == nullptr || _item_type == item_type );
}

bool player_morale::morale_point::matches( const morale_point &mp ) const
{
    return ( type == mp.type ) && ( item_type == mp.item_type );
}

void player_morale::morale_point::add( const int new_bonus, const int new_max_bonus,
                                       time_duration new_duration, time_duration new_decay_start, bool new_cap )
{
    new_duration = std::max( 0_turns, new_duration );
    new_decay_start = std::max( 0_turns, new_decay_start );

    const bool same_sign = ( bonus > 0 ) == ( new_max_bonus > 0 );

    if( new_cap || new_duration == 0_turns ) {
        duration = new_duration;
        decay_start = new_decay_start;
    } else {
        duration = pick_time( duration, new_duration, same_sign );
        decay_start = pick_time( decay_start, new_decay_start, same_sign );
    }

    int sqrt_of_sum_of_squares;
    if( new_cap || !same_sign ) {
        // If the morale bonus is capped apply the full bonus
        // This is because some morale types build up slowly to a cap over time (e.g. MORALE_WET)
        // If the new bonus is opposing apply the full bonus
        sqrt_of_sum_of_squares = get_net_bonus() + new_bonus;
    } else {
        // Otherwise use the sqrt of sum of squares to nerf stacking
        sqrt_of_sum_of_squares = std::pow( get_net_bonus(), 2 ) + std::pow( new_bonus, 2 );
        sqrt_of_sum_of_squares = std::sqrt( sqrt_of_sum_of_squares );
        sqrt_of_sum_of_squares *= sgn( bonus );
    }

    bonus = normalize_bonus( sqrt_of_sum_of_squares, new_max_bonus, new_cap );
    age = 0_turns; // Brand new. The assignment should stay below get_net_bonus() and pick_time().
}

time_duration player_morale::morale_point::pick_time( const time_duration &current_time,
        const time_duration &new_time, bool same_sign ) const
{
    const time_duration remaining_time = current_time - age;
    return ( remaining_time <= new_time && same_sign ) ? new_time : remaining_time;
}

void player_morale::morale_point::set_percent_contribution( double contribution )
{
    percent_contribution = contribution;
}

double player_morale::morale_point::get_percent_contribution() const
{
    return percent_contribution;
}
void player_morale::morale_point::decay( const time_duration &ticks )
{
    if( ticks < 0_turns ) {
        debugmsg( "The function called with negative ticks %d.", to_turns<int>( ticks ) );
        return;
    }

    age += ticks;
}

int player_morale::morale_point::normalize_bonus( int bonus, int max_bonus, bool capped ) const
{
    return ( std::abs( bonus ) > std::abs( max_bonus ) && ( max_bonus != 0 ||
             capped ) ) ? max_bonus : bonus;
}

bool player_morale::mutation_data::get_active() const
{
    return active;
}

void player_morale::mutation_data::set_active( player_morale *sender, bool new_active )
{
    if( active != new_active ) {
        active = new_active;
        if( active ) {
            on_gain( sender );
        } else {
            on_loss( sender );
        }
    }
}

void player_morale::mutation_data::clear()
{
    active = false;
}

player_morale::player_morale() :
    level( 0 ),
    level_is_valid( false ),
    took_prozac( false ),
    took_prozac_bad( false ),
    stylish( false ),
    perceived_pain( 0 ),
    radiation( 0 )
{
    // Cannot use 'this' because the object is copyable
    const auto set_optimist = []( player_morale * pm, int bonus ) {
        pm->set_permanent( MORALE_PERM_OPTIMIST, bonus, nullptr );
    };
    const auto set_badtemper = []( player_morale * pm, int bonus ) {
        pm->set_permanent( MORALE_PERM_BADTEMPER, bonus, nullptr );
    };
    const auto set_numb = []( player_morale * pm, int bonus ) {
        pm->set_permanent( MORALE_PERM_NUMB, bonus, nullptr );
    };
    const auto set_stylish = []( player_morale * pm, bool new_stylish ) {
        pm->set_stylish( new_stylish );
    };
    const auto update_constrained = []( player_morale * pm ) {
        pm->update_constrained_penalty();
    };
    const auto update_masochist = []( player_morale * pm ) {
        pm->update_masochist_bonus();
    };
    const auto update_radiophile = []( player_morale * pm ) {
        pm->update_radiophile_bonus();
    };

    mutations[trait_OPTIMISTIC] =
    mutation_data( [set_optimist]( player_morale * pm ) {
        return set_optimist( pm, 9 );
    },
    [set_optimist]( player_morale * pm ) {
        return set_optimist( pm, 0 );
    } );
    mutations[trait_BADTEMPER] =
    mutation_data( [set_badtemper]( player_morale * pm ) {
        return set_badtemper( pm, -9 );
    },
    [set_badtemper]( player_morale * pm ) {
        return set_badtemper( pm, 0 );
    } );
    mutations[trait_NUMB] =
    mutation_data( [set_numb]( player_morale * pm ) {
        return set_numb( pm, -1 );
    },
    [set_numb]( player_morale * pm ) {
        return set_numb( pm, 0 );
    } );
    mutations[trait_STYLISH] =
    mutation_data( [set_stylish]( player_morale * pm ) {
        return set_stylish( pm, true );
    },
    [set_stylish]( player_morale * pm ) {
        return set_stylish( pm, false );
    } );
    mutations[trait_FLOWERS]       = mutation_data( update_constrained );
    mutations[trait_ROOTS1]        = mutation_data( update_constrained );
    mutations[trait_ROOTS2]        = mutation_data( update_constrained );
    mutations[trait_ROOTS3]        = mutation_data( update_constrained );
    mutations[trait_CHLOROMORPH]   = mutation_data( update_constrained );
    mutations[trait_LEAVES2]       = mutation_data( update_constrained );
    mutations[trait_LEAVES3]       = mutation_data( update_constrained );
    mutations[trait_MASOCHIST]     = mutation_data( update_masochist );
    mutations[trait_MASOCHIST_MED] = mutation_data( update_masochist );
    mutations[trait_CENOBITE]      = mutation_data( update_masochist );
    mutations[trait_RADIOPHILE]    = mutation_data( update_radiophile );
    mutations[trait_VANITY]        = mutation_data( update_constrained );
}

void player_morale::add( const morale_type &type, int bonus, int max_bonus,
                         const time_duration &duration, const time_duration &decay_start,
                         bool capped, const itype *item_type )
{
    if( ( duration == 0_turns ) && !is_permanent_morale( type ) ) {
        debugmsg( "Tried to set a non-permanent morale \"%s\" as permanent.",
                  type.obj().describe( item_type ) );
        return;
    }

    for( player_morale::morale_point &m : points ) {
        if( m.matches( type, item_type ) ) {
            const int prev_bonus = m.get_net_bonus();

            m.add( bonus, max_bonus, duration, decay_start, capped );

            if( m.is_expired() ) {
                remove_expired();
            } else if( m.get_net_bonus() != prev_bonus ) {
                invalidate();
            }

            return;
        }
    }

    morale_point new_morale( type, item_type, bonus, max_bonus, duration, decay_start, capped );

    if( !new_morale.is_expired() ) {
        points.push_back( new_morale );
        invalidate();
    }
}

void player_morale::set_permanent( const morale_type &type, int bonus, const itype *item_type )
{
    add( type, bonus, bonus, 0_turns, 0_turns, true, item_type );
}

int player_morale::has( const morale_type &type, const itype *item_type ) const
{
    for( const morale_point &m : points ) {
        if( m.matches( type, item_type ) ) {
            return m.get_net_bonus();
        }
    }
    return 0;
}

void player_morale::remove_if( const std::function<bool( const morale_point & )> &func )
{
    const auto new_end = std::remove_if( points.begin(), points.end(), func );

    if( new_end != points.end() ) {
        points.erase( new_end, points.end() );
        invalidate();
    }
}

void player_morale::remove( const morale_type &type, const itype *item_type )
{
    remove_if( [ type, item_type ]( const morale_point & m ) -> bool {
        return m.matches( type, item_type );
    } );

}

void player_morale::remove_expired()
{
    remove_if( []( const morale_point & m ) -> bool {
        return m.is_expired();
    } );
}

morale_mult player_morale::get_temper_mult() const
{
    morale_mult mult;

    if( has( MORALE_PERM_OPTIMIST ) ) {
        mult *= morale_mults::optimist;
    }
    if( has( MORALE_PERM_BADTEMPER ) ) {
        mult *= morale_mults::badtemper;
    }
    if( has( MORALE_PERM_NUMB ) ) {
        mult *= morale_mults::numb;
    }

    return mult;
}

void player_morale::calculate_percentage()
{
    const morale_mult mult = get_temper_mult();

    int sum_of_positive_squares = 0;
    int sum_of_negative_squares = 0;

    for( player_morale::morale_point &m : points ) {
        const int bonus = m.get_net_bonus( mult );
        if( bonus > 0 ) {
            sum_of_positive_squares += std::pow( bonus, 2 );
        } else {
            sum_of_negative_squares += std::pow( bonus, 2 );
        }
    }

    for( player_morale::morale_point &m : points ) {
        const int bonus = m.get_net_bonus( mult );
        if( bonus > 0 ) {
            m.set_percent_contribution( ( std::pow( bonus, 2 ) / sum_of_positive_squares ) * 100 );
        } else {
            m.set_percent_contribution( ( std::pow( bonus, 2 ) / sum_of_negative_squares ) * 100 );
        }
    }
}

int player_morale::get_total_negative_value() const
{
    const morale_mult mult = get_temper_mult();
    int sum = 0;
    for( const morale_point &m : points ) {
        const int bonus = m.get_net_bonus( mult );
        if( bonus < 0 ) {
            sum += std::pow( bonus, 2 );
        }
    }
    return std::sqrt( sum );
}

int player_morale::get_perceived_pain() const
{
    return perceived_pain;
}

int player_morale::get_total_positive_value() const
{
    const morale_mult mult = get_temper_mult();
    int sum = 0;
    for( const morale_point &m : points ) {
        const int bonus = m.get_net_bonus( mult );
        if( bonus > 0 ) {
            sum += std::pow( bonus, 2 );
        }

    }
    return std::sqrt( sum );
}

int player_morale::get_level() const
{
    if( !level_is_valid ) {
        const morale_mult mult = get_temper_mult();

        int sum_of_positive_squares = 0;
        int sum_of_negative_squares = 0;

        for( const morale_point &m : points ) {
            const int bonus = m.get_net_bonus( mult );
            if( bonus > 0 ) {
                sum_of_positive_squares += std::pow( bonus, 2 );
            } else {
                sum_of_negative_squares += std::pow( bonus, 2 );
            }
        }

        level = std::sqrt( sum_of_positive_squares ) - std::sqrt( sum_of_negative_squares );

        if( took_prozac ) {
            level *= morale_mults::prozac;
            if( took_prozac_bad ) {
                level *= morale_mults::prozac_bad;
            }
        }

        level_is_valid = true;
    }

    return level;
}

void player_morale::decay( const time_duration &ticks )
{
    const auto do_decay = [ &ticks ]( morale_point & m ) {
        m.decay( ticks );
    };

    std::for_each( points.begin(), points.end(), do_decay );
    remove_expired();
    update_bodytemp_penalty( ticks );
    invalidate();
}

void player_morale::display( int focus_eq, int pain_penalty, int sleepiness_penalty )
{
    /*calculates the percent contributions of the morale points,
     * must be done before anything else in this method
     */
    calculate_percentage();

    constexpr int left_padding = 2; // including border
    constexpr int right_padding = 2; // including border
    constexpr int middle_padding_min = 2;
    constexpr int middle_padding_max = 12;

    class morale_line
    {
        public:
            enum class number_format : int {
                normal,
                signed_or_dash,
                percent,
            };

            enum class line_color : int {
                normal,
                green_gray_red,
                red_gray_green,
            };

            struct separation_line {};

        private:
            bool sep_line = false;
            std::string left;
            std::string right;
            nc_color color;

        public:
            morale_line() = default;
            explicit morale_line( const separation_line ) : sep_line( true ) {}
            morale_line( const std::string &text, const nc_color &color )
                : left( text ), color( color ) {}
            morale_line( const std::string &left, const std::string &right,
                         const nc_color &color )
                : left( left ), right( right ), color( color ) {}
            morale_line( const std::string &text, const int num,
                         const number_format num_fmt, const line_color col )
                : left( text ) {
                switch( num_fmt ) {
                    case number_format::normal:
                        right = string_format( "%d", num );
                        break;
                    case number_format::signed_or_dash:
                        if( num == 0 ) {
                            right = "-";
                        } else {
                            right = string_format( "%+d", num );
                        }
                        break;
                    case number_format::percent:
                        right = string_format( "%d%%", num );
                        break;
                }
                switch( col ) {
                    case line_color::normal:
                        color = c_white;
                        break;
                    case line_color::green_gray_red:
                    case line_color::red_gray_green:
                        if( num == 0 ) {
                            color = c_dark_gray;
                        } else if( ( num > 0 ) ^ ( col == line_color::red_gray_green ) ) {
                            color = c_green;
                        } else {
                            color = c_light_red;
                        }
                        break;
                }
            }

            int max_width() const {
                if( sep_line ) {
                    return 0;
                } else if( right.empty() ) {
                    return left_padding + utf8_width( left ) + right_padding;
                } else {
                    return left_padding + utf8_width( left ) + middle_padding_max
                           + utf8_width( right ) + right_padding;
                }
            }

            void draw( catacurses::window &w, const int posy ) const {
                int width = getmaxx( w );
                if( sep_line ) {
                    mvwhline( w, point( 0, posy ), LINE_XXXO, 1 );
                    mvwhline( w, point( 1, posy ), 0, width - 2 );
                    mvwhline( w, point( width - 1, posy ), LINE_XOXX, 1 );
                } else {
                    int text_width = width - left_padding - right_padding;
                    if( !right.empty() ) {
                        right_print( w, posy, right_padding, color, right );
                        text_width -= middle_padding_min + utf8_width( right );
                    }
                    trim_and_print( w, point( left_padding, posy ), text_width, color, left );
                }
            }
    };

    // Separate and sort positive and negative morale
    const morale_mult mult = get_temper_mult();
    std::vector<morale_point> positive_morale;
    std::vector<morale_point> negative_morale;
    for( const morale_point &mp : points ) {
        const int bonus = mp.get_net_bonus( mult );
        if( bonus > 0 ) {
            positive_morale.emplace_back( mp );
        } else if( bonus < 0 ) {
            negative_morale.emplace_back( mp );
        }
    }

    const auto sort_morale = []( const morale_point & lhs, const morale_point & rhs ) -> bool {
        const int lhs_percent = lhs.get_percent_contribution();
        const int rhs_percent = rhs.get_percent_contribution();
        return localized_compare( std::make_pair( -lhs_percent, lhs.get_name() ),
                                  std::make_pair( -rhs_percent, rhs.get_name() ) );
    };
    std::sort( positive_morale.begin(), positive_morale.end(), sort_morale );
    std::sort( negative_morale.begin(), negative_morale.end(), sort_morale );

    // Initialize lines
    const std::vector<morale_line> top_lines {
        {},
        { _( "Morale" ), c_white },
        morale_line{ morale_line::separation_line {} },

        positive_morale.empty() &&negative_morale.empty() ?
        morale_line( _( "Nothing affects your morale" ), c_dark_gray ) :
        morale_line( _( "Source" ), _( "Value" ), c_light_gray ),
    };

    struct middle_morale_line {
        bool is_caption;
        morale_line ml;

        middle_morale_line( const bool is_caption, const morale_line &ml )
            : is_caption( is_caption ), ml( ml ) {}
    };

    std::vector<middle_morale_line> middle_lines;
    if( !positive_morale.empty() || !negative_morale.empty() ) {
        middle_lines.emplace_back( true, morale_line(
                                       _( "Total positive morale" ), get_total_positive_value(),
                                       morale_line::number_format::signed_or_dash,
                                       morale_line::line_color::green_gray_red
                                   ) );
        for( const morale_point &mp : positive_morale ) {
            middle_lines.emplace_back( false, morale_line(
                                           mp.get_name(), mp.get_percent_contribution(),
                                           morale_line::number_format::percent,
                                           morale_line::line_color::green_gray_red
                                       ) );
        }
        middle_lines.emplace_back( false, morale_line() );
        middle_lines.emplace_back( true, morale_line(
                                       _( "Total negative morale" ), -get_total_negative_value(),
                                       morale_line::number_format::signed_or_dash,
                                       morale_line::line_color::green_gray_red
                                   ) );
        for( const morale_point &mp : negative_morale ) {
            middle_lines.emplace_back( false, morale_line(
                                           mp.get_name(), mp.get_percent_contribution(),
                                           morale_line::number_format::percent,
                                           morale_line::line_color::red_gray_green
                                       ) );
        }
    }

    std::vector<morale_line> bottom_lines;
    bottom_lines.emplace_back( morale_line::separation_line {} );
    bottom_lines.emplace_back(
        _( "Total morale:" ), get_level(),
        morale_line::number_format::signed_or_dash,
        morale_line::line_color::green_gray_red
    );
    if( pain_penalty != 0 ) {
        bottom_lines.emplace_back(
            _( "Pain level:" ), -pain_penalty,
            morale_line::number_format::signed_or_dash,
            morale_line::line_color::green_gray_red
        );
    }
    if( sleepiness_penalty != 0 ) {
        bottom_lines.emplace_back(
            _( "Sleepiness level:" ), -sleepiness_penalty,
            morale_line::number_format::signed_or_dash,
            morale_line::line_color::green_gray_red
        );
    }
    bottom_lines.emplace_back(
        _( "Focus trends towards:" ), focus_eq,
        morale_line::number_format::normal,
        morale_line::line_color::normal
    );
    bottom_lines.emplace_back();

    const auto calc_max_width = []( const int width, const morale_line & ml ) -> int {
        return std::max( width, ml.max_width() );
    };
    const int max_window_width = std::max( {
        std::accumulate( top_lines.begin(), top_lines.end(), 0, calc_max_width ),
        std::accumulate( middle_lines.begin(), middle_lines.end(), 0,
        []( const int width, const middle_morale_line & ml ) -> int {
            return std::max( width, ml.ml.max_width() );
        } ),
        std::accumulate( bottom_lines.begin(), bottom_lines.end(), 0, calc_max_width )
    } );

    const int static_lines_height = top_lines.size() + bottom_lines.size();

    int win_w = 0;
    int win_h = 0;

    const int rows_total = middle_lines.size();
    int rows_visible = 0;
    int offset = 0;

    catacurses::window w;

    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        win_w = std::min( max_window_width, FULL_SCREEN_WIDTH );
        win_h = FULL_SCREEN_HEIGHT;
        const point win( ( TERMX - win_w ) / 2, ( TERMY - win_h ) / 2 );

        rows_visible = std::max( win_h - static_lines_height, 0 );
        if( rows_total < rows_visible ) {
            offset = 0;
        } else if( offset + rows_visible > rows_total ) {
            offset = rows_total - rows_visible;
        }

        w = catacurses::newwin( win_h, win_w, win );

        ui.position_from_window( w );
    } );
    ui.mark_resize();

    ui.on_redraw( [&]( const ui_adaptor & ) {
        werase( w );

        draw_border( w );

        int line = 0;
        for( const morale_line &ml : top_lines ) {
            ml.draw( w, line );
            ++line;
        }

        line = win_h - static_cast<int>( bottom_lines.size() );
        for( const morale_line &ml : bottom_lines ) {
            ml.draw( w, line );
            ++line;
        }

        bool caption_drawn = false;
        for( int mid_line = 0; offset + mid_line >= 0; --mid_line ) {
            if( offset + mid_line < rows_total ) {
                const middle_morale_line &ml = middle_lines[offset + mid_line];
                if( ml.is_caption ) {
                    ml.ml.draw( w, top_lines.size() );
                    caption_drawn = true;
                    break;
                }
            }
        }
        for( int mid_line = caption_drawn ? 1 : 0;
             mid_line < rows_visible && offset + mid_line < rows_total;
             ++mid_line ) {
            if( offset + mid_line >= 0 ) {
                const middle_morale_line &ml = middle_lines[offset + mid_line];
                ml.ml.draw( w, static_cast<int>( top_lines.size() ) + mid_line );
            }
        }

        draw_scrollbar( w, offset, rows_visible, rows_total,
                        point( 0, top_lines.size() ), c_white, true );

        wnoutrefresh( w );
    } );

    input_context ctxt( "MORALE" );
    ctxt.register_action( "UP" );
    ctxt.register_action( "DOWN" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    std::string action;
    do {
        ui_manager::redraw();
        action = ctxt.handle_input();
        if( action == "DOWN" && offset < std::max( 0, rows_total - rows_visible ) ) {
            offset++;
        } else if( action == "UP" && offset > 0 ) {
            offset--;
        }
    } while( action != "CONFIRM" && action != "QUIT" );
}

bool player_morale::consistent_with( const player_morale &morale ) const
{
    const auto test_points = []( const player_morale & lhs, const player_morale & rhs ) {
        for( const player_morale::morale_point &lhp : lhs.points ) {
            if( !lhp.is_permanent() ) {
                continue;
            }

            const auto iter = std::find_if( rhs.points.begin(), rhs.points.end(),
            [ &lhp ]( const morale_point & rhp ) {
                return lhp.matches( rhp );
            } );

            if( iter == rhs.points.end() || lhp.get_net_bonus() != iter->get_net_bonus() ) {
                debugmsg( "Morale \"%s\" is inconsistent.", lhp.get_name() );
                return false;
            }
        }

        return true;
    };

    if( took_prozac != morale.took_prozac ) {
        debugmsg( "player_morale::took_prozac is inconsistent." );
        return false;
    } else if( took_prozac_bad != morale.took_prozac_bad ) {
        debugmsg( "player_morale::took_prozac (bad) is inconsistent." );
        return false;
    } else if( stylish != morale.stylish ) {
        debugmsg( "player_morale::stylish is inconsistent." );
        return false;
    } else if( perceived_pain != morale.perceived_pain ) {
        debugmsg( "player_morale::perceived_pain is inconsistent." );
        return false;
    } else if( radiation != morale.radiation ) {
        debugmsg( "player_morale::radiation is inconsistent." );
        return false;
    }

    return test_points( *this, morale ) && test_points( morale, *this );
}

void player_morale::clear()
{
    points.clear();
    no_body_part = body_part_data();
    body_parts.clear();
    for( auto &m : mutations ) {
        m.second.clear();
    }
    took_prozac = false;
    took_prozac_bad = false;
    stylish = false;
    super_fancy_items.clear();

    invalidate();
}

void player_morale::invalidate()
{
    level_is_valid = false;
}

bool player_morale::has_mutation( const trait_id &mid )
{
    const auto &mutation = mutations.find( mid );
    return ( mutation != mutations.end() && mutation->second.get_active() );
}

bool player_morale::has_flag( const json_character_flag &flag )
{
    return get_player_character().has_flag( flag );
}

void player_morale::set_mutation( const trait_id &mid, bool active )
{
    const auto &mutation = mutations.find( mid );
    if( mutation != mutations.end() ) {
        mutation->second.set_active( this, active );
    }
}

void player_morale::on_mutation_gain( const trait_id &mid )
{
    set_mutation( mid, true );
}

void player_morale::on_mutation_loss( const trait_id &mid )
{
    set_mutation( mid, false );
}

void player_morale::on_stat_change( const std::string &stat, int value )
{
    if( stat == "perceived_pain" ) {
        perceived_pain = value;
        update_masochist_bonus();
    }
    if( stat == "radiation" ) {
        radiation = value;
        update_radiophile_bonus();
    }
}

void player_morale::on_item_wear( const item &it )
{
    set_worn( it, true );
}

void player_morale::on_item_takeoff( const item &it )
{
    set_worn( it, false );
}

void player_morale::on_worn_item_transform( const item &old_it, const item &new_it )
{
    set_worn( old_it, false );
    set_worn( new_it, true );
}

void player_morale::on_worn_item_washed( const item &it )
{
    const auto update_body_part = [&]( body_part_data & bp_data ) {
        bp_data.filthy -= 1;
    };

    const body_part_set covered( it.get_covered_body_parts() );

    if( covered.any() ) {
        for( const bodypart_id &bp : get_player_character().get_all_body_parts() ) {
            if( covered.test( bp.id() ) ) {
                update_body_part( body_parts[bp] );
            }
        }
    } else {
        update_body_part( no_body_part );
    }

    update_squeamish_penalty();
}

void player_morale::on_effect_int_change( const efftype_id &eid, int intensity,
        const bodypart_id &bp )
{
    const bodypart_id bp_null( "bp_null" );
    if( eid == effect_took_prozac && bp == bp_null ) {
        set_prozac( intensity != 0 );
    } else if( eid == effect_took_prozac_bad && bp == bp_null ) {
        set_prozac_bad( intensity != 0 );
    } else if( eid == effect_cold && bp != bp_null ) {
        body_parts[bp].cold = intensity;
    } else if( eid == effect_hot && bp != bp_null ) {
        body_parts[bp].hot = intensity;
    }
}

void player_morale::set_worn( const item &it, bool worn )
{
    const bool fancy = it.has_flag( STATIC( flag_id( "FANCY" ) ) );
    const bool super_fancy = it.has_flag( STATIC( flag_id( "SUPER_FANCY" ) ) );
    const bool filthy_gear = it.has_flag( STATIC( flag_id( "FILTHY" ) ) );
    const bool integrated = it.has_flag( STATIC( flag_id( "INTEGRATED" ) ) );
    const int sign = worn ? 1 : -1;

    const auto update_body_part = [&]( body_part_data & bp_data ) {
        if( fancy || super_fancy ) {
            bp_data.fancy += sign;
        }
        if( filthy_gear ) {
            bp_data.filthy += sign;
        }
        // If armor is integrated (Subdermal CBM, Skin armor mutation) don't count it as covering
        if( !integrated ) {
            bp_data.covered += sign;
        }
    };

    const body_part_set covered( it.get_covered_body_parts() );

    if( covered.any() ) {
        for( const bodypart_id &bp : get_player_character().get_all_body_parts() ) {
            if( covered.test( bp.id() ) ) {
                update_body_part( body_parts[bp] );
            }
        }
    } else {
        update_body_part( no_body_part );
    }

    if( super_fancy ) {
        const itype_id id = it.typeId();
        const auto iter = super_fancy_items.find( id );

        if( iter != super_fancy_items.end() ) {
            iter->second += sign;
            if( iter->second == 0 ) {
                super_fancy_items.erase( iter );
            }
        } else if( worn ) {
            super_fancy_items[id] = 1;
        } else {
            debugmsg( "Tried to take off \"%s\" which isn't worn.", id.c_str() );
        }
    }
    if( fancy || super_fancy ) {
        update_stylish_bonus();
    }
    if( filthy_gear ) {
        update_squeamish_penalty();
    }
    update_constrained_penalty();
}

void player_morale::set_prozac( bool new_took_prozac )
{
    if( took_prozac != new_took_prozac ) {
        took_prozac = new_took_prozac;
        update_masochist_bonus();
        invalidate();
    }
}

void player_morale::set_prozac_bad( bool new_took_prozac_bad )
{
    if( took_prozac_bad != new_took_prozac_bad ) {
        took_prozac_bad = new_took_prozac_bad;
        invalidate();
    }
}

void player_morale::set_stylish( bool new_stylish )
{
    if( stylish != new_stylish ) {
        stylish = new_stylish;
        update_stylish_bonus();
    }
}

void player_morale::update_stylish_bonus()
{
    int bonus = 0;

    if( stylish ) {
        float tmp_bonus = 0.0f;
        for( const std::pair<const bodypart_id, body_part_data> &bpt : body_parts ) {
            if( bpt.second.fancy > 0 ) {
                tmp_bonus += bpt.first->stylish_bonus;
            }
        }
        bonus = std::min( static_cast<int>( 2 * super_fancy_items.size() ) +
                          2 * std::min( static_cast<int>( no_body_part.fancy ), 3 ) + static_cast<int>( tmp_bonus ), 20 );
    }
    set_permanent( MORALE_PERM_FANCY, bonus );
}

void player_morale::update_masochist_bonus()
{
    const bool amateur_masochist = has_mutation( trait_MASOCHIST );
    const bool advanced_masochist = has_mutation( trait_MASOCHIST_MED ) ||
                                    has_mutation( trait_CENOBITE );
    const bool any_masochist = amateur_masochist || advanced_masochist;

    int bonus = 0;

    if( any_masochist ) {
        bonus = perceived_pain;
        if( amateur_masochist ) {
            bonus = std::min( bonus, 20 );
        }
        if( took_prozac ) {
            bonus = bonus / 2;
        }
    }
    set_permanent( MORALE_PERM_MASOCHIST, bonus );
}

void player_morale::update_radiophile_bonus()
{
    const bool is_radiophile = has_mutation( trait_RADIOPHILE );

    int bonus = 0;

    if( is_radiophile ) {
        bonus = radiation / 20;
    }
    set_permanent( MORALE_PERM_RADIOPHILE, bonus );
}

void player_morale::update_bodytemp_penalty( const time_duration &ticks )
{
    float max_cold_penalty = 0.0f;
    float max_hot_penalty = 0.0f;
    for( const std::pair<const bodypart_id, body_part_data> &bpt : body_parts ) {
        const bodypart_id bp = bpt.first;
        max_cold_penalty += body_parts[bp].cold * bp->cold_morale_mod;
        max_hot_penalty += body_parts[bp].hot * bp->hot_morale_mod;
    }
    if( max_cold_penalty != 0.0f ) {
        add( MORALE_COLD, -2 * to_turns<int>( ticks ), -std::abs( max_cold_penalty ), 1_minutes, 30_seconds,
             true );
    }
    if( max_hot_penalty != 0 && !has_flag( STATIC( json_character_flag( "HEAT_IMMUNE" ) ) ) ) {
        add( MORALE_HOT, -2 * to_turns<int>( ticks ), -std::abs( max_hot_penalty ), 1_minutes, 30_seconds,
             true );
    }
}

void player_morale::update_constrained_penalty()
{
    const auto bp_pen = [ this ]( bodypart_id bp, int pen ) -> int {
        return ( body_parts[bp].covered > 0 ) ? pen : 0;
    };
    int pen = 0;

    if( has_mutation( trait_VANITY ) ) {
        pen += bp_pen( bodypart_id( "mouth" ), 5 );
        pen += bp_pen( bodypart_id( "eyes" ), 5 );
    }
    if( has_mutation( trait_FLOWERS ) ) {
        pen += bp_pen( bodypart_id( "head" ), 10 );
    }
    if( has_mutation( trait_ROOTS1 ) || has_mutation( trait_ROOTS2 ) ||
        has_mutation( trait_ROOTS3 ) || has_mutation( trait_CHLOROMORPH ) ) {
        pen += bp_pen( bodypart_id( "foot_l" ), 5 );
        pen += bp_pen( bodypart_id( "foot_r" ), 5 );
    }
    if( has_mutation( trait_LEAVES2 ) || has_mutation( trait_LEAVES3 ) ) {
        pen += bp_pen( bodypart_id( "arm_l" ), 5 );
        pen += bp_pen( bodypart_id( "arm_r" ), 5 );
    }
    set_permanent( MORALE_PERM_CONSTRAINED, -std::min( pen, 10 ) );
}

void player_morale::update_squeamish_penalty()
{
    int penalty = 0;
    for( const std::pair<const bodypart_id, body_part_data> &bpt : body_parts ) {
        if( bpt.second.filthy > 0 ) {
            penalty += bpt.first->squeamish_penalty;
        }
    }
    penalty += 2 * std::min( static_cast<int>( no_body_part.filthy ), 3 );
    set_permanent( MORALE_PERM_FILTHY, -penalty );
}
