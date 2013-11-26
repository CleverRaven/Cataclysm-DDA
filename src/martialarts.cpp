#include "player.h"
#include "martialarts.h"
#include "json.h"
#include "translations.h"
#include <map>
#include <string>

std::map<matype_id, martialart> martialarts;
std::map<mabuff_id, ma_buff> ma_buffs;
std::map<matec_id, ma_technique> ma_techniques;

std::map<std::string, technique_id> tech_id_lookup;


void load_technique(JsonObject &jo)
{
    ma_technique tec;

    tec.id = jo.get_string("id");
    //tec.name = _(jo.get_string("name").c_str());

    JsonArray jsarr = jo.get_array("messages");
    while (jsarr.has_more()) {
        tec.messages.push_back(_(jsarr.next_string().c_str()));
    }

    tec.reqs.unarmed_allowed = jo.get_bool("unarmed_allowed", false);
    tec.reqs.melee_allowed = jo.get_bool("melee_allowed", false);
    tec.reqs.min_melee = jo.get_int("min_melee", 0);
    tec.reqs.min_unarmed = jo.get_int("min_unarmed", 0);
    tec.reqs.req_buffs = jo.get_tags("req_buffs");

    tec.crit_tec = jo.get_bool("crit_tec", false);
    tec.defensive = jo.get_bool("defensive", false);
    tec.disarms = jo.get_bool("disarms", false);
    tec.grabs = jo.get_bool("grabs", false);
    tec.counters = jo.get_bool("counters", false);
    tec.miss_recovery = jo.get_bool("miss_recovery", false);
    tec.grab_break = jo.get_bool("grab_break", false);
    tec.flaming = jo.get_bool("flaming", false);
    tec.quick = jo.get_bool("quick", false);

    tec.down_dur = jo.get_int("down_dur", 0);
    tec.stun_dur = jo.get_int("stun_dur", 0);
    tec.knockback_dist = jo.get_int("knockback_dist", 0);
    tec.knockback_spread = jo.get_int("knockback_spread", 0);

    tec.aoe = jo.get_string("aoe", "");
    tec.flags = jo.get_tags("flags");

    ma_techniques[tec.id] = tec;
}

ma_buff load_buff(JsonObject &jo)
{
    ma_buff buff;

    buff.id = jo.get_string("id");

    buff.name = _(jo.get_string("name").c_str());
    buff.description = _(jo.get_string("description").c_str());

    buff.buff_duration = jo.get_int("buff_duration", 2);
    buff.max_stacks = jo.get_int("max_stacks", 1);

    buff.reqs.unarmed_allowed = jo.get_bool("unarmed_allowed", false);
    buff.reqs.melee_allowed = jo.get_bool("melee_allowed", false);

    buff.reqs.min_melee = jo.get_int("min_melee", 0);
    buff.reqs.min_unarmed = jo.get_int("min_unarmed", 0);

    buff.dodges_bonus = jo.get_int("bonus_dodges", 0);
    buff.blocks_bonus = jo.get_int("bonus_blocks", 0);

    buff.hit = jo.get_int("hit", 0);
    buff.bash = jo.get_int("bash", 0);
    buff.cut = jo.get_int("cut", 0);
    buff.dodge = jo.get_int("dodge", 0);
    buff.speed = jo.get_int("speed", 0);
    buff.block = jo.get_int("block", 0);

    buff.bash_stat_mult = jo.get_float("bash_mult", 1.0);
    buff.cut_stat_mult = jo.get_float("cut_mult", 1.0);

    buff.hit_str = jo.get_float("hit_str", 0.0);
    buff.hit_dex = jo.get_float("hit_dex", 0.0);
    buff.hit_int = jo.get_float("hit_int", 0.0);
    buff.hit_per = jo.get_float("hit_per", 0.0);

    buff.bash_str = jo.get_float("bash_str", 0.0);
    buff.bash_dex = jo.get_float("bash_dex", 0.0);
    buff.bash_int = jo.get_float("bash_int", 0.0);
    buff.bash_per = jo.get_float("bash_per", 0.0);

    buff.cut_str = jo.get_float("cut_str", 0.0);
    buff.cut_dex = jo.get_float("cut_dex", 0.0);
    buff.cut_int = jo.get_float("cut_int", 0.0);
    buff.cut_per = jo.get_float("cut_per", 0.0);

    buff.dodge_str = jo.get_float("dodge_str", 0.0);
    buff.dodge_dex = jo.get_float("dodge_dex", 0.0);
    buff.dodge_int = jo.get_float("dodge_int", 0.0);
    buff.dodge_per = jo.get_float("dodge_per", 0.0);

    buff.block_str = jo.get_float("block_str", 0.0);
    buff.block_dex = jo.get_float("block_dex", 0.0);
    buff.block_int = jo.get_float("block_int", 0.0);
    buff.block_per = jo.get_float("block_per", 0.0);

    buff.quiet = jo.get_bool("quiet", false);
    buff.throw_immune = jo.get_bool("throw_immune", false);

    buff.reqs.req_buffs = jo.get_tags("req_buffs");

    ma_buffs[buff.id] = buff;

    return buff;
}

void init_martial_arts() {
    // set up lookup tables for techniques
    tech_id_lookup["SWEEP"] = TEC_SWEEP;
    tech_id_lookup["PRECISE"] = TEC_PRECISE;
    tech_id_lookup["BRUTAL"] = TEC_BRUTAL;
    tech_id_lookup["GRAB"] = TEC_GRAB;
    tech_id_lookup["WIDE"] = TEC_WIDE;
    tech_id_lookup["RAPID"] = TEC_RAPID;
    tech_id_lookup["FEINT"] = TEC_FEINT;
    tech_id_lookup["THROW"] = TEC_THROW;
    tech_id_lookup["DISARM"] = TEC_DISARM;
    tech_id_lookup["FLAMING"] = TEC_FLAMING;

    tech_id_lookup["BLOCK"] = TEC_BLOCK;
    tech_id_lookup["BLOCK_LEGS"] = TEC_BLOCK_LEGS;
    tech_id_lookup["WBLOCK_1"] = TEC_WBLOCK_1;
    tech_id_lookup["WBLOCK_2"] = TEC_WBLOCK_2;
    tech_id_lookup["WBLOCK_3"] = TEC_WBLOCK_3;
    tech_id_lookup["COUNTER"] = TEC_COUNTER;
    tech_id_lookup["BREAK"] = TEC_BREAK;
    tech_id_lookup["DEF_THROW"] = TEC_DEF_THROW;
    tech_id_lookup["DEF_DISARM"] = TEC_DEF_DISARM;
}

void load_martial_art(JsonObject &jo)
{
    martialart ma;
    JsonArray jsarr;

    ma.id = jo.get_string("id");
    ma.name = _(jo.get_string("name").c_str());
    ma.description = _(jo.get_string("description").c_str());

    jsarr = jo.get_array("static_buffs");
    while (jsarr.has_more()) {
        JsonObject jsobj = jsarr.next_object();
        ma.static_buffs.push_back(load_buff(jsobj));
    }

    jsarr = jo.get_array("onhit_buffs");
    while (jsarr.has_more()) {
        JsonObject jsobj = jsarr.next_object();
        ma.onhit_buffs.push_back(load_buff(jsobj));
    }

    jsarr = jo.get_array("onmove_buffs");
    while (jsarr.has_more()) {
        JsonObject jsobj = jsarr.next_object();
        ma.onmove_buffs.push_back(load_buff(jsobj));
    }

    ma.techniques = jo.get_tags("techniques");

    ma.leg_block = jo.get_int("leg_block", 0);
    ma.arm_block = jo.get_int("arm_block", 0);

    martialarts[ma.id] = ma;
}


bool ma_requirements::is_valid_player(player& u) {
  for (std::set<mabuff_id>::iterator it = req_buffs.begin();
      it != req_buffs.end(); ++it) {
    mabuff_id buff_id = *it;
    if (!u.has_mabuff(*it)) return false;
  }
  return ((unarmed_allowed && u.unarmed_attack()) || (melee_allowed && !u.unarmed_attack()))
    && u.skillLevel("melee") >= min_melee
    && u.skillLevel("unarmed") >= min_unarmed
    && u.skillLevel("bashing") >= min_bashing
    && u.skillLevel("cutting") >= min_cutting
    && u.skillLevel("stabbing") >= min_stabbing
  ;
}


ma_technique::ma_technique() {

  crit_tec = false;
  defensive = false;

  down_dur = 0;
  stun_dur = 0;
  knockback_dist = 0;
  knockback_spread = 0; // adding randomness to knockback, like tec_throw

  // offensive
  disarms = false; // like tec_disarm
  grabs = false; // like tec_grab
  counters = false; // like tec_counter

  miss_recovery = false; // allows free recovery from misses, like tec_feint
  grab_break = false; // allows grab_breaks, like tec_break

  flaming = false; // applies fire effects etc
  quick = false; // moves discount based on attack speed, like tec_rapid

  hit = 0; // flat bonus to hit
  bash = 0; // flat bonus to bash
  cut = 0; // flat bonus to cut
  pain = 0; // causes pain

  bash_mult = 1.0f; // bash damage multiplier
  cut_mult = 1.0f; // cut damage multiplier

  //defensive
  block = 0;

  bash_resist = 0.0f; // multiplies bash by this (1 - amount)
  cut_resist = 0.0f; // "" cut ""

}

bool ma_technique::is_valid_player(player& u) {
  return reqs.is_valid_player(u);
}




ma_buff::ma_buff() {

  buff_duration = 2; // total length this buff lasts
  max_stacks = 1; // total number of stacks this buff can have

  dodges_bonus = 0; // extra dodges, like karate
  blocks_bonus = 0; // extra blocks, like karate

  hit = 0; // flat bonus to hit
  bash = 0; // flat bonus to bash
  cut = 0; // flat bonus to cut
  dodge = 0; // flat dodge bonus
  speed = 0; // flat speed bonus

  bash_stat_mult = 1.f; // bash damage multiplier, like aikido
  cut_stat_mult = 1.f; // cut damage multiplier

  bash_str = 0.f; // bonus damage to add per str point
  bash_dex = 0.f; // "" dex point
  bash_int = 0.f; // "" int point
  bash_per = 0.f; // "" per point

  cut_str = 0.f; // bonus cut damage to add per str point
  cut_dex = 0.f; // "" dex point
  cut_int = 0.f; // "" int point
  cut_per = 0.f; // "" per point

  hit_str = 0.f; // bonus to-hit to add per str point
  hit_dex = 0.f; // "" dex point
  hit_int = 0.f; // "" int point
  hit_per = 0.f; // "" per point

  dodge_str = 0.f; // bonus dodge to add per str point
  dodge_dex = 0.f; // "" dex point
  dodge_int = 0.f; // "" int point
  dodge_per = 0.f; // "" per point

  throw_immune = false;

}

void ma_buff::apply_buff(std::vector<disease>& dVec) {
  for (std::vector<disease>::iterator it = dVec.begin();
      it != dVec.end(); ++it) {
    if (it->is_mabuff() && it->buff_id == id) {
        it->duration = buff_duration;
        it->intensity++;
        if (it->intensity > max_stacks) it->intensity = max_stacks;
        return;
    }
  }
  disease d(id);
  d.duration = buff_duration;
  d.intensity = 1;
  dVec.push_back(d);
}

bool ma_buff::is_valid_player(player& u) {
  return reqs.is_valid_player(u);
}

void ma_buff::apply_player(player& u) {
  u.dodges_left += dodges_bonus;
  u.blocks_left += blocks_bonus;
}

int ma_buff::hit_bonus(player& u) {
  return hit + u.str_cur*hit_str +
         u.dex_cur*hit_dex +
         u.int_cur*hit_int +
         u.per_cur*hit_per;
}
int ma_buff::dodge_bonus(player& u) {
  return dodge + u.str_cur*dodge_str +
         u.dex_cur*dodge_dex +
         u.int_cur*dodge_int +
         u.per_cur*dodge_per;
}
int ma_buff::block_bonus(player& u) {
  return block + u.str_cur*block_str +
         u.dex_cur*block_dex +
         u.int_cur*block_int +
         u.per_cur*block_per;
}
int ma_buff::speed_bonus(player& u)
{
    (void)u; //unused
    return speed;
}
float ma_buff::bash_mult() {
  return bash_stat_mult;
}
int ma_buff::bash_bonus(player& u) {
  return bash + u.str_cur*bash_str +
         u.dex_cur*bash_dex +
         u.int_cur*bash_int +
         u.per_cur*bash_per;
}
float ma_buff::cut_mult() {
  return cut_stat_mult;
}
int ma_buff::cut_bonus(player& u) {
  return cut + u.str_cur*cut_str +
         u.dex_cur*cut_dex +
         u.int_cur*cut_int +
         u.per_cur*cut_per;
}
bool ma_buff::is_throw_immune() {
  return throw_immune;
}
bool ma_buff::is_quiet() {
  return quiet;
}

bool ma_buff::can_melee() {
  return melee_allowed;
}



martialart::martialart() {
  leg_block = -1;
  arm_block = -1;
}

// simultaneously check and add all buffs. this is so that buffs that have
// buff dependencies added by the same event trigger correctly
void simultaneous_add(player& u, std::vector<ma_buff>& buffs, std::vector<disease>& dVec) {
  std::vector<ma_buff> buffer; // hey get it because it's for buffs????
  for (std::vector<ma_buff>::iterator it = buffs.begin();
      it != buffs.end(); ++it) {
    if (it->is_valid_player(u)) {
      buffer.push_back(*it);
    }
  }
  for (std::vector<ma_buff>::iterator it = buffer.begin();
      it != buffer.end(); ++it) {
    it->apply_buff(dVec);
  }
}

void martialart::apply_static_buffs(player& u, std::vector<disease>& dVec) {
  simultaneous_add(u, static_buffs, dVec);
}

void martialart::apply_onhit_buffs(player& u, std::vector<disease>& dVec) {
  simultaneous_add(u, onhit_buffs, dVec);
}

void martialart::apply_onmove_buffs(player& u, std::vector<disease>& dVec) {
  simultaneous_add(u, onmove_buffs, dVec);
}

void martialart::apply_ondodge_buffs(player& u, std::vector<disease>& dVec) {
  simultaneous_add(u, ondodge_buffs, dVec);
}

bool martialart::has_technique(player& u, matec_id tec_id) {
  for (std::set<matec_id>::iterator it = techniques.begin();
      it != techniques.end(); ++it) {
    ma_technique tec = ma_techniques[*it];
    if (tec.is_valid_player(u) && tec.id == tec_id) {
      return true;
    }
  }
  return false;
}

std::string martialart::melee_verb(matec_id tec_id, player& u) {
  for (std::set<matec_id>::iterator it = techniques.begin();
      it != techniques.end(); ++it) {
    ma_technique tec = ma_techniques[*it];
    if (tec.id == tec_id) {
      if (u.is_npc())
        return tec.messages[1];
      else
        return tec.messages[0];
    }
  }
  return std::string("%s is attacked by bugs");
}



// Player stuff

// technique
std::vector<matec_id> player::get_all_techniques() {
  std::vector<matec_id> tecs;
  tecs.insert(tecs.end(), weapon.type->techniques.begin(), weapon.type->techniques.end());
  tecs.insert(tecs.end(), martialarts[style_selected].techniques.begin(),
      martialarts[style_selected].techniques.end());

  return tecs;
}

// defensive technique-related
bool player::has_miss_recovery_tec() {
  std::vector<matec_id> techniques = get_all_techniques();
  for (std::vector<matec_id>::iterator it = techniques.begin();
      it != techniques.end(); ++it) {
    if (ma_techniques[*it].miss_recovery == true)
      return true;
  }
  return false;
}

bool player::has_grab_break_tec() {
  std::vector<matec_id> techniques = get_all_techniques();
  for (std::vector<matec_id>::iterator it = techniques.begin();
      it != techniques.end(); ++it) {
    if (ma_techniques[*it].grab_break == true)
      return true;
  }
  return false;
}

bool player::can_leg_block() {
  if (martialarts[style_selected].leg_block < 1)
    return false;
  if (skillLevel("unarmed") >= martialarts[style_selected].leg_block &&
      (hp_cur[hp_leg_l] > 0 || hp_cur[hp_leg_l] > 0))
    return true;
  else
    return false;
}

bool player::can_arm_block() {
  if (martialarts[style_selected].arm_block < 1)
    return false;
  if (skillLevel("unarmed") >= martialarts[style_selected].arm_block &&
      (hp_cur[hp_arm_l] > 0 || hp_cur[hp_arm_l] > 0))
    return true;
  else
    return false;
}

bool player::can_block() {
  return can_arm_block() || can_leg_block();
}

// event handlers
void player::ma_static_effects() {
  martialarts[style_selected].apply_static_buffs(*this, illness);
}
void player::ma_onmove_effects() {
  martialarts[style_selected].apply_onmove_buffs(*this, illness);
}
void player::ma_onhit_effects() {
  martialarts[style_selected].apply_onhit_buffs(*this, illness);
}
// ondodge doesn't actually work yet
void player::ma_ondodge_effects() {
  martialarts[style_selected].apply_ondodge_buffs(*this, illness);
}

// bonuses
int player::mabuff_tohit_bonus() {
  int ret = 0;
  for (std::vector<disease>::iterator it = illness.begin();
      it != illness.end(); ++it) {
    if (it->is_mabuff() &&
        ma_buffs.find(it->buff_id) != ma_buffs.end()) {
      ret += ma_buffs[it->buff_id].hit_bonus(*this);
    }
  }
  return ret;
}
int player::mabuff_dodge_bonus() {
  int ret = 0;
  for (std::vector<disease>::iterator it = illness.begin();
      it != illness.end(); ++it) {
    if (it->is_mabuff() &&
        ma_buffs.find(it->buff_id) != ma_buffs.end()) {
      ret += it->intensity * ma_buffs[it->buff_id].dodge_bonus(*this);
    }
  }
  return ret;
}
int player::mabuff_block_bonus() {
  int ret = 0;
  for (std::vector<disease>::iterator it = illness.begin();
      it != illness.end(); ++it) {
    if (it->is_mabuff() &&
        ma_buffs.find(it->buff_id) != ma_buffs.end()) {
      ret += it->intensity * ma_buffs[it->buff_id].block_bonus(*this);
    }
  }
  return ret;
}
int player::mabuff_speed_bonus() {
  int ret = 0;
  for (std::vector<disease>::iterator it = illness.begin();
      it != illness.end(); ++it) {
    if (it->is_mabuff() &&
        ma_buffs.find(it->buff_id) != ma_buffs.end()) {
      ret += it->intensity * ma_buffs[it->buff_id].speed_bonus(*this);
    }
  }
  return ret;
}
float player::mabuff_bash_mult() {
  float ret = 1.f;
  for (std::vector<disease>::iterator it = illness.begin();
      it != illness.end(); ++it) {
    if (it->is_mabuff() &&
        ma_buffs.find(it->buff_id) != ma_buffs.end()) {
      ret *= it->intensity * (1-ma_buffs[it->buff_id].bash_mult())+1;
    }
  }
  return ret;
}
int player::mabuff_bash_bonus() {
  int ret = 0;
  for (std::vector<disease>::iterator it = illness.begin();
      it != illness.end(); ++it) {
    if (it->is_mabuff() &&
        ma_buffs.find(it->buff_id) != ma_buffs.end()) {
      ret += it->intensity * ma_buffs[it->buff_id].bash_bonus(*this);
    }
  }
  return ret;
}
float player::mabuff_cut_mult() {
  float ret = 1.f;
  for (std::vector<disease>::iterator it = illness.begin();
      it != illness.end(); ++it) {
    if (it->is_mabuff() &&
        ma_buffs.find(it->buff_id) != ma_buffs.end()) {
      ret *= it->intensity * (1-ma_buffs[it->buff_id].cut_mult())+1;
    }
  }
  return ret;
}
int player::mabuff_cut_bonus() {
  int ret = 0;
  for (std::vector<disease>::iterator it = illness.begin();
      it != illness.end(); ++it) {
    if (it->is_mabuff() &&
        ma_buffs.find(it->buff_id) != ma_buffs.end()) {
      ret += it->intensity * ma_buffs[it->buff_id].cut_bonus(*this);
    }
  }
  return ret;
}
bool player::is_throw_immune() {
  for (std::vector<disease>::iterator it = illness.begin();
      it != illness.end(); ++it) {
    if (it->is_mabuff() &&
        ma_buffs.find(it->buff_id) != ma_buffs.end()) {
      if (ma_buffs[it->buff_id].is_throw_immune()) return true;
    }
  }
  return false;
}
bool player::is_quiet() {
  for (std::vector<disease>::iterator it = illness.begin();
      it != illness.end(); ++it) {
    if (it->is_mabuff() &&
        ma_buffs.find(it->buff_id) != ma_buffs.end()) {
      if (ma_buffs[it->buff_id].is_quiet()) return true;
    }
  }
  return false;
}

bool player::can_melee() {
  for (std::vector<disease>::iterator it = illness.begin();
      it != illness.end(); ++it) {
    if (it->is_mabuff() &&
        ma_buffs.find(it->buff_id) != ma_buffs.end()) {
      if (ma_buffs[it->buff_id].can_melee()) return true;
    }
  }
  return false;
}

bool player::has_mabuff(mabuff_id id) {
  for (std::vector<disease>::iterator it = illness.begin();
      it != illness.end(); ++it) {
    if (it->is_mabuff() && it->buff_id == id) {
      return true;
    }
  }
  return false;
}
