#include "memorial_logger.h"

#include <sstream>

#include "addiction.h"
#include "avatar.h"
#include "bionics.h"
#include "effect.h"
#include "filesystem.h"
#include "game.h"
#include "get_version.h"
#include "item_factory.h"
#include "itype.h"
#include "kill_tracker.h"
#include "martialarts.h"
#include "messages.h"
#include "monstergenerator.h"
#include "mutation.h"
#include "overmapbuffer.h"
#include "profession.h"
#include "skill.h"

static const efftype_id effect_adrenaline( "adrenaline" );
static const efftype_id effect_datura( "datura" );
static const efftype_id effect_drunk( "drunk" );
static const efftype_id effect_jetinjector( "jetinjector" );

static const trap_str_id tr_bubblewrap( "tr_bubblewrap" );
static const trap_str_id tr_glass( "tr_glass" );
static const trap_str_id tr_beartrap( "tr_beartrap" );
static const trap_str_id tr_nailboard( "tr_nailboard" );
static const trap_str_id tr_caltrops( "tr_caltrops" );
static const trap_str_id tr_caltrops_glass( "tr_caltrops_glass" );
static const trap_str_id tr_tripwire( "tr_tripwire" );
static const trap_str_id tr_crossbow( "tr_crossbow" );
static const trap_str_id tr_shotgun_2( "tr_shotgun_2" );
static const trap_str_id tr_shotgun_1( "tr_shotgun_1" );
static const trap_str_id tr_blade( "tr_blade" );
static const trap_str_id tr_landmine( "tr_landmine" );
static const trap_str_id tr_light_snare( "tr_light_snare" );
static const trap_str_id tr_heavy_snare( "tr_heavy_snare" );
static const trap_str_id tr_telepad( "tr_telepad" );
static const trap_str_id tr_goo( "tr_goo" );
static const trap_str_id tr_dissector( "tr_dissector" );
static const trap_str_id tr_sinkhole( "tr_sinkhole" );
static const trap_str_id tr_pit( "tr_pit" );
static const trap_str_id tr_spike_pit( "tr_spike_pit" );
static const trap_str_id tr_lava( "tr_lava" );
static const trap_str_id tr_portal( "tr_portal" );
static const trap_str_id tr_ledge( "tr_ledge" );
static const trap_str_id tr_boobytrap( "tr_boobytrap" );
static const trap_str_id tr_temple_flood( "tr_temple_flood" );
static const trap_str_id tr_shadow( "tr_shadow" );
static const trap_str_id tr_drain( "tr_drain" );
static const trap_str_id tr_snake( "tr_snake" );
static const trap_str_id tr_glass_pit( "tr_glass_pit" );

static const trait_id trait_CANNIBAL( "CANNIBAL" );
static const trait_id trait_PSYCHOPATH( "PSYCHOPATH" );
static const trait_id trait_SAPIOVORE( "SAPIOVORE" );

void memorial_logger::clear()
{
    log.clear();
}

/**
 * Adds an event to the memorial log, to be written to the memorial file when
 * the character dies. The message should contain only the informational string,
 * as the timestamp and location will be automatically prepended.
 */
void memorial_logger::add( const std::string &male_msg,
                           const std::string &female_msg )
{
    const std::string &msg = g->u.male ? male_msg : female_msg;

    if( msg.empty() ) {
        return;
    }

    const oter_id &cur_ter = overmap_buffer.ter( g->u.global_omt_location() );
    const std::string &location = cur_ter->get_name();

    std::stringstream log_message;
    log_message << "| " << to_string( calendar::turn ) << " | " << location << " | " <<
                msg;

    log.push_back( log_message.str() );
}

/**
 * Loads the data in a memorial file from the given ifstream. All the memorial
 * entry lines begin with a pipe (|).
 * @param fin The ifstream to read the memorial entries from.
 */
void memorial_logger::load( std::istream &fin )
{
    std::string entry;
    log.clear();
    while( fin.peek() == '|' ) {
        getline( fin, entry );
        // strip all \r from end of string
        while( *entry.rbegin() == '\r' ) {
            entry.pop_back();
        }
        log.push_back( entry );
    }
}

/**
 * Concatenates all of the memorial log entries, delimiting them with newlines,
 * and returns the resulting string. Used for saving and for writing out to the
 * memorial file.
 */
std::string memorial_logger::dump() const
{
    static const char *eol = cata_files::eol();
    std::stringstream output;

    for( auto &elem : log ) {
        output << elem << eol;
    }

    return output.str();
}

void memorial_logger::write( std::ostream &file, const std::string &epitaph ) const
{
    avatar &u = g->u;
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

    const std::string locdesc = overmap_buffer.get_description_at( u.global_sm_location() );
    //~ First parameter is a pronoun ("He"/"She"), second parameter is a description
    // that designates the location relative to its surroundings.
    const std::string kill_place = string_format( _( "%1$s was killed in a %2$s." ),
                                   pronoun, locdesc );

    //Header
    file << string_format( _( "Cataclysm - Dark Days Ahead version %s memorial file" ),
                           getVersionString() ) << eol;
    file << eol;
    file << string_format( _( "In memory of: %s" ), u.name ) << eol;
    if( epitaph.length() > 0 ) {  //Don't record empty epitaphs
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
    [&file, &indent, &u]( const std::string & desc, const hp_part bp ) {
        file << indent <<
             string_format( desc, u.get_hp( bp ), u.get_hp_max( bp ) ) << eol;
    };

    file << _( "Final HP:" ) << eol;
    limb_hp( _( " Head: %d/%d" ), hp_head );
    limb_hp( _( "Torso: %d/%d" ), hp_torso );
    limb_hp( _( "L Arm: %d/%d" ), hp_arm_l );
    limb_hp( _( "R Arm: %d/%d" ), hp_arm_r );
    limb_hp( _( "L Leg: %d/%d" ), hp_leg_l );
    limb_hp( _( "R Leg: %d/%d" ), hp_leg_r );
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

    for( const std::pair<std::tuple<std::string, std::string>, int> &entry : kill_counts ) {
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
    for( const std::pair<skill_id, SkillLevel> &pair : u.get_all_skills() ) {
        const SkillLevel &lobj = pair.second;
        //~ 1. skill name, 2. skill level, 3. exercise percentage to next level
        file << indent << string_format( _( "%s: %d (%d %%)" ), pair.first->name(), lobj.level(),
                                         lobj.exercise() ) << eol;
    }
    file << eol;

    //Traits
    file << _( "Traits:" ) << eol;
    for( const trait_id &mut : u.get_mutations() ) {
        file << indent << mutation_branch::get_name( mut ) << eol;
    }
    if( u.get_mutations().empty() ) {
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
    for( const bionic_id bionic : u.get_bionics() ) {
        file << indent << bionic->name << eol;
    }
    if( u.get_bionics().empty() ) {
        file << indent << _( "No bionics were installed." ) << eol;
    }
    file << string_format(
             _( "Bionic Power: <color_light_blue>%d</color>/<color_light_blue>%d</color>" ),
             u.power_level, u.max_power_level ) << eol;
    file << eol;

    //Equipment
    file << _( "Weapon:" ) << eol;
    file << indent << u.weapon.invlet << " - " << u.weapon.tname( 1, false ) << eol;
    file << eol;

    file << _( "Equipment:" ) << eol;
    for( const item &elem : u.worn ) {
        item next_item = elem;
        file << indent << next_item.invlet << " - " << next_item.tname( 1, false );
        if( next_item.charges > 0 ) {
            file << " (" << next_item.charges << ")";
        } else if( next_item.contents.size() == 1 && next_item.contents.front().charges > 0 ) {
            file << " (" << next_item.contents.front().charges << ")";
        }
        file << eol;
    }
    file << eol;

    //Inventory
    file << _( "Inventory:" ) << eol;
    u.inv.restack( u );
    invslice slice = u.inv.slice();
    for( const std::list<item> *elem : slice ) {
        const item &next_item = elem->front();
        file << indent << next_item.invlet << " - " <<
             next_item.tname( static_cast<unsigned>( elem->size() ), false );
        if( elem->size() > 1 ) {
            file << " [" << elem->size() << "]";
        }
        if( next_item.charges > 0 ) {
            file << " (" << next_item.charges << ")";
        } else if( next_item.contents.size() == 1 && next_item.contents.front().charges > 0 ) {
            file << " (" << next_item.contents.front().charges << ")";
        }
        file << eol;
    }
    file << eol;

    //Lifetime stats
    file << _( "Lifetime Stats" ) << eol;
    file << indent << string_format( _( "Distance walked: %d squares" ),
                                     u.lifetime_stats.squares_walked ) << eol;
    file << indent << string_format( _( "Damage taken: %d damage" ),
                                     u.lifetime_stats.damage_taken ) << eol;
    file << indent << string_format( _( "Damage healed: %d damage" ),
                                     u.lifetime_stats.damage_healed ) << eol;
    file << indent << string_format( _( "Headshots: %d" ),
                                     u.lifetime_stats.headshots ) << eol;
    file << eol;

    //History
    file << _( "Game History" ) << eol;
    file << dump();
}

void memorial_logger::notify( const event &e )
{
    switch( e.type() ) {
        case event_type::activates_artifact: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == g->u.getID() ) {
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
            if( ch == g->u.getID() ) {
                add( pgettext( "memorial_male", "Activated a mininuke." ),
                     pgettext( "memorial_female", "Activated a mininuke." ) );
            }
            break;
        }
        case event_type::administers_mutagen: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == g->u.getID() ) {
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
        case event_type::awakes_dark_wyrms: {
            add( pgettext( "memorial_male", "Awoke a group of dark wyrms!" ),
                 pgettext( "memorial_female", "Awoke a group of dark wyrms!" ) );
            break;
        }
        case event_type::becomes_wanted: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == g->u.getID() ) {
                add( pgettext( "memorial_male", "Became wanted by the police!" ),
                     pgettext( "memorial_female", "Became wanted by the police!" ) );
            }
            break;
        }
        case event_type::broken_bone_mends: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == g->u.getID() ) {
                body_part part = e.get<body_part>( "part" );
                //~ %s is bodypart
                add( pgettext( "memorial_male", "Broken %s began to mend." ),
                     pgettext( "memorial_female", "Broken %s began to mend." ),
                     body_part_name( part ) );
            }
            break;
        }
        case event_type::buries_corpse: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == g->u.getID() ) {
                const mtype &corpse_type = e.get<mtype_id>( "corpse_type" ).obj();
                std::string corpse_name = e.get<cata_variant_type::string>( "corpse_name" );
                if( corpse_name.empty() ) {
                    if( corpse_type.has_flag( MF_HUMAN ) ) {
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
            if( ch == g->u.getID() ) {
                const effect_type &type = e.get<efftype_id>( "effect" ).obj();
                const std::string message = type.get_apply_memorial_log();
                if( !message.empty() ) {
                    add( pgettext( "memorial_male", message.c_str() ),
                         pgettext( "memorial_female", message.c_str() ) );
                }
            }
            break;
        }
        case event_type::character_kills_character: {
            character_id ch = e.get<character_id>( "killer" );
            if( ch == g->u.getID() ) {
                std::string name = e.get<cata_variant_type::string>( "victim_name" );
                bool cannibal = g->u.has_trait( trait_CANNIBAL );
                bool psycho = g->u.has_trait( trait_PSYCHOPATH );
                if( g->u.has_trait( trait_SAPIOVORE ) ) {
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
            if( ch == g->u.getID() ) {
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
            if( ch == g->u.getID() ) {
                const effect_type &type = e.get<efftype_id>( "effect" ).obj();
                const std::string message = type.get_remove_memorial_log();
                if( !message.empty() ) {
                    add( pgettext( "memorial_male", message.c_str() ),
                         pgettext( "memorial_female", message.c_str() ) );
                }
            }
            break;
        }
        case event_type::character_triggers_trap: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == g->u.getID() ) {
                trap_str_id trap = e.get<trap_str_id>( "trap" );
                if( trap == tr_bubblewrap ) {
                    add( pgettext( "memorial_male", "Stepped on bubble wrap." ),
                         pgettext( "memorial_female", "Stepped on bubble wrap." ) );
                } else if( trap == tr_glass ) {
                    add( pgettext( "memorial_male", "Stepped on glass." ),
                         pgettext( "memorial_female", "Stepped on glass." ) );
                } else if( trap == tr_beartrap ) {
                    add( pgettext( "memorial_male", "Caught by a beartrap." ),
                         pgettext( "memorial_female", "Caught by a beartrap." ) );
                } else if( trap == tr_nailboard ) {
                    add( pgettext( "memorial_male", "Stepped on a spiked board." ),
                         pgettext( "memorial_female", "Stepped on a spiked board." ) );
                } else if( trap == tr_caltrops ) {
                    add( pgettext( "memorial_male", "Stepped on a caltrop." ),
                         pgettext( "memorial_female", "Stepped on a caltrop." ) );
                } else if( trap == tr_caltrops_glass ) {
                    add( pgettext( "memorial_male", "Stepped on a glass caltrop." ),
                         pgettext( "memorial_female", "Stepped on a glass caltrop." ) );
                } else if( trap == tr_tripwire ) {
                    add( pgettext( "memorial_male", "Tripped on a tripwire." ),
                         pgettext( "memorial_female", "Tripped on a tripwire." ) );
                } else if( trap == tr_crossbow ) {
                    add( pgettext( "memorial_male", "Triggered a crossbow trap." ),
                         pgettext( "memorial_female", "Triggered a crossbow trap." ) );
                } else if( trap == tr_shotgun_1 || trap == tr_shotgun_2 ) {
                    add( pgettext( "memorial_male", "Triggered a shotgun trap." ),
                         pgettext( "memorial_female", "Triggered a shotgun trap." ) );
                } else if( trap == tr_blade ) {
                    add( pgettext( "memorial_male", "Triggered a blade trap." ),
                         pgettext( "memorial_female", "Triggered a blade trap." ) );
                } else if( trap == tr_light_snare ) {
                    add( pgettext( "memorial_male", "Triggered a light snare." ),
                         pgettext( "memorial_female", "Triggered a light snare." ) );
                } else if( trap == tr_heavy_snare ) {
                    add( pgettext( "memorial_male", "Triggered a heavy snare." ),
                         pgettext( "memorial_female", "Triggered a heavy snare." ) );
                } else if( trap == tr_landmine ) {
                    add( pgettext( "memorial_male", "Stepped on a land mine." ),
                         pgettext( "memorial_female", "Stepped on a land mine." ) );
                } else if( trap == tr_boobytrap ) {
                    add( pgettext( "memorial_male", "Triggered a booby trap." ),
                         pgettext( "memorial_female", "Triggered a booby trap." ) );
                } else if( trap == tr_telepad || trap == tr_portal ) {
                    add( pgettext( "memorial_male", "Triggered a teleport trap." ),
                         pgettext( "memorial_female", "Triggered a teleport trap." ) );
                } else if( trap == tr_goo ) {
                    add( pgettext( "memorial_male", "Stepped into thick goo." ),
                         pgettext( "memorial_female", "Stepped into thick goo." ) );
                } else if( trap == tr_dissector ) {
                    add( pgettext( "memorial_male", "Stepped into a dissector." ),
                         pgettext( "memorial_female", "Stepped into a dissector." ) );
                } else if( trap == tr_pit ) {
                    add( pgettext( "memorial_male", "Fell in a pit." ),
                         pgettext( "memorial_female", "Fell in a pit." ) );
                } else if( trap == tr_spike_pit ) {
                    add( pgettext( "memorial_male", "Fell into a spiked pit." ),
                         pgettext( "memorial_female", "Fell into a spiked pit." ) );
                } else if( trap == tr_glass_pit ) {
                    add( pgettext( "memorial_male", "Fell into a pit filled with glass shards." ),
                         pgettext( "memorial_female", "Fell into a pit filled with glass shards." ) );
                } else if( trap == tr_lava ) {
                    add( pgettext( "memorial_male", "Stepped into lava." ),
                         pgettext( "memorial_female", "Stepped into lava." ) );
                } else if( trap == tr_sinkhole ) {
                    add( pgettext( "memorial_male", "Stepped into a sinkhole." ),
                         pgettext( "memorial_female", "Stepped into a sinkhole." ) );
                } else if( trap == tr_ledge ) {
                    add( pgettext( "memorial_male", "Fell down a ledge." ),
                         pgettext( "memorial_female", "Fell down a ledge." ) );
                } else if( trap == tr_temple_flood ) {
                    add( pgettext( "memorial_male", "Triggered a flood trap." ),
                         pgettext( "memorial_female", "Triggered a flood trap." ) );
                } else if( trap == tr_shadow ) {
                    add( pgettext( "memorial_male", "Triggered a shadow trap." ),
                         pgettext( "memorial_female", "Triggered a shadow trap." ) );
                } else if( trap == tr_drain ) {
                    add( pgettext( "memorial_male", "Triggered a life-draining trap." ),
                         pgettext( "memorial_female", "Triggered a life-draining trap." ) );
                } else if( trap == tr_snake ) {
                    add( pgettext( "memorial_male", "Triggered a shadow snake trap." ),
                         pgettext( "memorial_female", "Triggered a shadow snake trap." ) );
                }
            }
            break;
        }
        case event_type::consumes_marloss_item: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == g->u.getID() ) {
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
            if( ch == g->u.getID() ) {
                add( pgettext( "memorial_male", "Opened the Marloss Gateway." ),
                     pgettext( "memorial_female", "Opened the Marloss Gateway." ) );
            }
            break;
        }
        case event_type::crosses_mutation_threshold: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == g->u.getID() ) {
                std::string category_id =
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
            if( ch == g->u.getID() ) {
                add( pgettext( "memorial_male", "Became one with the Mycus." ),
                     pgettext( "memorial_female", "Became one with the Mycus." ) );
            }
            break;
        }
        case event_type::dermatik_eggs_hatch: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == g->u.getID() ) {
                add( pgettext( "memorial_male", "Dermatik eggs hatched." ),
                     pgettext( "memorial_female", "Dermatik eggs hatched." ) );
            }
            break;
        }
        case event_type::dermatik_eggs_injected: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == g->u.getID() ) {
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
            if( ch == g->u.getID() ) {
                add( pgettext( "memorial_male", "Succumbed to an asthma attack." ),
                     pgettext( "memorial_female", "Succumbed to an asthma attack." ) );
            }
            break;
        }
        case event_type::dies_from_drug_overdose: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == g->u.getID() ) {
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
        case event_type::dies_of_infection: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == g->u.getID() ) {
                add( pgettext( "memorial_male", "Succumbed to the infection." ),
                     pgettext( "memorial_female", "Succumbed to the infection." ) );
            }
            break;
        }
        case event_type::dies_of_starvation: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == g->u.getID() ) {
                add( pgettext( "memorial_male", "Died of starvation." ),
                     pgettext( "memorial_female", "Died of starvation." ) );
            }
            break;
        }
        case event_type::dies_of_thirst: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == g->u.getID() ) {
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
            if( ch == g->u.getID() ) {
                trait_id from = e.get<trait_id>( "from_trait" );
                trait_id to = e.get<trait_id>( "to_trait" );
                add( pgettext( "memorial_male", "'%s' mutation turned into '%s'" ),
                     pgettext( "memorial_female", "'%s' mutation turned into '%s'" ),
                     from->name(), to->name() );
            }
            break;
        }
        case event_type::exhumes_grave: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == g->u.getID() ) {
                add( pgettext( "memorial_male", "Exhumed a grave." ),
                     pgettext( "memorial_female", "Exhumed a grave." ) );
            }
            break;
        }
        case event_type::fails_to_install_cbm: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == g->u.getID() ) {
                std::string cbm_name = e.get<bionic_id>( "bionic" )->name;
                add( pgettext( "memorial_male", "Failed install of bionic: %s." ),
                     pgettext( "memorial_female", "Failed install of bionic: %s." ),
                     cbm_name );
            }
            break;
        }
        case event_type::fails_to_remove_cbm: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == g->u.getID() ) {
                std::string cbm_name = e.get<bionic_id>( "bionic" )->name;
                add( pgettext( "memorial_male", "Failed to remove bionic: %s." ),
                     pgettext( "memorial_female", "Failed to remove bionic: %s." ),
                     cbm_name );
            }
            break;
        }
        case event_type::falls_asleep_from_exhaustion: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == g->u.getID() ) {
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
            if( ch == g->u.getID() ) {
                add_type type = e.get<add_type>( "add_type" );
                const std::string &type_name = addiction_type_name( type );
                //~ %s is addiction name
                add( pgettext( "memorial_male", "Became addicted to %s." ),
                     pgettext( "memorial_female", "Became addicted to %s." ),
                     type_name );
            }
            break;
        }
        case event_type::gains_mutation: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == g->u.getID() ) {
                trait_id trait = e.get<trait_id>( "trait" );
                add( pgettext( "memorial_male", "Gained the mutation '%s'." ),
                     pgettext( "memorial_female", "Gained the mutation '%s'." ),
                     trait->name() );
            }
            break;
        }
        case event_type::gains_skill_level: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == g->u.getID() ) {
                skill_id skill = e.get<skill_id>( "skill" );
                int new_level = e.get<int>( "new_level" );
                if( new_level % 4 == 0 ) {
                    //~ %d is skill level %s is skill name
                    add( pgettext( "memorial_male",
                                   "Reached skill level %1$d in %2$s." ),
                         pgettext( "memorial_female",
                                   "Reached skill level %1$d in %2$s." ),
                         new_level, skill->name() );
                }
            }
            break;
        }
        case event_type::game_over: {
            bool suicide = e.get<bool>( "is_suicide" );
            std::string last_words = e.get<cata_variant_type::string>( "last_words" );
            if( suicide ) {
                add( pgettext( "memorial_male", "%s committed suicide." ),
                     pgettext( "memorial_female", "%s committed suicide." ),
                     g->u.name );
            } else {
                add( pgettext( "memorial_male", "%s was killed." ),
                     pgettext( "memorial_female", "%s was killed." ),
                     g->u.name );
            }
            if( !last_words.empty() ) {
                add( pgettext( "memorial_male", "Last words: %s" ),
                     pgettext( "memorial_female", "Last words: %s" ),
                     last_words );
            }
            break;
        }
        case event_type::game_start: {
            add( //~ %s is player name
                pgettext( "memorial_male", "%s began their journey into the Cataclysm." ),
                pgettext( "memorial_female", "%s began their journey into the Cataclysm." ),
                g->u.name );
            break;
        }
        case event_type::installs_cbm: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == g->u.getID() ) {
                std::string cbm_name = e.get<bionic_id>( "bionic" )->name;
                add( pgettext( "memorial_male", "Installed bionic: %s." ),
                     pgettext( "memorial_female", "Installed bionic: %s." ),
                     cbm_name );
            }
            break;
        }
        case event_type::installs_faulty_cbm: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == g->u.getID() ) {
                std::string cbm_name = e.get<bionic_id>( "bionic" )->name;
                add( pgettext( "memorial_male", "Installed bad bionic: %s." ),
                     pgettext( "memorial_female", "Installed bad bionic: %s." ),
                     cbm_name );
            }
            break;
        }
        case event_type::launches_nuke: {
            oter_id oter = e.get<oter_id>( "target_terrain" );
            //~ %s is terrain name
            add( pgettext( "memorial_male", "Launched a nuke at a %s." ),
                 pgettext( "memorial_female", "Launched a nuke at a %s." ),
                 oter->get_name() );
            break;
        }
        case event_type::learns_martial_art: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == g->u.getID() ) {
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
            if( ch == g->u.getID() ) {
                add_type type = e.get<add_type>( "add_type" );
                const std::string &type_name = addiction_type_name( type );
                //~ %s is addiction name
                add( pgettext( "memorial_male", "Overcame addiction to %s." ),
                     pgettext( "memorial_female", "Overcame addiction to %s." ),
                     type_name );
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
        case event_type::releases_subspace_specimens: {
            add( pgettext( "memorial_male", "Released subspace specimens." ),
                 pgettext( "memorial_female", "Released subspace specimens." ) );
            break;
        }
        case event_type::removes_cbm: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == g->u.getID() ) {
                std::string cbm_name = e.get<bionic_id>( "bionic" )->name;
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
            if( ch == g->u.getID() ) {
                std::string victim_name = e.get<cata_variant_type::string>( "victim_name" );
                add( pgettext( "memorial_male", "Telefragged a %s." ),
                     pgettext( "memorial_female", "Telefragged a %s." ),
                     victim_name );
            }
            break;
        }
        case event_type::teleglow_teleports: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == g->u.getID() ) {
                add( pgettext( "memorial_male", "Spontaneous teleport." ),
                     pgettext( "memorial_female", "Spontaneous teleport." ) );
            }
            break;
        }
        case event_type::teleports_into_wall: {
            character_id ch = e.get<character_id>( "character" );
            if( ch == g->u.getID() ) {
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
            if( ch == g->u.getID() ) {
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
        case event_type::num_event_types: {
            debugmsg( "Invalid event type" );
            break;
        }

    }
}
