#include "game.h"
#include "player.h"
#include "martialarts.h"
#include "catajson.h"
#include <map>
#include <string>

std::map<std::string, technique_id> tech_id_lookup;


ma_technique loadTec(catajson& curTec) {
  ma_technique tec;

  tec.id = curTec.get("id").as_string();

  if (curTec.has("unarmed_allowed"))
    tec.reqs.unarmed_allowed = curTec.get("unarmed_allowed").as_bool();
  if (curTec.has("melee_allowed"))
    tec.reqs.melee_allowed = curTec.get("melee_allowed").as_bool();

  if (curTec.has("min_melee"))
    tec.reqs.min_melee = curTec.get("min_melee").as_int();
  if (curTec.has("min_unarmed"))
    tec.reqs.min_unarmed = curTec.get("min_unarmed").as_int();

  if (curTec.has("verb_you"))
    tec.verb_you = curTec.get("verb_you").as_string();
  if (curTec.has("verb_npc"))
    tec.verb_npc = curTec.get("verb_npc").as_string();

  if (curTec.has("flags"))
    tec.flags = curTec.get("flags").as_tags();

  if (curTec.has("req_buffs"))
    tec.reqs.req_buffs = curTec.get("req_buffs").as_tags();

  if (curTec.has("crit_tec"))
    tec.crit_tec = curTec.get("crit_tec").as_bool();
  if (curTec.has("defensive"))
    tec.defensive = curTec.get("defensive").as_bool();

  if (curTec.has("down_dur"))
    tec.down_dur = curTec.get("down_dur").as_int();
  if (curTec.has("stun_dur"))
    tec.stun_dur = curTec.get("stun_dur").as_int();
  if (curTec.has("knockback_dist"))
    tec.knockback_dist = curTec.get("knockback_dist").as_int();
  if (curTec.has("knockback_spread"))
    tec.knockback_spread = curTec.get("knockback_spread").as_int();

  if (curTec.has("disarms"))
    tec.disarms = curTec.get("disarms").as_bool();
  if (curTec.has("grabs"))
    tec.grabs = curTec.get("grabs").as_bool();
  if (curTec.has("counters"))
    tec.counters = curTec.get("counters").as_bool();

  if (curTec.has("miss_recovery"))
    tec.miss_recovery = curTec.get("miss_recovery").as_bool();
  if (curTec.has("grab_break"))
    tec.grab_break = curTec.get("grab_break").as_bool();

  if (curTec.has("flaming"))
    tec.flaming = curTec.get("flaming").as_bool();
  if (curTec.has("quick"))
    tec.quick = curTec.get("quick").as_bool();

  if (curTec.has("aoe"))
    tec.aoe = curTec.get("aoe").as_string();

  return tec;
}
void loadTecArray(game* g, std::vector<ma_technique>& tecArr, catajson& jsonObj) {
  for (jsonObj.set_begin(); jsonObj.has_curr() && json_good();
      jsonObj.next()) {
    catajson cur = jsonObj.curr();
    ma_technique tec = loadTec(cur);
    tecArr.push_back(tec);
    g->ma_techniques[tec.id] = tec;
  }
}

ma_buff loadBuff(catajson& curBuff) {
  ma_buff buff;

  buff.id = curBuff.get("id").as_string();
  if (curBuff.has("desc"))
    buff.desc = curBuff.get("desc").as_string();
  if (curBuff.has("name"))
    buff.name = curBuff.get("name").as_string();

  if (curBuff.has("buff_duration"))
    buff.buff_duration = curBuff.get("buff_duration").as_int();
  if (curBuff.has("max_stacks"))
    buff.max_stacks = curBuff.get("max_stacks").as_int();

  if (curBuff.has("unarmed_allowed"))
    buff.reqs.unarmed_allowed = curBuff.get("unarmed_allowed").as_bool();
  if (curBuff.has("melee_allowed"))
    buff.reqs.melee_allowed = curBuff.get("melee_allowed").as_bool();

  if (curBuff.has("min_melee"))
    buff.reqs.min_melee = curBuff.get("min_melee").as_int();
  if (curBuff.has("min_unarmed"))
    buff.reqs.min_unarmed = curBuff.get("min_unarmed").as_int();

  if (curBuff.has("bonus_dodges"))
    buff.dodges_bonus = curBuff.get("bonus_dodges").as_int();
  if (curBuff.has("bonus_blocks"))
    buff.blocks_bonus = curBuff.get("bonus_blocks").as_int();

  if (curBuff.has("hit"))
    buff.hit = curBuff.get("hit").as_int();
  if (curBuff.has("bash"))
    buff.bash = curBuff.get("bash").as_int();
  if (curBuff.has("cut"))
    buff.cut = curBuff.get("cut").as_int();
  if (curBuff.has("dodge"))
    buff.dodge = curBuff.get("dodge").as_int();
  if (curBuff.has("speed"))
    buff.speed = curBuff.get("speed").as_int();
  if (curBuff.has("block"))
    buff.block = curBuff.get("block").as_int();

  if (curBuff.has("bash_mult"))
    buff.bash_stat_mult = curBuff.get("bash_mult").as_double();
  if (curBuff.has("cut_mult"))
    buff.cut_stat_mult = curBuff.get("cut_mult").as_double();

  if (curBuff.has("hit_str"))
    buff.hit_str = curBuff.get("hit_str").as_double();
  if (curBuff.has("hit_dex"))
    buff.hit_dex = curBuff.get("hit_dex").as_double();
  if (curBuff.has("hit_int"))
    buff.hit_int = curBuff.get("hit_int").as_double();
  if (curBuff.has("hit_per"))
    buff.hit_per = curBuff.get("hit_per").as_double();

  if (curBuff.has("bash_str"))
    buff.bash_str = curBuff.get("bash_str").as_double();
  if (curBuff.has("bash_dex"))
    buff.bash_dex = curBuff.get("bash_dex").as_double();
  if (curBuff.has("bash_int"))
    buff.bash_int = curBuff.get("bash_int").as_double();
  if (curBuff.has("bash_per"))
    buff.bash_per = curBuff.get("bash_per").as_double();

  if (curBuff.has("cut_str"))
    buff.cut_str = curBuff.get("cut_str").as_double();
  if (curBuff.has("cut_dex"))
    buff.cut_dex = curBuff.get("cut_dex").as_double();
  if (curBuff.has("cut_int"))
    buff.cut_int = curBuff.get("cut_int").as_double();
  if (curBuff.has("cut_per"))
    buff.cut_per = curBuff.get("cut_per").as_double();

  if (curBuff.has("dodge_str"))
    buff.dodge_str = curBuff.get("dodge_str").as_double();
  if (curBuff.has("dodge_dex"))
    buff.dodge_dex = curBuff.get("dodge_dex").as_double();
  if (curBuff.has("dodge_int"))
    buff.dodge_int = curBuff.get("dodge_int").as_double();
  if (curBuff.has("dodge_per"))
    buff.dodge_per = curBuff.get("dodge_per").as_double();

  if (curBuff.has("block_str"))
    buff.block_str = curBuff.get("block_str").as_double();
  if (curBuff.has("block_dex"))
    buff.block_dex = curBuff.get("block_dex").as_double();
  if (curBuff.has("block_int"))
    buff.block_int = curBuff.get("block_int").as_double();
  if (curBuff.has("block_per"))
    buff.block_per = curBuff.get("block_per").as_double();

  if (curBuff.has("quiet"))
    buff.quiet = curBuff.get("quiet").as_bool();
  if (curBuff.has("throw_immune"))
    buff.throw_immune = curBuff.get("throw_immune").as_bool();

  if (curBuff.has("req_buffs"))
    buff.reqs.req_buffs = curBuff.get("req_buffs").as_tags();

  return buff;
}
void loadBuffArray(game* g, std::vector<ma_buff>& buffArr, catajson& jsonObj) {
  for (jsonObj.set_begin(); jsonObj.has_curr() && json_good();
      jsonObj.next()) {
    catajson cur = jsonObj.curr();
    ma_buff buff = loadBuff(cur);
    buffArr.push_back(buff);
    g->ma_buffs[buff.id] = buff;
  }
}

void game::init_techniques() {
  catajson techniquesRaw("data/raw/techniques.json");

  for (techniquesRaw.set_begin(); techniquesRaw.has_curr() && json_good();
      techniquesRaw.next()) {
    catajson cur = techniquesRaw.curr();
    ma_techniques[cur.get("id").as_string()] = loadTec(cur);
  }
}

void game::init_martialarts() {
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

  catajson martialartsRaw("data/raw/martialarts.json");

  for (martialartsRaw.set_begin(); martialartsRaw.has_curr() && json_good();
      martialartsRaw.next()) {
    catajson curMartialArt = martialartsRaw.curr();

    martialart ma;

    ma.id = curMartialArt.get("id").as_string();
    ma.name = curMartialArt.get("name").as_string();

    if( curMartialArt.has("static_buffs") ) {
      catajson staticBuffJson = curMartialArt.get("static_buffs");
      loadBuffArray(this, ma.static_buffs, staticBuffJson);
    }

    if( curMartialArt.has("onhit_buffs") ) {
      catajson hitBuffJson = curMartialArt.get("onhit_buffs");
      loadBuffArray(this, ma.onhit_buffs, hitBuffJson);
    }

    if( curMartialArt.has("onmove_buffs") ) {
      catajson moveBuffJson = curMartialArt.get("onmove_buffs");
      loadBuffArray(this, ma.onmove_buffs, moveBuffJson);
    }

    if( curMartialArt.has("techniques") ) {
      ma.techniques = curMartialArt.get("techniques").as_tags();
    }

    if( curMartialArt.has("can_leg_block") ) {
      ma.has_leg_block = curMartialArt.get("can_leg_block").as_bool();
    }

    martialarts[ma.id] = ma;
  }
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
int ma_buff::speed_bonus(player& u) {
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



martialart::martialart() {
  has_leg_block = false;
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

bool martialart::has_technique(player& u, matec_id tec_id, game* g) {
  for (std::set<matec_id>::iterator it = techniques.begin();
      it != techniques.end(); ++it) {
    ma_technique tec = g->ma_techniques[*it];
    if (tec.is_valid_player(u) && tec.id == tec_id) {
      return true;
    }
  }
  return false;
}

std::string martialart::melee_verb(matec_id tec_id, player& u, game* g) {
  for (std::set<matec_id>::iterator it = techniques.begin();
      it != techniques.end(); ++it) {
    ma_technique tec = g->ma_techniques[*it];
    if (tec.id == tec_id) {
      if (u.is_npc())
        return tec.verb_npc;
      else
        return tec.verb_you;
    }
  }
  return std::string("%1$s has bug program in %4$s!!!!");
}



// Player stuff

// technique
std::vector<matec_id> player::get_all_techniques(game* g) {
  std::vector<matec_id> tecs;
  tecs.insert(tecs.end(), weapon.type->techniques.begin(), weapon.type->techniques.end());
  tecs.insert(tecs.end(), g->martialarts[style_selected].techniques.begin(),
      g->martialarts[style_selected].techniques.end());

  return tecs;
}

// defensive technique-related
bool player::has_miss_recovery_tec(game* g) {
  std::vector<matec_id> techniques = get_all_techniques(g);
  for (std::vector<matec_id>::iterator it = techniques.begin();
      it != techniques.end(); ++it) {
    if (g->ma_techniques[*it].miss_recovery == true)
      return true;
  }
  return false;
}

bool player::has_grab_break_tec(game* g) {
  std::vector<matec_id> techniques = get_all_techniques(g);
  for (std::vector<matec_id>::iterator it = techniques.begin();
      it != techniques.end(); ++it) {
    if (g->ma_techniques[*it].grab_break == true)
      return true;
  }
  return false;
}

bool player::can_leg_block(game* g) {
  return g->martialarts[style_selected].has_leg_block;
}

// event handlers
void player::ma_static_effects() {
  g->martialarts[style_selected].apply_static_buffs(*this, illness);
}
void player::ma_onmove_effects() {
  g->martialarts[style_selected].apply_onmove_buffs(*this, illness);
}
void player::ma_onhit_effects() {
  g->martialarts[style_selected].apply_onhit_buffs(*this, illness);
}
// ondodge doesn't actually work yet
void player::ma_ondodge_effects() {
  g->martialarts[style_selected].apply_ondodge_buffs(*this, illness);
}

// bonuses
int player::mabuff_tohit_bonus() {
  int ret = 0;
  for (std::vector<disease>::iterator it = illness.begin();
      it != illness.end(); ++it) {
    if (it->is_mabuff() &&
        g->ma_buffs.find(it->buff_id) != g->ma_buffs.end()) {
      ret += g->ma_buffs[it->buff_id].hit_bonus(*this);
    }
  }
  return ret;
}
int player::mabuff_dodge_bonus() {
  int ret = 0;
  for (std::vector<disease>::iterator it = illness.begin();
      it != illness.end(); ++it) {
    if (it->is_mabuff() &&
        g->ma_buffs.find(it->buff_id) != g->ma_buffs.end()) {
      ret += it->intensity * g->ma_buffs[it->buff_id].dodge_bonus(*this);
    }
  }
  return ret;
}
int player::mabuff_block_bonus() {
  int ret = 0;
  for (std::vector<disease>::iterator it = illness.begin();
      it != illness.end(); ++it) {
    if (it->is_mabuff() &&
        g->ma_buffs.find(it->buff_id) != g->ma_buffs.end()) {
      ret += it->intensity * g->ma_buffs[it->buff_id].block_bonus(*this);
    }
  }
  return ret;
}
int player::mabuff_speed_bonus() {
  int ret = 0;
  for (std::vector<disease>::iterator it = illness.begin();
      it != illness.end(); ++it) {
    if (it->is_mabuff() &&
        g->ma_buffs.find(it->buff_id) != g->ma_buffs.end()) {
      ret += it->intensity * g->ma_buffs[it->buff_id].speed_bonus(*this);
    }
  }
  return ret;
}
float player::mabuff_bash_mult() {
  float ret = 1.f;
  for (std::vector<disease>::iterator it = illness.begin();
      it != illness.end(); ++it) {
    if (it->is_mabuff() &&
        g->ma_buffs.find(it->buff_id) != g->ma_buffs.end()) {
      ret *= it->intensity * (1-g->ma_buffs[it->buff_id].bash_mult())+1;
    }
  }
  return ret;
}
int player::mabuff_bash_bonus() {
  int ret = 0;
  for (std::vector<disease>::iterator it = illness.begin();
      it != illness.end(); ++it) {
    if (it->is_mabuff() &&
        g->ma_buffs.find(it->buff_id) != g->ma_buffs.end()) {
      ret += it->intensity * g->ma_buffs[it->buff_id].bash_bonus(*this);
    }
  }
  return ret;
}
float player::mabuff_cut_mult() {
  float ret = 1.f;
  for (std::vector<disease>::iterator it = illness.begin();
      it != illness.end(); ++it) {
    if (it->is_mabuff() &&
        g->ma_buffs.find(it->buff_id) != g->ma_buffs.end()) {
      ret *= it->intensity * (1-g->ma_buffs[it->buff_id].cut_mult())+1;
    }
  }
  return ret;
}
int player::mabuff_cut_bonus() {
  int ret = 0;
  for (std::vector<disease>::iterator it = illness.begin();
      it != illness.end(); ++it) {
    if (it->is_mabuff() &&
        g->ma_buffs.find(it->buff_id) != g->ma_buffs.end()) {
      ret += it->intensity * g->ma_buffs[it->buff_id].cut_bonus(*this);
    }
  }
  return ret;
}
bool player::is_throw_immune() {
  for (std::vector<disease>::iterator it = illness.begin();
      it != illness.end(); ++it) {
    if (it->is_mabuff() &&
        g->ma_buffs.find(it->buff_id) != g->ma_buffs.end()) {
      if (g->ma_buffs[it->buff_id].is_throw_immune()) return true;
    }
  }
  return false;
}
bool player::is_quiet() {
  for (std::vector<disease>::iterator it = illness.begin();
      it != illness.end(); ++it) {
    if (it->is_mabuff() &&
        g->ma_buffs.find(it->buff_id) != g->ma_buffs.end()) {
      if (g->ma_buffs[it->buff_id].is_quiet()) return true;
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
