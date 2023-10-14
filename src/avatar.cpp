#include "avatar.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>

#include "action.h"
#include "activity_type.h"
#include "activity_actor_definitions.h"
#include "bionics.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_assert.h"
#include "catacharset.h"
#include "character.h"
#include "character_id.h"
#include "character_martial_arts.h"
#include "clzones.h"
#include "color.h"
#include "cursesdef.h"
#include "debug.h"
#include "diary.h"
#include "effect.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "faction.h"
#include "field_type.h"
#include "game.h"
#include "game_constants.h"
#include "help.h"
#include "inventory.h"
#include "item.h"
#include "item_location.h"
#include "itype.h"
#include "iuse.h"
#include "kill_tracker.h"
#include "make_static.h"
#include "magic_enchantment.h"
#include "map.h"
#include "map_memory.h"
#include "martialarts.h"
#include "messages.h"
#include "mission.h"
#include "morale.h"
#include "morale_types.h"
#include "move_mode.h"
#include "mutation.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "pathfinding.h"
#include "pimpl.h"
#include "player_activity.h"
#include "profession.h"
#include "ranged.h"
#include "ret_val.h"
#include "rng.h"
#include "skill.h"
#include "stomach.h"
#include "string_formatter.h"
#include "talker.h"
#include "talker_avatar.h"
#include "translations.h"
#include "timed_event.h"
#include "trap.h"
#include "type_id.h"
#include "ui.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"

static const bionic_id bio_cloak( "bio_cloak" );
static const bionic_id bio_soporific( "bio_soporific" );

static const efftype_id effect_alarm_clock( "alarm_clock" );
static const efftype_id effect_boomered( "boomered" );
static const efftype_id effect_depressants( "depressants" );
static const efftype_id effect_happy( "happy" );
static const efftype_id effect_irradiated( "irradiated" );
static const efftype_id effect_onfire( "onfire" );
static const efftype_id effect_pkill( "pkill" );
static const efftype_id effect_relax_gas( "relax_gas" );
static const efftype_id effect_sad( "sad" );
static const efftype_id effect_sleep( "sleep" );
static const efftype_id effect_sleep_deprived( "sleep_deprived" );
static const efftype_id effect_slept_through_alarm( "slept_through_alarm" );
static const efftype_id effect_stim( "stim" );
static const efftype_id effect_stim_overdose( "stim_overdose" );
static const efftype_id effect_stunned( "stunned" );

static const faction_id faction_your_followers( "your_followers" );

static const itype_id itype_guidebook( "guidebook" );
static const itype_id itype_mut_longpull( "mut_longpull" );

static const json_character_flag json_flag_ALARMCLOCK( "ALARMCLOCK" );
static const json_character_flag json_flag_PAIN_IMMUNE( "PAIN_IMMUNE" );
static const json_character_flag json_flag_WEBBED_HANDS( "WEBBED_HANDS" );

static const move_mode_id move_mode_crouch( "crouch" );
static const move_mode_id move_mode_prone( "prone" );
static const move_mode_id move_mode_run( "run" );
static const move_mode_id move_mode_walk( "walk" );

static const string_id<monfaction> monfaction_player( "player" );

static const trait_id trait_ARACHNID_ARMS( "ARACHNID_ARMS" );
static const trait_id trait_ARACHNID_ARMS_OK( "ARACHNID_ARMS_OK" );
static const trait_id trait_CENOBITE( "CENOBITE" );
static const trait_id trait_CHITIN2( "CHITIN2" );
static const trait_id trait_CHITIN3( "CHITIN3" );
static const trait_id trait_CHITIN_FUR3( "CHITIN_FUR3" );
static const trait_id trait_CHLOROMORPH( "CHLOROMORPH" );
static const trait_id trait_COMPOUND_EYES( "COMPOUND_EYES" );
static const trait_id trait_DEBUG_CLOAK( "DEBUG_CLOAK" );
static const trait_id trait_INSECT_ARMS( "INSECT_ARMS" );
static const trait_id trait_INSECT_ARMS_OK( "INSECT_ARMS_OK" );
static const trait_id trait_M_SKIN3( "M_SKIN3" );
static const trait_id trait_PROF_DICEMASTER( "PROF_DICEMASTER" );
static const trait_id trait_SHELL2( "SHELL2" );
static const trait_id trait_SHELL3( "SHELL3" );
static const trait_id trait_STIMBOOST( "STIMBOOST" );
static const trait_id trait_THICK_SCALES( "THICK_SCALES" );
static const trait_id trait_THRESH_SPIDER( "THRESH_SPIDER" );
static const trait_id trait_WATERSLEEP( "WATERSLEEP" );
static const trait_id trait_WEB_SPINNER( "WEB_SPINNER" );
static const trait_id trait_WEB_WALKER( "WEB_WALKER" );
static const trait_id trait_WEB_WEAVER( "WEB_WEAVER" );
static const trait_id trait_WHISKERS( "WHISKERS" );
static const trait_id trait_WHISKERS_RAT( "WHISKERS_RAT" );

avatar::avatar()
{
    player_map_memory = std::make_unique<map_memory>();
    show_map_memory = true;
    active_mission = nullptr;
    grab_type = object_type::NONE;
    calorie_diary.emplace_front( );
    a_diary = nullptr;
}

avatar::~avatar() = default;
// NOLINTNEXTLINE(performance-noexcept-move-constructor)
avatar::avatar( avatar && ) = default;
// NOLINTNEXTLINE(performance-noexcept-move-constructor)
avatar &avatar::operator=( avatar && ) = default;

void avatar::control_npc( npc &np, const bool debug )
{
    if( !np.is_player_ally() ) {
        debugmsg( "control_npc() called on non-allied npc %s", np.name );
        return;
    }
    character_id new_character = np.getID();
    const std::function<void( npc & )> update_npc = [new_character]( npc & guy ) {
        guy.update_missions_target( get_avatar().getID(), new_character );
    };
    overmap_buffer.foreach_npc( update_npc );
    mission().update_world_missions_character( get_avatar().getID(), new_character );
    // move avatar character data into shadow npc
    swap_character( get_shadow_npc() );
    // swap target npc with shadow npc
    std::swap( get_shadow_npc(), np );
    // move shadow npc character data into avatar
    swap_character( get_shadow_npc() );
    // the avatar character is no longer a follower NPC
    g->remove_npc_follower( getID() );
    // the previous avatar character is now a follower
    g->add_npc_follower( np.getID() );
    np.set_fac( faction_your_followers );
    // perception and mutations may have changed, so reset light level caches
    g->reset_light_level();
    // center the map on the new avatar character
    const bool z_level_changed = g->vertical_shift( posz() );
    g->update_map( *this, z_level_changed );
    character_mood_face( true );

    profession_id prof_id = prof ? prof->ident() : profession::generic()->ident();
    get_event_bus().send<event_type::game_avatar_new>( /*is_new_game=*/false, debug,
            getID(), name, male, prof_id, custom_profession );
}

void avatar::control_npc_menu( const bool debug )
{
    std::vector<shared_ptr_fast<npc>> followers;
    uilist charmenu;
    int charnum = 0;
    for( const character_id &elem : g->get_follower_list() ) {
        shared_ptr_fast<npc> follower = overmap_buffer.find_npc( elem );
        if( follower ) {
            followers.emplace_back( follower );
            charmenu.addentry( charnum++, true, MENU_AUTOASSIGN, follower->get_name() );
        }
    }
    if( followers.empty() ) {
        popup( _( "There's no one to take control of!" ) );
        return;
    }
    charmenu.w_y_setup = 0;
    charmenu.query();
    if( charmenu.ret < 0 || static_cast<size_t>( charmenu.ret ) >= followers.size() ) {
        return;
    }
    get_avatar().control_npc( *followers[charmenu.ret], debug );
}

void avatar::longpull( const std::string &name )
{
    item wtmp( itype_mut_longpull );
    g->temp_exit_fullscreen();
    target_handler::trajectory traj = target_handler::mode_throw( *this, wtmp, false );
    g->reenter_fullscreen();
    if( traj.empty() ) {
        return; // cancel
    }

    Creature::longpull( name, traj.back() );
}

void avatar::toggle_map_memory()
{
    show_map_memory = !show_map_memory;
}

bool avatar::is_map_memory_valid() const
{
    return player_map_memory->is_valid();
}

bool avatar::should_show_map_memory() const
{
    if( get_timed_events().get( timed_event_type::OVERRIDE_PLACE ) ) {
        return false;
    }
    return show_map_memory;
}

bool avatar::save_map_memory()
{
    return player_map_memory->save( get_map().getglobal( pos() ) );
}

void avatar::load_map_memory()
{
    player_map_memory->load( get_map().getglobal( pos() ) );
}

void avatar::prepare_map_memory_region( const tripoint_abs_ms &p1, const tripoint_abs_ms &p2 )
{
    player_map_memory->prepare_region( p1, p2 );
}

const memorized_tile &avatar::get_memorized_tile( const tripoint_abs_ms &p ) const
{
    if( should_show_map_memory() ) {
        return player_map_memory->get_tile( p );
    }
    return mm_submap::default_tile;
}

void avatar::memorize_terrain( const tripoint_abs_ms &p, const std::string_view id,
                               int subtile, int rotation )
{
    player_map_memory->set_tile_terrain( p, id, subtile, rotation );
}

void avatar::memorize_decoration( const tripoint_abs_ms &p, const std::string_view id,
                                  int subtile, int rotation )
{
    player_map_memory->set_tile_decoration( p, id, subtile, rotation );
}

void avatar::memorize_symbol( const tripoint_abs_ms &p, char32_t symbol )
{
    player_map_memory->set_tile_symbol( p, symbol );
}

void avatar::memorize_clear_decoration( const tripoint_abs_ms &p, std::string_view prefix )
{
    player_map_memory->clear_tile_decoration( p, prefix );
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

void avatar::reset_all_missions()
{
    active_mission = nullptr;
    active_missions.clear();
    completed_missions.clear();
    failed_missions.clear();
}

tripoint_abs_omt avatar::get_active_mission_target() const
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
        if( !cur_mission.get_type().invisible_on_complete ) {
            failed_missions.push_back( &cur_mission );
        }
        add_msg_if_player( m_bad, _( "Mission \"%s\" is failed." ), cur_mission.name() );
    } else {
        if( !cur_mission.get_type().invisible_on_complete ) {
            completed_missions.push_back( &cur_mission );
        }
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

void avatar::remove_active_mission( mission &cur_mission )
{
    cur_mission.remove_active_world_mission( cur_mission );
    const auto iter = std::find( active_missions.begin(), active_missions.end(), &cur_mission );
    if( iter == active_missions.end() ) {
        debugmsg( "removed mission %d was not in the active_missions list", cur_mission.get_id() );
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

diary *avatar::get_avatar_diary()
{
    if( a_diary == nullptr ) {
        a_diary = std::make_unique<diary>();
    }
    return a_diary.get();
}

bool avatar::read( item_location &book, item_location ereader )
{
    if( !book ) {
        add_msg( m_info, _( "Never mind." ) );
        return false;
    }

    std::vector<std::string> fail_messages;
    const Character *reader = get_book_reader( *book, fail_messages );
    if( reader == nullptr ) {
        // We can't read, and neither can our followers
        for( const std::string &reason : fail_messages ) {
            add_msg( m_bad, reason );
        }
        return false;
    }

    // spells are handled in a different place
    // src/iuse_actor.cpp -> learn_spell_actor::use
    if( book->get_use( "learn_spell" ) ) {
        book->get_use( "learn_spell" )->call( this, *book, pos() );
        return true;
    }

    bool continuous = false;
    const time_duration time_taken = time_to_read( *book, *reader );
    add_msg_debug( debugmode::DF_ACT_READ, "avatar::read time_taken = %s",
                   to_string_writable( time_taken ) );

    // If the player hasn't read this book before, skim it to get an idea of what's in it.
    if( !has_identified( book->typeId() ) ) {
        if( reader != this ) {
            add_msg( m_info, fail_messages[0] );
            add_msg( m_info, _( "%s reads aloud…" ), reader->disp_name() );
        }

        assign_activity( read_activity_actor( time_taken, book, ereader, false ) );
        return true;
    }

    if( book->typeId() == itype_guidebook ) {
        // special guidebook effect: print a misc. hint when read
        if( reader != this ) {
            add_msg( m_info, fail_messages[0] );
            dynamic_cast<const npc &>( *reader ).say( get_hint() );
        } else {
            add_msg( m_info, get_hint() );
        }
        get_event_bus().send<event_type::reads_book>( getID(), book->typeId() );
        mod_moves( -100 );
        return false;
    }

    const cata::value_ptr<islot_book> &type = book->type->book;
    const skill_id &skill = type->skill;
    const std::string skill_name = skill ? skill.obj().name() : "";

    // Find NPCs to join the study session:
    std::map<npc *, std::string> learners;
    //reading only for fun
    std::map<npc *, std::string> fun_learners;
    std::map<npc *, std::string> nonlearners;
    for( npc *elem : get_crafting_helpers() ) {
        const book_mastery mastery = elem->get_book_mastery( *book );
        const bool morale_req = elem->fun_to_read( *book ) || elem->has_morale_to_read();

        // Note that the reader cannot be a nonlearner
        // since a reader should always have enough morale to read
        // and at the very least be able to understand the book

        if( elem->is_deaf() && elem != reader ) {
            nonlearners.insert( { elem, _( " (deaf)" ) } );

        } else if( mastery == book_mastery::MASTERED && elem->fun_to_read( *book ) ) {
            fun_learners.insert( {elem, elem == reader ? _( " (reading aloud to you)" ) : "" } );

        } else if( mastery == book_mastery::LEARNING && morale_req ) {
            learners.insert( {elem, elem == reader ? _( " (reading aloud to you)" ) : ""} );
        } else {
            std::string reason = _( " (uninterested)" );

            if( !morale_req ) {
                reason = _( " (too sad)" );

            } else if( mastery == book_mastery::CANT_UNDERSTAND ) {
                reason = string_format( _( " (needs %d %s)" ), type->req, skill_name );

            } else if( mastery == book_mastery::MASTERED ) {
                reason = string_format( _( " (already has %d %s)" ), type->level, skill_name );

            }
            nonlearners.insert( { elem, reason } );
        }
    }

    int learner_id = -1;
    const bool is_martialarts = book->type->use_methods.count( "MA_MANUAL" );

    //only show the menu if there's useful information or multiple options
    if( ( skill || !nonlearners.empty() || !fun_learners.empty() ) && !is_martialarts ) {
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
            const int lvl = elem.first->get_knowledge_level( skill );
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

        menu.title = !skill ? string_format( _( "Reading %s" ), book->type_name() ) :
                     //~ %1$s: book name, %2$s: skill name, %3$d and %4$d: skill levels
                     string_format( _( "Reading %1$s (can train %2$s from %3$d to %4$d)" ), book->type_name(),
                                    skill_name, type->req, type->level );

        menu.addentry( 0, true, '0', _( "Read once" ) );

        const int lvl = get_knowledge_level( skill );
        menu.addentry( 2 + getID().get_value(), lvl < type->level, '1',
                       string_format( _( "Read until you gain a level | current level: %d" ), lvl ) );

        // select until player gets level by default
        if( lvl < type->level ) {
            menu.selected = 1;
        }

        if( !learners.empty() ) {
            add_header( _( "Read until this NPC gains a level:" ) );
            for( const std::pair<npc *const, std::string> &elem : learners ) {
                menu.addentry( 2 + elem.first->getID().get_value(), true, -1,
                               get_text( learners, elem ) );
            }
        }

        if( !fun_learners.empty() ) {
            add_header( _( "Reading for fun:" ) );
            for( const std::pair<npc *const, std::string> &elem : fun_learners ) {
                menu.addentry( -1, false, -1, get_text( fun_learners, elem ) );
            }
        }

        if( !nonlearners.empty() ) {
            add_header( _( "Not participating:" ) );
            for( const std::pair<npc *const, std::string> &elem : nonlearners ) {
                menu.addentry( -1, false, -1, get_text( nonlearners, elem ) );
            }
        }

        menu.query( true );
        if( menu.ret == UILIST_CANCEL ) {
            add_msg( m_info, _( "Never mind." ) );
            return false;
        } else if( menu.ret >= 2 ) {
            continuous = true;
            learner_id = menu.ret - 2;
        }
    }

    if( is_martialarts ) {

        if( martial_arts_data->has_martialart( martial_art_learned_from( *book->type ) ) ) {
            add_msg_if_player( m_info, _( "You already know all this book has to teach." ) );
            return false;
        }

        uilist menu;
        menu.title = string_format( _( "Train %s from manual:" ),
                                    martial_art_learned_from( *book->type )->name );
        menu.addentry( 1, true, '1', _( "Train once" ) );
        menu.addentry( 2, true, '0', _( "Train until tired or success" ) );
        menu.query( true );
        if( menu.ret == UILIST_CANCEL ) {
            add_msg( m_info, _( "Never mind." ) );
            return false;
        } else if( menu.ret == 1 ) {
            continuous = false;
        } else {    // menu.ret == 2
            continuous = true;
        }
    }

    add_msg( m_info, _( "Now reading %s, %s to stop early." ),
             book->type_name(), press_x( ACTION_PAUSE ) );

    // Print some informational messages
    if( reader != this ) {
        add_msg( m_info, fail_messages[0] );
        add_msg( m_info, _( "%s reads aloud…" ), reader->disp_name() );
    } else if( !learners.empty() || !fun_learners.empty() ) {
        add_msg( m_info, _( "You read aloud…" ) );
    }

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
    for( const std::pair<npc *const, std::string> &elem : fun_learners ) {
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

    if( std::min( fine_detail_vision_mod(), reader->fine_detail_vision_mod() ) > 1.0 ) {
        add_msg( m_warning,
                 _( "It's difficult for %s to see fine details right now.  Reading will take longer than usual." ),
                 reader->disp_name() );
    }

    const int intelligence = get_int();
    const bool complex_penalty = type->intel > std::min( intelligence, reader->get_int() ) &&
                                 !reader->has_trait( trait_PROF_DICEMASTER );
    const Character *complex_player = reader->get_int() < intelligence ? reader : this;
    if( complex_penalty ) {
        add_msg( m_warning,
                 _( "This book is too complex for %s to easily understand.  It will take longer to read." ),
                 complex_player->disp_name() );
    }

    // push an identifier of martial art book to the action handling
    if( is_martialarts &&
        get_stamina() < get_stamina_max() / 10 )  {
        add_msg( m_info, _( "You are too exhausted to train martial arts." ) );
        return false;
    }

    assign_activity( read_activity_actor( time_taken, book, ereader, continuous, learner_id ) );

    return true;
}

void avatar::grab( object_type grab_type_new, const tripoint &grab_point_new )
{
    const auto update_memory =
    [this]( const object_type gtype, const tripoint & gpoint, const bool erase ) {
        map &m = get_map();
        if( gtype == object_type::VEHICLE ) {
            if( const optional_vpart_position ovp = m.veh_at( pos() + gpoint ) ) {
                for( const tripoint &target : ovp->vehicle().get_points() ) {
                    if( erase ) {
                        memorize_clear_decoration( m.getglobal( target ), /* prefix = */ "vp_" );
                    }
                    m.memory_cache_dec_set_dirty( target, true );
                }
            }
        } else if( gtype != object_type::NONE ) {
            if( erase ) {
                memorize_clear_decoration( m.getglobal( pos() + gpoint ) );
            }
            m.memory_cache_dec_set_dirty( pos() + gpoint, true );
        }
    };
    // Mark the area covered by the previous vehicle/furniture/etc for re-memorizing.
    update_memory( grab_type, grab_point, /* erase = */ false );

    grab_type = grab_type_new;
    grab_point = grab_point_new;

    // Clear the map memory for the area covered by the vehicle/furniture/etc to
    // eliminate ghost vehicles/furnitures/etc.
    update_memory( grab_type, grab_point, /* erase = */ true );

    path_settings->avoid_rough_terrain = grab_type != object_type::NONE;
}

object_type avatar::get_grab_type() const
{
    return grab_type;
}

bool avatar::has_identified( const itype_id &item_id ) const
{
    return items_identified.count( item_id ) > 0;
}

void avatar::identify( const item &item )
{
    if( has_identified( item.typeId() ) ) {
        return;
    }
    if( !item.is_book() ) {
        debugmsg( "tried to identify non-book item" );
        return;
    }

    const ::item &book = item; // alias
    cata_assert( !has_identified( item.typeId() ) );
    items_identified.insert( item.typeId() );
    cata_assert( has_identified( item.typeId() ) );

    const auto &reading = item.type->book;
    const skill_id &skill = reading->skill;

    add_msg( _( "You skim %s to find out what's in it." ), book.type_name() );
    if( skill && get_skill_level_object( skill ).can_train() ) {
        add_msg( m_info, _( "Can bring your %s knowledge to %d." ),
                 skill.obj().name(), reading->level );
        if( reading->req != 0 ) {
            add_msg( m_info, _( "Requires %1$s knowledge level %2$d to understand." ),
                     skill.obj().name(), reading->req );
        }
    }

    if( reading->intel != 0 ) {
        add_msg( m_info, _( "Requires intelligence of %d to easily read." ), reading->intel );
    }
    //It feels wrong to use a pointer to *this, but I can't find any other player pointers in this method.
    if( book_fun_for( book, *this ) != 0 ) {
        add_msg( m_info, _( "Reading this book affects your morale by %d." ), book_fun_for( book, *this ) );
    }

    if( book.type->use_methods.count( "MA_MANUAL" ) ) {
        const matype_id style_to_learn = martial_art_learned_from( *book.type );
        add_msg( m_info, _( "You can learn %s style from it." ), style_to_learn->name );
        add_msg( m_info, _( "This fighting style is %s to learn." ),
                 martialart_difficulty( style_to_learn ) );
        add_msg( m_info, _( "It would be easier to master if you'd have skill expertise in %s." ),
                 style_to_learn->primary_skill->name() );
        add_msg( m_info, _( "A training session with this book takes %s." ),
                 to_string_clipped( reading->time ) );
    } else {
        add_msg( m_info, _( "A chapter of this book takes %s to read." ),
                 to_string_clipped( reading->time ) );
    }

    std::vector<std::string> crafting_recipes;
    std::vector<std::string> practice_recipes;
    for( const islot_book::recipe_with_description_t &elem : reading->recipes ) {
        // If the player knows it, they recognize it even if it's not clearly stated.
        if( elem.is_hidden() && !knows_recipe( elem.recipe ) ) {
            continue;
        }
        if( elem.recipe->is_practice() ) {
            practice_recipes.emplace_back( elem.recipe->result_name() );
        } else {
            crafting_recipes.emplace_back( elem.name() );
        }
    }
    if( !crafting_recipes.empty() ) {
        add_msg( m_info, string_format( _( "This book can help you craft: %s" ),
                                        enumerate_as_string( crafting_recipes ) ) );
    }
    if( !practice_recipes.empty() ) {
        add_msg( m_info, string_format( _( "This book can help you practice: %s" ),
                                        enumerate_as_string( practice_recipes ) ) );
    }
    const std::size_t num_total_recipes = crafting_recipes.size() + practice_recipes.size();
    if( num_total_recipes < reading->recipes.size() ) {
        add_msg( m_info, _( "It might help you figuring out some more recipes." ) );
    }
}

void avatar::clear_nutrition()
{
    calorie_diary.clear();
    calorie_diary.emplace_front();
    consumption_history.clear();
}

void avatar::clear_identified()
{
    items_identified.clear();
}

void avatar::wake_up()
{
    if( has_effect( effect_sleep ) ) {
        if( calendar::turn - get_effect( effect_sleep ).get_start_time() > 2_hours ) {
            print_health();
        }
        // alarm was set and player hasn't slept through the alarm.
        if( has_effect( effect_alarm_clock ) && !has_effect( effect_slept_through_alarm ) ) {
            add_msg( _( "It looks like you woke up before your alarm." ) );
            // effects will be removed in Character::wake_up.
        } else if( has_effect( effect_slept_through_alarm ) ) {
            if( has_flag( json_flag_ALARMCLOCK ) ) {
                add_msg( m_warning, _( "It looks like you've slept through your internal alarm…" ) );
            } else {
                add_msg( m_warning, _( "It looks like you've slept through the alarm…" ) );
            }
        }
    }
    Character::wake_up();
}

void avatar::add_snippet( snippet_id snippet )
{
    if( has_seen_snippet( snippet ) ) {
        return;
    }

    snippets_read.emplace( snippet );
}

bool avatar::has_seen_snippet( const snippet_id &snippet ) const
{
    return snippets_read.count( snippet ) > 0;
}

const std::set<snippet_id> &avatar::get_snippets()
{
    return snippets_read;
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

bool avatar::try_break_relax_gas( const std::string &msg_success, const std::string &msg_failure )
{
    const effect &pacify = get_effect( effect_relax_gas, body_part_mouth );
    if( pacify.is_null() ) {
        return true;
    } else if( one_in( pacify.get_intensity() ) ) {
        add_msg( m_good, msg_success );
        return true;
    } else {
        mod_moves( std::max( 0, pacify.get_intensity() * 10 + rng( -30, 30 ) ) );
        add_msg( m_bad, msg_failure );
        return false;
    }
}

nc_color avatar::basic_symbol_color() const
{
    bool in_shell = has_active_mutation( trait_SHELL2 ) ||
                    has_active_mutation( trait_SHELL3 );
    if( has_effect( effect_onfire ) ) {
        return c_red;
    }
    if( has_effect( effect_stunned ) ) {
        return c_light_blue;
    }
    if( has_effect( effect_boomered ) ) {
        return c_pink;
    }
    if( in_shell ) {
        return c_magenta;
    }
    if( underwater ) {
        return c_blue;
    }
    if( has_active_bionic( bio_cloak ) ||
        is_wearing_active_optcloak() || has_trait( trait_DEBUG_CLOAK ) ) {
        return c_dark_gray;
    }
    return move_mode->symbol_color();
}

int avatar::print_info( const catacurses::window &w, int vStart, int, int column ) const
{
    return vStart + fold_and_print( w, point( column, vStart ), getmaxx( w ) - column - 1, c_dark_gray,
                                    _( "You (%s)" ),
                                    get_name() ) - 1;
}

mfaction_id avatar::get_monster_faction() const
{
    return monfaction_player.id();
}

void avatar::disp_morale()
{
    int equilibrium = calc_focus_equilibrium();

    int fatigue_penalty = 0;
    const int fatigue_cap = focus_equilibrium_fatigue_cap( equilibrium );

    if( fatigue_cap < equilibrium ) {
        fatigue_penalty = equilibrium - fatigue_cap;
        equilibrium = fatigue_cap;
    }

    int pain_penalty = 0;
    if( get_perceived_pain() && !has_trait( trait_CENOBITE ) ) {
        pain_penalty = calc_focus_equilibrium( true ) - equilibrium - fatigue_penalty;
    }

    morale->display( equilibrium, pain_penalty, fatigue_penalty );
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
    if( has_trait( trait_COMPOUND_EYES ) && !wearing_something_on( bodypart_id( "eyes" ) ) ) {
        mod_per_bonus( 2 );
    }
    if( has_trait( trait_INSECT_ARMS ) ) {
        add_miss_reason( _( "Your insect limbs get in the way." ), 2 );
    }
    if( has_trait( trait_INSECT_ARMS_OK ) ) {
        if( !wearing_fitting_on( bodypart_id( "torso" ) ) ) {
            mod_dex_bonus( 1 );
        } else {
            add_miss_reason( _( "Your clothing restricts your insect arms." ), 1 );
        }
    }
    if( has_flag( json_flag_WEBBED_HANDS ) ) {
        add_miss_reason( _( "Your webbed hands get in the way." ), 1 );
    }
    if( has_trait( trait_ARACHNID_ARMS ) ) {
        add_miss_reason( _( "Your arachnid limbs get in the way." ), 4 );
    }
    if( has_trait( trait_ARACHNID_ARMS_OK ) ) {
        if( !wearing_fitting_on( bodypart_id( "torso" ) ) ) {
            mod_dex_bonus( 2 );
        } else {
            add_miss_reason( _( "Your clothing constricts your arachnid limbs." ), 2 );
        }
    }
    const auto set_fake_effect_dur = [this]( const efftype_id & type, const time_duration & dur ) {
        effect &eff = get_effect( type );
        if( eff.get_duration() == dur ) {
            return;
        }

        if( eff.is_null() && dur > 0_turns ) {
            add_effect( type, dur, true );
        } else if( dur > 0_turns ) {
            eff.set_duration( dur );
        } else {
            remove_effect( type );
        }
    };
    // Painkiller
    set_fake_effect_dur( effect_pkill, 1_turns * get_painkiller() );

    // Pain
    if( get_perceived_pain() > 0 ) {
        const stat_mod ppen = get_pain_penalty();
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
    const float bmi = get_bmi_fat();
    if( bmi < character_weight_category::normal ) {
        const int str_penalty = std::floor( ( 1.0f - ( get_bmi_fat() /
                                              character_weight_category::normal ) ) * str_max );
        const int dexint_penalty = std::floor( ( character_weight_category::normal - bmi ) * 3.0f );
        add_miss_reason( _( "You're weak from hunger." ),
                         static_cast<unsigned>( ( get_starvation() + 300 ) / 1000 ) );
        mod_str_bonus( -1 * str_penalty );
        mod_dex_bonus( -1 * dexint_penalty );
        mod_int_bonus( -1 * dexint_penalty );
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
    mod_dodge_bonus( mabuff_dodge_bonus() );
    // Whiskers don't work so well if they're covered
    if( has_trait( trait_WHISKERS ) && !natural_attack_restricted_on( bodypart_id( "mouth" ) ) ) {
        mod_dodge_bonus( 1 );
    }
    if( has_trait( trait_WHISKERS_RAT ) && !natural_attack_restricted_on( bodypart_id( "mouth" ) ) ) {
        mod_dodge_bonus( 2 );
    }
    // depending on mounts size, attacks will hit the mount and use their dodge rating.
    // if they hit the player, the player cannot dodge as effectively.
    if( is_mounted() ) {
        mod_dodge_bonus( -4 );
    }
    // Spider hair is basically a full-body set of whiskers, once you get the brain for it
    if( has_trait( trait_CHITIN_FUR3 ) ) {
        static const std::array<bodypart_str_id, 5> parts {
            body_part_head, body_part_arm_r, body_part_arm_l,
            body_part_leg_r, body_part_leg_l
        };
        for( const bodypart_str_id &bp : parts ) {
            if( !wearing_something_on( bp ) ) {
                mod_dodge_bonus( +1 );
            }
        }
        // Torso handled separately, bigger bonus
        if( !wearing_something_on( bodypart_id( "torso" ) ) ) {
            mod_dodge_bonus( 4 );
        }
    }

    // Apply static martial arts buffs
    martial_arts_data->ma_static_effects( *this );

    // Effects
    for( const auto &maps : *effects ) {
        for( const auto &i : maps.second ) {
            const effect &it = i.second;
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
    int lvl = 0;
    for( const int &xp_lvl : xp_cutoffs ) {
        if( kill_xp >= xp_lvl ) {
            lvl++;
        } else {
            break;
        }
    }
    return lvl - spent_upgrade_points;
}

void avatar::upgrade_stat_prompt( const character_stat &stat )
{
    const int free_points = free_upgrade_points();

    if( free_points <= 0 ) {
        const std::size_t lvl = spent_upgrade_points + free_points;
        if( lvl >= xp_cutoffs.size() ) {
            popup( _( "You've already reached maximum level." ) );
        } else {
            popup( _( "Needs %d more experience to gain next level." ), xp_cutoffs[lvl] - kill_xp );
        }
        return;
    }

    std::string stat_string;
    switch( stat ) {
        case character_stat::STRENGTH:
            stat_string = _( "strength" );
            break;
        case character_stat::DEXTERITY:
            stat_string = _( "dexterity" );
            break;
        case character_stat::INTELLIGENCE:
            stat_string = _( "intelligence" );
            break;
        case character_stat::PERCEPTION:
            stat_string = _( "perception" );
            break;
        case character_stat::DUMMY_STAT:
            stat_string = _( "invalid stat" );
            debugmsg( "Tried to use invalid stat" );
            break;
        default:
            return;
    }

    if( query_yn( _( "Are you sure you want to raise %s?  %d points available." ), stat_string,
                  free_points ) ) {
        switch( stat ) {
            case character_stat::STRENGTH:
                str_max++;
                spent_upgrade_points++;
                recalc_hp();
                break;
            case character_stat::DEXTERITY:
                dex_max++;
                spent_upgrade_points++;
                break;
            case character_stat::INTELLIGENCE:
                int_max++;
                spent_upgrade_points++;
                break;
            case character_stat::PERCEPTION:
                per_max++;
                spent_upgrade_points++;
                break;
            case character_stat::DUMMY_STAT:
                debugmsg( "Tried to use invalid stat" );
                break;
        }
    }
}

faction *avatar::get_faction() const
{
    return g->faction_manager_ptr->get( faction_your_followers );
}

bool avatar::cant_see( const tripoint &p )
{

    // calc based on recoil
    if( !last_target_pos.has_value() ) {
        return false;
    }

    if( aim_cache_dirty ) {
        rebuild_aim_cache();
    }

    return aim_cache[p.x][p.y];
}

void avatar::rebuild_aim_cache()
{
    double pi = 2 * acos( 0.0 );

    const tripoint local_last_target = get_map().getlocal( last_target_pos.value() );

    float base_angle = atan2f( local_last_target.y - posy(),
                               local_last_target.x - posx() );

    // move from -pi to pi, to 0 to 2pi for angles
    if( base_angle < 0 ) {
        base_angle = base_angle + 2 * pi;
    }

    // todo: this is not the correct weapon when aiming with fake items
    item *weapon = get_wielded_item() ? &*get_wielded_item() : &null_item_reference();
    // calc steadiness with player recoil (like they are taking a regular shot not careful etc.
    float range = 3.0f - 2.8f * calc_steadiness( *this, *weapon, last_target_pos.value(), recoil );

    // pin between pi and negative pi
    float upper_bound = base_angle + range;
    float lower_bound = base_angle - range;

    // cap each within 0 - 2pi
    if( upper_bound > 2 * pi ) {
        upper_bound = upper_bound - 2 * pi;
    }

    if( lower_bound < 0 ) {
        lower_bound = lower_bound + 2 * pi;
    }

    for( int smx = 0; smx < MAPSIZE_X; ++smx ) {
        for( int smy = 0; smy < MAPSIZE_Y; ++smy ) {

            float current_angle = atan2f( smy - posy(), smx - posx() );

            // move from -pi to pi, to 0 to 2pi for angles
            if( current_angle < 0 ) {
                current_angle = current_angle + 2 * pi;
            }

            // some basic angle inclusion math, but also everything with 15 is still seen
            if( rl_dist( tripoint( point( smx, smy ), pos().z ), pos() ) < 15 ) {
                aim_cache[smx][smy] = false;
            } else if( lower_bound > upper_bound ) {
                aim_cache[smx][smy] = !( current_angle >= lower_bound ||
                                         current_angle <= upper_bound );
            } else {
                aim_cache[smx][smy] = !( current_angle >= lower_bound &&
                                         current_angle <= upper_bound );
            }
        }
    }

    // set cache as no longer dirty
    aim_cache_dirty = false;
}

void avatar::set_movement_mode( const move_mode_id &new_mode )
{
    if( can_switch_to( new_mode ) ) {
        if( is_hauling() && new_mode->stop_hauling() ) {
            stop_hauling();
        }
        add_msg( new_mode->change_message( true, get_steed_type() ) );
        move_mode = new_mode;
        // crouching affects visibility
        get_map().set_seen_cache_dirty( pos().z );
        recoil = MAX_RECOIL;
    } else {
        add_msg( new_mode->change_message( false, get_steed_type() ) );
    }
}

void avatar::toggle_run_mode()
{
    if( is_running() ) {
        set_movement_mode( move_mode_walk );
    } else {
        set_movement_mode( move_mode_run );
    }
}

void avatar::toggle_crouch_mode()
{
    if( is_crouching() ) {
        set_movement_mode( move_mode_walk );
    } else {
        set_movement_mode( move_mode_crouch );
    }
}

void avatar::toggle_prone_mode()
{
    if( is_prone() ) {
        set_movement_mode( move_mode_walk );
    } else {
        set_movement_mode( move_mode_prone );
    }
}
void avatar::activate_crouch_mode()
{
    if( !is_crouching() ) {
        set_movement_mode( move_mode_crouch );
    }
}

void avatar::reset_move_mode()
{
    if( !is_walking() ) {
        set_movement_mode( move_mode_walk );
    }
}

void avatar::cycle_move_mode()
{
    const move_mode_id next = current_movement_mode()->cycle();
    set_movement_mode( next );
    // if a movemode is disabled then just cycle to the next one
    if( !movement_mode_is( next ) ) {
        set_movement_mode( next->cycle() );
    }
}

void avatar::cycle_move_mode_reverse()
{
    const move_mode_id prev = current_movement_mode()->cycle_reverse();
    set_movement_mode( prev );
    // if a movemode is disabled then just cycle to the previous one
    if( !movement_mode_is( prev ) ) {
        set_movement_mode( prev->cycle_reverse() );
    }
}

bool avatar::wield( item_location target )
{
    return wield( *target, target.obtain_cost( *this ) );
}

bool avatar::wield( item &target )
{
    invalidate_inventory_validity_cache();
    invalidate_leak_level_cache();
    return wield( target,
                  item_handling_cost( target, true,
                                      is_worn( target ) ? INVENTORY_HANDLING_PENALTY / 2 :
                                      INVENTORY_HANDLING_PENALTY ) );
}

bool avatar::wield( item &target, const int obtain_cost )
{
    if( is_wielding( target ) ) {
        return true;
    }

    item_location weapon = get_wielded_item();
    if( weapon && weapon->has_item( target ) ) {
        add_msg( m_info, _( "You need to put the bag away before trying to wield something from it." ) );
        return false;
    }

    if( !can_wield( target ).success() ) {
        return false;
    }

    bool combine_stacks = weapon && target.can_combine( *weapon );
    if( !combine_stacks && !unwield() ) {
        return false;
    }
    cached_info.erase( "weapon_value" );
    if( target.is_null() ) {
        return true;
    }

    // Wielding from inventory is relatively slow and does not improve with increasing weapon skill.
    // Worn items (including guns with shoulder straps) are faster but still slower
    // than a skilled player with a holster.
    // There is an additional penalty when wielding items from the inventory whilst currently grabbed.

    bool worn = is_worn( target );
    const int mv = obtain_cost;

    if( worn ) {
        target.on_takeoff( *this );
    }

    add_msg_debug( debugmode::DF_AVATAR, "wielding took %d moves", mv );
    moves -= mv;

    if( has_item( target ) ) {
        item removed = i_rem( &target );
        if( combine_stacks ) {
            weapon->combine( removed );
        } else {
            set_wielded_item( removed );

        }
    } else {
        if( combine_stacks ) {
            weapon->combine( target );
        } else {
            set_wielded_item( target );
        }
    }

    // set_wielded_item invalidates the weapon item_location, so get it again
    weapon = get_wielded_item();
    last_item = weapon->typeId();
    recoil = MAX_RECOIL;

    weapon->on_wield( *this );

    cata::event e = cata::event::make<event_type::character_wields_item>( getID(), last_item );
    get_event_bus().send_with_talker( this, &weapon, e );

    inv->update_invlet( *weapon );
    inv->update_cache_with_item( *weapon );

    return true;
}

bool avatar::invoke_item( item *used, const tripoint &pt, int pre_obtain_moves )
{
    const std::map<std::string, use_function> &use_methods = used->type->use_methods;
    const int num_methods = use_methods.size();

    const bool has_relic = used->has_relic_activation();
    if( use_methods.empty() && !has_relic ) {
        return false;
    } else if( num_methods == 1 && !has_relic ) {
        return invoke_item( used, use_methods.begin()->first, pt, pre_obtain_moves );
    } else if( num_methods == 0 && has_relic ) {
        return used->use_relic( *this, pt );
    }

    uilist umenu;

    umenu.text = string_format( _( "What to do with your %s?" ), used->tname() );
    umenu.hilight_disabled = true;

    for( const auto &e : use_methods ) {
        const auto res = e.second.can_call( *this, *used, pt );
        umenu.addentry_desc( MENU_AUTOASSIGN, res.success(), MENU_AUTOASSIGN, e.second.get_name(),
                             res.str() );
    }
    if( has_relic ) {
        umenu.addentry_desc( MENU_AUTOASSIGN, true, MENU_AUTOASSIGN, _( "Use relic" ),
                             _( "Activate this relic." ) );
    }

    umenu.desc_enabled = std::any_of( umenu.entries.begin(),
    umenu.entries.end(), []( const uilist_entry & elem ) {
        return !elem.desc.empty();
    } );

    umenu.query();

    int choice = umenu.ret;
    // Use the relic
    if( choice == num_methods ) {
        return used->use_relic( *this, pt );
    }
    if( choice < 0 || choice >= num_methods ) {
        return false;
    }

    const std::string &method = std::next( use_methods.begin(), choice )->first;

    return invoke_item( used, method, pt, pre_obtain_moves );
}

bool avatar::invoke_item( item *used )
{
    return Character::invoke_item( used );
}

bool avatar::invoke_item( item *used, const std::string &method, const tripoint &pt,
                          int pre_obtain_moves )
{
    if( pre_obtain_moves == -1 ) {
        pre_obtain_moves = moves;
    }
    return Character::invoke_item( used, method, pt, pre_obtain_moves );
}

bool avatar::invoke_item( item *used, const std::string &method )
{
    return Character::invoke_item( used, method );
}

void avatar::update_cardio_acc()
{
    // This function should be called once every 24 hours,
    // before the front of the calorie diary is reset for the next day.

    // Cardio goal is 1000 times the ratio of kcals spent versus bmr,
    // giving a default of 1000 for no extra activity.
    const int bmr = get_bmr();
    const int last_24h_kcal = calorie_diary.front().spent;

    const int cardio_goal = ( last_24h_kcal * get_cardio_acc_base() ) / bmr;

    // If cardio accumulator is below cardio goal, gain some cardio.
    // Or, if cardio accumulator is above cardio goal, lose some cardio.
    const int cardio_accum = get_cardio_acc();
    int adjustment = 0;
    if( cardio_accum > cardio_goal ) {
        adjustment = -std::sqrt( cardio_accum - cardio_goal );
    } else if( cardio_goal > cardio_accum ) {
        adjustment = std::sqrt( cardio_goal - cardio_accum );
    }

    // Set a large sane upper limit to cardio fitness. This could be done
    // asymptotically instead of as a sharp cutoff, but the gradual growth
    // rate of cardio_acc should accomplish that naturally.
    set_cardio_acc( clamp( cardio_accum + adjustment, get_cardio_acc_base(),
                           get_cardio_acc_base() * 3 ) );
}

void avatar::advance_daily_calories()
{
    calorie_diary.emplace_front( );
    if( calorie_diary.size() > 30 ) {
        calorie_diary.pop_back();
    }
}

int avatar::get_daily_spent_kcal( bool yesterday ) const
{
    if( yesterday ) {
        if( calorie_diary.size() < 2 ) {
            return 0;
        }
        std::list<avatar::daily_calories> copy = calorie_diary;
        copy.pop_front();
        return copy.front().spent;
    }
    return calorie_diary.front().spent;
}

int avatar::get_daily_ingested_kcal( bool yesterday ) const
{
    if( yesterday ) {
        if( calorie_diary.size() < 2 ) {
            return 0;
        }
        std::list<avatar::daily_calories> copy = calorie_diary;
        copy.pop_front();
        return copy.front().ingested;
    }
    return calorie_diary.front().ingested;
}

void avatar::add_ingested_kcal( int kcal )
{
    calorie_diary.front().ingested += kcal;
}

void avatar::add_spent_calories( int cal )
{
    calorie_diary.front().spent += cal;
}

void avatar::add_gained_calories( int cal )
{
    calorie_diary.front().gained += cal;
}

void avatar::log_activity_level( float level )
{
    calorie_diary.front().activity_levels[level]++;
}

avatar::daily_calories::daily_calories()
{
    activity_levels.emplace( NO_EXERCISE, 0 );
    activity_levels.emplace( LIGHT_EXERCISE, 0 );
    activity_levels.emplace( MODERATE_EXERCISE, 0 );
    activity_levels.emplace( BRISK_EXERCISE, 0 );
    activity_levels.emplace( ACTIVE_EXERCISE, 0 );
    activity_levels.emplace( EXTRA_EXERCISE, 0 );
}

void avatar::daily_calories::serialize( JsonOut &json ) const
{
    json.start_object();

    json.member( "spent", spent );
    json.member( "gained", gained );
    json.member( "ingested", ingested );
    save_activity( json );

    json.end_object();
}

void avatar::daily_calories::deserialize( const JsonObject &data )
{
    data.read( "spent", spent );
    data.read( "gained", gained );
    data.read( "ingested", ingested );
    if( data.has_member( "activity" ) ) {
        read_activity( data );
    }
}

void avatar::daily_calories::save_activity( JsonOut &json ) const
{
    json.member( "activity" );
    json.start_array();
    for( const std::pair<const float, int> &level : activity_levels ) {
        if( level.second > 0 ) {
            json.start_array();
            json.write( level.first );
            json.write( level.second );
            json.end_array();
        }
    }
    json.end_array();
}

void avatar::daily_calories::read_activity( const JsonObject &data )
{
    if( data.has_array( "activity" ) ) {
        double act_level;
        for( JsonArray ja : data.get_array( "activity" ) ) {
            act_level = ja.next_float();
            activity_levels[ act_level ] = ja.next_int();
        }
        return;
    }
    // Fallback to legacy format for backward compatibility
    JsonObject jo = data.get_object( "activity" );
    for( const std::pair<const std::string, float> &member : activity_levels_map ) {
        int times;
        if( !jo.read( member.first, times ) ) {
            continue;
        }
        activity_levels.at( member.second ) = times;
    }
}

std::string avatar::total_daily_calories_string() const
{
    const std::string header_string =
        colorize( "       Minutes at each exercise level                  Calories per day",
                  c_white ) + "\n" +
        colorize( "  Day  Sleep None Light Moderate Brisk Active Extra    Gained  Spent  Total",
                  c_yellow ) + "\n";
    const std::string format_string =
        " %4d  %4d   %4d  %4d     %4d  %4d   %4d  %4d    %6d %6d";

    const float no_ex_thresh = ( SLEEP_EXERCISE + NO_EXERCISE ) / 2.0f;
    const float light_ex_thresh = ( NO_EXERCISE + LIGHT_EXERCISE ) / 2.0f;
    const float mod_ex_thresh = ( LIGHT_EXERCISE + MODERATE_EXERCISE ) / 2.0f;
    const float brisk_ex_thresh = ( MODERATE_EXERCISE + BRISK_EXERCISE ) / 2.0f;
    const float active_ex_thresh = ( BRISK_EXERCISE + ACTIVE_EXERCISE ) / 2.0f;
    const float extra_ex_thresh = ( ACTIVE_EXERCISE + EXTRA_EXERCISE ) / 2.0f;

    std::string ret = header_string;

    // Start with today in the first row, day number from start of the Cataclysm
    int today = day_of_season<int>( calendar::turn ) + 1;
    int day_offset = 0;
    for( const daily_calories &day : calorie_diary ) {
        // Yes, this is clunky.
        // Perhaps it should be done in log_activity_level? But that's called a lot more often.
        int sleep_exercise = 0;
        int no_exercise = 0;
        int light_exercise = 0;
        int moderate_exercise = 0;
        int brisk_exercise = 0;
        int active_exercise = 0;
        int extra_exercise = 0;
        for( const std::pair<const float, int> &level : day.activity_levels ) {
            if( level.second > 0 ) {
                if( level.first < no_ex_thresh ) {
                    sleep_exercise += level.second;
                } else if( level.first < light_ex_thresh ) {
                    no_exercise += level.second;
                } else if( level.first < mod_ex_thresh ) {
                    light_exercise += level.second;
                } else if( level.first < brisk_ex_thresh ) {
                    moderate_exercise += level.second;
                } else if( level.first < active_ex_thresh ) {
                    brisk_exercise += level.second;
                } else if( level.first < extra_ex_thresh ) {
                    active_exercise += level.second;
                } else {
                    extra_exercise += level.second;
                }
            }
        }
        std::string row_data = string_format( format_string, today + day_offset--,
                                              5 * sleep_exercise,
                                              5 * no_exercise,
                                              5 * light_exercise,
                                              5 * moderate_exercise,
                                              5 * brisk_exercise,
                                              5 * active_exercise,
                                              5 * extra_exercise,
                                              day.gained, day.spent );
        // Alternate gray and white text for row data
        if( day_offset % 2 == 0 ) {
            ret += colorize( row_data, c_white );
        } else {
            ret += colorize( row_data, c_light_gray );
        }

        // Color-code each day's net calories
        std::string total_kcals = string_format( " %6d", day.total() );
        if( day.total() > 4000 ) {
            ret += colorize( total_kcals, c_light_cyan );
        } else if( day.total() > 2000 ) {
            ret += colorize( total_kcals, c_cyan );
        } else if( day.total() > 250 ) {
            ret += colorize( total_kcals, c_light_blue );
        } else if( day.total() < -4000 ) {
            ret += colorize( total_kcals, c_pink );
        } else if( day.total() < -2000 ) {
            ret += colorize( total_kcals, c_red );
        } else if( day.total() < -250 ) {
            ret += colorize( total_kcals, c_light_red );
        } else {
            ret += colorize( total_kcals, c_light_gray );
        }
        ret += "\n";
    }
    return ret;
}

std::unique_ptr<talker> get_talker_for( avatar &me )
{
    return std::make_unique<talker_avatar>( &me );
}
std::unique_ptr<talker> get_talker_for( avatar *me )
{
    return std::make_unique<talker_avatar>( me );
}

void avatar::randomize_hobbies()
{
    hobbies.clear();
    std::vector<profession_id> choices = profession::get_all_hobbies();

    int random = rng( 0, 5 );

    if( random >= 1 ) {
        add_random_hobby( choices );
    }
    if( random >= 3 ) {
        add_random_hobby( choices );
    }
    if( random >= 5 ) {
        add_random_hobby( choices );
    }
}

void avatar::add_random_hobby( std::vector<profession_id> &choices )
{
    const profession_id hobby = random_entry_removed( choices );
    hobbies.insert( &*hobby );

    // Add or remove traits from hobby
    for( const trait_and_var &cur : hobby->get_locked_traits() ) {
        toggle_trait( cur.trait );
    }
}

void avatar::reassign_item( item &it, int invlet )
{
    bool remove_old = true;
    if( invlet ) {
        item *prev = invlet_to_item( invlet );
        if( prev != nullptr ) {
            remove_old = it.typeId() != prev->typeId();
            inv->reassign_item( *prev, it.invlet, remove_old );
        }
    }

    if( !invlet || inv_chars.valid( invlet ) ) {
        const auto iter = inv->assigned_invlet.find( it.invlet );
        bool found = iter != inv->assigned_invlet.end();
        if( found ) {
            inv->assigned_invlet.erase( iter );
        }
        if( invlet && ( !found || it.invlet != invlet ) ) {
            inv->assigned_invlet[invlet] = it.typeId();
        }
        inv->reassign_item( it, invlet, remove_old );
    }
}

void avatar::add_pain_msg( int val, const bodypart_id &bp ) const
{
    if( has_flag( json_flag_PAIN_IMMUNE ) ) {
        return;
    }
    if( bp == bodypart_id( "bp_null" ) ) {
        if( val > 20 ) {
            add_msg_if_player( _( "Your body is wracked with excruciating pain!" ) );
        } else if( val > 10 ) {
            add_msg_if_player( _( "Your body is wracked with terrible pain!" ) );
        } else if( val > 5 ) {
            add_msg_if_player( _( "Your body is wracked with pain!" ) );
        } else if( val > 1 ) {
            add_msg_if_player( _( "Your body pains you!" ) );
        } else {
            add_msg_if_player( _( "Your body aches." ) );
        }
    } else {
        if( val > 20 ) {
            add_msg_if_player( _( "Your %s is wracked with excruciating pain!" ),
                               body_part_name_accusative( bp ) );
        } else if( val > 10 ) {
            add_msg_if_player( _( "Your %s is wracked with terrible pain!" ),
                               body_part_name_accusative( bp ) );
        } else if( val > 5 ) {
            add_msg_if_player( _( "Your %s is wracked with pain!" ),
                               body_part_name_accusative( bp ) );
        } else if( val > 1 ) {
            add_msg_if_player( _( "Your %s pains you!" ),
                               body_part_name_accusative( bp ) );
        } else {
            add_msg_if_player( _( "Your %s aches." ),
                               body_part_name_accusative( bp ) );
        }
    }
}

bool avatar::wield_contents( item &container, item *internal_item, bool penalties, int base_cost )
{
    // if index not specified and container has multiple items then ask the player to choose one
    if( internal_item == nullptr ) {
        std::vector<std::string> opts;
        std::list<item *> container_contents = container.all_items_top();
        std::transform( container_contents.begin(), container_contents.end(),
        std::back_inserter( opts ), []( const item * elem ) {
            return elem->display_name();
        } );
        if( opts.size() > 1 ) {
            int pos = uilist( _( "Wield what?" ), opts );
            if( pos < 0 ) {
                return false;
            }
            internal_item = *std::next( container_contents.begin(), pos );
        } else {
            internal_item = container_contents.front();
        }
    }

    return Character::wield_contents( container, internal_item, penalties, base_cost );
}

void avatar::try_to_sleep( const time_duration &dur )
{
    map &here = get_map();
    const optional_vpart_position vp = here.veh_at( pos() );
    const trap &trap_at_pos = here.tr_at( pos() );
    const ter_id ter_at_pos = here.ter( pos() );
    const furn_id furn_at_pos = here.furn( pos() );
    bool plantsleep = false;
    bool fungaloid_cosplay = false;
    bool websleep = false;
    bool webforce = false;
    bool websleeping = false;
    bool in_shell = false;
    bool watersleep = false;
    if( has_trait( trait_CHLOROMORPH ) ) {
        plantsleep = true;
        if( ( ter_at_pos == t_dirt || ter_at_pos == t_pit ||
              ter_at_pos == t_dirtmound || ter_at_pos == t_pit_shallow ||
              ter_at_pos == t_grass ) && !vp &&
            furn_at_pos == f_null ) {
            add_msg_if_player( m_good, _( "You relax as your roots embrace the soil." ) );
        } else if( vp ) {
            add_msg_if_player( m_bad, _( "It's impossible to sleep in this wheeled pot!" ) );
        } else if( furn_at_pos != f_null ) {
            add_msg_if_player( m_bad,
                               _( "The humans' furniture blocks your roots.  You can't get comfortable." ) );
        } else { // Floor problems
            add_msg_if_player( m_bad, _( "Your roots scrabble ineffectively at the unyielding surface." ) );
        }
    } else if( has_trait( trait_M_SKIN3 ) ) {
        fungaloid_cosplay = true;
        if( here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_FUNGUS, pos() ) ) {
            add_msg_if_player( m_good,
                               _( "Our fibers meld with the ground beneath us.  The gills on our neck begin to seed the air with spores as our awareness fades." ) );
        }
    }
    if( has_trait( trait_WEB_WALKER ) ) {
        websleep = true;
    }
    // Not sure how one would get Arachnid w/o web-making, but Just In Case
    if( has_trait( trait_THRESH_SPIDER ) && ( has_trait( trait_WEB_SPINNER ) ||
            has_trait( trait_WEB_WEAVER ) ) ) {
        webforce = true;
    }
    if( websleep || webforce ) {
        int web = here.get_field_intensity( pos(), fd_web );
        if( !webforce ) {
            // At this point, it's kinda weird, but surprisingly comfy...
            if( web >= 3 ) {
                add_msg_if_player( m_good,
                                   _( "These thick webs support your weight, and are strangely comfortable…" ) );
                websleeping = true;
            } else if( web > 0 ) {
                add_msg_if_player( m_info,
                                   _( "You try to sleep, but the webs get in the way.  You brush them aside." ) );
                here.remove_field( pos(), fd_web );
            }
        } else {
            // Here, you're just not comfortable outside a nice thick web.
            if( web >= 3 ) {
                add_msg_if_player( m_good, _( "You relax into your web." ) );
                websleeping = true;
            } else {
                add_msg_if_player( m_bad,
                                   _( "You try to sleep, but you feel exposed and your spinnerets keep twitching." ) );
                add_msg_if_player( m_info, _( "Maybe a nice thick web would help you sleep." ) );
            }
        }
    }
    if( has_active_mutation( trait_SHELL2 ) || has_active_mutation( trait_SHELL3 ) ) {
        // Your shell's interior is a comfortable place to sleep.
        in_shell = true;
    }
    if( has_trait( trait_WATERSLEEP ) ) {
        if( underwater ) {
            add_msg_if_player( m_good,
                               _( "You lay beneath the waves' embrace, gazing up through the water's surface…" ) );
            watersleep = true;
        } else if( here.has_flag_ter( ter_furn_flag::TFLAG_SWIMMABLE, pos() ) ) {
            add_msg_if_player( m_good, _( "You settle into the water and begin to drowse…" ) );
            watersleep = true;
        }
    }
    if( !plantsleep && ( furn_at_pos.obj().comfort > static_cast<int>( comfort_level::neutral ) ||
                         ter_at_pos.obj().comfort > static_cast<int>( comfort_level::neutral ) ||
                         trap_at_pos.comfort > static_cast<int>( comfort_level::neutral ) ||
                         in_shell || websleeping || watersleep ||
                         vp.part_with_feature( "SEAT", true ) ||
                         vp.part_with_feature( "BED", true ) ) ) {
        add_msg_if_player( m_good, _( "This is a comfortable place to sleep." ) );
    } else if( !plantsleep && !fungaloid_cosplay && !watersleep ) {
        if( !vp && ter_at_pos != t_floor ) {
            add_msg_if_player( ter_at_pos.obj().movecost <= 2 ?
                               _( "It's a little hard to get to sleep on this %s." ) :
                               _( "It's hard to get to sleep on this %s." ),
                               ter_at_pos.obj().name() );
        } else if( vp ) {
            if( vp->part_with_feature( VPFLAG_AISLE, true ) ) {
                add_msg_if_player(
                    //~ %1$s: vehicle name, %2$s: vehicle part name
                    _( "It's a little hard to get to sleep on this %2$s in %1$s." ),
                    vp->vehicle().disp_name(),
                    vp->part_with_feature( VPFLAG_AISLE, true )->part().name( false ) );
            } else {
                add_msg_if_player(
                    //~ %1$s: vehicle name
                    _( "It's hard to get to sleep in %1$s." ),
                    vp->vehicle().disp_name() );
            }
        }
    }
    add_msg_if_player( _( "You start trying to fall asleep." ) );
    if( has_active_bionic( bio_soporific ) ) {
        bio_soporific_powered_at_last_sleep_check = has_power();
        if( bio_soporific_powered_at_last_sleep_check ) {
            // The actual bonus is applied in sleep_spot( p ).
            add_msg_if_player( m_good, _( "Your soporific inducer starts working its magic." ) );
        } else {
            add_msg_if_player( m_bad, _( "Your soporific inducer doesn't have enough power to operate." ) );
        }
    }
    assign_activity( try_sleep_activity_actor( dur ) );
}

bool avatar::query_yn( const std::string &mes ) const
{
    return ::query_yn( mes );
}

void avatar::set_location( const tripoint_abs_ms &loc )
{
    Creature::set_location( loc );
}

npc &avatar::get_shadow_npc()
{
    if( !shadow_npc ) {
        shadow_npc = std::make_unique<npc>();
        shadow_npc->op_of_u.trust = 10;
        shadow_npc->op_of_u.value = 10;
        shadow_npc->set_attitude( NPCATT_FOLLOW );
    }
    return *shadow_npc;
}

void avatar::export_as_npc( const cata_path &path )
{
    swap_character( get_shadow_npc() );
    get_shadow_npc().export_to( path );
    swap_character( get_shadow_npc() );
}

void monster_visible_info::remove_npc( npc *n )
{
    for( auto &t : unique_types ) {
        auto it = std::find( t.begin(), t.end(), n );
        if( it != t.end() ) {
            t.erase( it );
        }
    }
}
