#include "player_activity.h"

#include <algorithm>
#include <array>
#include <memory>
#include <utility>

#include "activity_handlers.h"
#include "activity_type.h"
#include "avatar.h"
#include "calendar.h"
#include "character.h"
#include "color.h"
#include "compatibility.h"
#include "construction.h"
#include "crafting.h"
#include "game.h"
#include "item.h"
#include "itype.h"
#include "map.h"
#include "player.h"
#include "recipe.h"
#include "rng.h"
#include "skill.h"
#include "sounds.h"
#include "string_formatter.h"
#include "string_id.h"
#include "translations.h"
#include "value_ptr.h"

static const activity_id ACT_FIRSTAID( "ACT_FIRSTAID" );
static const activity_id ACT_GAME( "ACT_GAME" );
static const activity_id ACT_PICKAXE( "ACT_PICKAXE" );
static const activity_id ACT_START_FIRE( "ACT_START_FIRE" );
static const activity_id ACT_HAND_CRANK( "ACT_HAND_CRANK" );
static const activity_id ACT_VIBE( "ACT_VIBE" );
static const activity_id ACT_OXYTORCH( "ACT_OXYTORCH" );
static const activity_id ACT_FISH( "ACT_FISH" );
static const activity_id ACT_ATM( "ACT_ATM" );
static const activity_id ACT_GUNMOD_ADD( "ACT_GUNMOD_ADD" );

player_activity::player_activity() : type( activity_id::NULL_ID() ) { }

player_activity::player_activity( activity_id t, int turns, int Index, int pos,
                                  const std::string &name_in ) :
    type( t ), moves_total( turns ), moves_left( turns ),
    index( Index ),
    position( pos ), name( name_in ),
    placement( tripoint_min ), auto_resume( false )
{
}

player_activity::player_activity( const activity_actor &actor ) : type( actor.get_type() ),
    actor( actor.clone() ), moves_total( 0 ), moves_left( 0 )
{
}

void player_activity::migrate_item_position( Character &guy )
{
    const bool simple_action_replace =
        type == ACT_FIRSTAID || type == ACT_GAME ||
        type == ACT_PICKAXE || type == ACT_START_FIRE ||
        type == ACT_HAND_CRANK || type == ACT_VIBE ||
        type == ACT_OXYTORCH || type == ACT_FISH ||
        type == ACT_ATM;

    if( simple_action_replace ) {
        targets.push_back( item_location( guy, &guy.i_at( position ) ) );
    } else if( type == ACT_GUNMOD_ADD ) {
        // this activity has two indices; "position" = gun and "values[0]" = mod
        targets.push_back( item_location( guy, &guy.i_at( position ) ) );
        targets.push_back( item_location( guy, &guy.i_at( values[0] ) ) );
    }
}

void player_activity::set_to_null()
{
    type = activity_id::NULL_ID();
    sfx::end_activity_sounds(); // kill activity sounds when activity is nullified
}

bool player_activity::rooted() const
{
    return type->rooted();
}

std::string player_activity::get_stop_phrase() const
{
    return type->stop_phrase();
}

const translation &player_activity::get_verb() const
{
    return type->verb();
}

int player_activity::get_value( size_t index, int def ) const
{
    return index < values.size() ? values[index] : def;
}

bool player_activity::is_suspendable() const
{
    return type->suspendable();
}

bool player_activity::is_multi_type() const
{
    return type->multi_activity();
}

std::string player_activity::get_str_value( size_t index, const std::string &def ) const
{
    return index < str_values.size() ? str_values[index] : def;
}

static std::string craft_progress_message( const avatar &u, const player_activity &act )
{
    const item *craft = act.targets.front().get_item();
    if( craft == nullptr ) {
        // Should never happen (?)
        return string_format( _( "%s…" ), act.get_verb().translated() );
    }

    // Horrid copypaste warning! TODO: Functions
    const recipe &rec = craft->get_making();
    const tripoint bench_pos = act.coords.front();
    // Ugly
    bench_type bench_t = bench_type( act.values[1] );

    const bench_location bench{bench_t, bench_pos};

    const float light_mult = u.lighting_craft_speed_multiplier( rec );
    const float bench_mult = workbench_crafting_speed_multiplier( *craft, bench );
    const float morale_mult = u.morale_crafting_speed_multiplier( rec );
    const int assistants = u.available_assistant_count( craft->get_making() );
    const float base_total_moves = std::max( 1, rec.batch_time( craft->charges, 1.0f, 0 ) );
    const float assist_total_moves = std::max( 1, rec.batch_time( craft->charges, 1.0f, assistants ) );
    const float assist_mult = base_total_moves / assist_total_moves;
    const float speed_mult = u.get_speed() / 100.0f;
    const float total_mult = light_mult * bench_mult * morale_mult * assist_mult * speed_mult;

    const double remaining_percentage = 1.0 - craft->item_counter / 10'000'000.0;
    int remaining_turns = remaining_percentage * base_total_moves / 100 / std::max( 0.01f, total_mult );
    std::string time_desc = string_format( _( "Time left: %s" ),
                                           to_string( time_duration::from_turns( remaining_turns ) ) );

    const std::array<std::pair<float, std::string>, 6> mults_with_data = {{
            { total_mult, _( "Total" ) },
            { speed_mult, _( "Speed" ) },
            { light_mult, _( "Light" ) },
            { bench_mult, _( "Workbench" ) },
            { morale_mult, _( "Morale" ) },
            { assist_mult, _( "Assistants" ) },
        }
    };
    std::string mults_desc = _( "Crafting speed multipliers:\n" );
    // Hack to make sure total always shows
    bool first = true;
    for( const std::pair<float, std::string> &p : mults_with_data ) {
        int percent = static_cast<int>( p.first * 100 );
        if( first || percent != 100 ) {
            nc_color col = percent > 100 ? c_green : c_red;
            std::string colorized = colorize( to_string( percent ) + '%', col );
            mults_desc += string_format( _( "%s: %s\n" ), p.second, colorized );
        }
        first = false;
    }

    return string_format( _( "%s: %s\n\n%s\n\n%s" ), act.get_verb().translated(), craft->tname(),
                          time_desc,
                          mults_desc );
}

cata::optional<std::string> player_activity::get_progress_message( const avatar &u ) const
{
    if( type == activity_id( "ACT_NULL" ) || get_verb().empty() ) {
        return cata::optional<std::string>();
    }

    std::string extra_info;
    if( type == activity_id( "ACT_CRAFT" ) ) {
        return craft_progress_message( u, *this );
    } else if( type == activity_id( "ACT_READ" ) ) {
        if( const item *book = targets.front().get_item() ) {
            if( const auto &reading = book->type->book ) {
                const skill_id &skill = reading->skill;
                if( skill && u.get_skill_level( skill ) < reading->level &&
                    u.get_skill_level_object( skill ).can_train() ) {
                    const SkillLevel &skill_level = u.get_skill_level_object( skill );
                    //~ skill_name current_skill_level -> next_skill_level (% to next level)
                    extra_info = string_format( pgettext( "reading progress", "%s %d -> %d (%d%%)" ),
                                                skill.obj().name(),
                                                skill_level.level(),
                                                skill_level.level() + 1,
                                                skill_level.exercise() );
                }
            }
        }
    } else if( moves_total > 0 ) {
        if( type == activity_id( "ACT_BURROW" ) ||
            type == activity_id( "ACT_HACKSAW" ) ||
            type == activity_id( "ACT_JACKHAMMER" ) ||
            type == activity_id( "ACT_PICKAXE" ) ||
            type == activity_id( "ACT_DISASSEMBLE" ) ||
            type == activity_id( "ACT_FILL_PIT" ) ||
            type == activity_id( "ACT_DIG" ) ||
            type == activity_id( "ACT_DIG_CHANNEL" ) ||
            type == activity_id( "ACT_CHOP_TREE" ) ||
            type == activity_id( "ACT_CHOP_LOGS" ) ||
            type == activity_id( "ACT_CHOP_PLANKS" )
          ) {
            const int percentage = ( ( moves_total - moves_left ) * 100 ) / moves_total;

            extra_info = string_format( "%d%%", percentage );
        }

        if( type == activity_id( "ACT_BUILD" ) ) {
            partial_con *pc = g->m.partial_con_at( g->m.getlocal( u.activity.placement ) );
            if( pc ) {
                int counter = std::min( pc->counter, 10000000 );
                const int percentage = counter / 100000;

                extra_info = string_format( "%d%%", percentage );
            }
        }
    }

    return extra_info.empty() ? string_format( _( "%s…" ),
            get_verb().translated() ) : string_format( _( "%s: %s" ),
                    get_verb().translated(), extra_info );
}

void player_activity::start( Character &who )
{
    if( actor ) {
        actor->start( *this, who );
    }
    if( rooted() ) {
        who.rooted_message();
    }
}

void player_activity::do_turn( player &p )
{
    // Should happen before activity or it may fail du to 0 moves
    if( *this && type->will_refuel_fires() ) {
        try_fuel_fire( *this, p );
    }
    if( calendar::once_every( 30_minutes ) ) {
        no_food_nearby_for_auto_consume = false;
        no_drink_nearby_for_auto_consume = false;
    }
    if( *this && !p.is_npc() && type->valid_auto_needs() && !no_food_nearby_for_auto_consume ) {
        if( p.get_kcal_percent() < 0.95f ) {
            if( !find_auto_consume( p, true ) ) {
                no_food_nearby_for_auto_consume = true;
            }
        }
        if( p.get_thirst() > thirst_levels::thirsty && !no_drink_nearby_for_auto_consume ) {
            if( !find_auto_consume( p, false ) ) {
                no_drink_nearby_for_auto_consume = true;
            }
        }
    }
    if( type->based_on() == based_on_type::TIME ) {
        if( moves_left >= 100 ) {
            moves_left -= 100;
            p.moves = 0;
        } else {
            p.moves -= p.moves * moves_left / 100;
            moves_left = 0;
        }
    } else if( type->based_on() == based_on_type::SPEED ) {
        if( p.moves <= moves_left ) {
            moves_left -= p.moves;
            p.moves = 0;
        } else {
            p.moves -= moves_left;
            moves_left = 0;
        }
    }
    int previous_stamina = p.get_stamina();
    if( p.is_npc() && p.check_outbounds_activity( *this ) ) {
        // npc might be operating at the edge of the reality bubble.
        // or just now reloaded back into it, and their activity target might
        // be still unloaded, can cause infinite loops.
        set_to_null();
        p.drop_invalid_inventory();
        return;
    }
    const bool travel_activity = id() == "ACT_TRAVELLING";
    // This might finish the activity (set it to null)
    if( actor ) {
        actor->do_turn( *this, p );
    } else {
        // Use the legacy turn function
        type->call_do_turn( this, &p );
    }
    // Activities should never excessively drain stamina.
    // adjusted stamina because
    // autotravel doesn't reduce stamina after do_turn()
    // it just sets a destination, clears the activity, then moves afterwards
    // so set stamina -1 if that is the case
    // to simulate that the next step will surely use up some stamina anyway
    // this is to ensure that resting will occur when traveling overburdened
    const int adjusted_stamina = travel_activity ? p.get_stamina() - 1 : p.get_stamina();
    if( adjusted_stamina < previous_stamina && p.get_stamina() < p.get_stamina_max() / 3 ) {
        if( one_in( 50 ) ) {
            p.add_msg_if_player( _( "You pause for a moment to catch your breath." ) );
        }
        auto_resume = true;
        player_activity new_act( activity_id( "ACT_WAIT_STAMINA" ), to_moves<int>( 1_minutes ) );
        new_act.values.push_back( 200 + p.get_stamina_max() / 3 );
        p.assign_activity( new_act );
        return;
    }
    if( *this && type->rooted() ) {
        p.rooted();
        p.pause();
    }

    if( *this && moves_left <= 0 ) {
        // Note: For some activities "finish" is a misnomer; that's why we explicitly check if the
        // type is ACT_NULL below.
        if( actor ) {
            actor->finish( *this, p );
        } else {
            if( !type->call_finish( this, &p ) ) {
                // "Finish" is never a misnomer for any activity without a finish function
                set_to_null();
            }
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

    // TODO: Once activity_handler_actors exist, the less ugly method of using a
    // pure virtual can_resume_with should be used

    if( !*this || !other || type->no_resume() ) {
        return false;
    }

    if( id() == activity_id( "ACT_CLEAR_RUBBLE" ) ) {
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
    } else if( id() == activity_id( "ACT_DIG" ) || id() == activity_id( "ACT_DIG_CHANNEL" ) ) {
        // We must be digging in the same location.
        if( placement != other.placement ) {
            return false;
        }

        // And all our parameters must be the same.
        if( !std::equal( values.begin(), values.end(), other.values.begin() ) ) {
            return false;
        }

        if( !std::equal( str_values.begin(), str_values.end(), other.str_values.begin() ) ) {
            return false;
        }

        if( !std::equal( coords.begin(), coords.end(), other.coords.begin() ) ) {
            return false;
        }
    }

    return !auto_resume && id() == other.id() && index == other.index &&
           position == other.position && name == other.name && targets == other.targets;
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
