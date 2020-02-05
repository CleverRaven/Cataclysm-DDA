#include "avatar.h"

#include <climits>
#include <cstdlib>
#include <algorithm>
#include <list>
#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <tuple>
#include <unordered_map>
#include <utility>

#include "action.h"
#include "bionics.h"
#include "calendar.h"
#include "character.h"
#include "effect.h"
#include "enums.h"
#include "filesystem.h"
#include "game.h"
#include "help.h"
#include "inventory.h"
#include "item.h"
#include "itype.h"
#include "kill_tracker.h"
#include "map.h"
#include "martialarts.h"
#include "messages.h"
#include "mission.h"
#include "monstergenerator.h"
#include "morale.h"
#include "morale_types.h"
#include "mutation.h"
#include "npc.h"
#include "options.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "player.h"
#include "profession.h"
#include "skill.h"
#include "type_id.h"
#include "get_version.h"
#include "ui.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "color.h"
#include "compatibility.h"
#include "debug.h"
#include "game_constants.h"
#include "item_location.h"
#include "iuse.h"
#include "mtype.h"
#include "optional.h"
#include "output.h"
#include "pathfinding.h"
#include "pimpl.h"
#include "player_activity.h"
#include "rng.h"
#include "stomach.h"
#include "string_formatter.h"
#include "string_id.h"
#include "translations.h"
#include "units.h"
#include "cata_string_consts.h"

class JsonIn;
class JsonOut;

avatar::avatar()
{
    show_map_memory = true;
    active_mission = nullptr;
    grab_type = OBJECT_NONE;
}

void avatar::toggle_map_memory()
{
    show_map_memory = !show_map_memory;
}

bool avatar::should_show_map_memory()
{
    return show_map_memory;
}

void avatar::serialize_map_memory( JsonOut &jsout ) const
{
    player_map_memory.store( jsout );
}

void avatar::deserialize_map_memory( JsonIn &jsin )
{
    player_map_memory.load( jsin );
}

memorized_terrain_tile avatar::get_memorized_tile( const tripoint &pos ) const
{
    return player_map_memory.get_tile( pos );
}

void avatar::memorize_tile( const tripoint &pos, const std::string &ter, const int subtile,
                            const int rotation )
{
    player_map_memory.memorize_tile( max_memorized_tiles(), pos, ter, subtile, rotation );
}

void avatar::memorize_symbol( const tripoint &pos, const int symbol )
{
    player_map_memory.memorize_symbol( max_memorized_tiles(), pos, symbol );
}

int avatar::get_memorized_symbol( const tripoint &p ) const
{
    return player_map_memory.get_symbol( p );
}

size_t avatar::max_memorized_tiles() const
{
    // Only check traits once a turn since this is called a huge number of times.
    if( current_map_memory_turn != calendar::turn ) {
        current_map_memory_turn = calendar::turn;
        float map_memory_capacity_multiplier =
            mutation_value( "map_memory_capacity_multiplier" );
        if( has_active_bionic( bio_memory ) ) {
            map_memory_capacity_multiplier = 50;
        }
        current_map_memory_capacity = 2 * SEEX * 2 * SEEY * 100 * map_memory_capacity_multiplier;
    }
    return current_map_memory_capacity;
}

void avatar::clear_memorized_tile( const tripoint &pos )
{
    player_map_memory.clear_memorized_tile( pos );
}

std::vector<mission *> avatar::get_active_missions() const
{
    return active_missions;
}

std::vector<mission *> avatar::get_completed_missions() const
{
    return completed_missions;
}

std::vector<mission *> avatar::get_failed_missions() const
{
    return failed_missions;
}

mission *avatar::get_active_mission() const
{
    return active_mission;
}

void avatar::reset_all_misions()
{
    active_mission = nullptr;
    active_missions.clear();
    completed_missions.clear();
    failed_missions.clear();
}

tripoint avatar::get_active_mission_target() const
{
    if( active_mission == nullptr ) {
        return overmap::invalid_tripoint;
    }
    return active_mission->get_target();
}

void avatar::set_active_mission( mission &cur_mission )
{
    const auto iter = std::find( active_missions.begin(), active_missions.end(), &cur_mission );
    if( iter == active_missions.end() ) {
        debugmsg( "new active mission %d is not in the active_missions list", cur_mission.get_id() );
    } else {
        active_mission = &cur_mission;
    }
}

void avatar::on_mission_assignment( mission &new_mission )
{
    active_missions.push_back( &new_mission );
    set_active_mission( new_mission );
}

void avatar::on_mission_finished( mission &cur_mission )
{
    if( cur_mission.has_failed() ) {
        failed_missions.push_back( &cur_mission );
        add_msg_if_player( m_bad, _( "Mission \"%s\" is failed." ), cur_mission.name() );
    } else {
        completed_missions.push_back( &cur_mission );
        add_msg_if_player( m_good, _( "Mission \"%s\" is successfully completed." ),
                           cur_mission.name() );
    }
    const auto iter = std::find( active_missions.begin(), active_missions.end(), &cur_mission );
    if( iter == active_missions.end() ) {
        debugmsg( "completed mission %d was not in the active_missions list", cur_mission.get_id() );
    } else {
        active_missions.erase( iter );
    }
    if( &cur_mission == active_mission ) {
        if( active_missions.empty() ) {
            active_mission = nullptr;
        } else {
            active_mission = active_missions.front();
        }
    }
}

const player *avatar::get_book_reader( const item &book, std::vector<std::string> &reasons ) const
{
    const player *reader = nullptr;
    if( !book.is_book() ) {
        reasons.push_back( string_format( _( "Your %s is not good reading material." ),
                                          book.tname() ) );
        return nullptr;
    }

    // Check for conditions that immediately disqualify the player from reading:
    const optional_vpart_position vp = g->m.veh_at( pos() );
    if( vp && vp->vehicle().player_in_control( *this ) ) {
        reasons.emplace_back( _( "It's a bad idea to read while driving!" ) );
        return nullptr;
    }
    const auto &type = book.type->book;
    if( !fun_to_read( book ) && !has_morale_to_read() && has_identified( book.typeId() ) ) {
        // Low morale still permits skimming
        reasons.emplace_back( _( "What's the point of studying?  (Your morale is too low!)" ) );
        return nullptr;
    }
    const skill_id &skill = type->skill;
    const int skill_level = get_skill_level( skill );
    if( skill && skill_level < type->req && has_identified( book.typeId() ) ) {
        reasons.push_back( string_format( _( "%s %d needed to understand.  You have %d" ),
                                          skill.obj().name(), type->req, skill_level ) );
        return nullptr;
    }

    // Check for conditions that disqualify us only if no NPCs can read to us
    if( type->intel > 0 && has_trait( trait_ILLITERATE ) ) {
        reasons.emplace_back( _( "You're illiterate!" ) );
    } else if( has_trait( trait_HYPEROPIC ) && !worn_with_flag( flag_FIX_FARSIGHT ) &&
               !has_effect( effect_contacts ) && !has_bionic( bio_eye_optic ) ) {
        reasons.emplace_back( _( "Your eyes won't focus without reading glasses." ) );
    } else if( fine_detail_vision_mod() > 4 ) {
        // Too dark to read only applies if the player can read to himself
        reasons.emplace_back( _( "It's too dark to read!" ) );
        return nullptr;
    } else {
        return this;
    }

    //Check for NPCs to read for you, negates Illiterate and Far Sighted
    //The fastest-reading NPC is chosen
    if( is_deaf() ) {
        reasons.emplace_back( _( "Maybe someone could read that to you, but you're deaf!" ) );
        return nullptr;
    }

    int time_taken = INT_MAX;
    auto candidates = get_crafting_helpers();

    for( const npc *elem : candidates ) {
        // Check for disqualifying factors:
        if( type->intel > 0 && elem->has_trait( trait_ILLITERATE ) ) {
            reasons.push_back( string_format( _( "%s is illiterate!" ),
                                              elem->disp_name() ) );
        } else if( skill && elem->get_skill_level( skill ) < type->req &&
                   has_identified( book.typeId() ) ) {
            reasons.push_back( string_format( _( "%s %d needed to understand.  %s has %d" ),
                                              skill.obj().name(), type->req, elem->disp_name(), elem->get_skill_level( skill ) ) );
        } else if( elem->has_trait( trait_HYPEROPIC ) && !elem->worn_with_flag( flag_FIX_FARSIGHT ) &&
                   !elem->has_effect( effect_contacts ) ) {
            reasons.push_back( string_format( _( "%s needs reading glasses!" ),
                                              elem->disp_name() ) );
        } else if( std::min( fine_detail_vision_mod(), elem->fine_detail_vision_mod() ) > 4 ) {
            reasons.push_back( string_format(
                                   _( "It's too dark for %s to read!" ),
                                   elem->disp_name() ) );
        } else if( !elem->sees( *this ) ) {
            reasons.push_back( string_format( _( "%s could read that to you, but they can't see you." ),
                                              elem->disp_name() ) );
        } else if( !elem->fun_to_read( book ) && !elem->has_morale_to_read() &&
                   has_identified( book.typeId() ) ) {
            // Low morale still permits skimming
            reasons.push_back( string_format( _( "%s morale is too low!" ), elem->disp_name( true ) ) );
        } else if( elem->is_blind() ) {
            reasons.push_back( string_format( _( "%s is blind." ), elem->disp_name() ) );
        } else {
            int proj_time = time_to_read( book, *elem );
            if( proj_time < time_taken ) {
                reader = elem;
                time_taken = proj_time;
            }
        }
    }
    //end for all candidates
    return reader;
}

int avatar::time_to_read( const item &book, const player &reader, const player *learner ) const
{
    const auto &type = book.type->book;
    const skill_id &skill = type->skill;
    // The reader's reading speed has an effect only if they're trying to understand the book as they read it
    // Reading speed is assumed to be how well you learn from books (as opposed to hands-on experience)
    const bool try_understand = reader.fun_to_read( book ) ||
                                reader.get_skill_level( skill ) < type->level;
    int reading_speed = try_understand ? std::max( reader.read_speed(), read_speed() ) : read_speed();
    if( learner ) {
        reading_speed = std::max( reading_speed, learner->read_speed() );
    }

    int retval = type->time * reading_speed;
    retval *= std::min( fine_detail_vision_mod(), reader.fine_detail_vision_mod() );

    const int effective_int = std::min( { get_int(), reader.get_int(), learner ? learner->get_int() : INT_MAX } );
    if( type->intel > effective_int && !reader.has_trait( trait_PROF_DICEMASTER ) ) {
        retval += type->time * ( type->intel - effective_int ) * 100;
    }
    if( !has_identified( book.typeId() ) ) {
        //skimming
        retval /= 10;
    }
    return retval;
}

/**
 * Explanation of ACT_READ activity values:
 *
 * position: ID of the reader
 * targets: 1-element vector with the item_location (always in inventory/wielded) of the book being read
 * index: We are studying until the player with this ID gains a level; 0 indicates reading once
 * values: IDs of the NPCs who will learn something
 * str_values: Parallel to values, these contain the learning penalties (as doubles in string form) as follows:
 *             Experience gained = Experience normally gained * penalty
 */
bool avatar::read( item &it, const bool continuous )
{
    if( it.is_null() ) {
        add_msg( m_info, _( "Never mind." ) );
        return false;
    }
    std::vector<std::string> fail_messages;
    const player *reader = get_book_reader( it, fail_messages );
    if( reader == nullptr ) {
        // We can't read, and neither can our followers
        for( const std::string &reason : fail_messages ) {
            add_msg( m_bad, reason );
        }
        return false;
    }
    const int time_taken = time_to_read( it, *reader );

    add_msg( m_debug, "avatar::read: time_taken = %d", time_taken );
    player_activity act( ACT_READ, time_taken, continuous ? activity.index : 0,
                         reader->getID().get_value() );
    act.targets.emplace_back( item_location( *this, &it ) );

    // If the player hasn't read this book before, skim it to get an idea of what's in it.
    if( !has_identified( it.typeId() ) ) {
        if( reader != this ) {
            add_msg( m_info, fail_messages[0] );
            add_msg( m_info, _( "%s reads aloud…" ), reader->disp_name() );
        }
        assign_activity( act );
        return true;
    }

    if( it.typeId() == "guidebook" ) {
        // special guidebook effect: print a misc. hint when read
        if( reader != this ) {
            add_msg( m_info, fail_messages[0] );
            dynamic_cast<const npc *>( reader )->say( get_hint() );
        } else {
            add_msg( m_info, get_hint() );
        }
        mod_moves( -100 );
        return false;
    }

    const auto &type = it.type->book;
    const skill_id &skill = type->skill;
    const std::string skill_name = skill ? skill.obj().name() : "";

    // Find NPCs to join the study session:
    std::map<npc *, std::string> learners;
    //reading only for fun
    std::map<npc *, std::string> fun_learners;
    std::map<npc *, std::string> nonlearners;
    auto candidates = get_crafting_helpers();
    for( npc *elem : candidates ) {
        const int lvl = elem->get_skill_level( skill );
        const bool skill_req = ( elem->fun_to_read( it ) && ( !skill || lvl >= type->req ) ) ||
                               ( skill && lvl < type->level && lvl >= type->req );
        const bool morale_req = elem->fun_to_read( it ) || elem->has_morale_to_read();

        if( !skill_req && elem != reader ) {
            if( skill && lvl < type->req ) {
                nonlearners.insert( { elem, string_format( _( " (needs %d %s)" ), type->req, skill_name ) } );
            } else if( skill ) {
                nonlearners.insert( { elem, string_format( _( " (already has %d %s)" ), type->level, skill_name ) } );
            } else {
                nonlearners.insert( { elem, _( " (uninterested)" ) } );
            }
        } else if( elem->is_deaf() && reader != elem ) {
            nonlearners.insert( { elem, _( " (deaf)" ) } );
        } else if( !morale_req ) {
            nonlearners.insert( { elem, _( " (too sad)" ) } );
        } else if( skill && lvl < type->level ) {
            const double penalty = static_cast<double>( time_taken ) / time_to_read( it, *reader, elem );
            learners.insert( {elem, elem == reader ? _( " (reading aloud to you)" ) : ""} );
            act.values.push_back( elem->getID().get_value() );
            act.str_values.push_back( to_string( penalty ) );
        } else {
            fun_learners.insert( {elem, elem == reader ? _( " (reading aloud to you)" ) : "" } );
            act.values.push_back( elem->getID().get_value() );
            act.str_values.emplace_back( "1" );
        }
    }

    if( !continuous ) {
        //only show the menu if there's useful information or multiple options
        if( skill || !nonlearners.empty() || !fun_learners.empty() ) {
            uilist menu;

            // Some helpers to reduce repetition:
            auto length = []( const std::pair<npc *, std::string> &elem ) {
                return utf8_width( elem.first->disp_name() ) + utf8_width( elem.second );
            };

            auto max_length = [&length]( const std::map<npc *, std::string> &m ) {
                auto max_ele = std::max_element( m.begin(),
                                                 m.end(), [&length]( const std::pair<npc *, std::string> &left,
                const std::pair<npc *, std::string> &right ) {
                    return length( left ) < length( right );
                } );
                return max_ele == m.end() ? 0 : length( *max_ele );
            };

            auto get_text =
            [&]( const std::map<npc *, std::string> &m, const std::pair<npc *, std::string> &elem ) {
                const int lvl = elem.first->get_skill_level( skill );
                const std::string lvl_text = skill ? string_format( _( " | current level: %d" ), lvl ) : "";
                const std::string name_text = elem.first->disp_name() + elem.second;
                return string_format( "%s%s", left_justify( name_text, max_length( m ) ), lvl_text );
            };

            auto add_header = [&menu]( const std::string & str ) {
                menu.addentry( -1, false, -1, "" );
                uilist_entry header( -1, false, -1, str, c_yellow, c_yellow );
                header.force_color = true;
                menu.entries.push_back( header );
            };

            menu.title = !skill ? string_format( _( "Reading %s" ), it.type_name() ) :
                         //~ %1$s: book name, %2$s: skill name, %3$d and %4$d: skill levels
                         string_format( _( "Reading %1$s (can train %2$s from %3$d to %4$d)" ), it.type_name(),
                                        skill_name, type->req, type->level );

            if( skill ) {
                const int lvl = get_skill_level( skill );
                menu.addentry( getID().get_value(), lvl < type->level, '0',
                               string_format( _( "Read until you gain a level | current level: %d" ), lvl ) );
            } else {
                menu.addentry( -1, false, '0', _( "Read until you gain a level" ) );
            }
            menu.addentry( 0, true, '1', _( "Read once" ) );

            if( skill && !learners.empty() ) {
                add_header( _( "Read until this NPC gains a level:" ) );
                for( const auto &elem : learners ) {
                    menu.addentry( elem.first->getID().get_value(), true, -1,
                                   get_text( learners, elem ) );
                }
            }
            if( !fun_learners.empty() ) {
                add_header( _( "Reading for fun:" ) );
                for( const auto &elem : fun_learners ) {
                    menu.addentry( -1, false, -1, get_text( fun_learners, elem ) );
                }
            }
            if( !nonlearners.empty() ) {
                add_header( _( "Not participating:" ) );
                for( const auto &elem : nonlearners ) {
                    menu.addentry( -1, false, -1, get_text( nonlearners, elem ) );
                }
            }

            menu.query( true );
            if( menu.ret == UILIST_CANCEL ) {
                add_msg( m_info, _( "Never mind." ) );
                return false;
            }
            act.index = menu.ret;
        }
        if( it.type->use_methods.count( "MA_MANUAL" ) ) {

            if( g->u.martial_arts_data.has_martialart( martial_art_learned_from( *it.type ) ) ) {
                g->u.add_msg_if_player( m_info, _( "You already know all this book has to teach." ) );
                activity.set_to_null();
                return false;
            }

            uilist menu;
            menu.title = string_format( _( "Train %s from manual:" ),
                                        martial_art_learned_from( *it.type )->name );
            menu.addentry( -1, true, 1, _( "Train once." ) );
            menu.addentry( getID().get_value(), true, 2, _( "Train until tired or success." ) );
            menu.query( true );
            if( menu.ret == UILIST_CANCEL ) {
                add_msg( m_info, _( "Never mind." ) );
                return false;
            }
            act.index = menu.ret;
        }
        add_msg( m_info, _( "Now reading %s, %s to stop early." ),
                 it.type_name(), press_x( ACTION_PAUSE ) );
    }

    // Print some informational messages, but only the first time or if the information changes

    if( !continuous || activity.position != act.position ) {
        if( reader != this ) {
            add_msg( m_info, fail_messages[0] );
            add_msg( m_info, _( "%s reads aloud…" ), reader->disp_name() );
        } else if( !learners.empty() || !fun_learners.empty() ) {
            add_msg( m_info, _( "You read aloud…" ) );
        }
    }

    if( !continuous ||
    !std::all_of( learners.begin(), learners.end(), [&]( const std::pair<npc *, std::string> &elem ) {
    return std::count( activity.values.begin(), activity.values.end(),
                       elem.first->getID().get_value() ) != 0;
    } ) ||
    !std::all_of( activity.values.begin(), activity.values.end(), [&]( int elem ) {
        return learners.find( g->find_npc( character_id( elem ) ) ) != learners.end();
    } ) ) {

        if( learners.size() == 1 ) {
            add_msg( m_info, _( "%s studies with you." ), learners.begin()->first->disp_name() );
        } else if( !learners.empty() ) {
            const std::string them = enumerate_as_string( learners.begin(),
            learners.end(), [&]( const std::pair<npc *, std::string> &elem ) {
                return elem.first->disp_name();
            } );
            add_msg( m_info, _( "%s study with you." ), them );
        }

        // Don't include the reader as it would be too redundant.
        std::set<std::string> readers;
        for( const auto &elem : fun_learners ) {
            if( elem.first != reader ) {
                readers.insert( elem.first->disp_name() );
            }
        }
        if( readers.size() == 1 ) {
            add_msg( m_info, _( "%s reads with you for fun." ), readers.begin()->c_str() );
        } else if( !readers.empty() ) {
            const std::string them = enumerate_as_string( readers );
            add_msg( m_info, _( "%s read with you for fun." ), them );
        }
    }

    if( std::min( fine_detail_vision_mod(), reader->fine_detail_vision_mod() ) > 1.0 ) {
        add_msg( m_warning,
                 _( "It's difficult for %s to see fine details right now.  Reading will take longer than usual." ),
                 reader->disp_name() );
    }

    const int intelligence = get_int();
    const bool complex_penalty = type->intel > std::min( intelligence, reader->get_int() ) &&
                                 !reader->has_trait( trait_PROF_DICEMASTER );
    const player *complex_player = reader->get_int() < intelligence ? reader : this;
    if( complex_penalty && !continuous ) {
        add_msg( m_warning,
                 _( "This book is too complex for %s to easily understand.  It will take longer to read." ),
                 complex_player->disp_name() );
    }

    // push an indentifier of martial art book to the action handling
    if( it.type->use_methods.count( "MA_MANUAL" ) ) {

        if( g->u.get_stamina() < g->u.get_stamina_max() / 10 ) {
            add_msg( m_info, _( "You are too exhausted to train martial arts." ) );
            return false;
        }
        act.str_values.clear();
        act.str_values.emplace_back( "martial_art" );
    }

    assign_activity( act );

    // Reinforce any existing morale bonus/penalty, so it doesn't decay
    // away while you read more.
    const time_duration decay_start = 1_turns * time_taken / 1000;
    std::set<player *> apply_morale = { this };
    for( const auto &elem : learners ) {
        apply_morale.insert( elem.first );
    }
    for( const auto &elem : fun_learners ) {
        apply_morale.insert( elem.first );
    }
    for( player *elem : apply_morale ) {
        //Fun bonuses for spritual and To Serve Man are no longer calculated here.
        elem->add_morale( MORALE_BOOK, 0, book_fun_for( it, *elem ) * 15, decay_start + 30_minutes,
                          decay_start, false, it.type );
    }

    return true;
}

void avatar::grab( object_type grab_type, const tripoint &grab_point )
{
    this->grab_type = grab_type;
    this->grab_point = grab_point;

    path_settings->avoid_rough_terrain = grab_type != OBJECT_NONE;
}

object_type avatar::get_grab_type() const
{
    return grab_type;
}

void avatar::do_read( item &book )
{
    const auto &reading = book.type->book;
    if( !reading ) {
        activity.set_to_null();
        return;
    }
    const skill_id &skill = reading->skill;

    if( !has_identified( book.typeId() ) ) {
        // Note that we've read the book.
        items_identified.insert( book.typeId() );

        add_msg( _( "You skim %s to find out what's in it." ), book.type_name() );
        if( skill && get_skill_level_object( skill ).can_train() ) {
            add_msg( m_info, _( "Can bring your %s skill to %d." ),
                     skill.obj().name(), reading->level );
            if( reading->req != 0 ) {
                add_msg( m_info, _( "Requires %s level %d to understand." ),
                         skill.obj().name(), reading->req );
            }
        }

        if( reading->intel != 0 ) {
            add_msg( m_info, _( "Requires intelligence of %d to easily read." ), reading->intel );
        }
        //It feels wrong to use a pointer to *this, but I can't find any other player pointers in this method.
        if( book_fun_for( book, *this ) != 0 ) {
            add_msg( m_info, _( "Reading this book affects your morale by %d" ), book_fun_for( book, *this ) );
        }

        if( book.type->use_methods.count( "MA_MANUAL" ) ) {
            const matype_id style_to_learn = martial_art_learned_from( *book.type );
            add_msg( m_info, _( "You can learn %s style from it." ), style_to_learn->name );
            add_msg( m_info, _( "This fighting style is %s to learn." ),
                     martialart_difficulty( style_to_learn ) );
            add_msg( m_info, _( "It would be easier to master if you'd have skill expertise in %s." ),
                     style_to_learn->primary_skill->name() );
            add_msg( m_info, _( "A training session with this book takes %s" ),
                     to_string( time_duration::from_minutes( reading->time ) ) );
        } else {
            add_msg( m_info, ngettext( "A chapter of this book takes %d minute to read.",
                                       "A chapter of this book takes %d minutes to read.", reading->time ),
                     reading->time );
        }

        std::vector<std::string> recipe_list;
        for( const auto &elem : reading->recipes ) {
            // If the player knows it, they recognize it even if it's not clearly stated.
            if( elem.is_hidden() && !knows_recipe( elem.recipe ) ) {
                continue;
            }
            recipe_list.push_back( elem.name );
        }
        if( !recipe_list.empty() ) {
            std::string recipe_line =
                string_format( ngettext( "This book contains %1$zu crafting recipe: %2$s",
                                         "This book contains %1$zu crafting recipes: %2$s",
                                         recipe_list.size() ),
                               recipe_list.size(),
                               enumerate_as_string( recipe_list ) );
            add_msg( m_info, recipe_line );
        }
        if( recipe_list.size() != reading->recipes.size() ) {
            add_msg( m_info, _( "It might help you figuring out some more recipes." ) );
        }
        activity.set_to_null();
        return;
    }

    //learners and their penalties
    std::vector<std::pair<player *, double>> learners;
    for( size_t i = 0; i < activity.values.size(); i++ ) {
        player *n = g->find_npc( character_id( activity.values[i] ) );
        if( n != nullptr ) {
            const std::string &s = activity.get_str_value( i, "1" );
            learners.push_back( { n, strtod( s.c_str(), nullptr ) } );
        }
        // Otherwise they must have died/teleported or something
    }
    learners.push_back( { this, 1.0 } );
    //whether to continue reading or not
    bool continuous = false;
    // NPCs who learned a little about the skill
    std::set<std::string> little_learned;
    std::set<std::string> cant_learn;
    std::list<std::string> out_of_chapters;

    for( auto &elem : learners ) {
        player *learner = elem.first;

        if( book_fun_for( book, *learner ) != 0 ) {
            //Fun bonus is no longer calculated here.
            learner->add_morale( MORALE_BOOK, book_fun_for( book, *learner ) * 5, book_fun_for( book,
                                 *learner ) * 15, 1_hours, 30_minutes, true,
                                 book.type );
        }

        book.mark_chapter_as_read( *learner );

        if( skill && learner->get_skill_level( skill ) < reading->level &&
            learner->get_skill_level_object( skill ).can_train() ) {
            SkillLevel &skill_level = learner->get_skill_level_object( skill );
            const int originalSkillLevel = skill_level.level();

            // Calculate experience gained
            /** @EFFECT_INT increases reading comprehension */
            // Enhanced Memory Banks modestly boosts experience
            int min_ex = std::max( 1, reading->time / 10 + learner->get_int() / 4 );
            int max_ex = reading->time /  5 + learner->get_int() / 2 - originalSkillLevel;
            if( has_active_bionic( bio_memory ) ) {
                min_ex += 2;
            }

            min_ex = adjust_for_focus( min_ex );
            max_ex = adjust_for_focus( max_ex );

            if( max_ex < 2 ) {
                max_ex = 2;
            }
            if( max_ex > 10 ) {
                max_ex = 10;
            }
            if( max_ex < min_ex ) {
                max_ex = min_ex;
            }

            min_ex *= ( originalSkillLevel + 1 ) * elem.second;
            min_ex = std::max( min_ex, 1 );
            max_ex *= ( originalSkillLevel + 1 ) * elem.second;
            max_ex = std::max( min_ex, max_ex );

            skill_level.readBook( min_ex, max_ex, reading->level );

            std::string skill_name = skill.obj().name();

            if( skill_level != originalSkillLevel ) {
                g->events().send<event_type::gains_skill_level>(
                    learner->getID(), skill, skill_level.level() );
                if( learner->is_player() ) {
                    add_msg( m_good, _( "You increase %s to level %d." ), skill.obj().name(),
                             originalSkillLevel + 1 );
                } else {
                    add_msg( m_good, _( "%s increases their %s level." ), learner->disp_name(), skill_name );
                }
            } else {
                //skill_level == originalSkillLevel
                if( activity.index == learner->getID().get_value() ) {
                    continuous = true;
                }
                if( learner->is_player() ) {
                    add_msg( m_info, _( "You learn a little about %s!  (%d%%)" ), skill_name, skill_level.exercise() );
                } else {
                    little_learned.insert( learner->disp_name() );
                }
            }

            if( ( skill_level == reading->level || !skill_level.can_train() ) ||
                ( ( learner->has_trait( trait_SCHIZOPHRENIC ) ||
                    learner->has_artifact_with( AEP_SCHIZO ) ) && one_in( 25 ) ) ) {
                if( learner->is_player() ) {
                    add_msg( m_info, _( "You can no longer learn from %s." ), book.type_name() );
                } else {
                    cant_learn.insert( learner->disp_name() );
                }
            }
        } else if( skill ) {
            if( learner->is_player() ) {
                add_msg( m_info, _( "You can no longer learn from %s." ), book.type_name() );
            } else {
                cant_learn.insert( learner->disp_name() );
            }
        }
    }
    //end for all learners

    if( little_learned.size() == 1 ) {
        add_msg( m_info, _( "%s learns a little about %s!" ), little_learned.begin()->c_str(),
                 skill.obj().name() );
    } else if( !little_learned.empty() ) {
        const std::string little_learned_msg = enumerate_as_string( little_learned );
        add_msg( m_info, _( "%s learn a little about %s!" ), little_learned_msg, skill.obj().name() );
    }

    if( !cant_learn.empty() ) {
        const std::string names = enumerate_as_string( cant_learn );
        add_msg( m_info, _( "%s can no longer learn from %s." ), names, book.type_name() );
    }
    if( !out_of_chapters.empty() ) {
        const std::string names = enumerate_as_string( out_of_chapters );
        add_msg( m_info, _( "Rereading the %s isn't as much fun for %s." ),
                 book.type_name(), names );
        if( out_of_chapters.front() == disp_name() && one_in( 6 ) ) {
            add_msg( m_info, _( "Maybe you should find something new to read…" ) );
        }
    }

    // NPCs can't learn martial arts from manuals (yet).
    auto m = book.type->use_methods.find( "MA_MANUAL" );
    if( m != book.type->use_methods.end() ) {
        const matype_id style_to_learn = martial_art_learned_from( *book.type );
        skill_id skill_used = style_to_learn->primary_skill;
        int difficulty = std::max( 1, style_to_learn->learn_difficulty );
        difficulty = std::max( 1, 20 + difficulty * 2 - g->u.get_skill_level( skill_used ) * 2 );
        add_msg( m_debug, _( "Chance to learn one in: %d" ), difficulty );

        if( one_in( difficulty ) ) {
            m->second.call( *this, book, false, pos() );
            continuous = false;
        } else {
            if( activity.index == g->u.getID().get_value() ) {
                continuous = true;
                switch( rng( 1, 5 ) ) {
                    case 1:
                        add_msg( m_info,
                                 _( "You train the moves according to the book, but can't get a grasp of the style, so you start from the beginning." ) );
                        break;
                    case 2:
                        add_msg( m_info,
                                 _( "This martial art is not easy to grasp.  You start training the moves from the beginning." ) );
                        break;
                    case 3:
                        add_msg( m_info,
                                 _( "You decide to read the manual and train even more.  In martial arts, patience leads to mastery." ) );
                        break;
                    case 4:
                    case 5:
                        add_msg( m_info, _( "You try again.  This training will finally pay off." ) );
                        break;
                }
            } else {
                add_msg( m_info, _( "You train for a while." ) );
            }
        }
    }

    if( continuous ) {
        activity.set_to_null();
        read( book, true );
        if( activity ) {
            return;
        }
    }

    activity.set_to_null();
}

bool avatar::has_identified( const std::string &item_id ) const
{
    return items_identified.count( item_id ) > 0;
}

hint_rating avatar::rate_action_read( const item &it ) const
{
    if( !it.is_book() ) {
        return HINT_CANT;
    }

    std::vector<std::string> dummy;
    return get_book_reader( it, dummy ) == nullptr ? HINT_IFFY : HINT_GOOD;
}

void avatar::wake_up()
{
    if( has_effect( effect_sleep ) ) {
        if( calendar::turn - get_effect( effect_sleep ).get_start_time() > 2_hours ) {
            print_health();
        }
        if( has_effect( effect_slept_through_alarm ) ) {
            if( has_bionic( bio_watch ) ) {
                add_msg( m_warning, _( "It looks like you've slept through your internal alarm…" ) );
            } else {
                add_msg( m_warning, _( "It looks like you've slept through the alarm…" ) );
            }
        }
    }
    Character::wake_up();
}

void avatar::vomit()
{
    if( stomach.contains() != 0_ml ) {
        // Remove all joy from previously eaten food and apply the penalty
        rem_morale( MORALE_FOOD_GOOD );
        rem_morale( MORALE_FOOD_HOT );
        // bears must suffer too
        rem_morale( MORALE_HONEY );
        // 1.5 times longer
        add_morale( MORALE_VOMITED, -2 * units::to_milliliter( stomach.contains() / 50 ), -40, 90_minutes,
                    45_minutes, false );

    } else {
        add_msg( m_warning, _( "You retched, but your stomach is empty." ) );
    }
    Character::vomit();
}

void avatar::disp_morale()
{
    int equilibrium = calc_focus_equilibrium();

    int fatigue_penalty = 0;
    if( get_fatigue() >= MASSIVE_FATIGUE && equilibrium > 20 ) {
        fatigue_penalty = equilibrium - 20;
        equilibrium = 20;
    } else if( get_fatigue() >= EXHAUSTED && equilibrium > 40 ) {
        fatigue_penalty = equilibrium - 40;
        equilibrium = 40;
    } else if( get_fatigue() >= DEAD_TIRED && equilibrium > 60 ) {
        fatigue_penalty = equilibrium - 60;
        equilibrium = 60;
    } else if( get_fatigue() >= TIRED && equilibrium > 80 ) {
        fatigue_penalty = equilibrium - 80;
        equilibrium = 80;
    }

    int pain_penalty = 0;
    if( get_perceived_pain() && !has_trait( trait_CENOBITE ) ) {
        pain_penalty = calc_focus_equilibrium( true ) - equilibrium - fatigue_penalty;
    }

    morale->display( equilibrium, pain_penalty, fatigue_penalty );
}

int avatar::calc_focus_equilibrium( bool ignore_pain ) const
{
    int focus_equilibrium = 100;

    if( activity.id() == ACT_READ ) {
        const item &book = *activity.targets[0].get_item();
        if( book.is_book() && get_item_position( &book ) != INT_MIN ) {
            auto &bt = *book.type->book;
            // apply a penalty when we're actually learning something
            const SkillLevel &skill_level = get_skill_level_object( bt.skill );
            if( skill_level.can_train() && skill_level < bt.level ) {
                focus_equilibrium -= 50;
            }
        }
    }

    int eff_morale = get_morale_level();
    // Factor in perceived pain, since it's harder to rest your mind while your body hurts.
    // Cenobites don't mind, though
    if( !ignore_pain && !has_trait( trait_CENOBITE ) ) {
        eff_morale = eff_morale - get_perceived_pain();
    }

    if( eff_morale < -99 ) {
        // At very low morale, focus is at it's minimum
        focus_equilibrium = 1;
    } else if( eff_morale <= 50 ) {
        // At -99 to +50 morale, each point of morale gives or takes 1 point of focus
        focus_equilibrium += eff_morale;
    } else {
        /* Above 50 morale, we apply strong diminishing returns.
        * Each block of 50 takes twice as many morale points as the previous one:
        * 150 focus at 50 morale (as before)
        * 200 focus at 150 morale (100 more morale)
        * 250 focus at 350 morale (200 more morale)
        * ...
        * Cap out at 400% focus gain with 3,150+ morale, mostly as a sanity check.
        */

        int block_multiplier = 1;
        int morale_left = eff_morale;
        while( focus_equilibrium < 400 ) {
            if( morale_left > 50 * block_multiplier ) {
                // We can afford the entire block.  Get it and continue.
                morale_left -= 50 * block_multiplier;
                focus_equilibrium += 50;
                block_multiplier *= 2;
            } else {
                // We can't afford the entire block.  Each block_multiplier morale
                // points give 1 focus, and then we're done.
                focus_equilibrium += morale_left / block_multiplier;
                break;
            }
        }
    }

    // This should be redundant, but just in case...
    if( focus_equilibrium < 1 ) {
        focus_equilibrium = 1;
    } else if( focus_equilibrium > 400 ) {
        focus_equilibrium = 400;
    }
    return focus_equilibrium;
}

int avatar::calc_focus_change() const
{
    int focus_gap = calc_focus_equilibrium() - focus_pool;

    // handle negative gain rates in a symmetric manner
    int base_change = 1;
    if( focus_gap < 0 ) {
        base_change = -1;
        focus_gap = -focus_gap;
    }

    // for every 100 points, we have a flat gain of 1 focus.
    // for every n points left over, we have an n% chance of 1 focus
    int gain = focus_gap / 100;
    if( rng( 1, 100 ) <= focus_gap % 100 ) {
        gain++;
    }

    gain *= base_change;

    // Fatigue will incrementally decrease any focus above related cap
    if( ( get_fatigue() >= TIRED && focus_pool > 80 ) ||
        ( get_fatigue() >= DEAD_TIRED && focus_pool > 60 ) ||
        ( get_fatigue() >= EXHAUSTED && focus_pool > 40 ) ||
        ( get_fatigue() >= MASSIVE_FATIGUE && focus_pool > 20 ) ) {

        //it can fall faster then 1
        if( gain > -1 ) {
            gain = -1;
        }
    }
    return gain;
}

void avatar::update_mental_focus()
{

    focus_pool += calc_focus_change();

    // Moved from calc_focus_equilibrium, because it is now const
    if( activity.id() == ACT_READ ) {
        const item *book = activity.targets[0].get_item();
        if( get_item_position( book ) == INT_MIN || !book->is_book() ) {
            add_msg_if_player( m_bad, _( "You lost your book!  You stop reading." ) );
            activity.set_to_null();
        }
    }
}

void avatar::reset_stats()
{
    const int current_stim = get_stim();

    // Trait / mutation buffs
    if( has_trait( trait_THICK_SCALES ) ) {
        add_miss_reason( _( "Your thick scales get in the way." ), 2 );
    }
    if( has_trait( trait_CHITIN2 ) || has_trait( trait_CHITIN3 ) || has_trait( trait_CHITIN_FUR3 ) ) {
        add_miss_reason( _( "Your chitin gets in the way." ), 1 );
    }
    if( has_trait( trait_COMPOUND_EYES ) && !wearing_something_on( bp_eyes ) ) {
        mod_per_bonus( 1 );
    }
    if( has_trait( trait_INSECT_ARMS ) ) {
        add_miss_reason( _( "Your insect limbs get in the way." ), 2 );
    }
    if( has_trait( trait_INSECT_ARMS_OK ) ) {
        if( !wearing_something_on( bp_torso ) ) {
            mod_dex_bonus( 1 );
        } else {
            mod_dex_bonus( -1 );
            add_miss_reason( _( "Your clothing restricts your insect arms." ), 1 );
        }
    }
    if( has_trait( trait_WEBBED ) ) {
        add_miss_reason( _( "Your webbed hands get in the way." ), 1 );
    }
    if( has_trait( trait_ARACHNID_ARMS ) ) {
        add_miss_reason( _( "Your arachnid limbs get in the way." ), 4 );
    }
    if( has_trait( trait_ARACHNID_ARMS_OK ) ) {
        if( !wearing_something_on( bp_torso ) ) {
            mod_dex_bonus( 2 );
        } else if( !exclusive_flag_coverage( "OVERSIZE" ).test( bp_torso ) ) {
            mod_dex_bonus( -2 );
            add_miss_reason( _( "Your clothing constricts your arachnid limbs." ), 2 );
        }
    }
    const auto set_fake_effect_dur = [this]( const efftype_id & type, const time_duration & dur ) {
        effect &eff = get_effect( type );
        if( eff.get_duration() == dur ) {
            return;
        }

        if( eff.is_null() && dur > 0_turns ) {
            add_effect( type, dur, num_bp, true );
        } else if( dur > 0_turns ) {
            eff.set_duration( dur );
        } else {
            remove_effect( type, num_bp );
        }
    };
    // Painkiller
    set_fake_effect_dur( effect_pkill, 1_turns * get_painkiller() );

    // Pain
    if( get_perceived_pain() > 0 ) {
        const auto ppen = get_pain_penalty();
        mod_str_bonus( -ppen.strength );
        mod_dex_bonus( -ppen.dexterity );
        mod_int_bonus( -ppen.intelligence );
        mod_per_bonus( -ppen.perception );
        if( ppen.dexterity > 0 ) {
            add_miss_reason( _( "Your pain distracts you!" ), static_cast<unsigned>( ppen.dexterity ) );
        }
    }

    // Radiation
    set_fake_effect_dur( effect_irradiated, 1_turns * get_rad() );
    // Morale
    const int morale = get_morale_level();
    set_fake_effect_dur( effect_happy, 1_turns * morale );
    set_fake_effect_dur( effect_sad, 1_turns * -morale );

    // Stimulants
    set_fake_effect_dur( effect_stim, 1_turns * current_stim );
    set_fake_effect_dur( effect_depressants, 1_turns * -current_stim );
    if( has_trait( trait_STIMBOOST ) ) {
        set_fake_effect_dur( effect_stim_overdose, 1_turns * ( current_stim - 60 ) );
    } else {
        set_fake_effect_dur( effect_stim_overdose, 1_turns * ( current_stim - 30 ) );
    }
    // Starvation
    const float bmi = get_bmi();
    if( bmi < character_weight_category::underweight ) {
        const int str_penalty = floor( ( 1.0f - ( bmi - 13.0f ) / 3.0f ) * get_str_base() );
        add_miss_reason( _( "You're weak from hunger." ),
                         static_cast<unsigned>( ( get_starvation() + 300 ) / 1000 ) );
        mod_str_bonus( -str_penalty );
        mod_dex_bonus( -( str_penalty / 2 ) );
        mod_int_bonus( -( str_penalty / 2 ) );
    }
    // Thirst
    if( get_thirst() >= 200 ) {
        // We die at 1200
        const int dex_mod = -get_thirst() / 200;
        add_miss_reason( _( "You're weak from thirst." ), static_cast<unsigned>( -dex_mod ) );
        mod_str_bonus( -get_thirst() / 200 );
        mod_dex_bonus( dex_mod );
        mod_int_bonus( -get_thirst() / 200 );
        mod_per_bonus( -get_thirst() / 200 );
    }
    if( get_sleep_deprivation() >= SLEEP_DEPRIVATION_HARMLESS ) {
        set_fake_effect_dur( effect_sleep_deprived, 1_turns * get_sleep_deprivation() );
    } else if( has_effect( effect_sleep_deprived ) ) {
        remove_effect( effect_sleep_deprived );
    }

    // Dodge-related effects
    mod_dodge_bonus( mabuff_dodge_bonus() -
                     ( encumb( bp_leg_l ) + encumb( bp_leg_r ) ) / 20.0f - encumb( bp_torso ) / 10.0f );
    // Whiskers don't work so well if they're covered
    if( has_trait( trait_WHISKERS ) && !wearing_something_on( bp_mouth ) ) {
        mod_dodge_bonus( 1 );
    }
    if( has_trait( trait_WHISKERS_RAT ) && !wearing_something_on( bp_mouth ) ) {
        mod_dodge_bonus( 2 );
    }
    // depending on mounts size, attacks will hit the mount and use their dodge rating.
    // if they hit the player, the player cannot dodge as effectively.
    if( is_mounted() ) {
        mod_dodge_bonus( -4 );
    }
    // Spider hair is basically a full-body set of whiskers, once you get the brain for it
    if( has_trait( trait_CHITIN_FUR3 ) ) {
        static const std::array<body_part, 5> parts{ { bp_head, bp_arm_r, bp_arm_l, bp_leg_r, bp_leg_l } };
        for( auto bp : parts ) {
            if( !wearing_something_on( bp ) ) {
                mod_dodge_bonus( +1 );
            }
        }
        // Torso handled separately, bigger bonus
        if( !wearing_something_on( bp_torso ) ) {
            mod_dodge_bonus( 4 );
        }
    }

    // Hit-related effects
    mod_hit_bonus( mabuff_tohit_bonus() + weapon.type->m_to_hit );

    // Apply static martial arts buffs
    martial_arts_data.ma_static_effects( *this );

    if( calendar::once_every( 1_minutes ) ) {
        update_mental_focus();
    }

    // Effects
    for( const auto &maps : *effects ) {
        for( auto i : maps.second ) {
            const auto &it = i.second;
            bool reduced = resists_effect( it );
            mod_str_bonus( it.get_mod( "STR", reduced ) );
            mod_dex_bonus( it.get_mod( "DEX", reduced ) );
            mod_per_bonus( it.get_mod( "PER", reduced ) );
            mod_int_bonus( it.get_mod( "INT", reduced ) );
        }
    }

    Character::reset_stats();

    recalc_sight_limits();
    recalc_speed_bonus();

}

int avatar::get_str_base() const
{
    return Character::get_str_base() + std::max( 0, str_upgrade );
}

int avatar::get_dex_base() const
{
    return Character::get_dex_base() + std::max( 0, dex_upgrade );
}

int avatar::get_int_base() const
{
    return Character::get_int_base() + std::max( 0, int_upgrade );
}

int avatar::get_per_base() const
{
    return Character::get_per_base() + std::max( 0, per_upgrade );
}

int avatar::kill_xp() const
{
    return g->get_kill_tracker().kill_xp();
}

// based on  D&D 5e level progression
static const std::array<int, 20> xp_cutoffs = { {
        300, 900, 2700, 6500, 14000,
        23000, 34000, 48000, 64000, 85000,
        100000, 120000, 140000, 165000, 195000,
        225000, 265000, 305000, 355000, 405000
    }
};

int avatar::free_upgrade_points() const
{
    const int xp = kill_xp();
    int lvl = 0;
    for( const int &xp_lvl : xp_cutoffs ) {
        if( xp >= xp_lvl ) {
            lvl++;
        } else {
            break;
        }
    }
    return lvl - str_upgrade - dex_upgrade - int_upgrade - per_upgrade;
}

static int xp_to_next( const avatar &you )
{
    const int cur_xp = you.kill_xp();
    // Initialize to 'already max level' sentinel
    int xp_next = -1;
    // Iterate in reverse: { 405k, 355k, 305k, ..., 300 }
    for( std::array<int, 20>::const_reverse_iterator iter = xp_cutoffs.crbegin();
         iter != xp_cutoffs.crend(); ++iter ) {
        if( cur_xp < *iter ) {
            xp_next = *iter;
        }
    }
    return xp_next;
}

void avatar::upgrade_stat_prompt( const Character::stat &stat )
{
    const int free_points = free_upgrade_points();
    const int next_lvl_xp = xp_to_next( *this );

    if( free_points <= 0 ) {
        popup( _( "No available stat points to spend.  Experience to next level: %d" ), next_lvl_xp );
        return;
    }

    std::string stat_string;
    switch( stat ) {
        case STRENGTH:
            stat_string = _( "strength" );
            break;
        case DEXTERITY:
            stat_string = _( "dexterity" );
            break;
        case INTELLIGENCE:
            stat_string = _( "intelligence" );
            break;
        case PERCEPTION:
            stat_string = _( "perception" );
            break;
        case DUMMY_STAT:
            stat_string = _( "invalid stat" );
            debugmsg( "Tried to use invalid stat" );
            break;
        default:
            return;
    }

    if( query_yn( _( "Are you sure you want to raise %s?  %d points available." ), stat_string,
                  free_points ) ) {
        switch( stat ) {
            case STRENGTH:
                str_upgrade++;
                break;
            case DEXTERITY:
                dex_upgrade++;
                break;
            case INTELLIGENCE:
                int_upgrade++;
                break;
            case PERCEPTION:
                per_upgrade++;
                break;
            case DUMMY_STAT:
                debugmsg( "Tried to use invalid stat" );
                break;
        }
    }
}

faction *avatar::get_faction() const
{
    return g->faction_manager_ptr->get( faction_id( "your_followers" ) );
}

void avatar::set_movement_mode( character_movemode new_mode )
{
    switch( new_mode ) {
        case CMM_WALK: {
            if( is_mounted() ) {
                if( mounted_creature->has_flag( MF_RIDEABLE_MECH ) ) {
                    add_msg( _( "You set your mech's leg power to a loping fast walk." ) );
                } else {
                    add_msg( _( "You nudge your steed into a steady trot." ) );
                }
            } else {
                add_msg( _( "You start walking." ) );
            }
            break;
        }
        case CMM_RUN: {
            if( can_run() ) {
                if( is_hauling() ) {
                    stop_hauling();
                }
                if( is_mounted() ) {
                    if( mounted_creature->has_flag( MF_RIDEABLE_MECH ) ) {
                        add_msg( _( "You set the power of your mech's leg servos to maximum." ) );
                    } else {
                        add_msg( _( "You spur your steed into a gallop." ) );
                    }
                } else {
                    add_msg( _( "You start running." ) );
                }
            } else {
                if( is_mounted() ) {
                    // mounts dont currently have stamina, but may do in future.
                    add_msg( m_bad, _( "Your steed is too tired to go faster." ) );
                } else if( get_working_leg_count() < 2 ) {
                    add_msg( m_bad, _( "You need two functional legs to run." ) );
                } else {
                    add_msg( m_bad, _( "You're too tired to run." ) );
                }
                return;
            }
            break;
        }
        case CMM_CROUCH: {
            if( is_mounted() ) {
                if( mounted_creature->has_flag( MF_RIDEABLE_MECH ) ) {
                    add_msg( _( "You reduce the power of your mech's leg servos to minimum." ) );
                } else {
                    add_msg( _( "You slow your steed to a walk." ) );
                }
            } else {
                add_msg( _( "You start crouching." ) );
            }
            break;
        }
        default: {
            return;
        }
    }
    move_mode = new_mode;
}

void avatar::toggle_run_mode()
{
    if( move_mode == CMM_RUN ) {
        set_movement_mode( CMM_WALK );
    } else {
        set_movement_mode( CMM_RUN );
    }
}

void avatar::toggle_crouch_mode()
{
    if( move_mode == CMM_CROUCH ) {
        set_movement_mode( CMM_WALK );
    } else {
        set_movement_mode( CMM_CROUCH );
    }
}

void avatar::reset_move_mode()
{
    if( move_mode != CMM_WALK ) {
        set_movement_mode( CMM_WALK );
    }
}

void avatar::cycle_move_mode()
{
    unsigned char as_uchar = static_cast<unsigned char>( move_mode );
    as_uchar = ( as_uchar + 1 + CMM_COUNT ) % CMM_COUNT;
    set_movement_mode( static_cast<character_movemode>( as_uchar ) );
    // if a movemode is disabled then just cycle to the next one
    if( !movement_mode_is( static_cast<character_movemode>( as_uchar ) ) ) {
        as_uchar = ( as_uchar + 1 + CMM_COUNT ) % CMM_COUNT;
        set_movement_mode( static_cast<character_movemode>( as_uchar ) );
    }
}

bool avatar::wield( item &target )
{
    if( is_wielding( target ) ) {
        return true;
    }

    if( !can_wield( target ).success() ) {
        return false;
    }

    if( !unwield() ) {
        return false;
    }
    cached_info.erase( "weapon_value" );
    if( target.is_null() ) {
        return true;
    }

    // Query whether to draw an item from a holster when attempting to wield the holster
    if( target.get_use( "holster" ) && !target.contents.empty() ) {
        //~ %1$s: weapon name, %2$s: holster name
        if( query_yn( pgettext( "holster", "Draw %1$s from %2$s?" ), target.get_contained().tname(),
                      target.tname() ) ) {
            invoke_item( &target );
            return false;
        }
    }

    // Wielding from inventory is relatively slow and does not improve with increasing weapon skill.
    // Worn items (including guns with shoulder straps) are faster but still slower
    // than a skilled player with a holster.
    // There is an additional penalty when wielding items from the inventory whilst currently grabbed.

    bool worn = is_worn( target );
    int mv = item_handling_cost( target, true,
                                 worn ? INVENTORY_HANDLING_PENALTY / 2 : INVENTORY_HANDLING_PENALTY );

    if( worn ) {
        target.on_takeoff( *this );
    }

    add_msg( m_debug, "wielding took %d moves", mv );
    moves -= mv;

    if( has_item( target ) ) {
        weapon = i_rem( &target );
    } else {
        weapon = target;
    }

    last_item = weapon.typeId();
    recoil = MAX_RECOIL;

    weapon.on_wield( *this, mv );

    inv.update_invlet( weapon );
    inv.update_cache_with_item( weapon );

    return true;
}

bool avatar::invoke_item( item *used, const tripoint &pt )
{
    const std::map<std::string, use_function> &use_methods = used->type->use_methods;

    if( use_methods.empty() ) {
        return false;
    } else if( use_methods.size() == 1 ) {
        return invoke_item( used, use_methods.begin()->first, pt );
    }

    uilist umenu;

    umenu.text = string_format( _( "What to do with your %s?" ), used->tname() );
    umenu.hilight_disabled = true;

    for( const auto &e : use_methods ) {
        const auto res = e.second.can_call( *this, *used, false, pt );
        umenu.addentry_desc( MENU_AUTOASSIGN, res.success(), MENU_AUTOASSIGN, e.second.get_name(),
                             res.str() );
    }

    umenu.desc_enabled = std::any_of( umenu.entries.begin(),
    umenu.entries.end(), []( const uilist_entry & elem ) {
        return !elem.desc.empty();
    } );

    umenu.query();

    int choice = umenu.ret;
    if( choice < 0 || choice >= static_cast<int>( use_methods.size() ) ) {
        return false;
    }

    const std::string &method = std::next( use_methods.begin(), choice )->first;

    g->refresh_all();

    return invoke_item( used, method, pt );
}

bool avatar::invoke_item( item *used )
{
    return Character::invoke_item( used );
}

bool avatar::invoke_item( item *used, const std::string &method, const tripoint &pt )
{
    return Character::invoke_item( used, method, pt );
}

bool avatar::invoke_item( item *used, const std::string &method )
{
    return Character::invoke_item( used, method );
}

points_left::points_left()
{
    limit = MULTI_POOL;
    init_from_options();
}

void points_left::init_from_options()
{
    stat_points = get_option<int>( "INITIAL_STAT_POINTS" );
    trait_points = get_option<int>( "INITIAL_TRAIT_POINTS" );
    skill_points = get_option<int>( "INITIAL_SKILL_POINTS" );
}

// Highest amount of points to spend on stats without points going invalid
int points_left::stat_points_left() const
{
    switch( limit ) {
        case FREEFORM:
        case ONE_POOL:
            return stat_points + trait_points + skill_points;
        case MULTI_POOL:
            return std::min( trait_points_left(),
                             stat_points + std::min( 0, trait_points + skill_points ) );
        case TRANSFER:
            return 0;
    }

    return 0;
}

int points_left::trait_points_left() const
{
    switch( limit ) {
        case FREEFORM:
        case ONE_POOL:
            return stat_points + trait_points + skill_points;
        case MULTI_POOL:
            return stat_points + trait_points + std::min( 0, skill_points );
        case TRANSFER:
            return 0;
    }

    return 0;
}

int points_left::skill_points_left() const
{
    return stat_points + trait_points + skill_points;
}

bool points_left::is_freeform()
{
    return limit == FREEFORM;
}

bool points_left::is_valid()
{
    return is_freeform() ||
           ( stat_points_left() >= 0 && trait_points_left() >= 0 &&
             skill_points_left() >= 0 );
}

bool points_left::has_spare()
{
    return !is_freeform() && is_valid() && skill_points_left() > 0;
}

std::string points_left::to_string()
{
    if( limit == MULTI_POOL ) {
        return string_format(
                   _( "Points left: <color_%s>%d</color>%c<color_%s>%d</color>%c<color_%s>%d</color>=<color_%s>%d</color>" ),
                   stat_points_left() >= 0 ? "light_gray" : "red", stat_points,
                   trait_points >= 0 ? '+' : '-',
                   trait_points_left() >= 0 ? "light_gray" : "red", abs( trait_points ),
                   skill_points >= 0 ? '+' : '-',
                   skill_points_left() >= 0 ? "light_gray" : "red", abs( skill_points ),
                   is_valid() ? "light_gray" : "red", stat_points + trait_points + skill_points );
    } else if( limit == ONE_POOL ) {
        return string_format( _( "Points left: %4d" ), skill_points_left() );
    } else if( limit == TRANSFER ) {
        return _( "Character Transfer: No changes can be made." );
    } else {
        return _( "Freeform" );
    }
}
