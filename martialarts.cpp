#include "game.h"
#include "martialarts.h"
#include "catajson.h"
#include <map>

void loadBuffArray(game* g, std::vector<ma_buff>& buffArr, catajson& jsonObj) {
  for (jsonObj.set_begin(); jsonObj.has_curr() && json_good();
      jsonObj.next()) {
    catajson curBuff = jsonObj.curr();

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
      buff.unarmed_allowed = curBuff.get("unarmed_allowed").as_bool();
    if (curBuff.has("melee_allowed"))
      buff.melee_allowed = curBuff.get("melee_allowed").as_bool();

    if (curBuff.has("dodges_bonus"))
      buff.dodges_bonus = curBuff.get("dodges_bonus").as_int();
    if (curBuff.has("blocks_bonus"))
      buff.blocks_bonus = curBuff.get("blocks_bonus").as_int();

    if (curBuff.has("hit"))
      buff.hit = curBuff.get("hit").as_int();
    if (curBuff.has("bash"))
      buff.bash = curBuff.get("bash").as_int();
    if (curBuff.has("cut"))
      buff.cut = curBuff.get("cut").as_int();

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



    buffArr.push_back(buff);
    g->ma_buffs[buff.id] = buff;
  }
}

void game::init_martialarts() {
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

    martialarts[ma.id] = ma;
  }
}

ma_buff::ma_buff() {

  buff_duration = 2;

  unarmed_allowed = false; // does this bonus work when unarmed?
  melee_allowed = false; // what about with a melee weapon?

  max_stacks = 1;

  min_melee = 0; // minimum amount of unarmed to trigger this bonus
  min_unarmed = 0; // minimum amount of unarmed to trigger this bonus
  min_bashing = 0; // minimum amount of unarmed to trigger this bonus
  min_cutting = 0; // minimum amount of unarmed to trigger this bonus
  min_stabbing = 0; // minimum amount of unarmed to trigger this bonus

  dodges_bonus = 0; // extra dodges, like karate
  blocks_bonus = 0; // extra blocks, like karate

  bash = 0;
  cut = 0;

  bash_stat_mult = 1.f; // bash damage multiplier, like aikido
  cut_stat_mult = 1.f; // cut damage multiplier

  bash_str = 0; // bonus damage to add per str point
  bash_dex = 0; // "" dex point
  bash_int = 0; // "" int point
  bash_per = 0; // "" per point

  cut_str = 0; // bonus cut damage to add per str point
  cut_dex = 0; // "" dex point
  cut_int = 0; // "" int point
  cut_per = 0; // "" per point

  hit_str = 0; // bonus to-hit to add per str point
  hit_dex = 0; // "" dex point
  hit_int = 0; // "" int point
  hit_per = 0; // "" per point

}

void ma_buff::apply_buff(std::vector<disease>& dVec) {
  for (std::vector<disease>::iterator it = dVec.begin();
      it != dVec.end(); ++it) {
    if (it->type == "ma_buff" && it->buff_id == id) {
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
  return ((unarmed_allowed && !u.is_armed()) || (melee_allowed && u.is_armed()))
    && u.skillLevel("melee") >= min_melee
    && u.skillLevel("unarmed") >= min_unarmed
    && u.skillLevel("bashing") >= min_bashing
    && u.skillLevel("cutting") >= min_cutting
    && u.skillLevel("stabbing") >= min_stabbing
  ;
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



martialart::martialart() {
  return;
}

void martialart::apply_static_buffs(player& u, std::vector<disease>& dVec) {
  for (std::vector<ma_buff>::iterator it = static_buffs.begin();
      it != static_buffs.end(); ++it) {
    if (it->is_valid_player(u)) {
      it->apply_buff(dVec);
    }
  }
}

void martialart::apply_onhit_buffs(player& u, std::vector<disease>& dVec) {
  for (std::vector<ma_buff>::iterator it = onhit_buffs.begin();
      it != onhit_buffs.end(); ++it) {
    if (it->is_valid_player(u)) {
      it->apply_buff(dVec);
    }
  }
}


