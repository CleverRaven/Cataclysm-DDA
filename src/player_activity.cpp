#include "player_activity.h"

#include "activity_type.h"
#include "construction.h"
#include "craft_command.h"
#include "player.h"
#include "requirements.h"
#include "translations.h"
#include "activity_handlers.h"
#include "messages.h"
#include "mapdata.h"

#include <algorithm>

player_activity::player_activity() : type( activity_id::NULL_ID() ) { }

player_activity::player_activity( activity_id t, int turns, int Index, int pos,
                                  std::string name_in ) :
    type( t ), moves_total( turns ), moves_left( turns ),
    index( Index ),
    position( pos ), name( name_in ), ignore_trivial( false ), values(), str_values(),
    placement( tripoint_min ), warned_of_proximity( false ), auto_resume( false )
{
}

player_activity::player_activity( const player_activity &rhs )
    : type( rhs.type ), moves_total( rhs.moves_total ), moves_left( rhs.moves_left ),
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

void player_activity::set_to_null()
{
    type = activity_id::NULL_ID();
}

bool player_activity::rooted() const
{
    return type->rooted();
}

std::string player_activity::get_stop_phrase() const
{
    return type->stop_phrase();
}

int player_activity::get_value( size_t index, int def ) const
{
    return ( index < values.size() ) ? values[index] : def;
}

bool player_activity::is_suspendable() const
{
    return type->suspendable();
}

std::string player_activity::get_str_value( size_t index, std::string def ) const
{
    return ( index < str_values.size() ) ? str_values[index] : def;
}

void player_activity::do_turn( player &p )
{
    // Should happen before activity or it may fail du to 0 moves
    if( *this && type->will_refuel_fires() ) {
        try_refuel_fire( p );
    }

    if( type->based_on() == based_on_type::TIME ) {
        moves_left -= 100;
    } else if( type->based_on() == based_on_type::SPEED ) {
        if( p.moves <= moves_left ) {
            moves_left -= p.moves;
            p.moves = 0;
        } else {
            p.moves -= moves_left;
            moves_left = 0;
        }
    }

    // This might finish the activity (set it to null)
    type->call_do_turn( this, &p );

    if( *this && type->rooted() ) {
        p.rooted();
        p.pause();
    }

    if( *this && moves_left <= 0 ) {
        // Note: For some activities "finish" is a misnomer; that's why we explicitly check if the
        // type is ACT_NULL below.
        if( !( type->call_finish( this, &p ) ) ) {
            // "Finish" is never a misnomer for any activity without a finish function
            set_to_null();
        }
    }
    if( !*this ) {
        // Make sure data of previous activity is cleared
        p.activity = player_activity();
        if( !p.backlog.empty() && p.backlog.front().auto_resume ) {
            p.activity = p.backlog.front();
            p.backlog.pop_front();
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
