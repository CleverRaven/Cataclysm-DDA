#include "character.h"
#include "game.h"
#include "map.h"
#include "debug.h"
#include "mission.h"
#include "translations.h"
#include "options.h"
#include "map_iterator.h"
#include <map>

Character::Character()
{
    name = "";
    Creature::set_speed_base(100);
}

field_id Character::bloodType() const
{
    if (has_trait("ACIDBLOOD"))
        return fd_acid;
    if (has_trait("THRESH_PLANT"))
        return fd_blood_veggy;
    if (has_trait("THRESH_INSECT") || has_trait("THRESH_SPIDER"))
        return fd_blood_insect;
    if (has_trait("THRESH_CEPHALOPOD"))
        return fd_blood_invertebrate;
    return fd_blood;
}
field_id Character::gibType() const
{
    return fd_gibs_flesh;
}

bool Character::is_warm() const
{
    return true; // TODO: is there a mutation (plant?) that makes a npc not warm blooded?
}

const std::string &Character::symbol() const
{
    static const std::string character_symbol("@");
    return character_symbol;
}

bool Character::move_effects(bool attacking)
{
    if (has_effect("downed")) {
        if (rng(0, 40) > get_dex() + int(get_str() / 2)) {
            add_msg_if_player(_("You struggle to stand."));
        } else {
            add_msg_player_or_npc(m_good, _("You stand up."),
                                    _("<npcname> stands up."));
            remove_effect("downed");
        }
        return false;
    }
    if (has_effect("webbed")) {
        if (x_in_y(get_str(), 6 * get_effect_int("webbed"))) {
            add_msg_player_or_npc(m_good, _("You free yourself from the webs!"),
                                    _("<npcname> frees themselves from the webs!"));
            remove_effect("webbed");
        } else {
            add_msg_if_player(_("You try to free yourself from the webs, but can't get loose!"));
        }
        return false;
    }
    if (has_effect("lightsnare")) {
        if(x_in_y(get_str(), 12) || x_in_y(get_dex(), 8)) {
            remove_effect("lightsnare");
            add_msg_player_or_npc(m_good, _("You free yourself from the light snare!"),
                                    _("<npcname> frees themselves from the light snare!"));
            item string("string_36", calendar::turn);
            item snare("snare_trigger", calendar::turn);
            g->m.add_item_or_charges(posx(), posy(), string);
            g->m.add_item_or_charges(posx(), posy(), snare);
        } else {
            add_msg_if_player(m_bad, _("You try to free yourself from the light snare, but can't get loose!"));
        }
        return false;
    }
    if (has_effect("heavysnare")) {
        if(x_in_y(get_str(), 32) || x_in_y(get_dex(), 16)) {
            remove_effect("heavysnare");
            add_msg_player_or_npc(m_good, _("You free yourself from the heavy snare!"),
                                    _("<npcname> frees themselves from the heavy snare!"));
            item rope("rope_6", calendar::turn);
            item snare("snare_trigger", calendar::turn);
            g->m.add_item_or_charges(posx(), posy(), rope);
            g->m.add_item_or_charges(posx(), posy(), snare);
        } else {
            add_msg_if_player(m_bad, _("You try to free yourself from the heavy snare, but can't get loose!"));
        }
        return false;
    }
    if (has_effect("beartrap")) {
        /* Real bear traps can't be removed without the proper tools or immense strength; eventually this should
           allow normal players two options: removal of the limb or removal of the trap from the ground
           (at which point the player could later remove it from the leg with the right tools).
           As such we are currently making it a bit easier for players and NPC's to get out of bear traps.
        */
        if(x_in_y(get_str(), 100)) {
            remove_effect("beartrap");
            add_msg_player_or_npc(m_good, _("You free yourself from the bear trap!"),
                                    _("<npcname> frees themselves from the bear trap!"));
            item beartrap("beartrap", calendar::turn);
            g->m.add_item_or_charges(posx(), posy(), beartrap);
        } else {
            add_msg_if_player(m_bad, _("You try to free yourself from the bear trap, but can't get loose!"));
        }
        return false;
    }
    if (has_effect("crushed")) {
        // Strength helps in getting free, but dex also helps you worm your way out of the rubble
        if(x_in_y(get_str() + get_dex() / 4, 100)) {
            remove_effect("crushed");
            add_msg_player_or_npc(m_good, _("You free yourself from the rubble!"),
                                    _("<npcname> frees themselves from the rubble!"));
        } else {
            add_msg_if_player(m_bad, _("You try to free yourself from the rubble, but can't get loose!"));
        }
        return false;
    }
    // Below this point are things that allow for movement if they succeed

    // Currently we only have one thing that forces movement if you succeed, should we get more
    // than this will need to be reworked to only have success effects if /all/ checks succeed
    if (has_effect("in_pit")) {
        if (rng(0, 40) > get_str() + int(get_dex() / 2)) {
            add_msg_if_player(m_bad, _("You try to escape the pit, but slip back in."));
            return false;
        } else {
            add_msg_player_or_npc(m_good, _("You escape the pit!"),
                                    _("<npcname> escapes the pit!"));
            remove_effect("in_pit");
        }
    }
    if (has_effect("grabbed")){
        int zed_number = 0;
        for( auto &&dest : g->m.points_in_radius( pos(), 1, 0 ) ){
            if (g->mon_at(dest) != -1){
                zed_number ++;
            }
        }
        if (attacking == true || zed_number == 0){
            return true;
        }
        if (get_dex() > get_str() ? rng(0, get_dex()) : rng( 0, get_str()) < rng( get_effect_int("grabbed") , 8) ){
            add_msg_player_or_npc(m_bad, _("You try break out of the grab, but fail!"),
                                            _("<npcname> tries to break out of the grab, but fails!"));
            return false;
        } else {
            add_msg_player_or_npc(m_good, _("You break out of the grab!"),
                                            _("<npcname> breaks out of the grab!"));
            remove_effect("grabbed");
        }
    }
    return Creature::move_effects(attacking);
}
void Character::add_effect( efftype_id eff_id, int dur, body_part bp,
                            bool permanent, int intensity, bool force )
{
    Creature::add_effect( eff_id, dur, bp, permanent, intensity, force );
}

void Character::recalc_hp()
{
    int new_max_hp[num_hp_parts];
    for( auto &elem : new_max_hp ) {
        elem = 60 + str_max * 3;
        if (has_trait("HUGE")) {
            // Bad-Huge doesn't quite have the cardio/skeletal/etc to support the mass,
            // so no HP bonus from the ST above/beyond that from Large
            elem -= 6;
        }
        // You lose half the HP you'd expect from BENDY mutations.  Your gelatinous
        // structure can help with that, a bit.
        if (has_trait("BENDY2")) {
            elem += 3;
        }
        if (has_trait("BENDY3")) {
            elem += 6;
        }
        // Only the most extreme applies.
        if (has_trait("TOUGH")) {
            elem *= 1.2;
        } else if (has_trait("TOUGH2")) {
            elem *= 1.3;
        } else if (has_trait("TOUGH3")) {
            elem *= 1.4;
        } else if (has_trait("FLIMSY")) {
            elem *= .75;
        } else if (has_trait("FLIMSY2")) {
            elem *= .5;
        } else if (has_trait("FLIMSY3")) {
            elem *= .25;
        }
        // Mutated toughness stacks with starting, by design.
        if (has_trait("MUT_TOUGH")) {
            elem *= 1.2;
        } else if (has_trait("MUT_TOUGH2")) {
            elem *= 1.3;
        } else if (has_trait("MUT_TOUGH3")) {
            elem *= 1.4;
        }
    }
    if (has_trait("GLASSJAW"))
    {
        new_max_hp[hp_head] *= 0.8;
    }
    for (int i = 0; i < num_hp_parts; i++)
    {
        hp_cur[i] *= (float)new_max_hp[i]/(float)hp_max[i];
        hp_max[i] = new_max_hp[i];
    }
}


// This must be called when any of the following change:
// - effects
// - bionics
// - traits
// - underwater
// - clothes
// With the exception of clothes, all changes to these character attributes must
// occur through a function in this class which calls this function. Clothes are
// typically added/removed with wear() and takeoff(), but direct access to the
// 'wears' vector is still allowed due to refactor exhaustion.
void Character::recalc_sight_limits()
{
    sight_max = 9999;

    // Set sight_max.
    if (has_effect("blind") || worn_with_flag("BLIND")) {
        sight_max = 0;
    } else if (has_effect("in_pit") ||
            (has_effect("boomered") && (!(has_trait("PER_SLIME_OK")))) ||
            (underwater && !has_bionic("bio_membrane") &&
                !has_trait("MEMBRANE") && !worn_with_flag("SWIM_GOGGLES") &&
                !has_trait("CEPH_EYES") && !has_trait("PER_SLIME_OK") ) ) {
        sight_max = 1;
    } else if (has_active_mutation("SHELL2")) {
        // You can kinda see out a bit.
        sight_max = 2;
    } else if ( (has_trait("MYOPIC") || has_trait("URSINE_EYE")) &&
            !is_wearing("glasses_eye") && !is_wearing("glasses_monocle") &&
            !is_wearing("glasses_bifocal") && !has_effect("contacts")) {
        sight_max = 4;
    } else if (has_trait("PER_SLIME")) {
        sight_max = 6;
    }

    vision_mode_cache.reset();
    // Debug-only NV, by vache's request
    if( has_trait("DEBUG_NIGHTVISION") ) {
        vision_mode_cache.set( DEBUG_NIGHTVISION );
    }
    if( has_nv() || is_wearing("rm13_armor_on") ) {
        vision_mode_cache.set( NV_GOGGLES );
    }
    if( has_active_mutation("NIGHTVISION3") ) {
        vision_mode_cache.set( NIGHTVISION_3 );
    }
    if( has_active_mutation("ELFA_FNV") ) {
        vision_mode_cache.set( FULL_ELFA_VISION );
    }
    if( has_active_mutation("CEPH_VISION") ) {
        vision_mode_cache.set( CEPH_VISION );
    }
    if (has_active_mutation("ELFA_NV")) {
        vision_mode_cache.set( ELFA_VISION );
    }
    if( has_active_mutation("NIGHTVISION2") ) {
        vision_mode_cache.set( NIGHTVISION_2 );
    }
    if( has_active_mutation("FEL_NV") ) {
        vision_mode_cache.set( FELINE_VISION );
    }
    if( has_active_mutation("URSINE_EYE") ) {
        vision_mode_cache.set( URSINE_VISION );
    }
    if (has_active_mutation("NIGHTVISION")) {
        vision_mode_cache.set(NIGHTVISION_1);
    }
    if( has_trait("BIRD_EYE") ) {
        vision_mode_cache.set( BIRD_EYE);
    }
}

float Character::get_vision_threshold(int light_level) const {
    // Bail out in extremely common case where character hs no special vision mode or
    // it's too bright for nightvision to work.
    if( vision_mode_cache.none() || light_level > LIGHT_AMBIENT_LIT ) {
        return LIGHT_AMBIENT_LOW;
    }
    // As ligt_level goes from LIGHT_AMBIENT_MINIMAL to LIGHT_AMBIENT_LIT,
    // dimming goes from 1.0 to 2.0.
    const float dimming_from_light = 1.0 + (((float)light_level - LIGHT_AMBIENT_MINIMAL) /
                                            (LIGHT_AMBIENT_LIT - LIGHT_AMBIENT_MINIMAL));
    float threshold = LIGHT_AMBIENT_LOW;

    /**
     * Consider vision modes in order of descending goodness until we get a hit.
     * The values are based on expected sight distance in "total darkness", which is set to 3.7.
     * The range is given by the formula distance = -log(threshold / light_level) / attenuation
     * This is an upper limit, any smoke or similar should shorten the effective distance.
     * The numbers here are hand-tuned to provide the desired ranges,
     * would be nice to derive them with a constexpr function or similar instead.
     */
    if( vision_mode_cache[DEBUG_NIGHTVISION] ) {
        // Debug vision always works with absurdly little light.
        threshold = 0.01;
    } else if( vision_mode_cache[NV_GOGGLES] || vision_mode_cache[NIGHTVISION_3] ||
               vision_mode_cache[FULL_ELFA_VISION] || vision_mode_cache[CEPH_VISION] ) {
        if( vision_mode_cache[BIRD_EYE] ) {
            // Bird eye adds one, so 13.
            threshold = 1.9;
        } else {
            // Highest normal night vision is expected to provide sight out to 12 squares.
            threshold = 1.99;
        }
    } else if( vision_mode_cache[ELFA_VISION] ) {
        // Range 7.
        threshold = 2.65;
    } else if( vision_mode_cache[NIGHTVISION_2] || vision_mode_cache[FELINE_VISION] ||
               vision_mode_cache[URSINE_VISION] ) {
        if( vision_mode_cache[BIRD_EYE] ) {
            // Range 5.
            threshold = 2.78;
        } else {
            // Range 4.
            threshold = 2.9;
        }
    } else if( vision_mode_cache[NIGHTVISION_1] ) {
        if( vision_mode_cache[BIRD_EYE] ) {
            // Range 3.
            threshold = 3.2;
        } else {
            // Range 2.
            threshold = 3.35;
        }
    }
    return std::min( (float)LIGHT_AMBIENT_LOW, threshold * dimming_from_light );
}

bool Character::has_bionic(const std::string & b) const
{
    for (auto &i : my_bionics) {
        if (i.id == b) {
            return true;
        }
    }
    return false;
}

bool Character::has_active_bionic(const std::string & b) const
{
    for (auto &i : my_bionics) {
        if (i.id == b) {
            return (i.powered);
        }
    }
    return false;
}

item& Character::i_add(item it)
{
 itype_id item_type_id = "null";
 if( it.type ) item_type_id = it.type->id;

 last_item = item_type_id;

 if (it.is_food() || it.is_ammo() || it.is_gun()  || it.is_armor() ||
     it.is_book() || it.is_tool() || it.is_weap() || it.is_food_container())
  inv.unsort();

    // if there's a desired invlet for this item type, try to use it
    bool keep_invlet = false;
    const std::set<char> cur_inv = allocated_invlets();
    for (auto iter : assigned_invlet) {
        if (iter.second == item_type_id && !cur_inv.count(iter.first)) {
            it.invlet = iter.first;
            keep_invlet = true;
            break;
        }
    }
    auto &item_in_inv = inv.add_item(it, keep_invlet);
    item_in_inv.on_pickup( *this );
    return item_in_inv;
}

item Character::i_rem(int pos)
{
 item tmp;
 if (pos == -1) {
     tmp = weapon;
     weapon = ret_null;
     return tmp;
 } else if (pos < -1 && pos > worn_position_to_index(worn.size())) {
     tmp = worn[worn_position_to_index(pos)];
     worn.erase(worn.begin() + worn_position_to_index(pos));
     return tmp;
 }
 return inv.remove_item(pos);
}

item Character::i_rem(const item *it)
{
    auto tmp = remove_items_with( [&it] (const item &i) { return &i == it; } );
    if( tmp.empty() ) {
        debugmsg( "did not found item %s to remove it!", it->tname().c_str() );
        return ret_null;
    }
    return tmp.front();
}

void Character::i_rem_keep_contents( const int pos )
{
    for( auto &content : i_rem( pos ).contents ) {
        i_add_or_drop( content );
    }
}

bool Character::i_add_or_drop(item& it, int qty) {
    bool retval = true;
    bool drop = false;
    inv.assign_empty_invlet(it);
    for (int i = 0; i < qty; ++i) {
        if (!drop && (!can_pickWeight(it.weight(), !OPTIONS["DANGEROUS_PICKUPS"])
                      || !can_pickVolume(it.volume()))) {
            drop = true;
        }
        if (drop) {
            retval &= g->m.add_item_or_charges(posx(), posy(), it);
        } else if ( !( it.has_flag("IRREMOVEABLE") && !it.is_gun() ) ){
            i_add(it);
        }
    }
    return retval;
}

std::set<char> Character::allocated_invlets() const {
    std::set<char> invlets = inv.allocated_invlets();

    if (weapon.invlet != 0) {
        invlets.insert(weapon.invlet);
    }
    for( const auto &w : worn ) {
        if( w.invlet != 0 ) {
            invlets.insert( w.invlet );
        }
    }

    return invlets;
}

bool Character::has_active_item(const itype_id & id) const
{
    return has_item_with( [id]( const item & it ) {
        return it.active && it.typeId() == id;
    } );
}

item Character::remove_weapon()
{
    if( weapon.active ) {
        weapon.deactivate_charger_gun();
    }
 item tmp = weapon;
 weapon = ret_null;
 return tmp;
}

void Character::remove_mission_items( int mission_id )
{
    if( mission_id == -1 ) {
        return;
    }
    remove_items_with( has_mission_item_filter { mission_id } );
}

int Character::weight_carried() const
{
    int ret = 0;
    ret += weapon.weight();
    for (auto &i : worn) {
        ret += i.weight();
    }
    ret += inv.weight();
    return ret;
}

int Character::volume_carried() const
{
    return inv.volume();
}

int Character::weight_capacity() const
{
    // Get base capacity from creature,
    // then apply player-only mutation and trait effects.
    int ret = Creature::weight_capacity();
    if (has_trait("BADBACK")) {
        ret = int(ret * .65);
    }
    if (has_trait("STRONGBACK")) {
        ret = int(ret * 1.35);
    }
    if (has_trait("LIGHT_BONES")) {
        ret = int(ret * .80);
    }
    if (has_trait("HOLLOW_BONES")) {
        ret = int(ret * .60);
    }
    if (has_artifact_with(AEP_CARRY_MORE)) {
        ret += 22500;
    }
    if (ret < 0) {
        ret = 0;
    }
    return ret;
}

int Character::volume_capacity() const
{
    int ret = 2; // A small bonus (the overflow)
    for (auto &i : worn) {
        ret += i.get_storage();
    }
    if (has_bionic("bio_storage")) {
        ret += 8;
    }
    if (has_trait("SHELL")) {
        ret += 16;
    }
    if (has_trait("SHELL2") && !has_active_mutation("SHELL2")) {
        ret += 24;
    }
    if (has_trait("PACKMULE")) {
        ret = int(ret * 1.4);
    }
    if (has_trait("DISORGANIZED")) {
        ret = int(ret * 0.6);
    }
    if (ret < 2) {
        ret = 2;
    }
    return ret;
}

bool Character::can_pickVolume( int volume, bool safe ) const
{
    if( !safe ) {
        return volume_carried() + volume <= volume_capacity();
    } else {
        return volume_carried() + volume <= volume_capacity() - 2;
    }
}
bool Character::can_pickWeight( int weight, bool safe ) const
{
    if (!safe)
    {
        // Character can carry up to four times their maximum weight
        return (weight_carried() + weight <= weight_capacity() * 4);
    }
    else
    {
        return (weight_carried() + weight <= weight_capacity());
    }
}

bool Character::has_artifact_with(const art_effect_passive effect) const
{
    if( weapon.has_effect_when_wielded( effect ) ) {
        return true;
    }
    for( auto & i : worn ) {
        if( i.has_effect_when_worn( effect ) ) {
            return true;
        }
    }
    return has_item_with( [effect]( const item & it ) {
        return it.has_effect_when_carried( effect );
    } );
}

bool Character::is_wearing(const itype_id & it) const
{
    for (auto &i : worn) {
        if (i.type->id == it) {
            return true;
        }
    }
    return false;
}

bool Character::is_wearing_on_bp(const itype_id & it, body_part bp) const
{
    for (auto &i : worn) {
        if (i.type->id == it && i.covers(bp)) {
            return true;
        }
    }
    return false;
}

bool Character::worn_with_flag( std::string flag ) const
{
    for (auto &i : worn) {
        if (i.has_flag( flag )) {
            return true;
        }
    }
    return false;
}

SkillLevel& Character::skillLevel(std::string ident)
{
    return _skills[Skill::skill(ident)];
}

SkillLevel& Character::skillLevel(const Skill* _skill)
{
    return _skills[_skill];
}

SkillLevel& Character::skillLevel(Skill const &_skill)
{
    return skillLevel(&_skill);
}

SkillLevel const& Character::get_skill_level(const Skill* _skill) const
{
    for( const auto &elem : _skills ) {
        if( elem.first == _skill ) {
            return elem.second;
        }
    }

    static SkillLevel const dummy_result;
    return dummy_result;
}

SkillLevel const& Character::get_skill_level(const Skill &_skill) const
{
    return get_skill_level(&_skill);
}

SkillLevel const& Character::get_skill_level(const std::string &ident) const
{
    const Skill* sk = Skill::skill(ident);
    return get_skill_level(sk);
}

void Character::normalize()
{
    Creature::normalize();

    ret_null = item("null", 0);
    weapon   = item("null", 0);

    recalc_hp();
}

// Actual player death is mostly handled in game::is_game_over
void Character::die(Creature* nkiller)
{
    set_killer( nkiller );
    set_turn_died(int(calendar::turn));
    if( has_effect( "lightsnare" ) ) {
        inv.add_item( item( "string_36", 0 ) );
        inv.add_item( item( "snare_trigger", 0 ) );
    }
    if( has_effect( "heavysnare" ) ) {
        inv.add_item( item( "rope_6", 0 ) );
        inv.add_item( item( "snare_trigger", 0 ) );
    }
    if( has_effect( "beartrap" ) ) {
        inv.add_item( item( "beartrap", 0 ) );
    }
    mission::on_creature_death( *this );
}

void Character::reset_stats()
{
    // Bionic buffs
    if (has_active_bionic("bio_hydraulics"))
        mod_str_bonus(20);
    if (has_bionic("bio_eye_enhancer"))
        mod_per_bonus(2);
    if (has_bionic("bio_str_enhancer"))
        mod_str_bonus(2);
    if (has_bionic("bio_int_enhancer"))
        mod_int_bonus(2);
    if (has_bionic("bio_dex_enhancer"))
        mod_dex_bonus(2);

    // Trait / mutation buffs
    if (has_trait("THICK_SCALES")) {
        mod_dex_bonus(-2);
    }
    if (has_trait("CHITIN2") || has_trait("CHITIN3") || has_trait("CHITIN_FUR3")) {
        mod_dex_bonus(-1);
    }
    if (has_trait("BIRD_EYE")) {
        mod_per_bonus(4);
    }
    if (has_trait("INSECT_ARMS")) {
        mod_dex_bonus(-2);
    }
    if (has_trait("WEBBED")) {
        mod_dex_bonus(-1);
    }
    if (has_trait("ARACHNID_ARMS")) {
        mod_dex_bonus(-4);
    }
    if (has_trait("ARM_TENTACLES") || has_trait("ARM_TENTACLES_4") ||
            has_trait("ARM_TENTACLES_8")) {
        mod_dex_bonus(1);
    }

    // Dodge-related effects
    if (has_trait("TAIL_LONG")) {
        mod_dodge_bonus(2);
    }
    if (has_trait("TAIL_CATTLE")) {
        mod_dodge_bonus(1);
    }
    if (has_trait("TAIL_RAT")) {
        mod_dodge_bonus(2);
    }
    if (has_trait("TAIL_THICK") && !(has_active_mutation("TAIL_THICK")) ) {
        mod_dodge_bonus(1);
    }
    if (has_trait("TAIL_RAPTOR")) {
        mod_dodge_bonus(3);
    }
    if (has_trait("TAIL_FLUFFY")) {
        mod_dodge_bonus(4);
    }
    if (has_trait("WINGS_BAT")) {
        mod_dodge_bonus(-3);
    }
    if (has_trait("WINGS_BUTTERFLY")) {
        mod_dodge_bonus(-4);
    }

    if (str_max >= 16) {mod_dodge_bonus(-1);} // Penalty if we're huge
    else if (str_max <= 5) {mod_dodge_bonus(1);} // Bonus if we're small

    nv_cached = false;

    // Has to be at the end because it applies the bonuses
    Creature::reset_stats();
}

bool Character::has_nv()
{
    static bool nv = false;

    if( !nv_cached ) {
        nv_cached = true;
        nv = (worn_with_flag("GNV_EFFECT") ||
              has_active_bionic("bio_night_vision"));
    }

    return nv;
}
