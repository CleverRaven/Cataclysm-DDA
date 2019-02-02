#include "player_activity.h"

#include <algorithm>

#include "activity_handlers.h"
#include "activity_type.h"
#include "craft_command.h"
#include "player.h"

player_activity::player_activity() : type( activity_id::NULL_ID() ) { }

player_activity::player_activity( activity_id t, int turns, int Index, int pos,
                                  const std::string &name_in ) :
    type( t ), moves_total( turns ), moves_left( turns ),
    index( Index ),
    position( pos ), name( name_in ),
    placement( tripoint_min ), auto_resume( false )
{
}

player_activity::player_activity( const player_activity &rhs )
    : type( rhs.type ), ignored_distractions( rhs.ignored_distractions ),
      moves_total( rhs.moves_total ), moves_left( rhs.moves_left ),
      index( rhs.index ), position( rhs.position ), name( rhs.name ),
      values( rhs.values ), str_values( rhs.str_values ),
      coords( rhs.coords ), placement( rhs.placement ),
      auto_resume( rhs.auto_resume )
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
    ignored_distractions = rhs.ignored_distractions;
    values = rhs.values;
    str_values = rhs.str_values;
    coords = rhs.coords;
    placement = rhs.placement;
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

std::string player_activity::get_str_value( size_t index, const std::string &def ) const
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
        p.resume_backlog_activity();

        // If whatever activity we were doing forced us to pick something up to
        // handle it, drop any overflow that may have caused
        p.drop_invalid_inventory();
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
        // The last value is a time stamp, and the last coord is the player
        // position.  We want to allow either to have changed.
        // (This would be much less hacky in the hypothetical future of
        // activity_handler_actors).
        if( !( values.size() == other.values.size() &&
               values.size() >= 1 &&
               std::equal( values.begin(), values.end() - 1, other.values.begin() ) &&
               coords.size() == other.coords.size() &&
               coords.size() >= 1 &&
               std::equal( coords.begin(), coords.end() - 1, other.coords.begin() ) ) ) {
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

void player_activity::resume_with( const player_activity &other )
{
    if( id() == activity_id( "ACT_CRAFT" ) || id() == activity_id( "ACT_LONGCRAFT" ) ) {
        // For crafting actions, we need to update the start turn and position
        // to the resumption time values.  These are stored in the last
        // elements of values and coords respectively.
        if( !( values.size() >= 1 && values.size() == other.values.size() &&
               coords.size() >= 1 && coords.size() == other.coords.size() ) ) {
            debugmsg( "Activities incompatible; should not have resumed" );
            return;
        }
        values.back() = other.values.back();
        coords.back() = other.coords.back();
    }
}

bool player_activity::is_distraction_ignored( distraction_type type ) const
{
    return ignored_distractions.find( type ) != ignored_distractions.end();
}

void player_activity::ignore_distraction( distraction_type type )
{
    ignored_distractions.emplace( type );
}

void player_activity::allow_distractions()
{
    ignored_distractions.clear();
}

void player_activity::inherit_distractions( const player_activity &other )
{
    for( auto &type : other.ignored_distractions ) {
        ignore_distraction( type );
    }
}
