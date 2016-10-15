#include "npc.h"

#include "auto_pickup.h"
#include "coordinate_conversions.h"
#include "rng.h"
#include "map.h"
#include "game.h"
#include "debug.h"
#include "bodypart.h"
#include "skill.h"
#include "output.h"
#include "line.h"
#include "item_group.h"
#include "translations.h"
#include "overmapbuffer.h"
#include "messages.h"
#include "mission.h"
#include "npc_class.h"
#include "json.h"
#include "sounds.h"
#include "morale_types.h"
#include "overmap.h"
#include "vehicle.h"
#include "mtype.h"
#include "iuse_actor.h"

#include <algorithm>
#include <sstream>
#include <string>

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

std::list<item> starting_clothes( const npc_class_id &type, bool male );
std::list<item> starting_inv( npc *me, const npc_class_id &type );

npc::npc()
{
    mapx = 0;
    mapy = 0;
    position.x = -1;
    position.y = -1;
    position.z = 500;
    last_player_seen_pos = no_goal_point;
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
    my_fac = NULL;
    fac_id = "";
    miss_id = NULL_ID;
    marked_for_death = false;
    dead = false;
    hit_by_player = false;
    moves = 100;
    mission = NPC_MISSION_NULL;
    myclass = NULL_ID;
    patience = 0;
    restock = -1;
    companion_mission = "";
    companion_mission_time = 0;
    // ret_null is a bit more than just a regular "null", it is the "fist" for unarmed attacks
    ret_null = item( "null", 0 );
    last_updated = calendar::turn;

    path_settings = pathfinding_settings( 0, 1000, 1000, true, false, true );
}

standard_npc::standard_npc( const std::string &name, const std::vector<itype_id> &clothing,
                            int sk_lvl, int s_str, int s_dex, int s_int, int s_per )
{
    this->name = name;
    position = { 0, 0, 0 };

    str_cur = std::max( s_str, 0 );
    str_max = std::max( s_str, 0 );
    dex_cur = std::max( s_dex, 0 );
    dex_max = std::max( s_dex, 0 );
    per_cur = std::max( s_per, 0 );
    per_max = std::max( s_per, 0 );
    int_cur = std::max( s_int, 0 );
    int_max = std::max( s_int, 0 );

    for( auto &e: Skill::skills ) {
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

npc::npc(const npc &) = default;
npc::npc(npc &&) = default;
npc &npc::operator=(const npc &) = default;
npc &npc::operator=(npc &&) = default;

npc_map npc::_all_npc;

void npc::load_npc(JsonObject &jsobj)
{
    npc guy;
    guy.idz = jsobj.get_string("id");
    if (jsobj.has_string("name+"))
        guy.name = jsobj.get_string("name+");
    if (jsobj.has_string("gender")){
        if (jsobj.get_string("gender") == "male"){
            guy.male = true;
        }else{
            guy.male = false;
        }
    }
    if (jsobj.has_string("faction"))
        guy.fac_id = jsobj.get_string("faction");

    if( jsobj.has_int( "class" ) ) {
        guy.myclass = npc_class::from_legacy_int( jsobj.get_int("class") );
    } else if( jsobj.has_string( "class" ) ) {
        guy.myclass = npc_class_id( jsobj.get_string("class") );
        if( !guy.myclass.is_valid() ) {
            debugmsg( "Invalid NPC class %s", guy.myclass.c_str() );
            guy.myclass = NULL_ID;
        }
    }

    guy.attitude = npc_attitude(jsobj.get_int("attitude"));
    guy.mission = npc_mission(jsobj.get_int("mission"));
    guy.chatbin.first_topic = jsobj.get_string( "chat" );
    if( jsobj.has_string( "mission_offered" ) ){
        guy.miss_id = mission_type_id( jsobj.get_string( "mission_offered" ) );
    } else {
        guy.miss_id = NULL_ID;
    }
    _all_npc[guy.idz] = std::move( guy );
}

npc* npc::find_npc(std::string ident)
{
    npc_map::iterator found = _all_npc.find(ident);
    if (found != _all_npc.end()){
        return &(found->second);
    } else {
        debugmsg("Tried to get invalid npc template: %s", ident.c_str());
        static npc null_npc;
    return &null_npc;
    }
}

void npc::load_npc_template(std::string ident)
{
    npc_map::iterator found = _all_npc.find(ident);
    if (found != _all_npc.end()){
        idz = found->second.idz;
        myclass = npc_class_id( found->second.myclass );
        randomize(myclass);
        std::string tmpname = found->second.name.c_str();
        if (tmpname[0] == ','){
            name = name + found->second.name;
        } else {
            name = found->second.name;
            //Assume if the name is unique, the gender might also be.
            male = found->second.male;
        }
        fac_id = found->second.fac_id;
        set_fac(fac_id);
        attitude = found->second.attitude;
        mission = found->second.mission;
        chatbin.first_topic = found->second.chatbin.first_topic;
        if( !found->second.miss_id.is_null() ){
            add_new_mission( mission::reserve_new( found->second.miss_id, getID() ) );
        }
        return;
    } else {
        debugmsg("Tried to get invalid npc: %s", ident.c_str());
        return;
    }
}

npc::~npc() { }

std::string npc::save_info() const
{
    return serialize(); // also saves contents
}

void npc::load_info(std::string data)
{
    std::stringstream dump;
    dump << data;

    JsonIn jsin(dump);
    try {
        deserialize(jsin);
    } catch( const JsonError &jsonerr ) {
        debugmsg("Bad npc json\n%s", jsonerr.c_str() );
    }
    if( fac_id != "" ) {
        set_fac(fac_id);
    }
}

void npc::randomize( const npc_class_id &type )
{
    if( getID() <= 0 ) {
        setID( g->assign_npc_id() );
    }

    ret_null = item("null", 0);
    weapon   = item("null", 0);
    inv.clear();
    personality.aggression = rng(-10, 10);
    personality.bravery =    rng( -3, 10);
    personality.collector =  rng( -1, 10);
    personality.altruism =   rng(-10, 10);
    moves = 100;
    mission = NPC_MISSION_NULL;
    male = one_in( 2 );
    pick_name();

    if( !type.is_valid() ) {
        debugmsg( "Invalid NPC class %s", type.c_str() );
        myclass = NULL_ID;
    } else if( type.is_null() && !one_in( 5 ) ) {
        npc_class_id typetmp;
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
        restock = DAYS( 3 );
        cash += 100000;
    }

    for( auto &skill : Skill::skills ) {
        int level = myclass->roll_skill( skill.ident() );

        set_skill_level( skill.ident(), level );
    }

    if( type.is_null() ) { // Untyped; no particular specialization
    } else if( type == NC_EVAC_SHOPKEEP ) {
  personality.collector += rng(1, 5);

 } else if( type == NC_BARTENDER ) {
  personality.collector += rng(1, 5);

 } else if( type == NC_JUNK_SHOPKEEP ) {
  personality.collector += rng(1, 5);

 } else if( type == NC_ARSONIST ) {
  personality.aggression += rng(0, 1);
  personality.collector += rng(0, 2);

 } else if( type == NC_SOLDIER ) {
  personality.aggression += rng(1, 3);
  personality.bravery += rng(0, 5);

 } else if( type == NC_HACKER ) {
  personality.bravery -= rng(1, 3);
  personality.aggression -= rng(0, 2);

 } else if( type == NC_DOCTOR ) {
  personality.aggression -= rng(0, 4);
  cash += 10000 * rng(0, 3) * rng(0, 3);

 } else if( type == NC_TRADER ) {
  personality.collector += rng(1, 5);
  cash += 25000 * rng(1, 10);

 } else if( type == NC_NINJA ) {
  personality.bravery += rng(0, 3);
  personality.collector -= rng(1, 6);
  // TODO: give ninja his styles back

 } else if( type == NC_COWBOY ) {
  personality.aggression += rng(0, 2);
  personality.bravery += rng(1, 5);

 } else if( type == NC_SCIENTIST ) {
  personality.aggression -= rng(1, 5);
  personality.bravery -= rng(2, 8);
  personality.collector += rng (0, 2);

 } else if( type == NC_BOUNTY_HUNTER ) {
  personality.aggression += rng(1, 6);
  personality.bravery += rng(0, 5);

 } else if( type == NC_THUG ) {
  personality.aggression += rng(1, 6);
  personality.bravery += rng(0, 5);

 } else if( type == NC_SCAVENGER ) {
  personality.aggression += rng(1, 3);
  personality.bravery += rng(1, 4);


 }
  //A universal barter boost to keep NPCs competitive with players
 //The int boost from trade wasn't active... now that it is, most
 //players will vastly outclass npcs in trade without a little help.
 boost_skill_level( skill_barter, rng(2, 4));

    recalc_hp();
    for( int i = 0; i < num_hp_parts; i++ ) {
        hp_cur[i] = hp_max[i];
    }

    starting_weapon(type);
    worn = starting_clothes(type, male);
    inv.clear();
    inv.add_stack(starting_inv(this, type));
    has_new_items = true;
}

void npc::randomize_from_faction(faction *fac)
{
// Personality = aggression, bravery, altruism, collector
 my_fac = fac;
 fac_id = fac->id;
    randomize( NULL_ID );

 switch (fac->goal) {
  case FACGOAL_DOMINANCE:
   personality.aggression += rng(0, 3);
   personality.bravery += rng(1, 4);
   personality.altruism -= rng(0, 2);
   break;
  case FACGOAL_CLEANSE:
   personality.aggression -= rng(0, 3);
   personality.bravery += rng(2, 4);
   personality.altruism += rng(0, 3);
   personality.collector -= rng(0, 2);
   break;
  case FACGOAL_SHADOW:
   personality.bravery += rng(4, 7);
   personality.collector -= rng(0, 3);
   int_max += rng(0, 2);
   per_max += rng(0, 2);
   break;
  case FACGOAL_APOCALYPSE:
   personality.aggression += rng(2, 5);
   personality.bravery += rng(4, 8);
   personality.altruism -= rng(1, 4);
   personality.collector -= rng(2, 5);
   break;
  case FACGOAL_ANARCHY:
   personality.aggression += rng(3, 6);
   personality.bravery += rng(0, 4);
   personality.altruism -= rng(3, 8);
   personality.collector -= rng(3, 6);
   int_max -= rng(1, 3);
   per_max -= rng(0, 2);
   str_max += rng(0, 3);
   break;
  case FACGOAL_KNOWLEDGE:
   if (one_in(2))
    randomize( NC_SCIENTIST );
   personality.aggression -= rng(2, 5);
   personality.bravery -= rng(1, 4);
   personality.collector += rng(2, 4);
   int_max += rng(2, 5);
   str_max -= rng(1, 4);
   per_max -= rng(0, 2);
   dex_max -= rng(0, 2);
   break;
  case FACGOAL_NATURE:
   personality.aggression -= rng(0, 3);
   personality.collector -= rng(1, 4);
   break;
  case FACGOAL_CIVILIZATION:
   personality.aggression -= rng(2, 4);
   personality.altruism += rng(1, 5);
   personality.collector += rng(1, 5);
   break;
  default:
    //Suppress warnings
    break;
 }
// Jobs
 if (fac->has_job(FACJOB_EXTORTION)) {
  personality.aggression += rng(0, 3);
  personality.bravery -= rng(0, 2);
  personality.altruism -= rng(2, 6);
 }
 if (fac->has_job(FACJOB_INFORMATION)) {
  int_max += rng(0, 4);
  per_max += rng(0, 4);
  personality.aggression -= rng(0, 4);
  personality.collector += rng(1, 3);
 }
 if (fac->has_job(FACJOB_TRADE) || fac->has_job(FACJOB_CARAVANS)) {
  if (!one_in(3))
   randomize( NC_TRADER );
  personality.aggression -= rng(1, 5);
  personality.collector += rng(1, 4);
  personality.altruism -= rng(0, 3);
 }
 if (fac->has_job(FACJOB_SCAVENGE))
  personality.collector += rng(4, 8);
 if (fac->has_job(FACJOB_MERCENARIES)) {
  if (!one_in(3)) {
   switch (rng(1, 3)) {
    case 1: randomize( NC_NINJA );  break;
    case 2: randomize( NC_COWBOY );  break;
    case 3: randomize( NC_BOUNTY_HUNTER ); break;
   }
  }
  personality.aggression += rng(0, 2);
  personality.bravery += rng(2, 4);
  personality.altruism -= rng(2, 4);
  str_max += rng(0, 2);
  per_max += rng(0, 2);
  dex_max += rng(0, 2);
 }
 if (fac->has_job(FACJOB_ASSASSINS)) {
  personality.bravery -= rng(0, 2);
  personality.altruism -= rng(1, 3);
  per_max += rng(1, 3);
  dex_max += rng(0, 2);
 }
 if (fac->has_job(FACJOB_RAIDERS)) {
  if (one_in(3))
   randomize( NC_COWBOY );
  personality.aggression += rng(3, 5);
  personality.bravery += rng(0, 2);
  personality.altruism -= rng(3, 6);
  str_max += rng(0, 3);
  int_max -= rng(0, 2);
 }
 if (fac->has_job(FACJOB_THIEVES)) {
  if (one_in(3))
   randomize( NC_NINJA );
  personality.aggression -= rng(2, 5);
  personality.bravery -= rng(1, 3);
  personality.altruism -= rng(1, 4);
  str_max -= rng(0, 2);
  per_max += rng(1, 4);
  dex_max += rng(1, 3);
 }
 if (fac->has_job(FACJOB_DOCTORS)) {
  if (!one_in(4))
   randomize( NC_DOCTOR );
  personality.aggression -= rng(3, 6);
  personality.bravery += rng(0, 4);
  personality.altruism += rng(0, 4);
  int_max += rng(2, 4);
  per_max += rng(0, 2);
  boost_skill_level( skill_firstaid, rng(1, 5));
 }
 if (fac->has_job(FACJOB_FARMERS)) {
  personality.aggression -= rng(2, 4);
  personality.altruism += rng(0, 3);
  str_max += rng(1, 3);
 }
 if (fac->has_job(FACJOB_DRUGS)) {
  personality.aggression -= rng(0, 2);
  personality.bravery -= rng(0, 3);
  personality.altruism -= rng(1, 4);
 }
 if (fac->has_job(FACJOB_MANUFACTURE)) {
  personality.aggression -= rng(0, 2);
  personality.bravery -= rng(0, 2);
  switch (rng(1, 4)) {
   case 1: boost_skill_level( skill_mechanics, dice(2, 4));   break;
   case 2: boost_skill_level( skill_electronics, dice(2, 4)); break;
   case 3: boost_skill_level( skill_cooking, dice(2, 4));     break;
   case 4: boost_skill_level( skill_tailor, dice(2,  4));     break;
  }
 }

 if (fac->has_value(FACVAL_CHARITABLE)) {
  personality.aggression -= rng(2, 5);
  personality.bravery += rng(0, 4);
  personality.altruism += rng(2, 5);
 }
 if (fac->has_value(FACVAL_LONERS)) {
  personality.aggression -= rng(1, 3);
  personality.altruism -= rng(1, 4);
 }
 if (fac->has_value(FACVAL_EXPLORATION)) {
  per_max += rng(0, 4);
  personality.aggression -= rng(0, 2);
 }
 if (fac->has_value(FACVAL_ARTIFACTS)) {
  personality.collector += rng(2, 5);
  personality.altruism -= rng(0, 2);
 }
 if (fac->has_value(FACVAL_BIONICS)) {
  str_max += rng(0, 2);
  dex_max += rng(0, 2);
  per_max += rng(0, 2);
  int_max += rng(0, 4);
  if (one_in(3)) {
   boost_skill_level( skill_mechanics, dice(2, 3));
   boost_skill_level( skill_electronics, dice(2, 3));
   boost_skill_level( skill_firstaid, dice(2, 3));
  }
 }
 if (fac->has_value(FACVAL_BOOKS)) {
  str_max -= rng(0, 2);
  per_max -= rng(0, 3);
  int_max += rng(0, 4);
  personality.aggression -= rng(1, 4);
  personality.bravery -= rng(0, 3);
  personality.collector += rng(0, 3);
 }
 if (fac->has_value(FACVAL_TRAINING)) {
  str_max += rng(0, 3);
  dex_max += rng(0, 3);
  per_max += rng(0, 2);
  int_max += rng(0, 2);
  for( auto const &skill : Skill::skills ) {
   if (one_in(3))
       boost_skill_level( skill.ident(), rng( 2, 4 ) );
  }
 }
 if (fac->has_value(FACVAL_ROBOTS)) {
  int_max += rng(0, 3);
  personality.aggression -= rng(0, 3);
  personality.collector += rng(0, 3);
 }
 if (fac->has_value(FACVAL_TREACHERY)) {
  personality.aggression += rng(0, 3);
  personality.altruism -= rng(2, 5);
 }
 if (fac->has_value(FACVAL_STRAIGHTEDGE)) {
  personality.collector -= rng(0, 2);
  str_max += rng(0, 1);
  per_max += rng(0, 2);
  int_max += rng(0, 3);
 }
 if (fac->has_value(FACVAL_LAWFUL)) {
  personality.aggression -= rng(3, 7);
  personality.altruism += rng(1, 5);
  int_max += rng(0, 2);
 }
 if (fac->has_value(FACVAL_CRUELTY)) {
  personality.aggression += rng(3, 6);
  personality.bravery -= rng(1, 4);
  personality.altruism -= rng(2, 5);
 }
}

void npc::set_fac(std::string fac_name)
{
    my_fac = g->faction_by_ident(fac_name);
    if ( my_fac == nullptr ) {
        debugmsg("The game could not find the %s faction", fac_name.c_str());
    } else {
        fac_id = my_fac->id;
    }
}

// item id from group "<class-name>_<what>" or from fallback group
// may still be a null item!
item random_item_from( const npc_class_id &type, const std::string &what, const std::string &fallback )
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
    if( male ) {
        return random_item_from( type, what + "_male", "npc_" + what + "_male" );
    } else {
        return random_item_from( type, what + "_female", "npc_" + what + "_female" );
    }
}

std::list<item> starting_clothes( const npc_class_id &type, bool male )
{
    std::list<item> ret;

    item pants = get_clothing_item( type, "pants", male);
    item shirt = get_clothing_item( type, "shirt", male );
    item gloves = random_item_from( type, "gloves" );
    item coat = random_item_from( type, "coat" );
    item shoes = random_item_from( type, "shoes" );
    item mask = random_item_from( type, "masks" );
    // Why is the alternative group not named "npc_glasses" but "npc_eyes"?
    item glasses = random_item_from( type, "glasses", "npc_eyes" );
    item hat = random_item_from( type, "hat" );
    item extras = random_item_from( type, "extra" );

    // Fill in the standard things we wear
    ret.push_back( shoes );
    ret.push_back( pants );
    ret.push_back( shirt );
    ret.push_back( coat );
    ret.push_back( gloves );
    // Bad to wear a mask under a motorcycle helmet
    if( hat.typeId() != "helmet_motor" ) {
        ret.push_back( mask );
    }
    ret.push_back( glasses );
    ret.push_back( hat );
    ret.push_back( extras );

    // the player class and other code all over the place assume that the
    // worn vector contains *only* armor items. It will *crash* when there
    // is a non-armor item!
    // Also: the above might have added null-items that must be filtered out.
    for( auto it = ret.begin(); it != ret.end(); ) {
        if( !it->is_null() && it->is_armor() ) {
            if( !one_in( 3 ) && it->has_flag( "VARSIZE" ) ) {
                it->item_tags.insert( "FIT" );
            }
            ++it;
        } else {
            it = ret.erase( it );
        }
    }
 return ret;
}

std::list<item> starting_inv( npc *me, const npc_class_id &type )
{
    std::list<item> res;
    res.emplace_back( "lighter" );

    // If wielding a gun, get some additional ammo for it
    if( me->weapon.is_gun() ) {
        item ammo( default_ammo( me->weapon.ammo_type() ) );
        ammo = ammo.in_its_container();
        if( ammo.made_of( LIQUID ) ) {
            item container( "bottle_plastic" );
            container.put_in( ammo );
            ammo = container;
        }

        // @todo Move to npc_class
        int qty = 1 + ( type == NC_COWBOY ||
                        type == NC_BOUNTY_HUNTER );
        qty = rng( qty, qty * 2 );

        while ( qty-- != 0 && me->can_pickVolume( ammo ) ) {
            // @todo give NPC a default magazine instead
            res.push_back( ammo );
        }
    }

    if( type == NC_ARSONIST ) {
        res.emplace_back( "molotov" );
    }

    // NC_COWBOY and NC_BOUNTY_HUNTER get 5-15 whilst all others get 3-6
    int qty = ( type == NC_EVAC_SHOPKEEP ||
                type == NC_TRADER ) ? 5 : 2;
    qty = rng( qty, qty * 3 );

    while ( qty-- != 0 ) {
        item tmp = random_item_from( type, "misc" ).in_its_container();
        if( !tmp.is_null() ) {
            if( !one_in( 3 ) && tmp.has_flag( "VARSIZE" ) ) {
                tmp.item_tags.insert( "FIT" );
            }
            if( me->can_pickVolume( tmp ) ) {
                res.push_back( tmp );
            }
        }
    }

    res.erase( std::remove_if( res.begin(), res.end(), [&]( const item& e ) {
        return e.has_flag( "TRADER_AVOID" );
    } ), res.end() );

    return res;
}

void npc::spawn_at(int x, int y, int z)
{
    mapx = x;
    mapy = y;
    position.x = rng(0, SEEX - 1);
    position.y = rng(0, SEEY - 1);
    position.z = z;
    const point pos_om = sm_to_om_copy( mapx, mapy );
    overmap &om = overmap_buffer.get( pos_om.x, pos_om.y );
    om.add_npc( *this );
}

void npc::spawn_at_random_city(overmap *o)
{
    int x, y;
    if(o->cities.empty()) {
        x = rng(0, OMAPX * 2 - 1);
        y = rng(0, OMAPY * 2 - 1);
    } else {
        const city& c = random_entry( o->cities );
        x = c.x + rng(-c.s, +c.s);
        y = c.y + rng(-c.s, +c.s);
    }
    x += o->pos().x * OMAPX * 2;
    y += o->pos().y * OMAPY * 2;
    spawn_at(x, y, 0);
}

tripoint npc::global_square_location() const
{
    return tripoint( mapx * SEEX + posx(), mapy * SEEY + posy(), position.z );
}

void npc::place_on_map()
{
    // The global absolute position (in map squares) of the npc is *always*
    // "mapx * SEEX + posx()" (analog for y).
    // The main map assumes that pos[xy] is in its own (local to the main map)
    // coordinate system. We have to change pos[xy] to match that assumption,
    // but also have to change map[xy] to keep the global position of the npc
    // unchanged.
    const int dmx = mapx - g->get_levx();
    const int dmy = mapy - g->get_levy();
    mapx -= dmx; // == g->get_levx()
    mapy -= dmy;
    position.x += dmx * SEEX; // value of "mapx * SEEX + posx()" is unchanged
    position.y += dmy * SEEY;

    // Places the npc at the nearest empty spot near (posx(), posy()).
    // Searches in a spiral pattern for a suitable location.
    int x = 0, y = 0, dx = 0, dy = -1;
    int temp;
    while( !g->is_empty( { posx() + x, posy() + y, posz() } ) )
    {
        if ((x == y) || ((x < 0) && (x == -y)) || ((x > 0) && (x == 1-y)))
        {//change direction
            temp = dx;
            dx = -dy;
            dy = temp;
        }
        x += dx;
        y += dy;
    }//end search, posx() + x , posy() + y contains a free spot.
    //place the npc at the free spot.
    position.x += x;
    position.y += y;
}

skill_id npc::best_skill() const
{
    int highest_level = std::numeric_limits<int>::min();
    skill_id highest_skill( NULL_ID );

    for (auto const &p : _skills) {
        if (p.first.obj().is_combat_skill()) {
            int const level = p.second;
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
        weapon.ammo_set( default_ammo( weapon.type->gun->ammo ) );
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
    }};

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
        return wear_item( it, false );
    }

    const int it_encumber = it.get_encumber();
    while( !worn.empty() ) {
        bool encumb_ok = true;
        const auto new_enc = get_encumbrance( it );
        // Strip until we can put the new item on
        // This is one of the reasons this command is not used by the AI
        for( size_t i = 0; i < num_bp; i++ ) {
            const auto bp = static_cast<body_part>( i );
            if( !it.covers( bp ) ) {
                continue;
            }

            if( it_encumber > max_encumb[i] ) {
                // Not an NPC-friendly item
                return false;
            }

            if( new_enc[i].encumbrance > max_encumb[i] ) {
                encumb_ok = false;
                break;
            }
        }

        if( encumb_ok && can_wear( it, false ) ) {
            // @todo Hazmat/power armor makes this not work due to 1 boots/headgear limit
            return wear_item( it, false );
        }
        // Otherwise, maybe we should take off one or more items and replace them
        bool took_off = false;
        for( size_t j = 0; j < num_bp; j++ ) {
            const body_part bp = static_cast<body_part>( j );
            if( !it.covers( bp ) ) {
                continue;
            }
            // Find an item that covers the same body part as the new item
            auto iter = std::find_if( worn.begin(), worn.end(), [bp]( const item& armor ) {
                return armor.covers( bp );
            } );
            if( iter != worn.end() ) {
                took_off = takeoff( *iter );
                break;
            }
        }

        if( !took_off ) {
            // Shouldn't happen, but does
            return wear_item( it, false );
        }
    }

    return worn.empty() && wear_item( it, false );
}

bool npc::wield( item& it )
{
    if( is_armed() ) {
        if ( volume_carried() + weapon.volume() <= volume_capacity() ) {
            add_msg_if_npc( m_info, _( "<npcname> puts away the %s." ), weapon.tname().c_str() );
            i_add( remove_weapon() );
            moves -= 15;
        } else { // No room for weapon, so we drop it
            add_msg_if_npc( m_info, _( "<npcname> drops the %s." ), weapon.tname().c_str() );
            g->m.add_item_or_charges( pos(), remove_weapon() );
        }
    }

    if( it.is_null() ) {
        weapon = ret_null;
        return true;
    }

    moves -= 15;
    if( inv.has_item( it ) ) {
        weapon = inv.remove_item( &it );
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
        // @todo Make bows not guns
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
    } else if( u.str_max <= 5 ) {
        op_of_u.fear -= 1;
    } else if( u.str_max <= 3 ) {
        op_of_u.fear -= 3;
    }

    for( int i = 0; i < num_hp_parts; i++ ) {
        if( u.hp_cur[i] <= u.hp_max[i] / 2 ) {
            op_of_u.fear--;
        }
        if( hp_cur[i] <= hp_max[i] / 2 ) {
            op_of_u.fear++;
        }
    }

    if (u.has_trait("SAPIOVORE")) {
        op_of_u.fear += 10; // Sapiovores = Scary
    }

    if (u.has_trait("PRETTY")) {
        op_of_u.fear += 1;
    } else if (u.has_trait("BEAUTIFUL")) {
        op_of_u.fear += 2;
    } else if (u.has_trait("BEAUTIFUL2")) {
        op_of_u.fear += 3;
    } else if (u.has_trait("BEAUTIFUL3")) {
        op_of_u.fear += 4;
    } else if (u.has_trait("UGLY")) {
        op_of_u.fear -= 1;
    } else if (u.has_trait("DEFORMED")) {
        op_of_u.fear += 3;
    } else if (u.has_trait("DEFORMED2")) {
        op_of_u.fear += 6;
    } else if (u.has_trait("DEFORMED3")) {
        op_of_u.fear += 9;
    }

    if (u.has_trait("TERRIFYING")) {
        op_of_u.fear += 6;
    }

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

    // @todo More effects
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

    if (u.has_trait("PRETTY")) {
      op_of_u.trust += 1;
    } else if (u.has_trait("BEAUTIFUL")) {
        op_of_u.trust += 3;
    } else if (u.has_trait("BEAUTIFUL2")) {
        op_of_u.trust += 5;
    } else if (u.has_trait("BEAUTIFUL3")) {
        op_of_u.trust += 7;
    } else if (u.has_trait("UGLY")) {
        op_of_u.trust -= 1;
    } else if (u.has_trait("DEFORMED")) {
        op_of_u.trust -= 3;
    } else if (u.has_trait("DEFORMED2")) {
        op_of_u.trust -= 6;
    } else if (u.has_trait("DEFORMED3")) {
        op_of_u.trust -= 9;
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
        attitude = NPCATT_TALK;
    } else if( op_of_u.fear - 2 * personality.aggression - personality.bravery < -30 ) {
        attitude = NPCATT_KILL;
    } else if( my_fac != nullptr && my_fac->likes_u < -10 ) {
        attitude = NPCATT_KILL;
    } else {
        attitude = NPCATT_FLEE;
    }

    add_msg( m_debug, "%s formed an opinion of u: %s",
             name.c_str(), npc_attitude_name( attitude ).c_str() );
}

float npc::vehicle_danger(int radius) const
{
    const tripoint from( posx() - radius, posy() - radius, posz() );
    const tripoint to( posx() + radius, posy() + radius, posz() );
    VehicleList vehicles = g->m.get_vehicles( from, to );

 int danger = 0;

 // TODO: check for most dangerous vehicle?
 for(size_t i = 0; i < vehicles.size(); ++i)
  if (vehicles[i].v->velocity > 0)
  {
   float facing = vehicles[i].v->face.dir();

   int ax = vehicles[i].v->global_x();
   int ay = vehicles[i].v->global_y();
   int bx = ax + cos (facing * M_PI / 180.0) * radius;
   int by = ay + sin (facing * M_PI / 180.0) * radius;

   // fake size
   /* This will almost certainly give the wrong size/location on customized
    * vehicles. This should just count frames instead. Or actually find the
    * size. */
   vehicle_part last_part = vehicles[i].v->parts.back();
   int size = std::max(last_part.mount.x, last_part.mount.y);

   float normal = sqrt((float)((bx - ax) * (bx - ax) + (by - ay) * (by - ay)));
   int closest = abs((posx() - ax) * (by - ay) - (posy() - ay) * (bx - ax)) / normal;

   if (size > closest)
    danger = i;
  }

 return danger;
}

bool npc::turned_hostile() const
{
 return (op_of_u.anger >= hostile_anger_level());
}

int npc::hostile_anger_level() const
{
 return (20 + op_of_u.fear - personality.aggression);
}

void npc::make_angry()
{
    if( is_enemy() ) {
        return; // We're already angry!
    }

    add_msg( m_debug, "%s gets angry", name.c_str() );
    // Make associated faction, if any, angry at the player too.
    if( my_fac != nullptr ) {
        my_fac->likes_u = std::max( -50, my_fac->likes_u - 50 );
        my_fac->respects_u = std::max( -50, my_fac->respects_u - 50 );
    }
    if( op_of_u.fear > 10 + personality.aggression + personality.bravery ) {
        attitude = NPCATT_FLEE; // We don't want to take u on!
    } else {
        attitude = NPCATT_KILL; // Yeah, we think we could take you!
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
    for( auto const &skill : Skill::skills ) {
        const auto &id = skill.ident();
        if( p.get_skill_level( id ).level() < get_skill_level( id ).level() ) {
            ret.push_back( id );
        }
    }
    return ret;
}

std::vector<matype_id> npc::styles_offered_to( const player &p ) const
{
    std::vector<matype_id> ret;
    for( auto & i : ma_styles ) {
        if( !p.has_martialart( i ) ) {
            ret.push_back( i );
        }
    }
    return ret;
}

bool npc::fac_has_value(faction_value value) const
{
    if( my_fac == nullptr ) {
        return false;
    }

    return my_fac->has_value(value);
}

bool npc::fac_has_job(faction_job job) const
{
    if( my_fac == nullptr ) {
        return false;
    }

    return my_fac->has_job(job);
}


void npc::decide_needs()
{
    int needrank[num_needs];
    for( auto &elem : needrank ) {
        elem = 20;
    }
    if (weapon.is_gun()) {
        needrank[need_ammo] = 5 * get_ammo(weapon.type->gun->ammo).size();
    }

    needrank[need_weapon] = weapon_value( weapon );
    needrank[need_food] = 15 - get_hunger();
    needrank[need_drink] = 15 - get_thirst();
    invslice slice = inv.slice();
    for (auto &i : slice) {
        if( i->front().is_food( )) {
            needrank[ need_food ] += nutrition_for( i->front().type ) / 4;
            needrank[ need_drink ] += i->front().type->comestible->quench / 4;
        } else if( i->front().is_food_container() ) {
            needrank[ need_food ] += nutrition_for( i->front().contents.front().type ) / 4;
            needrank[ need_drink ] += i->front().contents.front().type->comestible->quench / 4;
        }
    }
    needs.clear();
    size_t j;
    bool serious = false;
    for (int i = 1; i < num_needs; i++) {
        if (needrank[i] < 10) {
            serious = true;
        }
    }
    if (!serious) {
        needs.push_back(need_none);
        needrank[0] = 10;
    }
    for (int i = 1; i < num_needs; i++) {
        if (needrank[i] < 20) {
            for (j = 0; j < needs.size(); j++) {
                if (needrank[i] < needrank[needs[j]]) {
                    needs.insert(needs.begin() + j, npc_need(i));
                    j = needs.size() + 1;
                }
            }
            if (j == needs.size()) {
                needs.push_back(npc_need(i));
            }
        }
    }
}

void npc::say( const std::string line, ... ) const
{
    va_list ap;
    va_start(ap, line);
    std::string formatted_line = vstring_format(line, ap);
    va_end(ap);
    parse_tags( formatted_line, g->u, *this );
    const bool sees = g->u.sees( *this );
    const bool deaf = g->u.is_deaf();
    if( sees && !deaf ) {
        add_msg(_("%1$s says: \"%2$s\""), name.c_str(), formatted_line.c_str());
        sounds::sound(pos(), 16, "");
    } else if( !sees ) {
        std::string sound = string_format(_("%1$s saying \"%2$s\""), name.c_str(), formatted_line.c_str());
        sounds::sound(pos(), 16, sound);
    } else {
        add_msg( m_warning, _( "%1$s says something but you can't hear it!" ), name.c_str() );
        sounds::sound(pos(), 16, "");
    }
}

bool npc::wants_to_sell( const item &it ) const
{
    const int market_price = it.price( true );
    return wants_to_sell( it, value( it, market_price ), market_price );
}

bool npc::wants_to_sell( const item &it, int at_price, int market_price ) const
{
    (void)it;

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
    (void)market_price;
    (void)it;

    if( is_friend() ) {
        return true;
    }

    // TODO: Base on inventory
    return at_price >= 80;
}

void npc::shop_restock()
{
    restock = calendar::turn + DAYS( 3 );
    if( is_friend() ) {
        return;
    }

    const Group_tag &from = myclass->get_shopkeeper_items();
    if( from == "EMPTY_GROUP" ) {
        return;
    }

    units::volume total_space = volume_capacity();
    std::list<item> ret;

    while( total_space > 0 && !one_in( 50 ) ) {
        item tmpit = item_group::item_from( from, 0 );
        if( !tmpit.is_null() && total_space >= tmpit.volume() ) {
            ret.push_back( tmpit );
            total_space -= tmpit.volume();
        }
    }

    has_new_items = true;
    inv.clear();
    inv.add_stack( ret );
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
    int inv_val = inv.worst_item_value(this);
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
    if( it.is_dangerous() ) {
        // Live grenade or something similar
        return -1000;
    }

    int ret = 0;
    // TODO: Cache own weapon value (it can be a bit expensive to compute 50 times/turn)
    int weapon_val = weapon_value( it ) - weapon_value( weapon );
    if( weapon_val > 0 ) {
        ret += weapon_val;
    }

    if( it.is_food() ) {
        int comestval = 0;
        if( nutrition_for( it.type ) > 0 || it.type->comestible->quench > 0 ) {
            comestval++;
        }
        if( get_hunger() > 40 ) {
            comestval += ( nutrition_for( it.type ) + get_hunger() - 40 ) / 6;
        }
        if( get_thirst() > 40 ) {
            comestval += ( it.type->comestible->quench + get_thirst() - 40 ) / 4;
        }
        if( comestval > 0 && can_eat( it ) == EDIBLE ) {
            ret += comestval;
        }
    }

    if( it.is_ammo() ) {
        if( weapon.is_gun() && it.type->ammo->type.count( weapon.ammo_type() ) ) {
            ret += 14; // @todo magazines - don't count ammo as usable if the weapon isn't.
        }

        if( std::any_of( it.type->ammo->type.begin(), it.type->ammo->type.end(),
                         [&]( const ammotype &e ) { return has_gun_for_ammo( e ); } ) ) {
            ret += 14; // @todo consider making this cumulative (once was)
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
    if( fac_has_job(FACJOB_DRUGS) && it.is_food() && it.type->comestible->addict >= 5 ) {
        ret += 10;
    }

    if( fac_has_job(FACJOB_DOCTORS) && it.is_food() && it.type->comestible->comesttype == "MED" ) {
        ret += 10;
    }

    if( fac_has_value(FACVAL_BOOKS) && it.is_book()) {
        ret += 14;
    }

    if( fac_has_job(FACJOB_SCAVENGE) ) {
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
    item *best = &ret_null;
    visit_items( [&best, bleed, bite, infect, first_best]( item *node ) {
        const auto use = node->type->get_use( "heal" );
        if( use == nullptr ){
            return VisitResponse::NEXT;
        }

        auto &actor = dynamic_cast<const heal_actor &>( *(use->get_actor_ptr()) );
        if( (!bleed || actor.bleed > 0) ||
            (!bite || actor.bite > 0) ||
            (!infect || actor.infect > 0) ) {
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
 return (has_effect( effect_pkill1 ) || has_effect( effect_pkill2 ) ||
         has_effect( effect_pkill3 ) || has_effect( effect_pkill_l ));
}

bool npc::is_friend() const
{
    return attitude == NPCATT_FOLLOW || attitude == NPCATT_LEAD;
}

bool npc::is_minion() const
{
    return is_friend() && op_of_u.trust >= 5;
}

bool npc::is_following() const
{
 switch (attitude) {
 case NPCATT_FOLLOW:
 case NPCATT_WAIT:
  return true;
 default:
  return false;
 }
}

bool npc::is_leader() const
{
 return (attitude == NPCATT_LEAD);
}

bool npc::is_enemy() const
{
    return attitude == NPCATT_KILL || attitude == NPCATT_FLEE;
}

bool npc::is_guarding() const
{
    return mission == NPC_MISSION_SHELTER || mission == NPC_MISSION_BASE ||
           mission == NPC_MISSION_SHOPKEEP || mission == NPC_MISSION_GUARD ||
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
        if( is_enemy() && g->u.attitude_to( other ) == A_FRIENDLY ) {
            return A_HOSTILE;
        }

        return A_NEUTRAL;
    } else if( other.is_player() ) {
        // For now, make it symmetric.
        return other.attitude_to( *this );
    }
    // Fallback to use the same logic as player, even through it's wrong:
    // Hostile (towards the player) npcs should see friendly monsters as hostile, too.
    return player::attitude_to( other );
}

int npc::smash_ability() const
{
    if( !is_following() || rules.allow_bash ) {
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
    return melee_value( weapon );
}

bool npc::bravery_check(int diff)
{
 return (dice(10 + personality.bravery, 6) >= dice(diff, 4));
}

bool npc::emergency() const
{
    return emergency( ai_cache.danger_assessment );
}

bool npc::emergency( float danger ) const
{
    return (danger > (personality.bravery * 3 * hp_percentage()) / 100);
}

//Check if this npc is currently in the list of active npcs.
//Active npcs are the npcs near the player that are actively simulated.
bool npc::is_active() const
{
    return std::find(g->active_npc.begin(), g->active_npc.end(), this) != g->active_npc.end();
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
    // @todo Allow player to set that
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
        return c_ltgreen;
    }
    return c_pink;
}

int npc::print_info(WINDOW* w, int line, int vLines, int column) const
{
    const int last_line = line + vLines;
    const size_t iWidth = getmaxx(w) - 2;
    // First line of w is the border; the next 4 are terrain info, and after that
    // is a blank line. w is 13 characters tall, and we can't use the last one
    // because it's a border as well; so we have lines 6 through 11.
    // w is also 48 characters wide - 2 characters for border = 46 characters for us
    mvwprintz(w, line++, column, c_white, _("NPC: %s"), name.c_str());
    if( is_armed() ) {
        trim_and_print(w, line++, column, iWidth, c_red, _("Wielding a %s"), weapon.tname().c_str());
    }

    const std::string worn_str = enumerate_as_string( worn.begin(), worn.end(), []( const item &it ) {
        return it.tname();
    } );
    if( worn_str.empty() ) {
        return line;
    }
    std::string wearing = _( "Wearing: " ) + remove_color_tags( worn_str );
    // @todo Replace with 'fold_and_print()'. Extend it with a 'height' argument to prevent leaking.
    size_t split;
    do {
        split = (wearing.length() <= iWidth) ? std::string::npos :
                                     wearing.find_last_of(' ', iWidth);
        if (split == std::string::npos) {
            mvwprintz(w, line, column, c_blue, wearing.c_str());
        } else {
            mvwprintz(w, line, column, c_blue, wearing.substr(0, split).c_str());
        }
        wearing = wearing.substr(split + 1);
        line++;
    } while (split != std::string::npos && line <= last_line);

    return line;
}

std::string npc::short_description() const
{
    std::stringstream ret;

    if( is_armed() ) {
        ret << _("Wielding: ") << weapon.tname() << ";   ";
    }
    const std::string worn_str = enumerate_as_string( worn.begin(), worn.end(),
    []( const item &it ) {
        return it.tname();
    } );
    if( !worn_str.empty() ) {
        ret << _("Wearing: ") << worn_str << ";";
    }
    return ret.str();
}

std::string npc::opinion_text() const
{
 std::stringstream ret;
 if (op_of_u.trust <= -10)
  ret << _("Completely untrusting");
 else if (op_of_u.trust <= -6)
  ret << _("Very untrusting");
 else if (op_of_u.trust <= -3)
  ret << _("Untrusting");
 else if (op_of_u.trust <= 2)
  ret << _("Uneasy");
 else if (op_of_u.trust <= 4)
  ret << _("Trusting");
 else if (op_of_u.trust < 10)
  ret << _("Very trusting");
 else
  ret << _("Completely trusting");

 ret << " (" << _("Trust: ") << op_of_u.trust << "); ";

 if (op_of_u.fear <= -10)
  ret << _("Thinks you're laughably harmless");
 else if (op_of_u.fear <= -6)
  ret << _("Thinks you're harmless");
 else if (op_of_u.fear <= -3)
  ret << _("Unafraid");
 else if (op_of_u.fear <= 2)
  ret << _("Wary");
 else if (op_of_u.fear <= 5)
  ret << _("Afraid");
 else if (op_of_u.fear < 10)
  ret << _("Very afraid");
 else
  ret << _("Terrified");

 ret << " (" << _("Fear: ") << op_of_u.fear << "); ";

 if (op_of_u.value <= -10)
  ret << _("Considers you a major liability");
 else if (op_of_u.value <= -6)
  ret << _("Considers you a burden");
 else if (op_of_u.value <= -3)
  ret << _("Considers you an annoyance");
 else if (op_of_u.value <= 2)
  ret << _("Doesn't care about you");
 else if (op_of_u.value <= 5)
  ret << _("Values your presence");
 else if (op_of_u.value < 10)
  ret << _("Treasures you");
 else
  ret << _("Best Friends Forever!");

 ret << " (" << _("Value: ") << op_of_u.value << "); ";

 if (op_of_u.anger <= -10)
  ret << _("You can do no wrong!");
 else if (op_of_u.anger <= -6)
  ret << _("You're good people");
 else if (op_of_u.anger <= -3)
  ret << _("Thinks well of you");
 else if (op_of_u.anger <= 2)
  ret << _("Ambivalent");
 else if (op_of_u.anger <= 5)
  ret << _("Pissed off");
 else if (op_of_u.anger < 10)
  ret << _("Angry");
 else
  ret << _("About to kill you");

 ret << " (" << _("Anger: ") << op_of_u.anger << ")";

 return ret.str();
}

void maybe_shift( tripoint &pos, int dx, int dy )
{
    if( pos != tripoint_min ) {
        pos.x += dx;
        pos.y += dy;
    }
}

void npc::shift(int sx, int sy)
{
    const int shiftx = sx * SEEX;
    const int shifty = sy * SEEY;

    position.x -= shiftx;
    position.y -= shifty;
    const point pos_om_old = sm_to_om_copy( mapx, mapy );
    mapx += sx;
    mapy += sy;
    const point pos_om_new = sm_to_om_copy( mapx, mapy );
    if( pos_om_old != pos_om_new ) {
        overmap &om_old = overmap_buffer.get( pos_om_old.x, pos_om_old.y );
        overmap &om_new = overmap_buffer.get( pos_om_new.x, pos_om_new.y );
        auto a = std::find(om_old.npcs.begin(), om_old.npcs.end(), this);
        if (a != om_old.npcs.end()) {
            om_old.npcs.erase( a );
            om_new.npcs.push_back( this );
        } else {
            // Don't move the npc pointer around to avoid having two overmaps
            // with the same npc pointer
            debugmsg( "could not find npc %s on its old overmap", name.c_str() );
        }
    }

    maybe_shift( wanted_item_pos, -shiftx, -shifty );
    maybe_shift( last_player_seen_pos, -shiftx, -shifty );
    maybe_shift( pulp_location, -shiftx, -shifty );
    path.clear();
}

bool npc::is_dead() const
{
    return dead || is_dead_state();
}

void npc::die(Creature* nkiller) {
    if( dead ) {
        // We are already dead, don't die again, note that npc::dead is
        // *only* set to true in this function!
        return;
    }
    dead = true;
    Character::die( nkiller );
    if (in_vehicle) {
        g->m.unboard_vehicle( pos() );
    }

    if (g->u.sees( *this )) {
        add_msg(_("%s dies!"), name.c_str());
    }
    if( killer == &g->u ){
        if (is_friend()) {
            if (g->u.has_trait("SAPIOVORE")) {
                g->u.add_memorial_log(pgettext("memorial_male", "Killed a friendly ape, %s.  Better eaten than eating."),
                                      pgettext("memorial_female", "Killed a friendly ape, %s.  Better eaten than eating."),
                                      name.c_str());
            }
            else if(!g->u.has_trait("PSYCHOPATH")) {
                // Very long duration, about 7d, decay starts after 10h.
                g->u.add_memorial_log(pgettext("memorial_male", "Killed a friend, %s."),
                                      pgettext("memorial_female", "Killed a friend, %s."),
                                      name.c_str());
                g->u.add_morale(MORALE_KILLED_FRIEND, -500, 0, 10000, 600);
            } else if(!g->u.has_trait("CANNIBAL") && g->u.has_trait("PSYCHOPATH")) {
                g->u.add_memorial_log(pgettext("memorial_male", "Killed someone foolish enough to call you friend, %s. Didn't care."),
                                      pgettext("memorial_female", "Killed someone foolish enough to call you friend, %s. Didn't care."),
                                      name.c_str());
            } else {
                g->u.add_memorial_log(pgettext("memorial_male", "Killed a delicious-looking friend, %s, in cold blood."),
                                      pgettext("memorial_female", "Killed a delicious-looking friend, %s, in cold blood."),
                                      name.c_str());
            }
        } else if (!is_enemy() || this->hit_by_player) {
            if (g->u.has_trait("SAPIOVORE")) {
                g->u.add_memorial_log(pgettext("memorial_male", "Caught and killed an ape.  Prey doesn't have a name."),
                                      pgettext("memorial_female", "Caught and killed an ape.  Prey doesn't have a name."));
            }
            else if(!g->u.has_trait("CANNIBAL") && !g->u.has_trait("PSYCHOPATH")) {
                // Very long duration, about 3.5d, decay starts after 5h.
                g->u.add_memorial_log(pgettext("memorial_male","Killed an innocent person, %s, in cold blood and felt terrible afterwards."),
                                      pgettext("memorial_female","Killed an innocent person, %s, in cold blood and felt terrible afterwards."),
                                      name.c_str());
                g->u.add_morale(MORALE_KILLED_INNOCENT, -100, 0, 5000, 300);
            } else if(!g->u.has_trait("CANNIBAL") && g->u.has_trait("PSYCHOPATH")) {
                g->u.add_memorial_log(pgettext("memorial_male", "Killed an innocent, %s, in cold blood. They were weak."),
                                      pgettext("memorial_female", "Killed an innocent, %s, in cold blood. They were weak."),
                                      name.c_str());
            } else if(g->u.has_trait("CANNIBAL") && !g->u.has_trait("PSYCHOPATH")) {
                g->u.add_memorial_log(pgettext("memorial_male", "Killed an innocent, %s."),
                                      pgettext("memorial_female", "Killed an innocent, %s."),
                                      name.c_str());
                g->u.add_morale(MORALE_KILLED_INNOCENT, -5, 0, 500, 300);
            } else {
                g->u.add_memorial_log(pgettext("memorial_male", "Killed a delicious-looking innocent, %s, in cold blood."),
                                      pgettext("memorial_female", "Killed a delicious-looking innocent, %s, in cold blood."),
                                      name.c_str());
            }
        }
    }

    place_corpse();
}

std::string npc_attitude_name(npc_attitude att)
{
    switch( att ) {
        case NPCATT_NULL:          // Don't care/ignoring player
            return _("Ignoring");
        case NPCATT_TALK:          // Move to and talk to player
            return _("Wants to talk");
        case NPCATT_FOLLOW:        // Follow the player
            return _("Following");
        case NPCATT_LEAD:          // Lead the player, wait for them if they're behind
            return _("Leading");
        case NPCATT_WAIT:          // Waiting for the player
            return _("Waiting for you");
        case NPCATT_MUG:           // Mug the player
            return _("Mugging you");
        case NPCATT_WAIT_FOR_LEAVE:// Attack the player if our patience runs out
            return _("Waiting for you to leave");
        case NPCATT_KILL:          // Kill the player
            return _("Attacking to kill");
        case NPCATT_FLEE:          // Get away from the player
            return _("Fleeing");
        case NPCATT_HEAL:          // Get to the player and heal them
            return _("Healing you");
        default:
            break;
    }

    debugmsg( "Invalid attitude: %d", att );
    return _("Unknown");
}

void npc::setID (int i)
{
    this->player::setID(i);
}

//message related stuff

//message related stuff
void npc::add_msg_if_npc(const char *msg, ...) const
{
    va_list ap;
    va_start(ap, msg);
    std::string processed_npc_string = vstring_format(msg, ap);
    processed_npc_string = replace_with_npc_name(processed_npc_string, disp_name());
    add_msg(processed_npc_string.c_str());

    va_end(ap);
}
void npc::add_msg_player_or_npc(const char *, const char* npc_str, ...) const
{
    va_list ap;

    va_start(ap, npc_str);

    if (g->u.sees(*this)) {
        std::string processed_npc_string = vstring_format(npc_str, ap);
        processed_npc_string = replace_with_npc_name(processed_npc_string, disp_name());
        add_msg(processed_npc_string.c_str());
    }

    va_end(ap);
}
void npc::add_msg_if_npc(game_message_type type, const char *msg, ...) const
{
    va_list ap;
    va_start(ap, msg);
    std::string processed_npc_string = vstring_format(msg, ap);
    processed_npc_string = replace_with_npc_name(processed_npc_string, disp_name());
    add_msg(type, processed_npc_string.c_str());

    va_end(ap);
}
void npc::add_msg_player_or_npc(game_message_type type, const char *, const char* npc_str, ...) const
{
    va_list ap;

    va_start(ap, npc_str);

    if (g->u.sees(*this)) {
        std::string processed_npc_string = vstring_format(npc_str, ap);
        processed_npc_string = replace_with_npc_name(processed_npc_string, disp_name());
        add_msg(type, processed_npc_string.c_str());
    }

    va_end(ap);
}

void npc::add_msg_player_or_say( const char *, const char *npc_str, ... ) const
{
    va_list ap;
    va_start(ap, npc_str);
    const std::string text = vstring_format( npc_str, ap );
    say( text );
    va_end(ap);
}

void npc::add_msg_player_or_say( game_message_type, const char *, const char *npc_str, ... ) const
{
    va_list ap;
    va_start(ap, npc_str);
    const std::string text = vstring_format( npc_str, ap );
    say( text );
    va_end(ap);
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
    const int now = calendar::turn;
    // TODO: Sleeping, healing etc.
    int dt = now - last_updated;
    last_updated = calendar::turn;
    // Cap at some reasonable number, say 2 days (2 * 48 * 30 minutes)
    dt = std::min( dt, 2 * 48 * MINUTES(30) );
    int cur = now - dt;
    add_msg( m_debug, "on_load() by %s, %d turns", name.c_str(), dt );
    // First update with 30 minute granularity, then 5 minutes, then turns
    for( ; cur < now - MINUTES(30); cur += MINUTES(30) + 1 ) {
        update_body( cur, cur + MINUTES(30) );
    }
    for( ; cur < now - MINUTES(5); cur += MINUTES(5) + 1 ) {
        update_body( cur, cur + MINUTES(5) );
    }
    for( ; cur < now; cur++ ) {
        update_body( cur, cur + 1 );
    }

    if( dt > 0 ) {
        // This ensures food is properly rotten at load
        // Otherwise NPCs try to eat rotten food and fail
        process_active_items();
    }

    // Not necessarily true, but it's not a bad idea to set this
    has_new_items = true;
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
    is_unique = false;
    lines.push_back("                                                                            ");
    lines.push_back("                                                                            ");
    lines.push_back("                                                                            ");
    lines.push_back("                                                                            ");
    lines.push_back("                                                                            ");
    lines.push_back("                                                                            ");
    lines.push_back("           ###### #### ####   ######    ####    ###   #### ######           ");
    lines.push_back("            ##  #  ##   ##     ##  #     ##    ## ## ##  # # ## #           ");
    lines.push_back("            ####   ##   ##     ####      ##    ## ## ####    ##             ");
    lines.push_back("            ##     ##   ##     ##        ##    ## ##   ###   ##             ");
    lines.push_back("            ##     ##   ## ##  ## ##     ## ## ## ## #  ##   ##             ");
    lines.push_back("           ####   #### ###### ######    ######  ###  ####   ####            ");
    lines.push_back("                                                                            ");
    lines.push_back("                                                                            ");
    lines.push_back("                                                                            ");
    lines.push_back("                                                                            ");
    lines.push_back("                                                                            ");
    lines.push_back("                                                                            ");
    lines.push_back("                                                                            ");
    lines.push_back("                                                                            ");
}

epilogue_map epilogue::_all_epilogue;

void epilogue::load_epilogue(JsonObject &jsobj)
{
    epilogue base;
    base.id = jsobj.get_string("id");
    base.group = jsobj.get_string("group");
    base.is_unique = jsobj.get_bool("unique", false);
    base.lines.clear();
    base.lines.push_back(jsobj.get_string("line_01"));
    base.lines.push_back(jsobj.get_string("line_02"));
    base.lines.push_back(jsobj.get_string("line_03"));
    base.lines.push_back(jsobj.get_string("line_04"));
    base.lines.push_back(jsobj.get_string("line_05"));
    base.lines.push_back(jsobj.get_string("line_06"));
    base.lines.push_back(jsobj.get_string("line_07"));
    base.lines.push_back(jsobj.get_string("line_08"));
    base.lines.push_back(jsobj.get_string("line_09"));
    base.lines.push_back(jsobj.get_string("line_10"));
    base.lines.push_back(jsobj.get_string("line_11"));
    base.lines.push_back(jsobj.get_string("line_12"));
    base.lines.push_back(jsobj.get_string("line_13"));
    base.lines.push_back(jsobj.get_string("line_14"));
    base.lines.push_back(jsobj.get_string("line_15"));
    base.lines.push_back(jsobj.get_string("line_16"));
    base.lines.push_back(jsobj.get_string("line_17"));
    base.lines.push_back(jsobj.get_string("line_18"));
    base.lines.push_back(jsobj.get_string("line_19"));
    base.lines.push_back(jsobj.get_string("line_20"));
    _all_epilogue[base.id] = base;
}

epilogue* epilogue::find_epilogue(std::string ident)
{
    epilogue_map::iterator found = _all_epilogue.find(ident);
    if (found != _all_epilogue.end()){
        return &(found->second);
    } else {
        debugmsg("Tried to get invalid epilogue template: %s", ident.c_str());
        static epilogue null_epilogue;
    return &null_epilogue;
    }
}

void epilogue::random_by_group(std::string group, std::string name)
{
    std::vector<epilogue> v;
    for( auto epi : _all_epilogue ) {
        if (epi.second.group == group){
            v.push_back( epi.second );
        }
    }
    if (v.size() == 0)
        return;
    epilogue epi = random_entry( v );
    id = epi.id;
    group = epi.group;
    is_unique = epi.is_unique;
    lines.clear();
    lines = epi.lines;
    for( auto &ln : lines ) {
        if (!ln.empty() && ln[0]=='*'){
            ln.replace(0,name.size(),name);
        }
    }

}

const tripoint npc::no_goal_point(INT_MIN, INT_MIN, INT_MIN);

bool npc::query_yn( const char *, ... ) const
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

    for( auto& e : worn ) {
        if( e.can_holster( *obj ) ) {
            auto ptr = dynamic_cast<const holster_actor *>( e.type->get_use( "holster" )->get_actor_ptr() );
            opts.emplace_back( dispose_option {
                item_store_cost( *obj, e, false, ptr->draw_cost ),
                [this,ptr,&e,&obj]{ ptr->store( *this, e, *obj ); }
            } );
        }
    }

    if( volume_carried() + obj->volume() <= volume_capacity() ) {
        opts.emplace_back( dispose_option {
            item_handling_cost( *obj ) * INVENTORY_HANDLING_FACTOR,
            [this,&obj] {
                moves -= item_handling_cost( *obj ) * INVENTORY_HANDLING_FACTOR;
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
        []( const dispose_option &lop, const dispose_option &rop ) {
        return lop.moves < rop.moves;
    } );

    mn->action();
    return true;
}

void npc::process_turn()
{
    player::process_turn();

    if( is_following() && calendar::once_every( HOURS(1) ) &&
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
        int state_penalty = get_hunger() + get_thirst() + (100 - hp_percentage()) + get_pain();
        if( x_in_y( trust_chance, 240 + 10 * op_penalty + state_penalty ) ) {
            op_of_u.trust++;
        }

        // TODO: Similar checks for fear and anger
    }

    last_updated = calendar::turn;
    // TODO: Add decreasing trust/value/etc. here when player doesn't provide food
    // TODO: Make NPCs leave the player if there's a path out of map and player is sleeping/unseen/etc.
}

std::ostream& operator<< (std::ostream & os, npc_need need)
{
    switch (need)
    {
        case need_none :   return os << "need_none";
        case need_ammo :   return os << "need_ammo";
        case need_weapon : return os << "need_weapon";
        case need_gun :    return os << "need_gun";
        case need_food :   return os << "need_food";
        case need_drink :  return os << "need_drink";
        case num_needs :   return os << "num_needs";
    };
    return os << "unknown need";
}

bool npc::will_accept_from_player( const item &it ) const
{
    if( is_minion() || g->u.has_trait( "DEBUG_MIND_CONTROL" ) || it.has_flag( "NPC_SAFE" ) ) {
        return true;
    }

    if( !it.type->use_methods.empty() ) {
        return false;
    }

    const auto comest = it.type->comestible;
    if( comest != nullptr && ( comest->quench < 0 || it.poison > 0 ) ) {
        return false;
    }

    return true;
}

const pathfinding_settings &npc::get_pathfinding_settings() const
{
    path_settings.bash_strength = smash_ability();

    return path_settings;
}

const pathfinding_settings &npc::get_pathfinding_settings( bool no_bashing ) const
{
    path_settings.bash_strength = no_bashing ? 0 : smash_ability();

    return path_settings;
}

std::set<tripoint> npc::get_path_avoid() const
{
    std::set<tripoint> ret;
    ret.insert( g->u.pos() );
    for( size_t i = 0; i < g->num_zombies(); i++ ) {
        // @todo Cache this somewhere
        ret.insert( g->zombie( i ).pos() );
    }

    for( const npc *np : g->active_npc ) {
        ret.insert( np->pos() );
    }

    return ret;
}

