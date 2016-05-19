#include "character.h"
#include "game.h"
#include "map.h"
#include "map_selector.h"
#include "vehicle_selector.h"
#include "debug.h"
#include "mission.h"
#include "translations.h"
#include "options.h"
#include "map_iterator.h"
#include "field.h"
#include "messages.h"
#include "input.h"
#include "monster.h"
#include "mtype.h"
#include "player.h"
#include "mutation.h"
#include "vehicle.h"

const efftype_id effect_beartrap( "beartrap" );
const efftype_id effect_bite( "bite" );
const efftype_id effect_bleed( "bleed" );
const efftype_id effect_blind( "blind" );
const efftype_id effect_boomered( "boomered" );
const efftype_id effect_contacts( "contacts" );
const efftype_id effect_crushed( "crushed" );
const efftype_id effect_darkness( "darkness" );
const efftype_id effect_downed( "downed" );
const efftype_id effect_grabbed( "grabbed" );
const efftype_id effect_heavysnare( "heavysnare" );
const efftype_id effect_infected( "infected" );
const efftype_id effect_in_pit( "in_pit" );
const efftype_id effect_lightsnare( "lightsnare" );
const efftype_id effect_webbed( "webbed" );

const skill_id skill_throw( "throw" );

const std::string debug_nodmg( "DEBUG_NODMG" );

Character::Character() : Creature(), visitable<Character>()
{
    str_max = 0;
    dex_max = 0;
    per_max = 0;
    int_max = 0;
    str_cur = 0;
    dex_cur = 0;
    per_cur = 0;
    int_cur = 0;
    str_bonus = 0;
    dex_bonus = 0;
    per_bonus = 0;
    int_bonus = 0;
    healthy = 0;
    healthy_mod = 0;
    hunger = 0;
    thirst = 0;
    fatigue = 0;
    stomach_food = 0;
    stomach_water = 0;

    name = "";
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

void Character::mod_stat( const std::string &stat, int modifier )
{
    if( stat == "str" ) {
        mod_str_bonus( modifier );
    } else if( stat == "dex" ) {
        mod_dex_bonus( modifier );
    } else if( stat == "per" ) {
        mod_per_bonus( modifier );
    } else if( stat == "int" ) {
        mod_int_bonus( modifier );
    } else if( stat == "healthy" ) {
        mod_healthy( modifier );
    } else if( stat == "hunger" ) {
        mod_hunger( modifier );
    } else if( stat == "speed" ) {
        mod_speed_bonus( modifier );
    } else if( stat == "dodge" ) {
        mod_dodge_bonus( modifier );
    } else if( stat == "block" ) {
        mod_block_bonus( modifier );
    } else if( stat == "hit" ) {
        mod_hit_bonus( modifier );
    } else if( stat == "bash" ) {
        mod_bash_bonus( modifier );
    } else if( stat == "cut" ) {
        mod_cut_bonus( modifier );
    } else if( stat == "pain" ) {
        mod_pain( modifier );
    } else if( stat == "moves" ) {
        mod_moves( modifier );
    } else {
        Creature::mod_stat( stat, modifier );
    }
}

int Character::aim_per_time( const item& gun, int recoil ) const
{
    int penalty = 0;

    // Range [0 - 10] after adjustment
    penalty += skill_dispersion( gun, false ) / 60;

    // Ranges [0 - 12] after adjustment
    penalty += ranged_dex_mod() / 15;

    // Range [0 - 10]
    penalty += gun.aim_speed( recoil );

    // @todo consider character status effects

    // always improve by at least 1MOC
    penalty = std::max( 1, 32 - penalty );

    // improvement capped by max aim level of the gun sight being used.
    return std::min( penalty, recoil - gun.sight_dispersion( recoil ) );
}

bool Character::move_effects(bool attacking)
{
    if (has_effect( effect_downed )) {
        ///\EFFECT_DEX increases chance to stand up when knocked down

        ///\EFFECT_STR increases chance to stand up when knocked down, slightly
        if (rng(0, 40) > get_dex() + get_str() / 2) {
            add_msg_if_player(_("You struggle to stand."));
        } else {
            add_msg_player_or_npc(m_good, _("You stand up."),
                                    _("<npcname> stands up."));
            remove_effect( effect_downed);
        }
        return false;
    }
    if (has_effect( effect_webbed )) {
        ///\EFFECT_STR increases chance to escape webs
        if (x_in_y(get_str(), 6 * get_effect_int( effect_webbed ))) {
            add_msg_player_or_npc(m_good, _("You free yourself from the webs!"),
                                    _("<npcname> frees themselves from the webs!"));
            remove_effect( effect_webbed);
        } else {
            add_msg_if_player(_("You try to free yourself from the webs, but can't get loose!"));
        }
        return false;
    }
    if (has_effect( effect_lightsnare )) {
        ///\EFFECT_STR increases chance to escape light snare

        ///\EFFECT_DEX increases chance to escape light snare
        if(x_in_y(get_str(), 12) || x_in_y(get_dex(), 8)) {
            remove_effect( effect_lightsnare);
            add_msg_player_or_npc(m_good, _("You free yourself from the light snare!"),
                                    _("<npcname> frees themselves from the light snare!"));
            item string("string_36", calendar::turn);
            item snare("snare_trigger", calendar::turn);
            g->m.add_item_or_charges(pos(), string);
            g->m.add_item_or_charges(pos(), snare);
        } else {
            add_msg_if_player(m_bad, _("You try to free yourself from the light snare, but can't get loose!"));
        }
        return false;
    }
    if (has_effect( effect_heavysnare )) {
        ///\EFFECT_STR increases chance to escape heavy snare, slightly

        ///\EFFECT_DEX increases chance to escape light snare
        if(x_in_y(get_str(), 32) || x_in_y(get_dex(), 16)) {
            remove_effect( effect_heavysnare);
            add_msg_player_or_npc(m_good, _("You free yourself from the heavy snare!"),
                                    _("<npcname> frees themselves from the heavy snare!"));
            item rope("rope_6", calendar::turn);
            item snare("snare_trigger", calendar::turn);
            g->m.add_item_or_charges(pos(), rope);
            g->m.add_item_or_charges(pos(), snare);
        } else {
            add_msg_if_player(m_bad, _("You try to free yourself from the heavy snare, but can't get loose!"));
        }
        return false;
    }
    if (has_effect( effect_beartrap )) {
        /* Real bear traps can't be removed without the proper tools or immense strength; eventually this should
           allow normal players two options: removal of the limb or removal of the trap from the ground
           (at which point the player could later remove it from the leg with the right tools).
           As such we are currently making it a bit easier for players and NPC's to get out of bear traps.
        */
        ///\EFFECT_STR increases chance to escape bear trap
        if(x_in_y(get_str(), 100)) {
            remove_effect( effect_beartrap);
            add_msg_player_or_npc(m_good, _("You free yourself from the bear trap!"),
                                    _("<npcname> frees themselves from the bear trap!"));
            item beartrap("beartrap", calendar::turn);
            g->m.add_item_or_charges(pos(), beartrap);
        } else {
            add_msg_if_player(m_bad, _("You try to free yourself from the bear trap, but can't get loose!"));
        }
        return false;
    }
    if (has_effect( effect_crushed )) {
        ///\EFFECT_STR increases chance to escape crushing rubble

        ///\EFFECT_DEX increases chance to escape crushing rubble, slightly
        if(x_in_y(get_str() + get_dex() / 4, 100)) {
            remove_effect( effect_crushed);
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
    if (has_effect( effect_in_pit )) {
        ///\EFFECT_STR increases chance to escape pit

        ///\EFFECT_DEX increases chance to escape pit, slightly
        if (rng(0, 40) > get_str() + get_dex() / 2) {
            add_msg_if_player(m_bad, _("You try to escape the pit, but slip back in."));
            return false;
        } else {
            add_msg_player_or_npc(m_good, _("You escape the pit!"),
                                    _("<npcname> escapes the pit!"));
            remove_effect( effect_in_pit);
        }
    }
    if( has_effect( effect_grabbed ) && !attacking ) {
        int zed_number = 0;
        for( auto &&dest : g->m.points_in_radius( pos(), 1, 0 ) ){
            if( g->mon_at( dest ) != -1 &&
                ( g->zombie( g->mon_at( dest ) ).has_flag( MF_GRABS ) ||
                  g->zombie( g->mon_at( dest ) ).type->has_special_attack( "GRAB" ) ) ) {
                zed_number ++;
            }
        }
        if( zed_number == 0 ) {
            add_msg_player_or_npc( m_good, _( "You find yourself no longer grabbed." ),
                                   _( "<npcname> finds themselves no longer grabbed." ) );
            remove_effect( effect_grabbed );
        ///\EFFECT_DEX increases chance to escape grab, if >STR

        ///\EFFECT_STR increases chance to escape grab, if >DEX
        } else if( rng( 0, std::max( get_dex(), get_str() ) ) < rng( get_effect_int( effect_grabbed ), 8 ) ) {
            // Randomly compare higher of dex or str to grab intensity.
            add_msg_player_or_npc( m_bad, _( "You try break out of the grab, but fail!" ),
                                   _( "<npcname> tries to break out of the grab, but fails!" ) );
            return false;
        } else {
            add_msg_player_or_npc( m_good, _( "You break out of the grab!" ),
                                   _( "<npcname> breaks out of the grab!" ) );
            remove_effect( effect_grabbed );
        }
    }
    return Creature::move_effects( attacking );
}

void Character::add_effect( const efftype_id &eff_id, int dur, body_part bp,
                            bool permanent, int intensity, bool force )
{
    Creature::add_effect( eff_id, dur, bp, permanent, intensity, force );
}

void Character::process_turn()
{
    Creature::process_turn();
    drop_inventory_overflow();
}

void Character::recalc_hp()
{
    int new_max_hp[num_hp_parts];
    for( auto &elem : new_max_hp ) {
        ///\EFFECT_STR_MAX increases base hp
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
    if( has_trait( "GLASSJAW" ) ) {
        new_max_hp[hp_head] *= 0.8;
    }
    for( int i = 0; i < num_hp_parts; i++ ) {
        hp_cur[i] *= (float)new_max_hp[i] / (float)hp_max[i];
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
    vision_mode_cache.reset();

    // Set sight_max.
    if( is_blind() ) {
        sight_max = 0;
    } else if( has_effect( effect_boomered ) && (!(has_trait("PER_SLIME_OK"))) ) {
        sight_max = 1;
        vision_mode_cache.set( BOOMERED );
    } else if (has_effect( effect_in_pit ) ||
            (underwater && !has_bionic("bio_membrane") &&
                !has_trait("MEMBRANE") && !worn_with_flag("SWIM_GOGGLES") &&
                !has_trait("CEPH_EYES") && !has_trait("PER_SLIME_OK") ) ) {
        sight_max = 1;
    } else if (has_active_mutation("SHELL2")) {
        // You can kinda see out a bit.
        sight_max = 2;
    } else if ( (has_trait("MYOPIC") || has_trait("URSINE_EYE")) &&
            !is_wearing("glasses_eye") && !is_wearing("glasses_monocle") &&
            !is_wearing("glasses_bifocal") && !has_effect( effect_contacts )) {
        sight_max = 4;
    } else if (has_trait("PER_SLIME")) {
        sight_max = 6;
    } else if( has_effect( effect_darkness ) ) {
        vision_mode_cache.set( DARKNESS );
        sight_max = 10;
    }

    // Debug-only NV, by vache's request
    if( has_trait("DEBUG_NIGHTVISION") ) {
        vision_mode_cache.set( DEBUG_NIGHTVISION );
    }
    if( has_nv() ) {
        vision_mode_cache.set( NV_GOGGLES );
    }
    if( has_active_mutation("NIGHTVISION3") || is_wearing("rm13_armor_on") ) {
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

    // Not exactly a sight limit thing, but related enough
    if( has_active_bionic( "bio_infrared" ) ||
        has_trait( "INFRARED" ) ||
        has_trait( "LIZ_IR" ) ||
        worn_with_flag( "IR_EFFECT" ) ) {
        vision_mode_cache.set( IR_VISION );
    }

    if( has_artifact_with( AEP_SUPER_CLAIRVOYANCE ) ) {
        vision_mode_cache.set( VISION_CLAIRVOYANCE_SUPER );
    }

    if( has_artifact_with( AEP_CLAIRVOYANCE ) ) {
        vision_mode_cache.set( VISION_CLAIRVOYANCE );
    }
}

float Character::get_vision_threshold(int light_level) const {
    // Bail out in extremely common case where character hs no special vision mode or
    // it's too bright for nightvision to work.
    if( vision_mode_cache.none() || light_level > LIGHT_AMBIENT_LIT ) {
        return LIGHT_AMBIENT_LOW;
    }
    // As light_level goes from LIGHT_AMBIENT_MINIMAL to LIGHT_AMBIENT_LIT,
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
        return 0.01;
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

map_selector Character::nearby( int radius, bool accessible )
{
    return map_selector( pos(), radius, accessible );
}

item& Character::i_add(item it)
{
    itype_id item_type_id = "null";
    if( it.type ) {
        item_type_id = it.type->id;
    }

    last_item = item_type_id;

    if( it.is_food() || it.is_ammo() || it.is_gun()  || it.is_armor() ||
        it.is_book() || it.is_tool() || it.is_weap() || it.is_food_container() ) {
        inv.unsort();
    }

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

std::list<item> Character::remove_worn_items_with( std::function<bool(item &)> filter )
{
    std::list<item> result;
    for( auto iter = worn.begin(); iter != worn.end(); ) {
        if( filter( *iter ) ) {
            result.splice( result.begin(), worn, iter++ );
        } else {
            ++iter;
        }
    }
    return result;
}

// Negative positions indicate weapon/clothing, 0 & positive indicate inventory
const item& Character::i_at(int position) const
{
    if( position == -1 ) {
        return weapon;
    }
    if( position < -1 ) {
        int worn_index = worn_position_to_index(position);
        if (size_t(worn_index) < worn.size()) {
            auto iter = worn.begin();
            std::advance( iter, worn_index );
            return *iter;
        }
    }

    return inv.find_item(position);
}

item& Character::i_at(int position)
{
    return const_cast<item&>( const_cast<const Character*>(this)->i_at( position ) );
}

int Character::get_item_position( const item *it ) const
{
    if( weapon.has_item( *it ) ) {
        return -1;
    }

    int p = 0;
    for( const auto &e : worn ) {
        if( e.has_item( *it ) ) {
            return worn_position_to_index( p );
        }
        p++;
    }

    return inv.position_by_item( it );
}

item Character::i_rem(int pos)
{
 item tmp;
 if (pos == -1) {
     tmp = weapon;
     weapon = ret_null;
     return tmp;
 } else if (pos < -1 && pos > worn_position_to_index(worn.size())) {
     auto iter = worn.begin();
     std::advance( iter, worn_position_to_index( pos ) );
     tmp = *iter;
     worn.erase( iter );
     return tmp;
 }
 return inv.remove_item(pos);
}

item Character::i_rem(const item *it)
{
    auto tmp = remove_items_with( [&it] (const item &i) { return &i == it; }, 1 );
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
        if( drop ) {
            retval &= !g->m.add_item_or_charges( pos(), it ).is_null();
        } else if ( !( it.has_flag("IRREMOVABLE") && !it.is_gun() ) ){
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

std::vector<const item *> Character::get_ammo( const ammotype &at ) const
{
    return items_with( [at]( const item & it ) {
        return it.is_ammo() && it.ammo_type() == at;
    } );
}

template <typename T, typename Output>
void find_ammo_helper( T& src, const item& obj, bool empty, Output out, bool nested ) {
    if( obj.magazine_integral() ) {
        // find suitable ammo excluding that already loaded in magazines
        ammotype ammo = obj.ammo_type();

        src.visit_items( [&src,&nested,&out,ammo]( item *node ) {
            if( node->is_magazine() || node->is_gun() || node->is_tool() ) {
                // guns/tools never contain usable ammo so most efficient to skip them now
                return VisitResponse::SKIP;
            }
            if( !node->made_of( SOLID ) ) {
                // some liquids are ammo but we can't reload with them unless within a container
                return VisitResponse::SKIP;
            }
            if( node->is_ammo_container() && !node->contents[0].made_of( SOLID ) ) {
                if( node->contents[0].ammo_type() == ammo ) {
                    out = item_location( src, node );
                }
                return VisitResponse::SKIP;
            }
            if( node->is_ammo() && node->ammo_type() == ammo ) {
                out = item_location( src, node );
            }
            return nested ? VisitResponse::NEXT : VisitResponse::SKIP;
        } );

    } else {
        // find compatible magazines excluding those already loaded in tools/guns
        const auto mags = obj.magazine_compatible();

        src.visit_items( [&src,&nested,&out,mags,empty]( item *node ) {
            if( node->is_gun() || node->is_tool() ) {
                return VisitResponse::SKIP;
            }
            if( node->is_magazine() ) {
                if ( mags.count( node->typeId() ) && ( node->ammo_remaining() || empty ) ) {
                    out = item_location( src, node );
                }
                return VisitResponse::SKIP;
            }
            return nested ? VisitResponse::NEXT : VisitResponse::SKIP;
        } );
    }
}

std::vector<item_location> Character::find_ammo( const item& obj, bool empty, int radius )
{
    std::vector<item_location> res;

    find_ammo_helper( *this, obj, empty, std::back_inserter( res ), true );

    if( radius >= 0 ) {
        for( auto& cursor : map_selector( pos(), radius ) ) {
            find_ammo_helper( cursor, obj, empty, std::back_inserter( res ), false );
        }
        for( auto& cursor : vehicle_selector( pos(), radius ) ) {
            find_ammo_helper( cursor, obj, empty, std::back_inserter( res ), false );
        }
    }

    return res;
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
    ///\EFFECT_STR increases carrying capacity
    ret += get_str() * 4000;
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
    int ret = 0;
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
    ret = std::max(ret, 0);
    return ret;
}

bool Character::can_pickVolume( int volume, bool ) const
{
   return volume_carried() + volume <= volume_capacity();
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

void Character::drop_inventory_overflow() {
    if( volume_carried() > volume_capacity() ) {
        for( auto &item_to_drop :
               inv.remove_randomly_by_volume( volume_carried() - volume_capacity() ) ) {
            g->m.add_item_or_charges( pos(), item_to_drop );
        }
        add_msg_if_player( m_bad, _("Some items tumble to the ground.") );
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

bool Character::worn_with_flag( const std::string &flag ) const
{
    return std::any_of( worn.begin(), worn.end(), [&flag]( const item &it ) {
        return it.has_flag( flag );
    } );
}

SkillLevel& Character::get_skill_level(const skill_id &ident)
{
    if( !ident ) {
        static SkillLevel none;
        none.level( 0 );
        return none;
    }
    return _skills[ident];
}

SkillLevel const& Character::get_skill_level(const skill_id &ident) const
{
    if( !ident ) {
        static const SkillLevel none{};
        return none;
    }

    const auto iter = _skills.find( ident );
    if( iter != _skills.end() ) {
        return iter->second;
    }

    static SkillLevel const dummy_result;
    return dummy_result;
}

void Character::set_skill_level( const skill_id &ident, const int level )
{
    get_skill_level( ident ).level( level );
}

void Character::boost_skill_level( const skill_id &ident, const int delta )
{
    set_skill_level( ident, delta + get_skill_level( ident ) );
}

bool Character::meets_skill_requirements( const std::map<skill_id, int> &req ) const
{
    return std::all_of( req.begin(), req.end(), [this]( const std::pair<skill_id, int> &pr ) {
        return get_skill_level( pr.first ) >= pr.second;
    });
}

int Character::skill_dispersion( const item& gun, bool random ) const
{
    static skill_id skill_gun( "gun" );

    int dispersion = 0; // Measured in Minutes of Arc.

    const int lvl = get_skill_level( gun.gun_skill() );
    if( lvl < 10 ) {
        // Up to 0.75 degrees for each skill point < 10.
        ///\EFFECT_PISTOL <10 randomly increases dispersion for pistols
        ///\EFFECT_SMG <10 randomly increases dispersion for smgs
        ///\EFFECT_RIFLE <10 randomly increases dispersion for rifles
        ///\EFFECT_LAUNCHER <10 randomly increases dispersion for launchers
        dispersion += 45 * ( 10 - lvl );
    }

    if( get_skill_level( skill_gun ) < 10 ) {
        // Up to 0.25 deg per each skill point < 10.
        ///\EFFECT_GUN <10 randomly increased dispersion of all gunfire
        dispersion += 15 * ( 10 - lvl );
    }

    return random ? rng(0, dispersion) : dispersion;
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
    if( has_effect( effect_lightsnare ) ) {
        inv.add_item( item( "string_36", 0 ) );
        inv.add_item( item( "snare_trigger", 0 ) );
    }
    if( has_effect( effect_heavysnare ) ) {
        inv.add_item( item( "rope_6", 0 ) );
        inv.add_item( item( "snare_trigger", 0 ) );
    }
    if( has_effect( effect_beartrap ) ) {
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

    ///\EFFECT_STR_MAX above 15 decreases Dodge bonus by 1 (NEGATIVE)
    if (str_max >= 16) {mod_dodge_bonus(-1);} // Penalty if we're huge
    ///\EFFECT_STR_MAX below 6 increases Dodge bonus by 1
    else if (str_max <= 5) {mod_dodge_bonus(1);} // Bonus if we're small

    nv_cached = false;

    // Reset our stats to normal levels
    // Any persistent buffs/debuffs will take place in effects,
    // player::suffer(), etc.

    // repopulate the stat fields
    str_cur = str_max + get_str_bonus();
    dex_cur = dex_max + get_dex_bonus();
    per_cur = per_max + get_per_bonus();
    int_cur = int_max + get_int_bonus();

    // Floor for our stats.  No stat changes should occur after this!
    if( dex_cur < 0 ) {
        dex_cur = 0;
    }
    if( str_cur < 0 ) {
        str_cur = 0;
    }
    if( per_cur < 0 ) {
        per_cur = 0;
    }
    if( int_cur < 0 ) {
        int_cur = 0;
    }

    // Does nothing! TODO: Remove
    Creature::reset_stats();
}

void Character::reset()
{
    // TODO: Move reset_stats here, remove it from Creature
    Creature::reset();
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

void Character::reset_encumbrance()
{
    encumbrance_cache = calc_encumbrance();
}

std::array<encumbrance_data, num_bp> Character::calc_encumbrance() const
{
    return calc_encumbrance( ret_null );
}

std::array<encumbrance_data, num_bp> Character::calc_encumbrance( const item &new_item ) const
{
    std::array<encumbrance_data, num_bp> ret;

    item_encumb( ret, new_item );
    mut_cbm_encumb( ret );

    return ret;
}

std::array<encumbrance_data, num_bp> Character::get_encumbrance() const
{
    return encumbrance_cache;
}

std::array<encumbrance_data, num_bp> Character::get_encumbrance( const item &new_item ) const
{
    return calc_encumbrance( new_item );
}

using layer_data = std::array<int, MAX_CLOTHING_LAYER>;

void layer_item( std::array<encumbrance_data, num_bp> &vals,
                 std::array<layer_data, num_bp> &layers,
                 const item &it, bool power_armor )
{
    const auto item_layer = it.get_layer();
    int encumber_val = it.get_encumber();
    // For the purposes of layering penalty, set a min of 2 and a max of 10 per item.
    int layering_encumbrance = std::min( 10, std::max( 2, encumber_val ) );

    const int armorenc = !power_armor || !it.is_power_armor() ?
        encumber_val : std::max( 0, encumber_val - 40 );

    for( size_t i = 0; i < num_bp; i++ ) {
        body_part bp = body_part( i );
        if( !it.covers( bp ) ) {
            continue;
        }

        int &this_layer = layers[i][item_layer];
        this_layer = std::max( this_layer, layering_encumbrance );

        vals[i].armor_encumbrance += armorenc;
        vals[i].layer_penalty += layering_encumbrance;
    }
}

bool Character::is_wearing_active_power_armor() const
{
    for( const auto &w : worn ) {
        if( w.is_power_armor() && w.active ) {
            return true;
        }
    }
    return false;
}

/*
 * Encumbrance logic:
 * Some clothing is intrinsically encumbering, such as heavy jackets, backpacks, body armor, etc.
 * These simply add their encumbrance value to each body part they cover.
 * In addition, each article of clothing after the first in a layer imposes an additional penalty.
 * e.g. one shirt will not encumber you, but two is tight and starts to restrict movement.
 * Clothes on seperate layers don't interact, so if you wear e.g. a light jacket over a shirt,
 * they're intended to be worn that way, and don't impose a penalty.
 * The default is to assume that clothes do not fit, clothes that are "fitted" either
 * reduce the encumbrance penalty by ten, or if that is already 0, they reduce the layering effect.
 *
 * Use cases:
 * What would typically be considered normal "street clothes" should not be considered encumbering.
 * Tshirt, shirt, jacket on torso/arms, underwear and pants on legs, socks and shoes on feet.
 * This is currently handled by each of these articles of clothing
 * being on a different layer and/or body part, therefore accumulating no encumbrance.
 */
void Character::item_encumb( std::array<encumbrance_data, num_bp> &vals, const item &new_item ) const
{
    // The highest encumbrance of any one item on the layer.
    std::array<layer_data, num_bp> layers = {{
        {{}}, {{}}, {{}}, {{}}, {{}}, {{}}, {{}}, {{}}, {{}}, {{}}, {{}}, {{}}
    }};

    const bool power_armored = is_wearing_active_power_armor();
    for( auto& w : worn ) {
        layer_item( vals, layers, w, power_armored );
    }

    if( !new_item.is_null() ) {
        layer_item( vals, layers, new_item, power_armored );
    }

    // The stacking penalty applies by doubling the encumbrance of
    // each item except the highest encumbrance one.
    // So we add them together and then subtract out the highest.
    for( size_t i = 0; i < num_bp; i++ ) {
        for( size_t j = 0; j < MAX_CLOTHING_LAYER; j++ ) {
            vals[i].layer_penalty -= std::max( 0, layers[i][j] );
        }
    }

    // Make sure the values are sane
    for( auto &elem : vals ) {
        elem.armor_encumbrance = std::max( 0, elem.armor_encumbrance );
        elem.layer_penalty = std::max( 0, elem.layer_penalty );
        // Add armor and layering penalties for the final values
        elem.encumbrance += elem.armor_encumbrance + elem.layer_penalty;
    }
}

int Character::encumb( body_part bp ) const
{
    return encumbrance_cache[bp].encumbrance;
}

void apply_mut_encumbrance( std::array<encumbrance_data, num_bp> &vals,
                            const mutation_branch &mut,
                            const std::bitset<num_bp> &oversize )
{
    for( const auto &enc : mut.encumbrance_always ) {
        vals[enc.first].encumbrance += enc.second;
    }

    for( const auto &enc : mut.encumbrance_covered ) {
        if( !oversize.test( enc.first ) ) {
            vals[enc.first].encumbrance += enc.second;
        }
    }
}

void Character::mut_cbm_encumb( std::array<encumbrance_data, num_bp> &vals ) const
{
    if( has_bionic("bio_stiff") ) {
        // All but head, mouth and eyes
        for( auto &val : vals ) {
            val.encumbrance += 10;
        }

        vals[bp_head].encumbrance -= 10;
        vals[bp_mouth].encumbrance -= 10;
        vals[bp_eyes].encumbrance -= 10;
    }

    if( has_bionic("bio_nostril") ) {
        vals[bp_mouth].encumbrance += 10;
    }
    if( has_bionic("bio_thumbs") ) {
        vals[bp_hand_l].encumbrance += 10;
        vals[bp_hand_r].encumbrance += 10;
    }
    if( has_bionic("bio_pokedeye") ) {
        vals[bp_eyes].encumbrance += 10;
    }

    // Lower penalty for bps covered only by XL armor
    const auto oversize = exclusive_flag_coverage( "OVERSIZE" );
    for( const auto &mut_pair : my_mutations ) {
        const auto &branch = mutation_branch::get( mut_pair.first );
        apply_mut_encumbrance( vals, branch, oversize );
    }
}

std::bitset<num_bp> Character::exclusive_flag_coverage( const std::string &flag ) const
{
    std::bitset<num_bp> ret;
    ret.set();
    for( const auto &elem : worn ) {
        if( !elem.has_flag( flag ) ) {
            // Unset the parts covered by this item
            ret &= ( ~elem.get_covered_body_parts() );
        }
    }

    return ret;
}

/*
 * Innate stats getters
 */

// get_stat() always gets total (current) value, NEVER just the base
// get_stat_bonus() is always just the bonus amount
int Character::get_str() const
{
    return std::max(0, str_max + str_bonus);
}
int Character::get_dex() const
{
    return std::max(0, dex_max + dex_bonus);
}
int Character::get_per() const
{
    return std::max(0, per_max + per_bonus);
}
int Character::get_int() const
{
    return std::max(0, int_max + int_bonus);
}

int Character::get_str_base() const
{
    return str_max;
}
int Character::get_dex_base() const
{
    return dex_max;
}
int Character::get_per_base() const
{
    return per_max;
}
int Character::get_int_base() const
{
    return int_max;
}



int Character::get_str_bonus() const
{
    return str_bonus;
}
int Character::get_dex_bonus() const
{
    return dex_bonus;
}
int Character::get_per_bonus() const
{
    return per_bonus;
}
int Character::get_int_bonus() const
{
    return int_bonus;
}

int Character::ranged_dex_mod() const
{
    ///\EFFECT_DEX <12 increases ranged penalty
    return std::max( ( 12 - get_dex() ) * 15, 0 );
}

int Character::ranged_per_mod() const
{
    ///\EFFECT_PER <12 increases ranged penalty
    return std::max( ( 12 - get_per() ) * 15, 0 );
}

int Character::get_healthy() const
{
    return healthy;
}
int Character::get_healthy_mod() const
{
    return healthy_mod;
}

/*
 * Innate stats setters
 */

void Character::set_str_bonus(int nstr)
{
    str_bonus = nstr;
}
void Character::set_dex_bonus(int ndex)
{
    dex_bonus = ndex;
}
void Character::set_per_bonus(int nper)
{
    per_bonus = nper;
}
void Character::set_int_bonus(int nint)
{
    int_bonus = nint;
}
void Character::mod_str_bonus(int nstr)
{
    str_bonus += nstr;
}
void Character::mod_dex_bonus(int ndex)
{
    dex_bonus += ndex;
}
void Character::mod_per_bonus(int nper)
{
    per_bonus += nper;
}
void Character::mod_int_bonus(int nint)
{
    int_bonus += nint;
}

void Character::set_healthy(int nhealthy)
{
    healthy = nhealthy;
}
void Character::mod_healthy(int nhealthy)
{
    healthy += nhealthy;
}
void Character::set_healthy_mod(int nhealthy_mod)
{
    healthy_mod = nhealthy_mod;
}
void Character::mod_healthy_mod(int nhealthy_mod, int cap)
{
    // TODO: This really should be a full morale-like system, with per-effect caps
    //       and durations.  This version prevents any single effect from exceeding its
    //       intended ceiling, but multiple effects will overlap instead of adding.

    // Cap indicates how far the mod is allowed to shift in this direction.
    // It can have a different sign to the mod, e.g. for items that treat
    // extremely low health, but can't make you healthy.
    int low_cap;
    int high_cap;
    if( nhealthy_mod < 0 ) {
        low_cap = cap;
        high_cap = 200;
    } else {
        low_cap = -200;
        high_cap = cap;
    }

    // If we're already out-of-bounds, we don't need to do anything.
    if( (healthy_mod <= low_cap && nhealthy_mod < 0) ||
        (healthy_mod >= high_cap && nhealthy_mod > 0) ) {
        return;
    }

    healthy_mod += nhealthy_mod;

    // Since we already bailed out if we were out-of-bounds, we can
    // just clamp to the boundaries here.
    healthy_mod = std::min( healthy_mod, high_cap );
    healthy_mod = std::max( healthy_mod, low_cap );
}

int Character::get_hunger() const
{
    return hunger;
}

void Character::mod_hunger(int nhunger)
{
    set_hunger( hunger + nhunger );
}

void Character::set_hunger(int nhunger)
{
    if( hunger != nhunger ) {
        hunger = nhunger;
        on_stat_change( "hunger", hunger );
    }
}

int Character::get_thirst() const
{
    return thirst;
}

void Character::mod_thirst(int nthirst)
{
    set_thirst( thirst + nthirst );
}

void Character::set_thirst(int nthirst)
{
    if( thirst != nthirst ) {
        thirst = nthirst;
        on_stat_change( "thirst", thirst );
    }
}

int Character::get_stomach_food() const
{
    return stomach_food;
}
void Character::mod_stomach_food(int n_stomach_food)
{
    stomach_food = std::max(0, stomach_food + n_stomach_food);
}
void Character::set_stomach_food(int n_stomach_food)
{
    stomach_food = std::max(0, n_stomach_food);
}
int Character::get_stomach_water() const
{
    return stomach_water;
}
void Character::mod_stomach_water(int n_stomach_water)
{
    stomach_water = std::max(0, stomach_water + n_stomach_water);
}
void Character::set_stomach_water(int n_stomach_water)
{
    stomach_water = std::max(0, n_stomach_water);
}

void Character::mod_fatigue(int nfatigue)
{
    set_fatigue(fatigue + nfatigue);
}

void Character::set_fatigue(int nfatigue)
{
    nfatigue = std::max( nfatigue, -1000 );
    if( fatigue != nfatigue ) {
        fatigue = nfatigue;
        on_stat_change( "fatigue", fatigue );
    }
}

int Character::get_fatigue() const
{
    return fatigue;
}

void Character::reset_bonuses()
{
    // Reset all bonuses to 0 and mults to 1.0
    str_bonus = 0;
    dex_bonus = 0;
    per_bonus = 0;
    int_bonus = 0;

    reset_encumbrance();

    Creature::reset_bonuses();
}

void Character::update_health(int external_modifiers)
{
    // Limit healthy_mod to [-200, 200].
    // This also sets approximate bounds for the character's health.
    if( get_healthy_mod() > 200 ) {
        set_healthy_mod( 200 );
    } else if( get_healthy_mod() < -200 ) {
        set_healthy_mod( -200 );
    }

    // Over the long run, health tends toward healthy_mod.
    int break_even = get_healthy() - get_healthy_mod() + external_modifiers;

    // But we allow some random variation.
    const long roll = rng( -100, 100 );
    if( roll > break_even ) {
        mod_healthy( 1 );
    } else if( roll < break_even ) {
        mod_healthy( -1 );
    }

    // And healthy_mod decays over time.
    set_healthy_mod( get_healthy_mod() * 3 / 4 );

    add_msg( m_debug, "Health: %d, Health mod: %d", get_healthy(), get_healthy_mod() );
}

int Character::get_dodge_base() const
{
    ///\EFFECT_DEX increases dodge base
    return Creature::get_dodge_base() + (get_dex() / 2);
}
int Character::get_hit_base() const
{
    ///\EFFECT_DEX increases hit base, slightly
    return Creature::get_hit_base() + (get_dex() / 4) + 3;
}

hp_part Character::body_window( bool precise ) const
{
    return body_window( disp_name(), true, precise, 0, 0, 0, 0, 0, 0 );
}

hp_part Character::body_window( const std::string &menu_header,
                                bool show_all, bool precise,
                                int normal_bonus, int head_bonus, int torso_bonus,
                                bool bleed, bool bite, bool infect ) const
{
    WINDOW *hp_window = newwin(10, 31, (TERMY - 10) / 2, (TERMX - 31) / 2);
    draw_border(hp_window);

    trim_and_print( hp_window, 1, 1, getmaxx(hp_window) - 2, c_ltred, menu_header.c_str() );
    const int y_off = 2; // 1 for border, 1 for header

    /* This struct estabiles some kind of connection between the hp_part (which can be healed and
     * have HP) and the body_part. Note that there are more body_parts than hp_parts. For example:
     * Damage to bp_head, bp_eyes and bp_mouth is all applied on the HP of hp_head. */
    struct healable_bp {
        mutable bool allowed;
        body_part bp;
        hp_part hp;
        std::string name; // Translated name as it appears in the menu.
        int bonus;
    };
    /* The array of the menu entries show to the player. The entries are displayed in this order,
     * it may be changed here. */
    std::array<healable_bp, num_hp_parts> parts = { {
        { false, bp_head, hp_head, _("Head"), head_bonus },
        { false, bp_torso, hp_torso, _("Torso"), torso_bonus },
        { false, bp_arm_l, hp_arm_l, _("Left Arm"), normal_bonus },
        { false, bp_arm_r, hp_arm_r, _("Right Arm"), normal_bonus },
        { false, bp_leg_l, hp_leg_l, _("Left Leg"), normal_bonus },
        { false, bp_leg_r, hp_leg_r, _("Right Leg"), normal_bonus },
    } };

    for( size_t i = 0; i < parts.size(); i++ ) {
        const auto &e = parts[i];
        const body_part bp = e.bp;
        const hp_part hp = e.hp;
        const int maximal_hp = hp_max[hp];
        const int current_hp = hp_cur[hp];
        const int bonus = e.bonus;
        // This will c_ltgray if the part does not have any effects cured by the item
        // (e.g. it cures only bites, but the part does not have a bite effect)
        const nc_color state_col = limb_color( bp, bleed, bite, infect );
        const bool has_curable_effect = state_col != c_ltgray;
        // The same as in the main UI sidebar. Independent of the capability of the healing item!
        const nc_color all_state_col = limb_color( bp, true, true, true );
        const bool has_any_effect = all_state_col != c_ltgray;
        // Broken means no HP can be restored, it requires surgical attention.
        const bool limb_is_broken = current_hp == 0;
        // This considers only the effects that can *not* be removed.
        const nc_color new_state_col = limb_color( bp, !bleed, !bite, !infect );

        if( show_all ) {
            e.allowed = true;
        } else if( has_curable_effect ) {
            e.allowed = true;
        } else if( limb_is_broken ) {
            continue;
        } else if( current_hp < maximal_hp && e.bonus != 0 ) {
            e.allowed = true;
        } else {
            continue;
        }

        const int line = i + y_off;

        const nc_color color = show_all ? c_green : state_col;
        mvwprintz( hp_window, line, 1, color, "%d: %s", i + 1, e.name.c_str() );

        const auto print_hp = [&]( const int x, const nc_color col, const int hp ) {
            const auto bar = get_hp_bar( hp, maximal_hp, false );
            if( hp == 0 ) {
                mvwprintz( hp_window, line, x, col, "-----" );
            } else if( precise ) {
                mvwprintz( hp_window, line, x, col, "%5d", hp );
            } else {
                mvwprintz( hp_window, line, x, col, bar.first.c_str() );
            }
        };

        if( !limb_is_broken ) {
            // Drop the bar color, use the state color instead
            const nc_color color = has_any_effect ? all_state_col : c_green;
            print_hp( 15, color, current_hp );
        } else {
            // But still could be infected or bleeding
            const nc_color color = has_any_effect ? all_state_col : c_dkgray;
            print_hp( 15, color, 0 );
        }

        if( !limb_is_broken ) {
            const int new_hp = std::max( 0, std::min( maximal_hp, current_hp + bonus ) );

            if( new_hp == current_hp && !has_curable_effect ) {
                // Nothing would change
                continue;
            }

            mvwprintz( hp_window, line, 20, c_dkgray, " -> " );

            const nc_color color = has_any_effect ? new_state_col : c_green;
            print_hp( 24, color, new_hp );
        } else {
            const nc_color color = has_any_effect ? new_state_col : c_dkgray;
            mvwprintz( hp_window, line, 20, c_dkgray, " -> " );
            print_hp( 24, color, 0 );
        }
    }
    mvwprintz( hp_window, parts.size() + y_off, 1, c_ltgray, _("%d: Exit"), parts.size() + 1 );

    wrefresh(hp_window);
    char ch;
    hp_part healed_part = num_hp_parts;
    do {
        ch = getch();
        const size_t index = ch - '1';
        if( index < parts.size() && parts[index].allowed ) {
            healed_part = parts[index].hp;
            break;
        } else if( index == parts.size() || ch == KEY_ESCAPE) {
            healed_part = num_hp_parts;
            break;
        }
    } while (ch < '1' || ch > '7');
    werase(hp_window);
    wrefresh(hp_window);
    delwin(hp_window);
    refresh();

    return healed_part;
}

nc_color Character::limb_color( body_part bp, bool bleed, bool bite, bool infect ) const
{
    if( bp == num_bp ) {
        return c_ltgray;
    }

    int color_bit = 0;
    nc_color i_color = c_ltgray;
    if( bleed && has_effect( effect_bleed, bp ) ) {
        color_bit += 1;
    }
    if( bite && has_effect( effect_bite, bp ) ) {
        color_bit += 10;
    }
    if( infect && has_effect( effect_infected, bp ) ) {
        color_bit += 100;
    }
    switch( color_bit ) {
    case 1:
        i_color = c_red;
        break;
    case 10:
        i_color = c_blue;
        break;
    case 100:
        i_color = c_green;
        break;
    case 11:
        i_color = c_magenta;
        break;
    case 101:
        i_color = c_yellow;
        break;
    }

    return i_color;
}

std::string Character::get_name() const
{
    return name;
}

nc_color Character::symbol_color() const
{
    nc_color basic = basic_symbol_color();
    const auto &fields = g->m.field_at( pos() );
    bool has_fire = false;
    bool has_acid = false;
    bool has_elec = false;
    bool has_fume = false;
    for( const auto &field : fields ) {
        switch( field.first ) {
            case fd_incendiary:
            case fd_fire:
                has_fire = true;
                break;
            case fd_electricity:
                has_elec = true;
                break;
            case fd_acid:
                has_acid = true;
                break;
            case fd_relax_gas:
            case fd_fungal_haze:
            case fd_fungicidal_gas:
            case fd_toxic_gas:
            case fd_tear_gas:
            case fd_nuke_gas:
            case fd_smoke:
                has_fume = true;
                break;
            default:
                continue;
        }
    }

    // Priority: electricity, fire, acid, gases
    // Can't just return in the switch, because field order is alphabetic
    if( has_elec ) {
        return hilite( basic );
    } else if( has_fire ) {
        return red_background( basic );
    } else if( has_acid ) {
        return green_background( basic );
    } else if( has_fume ) {
        return white_background( basic );
    }

    if( in_sleep_state() ) {
        return hilite( basic );
    }

    return basic;
}

bool Character::is_dangerous_field( const field &fd ) const
{
    if( fd.fieldCount() == 0 || has_trait( debug_nodmg ) ) {
        return false;
    }

    for( auto &fld : fd ) {
        if( is_dangerous_field( fld.second ) ) {
            return true;
        }
    }

    return false;
}

bool Character::is_dangerous_field( const field_entry &entry ) const
{
    const field_id fid = entry.getFieldType();
    switch( fid ) {
        // @todo Lower density fields are less dangerous
        case fd_smoke:
        case fd_tear_gas:
        case fd_toxic_gas:
        case fd_gas_vent:
        case fd_relax_gas:
        case fd_fungal_haze:
        case fd_electricity:
        case fd_acid:
            return is_dangerous_field( fid );
        default:
            return !has_trait( debug_nodmg ) && entry.is_dangerous();
    }

    return false;
}

bool Character::is_dangerous_field( const field_id fid ) const
{
    if( has_trait( debug_nodmg ) ) {
        return false;
    }

    switch( fid ) {
        case fd_smoke:
            return get_env_resist( bp_mouth ) < 12;
        case fd_tear_gas:
        case fd_toxic_gas:
        case fd_gas_vent:
        case fd_relax_gas:
            return get_env_resist( bp_mouth ) < 15;
        case fd_fungal_haze:
            return get_env_resist( bp_mouth ) < 15 ||
                   get_env_resist( bp_eyes ) < 15 ||
                   has_trait("M_IMMUNE");
        case fd_electricity:
            return !is_elec_immune();
        case fd_acid:
            return !has_trait("ACIDPROOF") &&
                   (is_on_ground() ||
                   get_env_resist( bp_foot_l ) < 15 ||
                   get_env_resist( bp_foot_r ) < 15 ||
                   get_env_resist( bp_leg_l ) < 15 ||
                   get_env_resist( bp_leg_r ) < 15 ||
                   get_armor_type( DT_ACID, bp_foot_l ) < 5 ||
                   get_armor_type( DT_ACID, bp_foot_r ) < 5 ||
                   get_armor_type( DT_ACID, bp_leg_l ) < 5 ||
                   get_armor_type( DT_ACID, bp_leg_r ) < 5);
        default:
            return field_type_dangerous( fid );
    }

    return false;
}

int Character::throw_range( const item &it ) const
{
    if( it.is_null() ) {
        return -1;
    }

    item tmp = it;

    if( tmp.count_by_charges() && tmp.charges > 1 ) {
        tmp.charges = 1;
    }

    ///\EFFECT_STR determines maximum weight that can be thrown
    if( (tmp.weight() / 113) > int(str_cur * 15) ) {
        return 0;
    }
    // Increases as weight decreases until 150 g, then decreases again
    ///\EFFECT_STR increases throwing range, vs item weight (high or low)
    int ret = (str_cur * 8) / (tmp.weight() >= 150 ? tmp.weight() / 113 : 10 - int(tmp.weight() / 15));
    ret -= int(tmp.volume() / 4);
    static const std::vector<material_id> affected_materials = { material_id( "iron" ), material_id( "steel" ) };
    if( has_active_bionic("bio_railgun") && tmp.made_of_any( affected_materials ) ) {
        ret *= 2;
    }
    if( ret < 1 ) {
        return 1;
    }
    // Cap at double our strength + skill
    ///\EFFECT_STR caps throwing range

    ///\EFFECT_THROW caps throwing range
    if( ret > str_cur * 1.5 + get_skill_level( skill_throw ) ) {
        return str_cur * 1.5 + get_skill_level( skill_throw );
    }

    return ret;
}

bool Character::made_of( const material_id &m ) const {
    // TODO: check for mutations that change this.
    static const std::vector<material_id> fleshy = { material_id( "flesh" ), material_id( "hflesh" ) };
    return std::find( fleshy.begin(), fleshy.end(), m ) != fleshy.end();
}

bool Character::is_blind() const
{
    return ( worn_with_flag( "BLIND" ) ||
             has_effect( effect_blind ) ||
             has_active_bionic( "bio_blindfold" ) );
}

bool Character::pour_into( item &container, item &liquid )
{
    if( liquid.is_ammo() && ( container.is_tool() || container.is_gun() ) ) {
        // TODO: merge this part with game::reload
        // for filling up chainsaws, jackhammers and flamethrowers

        if( container.ammo_type() != liquid.ammo_type() ) {
            add_msg_if_player( m_info, _( "Your %1$s won't hold %2$s." ), container.tname().c_str(),
                               liquid.tname().c_str() );
            return false;
        }

        if( container.ammo_remaining() >= container.ammo_capacity() ) {
            add_msg_if_player( m_info, _( "Your %1$s can't hold any more %2$s." ), container.tname().c_str(),
                               liquid.tname().c_str() );
            return false;
        }

        if( container.ammo_remaining() && container.ammo_current() != liquid.typeId() ) {
            add_msg_if_player( m_info, _( "You can't mix loads in your %s." ), container.tname().c_str() );
            return false;
        }

        add_msg_if_player( _( "You pour %1$s into the %2$s." ), liquid.tname().c_str(),
                           container.tname().c_str() );
        auto qty = std::min( liquid.charges, container.ammo_capacity() - container.ammo_remaining() );
        liquid.charges -= qty;
        container.ammo_set( liquid.typeId(), container.ammo_remaining() + qty );
        if( liquid.charges > 0 ) {
            add_msg_if_player( _( "There's some left over!" ) );
        }

    } else {
        // Filling up normal containers
        bool allow_bucket = &container == &weapon || !has_item( container );
        std::string err;
        if( !container.fill_with( liquid, err, allow_bucket ) ) {
            add_msg_if_player( m_info, err.c_str() );
            return false;
        }

        inv.unsort();
        add_msg_if_player( _( "You pour %1$s into the %2$s." ), liquid.tname().c_str(),
                           container.tname().c_str() );
        if( liquid.charges > 0 ) {
            // TODO: maybe not show this if the source is infinite. Best would be to move it to the caller.
            add_msg_if_player( _( "There's some left over!" ) );
        }
    }

    return true;
}

bool Character::pour_into( vehicle &veh, item &liquid )
{
    const itype_id &ftype = liquid.type->id;
    const int fuel_per_charge = fuel_charges_to_amount_factor( ftype );
    const int fuel_cap = veh.fuel_capacity( ftype );
    const int fuel_amnt = veh.fuel_left( ftype );
    if( fuel_cap <= 0 ) {
        //~ %1$s - transport name, %2$s liquid fuel name
        add_msg_if_player( m_info, _( "The %1$s doesn't use %2$s." ), veh.name.c_str(), liquid.type_name().c_str() );
        return false;
    } else if( fuel_amnt >= fuel_cap ) {
        add_msg_if_player( m_info, _( "The %s is already full." ), veh.name.c_str() );
        return false;
    }
    const int charges_to_move = std::min<int>( liquid.charges, ( fuel_cap - fuel_amnt ) / fuel_per_charge );
    liquid.charges = veh.refill( ftype, charges_to_move * fuel_per_charge ) / fuel_per_charge;
    if( veh.fuel_left( ftype ) < fuel_cap ) {
        add_msg_if_player( _( "You refill the %1$s with %2$s." ), veh.name.c_str(), liquid.type_name().c_str() );
    } else {
        add_msg_if_player( _( "You refill the %1$s with %2$s to its maximum." ), veh.name.c_str(),
                 liquid.type_name().c_str() );
    }
    return true;
}
