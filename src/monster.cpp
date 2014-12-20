#include "monster.h"
#include "map.h"
#include "mondeath.h"
#include "output.h"
#include "game.h"
#include "rng.h"
#include "item.h"
#include "translations.h"
#include "overmapbuffer.h"
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <algorithm>
#include "cursesdef.h"
#include "monstergenerator.h"
#include "json.h"
#include "messages.h"

#define SGN(a) (((a)<0) ? -1 : 1)
#define SQR(a) ((a)*(a))

monster::monster()
{
 _posx = 20;
 _posy = 10;
 wandx = -1;
 wandy = -1;
 wandf = 0;
 hp = 60;
 moves = 0;
 def_chance = 0;
 friendly = 0;
 anger = 0;
 morale = 2;
 faction_id = -1;
 mission_id = -1;
 no_extra_death_drops = false;
 dead = false;
 made_footstep = false;
 unique_name = "";
 hallucination = false;
 ignoring = 0;
}

monster::monster(mtype *t)
{
 _posx = 20;
 _posy = 10;
 wandx = -1;
 wandy = -1;
 wandf = 0;
 type = t;
 moves = type->speed;
 Creature::set_speed_base(type->speed);
 hp = type->hp;
 for( auto &elem : type->sp_freq ) {
     sp_timeout.push_back( rng( 0, elem ) );
 }
 def_chance = type->def_chance;
 friendly = 0;
 anger = t->agro;
 morale = t->morale;
 faction_id = -1;
 mission_id = -1;
 no_extra_death_drops = false;
 dead = false;
 made_footstep = false;
 unique_name = "";
 hallucination = false;
 ignoring = 0;
 ammo = t->starting_ammo;
}

monster::monster(mtype *t, int x, int y)
{
 _posx = x;
 _posy = y;
 wandx = -1;
 wandy = -1;
 wandf = 0;
 type = t;
 moves = type->speed;
 Creature::set_speed_base(type->speed);
 hp = type->hp;
 for( auto &elem : type->sp_freq ) {
     sp_timeout.push_back( elem );
 }
 def_chance = type->def_chance;
 friendly = 0;
 anger = type->agro;
 morale = type->morale;
 faction_id = -1;
 mission_id = -1;
 no_extra_death_drops = false;
 dead = false;
 made_footstep = false;
 unique_name = "";
 hallucination = false;
 ignoring = 0;
 ammo = t->starting_ammo;
}

monster::~monster()
{
}

bool monster::setpos(const int x, const int y, const bool level_change)
{
    if (!level_change && x == _posx && y == _posy) {
        return true;
    }
    bool ret = level_change ? true : g->update_zombie_pos(*this, x, y);
    _posx = x;
    _posy = y;
    return ret;
}

bool monster::setpos(const point &p, const bool level_change)
{
    return setpos(p.x, p.y, level_change);
}

point monster::pos() const
{
    return point(_posx, _posy);
}

void monster::poly(mtype *t)
{
 double hp_percentage = double(hp) / double(type->hp);
 type = t;
 moves = 0;
 Creature::set_speed_base(type->speed);
 anger = type->agro;
 morale = type->morale;
 hp = int(hp_percentage * type->hp);
 sp_timeout.clear();
 for( auto &elem : type->sp_freq ) {
     sp_timeout.push_back( elem );
 }
 def_chance = type->def_chance;
}

void monster::spawn(int x, int y)
{
    _posx = x;
    _posy = y;
}

std::string monster::name(unsigned int quantity) const
{
 if (!type) {
  debugmsg ("monster::name empty type!");
  return std::string();
 }
 if (unique_name != "")
  return string_format("%s: %s",
                       (type->nname(quantity).c_str()), unique_name.c_str());
 return type->nname(quantity);
}

// MATERIALS-TODO: put description in materials.json?
std::string monster::name_with_armor() const
{
 std::string ret;
 if (type->in_species("INSECT")) {
     ret = string_format(_("carapace"));
 }
 else {
     if (type->mat == "veggy") {
         ret = string_format(_("thick bark"));
     } else if (type->mat == "flesh" || type->mat == "hflesh" || type->mat == "iflesh") {
         ret = string_format(_("thick hide"));
     } else if (type->mat == "iron" || type->mat == "steel") {
         ret = string_format(_("armor plating"));
     } else if (type->mat == "protoplasmic") {
         ret = string_format(_("hard protoplasmic hide"));
     }
 }
 return ret;
}

std::string monster::disp_name(bool possessive) const {
    if (!possessive) {
        return string_format(_("the %s"), name().c_str());
    } else {
        return string_format(_("the %s's"), name().c_str());
    }
}

std::string monster::skin_name() const {
    return name_with_armor();
}

void monster::get_HP_Bar(nc_color &color, std::string &text) const
{
    ::get_HP_Bar(hp, type->hp, color, text, true);
}

void monster::get_Attitude(nc_color &color, std::string &text) const
{
    switch (attitude(&(g->u))) {
        case MATT_FRIEND:
            color = h_white;
            text = _("Friendly ");
            break;
        case MATT_FPASSIVE:
            color = h_white;
            text = _("Passive ");
            break;
        case MATT_FLEE:
            color = c_green;
            text = _("Fleeing! ");
            break;
        case MATT_IGNORE:
            color = c_ltgray;
            text = _("Ignoring ");
            break;
        case MATT_FOLLOW:
            color = c_yellow;
            text = _("Tracking ");
            break;
        case MATT_ATTACK:
            color = c_red;
            text = _("Hostile! ");
            break;
        case MATT_ZLAVE:
            color = c_green;
            text = _("Zombie slave ");
            break;
        default:
            color = h_red;
            text = "BUG: Behavior unnamed. (monster.cpp:get_Attitude)";
            break;
    }
}

int monster::print_info(WINDOW* w, int vStart, int vLines, int column) const
{
    const int vEnd = vStart + vLines;

    mvwprintz(w, vStart++, column, c_white, "%s ", name().c_str());
    nc_color color = c_white;
    std::string attitude = "";

    get_Attitude(color, attitude);
    wprintz(w, color, "%s", attitude.c_str());

    if (has_effect("downed")) {
        wprintz(w, h_white, _("On ground"));
    } else if (has_effect("stunned")) {
        wprintz(w, h_white, _("Stunned"));
    } else if (has_effect("lightsnare") || has_effect("heavysnare") || has_effect("beartrap")) {
        wprintz(w, h_white, _("Trapped"));
    } else if (has_effect("tied")) {
        wprintz(w, h_white, _("Tied"));
    }
    std::string damage_info;
    nc_color col;
    if (hp >= type->hp) {
        damage_info = _("It is uninjured");
        col = c_green;
    } else if (hp >= type->hp * .8) {
        damage_info = _("It is lightly injured");
        col = c_ltgreen;
    } else if (hp >= type->hp * .6) {
        damage_info = _("It is moderately injured");
        col = c_yellow;
    } else if (hp >= type->hp * .3) {
        damage_info = _("It is heavily injured");
        col = c_yellow;
    } else if (hp >= type->hp * .1) {
        damage_info = _("It is severely injured");
        col = c_ltred;
    } else {
        damage_info = _("it is nearly dead");
        col = c_red;
    }
    mvwprintz(w, vStart++, column, col, "%s", damage_info.c_str());

    std::vector<std::string> lines = foldstring(type->description, getmaxx(w) - 1 - column);
    int numlines = lines.size();
    for (int i = 0; i < numlines && vStart <= vEnd; i++) {
        mvwprintz(w, vStart++, column, c_white, "%s", lines[i].c_str());
    }

    return vStart;
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
    return (friendly != 0);
}

nc_color monster::color_with_effects() const
{
    nc_color ret = type->color;
    if (has_effect("beartrap") || has_effect("stunned") || has_effect("downed") || has_effect("tied") ||
          has_effect("lightsnare") || has_effect("heavysnare")) {
        ret = hilite(ret);
    }
    if (has_effect("pacified")) {
        ret = invert_color(ret);
    }
    if (has_effect("onfire")) {
        ret = red_background(ret);
    }
    return ret;
}

bool monster::has_flag(const m_flag f) const
{
 return type->has_flag(f);
}

bool monster::can_see() const
{
 return has_flag(MF_SEES) && !has_effect("blind");
}

bool monster::can_hear() const
{
 return has_flag(MF_HEARS) && !has_effect("deaf");
}

bool monster::can_submerge() const
{
  return (has_flag(MF_NO_BREATHE) || has_flag(MF_SWIMS) || has_flag(MF_AQUATIC))
          && !has_flag(MF_ELECTRONIC);
}

bool monster::can_drown() const
{
 return !has_flag(MF_SWIMS) && !has_flag(MF_AQUATIC)
         && !has_flag(MF_NO_BREATHE) && !has_flag(MF_FLIES);
}

bool monster::digging() const
{
    return has_flag(MF_DIGS) || (has_flag(MF_CAN_DIG) && g->m.has_flag("DIGGABLE", posx(), posy()));
}

int monster::vision_range(const int x, const int y) const
{
    int range = g->light_level();
    // Set to max possible value if the target is lit brightly
    if (g->m.light_at(x, y) >= LL_LOW)
        range = DAYLIGHT_LEVEL;

    if(has_flag(MF_VIS10)) {
        range -= 50;
    } else if(has_flag(MF_VIS20)) {
        range -= 40;
    } else if(has_flag(MF_VIS30)) {
        range -= 30;
    } else if(has_flag(MF_VIS40)) {
        range -= 20;
    } else if(has_flag(MF_VIS50)) {
        range -= 10;
    }
    range = std::max(range, 1);

    return range;
}

bool monster::sees_player(int & bresenham_slope, player * p) const {
    if ( p == NULL ) {
        p = &g->u;
    }
    const int range = vision_range(p->posx, p->posy);
    // * p->visibility() / 100;
    return (
        g->m.sees( _posx, _posy, p->posx, p->posy, range, bresenham_slope ) &&
        p->is_invisible() == false
    );
}

bool monster::made_of(std::string m) const
{
    return type->mat == m;
}

bool monster::made_of(phase_id p) const
{
    return type->phase == p;
}

void monster::load_info(std::string data)
{
    std::stringstream dump;
    dump << data;
    if ( dump.peek() == '{' ) {
        JsonIn jsin(dump);
        try {
            deserialize(jsin);
        } catch (std::string jsonerr) {
            debugmsg("monster:load_info: Bad monster json\n%s", jsonerr.c_str() );
        }
        return;
    } else {
        load_legacy(dump);
    }
}

void monster::debug(player &u)
{
    debugmsg("monster::debug %s has %d steps planned.", name().c_str(), plans.size());
    debugmsg("monster::debug %s Moves %d Speed %d HP %d",name().c_str(), moves, get_speed(), hp);
    for (size_t i = 0; i < plans.size(); i++) {
        const int digit = '0' + (i % 10);
        mvaddch(plans[i].y - SEEY + u.posy, plans[i].x - SEEX + u.posx, digit);
    }
    getch();
}

void monster::shift(int sx, int sy)
{
    _posx -= sx * SEEX;
    _posy -= sy * SEEY;
    for (auto &i : plans) {
        i.x -= sx * SEEX;
        i.y -= sy * SEEY;
    }
}

point monster::move_target()
{
    if (plans.empty()) {
        // if we have no plans, pretend it's intentional
        return point(_posx, _posy);
    }
    return point(plans.back().x, plans.back().y);
}

bool monster::is_fleeing(player &u) const
{
 if (has_effect("run"))
  return true;
 monster_attitude att = attitude(&u);
 return (att == MATT_FLEE ||
         (att == MATT_FOLLOW && rl_dist(_posx, _posy, u.posx, u.posy) <= 4));
}

Creature::Attitude monster::attitude_to( const Creature &other ) const
{
    const auto m = dynamic_cast<const monster *>( &other );
    const auto p = dynamic_cast<const player *>( &other );
    if( m != nullptr ) {
        if( friendly != 0 && m->friendly != 0 ) {
            // Currently friendly means "friendly to the player" (on same side as player),
            // so if both monsters are friendly (towards the player), they are friendly towards
            // each other.
            return A_FRIENDLY;
        }
        // For now monsters are neutral (not hostile!) to all other monsters.
        return A_NEUTRAL;
    } else if( p != nullptr ) {
        switch( attitude( const_cast<player *>( p ) ) ) {
            case MATT_FRIEND:
                return A_FRIENDLY;
            case MATT_FPASSIVE:
            case MATT_FLEE:
            case MATT_IGNORE:
            case MATT_FOLLOW:
            case MATT_ZLAVE:
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

monster_attitude monster::attitude(player *u) const
{
    if (friendly != 0 && !(has_effect("docile"))) {
        return MATT_FRIEND;
    }
    if (friendly != 0 ) {
        return MATT_FPASSIVE;
    }
    if (has_effect("run")) {
        return MATT_FLEE;
    }
    if (has_effect("pacified")) {
        return MATT_ZLAVE;
    }

    int effective_anger  = anger;
    int effective_morale = morale;

    if (u != NULL) {
        if (((type->in_species("MAMMAL") && u->has_trait("PHEROMONE_MAMMAL")) ||
             (type->in_species("INSECT") && u->has_trait("PHEROMONE_INSECT"))) &&
            effective_anger >= 10) {
            effective_anger -= 20;
        }

        if ( (type->id == "mon_bee") && (u->has_trait("FLOWERS"))) {
            effective_anger -= 10;
        }

        if (u->has_trait("TERRIFYING")) {
            effective_morale -= 10;
        }

        if (u->has_trait("ANIMALEMPATH") && has_flag(MF_ANIMAL)) {
            if (effective_anger >= 10) {
                effective_anger -= 10;
            }
            if (effective_anger < 10) {
                effective_morale += 5;
            }
        }
        if (u->has_trait("ANIMALDISCORD") && has_flag(MF_ANIMAL)) {
            if (effective_anger >= 10) {
                effective_anger += 10;
            }
            if (effective_anger < 10) {
                effective_morale -= 5;
            }
        }
    }

    if (effective_morale < 0) {
        if (effective_morale + effective_anger > 0) {
            return MATT_FOLLOW;
        }
        return MATT_FLEE;
    }

    if (effective_anger <= 0) {
        return MATT_IGNORE;
    }

    if (effective_anger < 10) {
        return MATT_FOLLOW;
    }

    return MATT_ATTACK;
}

void monster::process_triggers()
{
 anger += trigger_sum(&(type->anger));
 anger -= trigger_sum(&(type->placate));
 if (morale < 0) {
  if (morale < type->morale && one_in(20))
  morale++;
 } else
  morale -= trigger_sum(&(type->fear));
}

// This Adjustes anger/morale levels given a single trigger.
void monster::process_trigger(monster_trigger trig, int amount)
{
    if (type->has_anger_trigger(trig)){
        anger += amount;
    }
    if (type->has_fear_trigger(trig)){
        morale -= amount;
    }
    if (type->has_placate_trigger(trig)){
        anger -= amount;
    }
}


int monster::trigger_sum(std::set<monster_trigger> *triggers) const
{
    int ret = 0;
    bool check_terrain = false, check_meat = false, check_fire = false;
    for( const auto &trigger : *triggers ) {
        switch( trigger ) {
            case MTRIG_STALK:
                if (anger > 0 && one_in(20)) {
                    ret++;
                }
                break;

            case MTRIG_MEAT:
                check_terrain = true;
                check_meat = true;
                break;

            case MTRIG_PLAYER_CLOSE:
                if (rl_dist(_posx, _posy, g->u.posx, g->u.posy) <= 5) {
                    ret += 5;
                }
                for (auto &i : g->active_npc) {
                    if (rl_dist(_posx, _posy, i->posx, i->posy) <= 5) {
                        ret += 5;
                    }
                }
                break;

            case MTRIG_FIRE:
                check_terrain = true;
                check_fire = true;
                break;

            case MTRIG_PLAYER_WEAK:
                if (g->u.hp_percentage() <= 70) {
                    ret += 10 - int(g->u.hp_percentage() / 10);
                }
                break;

            default:
                break; // The rest are handled when the impetus occurs
        }
    }

    if (check_terrain) {
        for (int x = _posx - 3; x <= _posx + 3; x++) {
            for (int y = _posy - 3; y <= _posy + 3; y++) {
                if (check_meat) {
                    auto &items = g->m.i_at(x, y);
                    for( auto &item : items ) {
                        if( item.type->id == "corpse" || item.type->id == "meat" ||
                            item.type->id == "meat_cooked" || item.type->id == "human_flesh" ) {
                            ret += 3;
                            check_meat = false;
                        }
                    }
                }
                if (check_fire) {
                    ret += ( 5 * g->m.get_field_strength( point(x, y), fd_fire) );
                }
            }
        }
        if (check_fire) {
            if (g->u.has_amount("torch_lit", 1)) {
                ret += 49;
            }
        }
    }

    return ret;
}

bool monster::is_underwater() const {
    return can_submerge();
}

bool monster::is_on_ground() const {
    return false; //TODO: actually make this work
}

bool monster::has_weapon() const {
    return false; // monsters will never have weapons, silly
}

bool monster::is_warm() const {
    return has_flag(MF_WARM);
}

bool monster::is_dead_state() const {
    return hp <= 0;
}

void monster::dodge_hit(Creature *, int) {
}

bool monster::block_hit(Creature *, body_part &, damage_instance &) {
    return false;
}

void monster::absorb_hit(body_part, damage_instance &dam) {
    for( auto &elem : dam.damage_units ) {
        elem.amount -= std::min( resistances( *this ).get_effective_resist( elem ), elem.amount );
    }
}

void monster::melee_attack(Creature &target, bool, matec_id) {
    mod_moves(-100);
    if (type->melee_dice == 0) { // We don't attack, so just return
        return;
    }
    add_effect("hit_by_player", 3); // Make us a valid target for a few turns

    if (has_flag(MF_HIT_AND_RUN)) {
        add_effect("run", 4);
    }

    bool u_see_me = g->u_see(this);

    body_part bp_hit;
    //int highest_hit = 0;

    damage_instance damage;
    if(!is_hallucination()) {
        if (type->melee_dice > 0) {
            damage.add_damage(DT_BASH,
                    dice(type->melee_dice,type->melee_sides));
        }
        if (type->melee_cut > 0) {
            damage.add_damage(DT_CUT, type->melee_cut);
        }
    }

    dealt_damage_instance dealt_dam;
    int hitspread = target.deal_melee_attack(this, hit_roll());
    if (hitspread >= 0) {
        target.deal_melee_hit(this, hitspread, false, damage, dealt_dam);
    }
    bp_hit = dealt_dam.bp_hit;

    if (hitspread < 0) { // a miss
        // TODO: characters practice dodge when a hit misses 'em
        if (target.is_player()) {
            if (u_see_me) {
                add_msg(_("You dodge %s."), disp_name().c_str());
            } else {
                add_msg(_("You dodge an attack from an unseen source."));
            }
        } else {
            if (u_see_me) {
                add_msg(_("The %1$s dodges %2$s attack."), name().c_str(),
                            target.disp_name(true).c_str());
            }
        }
    //Hallucinations always produce messages but never actually deal damage
    } else if (is_hallucination() || dealt_dam.total_damage() > 0) {
        if (target.is_player()) {
            if (u_see_me) {
                //~ 1$s is attacker name, 2$s is bodypart name in accusative.
                add_msg(m_bad, _("The %1$s hits your %2$s."), name().c_str(),
                        body_part_name_accusative(bp_hit).c_str());
            } else {
                //~ %s is bodypart name in accusative.
                add_msg(m_bad, _("Something hits your %s."),
                        body_part_name_accusative(bp_hit).c_str());
            }
        } else {
            if (u_see_me) {
                //~ 1$s is attacker name, 2$s is target name, 3$s is bodypart name in accusative.
                add_msg(_("The %1$s hits %2$s %3$s."), name().c_str(),
                            target.disp_name(true).c_str(),
                            body_part_name_accusative(bp_hit).c_str());
            }
        }
    } else {
        if (target.is_player()) {
            if (u_see_me) {
                //~ 1$s is attacker name, 2$s is bodypart name in accusative, 3$s is armor name
                add_msg(_("The %1$s hits your %2$s, but your %3$s protects you."), name().c_str(),
                        body_part_name_accusative(bp_hit).c_str(), target.skin_name().c_str());
            } else {
                //~ 1$s is bodypart name in accusative, 2$s is armor name.
                add_msg(_("Something hits your %1$s, but your %2$s protects you."),
                        body_part_name_accusative(bp_hit).c_str(), target.skin_name().c_str());
            }
        } else {
            if (u_see_me) {
                //~ $1s is monster name, %2$s is that monster target name,
                //~ $3s is target bodypart name in accusative, 4$s is target armor name.
                add_msg(_("The %1$s hits %2$s %3$s but is stopped by %2$s %4$s."), name().c_str(),
                            target.disp_name(true).c_str(),
                            body_part_name_accusative(bp_hit).c_str(),
                            target.skin_name().c_str());
            }
        }
    }

    if (is_hallucination()) {
        if(one_in(7)) {
            die( nullptr );
        }
        return;
    }

    // Adjust anger/morale of same-species monsters, if appropriate
    int anger_adjust = 0, morale_adjust = 0;
    if (type->has_anger_trigger(MTRIG_FRIEND_ATTACKED)){
        anger_adjust += 15;
    }
    if (type->has_fear_trigger(MTRIG_FRIEND_ATTACKED)){
        morale_adjust -= 15;
    }
    if (type->has_placate_trigger(MTRIG_FRIEND_ATTACKED)){
        anger_adjust -= 15;
    }

    if (anger_adjust != 0 && morale_adjust != 0)
    {
        for (size_t i = 0; i < g->num_zombies(); i++)
        {
            g->zombie(i).morale += morale_adjust;
            g->zombie(i).anger += anger_adjust;
        }
    }
}

void monster::hit_monster(monster &other)
{
 monster* target = &other;
 moves -= 100;

 if (this == target) {
  return;
 }

 int numdice = type->melee_skill;
 int dodgedice = target->get_dodge() * 2;
 switch (target->type->size) {
  case MS_TINY:  dodgedice += 6; break;
  case MS_SMALL: dodgedice += 3; break;
  case MS_MEDIUM: break;
  case MS_LARGE: dodgedice -= 2; break;
  case MS_HUGE:  dodgedice -= 4; break;
 }

 if (dice(numdice, 10) <= dice(dodgedice, 10)) {
  if (g->u_see(this))
   add_msg(_("The %s misses the %s!"), name().c_str(), target->name().c_str());
  return;
 }
 if (g->u_see(this))
  add_msg(_("The %s hits the %s!"), name().c_str(), target->name().c_str());
 int damage = dice(type->melee_dice, type->melee_sides);
 target->apply_damage( this, bp_torso, damage );
}

int monster::deal_melee_attack(Creature *source, int hitroll)
{
    mdefense mdf;
    if(!is_hallucination() && source != NULL) {
        (mdf.*type->sp_defense)(this, NULL);
    }
    return Creature::deal_melee_attack(source, hitroll);
}

int monster::deal_projectile_attack(Creature *source, double missed_by,
                                    const projectile& proj, dealt_damage_instance &dealt_dam) {
    bool u_see_mon = g->u_see(this);
    // Maxes out at 50% chance with perfect hit
    if (has_flag(MF_HARDTOSHOOT) && !one_in(10 - 10 * (.8 - missed_by)) && !proj.wide) {
        if (u_see_mon) {
            add_msg(_("The shot passes through %s without hitting."), disp_name().c_str());
        }
        return 1;
    }
    // Not HARDTOSHOOT
    // if it's a headshot with no head, make it not a headshot
    if (missed_by < 0.2 && has_flag(MF_NOHEAD)) {
        missed_by = 0.2;
    }
    mdefense mdf;
    if(!is_hallucination() && source != NULL) {
        (mdf.*type->sp_defense)(this, &proj);
    }

    // whip has a chance to scare wildlife
    if(proj.proj_effects.count("WHIP") && type->in_category("WILDLIFE") && one_in(3)) {
        add_effect("run", rng(3, 5));
    }

    return Creature::deal_projectile_attack(source, missed_by, proj, dealt_dam);
}

void monster::deal_damage_handle_type(const damage_unit& du, body_part bp, int& damage, int& pain) {
    switch (du.type) {
    case DT_ELECTRIC:
        if (has_flag(MF_ELECTRIC)) {
            damage += 0; // immunity
            pain += 0;
            return; // returns, since we don't want a fallthrough
        }
        break;
    case DT_COLD:
        if (!has_flag(MF_WARM)) {
            damage += 0; // immunity
            pain += 0;
            return;
        }
        break;
    case DT_BASH:
        if (has_flag(MF_PLASTIC)) {
            damage += du.amount / rng(2,4); // lessened effect
            pain += du.amount / 4;
            return;
        }
        break;
    case DT_NULL:
        debugmsg("monster::deal_damage_handle_type: illegal damage type DT_NULL");
        break;
    case DT_ACID:
        if( has_flag( MF_ACIDPROOF ) ) {
            damage += 0; // immunity
            pain += 0;
            return;
        }
    case DT_TRUE: // typeless damage, should always go through
    case DT_BIOLOGICAL: // internal damage, like from smoke or poison
    case DT_CUT:
    case DT_STAB:
    case DT_HEAT:
    default:
        break;
    }

    Creature::deal_damage_handle_type(du, bp, damage, pain);
}

void monster::apply_damage(Creature* source, body_part /*bp*/, int dam) {
    if( dead ) {
        return;
    }
    hp -= dam;
    if( hp < 1 ) {
        die( source );
    } else if( dam > 0 ) {
        process_trigger( MTRIG_HURT, 1 + int( dam / 3 ) );
    }
}

void monster::die_in_explosion(Creature* source)
{
    hp = -9999; // huge to trigger explosion and prevent corpse item
    die( source );
}

bool monster::move_effects()
{
    bool u_see_me = g->u_see(this);
    if (has_effect("tied")) {
        return false;
    }
    if (has_effect("downed")) {
        remove_effect("downed");
        if (u_see_me) {
            add_msg(_("The %s climbs to it's feet!"), name().c_str());
        }
        return false;
    }
    if (has_effect("webbed")) {
        if (x_in_y(type->melee_dice * type->melee_sides, 6 * get_effect_int("webbed"))) {
            if (u_see_me) {
                add_msg(_("The %s breaks free of the webs!"), name().c_str());
            }
            remove_effect("webbed");
        }
        return false;
    }
    if (has_effect("lightsnare")) {
        if(x_in_y(type->melee_dice * type->melee_sides, 12)) {
            remove_effect("lightsnare");
            g->m.spawn_item(xpos(), ypos(), "string_36");
            g->m.spawn_item(xpos(), ypos(), "snare_trigger");
            if (u_see_me) {
                add_msg(_("The %s escapes the light snare!"), name().c_str());
            }
        }
        return false;
    }
    if (has_effect("heavysnare")) {
        if (type->melee_dice * type->melee_sides >= 7) {
            if(x_in_y(type->melee_dice * type->melee_sides, 32)) {
                remove_effect("heavysnare");
                g->m.spawn_item(xpos(), ypos(), "rope_6");
                g->m.spawn_item(xpos(), ypos(), "snare_trigger");
                if (u_see_me) {
                    add_msg(_("The %s escapes the heavy snare!"), name().c_str());
                }
            }
        }
        return false;
    }
    if (has_effect("beartrap")) {
        if (type->melee_dice * type->melee_sides >= 18) {
            if(x_in_y(type->melee_dice * type->melee_sides, 200)) {
                remove_effect("beartrap");
                g->m.spawn_item(xpos(), ypos(), "beartrap");
                if (u_see_me) {
                    add_msg(_("The %s escapes the bear trap!"), name().c_str());
                }
            }
        }
        return false;
    }
    if (has_effect("crushed")) {
        // Strength helps in getting free, but dex also helps you worm your way out of the rubble
        if(x_in_y(type->melee_dice * type->melee_sides, 100)) {
            remove_effect("crushed");
            if (u_see_me) {
                add_msg(_("The %s frees itself from the rubble!"), name().c_str());
            }
        }
        return false;
    }

    // If we ever get more effects that force movement on success this will need to be reworked to
    // only trigger success effects if /all/ rolls succeed
    if (has_effect("in_pit")) {
        if (rng(0, 40) > type->melee_dice * type->melee_sides) {
            return false;
        } else {
            if (u_see_me) {
                add_msg(_("The %s escapes the pit!"), name().c_str());
            }
            remove_effect("in_pit");
        }
    }
    return Creature::move_effects();
}

void monster::add_eff_effects(effect e, bool reduced)
{
    if (e.get_amount("HURT", reduced) > 0) {
        if(e.activated(calendar::turn, "HURT", reduced)) {
            apply_damage(nullptr, bp_torso, e.get_amount("HURT", reduced));
        }
    }
    Creature::add_eff_effects(e, reduced);
}
void monster::add_effect(efftype_id eff_id, int dur, body_part bp, bool permanent, int intensity)
{
    bp = num_bp;
    Creature::add_effect(eff_id, dur, bp, permanent, intensity);
}

int monster::get_armor_cut(body_part bp) const
{
    (void) bp;
    // TODO: Add support for worn armor?
    return int(type->armor_cut) + armor_bash_bonus;
}

int monster::get_armor_bash(body_part bp) const
{
    (void) bp;
    return int(type->armor_bash) + armor_cut_bonus;
}

int monster::hit_roll() const {
    //Unstable ground chance of failure
    if (has_effect("bouldering")) {
        if(one_in(type->melee_skill)) {
            return 0;
        }
    }

    return dice(type->melee_skill, 10);
}

int monster::get_dodge() const
{
    if (has_effect("downed")) {
        return 0;
    }
    int ret = type->sk_dodge;
    if (has_effect("lightsnare") || has_effect("heavysnare") || has_effect("beartrap") || has_effect("tied")) {
        ret /= 2;
    }
    if (moves <= 0 - 100 - get_speed()) {
        ret = rng(0, ret);
    }
    return ret + get_dodge_bonus();
}

int monster::get_melee() const
{
    return type->melee_skill;
}

int monster::dodge_roll()
{
    if (has_effect("bouldering")) {
        if(one_in(type->sk_dodge)) {
            return 0;
        }
    }

    int numdice = get_dodge();

    switch (type->size) {
        case MS_TINY:
            numdice += 6;
            break;
        case MS_SMALL:
            numdice += 3;
            break;
        case MS_LARGE:
            numdice -= 2;
            break;
        case MS_HUGE:
            numdice -= 4;
            break;
        case MS_MEDIUM:
            break; // keep default
    }

    numdice += get_speed() / 80;
    return dice(numdice, 10);
}

int monster::fall_damage() const
{
 if (has_flag(MF_FLIES))
  return 0;
 switch (type->size) {
  case MS_TINY:   return rng(0, 4);  break;
  case MS_SMALL:  return rng(0, 6);  break;
  case MS_MEDIUM: return dice(2, 4); break;
  case MS_LARGE:  return dice(2, 6); break;
  case MS_HUGE:   return dice(3, 5); break;
 }

 return 0;
}

void monster::reset_special(int index)
{
    if (index < 0) {
        return;
    }

    sp_timeout[index] = type->sp_freq[index];
}

void monster::reset_special_rng(int index)
{
    if (index < 0) {
        return;
    }

    sp_timeout[index] = rng(0, type->sp_freq[index]);
}

void monster::set_special(int index, int time)
{
    if (index < 0) {
        return;
    }

    if (time < 0) {
        time = 0;
    }
    sp_timeout[index] = time;
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
        ammo[ammo_entry.first] = (old_ammo * ammo_entry.second) / (100 * total_ammo);
    }
}

void monster::explode()
{
    if( is_hallucination() ) {
        //Can't gib hallucinations
        return;
    }
    if( type->has_flag( MF_NOGIB ) || type->has_flag( MF_VERMIN ) ) {
        return;
    }
    // Send body parts and blood all over!
    const itype_id meat = type->get_meat_itype();
    const field_id type_blood = bloodType();
    const field_id type_gib = gibType();
    if( meat != "null" || type_blood != fd_null || type_gib != fd_null ) {
        // Only create chunks if we know what kind to make.
        int num_chunks = 0;
        switch( type->size ) {
            case MS_TINY:
                num_chunks = 1;
                break;
            case MS_SMALL:
                num_chunks = 2;
                break;
            case MS_MEDIUM:
                num_chunks = 4;
                break;
            case MS_LARGE:
                num_chunks = 8;
                break;
            case MS_HUGE:
                num_chunks = 16;
                break;
        }

        for( int i = 0; i < num_chunks; i++ ) {
            int tarx = _posx + rng( -3, 3 ), tary = _posy + rng( -3, 3 );
            std::vector<point> traj = line_to( _posx, _posy, tarx, tary, 0 );

            for( size_t j = 0; j < traj.size(); j++ ) {
                tarx = traj[j].x;
                tary = traj[j].y;
                if( one_in( 2 ) && type_blood != fd_null ) {
                    g->m.add_field( tarx, tary, type_blood, 1 );
                } else if( type_gib != fd_null ) {
                    g->m.add_field( tarx, tary, type_gib, rng( 1, j + 1 ) );
                }
                if( g->m.move_cost( tarx, tary ) == 0 ) {
                    if( !g->m.bash( tarx, tary, 3 ).second ) {
                        // Target is obstacle, not destroyed by bashing,
                        // stop trajectory in front of it, if this is the first
                        // point (e.g. wall adjacent to monster) , make it invalid.
                        if( j > 0 ) {
                            tarx = traj[j - 1].x;
                            tary = traj[j - 1].y;
                        } else {
                            tarx = -1;
                        }
                        break;
                    }
                }
            }
            if( meat != "null" && tarx != -1 ) {
                g->m.spawn_item( tarx, tary, meat, 1, 0, calendar::turn );
            }
        }
    }
}

void monster::die(Creature* nkiller) {
    if( dead ) {
        // We are already dead, don't die again, note that monster::dead is
        // *only* set to true in this function!
        return;
    }
    dead = true;
    if( nkiller != NULL && !nkiller->is_fake() ) {
        killer = nkiller;
    }
    if( hp < -( type->size < MS_MEDIUM ? 1.5 : 3 ) * type->hp ) {
        explode(); // Explode them if it was big overkill
    }
    if (!no_extra_death_drops) {
        drop_items_on_death();
    }
    // TODO: should actually be class Character
    player *ch = dynamic_cast<player*>( get_killer() );
    if( !is_hallucination() && ch != nullptr ) {
        if( has_flag( MF_GUILT ) || ( ch->has_trait( "PACIFIST" ) && has_flag( MF_HUMAN ) ) ) {
            // has guilt flag or player is pacifist && monster is humanoid
            mdeath tmpdeath;
            tmpdeath.guilt( this );
        }
        // TODO: add a kill counter to npcs?
        if( ch->is_player() ) {
            g->increase_kill_count( type->id );
        }
        if( type->difficulty >= 30 ) {
            ch->add_memorial_log( pgettext( "memorial_male", "Killed a %s." ),
                                  pgettext( "memorial_female", "Killed a %s." ),
                                  name().c_str() );
        }
    }
    if( !is_hallucination() ) {
        for( const auto &it : inv ) {
            g->m.add_item_or_charges( posx(), posy(), it );
        }
    }

    // If we're a queen, make nearby groups of our type start to die out
    if( !is_hallucination() && has_flag( MF_QUEEN ) ) {
        // The submap coordinates of this monster, monster groups coordinates are
        // submap coordinates.
        const point abssub = overmapbuffer::ms_to_sm_copy( g->m.getabs( _posx, _posy ) );
        // Do it for overmap above/below too
        for( int z = 1; z >= -1; --z ) {
            for( int x = -MAPSIZE / 2; x <= MAPSIZE / 2; x++ ) {
                for( int y = -MAPSIZE / 2; y <= MAPSIZE / 2; y++ ) {
                    std::vector<mongroup*> groups = overmap_buffer.groups_at( abssub.x + x, abssub.y + y, g->levz + z );
                    for( auto &mgp : groups ) {
                        if( MonsterGroupManager::IsMonsterInGroup( mgp->type, type->id ) ) {
                            mgp->dying = true;
                        }
                    }
                }
            }
        }
    }
    // If we're a mission monster, update the mission
    if (!is_hallucination() && mission_id != -1) {
        mission_type *misstype = g->find_mission_type(mission_id);
        if (misstype->goal == MGOAL_FIND_MONSTER) {
            g->fail_mission(mission_id);
        }
        if (misstype->goal == MGOAL_KILL_MONSTER) {
            g->mission_step_complete(mission_id, 1);
        }
    }
    // Also, perform our death function
    mdeath md;
    if(is_hallucination()) {
        //Hallucinations always just disappear
        md.disappear(this);
        return;
    }
    //Not a hallucination, go process the death effects.
    std::vector<void (mdeath::*)(monster *)> deathfunctions = type->dies;
    void (mdeath::*func)(monster *);
    for( auto &deathfunction : deathfunctions ) {
        func = deathfunction;
        (md.*func)(this);
    }
    // If our species fears seeing one of our own die, process that
    int anger_adjust = 0, morale_adjust = 0;
    if (type->has_anger_trigger(MTRIG_FRIEND_DIED)){
        anger_adjust += 15;
    }
    if (type->has_fear_trigger(MTRIG_FRIEND_DIED)){
        morale_adjust -= 15;
    }
    if (type->has_placate_trigger(MTRIG_FRIEND_DIED)){
        anger_adjust -= 15;
    }

    if (anger_adjust != 0 || morale_adjust != 0) {
        int light = g->light_level();
        for (size_t i = 0; i < g->num_zombies(); i++) {
            monster &critter = g->zombie( i );
            if( !critter.type->same_species( *type ) ) {
                continue;
            }
            int t = 0;
            if( g->m.sees( critter.posx(), critter.posy(), _posx, _posy, light, t ) ) {
                critter.morale += morale_adjust;
                critter.anger += anger_adjust;
            }
        }
    }
}

void monster::drop_items_on_death()
{
    if(is_hallucination()) {
        return;
    }
    if (type->death_drops.empty()) {
        return;
    }
    g->m.put_items_from_loc( type->death_drops, _posx, _posy, calendar::turn );
}

void monster::process_effects()
{
    // Monster only effects
    int mod = 1;
    for( auto &elem : effects ) {
        for( auto &_effect_it : elem.second ) {
            auto &it = _effect_it.second;
            // Monsters don't get trait-based reduction, but they do get effect based reduction
            bool reduced = has_effect(it.get_resist_effect());

            mod_speed_bonus(it.get_mod("SPEED", reduced));

            if (it.get_mod("HURT", reduced) > 0) {
                if(it.activated(calendar::turn, "HURT", reduced, mod)) {
                    apply_damage(nullptr, bp_torso, it.get_mod("HURT", reduced));
                }
            }

            std::string id = _effect_it.second.get_id();
            // MATERIALS-TODO: use fire resistance
            if (id == "onfire") {
                if (made_of("flesh") || made_of("iflesh"))
                    apply_damage( nullptr, bp_torso, rng( 3, 8 ) );
                if (made_of("veggy"))
                    apply_damage( nullptr, bp_torso, rng( 10, 20 ) );
                if (made_of("paper") || made_of("powder") || made_of("wood") || made_of("cotton") ||
                    made_of("wool"))
                    apply_damage( nullptr, bp_torso, rng( 15, 40 ) );
            }
        }
    }

    //If this monster has the ability to heal in combat, do it now.
    if( has_flag( MF_REGENERATES_50 ) ) {
        if( hp < type->hp ) {
            if( one_in( 2 ) && g->u.sees( this ) ) {
                add_msg( m_warning, _( "The %s is visibly regenerating!" ), name().c_str() );
            }
            hp += 50;
            if( hp > type->hp ) {
                hp = type->hp;
            }
        }
    }
    if( has_flag( MF_REGENERATES_10 ) ) {
        if( hp < type->hp ) {
            if( one_in( 2 ) && g->u.sees( this ) ) {
                add_msg( m_warning, _( "The %s seems a little healthier." ), name().c_str() );
            }
            hp += 10;
            if( hp > type->hp ) {
                hp = type->hp;
            }
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
    if( has_flag( MF_SUNDEATH ) && g->is_in_sunlight( posx(), posy() ) ) {
        if( g->u.sees( this ) ) {
            add_msg( m_good, _( "The %s burns horribly in the sunlight!" ), name().c_str() );
        }
        hp -= 100;
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
    std::string tid = type->id;
    if (type->in_species("FUNGUS")) { // No friendly-fungalizing ;-)
        return true;
    }
    if (tid == "mon_ant" || tid == "mon_ant_soldier" || tid == "mon_ant_queen" || tid == "mon_fly" ||
      tid == "mon_bee" || tid == "mon_dermatik") {
        polypick = 1;
    } else if (tid == "mon_zombie" || tid == "mon_zombie_shrieker" || tid == "mon_zombie_electric" ||
      tid == "mon_zombie_spitter" || tid == "mon_zombie_dog" || tid == "mon_zombie_brute" ||
      tid == "mon_zombie_hulk" || tid == "mon_zombie_soldier" || tid == "mon_zombie_tough" ||
      tid == "mon_zombie_scientist" || tid == "mon_zombie_hunter" || tid == "mon_zombie_child"||
      tid == "mon_zombie_bio_op" || tid == "mon_zombie_survivor" || tid == "mon_zombie_fireman" ||
      tid == "mon_zombie_cop" || tid == "mon_zombie_fat" || tid == "mon_zombie_rot" ||
      tid == "mon_zombie_swimmer" || tid == "mon_zombie_grabber" || tid == "mon_zombie_technician" ||
      tid == "mon_zombie_brute_shocker") {
        polypick = 2; // Necro and Master have enough Goo to resist conversion.
        // Firefighter, hazmat, and scarred/beekeeper have the PPG on.
    } else if (tid == "mon_zombie_necro" || tid == "mon_zombie_master" || tid == "mon_zombie_fireman" ||
      tid == "mon_zombie_hazmat" || tid == "mon_beekeeper") {
        return true;
    } else if (tid == "mon_boomer" || tid == "mon_zombie_gasbag" || tid == "mon_zombie_smoker") {
        polypick = 3;
    } else if (tid == "mon_triffid" || tid == "mon_triffid_young" || tid == "mon_triffid_queen") {
        polypick = 4;
    }
    switch (polypick) {
        case 1: // bugs, why do they all turn into fungal ants?
            poly(GetMType("mon_ant_fungus"));
            return true;
        case 2: // zombies, non-boomer
            poly(GetMType("mon_zombie_fungus"));
            return true;
        case 3:
            poly(GetMType("mon_boomer_fungus"));
            return true;
        case 4:
            poly(GetMType("mon_fungaloid"));
            return true;
        default:
            return false;
    }
}

void monster::make_friendly()
{
 plans.clear();
 friendly = rng(5, 30) + rng(0, 20);
}

void monster::add_item(item it)
{
 inv.push_back(it);
}

bool monster::is_hallucination() const
{
  return hallucination;
}

field_id monster::bloodType() const {
    return type->bloodType();
}
field_id monster::gibType() const {
    return type->gibType();
}

m_size monster::get_size() const {
    return type->size;
}

void monster::add_msg_if_npc(const char *msg, ...) const
{
    va_list ap;
    va_start(ap, msg);
    std::string processed_npc_string = vstring_format(msg, ap);
    // These strings contain the substring <npcname>,
    // if present replace it with the actual monster name.
    size_t offset = processed_npc_string.find("<npcname>");
    if (offset != std::string::npos) {
        processed_npc_string.replace(offset, 9, disp_name());
        if (offset == 0 && !processed_npc_string.empty()) {
            capitalize_letter(processed_npc_string, 0);
        }
    }
    add_msg(processed_npc_string.c_str());
    va_end(ap);
}

void monster::add_msg_player_or_npc(const char *, const char* npc_str, ...) const
{
    va_list ap;
    va_start(ap, npc_str);
    if (g->u_see(this)) {
        std::string processed_npc_string = vstring_format(npc_str, ap);
        // These strings contain the substring <npcname>,
        // if present replace it with the actual monster name.
        size_t offset = processed_npc_string.find("<npcname>");
        if (offset != std::string::npos) {
            processed_npc_string.replace(offset, 9, disp_name());
            if (offset == 0 && !processed_npc_string.empty()) {
                capitalize_letter(processed_npc_string, 0);
            }
        }
        add_msg(processed_npc_string.c_str());
    }
    va_end(ap);
}

void monster::add_msg_if_npc(game_message_type type, const char *msg, ...) const
{
    va_list ap;
    va_start(ap, msg);
    std::string processed_npc_string = vstring_format(msg, ap);
    // These strings contain the substring <npcname>,
    // if present replace it with the actual monster name.
    size_t offset = processed_npc_string.find("<npcname>");
    if (offset != std::string::npos) {
        processed_npc_string.replace(offset, 9, disp_name());
        if (offset == 0 && !processed_npc_string.empty()) {
            capitalize_letter(processed_npc_string, 0);
        }
    }
    add_msg(type, processed_npc_string.c_str());
    va_end(ap);
}

void monster::add_msg_player_or_npc(game_message_type type, const char *, const char* npc_str, ...) const
{
    va_list ap;
    va_start(ap, npc_str);
    if (g->u_see(this)) {
        std::string processed_npc_string = vstring_format(npc_str, ap);
        // These strings contain the substring <npcname>,
        // if present replace it with the actual monster name.
        size_t offset = processed_npc_string.find("<npcname>");
        if (offset != std::string::npos) {
            processed_npc_string.replace(offset, 9, disp_name());
            if (offset == 0 && !processed_npc_string.empty()) {
                capitalize_letter(processed_npc_string, 0);
            }
        }
        add_msg(type, processed_npc_string.c_str());
    }
    va_end(ap);
}

bool monster::is_dead() const
{
    return dead || is_dead_state();
}

item monster::to_item() const
{
    if( type->revert_to_itype.empty() ) {
        return item();
    }
    // Birthday is wrong, but the item created here does not use it anyway (I hope).
    item result( type->revert_to_itype, calendar::turn );
    const int damfac = std::max( 1, 5 * hp / type->hp ); // 1 ... 5 (or more for some monsters with hp > type->hp)
    result.damage = std::max( 0, 5 - damfac ); // 4 ... 0
    return result;
}
