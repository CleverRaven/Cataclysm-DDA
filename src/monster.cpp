#include "monster.h"

#include <cmath>
#include <algorithm>
#include <sstream>
#include <memory>
#include <tuple>
#include <unordered_map>

#include "avatar.h"
#include "coordinate_conversions.h"
#include "cursesdef.h"
#include "debug.h"
#include "effect.h"
#include "event_bus.h"
#include "explosion.h"
#include "field.h"
#include "game.h"
#include "item.h"
#include "itype.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "melee.h"
#include "messages.h"
#include "mission.h"
#include "mondeath.h"
#include "mondefense.h"
#include "monfaction.h"
#include "mongroup.h"
#include "morale_types.h"
#include "mutation.h"
#include "mtype.h"
#include "npc.h"
#include "optional.h"
#include "options.h"
#include "output.h"
#include "overmapbuffer.h"
#include "projectile.h"
#include "rng.h"
#include "sounds.h"
#include "string_formatter.h"
#include "text_snippets.h"
#include "translations.h"
#include "trap.h"
#include "character.h"
#include "compatibility.h"
#include "game_constants.h"
#include "mattack_common.h"
#include "pimpl.h"
#include "player.h"
#include "int_id.h"
#include "string_id.h"
#include "flat_set.h"
#include "weather.h"

struct pathfinding_settings;

// Limit the number of iterations for next upgrade_time calculations.
// This also sets the percentage of monsters that will never upgrade.
// The rough formula is 2^(-x), e.g. for x = 5 it's 0.03125 (~ 3%).
#define UPGRADE_MAX_ITERS 5

const mtype_id mon_ant( "mon_ant" );
const mtype_id mon_ant_fungus( "mon_ant_fungus" );
const mtype_id mon_ant_queen( "mon_ant_queen" );
const mtype_id mon_ant_soldier( "mon_ant_soldier" );
const mtype_id mon_bee( "mon_bee" );
const mtype_id mon_beekeeper( "mon_beekeeper" );
const mtype_id mon_boomer( "mon_boomer" );
const mtype_id mon_boomer_huge( "mon_boomer_huge" );
const mtype_id mon_boomer_fungus( "mon_boomer_fungus" );
const mtype_id mon_fungaloid( "mon_fungaloid" );
const mtype_id mon_triffid( "mon_triffid" );
const mtype_id mon_triffid_queen( "mon_triffid_queen" );
const mtype_id mon_triffid_young( "mon_triffid_young" );
const mtype_id mon_zombie( "mon_zombie" );
const mtype_id mon_zombie_bio_op( "mon_zombie_bio_op" );
const mtype_id mon_zombie_brute( "mon_zombie_brute" );
const mtype_id mon_zombie_brute_shocker( "mon_zombie_brute_shocker" );
const mtype_id mon_zombie_child( "mon_zombie_child" );
const mtype_id mon_zombie_cop( "mon_zombie_cop" );
const mtype_id mon_zombie_electric( "mon_zombie_electric" );
const mtype_id mon_zombie_fat( "mon_zombie_fat" );
const mtype_id mon_zombie_fireman( "mon_zombie_fireman" );
const mtype_id mon_zombie_fungus( "mon_zombie_fungus" );
const mtype_id mon_zombie_gasbag( "mon_zombie_gasbag" );
const mtype_id mon_zombie_gasbag_fungus( "mon_zombie_gasbag_fungus" );
const mtype_id mon_zombie_grabber( "mon_zombie_grabber" );
const mtype_id mon_zombie_hazmat( "mon_zombie_hazmat" );
const mtype_id mon_zombie_hulk( "mon_zombie_hulk" );
const mtype_id mon_skeleton_hulk( "mon_skeleton_hulk" );
const mtype_id mon_skeleton_hulk_fungus( "mon_skeleton_hulk_fungus" );
const mtype_id mon_skeleton_brute( "mon_skeleton_brute" );
const mtype_id mon_zombie_hunter( "mon_zombie_hunter" );
const mtype_id mon_zombie_master( "mon_zombie_master" );
const mtype_id mon_zombie_necro( "mon_zombie_necro" );
const mtype_id mon_zombie_rot( "mon_zombie_rot" );
const mtype_id mon_zombie_scientist( "mon_zombie_scientist" );
const mtype_id mon_zombie_scorched( "mon_zombie_scorched" );
const mtype_id mon_zombie_shrieker( "mon_zombie_shrieker" );
const mtype_id mon_zombie_smoker( "mon_zombie_smoker" );
const mtype_id mon_zombie_smoker_fungus( "mon_zombie_smoker_fungus" );
const mtype_id mon_zombie_soldier( "mon_zombie_soldier" );
const mtype_id mon_zombie_spitter( "mon_zombie_spitter" );
const mtype_id mon_zombie_survivor( "mon_zombie_survivor" );
const mtype_id mon_zombie_swimmer( "mon_zombie_swimmer" );
const mtype_id mon_zombie_technician( "mon_zombie_technician" );
const mtype_id mon_zombie_tough( "mon_zombie_tough" );
const mtype_id mon_zombie_child_fungus( "mon_zombie_child_fungus" );
const mtype_id mon_zombie_anklebiter( "mon_zombie_anklebiter" );
const mtype_id mon_zombie_creepy( "mon_zombie_creepy" );
const mtype_id mon_zombie_sproglodyte( "mon_zombie_sproglodyte" );
const mtype_id mon_zombie_shriekling( "mon_zombie_shriekling" );
const mtype_id mon_zombie_snotgobbler( "mon_zombie_snotgobbler" );
const mtype_id mon_zombie_waif( "mon_zombie_waif" );
const mtype_id mon_spider_fungus( "mon_spider_fungus" );

const species_id ZOMBIE( "ZOMBIE" );
const species_id FUNGUS( "FUNGUS" );
const species_id INSECT( "INSECT" );
const species_id MAMMAL( "MAMMAL" );
const species_id ABERRATION( "ABERRATION" );
const species_id MOLLUSK( "MOLLUSK" );
const species_id ROBOT( "ROBOT" );
const species_id FISH( "FISH" );
const species_id SPIDER( "SPIDER" );

const efftype_id effect_badpoison( "badpoison" );
const efftype_id effect_beartrap( "beartrap" );
const efftype_id effect_bleed( "bleed" );
const efftype_id effect_blind( "blind" );
const efftype_id effect_bouldering( "bouldering" );
const efftype_id effect_crushed( "crushed" );
const efftype_id effect_deaf( "deaf" );
const efftype_id effect_docile( "docile" );
const efftype_id effect_downed( "downed" );
const efftype_id effect_emp( "emp" );
const efftype_id effect_grabbed( "grabbed" );
const efftype_id effect_grabbing( "grabbing" );
const efftype_id effect_harnessed( "harnessed" );
const efftype_id effect_heavysnare( "heavysnare" );
const efftype_id effect_hit_by_player( "hit_by_player" );
const efftype_id effect_in_pit( "in_pit" );
const efftype_id effect_lightsnare( "lightsnare" );
const efftype_id effect_monster_armor( "monster_armor" );
const efftype_id effect_no_sight( "no_sight" );
const efftype_id effect_onfire( "onfire" );
const efftype_id effect_pacified( "pacified" );
const efftype_id effect_paralyzepoison( "paralyzepoison" );
const efftype_id effect_poison( "poison" );
const efftype_id effect_riding( "riding" );
const efftype_id effect_ridden( "ridden" );
const efftype_id effect_saddled( "saddled" );
const efftype_id effect_run( "run" );
const efftype_id effect_shrieking( "shrieking" );
const efftype_id effect_stunned( "stunned" );
const efftype_id effect_supercharged( "supercharged" );
const efftype_id effect_tied( "tied" );
const efftype_id effect_webbed( "webbed" );

static const trait_id trait_ANIMALDISCORD( "ANIMALDISCORD" );
static const trait_id trait_ANIMALDISCORD2( "ANIMALDISCORD2" );
static const trait_id trait_ANIMALEMPATH( "ANIMALEMPATH" );
static const trait_id trait_ANIMALEMPATH2( "ANIMALEMPATH2" );
static const trait_id trait_BEE( "BEE" );
static const trait_id trait_FLOWERS( "FLOWERS" );
static const trait_id trait_PACIFIST( "PACIFIST" );
static const trait_id trait_KILLER( "KILLER" );

static const std::map<m_size, std::string> size_names {
    {m_size::MS_TINY, translate_marker( "tiny" )},
    {m_size::MS_SMALL, translate_marker( "small" )},
    {m_size::MS_MEDIUM, translate_marker( "medium" )},
    {m_size::MS_LARGE, translate_marker( "large" )},
    {m_size::MS_HUGE, translate_marker( "huge" )},
};

static const std::map<monster_attitude, std::pair<std::string, color_id>> attitude_names {
    {monster_attitude::MATT_FRIEND, {translate_marker( "Friendly." ), def_h_white}},
    {monster_attitude::MATT_FPASSIVE, {translate_marker( "Passive." ), def_h_white}},
    {monster_attitude::MATT_FLEE, {translate_marker( "Fleeing!" ), def_c_green}},
    {monster_attitude::MATT_FOLLOW, {translate_marker( "Tracking." ), def_c_yellow}},
    {monster_attitude::MATT_IGNORE, {translate_marker( "Ignoring." ), def_c_light_gray}},
    {monster_attitude::MATT_ZLAVE, {translate_marker( "Zombie slave." ), def_c_green}},
    {monster_attitude::MATT_ATTACK, {translate_marker( "Hostile!" ), def_c_red}},
    {monster_attitude::MATT_NULL, {translate_marker( "BUG: Behavior unnamed." ), def_h_red}},
};

monster::monster()
{
    position.x = 20;
    position.y = 10;
    position.z = -500; // Some arbitrary number that will cause debugmsgs
    unset_dest();
    wandf = 0;
    hp = 60;
    moves = 0;
    friendly = 0;
    anger = 0;
    morale = 2;
    faction = mfaction_id( 0 );
    mission_id = -1;
    no_extra_death_drops = false;
    dead = false;
    death_drops = true;
    made_footstep = false;
    hallucination = false;
    ignoring = 0;
    upgrades = false;
    upgrade_time = -1;
    last_updated = 0;
    biosig_timer = -1;
}

monster::monster( const mtype_id &id ) : monster()
{
    type = &id.obj();
    moves = type->speed;
    Creature::set_speed_base( type->speed );
    hp = type->hp;
    for( auto &sa : type->special_attacks ) {
        auto &entry = special_attacks[sa.first];
        entry.cooldown = rng( 0, sa.second->cooldown );
    }
    anger = type->agro;
    morale = type->morale;
    faction = type->default_faction;
    ammo = type->starting_ammo;
    upgrades = type->upgrades && ( type->half_life || type->age_grow );
    reproduces = type->reproduces && type->baby_timer && !monster::has_flag( MF_NO_BREED );
    biosignatures = type->biosignatures;
    if( monster::has_flag( MF_AQUATIC ) ) {
        fish_population = dice( 1, 20 );
    }
    if( monster::has_flag( MF_RIDEABLE_MECH ) ) {
        itype_id mech_bat = itype_id( type->mech_battery );
        const itype &type = *item::find_type( mech_bat );
        int max_charge = type.magazine->capacity;
        item mech_bat_item = item( mech_bat, 0 );
        mech_bat_item.ammo_consume( rng( 0, max_charge ), tripoint_zero );
        battery_item = mech_bat_item;
    }
}

monster::monster( const mtype_id &id, const tripoint &p ) : monster( id )
{
    position = p;
    unset_dest();
}

monster::monster( const monster & ) = default;
monster::monster( monster && ) = default;
monster::~monster() = default;
monster &monster::operator=( const monster & ) = default;
monster &monster::operator=( monster && ) = default;

void monster::setpos( const tripoint &p )
{
    if( p == pos() ) {
        return;
    }

    bool wandering = wander();
    g->update_zombie_pos( *this, p );
    position = p;
    if( has_effect( effect_ridden ) && mounted_player && mounted_player->pos() != pos() ) {
        add_msg( m_debug, "Ridden monster %s moved independently and dumped player", get_name() );
        mounted_player->forced_dismount();
    }
    if( wandering ) {
        unset_dest();
    }
}

const tripoint &monster::pos() const
{
    return position;
}

void monster::poly( const mtype_id &id )
{
    double hp_percentage = static_cast<double>( hp ) / static_cast<double>( type->hp );
    type = &id.obj();
    moves = 0;
    Creature::set_speed_base( type->speed );
    anger = type->agro;
    morale = type->morale;
    hp = static_cast<int>( hp_percentage * type->hp );
    special_attacks.clear();
    for( auto &sa : type->special_attacks ) {
        auto &entry = special_attacks[sa.first];
        entry.cooldown = sa.second->cooldown;
    }
    faction = type->default_faction;
    upgrades = type->upgrades;
    reproduces = type->reproduces;
    biosignatures = type->biosignatures;
}

bool monster::can_upgrade()
{
    return upgrades && get_option<float>( "MONSTER_UPGRADE_FACTOR" ) > 0.0;
}

// For master special attack.
void monster::hasten_upgrade()
{
    if( !can_upgrade() || upgrade_time < 1 ) {
        return;
    }

    const int scaled_half_life = type->half_life * get_option<float>( "MONSTER_UPGRADE_FACTOR" );
    upgrade_time -= rng( 1, scaled_half_life );
    if( upgrade_time < 0 ) {
        upgrade_time = 0;
    }
}

// This will disable upgrades in case max iters have been reached.
// Checking for return value of -1 is necessary.
int monster::next_upgrade_time()
{
    if( type->age_grow > 0 ) {
        return type->age_grow;
    }
    const int scaled_half_life = type->half_life * get_option<float>( "MONSTER_UPGRADE_FACTOR" );
    int day = 1; // 1 day of guaranteed evolve time
    for( int i = 0; i < UPGRADE_MAX_ITERS; i++ ) {
        if( one_in( 2 ) ) {
            day += rng( 0, scaled_half_life );
            return day;
        } else {
            day += scaled_half_life;
        }
    }
    // didn't manage to upgrade, shouldn't ever then
    upgrades = false;
    return -1;
}

void monster::try_upgrade( bool pin_time )
{
    if( !can_upgrade() ) {
        return;
    }

    const int current_day = to_days<int>( calendar::turn - calendar::start_of_cataclysm );
    //This should only occur when a monster is created or upgraded to a new form
    if( upgrade_time < 0 ) {
        upgrade_time = next_upgrade_time();
        if( upgrade_time < 0 ) {
            return;
        }
        if( pin_time || type->age_grow > 0 ) {
            // offset by today, always true for growing creatures
            upgrade_time += current_day;
        } else {
            // offset by starting season
            // @todo revisit this and make it simpler
            upgrade_time += to_days<int>( calendar::start_of_cataclysm - calendar::turn_zero );
        }
    }

    // Here we iterate until we either are before upgrade_time or can't upgrade any more.
    // This is so that late into game new monsters can 'catch up' with all that half-life
    // upgrades they'd get if we were simulating whole world.
    while( true ) {
        if( upgrade_time > current_day ) {
            // not yet
            return;
        }

        if( type->upgrade_into ) {
            poly( type->upgrade_into );
        } else {
            const mtype_id &new_type = MonsterGroupManager::GetRandomMonsterFromGroup( type->upgrade_group );
            if( new_type ) {
                poly( new_type );
            }
        }

        if( !upgrades ) {
            // upgraded into a non-upgradeable monster
            return;
        }

        const int next_upgrade = next_upgrade_time();
        if( next_upgrade < 0 ) {
            // hit never_upgrade
            return;
        }
        upgrade_time += next_upgrade;
    }
}

void monster::try_reproduce()
{
    if( !reproduces ) {
        return;
    }
    // This can happen if the monster type has changed (from reproducing to non-reproducing monster)
    if( !type->baby_timer ) {
        return;
    }

    if( !baby_timer ) {
        // Assume this is a freshly spawned monster (because baby_timer is not set yet), set the point when it reproduce to somewhere in the future.
        baby_timer.emplace( calendar::turn + *type->baby_timer );
    }

    bool season_spawn = false;
    bool season_match = true;

    // only 50% of animals should reproduce
    bool female = one_in( 2 );
    for( auto &elem : type->baby_flags ) {
        if( elem == "SUMMER" || elem == "WINTER" || elem == "SPRING" || elem == "AUTUMN" ) {
            season_spawn = true;
        }
    }

    // add a decreasing chance of additional spawns when "catching up" an existing animal
    int chance = -1;
    while( true ) {
        if( *baby_timer > calendar::turn ) {
            return;
        }

        if( season_spawn ) {
            season_match = false;
            for( auto &elem : type->baby_flags ) {
                if( ( season_of_year( *baby_timer ) == SUMMER && elem == "SUMMER" ) ||
                    ( season_of_year( *baby_timer ) == WINTER && elem == "WINTER" ) ||
                    ( season_of_year( *baby_timer ) == SPRING && elem == "SPRING" ) ||
                    ( season_of_year( *baby_timer ) == AUTUMN && elem == "AUTUMN" ) ) {
                    season_match = true;
                }
            }
        }

        chance += 2;
        if( season_match && female && one_in( chance ) ) {
            int spawn_cnt = rng( 1, type->baby_count );
            if( type->baby_monster ) {
                g->m.add_spawn( type->baby_monster, spawn_cnt, pos().xy() );
            } else {
                g->m.add_item_or_charges( pos(), item( type->baby_egg, *baby_timer, spawn_cnt ), true );
            }
        }

        *baby_timer += *type->baby_timer;
    }
}

void monster::try_biosignature()
{
    if( !biosignatures ) {
        return;
    }
    if( !type->biosig_timer ) {
        return;
    }

    if( !biosig_timer ) {
        biosig_timer.emplace( calendar::turn + *type->biosig_timer );
    }

    while( true ) {
        if( *biosig_timer > calendar::turn ) {
            return;
        }

        g->m.add_item_or_charges( pos(), item( type->biosig_item, *biosig_timer, 1 ), true );
        *biosig_timer += *type->biosig_timer;
    }
}

void monster::spawn( const tripoint &p )
{
    position = p;
    unset_dest();
}

std::string monster::get_name() const
{
    return name( 1 );
}

std::string monster::name( unsigned int quantity ) const
{
    if( !type ) {
        debugmsg( "monster::name empty type!" );
        return std::string();
    }
    if( !unique_name.empty() ) {
        return string_format( "%s: %s", type->nname( quantity ), unique_name );
    }
    return type->nname( quantity );
}

// TODO: MATERIALS put description in materials.json?
std::string monster::name_with_armor() const
{
    std::string ret;
    if( type->in_species( INSECT ) ) {
        ret = _( "carapace" );
    } else if( made_of( material_id( "veggy" ) ) ) {
        ret = _( "thick bark" );
    } else if( made_of( material_id( "bone" ) ) ) {
        ret = _( "exoskeleton" );
    } else if( made_of( material_id( "flesh" ) ) || made_of( material_id( "hflesh" ) ) ||
               made_of( material_id( "iflesh" ) ) ) {
        ret = _( "thick hide" );
    } else if( made_of( material_id( "iron" ) ) || made_of( material_id( "steel" ) ) ) {
        ret = _( "armor plating" );
    } else if( made_of( LIQUID ) ) {
        ret = _( "dense jelly mass" );
    } else {
        ret = _( "armor" );
    }
    if( has_effect( effect_monster_armor ) && !inv.empty() ) {
        for( const item &armor : inv ) {
            if( armor.is_pet_armor( true ) ) {
                ret += string_format( _( "wearing %1$s" ), armor.tname( 1 ) );
                break;
            }
        }
    }

    return ret;
}

std::string monster::disp_name( bool possessive ) const
{
    if( !possessive ) {
        return string_format( _( "the %s" ), name() );
    } else {
        return string_format( _( "the %s's" ), name() );
    }
}

std::string monster::skin_name() const
{
    return name_with_armor();
}

void monster::get_HP_Bar( nc_color &color, std::string &text ) const
{
    std::tie( text, color ) = ::get_hp_bar( hp, type->hp, true );
}

std::pair<std::string, nc_color> monster::get_attitude() const
{
    const auto att = attitude_names.at( attitude( &g->u ) );
    return {
        _( att.first ),
        all_colors.get( att.second )
    };
}

static std::pair<std::string, nc_color> hp_description( int cur_hp, int max_hp )
{
    std::string damage_info;
    nc_color col;
    if( cur_hp >= max_hp ) {
        damage_info = _( "It is uninjured." );
        col = c_green;
    } else if( cur_hp >= max_hp * 0.8 ) {
        damage_info = _( "It is lightly injured." );
        col = c_light_green;
    } else if( cur_hp >= max_hp * 0.6 ) {
        damage_info = _( "It is moderately injured." );
        col = c_yellow;
    } else if( cur_hp >= max_hp * 0.3 ) {
        damage_info = _( "It is heavily injured." );
        col = c_yellow;
    } else if( cur_hp >= max_hp * 0.1 ) {
        damage_info = _( "It is severely injured." );
        col = c_light_red;
    } else {
        damage_info = _( "It is nearly dead!" );
        col = c_red;
    }
    /*
    // This is unused code that allows the player to see the exact amount of monster HP, to be implemented later!
    if( true ) ) {
        damage_info = string_format( _( "It has %d/%d HP." ), cur_hp, max_hp );
    }*/

    return std::make_pair( damage_info, col );
}

int monster::print_info( const catacurses::window &w, int vStart, int vLines, int column ) const
{
    const int vEnd = vStart + vLines;

    mvwprintz( w, point( column, vStart ), c_white, "%s ", name() );

    const auto att = get_attitude();
    wprintz( w, att.second, att.first );

    if( debug_mode ) {
        wprintz( w, c_light_gray, _( " Difficulty " ) + to_string( type->difficulty ) );
    }

    if( sees( g->u ) ) {
        mvwprintz( w, point( column, ++vStart ), c_yellow, _( "Aware of your presence!" ) );
    }

    std::string effects = get_effect_status();
    size_t used_space = att.first.length() + name().length() + 3;
    trim_and_print( w, point( used_space, vStart++ ), getmaxx( w ) - used_space - 2,
                    h_white, effects );

    const auto hp_desc = hp_description( hp, type->hp );
    mvwprintz( w, point( column, vStart++ ), hp_desc.second, hp_desc.first );
    if( has_effect( effect_ridden ) && mounted_player ) {
        mvwprintz( w, point( column, vStart++ ), c_white, _( "Rider: %s" ), mounted_player->disp_name() );
    }

    std::vector<std::string> lines = foldstring( type->get_description(), getmaxx( w ) - 1 - column );
    int numlines = lines.size();
    for( int i = 0; i < numlines && vStart <= vEnd; i++ ) {
        mvwprintz( w, point( column, vStart++ ), c_white, lines[i] );
    }

    return vStart;
}

std::string monster::extended_description() const
{
    std::ostringstream ss;
    const auto att = get_attitude();
    std::string att_colored = colorize( att.first, att.second );
    std::string difficulty_str;
    if( debug_mode ) {
        difficulty_str = _( "Difficulty " ) + to_string( type->difficulty );
    } else {
        if( type->difficulty < 3 ) {
            difficulty_str = _( "<color_light_gray>Minimal threat.</color>" );
        } else if( type->difficulty < 10 ) {
            difficulty_str = _( "<color_light_gray>Mildly dangerous.</color>" );
        } else if( type->difficulty < 20 ) {
            difficulty_str = _( "<color_light_red>Dangerous.</color>" );
        } else if( type->difficulty < 30 ) {
            difficulty_str = _( "<color_red>Very dangerous.</color>" );
        } else if( type->difficulty < 50 ) {
            difficulty_str = _( "<color_red>Extremely dangerous.</color>" );
        } else {
            difficulty_str = _( "<color_red>Fatally dangerous!</color>" );
        }
    }

    ss << string_format( _( "This is a %s.  %s %s" ), name(), att_colored,
                         difficulty_str ) << std::endl;
    if( !get_effect_status().empty() ) {
        ss << string_format( _( "<stat>It is %s.</stat>" ), get_effect_status() ) << std::endl;
    }

    ss << "--" << std::endl;
    auto hp_bar = hp_description( hp, type->hp );
    ss << colorize( hp_bar.first, hp_bar.second ) << std::endl;

    ss << "--" << std::endl;
    ss << string_format( "<dark>%s</dark>", type->get_description() ) << std::endl;
    ss << "--" << std::endl;

    ss << string_format( _( "It is %s in size." ),
                         _( size_names.at( get_size() ) ) ) << std::endl;

    std::vector<std::string> types;
    if( type->has_flag( MF_ANIMAL ) ) {
        types.emplace_back( _( "an animal" ) );
    }
    if( type->in_species( ZOMBIE ) ) {
        types.emplace_back( _( "a zombie" ) );
    }
    if( type->in_species( FUNGUS ) ) {
        types.emplace_back( _( "a fungus" ) );
    }
    if( type->in_species( INSECT ) ) {
        types.emplace_back( _( "an insect" ) );
    }
    if( type->in_species( ABERRATION ) ) {
        types.emplace_back( _( "an aberration" ) );
    }
    if( !types.empty() ) {
        ss << string_format( _( "It is %s." ),
                             enumerate_as_string( types ) ) << std::endl;
    }

    using flag_description = std::pair<m_flag, std::string>;
    const auto describe_flags = [this, &ss](
                                    const std::string & format,
                                    const std::vector<flag_description> &flags_names,
    const std::string &if_empty = "" ) {
        std::string flag_descriptions = enumerate_as_string( flags_names.begin(),
        flags_names.end(), [this]( const flag_description & fd ) {
            return type->has_flag( fd.first ) ? fd.second : "";
        } );
        if( !flag_descriptions.empty() ) {
            ss << string_format( format, flag_descriptions ) << std::endl;
        } else if( !if_empty.empty() ) {
            ss << if_empty << std::endl;
        }
    };

    describe_flags( _( "It has the following senses: %s." ), {
        {m_flag::MF_HEARS, pgettext( "Hearing as sense", "hearing" )},
        {m_flag::MF_SEES, pgettext( "Sight as sense", "sight" )},
        {m_flag::MF_SMELLS, pgettext( "Smell as sense", "smell" )},
    }, _( "It doesn't have senses." ) );

    describe_flags( _( "It can %s." ), {
        {m_flag::MF_SWIMS, pgettext( "Swim as an action", "swim" )},
        {m_flag::MF_FLIES, pgettext( "Fly as an action", "fly" )},
        {m_flag::MF_CAN_DIG, pgettext( "Dig as an action", "dig" )},
        {m_flag::MF_CLIMBS, pgettext( "Climb as an action", "climb" )}
    } );

    describe_flags( _( "<bad>In fight it can %s.</bad>" ), {
        {m_flag::MF_GRABS, pgettext( "Grab as an action", "grab" )},
        {m_flag::MF_VENOM, pgettext( "Poison as an action", "poison" )},
        {m_flag::MF_PARALYZE, pgettext( "Paralyze as an action", "paralyze" )},
        {m_flag::MF_BLEED, _( "cause bleed" )}
    } );

    if( !type->has_flag( m_flag::MF_NOHEAD ) ) {
        ss << _( "It has a head." ) << std::endl;
    }

    return replace_colors( ss.str() );
}

const std::string &monster::symbol() const
{
    return type->sym;
}

nc_color monster::basic_symbol_color() const
{
    return type->color;
}

nc_color monster::symbol_color() const
{
    return color_with_effects();
}

bool monster::is_symbol_highlighted() const
{
    return friendly != 0;
}

nc_color monster::color_with_effects() const
{
    nc_color ret = type->color;
    if( has_effect( effect_beartrap ) || has_effect( effect_stunned ) || has_effect( effect_downed ) ||
        has_effect( effect_tied ) ||
        has_effect( effect_lightsnare ) || has_effect( effect_heavysnare ) ) {
        ret = hilite( ret );
    }
    if( has_effect( effect_pacified ) ) {
        ret = invert_color( ret );
    }
    if( has_effect( effect_onfire ) ) {
        ret = red_background( ret );
    }
    return ret;
}

bool monster::avoid_trap( const tripoint & /* pos */, const trap &tr ) const
{
    // The trap position is not used, monsters are to stupid to remember traps. Actually, they do
    // not even see them.
    // Traps are on the ground, digging monsters go below, fliers and climbers go above.
    if( digging() || has_flag( MF_FLIES ) ) {
        return true;
    }
    return dice( 3, type->sk_dodge + 1 ) >= dice( 3, tr.get_avoidance() );
}

bool monster::has_flag( const m_flag f ) const
{
    return type->has_flag( f );
}

bool monster::can_see() const
{
    return has_flag( MF_SEES ) && !effect_cache[VISION_IMPAIRED];
}

bool monster::can_hear() const
{
    return has_flag( MF_HEARS ) && !has_effect( effect_deaf );
}

bool monster::can_submerge() const
{
    return ( has_flag( MF_NO_BREATHE ) || has_flag( MF_SWIMS ) || has_flag( MF_AQUATIC ) )
           && !has_flag( MF_ELECTRONIC );
}

bool monster::can_drown() const
{
    return !has_flag( MF_SWIMS ) && !has_flag( MF_AQUATIC )
           && !has_flag( MF_NO_BREATHE ) && !has_flag( MF_FLIES );
}

bool monster::digging() const
{
    return has_flag( MF_DIGS ) || ( has_flag( MF_CAN_DIG ) && underwater );
}

bool monster::can_act() const
{
    return moves > 0 &&
           ( effects->empty() ||
             ( !has_effect( effect_stunned ) && !has_effect( effect_downed ) && !has_effect( effect_webbed ) ) );
}

int monster::sight_range( const int light_level ) const
{
    // Non-aquatic monsters can't see much when submerged
    if( !can_see() || effect_cache[VISION_IMPAIRED] ||
        ( underwater && !has_flag( MF_SWIMS ) && !has_flag( MF_AQUATIC ) && !digging() ) ) {
        return 1;
    }
    static const int default_daylight = default_daylight_level();
    if( light_level == 0 ) {
        return type->vision_night;
    } else if( light_level == default_daylight ) {
        return type->vision_day;
    }
    int range = light_level * type->vision_day + ( default_daylight - light_level ) *
                type->vision_night;
    range /= default_daylight;

    return range;
}

bool monster::made_of( const material_id &m ) const
{
    return type->made_of( m );
}

bool monster::made_of_any( const std::set<material_id> &ms ) const
{
    return type->made_of_any( ms );
}

bool monster::made_of( phase_id p ) const
{
    return type->phase == p;
}

void monster::set_goal( const tripoint &p )
{
    goal = p;
}

void monster::shift( const point &sm_shift )
{
    const point ms_shift = sm_to_ms_copy( sm_shift );
    position -= ms_shift;
    goal -= ms_shift;
    if( wandf > 0 ) {
        wander_pos -= ms_shift;
    }
}

tripoint monster::move_target()
{
    return goal;
}

Creature *monster::attack_target()
{
    if( wander() ) {
        return nullptr;
    }

    Creature *target = g->critter_at( move_target() );
    if( target == nullptr || target == this ||
        attitude_to( *target ) == Creature::A_FRIENDLY || !sees( *target ) ) {
        return nullptr;
    }

    return target;
}

bool monster::is_fleeing( player &u ) const
{
    if( effect_cache[FLEEING] ) {
        return true;
    }
    if( anger >= 100 || morale >= 100 ) {
        return false;
    }
    monster_attitude att = attitude( &u );
    return att == MATT_FLEE || ( att == MATT_FOLLOW && rl_dist( pos(), u.pos() ) <= 4 );
}

Creature::Attitude monster::attitude_to( const Creature &other ) const
{
    const monster *m = other.is_monster() ? static_cast< const monster *>( &other ) : nullptr;
    const player *p = other.as_player();
    if( m != nullptr ) {
        if( m == this ) {
            return A_FRIENDLY;
        }

        auto faction_att = faction.obj().attitude( m->faction );
        if( ( friendly != 0 && m->friendly != 0 ) ||
            ( friendly == 0 && m->friendly == 0 && faction_att == MFA_FRIENDLY ) ) {
            // Friendly (to player) monsters are friendly to each other
            // Unfriendly monsters go by faction attitude
            return A_FRIENDLY;
        } else if( ( friendly == 0 && m->friendly == 0 && faction_att == MFA_NEUTRAL ) ||
                   morale < 0 || anger < 10 ) {
            // Stuff that won't attack is neutral to everything
            return A_NEUTRAL;
        } else {
            return A_HOSTILE;
        }
    } else if( p != nullptr ) {
        switch( attitude( const_cast<player *>( p ) ) ) {
            case MATT_FRIEND:
            case MATT_ZLAVE:
                return A_FRIENDLY;
            case MATT_FPASSIVE:
            case MATT_FLEE:
            case MATT_IGNORE:
            case MATT_FOLLOW:
                return A_NEUTRAL;
            case MATT_ATTACK:
                return A_HOSTILE;
            case MATT_NULL:
            case NUM_MONSTER_ATTITUDES:
                break;
        }
    }
    // Should not happen!, creature should be either player or monster
    return A_NEUTRAL;
}

monster_attitude monster::attitude( const Character *u ) const
{
    if( friendly != 0 ) {
        if( has_effect( effect_docile ) ) {
            return MATT_FPASSIVE;
        }
        if( u == &g->u ) {
            return MATT_FRIEND;
        }
        // Zombies don't understand not attacking NPCs, but dogs and bots should.
        const npc *np = dynamic_cast< const npc * >( u );
        if( np != nullptr && np->get_attitude() != NPCATT_KILL && !type->in_species( ZOMBIE ) ) {
            return MATT_FRIEND;
        }
        if( np != nullptr && np->is_hallucination() ) {
            return MATT_IGNORE;
        }
    }
    if( effect_cache[FLEEING] ) {
        return MATT_FLEE;
    }
    if( has_effect( effect_pacified ) ) {
        return MATT_ZLAVE;
    }

    int effective_anger  = anger;
    int effective_morale = morale;

    if( u != nullptr ) {
        // Those are checked quite often, so avoiding string construction is a good idea
        static const string_id<monfaction> faction_bee( "bee" );
        static const trait_id pheromone_mammal( "PHEROMONE_MAMMAL" );
        static const trait_id pheromone_insect( "PHEROMONE_INSECT" );
        static const trait_id mycus_thresh( "THRESH_MYCUS" );
        static const trait_id mycus_friend( "MYCUS_FRIEND" );
        static const trait_id terrifying( "TERRIFYING" );
        if( faction == faction_bee ) {
            if( u->has_trait( trait_BEE ) ) {
                return MATT_FRIEND;
            } else if( u->has_trait( trait_FLOWERS ) ) {
                effective_anger -= 10;
            }
        }

        if( type->in_species( FUNGUS ) && ( u->has_trait( mycus_thresh ) ||
                                            u->has_trait( mycus_friend ) ) ) {
            return MATT_FRIEND;
        }

        if( effective_anger >= 10 &&
            ( ( type->in_species( MAMMAL ) && u->has_trait( pheromone_mammal ) ) ||
              ( type->in_species( INSECT ) && u->has_trait( pheromone_insect ) ) ) ) {
            effective_anger -= 20;
        }

        if( u->has_trait( terrifying ) ) {
            effective_morale -= 10;
        }

        if( has_flag( MF_ANIMAL ) ) {
            if( u->has_trait( trait_ANIMALEMPATH ) ) {
                effective_anger -= 10;
                if( effective_anger < 10 ) {
                    effective_morale += 55;
                }
            } else if( u->has_trait( trait_ANIMALEMPATH2 ) ) {
                effective_anger -= 20;
                if( effective_anger < 20 ) {
                    effective_morale += 80;
                }
            } else if( u->has_trait( trait_ANIMALDISCORD ) ) {
                if( effective_anger >= 10 ) {
                    effective_anger += 10;
                }
                if( effective_anger < 10 ) {
                    effective_morale -= 5;
                }
            } else if( u->has_trait( trait_ANIMALDISCORD2 ) ) {
                if( effective_anger >= 20 ) {
                    effective_anger += 20;
                }
                if( effective_anger < 20 ) {
                    effective_morale -= 5;
                }
            }
        }

        for( const trait_id &mut : u->get_mutations() ) {
            for( const species_id &spe : mut.obj().ignored_by ) {
                if( type->in_species( spe ) ) {
                    return MATT_IGNORE;
                }
            }
        }
    }

    if( effective_morale < 0 ) {
        if( effective_morale + effective_anger > 0 && get_hp() > get_hp_max() / 3 ) {
            return MATT_FOLLOW;
        }
        return MATT_FLEE;
    }

    if( effective_anger <= 0 ) {
        if( get_hp() != get_hp_max() ) {
            return MATT_FLEE;
        } else {
            return MATT_IGNORE;
        }
    }

    if( effective_anger < 10 ) {
        return MATT_FOLLOW;
    }

    return MATT_ATTACK;
}

int monster::hp_percentage() const
{
    return get_hp( hp_torso ) * 100 / get_hp_max();
}

void monster::process_triggers()
{
    process_trigger( mon_trigger::STALK, [this]() {
        return anger > 0 && one_in( 5 ) ? 1 : 0;
    } );

    process_trigger( mon_trigger::FIRE, [this]() {
        int ret = 0;
        for( const auto &p : g->m.points_in_radius( pos(), 3 ) ) {
            ret += 5 * g->m.get_field_intensity( p, fd_fire );
        }
        return ret;
    } );

    // Meat checking is disabled as for now.
    // It's hard to ever see it in action
    // and even harder to balance it without making it exploitable

    if( morale != type->morale && one_in( 10 ) ) {
        if( morale < type->morale ) {
            morale++;
        } else {
            morale--;
        }
    }

    if( anger != type->agro && one_in( 10 ) ) {
        if( anger < type->agro ) {
            anger++;
        } else {
            anger--;
        }
    }

    // Cap values at [-100, 100] to prevent perma-angry moose etc.
    morale = std::min( 100, std::max( -100, morale ) );
    anger  = std::min( 100, std::max( -100, anger ) );
}

// This adjusts anger/morale levels given a single trigger.
void monster::process_trigger( mon_trigger trig, int amount )
{
    if( type->has_anger_trigger( trig ) ) {
        anger += amount;
    }
    if( type->has_fear_trigger( trig ) ) {
        morale -= amount;
    }
    if( type->has_placate_trigger( trig ) ) {
        anger -= amount;
    }
}

void monster::process_trigger( mon_trigger trig, const std::function<int()> &amount_func )
{
    if( type->has_anger_trigger( trig ) ) {
        anger += amount_func();
    }
    if( type->has_fear_trigger( trig ) ) {
        morale -= amount_func();
    }
    if( type->has_placate_trigger( trig ) ) {
        anger -= amount_func();
    }
}

bool monster::is_underwater() const
{
    return underwater && can_submerge();
}

bool monster::is_on_ground() const
{
    return false; // TODO: actually make this work
}

bool monster::has_weapon() const
{
    return false; // monsters will never have weapons, silly
}

bool monster::is_warm() const
{
    return has_flag( MF_WARM );
}

bool monster::in_species( const species_id &spec ) const
{
    return type->in_species( spec );
}

bool monster::is_elec_immune() const
{
    return is_immune_damage( DT_ELECTRIC );
}

bool monster::is_immune_effect( const efftype_id &effect ) const
{
    if( effect == effect_onfire ) {
        return is_immune_damage( DT_HEAT ) ||
               made_of( LIQUID ) ||
               has_flag( MF_FIREY );
    }

    if( effect == effect_bleed ) {
        return !has_flag( MF_WARM ) ||
               !made_of( material_id( "flesh" ) );
    }

    if( effect == effect_paralyzepoison ||
        effect == effect_badpoison ||
        effect == effect_poison ) {
        return !has_flag( MF_WARM ) ||
               ( !made_of( material_id( "flesh" ) ) && !made_of( material_id( "iflesh" ) ) );
    }

    if( effect == effect_stunned ) {
        return has_flag( MF_STUN_IMMUNE );
    }

    return false;
}

bool monster::is_immune_damage( const damage_type dt ) const
{
    switch( dt ) {
        case DT_NULL:
            return true;
        case DT_TRUE:
            return false;
        case DT_BIOLOGICAL: // NOTE: Unused
            return false;
        case DT_BASH:
            return false;
        case DT_CUT:
            return false;
        case DT_ACID:
            return has_flag( MF_ACIDPROOF );
        case DT_STAB:
            return false;
        case DT_HEAT:
            return made_of( material_id( "steel" ) ) ||
                   made_of( material_id( "stone" ) ); // Ugly hardcode - remove later
        case DT_COLD:
            return false;
        case DT_ELECTRIC:
            return type->sp_defense == &mdefense::zapback ||
                   has_flag( MF_ELECTRIC ) ||
                   has_flag( MF_ELECTRIC_FIELD );
        default:
            return true;
    }
}

bool monster::is_dead_state() const
{
    return hp <= 0;
}

bool monster::block_hit( Creature *, body_part &, damage_instance & )
{
    return false;
}

void monster::absorb_hit( body_part, damage_instance &dam )
{
    for( auto &elem : dam.damage_units ) {
        add_msg( m_debug, "Dam Type: %s :: Ar Pen: %.1f :: Armor Mult: %.1f",
                 name_by_dt( elem.type ), elem.res_pen, elem.res_mult );
        elem.amount -= std::min( resistances( *this ).get_effective_resist( elem ) +
                                 get_worn_armor_val( elem.type ), elem.amount );
    }
}

void monster::melee_attack( Creature &target )
{
    melee_attack( target, get_hit() );
}

void monster::melee_attack( Creature &target, float accuracy )
{
    int hitspread = target.deal_melee_attack( this, melee::melee_hit_range( accuracy ) );
    mod_moves( -type->attack_cost );
    if( type->melee_dice == 0 ) {
        // We don't attack, so just return
        return;
    }

    if( this == &target ) {
        // This happens sometimes
        return;
    }

    if( target.is_player() ||
        ( target.is_npc() && g->u.attitude_to( target ) == A_FRIENDLY ) ) {
        // Make us a valid target for a few turns
        add_effect( effect_hit_by_player, 3_turns );
    }

    if( has_flag( MF_HIT_AND_RUN ) ) {
        add_effect( effect_run, 4_turns );
    }

    const bool u_see_me = g->u.sees( *this );

    damage_instance damage = !is_hallucination() ? type->melee_damage : damage_instance();
    if( !is_hallucination() && type->melee_dice > 0 ) {
        damage.add_damage( DT_BASH, dice( type->melee_dice, type->melee_sides ) );
    }

    dealt_damage_instance dealt_dam;

    if( hitspread >= 0 ) {
        target.deal_melee_hit( this, hitspread, false, damage, dealt_dam );
    }
    body_part bp_hit = dealt_dam.bp_hit;

    const int total_dealt = dealt_dam.total_damage();
    if( hitspread < 0 ) {
        // Miss
        if( u_see_me && !target.in_sleep_state() ) {
            if( target.is_player() ) {
                add_msg( _( "You dodge %s." ), disp_name() );
            } else if( target.is_npc() ) {
                add_msg( _( "%1$s dodges %2$s attack." ),
                         target.disp_name(), name() );
            } else {
                add_msg( _( "The %1$s misses %2$s!" ),
                         name(), target.disp_name() );
            }
        } else if( target.is_player() ) {
            add_msg( _( "You dodge an attack from an unseen source." ) );
        }
    } else if( is_hallucination() || total_dealt > 0 ) {
        // Hallucinations always produce messages but never actually deal damage
        if( u_see_me ) {
            if( target.is_player() ) {
                sfx::play_variant_sound( "melee_attack", "monster_melee_hit",
                                         sfx::get_heard_volume( target.pos() ) );
                sfx::do_player_death_hurt( dynamic_cast<player &>( target ), false );
                //~ 1$s is attacker name, 2$s is bodypart name in accusative.
                add_msg( m_bad, _( "The %1$s hits your %2$s." ), name(),
                         body_part_name_accusative( bp_hit ) );
            } else if( target.is_npc() ) {
                if( has_effect( effect_ridden ) && has_flag( MF_RIDEABLE_MECH ) && pos() == g->u.pos() ) {
                    //~ %1$s: name of your mount, %2$s: target NPC name, %3$d: damage value
                    add_msg( m_good, _( "Your %1$s hits %2$s for %3$d damage!" ), name(), target.disp_name(),
                             total_dealt );
                } else {
                    //~ %1$s: attacker name, %2$s: target NPC name, %3$s: bodypart name in accusative
                    add_msg( _( "The %1$s hits %2$s %3$s." ), name(),
                             target.disp_name( true ),
                             body_part_name_accusative( bp_hit ) );
                }
            } else {
                if( has_effect( effect_ridden ) && has_flag( MF_RIDEABLE_MECH ) && pos() == g->u.pos() ) {
                    //~ %1$s: name of your mount, %2$s: target creature name, %3$d: damage value
                    add_msg( m_good, _( "Your %1$s hits %2$s for %3$d damage!" ), get_name(), target.disp_name(),
                             total_dealt );
                } else {
                    //~ %1$s: attacker name, %2$s: target creature name
                    add_msg( _( "The %1$s hits %2$s!" ), name(), target.disp_name() );
                }
            }
        } else if( target.is_player() ) {
            //~ %s is bodypart name in accusative.
            add_msg( m_bad, _( "Something hits your %s." ),
                     body_part_name_accusative( bp_hit ) );
        }
    } else {
        // No damage dealt
        if( u_see_me ) {
            if( target.is_player() ) {
                //~ 1$s is attacker name, 2$s is bodypart name in accusative, 3$s is armor name
                add_msg( _( "The %1$s hits your %2$s, but your %3$s protects you." ), name(),
                         body_part_name_accusative( bp_hit ), target.skin_name() );
            } else if( target.is_npc() ) {
                //~ $1s is monster name, %2$s is that monster target name,
                //~ $3s is target bodypart name in accusative, $4s is the monster target name,
                //~ 5$s is target armor name.
                add_msg( _( "The %1$s hits %2$s %3$s but is stopped by %4$s %5$s." ), name(),
                         target.disp_name( true ),
                         body_part_name_accusative( bp_hit ),
                         target.disp_name( true ),
                         target.skin_name() );
            } else {
                //~ $1s is monster name, %2$s is that monster target name,
                //~ $3s is target armor name.
                add_msg( _( "The %1$s hits %2$s but is stopped by its %3$s." ),
                         name(),
                         target.disp_name(),
                         target.skin_name() );
            }
        } else if( target.is_player() ) {
            //~ 1$s is bodypart name in accusative, 2$s is armor name.
            add_msg( _( "Something hits your %1$s, but your %2$s protects you." ),
                     body_part_name_accusative( bp_hit ), target.skin_name() );
        }
    }

    target.check_dead_state();

    if( is_hallucination() ) {
        if( one_in( 7 ) ) {
            die( nullptr );
        }
        return;
    }

    if( total_dealt <= 0 ) {
        return;
    }

    // Add any on damage effects
    for( const auto &eff : type->atk_effs ) {
        if( x_in_y( eff.chance, 100 ) ) {
            const body_part affected_bp = eff.affect_hit_bp ? bp_hit : eff.bp;
            target.add_effect( eff.id, time_duration::from_turns( eff.duration ), affected_bp, eff.permanent );
        }
    }

    const int stab_cut = dealt_dam.type_damage( DT_CUT ) + dealt_dam.type_damage( DT_STAB );

    if( stab_cut > 0 && has_flag( MF_VENOM ) ) {
        target.add_msg_if_player( m_bad, _( "You're envenomed!" ) );
        target.add_effect( effect_poison, 3_minutes );
    }

    if( stab_cut > 0 && has_flag( MF_BADVENOM ) ) {
        target.add_msg_if_player( m_bad,
                                  _( "You feel venom flood your body, wracking you with pain..." ) );
        target.add_effect( effect_badpoison, 4_minutes );
    }

    if( stab_cut > 0 && has_flag( MF_PARALYZE ) ) {
        target.add_msg_if_player( m_bad, _( "You feel venom enter your body!" ) );
        target.add_effect( effect_paralyzepoison, 10_minutes );
    }

    if( total_dealt > 6 && stab_cut > 0 && has_flag( MF_BLEED ) ) {
        // Maybe should only be if DT_CUT > 6... Balance question
        if( target.is_player() || target.is_npc() ) {
            target.as_character()->make_bleed( bp_hit, 6_minutes );
        } else {
            target.add_effect( effect_bleed, 6_minutes, bp_hit );
        }

    }
}

void monster::deal_projectile_attack( Creature *source, dealt_projectile_attack &attack,
                                      bool print_messages )
{
    const auto &proj = attack.proj;
    double &missed_by = attack.missed_by; // We can change this here
    const auto &effects = proj.proj_effects;

    // Whip has a chance to scare wildlife even if it misses
    if( effects.count( "WHIP" ) && type->in_category( "WILDLIFE" ) && one_in( 3 ) ) {
        add_effect( effect_run, rng( 3_turns, 5_turns ) );
    }

    if( missed_by > 1.0 ) {
        // Total miss
        return;
    }

    // if it's a headshot with no head, make it not a headshot
    if( missed_by < accuracy_headshot && has_flag( MF_NOHEAD ) ) {
        missed_by = accuracy_headshot;
    }

    Creature::deal_projectile_attack( source, attack, print_messages );

    if( !is_hallucination() && attack.hit_critter == this ) {
        // Maybe TODO: Get difficulty from projectile speed/size/missed_by
        on_hit( source, bp_torso, INT_MIN, &attack );
    }
}

void monster::deal_damage_handle_type( const damage_unit &du, body_part bp, int &damage,
                                       int &pain )
{
    switch( du.type ) {
        case DT_ELECTRIC:
            if( has_flag( MF_ELECTRIC ) ) {
                return; // immunity
            }
            break;
        case DT_COLD:
            if( !has_flag( MF_WARM ) ) {
                return; // immunity
            }
            break;
        case DT_BASH:
            if( has_flag( MF_PLASTIC ) ) {
                damage += du.amount / rng( 2, 4 ); // lessened effect
                pain += du.amount / 4;
                return;
            }
            break;
        case DT_NULL:
            debugmsg( "monster::deal_damage_handle_type: illegal damage type DT_NULL" );
            break;
        case DT_ACID:
            if( has_flag( MF_ACIDPROOF ) ) {
                return; // immunity
            }
        case DT_TRUE: // typeless damage, should always go through
        case DT_BIOLOGICAL: // internal damage, like from smoke or poison
        case DT_CUT:
        case DT_STAB:
        case DT_HEAT:
        default:
            break;
    }

    Creature::deal_damage_handle_type( du, bp, damage, pain );
}

int monster::heal( const int delta_hp, bool overheal )
{
    const int maxhp = type->hp;
    if( delta_hp <= 0 || ( hp >= maxhp && !overheal ) ) {
        return 0;
    }

    const int old_hp = hp;
    hp += delta_hp;
    if( hp > maxhp && !overheal ) {
        hp = maxhp;
    }
    return maxhp - old_hp;
}

void monster::set_hp( const int hp )
{
    this->hp = hp;
}

void monster::apply_damage( Creature *source, body_part /*bp*/, int dam,
                            const bool /*bypass_med*/ )
{
    if( is_dead_state() ) {
        return;
    }
    hp -= dam;
    if( hp < 1 ) {
        set_killer( source );
    } else if( dam > 0 ) {
        process_trigger( mon_trigger::HURT, 1 + static_cast<int>( dam / 3 ) );
    }
}

void monster::die_in_explosion( Creature *source )
{
    hp = -9999; // huge to trigger explosion and prevent corpse item
    die( source );
}

bool monster::movement_impaired()
{
    return effect_cache[MOVEMENT_IMPAIRED];
}

bool monster::move_effects( bool )
{
    // This function is relatively expensive, we want that cached
    // IMPORTANT: If adding any new effects here, make SURE to
    // add them to hardcoded_movement_impairing in effect.cpp
    if( !effect_cache[MOVEMENT_IMPAIRED] ) {
        return true;
    }

    bool u_see_me = g->u.sees( *this );
    if( has_effect( effect_tied ) ) {
        // friendly pet, will stay tied down and obey.
        if( friendly == -1 ) {
            return false;
        }
        // non-friendly monster will struggle to get free occasionally.
        // some monsters cant be tangled up with a net/bolas/lassoo etc.
        bool immediate_break = type->in_species( FISH ) || type->in_species( MOLLUSK ) ||
                               type->in_species( ROBOT ) || type->bodytype == "snake" || type->bodytype == "blob";
        if( !immediate_break && rng( 0, 900 ) > type->melee_dice * type->melee_sides * 1.5 ) {
            if( u_see_me ) {
                add_msg( _( "The %s struggles to break free of its bonds." ), name() );
            }
        } else if( immediate_break ) {
            remove_effect( effect_tied );
            if( tied_item ) {
                if( u_see_me ) {
                    add_msg( _( "The %s easily slips out of its bonds." ), name() );
                }
                g->m.add_item_or_charges( pos(), *tied_item );
                tied_item = cata::nullopt;
            }
        } else {
            if( tied_item ) {
                const bool broken = rng( type->melee_dice * type->melee_sides, std::min( 10000,
                                         type->melee_dice * type->melee_sides * 250 ) ) > 800;
                if( !broken ) {
                    g->m.add_item_or_charges( pos(), *tied_item );
                }
                tied_item = cata::nullopt;
                if( u_see_me ) {
                    if( broken ) {
                        add_msg( _( "The %s snaps the bindings holding it down." ), name() );
                    } else {
                        add_msg( _( "The %s breaks free of the bindings holding it down." ), name() );
                    }
                }
            }
            remove_effect( effect_tied );
        }
        return false;
    }
    if( has_effect( effect_downed ) ) {
        if( rng( 0, 40 ) > type->melee_dice * type->melee_sides * 1.5 ) {
            if( u_see_me ) {
                add_msg( _( "The %s struggles to stand." ), name() );
            }
        } else {
            if( u_see_me ) {
                add_msg( _( "The %s climbs to its feet!" ), name() );
            }
            remove_effect( effect_downed );
        }
        return false;
    }
    if( has_effect( effect_webbed ) ) {
        if( x_in_y( type->melee_dice * type->melee_sides, 6 * get_effect_int( effect_webbed ) ) ) {
            if( u_see_me ) {
                add_msg( _( "The %s breaks free of the webs!" ), name() );
            }
            remove_effect( effect_webbed );
        }
        return false;
    }
    if( has_effect( effect_lightsnare ) ) {
        if( x_in_y( type->melee_dice * type->melee_sides, 12 ) ) {
            remove_effect( effect_lightsnare );
            g->m.spawn_item( pos(), "string_36" );
            g->m.spawn_item( pos(), "snare_trigger" );
            if( u_see_me ) {
                add_msg( _( "The %s escapes the light snare!" ), name() );
            }
        }
        return false;
    }
    if( has_effect( effect_heavysnare ) ) {
        if( type->melee_dice * type->melee_sides >= 7 ) {
            if( x_in_y( type->melee_dice * type->melee_sides, 32 ) ) {
                remove_effect( effect_heavysnare );
                g->m.spawn_item( pos(), "rope_6" );
                g->m.spawn_item( pos(), "snare_trigger" );
                if( u_see_me ) {
                    add_msg( _( "The %s escapes the heavy snare!" ), name() );
                }
            }
        }
        return false;
    }
    if( has_effect( effect_beartrap ) ) {
        if( type->melee_dice * type->melee_sides >= 18 ) {
            if( x_in_y( type->melee_dice * type->melee_sides, 200 ) ) {
                remove_effect( effect_beartrap );
                g->m.spawn_item( pos(), "beartrap" );
                if( u_see_me ) {
                    add_msg( _( "The %s escapes the bear trap!" ), name() );
                }
            }
        }
        return false;
    }
    if( has_effect( effect_crushed ) ) {
        if( x_in_y( type->melee_dice * type->melee_sides, 100 ) ) {
            remove_effect( effect_crushed );
            if( u_see_me ) {
                add_msg( _( "The %s frees itself from the rubble!" ), name() );
            }
        }
        return false;
    }

    // If we ever get more effects that force movement on success this will need to be reworked to
    // only trigger success effects if /all/ rolls succeed
    if( has_effect( effect_in_pit ) ) {
        if( rng( 0, 40 ) > type->melee_dice * type->melee_sides ) {
            return false;
        } else {
            if( u_see_me ) {
                add_msg( _( "The %s escapes the pit!" ), name() );
            }
            remove_effect( effect_in_pit );
        }
    }
    if( has_effect( effect_grabbed ) ) {
        if( dice( type->melee_dice + type->melee_sides, 3 ) < get_effect_int( effect_grabbed ) ||
            !one_in( 4 ) ) {
            return false;
        } else {
            if( u_see_me ) {
                add_msg( _( "The %s breaks free from the grab!" ), name() );
            }
            remove_effect( effect_grabbed );
        }
    }
    return true;
}

void monster::add_effect( const efftype_id &eff_id, const time_duration dur, body_part/*bp*/,
                          bool permanent, int intensity, bool force, bool deferred )
{
    // Effects are not applied to specific monster body part
    Creature::add_effect( eff_id, dur, num_bp, permanent, intensity, force, deferred );
}

std::string monster::get_effect_status() const
{
    std::vector<std::string> effect_status;
    for( auto &elem : *effects ) {
        for( auto &_it : elem.second ) {
            if( elem.first->is_show_in_info() ) {
                effect_status.push_back( _it.second.disp_name() );
            }
        }
    }

    return enumerate_as_string( effect_status );
}

int monster::get_worn_armor_val( damage_type dt ) const
{
    if( !has_effect( effect_monster_armor ) || inv.empty() ) {
        return 0;
    }
    for( const item &armor : inv ) {
        if( !armor.is_pet_armor( true ) ) {
            continue;
        }
        return armor.damage_resist( dt );
    }
    return 0;
}

int monster::get_armor_cut( body_part bp ) const
{
    ( void ) bp;
    // TODO: Add support for worn armor?
    return static_cast<int>( type->armor_cut ) + armor_cut_bonus + get_worn_armor_val( DT_CUT );
}

int monster::get_armor_bash( body_part bp ) const
{
    ( void ) bp;
    return static_cast<int>( type->armor_bash ) + armor_bash_bonus + get_worn_armor_val( DT_BASH );
}

int monster::get_armor_type( damage_type dt, body_part bp ) const
{
    int worn_armor = get_worn_armor_val( dt );

    switch( dt ) {
        case DT_TRUE:
        case DT_BIOLOGICAL:
            return 0;
        case DT_BASH:
            return get_armor_bash( bp );
        case DT_CUT:
            return get_armor_cut( bp );
        case DT_ACID:
            return worn_armor + static_cast<int>( type->armor_acid );
        case DT_STAB:
            return worn_armor + static_cast<int>( type->armor_stab ) + armor_cut_bonus * 0.8f;
        case DT_HEAT:
            return worn_armor + static_cast<int>( type->armor_fire );
        case DT_COLD:
        case DT_ELECTRIC:
            return worn_armor;
        case DT_NULL:
        case NUM_DT:
            // Let it error below
            break;
    }

    debugmsg( "Invalid damage type: %d", dt );
    return 0;
}

float monster::get_hit_base() const
{
    return type->melee_skill;
}

float monster::get_dodge_base() const
{
    return type->sk_dodge;
}

float monster::hit_roll() const
{
    float hit = get_hit();
    if( has_effect( effect_bouldering ) ) {
        hit /= 4;
    }

    return melee::melee_hit_range( hit );
}

bool monster::has_grab_break_tec() const
{
    return false;
}

float monster::stability_roll() const
{
    int size_bonus = 0;
    switch( type->size ) {
        case MS_TINY:
            size_bonus -= 7;
            break;
        case MS_SMALL:
            size_bonus -= 3;
            break;
        case MS_LARGE:
            size_bonus += 5;
            break;
        case MS_HUGE:
            size_bonus += 10;
            break;
        case MS_MEDIUM:
            break; // keep default
    }

    int stability = dice( type->melee_sides, type->melee_dice ) + size_bonus;
    if( has_effect( effect_stunned ) ) {
        stability -= rng( 1, 5 );
    }
    return stability;
}

float monster::get_dodge() const
{
    if( has_effect( effect_downed ) ) {
        return 0.0f;
    }

    float ret = Creature::get_dodge();
    if( has_effect( effect_lightsnare ) || has_effect( effect_heavysnare ) ||
        has_effect( effect_beartrap ) || has_effect( effect_tied ) ) {
        ret /= 2;
    }

    if( has_effect( effect_bouldering ) ) {
        ret /= 4;
    }

    return ret;
}

float monster::get_melee() const
{
    return type->melee_skill;
}

float monster::dodge_roll()
{
    return get_dodge() * 5;
}

int monster::get_grab_strength() const
{
    return type->grab_strength;
}

float monster::fall_damage_mod() const
{
    if( has_flag( MF_FLIES ) ) {
        return 0.0f;
    }

    switch( type->size ) {
        case MS_TINY:
            return 0.2f;
        case MS_SMALL:
            return 0.6f;
        case MS_MEDIUM:
            return 1.0f;
        case MS_LARGE:
            return 1.4f;
        case MS_HUGE:
            return 2.0f;
    }

    return 0.0f;
}

int monster::impact( const int force, const tripoint &p )
{
    if( force <= 0 ) {
        return force;
    }

    const float mod = fall_damage_mod();
    int total_dealt = 0;
    if( g->m.has_flag( TFLAG_SHARP, p ) ) {
        const int cut_damage = std::max( 0.0f, 10 * mod - get_armor_cut( bp_torso ) );
        apply_damage( nullptr, bp_torso, cut_damage );
        total_dealt += 10 * mod;
    }

    const int bash_damage = std::max( 0.0f, force * mod - get_armor_bash( bp_torso ) );
    apply_damage( nullptr, bp_torso, bash_damage );
    total_dealt += force * mod;

    add_effect( effect_downed, time_duration::from_turns( rng( 0, mod * 3 + 1 ) ) );

    return total_dealt;
}

void monster::reset_bonuses()
{
    effect_cache.reset();

    Creature::reset_bonuses();
}

void monster::reset_stats()
{
    // Nothing here yet
}

void monster::reset_special( const std::string &special_name )
{
    set_special( special_name, type->special_attacks.at( special_name )->cooldown );
}

void monster::reset_special_rng( const std::string &special_name )
{
    set_special( special_name, rng( 0, type->special_attacks.at( special_name )->cooldown ) );
}

void monster::set_special( const std::string &special_name, int time )
{
    special_attacks[ special_name ].cooldown = time;
}

void monster::disable_special( const std::string &special_name )
{
    special_attacks.at( special_name ).enabled = false;
}

void monster::normalize_ammo( const int old_ammo )
{
    int total_ammo = 0;
    // Sum up the ammo entries to get a ratio.
    for( const auto &ammo_entry : type->starting_ammo ) {
        total_ammo += ammo_entry.second;
    }
    if( total_ammo == 0 ) {
        // Should never happen, but protect us from a div/0 if it does.
        return;
    }
    // Previous code gave robots 100 rounds of ammo.
    // This reassigns whatever is left from that in the appropriate proportions.
    for( const auto &ammo_entry : type->starting_ammo ) {
        ammo[ammo_entry.first] = old_ammo * ammo_entry.second / ( 100 * total_ammo );
    }
}

void monster::explode()
{
    // Handled in mondeath::normal
    // +1 to avoid overflow when evaluating -hp
    hp = INT_MIN + 1;
}

void monster::set_summon_time( const time_duration &length )
{
    summon_time_limit = length;
}

void monster::decrement_summon_timer()
{
    if( !summon_time_limit ) {
        return;
    }
    if( *summon_time_limit <= 0_turns ) {
        die( nullptr );
    } else {
        *summon_time_limit -= 1_turns;
    }
}

void monster::process_turn()
{
    decrement_summon_timer();
    if( !is_hallucination() ) {
        for( const auto &e : type->emit_fields ) {
            if( e == emit_id( "emit_shock_cloud" ) ) {
                if( has_effect( effect_emp ) ) {
                    continue; // don't emit electricity while EMPed
                } else if( has_effect( effect_supercharged ) ) {
                    g->m.emit_field( pos(), emit_id( "emit_shock_cloud_big" ) );
                    continue;
                }
            }
            g->m.emit_field( pos(), e );
        }
    }

    // Special attack cooldowns are updated here.
    // Loop through the monster's special attacks, same as monster::move.
    for( const auto &sp_type : type->special_attacks ) {
        const std::string &special_name = sp_type.first;
        const auto local_iter = special_attacks.find( special_name );
        if( local_iter == special_attacks.end() ) {
            continue;
        }
        mon_special_attack &local_attack_data = local_iter->second;
        if( !local_attack_data.enabled ) {
            continue;
        }

        if( local_attack_data.cooldown > 0 ) {
            local_attack_data.cooldown--;
        }
    }
    // Persist grabs as long as there's an adjacent target.
    if( has_effect( effect_grabbing ) ) {
        for( auto &dest : g->m.points_in_radius( pos(), 1, 0 ) ) {
            const player *const p = g->critter_at<player>( dest );
            if( p && p->has_effect( effect_grabbed ) ) {
                add_effect( effect_grabbing, 2_turns );
            }
        }
    }
    // We update electrical fields here since they act every turn.
    if( has_flag( MF_ELECTRIC_FIELD ) ) {
        if( has_effect( effect_emp ) ) {
            if( calendar::once_every( 10_turns ) ) {
                sounds::sound( pos(), 5, sounds::sound_t::combat, _( "hummmmm." ), false, "humming", "electric" );
            }
        } else {
            for( const tripoint &zap : g->m.points_in_radius( pos(), 1 ) ) {
                const bool player_sees = g->u.sees( zap );
                const auto items = g->m.i_at( zap );
                for( const auto &item : items ) {
                    if( item.made_of( LIQUID ) && item.flammable() ) { // start a fire!
                        g->m.add_field( zap, fd_fire, 2, 1_minutes );
                        sounds::sound( pos(), 30, sounds::sound_t::combat,  _( "fwoosh!" ), false, "fire", "ignition" );
                        break;
                    }
                }
                if( zap != pos() ) {
                    explosion_handler::emp_blast( zap ); // Fries electronics due to the intensity of the field
                }
                const auto t = g->m.ter( zap );
                if( t == ter_str_id( "t_gas_pump" ) || t == ter_str_id( "t_gas_pump_a" ) ) {
                    if( one_in( 4 ) ) {
                        explosion_handler::explosion( pos(), 40, 0.8, true );
                        if( player_sees ) {
                            add_msg( m_warning, _( "The %s explodes in a fiery inferno!" ), g->m.tername( zap ) );
                        }
                    } else {
                        if( player_sees ) {
                            add_msg( m_warning, _( "Lightning from %1$s engulfs the %2$s!" ), name(),
                                     g->m.tername( zap ) );
                        }
                        g->m.add_field( zap, fd_fire, 1, 2_turns );
                    }
                }
            }
            if( g->weather.lightning_active && !has_effect( effect_supercharged ) &&
                g->m.is_outside( pos() ) ) {
                g->weather.lightning_active = false; // only one supercharge per strike
                sounds::sound( pos(), 300, sounds::sound_t::combat, _( "BOOOOOOOM!!!" ), false, "environment",
                               "thunder_near" );
                sounds::sound( pos(), 20, sounds::sound_t::combat, _( "vrrrRRRUUMMMMMMMM!" ), false, "explosion",
                               "default" );
                if( g->u.sees( pos() ) ) {
                    add_msg( m_bad, _( "Lightning strikes the %s!" ), name() );
                    add_msg( m_bad, _( "Your vision goes white!" ) );
                    g->u.add_effect( effect_blind, rng( 1_minutes, 2_minutes ) );
                }
                add_effect( effect_supercharged, 12_hours );
            } else if( has_effect( effect_supercharged ) && calendar::once_every( 5_turns ) ) {
                sounds::sound( pos(), 20, sounds::sound_t::combat, _( "VMMMMMMMMM!" ), false, "humming",
                               "electric" );
            }
        }
    }

    Creature::process_turn();
}

void monster::die( Creature *nkiller )
{
    if( dead ) {
        // We are already dead, don't die again, note that monster::dead is
        // *only* set to true in this function!
        return;
    }
    // We were carrying a creature, deposit the rider
    if( has_effect( effect_ridden ) ) {
        if( has_effect( effect_saddled ) ) {
            item riding_saddle( "riding_saddle", 0 );
            g->m.add_item_or_charges( pos(), riding_saddle );
        }
        if( mounted_player ) {
            mounted_player->forced_dismount();
        }
    }
    g->set_critter_died();
    dead = true;
    set_killer( nkiller );
    if( !death_drops ) {
        return;
    }
    if( !no_extra_death_drops ) {
        drop_items_on_death();
    }
    // TODO: should actually be class Character
    player *ch = dynamic_cast<player *>( get_killer() );
    if( !is_hallucination() && ch != nullptr ) {
        if( ( has_flag( MF_GUILT ) && ch->is_player() ) || ( ch->has_trait( trait_PACIFIST ) &&
                has_flag( MF_HUMAN ) ) ) {
            // has guilt flag or player is pacifist && monster is humanoid
            mdeath::guilt( *this );
        }
        g->events().send<event_type::character_kills_monster>( ch->getID(), type->id );
        if( ch->is_player() && ch->has_trait( trait_KILLER ) ) {
            if( one_in( 4 ) ) {
                std::string snip = SNIPPET.random_from_category( "killer_on_kill" );
                ch->add_msg_if_player( m_good, _( snip ) );
            }
            ch->add_morale( MORALE_KILLER_HAS_KILLED, 5, 10, 6_hours, 4_hours );
            ch->rem_morale( MORALE_KILLER_NEED_TO_KILL );
        }
    }
    // We were tied up at the moment of death, add a short rope to inventory
    if( has_effect( effect_tied ) ) {
        if( tied_item ) {
            add_item( *tied_item );
            tied_item = cata::nullopt;
        }
    }
    if( has_effect( effect_lightsnare ) ) {
        add_item( item( "string_36", 0 ) );
        add_item( item( "snare_trigger", 0 ) );
    }
    if( has_effect( effect_heavysnare ) ) {
        add_item( item( "rope_6", 0 ) );
        add_item( item( "snare_trigger", 0 ) );
    }
    if( has_effect( effect_beartrap ) ) {
        add_item( item( "beartrap", 0 ) );
    }
    if( has_effect( effect_grabbing ) ) {
        remove_effect( effect_grabbing );
        for( auto &player_pos : g->m.points_in_radius( pos(), 1, 0 ) ) {
            player *p = g->critter_at<player>( player_pos );
            if( !p || !p->has_effect( effect_grabbed ) ) {
                continue;
            }
            bool grabbed = false;
            for( auto &mon_pos : g->m.points_in_radius( player_pos, 1, 0 ) ) {
                const monster *const mon = g->critter_at<monster>( mon_pos );
                if( mon && mon->has_effect( effect_grabbing ) ) {
                    grabbed = true;
                    break;
                }
            }
            if( !grabbed ) {
                p->add_msg_player_or_npc( m_good, _( "The last enemy holding you collapses!" ),
                                          _( "The last enemy holding <npcname> collapses!" ) );
                p->remove_effect( effect_grabbed );
            }
        }
    }
    if( !is_hallucination() ) {
        for( const auto &it : inv ) {
            g->m.add_item_or_charges( pos(), it );
        }
    }

    // If we're a queen, make nearby groups of our type start to die out
    if( !is_hallucination() && has_flag( MF_QUEEN ) ) {
        // The submap coordinates of this monster, monster groups coordinates are
        // submap coordinates.
        const tripoint abssub = ms_to_sm_copy( g->m.getabs( pos() ) );
        // Do it for overmap above/below too
        for( const tripoint &p : points_in_radius( abssub, HALF_MAPSIZE, 1 ) ) {
            for( auto &mgp : overmap_buffer.groups_at( p ) ) {
                if( MonsterGroupManager::IsMonsterInGroup( mgp->type, type->id ) ) {
                    mgp->dying = true;
                }
            }
        }
    }
    mission::on_creature_death( *this );
    // Also, perform our death function
    if( is_hallucination() || summon_time_limit ) {
        //Hallucinations always just disappear
        mdeath::disappear( *this );
        return;
    }

    //Not a hallucination, go process the death effects.
    for( const auto &deathfunction : type->dies ) {
        deathfunction( *this );
    }

    // If our species fears seeing one of our own die, process that
    int anger_adjust = 0;
    int morale_adjust = 0;
    if( type->has_anger_trigger( mon_trigger::FRIEND_DIED ) ) {
        anger_adjust += 15;
    }
    if( type->has_fear_trigger( mon_trigger::FRIEND_DIED ) ) {
        morale_adjust -= 15;
    }
    if( type->has_placate_trigger( mon_trigger::FRIEND_DIED ) ) {
        anger_adjust -= 15;
    }

    if( anger_adjust != 0 || morale_adjust != 0 ) {
        int light = g->light_level( posz() );
        for( monster &critter : g->all_monsters() ) {
            if( !critter.type->same_species( *type ) ) {
                continue;
            }

            if( g->m.sees( critter.pos(), pos(), light ) ) {
                critter.morale += morale_adjust;
                critter.anger += anger_adjust;
            }
        }
    }
}

bool monster::use_mech_power( int amt )
{
    if( is_hallucination() || !has_flag( MF_RIDEABLE_MECH ) || !battery_item ) {
        return false;
    }
    amt = -amt;
    battery_item->ammo_consume( amt, pos() );
    return battery_item->ammo_remaining() > 0;
}

int monster::mech_str_addition() const
{
    return type->mech_str_bonus;
}

bool monster::check_mech_powered() const
{
    if( is_hallucination() || !has_flag( MF_RIDEABLE_MECH ) || !battery_item ) {
        return false;
    }
    if( battery_item->ammo_remaining() <= 0 ) {
        return false;
    }
    const itype &type = *battery_item->type;
    if( battery_item->ammo_remaining() <= type.magazine->capacity / 10 && one_in( 10 ) ) {
        add_msg( m_bad, _( "Your %s emits a beeping noise as its batteries start to get low." ),
                 get_name() );
    }
    return true;
}

void monster::drop_items_on_death()
{
    if( is_hallucination() ) {
        return;
    }
    if( type->death_drops.empty() ) {
        return;
    }
    const auto dropped = g->m.put_items_from_loc( type->death_drops, pos(),
                         calendar::start_of_cataclysm );

    if( has_flag( MF_FILTHY ) && get_option<bool>( "FILTHY_CLOTHES" ) ) {
        for( const auto &it : dropped ) {
            if( it->is_armor() || it->is_pet_armor() ) {
                it->item_tags.insert( "FILTHY" );
            }
        }
    }
}

void monster::process_one_effect( effect &it, bool is_new )
{
    // Monsters don't get trait-based reduction, but they do get effect based reduction
    bool reduced = resists_effect( it );
    const auto get_effect = [&it, is_new]( const std::string & arg, bool reduced ) {
        if( is_new ) {
            return it.get_amount( arg, reduced );
        }
        return it.get_mod( arg, reduced );
    };

    mod_speed_bonus( get_effect( "SPEED", reduced ) );

    int val = get_effect( "HURT", reduced );
    if( val > 0 ) {
        if( is_new || it.activated( calendar::turn, "HURT", val, reduced, 1 ) ) {
            apply_damage( nullptr, bp_torso, val );
        }
    }

    const efftype_id &id = it.get_id();
    // TODO: MATERIALS use fire resistance
    if( it.impairs_movement() ) {
        effect_cache[MOVEMENT_IMPAIRED] = true;
    } else if( id == effect_onfire ) {
        int dam = 0;
        if( made_of( material_id( "veggy" ) ) ) {
            dam = rng( 10, 20 );
        } else if( made_of( material_id( "flesh" ) ) || made_of( material_id( "iflesh" ) ) ) {
            dam = rng( 5, 10 );
        }

        dam -= get_armor_type( DT_HEAT, bp_torso );
        if( dam > 0 ) {
            apply_damage( nullptr, bp_torso, dam );
        } else {
            it.set_duration( 0_turns );
        }
    } else if( id == effect_run ) {
        effect_cache[FLEEING] = true;
    } else if( id == effect_no_sight || id == effect_blind ) {
        effect_cache[VISION_IMPAIRED] = true;
    }
}

void monster::process_effects()
{
    // Monster only effects
    for( auto &elem : *effects ) {
        for( auto &_effect_it : elem.second ) {
            process_one_effect( _effect_it.second, false );
        }
    }

    // Like with player/NPCs - keep the speed above 0
    const int min_speed_bonus = -0.75 * get_speed_base();
    if( get_speed_bonus() < min_speed_bonus ) {
        set_speed_bonus( min_speed_bonus );
    }

    //If this monster has the ability to heal in combat, do it now.
    if( has_flag( MF_REGENERATES_50 ) && heal( 50 ) > 0 && one_in( 2 ) && g->u.sees( *this ) ) {
        add_msg( m_warning, _( "The %s is visibly regenerating!" ), name() );
    }

    if( has_flag( MF_REGENERATES_10 ) && heal( 10 ) > 0 && one_in( 2 ) && g->u.sees( *this ) ) {
        add_msg( m_warning, _( "The %s seems a little healthier." ), name() );
    }

    if( has_flag( MF_REGENERATES_1 ) && heal( 1 ) > 0 && one_in( 2 ) && g->u.sees( *this ) ) {
        add_msg( m_warning, _( "The %s is healing slowly." ), name() );
    }

    if( has_flag( MF_REGENERATES_IN_DARK ) ) {
        const float light = g->m.ambient_light_at( pos() );
        // Magic number 10000 was chosen so that a floodlight prevents regeneration in a range of 20 tiles
        if( heal( static_cast<int>( 50.0 *  exp( - light * light / 10000 ) )  > 0 && one_in( 2 ) &&
                  g->u.sees( *this ) ) ) {
            add_msg( m_warning, _( "The %s uses the darkness to regenerate." ), name() );
        }
    }

    //Monster will regen morale and aggression if it is on max HP
    //It regens more morale and aggression if is currently fleeing.
    if( has_flag( MF_REGENMORALE ) && hp >= type->hp ) {
        if( is_fleeing( g->u ) ) {
            morale = type->morale;
            anger = type->agro;
        }
        if( morale <= type->morale ) {
            morale += 1;
        }
        if( anger <= type->agro ) {
            anger += 1;
        }
        if( morale < 0 ) {
            morale += 5;
        }
        if( anger < 0 ) {
            anger += 5;
        }
    }

    // If this critter dies in sunlight, check & assess damage.
    if( has_flag( MF_SUNDEATH ) && g->is_in_sunlight( pos() ) ) {
        if( g->u.sees( *this ) ) {
            add_msg( m_good, _( "The %s burns horribly in the sunlight!" ), name() );
        }
        apply_damage( nullptr, bp_torso, 100 );
        if( hp < 0 ) {
            hp = 0;
        }
    }

    Creature::process_effects();
}

bool monster::make_fungus()
{
    if( is_hallucination() ) {
        return true;
    }
    char polypick = 0;
    const mtype_id &tid = type->id;
    if( type->in_species( FUNGUS ) ) { // No friendly-fungalizing ;-)
        return true;
    }
    if( !made_of( material_id( "flesh" ) ) && !made_of( material_id( "hflesh" ) ) &&
        !made_of( material_id( "veggy" ) ) && !made_of( material_id( "iflesh" ) ) &&
        !made_of( material_id( "bone" ) ) ) {
        // No fungalizing robots or weird stuff (mi-gos are technically fungi, blobs are goo)
        return true;
    }
    if( tid == mon_ant || tid == mon_ant_soldier || tid == mon_ant_queen ) {
        polypick = 1;
    } else if( tid == mon_zombie || tid == mon_zombie_shrieker || tid == mon_zombie_electric ||
               tid == mon_zombie_spitter || tid == mon_zombie_brute ||
               tid == mon_zombie_hulk || tid == mon_zombie_soldier || tid == mon_zombie_tough ||
               tid == mon_zombie_scientist || tid == mon_zombie_hunter || tid == mon_skeleton_brute ||
               tid == mon_zombie_bio_op || tid == mon_zombie_survivor || tid == mon_zombie_fireman ||
               tid == mon_zombie_cop || tid == mon_zombie_fat || tid == mon_zombie_rot ||
               tid == mon_zombie_swimmer || tid == mon_zombie_grabber || tid == mon_zombie_technician ||
               tid == mon_zombie_brute_shocker ) {
        polypick = 2;
    } else if( tid == mon_zombie_necro || tid == mon_zombie_master || tid == mon_zombie_fireman ||
               tid == mon_zombie_hazmat || tid == mon_beekeeper ) {
        // Necro and Master have enough Goo to resist conversion.
        // Firefighter, hazmat, and scarred/beekeeper have the PPG on.
        return true;
    } else if( tid == mon_boomer || tid == mon_boomer_huge ) {
        polypick = 3;
    } else if( tid == mon_triffid || tid == mon_triffid_young || tid == mon_triffid_queen ) {
        polypick = 4;
    } else if( tid == mon_zombie_anklebiter || tid == mon_zombie_child || tid == mon_zombie_creepy ||
               tid == mon_zombie_shriekling || tid == mon_zombie_snotgobbler || tid == mon_zombie_sproglodyte ||
               tid == mon_zombie_waif ) {
        polypick = 5;
    } else if( tid == mon_skeleton_hulk ) {
        polypick = 6;
    } else if( tid == mon_zombie_smoker ) {
        polypick = 7;
    } else if( tid == mon_zombie_gasbag ) {
        polypick = 8;
    } else if( type->in_species( SPIDER ) && get_size() > MS_TINY ) {
        polypick = 9;
    }

    const std::string old_name = name();
    switch( polypick ) {
        case 1:
            poly( mon_ant_fungus );
            break;
        case 2: // zombies, non-boomer
            poly( mon_zombie_fungus );
            break;
        case 3:
            poly( mon_boomer_fungus );
            break;
        case 4:
            poly( mon_fungaloid );
            break;
        case 5:
            poly( mon_zombie_child_fungus );
            break;
        case 6:
            poly( mon_skeleton_hulk_fungus );
            break;
        case 7:
            poly( mon_zombie_smoker_fungus );
            break;
        case 8:
            poly( mon_zombie_gasbag_fungus );
            break;
        case 9:
            poly( mon_spider_fungus );
            break;
        default:
            return false;
    }

    if( g->u.sees( pos() ) ) {
        add_msg( m_info, _( "The spores transform %1$s into a %2$s!" ),
                 old_name, name() );
    }

    return true;
}

void monster::make_friendly()
{
    unset_dest();
    friendly = rng( 5, 30 ) + rng( 0, 20 );
}

void monster::make_ally( const monster &z )
{
    friendly = z.friendly;
    faction = z.faction;
}

void monster::add_item( const item &it )
{
    inv.push_back( it );
}

bool monster::is_hallucination() const
{
    return hallucination;
}

field_type_id monster::bloodType() const
{
    if( is_hallucination() ) {
        return fd_null;
    }
    return type->bloodType();
}
field_type_id monster::gibType() const
{
    if( is_hallucination() ) {
        return fd_null;
    }
    return type->gibType();
}

m_size monster::get_size() const
{
    return type->size;
}

units::mass monster::get_weight() const
{
    return type->weight;
}

units::volume monster::get_volume() const
{
    return type->volume;
}

void monster::add_msg_if_npc( const std::string &msg ) const
{
    if( g->u.sees( *this ) ) {
        add_msg( replace_with_npc_name( msg ) );
    }
}

void monster::add_msg_player_or_npc( const std::string &/*player_msg*/,
                                     const std::string &npc_msg ) const
{
    if( g->u.sees( *this ) ) {
        add_msg( replace_with_npc_name( npc_msg ) );
    }
}

void monster::add_msg_if_npc( const game_message_type type, const std::string &msg ) const
{
    if( g->u.sees( *this ) ) {
        add_msg( type, replace_with_npc_name( msg ) );
    }
}

void monster::add_msg_player_or_npc( const game_message_type type,
                                     const std::string &/*player_msg*/, const std::string &npc_msg ) const
{
    if( g->u.sees( *this ) ) {
        add_msg( type, replace_with_npc_name( npc_msg ) );
    }
}

bool monster::is_dead() const
{
    return dead || is_dead_state();
}

void monster::init_from_item( const item &itm )
{
    if( itm.typeId() == "corpse" ) {
        set_speed_base( get_speed_base() * 0.8 );
        const int burnt_penalty = itm.burnt;
        hp = static_cast<int>( hp * 0.7 );
        if( itm.damage_level( 4 ) > 0 ) {
            set_speed_base( speed_base / ( itm.damage_level( 4 ) + 1 ) );
            hp /= itm.damage_level( 4 ) + 1;
        }

        hp -= burnt_penalty;

        // HP can be 0 or less, in this case revive_corpse will just deactivate the corpse
        if( hp > 0 && type->has_flag( MF_REVIVES_HEALTHY ) ) {
            hp = type->hp;
            set_speed_base( type->speed );
        }
    } else {
        // must be a robot
        const int damfac = itm.max_damage() - std::max( 0, itm.damage() ) + 1;
        // One hp at least, everything else would be unfair (happens only to monster with *very* low hp),
        hp = std::max( 1, hp * damfac / ( itm.max_damage() + 1 ) );
    }
}

item monster::to_item() const
{
    if( type->revert_to_itype.empty() ) {
        return item();
    }
    // Birthday is wrong, but the item created here does not use it anyway (I hope).
    item result( type->revert_to_itype, calendar::turn );
    const int damfac = std::max( 1, ( result.max_damage() + 1 ) * hp / type->hp );
    result.set_damage( std::max( 0, ( result.max_damage() + 1 ) - damfac ) );
    return result;
}

float monster::power_rating() const
{
    float ret = get_size() - 1; // Zed gets 1, cat -1, hulk 3
    ret += has_flag( MF_ELECTRONIC ) ? 2 : 0; // Robots tend to have guns
    // Hostile stuff gets a big boost
    // Neutral moose will still get burned if it comes close
    return ret;
}

float monster::speed_rating() const
{
    float ret = get_speed() / 100.0f;
    const auto leap = type->special_attacks.find( "leap" );
    if( leap != type->special_attacks.end() ) {
        // TODO: Make this calculate sane values here
        ret += 0.5f;
    }

    return ret;
}

void monster::on_dodge( Creature *, float )
{
    // Currently does nothing, later should handle faction relations
}

void monster::on_hit( Creature *source, body_part,
                      float, dealt_projectile_attack const *const proj )
{
    if( is_hallucination() ) {
        return;
    }

    if( rng( 0, 100 ) <= static_cast<int>( type->def_chance ) ) {
        type->sp_defense( *this, source, proj );
    }

    // Adjust anger/morale of same-species monsters, if appropriate
    int anger_adjust = 0;
    int morale_adjust = 0;
    if( type->has_anger_trigger( mon_trigger::FRIEND_ATTACKED ) ) {
        anger_adjust += 15;
    }
    if( type->has_fear_trigger( mon_trigger::FRIEND_ATTACKED ) ) {
        morale_adjust -= 15;
    }
    if( type->has_placate_trigger( mon_trigger::FRIEND_ATTACKED ) ) {
        anger_adjust -= 15;
    }

    if( anger_adjust != 0 || morale_adjust != 0 ) {
        int light = g->light_level( posz() );
        for( monster &critter : g->all_monsters() ) {
            if( !critter.type->same_species( *type ) ) {
                continue;
            }

            if( g->m.sees( critter.pos(), pos(), light ) ) {
                critter.morale += morale_adjust;
                critter.anger += anger_adjust;
            }
        }
    }

    check_dead_state();
    // TODO: Faction relations
}

body_part monster::get_random_body_part( bool ) const
{
    return bp_torso;
}

std::vector<body_part> monster::get_all_body_parts( bool ) const
{
    return std::vector<body_part>( 1, bp_torso );
}

int monster::get_hp_max( hp_part ) const
{
    return type->hp;
}

int monster::get_hp_max() const
{
    return type->hp;
}

int monster::get_hp( hp_part ) const
{
    return hp;
}

int monster::get_hp() const
{
    return hp;
}

void monster::hear_sound( const tripoint &source, const int vol, const int dist )
{
    if( !can_hear() ) {
        return;
    }

    const bool goodhearing = has_flag( MF_GOODHEARING );
    const int volume = goodhearing ? 2 * vol - dist : vol - dist;
    // Error is based on volume, louder sound = less error
    if( volume <= 0 ) {
        return;
    }

    int max_error = 0;
    if( volume < 2 ) {
        max_error = 10;
    } else if( volume < 5 ) {
        max_error = 5;
    } else if( volume < 10 ) {
        max_error = 3;
    } else if( volume < 20 ) {
        max_error = 1;
    }

    int target_x = source.x + rng( -max_error, max_error );
    int target_y = source.y + rng( -max_error, max_error );
    // target_z will require some special check due to soil muffling sounds

    int wander_turns = volume * ( goodhearing ? 6 : 1 );
    process_trigger( mon_trigger::SOUND, volume );
    if( morale >= 0 && anger >= 10 ) {
        // TODO: Add a proper check for fleeing attitude
        // but cache it nicely, because this part is called a lot
        wander_to( tripoint( target_x, target_y, source.z ), wander_turns );
    } else if( morale < 0 ) {
        // Monsters afraid of sound should not go towards sound
        wander_to( tripoint( 2 * posx() - target_x, 2 * posy() - target_y, 2 * posz() - source.z ),
                   wander_turns );
    }
}

monster_horde_attraction monster::get_horde_attraction()
{
    if( horde_attraction == MHA_NULL ) {
        horde_attraction = static_cast<monster_horde_attraction>( rng( 1, 5 ) );
    }
    return horde_attraction;
}

void monster::set_horde_attraction( monster_horde_attraction mha )
{
    horde_attraction = mha;
}

bool monster::will_join_horde( int size )
{
    const monster_horde_attraction mha = get_horde_attraction();
    if( mha == MHA_NEVER ) {
        return false;
    } else if( mha == MHA_ALWAYS ) {
        return true;
    } else if( g->m.has_flag( TFLAG_INDOORS, pos() ) && ( mha == MHA_OUTDOORS ||
               mha == MHA_OUTDOORS_AND_LARGE ) ) {
        return false;
    } else if( size < 3 && ( mha == MHA_LARGE || mha == MHA_OUTDOORS_AND_LARGE ) ) {
        return false;
    } else {
        return true;
    }
}

void monster::on_unload()
{
    last_updated = calendar::turn;
}

void monster::on_load()
{
    try_upgrade( false );
    try_reproduce();
    try_biosignature();

    const time_duration dt = calendar::turn - last_updated;
    last_updated = calendar::turn;
    if( dt <= 0_turns ) {
        return;
    }
    float regen = 0.0f;
    if( has_flag( MF_REGENERATES_50 ) ) {
        regen = 50.0f;
    } else if( has_flag( MF_REGENERATES_10 ) ) {
        regen = 10.0f;
    } else if( has_flag( MF_REVIVES ) ) {
        regen = 1.0f / to_turns<int>( 1_hours );
    } else if( made_of( material_id( "flesh" ) ) || made_of( material_id( "veggy" ) ) ) {
        // Most living stuff here
        regen = 0.25f / to_turns<int>( 1_hours );
    }

    const int heal_amount = roll_remainder( regen * to_turns<int>( dt ) );
    const int healed = heal( heal_amount );
    int healed_speed = 0;
    if( healed < heal_amount && get_speed_base() < type->speed ) {
        int old_speed = get_speed_base();
        set_speed_base( std::min( get_speed_base() + heal_amount - healed, type->speed ) );
        healed_speed = get_speed_base() - old_speed;
    }

    add_msg( m_debug, "on_load() by %s, %d turns, healed %d hp, %d speed",
             name(), to_turns<int>( dt ), healed, healed_speed );
}

const pathfinding_settings &monster::get_pathfinding_settings() const
{
    return type->path_settings;
}

std::set<tripoint> monster::get_path_avoid() const
{
    return std::set<tripoint>();
}
