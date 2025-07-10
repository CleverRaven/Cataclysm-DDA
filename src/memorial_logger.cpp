#include "memorial_logger.h"

#include <cstddef>
#include <istream>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <utility>

#include "achievement.h"
#include "addiction.h"
#include "avatar.h"
#include "bionics.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_variant.h"
#include "character.h"
#include "character_attire.h"
#include "character_id.h"
#include "coordinates.h"
#include "debug.h"
#include "debug_menu.h"
#include "effect.h"
#include "enum_conversions.h"
#include "event.h"
#include "event_statistics.h"
#include "filesystem.h"
#include "flexbuffer_json.h"
#include "game.h"
#include "get_version.h"
#include "inventory.h"
#include "item.h"
#include "item_factory.h"
#include "item_location.h"
#include "itype.h"
#include "json.h"
#include "json_loader.h"
#include "kill_tracker.h"
#include "magic.h"
#include "martialarts.h"
#include "messages.h"
#include "monstergenerator.h"
#include "mtype.h"
#include "mutation.h"
#include "omdata.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "past_games_info.h"
#include "pimpl.h"
#include "profession.h"
#include "proficiency.h"
#include "skill.h"
#include "stats_tracker.h"
#include "translation.h"
#include "translations.h"
#include "trap.h"
#include "type_id.h"
#include "units.h"

// IWYU pragma: no_forward_declare debug_menu::debug_menu_index

static const efftype_id effect_adrenaline( "adrenaline" );
static const efftype_id effect_datura( "datura" );
static const efftype_id effect_drunk( "drunk" );
static const efftype_id effect_jetinjector( "jetinjector" );

static const trait_id trait_CANNIBAL( "CANNIBAL" );
static const trait_id trait_PSYCHOPATH( "PSYCHOPATH" );
static const trait_id trait_SAPIOVORE( "SAPIOVORE" );

memorial_log_entry::memorial_log_entry( const std::string &preformatted_msg ) :
    preformatted_( preformatted_msg )
{}

memorial_log_entry::memorial_log_entry( time_point time, const oter_type_str_id &oter_id,
                                        std::string_view oter_name, std::string_view msg ) :
    time_( time ), oter_id_( oter_id ), oter_name_( oter_name ), message_( msg )
{}

std::string memorial_log_entry::to_string() const
{
    if( preformatted_ ) {
        return *preformatted_;
    } else {
        return "| " + ::to_string( time_ ) + " | " + oter_name_ + " | " + message_;
    }
}

void memorial_log_entry::deserialize( const JsonObject &jo )
{
    if( jo.read( "preformatted", preformatted_ ) ) {
        return;
    }
    jo.read( "time", time_, true );
    jo.read( "oter_id", oter_id_, true );
    jo.read( "oter_name", oter_name_, true );
    jo.read( "message", message_, true );
}

void memorial_log_entry::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    if( preformatted_ ) {
        jsout.member( "preformatted", preformatted_ );
    } else {
        jsout.member( "time", time_ );
        jsout.member( "oter_id", oter_id_ );
        jsout.member( "oter_name", oter_name_ );
        jsout.member( "message", message_ );
    }
    jsout.end_object();
}

void memorial_logger::clear()
{
    log.clear();
}

/**
 * Adds an event to the memorial log, to be written to the memorial file when
 * the character dies. The message should contain only the informational string,
 * as the timestamp and location will be automatically prepended.
 */
void memorial_logger::add( std::string_view male_msg,
                           std::string_view female_msg )
{
    Character &player_character = get_player_character();
    const std::string_view msg = player_character.male ? male_msg : female_msg;

    if( msg.empty() ) {
        return;
    }

    const oter_id &cur_ter = overmap_buffer.get_overmap_count() == 0 ?
                             oter_id() :
                             overmap_buffer.ter( player_character.pos_abs_omt() );
    const oter_type_str_id cur_oter_type = cur_ter->get_type_id();
    const std::string &oter_name = cur_ter->get_name( om_vision_level::full );

    log.emplace_back( calendar::turn, cur_oter_type, oter_name, msg );
}

/**
 * Loads the data in a memorial file from the given ifstream.
 * In legacy format all the memorial entry lines begin with a pipe (|).
 * (This legacy format stopped being used before 0.F release.)
 * In new format the entries are stored as json.
 * @param fin The stream to read the memorial entries from.
 */
void memorial_logger::load( std::istream &fin )
{
    log.clear();
    if( fin.peek() == '|' ) {
        // Legacy plain text format
        while( fin.peek() == '|' ) {
            std::string entry;
            getline( fin, entry );
            // strip all \r from end of string
            while( *entry.rbegin() == '\r' ) {
                entry.pop_back();
            }
            log.emplace_back( entry );
        }
    } else {
        // Unittests do not write data to a file first.
        std::string memorial_data;
        fin.seekg( 0, std::ios_base::end );
        size_t size = fin.tellg();
        fin.seekg( 0, std::ios_base::beg );
        memorial_data.resize( size );
        fin.read( memorial_data.data(), size );
        JsonValue jsin = json_loader::from_string( memorial_data );
        if( !jsin.read( log ) ) {
            debugmsg( "Error reading JSON memorial log" );
        }
    }
}

void memorial_logger::save( std::ostream &os ) const
{
    JsonOut jsout( os );
    jsout.write( log );
}

/**
 * Concatenates all of the memorial log entries, delimiting them with newlines,
 * and returns the resulting string. Used for saving and for writing out to the
 * memorial file.
 */
std::string memorial_logger::dump() const
{
    static const char *eol = cata_files::eol();
    std::string output;

    for( const memorial_log_entry &elem : log ) {
        output += elem.to_string();
        output += eol;
    }

    return output;
}

void memorial_logger::write_text_memorial( std::ostream &file,
        const std::string &epitaph ) const
{
    avatar &u = get_avatar();
    static const char *eol = cata_files::eol();

    //Size of indents in the memorial file
    const std::string indent = "  ";

    const std::string pronoun = u.male ? _( "He" ) : _( "She" );

    //Avoid saying "a male unemployed" or similar
    std::string profession_name;
    if( u.prof == profession::generic() ) {
        if( u.male ) {
            profession_name = _( "an unemployed male" );
        } else {
            profession_name = _( "an unemployed female" );
        }
    } else {
        profession_name = string_format( _( "a %s" ), u.prof->gender_appropriate_name( u.male ) );
    }

    const std::string locdesc =
        overmap_buffer.get_description_at( u.pos_abs_sm() );
    //~ First parameter is a pronoun ("He"/"She"), second parameter is a description
    //~ that designates the location relative to its surroundings.
    const std::string kill_place = string_format( _( "%1$s was killed in a %2$s." ),
                                   pronoun, locdesc );

    //Header
    file << string_format( _( "Cataclysm - Dark Days Ahead version %s memorial file" ),
                           getVersionString() ) << eol;
    file << eol;
    file << string_format( _( "In memory of: %s" ), u.get_name() ) << eol;
    if( !epitaph.empty() ) {  //Don't record empty epitaphs
        //~ The "%s" will be replaced by an epitaph as displayed in the memorial files. Replace the quotation marks as appropriate for your language.
        file << string_format( pgettext( "epitaph", "\"%s\"" ), epitaph ) << eol << eol;
    }
    //~ First parameter: Pronoun, second parameter: a profession name (with article)
    file << string_format( _( "%1$s was %2$s when the apocalypse began." ),
                           pronoun, profession_name ) << eol;
    file << string_format( _( "%1$s died on %2$s." ), pronoun,
                           to_string( calendar::turn ) ) << eol;
    file << kill_place << eol;
    file << eol;

    //Misc
    file << string_format( _( "Cash on hand: %s" ), format_money( u.cash ) ) << eol;
    file << eol;

    //HP

    const auto limb_hp =
    [&file, &indent, &u]( std::string_view desc, const bodypart_id & bp ) {
        file << indent <<
             string_format( desc, u.get_part_hp_cur( bp ), u.get_part_hp_max( bp ) ) << eol;
    };

    file << _( "Final HP:" ) << eol;
    limb_hp( _( " Head: %d/%d" ), bodypart_id( "head" ) );
    limb_hp( _( "Torso: %d/%d" ), bodypart_id( "torso" ) );
    limb_hp( _( "L Arm: %d/%d" ), bodypart_id( "arm_l" ) );
    limb_hp( _( "R Arm: %d/%d" ), bodypart_id( "arm_r" ) );
    limb_hp( _( "L Leg: %d/%d" ), bodypart_id( "leg_l" ) );
    limb_hp( _( "R Leg: %d/%d" ), bodypart_id( "leg_r" ) );
    file << eol;

    //Stats
    file << _( "Final Stats:" ) << eol;
    file << indent << string_format( _( "Str %d" ), u.str_cur )
         << indent << string_format( _( "Dex %d" ), u.dex_cur )
         << indent << string_format( _( "Int %d" ), u.int_cur )
         << indent << string_format( _( "Per %d" ), u.per_cur ) << eol;
    file << _( "Base Stats:" ) << eol;
    file << indent << string_format( _( "Str %d" ), u.str_max )
         << indent << string_format( _( "Dex %d" ), u.dex_max )
         << indent << string_format( _( "Int %d" ), u.int_max )
         << indent << string_format( _( "Per %d" ), u.per_max ) << eol;
    file << eol;

    //Last 20 messages
    file << _( "Final Messages:" ) << eol;
    std::vector<std::pair<std::string, std::string> > recent_messages =
        Messages::recent_messages( 20 );
    for( const std::pair<std::string, std::string> &recent_message : recent_messages ) {
        file << indent << recent_message.first << " " << recent_message.second;
        file << eol;
    }
    file << eol;

    //Kill list
    file << _( "Kills:" ) << eol;

    int total_kills = 0;

    std::map<std::tuple<std::string, std::string>, int> kill_counts;

    // map <name, sym> to kill count
    const kill_tracker &kills = g->get_kill_tracker();
    for( const mtype &type : MonsterGenerator::generator().get_all_mtypes() ) {
        int this_count = kills.kill_count( type.id );
        if( this_count > 0 ) {
            kill_counts[std::make_tuple( type.nname(), type.sym )] += this_count;
            total_kills += this_count;
        }
    }

    for( const std::pair<const std::tuple<std::string, std::string>, int> &entry : kill_counts ) {
        file << "  " << std::get<1>( entry.first ) << " - "
             << string_format( "%4d", entry.second ) << " "
             << std::get<0>( entry.first ) << eol;
    }

    if( total_kills == 0 ) {
        file << indent << _( "No monsters were killed." ) << eol;
    } else {
        file << string_format( _( "Total kills: %d" ), total_kills ) << eol;
    }
    file << eol;

    //Skills
    file << _( "Skills:" ) << eol;
    for( const std::pair<const skill_id, SkillLevel> &pair : u.get_all_skills() ) {
        const SkillLevel &lobj = pair.second;
        //~ 1. skill name, 2. skill level, 3. exercise percentage to next level
        file << indent << string_format( _( "%s: %d (%d %%)" ), pair.first->name(), lobj.level(),
                                         lobj.exercise() ) << eol;
    }
    file << eol;

    //Traits
    file << _( "Traits:" ) << eol;
    for( const trait_id &mut : u.get_functioning_mutations() ) {
        file << indent << u.mutation_name( mut ) << eol;
    }
    if( u.get_functioning_mutations().empty() ) {
        file << indent << _( "(None)" ) << eol;
    }
    file << eol;

    //Effects (illnesses)
    file << _( "Ongoing Effects:" ) << eol;
    bool had_effect = false;
    if( u.get_perceived_pain() > 0 ) {
        had_effect = true;
        file << indent << _( "Pain" ) << " (" << u.get_perceived_pain() << ")";
    }
    if( !had_effect ) {
        file << indent << _( "(None)" ) << eol;
    }
    file << eol;

    //Bionics
    file << _( "Bionics:" ) << eol;
    for( const bionic_id &bionic : u.get_bionics() ) {
        file << indent << bionic->name << eol;
    }
    if( u.get_bionics().empty() ) {
        file << indent << _( "No bionics were installed." ) << eol;
    }
    file << string_format(
             _( "Bionic Power: <color_light_blue>%d</color>/<color_light_blue>%d</color>" ),
             units::to_kilojoule( u.get_power_level() ), units::to_kilojoule( u.get_max_power_level() ) ) << eol;
    file << eol;

    //Equipment
    const item &weapon = u.get_wielded_item() ? *u.get_wielded_item() : null_item_reference();
    file << _( "Weapon:" ) << eol;
    file << indent << weapon.invlet << " - " << weapon.tname( 1, false ) << eol;
    file << eol;

    file << _( "Equipment:" ) << eol;
    u.worn.write_text_memorial( file, indent, eol );
    file << eol;

    //Inventory
    file << _( "Inventory:" ) << eol;
    u.inv->restack( u );
    invslice slice = u.inv->slice();
    for( const std::list<item> *elem : slice ) {
        const item &next_item = elem->front();
        file << indent << next_item.invlet << " - " <<
             next_item.tname( static_cast<unsigned>( elem->size() ), false );
        if( elem->size() > 1 ) {
            file << " [" << elem->size() << "]";
        }
        if( next_item.charges > 0 ) {
            file << " (" << next_item.charges << ")";
        }
        file << eol;
    }
    file << eol;

    //Lifetime stats
    file << _( "Lifetime Stats and Scores" ) << eol;

    for( const score *scr : get_stats().valid_scores() ) {
        file << indent << scr->description( get_stats() ) << eol;
    }
    file << eol;

    //History
    file << _( "Game History" ) << eol;
    file << dump();
}

void memorial_logger::write_json_memorial( std::ostream &memorial_file ) const
{
    JsonOut jsout( memorial_file, true, 2 );
    jsout.start_object();
    jsout.member( "memorial_version", 1 );
    jsout.member( "log", log );
    jsout.member( "achievements", get_achievements() );
    jsout.member( "stats", get_stats() );

    std::map<string_id<score>, cata_variant> scores;
    for( const score *scr : get_stats().valid_scores() ) {
        scores.emplace( scr->id, scr->value( get_stats() ) );
    }
    jsout.member( "scores", scores );

    jsout.end_object();

    // Having written a new memorial file, we need to ensure that
    // past_games_info now takes that into account
    clear_past_games();
}

void memorial_logger::notify( const cata::event &e )
{
    Character &player_character = get_player_character();
    character_id avatar_id = player_character.getID();
    std::string &avatar_name = player_character.name;
    switch( e.type() ) {
        case event_type::activates_artifact: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                std::string item_name = e.get<cata_variant_type::string>( "item_name" );
                //~ %s is artifact name
                add( pgettext( "memorial_male", "Activated the %s." ),
                     pgettext( "memorial_female", "Activated the %s." ),
                     item_name );
            }
            break;
        }
        case event_type::activates_mininuke: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                add( pgettext( "memorial_male", "Activated a mininuke." ),
                     pgettext( "memorial_female", "Activated a mininuke." ) );
            }
            break;
        }
        case event_type::administers_mutagen: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                mutagen_technique technique = e.get<mutagen_technique>( "technique" );
                switch( technique ) {
                    case mutagen_technique::consumed_mutagen:
                        add( pgettext( "memorial_male", "Consumed mutagen." ),
                             pgettext( "memorial_female", "Consumed mutagen." ) );
                        break;
                    case mutagen_technique::injected_mutagen:
                        add( pgettext( "memorial_male", "Injected mutagen." ),
                             pgettext( "memorial_female", "Injected mutagen." ) );
                        break;
                    case mutagen_technique::consumed_purifier:
                        add( pgettext( "memorial_male", "Consumed purifier." ),
                             pgettext( "memorial_female", "Consumed purifier." ) );
                        break;
                    case mutagen_technique::injected_purifier:
                        add( pgettext( "memorial_male", "Injected purifier." ),
                             pgettext( "memorial_female", "Injected purifier." ) );
                        break;
                    case mutagen_technique::injected_smart_purifier:
                        add( pgettext( "memorial_male", "Injected smart purifier." ),
                             pgettext( "memorial_female", "Injected smart purifier." ) );
                        break;
                    case mutagen_technique::num_mutagen_techniques:
                        break;
                }
            }
            break;
        }
        case event_type::angers_amigara_horrors: {
            add( pgettext( "memorial_male", "Angered a group of amigara horrors!" ),
                 pgettext( "memorial_female", "Angered a group of amigara horrors!" ) );
            break;
        }
        case event_type::avatar_dies: {
            add( pgettext( "memorial_male", "Died" ), pgettext( "memorial_female", "Died" ) );
            break;
        }
        case event_type::awakes_dark_wyrms: {
            add( pgettext( "memorial_male", "Awoke a group of dark wyrms!" ),
                 pgettext( "memorial_female", "Awoke a group of dark wyrms!" ) );
            break;
        }
        case event_type::becomes_wanted: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                add( pgettext( "memorial_male", "Became wanted by the police!" ),
                     pgettext( "memorial_female", "Became wanted by the police!" ) );
            }
            break;
        }
        case event_type::broken_bone: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                bodypart_id part = e.get<bodypart_id>( "part" );
                //~ %s is bodypart
                add( pgettext( "memorial_male", "Broke his %s." ),
                     pgettext( "memorial_female", "Broke her %s." ),
                     body_part_name( part ) );
            }
            break;
        }
        case event_type::broken_bone_mends: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                bodypart_id part = e.get<bodypart_id>( "part" );
                //~ %s is bodypart
                add( pgettext( "memorial_male", "Broken %s began to mend." ),
                     pgettext( "memorial_female", "Broken %s began to mend." ),
                     body_part_name( part ) );
            }
            break;
        }
        case event_type::buries_corpse: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                const mtype &corpse_type = e.get<mtype_id>( "corpse_type" ).obj();
                std::string corpse_name = e.get<cata_variant_type::string>( "corpse_name" );
                if( corpse_name.empty() ) {
                    if( corpse_type.has_flag( mon_flag_HUMAN ) ) {
                        add( pgettext( "memorial_male",
                                       "You buried an unknown victim of the Cataclysm." ),
                             pgettext( "memorial_female",
                                       "You buried an unknown victim of The Cataclysm." ) );
                    }
                } else {
                    add( pgettext( "memorial_male", "You buried %s." ),
                         pgettext( "memorial_female", "You buried %s." ),
                         corpse_name );
                }
            }
            break;
        }
        case event_type::causes_resonance_cascade: {
            add( pgettext( "memorial_male", "Caused a resonance cascade." ),
                 pgettext( "memorial_female", "Caused a resonance cascade." ) );
            break;
        }
        case event_type::character_gains_effect: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                const effect_type &type = e.get<efftype_id>( "effect" ).obj();
                const std::string male_message = type.get_apply_memorial_log(
                                                     effect_type::memorial_gender::male );
                const std::string female_message = type.get_apply_memorial_log(
                                                       effect_type::memorial_gender::female );
                if( !male_message.empty() || !female_message.empty() ) {
                    add( male_message, female_message );
                }
            }
            break;
        }
        case event_type::character_kills_character: {
            character_id ch = e.get<character_id>( "killer" );
            if( ch == avatar_id ) {
                std::string name = e.get<cata_variant_type::string>( "victim_name" );
                bool cannibal = player_character.has_trait( trait_CANNIBAL );
                bool psycho = player_character.has_trait( trait_PSYCHOPATH );
                if( player_character.has_trait( trait_SAPIOVORE ) ) {
                    add( pgettext( "memorial_male",
                                   "Caught and killed an ape.  Prey doesn't have a name." ),
                         pgettext( "memorial_female",
                                   "Caught and killed an ape.  Prey doesn't have a name." ) );
                } else if( psycho && cannibal ) {
                    add( pgettext( "memorial_male",
                                   "Killed a delicious-looking innocent, %s, in cold blood." ),
                         pgettext( "memorial_female",
                                   "Killed a delicious-looking innocent, %s, in cold blood." ),
                         name );
                } else if( psycho ) {
                    add( pgettext( "memorial_male",
                                   "Killed an innocent, %s, in cold blood.  They were weak." ),
                         pgettext( "memorial_female",
                                   "Killed an innocent, %s, in cold blood.  They were weak." ),
                         name );
                } else if( cannibal ) {
                    add( pgettext( "memorial_male", "Killed an innocent, %s." ),
                         pgettext( "memorial_female", "Killed an innocent, %s." ),
                         name );
                } else {
                    add( pgettext( "memorial_male",
                                   "Killed an innocent person, %s, in cold blood and "
                                   "felt terrible afterwards." ),
                         pgettext( "memorial_female",
                                   "Killed an innocent person, %s, in cold blood and "
                                   "felt terrible afterwards." ),
                         name );
                }
            }
            break;
        }
        case event_type::character_kills_monster: {
            character_id ch = e.get<character_id>( "killer" );
            if( ch == avatar_id ) {
                mtype_id victim_type = e.get<mtype_id>( "victim_type" );
                if( victim_type->difficulty >= 30 ) {
                    add( pgettext( "memorial_male", "Killed a %s." ),
                         pgettext( "memorial_female", "Killed a %s." ),
                         victim_type->nname() );
                }
            }
            break;
        }
        case event_type::character_loses_effect: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                const effect_type &type = e.get<efftype_id>( "effect" ).obj();
                const std::string male_message = type.get_remove_memorial_log(
                                                     effect_type::memorial_gender::male );
                const std::string female_message = type.get_remove_memorial_log(
                                                       effect_type::memorial_gender::female );
                if( !male_message.empty() || !female_message.empty() ) {
                    add( male_message, female_message );
                }
            }
            break;
        }
        case event_type::character_triggers_trap: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                trap_str_id trap = e.get<trap_str_id>( "trap" );
                if( trap->has_memorial_msg() ) {
                    add( trap->memorial_msg( true ), trap->memorial_msg( false ) );
                }
            }
            break;
        }
        case event_type::consumes_marloss_item: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                const itype *it = item_controller->find_template(
                                      e.get<cata_variant_type::itype_id>( "itype" ) );
                std::string itname = it->nname( 1 );
                add( pgettext( "memorial_male", "Consumed a %s." ),
                     pgettext( "memorial_female", "Consumed a %s." ), itname );
            }
            break;
        }
        case event_type::crosses_marloss_threshold: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                add( pgettext( "memorial_male", "Opened the Marloss Gateway." ),
                     pgettext( "memorial_female", "Opened the Marloss Gateway." ) );
            }
            break;
        }
        case event_type::crosses_mutation_threshold: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                mutation_category_id category_id =
                    e.get<cata_variant_type::mutation_category_id>( "category" );
                const mutation_category_trait &category =
                    mutation_category_trait::get_category( category_id );
                add( category.memorial_message_male(),
                     category.memorial_message_female() );
            }
            break;
        }
        case event_type::crosses_mycus_threshold: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                add( pgettext( "memorial_male", "Became one with the Mycus." ),
                     pgettext( "memorial_female", "Became one with the Mycus." ) );
            }
            break;
        }
        case event_type::dermatik_eggs_hatch: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                add( pgettext( "memorial_male", "Dermatik eggs hatched." ),
                     pgettext( "memorial_female", "Dermatik eggs hatched." ) );
            }
            break;
        }
        case event_type::dermatik_eggs_injected: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                add( pgettext( "memorial_male", "Injected with dermatik eggs." ),
                     pgettext( "memorial_female", "Injected with dermatik eggs." ) );
            }
            break;
        }
        case event_type::destroys_triffid_grove: {
            add( pgettext( "memorial_male", "Destroyed a triffid grove." ),
                 pgettext( "memorial_female", "Destroyed a triffid grove." ) );
            break;
        }
        case event_type::dies_from_asthma_attack: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                add( pgettext( "memorial_male", "Succumbed to an asthma attack." ),
                     pgettext( "memorial_female", "Succumbed to an asthma attack." ) );
            }
            break;
        }
        case event_type::dies_from_drug_overdose: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                efftype_id effect = e.get<efftype_id>( "effect" );
                if( effect == effect_datura ) {
                    add( pgettext( "memorial_male", "Died of datura overdose." ),
                         pgettext( "memorial_female", "Died of datura overdose." ) );
                } else if( effect == effect_jetinjector ) {
                    add( pgettext( "memorial_male", "Died of a healing stimulant overdose." ),
                         pgettext( "memorial_female", "Died of a healing stimulant overdose." ) );
                } else if( effect == effect_adrenaline ) {
                    add( pgettext( "memorial_male", "Died of adrenaline overdose." ),
                         pgettext( "memorial_female", "Died of adrenaline overdose." ) );
                } else if( effect == effect_drunk ) {
                    add( pgettext( "memorial_male", "Died of an alcohol overdose." ),
                         pgettext( "memorial_female", "Died of an alcohol overdose." ) );
                } else {
                    add( pgettext( "memorial_male", "Died of a drug overdose." ),
                         pgettext( "memorial_female", "Died of a drug overdose." ) );
                }
            }
            break;
        }
        case event_type::dies_from_bleeding: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                add( pgettext( "memorial_male", "Bled to death." ),
                     pgettext( "memorial_female", "Bled to death." ) );
            }
            break;
        }
        case event_type::dies_from_hypovolemia: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                add( pgettext( "memorial_male", "Died of hypovolemic shock." ),
                     pgettext( "memorial_female", "Died of hypovolemic shock." ) );
            }
            break;
        }
        case event_type::dies_from_redcells_loss: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                add( pgettext( "memorial_male", "Died from loss of red blood cells." ),
                     pgettext( "memorial_female", "Died from loss of red blood cells." ) );
            }
            break;
        }
        case event_type::dies_of_infection: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                add( pgettext( "memorial_male", "Succumbed to the infection." ),
                     pgettext( "memorial_female", "Succumbed to the infection." ) );
            }
            break;
        }
        case event_type::dies_of_starvation: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                add( pgettext( "memorial_male", "Died of starvation." ),
                     pgettext( "memorial_female", "Died of starvation." ) );
            }
            break;
        }
        case event_type::dies_of_thirst: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                add( pgettext( "memorial_male", "Died of thirst." ),
                     pgettext( "memorial_female", "Died of thirst." ) );
            }
            break;
        }
        case event_type::digs_into_lava: {
            add( pgettext( "memorial_male", "Dug a shaft into lava." ),
                 pgettext( "memorial_female", "Dug a shaft into lava." ) );
            break;
        }
        case event_type::disarms_nuke: {
            add( pgettext( "memorial_male", "Disarmed a nuclear missile." ),
                 pgettext( "memorial_female", "Disarmed a nuclear missile." ) );
            break;
        }
        case event_type::eats_sewage: {
            add( pgettext( "memorial_male", "Ate a sewage sample." ),
                 pgettext( "memorial_female", "Ate a sewage sample." ) );
            break;
        }
        case event_type::evolves_mutation: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                trait_id from = e.get<trait_id>( "from_trait" );
                trait_id to = e.get<trait_id>( "to_trait" );
                add( pgettext( "memorial_male", "'%s' mutation turned into '%s'" ),
                     pgettext( "memorial_female", "'%s' mutation turned into '%s'" ),
                     get_avatar().mutation_name( from ), get_avatar().mutation_name( to ) );
            }
            break;
        }
        case event_type::exhumes_grave: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                add( pgettext( "memorial_male", "Exhumed a grave." ),
                     pgettext( "memorial_female", "Exhumed a grave." ) );
            }
            break;
        }
        case event_type::fails_to_install_cbm: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                std::string cbm_name = e.get<bionic_id>( "bionic" )->name.translated();
                add( pgettext( "memorial_male", "Failed install of bionic: %s." ),
                     pgettext( "memorial_female", "Failed install of bionic: %s." ),
                     cbm_name );
            }
            break;
        }
        case event_type::fails_to_remove_cbm: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                std::string cbm_name = e.get<bionic_id>( "bionic" )->name.translated();
                add( pgettext( "memorial_male", "Failed to remove bionic: %s." ),
                     pgettext( "memorial_female", "Failed to remove bionic: %s." ),
                     cbm_name );
            }
            break;
        }
        case event_type::falls_asleep_from_exhaustion: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                add( pgettext( "memorial_male", "Succumbed to lack of sleep." ),
                     pgettext( "memorial_female", "Succumbed to lack of sleep." ) );
            }
            break;
        }
        case event_type::fuel_tank_explodes: {
            std::string name = e.get<cata_variant_type::string>( "vehicle_name" );
            add( pgettext( "memorial_male", "The fuel tank of the %s exploded!" ),
                 pgettext( "memorial_female", "The fuel tank of the %s exploded!" ),
                 name );
            break;
        }
        case event_type::gains_addiction: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                addiction_id type = e.get<addiction_id>( "add_type" );
                //~ %s is addiction name
                add( pgettext( "memorial_male", "Became addicted to %s." ),
                     pgettext( "memorial_female", "Became addicted to %s." ),
                     type->get_type_name().translated() );
            }
            break;
        }
        case event_type::gains_mutation: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                trait_id trait = e.get<trait_id>( "trait" );
                add( pgettext( "memorial_male", "Gained the mutation '%s'." ),
                     pgettext( "memorial_female", "Gained the mutation '%s'." ),
                     get_avatar().mutation_name( trait ) );
            }
            break;
        }
        case event_type::gains_proficiency: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                proficiency_id proficiency = e.get<proficiency_id>( "proficiency" );
                add( pgettext( "memorial_male", "Gained the proficiency '%s'." ),
                     pgettext( "memorial_female", "Gained the proficiency '%s'." ),
                     proficiency->name() );
            }
            break;
        }
        case event_type::gains_skill_level: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                int new_level = e.get<int>( "new_level" );
                if( new_level % 4 == 0 ) {
                    skill_id skill = e.get<skill_id>( "skill" );
                    add( pgettext( "memorial_male",
                                   //~ %d is skill level %s is skill name
                                   "Reached skill level %1$d in %2$s." ),
                         pgettext( "memorial_female",
                                   //~ %d is skill level %s is skill name
                                   "Reached skill level %1$d in %2$s." ),
                         new_level, skill->name() );
                }
            }
            break;
        }
        case event_type::game_avatar_death: {
            bool suicide = e.get<bool>( "is_suicide" );
            std::string last_words = e.get<cata_variant_type::string>( "last_words" );
            if( suicide ) {
                add( pgettext( "memorial_male", "%s committed suicide." ),
                     pgettext( "memorial_female", "%s committed suicide." ),
                     avatar_name );
            } else {
                add( pgettext( "memorial_male", "%s was killed." ),
                     pgettext( "memorial_female", "%s was killed." ),
                     avatar_name );
            }
            if( !last_words.empty() ) {
                add( pgettext( "memorial_male", "Last words: %s" ),
                     pgettext( "memorial_female", "Last words: %s" ),
                     last_words );
            }
            break;
        }
        case event_type::game_avatar_new: {
            bool new_game = e.get<bool>( "is_new_game" );
            if( new_game ) {
                add( //~ %s is player name
                    pgettext( "memorial_male", "%s began their journey into the Cataclysm." ),
                    pgettext( "memorial_female", "%s began their journey into the Cataclysm." ),
                    avatar_name );
            } else {
                add( //~ %s is player name
                    pgettext( "memorial_male", "%s took over the journey through the Cataclysm." ),
                    pgettext( "memorial_female", "%s took over the journey through the Cataclysm." ),
                    avatar_name );
            }
            break;
        }
        case event_type::installs_cbm: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                std::string cbm_name = e.get<bionic_id>( "bionic" )->name.translated();
                add( pgettext( "memorial_male", "Installed bionic: %s." ),
                     pgettext( "memorial_female", "Installed bionic: %s." ),
                     cbm_name );
            }
            break;
        }
        case event_type::installs_faulty_cbm: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                std::string cbm_name = e.get<bionic_id>( "bionic" )->name.translated();
                add( pgettext( "memorial_male", "Installed bad bionic: %s." ),
                     pgettext( "memorial_female", "Installed bad bionic: %s." ),
                     cbm_name );
            }
            break;
        }
        case event_type::learns_martial_art: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                matype_id mastyle = e.get<matype_id>( "martial_art" );
                //~ %s is martial art
                add( pgettext( "memorial_male", "Learned %s." ),
                     pgettext( "memorial_female", "Learned %s." ),
                     mastyle->name );
            }
            break;
        }
        case event_type::loses_addiction: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                addiction_id type = e.get<addiction_id>( "add_type" );
                //~ %s is addiction name
                add( pgettext( "memorial_male", "Overcame addiction to %s." ),
                     pgettext( "memorial_female", "Overcame addiction to %s." ),
                     type->get_type_name().translated() );
            }
            break;
        }
        case event_type::loses_mutation: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                trait_id trait = e.get<trait_id>( "trait" );
                add( pgettext( "memorial_male", "Lost the mutation '%s'." ),
                     pgettext( "memorial_female", "Lost the mutation '%s'." ),
                     get_avatar().mutation_name( trait ) );
            }
            break;
        }
        case event_type::npc_becomes_hostile: {
            std::string name = e.get<cata_variant_type::string>( "npc_name" );
            add( pgettext( "memorial_male", "%s became hostile." ),
                 pgettext( "memorial_female", "%s became hostile." ),
                 name );
            break;
        }
        case event_type::opens_portal: {
            add( pgettext( "memorial_male", "Opened a portal." ),
                 pgettext( "memorial_female", "Opened a portal." ) );
            break;
        }
        case event_type::opens_temple: {
            add( pgettext( "memorial_male", "Opened a strange temple." ),
                 pgettext( "memorial_female", "Opened a strange temple." ) );
            break;
        }
        case event_type::player_fails_conduct: {
            add( pgettext( "memorial_male", "Lost the conduct %s%s." ),
                 pgettext( "memorial_female", "Lost the conduct %s%s." ),
                 e.get<achievement_id>( "conduct" )->name(),
                 e.get<bool>( "achievements_enabled" ) ? "" : _( " (disabled)" ) );
            break;
        }
        case event_type::player_gets_achievement: {
            add( pgettext( "memorial_male", "Gained the achievement %s%s." ),
                 pgettext( "memorial_female", "Gained the achievement %s%s." ),
                 e.get<achievement_id>( "achievement" )->name(),
                 e.get<bool>( "achievements_enabled" ) ? "" : _( " (disabled)" ) );
            break;
        }
        case event_type::character_forgets_spell: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                std::string spell_name = e.get<spell_id>( "spell" )->name.translated();
                add( pgettext( "memorial_male", "Forgot the spell %s." ),
                     pgettext( "memorial_female", "Forgot the spell %s." ),
                     spell_name );
            }
            break;
        }
        case event_type::character_learns_spell: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                std::string spell_name = e.get<spell_id>( "spell" )->name.translated();
                add( pgettext( "memorial_male", "Learned the spell %s." ),
                     pgettext( "memorial_female", "Learned the spell %s." ),
                     spell_name );
            }
            break;
        }
        case event_type::player_levels_spell: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                std::string spell_name = e.get<spell_id>( "spell" )->name.translated();
                add( pgettext( "memorial_male", "Gained a spell level on %s." ),
                     pgettext( "memorial_female", "Gained a spell level on %s." ),
                     spell_name );
            }
            break;
        }
        case event_type::releases_subspace_specimens: {
            add( pgettext( "memorial_male", "Released subspace specimens." ),
                 pgettext( "memorial_female", "Released subspace specimens." ) );
            break;
        }
        case event_type::removes_cbm: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                std::string cbm_name = e.get<bionic_id>( "bionic" )->name.translated();
                add( pgettext( "memorial_male", "Removed bionic: %s." ),
                     pgettext( "memorial_female", "Removed bionic: %s." ),
                     cbm_name );
            }
            break;
        }
        case event_type::seals_hazardous_material_sarcophagus: {
            add( pgettext( "memorial_male", "Sealed a Hazardous Material Sarcophagus." ),
                 pgettext( "memorial_female", "Sealed a Hazardous Material Sarcophagus." ) );
            break;
        }
        case event_type::telefrags_creature: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                std::string victim_name = e.get<cata_variant_type::string>( "victim_name" );
                add( pgettext( "memorial_male", "Telefragged a %s." ),
                     pgettext( "memorial_female", "Telefragged a %s." ),
                     victim_name );
            }
            break;
        }
        case event_type::teleglow_teleports: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                add( pgettext( "memorial_male", "Spontaneous teleport." ),
                     pgettext( "memorial_female", "Spontaneous teleport." ) );
            }
            break;
        }
        case event_type::teleports_into_wall: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                std::string obstacle_name = e.get<cata_variant_type::string>( "obstacle_name" );
                add( pgettext( "memorial_male", "Teleported into a %s." ),
                     pgettext( "memorial_female", "Teleported into a %s." ),
                     obstacle_name );
            }
            break;
        }
        case event_type::terminates_subspace_specimens: {
            add( pgettext( "memorial_male", "Terminated subspace specimens." ),
                 pgettext( "memorial_female", "Terminated subspace specimens." ) );
            break;
        }
        case event_type::throws_up: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == avatar_id ) {
                add( pgettext( "memorial_male", "Threw up." ),
                     pgettext( "memorial_female", "Threw up." ) );
            }
            break;
        }
        case event_type::triggers_alarm: {
            add( pgettext( "memorial_male", "Set off an alarm." ),
                 pgettext( "memorial_female", "Set off an alarm." ) );
            break;
        }
        case event_type::uses_debug_menu: {
            add( pgettext( "memorial_male", "Used the debug menu (%s)." ),
                 pgettext( "memorial_female", "Used the debug menu (%s)." ),
                 io::enum_to_string( e.get<debug_menu::debug_menu_index>( "debug_menu_option" ) ) );
            break;
        }
        // All the events for which we have no memorial log are here
        case event_type::avatar_enters_omt:
        case event_type::avatar_moves:
        case event_type::camp_taken_over:
        case event_type::character_consumes_item:
        case event_type::character_dies:
        case event_type::character_eats_item:
        case event_type::character_finished_activity:
        case event_type::character_gets_headshot:
        case event_type::character_heals_damage:
        case event_type::character_melee_attacks_character:
        case event_type::character_melee_attacks_monster:
        case event_type::character_ranged_attacks_character:
        case event_type::character_ranged_attacks_monster:
        case event_type::character_smashes_tile:
        case event_type::character_starts_activity:
        case event_type::character_takes_damage:
        case event_type::monster_takes_damage:
        case event_type::character_wakes_up:
        case event_type::character_attempt_to_fall_asleep:
        case event_type::character_falls_asleep:
        case event_type::character_radioactively_mutates:
        case event_type::character_wears_item:
        case event_type::character_takeoff_item:
        case event_type::character_wields_item:
        case event_type::character_armor_destroyed:
        case event_type::character_casts_spell:
        case event_type::cuts_tree:
        case event_type::opens_spellbook:
        case event_type::reads_book:
        case event_type::spellcasting_finish:
        case event_type::game_load:
        case event_type::game_over:
        case event_type::game_save:
        case event_type::game_start:
        case event_type::game_begin:
        case event_type::u_var_changed:
        case event_type::vehicle_moves:
        case event_type::character_butchered_corpse:
            break;
        case event_type::num_event_types: {
            debugmsg( "Invalid event type" );
            break;
        }

    }
}
