#include "player_activity.h"

#include "game.h"
#include "map.h"
#include "construction.h"
#include "player.h"
#include "translations.h"
#include "activity_handlers.h"
#include "messages.h"
#include "mapdata.h"

// activity_item_handling.cpp
void activity_on_turn_drop();
void activity_on_turn_move_items();
void activity_on_turn_move_all_items();
void activity_on_turn_pickup();
void activity_on_turn_stash();

// advanced_inv.cpp
void advanced_inv();

// veh_interact.cpp
void complete_vehicle();


player_activity::player_activity(activity_type t, int turns, int Index, int pos,
                                 std::string name_in) :
    JsonSerializer(), JsonDeserializer(), type(t), moves_left(turns), index(Index),
    position(pos), name(name_in), ignore_trivial(false), values(), str_values(),
    placement( tripoint_min ), warned_of_proximity(false), auto_resume(false)
{
}

const std::string &player_activity::get_stop_phrase() const
{
    static const std::string stop_phrase[NUM_ACTIVITIES] = {
        _(" Stop?"), _(" Stop reloading?"),
        _(" Stop reading?"), _(" Stop playing?"),
        _(" Stop waiting?"), _(" Stop crafting?"),
        _(" Stop crafting?"), _(" Stop disassembly?"),
        _(" Stop butchering?"), _(" Stop salvaging?"), _(" Stop foraging?"),
        _(" Stop construction?"), _(" Stop construction?"),
        _(" Stop pumping gas?"), _(" Stop training?"),
        _(" Stop waiting?"), _(" Stop using first aid?"),
        _(" Stop fishing?"), _(" Stop mining?"), _(" Stop burrowing?"),
        _(" Stop smashing?"), _(" Stop de-stressing?"),
        _(" Stop cutting tissues?"), _(" Stop dropping?"),
        _(" Stop stashing?"), _(" Stop picking up?"),
        _(" Stop moving items?"), _(" Stop interacting with inventory?"),
        _(" Stop fiddling with your clothes?"), _(" Stop lighting the fire?"),
        _(" Stop working the winch?"), _(" Stop filling the container?"),
        _(" Stop hotwiring the vehicle?"), _(" Stop aiming?"),
        _(" Stop using the ATM?"), _(" Stop trying to start the vehicle?"),
        _(" Stop welding?"), _(" Stop cracking?"), _(" Stop repairing?")
    };
    return stop_phrase[type];
}

bool player_activity::is_abortable() const
{
    switch(type) {
        case ACT_READ:
        case ACT_BUILD:
        case ACT_CRAFT:
        case ACT_LONGCRAFT:
        case ACT_DISASSEMBLE:
        case ACT_REFILL_VEHICLE:
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
            return true;
        default:
            return false;
    }
}

bool player_activity::never_completes() const
{
    switch(type) {
        case ACT_ADV_INVENTORY:
            return true;
        default:
            return false;
    }
}

bool player_activity::is_complete() const
{
    return (never_completes()) ? false : moves_left <= 0;
}


bool player_activity::is_suspendable() const
{
    switch(type) {
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
            return false;
        default:
            return true;
    }
}


int player_activity::get_value(size_t index, int def) const
{
    return (index < values.size()) ? values[index] : def;
}

std::string player_activity::get_str_value(size_t index, std::string def) const
{
    return (index < str_values.size()) ? str_values[index] : def;
}

void player_activity::do_turn( player *p )
{
    switch (type) {
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
            if (p->moves <= moves_left) {
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
            if (p->moves <= moves_left) {
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
                g->plfire(false);
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
            // Takes care of u.activity.moves_left
            activity_handlers::refill_vehicle_do_turn( this, p );
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
            activity_on_turn_drop();
            break;
        case ACT_STASH:
            activity_on_turn_stash();
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
            moves_left -= 100; // based on time
            if (p->i_at(position).has_flag("LENS")) { // if using a lens, handle potential changes in weather
                activity_handlers::start_fire_lens_do_turn( this, p );
            }
            p->rooted();
            p->pause();
            break;
        case ACT_OPEN_GATE:
            // Based on speed, not time
            if (p->moves <= moves_left) {
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
            iexamine::atm(p, &g->m, p->pos());
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
             if ( !( p->has_amount("stethoscope", 1) || p->has_bionic("bio_ears") ) ) {
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
        case ACT_REPAIR_ITEM:
        {
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

    if (is_complete()) {
        finish( p );
    }
}


void player_activity::finish( player *p )
{
    switch (type) {
        case ACT_RELOAD:
            activity_handlers::reload_finish( this, p );
            break;
        case ACT_READ:
            p->do_read(&(p->i_at(position)));
            if (type == ACT_NULL) {
                add_msg(_("You finish reading."));
            }
            break;
        case ACT_WAIT:
        case ACT_WAIT_WEATHER:
            add_msg(_("You finish waiting."));
            type = ACT_NULL;
            break;
        case ACT_WAIT_NPC:
            add_msg(_("%s finishes with you..."),str_values[0].c_str());
            type = ACT_NULL;
            break;
        case ACT_CRAFT:
            p->complete_craft();
            type = ACT_NULL;
            break;
        case ACT_LONGCRAFT:
            {
                int batch_size = values.front();
                p->complete_craft();
                type = ACT_NULL;
                // Workaround for a bug where longcraft can be unset in complete_craft().
                if( p->making_would_work( p->lastrecipe, batch_size ) ) {
                    p->make_all_craft( p->lastrecipe, batch_size );
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
            add_msg(m_good, _("You feel much better."));
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
            activity_handlers::pickup_finish(this, p);
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
            if (!index) {
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
            activity_handlers::cracking_finish( this, p);
            type = ACT_NULL;
            break;
        case ACT_REPAIR_ITEM:
            // Unsets activity (if needed) inside function
            activity_handlers::repair_item_finish( this, p );
            break;
        default:
            type = ACT_NULL;
    }
    if (type == ACT_NULL) {
        // Make sure data of previous activity is cleared
        p->activity = player_activity();
        if( !p->backlog.empty() && p->backlog.front().auto_resume ) {
            p->activity = p->backlog.front();
            p->backlog.pop_front();
        }
    }
}
