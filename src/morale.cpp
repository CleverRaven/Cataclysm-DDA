#include "morale.h"
#include "morale_types.h"

#include "cata_utility.h"
#include "debug.h"
#include "item.h"
#include "itype.h"
#include "output.h"
#include "options.h"
#include "bodypart.h"
#include "translations.h"
#include "catacharset.h"
#include "game.h"
#include "weather.h"
#include "input.h"

#include <algorithm>
#include <set>

static const efftype_id effect_cold( "cold" );
static const efftype_id effect_hot( "hot" );
static const efftype_id effect_took_prozac( "took_prozac" );

namespace
{
static const std::string item_name_placeholder = "%s"; // Used to address an item name

bool is_permanent_morale( const morale_type id )
{
    static const std::set<morale_type> permanent_morale = {{
            MORALE_PERM_OPTIMIST,
            MORALE_PERM_BADTEMPER,
            MORALE_PERM_FANCY,
            MORALE_PERM_MASOCHIST,
            MORALE_PERM_CONSTRAINED,
            MORALE_PERM_FILTHY
        }
    };

    return permanent_morale.count( id ) != 0;
}

} // namespace

// Morale multiplier
struct morale_mult {
    morale_mult(): good( 1.0 ), bad( 1.0 ) {}
    morale_mult( double good, double bad ): good( good ), bad( bad ) {}
    morale_mult( double both ): good( both ), bad( both ) {}

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

inline double operator * ( double morale, const morale_mult &mult )
{
    return morale * ( ( morale >= 0.0 ) ? mult.good : mult.bad );
}

inline double operator * ( const morale_mult &mult, double morale )
{
    return morale * mult;
}

inline double operator *= ( double &morale, const morale_mult &mult )
{
    morale = morale * mult;
    return morale;
}

inline int operator *= ( int &morale, const morale_mult &mult )
{
    morale = morale * mult;
    return morale;
}

// Commonly used morale multipliers
namespace morale_mults
{
// Optimistic characters focus on the good things in life,
// and downplay the bad things.
static const morale_mult optimist( 1.25, 0.75 );
// Again, those grouchy Bad-Tempered folks always focus on the negative.
// They can't handle positive things as well.  They're No Fun.  D:
static const morale_mult badtemper( 0.75, 1.25 );
// Prozac reduces overall negative morale by 75%.
static const morale_mult prozac( 1.0, 0.25 );
}

std::string player_morale::morale_point::get_name() const
{
    return type.obj().describe( item_type );
}

int player_morale::morale_point::get_net_bonus() const
{
    return bonus * ( ( !is_permanent() && age > decay_start ) ?
                     logarithmic_range( decay_start, duration, age ) : 1 );
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
    return ( duration == 0 );
}

bool player_morale::morale_point::matches( morale_type _type, const itype *_item_type ) const
{
    return ( _type == type ) && ( _item_type == nullptr || _item_type == item_type );
}

bool player_morale::morale_point::matches( const morale_point &mp ) const
{
    return ( type == mp.type ) && ( item_type == mp.item_type );
}

void player_morale::morale_point::add( int new_bonus, int new_max_bonus, int new_duration,
                                       int new_decay_start,
                                       bool new_cap )
{
    new_duration = std::max( 0, new_duration );
    new_decay_start = std::max( 0, new_decay_start );

    if( new_cap || new_duration == 0 ) {
        duration = new_duration;
        decay_start = new_decay_start;
    } else {
        bool same_sign = ( bonus > 0 ) == ( new_max_bonus > 0 );

        duration = pick_time( duration, new_duration, same_sign );
        decay_start = pick_time( decay_start, new_decay_start, same_sign );
    }

    bonus = normalize_bonus( get_net_bonus() + new_bonus, new_max_bonus, new_cap );
    age = 0; // Brand new. The assignment should stay below get_net_bonus() and pick_time().
}

int player_morale::morale_point::pick_time( int current_time, int new_time, bool same_sign ) const
{
    const int remaining_time = current_time - age;
    return ( remaining_time <= new_time && same_sign ) ? new_time : remaining_time;
}

void player_morale::morale_point::decay( int ticks )
{
    if( ticks < 0 ) {
        debugmsg( "The function called with negative ticks %d.", ticks );
        return;
    }

    age += ticks;
}

int player_morale::morale_point::normalize_bonus( int bonus, int max_bonus, bool capped ) const
{
    return ( ( abs( bonus ) > abs( max_bonus ) && ( max_bonus != 0 || capped ) ) ? max_bonus : bonus );
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
    stylish( false ),
    perceived_pain( 0 )
{
    using namespace std::placeholders;
    // Cannot use 'this' because the object is copyable
    const auto set_optimist       = std::bind( &player_morale::set_permanent, _1, MORALE_PERM_OPTIMIST,
                                    _2, nullptr );
    const auto set_badtemper      = std::bind( &player_morale::set_permanent, _1, MORALE_PERM_BADTEMPER,
                                    _2, nullptr );
    const auto set_stylish        = std::bind( &player_morale::set_stylish, _1, _2 );
    const auto update_constrained = std::bind( &player_morale::update_constrained_penalty, _1 );
    const auto update_masochist   = std::bind( &player_morale::update_masochist_bonus, _1 );

    mutations[trait_id( "OPTIMISTIC" )]    = mutation_data(
                                     std::bind( set_optimist, _1, 4 ),
                                     std::bind( set_optimist, _1, 0 ) );
    mutations[trait_id( "BADTEMPER" )]     = mutation_data(
                                     std::bind( set_badtemper, _1, -4 ),
                                     std::bind( set_badtemper, _1, 0 ) );
    mutations[trait_id( "STYLISH" )]       = mutation_data(
                                     std::bind( set_stylish, _1, true ),
                                     std::bind( set_stylish, _1, false ) );
    mutations[trait_id( "FLOWERS" )]       = mutation_data( update_constrained );
    mutations[trait_id( "ROOTS1" )]         = mutation_data( update_constrained );
    mutations[trait_id( "ROOTS2" )]        = mutation_data( update_constrained );
    mutations[trait_id( "ROOTS3" )]        = mutation_data( update_constrained );
    mutations[trait_id( "MASOCHIST" )]     = mutation_data( update_masochist );
    mutations[trait_id( "MASOCHIST_MED" )] = mutation_data( update_masochist );
    mutations[trait_id( "CENOBITE" )]      = mutation_data( update_masochist );
}

void player_morale::add( morale_type type, int bonus, int max_bonus,
                         int duration, int decay_start,
                         bool capped, const itype *item_type )
{
    if( ( duration == 0 ) & !is_permanent_morale( type ) ) {
        debugmsg( "Tried to set a non-permanent morale \"%s\" as permanent.",
                  type.obj().describe( item_type ).c_str() );
        return;
    }

    for( auto &m : points ) {
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

void player_morale::set_permanent( morale_type type, int bonus, const itype *item_type )
{
    add( type, bonus, bonus, 0, 0, true, item_type );
}

int player_morale::has( morale_type type, const itype *item_type ) const
{
    for( auto &m : points ) {
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

void player_morale::remove( morale_type type, const itype *item_type )
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

    return mult;
}

int player_morale::get_level() const
{
    if( !level_is_valid ) {
        const morale_mult mult = get_temper_mult();

        level = 0;
        for( auto &m : points ) {
            level += m.get_net_bonus( mult );
        }

        if( took_prozac ) {
            level *= morale_mults::prozac;
        }

        level_is_valid = true;
    }

    return level;
}

void player_morale::decay( int ticks )
{
    const auto do_decay = [ ticks ]( morale_point & m ) {
        m.decay( ticks );
    };

    std::for_each( points.begin(), points.end(), do_decay );
    remove_expired();
    update_bodytemp_penalty( ticks );
    invalidate();
}

void player_morale::display( double focus_gain )
{
    const char *morale_gain_caption = _( "Total morale gain" );
    const char *focus_gain_caption = _( "Focus gain per minute" );

    // Figure out how wide the source column needs to be.
    int source_column_width = std::max( utf8_width( morale_gain_caption ),
                                        utf8_width( focus_gain_caption ) );
    for( auto &i : points ) {
        source_column_width = std::max( utf8_width( i.get_name() ), source_column_width );
    }

    const int win_w = std::min( source_column_width + 4 + 8, FULL_SCREEN_WIDTH );
    const int win_h = FULL_SCREEN_HEIGHT;
    const int win_x = ( TERMX - win_w ) / 2;
    const int win_y = ( TERMY - win_h ) / 2;

    WINDOW *w = newwin( win_h, win_w, win_y, win_x );

    draw_border( w );

    mvwprintz( w, 1, 2, c_white, _( "Morale" ) );

    mvwhline( w, 2, 0, LINE_XXXO, 1 );
    mvwhline( w, 2, 1, 0, win_w - 2 );
    mvwhline( w, 2, win_w - 1, LINE_XOXX, 1 );

    mvwhline( w, win_h - 4, 0, LINE_XXXO, 1 );
    mvwhline( w, win_h - 4, 1, 0, win_w - 2 );
    mvwhline( w, win_h - 4, win_w - 1, LINE_XOXX, 1 );

    const auto print_line = [ w ]( int y, const char *label, double value ) -> int {
        nc_color color;
        if( value != 0.0 )
        {
            const int decimals = ( value - static_cast<int>( value ) != 0.0 ) ? 2 : 0;
            color = ( value > 0.0 ) ? c_green : c_red;
            mvwprintz( w, y, getmaxx( w ) - 8, color, "%+6.*f", decimals, value );
        } else {
            color = c_dkgray;
            mvwprintz( w, y, getmaxx( w ) - 3, color, "-" );
        }
        return fold_and_print_from( w, y, 2, getmaxx( w ) - 9, 0, color, label );
    };

    if( !points.empty() ) {
        const char *source_column = _( "Source" );
        const char *value_column = _( "Value" );

        mvwprintz( w, 3, 2, c_ltgray, source_column );
        mvwprintz( w, 3, win_w - utf8_width( value_column ) - 2, c_ltgray, value_column );

        const morale_mult mult = get_temper_mult();

        int line = 0;
        for( size_t i = 0; i < points.size(); ++i ) {
            const std::string name = points[i].get_name();
            const int bonus = points[i].get_net_bonus( mult );

            line += print_line( 4 + line, name.c_str(), bonus );
            if( line >= win_h - 8 ) {
                break;  // This prevents overflowing (unlikely, but just in case)
            }
        }
    } else {
        fold_and_print_from( w, 3, 2, win_w - 4, 0, c_dkgray, _( "Nothing affects your morale" ) );
    }

    print_line( win_h - 3, morale_gain_caption, get_level() );
    print_line( win_h - 2, focus_gain_caption, focus_gain );

    wrefresh( w );

    inp_mngr.wait_for_any_key();

    werase( w );
    delwin( w );
}

bool player_morale::consistent_with( const player_morale &morale ) const
{
    const auto test_points = []( const player_morale & lhs, const player_morale & rhs ) {
        for( const auto &lhp : lhs.points ) {
            if( !lhp.is_permanent() ) {
                continue;
            }

            const auto iter = std::find_if( rhs.points.begin(), rhs.points.end(),
            [ &lhp ]( const morale_point & rhp ) {
                return lhp.matches( rhp );
            } );

            if( iter == rhs.points.end() || lhp.get_net_bonus() != iter->get_net_bonus() ) {
                debugmsg( "Morale \"%s\" is inconsistent.", lhp.get_name().c_str() );
                return false;
            }
        }

        return true;
    };

    if( took_prozac != morale.took_prozac ) {
        debugmsg( "player_morale::took_prozac is inconsistent." );
        return false;
    } else if( stylish != morale.stylish ) {
        debugmsg( "player_morale::stylish is inconsistent." );
        return false;
    } else if( perceived_pain != morale.perceived_pain ) {
        debugmsg( "player_morale::perceived_pain is inconsistent." );
        return false;
    }

    return test_points( *this, morale ) && test_points( morale, *this );
}

void player_morale::clear()
{
    points.clear();
    no_body_part = body_part_data();
    for( int i = 0; i < num_bp; ++i ) {
        body_parts[i] = body_part_data();
    }
    for( auto &m : mutations ) {
        m.second.clear();
    }
    took_prozac = false;
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
}

void player_morale::on_item_wear( const item &it )
{
    set_worn( it, true );
}

void player_morale::on_item_takeoff( const item &it )
{
    set_worn( it, false );
}

void player_morale::on_effect_int_change( const efftype_id &eid, int intensity, body_part bp )
{
    if( eid == effect_took_prozac && bp == num_bp ) {
        set_prozac( intensity != 0 );
    } else if( eid == effect_cold && bp < num_bp ) {
        body_parts[bp].cold = intensity;
    } else if( eid == effect_hot && bp < num_bp ) {
        body_parts[bp].hot = intensity;
    }
}

void player_morale::set_worn( const item &it, bool worn )
{
    const bool fancy = it.has_flag( "FANCY" );
    const bool super_fancy = it.has_flag( "SUPER_FANCY" );
    const bool filthy_gear = it.has_flag( "FILTHY" );
    const int sign = ( worn ) ? 1 : -1;

    const auto update_body_part = [&]( body_part_data & bp_data ) {
        if( fancy || super_fancy ) {
            bp_data.fancy += sign;
        }
        if( filthy_gear ) {
            bp_data.filthy += sign;
        }
        bp_data.covered += sign;
    };

    const auto covered( it.get_covered_body_parts() );

    if( covered.any() ) {
        for( int i = 0; i < num_bp; ++i ) {
            if( covered.test( i ) ) {
                update_body_part( body_parts[i] );
            }
        }
    } else {
        update_body_part( no_body_part );
    }

    if( super_fancy ) {
        const auto id = it.typeId();
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
        const auto bp_bonus = [ this ]( body_part bp, int bonus ) -> int {
            return (
                body_parts[bp].fancy > 0 ||
                body_parts[opposite_body_part( bp )].fancy > 0 ) ? bonus : 0;
        };
        bonus = std::min( int( 2 * super_fancy_items.size() ) +
                          2 * std::min( int( no_body_part.fancy ), 3 ) +
                          bp_bonus( bp_torso,  6 ) +
                          bp_bonus( bp_head,   3 ) +
                          bp_bonus( bp_eyes,   2 ) +
                          bp_bonus( bp_mouth,  2 ) +
                          bp_bonus( bp_leg_l,  2 ) +
                          bp_bonus( bp_foot_l, 1 ) +
                          bp_bonus( bp_hand_l, 1 ), 20 );
    }
    set_permanent( MORALE_PERM_FANCY, bonus );
}

void player_morale::update_masochist_bonus()
{
    const bool amateur_masochist = has_mutation( trait_id( "MASOCHIST" ) );
    const bool advanced_masochist = has_mutation( trait_id( "MASOCHIST_MED" ) ) || has_mutation( trait_id( "CENOBITE" ) );
    const bool any_masochist = amateur_masochist || advanced_masochist;

    int bonus = 0;

    if( any_masochist ) {
        bonus = perceived_pain / 2.5;
        if( amateur_masochist ) {
            bonus = std::min( bonus, 25 );
        }
        if( took_prozac ) {
            bonus = bonus / 3;
        }
    }
    set_permanent( MORALE_PERM_MASOCHIST, bonus );
}

void player_morale::update_bodytemp_penalty( int ticks )
{
    using bp_int_func = std::function<int( body_part )>;
    const auto apply_pen = [ this, ticks ]( morale_type type, bp_int_func bp_int ) -> void {
        const int max_pen =

        2  * bp_int( bp_head ) +
        2  * bp_int( bp_torso ) +
        2  * bp_int( bp_mouth ) +
        .5 * bp_int( bp_arm_l ) +
        .5 * bp_int( bp_arm_r ) +
        .5 * bp_int( bp_leg_l ) +
        .5 * bp_int( bp_leg_r ) +
        .5 * bp_int( bp_hand_l ) +
        .5 * bp_int( bp_hand_r ) +
        .5 * bp_int( bp_foot_l ) +
        .5 * bp_int( bp_foot_r );

        if( max_pen != 0 )
        {
            add( type, -2 * ticks, -std::abs( max_pen ), 10, 5, true );
        }
    };
    apply_pen( MORALE_COLD, [ this ]( body_part bp ) {
        return body_parts[bp].cold;
    } );
    apply_pen( MORALE_HOT, [ this ]( body_part bp ) {
        return body_parts[bp].hot;
    } );
}

void player_morale::update_constrained_penalty()
{
    const auto bp_pen = [ this ]( body_part bp, int pen ) -> int {
        return ( body_parts[bp].covered > 0 ) ? pen : 0;
    };
    int pen = 0;

    if( has_mutation( trait_id( "FLOWERS" ) ) ) {
        pen += bp_pen( bp_head, 10 );
    }
    if( has_mutation( trait_id( "ROOTS1" ) ) || has_mutation( trait_id( "ROOTS2" ) ) || has_mutation( trait_id( "ROOTS3" ) ) ) {
        pen += bp_pen( bp_foot_l, 5 );
        pen += bp_pen( bp_foot_r, 5 );
    }
    set_permanent( MORALE_PERM_CONSTRAINED, -std::min( pen, 10 ) );
}

void player_morale::update_squeamish_penalty()
{
    if( !get_option<bool>( "FILTHY_MORALE" ) ) {
        set_permanent( MORALE_PERM_FILTHY, 0 );
        return;
    }

    int penalty = 0;
    const auto bp_pen = [ this ]( body_part bp, int penalty ) -> int {
        return (
            body_parts[bp].filthy > 0 ||
            body_parts[opposite_body_part( bp )].filthy > 0 ) ? penalty : 0;
    };
    penalty = 2 * std::min( int( no_body_part.filthy ), 3 ) +
              bp_pen( bp_torso,  6 ) +
              bp_pen( bp_head,   7 ) +
              bp_pen( bp_eyes,   8 ) +
              bp_pen( bp_mouth,  9 ) +
              bp_pen( bp_leg_l,  5 ) +
              bp_pen( bp_leg_r,  5 ) +
              bp_pen( bp_arm_l,  5 ) +
              bp_pen( bp_arm_r,  5 ) +
              bp_pen( bp_foot_l, 3 ) +
              bp_pen( bp_foot_r, 3 ) +
              bp_pen( bp_hand_l, 3 ) +
              bp_pen( bp_hand_r, 3 );
    set_permanent( MORALE_PERM_FILTHY, -penalty );
}
