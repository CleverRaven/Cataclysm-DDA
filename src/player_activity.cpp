#include "player_activity.h"

#include "game.h"
#include "map.h"
#include "construction.h"
#include "craft_command.h"
#include "player.h"
#include "requirements.h"
#include "translations.h"
#include "activity_handlers.h"
#include "messages.h"
#include "mapdata.h"
#include "generic_factory.h"

#include <algorithm>

// activity_type functions

static std::map< activity_id, activity_type > activity_type_all;
template<>
const activity_id string_id<activity_type>::NULL_ID( "ACT_NULL" );

template<>
const activity_type &string_id<activity_type>::obj() const
{
    const auto found = activity_type_all.find( *this );
    if( found == activity_type_all.end() ) {
        debugmsg( "Tried to get invalid activity_type: %s", c_str() );
        static const activity_type null_activity_type {};
        return null_activity_type;
    }
    return found->second;
}

static const std::unordered_map< std::string, based_on_type > based_on_type_values = {
    { "time", based_on_type::TIME },
    { "speed", based_on_type::SPEED },
    { "neither", based_on_type::NEITHER }
};

void activity_type::load( JsonObject &jo )
{
    activity_type result;

    result.id_ = activity_id( jo.get_string( "id" ) );
    assign( jo, "rooted", result.rooted_, true );
    result.stop_phrase_ = _( jo.get_string( "stop_phrase" ).c_str() );
    assign( jo, "abortable", result.abortable_, true );
    assign( jo, "suspendable", result.suspendable_, true );
    assign( jo, "no_resume", result.no_resume_, true );

    result.based_on_ = io::string_to_enum_look_up( based_on_type_values, jo.get_string( "based_on" ) );

    if( activity_type_all.find( result.id_ ) != activity_type_all.end() ) {
        debugmsg( "Redefinition of %s", result.id_.c_str() );
    } else {
        activity_type_all.insert( { result.id_, result } );
    }
}

void activity_type::check_consistency()
{
    for( const auto &pair : activity_type_all ) {
        if( pair.second.stop_phrase_ == "" ) {
            debugmsg( "%s doesn't have a stop phrase", pair.first.c_str() );
        }
        if( pair.second.based_on_ == based_on_type::NEITHER &&
            activity_handlers::do_turn_functions.find( pair.second.id_ ) ==
            activity_handlers::do_turn_functions.end() ) {
            debugmsg( "%s needs a do_turn function if it's not based on time or speed.",
                      pair.second.id_.c_str() );
        }
    }

    for( const auto &pair : activity_handlers::do_turn_functions ) {
        if( activity_type_all.find( pair.first ) == activity_type_all.end() ) {
            debugmsg( "The do_turn function %s doesn't correspond to a valid activity_type.",
                      pair.first.c_str() );
        }
    }

    for( const auto &pair : activity_handlers::finish_functions ) {
        if( activity_type_all.find( pair.first ) == activity_type_all.end() ) {
            debugmsg( "The finish_function %s doesn't correspond to a valid activity_type",
                      pair.first.c_str() );
        }
    }
}

void activity_type::call_do_turn( player_activity *act, player *p ) const
{
    const auto &pair = activity_handlers::do_turn_functions.find( id_ );
    if( pair != activity_handlers::do_turn_functions.end() ) {
        pair->second( act, p );
    }
}

bool activity_type::call_finish( player_activity *act, player *p ) const
{
    const auto &pair = activity_handlers::finish_functions.find( id_ );
    if( pair != activity_handlers::finish_functions.end() ) {
        pair->second( act, p );
        return true;
    }
    return false;
}

void activity_type::reset()
{
    activity_type_all.clear();
}

// player_activity functions:

player_activity::player_activity( activity_id t, int turns, int Index, int pos,
                                  std::string name_in ) :
    JsonSerializer(), JsonDeserializer(), type( t ), moves_total( turns ), moves_left( turns ),
    index( Index ),
    position( pos ), name( name_in ), ignore_trivial( false ), values(), str_values(),
    placement( tripoint_min ), warned_of_proximity( false ), auto_resume( false )
{
}

player_activity::player_activity( const player_activity &rhs )
    : JsonSerializer( rhs ), JsonDeserializer( rhs ),
      type( rhs.type ), moves_total( rhs.moves_total ), moves_left( rhs.moves_left ),
      index( rhs.index ), position( rhs.position ), name( rhs.name ),
      ignore_trivial( rhs.ignore_trivial ), values( rhs.values ), str_values( rhs.str_values ),
      coords( rhs.coords ), placement( rhs.placement ),
      warned_of_proximity( rhs.warned_of_proximity ), auto_resume( rhs.auto_resume )
{
    targets.clear();
    targets.reserve( rhs.targets.size() );
    std::transform( rhs.targets.begin(), rhs.targets.end(), std::back_inserter( targets ),
    []( const item_location & e ) {
        return e.clone();
    } );
}

player_activity &player_activity::operator=( const player_activity &rhs )
{
    type = rhs.type;
    moves_total = rhs.moves_total;
    moves_left = rhs.moves_left;
    index = rhs.index;
    position = rhs.position;
    name = rhs.name;
    ignore_trivial = rhs.ignore_trivial;
    values = rhs.values;
    str_values = rhs.str_values;
    coords = rhs.coords;
    placement = rhs.placement;
    warned_of_proximity = rhs.warned_of_proximity;
    auto_resume = rhs.auto_resume;

    targets.clear();
    targets.reserve( rhs.targets.size() );
    std::transform( rhs.targets.begin(), rhs.targets.end(), std::back_inserter( targets ),
    []( const item_location & e ) {
        return e.clone();
    } );

    return *this;
}

int player_activity::get_value( size_t index, int def ) const
{
    return ( index < values.size() ) ? values[index] : def;
}

std::string player_activity::get_str_value( size_t index, std::string def ) const
{
    return ( index < str_values.size() ) ? str_values[index] : def;
}

void player_activity::do_turn( player *p )
{
    if( type->based_on() == based_on_type::TIME ) {
        moves_left -= 100;
    } else if( type->based_on() == based_on_type::SPEED ) {
        if( p->moves <= moves_left ) {
            moves_left -= p->moves;
            p->moves = 0;
        } else {
            p->moves -= moves_left;
            moves_left = 0;
        }
    }

    // This might finish the activity (set it to null)
    type->call_do_turn( this, p );

    if( *this && type->rooted() ) {
        p->rooted();
        p->pause();
    }

    if( *this && moves_left <= 0 ) {
        // Note: For some activities "finish" is a misnomer; that's why we explicitly check if the
        // type is ACT_NULL below.
        if( !( type->call_finish( this, p ) ) ) {
            // "Finish" is never a misnomer for any activity without a finish function
            set_to_null();
        }
    }
    if( !*this ) {
        // Make sure data of previous activity is cleared
        p->activity = player_activity();
        if( !p->backlog.empty() && p->backlog.front().auto_resume ) {
            p->activity = p->backlog.front();
            p->backlog.pop_front();
        }
    }
}

template <typename T>
bool containers_equal( const T &left, const T &right )
{
    if( left.size() != right.size() ) {
        return false;
    }

    return std::equal( left.begin(), left.end(), right.begin() );
}

bool player_activity::can_resume_with( const player_activity &other, const Character & ) const
{
    // Should be used for relative positions
    // And to forbid resuming now-invalid crafting

    // @todo: Once activity_handler_actors exist, the less ugly method of using a
    // pure virtual can_resume_with should be used

    if( !*this || !other || type->no_resume() ) {
        return false;
    }

    if( id() == activity_id( "ACT_CRAFT" ) || id() == activity_id( "ACT_LONGCRAFT" ) ) {
        if( !containers_equal( values, other.values ) ||
            !containers_equal( coords, other.coords ) ) {
            return false;
        }
    } else if( id() == activity_id( "ACT_CLEAR_RUBBLE" ) ) {
        if( other.coords.empty() || other.coords[0] != coords[0] ) {
            return false;
        }
    } else if( id() == activity_id( "ACT_READ" ) ) {
        // Return false if any NPCs joined or left the study session
        // the vector {1, 2} != {2, 1}, so we'll have to check manually
        if( values.size() != other.values.size() ) {
            return false;
        }
        for( int foo : other.values ) {
            if( std::find( values.begin(), values.end(), foo ) == values.end() ) {
                return false;
            }
        }
        if( targets.empty() || other.targets.empty() || targets[0] != other.targets[0] ) {
            return false;
        }
    }

    return !auto_resume && id() == other.id() && index == other.index &&
           position == other.position && name == other.name && targets == other.targets;
}

class danger_warning
{
    private:
        bool warn_noise = false;
        bool warn_footstep = false;
        std::set<creature_id> ignored_critters;
    public:
        /** Returns true if character cares. */
        bool warn_noise( Character &c, const sound_event &s );
        /** Returns true if character cares. */
        bool warn_critter( Character &c, const Creature &critter );
}

const std::string &describe_unknown_noise( const sound_event &s )
{
    // @todo Less hardcoded
    const std::array<std::string, 3> footstep_descriptions = {{
        _( "Heard tiny footsteps!" ), _( "Heard footsteps!" ), _( "Heard something stomping around!" )
    }};
    const std::array<std::string, 3> noise_descriptions = {{
        _( "Heard soft noise!" ), _( "Heard a noise!" ), _( "Heard loud noise!" )
    }};
    // Note: base volume, not heard one
    const size_t index = s.volume > 5 + s.volume > 10;
    return s.footstep ? footstep_descriptions[ index ] : noise_descriptions[ index ];
}

bool danger_warning::warn_noise( Character &c, const sound_event &s )
{
    if( ( s.footstep && ignore_footstep ) ||
        ( !s.footstep && ignore_noise ) ) {
        return false;
    }

    // @todo Get rid of casts when player/Character migration is done
    player &p = dynamic_cast<player &>( c );
    if( !p.activity ) {
        debugmsg( "No player activity" );
        return false;
    }

    const std::string &query = s.description.empty()
                               ? describe_unknown_noise( s ) 
                               : string_format( _( "Heard %s!" ), description.c_str() );

    std::string stop_message = text + " " + u.activity.get_stop_phrase() + " " +
                               _( "(Y)es, (N)o, (I)gnore further distractions and finish." );

    do {
        ch = popup(stop_message, PF_GET_KEY);
    } while (ch != '\n' && ch != ' ' && ch != KEY_ESCAPE &&
             ch != 'Y' && ch != 'N' && ch != 'I' &&
             (force_uc || (ch != 'y' && ch != 'n' && ch != 'i')));

    if (ch == 'Y' || ch == 'y') {
        u.cancel_activity();
    } else if (ch == 'I' || ch == 'i') {
        return true;
    }
    return false;
    
    if( g->cancel_activity_or_ignore_query( query.c_str() ) ) {
        p->activity.ignore_trivial = true;
        for( auto activity : p->backlog ) {
            activity.ignore_trivial = true;
        }
    }
}
bool danger_warning::warn_critter( Character &c, const Creature &critter )
