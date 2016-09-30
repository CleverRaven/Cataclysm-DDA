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

#include <algorithm>

// activity_item_handling.cpp
void activity_on_turn_drop();
void activity_on_turn_move_items();
void activity_on_turn_pickup();
void activity_on_turn_stash();

// advanced_inv.cpp
void advanced_inv();

// veh_interact.cpp
void complete_vehicle();


player_activity::player_activity( activity_type t, int turns, int Index, int pos,
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

const std::string &player_activity::get_stop_phrase() const
{
    static const std::array<std::string, NUM_ACTIVITIES> stop_phrase = {{
            _( "Stop?" ), _( "Stop reloading?" ),
            _( "Stop reading?" ), _( "Stop playing?" ),
            _( "Stop waiting?" ), _( "Stop crafting?" ),
            _( "Stop crafting?" ), _( "Stop disassembly?" ),
            _( "Stop butchering?" ), _( "Stop salvaging?" ), _( "Stop foraging?" ),
            _( "Stop construction?" ), _( "Stop interacting with the vehicle?" ),
            _( "Stop pumping gas?" ), _( "Stop training?" ),
            _( "Stop waiting?" ), _( "Stop using first aid?" ),
            _( "Stop fishing?" ), _( "Stop mining?" ), _( "Stop burrowing?" ),
            _( "Stop smashing?" ), _( "Stop de-stressing?" ),
            _( "Stop cutting tissues?" ), _( "Stop dropping?" ),
            _( "Stop stashing?" ), _( "Stop picking up?" ),
            _( "Stop moving items?" ), _( "Stop interacting with inventory?" ),
            _( "Stop fiddling with your clothes?" ), _( "Stop lighting the fire?" ),
            _( "Stop working the winch?" ), _( "Stop filling the container?" ),
            _( "Stop hotwiring the vehicle?" ), _( "Stop aiming?" ),
            _( "Stop using the ATM?" ), _( "Stop trying to start the vehicle?" ),
            _( "Stop welding?" ), _( "Stop cracking?" ), _( "Stop repairing?" ),
            _( "Stop mending?" ), _( "Stop modifying gun?" ),
            _( "Stop interacting with the NPC?" ), _( "Stop clearing that rubble?" ),
            _( "Stop meditating?" )
        }
    };
    return stop_phrase[type];
}

bool player_activity::is_abortable() const
{
    switch( type ) {
        case ACT_READ:
        case ACT_BUILD:
        case ACT_CRAFT:
        case ACT_LONGCRAFT:
        case ACT_DISASSEMBLE:
        case ACT_WAIT:
        case ACT_WAIT_WEATHER:
        case ACT_FIRSTAID:
        case ACT_PICKAXE:
        case ACT_BURROW:
        case ACT_PULP:
        case ACT_MAKE_ZLAVE:
        case ACT_DROP:
        case ACT_STASH:
        case ACT_PICKUP:
        case ACT_HOTWIRE_CAR:
        case ACT_MOVE_ITEMS:
        case ACT_ADV_INVENTORY:
        case ACT_ARMOR_LAYERS:
        case ACT_START_FIRE:
        case ACT_OPEN_GATE:
        case ACT_FILL_LIQUID:
        case ACT_START_ENGINES:
        case ACT_OXYTORCH:
        case ACT_CRACKING:
        case ACT_REPAIR_ITEM:
        case ACT_MEND_ITEM:
        case ACT_GUNMOD_ADD:
        case ACT_BUTCHER:
        case ACT_CLEAR_RUBBLE:
        case ACT_MEDITATE:
            return true;
        default:
            return false;
    }
}

bool player_activity::never_completes() const
{
    switch( type ) {
        case ACT_ADV_INVENTORY:
            return true;
        default:
            return false;
    }
}

bool player_activity::is_complete() const
{
    return ( never_completes() ) ? false : moves_left <= 0;
}


bool player_activity::is_suspendable() const
{
    switch( type ) {
        case ACT_NULL:
        case ACT_RELOAD:
        case ACT_DISASSEMBLE:
        case ACT_MAKE_ZLAVE:
        case ACT_DROP:
        case ACT_STASH:
        case ACT_PICKUP:
        case ACT_MOVE_ITEMS:
        case ACT_ADV_INVENTORY:
        case ACT_ARMOR_LAYERS:
        case ACT_AIM:
        case ACT_ATM:
        case ACT_START_ENGINES:
        case ACT_GUNMOD_ADD:
        case ACT_FILL_LIQUID:
            return false;
        default:
            return true;
    }
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
    switch( type ) {
        case ACT_WAIT:
        case ACT_WAIT_NPC:
        case ACT_WAIT_WEATHER:
            // Based on time, not speed
            moves_left -= 100;
            p->rooted();
            p->pause();
            break;
        case ACT_PICKAXE:
            // Based on speed, not time
            if( p->moves <= moves_left ) {
                moves_left -= p->moves;
                p->moves = 0;
            } else {
                p->moves -= moves_left;
                moves_left = 0;
            }
            activity_handlers::pickaxe_do_turn( this, p );
            break;
        case ACT_BURROW:
            // Based on speed, not time
            if( p->moves <= moves_left ) {
                moves_left -= p->moves;
                p->moves = 0;
            } else {
                p->moves -= moves_left;
                moves_left = 0;
            }
            activity_handlers::burrow_do_turn( this, p );
            break;
        case ACT_AIM:
            if( index == 0 ) {
                if( !p->weapon.is_gun() ) {
                    // We lost our gun somehow, bail out.
                    type = ACT_NULL;
                    break;
                }

                g->m.build_map_cache( g->get_levz() );
                g->plfire();
            }
            break;
        case ACT_GAME:
            // Takes care of u.activity.moves_left
            activity_handlers::game_do_turn( this, p );
            break;
        case ACT_VIBE:
            // Takes care of u.activity.moves_left
            activity_handlers::vibe_do_turn( this, p );
            break;
        case ACT_REFILL_VEHICLE:
            type = ACT_NULL; // activity is not used anymore.
            break;
        case ACT_PULP:
            // does not really use u.activity.moves_left, stops itself when finished
            activity_handlers::pulp_do_turn( this, p );
            break;
        case ACT_FISH:
            // Based on time, not speed--or it should be
            // (Being faster doesn't make the fish bite quicker)
            moves_left -= 100;
            p->rooted();
            p->pause();
            break;
        case ACT_DROP:
            activity_handlers::drop_do_turn( this, p );
            break;
        case ACT_STASH:
            activity_handlers::stash_do_turn( this, p );
            break;
        case ACT_PICKUP:
            activity_on_turn_pickup();
            break;
        case ACT_MOVE_ITEMS:
            activity_on_turn_move_items();
            break;
        case ACT_ADV_INVENTORY:
            p->cancel_activity();
            advanced_inv();
            break;
        case ACT_ARMOR_LAYERS:
            p->cancel_activity();
            p->sort_armor();
            break;
        case ACT_START_FIRE:
            activity_handlers::start_fire_do_turn( this, p );
            p->rooted();
            p->pause();
            break;
        case ACT_OPEN_GATE:
            // Based on speed, not time
            if( p->moves <= moves_left ) {
                moves_left -= p->moves;
                p->moves = 0;
            } else {
                p->moves -= moves_left;
                moves_left = 0;
            }
            break;
        case ACT_FILL_LIQUID:
            activity_handlers::fill_liquid_do_turn( this, p );
            break;
        case ACT_ATM:
            iexamine::atm( *p, p->pos() );
            break;
        case ACT_START_ENGINES:
            moves_left -= 100;
            p->rooted();
            p->pause();
            break;
        case ACT_OXYTORCH:
            if( p->moves <= moves_left ) {
                moves_left -= p->moves;
                p->moves = 0;
            } else {
                p->moves -= moves_left;
                moves_left = 0;
            }
            if( values[0] > 0 ) {
                activity_handlers::oxytorch_do_turn( this, p );
            }
            break;
        case ACT_CRACKING:
            if( !( p->has_amount( "stethoscope", 1 ) || p->has_bionic( "bio_ears" ) ) ) {
                // We lost our cracking tool somehow, bail out.
                type = ACT_NULL;
                break;
            }
            // Based on speed, not time
            if( p->moves <= moves_left ) {
                moves_left -= p->moves;
                p->moves = 0;
            } else {
                p->moves -= moves_left;
                moves_left = 0;
            }
            p->practice( skill_id( "mechanics" ), 1 );
            break;
        case ACT_REPAIR_ITEM: {
            // Based on speed * detail vision
            const int effective_moves = p->moves / p->fine_detail_vision_mod();
            if( effective_moves <= moves_left ) {
                moves_left -= effective_moves;
                p->moves = 0;
            } else {
                p->moves -= moves_left * p->fine_detail_vision_mod();
                moves_left = 0;
            }
        }

        break;

        case ACT_BUTCHER:
            // Drain some stamina
            p->mod_stat( "stamina", -20.0f * p->stamina / p->get_stamina_max() );
            // Based on speed, not time
            if( p->moves <= moves_left ) {
                moves_left -= p->moves;
                p->moves = 0;
            } else {
                p->moves -= moves_left;
                moves_left = 0;
            }
            break;

        case ACT_READ:
            if( p->moves <= moves_left ) {
                moves_left -= p->moves;
                p->moves = 0;
            } else {
                p->moves -= moves_left;
                moves_left = 0;
            }
            p->rooted();
            break;

        default:
            // Based on speed, not time
            if( p->moves <= moves_left ) {
                moves_left -= p->moves;
                p->moves = 0;
            } else {
                p->moves -= moves_left;
                moves_left = 0;
            }
    }

    if( is_complete() ) {
        finish( p );
    }
}


void player_activity::finish( player *p )
{
    switch( type ) {
        case ACT_RELOAD:
            activity_handlers::reload_finish( this, p );
            break;
        case ACT_READ:
            p->do_read( targets[0].get_item() );
            if( type == ACT_NULL ) {
                add_msg( m_info, _( "You finish reading." ) );
            }
            break;
        case ACT_WAIT:
        case ACT_WAIT_WEATHER:
            add_msg( _( "You finish waiting." ) );
            type = ACT_NULL;
            break;
        case ACT_WAIT_NPC:
            add_msg( _( "%s finishes with you..." ), str_values[0].c_str() );
            type = ACT_NULL;
            break;
        case ACT_CRAFT:
            p->complete_craft();
            type = ACT_NULL;
            break;
        case ACT_LONGCRAFT: {
            int batch_size = values.front();
            p->complete_craft();
            type = ACT_NULL;
            // Workaround for a bug where longcraft can be unset in complete_craft().
            if( p->making_would_work( p->lastrecipe, batch_size ) ) {
                p->last_craft->execute();
            }
        }
        break;
        case ACT_FORAGE:
            activity_handlers::forage_finish( this, p );
            type = ACT_NULL;
            break;
        case ACT_DISASSEMBLE:
            p->complete_disassemble();
            break;
        case ACT_BUTCHER:
            activity_handlers::butcher_finish( this, p );
            break;
        case ACT_LONGSALVAGE:
            activity_handlers::longsalvage_finish( this, p );
            break;
        case ACT_VEHICLE:
            activity_handlers::vehicle_finish( this, p );
            break;
        case ACT_BUILD:
            complete_construction();
            type = ACT_NULL;
            break;
        case ACT_TRAIN:
            activity_handlers::train_finish( this, p );
            break;
        case ACT_FIRSTAID:
            activity_handlers::firstaid_finish( this, p );
            break;
        case ACT_FISH:
            activity_handlers::fish_finish( this, p );
            break;
        case ACT_PICKAXE:
            activity_handlers::pickaxe_finish( this, p );
            type = ACT_NULL;
            break;
        case ACT_BURROW:
            activity_handlers::burrow_finish( this, p );
            type = ACT_NULL;
            break;
        case ACT_VIBE:
            add_msg( m_good, _( "You feel much better." ) );
            type = ACT_NULL;
            break;
        case ACT_MAKE_ZLAVE:
            activity_handlers::make_zlave_finish( this, p );
            type = ACT_NULL;
            break;
        case ACT_PICKUP:
        case ACT_MOVE_ITEMS:
            // Only do nothing if the item being picked up doesn't need to be equipped.
            // If it needs to be equipped, our activity_handler::pickup_finish() does so.
            // This is primarily used by AIM to advance moves while moving items around.
            activity_handlers::pickup_finish( this, p );
            break;
        case ACT_START_FIRE:
            activity_handlers::start_fire_finish( this, p );
            break;
        case ACT_OPEN_GATE:
            activity_handlers::open_gate_finish( this, p );
            type = ACT_NULL;
            break;
        case ACT_HOTWIRE_CAR:
            activity_handlers::hotwire_finish( this, p );
            break;
        case ACT_AIM:
            // Aim bails itself by resetting itself every turn,
            // you only re-enter if it gets set again.
            break;
        case ACT_ATM:
            // ATM sets index to 0 to indicate it's finished.
            if( !index ) {
                type = ACT_NULL;
            }
            break;
        case ACT_START_ENGINES:
            activity_handlers::start_engines_finish( this, p );
            type = ACT_NULL;
            break;
        case ACT_OXYTORCH:
            activity_handlers::oxytorch_finish( this, p );
            type = ACT_NULL;
            break;
        case ACT_CRACKING:
            activity_handlers::cracking_finish( this, p );
            type = ACT_NULL;
            break;
        case ACT_REPAIR_ITEM:
            // Unsets activity (if needed) inside function
            activity_handlers::repair_item_finish( this, p );
            break;
        case ACT_MEND_ITEM:
            activity_handlers::mend_item_finish( this, p );
            type = ACT_NULL;
            break;
        case ACT_GUNMOD_ADD:
            activity_handlers::gunmod_add_finish( this, p );
            type = ACT_NULL;
            break;
        case ACT_CLEAR_RUBBLE:
            activity_handlers::clear_rubble_finish( this, p );
            type = ACT_NULL;
            break;
        case ACT_MEDITATE:
            activity_handlers::meditate_finish( this, p );
            type = ACT_NULL;
            break;
        default:
            type = ACT_NULL;
    }
    if( type == ACT_NULL ) {
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

bool player_activity::can_resume_with( const player_activity &other, const Character &who ) const
{
    // Should be used for relative positions
    // And to forbid resuming now-invalid crafting
    ( void )who;
    switch( type ) {
        case ACT_NULL:
        case NUM_ACTIVITIES:
            return false;
        case ACT_RELOAD:
        case ACT_GAME:
        case ACT_REFILL_VEHICLE:
            break;
        case ACT_WAIT:
        case ACT_WAIT_NPC:
        case ACT_WAIT_WEATHER:
        case ACT_ARMOR_LAYERS:
        case ACT_PULP:
        case ACT_AIM:
        case ACT_LONGSALVAGE:
        case ACT_ATM:
        case ACT_DROP:
        case ACT_STASH:
        case ACT_PICKUP:
        case ACT_MOVE_ITEMS:
        case ACT_ADV_INVENTORY:
        case ACT_START_FIRE:
        case ACT_FILL_LIQUID:
            // Those shouldn't be resumed
            // They don't store their progress in moves
            return false;
        case ACT_CRAFT:
        case ACT_LONGCRAFT:
            // Batch size is stored in values
            // Coords may be matched incorrectly in some rare cases
            // But it will not result in a false negative
            if( !containers_equal( values, other.values ) ||
                !containers_equal( coords, other.coords ) ) {
                return false;
            }

            break;
        case ACT_DISASSEMBLE:
        // Disassembling is currently hardcoded in such a way
        // that easy resuming isn't possible
        case ACT_FORAGE:
        case ACT_OPEN_GATE:
        case ACT_OXYTORCH:
        case ACT_CRACKING:
        case ACT_FISH:
        case ACT_PICKAXE:
        case ACT_BURROW:
        // Those should check position
        // But position isn't set here yet!
        // @todo Update the functions that set those activities
        case ACT_BUTCHER:
        case ACT_TRAIN:
        case ACT_HOTWIRE_CAR:
        case ACT_START_ENGINES:
        case ACT_BUILD:
        case ACT_VEHICLE:
        case ACT_FIRSTAID:
        case ACT_VIBE:
        case ACT_MAKE_ZLAVE:
        case ACT_GUNMOD_ADD:
        case ACT_REPAIR_ITEM:
        case ACT_MEND_ITEM:
        case ACT_MEDITATE:
            // Those should have extra limitations
            // But for now it's better to allow too much than too little
            break;
        case ACT_READ:
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
            break;

        case ACT_CLEAR_RUBBLE:
            if( other.coords.empty() || other.coords[0] != coords[0] ) {
                return false;
            }
            break;
    }

    return !auto_resume && type == other.type && index == other.index &&
           position == other.position && name == other.name && targets == other.targets;
}
