#include "monster.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <tuple>
#include <unordered_map>

#include "bodypart.h"
#include "catacharset.h"
#include "character.h"
#include "compatibility.h"
#include "coordinate_conversions.h"
#include "coordinates.h"
#include "cursesdef.h"
#include "debug.h"
#include "effect.h"
#include "event.h"
#include "event_bus.h"
#include "explosion.h"
#include "field_type.h"
#include "flat_set.h"
#include "game.h"
#include "game_constants.h"
#include "int_id.h"
#include "item.h"
#include "item_group.h"
#include "itype.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "mattack_common.h"
#include "melee.h"
#include "messages.h"
#include "mission.h"
#include "mondeath.h"
#include "mondefense.h"
#include "monfaction.h"
#include "mongroup.h"
#include "morale_types.h"
#include "mtype.h"
#include "mutation.h"
#include "npc.h"
#include "optional.h"
#include "options.h"
#include "output.h"
#include "overmapbuffer.h"
#include "pimpl.h"
#include "player.h"
#include "projectile.h"
#include "rng.h"
#include "sounds.h"
#include "string_formatter.h"
#include "string_id.h"
#include "text_snippets.h"
#include "translations.h"
#include "trap.h"
#include "units.h"
#include "viewer.h"
#include "weather.h"

static const efftype_id effect_badpoison( "badpoison" );
static const efftype_id effect_beartrap( "beartrap" );
static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_blind( "blind" );
static const efftype_id effect_bouldering( "bouldering" );
static const efftype_id effect_crushed( "crushed" );
static const efftype_id effect_deaf( "deaf" );
static const efftype_id effect_docile( "docile" );
static const efftype_id effect_downed( "downed" );
static const efftype_id effect_emp( "emp" );
static const efftype_id effect_grabbed( "grabbed" );
static const efftype_id effect_grabbing( "grabbing" );
static const efftype_id effect_heavysnare( "heavysnare" );
static const efftype_id effect_hit_by_player( "hit_by_player" );
static const efftype_id effect_in_pit( "in_pit" );
static const efftype_id effect_lightsnare( "lightsnare" );
static const efftype_id effect_monster_armor( "monster_armor" );
static const efftype_id effect_no_sight( "no_sight" );
static const efftype_id effect_onfire( "onfire" );
static const efftype_id effect_pacified( "pacified" );
static const efftype_id effect_paralyzepoison( "paralyzepoison" );
static const efftype_id effect_poison( "poison" );
static const efftype_id effect_ridden( "ridden" );
static const efftype_id effect_run( "run" );
static const efftype_id effect_stunned( "stunned" );
static const efftype_id effect_supercharged( "supercharged" );
static const efftype_id effect_tied( "tied" );
static const efftype_id effect_webbed( "webbed" );

static const itype_id itype_corpse( "corpse" );
static const itype_id itype_milk( "milk" );
static const itype_id itype_milk_raw( "milk_raw" );

static const species_id species_FISH( "FISH" );
static const species_id species_FUNGUS( "FUNGUS" );
static const species_id species_INSECT( "INSECT" );
static const species_id species_MAMMAL( "MAMMAL" );
static const species_id species_MOLLUSK( "MOLLUSK" );
static const species_id species_ROBOT( "ROBOT" );
static const species_id species_SPIDER( "SPIDER" );
static const species_id species_ZOMBIE( "ZOMBIE" );

static const trait_id trait_ANIMALDISCORD( "ANIMALDISCORD" );
static const trait_id trait_ANIMALDISCORD2( "ANIMALDISCORD2" );
static const trait_id trait_ANIMALEMPATH( "ANIMALEMPATH" );
static const trait_id trait_ANIMALEMPATH2( "ANIMALEMPATH2" );
static const trait_id trait_BEE( "BEE" );
static const trait_id trait_FLOWERS( "FLOWERS" );
static const trait_id trait_KILLER( "KILLER" );
static const trait_id trait_MYCUS_FRIEND( "MYCUS_FRIEND" );
static const trait_id trait_PACIFIST( "PACIFIST" );
static const trait_id trait_PHEROMONE_INSECT( "PHEROMONE_INSECT" );
static const trait_id trait_PHEROMONE_MAMMAL( "PHEROMONE_MAMMAL" );
static const trait_id trait_TERRIFYING( "TERRIFYING" );
static const trait_id trait_THRESH_MYCUS( "THRESH_MYCUS" );

static const mtype_id mon_ant( "mon_ant" );
static const mtype_id mon_ant_fungus( "mon_ant_fungus" );
static const mtype_id mon_ant_queen( "mon_ant_queen" );
static const mtype_id mon_ant_soldier( "mon_ant_soldier" );
static const mtype_id mon_beekeeper( "mon_beekeeper" );
static const mtype_id mon_boomer( "mon_boomer" );
static const mtype_id mon_boomer_fungus( "mon_boomer_fungus" );
static const mtype_id mon_boomer_huge( "mon_boomer_huge" );
static const mtype_id mon_fungaloid( "mon_fungaloid" );
static const mtype_id mon_skeleton_brute( "mon_skeleton_brute" );
static const mtype_id mon_skeleton_hulk( "mon_skeleton_hulk" );
static const mtype_id mon_skeleton_hulk_fungus( "mon_skeleton_hulk_fungus" );
static const mtype_id mon_spider_fungus( "mon_spider_fungus" );
static const mtype_id mon_triffid( "mon_triffid" );
static const mtype_id mon_triffid_queen( "mon_triffid_queen" );
static const mtype_id mon_triffid_young( "mon_triffid_young" );
static const mtype_id mon_zombie( "mon_zombie" );
static const mtype_id mon_zombie_anklebiter( "mon_zombie_anklebiter" );
static const mtype_id mon_zombie_bio_op( "mon_zombie_bio_op" );
static const mtype_id mon_zombie_brute( "mon_zombie_brute" );
static const mtype_id mon_zombie_brute_shocker( "mon_zombie_brute_shocker" );
static const mtype_id mon_zombie_child( "mon_zombie_child" );
static const mtype_id mon_zombie_child_fungus( "mon_zombie_child_fungus" );
static const mtype_id mon_zombie_cop( "mon_zombie_cop" );
static const mtype_id mon_zombie_creepy( "mon_zombie_creepy" );
static const mtype_id mon_zombie_electric( "mon_zombie_electric" );
static const mtype_id mon_zombie_fat( "mon_zombie_fat" );
static const mtype_id mon_zombie_fireman( "mon_zombie_fireman" );
static const mtype_id mon_zombie_fungus( "mon_zombie_fungus" );
static const mtype_id mon_zombie_gasbag( "mon_zombie_gasbag" );
static const mtype_id mon_zombie_gasbag_fungus( "mon_zombie_gasbag_fungus" );
static const mtype_id mon_zombie_grabber( "mon_zombie_grabber" );
static const mtype_id mon_zombie_hazmat( "mon_zombie_hazmat" );
static const mtype_id mon_zombie_hulk( "mon_zombie_hulk" );
static const mtype_id mon_zombie_hunter( "mon_zombie_hunter" );
static const mtype_id mon_zombie_master( "mon_zombie_master" );
static const mtype_id mon_zombie_necro( "mon_zombie_necro" );
static const mtype_id mon_zombie_rot( "mon_zombie_rot" );
static const mtype_id mon_zombie_scientist( "mon_zombie_scientist" );
static const mtype_id mon_zombie_shrieker( "mon_zombie_shrieker" );
static const mtype_id mon_zombie_shriekling( "mon_zombie_shriekling" );
static const mtype_id mon_zombie_smoker( "mon_zombie_smoker" );
static const mtype_id mon_zombie_smoker_fungus( "mon_zombie_smoker_fungus" );
static const mtype_id mon_zombie_snotgobbler( "mon_zombie_snotgobbler" );
static const mtype_id mon_zombie_soldier( "mon_zombie_soldier" );
static const mtype_id mon_zombie_spitter( "mon_zombie_spitter" );
static const mtype_id mon_zombie_sproglodyte( "mon_zombie_sproglodyte" );
static const mtype_id mon_zombie_survivor( "mon_zombie_survivor" );
static const mtype_id mon_zombie_swimmer( "mon_zombie_swimmer" );
static const mtype_id mon_zombie_technician( "mon_zombie_technician" );
static const mtype_id mon_zombie_tough( "mon_zombie_tough" );
static const mtype_id mon_zombie_waif( "mon_zombie_waif" );

struct pathfinding_settings;

// Limit the number of iterations for next upgrade_time calculations.
// This also sets the percentage of monsters that will never upgrade.
// The rough formula is 2^(-x), e.g. for x = 5 it's 0.03125 (~ 3%).
static constexpr int UPGRADE_MAX_ITERS = 5;

static const std::map<creature_size, translation> size_names {
    { creature_size::tiny, to_translation( "size adj", "tiny" ) },
    { creature_size::small, to_translation( "size adj", "small" ) },
    { creature_size::medium, to_translation( "size adj", "medium" ) },
    { creature_size::large, to_translation( "size adj", "large" ) },
    { creature_size::huge, to_translation( "size adj", "huge" ) },
};

static const std::map<monster_attitude, std::pair<std::string, color_id>> attitude_names {
    {monster_attitude::MATT_FRIEND, {translate_marker( "Friendly." ), def_h_white}},
    {monster_attitude::MATT_FPASSIVE, {translate_marker( "Passive." ), def_h_white}},
    {monster_attitude::MATT_FLEE, {translate_marker( "Fleeing!" ), def_c_green}},
    {monster_attitude::MATT_FOLLOW, {translate_marker( "Tracking." ), def_c_yellow}},
    {monster_attitude::MATT_IGNORE, {translate_marker( "Ignoring." ), def_c_light_gray}},
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
    udder_timer = calendar::turn;
    horde_attraction = MHA_NULL;
    set_anatomy( anatomy_id( "default_anatomy" ) );
    set_body();
}

monster::monster( const mtype_id &id ) : monster()
{
    type = &id.obj();
    moves = type->speed;
    Creature::set_speed_base( type->speed );
    hp = type->hp;
    for( const auto &sa : type->special_attacks ) {
        mon_special_attack &entry = special_attacks[sa.first];
        entry.cooldown = rng( 0, sa.second->cooldown );
    }
    anger = type->agro;
    morale = type->morale;
    faction = type->default_faction;
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
        battery_item = cata::make_value<item>( mech_bat_item );
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
    for( const auto &sa : type->special_attacks ) {
        mon_special_attack &entry = special_attacks[sa.first];
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

int monster::get_upgrade_time() const
{
    return upgrade_time;
}

// Sets time to upgrade to 0.
void monster::allow_upgrade()
{
    upgrade_time = 0;
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

    const int current_day = to_days<int>( calendar::turn - calendar::turn_zero );
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
            // TODO: revisit this and make it simpler
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
    for( const std::string &elem : type->baby_flags ) {
        if( elem == "SUMMER" || elem == "WINTER" || elem == "SPRING" || elem == "AUTUMN" ) {
            season_spawn = true;
        }
    }

    map &here = get_map();
    // add a decreasing chance of additional spawns when "catching up" an existing animal
    int chance = -1;
    while( true ) {
        if( *baby_timer > calendar::turn ) {
            return;
        }

        if( season_spawn ) {
            season_match = false;
            for( const std::string &elem : type->baby_flags ) {
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
                here.add_spawn( type->baby_monster, spawn_cnt, pos() );
            } else {
                here.add_item_or_charges( pos(), item( type->baby_egg, *baby_timer, spawn_cnt ), true );
            }
        }

        *baby_timer += *type->baby_timer;
    }
}

void monster::refill_udders()
{
    if( type->starting_ammo.empty() ) {
        debugmsg( "monster %s has no starting ammo to refill udders", get_name() );
        return;
    }
    if( ammo.empty() ) {
        // legacy animals got empty ammo map, fill them up now if needed.
        ammo[type->starting_ammo.begin()->first] = type->starting_ammo.begin()->second;
    }
    auto current_milk = ammo.find( itype_milk_raw );
    if( current_milk == ammo.end() ) {
        current_milk = ammo.find( itype_milk );
        if( current_milk != ammo.end() ) {
            // take this opportunity to update milk udders to raw_milk
            ammo[itype_milk_raw] = current_milk->second;
            // Erase old key-value from map
            ammo.erase( current_milk );
        }
    }
    // if we got here, we got milk.
    if( current_milk->second == type->starting_ammo.begin()->second ) {
        // already full up
        return;
    }
    if( calendar::turn - udder_timer > 1_days ) {
        // no point granularizing this really, you milk once a day.
        ammo.begin()->second = type->starting_ammo.begin()->second;
        udder_timer = calendar::turn;
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
    map &here = get_map();
    int counter = 0;
    while( true ) {
        // don't catch up too much, otherwise on some scenarios,
        // we could have years worth of poop just deposited on the floor.
        if( *biosig_timer > calendar::turn || counter > 50 ) {
            return;
        }
        here.add_item_or_charges( pos(), item( type->biosig_item, *biosig_timer, 1 ), true );
        *biosig_timer += *type->biosig_timer;
        counter += 1;
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
    if( type->in_species( species_INSECT ) ) {
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
    } else if( made_of( phase_id::LIQUID ) ) {
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

std::string monster::disp_name( bool possessive, bool capitalize_first ) const
{
    if( !possessive ) {
        return string_format( capitalize_first ? _( "The %s" ) : _( "the %s" ), name() );
    } else {
        return string_format( capitalize_first ? _( "The %s's" ) : _( "the %s's" ), name() );
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
    const auto att = attitude_names.at( attitude( &get_player_character() ) );
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

    return std::make_pair( damage_info, col );
}

int monster::print_info( const catacurses::window &w, int vStart, int vLines, int column ) const
{
    const int vEnd = vStart + vLines;
    const int max_width = getmaxx( w ) - column - 1;

    // Print health bar, monster name, then statuses on the first line.
    nc_color color = c_white;
    std::string bar_str;
    get_HP_Bar( color, bar_str );
    mvwprintz( w, point( column, vStart ), color, bar_str );
    const int bar_max_width = 5;
    const int bar_width = utf8_width( bar_str );
    for( int i = 0; i < bar_max_width - bar_width; ++i ) {
        mvwprintz( w, point( column + 4 - i, vStart ), c_white, "." );
    }
    mvwprintz( w, point( column + bar_max_width + 1, vStart ), basic_symbol_color(), name() );
    trim_and_print( w, point( column + bar_max_width + utf8_width( " " + name() + " " ), vStart ),
                    max_width - bar_max_width - utf8_width( " " + name() + " " ), h_white, get_effect_status() );

    // Hostility indicator on the second line.
    std::pair<std::string, nc_color> att = get_attitude();
    mvwprintz( w, point( column, ++vStart ), att.second, att.first );

    // Awareness indicator in the third line.
    bool sees_player = sees( get_player_character() );
    std::string senses_str = sees_player ? _( "Can see to your current location" ) :
                             _( "Can't see to your current location" );
    mvwprintz( w, point( column, ++vStart ), sees_player ? c_red : c_green, senses_str );

    // Monster description on following lines.
    std::vector<std::string> lines = foldstring( type->get_description(), max_width );
    int numlines = lines.size();
    for( int i = 0; i < numlines && vStart < vEnd; i++ ) {
        mvwprintz( w, point( column, ++vStart ), c_light_gray, lines[i] );
    }

    // Riding indicator on next line after description.
    if( has_effect( effect_ridden ) && mounted_player ) {
        mvwprintz( w, point( column, ++vStart ), c_white, _( "Rider: %s" ), mounted_player->disp_name() );
    }

    // Show monster size on the last line
    if( size_bonus > 0 ) {
        mvwprintz( w, point( column, ++vStart ), c_light_gray, _( " It is %s." ),
                   size_names.at( get_size() ) );
    }

    return ++vStart;
}

std::string monster::extended_description() const
{
    std::string ss;
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

    ss += string_format( _( "This is a %s.  %s %s" ), name(), att_colored,
                         difficulty_str ) + "\n";
    if( !get_effect_status().empty() ) {
        ss += string_format( _( "<stat>It is %s.</stat>" ), get_effect_status() ) + "\n";
    }

    ss += "--\n";
    auto hp_bar = hp_description( hp, type->hp );
    ss += colorize( hp_bar.first, hp_bar.second ) + "\n";

    ss += "--\n";
    ss += string_format( "<dark>%s</dark>", type->get_description() ) + "\n";
    ss += "--\n";

    ss += string_format( _( "It is %s in size." ),
                         size_names.at( get_size() ) ) + "\n";

    std::vector<std::string> types = type->species_descriptions();
    if( type->has_flag( MF_ANIMAL ) ) {
        types.emplace_back( _( "an animal" ) );
    }
    if( !types.empty() ) {
        ss += string_format( _( "It is %s." ),
                             enumerate_as_string( types ) ) + "\n";
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
            ss += string_format( format, flag_descriptions ) + "\n";
        } else if( !if_empty.empty() ) {
            ss += if_empty + "\n";
        }
    };

    using property_description = std::pair<bool, std::string>;
    const auto describe_properties = [&ss](
                                         const std::string & format,
                                         const std::vector<property_description> &property_names,
    const std::string &if_empty = "" ) {
        std::string property_descriptions = enumerate_as_string( property_names.begin(),
        property_names.end(), []( const property_description & pd ) {
            return pd.first ? pd.second : "";
        } );
        if( !property_descriptions.empty() ) {
            ss += string_format( format, property_descriptions ) + "\n";
        } else if( !if_empty.empty() ) {
            ss += if_empty + "\n";
        }
    };

    describe_flags( _( "It has the following senses: %s." ), {
        {m_flag::MF_HEARS, pgettext( "Hearing as sense", "hearing" )},
        {m_flag::MF_SEES, pgettext( "Sight as sense", "sight" )},
        {m_flag::MF_SMELLS, pgettext( "Smell as sense", "smell" )},
    }, _( "It doesn't have senses." ) );

    describe_properties( _( "It can %s." ), {
        {swims(), pgettext( "Swim as an action", "swim" )},
        {flies(), pgettext( "Fly as an action", "fly" )},
        {can_dig(), pgettext( "Dig as an action", "dig" )},
        {climbs(), pgettext( "Climb as an action", "climb" )}
    } );

    describe_flags( _( "<bad>In fight it can %s.</bad>" ), {
        {m_flag::MF_GRABS, pgettext( "Grab as an action", "grab" )},
        {m_flag::MF_VENOM, pgettext( "Poison as an action", "poison" )},
        {m_flag::MF_PARALYZE, pgettext( "Paralyze as an action", "paralyze" )}
    } );

    if( !type->has_flag( m_flag::MF_NOHEAD ) ) {
        ss += std::string( _( "It has a head." ) ) + "\n";
    }

    return replace_colors( ss );
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
    if( digging() || flies() ) {
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
    return ( has_flag( MF_NO_BREATHE ) || swims() || has_flag( MF_AQUATIC ) ) &&
           !has_flag( MF_ELECTRONIC );
}

bool monster::can_drown() const
{
    return !swims() && !has_flag( MF_AQUATIC ) &&
           !has_flag( MF_NO_BREATHE ) && !flies();
}

bool monster::can_climb() const
{
    return climbs() || flies();
}

bool monster::digging() const
{
    return digs() || ( can_dig() && underwater );
}

bool monster::can_dig() const
{
    return has_flag( MF_CAN_DIG );
}

bool monster::digs() const
{
    return has_flag( MF_DIGS );
}

bool monster::flies() const
{
    return has_flag( MF_FLIES );
}

bool monster::climbs() const
{
    return has_flag( MF_CLIMBS );
}

bool monster::swims() const
{
    return has_flag( MF_SWIMS );
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
        ( underwater && !swims() && !has_flag( MF_AQUATIC ) && !digging() ) ) {
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
        attitude_to( *target ) == Attitude::FRIENDLY || !sees( *target ) ) {
        return nullptr;
    }

    return target;
}

bool monster::is_fleeing( Character &u ) const
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
    const Character *p = other.as_character();
    if( m != nullptr ) {
        if( m == this ) {
            return Attitude::FRIENDLY;
        }

        auto faction_att = faction.obj().attitude( m->faction );
        if( ( friendly != 0 && m->friendly != 0 ) ||
            ( friendly == 0 && m->friendly == 0 && faction_att == MFA_FRIENDLY ) ) {
            // Friendly (to player) monsters are friendly to each other
            // Unfriendly monsters go by faction attitude
            return Attitude::FRIENDLY;
        } else if( ( friendly == 0 && m->friendly == 0 && faction_att == MFA_HATE ) ) {
            // Stuff that hates a specific faction will always attack that faction
            return Attitude::HOSTILE;
        } else if( ( friendly == 0 && m->friendly == 0 && faction_att == MFA_NEUTRAL ) ||
                   morale < 0 || anger < 10 ) {
            // Stuff that won't attack is neutral to everything
            return Attitude::NEUTRAL;
        } else {
            return Attitude::HOSTILE;
        }
    } else if( p != nullptr ) {
        switch( attitude( p ) ) {
            case MATT_FRIEND:
                return Attitude::FRIENDLY;
            case MATT_FPASSIVE:
            case MATT_FLEE:
            case MATT_IGNORE:
            case MATT_FOLLOW:
                return Attitude::NEUTRAL;
            case MATT_ATTACK:
                return Attitude::HOSTILE;
            case MATT_NULL:
            case NUM_MONSTER_ATTITUDES:
                break;
        }
    }
    // Should not happen!, creature should be either player or monster
    return Attitude::NEUTRAL;
}

monster_attitude monster::attitude( const Character *u ) const
{
    if( friendly != 0 ) {
        if( has_effect( effect_docile ) ) {
            return MATT_FPASSIVE;
        }
        if( u != nullptr ) {
            if( u->is_avatar() ) {
                return MATT_FRIEND;
            }
            if( u->is_npc() ) {
                // Zombies don't understand not attacking NPCs, but dogs and bots should.
                if( u->get_attitude() != NPCATT_KILL && !type->in_species( species_ZOMBIE ) ) {
                    return MATT_FRIEND;
                }
                if( u->is_hallucination() ) {
                    return MATT_IGNORE;
                }
            }
        }
    }
    if( effect_cache[FLEEING] ) {
        return MATT_FLEE;
    }

    int effective_anger  = anger;
    int effective_morale = morale;

    if( u != nullptr ) {
        // Those are checked quite often, so avoiding string construction is a good idea
        static const string_id<monfaction> faction_bee( "bee" );
        if( faction == faction_bee ) {
            if( u->has_trait( trait_BEE ) ) {
                return MATT_FRIEND;
            } else if( u->has_trait( trait_FLOWERS ) ) {
                effective_anger -= 10;
            }
        }

        if( type->in_species( species_FUNGUS ) && ( u->has_trait( trait_THRESH_MYCUS ) ||
                u->has_trait( trait_MYCUS_FRIEND ) ) ) {
            return MATT_FRIEND;
        }

        if( effective_anger >= 10 &&
            ( ( type->in_species( species_MAMMAL ) && u->has_trait( trait_PHEROMONE_MAMMAL ) ) ||
              ( type->in_species( species_INSECT ) && u->has_trait( trait_PHEROMONE_INSECT ) ) ) ) {
            effective_anger -= 20;
        }

        if( u->has_trait( trait_TERRIFYING ) ) {
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
            for( const std::pair<const species_id, int> &elem : mut.obj().anger_relations ) {
                if( type->in_species( elem.first ) ) {
                    effective_anger += elem.second;
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
    return get_hp( bodypart_id( "torso" ) ) * 100 / get_hp_max();
}

void monster::process_triggers()
{
    process_trigger( mon_trigger::STALK, [this]() {
        return anger > 0 && one_in( 5 ) ? 1 : 0;
    } );

    process_trigger( mon_trigger::FIRE, [this]() {
        int ret = 0;
        map &here = get_map();
        for( const auto &p : here.points_in_radius( pos(), 3 ) ) {
            ret += 5 * here.get_field_intensity( p, fd_fire );
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
    // TODO: actually make this work
    return false;
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
               made_of( phase_id::LIQUID ) ||
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
        case DT_NONE:
            return true;
        case DT_TRUE:
            return false;
        case DT_BIOLOGICAL:
            // NOTE: Unused
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
            // Ugly hardcode - remove later
            return made_of( material_id( "steel" ) ) || made_of( material_id( "stone" ) );
        case DT_COLD:
            return false;
        case DT_ELECTRIC:
            return type->sp_defense == &mdefense::zapback ||
                   has_flag( MF_ELECTRIC ) ||
                   has_flag( MF_ELECTRIC_FIELD );
        case DT_BULLET:
            return false;
        default:
            return true;
    }
}

bool monster::is_dead_state() const
{
    return hp <= 0;
}

bool monster::block_hit( Creature *, bodypart_id &, damage_instance & )
{
    return false;
}

void monster::absorb_hit( const bodypart_id &, damage_instance &dam )
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

    Character &player_character = get_player_character();
    if( target.is_player() ||
        ( target.is_npc() && player_character.attitude_to( target ) == Attitude::FRIENDLY ) ) {
        // Make us a valid target for a few turns
        add_effect( effect_hit_by_player, 3_turns );
    }

    if( has_flag( MF_HIT_AND_RUN ) ) {
        add_effect( effect_run, 4_turns );
    }

    const bool u_see_me = player_character.sees( *this );

    damage_instance damage = !is_hallucination() ? type->melee_damage : damage_instance();
    if( !is_hallucination() && type->melee_dice > 0 ) {
        damage.add_damage( DT_BASH, dice( type->melee_dice, type->melee_sides ) );
    }

    dealt_damage_instance dealt_dam;

    if( hitspread >= 0 ) {
        target.deal_melee_hit( this, hitspread, false, damage, dealt_dam );
    }

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
                         body_part_name_accusative( dealt_dam.bp_hit ) );
            } else if( target.is_npc() ) {
                if( has_effect( effect_ridden ) && has_flag( MF_RIDEABLE_MECH ) &&
                    pos() == player_character.pos() ) {
                    //~ %1$s: name of your mount, %2$s: target NPC name, %3$d: damage value
                    add_msg( m_good, _( "Your %1$s hits %2$s for %3$d damage!" ), name(), target.disp_name(),
                             total_dealt );
                } else {
                    //~ %1$s: attacker name, %2$s: target NPC name, %3$s: bodypart name in accusative
                    add_msg( _( "The %1$s hits %2$s %3$s." ), name(),
                             target.disp_name( true ),
                             body_part_name_accusative( dealt_dam.bp_hit ) );
                }
            } else {
                if( has_effect( effect_ridden ) && has_flag( MF_RIDEABLE_MECH ) &&
                    pos() == player_character.pos() ) {
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
                     body_part_name_accusative( dealt_dam.bp_hit ) );
        }
    } else {
        // No damage dealt
        if( u_see_me ) {
            if( target.is_player() ) {
                //~ 1$s is attacker name, 2$s is bodypart name in accusative, 3$s is armor name
                add_msg( _( "The %1$s hits your %2$s, but your %3$s protects you." ), name(),
                         body_part_name_accusative( dealt_dam.bp_hit ), target.skin_name() );
            } else if( target.is_npc() ) {
                //~ $1s is monster name, %2$s is that monster target name,
                //~ $3s is target bodypart name in accusative, $4s is the monster target name,
                //~ 5$s is target armor name.
                add_msg( _( "The %1$s hits %2$s %3$s but is stopped by %4$s %5$s." ), name(),
                         target.disp_name( true ),
                         body_part_name_accusative( dealt_dam.bp_hit ),
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
                     body_part_name_accusative( dealt_dam.bp_hit ), target.skin_name() );
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
    for( const mon_effect_data &eff : type->atk_effs ) {
        if( x_in_y( eff.chance, 100 ) ) {
            const bodypart_id affected_bp = eff.affect_hit_bp ? dealt_dam.bp_hit :  eff.bp.id();
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
                                  _( "You feel venom flood your body, wracking you with pain…" ) );
        target.add_effect( effect_badpoison, 4_minutes );
    }

    if( stab_cut > 0 && has_flag( MF_PARALYZE ) ) {
        target.add_msg_if_player( m_bad, _( "You feel venom enter your body!" ) );
        target.add_effect( effect_paralyzepoison, 10_minutes );
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
        on_hit( source, bodypart_id( "torso" ), INT_MIN, &attack );
    }
}

void monster::deal_damage_handle_type( const damage_unit &du, bodypart_id bp, int &damage,
                                       int &pain )
{
    switch( du.type ) {
        case DT_ELECTRIC:
            if( has_flag( MF_ELECTRIC ) ) {
                return; // immunity
            }
            break;
        case DT_COLD:
            if( has_flag( MF_COLDPROOF ) ) {
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
        case DT_NONE:
            debugmsg( "monster::deal_damage_handle_type: illegal damage type DT_NONE" );
            break;
        case DT_ACID:
            if( has_flag( MF_ACIDPROOF ) ) {
                // immunity
                return;
            }
        case DT_TRUE:
        // typeless damage, should always go through
        case DT_BIOLOGICAL:
        // internal damage, like from smoke or poison
        case DT_CUT:
        case DT_STAB:
        case DT_BULLET:
        case DT_HEAT:
        default:
            break;
    }

    Creature::deal_damage_handle_type( du,  bp, damage, pain );
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

void monster::apply_damage( Creature *source, bodypart_id /*bp*/, int dam,
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

void monster::heal_bp( bodypart_id, int dam )
{
    heal( dam );
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

    map &here = get_map();
    bool u_see_me = get_player_view().sees( *this );
    if( has_effect( effect_tied ) ) {
        // friendly pet, will stay tied down and obey.
        if( friendly == -1 ) {
            return false;
        }
        // non-friendly monster will struggle to get free occasionally.
        // some monsters can't be tangled up with a net/bolas/lasso etc.
        bool immediate_break = type->in_species( species_FISH ) || type->in_species( species_MOLLUSK ) ||
                               type->in_species( species_ROBOT ) || type->bodytype == "snake" || type->bodytype == "blob";
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
                here.add_item_or_charges( pos(), *tied_item );
                tied_item.reset();
            }
        } else {
            if( tied_item ) {
                const bool broken = rng( type->melee_dice * type->melee_sides, std::min( 10000,
                                         type->melee_dice * type->melee_sides * 250 ) ) > 800;
                if( !broken ) {
                    here.add_item_or_charges( pos(), *tied_item );
                }
                tied_item.reset();
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
            here.spawn_item( pos(), "string_36" );
            here.spawn_item( pos(), "snare_trigger" );
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
                here.spawn_item( pos(), "rope_6" );
                here.spawn_item( pos(), "snare_trigger" );
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
                here.spawn_item( pos(), "beartrap" );
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
    if( !has_effect( effect_monster_armor ) ) {
        return 0;
    }
    if( armor_item ) {
        return armor_item->damage_resist( dt );
    }
    return 0;
}

int monster::get_armor_cut( bodypart_id bp ) const
{
    ( void ) bp;
    // TODO: Add support for worn armor?
    return static_cast<int>( type->armor_cut ) + armor_cut_bonus + get_worn_armor_val( DT_CUT );
}

int monster::get_armor_bash( bodypart_id bp ) const
{
    ( void ) bp;
    return static_cast<int>( type->armor_bash ) + armor_bash_bonus + get_worn_armor_val( DT_BASH );
}

int monster::get_armor_bullet( bodypart_id bp ) const
{
    ( void ) bp;
    return static_cast<int>( type->armor_bullet ) + armor_bullet_bonus + get_worn_armor_val(
               DT_BULLET );
}

int monster::get_armor_type( damage_type dt, bodypart_id bp ) const
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
        case DT_BULLET:
            return get_armor_bullet( bp );
        case DT_ACID:
            return worn_armor + static_cast<int>( type->armor_acid );
        case DT_STAB:
            return worn_armor + static_cast<int>( type->armor_stab ) + armor_cut_bonus * 0.8f;
        case DT_HEAT:
            return worn_armor + static_cast<int>( type->armor_fire );
        case DT_COLD:
        case DT_ELECTRIC:
            return worn_armor;
        case DT_NONE:
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
        case creature_size::tiny:
            size_bonus -= 7;
            break;
        case creature_size::small:
            size_bonus -= 3;
            break;
        case creature_size::large:
            size_bonus += 5;
            break;
        case creature_size::huge:
            size_bonus += 10;
            break;
        case creature_size::medium:
            break; // keep default
        case creature_size::num_sizes:
            debugmsg( "ERROR: Invalid Creature size class." );
            break;
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
    if( flies() ) {
        return 0.0f;
    }

    switch( type->size ) {
        case creature_size::tiny:
            return 0.2f;
        case creature_size::small:
            return 0.6f;
        case creature_size::medium:
            return 1.0f;
        case creature_size::large:
            return 1.4f;
        case creature_size::huge:
            return 2.0f;
        case creature_size::num_sizes:
            debugmsg( "ERROR: Invalid Creature size class." );
            return 0.0f;
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
    if( get_map().has_flag( TFLAG_SHARP, p ) ) {
        const int cut_damage = std::max( 0.0f, 10 * mod - get_armor_cut( bodypart_id( "torso" ) ) );
        apply_damage( nullptr, bodypart_id( "torso" ), cut_damage );
        total_dealt += 10 * mod;
    }

    const int bash_damage = std::max( 0.0f, force * mod - get_armor_bash( bodypart_id( "torso" ) ) );
    apply_damage( nullptr, bodypart_id( "torso" ), bash_damage );
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

bool monster::special_available( const std::string &special_name ) const
{
    std::map<std::string, mon_special_attack>::const_iterator iter = special_attacks.find(
                special_name );
    return iter != special_attacks.end() && iter->second.enabled && iter->second.cooldown == 0;
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
    map &here = get_map();
    decrement_summon_timer();
    if( !is_hallucination() ) {
        for( const std::pair<const emit_id, time_duration> &e : type->emit_fields ) {
            if( !calendar::once_every( e.second ) ) {
                continue;
            }
            const emit_id emid = e.first;
            if( emid == emit_id( "emit_shock_cloud" ) ) {
                if( has_effect( effect_emp ) ) {
                    continue; // don't emit electricity while EMPed
                } else if( has_effect( effect_supercharged ) ) {
                    here.emit_field( pos(), emit_id( "emit_shock_cloud_big" ) );
                    continue;
                }
            }
            here.emit_field( pos(), emid );
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
        for( const tripoint &dest : here.points_in_radius( pos(), 1, 0 ) ) {
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
            for( const tripoint &zap : here.points_in_radius( pos(), 1 ) ) {
                const map_stack items = here.i_at( zap );
                for( const auto &item : items ) {
                    if( item.made_of( phase_id::LIQUID ) && item.flammable() ) { // start a fire!
                        here.add_field( zap, fd_fire, 2, 1_minutes );
                        sounds::sound( pos(), 30, sounds::sound_t::combat,  _( "fwoosh!" ), false, "fire", "ignition" );
                        break;
                    }
                }
                if( zap != pos() ) {
                    explosion_handler::emp_blast( zap ); // Fries electronics due to the intensity of the field
                }
                const auto t = here.ter( zap );
                if( t == ter_str_id( "t_gas_pump" ) || t == ter_str_id( "t_gas_pump_a" ) ) {
                    if( one_in( 4 ) ) {
                        explosion_handler::explosion( pos(), 40, 0.8, true );
                        add_msg_if_player_sees( zap, m_warning, _( "The %s explodes in a fiery inferno!" ),
                                                here.tername( zap ) );
                    } else {
                        add_msg_if_player_sees( zap, m_warning, _( "Lightning from %1$s engulfs the %2$s!" ),
                                                name(), here.tername( zap ) );
                        here.add_field( zap, fd_fire, 1, 2_turns );
                    }
                }
            }
            if( g->weather.lightning_active && !has_effect( effect_supercharged ) &&
                here.is_outside( pos() ) ) {
                g->weather.lightning_active = false; // only one supercharge per strike
                sounds::sound( pos(), 300, sounds::sound_t::combat, _( "BOOOOOOOM!!!" ), false, "environment",
                               "thunder_near" );
                sounds::sound( pos(), 20, sounds::sound_t::combat, _( "vrrrRRRUUMMMMMMMM!" ), false, "explosion",
                               "default" );
                Character &player_character = get_player_character();
                if( player_character.sees( pos() ) ) {
                    add_msg( m_bad, _( "Lightning strikes the %s!" ), name() );
                    add_msg( m_bad, _( "Your vision goes white!" ) );
                    player_character.add_effect( effect_blind, rng( 1_minutes, 2_minutes ) );
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
    if( has_effect( effect_ridden ) && mounted_player ) {
        mounted_player->forced_dismount();
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
        get_event_bus().send<event_type::character_kills_monster>( ch->getID(), type->id );
        if( ch->is_player() && ch->has_trait( trait_KILLER ) ) {
            if( one_in( 4 ) ) {
                const translation snip = SNIPPET.random_from_category( "killer_on_kill" ).value_or( translation() );
                ch->add_msg_if_player( m_good, "%s", snip );
            }
            ch->add_morale( MORALE_KILLER_HAS_KILLED, 5, 10, 6_hours, 4_hours );
            ch->rem_morale( MORALE_KILLER_NEED_TO_KILL );
        }
    }
    // Drop items stored in optionals
    move_special_item_to_inv( tack_item );
    move_special_item_to_inv( armor_item );
    move_special_item_to_inv( storage_item );
    move_special_item_to_inv( tied_item );

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
    map &here = get_map();
    if( has_effect( effect_grabbing ) ) {
        remove_effect( effect_grabbing );
        for( const tripoint &player_pos : here.points_in_radius( pos(), 1, 0 ) ) {
            player *p = g->critter_at<player>( player_pos );
            if( !p || !p->has_effect( effect_grabbed ) ) {
                continue;
            }
            bool grabbed = false;
            for( const tripoint &mon_pos : here.points_in_radius( player_pos, 1, 0 ) ) {
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
            here.add_item_or_charges( pos(), it );
        }
    }

    // If we're a queen, make nearby groups of our type start to die out
    if( !is_hallucination() && has_flag( MF_QUEEN ) ) {
        // The submap coordinates of this monster, monster groups coordinates are
        // submap coordinates.
        const tripoint abssub = ms_to_sm_copy( here.getabs( pos() ) );
        // Do it for overmap above/below too
        for( const tripoint &p : points_in_radius( abssub, HALF_MAPSIZE, 1 ) ) {
            // TODO: fix point types
            for( auto &mgp : overmap_buffer.groups_at( tripoint_abs_sm( p ) ) ) {
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

            if( here.sees( critter.pos(), pos(), light ) ) {
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

    std::vector<item> items = item_group::items_from( type->death_drops, calendar::start_of_cataclysm );

    // This block removes some items, according to item spawn scaling factor
    const float spawn_rate = get_option<float>( "ITEM_SPAWNRATE" );
    if( spawn_rate < 1 ) {
        // Temporary vector, to remember which items will be dropped
        std::vector<item> remaining;
        for( const item &it : items ) {
            // Mission items are not affected by item spawn rate
            if( rng_float( 0, 1 ) < spawn_rate || it.has_flag( "MISSION_ITEM" ) ) {
                remaining.push_back( it );
            }
        }
        // If there aren't any items left, there's nothing left to do
        if( remaining.empty() ) {
            return;
        }
        items = remaining;
    }

    const auto dropped = get_map().spawn_items( pos(), items );

    if( has_flag( MF_FILTHY ) ) {
        for( const auto &it : dropped ) {
            if( ( it->is_armor() || it->is_pet_armor() ) && !it->is_gun() ) {
                // handle wearable guns as a special case
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
    mod_dodge_bonus( get_effect( "DODGE", reduced ) );
    mod_hit_bonus( get_effect( "HIT", reduced ) );
    mod_bash_bonus( get_effect( "BASH", reduced ) );
    mod_cut_bonus( get_effect( "CUT", reduced ) );
    mod_size_bonus( get_effect( "SIZE", reduced ) );

    int val = get_effect( "HURT", reduced );
    if( val > 0 ) {
        if( is_new || it.activated( calendar::turn, "HURT", val, reduced, 1 ) ) {
            apply_damage( nullptr, bodypart_id( "torso" ), val );
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

        dam -= get_armor_type( DT_HEAT, bodypart_id( "torso" ) );
        if( dam > 0 ) {
            apply_damage( nullptr, bodypart_id( "torso" ), dam );
        } else {
            it.set_duration( 0_turns );
        }
    } else if( id == effect_run ) {
        effect_cache[FLEEING] = true;
    } else if( id == effect_no_sight || id == effect_blind ) {
        effect_cache[VISION_IMPAIRED] = true;
    } else if( id == effect_bleed && x_in_y( it.get_intensity(), it.get_max_intensity() ) ) {
        // monsters are simplified so they just take damage from bleeding
        apply_damage( nullptr, bodypart_id( "torso" ), 1 );
        // this is for balance only
        it.mod_duration( -rng( 1_turns, it.get_int_dur_factor() / 2 ) );
        bleed();
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

    Character &player_character = get_player_character();
    //If this monster has the ability to heal in combat, do it now.
    const int healed_amount = heal( type->regenerates );
    if( healed_amount > 0 && one_in( 2 ) ) {
        std::string healing_format_string;
        if( healed_amount >= 50 ) {
            healing_format_string = _( "The %s is visibly regenerating!" );
        } else if( healed_amount >= 10 ) {
            healing_format_string = _( "The %s seems a little healthier." );
        } else {
            healing_format_string = _( "The %s is healing slowly." );
        }
        add_msg_if_player_sees( *this, m_warning, healing_format_string, name() );
    }

    if( type->regenerates_in_dark ) {
        const float light = get_map().ambient_light_at( pos() );
        // Magic number 10000 was chosen so that a floodlight prevents regeneration in a range of 20 tiles
        if( heal( static_cast<int>( 50.0 *  std::exp( - light * light / 10000 ) )  > 0 && one_in( 2 ) ) ) {
            add_msg_if_player_sees( *this, m_warning, _( "The %s uses the darkness to regenerate." ), name() );
        }
    }

    //Monster will regen morale and aggression if it is on max HP
    //It regens more morale and aggression if is currently fleeing.
    if( type->regen_morale && hp >= type->hp ) {
        if( is_fleeing( player_character ) ) {
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
        add_msg_if_player_sees( *this, m_good, _( "The %s burns horribly in the sunlight!" ), name() );
        apply_damage( nullptr, bodypart_id( "torso" ), 100 );
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
    if( type->in_species( species_FUNGUS ) ) { // No friendly-fungalizing ;-)
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
    } else if( type->in_species( species_SPIDER ) && get_size() > creature_size::tiny ) {
        polypick = 9;
    }

    const std::string old_name = name();
    switch( polypick ) {
        case 1:
            poly( mon_ant_fungus );
            break;
        case 2:
            // zombies, non-boomer
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

    add_msg_if_player_sees( pos(), m_info, _( "The spores transform %1$s into a %2$s!" ),
                            old_name, name() );

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

creature_size monster::get_size() const
{
    return creature_size( type->size + size_bonus );
}

units::mass monster::get_weight() const
{
    return units::operator*( type->weight, get_size() / type->size );
}

units::mass monster::weight_capacity() const
{
    return type->weight * type->mountable_weight_ratio;
}

units::volume monster::get_volume() const
{
    return units::operator*( type->volume, get_size() / type->size );
}

void monster::add_msg_if_npc( const std::string &msg ) const
{
    add_msg_if_player_sees( *this, replace_with_npc_name( msg ) );
}

void monster::add_msg_player_or_npc( const std::string &/*player_msg*/,
                                     const std::string &npc_msg ) const
{
    add_msg_if_player_sees( *this, replace_with_npc_name( npc_msg ) );
}

void monster::add_msg_if_npc( const game_message_params &params, const std::string &msg ) const
{
    add_msg_if_player_sees( *this, params, replace_with_npc_name( msg ) );
}

void monster::add_msg_player_or_npc( const game_message_params &params,
                                     const std::string &/*player_msg*/, const std::string &npc_msg ) const
{
    add_msg_if_player_sees( *this, params, replace_with_npc_name( npc_msg ) );
}

units::mass monster::get_carried_weight()
{
    units::mass total_weight = 0_gram;
    if( tack_item ) {
        total_weight += tack_item->weight();
    }
    if( storage_item ) {
        total_weight += storage_item->weight();
    }
    if( armor_item ) {
        total_weight += armor_item->weight();
    }
    for( const item &it : inv ) {
        total_weight += it.weight();
    }
    return total_weight;
}

units::volume monster::get_carried_volume()
{
    units::volume total_volume = 0_ml;
    for( const item &it : inv ) {
        total_volume += it.volume();
    }
    return total_volume;
}

void monster::move_special_item_to_inv( cata::value_ptr<item> &it )
{
    if( it ) {
        add_item( *it );
        it.reset();
    }
}

bool monster::is_dead() const
{
    return dead || is_dead_state();
}

void monster::init_from_item( const item &itm )
{
    if( itm.typeId() == itype_corpse ) {
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
        const std::string up_time = itm.get_var( "upgrade_time" );
        if( !up_time.empty() ) {
            upgrade_time = std::stoi( up_time );
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
    if( type->revert_to_itype.is_empty() ) {
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
    float ret = get_size() - 2.0f; // Zed gets 1, cat -1, hulk 3
    ret += has_flag( MF_ELECTRONIC ) ? 2.0f : 0.0f; // Robots tend to have guns
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

void monster::on_hit( Creature *source, bodypart_id,
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
        map &here = get_map();
        for( monster &critter : g->all_monsters() ) {
            if( !critter.type->same_species( *type ) ) {
                continue;
            }

            if( here.sees( critter.pos(), pos(), light ) ) {
                critter.morale += morale_adjust;
                critter.anger += anger_adjust;
            }
        }
    }

    check_dead_state();
    // TODO: Faction relations
}

int monster::get_hp_max( const bodypart_id & ) const
{
    return type->hp;
}

int monster::get_hp_max() const
{
    return type->hp;
}

int monster::get_hp( const bodypart_id & ) const
{
    return hp;
}

int monster::get_hp() const
{
    return hp;
}

float monster::get_mountable_weight_ratio() const
{
    return type->mountable_weight_ratio;
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
    } else if( get_map().has_flag( TFLAG_INDOORS, pos() ) && ( mha == MHA_OUTDOORS ||
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
    if( has_flag( MF_MILKABLE ) ) {
        refill_udders();
    }

    const time_duration dt = calendar::turn - last_updated;
    last_updated = calendar::turn;
    if( dt <= 0_turns ) {
        return;
    }
    float regen = type->regenerates;
    if( regen <= 0 ) {
        if( has_flag( MF_REVIVES ) ) {
            regen = 1.0f / to_turns<int>( 1_hours );
        } else if( made_of( material_id( "flesh" ) ) || made_of( material_id( "veggy" ) ) ) {
            // Most living stuff here
            regen = 0.25f / to_turns<int>( 1_hours );
        }
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
