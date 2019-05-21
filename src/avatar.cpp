#include "avatar.h"

#include "bionics.h"
#include "character.h"
#include "filesystem.h"
#include "game.h"
#include "inventory.h"
#include "item.h"
#include "messages.h"
#include "monstergenerator.h"
#include "mutation.h"
#include "overmapbuffer.h"
#include "player.h"
#include "profession.h"
#include "skill.h"
#include "type_id.h"
#include "get_version.h"

void avatar::memorial( std::ostream &memorial_file, const std::string &epitaph )
{
    static const char *eol = cata_files::eol();

    //Size of indents in the memorial file
    const std::string indent = "  ";

    const std::string pronoun = male ? _( "He" ) : _( "She" );

    //Avoid saying "a male unemployed" or similar
    std::string profession_name;
    if( prof == prof->generic( ) ) {
        if( male ) {
            profession_name = _( "an unemployed male" );
        } else {
            profession_name = _( "an unemployed female" );
        }
    } else {
        profession_name = string_format( _( "a %s" ), prof->gender_appropriate_name( male ) );
    }

    const std::string locdesc = overmap_buffer.get_description_at( global_sm_location() );
    //~ First parameter is a pronoun ("He"/"She"), second parameter is a description
    // that designates the location relative to its surroundings.
    const std::string kill_place = string_format( _( "%1$s was killed in a %2$s." ),
                                   pronoun, locdesc );

    //Header
    memorial_file << string_format( _( "Cataclysm - Dark Days Ahead version %s memorial file" ),
                                    getVersionString() ) << eol;
    memorial_file << eol;
    memorial_file << string_format( _( "In memory of: %s" ), name ) << eol;
    if( epitaph.length() > 0 ) {  //Don't record empty epitaphs
        //~ The "%s" will be replaced by an epitaph as displayed in the memorial files. Replace the quotation marks as appropriate for your language.
        memorial_file << string_format( pgettext( "epitaph", "\"%s\"" ), epitaph ) << eol << eol;
    }
    //~ First parameter: Pronoun, second parameter: a profession name (with article)
    memorial_file << string_format( _( "%1$s was %2$s when the apocalypse began." ),
                                    pronoun, profession_name ) << eol;
    memorial_file << string_format( _( "%1$s died on %2$s." ), pronoun,
                                    to_string( time_point( calendar::turn ) ) ) << eol;
    memorial_file << kill_place << eol;
    memorial_file << eol;

    //Misc
    memorial_file << string_format( _( "Cash on hand: %s" ), format_money( cash ) ) << eol;
    memorial_file << eol;

    //HP

    const auto limb_hp =
    [this, &memorial_file, &indent]( const std::string & desc, const hp_part bp ) {
        memorial_file << indent << string_format( desc, get_hp( bp ), get_hp_max( bp ) ) << eol;
    };

    memorial_file << _( "Final HP:" ) << eol;
    limb_hp( _( " Head: %d/%d" ), hp_head );
    limb_hp( _( "Torso: %d/%d" ), hp_torso );
    limb_hp( _( "L Arm: %d/%d" ), hp_arm_l );
    limb_hp( _( "R Arm: %d/%d" ), hp_arm_r );
    limb_hp( _( "L Leg: %d/%d" ), hp_leg_l );
    limb_hp( _( "R Leg: %d/%d" ), hp_leg_r );
    memorial_file << eol;

    //Stats
    memorial_file << _( "Final Stats:" ) << eol;
    memorial_file << indent << string_format( _( "Str %d" ), str_cur )
                  << indent << string_format( _( "Dex %d" ), dex_cur )
                  << indent << string_format( _( "Int %d" ), int_cur )
                  << indent << string_format( _( "Per %d" ), per_cur ) << eol;
    memorial_file << _( "Base Stats:" ) << eol;
    memorial_file << indent << string_format( _( "Str %d" ), str_max )
                  << indent << string_format( _( "Dex %d" ), dex_max )
                  << indent << string_format( _( "Int %d" ), int_max )
                  << indent << string_format( _( "Per %d" ), per_max ) << eol;
    memorial_file << eol;

    //Last 20 messages
    memorial_file << _( "Final Messages:" ) << eol;
    std::vector<std::pair<std::string, std::string> > recent_messages = Messages::recent_messages( 20 );
    for( const std::pair<std::string, std::string> &recent_message : recent_messages ) {
        memorial_file << indent << recent_message.first << " " << recent_message.second;
        memorial_file << eol;
    }
    memorial_file << eol;

    //Kill list
    memorial_file << _( "Kills:" ) << eol;

    int total_kills = 0;

    std::map<std::tuple<std::string, std::string>, int> kill_counts;

    // map <name, sym> to kill count
    for( const mtype &type : MonsterGenerator::generator().get_all_mtypes() ) {
        if( g->kill_count( type.id ) > 0 ) {
            kill_counts[std::tuple<std::string, std::string>(
                                                    type.nname(),
                                                    type.sym
                                                )] += g->kill_count( type.id );
            total_kills += g->kill_count( type.id );
        }
    }

    for( const std::pair<std::tuple<std::string, std::string>, int> &entry : kill_counts ) {
        memorial_file << "  " << std::get<1>( entry.first ) << " - "
                      << string_format( "%4d", entry.second ) << " "
                      << std::get<0>( entry.first ) << eol;
    }

    if( total_kills == 0 ) {
        memorial_file << indent << _( "No monsters were killed." ) << eol;
    } else {
        memorial_file << string_format( _( "Total kills: %d" ), total_kills ) << eol;
    }
    memorial_file << eol;

    //Skills
    memorial_file << _( "Skills:" ) << eol;
    for( const std::pair<skill_id, SkillLevel> &pair : *_skills ) {
        const SkillLevel &lobj = pair.second;
        //~ 1. skill name, 2. skill level, 3. exercise percentage to next level
        memorial_file << indent << string_format( _( "%s: %d (%d %%)" ), pair.first->name(), lobj.level(),
                      lobj.exercise() ) << eol;
    }
    memorial_file << eol;

    //Traits
    memorial_file << _( "Traits:" ) << eol;
    for( const std::pair<trait_id, Character::trait_data> &iter : my_mutations ) {
        memorial_file << indent << mutation_branch::get_name( iter.first ) << eol;
    }
    if( !my_mutations.empty() ) {
        memorial_file << indent << _( "(None)" ) << eol;
    }
    memorial_file << eol;

    //Effects (illnesses)
    memorial_file << _( "Ongoing Effects:" ) << eol;
    bool had_effect = false;
    if( get_perceived_pain() > 0 ) {
        had_effect = true;
        memorial_file << indent << _( "Pain" ) << " (" << get_perceived_pain() << ")";
    }
    if( !had_effect ) {
        memorial_file << indent << _( "(None)" ) << eol;
    }
    memorial_file << eol;

    //Bionics
    memorial_file << _( "Bionics:" ) << eol;
    int total_bionics = 0;
    for( size_t i = 0; i < my_bionics->size(); ++i ) {
        memorial_file << indent << ( i + 1 ) << ": " << ( *my_bionics )[i].id->name << eol;
        total_bionics++;
    }
    if( total_bionics == 0 ) {
        memorial_file << indent << _( "No bionics were installed." ) << eol;
    } else {
        memorial_file << string_format( _( "Total bionics: %d" ), total_bionics ) << eol;
    }
    memorial_file << string_format(
                      _( "Bionic Power: <color_light_blue>%d</color>/<color_light_blue>%d</color>" ), power_level,
                      max_power_level ) << eol;
    memorial_file << eol;

    //Equipment
    memorial_file << _( "Weapon:" ) << eol;
    memorial_file << indent << weapon.invlet << " - " << weapon.tname( 1, false ) << eol;
    memorial_file << eol;

    memorial_file << _( "Equipment:" ) << eol;
    for( const item &elem : worn ) {
        item next_item = elem;
        memorial_file << indent << next_item.invlet << " - " << next_item.tname( 1, false );
        if( next_item.charges > 0 ) {
            memorial_file << " (" << next_item.charges << ")";
        } else if( next_item.contents.size() == 1 && next_item.contents.front().charges > 0 ) {
            memorial_file << " (" << next_item.contents.front().charges << ")";
        }
        memorial_file << eol;
    }
    memorial_file << eol;

    //Inventory
    memorial_file << _( "Inventory:" ) << eol;
    inv.restack( *this );
    invslice slice = inv.slice();
    for( const std::list<item> *elem : slice ) {
        const item &next_item = elem->front();
        memorial_file << indent << next_item.invlet << " - " <<
                      next_item.tname( static_cast<unsigned>( elem->size() ), false );
        if( elem->size() > 1 ) {
            memorial_file << " [" << elem->size() << "]";
        }
        if( next_item.charges > 0 ) {
            memorial_file << " (" << next_item.charges << ")";
        } else if( next_item.contents.size() == 1 && next_item.contents.front().charges > 0 ) {
            memorial_file << " (" << next_item.contents.front().charges << ")";
        }
        memorial_file << eol;
    }
    memorial_file << eol;

    //Lifetime stats
    memorial_file << _( "Lifetime Stats" ) << eol;
    memorial_file << indent << string_format( _( "Distance walked: %d squares" ),
                  lifetime_stats.squares_walked ) << eol;
    memorial_file << indent << string_format( _( "Damage taken: %d damage" ),
                  lifetime_stats.damage_taken ) << eol;
    memorial_file << indent << string_format( _( "Damage healed: %d damage" ),
                  lifetime_stats.damage_healed ) << eol;
    memorial_file << indent << string_format( _( "Headshots: %d" ),
                  lifetime_stats.headshots ) << eol;
    memorial_file << eol;

    //History
    memorial_file << _( "Game History" ) << eol;
    memorial_file << dump_memorial();
}
