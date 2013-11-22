#include "player.h"
#include "npc.h"
#include "profession.h"
#include "bionics.h"
#include "mission.h"
#include "game.h"
#include "disease.h"
#include "addiction.h"
#include "keypress.h"
#include "moraledata.h"
#include "inventory.h"
#include "artifact.h"
#include "options.h"
#include <sstream>
#include <stdlib.h>
#include <algorithm>
#include "weather.h"
#include "item.h"
#include "material.h"
#include "translations.h"
#include "name.h"
#include "cursesdef.h"
#include "catacharset.h"
#include "disease.h"
#include "crafting.h"
#include "get_version.h"
#include "monstergenerator.h"

#include "savegame.h"
#include "tile_id_data.h" // for monster::json_save
#include <ctime>

#include "json.h"

#include "debug.h"
#define dbg(x) dout((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

////////////////////////////////////////////////////////////////////////////////////////////////////
///// player.h

void player_activity::serialize(JsonOut &json) const
{
    json.start_object();
    json.member( "type", int(type) );
    json.member( "moves_left", moves_left );
    json.member( "index", int(index) );
    json.member( "invlet", int(invlet) );
    json.member( "name", name );
    json.member( "placement", placement );
    json.member( "values", values );
    json.member( "str_values", str_values );
    json.end_object();
}

void player_activity::deserialize(JsonIn &jsin)
{
    JsonObject data = jsin.get_object();
    int tmptype;
    int tmpinv;
    if ( !data.read_into( "type", tmptype ) || type >= NUM_ACTIVITIES ) {
        debugmsg( "Bad activity data:\n%s", data.str().c_str() );
    }
    type = activity_type(tmptype);
    data.read_into( "moves_left", moves_left );
    data.read_into( "index", index );
    data.read_into( "invlet", tmpinv );
    invlet = (char)tmpinv;
    data.read_into( "name", name );
    data.read_into( "placement", placement );
    values = data.get_int_array("values");
    str_values = data.get_string_array("str_values");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// skill.h
void SkillLevel::serialize(JsonOut &json) const
{
    json.start_object();
    json.member("level", level() );
    json.member("exercise", exercise(true) );
    json.member("istraining", isTraining() );
    json.member("lastpracticed", int ( lastPracticed() ) );
    json.end_object();
}

void SkillLevel::deserialize(JsonIn & jsin)
{
    JsonObject data = jsin.get_object();
    int lastpractice=0;
    data.read_into( "level", _level );
    data.read_into( "exercise", _exercise );
    data.read_into( "istraining", _isTraining );
    data.read_into( "lastpracticed", lastpractice );
    if(lastpractice == 0) {
        _lastPracticed = HOURS(OPTIONS["INITIAL_TIME"]);
    } else {
        _lastPracticed = lastpractice;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// player.h, player + npc
/*
 * Gather variables for saving. These variables are common to both the player and NPCs (which is a kind of player).
 * Do not overload; NPC or player specific stuff should go to player::json_save or npc::json_save.
 */

void player::json_load_common_variables(JsonObject & data)
{
    JsonArray parray;

// todo/maybe:
// std::map<std::string, int*> strmap_common_variables;
// void player::init_strmap_common_variables() {
//     strmap_common_variables["posx"]=&posx; // + all this below and in save_common_variables
// }
// load:
// for(std::map<std::string, int*>::iterator it...
//     data.read_into(it->first,it->second);
// save:
// for(...
//     json.member( it->first, it->second );
    if(!data.read_into("posx",posx) ) { // uh-oh.
         debugmsg("BAD PLAYER/NPC JSON: no 'posx'?");
    }
    data.read_into("posy",posy);
    data.read_into("str_cur",str_cur);      data.read_into("str_max",str_max);
    data.read_into("dex_cur",dex_cur);      data.read_into("dex_max",dex_max);
    data.read_into("int_cur",int_cur);      data.read_into("int_max",int_max);
    data.read_into("per_cur",per_cur);      data.read_into("per_max",per_max);
    data.read_into("hunger",hunger);        data.read_into("thirst",thirst);
    data.read_into("fatigue",fatigue);      data.read_into("stim",stim);
    data.read_into("pain",pain);            data.read_into("pkill",pkill);
    data.read_into("radiation",radiation);
    data.read_into("scent",scent);
    data.read_into("moves",moves);
    data.read_into("dodges_left",dodges_left);
    data.read_into("underwater",underwater);
    data.read_into("oxygen",oxygen);
    data.read_into("male",male);
    data.read_into("cash",cash);
    data.read_into("recoil",recoil);

    parray = data.get_array("hp_cur");
    if ( parray.size() == num_hp_parts ) {
        for(int i=0; i < num_hp_parts; i++) {
            hp_cur[i] = parray.get_int(i);
        }
    } else {
        debugmsg("Error, incompatible hp_cur in save file '%s'",parray.str().c_str());
    }

    parray = data.get_array("hp_max");
    if ( parray.size() == num_hp_parts ) {
        for(int i=0; i < num_hp_parts; i++) {
            hp_max[i] = parray.get_int(i);
        }
    } else {
        debugmsg("Error, incompatible hp_max in save file '%s'",parray.str().c_str());
    }

    data.read_into("power_level",power_level);
    data.read_into("max_power_level",max_power_level);
    data.read_into("traits",my_traits);

    if (data.has_object("skills")) {
        JsonObject pmap = data.get_object("skills");
        for (std::vector<Skill*>::iterator aSkill = Skill::skills.begin(); aSkill != Skill::skills.end(); ++aSkill) {
            if ( pmap.has_object( (*aSkill)->ident() ) ) {
                pmap.read_into( (*aSkill)->ident(), skillLevel(*aSkill) );
            } else {
                 debugmsg("Load (%s) Missing skill %s","",(*aSkill)->ident().c_str() );
            }
        }
    } else {
        debugmsg("Skills[] no bueno");
    }

    data.read_into("ma_styles",ma_styles);
    data.read_into("illness",illness);
    data.read_into("addictions",addictions);
    data.read_into("my_bionics",my_bionics);
}

/*
 * Variables common to player and npc
 */
void player::json_save_common_variables(JsonOut &json) const
{
    // assumes already in player object
    // positional data
    json.member( "posx", posx );            json.member( "posy", posy );

    // attributes, current / max levels
    json.member( "str_cur", str_cur );      json.member( "str_max", str_max );
    json.member( "dex_cur", dex_cur );      json.member( "dex_max", dex_max );
    json.member( "int_cur", int_cur );      json.member( "int_max", int_max );
    json.member( "per_cur", per_cur );      json.member( "per_max", per_max );

    // om-noms or lack thereof
    json.member( "hunger", hunger );        json.member( "thirst", thirst );
    // energy
    json.member( "fatigue", fatigue );      json.member( "stim", stim );
    // pain
    json.member( "pain", pain );            json.member( "pkill", pkill );
    // misc levels
    json.member( "radiation", radiation );
    json.member( "scent", int(scent) );

    // initiative type stuff
    json.member( "moves", moves );
    json.member( "dodges_left", dodges_left );

    // breathing
    json.member( "underwater", underwater );
    json.member( "oxygen", oxygen );

    // gender
    json.member( "male", male );

    json.member( "cash", cash );
    json.member( "recoil", int(recoil) );

    // potential incompatibility with future expansion
    // todo: consider ["parts"]["head"]["hp_cur"] instead of ["hp_cur"][head_enum_value]
    json.member( "hp_cur", std::vector<int>( hp_cur, hp_cur + num_hp_parts ) );
    json.member( "hp_max", std::vector<int>( hp_max, hp_max + num_hp_parts ) );

    // npc; unimplemented
    json.member( "power_level", power_level );
    json.member( "max_power_level", max_power_level );

    // traits: permanent 'mutations' more or less
    json.member( "traits", my_traits );

    // skills
    json.member( "skills" );
    json.start_object();
    for (std::vector<Skill*>::iterator aSkill = Skill::skills.begin(); aSkill != Skill::skills.end(); ++aSkill) {
        SkillLevel sk = get_skill_level(*aSkill);
        json.member((*aSkill)->ident(), sk);
    }
    json.end_object();

    // martial arts
    /*for (int i = 0; i < ma_styles.size(); i++) {
        ptmpvect.push_back( pv( ma_styles[i] ) );
    }*/
    json.member( "ma_styles", ma_styles );

    // disease
    json.member( "illness", illness );

    // "Looks like I picked the wrong week to quit smoking." - Steve McCroskey
    json.member( "addictions", addictions );

    // "Fracking Toasters" - Saul Tigh, toaster
    json.member( "my_bionics", my_bionics );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// player.h, player
/*
 * Prepare a json object for player, including player specific data, and data common to
   players and npcs ( which json_save_actor_data() handles ).
 */
void player::serialize(JsonOut &json, bool save_contents) const
{
    json.start_object();

    json_save_common_variables( json );

    // player-specific specifics
    if ( prof != NULL ) {
        json.member( "profession", prof->ident() );
    }

    // someday, npcs may drive
    json.member( "driving_recoil", int(driving_recoil) );
    json.member( "in_vehicle", in_vehicle );
    json.member( "controlling_vehicle", controlling_vehicle );

    // shopping carts, furniture etc
    json.member( "grab_point", grab_point );
    json.member( "grab_type", obj_type_name[ (int)grab_type ] );

    // misc player specific stuff
    json.member( "blocks_left", blocks_left );
    json.member( "focus_pool", focus_pool );
    json.member( "style_selected", style_selected );

    // possibly related to illness[] ?
    json.member( "health", health );

    // crafting etc
    json.member( "activity", activity );
    json.member( "backlog", activity );

    // mutations; just like traits but can be removed.
    json.member( "mutations", my_mutations );

    // "The cold wakes you up."
    json.member( "temp_cur", std::vector<int>( temp_cur, temp_cur + num_bp ) );
    json.member( "temp_conv", std::vector<int>( temp_conv, temp_conv + num_bp ) );
    json.member( "frostbite_timer", std::vector<int>( frostbite_timer, frostbite_timer + num_bp ) );

    // npc: unimplemented, potentially useful
    json.member( "learned_recipes" );
    json.start_array();
    for (std::map<std::string, recipe*>::const_iterator iter = learned_recipes.begin(); iter != learned_recipes.end(); ++iter) {
        json.write( iter->first );
    }
    json.end_array();

    // :(
    json.member( "morale", morale );

    // mission stuff
    json.member("active_mission", active_mission );

    json.member( "active_missions", active_missions );
    json.member( "completed_missions", completed_missions );
    json.member( "failed_missions", failed_missions );

    json.member( "player_stats", get_stats() );

    if ( save_contents ) {
        json.member( "worn", worn ); // also saves contents

        json.member("inv");
        inv.json_save_items(json);

        json.member("invcache");
        inv.json_save_invcache(json);

        if (!weapon.is_null()) {
            json.member( "weapon", weapon ); // also saves contents
        }
//FIXME: seperate function, better still another file
  /*      for(int i = 0; i < memorial_log.size(); i++) {
            ptmpvect.push_back(pv(memorial_log[i]));
        }
        json.member("memorial",ptmpvect);
*/
    }

    json.end_object();
}

/*
 * load player from ginormous json blob
 */
void player::deserialize(JsonIn &jsin)
{
    JsonObject data = jsin.get_object();
    JsonArray parray;

    json_load_common_variables( data );

    std::string prof_ident="(null)";
    if ( data.read_into("profession",prof_ident) && profession::exists(prof_ident) ) {
        prof = profession::prof(prof_ident);
    } else {
        debugmsg("Tried to use non-existent profession '%s'", prof_ident.c_str());
    }

    data.read_into("activity",activity);
    data.read_into("backlog",backlog);

    data.read_into("driving_recoil",driving_recoil);
    data.read_into("in_vehicle",in_vehicle);
    data.read_into("controlling_vehicle",controlling_vehicle);

    data.read_into("grab_point", grab_point);
    std::string grab_typestr="OBJECT_NONE";
    if( grab_point.x != 0 || grab_point.y != 0 ) {
        grab_typestr = "OBJECT_VEHICLE";
        data.read_into( "grab_type", grab_typestr);
    }

    if ( obj_type_id.find(grab_typestr) != obj_type_id.end() ) {
        grab_type = (object_type)obj_type_id[grab_typestr];
    }

    data.read_into( "blocks_left", blocks_left);
    data.read_into( "focus_pool", focus_pool);
    data.read_into( "style_selected", style_selected );

    data.read_into( "health", health );

    data.read_into( "mutations", my_mutations );

    set_highest_cat_level();
    // testme  drench_mut_calc();

    parray = data.get_array("temp_cur");
    if ( parray.size() == num_bp ) {
        for(int i=0; i < num_bp; i++) {
            temp_cur[i]=parray.get_int(i);
        }
    } else {
        debugmsg("Error, incompatible temp_cur in save file %s",parray.str().c_str());
    }

    parray = data.get_array("temp_conv");
    if ( parray.size() == num_bp ) {
        for(int i=0; i < num_bp; i++) {
            temp_conv[i]=parray.get_int(i);
        }
    } else {
        debugmsg("Error, incompatible temp_conv in save file %s",parray.str().c_str());
    }

    parray = data.get_array("frostbite_timer");
    if ( parray.size() == num_bp ) {
        for(int i=0; i < num_bp; i++) {
            frostbite_timer[i]=parray.get_int(i);
        }
    } else {
        debugmsg("Error, incompatible frostbite_timer in save file %s",parray.str().c_str());
    }

    parray = data.get_array("learned_recipes");
    if ( !parray.empty() ) {
        learned_recipes.clear();
        std::string pstr="";
        while ( parray.has_more() ) {
            if ( parray.read_into(pstr) ) {
                learned_recipes[ pstr ] = recipe_by_name( pstr );
            }
        }
    }

    data.read_into("morale", morale);

    data.read_into( "active_mission", active_mission );

    data.read_into( "active_missions", active_missions );
    data.read_into( "failed_missions", failed_missions );
    data.read_into( "completed_missions", completed_missions );

    stats & pstats = *lifetime_stats();
    data.read_into("player_stats",pstats);

    inv.clear();
    if ( data.has_member("inv") ) {
        JsonIn* jip = data.get_raw("inv");
        inv.json_load_items( *jip );
    }

    worn.clear();
    data.read_into("worn", worn);

    weapon.contents.clear();
    data.read_into("weapon", weapon);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// npc.h

void npc_combat_rules::serialize(JsonOut &json) const
{
    json.start_object();
    json.member("engagement", (int)engagement );
    json.member("use_guns", use_guns );
    json.member("use_grenades", use_grenades );
    json.member("use_silent", use_silent );
    //todo    json.member("guard_pos", guard_pos );
    json.end_object();
}

void npc_combat_rules::deserialize(JsonIn &jsin)
{
    JsonObject data = jsin.get_object();
    int tmpeng;
    data.read_into("engagement", tmpeng);
    engagement = (combat_engagement)tmpeng;
    data.read_into( "use_guns", use_guns);
    data.read_into( "use_grenades", use_grenades);
    data.read_into( "use_silent", use_silent);
}

void npc_chatbin::serialize(JsonOut &json) const
{
    json.start_object();
    json.member( "first_topic", (int)first_topic );
    json.member( "mission_selected", mission_selected );
    json.member( "tempvalue", tempvalue );     //No clue what this value does, but it is used all over the place. So it is NOT temp.
    if ( skill ) {
        json.member("skill", skill->ident() );
    }
    json.member( "missions", missions );
    json.member( "missions_assigned", missions_assigned );
    json.end_object();
}

void npc_chatbin::deserialize(JsonIn &jsin)
{
    JsonObject data = jsin.get_object();
    int tmptopic;
    std::string skill_ident;

    data.read_into("first_topic", tmptopic);
    first_topic = talk_topic(tmptopic);

    if ( data.read_into("skill",skill_ident) ) {
        skill = Skill::skill(skill_ident);
    }

    data.read_into("tempvalue", tempvalue);
    data.read_into("mission_selected", mission_selected);
    data.read_into( "missions", missions );
    data.read_into( "missions_assigned", missions_assigned );
}

void npc_personality::deserialize(JsonIn &jsin)
{
    JsonObject data = jsin.get_object();
    int tmpagg, tmpbrav, tmpcol, tmpalt;
    if ( data.read_into("aggression", tmpagg) &&
            data.read_into("bravery", tmpbrav) &&
            data.read_into("collector", tmpcol) &&
            data.read_into("altruism", tmpalt) ) {
        aggression=(signed char)tmpagg;
        bravery=(signed char)tmpbrav;
        collector=(signed char)tmpcol;
        altruism=(signed char)tmpalt;
    } else {
        debugmsg("npc_personality: bad data");
    }
}

void npc_personality::serialize(JsonOut &json) const
{
    json.start_object();
    json.member( "aggression", (int)aggression );
    json.member( "bravery", (int)bravery );
    json.member( "collector", (int)collector );
    json.member( "altruism", (int)altruism );
    json.end_object();
};

void npc_opinion::deserialize(JsonIn &jsin)
{
    JsonObject data = jsin.get_object();
    data.read_into("trust",trust);
    data.read_into("fear",fear);
    data.read_into("value",value);
    data.read_into("anger",anger);
    data.read_into("owed",owed);
    data.read_into("favors",favors);
}

void npc_opinion::serialize(JsonOut &json) const
{
    json.start_object();
    json.member( "trust", trust );
    json.member( "fear", fear );
    json.member( "value", value );
    json.member( "anger", anger );
    json.member( "owed", owed );
    json.member( "favors", favors );
    json.end_object();
}

void npc_favor::deserialize(JsonIn &jsin)
{
    JsonObject jo = jsin.get_object();
    type = npc_favor_type(jo.get_int("type"));
    jo.read_into("value", value);
    jo.read_into("itype_id", item_id);
    skill = NULL;
    if (jo.has_int("skill_id")) {
        skill = Skill::skill(jo.get_int("skill_id"));
    } else if (jo.has_string("skill_id")) {
        skill = Skill::skill(jo.get_string("skill_id"));
    }
}

void npc_favor::serialize(JsonOut &json) const
{
    json.start_object();
    json.member("type", (int)type);
    json.member("value", value);
    json.member("itype_id", (std::string)item_id);
    if (skill != NULL) {
        json.member("skill_id", skill->ident());
    }
    json.end_object();
}

/*
 * load npc
 */
void npc::deserialize(JsonIn &jsin)
{
    JsonObject data = jsin.get_object();

    json_load_common_variables(data);

    int misstmp, classtmp, flagstmp, atttmp, tmpid;

    data.read_into("id",tmpid);  setID(tmpid);
    data.read_into("name",name);
    data.read_into("marked_for_death", marked_for_death);
    data.read_into("dead", dead);
    if ( data.read_into( "myclass", classtmp) ) {
        myclass = npc_class( classtmp );
    }

    data.read_into("personality", personality);

    data.read_into("wandx",wandx);
    data.read_into("wandy",wandy);
    data.read_into("wandf",wandf);
    data.read_into("omx",omx);
    data.read_into("omy",omy);
    data.read_into("omz",omz);

    data.read_into("mapx",mapx);
    data.read_into("mapy",mapy);

    data.read_into("plx",plx);
    data.read_into("ply",ply);

    data.read_into("goalx",goalx);
    data.read_into("goaly",goaly);
    data.read_into("goalz",goalz);

    if ( data.read_into("mission",misstmp) ) {
        mission = npc_mission( misstmp );
    }

    if ( data.read_into( "flags", flagstmp) ) {
        flags = flagstmp;
    }
    if ( data.read_into( "attitude", atttmp) ) {
        attitude = npc_attitude(atttmp);
    }

    inv.clear();
    if ( data.has_member("inv") ) {
        JsonIn *invin = data.get_raw("inv");
        inv.json_load_items( *invin );
    }

    worn.clear();
    data.read_into("worn",worn);

    weapon.contents.clear();
    data.read_into("weapon",weapon);

    data.read_into("op_of_u",op_of_u);
    data.read_into("chatbin",chatbin);
    data.read_into("combat_rules",combat_rules);
}

/*
 * save npc
 */
void npc::serialize(JsonOut &json, bool save_contents) const
{
    json.start_object();

    json_save_common_variables( json );

    json.member( "name", name );
    json.member( "id", getID() );
    json.member( "marked_for_death", marked_for_death );
    json.member( "dead", dead );
    json.member( "patience", patience );
    json.member( "myclass", (int)myclass );

    json.member( "personality", personality );
    json.member( "wandx", wandx );
    json.member( "wandy", wandy );
    json.member( "wandf", wandf );
    json.member( "omx", omx );
    json.member( "omy", omy );
    json.member( "omz", omz );

    json.member( "mapx", mapx );
    json.member( "mapy", mapy );
    json.member( "plx", plx );
    json.member( "ply", ply );
    json.member( "goalx", goalx );
    json.member( "goaly", goaly );
    json.member( "goalz", goalz );

    json.member( "mission", mission ); // todo: stringid
    json.member( "flags", flags );
    if ( my_fac != NULL ) { // set in constructor
        json.member( "my_fac", my_fac->id );
    }
    json.member( "attitude", (int)attitude );
    json.member("op_of_u", op_of_u);
    json.member("chatbin", chatbin);
    json.member("combat_rules", combat_rules);

    if ( save_contents ) {
        json.member( "worn", worn ); // also saves contents

        json.member("inv");
        inv.json_save_items(json);

        if (!weapon.is_null()) {
            json.member( "weapon", weapon ); // also saves contents
        }
    }

    json.end_object();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// inventory.h
/*
 * Save invlet cache
 */
void inventory::json_save_invcache(JsonOut &json) const
{
    json.start_array();
    for( std::map<std::string, std::vector<char> >::const_iterator invlet_id =  invlet_cache.begin(); invlet_id != invlet_cache.end(); ++invlet_id ) {
        json.start_object();
        json.member( invlet_id->first );
        json.start_array();
        for( std::vector<char>::const_iterator sym = invlet_id->second.begin();
                sym != invlet_id->second.end(); ++sym ) {
            json.write( int(*sym) );
        }
        json.end_array();
        json.end_object();
    }
    json.end_array();
}

/*
 * Invlet cache: player specific, thus not wrapped in inventory::json_load/save
 */
void inventory::json_load_invcache(JsonIn &jsin)
{
    try {
        JsonArray ja = jsin.get_array();
        while ( ja.has_more() ) {
            JsonObject jo = ja.next_object();
            std::set<std::string> members = jo.get_member_names();
            for (std::set<std::string>::iterator it = members.begin();
                    it != members.end(); ++it) {
                std::vector<char> vect;
                JsonArray pvect = jo.get_array(*it);
                while ( pvect.has_more() ) {
                    vect.push_back( pvect.next_int() );
                }
                invlet_cache[*it] = vect;
            }
        }
    } catch (std::string jsonerr) {
        debugmsg("bad invcache json:\n%s", jsonerr.c_str() );
    }
}

/*
 * save all items. Just this->items, invlet cache saved seperately
 */
void inventory::json_save_items(JsonOut &json) const
{
    json.start_array();
    for (invstack::const_iterator iter = items.begin(); iter != items.end(); ++iter) {
        for (std::list<item>::const_iterator stack_iter = iter->begin();
             stack_iter != iter->end();
             ++stack_iter)
        {
            stack_iter->serialize(json, true);
        }
    }
    json.end_array();
}


void inventory::json_load_items(JsonIn &jsin)
{
    try {
        JsonArray ja = jsin.get_array();
        while ( ja.has_more() ) {
            JsonObject jo = ja.next_object();
            push_back( item( jo ) );
        }
    } catch (std::string jsonerr) {
         debugmsg("bad inventory json:\n%s", jsonerr.c_str() );
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// monster.h

void monster::deserialize(JsonIn &jsin)
{
    JsonObject data = jsin.get_object();

    int iidtmp;
    std::string sidtmp;
    // load->str->int
    if ( ! data.read_into("typeid", sidtmp) ) {
        // or load->int->str->possibly_shifted_int
        data.read_into("typeid", iidtmp);
        sidtmp = legacy_mon_id[ iidtmp ];
    }
    type = GetMType(sidtmp);

    data.read_into("posx", _posx);
    data.read_into("posy", _posy);
    data.read_into("wandx", wandx);
    data.read_into("wandy", wandy);
    data.read_into("wandf", wandf);
    data.read_into("moves", moves);
    data.read_into("speed", speed);
    data.read_into("hp", hp);
    data.read_into("sp_timeout", sp_timeout);
    data.read_into("friendly", friendly);
    data.read_into("faction_id", faction_id);
    data.read_into("mission_id", mission_id);
    data.read_into("no_extra_death_drops", no_extra_death_drops);
    data.read_into("dead", dead);
    data.read_into("anger", anger);
    data.read_into("morale", morale);
    data.read_into("hallucination", hallucination);
    data.read_into("onstairs", onstairs);
    data.read_into("stairscount", staircount); // really? re: exploitable if not.

    data.read_into("plans", plans);

    data.read_into("inv", inv);
    data.read_into("ammo", ammo);
}

/*
 * Save, json ed; serialization that won't break as easily. In theory.
 */
void monster::serialize(JsonOut &json, bool save_contents) const
{
    json.start_object();

    json.member( "typeid", type->id );
    json.member("posx",_posx);
    json.member("posy",_posy);
    json.member("wandx",wandx);
    json.member("wandy",wandy);
    json.member("wandf",wandf);
    json.member("moves",moves);
    json.member("speed",speed);
    json.member("hp",hp);
    json.member("sp_timeout",sp_timeout);
    json.member("friendly",friendly);
    json.member("faction_id",faction_id);
    json.member("mission_id",mission_id);
    json.member("no_extra_death_drops", no_extra_death_drops );
    json.member("dead",dead);
    json.member("anger",anger);
    json.member("morale",morale);
    json.member("hallucination",hallucination);
    json.member("onstairs",onstairs);
    json.member("stairscount",staircount); // really...  re: exploitable if not.
    json.member("plans", plans);
    json.member("ammo", ammo);


    if ( save_contents ) {
        json.member("inv");
        json.start_array();
        for(int i=0;i<inv.size();i++) {
            inv[i].serialize(json, true);
        }
        json.end_array();
    }

    json.end_object();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// item.h

void item::deserialize(JsonObject &data)
{
    init();
    clear();

    std::string idtmp="";
    std::string ammotmp="null";
    int lettmp = 0;
    std::string corptmp = "null";
    int damtmp = 0;

    if ( ! data.read_into( "typeid", idtmp) ) {
        debugmsg("Invalid item type: %s ", data.str().c_str() );
        idtmp = "null";
    }

    data.read_into( "charges", charges );
    data.read_into( "burnt", burnt );
    data.read_into( "poison", poison );
    data.read_into( "owned", owned );

    data.read_into( "bday", bday );

    data.read_into( "mode", mode );
    data.read_into( "mission_id", mission_id );
    data.read_into( "player_id", player_id );

    if (!data.read_into( "corpse", corptmp )) {
        int ctmp = -1;
        data.read_into( "corpse", ctmp );
        if (ctmp != -1) {
            corptmp = legacy_mon_id[ctmp];
        } else {
            corptmp = "null";
        }
    }
    if (corptmp != "null") {
        corpse = GetMType(corptmp);
    } else {
        corpse = NULL;
    }

    make(itypes[idtmp]);

    if ( ! data.read_into( "name", name ) ) {
        name=type->name;
    }

    data.read_into( "invlet", lettmp );
    invlet = char(lettmp);

    data.read_into( "damage", damtmp );
    damage = damtmp; // todo: check why this is done after make(), using a tmp variable
    data.read_into( "active", active );


    data.read_into( "curammo", ammotmp );
    if ( ammotmp != "null" ) {
        curammo = dynamic_cast<it_ammo*>(itypes[ammotmp]);
    } else {
        curammo = NULL;
    }

    data.read_into("item_tags", item_tags);

    JsonObject pvars = data.get_object("item_vars");
    std::set<std::string> members = pvars.get_member_names();
    for ( std::set<std::string>::iterator pvarsit = members.begin();
            pvarsit != members.end(); ++pvarsit ) {
        if ( pvars.has_string( *pvarsit ) ) {
            item_vars[ *pvarsit ] = pvars.get_string( *pvarsit );
        }
    }

    int tmplum=0;
    if ( data.read_into("light",tmplum) ) {

        light=nolight;
        int tmpwidth=0;
        int tmpdir=0;

        data.read_into("light_width",tmpwidth);
        data.read_into("light_dir",tmpdir);
        light.luminance = tmplum;
        light.width = (short)tmpwidth;
        light.direction = (short)tmpdir;
    }

    data.read_into("contents", contents);
}

void item::serialize(JsonOut &json, bool save_contents) const
{
    json.start_object();

    /////
    if (type == NULL) {
        debugmsg("Tried to save an item with NULL type!");
    }
    itype_id ammotmp = "null";

    /* TODO: This causes a segfault sometimes, even though we check to make sure
     * curammo isn't NULL.  The crashes seem to occur most frequently when saving an
     * NPC, or when saving map data containing an item an NPC has dropped.
     */
    if (curammo != NULL) {
        ammotmp = curammo->id;
    }
    if( std::find(unreal_itype_ids.begin(), unreal_itype_ids.end(),
                  ammotmp) != unreal_itype_ids.end()  &&
        std::find(artifact_itype_ids.begin(), artifact_itype_ids.end(),
                  ammotmp) != artifact_itype_ids.end()
      ) {
        ammotmp = "null"; //Saves us from some bugs, apparently?
    }

    /////
    json.member( "invlet", int(invlet) );
    json.member( "typeid", typeId() );
    json.member( "bday", bday );

    if ( charges != -1 )     json.member( "charges", int(charges) );
    if ( damage != 0 )       json.member( "damage", int(damage) );
    if ( burnt != 0 )        json.member( "burnt", burnt );
    if ( poison != 0 )       json.member( "poison", poison );
    if ( ammotmp != "null" ) json.member( "curammo", ammotmp );
    if ( mode != "NULL" )    json.member( "mode", mode );
    if ( active == true )    json.member( "active", true );
    if ( corpse != NULL )    json.member( "corpse", corpse->id );

    if ( owned != -1 )       json.member( "owned", owned );
    if ( player_id != -1 )   json.member( "player_id", player_id );
    if ( mission_id != -1 )  json.member( "mission_id", mission_id );

    if ( ! item_tags.empty() ) {
        json.member( "item_tags", item_tags );
    }

    if ( ! item_vars.empty() ) {
        json.member("item_vars");
        json.start_object();
        for (std::map<std::string,std::string>::const_iterator it = item_vars.begin();
                it != item_vars.end(); ++it) {
            json.member( it->first, it->second );
        }
        json.end_object();
    }

    if ( name != type->name ) {
        json.member( "name", name );
    }

    if ( light.luminance != 0 ) {
        json.member( "light", int(light.luminance) );
        if ( light.width != 0 ) {
            json.member( "light_width", int(light.width) );
            json.member( "light_dir", int(light.direction) );
        }
    }

    if ( save_contents && contents.size() > 0 ) {
        json.member("contents");
        json.start_array();
        for (int k = 0; k < contents.size(); k++) {
            contents[k].serialize(json, false); // no matryoshka dolls
        }
        json.end_array();
    }

    json.end_object();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// vehicle.h

/*
 * vehicle_part
 */
void vehicle_part::deserialize(JsonIn &jsin)
{
    JsonObject data = jsin.get_object();
    int intpid;
    std::string pid;
    if ( data.read_into("id_enum", intpid) && intpid < 74 ) {
        pid = legacy_vpart_id[intpid];
    } else {
        data.read_into("id",pid);
    }
    // if we don't know what type of part it is, it'll cause problems later.
    if (vehicle_part_types.find(pid) == vehicle_part_types.end()) {
        if (pid == "wheel_underbody") {
            pid = "wheel_wide";
        } else {
            throw (std::string)"bad vehicle part, id: %s" + pid;
        }
    }
    id = pid;
    data.read_into("mount_dx", mount_dx);
    data.read_into("mount_dy", mount_dy);
    data.read_into("hp", hp );
    data.read_into("amount", amount );
    data.read_into("blood", blood );
    data.read_into("bigness", bigness );
    data.read_into( "flags", flags );
    data.read_into( "passenger_id", passenger_id );
    data.read_into("items", items);
}

void vehicle_part::serialize(JsonOut &json) const
{
    json.start_object();
    json.member("id", id);
    json.member("mount_dx", mount_dx);
    json.member("mount_dy", mount_dy);
    json.member("hp", hp);
    json.member("amount", amount);
    json.member("blood", blood);
    json.member("bigness", bigness);
    json.member("flags", flags);
    json.member("passenger_id", passenger_id);
    json.member("items", items);
    json.end_object();
}

/*
 * Load vehicle from a json blob that might just exceed player in size.
 */
void vehicle::deserialize(JsonIn &jsin)
{
    JsonObject data = jsin.get_object();

    int fdir, mdir;

    data.read_into("type", type);
    data.read_into("posx", posx);
    data.read_into("posy", posy);
    data.read_into("levx", levx);
    data.read_into("levy", levy);
    data.read_into("om_id", om_id);
    data.read_into("faceDir", fdir);
    data.read_into("moveDir", mdir);
    data.read_into("turn_dir", turn_dir);
    data.read_into("velocity", velocity);
    data.read_into("cruise_velocity", cruise_velocity);
    data.read_into("cruise_on", cruise_on);
    data.read_into("tracking_on", tracking_on);
    data.read_into("lights_on", lights_on);
    data.read_into("overhead_lights_on", overhead_lights_on);
    data.read_into("skidding", skidding);
    data.read_into("turret_mode", turret_mode);
    data.read_into("of_turn_carry", of_turn_carry);

    face.init (fdir);
    move.init (mdir);
    data.read_into("name",name);

    data.read_into("parts", parts);

    /* After loading, check if the vehicle is from the old rules and is missing
     * frames. */
    if ( savegame_loading_version < 11 ) {
        add_missing_frames();
    }
    find_horns ();
    find_lights ();
    find_fuel_tanks ();
    find_exhaust ();
    insides_dirty = true;
    precalc_mounts (0, face.dir());

    data.read_into("tags",tags);
}

void vehicle::serialize(JsonOut &json) const
{
    json.start_object();
    json.member( "type", type );
    json.member( "posx", posx );
    json.member( "posy", posy );
    json.member( "levx", levx );
    json.member( "levy", levy );
    json.member( "om_id", om_id );
    json.member( "faceDir", face.dir() );
    json.member( "moveDir", move.dir() );
    json.member( "turn_dir", turn_dir );
    json.member( "velocity", velocity );
    json.member( "cruise_velocity", cruise_velocity );
    json.member( "cruise_on", cruise_on );
    json.member( "tracking_on", tracking_on );
    json.member( "lights_on", lights_on );
    json.member( "overhead_lights_on", overhead_lights_on );
    json.member( "skidding", skidding );
    json.member( "turret_mode", turret_mode );
    json.member( "of_turn_carry", of_turn_carry );
    json.member( "name", name );
    json.member( "parts", parts );
    json.member( "tags", tags );
    json.end_object();
}

////////////////// mission.h
////
void mission::deserialize(JsonIn &jsin)
{
    JsonObject jo = jsin.get_object();

    if (jo.has_member("type_id")) {
        type = &(g->mission_types[jo.get_int("type_id")]);
    }
    jo.read_into("description", description);
    jo.read_into("failed", failed);
    jo.read_into("value", value);
    jo.read_into("reward", reward);
    jo.read_into("uid", uid );
    JsonArray ja = jo.get_array("target");
    if (ja.size() == 2) {
        target.x = ja.get_int(0);
        target.y = ja.get_int(1);
    }
    follow_up = mission_id(jo.get_int("follow_up", follow_up));
    item_id = itype_id(jo.get_string("item_id", item_id));
    jo.read_into("deadline", deadline );
    jo.read_into("step", step );
    jo.read_into("count", count );
    jo.read_into("npc_id", npc_id );
    jo.read_into("good_fac_id", good_fac_id );
    jo.read_into("bad_fac_id", bad_fac_id );
}

void mission::serialize(JsonOut &json) const
{
    json.start_object();

    json.member("type_id", type == NULL ? -1 : (int)type->id);
    json.member("description", description);
    json.member("failed", failed);
    json.member("value", value);
    json.member("reward", reward);
    json.member("uid", uid);

    json.member("target");
    json.start_array();
    json.write(target.x);
    json.write(target.y);
    json.end_array();

    json.member("count", count);
    json.member("deadline", deadline);
    json.member("npc_id", npc_id);
    json.member("good_fac_id", good_fac_id);
    json.member("bad_fac_id", bad_fac_id);
    json.member("step", step);
    json.member("follow_up", (int)follow_up);

    json.end_object();
}

////////////////// faction.h
////
void faction::deserialize(JsonIn &jsin)
{
    JsonObject jo = jsin.get_object();

    jo.read_into("id", id);
    jo.read_into("name", name);
    goal = faction_goal(jo.get_int("goal", goal));
    values = jo.get_int("values", values);
    job1 = faction_job(jo.get_int("job1", job1));
    job2 = faction_job(jo.get_int("job2", job2));
    jo.read_into("likes_u", likes_u);
    jo.read_into("respects_u", respects_u);
    jo.read_into("known_by_u", known_by_u);
    jo.read_into("strength", strength);
    jo.read_into("sneak", sneak);
    jo.read_into("crime", crime);
    jo.read_into("cult", cult);
    jo.read_into("good", good);
    jo.read_into("omx", omx);
    jo.read_into("omy", omy);
    jo.read_into("mapx", mapx);
    jo.read_into("mapy", mapy);
    jo.read_into("size", size);
    jo.read_into("power", power);
    if (jo.has_array("opinion_of")) {
        opinion_of = jo.get_int_array("opinion_of");
    }
}

void faction::serialize(JsonOut &json) const
{
    json.start_object();

    json.member("id", id);
    json.member("name", name);
    json.member("values", values);
    json.member("goal", goal);
    json.member("job1", job1);
    json.member("job2", job2);
    json.member("likes_u", likes_u);
    json.member("respects_u", respects_u);
    json.member("known_by_u", known_by_u);
    json.member("strength", strength);
    json.member("sneak", sneak);
    json.member("crime", crime);
    json.member("cult", cult);
    json.member("good", good);
    json.member("omx", omx);
    json.member("omy", omy);
    json.member("mapx", mapx);
    json.member("mapy", mapy);
    json.member("size", size);
    json.member("power", power);
    json.member("opinion_of", opinion_of);

    json.end_object();
}

