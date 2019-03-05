#include "npc.h"

#include "ammo.h"
#include "auto_pickup.h"
#include "coordinate_conversions.h"
#include "game.h"
#include "item_group.h"
#include "itype.h"
#include "iuse_actor.h"
#include "json.h"
#include "map.h"
#include "mapdata.h"
#include "messages.h"
#include "mission.h"
#include "monfaction.h"
#include "morale_types.h"
#include "mtype.h"
#include "mutation.h"
#include "npc_class.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "skill.h"
#include "sounds.h"
#include "string_formatter.h"
#include "trait_group.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_reference.h" // IWYU pragma: keep

const skill_id skill_mechanics( "mechanics" );
const skill_id skill_electronics( "electronics" );
const skill_id skill_speech( "speech" );
const skill_id skill_barter( "barter" );
const skill_id skill_gun( "gun" );
const skill_id skill_pistol( "pistol" );
const skill_id skill_throw( "throw" );
const skill_id skill_rifle( "rifle" );
const skill_id skill_dodge( "dodge" );
const skill_id skill_melee( "melee" );
const skill_id skill_unarmed( "unarmed" );
const skill_id skill_computer( "computer" );
const skill_id skill_firstaid( "firstaid" );
const skill_id skill_bashing( "bashing" );
const skill_id skill_stabbing( "stabbing" );
const skill_id skill_archery( "archery" );
const skill_id skill_cooking( "cooking" );
const skill_id skill_tailor( "tailor" );
const skill_id skill_shotgun( "shotgun" );
const skill_id skill_smg( "smg" );
const skill_id skill_launcher( "launcher" );
const skill_id skill_cutting( "cutting" );

const efftype_id effect_drunk( "drunk" );
const efftype_id effect_high( "high" );
const efftype_id effect_pkill1( "pkill1" );
const efftype_id effect_pkill2( "pkill2" );
const efftype_id effect_pkill3( "pkill3" );
const efftype_id effect_pkill_l( "pkill_l" );
const efftype_id effect_infection( "infection" );
const efftype_id effect_bouldering( "bouldering" );

static const trait_id trait_CANNIBAL( "CANNIBAL" );
static const trait_id trait_PSYCHOPATH( "PSYCHOPATH" );
static const trait_id trait_SAPIOVORE( "SAPIOVORE" );
static const trait_id trait_TERRIFYING( "TERRIFYING" );

void starting_clothes( npc &who, const npc_class_id &type, bool male );
void starting_inv( npc &who, const npc_class_id &type );

npc::npc()
    : player()
    , restock( calendar::before_time_starts )
    , companion_mission_time( calendar::before_time_starts )
    , companion_mission_time_ret( calendar::before_time_starts )
    , last_updated( calendar::turn )
{
    submap_coords = point_zero;
    position.x = -1;
    position.y = -1;
    position.z = 500;
    last_player_seen_pos = cata::nullopt;
    last_seen_player_turn = 999;
    wanted_item_pos = no_goal_point;
    guard_pos = no_goal_point;
    goal = no_goal_point;
    fetching_item = false;
    has_new_items = true;
    worst_item_value = 0;
    str_max = 0;
    dex_max = 0;
    int_max = 0;
    per_max = 0;
    my_fac = nullptr;
    miss_id = mission_type_id::NULL_ID();
    marked_for_death = false;
    death_drops = true;
    dead = false;
    hit_by_player = false;
    moves = 100;
    mission = NPC_MISSION_NULL;
    myclass = npc_class_id::NULL_ID();
    patience = 0;
    attitude = NPCATT_NULL;

    *path_settings = pathfinding_settings( 0, 1000, 1000, 10, true, true, true, false );
}

standard_npc::standard_npc( const std::string &name, const std::vector<itype_id> &clothing,
                            int sk_lvl, int s_str, int s_dex, int s_int, int s_per )
{
    this->name = name;
    position = tripoint_zero;

    str_cur = std::max( s_str, 0 );
    str_max = std::max( s_str, 0 );
    dex_cur = std::max( s_dex, 0 );
    dex_max = std::max( s_dex, 0 );
    per_cur = std::max( s_per, 0 );
    per_max = std::max( s_per, 0 );
    int_cur = std::max( s_int, 0 );
    int_max = std::max( s_int, 0 );

    recalc_hp();
    for( int i = 0; i < num_hp_parts; i++ ) {
        hp_cur[i] = hp_max[i];
    }
    for( auto &e : Skill::skills ) {
        set_skill_level( e.ident(), std::max( sk_lvl, 0 ) );
    }

    for( const auto &e : clothing ) {
        wear_item( item( e ) );
    }

    for( item &e : worn ) {
        if( e.has_flag( "VARSIZE" ) ) {
            e.item_tags.insert( "FIT" );
        }
    }
}

npc::npc( npc && ) = default;
npc &npc::operator=( npc && ) = default;

static std::map<string_id<npc_template>, npc_template> npc_templates;

void npc_template::load( JsonObject &jsobj )
{
    npc guy;
    guy.idz = jsobj.get_string( "id" );
    guy.name.clear();
    if( jsobj.has_string( "name_unique" ) ) {
        guy.name = static_cast<std::string>( _( jsobj.get_string( "name_unique" ).c_str() ) );
    }
    if( jsobj.has_string( "name_suffix" ) ) {
        guy.name += ", " + static_cast<std::string>( _( jsobj.get_string( "name_suffix" ).c_str() ) );
    }
    if( jsobj.has_string( "gender" ) ) {
        if( jsobj.get_string( "gender" ) == "male" ) {
            guy.male = true;
        } else {
            guy.male = false;
        }
    }
    if( jsobj.has_string( "faction" ) ) {
        guy.fac_id = faction_id( jsobj.get_string( "faction" ) );
    }

    if( jsobj.has_int( "class" ) ) {
        guy.myclass = npc_class::from_legacy_int( jsobj.get_int( "class" ) );
    } else if( jsobj.has_string( "class" ) ) {
        guy.myclass = npc_class_id( jsobj.get_string( "class" ) );
    }

    guy.set_attitude( npc_attitude( jsobj.get_int( "attitude" ) ) );
    guy.mission = npc_mission( jsobj.get_int( "mission" ) );
    guy.chatbin.first_topic = jsobj.get_string( "chat" );
    if( jsobj.has_string( "mission_offered" ) ) {
        guy.miss_id = mission_type_id( jsobj.get_string( "mission_offered" ) );
    } else {
        guy.miss_id = mission_type_id::NULL_ID();
    }
    npc_templates[string_id<npc_template>( guy.idz )].guy = std::move( guy );
}

void npc_template::reset()
{
    npc_templates.clear();
}

void npc_template::check_consistency()
{
    for( const auto &e : npc_templates ) {
        const auto &guy = e.second.guy;
        if( !guy.myclass.is_valid() ) {
            debugmsg( "Invalid NPC class %s", guy.myclass.c_str() );
        }
    }
}

template<>
bool string_id<npc_template>::is_valid() const
{
    return npc_templates.count( *this ) > 0;
}

template<>
const npc_template &string_id<npc_template>::obj() const
{
    const auto found = npc_templates.find( *this );
    if( found == npc_templates.end() ) {
        debugmsg( "Tried to get invalid npc: %s", c_str() );
        static const npc_template dummy;
        return dummy;
    }
    return found->second;
}

void npc::load_npc_template( const string_id<npc_template> &ident )
{
    auto found = npc_templates.find( ident );
    if( found == npc_templates.end() ) {
        debugmsg( "Tried to get invalid npc: %s", ident.c_str() );
        return;
    }
    const npc &tguy = found->second.guy;

    idz = tguy.idz;
    myclass = npc_class_id( tguy.myclass );
    randomize( myclass );
    std::string tmpname = tguy.name;
    if( tmpname[0] == ',' ) {
        name = name + tguy.name;
    } else {
        name = tguy.name;
        //Assume if the name is unique, the gender might also be.
        male = tguy.male;
    }
    fac_id = tguy.fac_id;
    set_fac( fac_id );
    attitude = tguy.attitude;
    mission = tguy.mission;
    chatbin.first_topic = tguy.chatbin.first_topic;
    if( !tguy.miss_id.is_null() ) {
        add_new_mission( mission::reserve_new( tguy.miss_id, getID() ) );
    }
}

npc::~npc() = default;

std::string npc::save_info() const
{
    return ::serialize( *this );
}

void npc::load_info( std::string data )
{
    std::stringstream dump;
    dump << data;

    JsonIn jsin( dump );
    try {
        deserialize( jsin );
    } catch( const JsonError &jsonerr ) {
        debugmsg( "Bad npc json\n%s", jsonerr.c_str() );
    }
    if( !fac_id.str().empty() ) {
        set_fac( fac_id );
    }
}

void npc::randomize( const npc_class_id &type )
{
    if( getID() <= 0 ) {
        setID( g->assign_npc_id() );
    }

    weapon   = item( "null", 0 );
    inv.clear();
    personality.aggression = rng( -10, 10 );
    personality.bravery =    rng( -3, 10 );
    personality.collector =  rng( -1, 10 );
    personality.altruism =   rng( -10, 10 );
    moves = 100;
    mission = NPC_MISSION_NULL;
    male = one_in( 2 );
    pick_name();

    if( !type.is_valid() ) {
        debugmsg( "Invalid NPC class %s", type.c_str() );
        myclass = npc_class_id::NULL_ID();
    } else if( type.is_null() ) {
        myclass = npc_class::random_common();
    } else {
        myclass = type;
    }

    const auto &the_class = myclass.obj();
    str_max = the_class.roll_strength();
    dex_max = the_class.roll_dexterity();
    int_max = the_class.roll_intelligence();
    per_max = the_class.roll_perception();

    if( myclass->get_shopkeeper_items() != "EMPTY_GROUP" ) {
        restock = calendar::turn + 3_days;
        cash += 100000;
    }

    for( auto &skill : Skill::skills ) {
        int level = myclass->roll_skill( skill.ident() );

        set_skill_level( skill.ident(), level );
    }

    if( type.is_null() ) { // Untyped; no particular specialization
    } else if( type == NC_EVAC_SHOPKEEP ) {
        personality.collector += rng( 1, 5 );

    } else if( type == NC_BARTENDER ) {
        personality.collector += rng( 1, 5 );

    } else if( type == NC_JUNK_SHOPKEEP ) {
        personality.collector += rng( 1, 5 );

    } else if( type == NC_ARSONIST ) {
        personality.aggression += rng( 0, 1 );
        personality.collector += rng( 0, 2 );

    } else if( type == NC_SOLDIER ) {
        personality.aggression += rng( 1, 3 );
        personality.bravery += rng( 0, 5 );

    } else if( type == NC_HACKER ) {
        personality.bravery -= rng( 1, 3 );
        personality.aggression -= rng( 0, 2 );

    } else if( type == NC_DOCTOR ) {
        personality.aggression -= rng( 0, 4 );
        cash += 10000 * rng( 0, 3 ) * rng( 0, 3 );

    } else if( type == NC_TRADER ) {
        personality.collector += rng( 1, 5 );
        cash += 25000 * rng( 1, 10 );

    } else if( type == NC_NINJA ) {
        personality.bravery += rng( 0, 3 );
        personality.collector -= rng( 1, 6 );
        // TODO: give ninja his styles back

    } else if( type == NC_COWBOY ) {
        personality.aggression += rng( 0, 2 );
        personality.bravery += rng( 1, 5 );

    } else if( type == NC_SCIENTIST ) {
        personality.aggression -= rng( 1, 5 );
        personality.bravery -= rng( 2, 8 );
        personality.collector += rng( 0, 2 );

    } else if( type == NC_BOUNTY_HUNTER ) {
        personality.aggression += rng( 1, 6 );
        personality.bravery += rng( 0, 5 );

    } else if( type == NC_THUG ) {
        personality.aggression += rng( 1, 6 );
        personality.bravery += rng( 0, 5 );

    } else if( type == NC_SCAVENGER ) {
        personality.aggression += rng( 1, 3 );
        personality.bravery += rng( 1, 4 );

    }
    //A universal barter boost to keep NPCs competitive with players
    //The int boost from trade wasn't active... now that it is, most
    //players will vastly outclass npcs in trade without a little help.
    mod_skill_level( skill_barter, rng( 2, 4 ) );

    recalc_hp();
    for( int i = 0; i < num_hp_parts; i++ ) {
        hp_cur[i] = hp_max[i];
    }

    starting_weapon( myclass );
    starting_clothes( *this, myclass, male );
    starting_inv( *this, myclass );
    has_new_items = true;

    my_mutations.clear();
    my_traits.clear();

    // Add fixed traits
    for( const auto &tid : trait_group::traits_from( myclass->traits ) ) {
        set_mutation( tid );
    }

    // Run mutation rounds
    for( const auto &mr : type->mutation_rounds ) {
        int rounds = mr.second.roll();
        for( int i = 0; i < rounds; ++i ) {
            mutate_category( mr.first );
        }
    }
}

void npc::randomize_from_faction( faction *fac )
{
    // Personality = aggression, bravery, altruism, collector
    my_fac = fac;
    fac_id = fac->id;
    randomize( npc_class_id::NULL_ID() );

    switch( fac->goal ) {
        case FACGOAL_DOMINANCE:
            personality.aggression += rng( 0, 3 );
            personality.bravery += rng( 1, 4 );
            personality.altruism -= rng( 0, 2 );
            break;
        case FACGOAL_CLEANSE:
            personality.aggression -= rng( 0, 3 );
            personality.bravery += rng( 2, 4 );
            personality.altruism += rng( 0, 3 );
            personality.collector -= rng( 0, 2 );
            break;
        case FACGOAL_SHADOW:
            personality.bravery += rng( 4, 7 );
            personality.collector -= rng( 0, 3 );
            int_max += rng( 0, 2 );
            per_max += rng( 0, 2 );
            break;
        case FACGOAL_APOCALYPSE:
            personality.aggression += rng( 2, 5 );
            personality.bravery += rng( 4, 8 );
            personality.altruism -= rng( 1, 4 );
            personality.collector -= rng( 2, 5 );
            break;
        case FACGOAL_ANARCHY:
            personality.aggression += rng( 3, 6 );
            personality.bravery += rng( 0, 4 );
            personality.altruism -= rng( 3, 8 );
            personality.collector -= rng( 3, 6 );
            int_max -= rng( 1, 3 );
            per_max -= rng( 0, 2 );
            str_max += rng( 0, 3 );
            break;
        case FACGOAL_KNOWLEDGE:
            if( one_in( 2 ) ) {
                randomize( NC_SCIENTIST );
            }
            personality.aggression -= rng( 2, 5 );
            personality.bravery -= rng( 1, 4 );
            personality.collector += rng( 2, 4 );
            int_max += rng( 2, 5 );
            str_max -= rng( 1, 4 );
            per_max -= rng( 0, 2 );
            dex_max -= rng( 0, 2 );
            break;
        case FACGOAL_NATURE:
            personality.aggression -= rng( 0, 3 );
            personality.collector -= rng( 1, 4 );
            break;
        case FACGOAL_CIVILIZATION:
            personality.aggression -= rng( 2, 4 );
            personality.altruism += rng( 1, 5 );
            personality.collector += rng( 1, 5 );
            break;
        default:
            //Suppress warnings
            break;
    }
    // Jobs
    if( fac->has_job( FACJOB_EXTORTION ) ) {
        personality.aggression += rng( 0, 3 );
        personality.bravery -= rng( 0, 2 );
        personality.altruism -= rng( 2, 6 );
    }
    if( fac->has_job( FACJOB_INFORMATION ) ) {
        int_max += rng( 0, 4 );
        per_max += rng( 0, 4 );
        personality.aggression -= rng( 0, 4 );
        personality.collector += rng( 1, 3 );
    }
    if( fac->has_job( FACJOB_TRADE ) || fac->has_job( FACJOB_CARAVANS ) ) {
        if( !one_in( 3 ) ) {
            randomize( NC_TRADER );
        }
        personality.aggression -= rng( 1, 5 );
        personality.collector += rng( 1, 4 );
        personality.altruism -= rng( 0, 3 );
    }
    if( fac->has_job( FACJOB_SCAVENGE ) ) {
        personality.collector += rng( 4, 8 );
    }
    if( fac->has_job( FACJOB_MERCENARIES ) ) {
        if( !one_in( 3 ) ) {
            switch( rng( 1, 3 ) ) {
                case 1:
                    randomize( NC_NINJA );
                    break;
                case 2:
                    randomize( NC_COWBOY );
                    break;
                case 3:
                    randomize( NC_BOUNTY_HUNTER );
                    break;
                default:
                    randomize( NC_COWBOY );
                    break;
            }
        }
        personality.aggression += rng( 0, 2 );
        personality.bravery += rng( 2, 4 );
        personality.altruism -= rng( 2, 4 );
        str_max += rng( 0, 2 );
        per_max += rng( 0, 2 );
        dex_max += rng( 0, 2 );
    }
    if( fac->has_job( FACJOB_ASSASSINS ) ) {
        personality.bravery -= rng( 0, 2 );
        personality.altruism -= rng( 1, 3 );
        per_max += rng( 1, 3 );
        dex_max += rng( 0, 2 );
    }
    if( fac->has_job( FACJOB_RAIDERS ) ) {
        if( one_in( 3 ) ) {
            randomize( NC_COWBOY );
        }
        personality.aggression += rng( 3, 5 );
        personality.bravery += rng( 0, 2 );
        personality.altruism -= rng( 3, 6 );
        str_max += rng( 0, 3 );
        int_max -= rng( 0, 2 );
    }
    if( fac->has_job( FACJOB_THIEVES ) ) {
        if( one_in( 3 ) ) {
            randomize( NC_NINJA );
        }
        personality.aggression -= rng( 2, 5 );
        personality.bravery -= rng( 1, 3 );
        personality.altruism -= rng( 1, 4 );
        str_max -= rng( 0, 2 );
        per_max += rng( 1, 4 );
        dex_max += rng( 1, 3 );
    }
    if( fac->has_job( FACJOB_DOCTORS ) ) {
        if( !one_in( 4 ) ) {
            randomize( NC_DOCTOR );
        }
        personality.aggression -= rng( 3, 6 );
        personality.bravery += rng( 0, 4 );
        personality.altruism += rng( 0, 4 );
        int_max += rng( 2, 4 );
        per_max += rng( 0, 2 );
        mod_skill_level( skill_firstaid, int( rng( 1, 5 ) ) );
    }
    if( fac->has_job( FACJOB_FARMERS ) ) {
        personality.aggression -= rng( 2, 4 );
        personality.altruism += rng( 0, 3 );
        str_max += rng( 1, 3 );
    }
    if( fac->has_job( FACJOB_DRUGS ) ) {
        personality.aggression -= rng( 0, 2 );
        personality.bravery -= rng( 0, 3 );
        personality.altruism -= rng( 1, 4 );
    }
    if( fac->has_job( FACJOB_MANUFACTURE ) ) {
        personality.aggression -= rng( 0, 2 );
        personality.bravery -= rng( 0, 2 );
        switch( rng( 1, 4 ) ) {
            case 1:
                mod_skill_level( skill_mechanics,   dice( 2, 4 ) );
                break;
            case 2:
                mod_skill_level( skill_electronics, dice( 2, 4 ) );
                break;
            case 3:
                mod_skill_level( skill_cooking,     dice( 2, 4 ) );
                break;
            case 4:
                mod_skill_level( skill_tailor,      dice( 2, 4 ) );
                break;
            default:
                mod_skill_level( skill_cooking,     dice( 2, 4 ) );
                break;
        }
    }

    if( fac->has_value( FACVAL_CHARITABLE ) ) {
        personality.aggression -= rng( 2, 5 );
        personality.bravery += rng( 0, 4 );
        personality.altruism += rng( 2, 5 );
    }
    if( fac->has_value( FACVAL_LONERS ) ) {
        personality.aggression -= rng( 1, 3 );
        personality.altruism -= rng( 1, 4 );
    }
    if( fac->has_value( FACVAL_EXPLORATION ) ) {
        per_max += rng( 0, 4 );
        personality.aggression -= rng( 0, 2 );
    }
    if( fac->has_value( FACVAL_ARTIFACTS ) ) {
        personality.collector += rng( 2, 5 );
        personality.altruism -= rng( 0, 2 );
    }
    if( fac->has_value( FACVAL_BIONICS ) ) {
        str_max += rng( 0, 2 );
        dex_max += rng( 0, 2 );
        per_max += rng( 0, 2 );
        int_max += rng( 0, 4 );
        if( one_in( 3 ) ) {
            mod_skill_level( skill_mechanics, dice( 2, 3 ) );
            mod_skill_level( skill_electronics, dice( 2, 3 ) );
            mod_skill_level( skill_firstaid, dice( 2, 3 ) );
        }
    }
    if( fac->has_value( FACVAL_BOOKS ) ) {
        str_max -= rng( 0, 2 );
        per_max -= rng( 0, 3 );
        int_max += rng( 0, 4 );
        personality.aggression -= rng( 1, 4 );
        personality.bravery -= rng( 0, 3 );
        personality.collector += rng( 0, 3 );
    }
    if( fac->has_value( FACVAL_TRAINING ) ) {
        str_max += rng( 0, 3 );
        dex_max += rng( 0, 3 );
        per_max += rng( 0, 2 );
        int_max += rng( 0, 2 );
        for( const auto &skill : Skill::skills ) {
            if( one_in( 3 ) ) {
                mod_skill_level( skill.ident(), rng( 2, 4 ) );
            }
        }
    }
    if( fac->has_value( FACVAL_ROBOTS ) ) {
        int_max += rng( 0, 3 );
        personality.aggression -= rng( 0, 3 );
        personality.collector += rng( 0, 3 );
    }
    if( fac->has_value( FACVAL_TREACHERY ) ) {
        personality.aggression += rng( 0, 3 );
        personality.altruism -= rng( 2, 5 );
    }
    if( fac->has_value( FACVAL_STRAIGHTEDGE ) ) {
        personality.collector -= rng( 0, 2 );
        str_max += rng( 0, 1 );
        per_max += rng( 0, 2 );
        int_max += rng( 0, 3 );
    }
    if( fac->has_value( FACVAL_LAWFUL ) ) {
        personality.aggression -= rng( 3, 7 );
        personality.altruism += rng( 1, 5 );
        int_max += rng( 0, 2 );
    }
    if( fac->has_value( FACVAL_CRUELTY ) ) {
        personality.aggression += rng( 3, 6 );
        personality.bravery -= rng( 1, 4 );
        personality.altruism -= rng( 2, 5 );
    }
}

void npc::set_fac( const string_id<faction> &id )
{
    my_fac = g->faction_manager_ptr->get( id );
    fac_id = my_fac->id;
}

// item id from group "<class-name>_<what>" or from fallback group
// may still be a null item!
item random_item_from( const npc_class_id &type, const std::string &what,
                       const std::string &fallback )
{
    auto result = item_group::item_from( type.str() + "_" + what );
    if( result.is_null() ) {
        result = item_group::item_from( fallback );
    }
    return result;
}

// item id from "<class-name>_<what>" or from "npc_<what>"
item random_item_from( const npc_class_id &type, const std::string &what )
{
    return random_item_from( type, what, "npc_" + what );
}

// item id from "<class-name>_<what>_<gender>" or from "npc_<what>_<gender>"
item get_clothing_item( const npc_class_id &type, const std::string &what, bool male )
{
    item result;
    //Check if class has gendered clothing
    //Then check if it has an ungendered version
    //Only if all that fails, grab from the default class.
    if( male ) {
        result = random_item_from( type, what + "_male", "null" );
    } else {
        result = random_item_from( type, what + "_female", "null" );
    }
    if( result.is_null() ) {
        if( male ) {
            result = random_item_from( type, what, "npc_" + what + "_male" );
        } else {
            result = random_item_from( type, what, "npc_" + what + "_female" );
        }
    }

    return result;
}

void starting_clothes( npc &who, const npc_class_id &type, bool male )
{
    std::vector<item> ret;

    if( item_group::group_is_defined( type->worn_override ) ) {
        ret = item_group::items_from( type->worn_override );
    } else {
        ret.push_back( get_clothing_item( type, "pants", male ) );
        ret.push_back( get_clothing_item( type, "shirt", male ) );
        ret.push_back( get_clothing_item( type, "underwear_top", male ) );
        ret.push_back( get_clothing_item( type, "underwear_bottom", male ) );
        ret.push_back( get_clothing_item( type, "underwear_feet", male ) );
        ret.push_back( get_clothing_item( type, "shoes", male ) );
        ret.push_back( random_item_from( type, "gloves" ) );
        ret.push_back( random_item_from( type, "coat" ) );
        ret.push_back( random_item_from( type, "vest" ) );
        ret.push_back( random_item_from( type, "masks" ) );
        // Why is the alternative group not named "npc_glasses" but "npc_eyes"?
        ret.push_back( random_item_from( type, "glasses", "npc_eyes" ) );
        ret.push_back( random_item_from( type, "hat" ) );
        ret.push_back( random_item_from( type, "scarf" ) );
        ret.push_back( random_item_from( type, "storage" ) );
        ret.push_back( random_item_from( type, "holster" ) );
        ret.push_back( random_item_from( type, "belt" ) );
        ret.push_back( random_item_from( type, "wrist" ) );
        ret.push_back( random_item_from( type, "extra" ) );
    }

    who.worn.clear();
    for( item &it : ret ) {
        if( it.has_flag( "VARSIZE" ) ) {
            it.item_tags.insert( "FIT" );
        }

        if( who.can_wear( it ).success() ) {
            who.worn.push_back( it );
        }
    }
}

void starting_inv( npc &who, const npc_class_id &type )
{
    std::list<item> res;
    who.inv.clear();
    if( item_group::group_is_defined( type->carry_override ) ) {
        who.inv += item_group::items_from( type->carry_override );
        return;
    }

    res.emplace_back( "lighter" );
    // If wielding a gun, get some additional ammo for it
    if( who.weapon.is_gun() ) {
        item ammo( who.weapon.ammo_type()->default_ammotype() );
        ammo = ammo.in_its_container();
        if( ammo.made_of( LIQUID ) ) {
            item container( "bottle_plastic" );
            container.put_in( ammo );
            ammo = container;
        }

        // @todo: Move to npc_class
        // NC_COWBOY and NC_BOUNTY_HUNTER get 5-15 whilst all others get 3-6
        long qty = 1 + ( type == NC_COWBOY ||
                         type == NC_BOUNTY_HUNTER );
        qty = rng( qty, qty * 2 );

        while( qty-- != 0 && who.can_pickVolume( ammo ) ) {
            // @todo: give NPC a default magazine instead
            res.push_back( ammo );
        }
    }

    if( type == NC_ARSONIST ) {
        res.emplace_back( "molotov" );
    }

    long qty = ( type == NC_EVAC_SHOPKEEP ||
                 type == NC_TRADER ) ? 5 : 2;
    qty = rng( qty, qty * 3 );

    while( qty-- != 0 ) {
        item tmp = random_item_from( type, "misc" ).in_its_container();
        if( !tmp.is_null() ) {
            if( !one_in( 3 ) && tmp.has_flag( "VARSIZE" ) ) {
                tmp.item_tags.insert( "FIT" );
            }
            if( who.can_pickVolume( tmp ) ) {
                res.push_back( tmp );
            }
        }
    }

    res.erase( std::remove_if( res.begin(), res.end(), [&]( const item & e ) {
        return e.has_flag( "TRADER_AVOID" );
    } ), res.end() );

    who.inv += res;
}

void npc::spawn_at_sm( int x, int y, int z )
{
    spawn_at_precise( point( x, y ), tripoint( rng( 0, SEEX - 1 ), rng( 0, SEEY - 1 ), z ) );
}

void npc::spawn_at_precise( const point &submap_offset, const tripoint &square )
{
    submap_coords = submap_offset;
    submap_coords.x += square.x / SEEX;
    submap_coords.y += square.y / SEEY;
    position.x = square.x % SEEX;
    position.y = square.y % SEEY;
    position.z = square.z;
}

tripoint npc::global_square_location() const
{
    return tripoint( submap_coords.x * SEEX + posx() % SEEX, submap_coords.y * SEEY + posy() % SEEY,
                     position.z );
}

void npc::place_on_map()
{
    // The global absolute position (in map squares) of the npc is *always*
    // "submap_coords.x * SEEX + posx() % SEEX" (analog for y).
    // The main map assumes that pos is in its own (local to the main map)
    // coordinate system. We have to change pos to match that assumption
    const int dmx = submap_coords.x - g->get_levx();
    const int dmy = submap_coords.y - g->get_levy();
    const int offset_x = position.x % SEEX;
    const int offset_y = position.y % SEEY;
    // value of "submap_coords.x * SEEX + posx()" is unchanged
    setpos( tripoint( offset_x + dmx * SEEX, offset_y + dmy * SEEY, posz() ) );

    if( g->is_empty( pos() ) ) {
        return;
    }

    for( const tripoint &p : closest_tripoints_first( SEEX + 1, pos() ) ) {
        if( g->is_empty( p ) ) {
            setpos( p );
            return;
        }
    }

    debugmsg( "Failed to place NPC in a valid location near (%d,%d,%d)", posx(), posy(), posz() );
}

skill_id npc::best_skill() const
{
    int highest_level = std::numeric_limits<int>::min();
    skill_id highest_skill( skill_id::NULL_ID() );

    for( const auto &p : *_skills ) {
        if( p.first.obj().is_combat_skill() ) {
            const int level = p.second.level();
            if( level > highest_level ) {
                highest_level = level;
                highest_skill = p.first;
            }
        }
    }

    return highest_skill;
}

void npc::starting_weapon( const npc_class_id &type )
{
    if( item_group::group_is_defined( type->weapon_override ) ) {
        weapon = item_group::item_from( type->weapon_override );
        return;
    }

    const skill_id best = best_skill();

    // if NPC has no suitable skills default to stabbing weapon
    if( !best || best == skill_stabbing ) {
        weapon = random_item_from( type, "stabbing", "survivor_stabbing" );
    } else if( best == skill_bashing ) {
        weapon = random_item_from( type, "bashing", "survivor_bashing" );
    } else if( best == skill_cutting ) {
        weapon = random_item_from( type, "cutting", "survivor_cutting" );
    } else if( best == skill_throw ) {
        weapon = random_item_from( type, "throw" );
    } else if( best == skill_archery ) {
        weapon = random_item_from( type, "archery" );
    } else if( best == skill_pistol ) {
        weapon = random_item_from( type, "pistol", "guns_pistol_common" );
    } else if( best == skill_shotgun ) {
        weapon = random_item_from( type, "shotgun", "guns_shotgun_common" );
    } else if( best == skill_smg ) {
        weapon = random_item_from( type, "smg", "guns_smg_common" );
    } else if( best == skill_rifle ) {
        weapon = random_item_from( type, "rifle", "guns_rifle_common" );
    }

    if( weapon.is_gun() ) {
        weapon.ammo_set( weapon.type->gun->ammo->default_ammotype() );
    }
}

bool npc::wear_if_wanted( const item &it )
{
    // Note: this function isn't good enough to use with NPC AI alone
    // Restrict it to player's orders for now
    if( !it.is_armor() ) {
        return false;
    }

    // TODO: Make it depend on stuff
    static const std::array<int, num_bp> max_encumb = {{
            30, // bp_torso - Higher if ranged?
            100, // bp_head
            30, // bp_eyes - Lower if using ranged?
            30, // bp_mouth
            30, // bp_arm_l
            30, // bp_arm_r
            30, // bp_hand_l - Lower if throwing?
            30, // bp_hand_r
            // Must be enough to allow hazmat, turnout etc.
            30, // bp_leg_l - Higher if ranged?
            30, // bp_leg_r
            // Doesn't hurt much
            50, // bp_foot_l
            50, // bp_foot_r
        }
    };

    // Splints ignore limits, but only when being equipped on a broken part
    // TODO: Drop splints when healed
    bool splint = it.has_flag( "SPLINT" );
    if( splint ) {
        splint = false;
        for( int i = 0; i < num_hp_parts; i++ ) {
            hp_part hpp = hp_part( i );
            body_part bp = player::hp_to_bp( hpp );
            if( hp_cur[i] <= 0 && it.covers( bp ) ) {
                splint = true;
                break;
            }
        }
    }

    if( splint ) {
        return !!wear_item( it, false );
    }

    const int it_encumber = it.get_encumber( *this );
    while( !worn.empty() ) {
        auto size_before = worn.size();
        bool encumb_ok = true;
        const auto new_enc = get_encumbrance( it );
        // Strip until we can put the new item on
        // This is one of the reasons this command is not used by the AI
        for( const body_part bp : all_body_parts ) {
            if( !it.covers( bp ) ) {
                continue;
            }

            if( it_encumber > max_encumb[bp] ) {
                // Not an NPC-friendly item
                return false;
            }

            if( new_enc[bp].encumbrance > max_encumb[bp] ) {
                encumb_ok = false;
                break;
            }
        }

        if( encumb_ok && can_wear( it ).success() ) {
            // @todo: Hazmat/power armor makes this not work due to 1 boots/headgear limit
            return !!wear_item( it, false );
        }
        // Otherwise, maybe we should take off one or more items and replace them
        bool took_off = false;
        for( const body_part bp : all_body_parts ) {
            if( !it.covers( bp ) ) {
                continue;
            }
            // Find an item that covers the same body part as the new item
            auto iter = std::find_if( worn.begin(), worn.end(), [bp]( const item & armor ) {
                return armor.covers( bp );
            } );
            if( iter != worn.end() ) {
                took_off = takeoff( *iter );
                break;
            }
        }

        if( !took_off || worn.size() >= size_before ) {
            // Shouldn't happen, but does
            return false;
        }
    }

    return worn.empty() && wear_item( it, false );
}

bool npc::wield( item &it )
{
    if( is_armed() ) {
        // If weapon has a shoulder strap, try to wear it.
        if( wear_item( weapon, false ) ) {
            // Wearing the item was successful, remove weapon and post message.
            add_msg_if_npc( m_info, _( "<npcname> wears the %s." ), weapon.tname().c_str() );
            remove_weapon();
            moves -= 15;
        } else { // Weapon cannot be worn or wearing was not successful. Store it in inventory if possible, otherwise drop it.
            if( volume_carried() + weapon.volume() <= volume_capacity() ) {
                add_msg_if_npc( m_info, _( "<npcname> puts away the %s." ), weapon.tname().c_str() );
                i_add( remove_weapon() );
                moves -= 15;
            } else { // No room for weapon, so we drop it
                add_msg_if_npc( m_info, _( "<npcname> drops the %s." ), weapon.tname().c_str() );
                g->m.add_item_or_charges( pos(), remove_weapon() );
            }
        }
    }

    if( it.is_null() ) {
        weapon = item();
        return true;
    }

    moves -= 15;
    if( has_item( it ) ) {
        weapon = remove_item( it );
    } else {
        weapon = it;
    }

    add_msg_if_npc( m_info, _( "<npcname> wields a %s." ),  weapon.tname().c_str() );
    return true;
}

void npc::form_opinion( const player &u )
{
    // FEAR
    if( u.weapon.is_gun() ) {
        // @todo: Make bows not guns
        if( weapon.is_gun() ) {
            op_of_u.fear += 2;
        } else {
            op_of_u.fear += 6;
        }
    } else if( u.weapon_value( u.weapon ) > 20 ) {
        op_of_u.fear += 2;
    } else if( !u.is_armed() ) {
        // Unarmed, but actually unarmed ("unarmed weapons" are not unarmed)
        op_of_u.fear -= 3;
    }

    ///\EFFECT_STR increases NPC fear of the player
    if( u.str_max >= 16 ) {
        op_of_u.fear += 2;
    } else if( u.str_max >= 12 ) {
        op_of_u.fear += 1;
    } else if( u.str_max <= 3 ) {
        op_of_u.fear -= 3;
    } else if( u.str_max <= 5 ) {
        op_of_u.fear -= 1;
    }

    for( int i = 0; i < num_hp_parts; i++ ) {
        if( u.hp_cur[i] <= u.hp_max[i] / 2 ) {
            op_of_u.fear--;
        }
        if( hp_cur[i] <= hp_max[i] / 2 ) {
            op_of_u.fear++;
        }
    }

    if( u.has_trait( trait_SAPIOVORE ) ) {
        op_of_u.fear += 10; // Sapiovores = Scary
    }
    if( u.has_trait( trait_TERRIFYING ) ) {
        op_of_u.fear += 6;
    }

    int u_ugly = 0;
    for( trait_id &mut : u.get_mutations() ) {
        u_ugly += mut.obj().ugliness;
    }
    op_of_u.fear += u_ugly / 2;
    op_of_u.trust -= u_ugly / 3;

    if( u.stim > 20 ) {
        op_of_u.fear++;
    }

    if( u.has_effect( effect_drunk ) ) {
        op_of_u.fear -= 2;
    }

    // TRUST
    if( op_of_u.fear > 0 ) {
        op_of_u.trust -= 3;
    } else {
        op_of_u.trust += 1;
    }

    if( u.weapon.is_gun() ) {
        op_of_u.trust -= 2;
    } else if( !u.is_armed() ) {
        op_of_u.trust += 2;
    }

    // @todo: More effects
    if( u.has_effect( effect_high ) ) {
        op_of_u.trust -= 1;
    }
    if( u.has_effect( effect_drunk ) ) {
        op_of_u.trust -= 2;
    }
    if( u.stim > 20 || u.stim < -20 ) {
        op_of_u.trust -= 1;
    }
    if( u.get_painkiller() > 30 ) {
        op_of_u.trust -= 1;
    }

    if( op_of_u.trust > 0 ) {
        // Trust is worth a lot right now
        op_of_u.trust /= 2;
    }

    // VALUE
    op_of_u.value = 0;
    for( int i = 0; i < num_hp_parts; i++ ) {
        if( hp_cur[i] < hp_max[i] * 0.8f ) {
            op_of_u.value++;
        }
    }
    decide_needs();
    for( auto &i : needs ) {
        if( i == need_food || i == need_drink ) {
            op_of_u.value += 2;
        }
    }

    if( op_of_u.fear < personality.bravery + 10 &&
        op_of_u.fear - personality.aggression > -10 && op_of_u.trust > -8 ) {
        set_attitude( NPCATT_TALK );
    } else if( op_of_u.fear - 2 * personality.aggression - personality.bravery < -30 ) {
        set_attitude( NPCATT_KILL );
    } else if( my_fac != nullptr && my_fac->likes_u < -10 ) {
        set_attitude( NPCATT_KILL );
    } else {
        set_attitude( NPCATT_FLEE );
    }

    add_msg( m_debug, "%s formed an opinion of u: %s",
             name.c_str(), npc_attitude_name( attitude ).c_str() );
}

float npc::vehicle_danger( int radius ) const
{
    const tripoint from( posx() - radius, posy() - radius, posz() );
    const tripoint to( posx() + radius, posy() + radius, posz() );
    VehicleList vehicles = g->m.get_vehicles( from, to );

    int danger = 0;

    // TODO: check for most dangerous vehicle?
    for( unsigned int i = 0; i < vehicles.size(); ++i ) {
        const wrapped_vehicle &wrapped_veh = vehicles[i];
        if( wrapped_veh.v->is_moving() ) {
            // #FIXME this can't be the right way to do this
            float facing = wrapped_veh.v->face.dir();

            int ax = wrapped_veh.v->global_pos3().x;
            int ay = wrapped_veh.v->global_pos3().y;
            int bx = int( ax + cos( facing * M_PI / 180.0 ) * radius );
            int by = int( ay + sin( facing * M_PI / 180.0 ) * radius );

            // fake size
            /* This will almost certainly give the wrong size/location on customized
             * vehicles. This should just count frames instead. Or actually find the
             * size. */
            vehicle_part last_part = wrapped_veh.v->parts.back();
            int size = std::max( last_part.mount.x, last_part.mount.y );

            double normal = sqrt( static_cast<float>( ( bx - ax ) * ( bx - ax ) + ( by - ay ) * ( by - ay ) ) );
            int closest = int( abs( ( posx() - ax ) * ( by - ay ) - ( posy() - ay ) * ( bx - ax ) ) / normal );

            if( size > closest ) {
                danger = i;
            }
        }
    }
    return danger;
}

bool npc::turned_hostile() const
{
    return ( op_of_u.anger >= hostile_anger_level() );
}

int npc::hostile_anger_level() const
{
    return ( 20 + op_of_u.fear - personality.aggression );
}

void npc::make_angry()
{
    if( is_enemy() ) {
        return; // We're already angry!
    }

    // Make associated faction, if any, angry at the player too.
    if( my_fac != nullptr ) {
        my_fac->likes_u = std::max( -50, my_fac->likes_u - 50 );
        my_fac->respects_u = std::max( -50, my_fac->respects_u - 50 );
    }
    if( op_of_u.fear > 10 + personality.aggression + personality.bravery ) {
        set_attitude( NPCATT_FLEE ); // We don't want to take u on!
    } else {
        set_attitude( NPCATT_KILL ); // Yeah, we think we could take you!
    }
}

void npc::on_attacked( const Creature &attacker )
{
    if( attacker.is_player() && !is_enemy() ) {
        make_angry();
        hit_by_player = true;
    }
}

int npc::assigned_missions_value()
{
    int ret = 0;
    for( auto &m : chatbin.missions_assigned ) {
        ret += m->get_value();
    }
    return ret;
}

std::vector<skill_id> npc::skills_offered_to( const player &p ) const
{
    std::vector<skill_id> ret;
    for( const auto &pair : *_skills ) {
        const skill_id &id = pair.first;
        if( p.get_skill_level( id ) < pair.second.level() ) {
            ret.push_back( id );
        }
    }
    return ret;
}

std::vector<matype_id> npc::styles_offered_to( const player &p ) const
{
    std::vector<matype_id> ret;
    for( auto &i : ma_styles ) {
        if( !p.has_martialart( i ) ) {
            ret.push_back( i );
        }
    }
    return ret;
}

bool npc::fac_has_value( faction_value value ) const
{
    if( my_fac == nullptr ) {
        return false;
    }

    return my_fac->has_value( value );
}

bool npc::fac_has_job( faction_job job ) const
{
    if( my_fac == nullptr ) {
        return false;
    }

    return my_fac->has_job( job );
}

void npc::decide_needs()
{
    double needrank[num_needs];
    for( auto &elem : needrank ) {
        elem = 20;
    }
    if( weapon.is_gun() ) {
        needrank[need_ammo] = 5 * get_ammo( weapon.type->gun->ammo ).size();
    }

    needrank[need_weapon] = weapon_value( weapon );
    needrank[need_food] = 15 - get_hunger();
    needrank[need_drink] = 15 - get_thirst();
    invslice slice = inv.slice();
    for( auto &i : slice ) {
        item inventory_item = i->front();
        if( inventory_item.is_food( ) ) {
            needrank[ need_food ] += nutrition_for( inventory_item ) / 4;
            needrank[ need_drink ] += inventory_item.type->comestible->quench / 4;
        } else if( inventory_item.is_food_container() ) {
            needrank[ need_food ] += nutrition_for( inventory_item.contents.front() ) / 4;
            needrank[ need_drink ] += inventory_item.contents.front().type->comestible->quench / 4;
        }
    }
    needs.clear();
    size_t j;
    bool serious = false;
    for( int i = 1; i < num_needs; i++ ) {
        if( needrank[i] < 10 ) {
            serious = true;
        }
    }
    if( !serious ) {
        needs.push_back( need_none );
        needrank[0] = 10;
    }
    for( int i = 1; i < num_needs; i++ ) {
        if( needrank[i] < 20 ) {
            for( j = 0; j < needs.size(); j++ ) {
                if( needrank[i] < needrank[needs[j]] ) {
                    needs.insert( needs.begin() + j, npc_need( i ) );
                    j = needs.size() + 1;
                }
            }
            if( j == needs.size() ) {
                needs.push_back( npc_need( i ) );
            }
        }
    }
}

void npc::say( const std::string &line ) const
{
    std::string formatted_line = line;
    parse_tags( formatted_line, g->u, *this );
    if( has_trait( trait_id( "MUTE" ) ) ) {
        return;
    }

    std::string sound = string_format( _( "%1$s saying \"%2$s\"" ), name, formatted_line );
    if( g->u.sees( *this ) && g->u.is_deaf() ) {
        add_msg( m_warning, _( "%1$s says something but you can't hear it!" ), name );
    }
    // Sound happens even if we can't hear it
    sounds::sound( pos(), 16, sounds::sound_t::speech, sound );
}

bool npc::wants_to_sell( const item &it ) const
{
    const int market_price = it.price( true );
    return wants_to_sell( it, value( it, market_price ), market_price );
}

bool npc::wants_to_sell( const item &it, int at_price, int market_price ) const
{
    ( void )it;

    if( mission == NPC_MISSION_SHOPKEEP ) {
        return true;
    }

    if( is_friend() ) {
        return true;
    }

    // TODO: Base on inventory
    return at_price - market_price <= 50;
}

bool npc::wants_to_buy( const item &it ) const
{
    const int market_price = it.price( true );
    return wants_to_buy( it, value( it, market_price ), market_price );
}

bool npc::wants_to_buy( const item &it, int at_price, int market_price ) const
{
    ( void )market_price;
    ( void )it;

    if( is_friend() ) {
        return true;
    }

    // TODO: Base on inventory
    return at_price >= 80;
}

void npc::shop_restock()
{
    restock = calendar::turn + 3_days;
    if( is_friend() ) {
        return;
    }

    const Group_tag &from = myclass->get_shopkeeper_items();
    if( from == "EMPTY_GROUP" ) {
        return;
    }

    units::volume total_space = volume_capacity();
    std::list<item> ret;

    while( total_space > 0_ml && !one_in( 50 ) ) {
        item tmpit = item_group::item_from( from, 0 );
        if( !tmpit.is_null() && total_space >= tmpit.volume() ) {
            ret.push_back( tmpit );
            total_space -= tmpit.volume();
        }
    }

    has_new_items = true;
    inv.clear();
    inv.push_back( ret );
}

int npc::minimum_item_value() const
{
    // TODO: Base on inventory
    int ret = 20;
    ret -= personality.collector;
    return ret;
}

void npc::update_worst_item_value()
{
    worst_item_value = 99999;
    // TODO: Cache this
    int inv_val = inv.worst_item_value( this );
    if( inv_val < worst_item_value ) {
        worst_item_value = inv_val;
    }
}

int npc::value( const item &it ) const
{
    int market_price = it.price( true );
    return value( it, market_price );
}

int npc::value( const item &it, int market_price ) const
{
    if( it.is_dangerous() || ( it.has_flag( "BOMB" ) && it.active ) || it.made_of( LIQUID ) ) {
        // NPCs won't be interested in buying active explosives or spilled liquids
        return -1000;
    }

    int ret = 0;
    // TODO: Cache own weapon value (it can be a bit expensive to compute 50 times/turn)
    double weapon_val = weapon_value( it ) - weapon_value( weapon );
    if( weapon_val > 0 ) {
        ret += weapon_val;
    }

    if( it.is_food() ) {
        int comestval = 0;
        if( nutrition_for( it ) > 0 || it.type->comestible->quench > 0 ) {
            comestval++;
        }
        if( get_hunger() > 40 ) {
            comestval += ( nutrition_for( it ) + get_hunger() - 40 ) / 6;
        }
        if( get_thirst() > 40 ) {
            comestval += ( it.type->comestible->quench + get_thirst() - 40 ) / 4;
        }
        if( comestval > 0 && will_eat( it ).success() ) {
            ret += comestval;
        }
    }

    if( it.is_ammo() ) {
        if( weapon.is_gun() && it.type->ammo->type.count( weapon.ammo_type() ) ) {
            ret += 14; // @todo: magazines - don't count ammo as usable if the weapon isn't.
        }

        if( std::any_of( it.type->ammo->type.begin(), it.type->ammo->type.end(),
        [&]( const ammotype & e ) {
        return has_gun_for_ammo( e );
        } ) ) {
            ret += 14; // @todo: consider making this cumulative (once was)
        }
    }

    if( it.is_book() ) {
        auto &book = *it.type->book;
        ret += book.fun;
        if( book.skill && get_skill_level( book.skill ) < book.level &&
            get_skill_level( book.skill ) >= book.req ) {
            ret += book.level * 3;
        }
    }

    // TODO: Sometimes we want more than one tool?  Also we don't want EVERY tool.
    if( it.is_tool() && !has_amount( it.typeId(), 1 ) ) {
        ret += 8;
    }

    // TODO: Artifact hunting from relevant factions
    // ALSO TODO: Bionics hunting from relevant factions
    if( fac_has_job( FACJOB_DRUGS ) && it.is_food() && it.type->comestible->addict >= 5 ) {
        ret += 10;
    }

    if( fac_has_job( FACJOB_DOCTORS ) && it.is_food() && it.type->comestible->comesttype == "MED" ) {
        ret += 10;
    }

    if( fac_has_value( FACVAL_BOOKS ) && it.is_book() ) {
        ret += 14;
    }

    if( fac_has_job( FACJOB_SCAVENGE ) ) {
        // Computed last for _reasons_.
        ret += 6;
        ret *= 1.3;
    }

    // Practical item value is more important than price
    ret *= 50;
    ret += market_price;
    return ret;
}

bool npc::has_healing_item( bool bleed, bool bite, bool infect )
{
    return !get_healing_item( bleed, bite, infect, true ).is_null();
}

item &npc::get_healing_item( bool bleed, bool bite, bool infect, bool first_best )
{
    item *best = &null_item_reference();
    visit_items( [&best, bleed, bite, infect, first_best]( item * node ) {
        const auto use = node->type->get_use( "heal" );
        if( use == nullptr ) {
            return VisitResponse::NEXT;
        }

        auto &actor = dynamic_cast<const heal_actor &>( *( use->get_actor_ptr() ) );
        if( ( !bleed || actor.bleed > 0 ) ||
            ( !bite || actor.bite > 0 ) ||
            ( !infect || actor.infect > 0 ) ) {
            best = node;
            if( first_best ) {
                return VisitResponse::ABORT;
            }
        }

        return VisitResponse::NEXT;
    } );

    return *best;
}

bool npc::has_painkiller()
{
    return inv.has_enough_painkiller( get_pain() );
}

bool npc::took_painkiller() const
{
    return ( has_effect( effect_pkill1 ) || has_effect( effect_pkill2 ) ||
             has_effect( effect_pkill3 ) || has_effect( effect_pkill_l ) );
}

bool npc::is_friend() const
{
    return attitude == NPCATT_FOLLOW || attitude == NPCATT_LEAD;
}

bool npc::is_minion() const
{
    return is_friend() && op_of_u.trust >= 5;
}

bool npc::guaranteed_hostile() const
{
    return is_enemy() || ( my_fac != nullptr && my_fac->likes_u < -10 );
}

bool npc::is_following() const
{
    switch( attitude ) {
        case NPCATT_FOLLOW:
        case NPCATT_WAIT:
            return true;
        default:
            return false;
    }
}

bool npc::is_leader() const
{
    return ( attitude == NPCATT_LEAD );
}

bool npc::is_enemy() const
{
    return attitude == NPCATT_KILL || attitude == NPCATT_FLEE;
}

bool npc::is_guarding() const
{
    return mission == NPC_MISSION_SHELTER || mission == NPC_MISSION_BASE ||
           mission == NPC_MISSION_SHOPKEEP || mission == NPC_MISSION_GUARD ||
           mission == NPC_MISSION_GUARD_ALLY ||
           has_effect( effect_infection );
}

Creature::Attitude npc::attitude_to( const Creature &other ) const
{
    if( is_friend() ) {
        // Friendly NPCs share player's alliances
        return g->u.attitude_to( other );
    }

    if( other.is_npc() ) {
        // Hostile NPCs are also hostile towards player's allied
        if( is_enemy() && other.attitude_to( g->u ) == A_FRIENDLY ) {
            return A_HOSTILE;
        }

        return A_NEUTRAL;
    } else if( other.is_player() ) {
        // For now, make it symmetric.
        return other.attitude_to( *this );
    }

    // @todo: Get rid of the ugly cast without duplicating checks
    const monster &m = dynamic_cast<const monster &>( other );
    switch( m.attitude( this ) ) {
        case MATT_FOLLOW:
        case MATT_FPASSIVE:
        case MATT_IGNORE:
        case MATT_FLEE:
            return A_NEUTRAL;
        case MATT_FRIEND:
        case MATT_ZLAVE:
            return A_FRIENDLY;
        case MATT_ATTACK:
            return A_HOSTILE;
        case MATT_NULL:
        case NUM_MONSTER_ATTITUDES:
            break;
    }

    return A_NEUTRAL;
}

int npc::smash_ability() const
{
    if( !is_following() || rules.has_flag( ally_rule::allow_bash ) ) {
        ///\EFFECT_STR_NPC increases smash ability
        return str_cur + weapon.damage_melee( DT_BASH );
    }

    // Not allowed to bash
    return 0;
}

float npc::danger_assessment()
{
    return ai_cache.danger_assessment;
}

float npc::average_damage_dealt()
{
    return float( melee_value( weapon ) );
}

bool npc::bravery_check( int diff )
{
    return ( dice( 10 + personality.bravery, 6 ) >= dice( diff, 4 ) );
}

bool npc::emergency() const
{
    return emergency( ai_cache.danger_assessment );
}

bool npc::emergency( float danger ) const
{
    return ( danger > ( personality.bravery * 3 * hp_percentage() ) / 100 );
}

//Check if this npc is currently in the list of active npcs.
//Active npcs are the npcs near the player that are actively simulated.
bool npc::is_active() const
{
    return g->critter_at<npc>( pos() ) == this;
}

int npc::follow_distance() const
{
    // If the player is standing on stairs, follow closely
    // This makes the stair hack less painful to use
    if( is_friend() &&
        ( g->m.has_flag( TFLAG_GOES_DOWN, g->u.pos() ) ||
          g->m.has_flag( TFLAG_GOES_UP, g->u.pos() ) ) ) {
        return 1;
    }
    // @todo: Allow player to set that
    return 4;
}

nc_color npc::basic_symbol_color() const
{
    if( attitude == NPCATT_KILL ) {
        return c_red;
    } else if( attitude == NPCATT_FLEE ) {
        return c_red;
    } else if( is_friend() ) {
        return c_green;
    } else if( is_following() ) {
        return c_light_green;
    } else if( guaranteed_hostile() ) {
        return c_red;
    }
    return c_pink;
}

int npc::print_info( const catacurses::window &w, int line, int vLines, int column ) const
{
    const int last_line = line + vLines;
    const unsigned int iWidth = getmaxx( w ) - 2;
    // First line of w is the border; the next 4 are terrain info, and after that
    // is a blank line. w is 13 characters tall, and we can't use the last one
    // because it's a border as well; so we have lines 6 through 11.
    // w is also 48 characters wide - 2 characters for border = 46 characters for us
    mvwprintz( w, line++, column, c_white, _( "NPC: %s" ), name.c_str() );
    if( is_armed() ) {
        trim_and_print( w, line++, column, iWidth, c_red, _( "Wielding a %s" ), weapon.tname().c_str() );
    }

    const auto enumerate_print = [ w, last_line, column, iWidth, &line ]( std::string & str_in,
    nc_color color ) {
        // @todo: Replace with 'fold_and_print()'. Extend it with a 'height' argument to prevent leaking.
        size_t split;
        do {
            split = ( str_in.length() <= iWidth ) ? std::string::npos : str_in.find_last_of( ' ',
                    long( iWidth ) );
            if( split == std::string::npos ) {
                mvwprintz( w, line, column, color, str_in.c_str() );
            } else {
                mvwprintz( w, line, column, color, str_in.substr( 0, split ).c_str() );
            }
            str_in = str_in.substr( split + 1 );
            line++;
        } while( split != std::string::npos && line <= last_line );
    };

    const std::string worn_str = enumerate_as_string( worn.begin(), worn.end(), []( const item & it ) {
        return it.tname();
    } );
    if( !worn_str.empty() ) {
        std::string wearing = _( "Wearing: " ) + remove_color_tags( worn_str );
        enumerate_print( wearing, c_blue );
    }

    // @todo: Balance this formula
    const int visibility_cap = g->u.get_per() - rl_dist( g->u.pos(), pos() );

    const auto trait_str = visible_mutations( visibility_cap );
    if( !trait_str.empty() ) {
        std::string mutations = _( "Traits: " ) + remove_color_tags( trait_str );
        enumerate_print( mutations, c_green );
    }

    return line;
}

std::string npc::opinion_text() const
{
    std::stringstream ret;
    if( op_of_u.trust <= -10 ) {
        ret << _( "Completely untrusting" );
    } else if( op_of_u.trust <= -6 ) {
        ret << _( "Very untrusting" );
    } else if( op_of_u.trust <= -3 ) {
        ret << _( "Untrusting" );
    } else if( op_of_u.trust <= 2 ) {
        ret << _( "Uneasy" );
    } else if( op_of_u.trust <= 4 ) {
        ret << _( "Trusting" );
    } else if( op_of_u.trust < 10 ) {
        ret << _( "Very trusting" );
    } else {
        ret << _( "Completely trusting" );
    }

    ret << " (" << _( "Trust: " ) << op_of_u.trust << "); ";

    if( op_of_u.fear <= -10 ) {
        ret << _( "Thinks you're laughably harmless" );
    } else if( op_of_u.fear <= -6 ) {
        ret << _( "Thinks you're harmless" );
    } else if( op_of_u.fear <= -3 ) {
        ret << _( "Unafraid" );
    } else if( op_of_u.fear <= 2 ) {
        ret << _( "Wary" );
    } else if( op_of_u.fear <= 5 ) {
        ret << _( "Afraid" );
    } else if( op_of_u.fear < 10 ) {
        ret << _( "Very afraid" );
    } else {
        ret << _( "Terrified" );
    }

    ret << " (" << _( "Fear: " ) << op_of_u.fear << "); ";

    if( op_of_u.value <= -10 ) {
        ret << _( "Considers you a major liability" );
    } else if( op_of_u.value <= -6 ) {
        ret << _( "Considers you a burden" );
    } else if( op_of_u.value <= -3 ) {
        ret << _( "Considers you an annoyance" );
    } else if( op_of_u.value <= 2 ) {
        ret << _( "Doesn't care about you" );
    } else if( op_of_u.value <= 5 ) {
        ret << _( "Values your presence" );
    } else if( op_of_u.value < 10 ) {
        ret << _( "Treasures you" );
    } else {
        ret << _( "Best Friends Forever!" );
    }

    ret << " (" << _( "Value: " ) << op_of_u.value << "); ";

    if( op_of_u.anger <= -10 ) {
        ret << _( "You can do no wrong!" );
    } else if( op_of_u.anger <= -6 ) {
        ret << _( "You're good people" );
    } else if( op_of_u.anger <= -3 ) {
        ret << _( "Thinks well of you" );
    } else if( op_of_u.anger <= 2 ) {
        ret << _( "Ambivalent" );
    } else if( op_of_u.anger <= 5 ) {
        ret << _( "Pissed off" );
    } else if( op_of_u.anger < 10 ) {
        ret << _( "Angry" );
    } else {
        ret << _( "About to kill you" );
    }

    ret << " (" << _( "Anger: " ) << op_of_u.anger << ")";

    return ret.str();
}

void npc::setpos( const tripoint &pos )
{
    position = pos;
    const point pos_om_old = sm_to_om_copy( submap_coords );
    submap_coords.x = g->get_levx() + pos.x / SEEX;
    submap_coords.y = g->get_levy() + pos.y / SEEY;
    const point pos_om_new = sm_to_om_copy( submap_coords );
    if( !is_fake() && pos_om_old != pos_om_new ) {
        overmap &om_old = overmap_buffer.get( pos_om_old.x, pos_om_old.y );
        overmap &om_new = overmap_buffer.get( pos_om_new.x, pos_om_new.y );
        if( const auto ptr = om_old.erase_npc( getID() ) ) {
            om_new.insert_npc( ptr );
        } else {
            // Don't move the npc pointer around to avoid having two overmaps
            // with the same npc pointer
            debugmsg( "could not find npc %s on its old overmap", name.c_str() );
        }
    }
}

void maybe_shift( cata::optional<tripoint> &pos, int dx, int dy )
{
    if( pos ) {
        pos->x += dx;
        pos->y += dy;
    }
}

void maybe_shift( tripoint &pos, int dx, int dy )
{
    if( pos != tripoint_min ) {
        pos.x += dx;
        pos.y += dy;
    }
}

void npc::shift( int sx, int sy )
{
    const int shiftx = sx * SEEX;
    const int shifty = sy * SEEY;

    setpos( pos() - point( shiftx, shifty ) );

    maybe_shift( wanted_item_pos, -shiftx, -shifty );
    maybe_shift( last_player_seen_pos, -shiftx, -shifty );
    maybe_shift( pulp_location, -shiftx, -shifty );
    path.clear();
}

bool npc::is_dead() const
{
    return dead || is_dead_state();
}

void npc::die( Creature *nkiller )
{
    if( dead ) {
        // We are already dead, don't die again, note that npc::dead is
        // *only* set to true in this function!
        return;
    }
    // Need to unboard from vehicle before dying, otherwise
    // the vehicle code cannot find us
    if( in_vehicle ) {
        g->m.unboard_vehicle( pos(), true );
    }

    dead = true;
    Character::die( nkiller );

    if( g->u.sees( *this ) ) {
        add_msg( _( "%s dies!" ), name.c_str() );
    }

    if( killer == &g->u && ( !guaranteed_hostile() || hit_by_player ) ) {
        g->record_npc_kill( *this );
        bool cannibal = g->u.has_trait( trait_CANNIBAL );
        bool psycho = g->u.has_trait( trait_PSYCHOPATH );
        if( g->u.has_trait( trait_SAPIOVORE ) ) {
            g->u.add_memorial_log( pgettext( "memorial_male",
                                             "Caught and killed an ape.  Prey doesn't have a name." ),
                                   pgettext( "memorial_female", "Caught and killed an ape.  Prey doesn't have a name." ) );
        } else if( psycho && cannibal ) {
            g->u.add_memorial_log( pgettext( "memorial_male",
                                             "Killed a delicious-looking innocent, %s, in cold blood." ),
                                   pgettext( "memorial_female", "Killed a delicious-looking innocent, %s, in cold blood." ),
                                   name.c_str() );
        } else if( psycho ) {
            g->u.add_memorial_log( pgettext( "memorial_male",
                                             "Killed an innocent, %s, in cold blood.  They were weak." ),
                                   pgettext( "memorial_female", "Killed an innocent, %s, in cold blood.  They were weak." ),
                                   name.c_str() );
        } else if( cannibal ) {
            g->u.add_memorial_log( pgettext( "memorial_male", "Killed an innocent, %s." ),
                                   pgettext( "memorial_female", "Killed an innocent, %s." ),
                                   name.c_str() );
            g->u.add_morale( MORALE_KILLED_INNOCENT, -5, 0, 2_days, 3_hours );
        } else {
            g->u.add_memorial_log( pgettext( "memorial_male",
                                             "Killed an innocent person, %s, in cold blood and felt terrible afterwards." ),
                                   pgettext( "memorial_female",
                                             "Killed an innocent person, %s, in cold blood and felt terrible afterwards." ),
                                   name.c_str() );
            g->u.add_morale( MORALE_KILLED_INNOCENT, -100, 0, 2_days, 3_hours );
        }
    }

    place_corpse();
}

std::string npc_attitude_name( npc_attitude att )
{
    switch( att ) {
        case NPCATT_NULL:          // Don't care/ignoring player
            return _( "Ignoring" );
        case NPCATT_TALK:          // Move to and talk to player
            return _( "Wants to talk" );
        case NPCATT_FOLLOW:        // Follow the player
            return _( "Following" );
        case NPCATT_LEAD:          // Lead the player, wait for them if they're behind
            return _( "Leading" );
        case NPCATT_WAIT:          // Waiting for the player
            return _( "Waiting for you" );
        case NPCATT_MUG:           // Mug the player
            return _( "Mugging you" );
        case NPCATT_WAIT_FOR_LEAVE:// Attack the player if our patience runs out
            return _( "Waiting for you to leave" );
        case NPCATT_KILL:          // Kill the player
            return _( "Attacking to kill" );
        case NPCATT_FLEE:          // Get away from the player
            return _( "Fleeing" );
        case NPCATT_HEAL:          // Get to the player and heal them
            return _( "Healing you" );
        default:
            break;
    }

    debugmsg( "Invalid attitude: %d", att );
    return _( "Unknown attitude" );
}

void npc::setID( int i )
{
    this->player::setID( i );
}

//message related stuff

//message related stuff
void npc::add_msg_if_npc( const std::string &msg ) const
{
    add_msg( replace_with_npc_name( msg ) );
}

void npc::add_msg_player_or_npc( const std::string &/*player_msg*/,
                                 const std::string &npc_msg ) const
{
    if( g->u.sees( *this ) ) {
        add_msg( replace_with_npc_name( npc_msg ) );
    }
}

void npc::add_msg_if_npc( const game_message_type type, const std::string &msg ) const
{
    add_msg( type, replace_with_npc_name( msg ) );
}

void npc::add_msg_player_or_npc( const game_message_type type, const std::string &/*player_msg*/,
                                 const std::string &npc_msg ) const
{
    if( g->u.sees( *this ) ) {
        add_msg( type, replace_with_npc_name( npc_msg ) );
    }
}

void npc::add_msg_player_or_say( const std::string &/*player_msg*/,
                                 const std::string &npc_speech ) const
{
    say( npc_speech );
}

void npc::add_msg_player_or_say( const game_message_type /*type*/,
                                 const std::string &/*player_msg*/, const std::string &npc_speech ) const
{
    say( npc_speech );
}

void npc::add_new_mission( class mission *miss )
{
    chatbin.add_new_mission( miss );
}

void npc::on_unload()
{
    last_updated = calendar::turn;
}

void npc::on_load()
{
    // Cap at some reasonable number, say 2 days
    const time_duration dt = std::min( calendar::turn - last_updated, 2_days );
    // TODO: Sleeping, healing etc.
    last_updated = calendar::turn;
    time_point cur = calendar::turn - dt;
    add_msg( m_debug, "on_load() by %s, %d turns", name, to_turns<int>( dt ) );
    // First update with 30 minute granularity, then 5 minutes, then turns
    for( ; cur < calendar::turn - 30_minutes; cur += 30_minutes + 1_turns ) {
        update_body( cur, cur + 30_minutes );
    }
    for( ; cur < calendar::turn - 5_minutes; cur += 5_minutes + 1_turns ) {
        update_body( cur, cur + 5_minutes );
    }
    for( ; cur < calendar::turn; cur += 1_turns ) {
        update_body( cur, cur + 1_turns );
    }

    if( dt > 0_turns ) {
        // This ensures food is properly rotten at load
        // Otherwise NPCs try to eat rotten food and fail
        process_active_items();
    }

    // Not necessarily true, but it's not a bad idea to set this
    has_new_items = true;

    // for spawned npcs
    if( g->m.has_flag( "UNSTABLE", pos() ) ) {
        add_effect( effect_bouldering, 1_turns, num_bp, true );
    } else if( has_effect( effect_bouldering ) ) {
        remove_effect( effect_bouldering );
    }
    if( g->m.veh_at( pos() ).part_with_feature( VPFLAG_BOARDABLE, true ) && !in_vehicle ) {
        g->m.board_vehicle( pos(), this );
    }
}

void npc_chatbin::add_new_mission( mission *miss )
{
    if( miss == nullptr ) {
        return;
    }
    missions.push_back( miss );
}

epilogue::epilogue()
{
    id = "NONE";
    group = "NONE";
    text = "Error: file lost!";
}

epilogue_map epilogue::_all_epilogue;

void epilogue::load_epilogue( JsonObject &jsobj )
{
    epilogue base;
    base.id = jsobj.get_string( "id" );
    base.group = jsobj.get_string( "group" );
    base.text = jsobj.get_string( "text" );

    _all_epilogue[base.id] = base;
}

epilogue *epilogue::find_epilogue( const std::string &ident )
{
    epilogue_map::iterator found = _all_epilogue.find( ident );
    if( found != _all_epilogue.end() ) {
        return &( found->second );
    } else {
        debugmsg( "Tried to get invalid epilogue template: %s", ident.c_str() );
        static epilogue null_epilogue;
        return &null_epilogue;
    }
}

void epilogue::random_by_group( std::string group )
{
    std::vector<epilogue> v;
    for( auto epi : _all_epilogue ) {
        if( epi.second.group == group ) {
            v.push_back( epi.second );
        }
    }
    if( v.empty() ) {
        return;
    }
    epilogue epi = random_entry( v );
    id = epi.id;
    group = epi.group;
    text = epi.text;
}

constexpr tripoint npc::no_goal_point;

bool npc::query_yn( const std::string &/*msg*/ ) const
{
    // NPCs don't like queries - most of them are in the form of "Do you want to get hurt?".
    return false;
}

float npc::speed_rating() const
{
    float ret = get_speed() / 100.0f;
    ret *= 100.0f / run_cost( 100, false );

    return ret;
}

bool npc::dispose_item( item_location &&obj, const std::string & )
{
    using dispose_option = struct {
        int moves;
        std::function<void()> action;
    };

    std::vector<dispose_option> opts;

    for( auto &e : worn ) {
        if( e.can_holster( *obj ) ) {
            auto ptr = dynamic_cast<const holster_actor *>( e.type->get_use( "holster" )->get_actor_ptr() );
            opts.emplace_back( dispose_option {
                item_store_cost( *obj, e, false, ptr->draw_cost ),
                [this, ptr, &e, &obj]{ ptr->store( *this, e, *obj ); }
            } );
        }
    }

    if( volume_carried() + obj->volume() <= volume_capacity() ) {
        opts.emplace_back( dispose_option {
            item_handling_cost( *obj ),
            [this, &obj] {
                moves -= item_handling_cost( *obj );
                inv.add_item_keep_invlet( *obj );
                obj.remove_item();
                inv.unsort();
            }
        } );
    }

    if( opts.empty() ) {
        // Drop it
        g->m.add_item_or_charges( pos(), *obj );
        obj.remove_item();
        return true;
    }

    const auto mn = std::min_element( opts.begin(), opts.end(),
    []( const dispose_option & lop, const dispose_option & rop ) {
        return lop.moves < rop.moves;
    } );

    mn->action();
    return true;
}

void npc::process_turn()
{
    player::process_turn();

    if( is_following() && calendar::once_every( 1_hours ) &&
        get_hunger() < 200 && get_thirst() < 100 && op_of_u.trust < 5 ) {
        // Friends who are well fed will like you more
        // 24 checks per day, best case chance at trust 0 is 1 in 48 for +1 trust per 2 days
        float trust_chance = 5 - op_of_u.trust;
        // Penalize for bad impression
        // TODO: Penalize for traits and actions (especially murder, unless NPC is psycho)
        int op_penalty = std::max( 0, op_of_u.anger ) +
                         std::max( 0, -op_of_u.value ) +
                         std::max( 0, op_of_u.fear );
        // Being barely hungry and thirsty, not in pain and not wounded means good care
        int state_penalty = get_hunger() + get_thirst() + ( 100 - hp_percentage() ) + get_pain();
        if( x_in_y( trust_chance, 240 + 10 * op_penalty + state_penalty ) ) {
            op_of_u.trust++;
        }

        // TODO: Similar checks for fear and anger
    }

    last_updated = calendar::turn;
    // TODO: Add decreasing trust/value/etc. here when player doesn't provide food
    // TODO: Make NPCs leave the player if there's a path out of map and player is sleeping/unseen/etc.
}

std::array<std::pair<std::string, overmap_location_str_id>, npc_need::num_needs> npc::need_data = {
    {
        { "need_none", overmap_location_str_id( "source_of_anything" ) },
        { "need_ammo", overmap_location_str_id( "source_of_ammo" )},
        { "need_weapon", overmap_location_str_id( "source_of_weapon" )},
        { "need_gun", overmap_location_str_id( "source_of_gun" ) },
        { "need_food", overmap_location_str_id( "source_of_food" )},
        { "need_drink", overmap_location_str_id( "source_of_drink" ) }
    }
};

std::string npc::get_need_str_id( const npc_need &need )
{
    return need_data[static_cast<size_t>( need )].first;
}

overmap_location_str_id npc::get_location_for( const npc_need &need )
{
    return need_data[static_cast<size_t>( need )].second;
}

std::ostream &operator<< ( std::ostream &os, const npc_need &need )
{
    return os << npc::get_need_str_id( need );
}

bool npc::will_accept_from_player( const item &it ) const
{
    if( is_minion() || g->u.has_trait( trait_id( "DEBUG_MIND_CONTROL" ) ) ||
        it.has_flag( "NPC_SAFE" ) ) {
        return true;
    }

    if( !it.type->use_methods.empty() ) {
        return false;
    }

    if( const auto &comest = it.is_container() ? it.get_contained().type->comestible :
                             it.type->comestible ) {
        if( comest->fun < 0 || it.poison > 0 ) {
            return false;
        }
    }

    return true;
}

const pathfinding_settings &npc::get_pathfinding_settings() const
{
    return get_pathfinding_settings( false );
}

const pathfinding_settings &npc::get_pathfinding_settings( bool no_bashing ) const
{
    path_settings->bash_strength = no_bashing ? 0 : smash_ability();
    // @todo: Extract climb skill
    const int climb = std::min( 20, get_dex() );
    if( climb > 1 ) {
        // Success is !one_in(dex), so 0%, 50%, 66%, 75%...
        // Penalty for failure chance is 1/success = 1/(1-failure) = 1/(1-(1/dex)) = dex/(dex-1)
        path_settings->climb_cost = ( 10 - climb / 5 ) * climb / ( climb - 1 );
    } else {
        // Climbing at this dexterity will always fail
        path_settings->climb_cost = 0;
    }

    return *path_settings;
}

std::set<tripoint> npc::get_path_avoid() const
{
    std::set<tripoint> ret;
    for( Creature &critter : g->all_creatures() ) {
        // @todo: Cache this somewhere
        ret.insert( critter.pos() );
    }
    return ret;
}

mfaction_id npc::get_monster_faction() const
{
    // Those can't be static int_ids, because mods add factions
    static const string_id<monfaction> human_fac( "human" );
    static const string_id<monfaction> player_fac( "player" );
    static const string_id<monfaction> bee_fac( "bee" );

    if( is_friend() ) {
        return player_fac.id();
    }

    if( has_trait( trait_id( "BEE" ) ) ) {
        return bee_fac.id();
    }

    return human_fac.id();
}

std::string npc::extended_description() const
{
    std::ostringstream ss;
    // For some reason setting it using str or constructor doesn't work
    ss << Character::extended_description();

    ss << std::endl << "--" << std::endl;
    if( attitude == NPCATT_KILL ) {
        ss << _( "Is trying to kill you." );
    } else if( attitude == NPCATT_FLEE ) {
        ss << _( "Is trying to flee from you." );
    } else if( is_friend() ) {
        ss << _( "Is your friend." );
    } else if( is_following() ) {
        ss << _( "Is following you." );
    } else if( guaranteed_hostile() ) {
        ss << _( "Will try to kill you or flee from you if you reveal yourself." );
    } else {
        ss << _( "Is neutral." );
    }

    if( hit_by_player ) {
        ss << "--" << std::endl;
        ss << _( "Is still innocent and killing them will be considered murder." );
        // @todo: "But you don't care because you're an edgy psycho"
    }

    return replace_colors( ss.str() );
}

void npc::set_companion_mission( npc &p, const std::string &id )
{
    const point omt_pos = ms_to_omt_copy( g->m.getabs( p.posx(), p.posy() ) );
    comp_mission.position = tripoint( omt_pos.x, omt_pos.y, p.posz() );
    comp_mission.mission_id =  id;
    comp_mission.role_id = p.companion_mission_role_id;
}

void npc::reset_companion_mission()
{
    comp_mission.position = tripoint( -999, -999, -999 );
    comp_mission.mission_id.clear();
    comp_mission.role_id.clear();
}

bool npc::has_companion_mission() const
{
    return !comp_mission.mission_id.empty();
}

npc_companion_mission npc::get_companion_mission() const
{
    return comp_mission;
}

attitude_group get_attitude_group( npc_attitude att )
{
    switch( att ) {
        case NPCATT_MUG:
        case NPCATT_WAIT_FOR_LEAVE:
        case NPCATT_KILL:
            return attitude_group::hostile;
        case NPCATT_FLEE:
            return attitude_group::fearful;
        case NPCATT_FOLLOW:
        case NPCATT_LEAD:
            return attitude_group::friendly;
        default:
            break;
    }
    return attitude_group::neutral;
}

npc_attitude npc::get_attitude() const
{
    return attitude;
}

void npc::set_attitude( npc_attitude new_attitude )
{
    if( new_attitude == attitude ) {
        return;
    }
    add_msg( m_debug, "%s changes attitude from %s to %s",
             name.c_str(), npc_attitude_name( attitude ).c_str(), npc_attitude_name( new_attitude ).c_str() );
    attitude_group new_group = get_attitude_group( new_attitude );
    attitude_group old_group = get_attitude_group( attitude );
    if( new_group != old_group && !is_fake() ) {
        switch( new_group ) {
            case attitude_group::hostile:
                add_msg_if_npc( m_bad, _( "<npcname> gets angry!" ) );
                break;
            case attitude_group::fearful:
                add_msg_if_npc( m_warning, _( "<npcname> gets scared!" ) );
                break;
            default:
                if( old_group == attitude_group::hostile ) {
                    add_msg_if_npc( m_good, _( "<npcname> calms down." ) );
                } else if( old_group == attitude_group::fearful ) {
                    add_msg_if_npc( _( "<npcname> is no longer afraid." ) );
                }
                break;
        }
    }
    attitude = new_attitude;
}

npc_follower_rules::npc_follower_rules()
{
    engagement = ENGAGE_CLOSE;
    aim = AIM_WHEN_CONVENIENT;
    set_flag( ally_rule::use_guns );
    set_flag( ally_rule::use_grenades );
    clear_flag( ally_rule::use_silent );
    set_flag( ally_rule::avoid_friendly_fire );

    clear_flag( ally_rule::allow_pick_up );
    clear_flag( ally_rule::allow_bash );
    clear_flag( ally_rule::allow_sleep );
    set_flag( ally_rule::allow_complain );
    set_flag( ally_rule::allow_pulp );
    clear_flag( ally_rule::close_doors );
}

bool npc_follower_rules::has_flag( ally_rule test ) const
{
    return static_cast<int>( test ) & static_cast<int>( flags );
}

void npc_follower_rules::set_flag( ally_rule setit )
{
    flags = static_cast<ally_rule>( static_cast<int>( flags ) | static_cast<int>( setit ) );
}

void npc_follower_rules::clear_flag( ally_rule clearit )
{
    flags = static_cast<ally_rule>( static_cast<int>( flags ) & ~static_cast<int>( clearit ) );
}

void npc_follower_rules::toggle_flag( ally_rule toggle )
{
    if( has_flag( toggle ) ) {
        clear_flag( toggle );
    } else {
        set_flag( toggle );
    }
}
