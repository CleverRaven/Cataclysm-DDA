#include "morale.h"

#include "cata_utility.h"
#include "debug.h"
#include "itype.h"
#include "output.h"
#include "bodypart.h"
#include "player.h"
#include "catacharset.h"
#include "game.h"
#include "weather.h"

#include <stdlib.h>
#include <algorithm>

const efftype_id effect_cold( "cold" );
const efftype_id effect_hot( "hot" );
const efftype_id effect_took_prozac( "took_prozac" );
const efftype_id effect_took_xanax( "took_xanax" );

namespace
{
static const std::string item_name_placeholder = "%i"; // Used to address an item name

const std::string &get_morale_data( const morale_type id )
{
    static const std::array<std::string, NUM_MORALE_TYPES> morale_data = { {
            { "This is a bug (player.cpp:moraledata)" },
            { _( "Enjoyed %i" ) },
            { _( "Enjoyed a hot meal" ) },
            { _( "Music" ) },
            { _( "Enjoyed honey" ) },
            { _( "Played Video Game" ) },
            { _( "Marloss Bliss" ) },
            { _( "Mutagenic Anticipation" ) },
            { _( "Good Feeling" ) },
            { _( "Supported" ) },
            { _( "Looked at photos" ) },

            { _( "Nicotine Craving" ) },
            { _( "Caffeine Craving" ) },
            { _( "Alcohol Craving" ) },
            { _( "Opiate Craving" ) },
            { _( "Speed Craving" ) },
            { _( "Cocaine Craving" ) },
            { _( "Crack Cocaine Craving" ) },
            { _( "Mutagen Craving" ) },
            { _( "Diazepam Craving" ) },
            { _( "Marloss Craving" ) },

            { _( "Disliked %i" ) },
            { _( "Ate Human Flesh" ) },
            { _( "Ate Meat" ) },
            { _( "Ate Vegetables" ) },
            { _( "Ate Fruit" ) },
            { _( "Lactose Intolerance" ) },
            { _( "Ate Junk Food" ) },
            { _( "Wheat Allergy" ) },
            { _( "Ate Indigestible Food" ) },
            { _( "Wet" ) },
            { _( "Dried Off" ) },
            { _( "Cold" ) },
            { _( "Hot" ) },
            { _( "Bad Feeling" ) },
            { _( "Killed Innocent" ) },
            { _( "Killed Friend" ) },
            { _( "Guilty about Killing" ) },
            { _( "Guilty about Mutilating Corpse" ) },
            { _( "Fey Mutation" ) },
            { _( "Chimerical Mutation" ) },
            { _( "Mutation" ) },

            { _( "Moodswing" ) },
            { _( "Read %i" ) },
            { _( "Got comfy" ) },

            { _( "Heard Disturbing Scream" ) },

            { _( "Masochism" ) },
            { _( "Hoarder" ) },
            { _( "Stylish" ) },
            { _( "Optimist" ) },
            { _( "Bad Tempered" ) },
            //~ You really don't like wearing the Uncomfy Gear
            { _( "Uncomfy Gear" ) },
            { _( "Found kitten <3" ) },

            { _( "Got a Haircut" ) },
            { _( "Freshly Shaven" ) },
        }
    };

    if( static_cast<size_t>( id ) >= morale_data.size() ) {
        debugmsg( "invalid morale type: %d", id );
        return morale_data[0];
    }

    return morale_data[id];
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
static const morale_mult optimistic( 1.25, 0.75 );
// Again, those grouchy Bad-Tempered folks always focus on the negative.
// They can't handle positive things as well.  They're No Fun.  D:
static const morale_mult badtemper( 0.75, 1.25 );
// Prozac reduces overall negative morale by 75%.
static const morale_mult prozac( 1.0, 0.25 );
}

std::string player_morale::morale_point::get_name() const
{
    std::string name = get_morale_data( type );

    if( item_type != nullptr ) {
        name = string_replace( name, item_name_placeholder, item_type->nname( 1 ) );
    } else if( name.find( item_name_placeholder ) != std::string::npos ) {
        debugmsg( "%s(): Morale #%d (%s) requires item_type to be specified.", __FUNCTION__, type,
                  name.c_str() );
    }

    return name;
}

int player_morale::morale_point::get_net_bonus() const
{
    return bonus * ( ( age > decay_start ) ? logarithmic_range( decay_start, duration, age ) : 1 );
}

int player_morale::morale_point::get_net_bonus( const morale_mult &mult ) const
{
    return get_net_bonus() * mult;
}

bool player_morale::morale_point::is_expired() const
{
    return age >= duration || bonus == 0; // Zero morale bonuses will be shown occasionally anyway
}

void player_morale::morale_point::add( int new_bonus, int new_max_bonus, int new_duration,
                                       int new_decay_start,
                                       bool new_cap )
{
    if( new_cap ) {
        duration = new_duration;
        decay_start = new_decay_start;
    } else {
        bool same_sign = ( bonus > 0 ) == ( new_max_bonus > 0 );

        duration = pick_time( duration, new_duration, same_sign );
        decay_start = pick_time( decay_start, new_decay_start, same_sign );
    }

    bonus = get_net_bonus() + new_bonus;

    if( abs( bonus ) > abs( new_max_bonus ) && ( new_max_bonus != 0 || new_cap ) ) {
        bonus = new_max_bonus;
    }

    age = 0; // Brand new. The assignment should stay below.
}

int player_morale::morale_point::pick_time( int current_time, int new_time, bool same_sign ) const
{
    const int remaining_time = current_time - age;
    return ( remaining_time <= new_time && same_sign ) ? new_time : remaining_time;
}

void player_morale::morale_point::decay( int ticks )
{
    if( ticks < 0 ) {
        debugmsg( "%s(): Called with negative ticks %d.", __FUNCTION__, ticks );
        return;
    }

    age += ticks;
}

void player_morale::add( morale_type type, int bonus, int max_bonus,
                         int duration, int decay_start,
                         bool capped, const itype *item_type )
{
    // Search for a matching morale entry.
    for( auto &m : points ) {
        if( m.get_type() == type && m.get_item_type() == item_type ) {
            const int prev_bonus = m.get_net_bonus();

            m.add( bonus, max_bonus, duration, decay_start, capped );
            if( m.get_net_bonus() != prev_bonus ) {
                invalidate();
            }
            return;
        }
    }

    morale_point new_morale( type, item_type, bonus, duration, decay_start );

    if( !new_morale.is_expired() ) {
        points.push_back( new_morale );
        invalidate();
    }
}

int player_morale::has( morale_type type, const itype *item_type ) const
{
    for( auto &m : points ) {
        if( m.get_type() == type && ( item_type == nullptr || m.get_item_type() == item_type ) ) {
            return m.get_net_bonus();
        }
    }
    return 0;
}

void player_morale::remove( morale_type type, const itype *item_type )
{
    for( size_t i = 0; i < points.size(); ++i ) {
        if( points[i].get_type() == type && points[i].get_item_type() == item_type ) {
            if( points[i].get_net_bonus() > 0 ) {
                invalidate();
            }
            points.erase( points.begin() + i );
            break;
        }
    }
}

morale_mult player_morale::get_traits_mult( const player &p ) const
{
    morale_mult ret;

    if( p.has_trait( "OPTIMISTIC" ) ) {
        ret *= morale_mults::optimistic;
    }

    if( p.has_trait( "BADTEMPER" ) ) {
        ret *= morale_mults::badtemper;
    }

    return ret;
}

morale_mult player_morale::get_effects_mult( const player &p ) const
{
    morale_mult ret;

    //TODO: Maybe add something here to cheer you up as well?
    if( p.has_effect( effect_took_prozac ) ) {
        ret *= morale_mults::prozac;
    }

    return ret;
}

int player_morale::get_level( const player &p ) const
{
    if( !level_is_valid ) {
        const morale_mult mult = get_traits_mult( p );

        level = 0;
        for( auto &m : points ) {
            level += m.get_net_bonus( mult );
        }

        level *= get_effects_mult( p );
        level_is_valid = true;
    }

    return level;
}

void player_morale::update( const player &p, const int ticks )
{
    decay( ticks );

    apply_permanent( p );
    apply_body_temperature( p );
    //apply_body_wetness( p );
}

void player_morale::decay( int ticks )
{
    const auto proceed = [ ticks ]( morale_point & m ) {
        m.decay( ticks );
    };
    const auto is_expired = []( const morale_point & m ) -> bool { return m.is_expired(); };

    std::for_each( points.begin(), points.end(), proceed );
    const auto new_end = std::remove_if( points.begin(), points.end(), is_expired );
    points.erase( new_end, points.end() );
    // Invalidate level to recalculate it on demand
    invalidate();
}

void player_morale::display( const player &p )
{
    // Create and draw the window itself.
    WINDOW *w = newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                        ( TERMY > FULL_SCREEN_HEIGHT ) ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0,
                        ( TERMX > FULL_SCREEN_WIDTH ) ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0 );
    draw_border( w );

    // Figure out how wide the name column needs to be.
    int name_column_width = 18;
    for( auto &i : points ) {
        int length = utf8_width( i.get_name() );
        if( length > name_column_width ) {
            name_column_width = length;
        }
    }

    // If it's too wide, truncate.
    if( name_column_width > 72 ) {
        name_column_width = 72;
    }

    // Start printing the number right after the name column.
    // We'll right-justify it later.
    int number_pos = name_column_width + 1;

    // Header
    mvwprintz( w, 1,  1, c_white, _( "Morale Modifiers:" ) );
    mvwprintz( w, 2,  1, c_ltgray, _( "Name" ) );
    mvwprintz( w, 2, name_column_width + 2, c_ltgray, _( "Value" ) );

    // Ensure the player's persistent morale effects are up-to-date.
    apply_permanent( p );

    const morale_mult mult = get_traits_mult( p );
    // Print out the morale entries.
    for( size_t i = 0; i < points.size(); i++ ) {
        std::string name = points[i].get_name();
        int bonus = points[i].get_net_bonus( mult );

        // Print out the name.
        trim_and_print( w, i + 3,  1, name_column_width, ( bonus < 0 ? c_red : c_green ), name.c_str() );

        // Print out the number, right-justified.
        mvwprintz( w, i + 3, number_pos, ( bonus < 0 ? c_red : c_green ),
                   "% 6d", bonus );
    }

    // Print out the total morale, right-justified.
    int mor = get_level( p );
    mvwprintz( w, 20, 1, ( mor < 0 ? c_red : c_green ), _( "Total:" ) );
    mvwprintz( w, 20, number_pos, ( mor < 0 ? c_red : c_green ), "% 6d", mor );

    // Print out the focus gain rate, right-justified.
    double gain = ( p.calc_focus_equilibrium() - p.focus_pool ) / 100.0;
    mvwprintz( w, 22, 1, ( gain < 0 ? c_red : c_green ), _( "Focus gain:" ) );
    mvwprintz( w, 22, number_pos - 3, ( gain < 0 ? c_red : c_green ), _( "%6.2f per minute" ), gain );

    // Make sure the changes are shown.
    wrefresh( w );

    // Wait for any keystroke.
    getch();

    // Close the window.
    werase( w );
    delwin( w );
}

void player_morale::clear()
{
    points.clear();
    invalidate();
}

void player_morale::invalidate()
{
    level_is_valid = false;
}

int player_morale::get_hoarder_penalty( const player &p ) const
{
    int pen = int( ( p.volume_capacity() - p.volume_carried() ) / 2 );

    if( pen > 70 ) {
        pen = 70;
    } else if( pen < 0 ) {
        pen = 0;
    }
    if( p.has_effect( effect_took_xanax ) ) {
        pen = int( pen / 7 );
    } else if( p.has_effect( effect_took_prozac ) ) {
        pen = int( pen / 2 );
    }
    return pen;
}

int player_morale::get_stylish_bonus( const player &p ) const
{
    int bonus = 0;
    std::bitset<num_bp> covered; // body parts covered

    for( auto &elem : p.worn ) {
        const bool basic_flag = elem.has_flag( "FANCY" );
        const bool bonus_flag = elem.has_flag( "SUPER_FANCY" );

        if( basic_flag || bonus_flag ) {
            if( bonus_flag ) {
                bonus += 2;
            }
            covered |= elem.get_covered_body_parts();
        }
    }

    if( covered.test( bp_torso ) ) {
        bonus += 6;
    }
    if( covered.test( bp_head ) ) {
        bonus += 3;
    }
    if( covered.test( bp_eyes ) ) {
        bonus += 2;
    }
    if( covered.test( bp_mouth ) ) {
        bonus += 2;
    }
    if( covered.test( bp_leg_l ) || covered.test( bp_leg_r ) ) {
        bonus += 2;
    }
    if( covered.test( bp_foot_l ) || covered.test( bp_foot_r ) ) {
        bonus += 1;
    }
    if( covered.test( bp_hand_l ) || covered.test( bp_hand_r ) ) {
        bonus += 1;
    }
    if( covered.test( bp_arm_l ) || covered.test( bp_arm_r ) ) {
        bonus += 1;
    }

    return std::min( bonus, 20 );
}

int player_morale::get_pain_bonus( const player &p ) const
{
    int bonus = p.pain / 2.5;
    // Advanced masochists really get a morale bonus from pain.
    // (It's not capped.)
    if( p.has_trait( "MASOCHIST" ) && ( bonus > 25 ) ) {
        bonus = 25;
    }
    if( p.has_effect( effect_took_prozac ) ) {
        bonus = int( bonus / 3 );
    }

    return bonus;
}

void player_morale::apply_permanent( const player &p )
{
    // Hoarders get a morale penalty if they're not carrying a full inventory.
    if( p.has_trait( "HOARDER" ) ) {
        const int pen = get_hoarder_penalty( p );
        if( pen > 0 ) {
            add( MORALE_PERM_HOARDER, -pen, -pen, 5, 5, true );
        }
    }

    // The stylish get a morale bonus for each body part covered in an item
    // with the FANCY or SUPER_FANCY tag.
    if( p.has_trait( "STYLISH" ) ) {
        const int bonus = get_stylish_bonus( p );
        if( bonus > 0 ) {
            add( MORALE_PERM_FANCY, bonus, bonus, 5, 5, true );
        }
    }

    // Floral folks really don't like having their flowers covered.
    if( p.has_trait( "FLOWERS" ) && p.wearing_something_on( bp_head ) ) {
        add( MORALE_PERM_CONSTRAINED, -10, -10, 5, 5, true );
    }

    // The same applies to rooters and their feet; however, they don't take
    // too many problems from no-footgear.
    if( ( p.has_trait( "ROOTS" ) || p.has_trait( "ROOTS2" ) || p.has_trait( "ROOTS3" ) ) ) {
        double shoe_factor = p.footwear_factor();

        if( shoe_factor > 0.0 ) {
            add( MORALE_PERM_CONSTRAINED, -10 * shoe_factor, -10 * shoe_factor, 5, 5, true );
        }
    }

    // Masochists get a morale bonus from pain.
    if( p.has_trait( "MASOCHIST" ) || p.has_trait( "MASOCHIST_MED" ) ||
        p.has_trait( "CENOBITE" ) ) {
        const int bonus = get_pain_bonus( p );
        if( bonus > 0 ) {
            add( MORALE_PERM_MASOCHIST, bonus, bonus, 5, 5, true );
        }
    }

    // Optimist gives a base +4 to morale.
    // The +25% boost from optimist also applies here, for a net of +5.
    if( p.has_trait( "OPTIMISTIC" ) ) {
        add( MORALE_PERM_OPTIMIST, 4, 4, 5, 5, true );
    }

    // And Bad Temper works just the same way.  But in reverse.  ):
    if( p.has_trait( "BADTEMPER" ) ) {
        add( MORALE_PERM_BADTEMPER, -4, -4, 5, 5, true );
    }
}

void player_morale::apply_body_temperature( const player &p )
{
    if( p.has_trait( "DEBUG_NOTEMP" ) ) {
        return;
    }

    int pen = 0;

    for( int i = 0 ; i < num_bp; i++ ) {
        // A negative morale_pen means the player is cold
        const int cold_int = p.get_effect_int( effect_cold, ( body_part )i );
        const int hot_int = p.get_effect_int( effect_hot, ( body_part )i );
        const int balance = hot_int - cold_int;

        if( balance == 0 ) {
            continue;
        }

        switch( i ) {
            case bp_head:
            case bp_torso:
            case bp_mouth:
                pen += 2 * balance;
                break;
            case bp_arm_l:
            case bp_arm_r:
            case bp_leg_l:
            case bp_leg_r:
                pen += .5 * balance;
                break;
            case bp_hand_l:
            case bp_hand_r:
            case bp_foot_l:
            case bp_foot_r:
                pen += .5 * balance;
                break;
        }
    }
    // TODO: MORALE_COMFY should be applied here as well. Now it's in player::update_bodytemp()
    // TODO: Probably penalties should be independent. I.e cold hands and sweating torso.
    if( pen != 0 ) {
        add( ( pen > 0 ) ? MORALE_HOT : MORALE_COLD, -2, -abs( pen ), 10, 5, true );
    }
}

void player_morale::apply_body_wetness( const player &p )
{
    // First, a quick check if we have any wetness to calculate morale from
    // Faster than checking all worn items for friendliness
    if( !std::any_of( p.body_wetness.begin(), p.body_wetness.end(),
        []( const int w ) { return w != 0; } ) ) {
        return;
    }

    // Normalize temperature to [-1.0,1.0]
    int temperature = std::max( 0, std::min( 100, ( int ) g->temperature ) );
    const double global_temperature_mod = -1.0 + ( 2.0 * temperature / 100.0 );

    int total_morale = 0;
    const auto wet_friendliness = p.exclusive_flag_coverage( "WATER_FRIENDLY" );
    for( int i = bp_torso; i < num_bp; ++i ) {
        // Sum of body wetness can go up to 103
        const int part_drench = p.body_wetness[i];
        if( part_drench == 0 ) {
            continue;
        }

        const auto &part_arr = p.mut_drench[i];
        const int part_ignored = part_arr[player::WT_IGNORED];
        const int part_neutral = part_arr[player::WT_NEUTRAL];
        const int part_good    = part_arr[player::WT_GOOD];

        if( part_ignored >= part_drench ) {
            continue;
        }

        int bp_morale = 0;
        const bool is_friendly = wet_friendliness[i];
        const int effective_drench = part_drench - part_ignored;
        if( is_friendly ) {
            // Using entire bonus from mutations and then some "human" bonus
            bp_morale = std::min( part_good, effective_drench ) + effective_drench / 2;
        } else if( effective_drench < part_good ) {
            // Positive or 0
            // Won't go higher than part_good / 2
            // Wet slime/scale doesn't feel as good when covered by wet rags/fur/kevlar
            bp_morale = std::min( effective_drench, part_good - effective_drench );
        } else if( effective_drench > part_good + part_neutral ) {
            // This one will be negative
            bp_morale = part_good + part_neutral - effective_drench;
        }

        // Clamp to [COLD,HOT] and cast to double
        const double part_temperature =
            std::min( BODYTEMP_HOT, std::max( BODYTEMP_COLD, p.temp_cur[i] ) );
        // 0.0 at COLD, 1.0 at HOT
        const double part_mod = (part_temperature - BODYTEMP_COLD) /
                                (BODYTEMP_HOT - BODYTEMP_COLD);
        // Average of global and part temperature modifiers, each in range [-1.0, 1.0]
        double scaled_temperature = ( global_temperature_mod + part_mod ) / 2;

        if( bp_morale < 0 ) {
            // Damp, hot clothing on hot skin feels bad
            scaled_temperature = fabs( scaled_temperature );
        }

        // For an unmutated human swimming in deep water, this will add up to:
        // +51 when hot in 100% water friendly clothing
        // -103 when cold/hot in 100% unfriendly clothing
        total_morale += static_cast<int>( bp_morale * ( 1.0 + scaled_temperature ) / 2.0 );
    }

    if( total_morale == 0 ) {
        return;
    }

    int morale_effect = total_morale / 8;
    if( morale_effect == 0 ) {
        if( total_morale > 0 ) {
            morale_effect = 1;
        } else {
            morale_effect = -1;
        }
    }

    add( MORALE_WET, morale_effect, total_morale, 5, 5, true );
}
