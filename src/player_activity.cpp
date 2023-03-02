#include "player_activity.h"

#include <algorithm>
#include <memory>
#include <new>

#include "activity_handlers.h"
#include "activity_type.h"
#include "avatar.h"
#include "calendar.h"
#include "character.h"
#include "construction.h"
#include "field.h"
#include "game.h"
#include "item.h"
#include "itype.h"
#include "map.h"
#include "rng.h"
#include "skill.h"
#include "sounds.h"
#include "stomach.h"
#include "string_formatter.h"
#include "translations.h"
#include "ui.h"
#include "units.h"
#include "value_ptr.h"

static const activity_id ACT_ADV_INVENTORY( "ACT_ADV_INVENTORY" );
static const activity_id ACT_AIM( "ACT_AIM" );
static const activity_id ACT_ARMOR_LAYERS( "ACT_ARMOR_LAYERS" );
static const activity_id ACT_ATM( "ACT_ATM" );
static const activity_id ACT_BUILD( "ACT_BUILD" );
static const activity_id ACT_CHOP_LOGS( "ACT_CHOP_LOGS" );
static const activity_id ACT_CHOP_PLANKS( "ACT_CHOP_PLANKS" );
static const activity_id ACT_CHOP_TREE( "ACT_CHOP_TREE" );
static const activity_id ACT_CLEAR_RUBBLE( "ACT_CLEAR_RUBBLE" );
static const activity_id ACT_CONSUME( "ACT_CONSUME" );
static const activity_id ACT_CONSUME_DRINK_MENU( "ACT_CONSUME_DRINK_MENU" );
static const activity_id ACT_CONSUME_FOOD_MENU( "ACT_CONSUME_FOOD_MENU" );
static const activity_id ACT_CONSUME_MEDS_MENU( "ACT_CONSUME_MEDS_MENU" );
static const activity_id ACT_EAT_MENU( "ACT_EAT_MENU" );
static const activity_id ACT_FIRSTAID( "ACT_FIRSTAID" );
static const activity_id ACT_FISH( "ACT_FISH" );
static const activity_id ACT_GAME( "ACT_GAME" );
static const activity_id ACT_GUNMOD_ADD( "ACT_GUNMOD_ADD" );
static const activity_id ACT_HACKSAW( "ACT_HACKSAW" );
static const activity_id ACT_HAND_CRANK( "ACT_HAND_CRANK" );
static const activity_id ACT_HEATING( "ACT_HEATING" );
static const activity_id ACT_JACKHAMMER( "ACT_JACKHAMMER" );
static const activity_id ACT_MIGRATION_CANCEL( "ACT_MIGRATION_CANCEL" );
static const activity_id ACT_NULL( "ACT_NULL" );
static const activity_id ACT_OXYTORCH( "ACT_OXYTORCH" );
static const activity_id ACT_PICKAXE( "ACT_PICKAXE" );
static const activity_id ACT_PICKUP_MENU( "ACT_PICKUP_MENU" );
static const activity_id ACT_READ( "ACT_READ" );
static const activity_id ACT_START_FIRE( "ACT_START_FIRE" );
static const activity_id ACT_TRAVELLING( "ACT_TRAVELLING" );
static const activity_id ACT_VEHICLE( "ACT_VEHICLE" );
static const activity_id ACT_VIBE( "ACT_VIBE" );
static const activity_id ACT_VIEW_RECIPE( "ACT_VIEW_RECIPE" );
static const activity_id ACT_WAIT_STAMINA( "ACT_WAIT_STAMINA" );
static const activity_id ACT_WORKOUT_ACTIVE( "ACT_WORKOUT_ACTIVE" );
static const activity_id ACT_WORKOUT_HARD( "ACT_WORKOUT_HARD" );
static const activity_id ACT_WORKOUT_LIGHT( "ACT_WORKOUT_LIGHT" );
static const activity_id ACT_WORKOUT_MODERATE( "ACT_WORKOUT_MODERATE" );

static const efftype_id effect_nausea( "nausea" );

static const std::vector<activity_id> consuming {
    ACT_CONSUME,
    ACT_EAT_MENU,
    ACT_CONSUME_FOOD_MENU,
    ACT_CONSUME_DRINK_MENU,
    ACT_CONSUME_MEDS_MENU };

constexpr tripoint_abs_ms player_activity::invalid_place;

player_activity::player_activity() : type( activity_id::NULL_ID() ) { }

player_activity::player_activity( activity_id t, int turns, int Index, int pos,
                                  const std::string &name_in ) :
    type( t ), moves_total( turns ), moves_left( turns ),
    index( Index ),
    position( pos ), name( name_in ),
    placement( tripoint_min )
{
}

player_activity::player_activity( const activity_actor &actor ) : type( actor.get_type() ),
    actor( actor.clone() )
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
        targets.emplace_back( guy, &guy.i_at( position ) );
    } else if( type == ACT_GUNMOD_ADD ) {
        // this activity has two indices; "position" = gun and "values[0]" = mod
        targets.emplace_back( guy, &guy.i_at( position ) );
        targets.emplace_back( guy, &guy.i_at( values[0] ) );
    }
}

void player_activity::set_to_null()
{
    type = activity_id::NULL_ID();
    sfx::end_activity_sounds(); // kill activity sounds when activity is nullified
}

void player_activity::synchronize_type_with_actor()
{
    if( actor && type != activity_id::NULL_ID() ) {
        type = actor->get_type();
    }
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

cata::optional<std::string> player_activity::get_progress_message( const avatar &u ) const
{
    if( type == ACT_NULL || get_verb().empty() ) {
        return cata::optional<std::string>();
    }

    if( type == ACT_ADV_INVENTORY ||
        type == ACT_AIM ||
        type == ACT_ARMOR_LAYERS ||
        type == ACT_ATM ||
        type == ACT_CONSUME_DRINK_MENU ||
        type == ACT_CONSUME_FOOD_MENU ||
        type == ACT_CONSUME_MEDS_MENU ||
        type == ACT_EAT_MENU ||
        type == ACT_PICKUP_MENU ||
        type == ACT_VIEW_RECIPE ) {
        return cata::nullopt;
    }

    std::string extra_info;
    if( type == ACT_READ ) {
        if( const item *book = targets.front().get_item() ) {
            if( const auto &reading = book->type->book ) {
                const skill_id &skill = reading->skill;
                if( skill && u.get_knowledge_level( skill ) < reading->level &&
                    u.get_skill_level_object( skill ).can_train() && u.has_identified( book->typeId() ) ) {
                    const SkillLevel &skill_level = u.get_skill_level_object( skill );
                    //~ skill_name current_skill_level -> next_skill_level (% to next level)
                    extra_info = string_format( pgettext( "reading progress", "%s %d -> %d (%d%%)" ),
                                                skill.obj().name(),
                                                skill_level.knowledgeLevel(),
                                                skill_level.knowledgeLevel() + 1,
                                                skill_level.knowledgeExperience() );
                }
            }
        }
    } else if( moves_total > 0 ) {
        if( type == ACT_HACKSAW ||
            type == ACT_JACKHAMMER ||
            type == ACT_PICKAXE ||
            type == ACT_VEHICLE ||
            type == ACT_CHOP_TREE ||
            type == ACT_CHOP_LOGS ||
            type == ACT_CHOP_PLANKS ||
            type == ACT_HEATING
          ) {
            const int percentage = ( ( moves_total - moves_left ) * 100 ) / moves_total;

            extra_info = string_format( "%d%%", percentage );
        }

        if( type == ACT_BUILD ) {
            partial_con *pc =
                get_map().partial_con_at( get_map().bub_from_abs( u.activity.placement ) );
            if( pc ) {
                int counter = std::min( pc->counter, 10000000 );
                const int percentage = counter / 100000;

                extra_info = string_format( "%d%%", percentage );
            }
        }
    }

    if( actor ) {
        extra_info = actor->get_progress_message( *this );
    }

    return extra_info.empty() ? string_format( _( "%s…" ),
            get_verb().translated() ) : string_format( _( "%s: %s" ),
                    get_verb().translated(), extra_info );
}

void player_activity::start_or_resume( Character &who, bool resuming )
{
    if( actor && !resuming ) {
        actor->start( *this, who );
    }
    if( !type.is_null() && rooted() ) {
        who.rooted_message();
    }
    // last, as start function may have changed the type
    synchronize_type_with_actor();
}

void player_activity::do_turn( Character &you )
{
    // Specifically call the do turn function for the cancellation activity early
    // This is because the game can get stuck trying to fuel a fire when it's not...
    if( type == ACT_MIGRATION_CANCEL ) {
        actor->do_turn( *this, you );
        return;
    }
    // first to ensure sync with actor
    synchronize_type_with_actor();
    // Should happen before activity or it may fail due to 0 moves
    if( *this && type->will_refuel_fires() && have_fire ) {
        have_fire = try_fuel_fire( *this, you );
    }
    if( calendar::once_every( 30_minutes ) ) {
        no_food_nearby_for_auto_consume = false;
        no_drink_nearby_for_auto_consume = false;
    }
    // Only do once every two minutes to loosely simulate consume times,
    // the exact amount of time is added correctly below, here we just want to prevent eating something every second
    if( calendar::once_every( 2_minutes ) && *this && !you.is_npc() && type->valid_auto_needs() &&
        !you.has_effect( effect_nausea ) ) {
        if( you.stomach.contains() <= you.stomach.capacity( you ) / 4 && you.get_kcal_percent() < 0.95f &&
            !no_food_nearby_for_auto_consume ) {
            int consume_moves = get_auto_consume_moves( you, true );
            moves_left += consume_moves;
            if( consume_moves == 0 ) {
                no_food_nearby_for_auto_consume = true;
            }
        }
        if( you.get_thirst() > 130 && !no_drink_nearby_for_auto_consume ) {
            int consume_moves = get_auto_consume_moves( you, false );
            moves_left += consume_moves;
            if( consume_moves == 0 ) {
                no_drink_nearby_for_auto_consume = true;
            }
        }
    }
    const float activity_mult = you.exertion_adjusted_move_multiplier( exertion_level() );
    if( type->based_on() == based_on_type::TIME ) {
        if( moves_left >= 100 ) {
            moves_left -= 100 * activity_mult;
            you.moves = 0;
        } else {
            you.moves -= you.moves * moves_left / 100;
            moves_left = 0;
        }
    } else if( type->based_on() == based_on_type::SPEED ) {
        if( you.moves <= moves_left ) {
            moves_left -= you.moves * activity_mult;
            you.moves = 0;
        } else {
            you.moves -= moves_left;
            moves_left = 0;
        }
    }
    int previous_stamina = you.get_stamina();
    if( you.is_npc() && you.check_outbounds_activity( *this ) ) {
        // npc might be operating at the edge of the reality bubble.
        // or just now reloaded back into it, and their activity target might
        // be still unloaded, can cause infinite loops.
        set_to_null();
        you.drop_invalid_inventory();
        return;
    }
    const bool travel_activity = id() == ACT_TRAVELLING;
    you.set_activity_level( exertion_level() );
    // This might finish the activity (set it to null)
    if( actor ) {
        actor->do_turn( *this, you );
    } else {
        // Use the legacy turn function
        type->call_do_turn( this, &you );
    }
    // Activities should never excessively drain stamina.
    // adjusted stamina because
    // autotravel doesn't reduce stamina after do_turn()
    // it just sets a destination, clears the activity, then moves afterwards
    // so set stamina -1 if that is the case
    // to simulate that the next step will surely use up some stamina anyway
    // this is to ensure that resting will occur when traveling overburdened
    const int adjusted_stamina = travel_activity ? you.get_stamina() - 1 : you.get_stamina();
    activity_id act_id = actor ? actor->get_type() : type;
    bool excluded = act_id == ACT_WORKOUT_HARD ||
                    act_id == ACT_WORKOUT_ACTIVE ||
                    act_id == ACT_WORKOUT_MODERATE ||
                    act_id == ACT_WORKOUT_LIGHT;
    if( !excluded && adjusted_stamina < previous_stamina &&
        you.get_stamina() < you.get_stamina_max() / 3 ) {
        if( one_in( 50 ) ) {
            you.add_msg_if_player( _( "You pause for a moment to catch your breath." ) );
        }

        auto_resume = true;
        player_activity new_act( ACT_WAIT_STAMINA, to_moves<int>( 5_minutes ) );
        new_act.values.push_back( you.get_stamina_max() );
        if( you.is_avatar() && !ignoreQuery ) {
            uilist tired_query;
            tired_query.text = _( "You struggle to continue.  Keep trying?" );
            tired_query.addentry( 1, true, 'c', _( "Continue after a break." ) );
            tired_query.addentry( 2, true, 'm', _( "Maybe later." ) );
            tired_query.addentry( 3, true, 'f', _( "Finish it." ) );
            tired_query.query();
            switch( tired_query.ret ) {
                case UILIST_CANCEL:
                case 2:
                    auto_resume = false;
                    set_to_null();
                    break;
                case 3:
                    ignoreQuery = true;
                    break;
                default:
                    break;
            }
        }
        if( !ignoreQuery && auto_resume ) {
            you.assign_activity( new_act );
        }
        return;
    }
    if( *this && type->rooted() ) {
        you.rooted();
        you.pause();
    }

    if( *this && moves_left <= 0 ) {
        // Note: For some activities "finish" is a misnomer; that's why we explicitly check if the
        // type is ACT_NULL below.
        if( actor ) {
            actor->finish( *this, you );
        } else {
            if( !type->call_finish( this, &you ) ) {
                // "Finish" is never a misnomer for any activity without a finish function
                set_to_null();
            }
        }
    }
    if( !*this ) {
        // Make sure data of previous activity is cleared
        you.activity = player_activity();
        you.resume_backlog_activity();

        // If whatever activity we were doing forced us to pick something up to
        // handle it, drop any overflow that may have caused
        you.drop_invalid_inventory();
    }
}

void player_activity::canceled( Character &who )
{
    if( *this && actor ) {
        actor->canceled( *this, who );
    }
}

float player_activity::exertion_level() const
{
    if( actor ) {
        return actor->exertion_level();
    }
    return type->exertion_level();
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

    if( !*this || !other || type->no_resume() ) {
        return false;
    }

    if( id() != other.id() ) {
        return false;
    }

    // if actor XOR other.actor then id() != other.id() so
    // we will correctly return false based on final return statement
    if( actor && other.actor ) {
        return actor->can_resume_with( *other.actor, who );
    }

    if( id() == ACT_CLEAR_RUBBLE ) {
        if( other.coords.empty() || other.coords[0] != coords[0] ) {
            return false;
        }
    } else if( id() == ACT_VEHICLE ) {
        if( values != other.values || str_values != other.str_values ) {
            return false;
        }
    }

    return !auto_resume && index == other.index &&
           position == other.position && name == other.name && targets == other.targets;
}

bool player_activity::is_interruptible() const
{
    return ( type.is_null() || type->interruptable() ) && interruptable;
}

bool player_activity::is_interruptible_with_kb() const
{
    return ( type.is_null() || type->interruptable_with_kb() ) && interruptable_with_kb;
}

bool player_activity::is_distraction_ignored( distraction_type distraction ) const
{
    return !is_interruptible() ||
           ignored_distractions.find( distraction ) != ignored_distractions.end();
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
    for( const distraction_type &type : other.ignored_distractions ) {
        ignore_distraction( type );
    }
}

std::map<distraction_type, std::string> player_activity::get_distractions() const
{
    std::map < distraction_type, std::string > res;
    activity_id act_id = id();
    if( act_id != ACT_AIM && moves_left > 0 ) {
        if( uistate.distraction_hostile_close &&
            !is_distraction_ignored( distraction_type::hostile_spotted_near ) ) {
            Creature *hostile_critter = g->is_hostile_very_close( true );
            if( hostile_critter != nullptr ) {
                res.emplace( distraction_type::hostile_spotted_near,
                             string_format( _( "The %s is dangerously close!" ),
                                            g->is_hostile_very_close( true )->get_name() ) );
            }
        }
        if( uistate.distraction_dangerous_field &&
            !is_distraction_ignored( distraction_type::dangerous_field ) ) {
            field_entry *field = g->is_in_dangerous_field();
            if( field != nullptr ) {
                res.emplace( distraction_type::dangerous_field, string_format( _( "You stand in %s!" ),
                             g->is_in_dangerous_field()->name() ) );
            }
        }
        // Nested in the !ACT_AIM to avoid nuisance during combat
        // If this is too bothersome, maybe a list of just ACT_CRAFT, ACT_DIG etc
        if( std::find( consuming.begin(), consuming.end(), act_id ) == consuming.end() ) {
            avatar &player_character = get_avatar();
            if( uistate.distraction_hunger &&
                !is_distraction_ignored( distraction_type::hunger ) ) {
                // Starvation value of 5300 equates to about 5kCal.
                if( calendar::once_every( 2_hours ) && player_character.get_hunger() >= 300 &&
                    player_character.get_starvation() > 5300 ) {
                    res.emplace( distraction_type::hunger, _( "You are at risk of starving!" ) );
                }
            }
            if( uistate.distraction_thirst &&
                !is_distraction_ignored( distraction_type::thirst ) ) {
                if( player_character.get_thirst() > 520 ) {
                    res.emplace( distraction_type::thirst, _( "You are dangerously dehydrated!" ) );
                }
            }
        }
        if( uistate.distraction_temperature && !is_distraction_ignored( distraction_type::temperature ) ) {
            for( const bodypart_id &bp : get_avatar().get_all_body_parts() ) {
                if( get_avatar().get_part_temp_cur( bp ) > BODYTEMP_VERY_HOT ) {
                    res.emplace( distraction_type::temperature, _( "You are overheating!" ) );
                    break;
                } else if( get_avatar().get_part_temp_cur( bp ) < BODYTEMP_VERY_COLD ) {
                    res.emplace( distraction_type::temperature, _( "You are freezing!" ) );
                    break;
                }
            }
        }
    }

    return res;
}
