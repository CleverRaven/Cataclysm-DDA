#include "player.h"
#include "action.h"
#include "profession.h"
#include "bionics.h"
#include "mission.h"
#include "game.h"
#include "map.h"
#include "debug.h"
#include "addiction.h"
#include "inventory.h"
#include "options.h"
#include "weather.h"
#include "item.h"
#include "material.h"
#include "translations.h"
#include "name.h"
#include "cursesdef.h"
#include "catacharset.h"
#include "get_version.h"
#include "crafting.h"
#include "monstergenerator.h"
#include "help.h" // get_hint
#include "martialarts.h"
#include "output.h"
#include "overmapbuffer.h"
#include "messages.h"
#include "sounds.h"
#include "item_action.h"
#include "mongroup.h"
#include "morale.h"
#include "input.h"
#include "veh_type.h"
#include "overmap.h"
#include "vehicle.h"
#include "trap.h"
#include "mutation.h"
#include "ui.h"
#include "trap.h"
#include "map_iterator.h"
#include "submap.h"
#include "mtype.h"
#include "weather_gen.h"
#include "cata_utility.h"

#include <map>

#ifdef TILES
#include "SDL2/SDL.h"
#endif // TILES

//Used for e^(x) functions
#include <stdio.h>
#include <math.h>

#include <ctime>
#include <algorithm>
#include <numeric>
#include <string>
#include <memory>
#include <array>
#include <bitset>
#include <sstream>
#include <stdlib.h>
#include <fstream>
#include <limits>

const mtype_id mon_dermatik_larva( "mon_dermatik_larva" );
const mtype_id mon_player_blob( "mon_player_blob" );
const mtype_id mon_shadow_snake( "mon_shadow_snake" );

const skill_id skill_dodge( "dodge" );
const skill_id skill_gun( "gun" );
const skill_id skill_swimming( "swimming" );
const skill_id skill_throw( "throw" );
const skill_id skill_unarmed( "unarmed" );

// use this instead of having to type out 26 spaces like before
static const std::string header_spaces(26, ' ');

stats player_stats;

static const itype_id OPTICAL_CLOAK_ITEM_ID( "optical_cloak" );

namespace {
    const std::string &get_morale_data( const morale_type id )
    {
        static const std::array<std::string, NUM_MORALE_TYPES> morale_data = { {
            { "This is a bug (player.cpp:moraledata)" },
            { _( "Enjoyed %i" ) },
            { _( "Enjoyed a hot meal" ) },
            { _( "Music" ) },
            { _( "Enjoyed honey" ) },
            { _( "Played Video Game" ) },
            { _( "Marloss Bliss" ) },
            { _( "Mutagenic Anticipation" ) },
            { _( "Good Feeling" ) },
            { _( "Supported" ) },
            { _( "Looked at photos" ) },

            { _( "Nicotine Craving" ) },
            { _( "Caffeine Craving" ) },
            { _( "Alcohol Craving" ) },
            { _( "Opiate Craving" ) },
            { _( "Speed Craving" ) },
            { _( "Cocaine Craving" ) },
            { _( "Crack Cocaine Craving" ) },
            { _( "Mutagen Craving" ) },
            { _( "Diazepam Craving" ) },
            { _( "Marloss Craving" ) },

            { _( "Disliked %i" ) },
            { _( "Ate Human Flesh" ) },
            { _( "Ate Meat" ) },
            { _( "Ate Vegetables" ) },
            { _( "Ate Fruit" ) },
            { _( "Lactose Intolerance" ) },
            { _( "Ate Junk Food" ) },
            { _( "Wheat Allergy" ) },
            { _( "Ate Indigestible Food" ) },
            { _( "Wet" ) },
            { _( "Dried Off" ) },
            { _( "Cold" ) },
            { _( "Hot" ) },
            { _( "Bad Feeling" ) },
            { _( "Killed Innocent" ) },
            { _( "Killed Friend" ) },
            { _( "Guilty about Killing" ) },
            { _( "Guilty about Mutilating Corpse" ) },
            { _( "Fey Mutation" ) },
            { _( "Chimerical Mutation" ) },
            { _( "Mutation" ) },

            { _( "Moodswing" ) },
            { _( "Read %i" ) },
            { _( "Got comfy" ) },

            { _( "Heard Disturbing Scream" ) },

            { _( "Masochism" ) },
            { _( "Hoarder" ) },
            { _( "Stylish" ) },
            { _( "Optimist" ) },
            { _( "Bad Tempered" ) },
            //~ You really don't like wearing the Uncomfy Gear
            { _( "Uncomfy Gear" ) },
            { _( "Found kitten <3" ) },

            { _( "Got a Haircut" ) },
            { _( "Freshly Shaven" ) },
        } };
        if( static_cast<size_t>( id ) >= morale_data.size() ) {
            debugmsg( "invalid morale type: %d", id );
            return morale_data[0];
        }
        return morale_data[id];
    }
} // namespace

std::string morale_point::name() const
{
    // Start with the morale type's description.
    std::string ret = get_morale_data( type );

    // Get the name of the referenced item (if any).
    std::string item_name = "";
    if( item_type != NULL ) {
        item_name = item_type->nname( 1 );
    }

    // Replace each instance of %i with the item's name.
    size_t it = ret.find( "%i" );
    while( it != std::string::npos ) {
        ret.replace( it, 2, item_name );
        it = ret.find( "%i" );
    }

    return ret;
}

player::player() : Character()
{
 id = -1; // -1 is invalid
 str_cur = 8;
 str_max = 8;
 dex_cur = 8;
 dex_max = 8;
 int_cur = 8;
 int_max = 8;
 per_cur = 8;
 per_max = 8;
 underwater = false;
 dodges_left = 1;
 blocks_left = 1;
 power_level = 0;
 max_power_level = 0;
 thirst = 0;
 fatigue = 0;
 stamina = get_stamina_max();
 stim = 0;
 pain = 0;
 pkill = 0;
 radiation = 0;
 tank_plut = 0;
 reactor_plut = 0;
 slow_rad = 0;
 cash = 0;
 recoil = 0;
 driving_recoil = 0;
 scent = 500;
 male = true;
 prof = profession::has_initialized() ? profession::generic() : NULL; //workaround for a potential structural limitation, see player::create

 start_location = "shelter";
 moves = 100;
 movecounter = 0;
 cached_turn = -1;
 oxygen = 0;
 next_climate_control_check=0;
 last_climate_control_ret=false;
 active_mission = nullptr;
 in_vehicle = false;
 controlling_vehicle = false;
 grab_point = {0, 0, 0};
 grab_type = OBJECT_NONE;
 move_mode = "walk";
 style_selected = matype_id( "style_none" );
 keep_hands_free = false;
 focus_pool = 100;
 last_item = itype_id("null");
 sight_max = 9999;
 last_batch = 0;
 lastconsumed = itype_id("null");
 next_expected_position = tripoint_min;

 empty_traits();

 for( auto &skill : Skill::skills ) {
     skillLevel( skill ).level( 0 );
 }

 for (int i = 0; i < num_bp; i++) {
  temp_cur[i] = BODYTEMP_NORM;
  frostbite_timer[i] = 0;
  temp_conv[i] = BODYTEMP_NORM;
  body_wetness[i] = 0;
 }
 nv_cached = false;
 pda_cached = false;
 volume = 0;

 memorial_log.clear();
 player_stats.reset();

    drench_capacity[bp_eyes] = 1;
    drench_capacity[bp_mouth] = 1;
    drench_capacity[bp_head] = 7;
    drench_capacity[bp_leg_l] = 11;
    drench_capacity[bp_leg_r] = 11;
    drench_capacity[bp_foot_l] = 3;
    drench_capacity[bp_foot_r] = 3;
    drench_capacity[bp_arm_l] = 10;
    drench_capacity[bp_arm_r] = 10;
    drench_capacity[bp_hand_l] = 3;
    drench_capacity[bp_hand_r] = 3;
    drench_capacity[bp_torso] = 40;

 recalc_sight_limits();
}

player::~player()
{
}

void player::normalize()
{
    Character::normalize();

    style_selected = matype_id( "style_none" );

    recalc_hp();

    for (int i = 0 ; i < num_bp; i++) {
        temp_conv[i] = BODYTEMP_NORM;
    }
    stamina = get_stamina_max();
}

std::string player::disp_name(bool possessive) const
{
    if (!possessive) {
        if (is_player()) {
            return _("you");
        }
        return name;
    } else {
        if (is_player()) {
            return _("your");
        }
        return string_format(_("%s's"), name.c_str());
    }
}

std::string player::skin_name() const
{
    //TODO: Return actual deflecting layer name
    return _("armor");
}

void player::reset_stats()
{
    clear_miss_reasons();

    // Trait / mutation buffs
    if (has_trait("THICK_SCALES")) {
        add_miss_reason(_("Your thick scales get in the way."), 2);
    }
    if (has_trait("CHITIN2") || has_trait("CHITIN3") || has_trait("CHITIN_FUR3")) {
        add_miss_reason(_("Your chitin gets in the way."), 1);
    }
    if (has_trait("COMPOUND_EYES") && !wearing_something_on(bp_eyes)) {
        mod_per_bonus(1);
    }
    if (has_trait("INSECT_ARMS")) {
        add_miss_reason(_("Your insect limbs get in the way."), 2);
    }
    if (has_trait("INSECT_ARMS_OK")) {
        if (!wearing_something_on(bp_torso)) {
            mod_dex_bonus(1);
        }
        else {
            mod_dex_bonus(-1);
            add_miss_reason(_("Your clothing restricts your insect arms."), 1);
        }
    }
    if (has_trait("WEBBED")) {
        add_miss_reason(_("Your webbed hands get in the way."), 1);
    }
    if (has_trait("ARACHNID_ARMS")) {
        add_miss_reason(_("Your arachnid limbs get in the way."), 4);
    }
    if( has_trait( "ARACHNID_ARMS_OK" ) ) {
        if( !wearing_something_on( bp_torso ) ) {
            mod_dex_bonus( 2 );
        } else if( !exclusive_flag_coverage( "OVERSIZE" )[bp_torso] ) {
            mod_dex_bonus( -2 );
            add_miss_reason( _( "Your clothing constricts your arachnid limbs." ), 2 );
        }
    }

    // Pain
    if (pain > pkill) {
        if (!(has_trait("CENOBITE"))) {
            mod_str_bonus(-int((pain - pkill) / 15));
            mod_dex_bonus(-int((pain - pkill) / 15));
            add_miss_reason(_("Your pain distracts you!"), int(pain - pkill) / 15);
        }
        mod_per_bonus(-int((pain - pkill) / 20));
        if (!(has_trait("INT_SLIME"))) {
            mod_int_bonus(-(1 + int((pain - pkill) / 25)));
        } else if (has_trait("INT_SLIME")) {
        // Having one's brain throughout one's body does have its downsides.
        // Be glad we don't assess permanent damage.
            mod_int_bonus(-(1 + int(pain - pkill)));
        }
    }
    // Morale
    if (abs(morale_level()) >= 100) {
        mod_str_bonus(int(morale_level() / 180));
        int dex_mod = int(morale_level() / 200);
        mod_dex_bonus(dex_mod);
        if (dex_mod < 0) {
            add_miss_reason(_("What's the point of fighting?"), -dex_mod);
        }
        mod_per_bonus(int(morale_level() / 125));
        mod_int_bonus(int(morale_level() / 100));
    }
    // Radiation
    if (radiation > 0) {
        mod_str_bonus(-int(radiation / 80));
        int dex_mod = -int(radiation / 110);
        mod_dex_bonus(dex_mod);
        if (dex_mod < 0) {
            add_miss_reason(_("Radiation weakens you."), -dex_mod);
        }
        mod_per_bonus(-int(radiation / 100));
        mod_int_bonus(-int(radiation / 120));
    }
    // Stimulants
    mod_dex_bonus(int(stim / 10));
    mod_per_bonus(int(stim /  7));
    mod_int_bonus(int(stim /  6));
    if (stim >= 30) {
        int dex_mod = -int(abs(stim - 15) / 8);
        mod_dex_bonus(dex_mod);
        add_miss_reason(_("You shake with the excess stimulation."), -dex_mod);
        mod_per_bonus(-int(abs(stim - 15) / 12));
        mod_int_bonus(-int(abs(stim - 15) / 14));
    } else if (stim <= 10) {
        add_miss_reason(_("You feel woozy."), -int(stim / 10));
    }

    // Dodge-related effects
    mod_dodge_bonus( mabuff_dodge_bonus() - ( encumb(bp_leg_l) + encumb(bp_leg_r) )/20 - (encumb(bp_torso) / 10) );
    // Whiskers don't work so well if they're covered
    if (has_trait("WHISKERS") && !wearing_something_on(bp_mouth)) {
        mod_dodge_bonus(1);
    }
    if (has_trait("WHISKERS_RAT") && !wearing_something_on(bp_mouth)) {
        mod_dodge_bonus(2);
    }
    // Spider hair is basically a full-body set of whiskers, once you get the brain for it
    if (has_trait("CHITIN_FUR3")) {
    static const std::array<body_part, 5> parts {{bp_head, bp_arm_r, bp_arm_l, bp_leg_r, bp_leg_l}};
        for( auto bp : parts ) {
            if( !wearing_something_on( bp ) ) {
                mod_dodge_bonus(+1);
            }
        }
        // Torso handled separately, bigger bonus
        if (!wearing_something_on(bp_torso)) {
            mod_dodge_bonus(4);
        }
    }

    // Hit-related effects
    mod_hit_bonus( mabuff_tohit_bonus() + weapon.type->m_to_hit );

    // Apply static martial arts buffs
    ma_static_effects();

    if( calendar::once_every(MINUTES(1)) ) {
        update_mental_focus();
    }
    pda_cached = false;

    recalc_sight_limits();
    recalc_speed_bonus();

    // Effects
    for( auto maps : effects ) {
        for( auto i : maps.second ) {
            const auto &it = i.second;
            bool reduced = resists_effect(it);
            mod_str_bonus( it.get_mod( "STR", reduced ) );
            mod_dex_bonus( it.get_mod( "DEX", reduced ) );
            mod_per_bonus( it.get_mod( "PER", reduced ) );
            mod_int_bonus( it.get_mod( "INT", reduced ) );
        }
    }

    Character::reset_stats();
}

void player::process_turn()
{
    Character::process_turn();

    // Didn't just pick something up
    last_item = itype_id("null");

    if( has_active_bionic("bio_metabolics") && power_level + 25 <= max_power_level &&
        get_hunger() < 100 && calendar::once_every(5) ) {
        mod_hunger(2);
        charge_power(25);
    }

    remove_items_with( [this]( item &itm ) {
        return itm.process_artifact( this, pos3() );
    } );

    suffer();

    // Set our scent towards the norm
    int norm_scent = 500;
    if (has_trait("WEAKSCENT")) {
        norm_scent = 300;
    }
    if (has_trait("SMELLY")) {
        norm_scent = 800;
    }
    if (has_trait("SMELLY2")) {
        norm_scent = 1200;
    }
    // Not so much that you don't have a scent
    // but that you smell like a plant, rather than
    // a human. When was the last time you saw a critter
    // attack a bluebell or an apple tree?
    if ( (has_trait("FLOWERS")) && (!(has_trait("CHLOROMORPH"))) ) {
        norm_scent -= 200;
    }
    // You *are* a plant.  Unless someone hunts triffids by scent,
    // you don't smell like prey.
    if( has_trait("CHLOROMORPH") ) {
        norm_scent = 0;
    }

    // Scent increases fast at first, and slows down as it approaches normal levels.
    // Estimate it will take about norm_scent * 2 turns to go from 0 - norm_scent / 2
    // Without smelly trait this is about 1.5 hrs. Slows down significantly after that.
    if (scent < rng(0, norm_scent))
        scent++;

    // Unusually high scent decreases steadily until it reaches normal levels.
    if (scent > norm_scent)
        scent--;

    // We can dodge again! Assuming we can actually move...
    if (moves > 0) {
        blocks_left = get_num_blocks();
        dodges_left = get_num_dodges();
    }

    // auto-learning. This is here because skill-increases happens all over the place:
    // SkillLevel::readBook (has no connection to the skill or the player),
    // player::read, player::practice, ...
    ///\EFFECT_UNARMED >1 allows spontaneous discovery of brawling martial art style
    if( get_skill_level( skill_unarmed ) >= 2 ) {
        const matype_id brawling( "style_brawling" );
        if( !has_martialart( brawling ) ) {
            add_martialart( brawling );
            add_msg_if_player( m_info, _( "You learned a new style." ) );
        }
    }
}

void player::action_taken()
{
    nv_cached = false;
    pda_cached = false;
}

void player::update_morale()
{
    // Decay existing morale entries.
    for (size_t i = 0; i < morale.size(); i++) {
        // Age the morale entry by one turn.
        morale[i].age += 1;

        // If it's past its expiration date, remove it.
        if (morale[i].age >= morale[i].duration) {
            morale.erase(morale.begin() + i);
            i--;

            // Future-proofing.
            continue;
        }

        // We don't actually store the effective strength; it gets calculated when we
        // need it.
    }

    // We reapply persistent morale effects after every decay step, to keep them fresh.
    apply_persistent_morale();
}

void player::apply_persistent_morale()
{
    // Hoarders get a morale penalty if they're not carrying a full inventory.
    if (has_trait("HOARDER"))
    {
        int pen = int((volume_capacity() - volume_carried()) / 2);
        if (pen > 70) {
            pen = 70;
        }
        if (pen <= 0) {
            pen = 0;
        }
        if (has_effect("took_xanax")) {
            pen = int(pen / 7);
        } else if (has_effect("took_prozac")) {
            pen = int(pen / 2);
        }
        if (pen > 0) {
            add_morale(MORALE_PERM_HOARDER, -pen, -pen, 5, 5, true);
        }
    }

    // The stylish get a morale bonus for each body part covered in an item
    // with the FANCY or SUPER_FANCY tag.
    if( has_trait("STYLISH") ) {
        int bonus = 0;
        std::string basic_flag = "FANCY";
        std::string bonus_flag = "SUPER_FANCY";

        std::bitset<num_bp> covered; // body parts covered
        for( auto &elem : worn ) {
            if( elem.has_flag( basic_flag ) || elem.has_flag( bonus_flag ) ) {
                covered |= elem.get_covered_body_parts();
            }
            if( elem.has_flag( bonus_flag ) ) {
              bonus+=2;
            } else if( elem.has_flag( basic_flag ) ) {
                if( ( covered & elem.get_covered_body_parts() ).none() ) {
                    bonus += 1;
                }
            }
        }
        if(covered.test(bp_torso)) {
            bonus += 6;
        }
        if(covered.test(bp_leg_l) || covered.test(bp_leg_r)) {
            bonus += 2;
        }
        if(covered.test(bp_foot_l) || covered.test(bp_foot_r)) {
            bonus += 1;
        }
        if(covered.test(bp_hand_l) || covered.test(bp_hand_r)) {
            bonus += 1;
        }
        if(covered.test(bp_head)) {
            bonus += 3;
        }
        if(covered.test(bp_eyes)) {
            bonus += 2;
        }
        if(covered.test(bp_arm_l) || covered.test(bp_arm_r)) {
            bonus += 1;
        }
        if(covered.test(bp_mouth)) {
            bonus += 2;
        }

        if(bonus > 20)
            bonus = 20;

        if(bonus) {
            add_morale(MORALE_PERM_FANCY, bonus, bonus, 5, 5, true);
        }
    }

    // Floral folks really don't like having their flowers covered.
    if( has_trait("FLOWERS") && wearing_something_on(bp_head) ) {
        add_morale(MORALE_PERM_CONSTRAINED, -10, -10, 5, 5, true);
    }

    // The same applies to rooters and their feet; however, they don't take
    // too many problems from no-footgear.
    double shoe_factor = footwear_factor();
    if( (has_trait("ROOTS") || has_trait("ROOTS2") || has_trait("ROOTS3") ) &&
        shoe_factor ) {
        add_morale(MORALE_PERM_CONSTRAINED, -10 * shoe_factor, -10 * shoe_factor, 5, 5, true);
    }

    // Masochists get a morale bonus from pain.
    if (has_trait("MASOCHIST") || has_trait("MASOCHIST_MED") ||  has_trait("CENOBITE")) {
        int bonus = pain / 2.5;
        // Advanced masochists really get a morale bonus from pain.
        // (It's not capped.)
        if (has_trait("MASOCHIST") && (bonus > 25)) {
            bonus = 25;
        }
        if (has_effect("took_prozac")) {
            bonus = int(bonus / 3);
        }
        if (bonus != 0) {
            add_morale(MORALE_PERM_MASOCHIST, bonus, bonus, 5, 5, true);
        }
    }

    // Optimist gives a base +4 to morale.
    // The +25% boost from optimist also applies here, for a net of +5.
    if (has_trait("OPTIMISTIC")) {
        add_morale(MORALE_PERM_OPTIMIST, 4, 4, 5, 5, true);
    }

    // And Bad Temper works just the same way.  But in reverse.  ):
    if (has_trait("BADTEMPER")) {
        add_morale(MORALE_PERM_BADTEMPER, -4, -4, 5, 5, true);
    }
}

void player::update_mental_focus()
{
    int focus_gain_rate = calc_focus_equilibrium() - focus_pool;

    // handle negative gain rates in a symmetric manner
    int base_change = 1;
    if (focus_gain_rate < 0)
    {
        base_change = -1;
        focus_gain_rate = -focus_gain_rate;
    }

    // for every 100 points, we have a flat gain of 1 focus.
    // for every n points left over, we have an n% chance of 1 focus
    int gain = focus_gain_rate / 100;
    if (rng(1, 100) <= (focus_gain_rate % 100))
    {
        gain++;
    }

    focus_pool += (gain * base_change);

    // Fatigue should at least prevent high focus
    // This caps focus gain at 60(arbitrary value) if you're Dead Tired
    if (fatigue >= DEAD_TIRED && focus_pool > 60) {
        focus_pool = 60;
    }

    // Moved from calc_focus_equilibrium, because it is now const
    if( activity.type == ACT_READ ) {
        const item &book = i_at(activity.position);
        if( !book.is_book() ) {
            activity.type = ACT_NULL;
        }
    }
}

// written mostly by FunnyMan3595 in Github issue #613 (DarklingWolf's repo),
// with some small edits/corrections by Soron
int player::calc_focus_equilibrium() const
{
    // Factor in pain, since it's harder to rest your mind while your body hurts.
    int eff_morale = morale_level() - pain;
    // Cenobites don't mind, though
    if (has_trait("CENOBITE")) {
        eff_morale = eff_morale + pain;
    }
    int focus_gain_rate = 100;

    if (activity.type == ACT_READ) {
        const item &book = i_at(activity.position);
        if( book.is_book() ) {
            auto &bt = *book.type->book;
            // apply a penalty when we're actually learning something
            const auto &skill_level = get_skill_level( bt.skill );
            if( skill_level.can_train() && skill_level < bt.level ) {
                focus_gain_rate -= 50;
            }
        }
    }

    if (eff_morale < -99) {
        // At very low morale, focus goes up at 1% of the normal rate.
        focus_gain_rate = 1;
    } else if (eff_morale <= 50) {
        // At -99 to +50 morale, each point of morale gives 1% of the normal rate.
        focus_gain_rate += eff_morale;
    } else {
        /* Above 50 morale, we apply strong diminishing returns.
         * Each block of 50% takes twice as many morale points as the previous one:
         * 150% focus gain at 50 morale (as before)
         * 200% focus gain at 150 morale (100 more morale)
         * 250% focus gain at 350 morale (200 more morale)
         * ...
         * Cap out at 400% focus gain with 3,150+ morale, mostly as a sanity check.
         */

        int block_multiplier = 1;
        int morale_left = eff_morale;
        while (focus_gain_rate < 400) {
            if (morale_left > 50 * block_multiplier) {
                // We can afford the entire block.  Get it and continue.
                morale_left -= 50 * block_multiplier;
                focus_gain_rate += 50;
                block_multiplier *= 2;
            } else {
                // We can't afford the entire block.  Each block_multiplier morale
                // points give 1% focus gain, and then we're done.
                focus_gain_rate += morale_left / block_multiplier;
                break;
            }
        }
    }

    // This should be redundant, but just in case...
    if (focus_gain_rate < 1) {
        focus_gain_rate = 1;
    } else if (focus_gain_rate > 400) {
        focus_gain_rate = 400;
    }

    return focus_gain_rate;
}

/* Here lies the intended effects of body temperature

Assumption 1 : a naked person is comfortable at 19C/66.2F (31C/87.8F at rest).
Assumption 2 : a "lightly clothed" person is comfortable at 13C/55.4F (25C/77F at rest).
Assumption 3 : the player is always running, thus generating more heat.
Assumption 4 : frostbite cannot happen above 0C temperature.*
* In the current model, a naked person can get frostbite at 1C. This isn't true, but it's a compromise with using nice whole numbers.

Here is a list of warmth values and the corresponding temperatures in which the player is comfortable, and in which the player is very cold.

Warmth  Temperature (Comfortable)    Temperature (Very cold)    Notes
  0       19C /  66.2F               -11C /  12.2F               * Naked
 10       13C /  55.4F               -17C /   1.4F               * Lightly clothed
 20        7C /  44.6F               -23C /  -9.4F
 30        1C /  33.8F               -29C / -20.2F
 40       -5C /  23.0F               -35C / -31.0F
 50      -11C /  12.2F               -41C / -41.8F
 60      -17C /   1.4F               -47C / -52.6F
 70      -23C /  -9.4F               -53C / -63.4F
 80      -29C / -20.2F               -59C / -74.2F
 90      -35C / -31.0F               -65C / -85.0F
100      -41C / -41.8F               -71C / -95.8F

WIND POWER
Except for the last entry, pressures are sort of made up...

Breeze : 5mph (1015 hPa)
Strong Breeze : 20 mph (1000 hPa)
Moderate Gale : 30 mph (990 hPa)
Storm : 50 mph (970 hPa)
Hurricane : 100 mph (920 hPa)
HURRICANE : 185 mph (880 hPa) [Ref: Hurricane Wilma]
*/

void player::update_bodytemp()
{
    if( has_trait("DEBUG_NOTEMP") ) {
        for( int i = 0 ; i < num_bp ; i++ ) {
            temp_cur[i] = BODYTEMP_NORM;
        }
        return;
    }
    // NOTE : visit weather.h for some details on the numbers used
    // Converts temperature to Celsius/10(Wito plans on using degrees Kelvin later)
    int Ctemperature = 100 * (g->get_temperature() - 32) * 5 / 9;
    w_point const weather = g->weather_gen->get_weather( global_square_location(), calendar::turn );
    int vpart = -1;
    vehicle *veh = g->m.veh_at( pos(), vpart );
    int vehwindspeed = 0;
    if( veh ) {
        vehwindspeed = abs(veh->velocity / 100); // vehicle velocity in mph
    }
    const oter_id &cur_om_ter = overmap_buffer.ter( global_omt_location() );
    std::string omtername = otermap[cur_om_ter].name;
    bool sheltered = g->is_sheltered(pos());
    int total_windpower = get_local_windpower(weather.windpower + vehwindspeed, omtername, sheltered);
    // Temperature norms
    // Ambient normal temperature is lower while asleep
    int ambient_norm = (has_effect("sleep") ? 3100 : 1900);
    // This gets incremented in the for loop and used in the morale calculation
    int morale_pen = 0;

    // Current temperature and converging temperature calculations
    for( int i = 0 ; i < num_bp; i++ ) {
        // This adjusts the temperature scale to match the bodytemp scale,
        // it needs to be reset every iteration
        int adjusted_temp = (Ctemperature - ambient_norm);
        int bp_windpower = total_windpower;
        // Skip eyes
        if (i == bp_eyes) {
            continue;
        }
        // Represents the fact that the body generates heat when it is cold.
        // TODO : should this increase hunger?
        double scaled_temperature = logarithmic_range( BODYTEMP_VERY_COLD, BODYTEMP_VERY_HOT,
                                                    temp_cur[i] );
        // Produces a smooth curve between 30.0 and 60.0.
        float homeostasis_adjustement = 30.0 * (1.0 + scaled_temperature);
        int clothing_warmth_adjustement = homeostasis_adjustement * warmth(body_part(i));
        int clothing_warmth_adjusted_bonus = homeostasis_adjustement * bonus_warmth(body_part(i));
        // WINDCHILL

        bp_windpower = (float)bp_windpower * (1 - get_wind_resistance(body_part(i)) / 100.0);
        // Calculate windchill
        int windchill = get_local_windchill( g->get_temperature(),
                                             get_local_humidity(weather.humidity, g->weather,
                                                     sheltered),
                                             bp_windpower );
        // If you're standing in water, air temperature is replaced by water temperature. No wind.
        const ter_id ter_at_pos = g->m.ter( pos() );
        // Convert to C.
        int water_temperature = 100 * (g->weather_gen->get_water_temperature() - 32) * 5 / 9;
        if ( (ter_at_pos == t_water_dp || ter_at_pos == t_water_pool || ter_at_pos == t_swater_dp) ||
             ((ter_at_pos == t_water_sh || ter_at_pos == t_swater_sh || ter_at_pos == t_sewage) &&
              (i == bp_foot_l || i == bp_foot_r || i == bp_leg_l || i == bp_leg_r)) ) {
            adjusted_temp += water_temperature - Ctemperature; // Swap out air temp for water temp.
            windchill = 0;
        }
        // Warn the player that wind is going to be a problem.
        if (windchill < -10 && one_in(200)) {
            add_msg(m_bad, _("The wind is making your %s feel quite cold."), body_part_name(body_part(i)).c_str());
        } else if (windchill < -20 && one_in(100)) {
            add_msg(m_bad, _("The wind is very strong, you should find some more wind-resistant clothing for your %s."), body_part_name(body_part(i)).c_str());
        } else if (windchill < -30 && one_in(50)) {
            add_msg(m_bad, _("Your clothing is not providing enough protection from the wind for your %s!"), body_part_name(body_part(i)).c_str());
        }

        // Convergent temperature is affected by ambient temperature,
        // clothing warmth, and body wetness.
        temp_conv[i] = BODYTEMP_NORM + adjusted_temp + windchill * 100 + clothing_warmth_adjustement;
        // HUNGER
        temp_conv[i] -= get_hunger() / 6 + 100;
        // FATIGUE
        if( !has_effect("sleep") ) {
            temp_conv[i] -= std::max(0.0, 1.5 * fatigue);
        }
        // CONVECTION HEAT SOURCES (generates body heat, helps fight frostbite)
        // Bark : lowers blister count to -100; harder to get blisters
        int blister_count = (has_trait("BARK") ? -100 : 0); // If the counter is high, your skin starts to burn
        int best_fire = 0;
        for (int j = -6 ; j <= 6 ; j++) {
            for (int k = -6 ; k <= 6 ; k++) {
                tripoint dest( posx() + j, posy() + k, posz() );
                int heat_intensity = 0;

                int ffire = g->m.get_field_strength( dest, fd_fire );
                if(ffire > 0) {
                    heat_intensity = ffire;
                } else if (g->m.tr_at( dest ).loadid == tr_lava ) {
                    heat_intensity = 3;
                }
                if( heat_intensity > 0 &&
                    g->m.sees( pos(), dest, -1 ) ) {
                    // Ensure fire_dist >= 1 to avoid divide-by-zero errors.
                    int fire_dist = std::max(1, std::max( std::abs( j ), std::abs( k ) ) );
                    if (frostbite_timer[i] > 0) {
                        frostbite_timer[i] -= heat_intensity - fire_dist / 2;
                    }
                    temp_conv[i] +=  300 * heat_intensity * heat_intensity / (fire_dist);
                    blister_count += heat_intensity / (fire_dist * fire_dist);
                    if( fire_dist <= 1 ) {
                        // Extend limbs/lean over a single adjacent fire to warm up
                        best_fire = std::max( best_fire, heat_intensity );
                    }
                }
            }
        }
        // Bionic "Thermal Dissipation" says it prevents fire damage up to 2000F.
        // 500 is picked at random...
        if( has_bionic( "bio_heatsink" ) || is_wearing( "rm13_armor_on" ) ) {
            blister_count -= 500;
        }
        // BLISTERS : Skin gets blisters from intense heat exposure.
        if( blister_count - 10 * get_env_resist( body_part( i ) ) > 20 ) {
            add_effect( "blisters", 1, ( body_part )i );
        }

        temp_conv[i] += bodytemp_modifier_fire();
        // WEATHER
        if( g->weather == WEATHER_SUNNY && g->is_in_sunlight(pos()) ) {
            temp_conv[i] += 1000;
        }
        if( g->weather == WEATHER_CLEAR && g->is_in_sunlight(pos()) ) {
            temp_conv[i] += 500;
        }
        // DISEASES
        if( has_effect("flu") && i == bp_head ) {
            temp_conv[i] += 1500;
        }
        if( has_effect("common_cold") ) {
            temp_conv[i] -= 750;
        }
        // Loss of blood results in loss of body heat, 1% bodyheat lost per 2% hp lost
        temp_conv[i] -= blood_loss(body_part(i)) * temp_conv[i] / 200;

        // EQUALIZATION
        switch (i) {
        case bp_torso:
            temp_equalizer(bp_torso, bp_arm_l);
            temp_equalizer(bp_torso, bp_arm_r);
            temp_equalizer(bp_torso, bp_leg_l);
            temp_equalizer(bp_torso, bp_leg_r);
            temp_equalizer(bp_torso, bp_head);
            break;
        case bp_head:
            temp_equalizer(bp_head, bp_torso);
            temp_equalizer(bp_head, bp_mouth);
            break;
        case bp_arm_l:
            temp_equalizer(bp_arm_l, bp_torso);
            temp_equalizer(bp_arm_l, bp_hand_l);
            break;
        case bp_arm_r:
            temp_equalizer(bp_arm_r, bp_torso);
            temp_equalizer(bp_arm_r, bp_hand_r);
            break;
        case bp_leg_l:
            temp_equalizer(bp_leg_l, bp_torso);
            temp_equalizer(bp_leg_l, bp_foot_l);
            break;
        case bp_leg_r:
            temp_equalizer(bp_leg_r, bp_torso);
            temp_equalizer(bp_leg_r, bp_foot_r);
            break;
        case bp_mouth:
            temp_equalizer(bp_mouth, bp_head);
            break;
        case bp_hand_l:
            temp_equalizer(bp_hand_l, bp_arm_l);
            break;
        case bp_hand_r:
            temp_equalizer(bp_hand_r, bp_arm_r);
            break;
        case bp_foot_l:
            temp_equalizer(bp_foot_l, bp_leg_l);
            break;
        case bp_foot_r:
            temp_equalizer(bp_foot_r, bp_leg_r);
            break;
        }

        // Correction of body temperature due to traits and mutations
        temp_conv[i] += bodytemp_modifier_traits( temp_cur[i] > BODYTEMP_NORM );

        // Climate Control eases the effects of high and low ambient temps
        temp_conv[i] = temp_corrected_by_climate_control( temp_conv[i] );

        // FINAL CALCULATION : Increments current body temperature towards convergent.
        int bonus_warmth = 0;
        const furn_id furn_at_pos = g->m.furn( pos() );
        if( in_sleep_state() ) {
            bonus_warmth = warmth_in_sleep();
        } else if( best_fire > 0 ) {
            // Warming up over a fire
            // Extremities are easier to extend over a fire
            switch (i) {
            case bp_head:
            case bp_torso:
            case bp_mouth:
            case bp_leg_l:
            case bp_leg_r:
                bonus_warmth = best_fire * best_fire * 150; // Not much
                break;
            case bp_arm_l:
            case bp_arm_r:
                bonus_warmth = best_fire * 600; // A fair bit
                break;
            case bp_foot_l:
            case bp_foot_r:
                if( furn_at_pos == f_armchair || furn_at_pos == f_chair || furn_at_pos == f_bench ) {
                    // Can sit on something to lift feet up to the fire
                    bonus_warmth = best_fire * 1000;
                } else {
                    // Has to stand
                    bonus_warmth = best_fire * 300;
                }
                break;
            case bp_hand_l:
            case bp_hand_r:
                bonus_warmth = best_fire * 1500; // A lot
            }
        }
        if( bonus_warmth > 0 ) {
            // Approximate temp_conv needed to reach comfortable temperature in this very turn
            // Basically inverted formula for temp_cur below
            int desired = 501 * BODYTEMP_NORM - 499 * temp_cur[i];
            if( std::abs( BODYTEMP_NORM - desired ) < 1000 ) {
                desired = BODYTEMP_NORM; // Ensure that it converges
            } else if( desired > BODYTEMP_HOT ) {
                desired = BODYTEMP_HOT; // Cap excess at sane temperature
            }

            if( desired < temp_conv[i] ) {
                // Too hot, can't help here
            } else if( desired < temp_conv[i] + bonus_warmth ) {
                // Use some heat, but not all of it
                temp_conv[i] = desired;
            } else {
                // Use all the heat
                temp_conv[i] += bonus_warmth;
            }

            // Morale bonus for comfiness - only if actually comfy (not too warm/cold)
            // Spread the morale bonus in time.
            int mytime = MINUTES( i ) / MINUTES( num_bp );
            if( calendar::turn % MINUTES( 1 ) == mytime &&
                get_effect_int( "cold", ( body_part )num_bp ) == 0 &&
                get_effect_int( "hot", ( body_part )num_bp ) == 0 &&
                temp_cur[i] > BODYTEMP_COLD && temp_cur[i] <= BODYTEMP_NORM ) {
                add_morale( MORALE_COMFY, 1, 5, 20, 10, true );
            }
        }

        int temp_before = temp_cur[i];
        int temp_difference = temp_before - temp_conv[i]; // Negative if the player is warming up.
        // exp(-0.001) : half life of 60 minutes, exp(-0.002) : half life of 30 minutes,
        // exp(-0.003) : half life of 20 minutes, exp(-0.004) : half life of 15 minutes
        int rounding_error = 0;
        // If temp_diff is small, the player cannot warm up due to rounding errors. This fixes that.
        if( temp_difference < 0 && temp_difference > -600 ) {
            rounding_error = 1;
        }
        if( temp_cur[i] != temp_conv[i] ) {
            temp_cur[i] = temp_difference * exp(-0.002) + temp_conv[i] + rounding_error;
        }
        // This statement checks if we should be wearing our bonus warmth.
        // If, after all the warmth calculations, we should be, then we have to recalculate the temperature.
        if (clothing_warmth_adjusted_bonus != 0 &&
            ((temp_conv[i] + clothing_warmth_adjusted_bonus) < BODYTEMP_HOT || temp_cur[i] < BODYTEMP_COLD)) {
            temp_conv[i] += clothing_warmth_adjusted_bonus;
            rounding_error = 0;
            if( temp_difference < 0 && temp_difference > -600 ) {
                rounding_error = 1;
            }
            if( temp_before != temp_conv[i] ) {
                temp_difference = temp_before - temp_conv[i];
                temp_cur[i] = temp_difference * exp(-0.002) + temp_conv[i] + rounding_error;
            }
        }
        int temp_after = temp_cur[i];
        // PENALTIES
        if (temp_cur[i] < BODYTEMP_FREEZING) {
            add_effect("cold", 1, (body_part)i, true, 3);
        } else if( temp_cur[i] < BODYTEMP_VERY_COLD ) {
            add_effect("cold", 1, (body_part)i, true, 2);
        } else if( temp_cur[i] < BODYTEMP_COLD ) {
            add_effect("cold", 1, (body_part)i, true, 1);
        } else if( temp_cur[i] > BODYTEMP_SCORCHING ) {
            add_effect("hot", 1, (body_part)i, true, 3);
        } else if( temp_cur[i] > BODYTEMP_VERY_HOT ) {
            add_effect("hot", 1, (body_part)i, true, 2);
        } else if( temp_cur[i] > BODYTEMP_HOT ) {
            add_effect("hot", 1, (body_part)i, true, 1);
        } else {
            if (temp_cur[i] >= BODYTEMP_COLD) {
                remove_effect("cold", (body_part)i);
            }
            if (temp_cur[i] <= BODYTEMP_HOT) {
                remove_effect("hot", (body_part)i);
            }
        }
        // MORALE : a negative morale_pen means the player is cold
        // Intensity multiplier is negative for cold, positive for hot
        if( has_effect("cold", (body_part)i) || has_effect("hot", (body_part)i) ) {
            int cold_int = get_effect_int("cold", (body_part)i);
            int hot_int = get_effect_int("hot", (body_part)i);
            int intensity_mult = hot_int - cold_int;

            switch (i) {
            case bp_head:
            case bp_torso:
            case bp_mouth:
                morale_pen += 2 * intensity_mult;
                break;
            case bp_arm_l:
            case bp_arm_r:
            case bp_leg_l:
            case bp_leg_r:
                morale_pen += .5 * intensity_mult;
                break;
            case bp_hand_l:
            case bp_hand_r:
            case bp_foot_l:
            case bp_foot_r:
                morale_pen += .5 * intensity_mult;
                break;
            }
        }
        // FROSTBITE - only occurs to hands, feet, face
        /**

        Source : http://www.atc.army.mil/weather/windchill.pdf

        Temperature and wind chill are main factors, mitigated by clothing warmth. Each 10 warmth protects against 2C of cold.

        1200 turns in low risk, + 3 tics
        450 turns in moderate risk, + 8 tics
        50 turns in high risk, +72 tics

        Let's say frostnip @ 1800 tics, frostbite @ 3600 tics

        >> Chunked into 8 parts (http://imgur.com/xlTPmJF)
        -- 2 hour risk --
        Between 30F and 10F
        Between 10F and -5F, less than 20mph, -4x + 3y - 20 > 0, x : F, y : mph
        -- 45 minute risk --
        Between 10F and -5F, less than 20mph, -4x + 3y - 20 < 0, x : F, y : mph
        Between 10F and -5F, greater than 20mph
        Less than -5F, less than 10 mph
        Less than -5F, more than 10 mph, -4x + 3y - 170 > 0, x : F, y : mph
        -- 5 minute risk --
        Less than -5F, more than 10 mph, -4x + 3y - 170 < 0, x : F, y : mph
        Less than -35F, more than 10 mp
        **/

        if( i == bp_mouth || i == bp_foot_r || i == bp_foot_l || i == bp_hand_r || i == bp_hand_l ) {
            // Handle the frostbite timer
            // Need temps in F, windPower already in mph
            int wetness_percentage = 100 * body_wetness[i] / drench_capacity[i]; // 0 - 100
            // Warmth gives a slight buff to temperature resistance
            // Wetness gives a heavy nerf to temperature resistance
            int Ftemperature = g->get_temperature() +
                               warmth((body_part)i) * 0.2 - 20 * wetness_percentage / 100;
            // Windchill reduced by your armor
            int FBwindPower = total_windpower * (1 - get_wind_resistance(body_part(i)) / 100.0);

            int intense = get_effect_int("frostbite", (body_part)i);

            // This has been broken down into 8 zones
            // Low risk zones (stops at frostnip)
            if( temp_cur[i] < BODYTEMP_COLD &&
                ((Ftemperature < 30 && Ftemperature >= 10) ||
                 (Ftemperature < 10 && Ftemperature >= -5 &&
                  FBwindPower < 20 && -4 * Ftemperature + 3 * FBwindPower - 20 >= 0)) ) {
                if( frostbite_timer[i] < 2000 ) {
                    frostbite_timer[i] += 3;
                }
                if( one_in(100) && !has_effect("frostbite", (body_part)i)) {
                    add_msg(m_warning, _("Your %s will be frostnipped in the next few hours."),
                            body_part_name(body_part(i)).c_str());
                }
                // Medium risk zones
            } else if( temp_cur[i] < BODYTEMP_COLD &&
                       ((Ftemperature < 10 && Ftemperature >= -5 && FBwindPower < 20 &&
                         -4 * Ftemperature + 3 * FBwindPower - 20 < 0) ||
                        (Ftemperature < 10 && Ftemperature >= -5 && FBwindPower >= 20) ||
                        (Ftemperature < -5 && FBwindPower < 10) ||
                        (Ftemperature < -5 && FBwindPower >= 10 &&
                         -4 * Ftemperature + 3 * FBwindPower - 170 >= 0)) ) {
                frostbite_timer[i] += 8;
                if (one_in(100) && intense < 2) {
                    add_msg(m_warning, _("Your %s will be frostbitten within the hour!"),
                            body_part_name(body_part(i)).c_str());
                }
                // High risk zones
            } else if (temp_cur[i] < BODYTEMP_COLD &&
                       ((Ftemperature < -5 && FBwindPower >= 10 &&
                         -4 * Ftemperature + 3 * FBwindPower - 170 < 0) ||
                        (Ftemperature < -35 && FBwindPower >= 10)) ) {
                frostbite_timer[i] += 72;
                if (one_in(100) && intense < 2) {
                    add_msg(m_warning, _("Your %s will be frostbitten any minute now!!"),
                            body_part_name(body_part(i)).c_str());
                }
                // Risk free, so reduce frostbite timer
            } else {
                frostbite_timer[i] -= 3;
            }

            // Handle the bestowing of frostbite
            if( frostbite_timer[i] < 0 ) {
                frostbite_timer[i] = 0;
            } else if (frostbite_timer[i] > 4200) {
                // This ensures that the player will recover in at most 3 hours.
                frostbite_timer[i] = 4200;
            }
            // Frostbite, no recovery possible
            if (frostbite_timer[i] >= 3600) {
                add_effect("frostbite", 1, (body_part)i, true, 2);
                remove_effect("frostbite_recovery", (body_part)i);
            // Else frostnip, add recovery if we were frostbitten
            } else if (frostbite_timer[i] >= 1800) {
                if (intense == 2) {
                    add_effect("frostbite_recovery", 1, (body_part)i, true);
                }
                add_effect("frostbite", 1, (body_part)i, true, 1);
            // Else fully recovered
            } else if (frostbite_timer[i] == 0) {
                remove_effect("frostbite", (body_part)i);
                remove_effect("frostbite_recovery", (body_part)i);
            }
        }
        // Warn the player if condition worsens
        if( temp_before > BODYTEMP_FREEZING && temp_after < BODYTEMP_FREEZING ) {
            //~ %s is bodypart
            add_msg(m_warning, _("You feel your %s beginning to go numb from the cold!"),
                    body_part_name(body_part(i)).c_str());
        } else if( temp_before > BODYTEMP_VERY_COLD && temp_after < BODYTEMP_VERY_COLD ) {
            //~ %s is bodypart
            add_msg(m_warning, _("You feel your %s getting very cold."),
                    body_part_name(body_part(i)).c_str());
        } else if( temp_before > BODYTEMP_COLD && temp_after < BODYTEMP_COLD ) {
            //~ %s is bodypart
            add_msg(m_warning, _("You feel your %s getting chilly."),
                    body_part_name(body_part(i)).c_str());
        } else if( temp_before < BODYTEMP_SCORCHING && temp_after > BODYTEMP_SCORCHING ) {
            //~ %s is bodypart
            add_msg(m_bad, _("You feel your %s getting red hot from the heat!"),
                    body_part_name(body_part(i)).c_str());
        } else if( temp_before < BODYTEMP_VERY_HOT && temp_after > BODYTEMP_VERY_HOT ) {
            //~ %s is bodypart
            add_msg(m_warning, _("You feel your %s getting very hot."),
                    body_part_name(body_part(i)).c_str());
        } else if( temp_before < BODYTEMP_HOT && temp_after > BODYTEMP_HOT ) {
            //~ %s is bodypart
            add_msg(m_warning, _("You feel your %s getting warm."),
                    body_part_name(body_part(i)).c_str());
        }
    }
    // Morale penalties, updated at the same rate morale is
    if( morale_pen < 0 && calendar::once_every(MINUTES(1)) ) {
        add_morale(MORALE_COLD, -2, -abs(morale_pen), 10, 5, true);
    }
    if( morale_pen > 0 && calendar::once_every(MINUTES(1)) ) {
        add_morale(MORALE_HOT,  -2, -abs(morale_pen), 10, 5, true);
    }
}

int player::warmth_in_sleep()
{
    const trap &trap_at_pos = g->m.tr_at( pos() );
    const ter_id ter_at_pos = g->m.ter( pos() );
    const furn_id furn_at_pos = g->m.furn( pos() );
    // When the player is sleeping, he will use floor items for warmth
    int floor_item_warmth = 0;
    // When the player is sleeping, he will use floor bedding for warmth
    int floor_bedding_warmth = 0;

    // Search the floor for items
    auto floor_item = g->m.i_at( pos() );
    for( auto &elem : floor_item ) {
        if( !elem.is_armor() ) {
            continue;
        }
        // Items that are big enough and covers the torso are used to keep warm.
        // Smaller items don't do as good a job
        if( elem.volume() > 1 &&
            ( elem.covers( bp_torso ) || elem.covers( bp_leg_l ) ||
              elem.covers( bp_leg_r ) ) ) {
            floor_item_warmth += 60 * elem.get_warmth() * elem.volume() / 10;
        }
    }

    int vpart = -1;
    vehicle *veh = g->m.veh_at( pos(), vpart );
    bool veh_bed = ( veh && veh->part_with_feature( vpart, "BED" ) >= 0 );
    bool veh_seat = ( veh && veh->part_with_feature( vpart, "SEAT" ) >= 0 );

    // Search the floor for bedding
    if( furn_at_pos == f_bed ) {
        floor_bedding_warmth += 1000;
    } else if( furn_at_pos == f_makeshift_bed || furn_at_pos == f_armchair ||
               furn_at_pos == f_sofa ) {
        floor_bedding_warmth += 500;
    } else if( veh_bed && veh_seat ) {
        // BED+SEAT is intentionally worse than just BED
        floor_bedding_warmth += 250;
    } else if( veh_bed ) {
        floor_bedding_warmth += 300;
    } else if( veh_seat ) {
        floor_bedding_warmth += 200;
    } else if( furn_at_pos == f_straw_bed ) {
        floor_bedding_warmth += 200;
    } else if( trap_at_pos.loadid == tr_fur_rollmat || furn_at_pos == f_hay ) {
        floor_bedding_warmth += 0;
    } else if( trap_at_pos.loadid == tr_cot || ter_at_pos == t_improvised_shelter ||
               furn_at_pos == f_tatami ) {
        floor_bedding_warmth -= 500;
    } else if( trap_at_pos.loadid == tr_rollmat ) {
        floor_bedding_warmth -= 1000;
    } else {
        floor_bedding_warmth -= 2000;
    }

    // If the PC has fur, etc, that'll apply too
    int floor_mut_warmth = bodytemp_modifier_traits_sleep();
    // DOWN doesn't provide floor insulation, though.
    // Better-than-light fur or being in one's shell does.
    if( ( !( has_trait( "DOWN" ) ) ) && ( floor_mut_warmth >= 200 ) ) {
        floor_bedding_warmth = std::max( 0, floor_bedding_warmth );
    }
    return ( floor_item_warmth + floor_bedding_warmth + floor_mut_warmth );
}

int player::bodytemp_modifier_fire()
{
    int temp_conv = 0;
    // Being on fire increases very intensely the convergent temperature.
    if( has_effect( "onfire" ) ) {
        temp_conv += 15000;
    }

    const trap &trap_at_pos = g->m.tr_at( pos() );
    // Same with standing on fire.
    int tile_strength = g->m.get_field_strength( pos(), fd_fire );
    if( tile_strength > 2 || trap_at_pos.loadid == tr_lava ) {
        temp_conv += 15000;
    }
    // Standing in the hot air of a fire is nice.
    tile_strength = g->m.get_field_strength( pos(), fd_hot_air1 );
    switch( tile_strength ) {
        case 3:
            temp_conv += 500;
            break;
        case 2:
            temp_conv += 300;
            break;
        case 1:
            temp_conv += 100;
            break;
        default:
            break;
    }
    tile_strength = g->m.get_field_strength( pos(), fd_hot_air2 );
    switch( tile_strength ) {
        case 3:
            temp_conv += 1000;
            break;
        case 2:
            temp_conv += 800;
            break;
        case 1:
            temp_conv += 300;
            break;
        default:
            break;
    }
    tile_strength = g->m.get_field_strength( pos(), fd_hot_air3 );
    switch( tile_strength ) {
        case 3:
            temp_conv += 3500;
            break;
        case 2:
            temp_conv += 2000;
            break;
        case 1:
            temp_conv += 800;
            break;
        default:
            break;
    }
    tile_strength = g->m.get_field_strength( pos(), fd_hot_air4 );
    switch( tile_strength ) {
        case 3:
            temp_conv += 8000;
            break;
        case 2:
            temp_conv += 5000;
            break;
        case 1:
            temp_conv += 3500;
            break;
        default:
            break;
    }
    return temp_conv;
}

int player::bodytemp_modifier_traits( bool overheated )
{
    int mod = 0;

    // Lightly furred
    if( has_trait( "LIGHTFUR" ) ) {
        mod += ( overheated ? 250 : 500 );
    }
    // Furry or Lupine/Ursine Fur
    if( has_trait( "FUR" ) || has_trait( "LUPINE_FUR" ) || has_trait( "URSINE_FUR" ) ) {
        mod += ( overheated ? 750 : 1500 );
    }
    // Feline fur
    if( has_trait( "FELINE_FUR" ) ) {
        mod += ( overheated ? 500 : 1000 );
    }
    // Feathers: minor means minor.
    if( has_trait( "FEATHERS" ) ) {
        mod += ( overheated ? 50 : 100 );
    }
    if( has_trait( "CHITIN_FUR" ) ) {
        mod += ( overheated ? 100 : 150 );
    }
    if( has_trait( "CHITIN_FUR2" ) || has_trait( "CHITIN_FUR3" ) ) {
        mod += ( overheated ? 150 : 250 );
    }
    // Down; lets heat out more easily if needed but not as Warm
    // as full-blown fur.  So less miserable in Summer.
    if( has_trait( "DOWN" ) ) {
        mod += ( overheated ? 300 : 800 );
    }
    // Fat deposits don't hold in much heat, but don't shift for temp
    if( has_trait( "FAT" ) ) {
        mod += 200;
    }
    // Being in the shell holds in heat, but lets out less in summer :-/
    if( has_active_mutation( "SHELL2" ) ) {
        mod += ( overheated ? 500 : 750 );
    }
    // Disintegration
    if( has_trait( "ROT1" ) ) {
        mod -= 250;
    } else if( has_trait( "ROT2" ) ) {
        mod -= 750;
    } else if( has_trait( "ROT3" ) ) {
        mod -= 1500;
    }
    // Radioactive
    if( has_trait( "RADIOACTIVE1" ) ) {
        mod += 250;
    } else if( has_trait( "RADIOACTIVE2" ) ) {
        mod += 750;
    } else if( has_trait( "RADIOACTIVE3" ) ) {
        mod += 1500;
    }
    return mod;
}

int player::bodytemp_modifier_traits_sleep()
{
    int floor_mut_warmth = 0;
    // Fur, etc effects for sleeping  here.
    // Full-power fur is about as effective as a makeshift bed
    if( has_trait( "FUR" ) || has_trait( "LUPINE_FUR" ) || has_trait( "URSINE_FUR" ) ) {
        floor_mut_warmth += 500;
    }
    // Feline fur, not quite as warm.  Cats do better in warmer spots.
    if( has_trait( "FELINE_FUR" ) ) {
        floor_mut_warmth += 300;
    }
    // Light fur's better than nothing!
    if( has_trait( "LIGHTFUR" ) ) {
        floor_mut_warmth += 100;
    }
    // Spider hair really isn't meant for this sort of thing
    if( has_trait( "CHITIN_FUR" ) ) {
        floor_mut_warmth += 50;
    }
    if( has_trait( "CHITIN_FUR2" ) || has_trait( "CHITIN_FUR3" ) ) {
        floor_mut_warmth += 75;
    }
    // Down helps too
    if( has_trait( "DOWN" ) ) {
        floor_mut_warmth += 250;
    }
    // Curl up in your shell to conserve heat & stay warm
    if( has_active_mutation( "SHELL2" ) ) {
        floor_mut_warmth += 200;
    }
    return floor_mut_warmth;
}

int player::temp_corrected_by_climate_control( int temperature )
{
    const int variation = BODYTEMP_NORM * 0.5;
    if( in_climate_control() && temperature < BODYTEMP_SCORCHING + variation &&
        temperature > BODYTEMP_FREEZING - variation ) {
        if( temperature > BODYTEMP_SCORCHING ) {
            temperature = BODYTEMP_VERY_HOT;
        } else if( temperature > BODYTEMP_VERY_HOT ) {
            temperature = BODYTEMP_HOT;
        } else if( temperature > BODYTEMP_HOT ) {
            temperature = BODYTEMP_NORM;
        } else if( temperature < BODYTEMP_FREEZING ) {
            temperature = BODYTEMP_VERY_COLD;
        } else if( temperature < BODYTEMP_VERY_COLD) {
            temperature = BODYTEMP_COLD;
        } else if( temperature < BODYTEMP_COLD ) {
            temperature = BODYTEMP_NORM;
        }
    }
    return temperature;
}

int player::blood_loss(body_part bp)
{
    int blood_loss = 0;
    if( bp == bp_leg_l || bp == bp_leg_r ) {
        blood_loss = (100 - 100 * (hp_cur[hp_leg_l] + hp_cur[hp_leg_r]) /
                      (hp_max[hp_leg_l] + hp_max[hp_leg_r]));
    } else if( bp == bp_arm_l || bp == bp_arm_r ) {
        blood_loss = (100 - 100 * (hp_cur[hp_arm_l] + hp_cur[hp_arm_r]) /
                      (hp_max[hp_arm_l] + hp_max[hp_arm_r]));
    } else if( bp == bp_torso ) {
        blood_loss = (100 - 100 * hp_cur[hp_torso] / hp_max[hp_torso]);
    } else if( bp == bp_head ) {
        blood_loss = (100 - 100 * hp_cur[hp_head] / hp_max[hp_head]);
    }
    return blood_loss;
}

void player::temp_equalizer(body_part bp1, body_part bp2)
{
    // Body heat is moved around.
    // Shift in one direction only, will be shifted in the other direction separately.
    int diff = (temp_cur[bp2] - temp_cur[bp1]) * 0.0001; // If bp1 is warmer, it will lose heat
    temp_cur[bp1] += diff;
}

void player::recalc_speed_bonus()
{
    // Minus some for weight...
    int carry_penalty = 0;
    if (weight_carried() > weight_capacity()) {
        carry_penalty = 25 * (weight_carried() - weight_capacity()) / (weight_capacity());
    }
    mod_speed_bonus(-carry_penalty);

    if (pain > pkill) {
        int pain_penalty = int((pain - pkill) * .7);
        // Cenobites aren't slowed nearly as much by pain
        if (has_trait("CENOBITE")) {
            pain_penalty /= 4;
        }
        if (pain_penalty > 60) {
            pain_penalty = 60;
        }
        mod_speed_bonus(-pain_penalty);
    }
    if (pkill >= 10) {
        int pkill_penalty = int(pkill * .1);
        if (pkill_penalty > 30) {
            pkill_penalty = 30;
        }
        mod_speed_bonus(-pkill_penalty);
    }

    if (abs(morale_level()) >= 100) {
        int morale_bonus = int(morale_level() / 25);
        if (morale_bonus < -10) {
            morale_bonus = -10;
        } else if (morale_bonus > 10) {
            morale_bonus = 10;
        }
        mod_speed_bonus(morale_bonus);
    }

    if (radiation >= 40) {
        int rad_penalty = radiation / 40;
        if (rad_penalty > 20) {
            rad_penalty = 20;
        }
        mod_speed_bonus(-rad_penalty);
    }

    if (thirst > 40) {
        mod_speed_bonus(-int((thirst - 40) / 10));
    }
    if (get_hunger() > 100) {
        mod_speed_bonus(-int((get_hunger() - 100) / 10));
    }

    mod_speed_bonus( stim > 10 ? 10 : stim / 4);

    for (auto maps : effects) {
        for (auto i : maps.second) {
            bool reduced = resists_effect(i.second);
            mod_speed_bonus(i.second.get_mod("SPEED", reduced));
        }
    }

    // add martial arts speed bonus
    mod_speed_bonus(mabuff_speed_bonus());

    // Not sure why Sunlight Dependent is here, but OK
    // Ectothermic/COLDBLOOD4 is intended to buff folks in the Summer
    // Threshold-crossing has its charms ;-)
    if (g != NULL) {
        if (has_trait("SUNLIGHT_DEPENDENT") && !g->is_in_sunlight(pos())) {
            mod_speed_bonus(-(g->light_level( posz() ) >= 12 ? 5 : 10));
        }
        if ((has_trait("COLDBLOOD4")) && g->get_temperature() > 60) {
            mod_speed_bonus(+int( (g->get_temperature() - 65) / 2));
        }
        if ((has_trait("COLDBLOOD3") || has_trait("COLDBLOOD4")) && g->get_temperature() < 60) {
            mod_speed_bonus(-int( (65 - g->get_temperature()) / 2));
        } else if (has_trait("COLDBLOOD2") && g->get_temperature() < 60) {
            mod_speed_bonus(-int( (65 - g->get_temperature()) / 3));
        } else if (has_trait("COLDBLOOD") && g->get_temperature() < 60) {
            mod_speed_bonus(-int( (65 - g->get_temperature()) / 5));
        }
    }

    if (has_trait("M_SKIN2")) {
        mod_speed_bonus(-20); // Could be worse--you've got the armor from a (sessile!) Spire
    }

    if (has_artifact_with(AEP_SPEED_UP)) {
        mod_speed_bonus(20);
    }
    if (has_artifact_with(AEP_SPEED_DOWN)) {
        mod_speed_bonus(-20);
    }

    if (has_trait("QUICK")) { // multiply by 1.1
        set_speed_bonus(get_speed() * 1.10 - get_speed_base());
    }
    if (has_bionic("bio_speed")) { // multiply by 1.1
        set_speed_bonus(get_speed() * 1.10 - get_speed_base());
    }

    // Speed cannot be less than 25% of base speed, so minimal speed bonus is -75% base speed.
    const int min_speed_bonus = -0.75 * get_speed_base();
    if (get_speed_bonus() < min_speed_bonus) {
        set_speed_bonus(min_speed_bonus);
    }
}

int player::run_cost(int base_cost, bool diag) const
{
    float movecost = float(base_cost);
    if( diag ) {
        movecost *= 0.7071f; // because everything here assumes 100 is base
    }

    const bool flatground = movecost < 105;
    // The "FLAT" tag includes soft surfaces, so not a good fit.
    const bool on_road = flatground && g->m.has_flag( "ROAD", pos() );

    if (has_trait("PARKOUR") && movecost > 100 ) {
        movecost *= .5f;
        if (movecost < 100)
            movecost = 100;
    }
    if (has_trait("BADKNEES") && movecost > 100 ) {
        movecost *= 1.25f;
        if (movecost < 100)
            movecost = 100;
    }

    if (hp_cur[hp_leg_l] == 0) {
        movecost += 50;
    } else if (hp_cur[hp_leg_l] < hp_max[hp_leg_l] * .40) {
        movecost += 25;
    }

    if (hp_cur[hp_leg_r] == 0) {
        movecost += 50;
    } else if (hp_cur[hp_leg_r] < hp_max[hp_leg_r] * .40) {
        movecost += 25;
    }

    if (has_trait("FLEET") && flatground) {
        movecost *= .85f;
    }
    if (has_trait("FLEET2") && flatground) {
        movecost *= .7f;
    }
    if (has_trait("SLOWRUNNER") && flatground) {
        movecost *= 1.15f;
    }
    if (has_trait("PADDED_FEET") && !footwear_factor()) {
        movecost *= .9f;
    }
    if (has_trait("LIGHT_BONES")) {
        movecost *= .9f;
    }
    if (has_trait("HOLLOW_BONES")) {
        movecost *= .8f;
    }
    if (has_active_mutation("WINGS_INSECT")) {
        movecost *= .75f;
    }
    if (has_trait("WINGS_BUTTERFLY")) {
        movecost -= 10; // You can't fly, but you can make life easier on your legs
    }
    if (has_trait("LEG_TENTACLES")) {
        movecost += 20;
    }
    if (has_trait("FAT")) {
        movecost *= 1.05f;
    }
    if (has_trait("PONDEROUS1")) {
        movecost *= 1.1f;
    }
    if (has_trait("PONDEROUS2")) {
        movecost *= 1.2f;
    }
    if (has_trait("AMORPHOUS")) {
        movecost *= 1.25f;
    }
    if (has_trait("PONDEROUS3")) {
        movecost *= 1.3f;
    }
    if( is_wearing("swim_fins") ) {
        movecost *= 1.5f;
    }
    if( is_wearing("roller_blades") ) {
        if( on_road ) {
            movecost *= 0.5f;
        } else {
            movecost *= 1.5f;
        }
    }
    // Quad skates might be more stable than inlines,
    // but that also translates into a slower speed when on good surfaces.
    if ( is_wearing("rollerskates") ) {
        if( on_road ) {
            movecost *= 0.7f;
        } else {
            movecost *= 1.3f;
        }
    }

    movecost +=
        ( ( encumb(bp_foot_l) + encumb(bp_foot_r) ) * 2.5 +
          ( encumb(bp_leg_l) + encumb(bp_leg_r) ) * 1.5 ) / 10;

    // ROOTS3 does slow you down as your roots are probing around for nutrients,
    // whether you want them to or not.  ROOTS1 is just too squiggly without shoes
    // to give you some stability.  Plants are a bit of a slow-mover.  Deal.
    const bool mutfeet = has_trait("LEG_TENTACLES") || has_trait("PADDED_FEET") ||
        has_trait("HOOVES") || has_trait("TOUGH_FEET") || has_trait("ROOTS2");
    if( !is_wearing_shoes("left") && !mutfeet ) {
        movecost += 8;
    }
    if( !is_wearing_shoes("right") && !mutfeet ) {
        movecost += 8;
    }

    if( !footwear_factor() && has_trait("ROOTS3") &&
        g->m.has_flag("DIGGABLE", pos()) ) {
        movecost += 10 * footwear_factor();
    }

    // Both walk and run speed drop to half their maximums as stamina approaches 0.
    float stamina_modifier = (1.0 + ((float)stamina / (float)get_stamina_max())) / 2.0;
    if( move_mode == "run" && stamina > 0 ) {
        // Rationale: Average running speed is 2x walking speed. (NOT sprinting)
        stamina_modifier *= 2.0;
    }
    movecost /= stamina_modifier;

    if (diag) {
        movecost *= 1.4142;
    }

    return int(movecost);
}

int player::swim_speed() const
{
    int ret = 440 + weight_carried() / 60 - 50 * get_skill_level( skill_swimming );
    ///\EFFECT_STR increases swim speed bonus from PAWS
    if (has_trait("PAWS")) {
        ret -= 20 + str_cur * 3;
    }
    ///\EFFECT_STR increases swim speed bonus from PAWS_LARGE
    if (has_trait("PAWS_LARGE")) {
        ret -= 20 + str_cur * 4;
    }
    ///\EFFECT_STR increases swim speed bonus from swim_fins
    if (is_wearing("swim_fins")) {
        ret -= (15 * str_cur) / (3 - shoe_type_count("swim_fins"));
    }
    ///\EFFECT_STR increases swim speed bonus from WEBBED
    if (has_trait("WEBBED")) {
        ret -= 60 + str_cur * 5;
    }
    ///\EFFECT_STR increases swim speed bonus from TAIL_FIN
    if (has_trait("TAIL_FIN")) {
        ret -= 100 + str_cur * 10;
    }
    if (has_trait("SLEEK_SCALES")) {
        ret -= 100;
    }
    if (has_trait("LEG_TENTACLES")) {
        ret -= 60;
    }
    if (has_trait("FAT")) {
        ret -= 30;
    }
    ///\EFFECT_SWIMMING increases swim speed
    ret += (50 - get_skill_level( skill_swimming ) * 2) * ((encumb(bp_leg_l) + encumb(bp_leg_r)) / 10);
    ret += (80 - get_skill_level( skill_swimming ) * 3) * (encumb(bp_torso) / 10);
    if (get_skill_level( skill_swimming ) < 10) {
        for (auto &i : worn) {
            ret += (i.volume() * (10 - get_skill_level( skill_swimming ))) / 2;
        }
    }
    ///\EFFECT_STR increases swim speed

    ///\EFFECT_DEX increases swim speed
    ret -= str_cur * 6 + dex_cur * 4;
    if( worn_with_flag("FLOTATION") ) {
        ret = std::min(ret, 400);
        ret = std::max(ret, 200);
    }
    // If (ret > 500), we can not swim; so do not apply the underwater bonus.
    if (underwater && ret < 500) {
        ret -= 50;
    }
    if (ret < 30) {
        ret = 30;
    }
    return ret;
}

bool player::digging() const
{
    return false;
}

bool player::is_on_ground() const
{
    return hp_cur[hp_leg_l] == 0 || hp_cur[hp_leg_r] == 0 || has_effect("downed");
}

bool player::is_elec_immune() const
{
    return is_immune_damage( DT_ELECTRIC );
}

bool player::is_immune_effect( const efftype_id &effect ) const
{
    if( effect == "downed" ) {
        return is_throw_immune() || ( has_trait("LEG_TENT_BRACE") && footwear_factor() == 0 );
    } else if( effect == "onfire" ) {
        return is_immune_damage( DT_HEAT );
    } else if( effect == "deaf" ) {
        return worn_with_flag("DEAF") || has_bionic("bio_ears") || is_wearing("rm13_armor_on");
    } else if( effect == "corroding" ) {
        return has_trait( "ACIDPROOF" );
    }

    return false;
}

int player::stability_roll() const
{
    ///\EFFECT_STR improves player stability roll

    ///\EFFECT_PER slightly improves player stability roll

    ///\EFFECT_DEX slightly improves player stability roll

    ///\EFFECT_MELEE improves player stability roll
    int stability = (get_melee()) + get_str() + (get_per() / 3) + (get_dex() / 4);
        return stability;
}

bool player::is_immune_damage( const damage_type dt ) const
{
    switch( dt ) {
    case DT_NULL:
        return true;
    case DT_TRUE:
        return false;
    case DT_BIOLOGICAL:
        return false;
    case DT_BASH:
        return false;
    case DT_CUT:
        return false;
    case DT_ACID:
        return has_trait( "ACIDPROOF" );
    case DT_STAB:
        return false;
    case DT_HEAT:
        return has_trait("M_SKIN2");
    case DT_COLD:
        return false;
    case DT_ELECTRIC:
        return has_active_bionic( "bio_faraday" ) ||
               worn_with_flag( "ELECTRIC_IMMUNE" ) ||
               has_artifact_with( AEP_RESIST_ELECTRICITY );
    default:
        return true;
    }
}

bool player::is_underwater() const
{
    return underwater;
}

bool player::is_hallucination() const
{
    return false;
}

void player::set_underwater(bool u)
{
    if (underwater != u) {
        underwater = u;
        recalc_sight_limits();
    }
}


nc_color player::basic_symbol_color() const
{
    if (has_effect("onfire")) {
        return c_red;
    }
    if (has_effect("stunned")) {
        return c_ltblue;
    }
    if (has_effect("boomered")) {
        return c_pink;
    }
    if (has_active_mutation("SHELL2")) {
        return c_magenta;
    }
    if (underwater) {
        return c_blue;
    }
    if (has_active_bionic("bio_cloak") || has_artifact_with(AEP_INVISIBLE) ||
          has_active_optcloak() || has_trait("DEBUG_CLOAK")) {
        return c_dkgray;
    }
    return c_white;
}

void player::load_info(std::string data)
{
    std::stringstream dump;
    dump << data;

    JsonIn jsin(dump);
    try {
        deserialize(jsin);
    } catch( const JsonError &jsonerr ) {
        debugmsg("Bad player json\n%s", jsonerr.c_str() );
    }
}

std::string player::save_info() const
{
    std::stringstream dump;
    dump << serialize(); // saves contents
    dump << std::endl;
    dump << dump_memorial();
    return dump.str();
}

void player::memorial( std::ofstream &memorial_file, std::string epitaph )
{
    //Size of indents in the memorial file
    const std::string indent = "  ";

    const std::string pronoun = male ? _("He") : _("She");

    //Avoid saying "a male unemployed" or similar
    std::string profession_name;
    if(prof == prof->generic()) {
        if (male) {
            profession_name = _("an unemployed male");
        } else {
            profession_name = _("an unemployed female");
        }
    } else {
        profession_name = string_format(_("a %s"), prof->gender_appropriate_name(male).c_str());
    }

    //Figure out the location
    const oter_id &cur_ter = overmap_buffer.ter( global_omt_location() );
    std::string tername = otermap[cur_ter].name;

    //Were they in a town, or out in the wilderness?
    const auto global_sm_pos = global_sm_location();
    const auto closest_city = overmap_buffer.closest_city( global_sm_pos );
    std::string kill_place;
    if( !closest_city ) {
        //~ First parameter is a pronoun ("He"/"She"), second parameter is a terrain name.
        kill_place = string_format(_("%1$s was killed in a %2$s in the middle of nowhere."),
                     pronoun.c_str(), tername.c_str());
    } else {
        const auto &nearest_city = *closest_city.city;
        //Give slightly different messages based on how far we are from the middle
        const int distance_from_city = closest_city.distance - nearest_city.s;
        if(distance_from_city > nearest_city.s + 4) {
            //~ First parameter is a pronoun ("He"/"She"), second parameter is a terrain name.
            kill_place = string_format(_("%1$s was killed in a %2$s in the wilderness."),
                         pronoun.c_str(), tername.c_str());

        } else if(distance_from_city >= nearest_city.s) {
            //~ First parameter is a pronoun ("He"/"She"), second parameter is a terrain name, third parameter is a city name.
            kill_place = string_format(_("%1$s was killed in a %2$s on the outskirts of %3$s."),
                         pronoun.c_str(), tername.c_str(), nearest_city.name.c_str());
        } else {
            //~ First parameter is a pronoun ("He"/"She"), second parameter is a terrain name, third parameter is a city name.
            kill_place = string_format(_("%1$s was killed in a %2$s in %3$s."),
                         pronoun.c_str(), tername.c_str(), nearest_city.name.c_str());
        }
    }

    //Header
    std::string version = string_format("%s", getVersionString());
    memorial_file << string_format(_("Cataclysm - Dark Days Ahead version %s memorial file"), version.c_str()) << "\n";
    memorial_file << "\n";
    memorial_file << string_format(_("In memory of: %s"), name.c_str()) << "\n";
    if(epitaph.length() > 0) { //Don't record empty epitaphs
        //~ The "%s" will be replaced by an epitaph as displyed in the memorial files. Replace the quotation marks as appropriate for your language.
        memorial_file << string_format(pgettext("epitaph","\"%s\""), epitaph.c_str()) << "\n\n";
    }
    //~ First parameter: Pronoun, second parameter: a profession name (with article)
    memorial_file << string_format("%1$s was %2$s when the apocalypse began.",
                                   pronoun.c_str(), profession_name.c_str()) << "\n";
    memorial_file << string_format("%1$s died on %2$s of year %3$d, day %4$d, at %5$s.",
                     pronoun.c_str(), season_name_upper(calendar::turn.get_season()).c_str(), (calendar::turn.years() + 1),
                     (calendar::turn.days() + 1), calendar::turn.print_time().c_str()) << "\n";
    memorial_file << kill_place << "\n";
    memorial_file << "\n";

    //Misc
    memorial_file << string_format(_("Cash on hand: $%d"), cash) << "\n";
    memorial_file << "\n";

    //HP
    memorial_file << _("Final HP:") << "\n";
    memorial_file << indent << string_format(_(" Head: %d/%d"), hp_cur[hp_head],  hp_max[hp_head] ) << "\n";
    memorial_file << indent << string_format(_("Torso: %d/%d"), hp_cur[hp_torso], hp_max[hp_torso]) << "\n";
    memorial_file << indent << string_format(_("L Arm: %d/%d"), hp_cur[hp_arm_l], hp_max[hp_arm_l]) << "\n";
    memorial_file << indent << string_format(_("R Arm: %d/%d"), hp_cur[hp_arm_r], hp_max[hp_arm_r]) << "\n";
    memorial_file << indent << string_format(_("L Leg: %d/%d"), hp_cur[hp_leg_l], hp_max[hp_leg_l]) << "\n";
    memorial_file << indent << string_format(_("R Leg: %d/%d"), hp_cur[hp_leg_r], hp_max[hp_leg_r]) << "\n";
    memorial_file << "\n";

    //Stats
    memorial_file << _("Final Stats:") << "\n";
    memorial_file << indent << string_format(_("Str %d"), str_cur)
                  << indent << string_format(_("Dex %d"), dex_cur)
                  << indent << string_format(_("Int %d"), int_cur)
                  << indent << string_format(_("Per %d"), per_cur) << "\n";
    memorial_file << _("Base Stats:") << "\n";
    memorial_file << indent << string_format(_("Str %d"), str_max)
                  << indent << string_format(_("Dex %d"), dex_max)
                  << indent << string_format(_("Int %d"), int_max)
                  << indent << string_format(_("Per %d"), per_max) << "\n";
    memorial_file << "\n";

    //Last 20 messages
    memorial_file << _("Final Messages:") << "\n";
    std::vector<std::pair<std::string, std::string> > recent_messages = Messages::recent_messages(20);
    for( auto &recent_message : recent_messages ) {
        memorial_file << indent << recent_message.first << " " << recent_message.second;
      memorial_file << "\n";
    }
    memorial_file << "\n";

    //Kill list
    memorial_file << _("Kills:") << "\n";

    int total_kills = 0;

    std::map<std::tuple<std::string,std::string>,int> kill_counts;

    // map <name, sym> to kill count
    for( const auto &monid : MonsterGenerator::generator().get_all_mtypes() ) {
        if( g->kill_count( monid.first ) > 0 ) {
            kill_counts[std::tuple<std::string,std::string>(
                monid.second->nname(),
                monid.second->sym
            )] += g->kill_count( monid.first );
            total_kills += g->kill_count( monid.first );
        }
    }

    for( const auto entry : kill_counts ) {
        memorial_file << "  " << std::get<1>( entry.first ) << " - "
                      << string_format( "%4d", entry.second ) << " "
                      << std::get<0>( entry.first ) << "\n";
    }

    if( total_kills == 0 ) {
      memorial_file << indent << _( "No monsters were killed." ) << "\n";
    } else {
      memorial_file << string_format( _( "Total kills: %d" ), total_kills ) << "\n";
    }
    memorial_file << "\n";

    //Skills
    memorial_file << _( "Skills:" ) << "\n";
    for( auto &skill : Skill::skills ) {
        SkillLevel next_skill_level = get_skill_level( skill );
        memorial_file << indent << skill.name() << ": " << next_skill_level.level() << " ("
                      << next_skill_level.exercise() << "%)\n";
    }
    memorial_file << "\n";

    //Traits
    memorial_file << _("Traits:") << "\n";
    for( auto &iter : my_mutations ) {
        memorial_file << indent << mutation_branch::get_name( iter.first ) << "\n";
    }
    if( !my_mutations.empty() ) {
      memorial_file << indent << _("(None)") << "\n";
    }
    memorial_file << "\n";

    //Effects (illnesses)
    memorial_file << _("Ongoing Effects:") << "\n";
    bool had_effect = false;
    if(morale_level() >= 100) {
      had_effect = true;
      memorial_file << indent << _("Elated") << "\n";
    }
    if(morale_level() <= -100) {
      had_effect = true;
      memorial_file << indent << _("Depressed") << "\n";
    }
    if(pain - pkill > 0) {
      had_effect = true;
      memorial_file << indent << _("Pain") << " (" << (pain - pkill) << ")";
    }
    if(stim > 0) {
      had_effect = true;
      int dexbonus = int(stim / 10);
      if (abs(stim) >= 30) {
        dexbonus -= int(abs(stim - 15) /  8);
      }
      if(dexbonus < 0) {
        memorial_file << indent << _("Stimulant Overdose") << "\n";
      } else {
        memorial_file << indent << _("Stimulant") << "\n";
      }
    } else if(stim < 0) {
      had_effect = true;
      memorial_file << indent << _("Depressants") << "\n";
    }
    if(!had_effect) {
      memorial_file << indent << _("(None)") << "\n";
    }
    memorial_file << "\n";

    //Bionics
    memorial_file << _("Bionics:") << "\n";
    int total_bionics = 0;
    for( size_t i = 0; i < my_bionics.size(); ++i ) {
      memorial_file << indent << (i+1) << ": " << bionic_info(my_bionics[i].id).name << "\n";
      total_bionics++;
    }
    if(total_bionics == 0) {
      memorial_file << indent << _("No bionics were installed.") << "\n";
    } else {
      memorial_file << string_format(_("Total bionics: %d"), total_bionics) << "\n";
    }
    memorial_file << string_format(_("Power: %d/%d"), power_level,  max_power_level) << "\n";
    memorial_file << "\n";

    //Equipment
    memorial_file << _("Weapon:") << "\n";
    memorial_file << indent << weapon.invlet << " - " << weapon.tname(1, false) << "\n";
    memorial_file << "\n";

    memorial_file << _("Equipment:") << "\n";
    for( auto &elem : worn ) {
        item next_item = elem;
        memorial_file << indent << next_item.invlet << " - " << next_item.tname(1, false);
        if( next_item.charges > 0 ) {
            memorial_file << " (" << next_item.charges << ")";
        } else if( next_item.contents.size() == 1 && next_item.contents[0].charges > 0 ) {
            memorial_file << " (" << next_item.contents[0].charges << ")";
        }
        memorial_file << "\n";
    }
    memorial_file << "\n";

    //Inventory
    memorial_file << _("Inventory:") << "\n";
    inv.restack(this);
    inv.sort();
    invslice slice = inv.slice();
    for( auto &elem : slice ) {
        item &next_item = elem->front();
        memorial_file << indent << next_item.invlet << " - " <<
            next_item.tname(elem->size(), false);
        if( elem->size() > 1 ) {
            memorial_file << " [" << elem->size() << "]";
        }
        if( next_item.charges > 0 ) {
            memorial_file << " (" << next_item.charges << ")";
        } else if( next_item.contents.size() == 1 && next_item.contents[0].charges > 0 ) {
            memorial_file << " (" << next_item.contents[0].charges << ")";
        }
        memorial_file << "\n";
    }
    memorial_file << "\n";

    //Lifetime stats
    memorial_file << _("Lifetime Stats") << "\n";
    memorial_file << indent << string_format(_("Distance walked: %d squares"),
                       player_stats.squares_walked) << "\n";
    memorial_file << indent << string_format(_("Damage taken: %d damage"),
                       player_stats.damage_taken) << "\n";
    memorial_file << indent << string_format(_("Damage healed: %d damage"),
                       player_stats.damage_healed) << "\n";
    memorial_file << indent << string_format(_("Headshots: %d"),
                       player_stats.headshots) << "\n";
    memorial_file << "\n";

    //History
    memorial_file << _("Game History") << "\n";
    memorial_file << dump_memorial();

}

/**
 * Adds an event to the memorial log, to be written to the memorial file when
 * the character dies. The message should contain only the informational string,
 * as the timestamp and location will be automatically prepended.
 */
void player::add_memorial_log(const char* male_msg, const char* female_msg, ...)
{

    va_list ap;

    va_start(ap, female_msg);
    std::string msg;
    if(this->male) {
        msg = vstring_format(male_msg, ap);
    } else {
        msg = vstring_format(female_msg, ap);
    }
    va_end(ap);

    if(msg.empty()) {
        return;
    }

    std::stringstream timestamp;
    //~ A timestamp. Parameters from left to right: Year, season, day, time
    timestamp << string_format(_("Year %1$d, %2$s %3$d, %4$s"), calendar::turn.years() + 1,
                               season_name_upper(calendar::turn.get_season()).c_str(),
                               calendar::turn.days() + 1, calendar::turn.print_time().c_str()
                               );

    const oter_id &cur_ter = overmap_buffer.ter( global_omt_location() );
    std::string location = otermap[cur_ter].name;

    std::stringstream log_message;
    log_message << "| " << timestamp.str() << " | " << location.c_str() << " | " << msg;

    memorial_log.push_back(log_message.str());

}

/**
 * Loads the data in a memorial file from the given ifstream. All the memorial
 * entry lines begin with a pipe (|).
 * @param fin The ifstream to read the memorial entries from.
 */
void player::load_memorial_file(std::ifstream &fin)
{
  std::string entry;
  memorial_log.clear();
  while(fin.peek() == '|') {
    getline(fin, entry);
    memorial_log.push_back(entry);
  }
}

/**
 * Concatenates all of the memorial log entries, delimiting them with newlines,
 * and returns the resulting string. Used for saving and for writing out to the
 * memorial file.
 */
std::string player::dump_memorial() const
{

  std::stringstream output;

  for( auto &elem : memorial_log ) {
      output << elem << "\n";
  }

  return output.str();

}

/**
 * Returns a pointer to the stat-tracking struct. Its fields should be edited
 * as necessary to track ongoing counters, which will be added to the memorial
 * file. For single events, rather than cumulative counters, see
 * add_memorial_log.
 * @return A pointer to the stats struct being used to track this player's
 *         lifetime stats.
 */
stats* player::lifetime_stats()
{
    return &player_stats;
}

// copy of stats, for saving
stats player::get_stats() const
{
    return player_stats;
}

void player::mod_stat( const std::string &stat, int modifier )
{
    if( stat == "thirst" ) {
        thirst += modifier;
    } else if( stat == "fatigue" ) {
        fatigue += modifier;
    } else if( stat == "oxygen" ) {
        oxygen += modifier;
    } else if( stat == "stamina" ) {
        stamina += modifier;
        stamina = std::min( stamina, get_stamina_max() );
        stamina = std::max( 0, stamina );
    } else {
        // Fall through to the creature method.
        Character::mod_stat( stat, modifier );
    }
}

std::string swim_cost_text(int moves)
{
    return string_format( ngettext( "Swimming costs %+d movement point. ",
                                    "Swimming costs %+d movement points. ",
                                    moves ),
                          moves );
}

std::string run_cost_text(int moves)
{
    return string_format( ngettext( "Running costs %+d movement point. ",
                                    "Running costs %+d movement points. ",
                                    moves ),
                          moves );
}

std::string reload_cost_text(int moves)
{
    return string_format( ngettext( "Reloading costs %+d movement point. ",
                                    "Reloading costs %+d movement points. ",
                                    moves ),
                          moves );
}

std::string melee_cost_text(int moves)
{
    return string_format( ngettext( "Melee and thrown attacks cost %+d movement point. ",
                                    "Melee and thrown attacks cost %+d movement points. ",
                                    moves ),
                          moves );
}

std::string dodge_skill_text(double mod)
{
    return string_format( _( "Dodge skill %+.1f. " ), mod );
}

void player::disp_info()
{
    unsigned line;
    std::vector<std::string> effect_name;
    std::vector<std::string> effect_text;
    std::string tmp = "";
    for( auto &elem : effects ) {
        for( auto &_effect_it : elem.second ) {
            tmp = _effect_it.second.disp_name();
            if (tmp != "") {
                effect_name.push_back( tmp );
                effect_text.push_back( _effect_it.second.disp_desc() );
            }
        }
    }
    if (abs(morale_level()) >= 100) {
        bool pos = (morale_level() > 0);
        effect_name.push_back(pos ? _("Elated") : _("Depressed"));
        std::stringstream morale_text;
        if (abs(morale_level()) >= 200) {
            morale_text << _("Dexterity") << (pos ? " +" : " ") <<
                int(morale_level() / 200) << "   ";
        }
        if (abs(morale_level()) >= 180) {
            morale_text << _("Strength") << (pos ? " +" : " ") <<
                int(morale_level() / 180) << "   ";
        }
        if (abs(morale_level()) >= 125) {
            morale_text << _("Perception") << (pos ? " +" : " ") <<
                int(morale_level() / 125) << "   ";
        }
        morale_text << _("Intelligence") << (pos ? " +" : " ") <<
            int(morale_level() / 100) << "   ";
        effect_text.push_back(morale_text.str());
    }
    if (pain - pkill > 0) {
        effect_name.push_back(_("Pain"));
        std::stringstream pain_text;
        // Cenobites aren't markedly physically impaired by pain.
        if ((pain - pkill >= 15) && (!(has_trait("CENOBITE")))) {
            pain_text << _("Strength") << " -" << int((pain - pkill) / 15) << "   " << _("Dexterity") << " -" <<
                int((pain - pkill) / 15) << "   ";
        }
        // They do find the sensations distracting though.
        // Pleasurable...but distracting.
        if (pain - pkill >= 20) {
            pain_text << _("Perception") << " -" << int((pain - pkill) / 15) << "   ";
        }
        pain_text << _("Intelligence") << " -" << 1 + int((pain - pkill) / 25);
        effect_text.push_back(pain_text.str());
    }
    if (stim > 0) {
        int dexbonus = int(stim / 10);
        int perbonus = int(stim /  7);
        int intbonus = int(stim /  6);
        if (abs(stim) >= 30) {
            dexbonus -= int(abs(stim - 15) /  8);
            perbonus -= int(abs(stim - 15) / 12);
            intbonus -= int(abs(stim - 15) / 14);
        }

        if (dexbonus < 0) {
            effect_name.push_back(_("Stimulant Overdose"));
        } else {
            effect_name.push_back(_("Stimulant"));
        }
        std::stringstream stim_text;
        stim_text << _("Speed") << " +" << stim << "   " << _("Intelligence") <<
            (intbonus > 0 ? " + " : " ") << intbonus << "   " << _("Perception") <<
            (perbonus > 0 ? " + " : " ") << perbonus << "   " << _("Dexterity")  <<
            (dexbonus > 0 ? " + " : " ") << dexbonus;
        effect_text.push_back(stim_text.str());
    } else if (stim < 0) {
        effect_name.push_back(_("Depressants"));
        std::stringstream stim_text;
        int dexpen = int(stim / 10);
        int perpen = int(stim /  7);
        int intpen = int(stim /  6);
        // Since dexpen etc. are always less than 0, no need for + signs
        stim_text << _("Speed") << " " << stim / 4 << "   " << _("Intelligence") << " " << intpen <<
            "   " << _("Perception") << " " << perpen << "   " << _("Dexterity") << " " << dexpen;
        effect_text.push_back(stim_text.str());
    }

    if ((has_trait("TROGLO") && g->is_in_sunlight(pos()) &&
         g->weather == WEATHER_SUNNY) ||
        (has_trait("TROGLO2") && g->is_in_sunlight(pos()) &&
         g->weather != WEATHER_SUNNY)) {
        effect_name.push_back(_("In Sunlight"));
        effect_text.push_back(_("The sunlight irritates you.\n\
Strength - 1;    Dexterity - 1;    Intelligence - 1;    Perception - 1"));
    } else if (has_trait("TROGLO2") && g->is_in_sunlight(pos())) {
        effect_name.push_back(_("In Sunlight"));
        effect_text.push_back(_("The sunlight irritates you badly.\n\
Strength - 2;    Dexterity - 2;    Intelligence - 2;    Perception - 2"));
    } else if (has_trait("TROGLO3") && g->is_in_sunlight(pos())) {
        effect_name.push_back(_("In Sunlight"));
        effect_text.push_back(_("The sunlight irritates you terribly.\n\
Strength - 4;    Dexterity - 4;    Intelligence - 4;    Perception - 4"));
    }

    for( auto &elem : addictions ) {
        if( elem.sated < 0 && elem.intensity >= MIN_ADDICTION_LEVEL ) {
            effect_name.push_back( addiction_name( elem ) );
            effect_text.push_back( addiction_text( *this, elem ) );
        }
    }

    unsigned maxy = unsigned(TERMY);

    unsigned infooffsetytop = 11;
    unsigned infooffsetybottom = 15;
    std::vector<std::string> traitslist = get_mutations();

    unsigned effect_win_size_y = 1 + effect_name.size();
    unsigned trait_win_size_y = 1 + traitslist.size();
    unsigned skill_win_size_y = 1 + Skill::skill_count();

    if (trait_win_size_y + infooffsetybottom > maxy) {
        trait_win_size_y = maxy - infooffsetybottom;
    }

    if (skill_win_size_y + infooffsetybottom > maxy) {
        skill_win_size_y = maxy - infooffsetybottom;
    }

    WINDOW* w_grid_top    = newwin(infooffsetybottom, FULL_SCREEN_WIDTH+1, VIEW_OFFSET_Y, VIEW_OFFSET_X);
    WINDOW* w_grid_skill  = newwin(skill_win_size_y + 1, 27, infooffsetybottom + VIEW_OFFSET_Y, 0 + VIEW_OFFSET_X);
    WINDOW* w_grid_trait  = newwin(trait_win_size_y + 1, 27, infooffsetybottom + VIEW_OFFSET_Y, 27 + VIEW_OFFSET_X);
    WINDOW* w_grid_effect = newwin(effect_win_size_y+ 1, 28, infooffsetybottom + VIEW_OFFSET_Y, 53 + VIEW_OFFSET_X);

    WINDOW* w_tip     = newwin(1, FULL_SCREEN_WIDTH,  VIEW_OFFSET_Y,  0 + VIEW_OFFSET_X);
    WINDOW* w_stats   = newwin(9, 26,  1 + VIEW_OFFSET_Y,  0 + VIEW_OFFSET_X);
    WINDOW* w_traits  = newwin(trait_win_size_y, 26, infooffsetybottom + VIEW_OFFSET_Y,  27 + VIEW_OFFSET_X);
    WINDOW* w_encumb  = newwin(9, 26,  1 + VIEW_OFFSET_Y, 27 + VIEW_OFFSET_X);
    WINDOW* w_effects = newwin(effect_win_size_y, 26, infooffsetybottom + VIEW_OFFSET_Y, 54 + VIEW_OFFSET_X);
    WINDOW* w_speed   = newwin(9, 26,  1 + VIEW_OFFSET_Y, 54 + VIEW_OFFSET_X);
    WINDOW* w_skills  = newwin(skill_win_size_y, 26, infooffsetybottom + VIEW_OFFSET_Y, 0 + VIEW_OFFSET_X);
    WINDOW* w_info    = newwin(3, FULL_SCREEN_WIDTH, infooffsetytop + VIEW_OFFSET_Y,  0 + VIEW_OFFSET_X);

    for (unsigned i = 0; i < unsigned(FULL_SCREEN_WIDTH + 1); i++) {
        //Horizontal line top grid
        mvwputch(w_grid_top, 10, i, BORDER_COLOR, LINE_OXOX);
        mvwputch(w_grid_top, 14, i, BORDER_COLOR, LINE_OXOX);

        //Vertical line top grid
        if (i <= infooffsetybottom) {
            mvwputch(w_grid_top, i, 26, BORDER_COLOR, LINE_XOXO);
            mvwputch(w_grid_top, i, 53, BORDER_COLOR, LINE_XOXO);
            mvwputch(w_grid_top, i, FULL_SCREEN_WIDTH, BORDER_COLOR, LINE_XOXO);
        }

        //Horizontal line skills
        if (i <= 26) {
            mvwputch(w_grid_skill, skill_win_size_y, i, BORDER_COLOR, LINE_OXOX);
        }

        //Vertical line skills
        if (i <= skill_win_size_y) {
            mvwputch(w_grid_skill, i, 26, BORDER_COLOR, LINE_XOXO);
        }

        //Horizontal line traits
        if (i <= 26) {
            mvwputch(w_grid_trait, trait_win_size_y, i, BORDER_COLOR, LINE_OXOX);
        }

        //Vertical line traits
        if (i <= trait_win_size_y) {
            mvwputch(w_grid_trait, i, 26, BORDER_COLOR, LINE_XOXO);
        }

        //Horizontal line effects
        if (i <= 27) {
            mvwputch(w_grid_effect, effect_win_size_y, i, BORDER_COLOR, LINE_OXOX);
        }

        //Vertical line effects
        if (i <= effect_win_size_y) {
            mvwputch(w_grid_effect, i, 0, BORDER_COLOR, LINE_XOXO);
            mvwputch(w_grid_effect, i, 27, BORDER_COLOR, LINE_XOXO);
        }
    }

    //Intersections top grid
    mvwputch(w_grid_top, 14, 26, BORDER_COLOR, LINE_OXXX); // T
    mvwputch(w_grid_top, 14, 53, BORDER_COLOR, LINE_OXXX); // T
    mvwputch(w_grid_top, 10, 26, BORDER_COLOR, LINE_XXOX); // _|_
    mvwputch(w_grid_top, 10, 53, BORDER_COLOR, LINE_XXOX); // _|_
    mvwputch(w_grid_top, 10, FULL_SCREEN_WIDTH, BORDER_COLOR, LINE_XOXX); // -|
    mvwputch(w_grid_top, 14, FULL_SCREEN_WIDTH, BORDER_COLOR, LINE_XOXX); // -|
    wrefresh(w_grid_top);

    mvwputch(w_grid_skill, skill_win_size_y, 26, BORDER_COLOR, LINE_XOOX); // _|

    if (skill_win_size_y > trait_win_size_y)
        mvwputch(w_grid_skill, trait_win_size_y, 26, BORDER_COLOR, LINE_XXXO); // |-
    else if (skill_win_size_y == trait_win_size_y)
        mvwputch(w_grid_skill, trait_win_size_y, 26, BORDER_COLOR, LINE_XXOX); // _|_

    mvwputch(w_grid_trait, trait_win_size_y, 26, BORDER_COLOR, LINE_XOOX); // _|

    if (trait_win_size_y > effect_win_size_y) {
        mvwputch(w_grid_trait, effect_win_size_y, 26, BORDER_COLOR, LINE_XXXO); // |-
    } else if (trait_win_size_y == effect_win_size_y) {
        mvwputch(w_grid_trait, effect_win_size_y, 26, BORDER_COLOR, LINE_XXOX); // _|_
    } else if (trait_win_size_y < effect_win_size_y) {
        mvwputch(w_grid_trait, trait_win_size_y, 26, BORDER_COLOR, LINE_XOXX); // -|
        mvwputch(w_grid_trait, effect_win_size_y, 26, BORDER_COLOR, LINE_XXOO); // |_
    }

    mvwputch(w_grid_effect, effect_win_size_y, 0, BORDER_COLOR, LINE_XXOO); // |_
    mvwputch(w_grid_effect, effect_win_size_y, 27, BORDER_COLOR, LINE_XOOX); // _|

    wrefresh(w_grid_skill);
    wrefresh(w_grid_effect);
    wrefresh(w_grid_trait);

    //-1 for header
    trait_win_size_y--;
    skill_win_size_y--;
    effect_win_size_y--;

    // Print name and header
    // Post-humanity trumps your pre-Cataclysm life.
    if (crossed_threshold()) {
        std::string race;
        for( auto &mut : my_mutations ) {
            const auto &mdata = mutation_branch::get( mut.first );
            if( mdata.threshold ) {
                race = mdata.name;
                break;
            }
        }
        //~ player info window: 1s - name, 2s - gender, 3s - Prof or Mutation name
        mvwprintw(w_tip, 0, 0, _("%1$s | %2$s | %3$s"), name.c_str(),
                  male ? _("Male") : _("Female"), race.c_str());
    } else if (prof == NULL || prof == prof->generic()) {
        // Regular person. Nothing interesting.
        //~ player info window: 1s - name, 2s - gender, '|' - field separator.
        mvwprintw(w_tip, 0, 0, _("%1$s | %2$s"), name.c_str(),
                  male ? _("Male") : _("Female"));
    } else {
        mvwprintw(w_tip, 0, 0, _("%1$s | %2$s | %3$s"), name.c_str(),
                  male ? _("Male") : _("Female"), prof->gender_appropriate_name(male).c_str());
    }

    input_context ctxt("PLAYER_INFO");
    ctxt.register_updown();
    ctxt.register_action("NEXT_TAB", _("Cycle to next category"));
    ctxt.register_action("QUIT");
    ctxt.register_action("CONFIRM", _("Toggle skill training"));
    ctxt.register_action("HELP_KEYBINDINGS");
    std::string action;

    std::string help_msg = string_format(_("Press %s for help."), ctxt.get_desc("HELP_KEYBINDINGS").c_str());
    mvwprintz(w_tip, 0, FULL_SCREEN_WIDTH - utf8_width(help_msg), c_ltred, help_msg.c_str());
    help_msg.clear();
    wrefresh(w_tip);

    // First!  Default STATS screen.
    const char* title_STATS = _("STATS");
    mvwprintz(w_stats, 0, 13 - utf8_width(title_STATS)/2, c_ltgray, title_STATS);

    nc_color status = c_white;

    // Stats
    const auto display_stat = [&w_stats]( const char *name, int cur, int max, int line ) {
        nc_color status = c_white;
        if( cur <= 0 ) {
            status = c_dkgray;
        } else if( cur < max / 2 ) {
            status = c_red;
        } else if( cur < max ) {
            status = c_ltred;
        } else if( cur == max ) {
            status = c_white;
        } else if( cur < max * 1.5 ) {
            status = c_ltgreen;
        } else {
            status = c_green;
        }

        mvwprintz( w_stats, line, 1, c_ltgray, name );
        mvwprintz( w_stats, line, 18, status, "%2d", cur );
        mvwprintz( w_stats, line, 21, c_ltgray, "(%2d)", max );
    };

    display_stat( _("Strength:"),     str_cur, str_max, 2 );
    display_stat( _("Dexterity:"),    dex_cur, dex_max, 3 );
    display_stat( _("Intelligence:"), int_cur, int_max, 4 );
    display_stat( _("Perception:"),   per_cur, per_max, 5 );

    wrefresh(w_stats);

    // Next, draw encumberment.
    const char *title_ENCUMB = _("ENCUMBRANCE AND WARMTH");
    mvwprintz(w_encumb, 0, 13 - utf8_width(title_ENCUMB) / 2, c_ltgray, title_ENCUMB);
    print_encumbrance(w_encumb, 0, 8);
    wrefresh(w_encumb);

    // Next, draw traits.
    const char *title_TRAITS = _("TRAITS");
    mvwprintz(w_traits, 0, 13 - utf8_width(title_TRAITS)/2, c_ltgray, title_TRAITS);
    std::sort(traitslist.begin(), traitslist.end(), trait_display_sort);
    for (size_t i = 0; i < traitslist.size() && i < trait_win_size_y; i++) {
        const auto &mdata = mutation_branch::get( traitslist[i] );
        const auto color = mdata.get_display_color();
        mvwprintz(w_traits, i+1, 1, color, mdata.name.c_str());
    }
    wrefresh(w_traits);

    // Next, draw effects.
    const char *title_EFFECTS = _("EFFECTS");
    mvwprintz(w_effects, 0, 13 - utf8_width(title_EFFECTS)/2, c_ltgray, title_EFFECTS);
    for (size_t i = 0; i < effect_name.size() && i < effect_win_size_y; i++) {
        mvwprintz(w_effects, i+1, 0, c_ltgray, "%s", effect_name[i].c_str());
    }
    wrefresh(w_effects);

    // Next, draw skills.
    line = 1;

    const char *title_SKILLS = _("SKILLS");
    mvwprintz(w_skills, 0, 13 - utf8_width(title_SKILLS)/2, c_ltgray, title_SKILLS);

    auto skillslist = Skill::get_skills_sorted_by([&](Skill const& a, Skill const& b) {
        int const level_a = get_skill_level(a).exercised_level();
        int const level_b = get_skill_level(b).exercised_level();
        return level_a > level_b || (level_a == level_b && a.name() < b.name());
    });

    for( auto &elem : skillslist ) {
        SkillLevel level = get_skill_level( elem );

        // Default to not training and not rusting
        nc_color text_color = c_blue;
        bool not_capped = level.can_train();
        bool training = level.isTraining();
        bool rusting = level.isRusting();

        if( training && rusting ) {
            text_color = c_ltred;
        } else if( training && not_capped ) {
            text_color = c_ltblue;
        } else if( rusting ) {
            text_color = c_red;
        } else if( !not_capped ) {
            text_color = c_white;
        }

        int level_num = (int)level;
        int exercise = level.exercise();

        // TODO: this skill list here is used in other places as well. Useless redundancy and
        // dependency. Maybe change it into a flag of the skill that indicates it's a skill used
        // by the bionic?
        static const std::array<skill_id, 5> cqb_skills = { {
            skill_id( "melee" ), skill_id( "unarmed" ), skill_id( "cutting" ),
            skill_id( "bashing" ), skill_id( "stabbing" ),
        } };
        if( has_active_bionic( "bio_cqb" ) &&
            std::find( cqb_skills.begin(), cqb_skills.end(), elem->ident() ) != cqb_skills.end() ) {
            level_num = 5;
            exercise = 0;
            text_color = c_yellow;
        }

        if (line < skill_win_size_y + 1) {
            mvwprintz( w_skills, line, 1, text_color, "%s:", ( elem )->name().c_str() );
            mvwprintz(w_skills, line, 19, text_color, "%-2d(%2d%%)", level_num,
                      (exercise <  0 ? 0 : exercise));
            line++;
        }
    }
    wrefresh(w_skills);

    // Finally, draw speed.
    const char *title_SPEED = _("SPEED");
    mvwprintz(w_speed, 0, 13 - utf8_width(title_SPEED)/2, c_ltgray, title_SPEED);
    mvwprintz(w_speed, 1,  1, c_ltgray, _("Base Move Cost:"));
    mvwprintz(w_speed, 2,  1, c_ltgray, _("Current Speed:"));
    int newmoves = get_speed();
    int pen = 0;
    line = 3;
    if (weight_carried() > weight_capacity()) {
        pen = 25 * (weight_carried() - weight_capacity()) / (weight_capacity());
        mvwprintz(w_speed, line, 1, c_red, _("Overburdened        -%s%d%%"),
                  (pen < 10 ? " " : ""), pen);
        line++;
    }
    pen = int(morale_level() / 25);
    if (abs(pen) >= 4) {
        if (pen > 10)
            pen = 10;
        else if (pen < -10)
            pen = -10;
        if (pen > 0)
            mvwprintz(w_speed, line, 1, c_green, _("Good mood           +%s%d%%"),
                      (pen < 10 ? " " : ""), pen);
        else
            mvwprintz(w_speed, line, 1, c_red, _("Depressed           -%s%d%%"),
                      (abs(pen) < 10 ? " " : ""), abs(pen));
        line++;
    }
    pen = int((pain - pkill) * .7);
    if (has_trait("CENOBITE")) {
        pen /= 4;
    }
    if (pen > 60)
        pen = 60;
    if (pen >= 1) {
        mvwprintz(w_speed, line, 1, c_red, _("Pain                -%s%d%%"),
                  (pen < 10 ? " " : ""), pen);
        line++;
    }
    if (pkill >= 10) {
        pen = int(pkill * .1);
        mvwprintz(w_speed, line, 1, c_red, _("Painkillers         -%s%d%%"),
                  (pen < 10 ? " " : ""), pen);
        line++;
    }
    if (stim != 0) {
        pen = std::min( 10, stim / 4 );
        if (pen > 0)
            mvwprintz(w_speed, line, 1, c_green, _("Stimulants          +%s%d%%"),
                      (pen < 10 ? " " : ""), pen);
        else
            mvwprintz(w_speed, line, 1, c_red, _("Depressants         -%s%d%%"),
                      (abs(pen) < 10 ? " " : ""), abs(pen));
        line++;
    }
    if (thirst > 40) {
        pen = int((thirst - 40) / 10);
        mvwprintz(w_speed, line, 1, c_red, _("Thirst              -%s%d%%"),
                  (pen < 10 ? " " : ""), pen);
        line++;
    }
    if (get_hunger() > 100) {
        pen = int((get_hunger() - 100) / 10);
        mvwprintz(w_speed, line, 1, c_red, _("Hunger              -%s%d%%"),
                  (pen < 10 ? " " : ""), pen);
        line++;
    }
    if (has_trait("SUNLIGHT_DEPENDENT") && !g->is_in_sunlight(pos())) {
        pen = (g->light_level( posz() ) >= 12 ? 5 : 10);
        mvwprintz(w_speed, line, 1, c_red, _("Out of Sunlight     -%s%d%%"),
                  (pen < 10 ? " " : ""), pen);
        line++;
    }
    if (has_trait ("COLDBLOOD4") && g->get_temperature() > 65) {
        pen = int( (g->get_temperature() - 65) / 2);
        mvwprintz(w_speed, line, 1, c_green, _("Cold-Blooded        +%s%d%%"),
                  (pen < 10 ? " " : ""), pen);
        line++;
    }
    if ((has_trait("COLDBLOOD") || has_trait("COLDBLOOD2") ||
         has_trait("COLDBLOOD3") || has_trait("COLDBLOOD4")) &&
        g->get_temperature() < 65) {
        if (has_trait("COLDBLOOD3") || has_trait("COLDBLOOD4")) {
            pen = int( (65 - g->get_temperature()) / 2);
        } else if (has_trait("COLDBLOOD2")) {
            pen = int( (65 - g->get_temperature()) / 3);
        } else {
            pen = int( (65 - g->get_temperature()) / 5);
        }
        mvwprintz(w_speed, line, 1, c_red, _("Cold-Blooded        -%s%d%%"),
                  (pen < 10 ? " " : ""), pen);
        line++;
    }

    std::map<std::string, int> speed_effects;
    std::string dis_text = "";
    for( auto &elem : effects ) {
        for( auto &_effect_it : elem.second ) {
            auto &it = _effect_it.second;
            bool reduced = resists_effect(it);
            int move_adjust = it.get_mod("SPEED", reduced);
            if (move_adjust != 0) {
                dis_text = it.get_speed_name();
                speed_effects[dis_text] += move_adjust;
            }
        }
    }

    for( auto &speed_effect : speed_effects ) {
        nc_color col = ( speed_effect.second > 0 ? c_green : c_red );
        mvwprintz( w_speed, line, 1, col, "%s", _(speed_effect.first.c_str()) );
        mvwprintz( w_speed, line, 21, col, ( speed_effect.second > 0 ? "+" : "-" ) );
        mvwprintz( w_speed, line, ( abs( speed_effect.second ) >= 10 ? 22 : 23 ), col, "%d%%",
                   abs( speed_effect.second ) );
        line++;
    }

    int quick_bonus = int(newmoves - (newmoves / 1.1));
    int bio_speed_bonus = quick_bonus;
    if (has_trait("QUICK") && has_bionic("bio_speed")) {
        bio_speed_bonus = int(newmoves/1.1 - (newmoves / 1.1 / 1.1));
        std::swap(quick_bonus, bio_speed_bonus);
    }
    if (has_trait("QUICK")) {
        mvwprintz(w_speed, line, 1, c_green, _("Quick               +%s%d%%"),
                  (quick_bonus < 10 ? " " : ""), quick_bonus);
        line++;
    }
    if (has_bionic("bio_speed")) {
        mvwprintz(w_speed, line, 1, c_green, _("Bionic Speed        +%s%d%%"),
                  (bio_speed_bonus < 10 ? " " : ""), bio_speed_bonus);
    }

    int runcost = run_cost(100);
    nc_color col = (runcost <= 100 ? c_green : c_red);
    mvwprintz(w_speed, 1, (runcost  >= 100 ? 21 : (runcost  < 10 ? 23 : 22)), col,
              "%d", runcost);
    col = (newmoves >= 100 ? c_green : c_red);
    mvwprintz(w_speed, 2, (newmoves >= 100 ? 21 : (newmoves < 10 ? 23 : 22)), col,
              "%d", newmoves);
    wrefresh(w_speed);

    refresh();

    int curtab = 1;
    unsigned min, max;
    line = 0;
    bool done = false;
    unsigned half_y = 0;

    // Initial printing is DONE.  Now we give the player a chance to scroll around
    // and "hover" over different items for more info.
    do {
        werase(w_info);
        switch (curtab) {
            case 1: // Stats tab
                mvwprintz(w_stats, 0, 0, h_ltgray, header_spaces.c_str());
                mvwprintz(w_stats, 0, 13 - utf8_width(title_STATS)/2, h_ltgray, title_STATS);

                // Clear bonus/penalty menu.
                mvwprintz(w_stats, 6, 0, c_ltgray, "%26s", "");
                mvwprintz(w_stats, 7, 0, c_ltgray, "%26s", "");
                mvwprintz(w_stats, 8, 0, c_ltgray, "%26s", "");

                if (line == 0) {
                    // Display player current strength effects
                    mvwprintz(w_stats, 2, 1, h_ltgray, _("Strength:"));
                    mvwprintz(w_stats, 6, 1, c_magenta, _("Base HP:"));
                    mvwprintz(w_stats, 6, 22, c_magenta, "%3d", hp_max[1]);
                    if (OPTIONS["USE_METRIC_WEIGHTS"] == "kg") {
                        mvwprintz(w_stats, 7, 1, c_magenta, _("Carry weight(kg):"));
                    } else {
                        mvwprintz(w_stats, 7, 1, c_magenta, _("Carry weight(lbs):"));
                    }
                    mvwprintz(w_stats, 7, 21, c_magenta, "%4.1f", convert_weight(weight_capacity()));
                    mvwprintz(w_stats, 8, 1, c_magenta, _("Melee damage:"));
                    mvwprintz(w_stats, 8, 22, c_magenta, "%3d", base_damage(false));

                    fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta,
                     _("Strength affects your melee damage, the amount of weight you can carry, your total HP, "
                     "your resistance to many diseases, and the effectiveness of actions which require brute force."));
                } else if (line == 1) {
                    // Display player current dexterity effects
                    mvwprintz(w_stats, 3, 1, h_ltgray, _("Dexterity:"));

                    mvwprintz(w_stats, 6, 1, c_magenta, _("Melee to-hit bonus:"));
                    mvwprintz(w_stats, 6, 22, c_magenta, "%+3d", get_hit_base());
                    mvwprintz(w_stats, 7, 1, c_magenta, _("Ranged penalty:"));
                    mvwprintz(w_stats, 7, 21, c_magenta, "%+4d", -(abs(ranged_dex_mod())));
                    if (throw_dex_mod(false) <= 0) {
                        mvwprintz(w_stats, 8, 1, c_magenta, _("Throwing bonus:"));
                    } else {
                        mvwprintz(w_stats, 8, 1, c_magenta, _("Throwing penalty:"));
                    }
                    mvwprintz(w_stats, 8, 22, c_magenta, "%+3d", -(throw_dex_mod(false)));

                    fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta,
                     _("Dexterity affects your chance to hit in melee combat, helps you steady your "
                     "gun for ranged combat, and enhances many actions that require finesse."));
                } else if (line == 2) {
                    // Display player current intelligence effects
                    mvwprintz(w_stats, 4, 1, h_ltgray, _("Intelligence:"));
                    mvwprintz(w_stats, 6, 1, c_magenta, _("Read times:"));
                    mvwprintz(w_stats, 6, 21, c_magenta, "%3d%%", read_speed(false));
                    mvwprintz(w_stats, 7, 1, c_magenta, _("Skill rust:"));
                    mvwprintz(w_stats, 7, 22, c_magenta, "%2d%%", rust_rate(false));
                    mvwprintz(w_stats, 8, 1, c_magenta, _("Crafting Bonus:"));
                    mvwprintz(w_stats, 8, 22, c_magenta, "%2d%%", get_int());

                    fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta,
                     _("Intelligence is less important in most situations, but it is vital for more complex tasks like "
                     "electronics crafting.  It also affects how much skill you can pick up from reading a book."));
                } else if (line == 3) {
                    // Display player current perception effects
                    mvwprintz(w_stats, 5, 1, h_ltgray, _("Perception:"));
                    mvwprintz(w_stats, 6, 1,  c_magenta, _("Ranged penalty:"));
                    mvwprintz(w_stats, 6, 21, c_magenta, "%+4d", -(abs(ranged_per_mod())));
                    mvwprintz(w_stats, 7, 1, c_magenta, _("Trap detection level:"));
                    mvwprintz(w_stats, 7, 23, c_magenta, "%2d", get_per());

                    fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta,
                     _("Perception is the most important stat for ranged combat.  It's also used for "
                     "detecting traps and other things of interest."));
                }
                wrefresh(w_stats);
                wrefresh(w_info);

                action = ctxt.handle_input();
                if (action == "DOWN") {
                    line++;
                    if (line == 4)
                        line = 0;
                } else if (action == "UP") {
                    if (line == 0) {
                        line = 3;
                    } else {
                        line--;
                    }
                } else if (action == "NEXT_TAB") {
                    mvwprintz(w_stats, 0, 0, c_ltgray, header_spaces.c_str());
                    mvwprintz(w_stats, 0, 13 - utf8_width(title_STATS)/2, c_ltgray, title_STATS);
                    wrefresh(w_stats);
                    line = 0;
                    curtab++;
                } else if (action == "QUIT") {
                    done = true;
                }
            mvwprintz(w_stats, 2, 1, c_ltgray, _("Strength:"));
            mvwprintz(w_stats, 3, 1, c_ltgray, _("Dexterity:"));
            mvwprintz(w_stats, 4, 1, c_ltgray, _("Intelligence:"));
            mvwprintz(w_stats, 5, 1, c_ltgray, _("Perception:"));
            wrefresh(w_stats);
            break;
        case 2: // Encumberment tab
        {
            werase(w_encumb);
            mvwprintz(w_encumb, 0, 0, h_ltgray, header_spaces.c_str());
            mvwprintz(w_encumb, 0, 13 - utf8_width(title_ENCUMB)/2, h_ltgray, title_ENCUMB);
            int encumb_win_size_y = 8;
            half_y = encumb_win_size_y / 2;
            if (line <= half_y) {
                min = 0;
                max = encumb_win_size_y;
            } else if (line >= 12 - half_y) {
                min = (12 - encumb_win_size_y);
                max = 12;
            } else {
                min = line - half_y;
                max = line - half_y + encumb_win_size_y;
            }

            print_encumbrance(w_encumb, min, max, line);
            draw_scrollbar(w_encumb, line, encumb_win_size_y, 12, 1);
            wrefresh(w_encumb);

            werase(w_info);
            std::string s;
            if (line == 0) {
                const int melee_roll_pen = std::max( -( encumb( bp_torso ) / 10 ) * 10, -80 );
                s += string_format( _("Melee attack rolls %+d%%; "), melee_roll_pen );
                s += dodge_skill_text( - (encumb( bp_torso ) / 10));
                s += swim_cost_text( (encumb( bp_torso ) / 10) * ( 80 - get_skill_level( skill_swimming ) * 3 ) );
                s += melee_cost_text( encumb( bp_torso ) );
            } else if (line == 1) { //Torso
                s += _("Head encumbrance has no effect; it simply limits how much you can put on.");
            } else if (line == 2) { //Head
                s += string_format(_("Perception %+d when checking traps or firing ranged weapons;\n"
                                     "Perception %+.1f when throwing items."),
                                   -(encumb(bp_eyes) / 10),
                                   double(double(-(encumb(bp_eyes) / 10)) / 2));
            } else if (line == 3) { //Eyes
                s += _("Covering your mouth will make it more difficult to breathe and catch your breath.");
            } else if (line == 4) { //Left Arm
                s += _("Arm encumbrance affects stamina cost of melee attacks and accuracy with ranged weapons.");
            } else if (line == 5) { //Right Arm
                s += _("Arm encumbrance affects stamina cost of melee attacks and accuracy with ranged weapons.");
            } else if (line == 6) { //Left Hand
                s += reload_cost_text( (encumb( bp_hand_l ) / 10) * 15 );
                s += string_format( _("Dexterity %+d when throwing items;\n"), -(encumb( bp_hand_l )/10) );
                s += melee_cost_text( encumb( bp_hand_l ) / 2 );
            } else if (line == 7) { //Right Hand
                s += reload_cost_text( (encumb( bp_hand_r ) / 10) * 15 );
                s += string_format( _("Dexterity %+d when throwing items;\n"), -(encumb( bp_hand_r )/10) );
                s += melee_cost_text( encumb( bp_hand_r ) / 2 );
            } else if (line == 8) { //Left Leg
                s += run_cost_text( encumb( bp_leg_l ) * 0.15 );
                s += swim_cost_text( (encumb( bp_leg_l ) / 10) * ( 50 - get_skill_level( skill_swimming ) * 2 ) / 2 );
                s += dodge_skill_text( -(encumb( bp_leg_l ) / 10) / 4.0 );
            } else if (line == 9) { //Right Leg
                s += run_cost_text( encumb( bp_leg_r ) * 0.15 );
                s += swim_cost_text( (encumb( bp_leg_r ) / 10) * ( 50 - get_skill_level( skill_swimming ) * 2 ) / 2 );
                s += dodge_skill_text( -(encumb( bp_leg_r ) / 10) / 4.0 );
            } else if (line == 10) { //Left Foot
                s += run_cost_text( encumb( bp_foot_l ) * 0.25 );
            } else if (line == 11) { //Right Foot
                s += run_cost_text( encumb( bp_foot_r ) * 0.25 );
            }
            fold_and_print( w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta, s );
            wrefresh(w_info);


            action = ctxt.handle_input();
            if (action == "DOWN") {
                if (line < 11) {
                    line++;
                }
            } else if (action == "UP") {
                if (line > 0) {
                    line--;
                }
            } else if (action == "NEXT_TAB") {
                mvwprintz(w_encumb, 0, 0, c_ltgray, header_spaces.c_str());
                mvwprintz(w_encumb, 0, 13 - utf8_width(title_ENCUMB)/2, c_ltgray, title_ENCUMB);
                wrefresh(w_encumb);
                line = 0;
                curtab++;
            } else if (action == "QUIT") {
                done = true;
            }
            break;
        }
        case 4: // Traits tab
            mvwprintz(w_traits, 0, 0, h_ltgray, header_spaces.c_str());
            mvwprintz(w_traits, 0, 13 - utf8_width(title_TRAITS)/2, h_ltgray, title_TRAITS);
            if (line <= (trait_win_size_y-1)/2) {
                min = 0;
                max = trait_win_size_y;
                if (traitslist.size() < max)
                    max = traitslist.size();
            } else if (line >= traitslist.size() - (trait_win_size_y+1)/2) {
                min = (traitslist.size() < trait_win_size_y ? 0 : traitslist.size() - trait_win_size_y);
                max = traitslist.size();
            } else {
                min = line - (trait_win_size_y-1)/2;
                max = line + (trait_win_size_y+1)/2;
                if (traitslist.size() < max)
                    max = traitslist.size();
            }

            for (unsigned i = min; i < max; i++) {
                const auto &mdata = mutation_branch::get( traitslist[i] );
                mvwprintz(w_traits, 1 + i - min, 1, c_ltgray, "                         ");
                const auto color = mdata.get_display_color();
                if (i == line) {
                    mvwprintz(w_traits, 1 + i - min, 1, hilite(color), "%s",
                              mdata.name.c_str());
                } else {
                    mvwprintz(w_traits, 1 + i - min, 1, color, "%s",
                              mdata.name.c_str());
                }
            }
            if (line < traitslist.size()) {
                const auto &mdata = mutation_branch::get( traitslist[line] );
                fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH-2, c_magenta,
                               mdata.description);
            }
            wrefresh(w_traits);
            wrefresh(w_info);

            action = ctxt.handle_input();
            if (action == "DOWN") {
                if (line < traitslist.size() - 1)
                    line++;
                break;
            } else if (action == "UP") {
                if (line > 0)
                    line--;
            } else if (action == "NEXT_TAB") {
                mvwprintz(w_traits, 0, 0, c_ltgray, header_spaces.c_str());
                mvwprintz(w_traits, 0, 13 - utf8_width(title_TRAITS)/2, c_ltgray, title_TRAITS);
                for (size_t i = 0; i < traitslist.size() && i < trait_win_size_y; i++) {
                    const auto &mdata = mutation_branch::get( traitslist[i] );
                    mvwprintz(w_traits, i + 1, 1, c_black, "                         ");
                    const auto color = mdata.get_display_color();
                    mvwprintz(w_traits, i + 1, 1, color, "%s", mdata.name.c_str());
                }
                wrefresh(w_traits);
                line = 0;
                curtab++;
            } else if (action == "QUIT") {
                done = true;
            }
            break;

        case 5: // Effects tab
            mvwprintz(w_effects, 0, 0, h_ltgray, header_spaces.c_str());
            mvwprintz(w_effects, 0, 13 - utf8_width(title_EFFECTS)/2, h_ltgray, title_EFFECTS);
            half_y = effect_win_size_y / 2;
            if (line <= half_y) {
                min = 0;
                max = effect_win_size_y;
            if (effect_name.size() < max)
                max = effect_name.size();
            } else if (line >= effect_name.size() - half_y) {
                min = (effect_name.size() < effect_win_size_y ? 0 : effect_name.size() - effect_win_size_y);
                max = effect_name.size();
            } else {
                min = line - half_y;
                max = line - half_y + effect_win_size_y;
                if (effect_name.size() < max)
                    max = effect_name.size();
            }

            for (unsigned i = min; i < max; i++) {
                if (i == line)
                    mvwprintz(w_effects, 1 + i - min, 0, h_ltgray, "%s", effect_name[i].c_str());
                else
                    mvwprintz(w_effects, 1 + i - min, 0, c_ltgray, "%s", effect_name[i].c_str());
            }
            if (line < effect_text.size()) {
                fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH-2, c_magenta, effect_text[line]);
            }
            wrefresh(w_effects);
            wrefresh(w_info);

            action = ctxt.handle_input();
            if (action == "DOWN") {
                if (line < effect_name.size() - 1)
                    line++;
                break;
            } else if (action == "UP") {
                if (line > 0)
                    line--;
            } else if (action == "NEXT_TAB") {
                mvwprintz(w_effects, 0, 0, c_ltgray, header_spaces.c_str());
                mvwprintz(w_effects, 0, 13 - utf8_width(title_EFFECTS)/2, c_ltgray, title_EFFECTS);
                for (size_t i = 0; i < effect_name.size() && i < 7; i++) {
                    mvwprintz(w_effects, i + 1, 0, c_ltgray, "%s", effect_name[i].c_str());
                }
                wrefresh(w_effects);
                line = 0;
                curtab = 1;
            } else if (action == "QUIT") {
                done = true;
            }
            break;

        case 3: // Skills tab
            mvwprintz(w_skills, 0, 0, h_ltgray, header_spaces.c_str());
            mvwprintz(w_skills, 0, 13 - utf8_width(title_SKILLS)/2, h_ltgray, title_SKILLS);
            half_y = skill_win_size_y / 2;
            if (line <= half_y) {
                min = 0;
                max = skill_win_size_y;
                if (skillslist.size() < max)
                    max = skillslist.size();
            } else if (line >= skillslist.size() - half_y) {
                min = (skillslist.size() < size_t(skill_win_size_y) ? 0 : skillslist.size() - skill_win_size_y);
                max = skillslist.size();
            } else {
                min = line - half_y;
                max = line - half_y + skill_win_size_y;
                if (skillslist.size() < max)
                    max = skillslist.size();
            }

            const Skill* selectedSkill = NULL;

            for (unsigned i = min; i < max; i++) {
                const Skill* aSkill = skillslist[i];
                SkillLevel level = get_skill_level(aSkill);

                const bool can_train = level.can_train();
                const bool training = level.isTraining();
                const bool rusting = level.isRusting();
                const int exercise = level.exercise();

                if (i == line) {
                    selectedSkill = aSkill;
                    if( !can_train ) {
                        status = rusting ? h_ltred : h_white;
                    } else if( exercise >= 100 ) {
                        status = training ? h_pink : h_magenta;
                    } else if( rusting ) {
                        status = training ? h_ltred : h_red;
                    } else {
                        status = training ? h_ltblue : h_blue;
                    }
                } else {
                    if( rusting ) {
                        status = training ? c_ltred : c_red;
                    } else if( !can_train ) {
                        status = c_white;
                    } else {
                        status = training ? c_ltblue : c_blue;
                    }
                }
                mvwprintz(w_skills, 1 + i - min, 1, c_ltgray, "                         ");
                mvwprintz(w_skills, 1 + i - min, 1, status, "%s:", aSkill->name().c_str());
                mvwprintz(w_skills, 1 + i - min,19, status, "%-2d(%2d%%)", (int)level, (exercise <  0 ? 0 : exercise));
            }

            draw_scrollbar(w_skills, line, skill_win_size_y, skillslist.size(), 1);
            wrefresh(w_skills);

            werase(w_info);

            if (line < skillslist.size()) {
                fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH-2, c_magenta, selectedSkill->description());
            }
            wrefresh(w_info);

            action = ctxt.handle_input();
            if (action == "DOWN") {
                if (size_t(line) < skillslist.size() - 1)
                    line++;
            } else if (action == "UP") {
                if (line > 0)
                    line--;
            } else if (action == "NEXT_TAB") {
                werase(w_skills);
                mvwprintz(w_skills, 0, 0, c_ltgray, header_spaces.c_str());
                mvwprintz(w_skills, 0, 13 - utf8_width(title_SKILLS)/2, c_ltgray, title_SKILLS);
                for (size_t i = 0; i < skillslist.size() && i < size_t(skill_win_size_y); i++) {
                    const Skill* thisSkill = skillslist[i];
                    SkillLevel level = get_skill_level(thisSkill);
                    bool can_train = level.can_train();
                    bool isLearning = level.isTraining();
                    bool rusting = level.isRusting();

                    if( rusting ) {
                        status = isLearning ? c_ltred : c_red;
                    } else if( !can_train ) {
                        status = c_white;
                    } else {
                        status = isLearning ? c_ltblue : c_blue;
                    }

                    mvwprintz(w_skills, i + 1,  1, status, "%s:", thisSkill->name().c_str());
                    mvwprintz(w_skills, i + 1, 19, status, "%-2d(%2d%%)", (int)level,
                              (level.exercise() <  0 ? 0 : level.exercise()));
                }
                wrefresh(w_skills);
                line = 0;
                curtab++;
            } else if (action == "CONFIRM") {
                skillLevel(selectedSkill).toggleTraining();
            } else if (action == "QUIT") {
                done = true;
            }
        }
    } while (!done);

    werase(w_info);
    werase(w_tip);
    werase(w_stats);
    werase(w_encumb);
    werase(w_traits);
    werase(w_effects);
    werase(w_skills);
    werase(w_speed);
    werase(w_info);
    werase(w_grid_top);
    werase(w_grid_effect);
    werase(w_grid_skill);
    werase(w_grid_trait);

    delwin(w_info);
    delwin(w_tip);
    delwin(w_stats);
    delwin(w_encumb);
    delwin(w_traits);
    delwin(w_effects);
    delwin(w_skills);
    delwin(w_speed);
    delwin(w_grid_top);
    delwin(w_grid_effect);
    delwin(w_grid_skill);
    delwin(w_grid_trait);

    g->refresh_all();
}

void player::disp_morale()
{
    // Ensure the player's persistent morale effects are up-to-date.
    apply_persistent_morale();

    // Create and draw the window itself.
    WINDOW *w = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                        (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 0,
                        (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0);
    draw_border(w);

    // Figure out how wide the name column needs to be.
    int name_column_width = 18;
    for (auto &i : morale) {
        int length = utf8_width(i.name());
        if ( length > name_column_width) {
            name_column_width = length;
        }
    }

    // If it's too wide, truncate.
    if (name_column_width > 72) {
        name_column_width = 72;
    }

    // Start printing the number right after the name column.
    // We'll right-justify it later.
    int number_pos = name_column_width + 1;

    // Header
    mvwprintz(w, 1,  1, c_white, _("Morale Modifiers:"));
    mvwprintz(w, 2,  1, c_ltgray, _("Name"));
    mvwprintz(w, 2, name_column_width+2, c_ltgray, _("Value"));

    // Print out the morale entries.
    for (size_t i = 0; i < morale.size(); i++)
    {
        std::string name = morale[i].name();
        int bonus = net_morale(morale[i]);

        // Print out the name.
        trim_and_print(w, i + 3,  1, name_column_width, (bonus < 0 ? c_red : c_green), name.c_str());

        // Print out the number, right-justified.
        mvwprintz(w, i + 3, number_pos, (bonus < 0 ? c_red : c_green),
                  "% 6d", bonus);
    }

    // Print out the total morale, right-justified.
    int mor = morale_level();
    mvwprintz(w, 20, 1, (mor < 0 ? c_red : c_green), _("Total:"));
    mvwprintz(w, 20, number_pos, (mor < 0 ? c_red : c_green), "% 6d", mor);

    // Print out the focus gain rate, right-justified.
    double gain = (calc_focus_equilibrium() - focus_pool) / 100.0;
    mvwprintz(w, 22, 1, (gain < 0 ? c_red : c_green), _("Focus gain:"));
    mvwprintz(w, 22, number_pos-3, (gain < 0 ? c_red : c_green), _("%6.2f per minute"), gain);

    // Make sure the changes are shown.
    wrefresh(w);

    // Wait for any keystroke.
    getch();

    // Close the window.
    werase(w);
    delwin(w);
}

int player::print_aim_bars( WINDOW *w, int line_number, item *weapon, Creature *target )
{
    // Window width minus borders.
    const int window_width = getmaxx( w ) - 2;
    // This is absolute accuracy for the player.
    // TODO: push the calculations duplicated from Creature::deal_projectile_attack() and
    // Creature::projectile_attack() into shared methods.
    // Dodge is intentionally not accounted for.
    const double aim_level =
        recoil + driving_recoil + get_weapon_dispersion( weapon, false );
    const double range = rl_dist( pos(), target->pos() );
    const double missed_by = aim_level * 0.00021666666666666666 * range;
    const double hit_rating = missed_by / std::max(double(get_speed()) / 80., 1.0);
    // Confidence is chance of the actual shot being under the target threshold,
    // This simplifies the calculation greatly, that's intentional.
    const std::array<std::pair<double, char>, 3> ratings =
        {{ std::make_pair(0.1, '*'), std::make_pair(0.4, '+'), std::make_pair(0.6, '|') }};
    const std::string confidence_label = _("Confidence: ");
    const int confidence_width = window_width - utf8_width( confidence_label );
    int used_width = 0;
    std::string confidence_meter;
    for( auto threshold : ratings ) {
        const double confidence =
            std::min( 1.0, std::max( 0.0, threshold.first / hit_rating ) );
        const int confidence_meter_width = int(confidence_width * confidence) - used_width;
        used_width += confidence_meter_width;
        confidence_meter += std::string( confidence_meter_width, threshold.second );
    }
    mvwprintw(w, line_number++, 1, "%s%s",
              confidence_label.c_str(), confidence_meter.c_str() );

    // This is a relative measure of how steady the player's aim is,
    // 0 it is the best the player can do.
    const double steady_score = recoil - weapon->sight_dispersion( -1 );
    // Fairly arbitrary cap on steadiness...
    const double steadiness = std::max( 0.0, 1.0 - (steady_score / 250) );
    const std::string steadiness_label = _("Steadiness: ");
    const int steadiness_width = window_width - utf8_width( steadiness_label );
    const std::string steadiness_meter = std::string( steadiness_width * steadiness, '*' );
    mvwprintw(w, line_number++, 1, "%s%s",
              steadiness_label.c_str(), steadiness_meter.c_str() );
    return line_number;
}

std::string player::print_gun_mode() const
{
    // Print current weapon, or attachment if active.
    const item* gunmod = weapon.active_gunmod();
    std::stringstream attachment;
    if (gunmod != NULL) {
        attachment << gunmod->type_name().c_str();
        for( auto &mod : weapon.contents ) {
            if( mod.is_auxiliary_gunmod() ) {
                attachment << " (" << mod.charges << ")";
            }
        }
        return string_format( _("%s (Mod)"), attachment.str().c_str() );
    } else {
        if (weapon.get_gun_mode() == "MODE_BURST") {
            return string_format( _("%s (Burst)"), weapname().c_str() );
        } else if( weapon.get_gun_mode() == "MODE_REACH" ) {
            return string_format( _("%s (Bayonet)"), weapname().c_str() );
        } else {
            return string_format( _("%s"), weapname().c_str() );
        }
    }
}

std::string player::print_recoil() const
{
    if (weapon.is_gun()) {
        const int adj_recoil = recoil + driving_recoil;
        if( adj_recoil > 150 ) {
            // 150 is the minimum when not actively aiming
            const char *color_name = "c_ltgray";
            if( adj_recoil >= 690 ) {
                color_name = "c_red";
            } else if( adj_recoil >= 450 ) {
                color_name = "c_ltred";
            } else if( adj_recoil >= 210 ) {
                color_name = "c_yellow";
            }
            return string_format("<color_%s>%s</color>", color_name, _("Recoil"));
        }
    }
    return std::string();
}

int player::draw_turret_aim( WINDOW *w, int line_number, const tripoint &targ ) const
{
    vehicle *veh = g->m.veh_at( pos() );
    if( veh == nullptr ) {
        debugmsg( "Tried to aim turret while outside vehicle" );
        return line_number;
    }

    const auto turret_state = veh->turrets_can_shoot( targ );
    int num_ok = 0;
    for( const auto &pr : turret_state ) {
        if( pr.second == turret_all_ok ) {
            num_ok++;
        }
    }

    mvwprintw( w, line_number++, 1, _("Turrets in range: %d"), num_ok );
    return line_number;
}

void player::print_stamina_bar( WINDOW *w ) const
{
    std::string sta_bar = "";
    nc_color sta_color;
    std::tie(sta_bar, sta_color) = get_hp_bar( stamina ,  get_stamina_max() );
    wprintz(w, sta_color, sta_bar.c_str());
}

void player::disp_status(WINDOW *w, WINDOW *w2)
{
    bool sideStyle = use_narrow_sidebar();
    WINDOW *weapwin = sideStyle ? w2 : w;

    {
        const int y = sideStyle ? 1 : 0;
        const int w = getmaxx( weapwin );
        trim_and_print( weapwin, y, 0, w, c_ltgray, "%s", print_gun_mode().c_str() );
    }

    // Print currently used style or weapon mode.
    std::string style = "";
    if (is_armed()) {
        // Show normal if no martial style is selected,
        // or if the currently selected style does nothing for your weapon.
        if (style_selected == matype_id( "style_none" ) ||
            (!can_melee() && !style_selected.obj().has_weapon(weapon.type->id))) {
            style = _("Normal");
        } else {
            style = style_selected.obj().name;
        }

        int x = sideStyle ? (getmaxx(weapwin) - 13) : 0;
        mvwprintz(weapwin, 1, x, c_red, style.c_str());
    } else {
        if (style_selected == matype_id( "style_none" ) ) {
            style = _("No Style");
        } else {
            style = style_selected.obj().name;
        }
        if (style != "") {
            int x = sideStyle ? (getmaxx(weapwin) - 13) : 0;
            mvwprintz(weapwin, 1, x, c_blue, style.c_str());
        }
    }

    wmove(w, sideStyle ? 1 : 2, 0);
    if (get_hunger() > 2800)
        wprintz(w, c_red,    _("Starving!"));
    else if (get_hunger() > 1400)
        wprintz(w, c_ltred,  _("Near starving"));
    else if (get_hunger() > 300)
        wprintz(w, c_ltred,  _("Famished"));
    else if (get_hunger() > 100)
        wprintz(w, c_yellow, _("Very hungry"));
    else if (get_hunger() > 40)
        wprintz(w, c_yellow, _("Hungry"));
    else if (get_hunger() < 0)
        wprintz(w, c_green,  _("Full"));
    else if (get_hunger() < -20)
        wprintz(w, c_green,  _("Sated"));
    else if (get_hunger() < -60)
        wprintz(w, c_green,  _("Engorged"));

    /// Find hottest/coldest bodypart
    // Calculate the most extreme body tempearatures
    int current_bp_extreme = 0, conv_bp_extreme = 0;
    for (int i = 0; i < num_bp ; i++ ){
        if (abs(temp_cur[i] - BODYTEMP_NORM) > abs(temp_cur[current_bp_extreme] - BODYTEMP_NORM)) current_bp_extreme = i;
        if (abs(temp_conv[i] - BODYTEMP_NORM) > abs(temp_conv[conv_bp_extreme] - BODYTEMP_NORM)) conv_bp_extreme = i;
    }

    // Assign zones for comparisons
    int cur_zone = 0, conv_zone = 0;
    if      (temp_cur[current_bp_extreme] >  BODYTEMP_SCORCHING) cur_zone = 7;
    else if (temp_cur[current_bp_extreme] >  BODYTEMP_VERY_HOT)  cur_zone = 6;
    else if (temp_cur[current_bp_extreme] >  BODYTEMP_HOT)       cur_zone = 5;
    else if (temp_cur[current_bp_extreme] >  BODYTEMP_COLD)      cur_zone = 4;
    else if (temp_cur[current_bp_extreme] >  BODYTEMP_VERY_COLD) cur_zone = 3;
    else if (temp_cur[current_bp_extreme] >  BODYTEMP_FREEZING)  cur_zone = 2;
    else if (temp_cur[current_bp_extreme] <= BODYTEMP_FREEZING)  cur_zone = 1;

    if      (temp_conv[conv_bp_extreme] >  BODYTEMP_SCORCHING) conv_zone = 7;
    else if (temp_conv[conv_bp_extreme] >  BODYTEMP_VERY_HOT)  conv_zone = 6;
    else if (temp_conv[conv_bp_extreme] >  BODYTEMP_HOT)       conv_zone = 5;
    else if (temp_conv[conv_bp_extreme] >  BODYTEMP_COLD)      conv_zone = 4;
    else if (temp_conv[conv_bp_extreme] >  BODYTEMP_VERY_COLD) conv_zone = 3;
    else if (temp_conv[conv_bp_extreme] >  BODYTEMP_FREEZING)  conv_zone = 2;
    else if (temp_conv[conv_bp_extreme] <= BODYTEMP_FREEZING)  conv_zone = 1;

    // delta will be positive if temp_cur is rising
    int delta = conv_zone - cur_zone;
    // Decide if temp_cur is rising or falling
    const char *temp_message = "Error";
    if      (delta >   2) temp_message = _(" (Rising!!)");
    else if (delta ==  2) temp_message = _(" (Rising!)");
    else if (delta ==  1) temp_message = _(" (Rising)");
    else if (delta ==  0) temp_message = "";
    else if (delta == -1) temp_message = _(" (Falling)");
    else if (delta == -2) temp_message = _(" (Falling!)");
    else if (delta <  -2) temp_message = _(" (Falling!!)");

    // printCur the hottest/coldest bodypart, and if it is rising or falling in temperature
    wmove(w, sideStyle ? 6 : 1, sideStyle ? 0 : 9);
    if      (temp_cur[current_bp_extreme] >  BODYTEMP_SCORCHING)
        wprintz(w, c_red,   _("Scorching!%s"), temp_message);
    else if (temp_cur[current_bp_extreme] >  BODYTEMP_VERY_HOT)
        wprintz(w, c_ltred, _("Very hot!%s"), temp_message);
    else if (temp_cur[current_bp_extreme] >  BODYTEMP_HOT)
        wprintz(w, c_yellow,_("Warm%s"), temp_message);
    else if (temp_cur[current_bp_extreme] >  BODYTEMP_COLD) // If you're warmer than cold, you are comfortable
        wprintz(w, c_green, _("Comfortable%s"), temp_message);
    else if (temp_cur[current_bp_extreme] >  BODYTEMP_VERY_COLD)
        wprintz(w, c_ltblue,_("Chilly%s"), temp_message);
    else if (temp_cur[current_bp_extreme] >  BODYTEMP_FREEZING)
        wprintz(w, c_cyan,  _("Very cold!%s"), temp_message);
    else if (temp_cur[current_bp_extreme] <= BODYTEMP_FREEZING)
        wprintz(w, c_blue,  _("Freezing!%s"), temp_message);

    int x = 32;
    int y = sideStyle ?  0 :  1;
    if(is_deaf()) {
        mvwprintz(sideStyle ? w2 : w, y, x, c_red, _("Deaf!"), volume);
    } else {
        mvwprintz(sideStyle ? w2 : w, y, x, c_yellow, _("Sound %d"), volume);
    }
    volume = 0;

    wmove(w, 2, sideStyle ? 0 : 15);
    if (thirst > 520)
        wprintz(w, c_ltred,  _("Parched"));
    else if (thirst > 240)
        wprintz(w, c_ltred,  _("Dehydrated"));
    else if (thirst > 80)
        wprintz(w, c_yellow, _("Very thirsty"));
    else if (thirst > 40)
        wprintz(w, c_yellow, _("Thirsty"));
    else if (thirst < 0)
        wprintz(w, c_green,  _("Slaked"));
    else if (thirst < -20)
        wprintz(w, c_green,  _("Hydrated"));
    else if (thirst < -60)
        wprintz(w, c_green,  _("Turgid"));

    wmove(w, sideStyle ? 3 : 2, sideStyle ? 0 : 30);
    if (fatigue > EXHAUSTED)
        wprintz(w, c_red,    _("Exhausted"));
    else if (fatigue > DEAD_TIRED)
        wprintz(w, c_ltred,  _("Dead tired"));
    else if (fatigue > TIRED)
        wprintz(w, c_yellow, _("Tired"));

    wmove(w, sideStyle ? 4 : 2, sideStyle ? 0 : 41);
    wprintz(w, c_white, _("Focus"));
    nc_color col_xp = c_dkgray;
    if (focus_pool >= 100)
        col_xp = c_white;
    else if (focus_pool >  0)
        col_xp = c_ltgray;
    wprintz(w, col_xp, " %d", focus_pool);

    nc_color col_pain = c_yellow;
    if (pain - pkill >= 60)
        col_pain = c_red;
    else if (pain - pkill >= 40)
        col_pain = c_ltred;
    if (pain - pkill > 0)
        mvwprintz(w, sideStyle ? 0 : 3, 0, col_pain, _("Pain %d"), pain - pkill);

    int morale_cur = morale_level ();
    nc_color col_morale = c_white;
    if (morale_cur >= 10)
        col_morale = c_green;
    else if (morale_cur <= -10)
        col_morale = c_red;
    const char *morale_str;
    if      (morale_cur >= 200) morale_str = "8D";
    else if (morale_cur >= 100) morale_str = ":D";
    else if (has_trait("THRESH_FELINE") && morale_cur >= 10)  morale_str = ":3";
    else if (!has_trait("THRESH_FELINE") && morale_cur >= 10)  morale_str = ":)";
    else if (morale_cur > -10)  morale_str = ":|";
    else if (morale_cur > -100) morale_str = "):";
    else if (morale_cur > -200) morale_str = "D:";
    else                        morale_str = "D8";
    mvwprintz(w, sideStyle ? 0 : 3, sideStyle ? 11 : 9, col_morale, morale_str);

    vehicle *veh = g->remoteveh();
    if( veh == nullptr && in_vehicle ) {
        veh = g->m.veh_at( pos() );
    }
    if( veh ) {
  veh->print_fuel_indicators(w, sideStyle ? 2 : 3, sideStyle ? getmaxx(w) - 5 : 49);
  nc_color col_indf1 = c_ltgray;

  float strain = veh->strain();
  nc_color col_vel = strain <= 0? c_ltblue :
                     (strain <= 0.2? c_yellow :
                     (strain <= 0.4? c_ltred : c_red));

    bool has_turrets = false;
    for (size_t p = 0; p < veh->parts.size(); p++) {
        if (veh->part_flag (p, "TURRET")) {
            has_turrets = true;
            break;
        }
    }

  if (has_turrets) {
   mvwprintz(w, 3, sideStyle ? 16 : 25, col_indf1, "Gun:");
   wprintz(w, veh->turret_mode ? c_ltred : c_ltblue,
              veh->turret_mode ? "auto" : "off ");
  }

  //
  // Draw the speedometer.
  //

  int speedox = sideStyle ? 0 : 33;
  int speedoy = sideStyle ? 5 :  3;

  bool metric = OPTIONS["USE_METRIC_SPEEDS"] == "km/h";
  const char *units = metric ? _("km/h") : _("mph");
  int velx    = metric ? 4 : 3; // strlen(units) + 1
  int cruisex = metric ? 9 : 8; // strlen(units) + 6
  float conv  = metric ? 0.0161f : 0.01f;

  if (0 == sideStyle) {
    if (!veh->cruise_on) speedox += 2;
    if (!metric)         speedox++;
  }

  const char *speedo = veh->cruise_on ? "%s....>...." : "%s....";
  mvwprintz(w, speedoy, speedox,        col_indf1, speedo, units);
  mvwprintz(w, speedoy, speedox + velx, col_vel,   "%4d", int(veh->velocity * conv));
  if (veh->cruise_on)
    mvwprintz(w, speedoy, speedox + cruisex, c_ltgreen, "%4d", int(veh->cruise_velocity * conv));

  if (veh->velocity != 0) {
   const int offset_from_screen_edge = sideStyle ? 13 : 8;
   nc_color col_indc = veh->skidding? c_red : c_green;
   int dfm = veh->face.dir() - veh->move.dir();
   wmove(w, sideStyle ? 4 : 3, getmaxx(w) - offset_from_screen_edge);
   if (dfm == 0)
     wprintz(w, col_indc, "^");
   else if (dfm < 0)
     wprintz(w, col_indc, "<");
   else
     wprintz(w, col_indc, ">");
  }

  if( sideStyle ) {
      // Make sure this is left-aligned.
      mvwprintz(w, speedoy, getmaxx(w) - 9, c_white, "%s", _("Stm "));
      print_stamina_bar(w);
  }
 } else {  // Not in vehicle
  nc_color col_str = c_white, col_dex = c_white, col_int = c_white,
           col_per = c_white, col_spd = c_white, col_time = c_white;
  int str_bonus = get_str_bonus();
  int dex_bonus = get_dex_bonus();
  int int_bonus = get_int_bonus();
  int per_bonus = get_per_bonus();
  int spd_bonus = get_speed_bonus();
  if (str_bonus < 0)
   col_str = c_red;
  if (str_bonus > 0)
   col_str = c_green;
  if (dex_bonus  < 0)
   col_dex = c_red;
  if (dex_bonus  > 0)
   col_dex = c_green;
  if (int_bonus  < 0)
   col_int = c_red;
  if (int_bonus  > 0)
   col_int = c_green;
  if (per_bonus  < 0)
   col_per = c_red;
  if (per_bonus  > 0)
   col_per = c_green;
  if (spd_bonus < 0)
   col_spd = c_red;
  if (spd_bonus > 0)
   col_spd = c_green;

    int x  = sideStyle ? 18 : 12;
    int y  = sideStyle ?  0 :  3;
    int dx = sideStyle ?  0 :  7;
    int dy = sideStyle ?  1 :  0;
    mvwprintz( w, y + dy * 0, x + dx * 0, col_str, _("Str %d"), str_cur );
    mvwprintz( w, y + dy * 1, x + dx * 1, col_dex, _("Dex %d"), dex_cur );
    mvwprintz( w, y + dy * 2, x + dx * 2, col_int, _("Int %d"), int_cur );
    mvwprintz( w, y + dy * 3, x + dx * 3, col_per, _("Per %d"), per_cur );

    int spdx = sideStyle ?  0 : x + dx * 4 + 1;
    int spdy = sideStyle ?  5 : y + dy * 4;
    mvwprintz(w, spdy, spdx, col_spd, _("Spd %d"), get_speed());
    if (this->weight_carried() > this->weight_capacity()) {
        col_time = h_black;
    }
    if( this->volume_carried() > this->volume_capacity() ) {
        if( this->weight_carried() > this->weight_capacity() ) {
            col_time = c_dkgray_magenta;
        } else {
            col_time = c_dkgray_red;
        }
    }
    wprintz(w, col_time, " %d", movecounter);

    //~ Movement type: "walking". Max string length: one letter.
    const auto str_walk = pgettext( "movement-type", "W" );
    //~ Movement type: "running". Max string length: one letter.
    const auto str_run = pgettext( "movement-type", "R" );
    wprintz(w, c_white, " %s", move_mode == "walk" ? str_walk : str_run);
    if( sideStyle ) {
        mvwprintz(w, spdy, x + dx * 4 - 3, c_white, _("Stm "));
        print_stamina_bar(w);
    }
 }
}

bool player::has_conflicting_trait(const std::string &flag) const
{
    return (has_opposite_trait(flag) || has_lower_trait(flag) || has_higher_trait(flag));
}

bool player::has_opposite_trait(const std::string &flag) const
{
        for (auto &i : mutation_branch::get( flag ).cancels) {
            if (has_trait(i)) {
                return true;
            }
        }
    return false;
}

bool player::has_lower_trait(const std::string &flag) const
{
        for (auto &i : mutation_branch::get( flag ).prereqs) {
            if (has_trait(i) || has_lower_trait(i)) {
                return true;
            }
        }
    return false;
}

bool player::has_higher_trait(const std::string &flag) const
{
        for (auto &i : mutation_branch::get( flag ).replacements) {
            if (has_trait(i) || has_higher_trait(i)) {
                return true;
            }
        }
    return false;
}

bool player::crossed_threshold() const
{
    for( auto &mut : my_mutations ) {
        if( mutation_branch::get( mut.first ).threshold ) {
            return true;
        }
    }
    return false;
}

bool player::purifiable(const std::string &flag) const
{
    if(mutation_branch::get( flag ).purifiable) {
        return true;
    }
    return false;
}

void player::set_cat_level_rec(const std::string &sMut)
{
    if (!has_base_trait(sMut)) { //Skip base traits
        const auto &mdata = mutation_branch::get( sMut );
        for( auto &elem : mdata.category ) {
            mutation_category_level[elem] += 8;
        }

        for (auto &i : mdata.prereqs) {
            set_cat_level_rec(i);
        }

        for (auto &i : mdata.prereqs2) {
            set_cat_level_rec(i);
        }
    }
}

void player::set_highest_cat_level()
{
    mutation_category_level.clear();

    // Loop through our mutations
    for( auto &mut : my_mutations ) {
        set_cat_level_rec( mut.first );
    }
}

std::string player::get_highest_category() const // Returns the mutation category with the highest strength
{
    int iLevel = 0;
    std::string sMaxCat = "";

    for( const auto &elem : mutation_category_level ) {
        if( elem.second > iLevel ) {
            sMaxCat = elem.first;
            iLevel = elem.second;
        } else if( elem.second == iLevel ) {
            sMaxCat = "";  // no category on ties
        }
    }
    return sMaxCat;
}

std::string player::get_category_dream(const std::string &cat, int strength) const // Returns a randomly selected dream
{
    std::string message;
    std::vector<dream> valid_dreams;
    //Pull the list of dreams
    for (auto &i : dreams) {
        //Pick only the ones matching our desired category and strength
        if ((i.category == cat) && (i.strength == strength)) {
            // Put the valid ones into our list
            valid_dreams.push_back(i);
        }
    }
    if( valid_dreams.empty() ) {
        return "";
    }
    const dream& selected_dream = random_entry( valid_dreams );
    return random_entry( selected_dream.messages );
}

bool player::in_climate_control()
{
    bool regulated_area=false;
    // Check
    if( has_active_bionic("bio_climate") ) {
        return true;
    }
    for( auto &w : worn ) {
        if( w.typeId() == "rm13_armor_on" ) {
            return true;
        }
        if( w.active && w.is_power_armor() ) {
            return true;
        }
    }
    if( int(calendar::turn) >= next_climate_control_check ) {
        // save cpu and simulate acclimation.
        next_climate_control_check = int(calendar::turn) + 20;
        int vpart = -1;
        vehicle *veh = g->m.veh_at( pos(), vpart );
        if(veh) {
            regulated_area = (
                veh->is_inside(vpart) &&    // Already checks for opened doors
                veh->total_power(true) > 0  // Out of gas? No AC for you!
            );  // TODO: (?) Force player to scrounge together an AC unit
        }
        // TODO: AC check for when building power is implemented
        last_climate_control_ret = regulated_area;
        if( !regulated_area ) {
            // Takes longer to cool down / warm up with AC, than it does to step outside and feel cruddy.
            next_climate_control_check += 40;
        }
    } else {
        return ( last_climate_control_ret ? true : false );
    }
    return regulated_area;
}

std::list<item *> player::get_radio_items()
{
    std::list<item *> rc_items;
    const invslice & stacks = inv.slice();
    for( auto &stack : stacks ) {
        item &itemit = stack->front();
        item *stack_iter = &itemit;
        if( stack_iter->has_flag("RADIO_ACTIVATION") ) {
            rc_items.push_back( stack_iter );
        }
    }

    for( auto &elem : worn ) {
        if( elem.has_flag( "RADIO_ACTIVATION" ) ) {
            rc_items.push_back( &elem );
        }
    }

    if (!weapon.is_null()) {
        if ( weapon.has_flag("RADIO_ACTIVATION")) {
            rc_items.push_back( &weapon );
        }
    }
    return rc_items;
}

bool player::has_active_optcloak() const
{
    for( auto &w : worn ) {
        if( w.active && w.typeId() == OPTICAL_CLOAK_ITEM_ID ) {
            return true;
        }
    }
    return false;
}

void player::charge_power(int amount)
{
 power_level += amount;
 if (power_level > max_power_level)
  power_level = max_power_level;
 if (power_level < 0)
  power_level = 0;
}


/*
 * Calculate player brightness based on the brightest active item, as
 * per itype tag LIGHT_* and optional CHARGEDIM ( fade starting at 20% charge )
 * item.light.* is -unimplemented- for the moment, as it is a custom override for
 * applying light sources/arcs with specific angle and direction.
 */
float player::active_light() const
{
    float lumination = 0;

    int maxlum = 0;
    has_item_with( [&maxlum]( const item &it ) {
        const int lumit = it.getlight_emit();
        if( maxlum < lumit ) {
            maxlum = lumit;
        }
        return false; // continue search, otherwise has_item_with would cancel the search
    } );

    lumination = (float)maxlum;

    if ( lumination < 60 && has_active_bionic("bio_flashlight") ) {
        lumination = 60;
    } else if ( lumination < 25 && has_artifact_with(AEP_GLOW) ) {
        lumination = 25;
    } else if (lumination < 5 && has_effect("glowing")){
            lumination = 5;
    }
    return lumination;
}

tripoint player::global_square_location() const
{
    return g->m.getabs( position );
}

tripoint player::global_sm_location() const
{
    return overmapbuffer::ms_to_sm_copy( global_square_location() );
}

tripoint player::global_omt_location() const
{
    return overmapbuffer::ms_to_omt_copy( global_square_location() );
}

const tripoint &player::pos() const
{
    return position;
}

int player::sight_range(int light_level) const
{
    /* Via Beer-Lambert we have:
     * light_level * (1 / exp( LIGHT_TRANSPARENCY_OPEN_AIR * distance) ) <= LIGHT_AMBIENT_LOW
     * Solving for distance:
     * 1 / exp( LIGHT_TRANSPARENCY_OPEN_AIR * distance ) <= LIGHT_AMBIENT_LOW / light_level
     * 1 <= exp( LIGHT_TRANSPARENCY_OPEN_AIR * distance ) * LIGHT_AMBIENT_LOW / light_level
     * light_level <= exp( LIGHT_TRANSPARENCY_OPEN_AIR * distance ) * LIGHT_AMBIENT_LOW
     * log(light_level) <= LIGHT_TRANSPARENCY_OPEN_AIR * distance + log(LIGHT_AMBIENT_LOW)
     * log(light_level) - log(LIGHT_AMBIENT_LOW) <= LIGHT_TRANSPARENCY_OPEN_AIR * distance
     * log(LIGHT_AMBIENT_LOW / light_level) <= LIGHT_TRANSPARENCY_OPEN_AIR * distance
     * log(LIGHT_AMBIENT_LOW / light_level) * (1 / LIGHT_TRANSPARENCY_OPEN_AIR) <= distance
     */
    int range = -log( get_vision_threshold( g->m.ambient_light_at(pos()) ) / (float)light_level ) *
        (1.0 / LIGHT_TRANSPARENCY_OPEN_AIR);
    // int range = log(light_level * LIGHT_AMBIENT_LOW) / LIGHT_TRANSPARENCY_OPEN_AIR;

    // Clamp to sight_max.
    return std::min(range, sight_max);
}

int player::unimpaired_range() const
{
    return std::min(sight_max, 60);
}

bool player::overmap_los( const tripoint &omt, int sight_points )
{
    const tripoint ompos = global_omt_location();
    if( omt.x < ompos.x - sight_points || omt.x > ompos.x + sight_points ||
        omt.y < ompos.y - sight_points || omt.y > ompos.y + sight_points ) {
        // Outside maximum sight range
        return false;
    }

    const std::vector<tripoint> line = line_to( ompos, omt, 0, 0 );
    for( size_t i = 0; i < line.size() && sight_points >= 0; i++ ) {
        const tripoint &pt = line[i];
        const oter_id &ter = overmap_buffer.ter( pt );
        const int cost = otermap[ter].see_cost;
        sight_points -= cost;
        if( sight_points < 0 )
            return false;
    }
    return true;
}

int player::overmap_sight_range(int light_level) const
{
    int sight = sight_range(light_level);
    if( sight < SEEX ) {
        return 0;
    }
    if( sight <= SEEX * 4) {
        return (sight / (SEEX / 2) );
    }
    if ((has_amount("binoculars", 1) || has_amount("rifle_scope", 1) ||
        has_amount("survivor_scope", 1) || -1 != weapon.has_gunmod("rifle_scope") ) &&
        !has_trait("EAGLEEYED"))  {
         if (has_trait("BIRD_EYE")) {
             return 25;
         }
        return 20;
    }
    else if (!(has_amount("binoculars", 1) || has_amount("rifle_scope", 1) ||
        has_amount("survivor_scope", 1) || -1 != weapon.has_gunmod("rifle_scope") ) &&
        has_trait("EAGLEEYED"))  {
         if (has_trait("BIRD_EYE")) {
             return 25;
         }
        return 20;
    }
    else if ((has_amount("binoculars", 1) || has_amount("rifle_scope", 1) ||
        has_amount("survivor_scope", 1) || -1 != weapon.has_gunmod("rifle_scope") ) &&
        has_trait("EAGLEEYED"))  {
         if (has_trait("BIRD_EYE")) {
             return 30;
         }
        return 25;
    }
    else if (has_trait("BIRD_EYE")) {
            return 15;
        }
    return 10;
}

#define MAX_CLAIRVOYANCE 40
int player::clairvoyance() const
{
    if (has_artifact_with(AEP_SUPER_CLAIRVOYANCE)) {
        return MAX_CLAIRVOYANCE;
    }
    if (has_artifact_with(AEP_CLAIRVOYANCE)) {
        return 3;
    }
    return 0;
}

bool player::sight_impaired() const
{
 return ((( has_effect("boomered") || has_effect("darkness") ) &&
          (!(has_trait("PER_SLIME_OK")))) ||
  (underwater && !has_bionic("bio_membrane") && !has_trait("MEMBRANE") &&
              !worn_with_flag("SWIM_GOGGLES") && !has_trait("PER_SLIME_OK") &&
              !has_trait("CEPH_EYES") ) ||
  ((has_trait("MYOPIC") || has_trait("URSINE_EYE") ) &&
                        !is_wearing("glasses_eye") &&
                        !is_wearing("glasses_monocle") &&
                        !is_wearing("glasses_bifocal") &&
                        !has_effect("contacts")) ||
   has_trait("PER_SLIME"));
}

bool player::has_two_arms() const
{
 if ((has_bionic("bio_blaster") || hp_cur[hp_arm_l] < 10 || hp_cur[hp_arm_r] < 10) ||
   has_active_mutation("SHELL2")) {
    // You can't effectively use both arms to wield something when they're in your shell
    return false;
 }
 return true;
}

bool player::avoid_trap( const tripoint &pos, const trap &tr ) const
{
    ///\EFFECT_DEX increases chance to avoid traps

    ///\EFFECT_DODGE increases chance to avoid traps
    int myroll = dice( 3, int(dex_cur + get_skill_level( skill_dodge ) * 1.5) );
    int traproll;
    if( tr.can_see( pos, *this ) ) {
        traproll = dice( 3, tr.get_avoidance() );
    } else {
        traproll = dice( 6, tr.get_avoidance() );
    }

    if( has_trait( "LIGHTSTEP" ) ) {
        myroll += dice( 2, 6 );
    }

    if( has_trait( "CLUMSY" ) ) {
        myroll -= dice( 2, 6 );
    }

    return myroll >= traproll;
}

body_part player::get_random_body_part( bool main ) const
{
    // TODO: Refuse broken limbs, adjust for mutations
    return random_body_part( main );
}

bool player::has_pda()
{
    static bool pda = false;
    if ( !pda_cached ) {
      pda_cached = true;
      pda = has_amount("pda", 1)  || has_amount("pda_flashlight", 1);
    }

    return pda;
}


bool player::has_alarm_clock() const
{
    return ( has_item_with_flag("ALARMCLOCK") ||
             (
               ( g->m.veh_at( pos() ) != nullptr ) &&
               !g->m.veh_at( pos() )->all_parts_with_feature( "ALARMCLOCK", true ).empty()
             ) ||
             has_bionic("bio_watch")
           );
}

bool player::has_watch() const
{
    return ( has_item_with_flag("WATCH") ||
             (
               ( g->m.veh_at( pos() ) != nullptr ) &&
               !g->m.veh_at( pos() )->all_parts_with_feature( "WATCH", true ).empty()
             ) ||
             has_bionic("bio_watch")
           );
}

void player::pause()
{
    moves = 0;
    ///\EFFECT_STR increases recoil recovery speed

    ///\EFFECT_GUN increases recoil recovery speed
    recoil -= str_cur + 2 * get_skill_level( skill_gun );
    recoil = std::max(MIN_RECOIL * 2, recoil);
    recoil = int(recoil / 2);

    // Train swimming if underwater
    if( underwater ) {
        practice( skill_swimming, 1 );
        drench(100, mfb(bp_leg_l)|mfb(bp_leg_r)|mfb(bp_torso)|mfb(bp_arm_l)|mfb(bp_arm_r)|
                    mfb(bp_head)| mfb(bp_eyes)|mfb(bp_mouth)|mfb(bp_foot_l)|mfb(bp_foot_r)|
                    mfb(bp_hand_l)|mfb(bp_hand_r), true );
    } else if( g->m.has_flag( TFLAG_DEEP_WATER, pos() ) ) {
        practice( skill_swimming, 1 );
        // Same as above, except no head/eyes/mouth
        drench(100, mfb(bp_leg_l)|mfb(bp_leg_r)|mfb(bp_torso)|mfb(bp_arm_l)|mfb(bp_arm_r)|
                    mfb(bp_foot_l)|mfb(bp_foot_r)| mfb(bp_hand_l)|mfb(bp_hand_r), true );
    } else if( g->m.has_flag( "SWIMMABLE", pos() ) ) {
        drench( 40, mfb(bp_foot_l) | mfb(bp_foot_r) | mfb(bp_leg_l) | mfb(bp_leg_r), false );
    }

    if( is_npc() ) {
        // The stuff below doesn't apply to NPCs
        // search_surroundings should eventually do, though
        return;
    }

    VehicleList vehs = g->m.get_vehicles();
    vehicle* veh = NULL;
    for (auto &v : vehs) {
        veh = v.v;
        if (veh && veh->velocity != 0 && veh->player_in_control(*this)) {
            if (one_in(8)) {
                double exp_temp = 1 + veh->total_mass() / 400.0 + std::abs (veh->velocity / 3200.0);
                int experience = static_cast<int>( floorf( exp_temp ) );
                if( exp_temp - experience > 0 && x_in_y( exp_temp - experience, 1.0 ) ) {
                    experience++;
                }
                practice( skill_id( "driving" ), experience );
            }
            break;
        }
    }

    search_surroundings();
}

void player::shout( std::string msg )
{
    int base = 10;
    int shout_multiplier = 2;

    // Mutations make shouting louder, they also define the defualt message
    if ( has_trait("SHOUT2") ) {
        base = 15;
        shout_multiplier = 3;
        if ( msg.empty() ) {
            msg = _("You scream loudly!");
        }
    }

    if ( has_trait("SHOUT3") ) {
        shout_multiplier = 4;
        base = 20;
        if ( msg.empty() ) {
            msg = _("You let out a piercing howl!");
        }
    }

    if ( msg.empty() ) {
        msg = _("You shout loudly!");
    }
    // Masks and such dampen the sound
    // Balanced around  whisper for wearing bondage mask
    // and noise ~= 10(door smashing) for wearing dust mask for character with strength = 8
    ///\EFFECT_STR increases shouting volume
    int noise = base + str_cur * shout_multiplier - encumb( bp_mouth ) * 3 / 2;

    // Minimum noise volume possible after all reductions.
    // Volume 1 can't be heard even by player
    constexpr int minimum_noise = 2;

    if ( noise <= base ) {
        std::string dampened_shout;
        std::transform( msg.begin(), msg.end(), std::back_inserter(dampened_shout), tolower );
        noise = std::max( minimum_noise, noise );
        msg = std::move( dampened_shout );
    }

    // Screaming underwater is not good for oxygen and harder to do overall
    if ( underwater ) {
        if ( !has_trait("GILLS") && !has_trait("GILLS_CEPH") ) {
            mod_stat( "oxygen", -noise );
        }

        noise = std::max(minimum_noise, noise / 2);
    }

    sounds::sound( pos(), noise, msg );
}

void player::toggle_move_mode()
{
    if( move_mode == "walk" ) {
        if( stamina > 0 && !has_effect("winded") ) {
            move_mode = "run";
            add_msg(_("You start running."));
        } else {
            add_msg(m_bad, _("You're too tired to run."));
        }
    } else if( move_mode == "run" ) {
        move_mode = "walk";
        add_msg(_("You slow to a walk."));
    }
}

void player::search_surroundings()
{
    if (controlling_vehicle) {
        return;
    }
    // Search for traps in a larger area than before because this is the only
    // way we can "find" traps that aren't marked as visible.
    // Detection formula takes care of likelihood of seeing within this range.
    const int z = posz();
    for( int xd = -5; xd <= 5; xd++ ) {
        for( int yd = -5; yd <= 5; yd++ ) {
            const int x = posx() + xd;
            const int y = posy() + yd;
            const trap &tr = g->m.tr_at( tripoint( x, y, z ) );
            if( tr.is_null() || (x == posx() && y == posy() ) ) {
                continue;
            }
            if( !sees( x, y ) ) {
                continue;
            }
            if( tr.name.empty() || tr.can_see( tripoint( x, y, z ), *this ) ) {
                // Already seen, or has no name -> can never be seen
                continue;
            }
            // Chance to detect traps we haven't yet seen.
            if (tr.detect_trap( tripoint( x, y, z ), *this )) {
                if( tr.get_visibility() > 0 ) {
                    // Only bug player about traps that aren't trivial to spot.
                    const std::string direction = direction_name(
                        direction_from(posx(), posy(), x, y));
                    add_msg_if_player(_("You've spotted a %1$s to the %2$s!"),
                                      tr.name.c_str(), direction.c_str());
                }
                add_known_trap( tripoint( x, y, z ), tr);
            }
        }
    }
}

int player::throw_range(int pos) const
{
    item tmp;
    if( pos == -1 ) {
        tmp = weapon;
    } else if( pos == INT_MIN ) {
        return -1;
    } else {
        tmp = inv.find_item(pos);
    }

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
    if( has_active_bionic("bio_railgun") && (tmp.made_of("iron") || tmp.made_of("steel"))) {
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

int player::ranged_dex_mod() const
{
    const int dex = get_dex();

    ///\EFFECT_DEX <12 increases ranged penalty
    if (dex >= 12) { return 0; }
    return (12 - dex) * 15;
}

int player::ranged_per_mod() const
{
    const int per = get_per();

    ///\EFFECT_PER <12 increases ranged penalty
    if (per >= 12) { return 0; }
    return (12 - per) * 15;
}

int player::throw_dex_mod(bool return_stat_effect) const
{
  // Stat window shows stat effects on based on current stat
 int dex = get_dex();
 if (dex == 8 || dex == 9)
  return 0;
 ///\EFFECT_DEX increases throwing bonus
 if (dex >= 10)
  return (return_stat_effect ? 0 - rng(0, dex - 9) : 9 - dex);

 int deviation = 0;
 if (dex < 4)
  deviation = 4 * (8 - dex);
 else if (dex < 6)
  deviation = 3 * (8 - dex);
 else
  deviation = 2 * (8 - dex);

 // return_stat_effect actually matters here
 return (return_stat_effect ? rng(0, deviation) : deviation);
}

// Calculates MOC of aim improvement per 10 moves, based on
// skills, stats, and quality of the gun sight being used.
// Using this weird setup because # of MOC per move is too fast, (slowest is one MOC/move)
// and number of moves per MOC is too slow. (fastest is one MOC/move)
// A worst case of 1 MOC per 10 moves is acceptable, and it scales up
// indefinitely, though the smallest unit of aim time is 10 moves.
int player::aim_per_time( item *gun ) const
{
    // Account for Dexterity, weapon skill, weapon mods and flags,
    int speed_penalty = 0;
    // Ranges from 0 - 600.
    // 0 - 10 after adjustment.
    speed_penalty += skill_dispersion( gun, false ) / 60;
    // Ranges from 0 - 12 after adjustment.
    speed_penalty += ranged_dex_mod() / 15;
    // Ranges from 0 - 10
    speed_penalty += gun->aim_speed( recoil );
    // TODO: should any conditions, mutations, etc affect this?
    // Probably CBMs too.
    int improvement_amount = std::max( 1, 32 - speed_penalty );
    // Improvement rate is capped by the max aim level of the gun sight being used.
    return std::min( improvement_amount, recoil - gun->sight_dispersion( recoil ) );
}

int player::read_speed(bool return_stat_effect) const
{
    // Stat window shows stat effects on based on current stat
    const int intel = get_int();
    ///\EFFECT_INT increases reading speed
    int ret = 1000 - 50 * (intel - 8);
    if( has_trait("FASTREADER") ) {
        ret *= .8;
    }

    if( has_trait("SLOWREADER") ) {
        ret *= 1.3;
    }

    if( ret < 100 ) {
        ret = 100;
    }
    // return_stat_effect actually matters here
    return (return_stat_effect ? ret : ret / 10);
}

int player::rust_rate(bool return_stat_effect) const
{
    if (OPTIONS["SKILL_RUST"] == "off") {
        return 0;
    }

    // Stat window shows stat effects on based on current stat
    int intel = (return_stat_effect ? get_int() : get_int());
    ///\EFFECT_INT reduces skill rust
    int ret = ((OPTIONS["SKILL_RUST"] == "vanilla" || OPTIONS["SKILL_RUST"] == "capped") ? 500 : 500 - 35 * (intel - 8));

    if (has_trait("FORGETFUL")) {
        ret *= 1.33;
    }

    if (has_trait("GOODMEMORY")) {
        ret *= .66;
    }

    if (ret < 0) {
        ret = 0;
    }

    // return_stat_effect actually matters here
    return (return_stat_effect ? ret : ret / 10);
}

int player::talk_skill() const
{
    ///\EFFECT_INT slightly increases talking skill

    ///\EFFECT_PER slightly increases talking skill

    ///\EFFECT_SPEECH increases talking skill
    int ret = get_int() + get_per() + get_skill_level( skill_id( "speech" ) ) * 3;
    if (has_trait("SAPIOVORE")) {
        ret -= 20; // Friendly convo with your prey? unlikely
    } else if (has_trait("UGLY")) {
        ret -= 3;
    } else if (has_trait("DEFORMED")) {
        ret -= 6;
    } else if (has_trait("DEFORMED2")) {
        ret -= 12;
    } else if (has_trait("DEFORMED3")) {
        ret -= 18;
    } else if (has_trait("PRETTY")) {
        ret += 1;
    } else if (has_trait("BEAUTIFUL")) {
        ret += 2;
    } else if (has_trait("BEAUTIFUL2")) {
        ret += 4;
    } else if (has_trait("BEAUTIFUL3")) {
        ret += 6;
    }
    return ret;
}

int player::intimidation() const
{
    ///\EFFECT_STR increases intimidation factor
    int ret = get_str() * 2;
    if (weapon.is_gun()) {
        ret += 10;
    }
    if (weapon.damage_bash() >= 12 || weapon.damage_cut() >= 12) {
        ret += 5;
    }
    if (has_trait("SAPIOVORE")) {
        ret += 5; // Scaring one's prey, on the other claw...
    } else if (has_trait("DEFORMED2")) {
        ret += 3;
    } else if (has_trait("DEFORMED3")) {
        ret += 6;
    } else if (has_trait("PRETTY")) {
        ret -= 1;
    } else if (has_trait("BEAUTIFUL") || has_trait("BEAUTIFUL2") || has_trait("BEAUTIFUL3")) {
        ret -= 4;
    }
    if (stim > 20) {
        ret += 2;
    }
    if (has_effect("drunk")) {
        ret -= 4;
    }

    return ret;
}

bool player::is_dead_state() const
{
    return hp_cur[hp_head] <= 0 || hp_cur[hp_torso] <= 0;
}

void player::on_dodge( Creature *source, int difficulty )
{
    if( difficulty == INT_MIN && source != nullptr ) {
        difficulty = source->get_melee();
    }

    if( difficulty > 0 ) {
        practice( skill_dodge, difficulty );
    }

    ma_ondodge_effects();
}

void player::on_hit( Creature *source, body_part bp_hit,
                     int difficulty, dealt_projectile_attack const* const proj ) {
    check_dead_state();
    bool u_see = g->u.sees( *this );
    if( source == nullptr || proj != nullptr ) {
        return;
    }

    if( difficulty == INT_MIN ) {
        difficulty = source->get_melee();
    }

    if( difficulty > 0 ) {
        practice( skill_dodge, difficulty );
    }

    if (has_active_bionic("bio_ods")) {
        if (is_player()) {
            add_msg(m_good, _("Your offensive defense system shocks %s in mid-attack!"),
                            source->disp_name().c_str());
        } else if (u_see) {
            add_msg(_("%1$s's offensive defense system shocks %2$s in mid-attack!"),
                        disp_name().c_str(),
                        source->disp_name().c_str());
        }
        damage_instance ods_shock_damage;
        ods_shock_damage.add_damage(DT_ELECTRIC, rng(10,40));
        source->deal_damage(this, bp_torso, ods_shock_damage);
    }
    if ((!(wearing_something_on(bp_hit))) && (has_trait("SPINES") || has_trait("QUILLS"))) {
        int spine = rng(1, (has_trait("QUILLS") ? 20 : 8));
        if (!is_player()) {
            if( u_see ) {
                add_msg(_("%1$s's %2$s puncture %3$s in mid-attack!"), name.c_str(),
                            (has_trait("QUILLS") ? _("quills") : _("spines")),
                            source->disp_name().c_str());
            }
        } else {
            add_msg(m_good, _("Your %1$s puncture %2$s in mid-attack!"),
                            (has_trait("QUILLS") ? _("quills") : _("spines")),
                            source->disp_name().c_str());
        }
        damage_instance spine_damage;
        spine_damage.add_damage(DT_STAB, spine);
        source->deal_damage(this, bp_torso, spine_damage);
    }
    if ((!(wearing_something_on(bp_hit))) && (has_trait("THORNS")) && (!(source->has_weapon()))) {
        if (!is_player()) {
            if( u_see ) {
                add_msg(_("%1$s's %2$s scrape %3$s in mid-attack!"), name.c_str(),
                            _("thorns"), source->disp_name().c_str());
            }
        } else {
            add_msg(m_good, _("Your thorns scrape %s in mid-attack!"), source->disp_name().c_str());
        }
        int thorn = rng(1, 4);
        damage_instance thorn_damage;
        thorn_damage.add_damage(DT_CUT, thorn);
        // In general, critters don't have separate limbs
        // so safer to target the torso
        source->deal_damage(this, bp_torso, thorn_damage);
    }
    if ((!(wearing_something_on(bp_hit))) && (has_trait("CF_HAIR"))) {
        if (!is_player()) {
            if( u_see ) {
                add_msg(_("%1$s gets a load of %2$s's %3$s stuck in!"), source->disp_name().c_str(),
                  name.c_str(), (_("hair")));
            }
        } else {
            add_msg(m_good, _("Your hairs detach into %s!"), source->disp_name().c_str());
        }
        source->add_effect("stunned", 2);
        if (one_in(3)) { // In the eyes!
            source->add_effect("blind", 2);
        }
    }
}

void player::on_hurt( Creature *source, bool disturb /*= true*/ )
{
    if( has_trait("ADRENALINE") && !has_effect("adrenaline") &&
        (hp_cur[hp_head] < 25 || hp_cur[hp_torso] < 15) ) {
        add_effect("adrenaline", 200);
    }

    if( disturb ) {
        if( in_sleep_state() ) {
            wake_up();
        }
        if( !is_npc() ) {
            if( source != nullptr ) {
                g->cancel_activity_query(_("You were attacked by %s!"), source->disp_name().c_str());
            } else {
                g->cancel_activity_query(_("You were hurt!"));
            }
        }
    }

    if( is_dead_state() ) {
        set_killer( source );
    }
}

dealt_damage_instance player::deal_damage(Creature* source, body_part bp, const damage_instance& d)
{
    if( has_trait( "DEBUG_NODMG" ) ) {
        return dealt_damage_instance();
    }

    dealt_damage_instance dealt_dams = Creature::deal_damage(source, bp, d); //damage applied here
    int dam = dealt_dams.total_damage(); //block reduction should be by applied this point

    // TODO: Pre or post blit hit tile onto "this"'s location here
    if(g->u.sees( pos() )) {
        g->draw_hit_player(*this, dam);

        if (dam > 0 && is_player() && source) {
            //monster hits player melee
            SCT.add(posx(), posy(),
                    direction_from(0, 0, posx() - source->posx(), posy() - source->posy()),
                    get_hp_bar(dam, get_hp_max(player::bp_to_hp(bp))).first, m_bad,
                    body_part_name(bp), m_neutral);
        }
    }

    // handle snake artifacts
    if (has_artifact_with(AEP_SNAKES) && dam >= 6) {
        int snakes = int(dam / 6);
        std::vector<tripoint> valid;
        for (int x = posx() - 1; x <= posx() + 1; x++) {
            for (int y = posy() - 1; y <= posy() + 1; y++) {
                tripoint dest(x, y, posz());
                if (g->is_empty( dest )) {
                    valid.push_back( dest );
                }
            }
        }
        if (snakes > int(valid.size())) {
            snakes = int(valid.size());
        }
        if (snakes == 1) {
            add_msg(m_warning, _("A snake sprouts from your body!"));
        } else if (snakes >= 2) {
            add_msg(m_warning, _("Some snakes sprout from your body!"));
        }
        for (int i = 0; i < snakes && !valid.empty(); i++) {
            const tripoint target = random_entry_removed( valid );
            if (g->summon_mon(mon_shadow_snake, target)) {
                monster *snake = g->monster_at( target );
                snake->friendly = -1;
            }
        }
    }

    // And slimespawners too
    if ((has_trait("SLIMESPAWNER")) && (dam >= 10) && one_in(20 - dam)) {
        std::vector<tripoint> valid;
        for (int x = posx() - 1; x <= posx() + 1; x++) {
            for (int y = posy() - 1; y <= posy() + 1; y++) {
                tripoint dest(x, y, posz());
                if (g->is_empty(dest)) {
                    valid.push_back( dest );
                }
            }
        }
        add_msg(m_warning, _("Slime is torn from you, and moves on its own!"));
        int numslime = 1;
        for (int i = 0; i < numslime && !valid.empty(); i++) {
            const tripoint target = random_entry_removed( valid );
            if (g->summon_mon(mon_player_blob, target)) {
                monster *slime = g->monster_at( target );
                slime->friendly = -1;
            }
        }
    }

    //Acid blood effects.
    bool u_see = g->u.sees(*this);
    int cut_dam = dealt_dams.type_damage(DT_CUT);
    if (has_trait("ACIDBLOOD") && !one_in(3) && (dam >= 4 || cut_dam > 0) && (rl_dist(g->u.pos(), source->pos()) <= 1)) {
        if (is_player()) {
            add_msg(m_good, _("Your acidic blood splashes %s in mid-attack!"),
                            source->disp_name().c_str());
        } else if (u_see) {
            add_msg(_("%1$s's acidic blood splashes on %2$s in mid-attack!"),
                        disp_name().c_str(),
                        source->disp_name().c_str());
        }
        damage_instance acidblood_damage;
        acidblood_damage.add_damage(DT_ACID, rng(4,16));
        if (!one_in(4)) {
            source->deal_damage(this, bp_arm_l, acidblood_damage);
            source->deal_damage(this, bp_arm_r, acidblood_damage);
        } else {
            source->deal_damage(this, bp_torso, acidblood_damage);
            source->deal_damage(this, bp_head, acidblood_damage);
        }
    }

    switch (bp) {
    case bp_eyes:
        if (dam > 5 || cut_dam > 0) {
            int minblind = int((dam + cut_dam) / 10);
            if (minblind < 1) {
                minblind = 1;
            }
            int maxblind = int((dam + cut_dam) /  4);
            if (maxblind > 5) {
                maxblind = 5;
            }
            add_effect("blind", rng(minblind, maxblind));
        }
        break;
    case bp_torso:
        // getting hit throws off our shooting
        recoil += int(dam * 3);
        break;
    case bp_hand_l: // Fall through to arms
    case bp_arm_l:
        if( weapon.is_two_handed( *this ) ) {
            recoil += int(dam * 5);
        }
        break;
    case bp_hand_r: // Fall through to arms
    case bp_arm_r:
        // getting hit in the arms throws off our shooting
        recoil += int(dam * 5);
        break;
    case bp_foot_l: // Fall through to legs
    case bp_leg_l:
        break;
    case bp_foot_r: // Fall through to legs
    case bp_leg_r:
        break;
    case bp_mouth: // Fall through to head damage
    case bp_head:
        break;
    default:
        debugmsg("Wacky body part hit!");
    }
    //looks like this should be based off of dealtdams, not d as d has no damage reduction applied.
    // Skip all this if the damage isn't from a creature. e.g. an explosion.
    if( source != NULL ) {
        // Add any monster on damage effects
        if (source->is_monster() && dealt_dams.total_damage() > 0) {
            monster *m = dynamic_cast<monster *>(source); if( m != NULL ) {
                for (auto eff : m->type->atk_effs) {
                    if (eff.id == "null") {
                        continue;
                    }
                    if (x_in_y(eff.chance, 100)) {
                        add_effect( eff.id, eff.duration, eff.bp, eff.permanent );
                    }
                }
            }
        }

        if (dealt_dams.total_damage() > 0 && source->has_flag(MF_VENOM)) {
            add_msg_if_player(m_bad, _("You're poisoned!"));
            add_effect("poison", 30);
        }
        else if (dealt_dams.total_damage() > 0 && source->has_flag(MF_BADVENOM)) {
            add_msg_if_player(m_bad, _("You feel poison flood your body, wracking you with pain..."));
            add_effect("badpoison", 40);
        }
        else if (dealt_dams.total_damage() > 0 && source->has_flag(MF_PARALYZE)) {
            add_msg_if_player(m_bad, _("You feel poison enter your body!"));
            add_effect("paralyzepoison", 100);
        }

        if (source->has_flag(MF_BLEED) && dealt_dams.total_damage() > 6 &&
            dealt_dams.type_damage(DT_CUT) > 0) {
            // Maybe should only be if DT_CUT > 6... Balance question
            add_effect("bleed", 60, bp);
        }

        if ( source->has_flag(MF_GRABS) && !source->is_hallucination() ) {
            ///\EFFECT_DEX increases chance to avoid being grabbed, if DEX>STR

            ///\EFFECT_STR increases chance to avoid being grabbed, if STR>DEX
            if (has_grab_break_tec() && get_grab_resist() > 0 && get_dex() > get_str() ?
                rng(0, get_dex()) : rng( 0, get_str()) > rng( 0 , 10)) {
                if (has_effect("grabbed")){
                    add_msg_if_player(m_info,_("You are being grabbed by %s, but you bat it away!"),
                                      source->disp_name().c_str());
                } else {add_msg_player_or_npc(m_info, _("You are being grabbed by %s, but you break its grab!"),
                                    _("<npcname> are being grabbed by %s, but they break its grab!"),
                                    source->disp_name().c_str());
                }
            } else {
                int prev_effect = get_effect_int("grabbed");
                add_effect("grabbed", 2, bp_torso, false, prev_effect + 2);
                add_msg_player_or_npc(m_bad, _("You are grabbed by %s!"), _("<npcname> is grabbed by %s!"),
                                source->disp_name().c_str());
            }
        }
    }

    on_hurt( source );
    return dealt_damage_instance(dealt_dams);
}

void player::mod_pain(int npain) {
    if ((has_trait("NOPAIN"))) {
        return;
    }
    if (has_trait("PAINRESIST") && npain > 1) {
        // if it's 1 it'll just become 0, which is bad
        npain = npain * 4 / rng(4,8);
    }
    // Dwarves get better pain-resist, what with mining and all
    if (has_trait("PAINRESIST_TROGLO") && npain > 1) {
        npain = npain * 4 / rng(6,9);
    }
    // reduce new felt pain by excess pkill, if any.
    int felt_pain = npain - std::max(0, pkill - pain);
    // Only trigger the "you felt it" effects if we are going to feel it.
    if (felt_pain > 0) {
        // Putting the threshold at 2 here to avoid most basic "ache" style
        // effects from constantly asking you to stop crafting.
        if (!is_npc() && felt_pain >= 2) {
            g->cancel_activity_query(_("Ouch, something hurts!"));
        }
        // Only a large pain burst will actually wake people while sleeping.
        if(in_sleep_state()) {
            int pain_thresh = rng(3, 5);
            if (has_trait("HEAVYSLEEPER")) {
                pain_thresh += 2;
            } else if (has_trait("HEAVYSLEEPER2")) {
                pain_thresh += 5;
            }
            if (felt_pain >= pain_thresh) {
                wake_up();
            }
        }
    }
    Creature::mod_pain(npain);
}

/*
    Where damage to player is actually applied to hit body parts
    Might be where to put bleed stuff rather than in player::deal_damage()
 */
void player::apply_damage(Creature *source, body_part hurt, int dam)
{
    if( is_dead_state() || has_trait( "DEBUG_NODMG" ) ) {
        // don't do any more damage if we're already dead
        // Or if we're debugging and don't want to die
        return;
    }

    hp_part hurtpart = bp_to_hp( hurt );
    if( hurtpart == num_hp_parts ) {
        debugmsg("Wacky body part hurt!");
        hurtpart = hp_torso;
    }

    mod_pain( dam / 2 );

    hp_cur[hurtpart] -= dam;
    if (hp_cur[hurtpart] < 0) {
        lifetime_stats()->damage_taken += hp_cur[hurtpart];
        hp_cur[hurtpart] = 0;
    }

    lifetime_stats()->damage_taken += dam;
    if( dam > pkill ) {
        on_hurt( source );
    }
}

void player::heal(body_part healed, int dam)
{
    hp_part healpart;
    switch (healed) {
        case bp_eyes: // Fall through to head damage
        case bp_mouth: // Fall through to head damage
        case bp_head:
            healpart = hp_head;
            break;
        case bp_torso:
            healpart = hp_torso;
            break;
        case bp_hand_l:
            // Shouldn't happen, but fall through to arms
            debugmsg("Heal against hands!");
        case bp_arm_l:
            healpart = hp_arm_l;
            break;
        case bp_hand_r:
            // Shouldn't happen, but fall through to arms
            debugmsg("Heal against hands!");
        case bp_arm_r:
            healpart = hp_arm_r;
            break;
        case bp_foot_l:
            // Shouldn't happen, but fall through to legs
            debugmsg("Heal against feet!");
        case bp_leg_l:
            healpart = hp_leg_l;
            break;
        case bp_foot_r:
            // Shouldn't happen, but fall through to legs
            debugmsg("Heal against feet!");
        case bp_leg_r:
            healpart = hp_leg_r;
            break;
        default:
            debugmsg("Wacky body part healed!");
            healpart = hp_torso;
    }
    heal( healpart, dam );
}

void player::heal(hp_part healed, int dam)
{
    if (hp_cur[healed] > 0) {
        hp_cur[healed] += dam;
        if (hp_cur[healed] > hp_max[healed]) {
            lifetime_stats()->damage_healed -= hp_cur[healed] - hp_max[healed];
            hp_cur[healed] = hp_max[healed];
        }
        lifetime_stats()->damage_healed += dam;
    }
}

void player::healall(int dam)
{
    for( int healed_part = 0; healed_part < num_hp_parts; healed_part++) {
        heal( (hp_part)healed_part, dam );
    }
}

void player::hurtall(int dam, Creature *source, bool disturb /*= true*/)
{
    if( is_dead_state() || has_trait( "DEBUG_NODMG" ) || dam <= 0 ) {
        return;
    }

    for( int i = 0; i < num_hp_parts; i++ ) {
        const hp_part bp = static_cast<hp_part>( i );
        // Don't use apply_damage here or it will annoy the player with 6 queries
        hp_cur[bp] -= dam;
        lifetime_stats()->damage_taken += dam;
        if( hp_cur[bp] < 0 ) {
            lifetime_stats()->damage_taken += hp_cur[bp];
            hp_cur[bp] = 0;
        }
    }

    // Low pain: damage is spread all over the body, so not as painful as 6 hits in one part
    mod_pain( dam );
    on_hurt( source, disturb );
}

int player::hitall(int dam, int vary, Creature *source)
{
    int damage_taken = 0;
    for (int i = 0; i < num_hp_parts; i++) {
        const body_part bp = hp_to_bp( static_cast<hp_part>( i ) );
        int ddam = vary ? dam * rng (100 - vary, 100) / 100 : dam;
        int cut = 0;
        auto damage = damage_instance::physical(ddam, cut, 0);
        damage_taken += deal_damage( source, bp, damage ).total_damage();
    }
    return damage_taken;
}

float player::fall_damage_mod() const
{
    float ret = 1.0f;

    // Ability to land properly is 2x as important as dexterity itself
    ///\EFFECT_DEX decreases damage from falling

    ///\EFFECT_DODGE decreases damage from falling
    float dex_dodge = dex_cur / 2 + get_skill_level( skill_dodge );
    // Penalize for wearing heavy stuff
    dex_dodge -= ( ( ( encumb(bp_leg_l) + encumb(bp_leg_r) ) / 2 ) + ( encumb(bp_torso) / 1 ) ) / 10;
    // But prevent it from increasing damage
    dex_dodge = std::max( 0.0f, dex_dodge );
    // 100% damage at 0, 75% at 10, 50% at 20 and so on
    ret *= (100.0f - (dex_dodge * 4.0f)) / 100.0f;

    if( has_trait("PARKOUR") ) {
        ret *= 2.0f / 3.0f;
    }

    // TODO: Bonus for Judo, mutations. Penalty for heavy weight (including mutations)
    return std::max( 0.0f, ret );
}

// force is maximum damage to hp before scaling
int player::impact( const int force, const tripoint &p )
{
    // Falls over ~30m are fatal more often than not
    // But that would be quite a lot considering 21 z-levels in game
    // so let's assume 1 z-level is comparable to 30 force

    if( force <= 0 ) {
        return force;
    }

    // Damage modifier (post armor)
    float mod = 1.0f;
    int effective_force = force;
    int cut = 0;
    // Percentage arpen - armor won't help much here
    // TODO: Make cushioned items like bike helmets help more
    float armor_eff = 1.0f;

    // Being slammed against things rather than landing means we can't
    // control the impact as well
    const bool slam = p != pos();
    std::string target_name = "a swarm of bugs";
    Creature *critter = g->critter_at( p );
    int part_num = -1;
    vehicle *veh = g->m.veh_at( p, part_num );
    if( critter != this && critter != nullptr ) {
        target_name = critter->disp_name();
        // Slamming into creatures and NPCs
        // TODO: Handle spikes/horns and hard materials
        armor_eff = 0.5f; // 2x as much as with the ground
        // TODO: Modify based on something?
        mod = 1.0f;
        effective_force = force;
    } else if( veh != nullptr ) {
        // Slamming into vehicles
        // TODO: Integrate it with vehicle collision function somehow
        target_name = veh->disp_name();
        if( veh->part_with_feature( part_num, "SHARP" ) != -1 ) {
            // Now we're actually getting impaled
            cut = force; // Lots of fun
        }

        mod = slam ? 1.0f : fall_damage_mod();
        armor_eff = 0.25f; // Not much
        if( !slam && veh->part_with_feature( part_num, "ROOF" ) ) {
            // Roof offers better landing than frame or pavement
            effective_force /= 2; // TODO: Make this not happen with heavy duty/plated roof
        }
    } else {
        // Slamming into terrain/furniture
        target_name = g->m.disp_name( p );
        int hard_ground = g->m.has_flag( TFLAG_DIGGABLE, p ) ? 0 : 3;
        armor_eff = 0.25f; // Not much
        // Get cut by stuff
        // This isn't impalement on metal wreckage, more like flying through a closed window
        cut = g->m.has_flag( TFLAG_SHARP, p ) ? 5 : 0;
        effective_force = force + hard_ground;
        mod = slam ? 1.0f : fall_damage_mod();
        if( g->m.has_furn( p ) ) {
            // TODO: Make furniture matter
        } else if( g->m.has_flag( TFLAG_SWIMMABLE, p ) ) {
            // TODO: Some formula of swimming
            effective_force /= 4;
        }
    }

    // Rescale for huge force
    // At >30 force, proper landing is impossible and armor helps way less
    if( effective_force > 30 ) {
        // Armor simply helps way less
        armor_eff *= 30.0f / effective_force;
        if( mod < 1.0f ) {
            // Everything past 30 damage gets a worse modifier
            const float scaled_mod = std::pow( mod, 30.0f / effective_force );
            const float scaled_damage = ( 30.0f * mod ) + scaled_mod * ( effective_force - 30.0f );
            mod = scaled_damage / effective_force;
        }
    }

    if( !slam && mod < 1.0f && mod * force < 5 ) {
        // Perfect landing, no damage (regardless of armor)
        add_msg_if_player( m_warning, _("You land on %s."), target_name.c_str() );
        return 0;
    }

    int total_dealt = 0;
    for( int i = 0; i < num_hp_parts; i++ ) {
        const body_part bp = hp_to_bp( static_cast<hp_part>( i ) );
        int bash = ( effective_force * rng(60, 100) / 100 );
        damage_instance di;
        di.add_damage( DT_BASH, bash, 0, armor_eff, mod );
        // No good way to land on sharp stuff, so here modifier == 1.0f
        di.add_damage( DT_CUT,  cut,  0, armor_eff, 1.0f );
        total_dealt += deal_damage( nullptr, bp, di ).total_damage();
    }

    if( total_dealt > 0 && is_player() ) {
        // "You slam against the dirt" is fine
        add_msg( m_bad, _("You are slammed against %s for %d damage."),
                 target_name.c_str(), total_dealt );
    } else if( slam ) {
        // Only print this line if it is a slam and not a landing
        // Non-players should only get this one: player doesn't know how much damage was dealt
        // and landing messages for each slammed creature would be too much
        add_msg_player_or_npc( m_bad,
                               _("You are slammed against %s."),
                               _("<npcname> is slammed against %s."),
                               target_name.c_str() );
    } else {
        // No landing message for NPCs
        add_msg_if_player( m_warning, _("You land on %s."), target_name.c_str() );
    }

    if( x_in_y( mod, 1.0f ) ) {
        add_effect( "downed", 1 + rng( 0, mod * 3 ) );
    }

    return total_dealt;
}

void player::knock_back_from( const tripoint &p )
{
    if( p == pos() ) {
        return;
    }

    tripoint to = pos();
    const tripoint dp = pos() - p;
    to.x += sgn( dp.x );
    to.y += sgn( dp.y );

    // First, see if we hit a monster
    int mondex = g->mon_at( to );
    if (mondex != -1) {
        monster *critter = &(g->zombie(mondex));
        deal_damage( critter, bp_torso, damage_instance( DT_BASH, critter->type->size ) );
        add_effect("stunned", 1);
        ///\EFFECT_STR_MAX allows knocked back player to knock back, damage, stun some monsters
        if ((str_max - 6) / 4 > critter->type->size) {
            critter->knock_back_from(pos3()); // Chain reaction!
            critter->apply_damage( this, bp_torso, (str_max - 6) / 4);
            critter->add_effect("stunned", 1);
        } else if ((str_max - 6) / 4 == critter->type->size) {
            critter->apply_damage( this, bp_torso, (str_max - 6) / 4);
            critter->add_effect("stunned", 1);
        }
        critter->check_dead_state();

        add_msg_player_or_npc(_("You bounce off a %s!"), _("<npcname> bounces off a %s!"),
                              critter->name().c_str() );
        return;
    }

    int npcdex = g->npc_at( to );
    if (npcdex != -1) {
        npc *p = g->active_npc[npcdex];
        deal_damage( p, bp_torso, damage_instance( DT_BASH, p->get_size() ) );
        add_effect("stunned", 1);
        p->deal_damage( this, bp_torso, damage_instance( DT_BASH, 3 ) );
        add_msg_player_or_npc( _("You bounce off %s!"), _("<npcname> bounces off %s!"), p->name.c_str() );
        p->check_dead_state();
        return;
    }

    // If we're still in the function at this point, we're actually moving a tile!
    if (g->m.has_flag( "LIQUID", to ) && g->m.has_flag( TFLAG_DEEP_WATER, to )) {
        if (!is_npc()) {
            g->plswim( to );
        }
        // TODO: NPCs can't swim!
    } else if (g->m.move_cost( to ) == 0) { // Wait, it's a wall (or water)

        // It's some kind of wall.
        apply_damage( nullptr, bp_torso, 3 ); // TODO: who knocked us back? Maybe that creature should be the source of the damage?
        add_effect("stunned", 2);
        add_msg_player_or_npc( _("You bounce off a %s!"), _("<npcname> bounces off a %s!"),
                               g->m.tername( to ).c_str() );

    } else { // It's no wall
        setpos( to );
    }
}

hp_part player::bp_to_hp( const body_part bp )
{
    switch(bp) {
        case bp_head:
        case bp_eyes:
        case bp_mouth:
            return hp_head;
        case bp_torso:
            return hp_torso;
        case bp_arm_l:
        case bp_hand_l:
            return hp_arm_l;
        case bp_arm_r:
        case bp_hand_r:
            return hp_arm_r;
        case bp_leg_l:
        case bp_foot_l:
            return hp_leg_l;
        case bp_leg_r:
        case bp_foot_r:
            return hp_leg_r;
        default:
            return num_hp_parts;
    }
}

body_part player::hp_to_bp( const hp_part hpart )
{
    switch(hpart) {
        case hp_head:
            return bp_head;
        case hp_torso:
            return bp_torso;
        case hp_arm_l:
            return bp_arm_l;
        case hp_arm_r:
            return bp_arm_r;
        case hp_leg_l:
            return bp_leg_l;
        case hp_leg_r:
            return bp_leg_r;
        default:
            return num_bp;
    }
}

int player::hp_percentage() const
{
    int total_cur = 0, total_max = 0;
    // Head and torso HP are weighted 3x and 2x, respectively
    total_cur = hp_cur[hp_head] * 3 + hp_cur[hp_torso] * 2;
    total_max = hp_max[hp_head] * 3 + hp_max[hp_torso] * 2;
    for (int i = hp_arm_l; i < num_hp_parts; i++) {
        total_cur += hp_cur[i];
        total_max += hp_max[i];
    }

    return (100 * total_cur) / total_max;
}

// Returns the number of multiples of tick_length we would "pass" on our way `from` to `to`
// For example, if `tick_length` is 1 hour, then going from 0:59 to 1:01 should return 1
inline int ticks_between( int from, int to, int tick_length )
{
    return (to / tick_length) - (from / tick_length);
}

void player::update_body()
{
    const int now = calendar::turn;
    update_body( now - 1, now );
}

void player::update_body( int from, int to )
{
    update_stamina( to - from );
    const int five_mins = ticks_between( from, to, MINUTES(5) );
    if( five_mins > 0 ) {
        check_needs_extremes();
        update_needs( five_mins );
        if( has_effect( "sleep" ) ) {
            sleep_hp_regen( five_mins );
        }
    }

    const int thirty_mins = ticks_between( from, to, MINUTES(30) );
    if( thirty_mins > 0 ) {
        regen( thirty_mins );
        get_sick();
        mend( thirty_mins );
    }

    if( ticks_between( from, to, HOURS(6) ) ) {
        update_health();
    }
}

void player::get_sick()
{
    // NPCs are too dumb to handle infections now
    if( is_npc() || has_trait("DISIMMUNE") ) {
        // In a shocking twist, disease immunity prevents diseases.
        return;
    }

    if (has_effect("flu") || has_effect("common_cold")) {
        // While it's certainly possible to get sick when you already are,
        // it wouldn't be very fun.
        return;
    }

    // Normal people get sick about 2-4 times/year.
    int base_diseases_per_year = 3;
    if (has_trait("DISRESISTANT")) {
        // Disease resistant people only get sick once a year.
        base_diseases_per_year = 1;
    }

    // This check runs once every 30 minutes, so double to get hours, *24 to get days.
    const int checks_per_year = 2 * 24 * 365;

    // Health is in the range [-200,200].
    // A character who takes vitamins every 6 hours will have ~50 health.
    // Diseases are half as common for every 50 health you gain.
    float health_factor = std::pow(2.0f, get_healthy() / 50.0f);

    int disease_rarity = (int) (checks_per_year * health_factor / base_diseases_per_year);
    add_msg( m_debug, "disease_rarity = %d", disease_rarity);
    if (one_in(disease_rarity)) {
        if (one_in(6)) {
            // The flu typically lasts 3-10 days.
            const int short_flu = DAYS(3);
            const int long_flu = DAYS(10);
            const int duration = rng(short_flu, long_flu);
            add_env_effect("flu", bp_mouth, 3, duration);
        } else {
            // A cold typically lasts 1-14 days.
            int short_cold = DAYS(1);
            int long_cold = DAYS(14);
            int duration = rng(short_cold, long_cold);
            add_env_effect("common_cold", bp_mouth, 3, duration);
        }
    }
}

void player::update_health(int external_modifiers)
{
    if( has_artifact_with( AEP_SICK ) ) {
        // Carrying a sickness artifact makes your health 50 points worse on
        // average.  This is the negative equivalent of eating vitamins every
        // 6 hours.
        external_modifiers -= 50;
    }
    Character::update_health( external_modifiers );
}

void player::check_needs_extremes()
{
    // Check if we've overdosed... in any deadly way.
    if (stim > 250) {
        add_msg_if_player(m_bad, _("You have a sudden heart attack!"));
        add_memorial_log(pgettext("memorial_male", "Died of a drug overdose."),
                           pgettext("memorial_female", "Died of a drug overdose."));
        hp_cur[hp_torso] = 0;
    } else if (stim < -200 || pkill > 240) {
        add_msg_if_player(m_bad, _("Your breathing stops completely."));
        add_memorial_log(pgettext("memorial_male", "Died of a drug overdose."),
                           pgettext("memorial_female", "Died of a drug overdose."));
        hp_cur[hp_torso] = 0;
    } else if (has_effect("jetinjector")) {
            if (get_effect_dur("jetinjector") > 400) {
                if (!(has_trait("NOPAIN"))) {
                    add_msg_if_player(m_bad, _("Your heart spasms painfully and stops."));
                } else {
                    add_msg_if_player(_("Your heart spasms and stops."));
                }
                add_memorial_log(pgettext("memorial_male", "Died of a healing stimulant overdose."),
                                   pgettext("memorial_female", "Died of a healing stimulant overdose."));
                hp_cur[hp_torso] = 0;
            }
    } else if (has_effect("datura") && get_effect_dur("datura") > 14000 && one_in(512)) {
        if (!(has_trait("NOPAIN"))) {
            add_msg_if_player(m_bad, _("Your heart spasms painfully and stops, dragging you back to reality as you die."));
        } else {
            add_msg_if_player(_("You dissolve into beautiful paroxysms of energy.  Life fades from your nebulae and you are no more."));
        }
        add_memorial_log(pgettext("memorial_male", "Died of datura overdose."),
                           pgettext("memorial_female", "Died of datura overdose."));
        hp_cur[hp_torso] = 0;
    }
    // Check if we're starving or have starved
    if( is_player() && get_hunger() >= 3000 ) {
        if (get_hunger() >= 6000) {
            add_msg_if_player(m_bad, _("You have starved to death."));
            add_memorial_log(pgettext("memorial_male", "Died of starvation."),
                               pgettext("memorial_female", "Died of starvation."));
            hp_cur[hp_torso] = 0;
        } else if( get_hunger() >= 5000 && calendar::once_every(MINUTES(2)) ) {
            add_msg_if_player(m_warning, _("Food..."));
        } else if( get_hunger() >= 4000 && calendar::once_every(MINUTES(2)) ) {
            add_msg_if_player(m_warning, _("You are STARVING!"));
        } else if( calendar::once_every(MINUTES(2)) ) {
            add_msg_if_player(m_warning, _("Your stomach feels so empty..."));
        }
    }

    // Check if we're dying of thirst
    if( is_player() && thirst >= 600 ) {
        if( thirst >= 1200 ) {
            add_msg_if_player(m_bad, _("You have died of dehydration."));
            add_memorial_log(pgettext("memorial_male", "Died of thirst."),
                               pgettext("memorial_female", "Died of thirst."));
            hp_cur[hp_torso] = 0;
        } else if( thirst >= 1000 && calendar::once_every(MINUTES(2)) ) {
            add_msg_if_player(m_warning, _("Even your eyes feel dry..."));
        } else if( thirst >= 800 && calendar::once_every(MINUTES(2)) ) {
            add_msg_if_player(m_warning, _("You are THIRSTY!"));
        } else if( calendar::once_every(MINUTES(2)) ) {
            add_msg_if_player(m_warning, _("Your mouth feels so dry..."));
        }
    }

    // Check if we're falling asleep, unless we're sleeping
    if( fatigue >= EXHAUSTED + 25 && !in_sleep_state() ) {
        if( fatigue >= MASSIVE_FATIGUE ) {
            add_msg_if_player(m_bad, _("Survivor sleep now."));
            add_memorial_log(pgettext("memorial_male", "Succumbed to lack of sleep."),
                               pgettext("memorial_female", "Succumbed to lack of sleep."));
            fatigue -= 10;
            try_to_sleep();
        } else if( fatigue >= 800 && calendar::once_every(MINUTES(1)) ) {
            add_msg_if_player(m_warning, _("Anywhere would be a good place to sleep..."));
        } else if( calendar::once_every(MINUTES(5)) ) {
            add_msg_if_player(m_warning, _("You feel like you haven't slept in days."));
        }
    }

    // Even if we're not Exhausted, we really should be feeling lack/sleep earlier
    // Penalties start at Dead Tired and go from there
    if( fatigue >= DEAD_TIRED && !in_sleep_state() ) {
        if( fatigue >= 700 ) {
           if( calendar::once_every(MINUTES(5)) ) {
                add_msg_if_player(m_warning, _("You're too tired to stop yawning."));
                add_effect("lack_sleep", 50);
            }
            ///\EFFECT_INT slightly decreases occurrence of short naps when dead tired
            if( one_in(50 + int_cur) ) {
                // Rivet's idea: look out for microsleeps!
                fall_asleep(5);
            }
        } else if( fatigue >= EXHAUSTED ) {
            if( calendar::once_every(MINUTES(5)) ) {
                add_msg_if_player(m_warning, _("How much longer until bedtime?"));
                add_effect("lack_sleep", 50);
            }
            ///\EFFECT_INT slightly decreases occurrence of short naps when exhausted
            if (one_in(100 + int_cur)) {
                fall_asleep(5);
            }
        } else if (fatigue >= DEAD_TIRED && calendar::once_every(MINUTES(5))) {
            add_msg_if_player(m_warning, _("*yawn* You should really get some sleep."));
            add_effect("lack_sleep", 50);
        }
    }
}

void player::update_needs( int rate_multiplier )
{
    // Hunger, thirst, & fatigue up every 5 minutes
    const bool debug_ls = has_trait("DEBUG_LS");
    const bool has_recycler = has_bionic("bio_recycler");
    const bool asleep = in_sleep_state();
    const bool hibernating = asleep && is_hibernating();
    float hunger_rate = 1.0f;
    if( has_trait("LIGHTEATER") ) {
        hunger_rate -= (1.0f / 3.0f);
    }

    if( has_trait( "HUNGER" ) ) {
        hunger_rate += 0.5f;
    } else if( has_trait( "HUNGER2" ) ) {
        hunger_rate += 1.0f;
    } else if( has_trait( "HUNGER3" ) ) {
        hunger_rate += 2.0f;
    }

    if( has_trait( "MET_RAT" ) ) {
        hunger_rate += (1.0f / 3.0f);
    }

    float thirst_rate = 1.0f;
    if( has_trait("PLANTSKIN") ) {
        thirst_rate -= 0.2f;
    }

    if( has_trait("THIRST") ) {
        thirst_rate += 0.5f;
    } else if( has_trait("THIRST2") ) {
        thirst_rate += 1.0f;
    } else if( has_trait("THIRST3") ) {
        thirst_rate += 2.0f;
    }

    if( has_recycler ) {
        // A LOT! Also works on mutant hunger
        // Should it be so good?
        hunger_rate /= 6.0f;
        thirst_rate /= 2.0f;
    }

    if( asleep && !has_recycler && !hibernating ) {
        // Hunger and thirst advance more slowly while we sleep. This is the standard rate.
        hunger_rate *= 0.5f;
        thirst_rate *= 0.5f;
    } else if( asleep && !has_recycler && hibernating) {
        // Hunger and thirst advance *much* more slowly whilst we hibernate.
        // (int (calendar::turn) % 50 would be zero burn.)
        // Very Thirsty catch deliberately NOT applied here, to fend off Dehydration debuffs
        // until the char wakes.  This was time-trial'd quite thoroughly,so kindly don't "rebalance"
        // without a good explanation and taking a night to make sure it works
        // with the extended sleep duration, OK?
        hunger_rate *= (2.0f / 7.0f);
        thirst_rate *= (2.0f / 7.0f);
    }

    if( !debug_ls && hunger_rate > 0.0f ) {
        const int rolled_hunger = divide_roll_remainder( hunger_rate * rate_multiplier, 1.0 );
        mod_hunger( rolled_hunger );
    }

    if( !debug_ls && thirst_rate > 0.0f ) {
        thirst += divide_roll_remainder( thirst_rate * rate_multiplier, 1.0 );
    }

    // Sanity check for negative fatigue value.
    if( fatigue < -1000 ) {
        fatigue = -1000;
    }

    const bool wasnt_fatigued = fatigue <= DEAD_TIRED;
    // Don't increase fatigue if sleeping or trying to sleep or if we're at the cap.
    if( fatigue < 1050 && !asleep && !debug_ls ) {
        float fatigue_rate = 1.0f;
        // Wakeful folks don't always gain fatigue!
        if (has_trait("WAKEFUL")) {
            fatigue_rate -= (1.0f / 6.0f);
        } else if (has_trait("WAKEFUL2")) {
            fatigue_rate -= 0.25f;
        } else if (has_trait("WAKEFUL3")) {
            // You're looking at over 24 hours to hit Tired here
            fatigue_rate -= 0.5f;
        }
        // Sleepy folks gain fatigue faster; Very Sleepy is twice as fast as typical
        if (has_trait("SLEEPY")) {
            fatigue_rate += (1.0f / 3.0f);
        } else if (has_trait("SLEEPY2")) {
            fatigue_rate += 1.0f;
        }

        if( has_trait("MET_RAT") ) {
            fatigue_rate += 0.5f;
        }

        // Freakishly Huge folks tire quicker
        if( has_trait("HUGE") ) {
            fatigue_rate += (1.0f / 6.0f);
        }

        if( !debug_ls && fatigue_rate > 0.0f ) {
            fatigue += divide_roll_remainder( fatigue_rate * rate_multiplier, 1.0 );
        }
    } else if( has_effect( "sleep" ) ) {
        effect &sleep = get_effect( "sleep" );
        const int intense = sleep.is_null() ? 0 : sleep.get_intensity();
        // Accelerated recovery capped to 2x over 2 hours
        // After 16 hours of activity, equal to 7.25 hours of rest
        const int accelerated_recovery_chance = 24 - intense + 1;
        const float accelerated_recovery_rate = 1.0f / accelerated_recovery_chance;
        float recovery_rate = 1.0f + accelerated_recovery_rate;

        // You fatigue & recover faster with Sleepy
        // Very Sleepy, you just fatigue faster
        if( !hibernating && ( has_trait("SLEEPY") || has_trait("MET_RAT") ) ) {
            recovery_rate += (1.0f + accelerated_recovery_rate) / 2.0f;
        }

        // Tireless folks recover fatigue really fast
        // as well as gaining it really slowly
        // (Doesn't speed healing any, though...)
        if( !hibernating && has_trait("WAKEFUL3") ) {
            recovery_rate += (1.0f + accelerated_recovery_rate) / 2.0f;
        }

        // Untreated pain causes a flat penalty to fatigue reduction
        recovery_rate -= float(pain - pkill) / 60;

        if( recovery_rate > 0.0f ) {
            int recovered = divide_roll_remainder( recovery_rate * rate_multiplier, 1.0 );
            if( fatigue - recovered < -20 ) {
                // Should be wake up, but that could prevent some retroactive regeneration
                sleep.set_duration( 1 );
                fatigue = -25;
            } else {
                fatigue -= recovered;
            }
        }
    }
    if( is_player() && wasnt_fatigued && fatigue > DEAD_TIRED && !in_sleep_state() ) {
        if (activity.type == ACT_NULL) {
            add_msg_if_player(m_warning, _("You're feeling tired.  %s to lie down for sleep."),
                press_x(ACTION_SLEEP).c_str());
        } else {
            g->cancel_activity_query(_("You're feeling tired."));
        }
    }

    if( stim < 0 ) {
        stim = std::min( stim + rate_multiplier, 0 );
    } else if( stim > 0 ) {
        stim = std::max( stim - rate_multiplier, 0 );
    }

    if( pkill < 0 ) {
        pkill = std::min( pkill + rate_multiplier, 0 );
    } else if( pkill > 0 ) {
        pkill = std::max( pkill - rate_multiplier, 0 );
    }

    if( has_bionic("bio_solar") && g->is_in_sunlight( pos() ) ) {
        charge_power( rate_multiplier * 25 );
    }

    if( is_npc() ) {
        // TODO: Remove this `if` once NPCs start eating and getting huge
        return;
    }
    // Huge folks take penalties for cramming themselves in vehicles
    if( (has_trait("HUGE") || has_trait("HUGE_OK")) && in_vehicle ) {
        add_msg_if_player(m_bad, _("You're cramping up from stuffing yourself in this vehicle."));
        pain += 2 * rng(2, 3);
        focus_pool -= 1;
    }

    int dec_stom_food = get_stomach_food() * 0.2;
    int dec_stom_water = get_stomach_water() * 0.2;
    dec_stom_food = dec_stom_food < 10 ? 10 : dec_stom_food;
    dec_stom_water = dec_stom_water < 10 ? 10 : dec_stom_water;
    mod_stomach_food(-dec_stom_food);
    mod_stomach_water(-dec_stom_water);
}

void player::regen( int rate_multiplier )
{
    int pain_ticks = rate_multiplier;
    while( pain > 0 && pain_ticks-- > 0 ) {
        pain -= 1 + int( pain / 10 );
    }

    if( pain < 0 ) {
        pain = 0;
    }

    float heal_rate = 0.0f;
    // Mutation healing effects
    if( has_trait("FASTHEALER2") ) {
        heal_rate += 0.2f;
    } else if( has_trait("REGEN") ) {
        heal_rate += 0.5f;
    }

    if( heal_rate > 0.0f ) {
        int hp_rate = divide_roll_remainder( rate_multiplier * heal_rate, 1.0f );
        healall( hp_rate );
    }

    float hurt_rate = 0.0f;
    if( has_trait("ROT2") ) {
        hurt_rate += 0.2f;
    } else if( has_trait("ROT3") ) {
        hurt_rate += 0.5f;
    }

    if( hurt_rate > 0.0f ) {
        int rot_rate = divide_roll_remainder( rate_multiplier * hurt_rate, 1.0f );
        // Has to be in loop because some effects depend on rounding
        while( rot_rate-- > 0 ) {
            hurtall( 1, nullptr, false );
        }
    }

    if( radiation > 0 ) {
        radiation = std::max( 0, radiation - divide_roll_remainder( rate_multiplier * radiation / 3.0, 1.0f ) );
    }
}

void player::update_stamina( int turns )
{
    int stamina_recovery = 0;
    // Recover some stamina every turn.
    if( !has_effect("winded") ) {
        // But mouth encumberance interferes.
        stamina_recovery += std::max( 1, 10 - (encumb(bp_mouth) / 10) );
        // TODO: recovering stamina causes hunger/thirst/fatigue.
        // TODO: Tiredness slowing recovery
    }

    // stim recovers stamina (or impairs recovery)
    if( stim > 0 ) {
        // TODO: Make stamina recovery with stims cost health
        stamina_recovery += std::min( 5, stim / 20 );
    } else if( stim < 0 ) {
        // Affect it less near 0 and more near full
        // Stamina maxes out around 1000, stims kill at -200
        // At -100 stim fully counters regular regeneration at 500 stamina,
        // halves at 250 stamina, cuts by 25% at 125 stamina
        // At -50 stim fully counters at 1000, halves at 500
        stamina_recovery += stim * stamina / 1000 / 5 ;
    }

    // Cap at max
    stamina = std::min( stamina + stamina_recovery * turns, get_stamina_max() );
    stamina = std::max( stamina, 0 );
}

bool player::is_hibernating() const
{
    // Hibernating only kicks in whilst Engorged; separate tracking for hunger/thirst here
    // as a safety catch.  One test subject managed to get two Colds during hibernation;
    // since those add fatigue and dry out the character, the subject went for the full 10 days plus
    // a little, and came out of it well into Parched.  Hibernating shouldn't endanger your
    // life like that--but since there's much less fluid reserve than food reserve,
    // simply using the same numbers won't work.
    return has_effect("sleep") && get_hunger() <= -60 && thirst <= 80 && has_active_mutation("HIBERNATE");
}

void player::sleep_hp_regen( int rate_multiplier )
{
    float heal_chance = get_healthy() / 400.0f;
    if( has_trait("FASTHEALER") || has_trait("MET_RAT") ) {
        heal_chance += 1.0f;
    } else if (has_trait("FASTHEALER2")) {
        heal_chance += 1.5f;
    } else if (has_trait("REGEN")) {
        heal_chance += 2.0f;
    } else if (has_trait("SLOWHEALER")) {
        heal_chance += 0.13f;
    } else {
        heal_chance += 0.25f;
    }

    if( is_hibernating() ) {
        heal_chance /= 7.0f;
    }

    if( has_trait("FLIMSY") ) {
        heal_chance /= (4.0f / 3.0f);
    } else if( has_trait("FLIMSY2") ) {
        heal_chance /= 2.0f;
    } else if( has_trait("FLIMSY3") ) {
        heal_chance /= 4.0f;
    }

    const int heal_roll = divide_roll_remainder( rate_multiplier * heal_chance, 1.0f );
    if( heal_roll > 0 ) {
        healall( heal_roll );
    }
}

void player::add_addiction(add_type type, int strength)
{
    if (type == ADD_NULL) {
        return;
    }
    int timer = 1200;
    if (has_trait("ADDICTIVE")) {
        strength = int(strength * 1.5);
        timer = 800;
    }
    if (has_trait("NONADDICTIVE")) {
        strength = int(strength * .50);
        timer = 1800;
    }
    //Update existing addiction
    for (auto &i : addictions) {
        if (i.type == type) {
            if (i.sated < 0) {
                i.sated = timer;
            } else if (i.sated < 600) {
                i.sated += timer; // TODO: Make this variable?
            } else {
                i.sated += int((3000 - i.sated) / 2);
            }
            if ((rng(0, strength) > rng(0, i.intensity * 5) || rng(0, 500) < strength) &&
                  i.intensity < 20) {
                i.intensity++;
            }
            return;
        }
    }
    //Add a new addiction
    if (rng(0, 100) < strength) {
        //~ %s is addiction name
        add_memorial_log(pgettext("memorial_male", "Became addicted to %s."),
                            pgettext("memorial_female", "Became addicted to %s."),
                            addiction_type_name(type).c_str());
        addiction tmp(type, 1);
        addictions.push_back(tmp);
    }
}

bool player::has_addiction(add_type type) const
{
    for (auto &i : addictions) {
        if (i.type == type && i.intensity >= MIN_ADDICTION_LEVEL) {
            return true;
        }
    }
    return false;
}

void player::rem_addiction(add_type type)
{
    for (size_t i = 0; i < addictions.size(); i++) {
        if (addictions[i].type == type) {
            //~ %s is addiction name
            if (has_trait("THRESH_MYCUS") && ((type == ADD_MARLOSS_R) || (type == ADD_MARLOSS_B) ||
              (type == ADD_MARLOSS_Y))) {
                  add_memorial_log(pgettext("memorial_male", "Transcended addiction to %s."),
                            pgettext("memorial_female", "Transcended addiction to %s."),
                            addiction_type_name(type).c_str());
            }
            else {
                add_memorial_log(pgettext("memorial_male", "Overcame addiction to %s."),
                            pgettext("memorial_female", "Overcame addiction to %s."),
                            addiction_type_name(type).c_str());
            }
            addictions.erase(addictions.begin() + i);
            return;
        }
    }
}

int player::addiction_level( add_type type ) const
{
    for (auto &i : addictions) {
        if (i.type == type) {
            return i.intensity;
        }
    }
    return 0;
}

bool player::siphon(vehicle *veh, const itype_id &desired_liquid)
{
    int liquid_amount = veh->drain( desired_liquid, veh->fuel_capacity(desired_liquid) );
    item used_item( desired_liquid, calendar::turn );
    const int fuel_per_charge = fuel_charges_to_amount_factor( desired_liquid );
    used_item.charges = liquid_amount / fuel_per_charge;
    if( used_item.charges <= 0 ) {
        add_msg( _( "There is not enough %s left to siphon it." ), used_item.type_name().c_str() );
        veh->refill( desired_liquid, liquid_amount );
        return false;
    }
    int extra = g->move_liquid( used_item );
    if( extra == -1 ) {
        // Failed somehow, put the liquid back and bail out.
        veh->refill( desired_liquid, used_item.charges * fuel_per_charge );
        return false;
    }
    int siphoned = liquid_amount - extra;
    veh->refill( desired_liquid, extra );
    if( siphoned > 0 ) {
        add_msg(ngettext("Siphoned %1$d unit of %2$s from the %3$s.",
                            "Siphoned %1$d units of %2$s from the %3$s.",
                            siphoned),
                   siphoned, used_item.tname().c_str(), veh->name.c_str());
        //Don't consume turns if we decided not to siphon
        return true;
    } else {
        return false;
    }
}

void player::cough(bool harmful, int loudness)
{
    if( !is_npc() ) {
        add_msg(m_bad, _("You cough heavily."));
        sounds::sound(pos(), loudness, "");
    } else {
        sounds::sound(pos(), loudness, _("a hacking cough."));
    }

    moves -= 80;
    if( harmful ) {
        const int stam = stamina;
        mod_stat( "stamina", -100 );
        if( stam < 100 && x_in_y( 100 - stam, 100 ) ) {
            apply_damage( nullptr, bp_torso, 1 );
        }
    }
    if( has_effect("sleep") && ((harmful && one_in(3)) || one_in(10)) ) {
        wake_up();
    }
}

void player::add_pain_msg(int val, body_part bp) const
{
    if (has_trait("NOPAIN")) {
        return;
    }
    if (bp == num_bp) {
        if (val > 20) {
            add_msg_if_player(_("Your body is wracked with excruciating pain!"));
        } else if (val > 10) {
            add_msg_if_player(_("Your body is wracked with terrible pain!"));
        } else if (val > 5) {
            add_msg_if_player(_("Your body is wracked with pain!"));
        } else if (val > 1) {
            add_msg_if_player(_("Your body pains you!"));
        } else {
            add_msg_if_player(_("Your body aches."));
        }
    } else {
        if (val > 20) {
            add_msg_if_player(_("Your %s is wracked with excruciating pain!"),
                                body_part_name_accusative(bp).c_str());
        } else if (val > 10) {
            add_msg_if_player(_("Your %s is wracked with terrible pain!"),
                                body_part_name_accusative(bp).c_str());
        } else if (val > 5) {
            add_msg_if_player(_("Your %s is wracked with pain!"),
                                body_part_name_accusative(bp).c_str());
        } else if (val > 1) {
            add_msg_if_player(_("Your %s pains you!"),
                                body_part_name_accusative(bp).c_str());
        } else {
            add_msg_if_player(_("Your %s aches."),
                                body_part_name_accusative(bp).c_str());
        }
    }
}

void player::print_health() const
{
    if( !is_player() ) {
        return;
    }
    int current_health = get_healthy();
    if( has_trait( "SELFAWARE" ) ) {
        add_msg( "Your current health value is: %d", current_health );
    }
    bool ill = false;
    if( has_effect( "common_cold" ) || has_effect( "flu" ) ) {
        ill = true;
    }
    int roll = rng(0,4);
    std::string message = "";
    if (!ill && current_health > 100) {
        switch (roll) {
        case 0:
            message = _("You feel great! It doesn't seem like wounds could even slow you down for more than a day.");
            break;
        case 1:
            message = _("Within moments you're ready and up. You don't feel like anything could stop you today!");
            break;
        case 2:
            message = _("Your eyes open and your entire body feels like it is just bursting with energy to burn!");
            break;
        case 3:
            message = _("You feel like a rubber ball; whatever hits you, you'll just bounce back!");
            break;
        case 4:
            message = _("You're up and you feel fantastic. No sickness is going to keep you down today!");
            break;
        }
    } else if (!ill && current_health > 50) {
        switch (roll) {
        case 0:
            message = _("You're up and going rather quickly, and all the little aches from yesterday are gone.");
            break;
        case 1:
            message = _("You get up feeling pretty good, as if all your little aches were fading faster.");
            break;
        case 2:
            message = _("Getting up comes easy to you, your muscles revitalized after your rest.");
            break;
        case 3:
            message = _("You're up and your little pains from before seem to have faded away rather quickly.");
            break;
        case 4:
            message = _("Awareness comes fast, your body coming quickly to attention after your rest.");
            break;
        }
    } else if (!ill && current_health > 10) {
        switch (roll) {
        case 0:
            message = _("You feel good. Healthy living does seem to have some rewards.");
            break;
        case 1:
            message = _("Getting out of bed doesn't seem too hard today. You could get used to this!");
            break;
        case 2:
            message = _("Alertness comes somewhat fast, and your muscles stretch easier than before you went to bed.");
            break;
        case 3:
            message = _("You feel extra alert, and your body feels ready to go.");
            break;
        case 4:
            message = _("Your body stretches with ease, and you feel ready to take on the world.");
            break;
        }
    } else if(current_health >= -10) {
        // No message from -10 to 10
    } else if(current_health >= -50) {
        switch (roll) {
        case 0:
            message = _("You feel cruddy. Maybe you should consider eating a bit healthier.");
            break;
        case 1:
            message = _("You get up with a bit of a scratch in your throat. Might be time for some vitamins.");
            break;
        case 2:
            message = _("You stretch, but your muscles don't seem to be doing so good today.");
            break;
        case 3:
            message = _("Your stomach gurgles. It's probably nothing, but maybe you should look into eating something healthy.");
            break;
        case 4:
            message = _("You struggle to awareness. Being awake seems somewhat harder to reach today.");
            break;
        }
    } else if (current_health >= -100) {
        switch (roll) {
        case 0:
            message = _("Getting out of bed only comes with great difficulty, and your muscles resist the movement.");
            break;
        case 1:
            message = _("Getting up seems like it should be easy, but all you want to do is go back to bed.");
            break;
        case 2:
            message = _("Tired hands rub at your eyes, the little aches of yesterday protesting your stretches.");
            break;
        case 3:
            message = _("Alertness seems flighty today, and your body argues when you move towards it.");
            break;
        case 4:
            message = _("You're up, but your body seems like it would rather stay in bed.");
            break;
        }
    } else {
        switch (roll) {
        case 0:
            message = _("You get up feeling horrible, as if something was messing with your body.");
            break;
        case 1:
            message = _("You feel awful, and every ache from yesterday is still there.");
            break;
        case 2:
            message = _("Your eyes struggle to open, and your muscles ache like you didn't sleep at all.");
            break;
        case 3:
            message = _("Bleary-eyed and half-asleep, you consider why you are doing this to yourself.");
            break;
        case 4:
            message = _("Awareness seems to only come with a battle... and your body seem to be on its side.");
            break;
        }
    }
    if (message != "") {
        add_msg((current_health > 0 ? m_good : m_bad), message.c_str());
    }
}

void player::add_eff_effects(effect e, bool reduced)
{
    body_part bp = e.get_bp();
    // Add hurt
    if (e.get_amount("HURT", reduced) > 0) {
        if (bp == num_bp) {
            add_msg_if_player(_("Your %s hurts!"), body_part_name_accusative(bp_torso).c_str());
            apply_damage(nullptr, bp_torso, e.get_mod("HURT"));
        } else {
            add_msg_if_player(_("Your %s hurts!"), body_part_name_accusative(bp).c_str());
            apply_damage(nullptr, bp, e.get_mod("HURT"));
        }
    }
    // Add sleep
    if (e.get_amount("SLEEP", reduced) > 0) {
        add_msg_if_player(_("You pass out!"));
        fall_asleep(e.get_amount("SLEEP"));
    }
    // Add pkill
    if (e.get_amount("PKILL", reduced) > 0) {
        pkill += bound_mod_to_vals(pkill, e.get_amount("PKILL", reduced),
                        e.get_max_val("PKILL", reduced), 0);
    }

    // Add radiation
    if (e.get_amount("RAD", reduced) > 0) {
        radiation += bound_mod_to_vals(radiation, e.get_amount("RAD", reduced),
                        e.get_max_val("RAD", reduced), 0);
    }
    // Add health mod
    if (e.get_amount("H_MOD", reduced) > 0) {
        int bounded = bound_mod_to_vals(get_healthy_mod(), e.get_amount("H_MOD", reduced),
                e.get_max_val("H_MOD", reduced), e.get_min_val("H_MOD", reduced));
        // This already applies bounds, so we pass them through.
        mod_healthy_mod(bounded, get_healthy_mod() + bounded);
    }
    // Add health
    if (e.get_amount("HEALTH", reduced) > 0) {
        mod_healthy(bound_mod_to_vals(get_healthy(), e.get_amount("HEALTH", reduced),
                        e.get_max_val("HEALTH", reduced), e.get_min_val("HEALTH", reduced)));
    }
    // Add stim
    if (e.get_amount("STIM", reduced) > 0) {
        stim += bound_mod_to_vals(stim, e.get_amount("STIM", reduced),
                        e.get_max_val("STIM", reduced), e.get_min_val("STIM", reduced));
    }
    // Add hunger
    if (e.get_amount("HUNGER", reduced) > 0) {
        mod_hunger(bound_mod_to_vals(get_hunger(), e.get_amount("HUNGER", reduced),
                        e.get_max_val("HUNGER", reduced), e.get_min_val("HUNGER", reduced)));
    }
    // Add thirst
    if (e.get_amount("THIRST", reduced) > 0) {
        thirst += bound_mod_to_vals(thirst, e.get_amount("THIRST", reduced),
                        e.get_max_val("THIRST", reduced), e.get_min_val("THIRST", reduced));
    }
    // Add fatigue
    if (e.get_amount("FATIGUE", reduced) > 0) {
        fatigue += bound_mod_to_vals(fatigue, e.get_amount("FATIGUE", reduced),
                        e.get_max_val("FATIGUE", reduced), e.get_min_val("FATIGUE", reduced));
    }
    // Add pain
    if (e.get_amount("PAIN", reduced) > 0) {
        int pain_inc = bound_mod_to_vals(pain, e.get_amount("PAIN", reduced),
                        e.get_max_val("PAIN", reduced), 0);
        mod_pain(pain_inc);
        if (pain_inc > 0) {
            add_pain_msg(e.get_amount("PAIN", reduced), bp);
        }
    }
    Creature::add_eff_effects(e, reduced);
}

void player::process_effects() {
    //Special Removals
    if (has_effect("darkness") && g->is_in_sunlight(pos())) {
        remove_effect("darkness");
    }
    if (has_trait("M_IMMUNE") && has_effect("fungus")) {
        vomit();
        remove_effect("fungus");
        add_msg_if_player(m_bad,  _("We have mistakenly colonized a local guide!  Purging now."));
    }
    if (has_trait("PARAIMMUNE") && (has_effect("dermatik") || has_effect("tapeworm") ||
          has_effect("bloodworms") || has_effect("brainworms") || has_effect("paincysts")) ) {
        remove_effect("dermatik");
        remove_effect("tapeworm");
        remove_effect("bloodworms");
        remove_effect("brainworms");
        remove_effect("paincysts");
        add_msg_if_player(m_good, _("Something writhes and inside of you as it dies."));
    }
    if (has_trait("ACIDBLOOD") && (has_effect("dermatik") || has_effect("bloodworms") ||
          has_effect("brainworms"))) {
        remove_effect("dermatik");
        remove_effect("bloodworms");
        remove_effect("brainworms");
    }
    if (has_trait("EATHEALTH") && has_effect("tapeworm")) {
        remove_effect("tapeworm");
        add_msg_if_player(m_good, _("Your bowels gurgle as something inside them dies."));
    }
    if (has_trait("INFIMMUNE") && (has_effect("bite") || has_effect("infected") ||
          has_effect("recover")) ) {
        remove_effect("bite");
        remove_effect("infected");
        remove_effect("recover");
    }
    if (!(in_sleep_state()) && has_effect("alarm_clock")) {
        remove_effect("alarm_clock");
    }

    //Human only effects
    for( auto &elem : effects ) {
        for( auto &_effect_it : elem.second ) {
            auto &it = _effect_it.second;
            bool reduced = resists_effect(it);
            double mod = 1;
            body_part bp = it.get_bp();
            int val = 0;

            // Still hardcoded stuff, do this first since some modify their other traits
            hardcoded_effects(it);

            // Handle miss messages
            auto msgs = it.get_miss_msgs();
            if (!msgs.empty()) {
                for (auto i : msgs) {
                    add_miss_reason(_(i.first.c_str()), i.second);
                }
            }

            // Handle health mod
            val = it.get_mod("H_MOD", reduced);
            if (val != 0) {
                mod = 1;
                if(it.activated(calendar::turn, "H_MOD", val, reduced, mod)) {
                    int bounded = bound_mod_to_vals(
                            get_healthy_mod(), val, it.get_max_val("H_MOD", reduced),
                            it.get_min_val("H_MOD", reduced));
                    // This already applies bounds, so we pass them through.
                    mod_healthy_mod(bounded, get_healthy_mod() + bounded);
                }
            }

            // Handle health
            val = it.get_mod("HEALTH", reduced);
            if (val != 0) {
                mod = 1;
                if(it.activated(calendar::turn, "HEALTH", val, reduced, mod)) {
                    mod_healthy(bound_mod_to_vals(get_healthy(), val,
                                it.get_max_val("HEALTH", reduced), it.get_min_val("HEALTH", reduced)));
                }
            }

            // Handle stim
            val = it.get_mod("STIM", reduced);
            if (val != 0) {
                mod = 1;
                if(it.activated(calendar::turn, "STIM", val, reduced, mod)) {
                    stim += bound_mod_to_vals(stim, val, it.get_max_val("STIM", reduced),
                                                it.get_min_val("STIM", reduced));
                }
            }

            // Handle hunger
            val = it.get_mod("HUNGER", reduced);
            if (val != 0) {
                mod = 1;
                if(it.activated(calendar::turn, "HUNGER", val, reduced, mod)) {
                    mod_hunger(bound_mod_to_vals(get_hunger(), val, it.get_max_val("HUNGER", reduced),
                                                it.get_min_val("HUNGER", reduced)));
                }
            }

            // Handle thirst
            val = it.get_mod("THIRST", reduced);
            if (val != 0) {
                mod = 1;
                if(it.activated(calendar::turn, "THIRST", val, reduced, mod)) {
                    thirst += bound_mod_to_vals(thirst, val, it.get_max_val("THIRST", reduced),
                                                it.get_min_val("THIRST", reduced));
                }
            }

            // Handle fatigue
            val = it.get_mod("FATIGUE", reduced);
            // Prevent ongoing fatigue effects while asleep.
            // These are meant to change how fast you get tired, not how long you sleep.
            if (val != 0 && !in_sleep_state()) {
                mod = 1;
                if(it.activated(calendar::turn, "FATIGUE", val, reduced, mod)) {
                    fatigue += bound_mod_to_vals(fatigue, val, it.get_max_val("FATIGUE", reduced),
                                                it.get_min_val("FATIGUE", reduced));
                }
            }

            // Handle Radiation
            val = it.get_mod("RAD", reduced);
            if (val != 0) {
                mod = 1;
                if(it.activated(calendar::turn, "RAD", val, reduced, mod)) {
                    radiation += bound_mod_to_vals(radiation, val, it.get_max_val("RAD", reduced), 0);
                    // Radiation can't go negative
                    if (radiation < 0) {
                        radiation = 0;
                    }
                }
            }

            // Handle Pain
            val = it.get_mod("PAIN", reduced);
            if (val != 0) {
                mod = 1;
                if (it.get_sizing("PAIN")) {
                    if (has_trait("FAT")) {
                        mod *= 1.5;
                    }
                    if (has_trait("LARGE") || has_trait("LARGE_OK")) {
                        mod *= 2;
                    }
                    if (has_trait("HUGE") || has_trait("HUGE_OK")) {
                        mod *= 3;
                    }
                }
                if(it.activated(calendar::turn, "PAIN", val, reduced, mod)) {
                    int pain_inc = bound_mod_to_vals(pain, val, it.get_max_val("PAIN", reduced), 0);
                    mod_pain(pain_inc);
                    if (pain_inc > 0) {
                        add_pain_msg(val, bp);
                    }
                }
            }

            // Handle Damage
            val = it.get_mod("HURT", reduced);
            if (val != 0) {
                mod = 1;
                if (it.get_sizing("HURT")) {
                    if (has_trait("FAT")) {
                        mod *= 1.5;
                    }
                    if (has_trait("LARGE") || has_trait("LARGE_OK")) {
                        mod *= 2;
                    }
                    if (has_trait("HUGE") || has_trait("HUGE_OK")) {
                        mod *= 3;
                    }
                }
                if(it.activated(calendar::turn, "HURT", val, reduced, mod)) {
                    if (bp == num_bp) {
                        if (val > 5) {
                            add_msg_if_player(_("Your %s HURTS!"), body_part_name_accusative(bp_torso).c_str());
                        } else {
                            add_msg_if_player(_("Your %s hurts!"), body_part_name_accusative(bp_torso).c_str());
                        }
                        apply_damage(nullptr, bp_torso, val);
                    } else {
                        if (val > 5) {
                            add_msg_if_player(_("Your %s HURTS!"), body_part_name_accusative(bp).c_str());
                        } else {
                            add_msg_if_player(_("Your %s hurts!"), body_part_name_accusative(bp).c_str());
                        }
                        apply_damage(nullptr, bp, val);
                    }
                }
            }

            // Handle Sleep
            val = it.get_mod("SLEEP", reduced);
            if (val != 0) {
                mod = 1;
                if(it.activated(calendar::turn, "SLEEP", val, reduced, mod)) {
                    add_msg_if_player(_("You pass out!"));
                    fall_asleep(val);
                }
            }

            // Handle painkillers
            val = it.get_mod("PKILL", reduced);
            if (val != 0) {
                mod = it.get_addict_mod("PKILL", addiction_level(ADD_PKILLER));
                if(it.activated(calendar::turn, "PKILL", val, reduced, mod)) {
                    pkill += bound_mod_to_vals(pkill, val, it.get_max_val("PKILL", reduced), 0);
                }
            }

            // Handle coughing
            mod = 1;
            val = 0;
            if (it.activated(calendar::turn, "COUGH", val, reduced, mod)) {
                cough(it.get_harmful_cough());
            }

            // Handle vomiting
            mod = vomit_mod();
            val = 0;
            if (it.activated(calendar::turn, "VOMIT", val, reduced, mod)) {
                vomit();
            }

            // Handle stamina
            val = it.get_mod("STAMINA", reduced);
            if (val != 0) {
                mod = 1;
                if(it.activated(calendar::turn, "STAMINA", val, reduced, mod)) {
                    stamina += bound_mod_to_vals( stamina, val,
                                                  it.get_max_val("STAMINA", reduced),
                                                  it.get_min_val("STAMINA", reduced) );
                    if( stamina < 0 ) {
                        // TODO: Make it drain fatigue and/or oxygen?
                        stamina = 0;
                    } else if( stamina > get_stamina_max() ) {
                        stamina = get_stamina_max();
                    }
                }
            }

            // Speed and stats are handled in recalc_speed_bonus and reset_stats respectively
        }
    }

    Creature::process_effects();
}

void player::hardcoded_effects(effect &it)
{
    if( auto buff = ma_buff::from_effect( it ) ) {
        if( buff->is_valid_player( *this ) ) {
            buff->apply_player( *this );
        } else {
            it.set_duration( 0 ); // removes the effect
        }
        return;
    }
    std::string id = it.get_id();
    int start = it.get_start_turn();
    int dur = it.get_duration();
    int intense = it.get_intensity();
    body_part bp = it.get_bp();
    bool sleeping = has_effect("sleep");
    bool msg_trig = one_in(400);
    if (id == "onfire") {
        // TODO: this should be determined by material properties
        if (!has_trait("M_SKIN2")) {
            hurtall(3, nullptr);
        }
        remove_worn_items_with( []( item &tmp ) {
            bool burnVeggy = (tmp.made_of("veggy") || tmp.made_of("paper"));
            bool burnFabric = ((tmp.made_of("cotton") || tmp.made_of("wool")) && one_in(10));
            bool burnPlastic = ((tmp.made_of("plastic")) && one_in(50));
            return burnVeggy || burnFabric || burnPlastic;
        } );
    } else if (id == "spores") {
        // Equivalent to X in 150000 + health * 100
        if ((!has_trait("M_IMMUNE")) && (one_in(100) && x_in_y(intense, 150 + get_healthy() / 10)) ) {
            add_effect("fungus", 1, num_bp, true);
        }
    } else if (id == "fungus") {
        int bonus = get_healthy() / 10 + (resists_effect(it) ? 100 : 0);
        switch (intense) {
        case 1: // First hour symptoms
            if (one_in(160 + bonus)) {
                cough(true);
            }
            if (one_in(100 + bonus)) {
                add_msg_if_player(m_warning, _("You feel nauseous."));
            }
            if (one_in(100 + bonus)) {
                add_msg_if_player(m_warning, _("You smell and taste mushrooms."));
            }
            it.mod_duration(1);
            if (dur > 600) {
                it.mod_intensity(1);
            }
            break;
        case 2: // Five hours of worse symptoms
            if (one_in(600 + bonus * 3)) {
                add_msg_if_player(m_bad,  _("You spasm suddenly!"));
                moves -= 100;
                apply_damage( nullptr, bp_torso, 5 );
            }
            if (x_in_y(vomit_mod(), (800 + bonus * 4)) || one_in(2000 + bonus * 10)) {
                add_msg_player_or_npc(m_bad, _("You vomit a thick, gray goop."),
                                               _("<npcname> vomits a thick, gray goop.") );

                int awfulness = rng(0,70);
                moves = -200;
                mod_hunger(awfulness);
                thirst += awfulness;
                ///\EFFECT_STR decreases damage taken by fungus effect
                apply_damage( nullptr, bp_torso, awfulness / std::max( str_cur, 1 ) ); // can't be healthy
            }
            it.mod_duration(1);
            if (dur > 3600) {
                it.mod_intensity(1);
            }
            break;
        case 3: // Permanent symptoms
            if (one_in(1000 + bonus * 8)) {
                add_msg_player_or_npc(m_bad,  _("You vomit thousands of live spores!"),
                                              _("<npcname> vomits thousands of live spores!") );

                moves = -500;
                for (int i = -1; i <= 1; i++) {
                    for (int j = -1; j <= 1; j++) {
                        if (i == 0 && j == 0) {
                            continue;
                        }

                        tripoint sporep( posx() + i, posy() + j, posz() );
                        g->m.fungalize( sporep, this, 0.25 );
                    }
                }
            // We're fucked
            } else if (one_in(6000 + bonus * 20)) {
                if(hp_cur[hp_arm_l] <= 0 || hp_cur[hp_arm_r] <= 0) {
                    if(hp_cur[hp_arm_l] <= 0 && hp_cur[hp_arm_r] <= 0) {
                        add_msg_player_or_npc(m_bad, _("The flesh on your broken arms bulges. Fungus stalks burst through!"),
                        _("<npcname>'s broken arms bulge. Fungus stalks burst out of the bulges!"));
                    } else {
                        add_msg_player_or_npc(m_bad, _("The flesh on your broken and unbroken arms bulge. Fungus stalks burst through!"),
                        _("<npcname>'s arms bulge. Fungus stalks burst out of the bulges!"));
                    }
                } else {
                    add_msg_player_or_npc(m_bad, _("Your hands bulge. Fungus stalks burst through the bulge!"),
                        _("<npcname>'s hands bulge. Fungus stalks burst through the bulge!"));
                }
                apply_damage( nullptr, bp_arm_l, 999 );
                apply_damage( nullptr, bp_arm_r, 999 );
            }
            break;
        }
    } else if (id == "rat") {
        it.set_intensity(dur / 10);
        if (rng(0, 100) < dur / 10) {
            if (!one_in(5)) {
                mutate_category("MUTCAT_RAT");
                it.mult_duration(.2);
            } else {
                mutate_category("MUTCAT_TROGLOBITE");
                it.mult_duration(.33);
            }
        } else if (rng(0, 100) < dur / 8) {
            if (one_in(3)) {
                vomit();
                it.mod_duration(-10);
            } else {
                add_msg(m_bad, _("You feel nauseous!"));
                it.mod_duration(3);
            }
        }
    } else if (id == "bleed") {
        // Presuming that during the first-aid process you're putting pressure
        // on the wound or otherwise suppressing the flow. (Kits contain either
        // quikclot or bandages per the recipe.)
        if ( one_in(6 / intense) && activity.type != ACT_FIRSTAID ) {
            add_msg_player_or_npc(m_bad, _("You lose some blood."),
                                           _("<npcname> loses some blood.") );
            mod_pain(1);
            apply_damage( nullptr, bp, 1 );
            g->m.add_field( pos(), playerBloodType(), 1, 0 );
        }
    } else if (id == "hallu") {
        // TODO: Redo this to allow for variable durations
        // Time intervals are drawn from the old ones based on 3600 (6-hour) duration.
        static bool puked = false;
        int maxDuration = 3600;
        int comeupTime = int(maxDuration*0.9);
        int noticeTime = int(comeupTime + (maxDuration-comeupTime)/2);
        int peakTime = int(maxDuration*0.8);
        int comedownTime = int(maxDuration*0.3);
        // Baseline
        if (dur == noticeTime) {
            add_msg_if_player(m_warning, _("You feel a little strange."));
        } else if (dur == comeupTime) {
            // Coming up
            if (one_in(2)) {
                add_msg_if_player(m_warning, _("The world takes on a dreamlike quality."));
            } else if (one_in(3)) {
                add_msg_if_player(m_warning, _("You have a sudden nostalgic feeling."));
            } else if (one_in(5)) {
                add_msg_if_player(m_warning, _("Everything around you is starting to breathe."));
            } else {
                add_msg_if_player(m_warning, _("Something feels very, very wrong."));
            }
        } else if (dur > peakTime && dur < comeupTime) {
            if ((one_in(200) || x_in_y(vomit_mod(), 50)) && !puked) {
                add_msg_if_player(m_bad, _("You feel sick to your stomach."));
                mod_hunger(-2);
                if (one_in(6)) {
                    vomit();
                    if (one_in(2)) {
                        // we've vomited enough for now
                        puked = true;
                    }
                }
            }
            if (is_npc() && one_in(200)) {
                const char *npcText;
                switch(rng(1,4)) {
                    case 1:
                        npcText = _("\"I think it's starting to kick in.\"");
                        break;
                    case 2:
                        npcText = _("\"Oh God, what's happening?\"");
                        break;
                    case 3:
                        npcText = _("\"Of course... it's all fractals!\"");
                        break;
                    default:
                        npcText = _("\"Huh?  What was that?\"");
                        break;

                }
                ///\EFFECT_STR_NPC increases volume of hallucination sounds (NEGATIVE)

                ///\EFFECT_INT_NPC decreases volume of hallucination sounds
                int loudness = 20 + str_cur - int_cur;
                loudness = (loudness > 5 ? loudness : 5);
                loudness = (loudness < 30 ? loudness : 30);
                sounds::sound( pos(), loudness, npcText);
            }
        } else if (dur == peakTime) {
            // Visuals start
            add_msg_if_player(m_bad, _("Fractal patterns dance across your vision."));
            add_effect("visuals", peakTime - comedownTime);
        } else if (dur > comedownTime && dur < peakTime) {
            // Full symptoms
            mod_per_bonus(-2);
            mod_int_bonus(-1);
            mod_dex_bonus(-2);
            add_miss_reason(_("Dancing fractals distract you."), 2);
            mod_str_bonus(-1);
            if (one_in(50)) {
                g->spawn_hallucination();
            }
        } else if (dur == comedownTime) {
            if (one_in(42)) {
                add_msg_if_player(_("Everything looks SO boring now."));
            } else {
                add_msg_if_player(_("Things are returning to normal."));
            }
            puked = false;
        }
    } else if (id == "cold") {
        switch(bp) {
        case bp_head:
            switch(intense) {
            case 3:
                mod_int_bonus(-2);
                if (!sleeping && msg_trig) {
                    add_msg_if_player(_("Your thoughts are unclear."));
                }
            case 2:
                mod_int_bonus(-1);
            default:
                break;
            }
            break;
        case bp_mouth:
            switch(intense) {
            case 3:
                mod_per_bonus(-2);
            case 2:
                mod_per_bonus(-1);
                if (!sleeping && msg_trig) {
                    add_msg_if_player(m_bad, _("Your face is stiff from the cold."));
                }
            default:
                break;
            }
            break;
        case bp_torso:
            switch(intense) {
            case 3:
                mod_dex_bonus(-2);
                add_miss_reason(_("You quiver from the cold."), 2);
                if (!sleeping && msg_trig) {
                    add_msg_if_player(m_bad, _("Your torso is freezing cold. You should put on a few more layers."));
                }
            case 2:
                mod_dex_bonus(-2);
                add_miss_reason(_("Your shivering makes you unsteady."), 2);
            }
            break;
        case bp_arm_l:
            switch(intense) {
            case 3:
                mod_dex_bonus(-1);
            case 2:
                mod_dex_bonus(-1);
                add_miss_reason(_("Your left arm trembles from the cold."), 1);
                if (!sleeping && msg_trig && one_in(2)) {
                    add_msg_if_player(m_bad, _("Your left arm is shivering."));
                }
            default:
                break;
            }
            break;
        case bp_arm_r:
            switch(intense) {
            case 3:
                mod_dex_bonus(-1);
            case 2:
                mod_dex_bonus(-1);
                add_miss_reason(_("Your right arm trembles from the cold."), 1);
                if (!sleeping && msg_trig && one_in(2)) {
                    add_msg_if_player(m_bad, _("Your right arm is shivering."));
                }
            default:
                break;
            }
            break;
        case bp_hand_l:
            switch(intense) {
            case 3:
                mod_dex_bonus(-1);
            case 2:
                mod_dex_bonus(-1);
                add_miss_reason(_("Your left hand quivers in the cold."), 1);
                if (!sleeping && msg_trig && one_in(2)) {
                    add_msg_if_player(m_bad, _("Your left hand feels like ice."));
                }
            default:
                break;
            }
            break;
        case bp_hand_r:
            switch(intense) {
            case 3:
                mod_dex_bonus(-1);
            case 2:
                mod_dex_bonus(-1);
                add_miss_reason(_("Your right hand trembles in the cold."), 1);
                if (!sleeping && msg_trig && one_in(2)) {
                    add_msg_if_player(m_bad, _("Your right hand feels like ice."));
                }
            default:
                break;
            }
            break;
        case bp_leg_l:
            switch(intense) {
            case 3:
                mod_dex_bonus(-1);
                add_miss_reason(_("Your legs uncontrollably shake from the cold."), 1);
                mod_str_bonus(-1);
                if (!sleeping && msg_trig && one_in(2)) {
                    add_msg_if_player(m_bad, _("Your left leg trembles against the relentless cold."));
                }
            case 2:
                mod_dex_bonus(-1);
                add_miss_reason(_("Your legs unsteadily shiver against the cold."), 1);
                mod_str_bonus(-1);
            default:
                break;
            }
            break;
        case bp_leg_r:
            switch(intense) {
            case 3:
                mod_dex_bonus(-1);
                mod_str_bonus(-1);
                if (!sleeping && msg_trig && one_in(2)) {
                    add_msg_if_player(m_bad, _("Your right leg trembles against the relentless cold."));
                }
            case 2:
                mod_dex_bonus(-1);
                mod_str_bonus(-1);
            default:
                break;
            }
            break;
        case bp_foot_l:
            switch(intense) {
            case 3:
                mod_dex_bonus(-1);
                add_miss_reason(_("Your left foot is as nimble as a block of ice."), 1);
                mod_str_bonus(-1);
                break;
            case 2:
                mod_dex_bonus(-1);
                add_miss_reason(_("Your freezing left foot messes up your balance."), 1);
                if (!sleeping && msg_trig && one_in(2)) {
                    add_msg_if_player(m_bad, _("Your left foot feels frigid."));
                }
            default:
                break;
            }
            break;
        case bp_foot_r:
            switch(intense) {
            case 3:
                mod_dex_bonus(-1);
                add_miss_reason(_("Your right foot is as nimble as a block of ice."), 1);
                mod_str_bonus(-1);
                break;
            case 2:
                mod_dex_bonus(-1);
                add_miss_reason(_("Your freezing right foot messes up your balance."), 1);
                if (!sleeping && msg_trig && one_in(2)) {
                    add_msg_if_player(m_bad, _("Your right foot feels frigid."));
                }
            default:
                break;
            }
            break;
        default: // Suppress compiler warning [-Wswitch]
            break;
        }
    } else if (id == "hot") {
        switch(bp) {
        case bp_head:
            switch(intense) {
            case 3:
                if (!sleeping && msg_trig) {
                    add_msg_if_player(m_bad, _("Your head is pounding from the heat."));
                }
                // Fall-through
            case 2:
                // Hallucinations handled in game.cpp
                if (one_in(std::min(14500, 15000 - temp_cur[bp_head]))) {
                    vomit();
                }
                if (!sleeping && msg_trig) {
                    add_msg_if_player(m_bad, _("The heat is making you see things."));
                }
                break;
            default: // Suppress compiler warning [-Wswitch]
                break;
            }
            break;
        case bp_torso:
            switch(intense) {
            case 3:
                mod_str_bonus(-1);
                if (!sleeping && msg_trig) {
                    add_msg_if_player(m_bad, _("You are sweating profusely."));
                }
                // Fall-through
            case 2:
                mod_str_bonus(-1);
                break;
            default:
                break;
            }
            break;
        case bp_hand_l:
            switch(intense) {
            case 3:
                mod_dex_bonus(-1);
                // Fall-through
            case 2:
                add_miss_reason(_("Your left hand's too sweaty to grip well."), 1);
                mod_dex_bonus(-1);
            default:
                break;
            }
            break;
        case bp_hand_r:
            switch(intense) {
            case 3:
                mod_dex_bonus(-1);
                // Fall-through
            case 2:
                mod_dex_bonus(-1);
                add_miss_reason(_("Your right hand's too sweaty to grip well."), 1);
                break;
            default:
                break;
            }
            break;
        case bp_leg_l:
            switch (intense) {
            case 3 :
                if (one_in(2)) {
                    if (!sleeping && msg_trig) {
                        add_msg_if_player(m_bad, _("Your left leg is cramping up."));
                    }
                }
                break;
            default:
                break;
            }
            break;
        case bp_leg_r:
            switch (intense) {
            case 3 :
                if (one_in(2)) {
                    if (!sleeping && msg_trig) {
                        add_msg_if_player(m_bad, _("Your right leg is cramping up."));
                    }
                }
                break;
            default:
                break;
            }
            break;
        case bp_foot_l:
            switch (intense) {
            case 3:
                if (one_in(2)) {
                    if (!sleeping && msg_trig) {
                        add_msg_if_player(m_bad, _("Your left foot is swelling in the heat."));
                    }
                }
                break;
            default:
                break;
            }
            break;
        case bp_foot_r:
            switch (intense) {
            case 3:
                if (one_in(2)) {
                    if (!sleeping && msg_trig) {
                        add_msg_if_player(m_bad, _("Your right foot is swelling in the heat."));
                    }
                }
                break;
            default:
                break;
            }
            break;
        default: // Suppress compiler warning [-Wswitch]
            break;
        }
    } else if (id == "frostbite") {
        switch(bp) {
        case bp_hand_l:
        case bp_hand_r:
            switch(intense) {
            case 2:
                add_miss_reason(_("You have trouble grasping with your numb fingers."), 2);
                mod_dex_bonus(-2);
            default:
                break;
            }
            break;
        case bp_foot_l:
        case bp_foot_r:
            switch(intense) {
            case 2:
            case 1:
                if (!sleeping && msg_trig && one_in(2)) {
                    add_msg_if_player(m_bad, _("Your foot has gone numb."));
                }
            default:
                break;
            }
            break;
        case bp_mouth:
            switch(intense) {
            case 2:
                mod_per_bonus(-2);
            case 1:
                mod_per_bonus(-1);
                if (!sleeping && msg_trig) {
                    add_msg_if_player(m_bad, _("Your face feels numb."));
                }
            default:
                break;
            }
            break;
        default: // Suppress compiler warnings [-Wswitch]
            break;
        }
    } else if (id == "dermatik") {
        bool triggered = false;
        int formication_chance = 600;
        if (dur < 2400) {
            formication_chance += 2400 - dur;
        }
        if (one_in(formication_chance)) {
            add_effect("formication", 600, bp);
        }
        if (dur < 14400 && one_in(2400)) {
            vomit();
        }
        if (dur > 14400) {
            // Spawn some larvae!
            // Choose how many insects; more for large characters
            ///\EFFECT_STR_MAX increases number of insects hatched from dermatik infection
            int num_insects = rng(1, std::min(3, str_max / 3));
            apply_damage( nullptr, bp, rng( 2, 4 ) * num_insects );
            // Figure out where they may be placed
            add_msg_player_or_npc( m_bad, _("Your flesh crawls; insects tear through the flesh and begin to emerge!"),
                _("Insects begin to emerge from <npcname>'s skin!") );
            for (int i = posx() - 1; i <= posx() + 1; i++) {
                for (int j = posy() - 1; j <= posy() + 1; j++) {
                    if (num_insects == 0) {
                        break;
                    } else if (i == 0 && j == 0) {
                        continue;
                    }
                    tripoint dest( i, j, posz() );
                    if (g->mon_at( dest ) == -1) {
                        if (g->summon_mon(mon_dermatik_larva, dest)) {
                            monster *grub = g->monster_at(dest);
                            if (one_in(3)) {
                                grub->friendly = -1;
                            }
                        }
                        num_insects--;
                    }
                }
                if (num_insects == 0) {
                    break;
                }
            }
            add_memorial_log(pgettext("memorial_male", "Dermatik eggs hatched."),
                               pgettext("memorial_female", "Dermatik eggs hatched."));
            remove_effect("formication", bp);
            moves -= 600;
            triggered = true;
        }
        if (triggered) {
            // Set ourselves up for removal
            it.set_duration(0);
        } else {
            // Count duration up
            it.mod_duration(1);
        }
    } else if (id == "formication") {
        ///\EFFECT_INT decreases occurence of itching from formication effect
        if (x_in_y(intense, 100 + 50 * get_int())) {
            if (!is_npc()) {
                //~ %s is bodypart in accusative.
                 add_msg(m_warning, _("You start scratching your %s!"),
                                          body_part_name_accusative(bp).c_str());
                 g->cancel_activity();
            } else if (g->u.sees( pos() )) {
                //~ 1$s is NPC name, 2$s is bodypart in accusative.
                add_msg(_("%1$s starts scratching their %2$s!"), name.c_str(),
                                   body_part_name_accusative(bp).c_str());
            }
            moves -= 150;
            apply_damage( nullptr, bp, 1 );
        }
    } else if (id == "evil") {
        // Worn or wielded; diminished effects
        bool lesserEvil = weapon.has_effect_when_wielded( AEP_EVIL ) ||
                          weapon.has_effect_when_carried( AEP_EVIL );
        for( auto &w : worn ) {
            if( w.has_effect_when_worn( AEP_EVIL ) ) {
                lesserEvil = true;
                break;
            }
        }
        if (lesserEvil) {
            // Only minor effects, some even good!
            mod_str_bonus(dur > 4500 ? 10 : int(dur / 450));
            if (dur < 600) {
                mod_dex_bonus(1);
            } else {
                int dex_mod = -(dur > 3600 ? 10 : int((dur - 600) / 300));
                mod_dex_bonus(dex_mod);
                add_miss_reason(_("Why waste your time on that insignificant speck?"), -dex_mod);
            }
            mod_int_bonus(-(dur > 3000 ? 10 : int((dur - 500) / 250)));
            mod_per_bonus(-(dur > 4800 ? 10 : int((dur - 800) / 400)));
        } else {
            // Major effects, all bad.
            mod_str_bonus(-(dur > 5000 ? 10 : int(dur / 500)));
            int dex_mod = -(dur > 6000 ? 10 : int(dur / 600));
            mod_dex_bonus(dex_mod);
            add_miss_reason(_("Why waste your time on that insignificant speck?"), -dex_mod);
            mod_int_bonus(-(dur > 4500 ? 10 : int(dur / 450)));
            mod_per_bonus(-(dur > 4000 ? 10 : int(dur / 400)));
        }
    } else if (id == "attention") {
        if (one_in(100000 / dur) && one_in(100000 / dur) && one_in(250)) {
            tripoint dest( 0, 0, posz() );
            int tries = 0;
            do {
                dest.x = posx() + rng(-4, 4);
                dest.y = posy() + rng(-4, 4);
                tries++;
            } while ((dest == pos() || g->mon_at(dest) != -1) && tries < 10);
            if (tries < 10) {
                if (g->m.move_cost( dest ) == 0) {
                    g->m.make_rubble( dest, f_rubble_rock, true);
                }
                MonsterGroupResult spawn_details = MonsterGroupManager::GetResultFromGroup( mongroup_id( "GROUP_NETHER" ) );
                g->summon_mon(spawn_details.name, dest);
                if (g->u.sees( dest )) {
                    g->cancel_activity_query(_("A monster appears nearby!"));
                    add_msg_if_player(m_warning, _("A portal opens nearby, and a monster crawls through!"));
                }
                it.mult_duration(.25);
            }
        }
    } else if (id == "meth") {
        if (intense == 1) {
            add_miss_reason(_("The bees have started escaping your teeth."), 2);
            if (one_in(150)) {
                add_msg_if_player(m_bad, _("You feel paranoid.  They're watching you."));
                mod_pain(1);
                fatigue += dice(1,6);
            } else if (one_in(500)) {
                add_msg_if_player(m_bad, _("You feel like you need less teeth.  You pull one out, and it is rotten to the core."));
                mod_pain(1);
            } else if (one_in(500)) {
                add_msg_if_player(m_bad, _("You notice a large abscess.  You pick at it."));
                body_part itch = random_body_part(true);
                add_effect("formication", 600, itch);
                mod_pain(1);
            } else if (one_in(500)) {
                add_msg_if_player(m_bad, _("You feel so sick, like you've been poisoned, but you need more.  So much more."));
                vomit();
                fatigue += dice(1,6);
            }
        }
    } else if (id == "teleglow") {
        // Default we get around 300 duration points per teleport (possibly more
        // depending on the source).
        // TODO: Include a chance to teleport to the nether realm.
        // TODO: This with regards to NPCS
        if(!is_player()) {
            // NO, no teleporting around the player because an NPC has teleglow!
            return;
        }
        if (dur > 6000) {
            // 20 teles (no decay; in practice at least 21)
            if (one_in(1000 - ((dur - 6000) / 10))) {
                if (!is_npc()) {
                    add_msg(_("Glowing lights surround you, and you teleport."));
                    add_memorial_log(pgettext("memorial_male", "Spontaneous teleport."),
                                          pgettext("memorial_female", "Spontaneous teleport."));
                }
                g->teleport();
                if (one_in(10)) {
                    // Set ourselves up for removal
                    it.set_duration(0);
                }
            }
            if (one_in(1200 - ((dur - 6000) / 5)) && one_in(20)) {
                if (!is_npc()) {
                    add_msg(m_bad, _("You pass out."));
                }
                fall_asleep(1200);
                if (one_in(6)) {
                    // Set ourselves up for removal
                    it.set_duration(0);
                }
            }
        }
        if (dur > 3600) {
            // 12 teles
            if (one_in(4000 - int(.25 * (dur - 3600)))) {
                tripoint dest( 0, 0, posz() );
                int &x = dest.x;
                int &y = dest.y;
                int tries = 0;
                do {
                    x = posx() + rng(-4, 4);
                    y = posy() + rng(-4, 4);
                    tries++;
                    if (tries >= 10) {
                        break;
                    }
                } while (((x == posx() && y == posy()) || g->mon_at( dest ) != -1));
                if (tries < 10) {
                    if (g->m.move_cost(x, y) == 0) {
                        g->m.make_rubble( tripoint( x, y, posz() ), f_rubble_rock, true);
                    }
                    MonsterGroupResult spawn_details = MonsterGroupManager::GetResultFromGroup( mongroup_id( "GROUP_NETHER" ) );
                    g->summon_mon( spawn_details.name, dest );
                    if (g->u.sees(x, y)) {
                        g->cancel_activity_query(_("A monster appears nearby!"));
                        add_msg(m_warning, _("A portal opens nearby, and a monster crawls through!"));
                    }
                    if (one_in(2)) {
                        // Set ourselves up for removal
                        it.set_duration(0);
                    }
                }
            }
            if (one_in(3500 - int(.25 * (dur - 3600)))) {
                add_msg_if_player(m_bad, _("You shudder suddenly."));
                mutate();
                if (one_in(4)) {
                    // Set ourselves up for removal
                    it.set_duration(0);
                }
            }
        } if (dur > 2400) {
            // 8 teleports
            if (one_in(10000 - dur) && !has_effect("valium")) {
                add_effect("shakes", rng(40, 80));
            }
            if (one_in(12000 - dur)) {
                add_msg_if_player(m_bad, _("Your vision is filled with bright lights..."));
                add_effect("blind", rng(10, 20));
                if (one_in(8)) {
                    // Set ourselves up for removal
                    it.set_duration(0);
                }
            }
            if (one_in(5000) && !has_effect("hallu")) {
                add_effect("hallu", 3600);
                if (one_in(5)) {
                    // Set ourselves up for removal
                    it.set_duration(0);
                }
            }
        }
        if (one_in(4000)) {
            add_msg_if_player(m_bad, _("You're suddenly covered in ectoplasm."));
            add_effect("boomered", 100);
            if (one_in(4)) {
                // Set ourselves up for removal
                it.set_duration(0);
            }
        }
        if (one_in(10000)) {
            if (!has_trait("M_IMMUNE")) {
                add_effect("fungus", 1, num_bp, true);
            } else {
                add_msg_if_player(m_info, _("We have many colonists awaiting passage."));
            }
            // Set ourselves up for removal
            it.set_duration(0);
        }
    } else if (id == "asthma") {
        if (dur > 1200) {
            add_msg_if_player(m_bad, _("Your asthma overcomes you.\nYou asphyxiate."));
            if (is_player()) {
                add_memorial_log(pgettext("memorial_male", "Succumbed to an asthma attack."),
                                  pgettext("memorial_female", "Succumbed to an asthma attack."));
            }
            hurtall(500, nullptr);
        } else if (dur > 700) {
            if (one_in(20)) {
                add_msg_if_player(m_bad, _("You wheeze and gasp for air."));
            }
        }
    } else if (id == "stemcell_treatment") {
        // slightly repair broken limbs. (also nonbroken limbs (unless they're too healthy))
        for (int i = 0; i < num_hp_parts; i++) {
            if (one_in(6)) {
                if (hp_cur[i] < rng(0, 40)) {
                    add_msg_if_player(m_good, _("Your bones feel like rubber as they melt and remend."));
                    hp_cur[i]+= rng(1,8);
                } else if (hp_cur[i] > rng(10, 2000)) {
                    add_msg_if_player(m_bad, _("Your bones feel like they're crumbling."));
                    hp_cur[i] -= rng(0,8);
                }
            }
        }
    } else if (id == "brainworms") {
        if (one_in(256)) {
            add_msg_if_player(m_bad, _("Your head aches faintly."));
        }
        if(one_in(1024)) {
            mod_healthy_mod(-10, -100);
            apply_damage( nullptr, bp_head, rng( 0, 1 ) );
            if (!has_effect("visuals")) {
                add_msg_if_player(m_bad, _("Your vision is getting fuzzy."));
                add_effect("visuals", rng(10, 600));
            }
        }
        if(one_in(4096)) {
            mod_healthy_mod(-10, -100);
            apply_damage( nullptr, bp_head, rng( 1, 2 ) );
            if (!has_effect("blind")) {
                add_msg_if_player(m_bad, _("Your vision goes black!"));
                add_effect("blind", rng(5, 20));
            }
        }
    } else if (id == "tapeworm") {
        if (one_in(512)) {
            add_msg_if_player(m_bad, _("Your bowels ache."));
        }
    } else if (id == "bloodworms") {
        if (one_in(512)) {
            add_msg_if_player(m_bad, _("Your veins itch."));
        }
    } else if (id == "paincysts") {
        if (one_in(512)) {
            add_msg_if_player(m_bad, _("Your muscles feel like they're knotted and tired."));
        }
    } else if (id == "tetanus") {
        if (one_in(256)) {
            add_msg_if_player(m_bad, _("Your muscles are tight and sore."));
        }
        if (!has_effect("valium")) {
            add_miss_reason(_("Your muscles are locking up and you can't fight effectively."), 4);
            if (one_in(512)) {
                add_msg_if_player(m_bad, _("Your muscles spasm."));
                add_effect("downed", rng( 1,4 ), num_bp, false, 0, true );
                add_effect("stunned",rng(1,4));
                if (one_in(10)) {
                    mod_pain(rng(1, 10));
                }
            }
        }
    } else if (id == "datura") {
        if (has_effect("asthma")) {
            add_msg_if_player(m_good, _("You can breathe again!"));
            remove_effect("asthma");
        }
        if (dur > 1000 && focus_pool >= 1 && one_in(4)) {
            focus_pool--;
        }
        if (dur > 2000 && one_in(8) && stim < 20) {
            stim++;
        }
        if (dur > 3000 && focus_pool >= 1 && one_in(2)) {
            focus_pool--;
        }
        if (dur > 4000 && one_in(64)) {
            mod_pain(rng(-1, -8));
        }
        if ((!has_effect("hallu")) && (dur > 5000 && one_in(4))) {
            add_effect("hallu", 3600);
        }
        if (dur > 6000 && one_in(128)) {
            mod_pain(rng(-3, -24));
            if (dur > 8000 && one_in(16)) {
                add_msg_if_player(m_bad, _("You're experiencing loss of basic motor skills and blurred vision.  Your mind recoils in horror, unable to communicate with your spinal column."));
                add_msg_if_player(m_bad, _("You stagger and fall!"));
                add_effect("downed", rng( 1, 4 ), num_bp, false, 0, true );
                if (one_in(8) || x_in_y(vomit_mod(), 10)) {
                    vomit();
                }
            }
        }
        if (dur > 7000 && focus_pool >= 1) {
            focus_pool--;
        }
        if (dur > 8000 && one_in(256)) {
            add_effect("visuals", rng(40, 200));
            mod_pain(rng(-8, -40));
        }
        if (dur > 12000 && one_in(256)) {
            add_msg_if_player(m_bad, _("There's some kind of big machine in the sky."));
            add_effect("visuals", rng(80, 400));
            if (one_in(32)) {
                add_msg_if_player(m_bad, _("It's some kind of electric snake, coming right at you!"));
                mod_pain(rng(4, 40));
                vomit();
            }
        }
        if (dur > 14000 && one_in(128)) {
            add_msg_if_player(m_bad, _("Order us some golf shoes, otherwise we'll never get out of this place alive."));
            add_effect("visuals", rng(400, 2000));
            if (one_in(8)) {
                add_msg_if_player(m_bad, _("The possibility of physical and mental collapse is now very real."));
                if (one_in(2) || x_in_y(vomit_mod(), 10)) {
                    add_msg_if_player(m_bad, _("No one should be asked to handle this trip."));
                    vomit();
                    mod_pain(rng(8, 40));
                }
            }
        }
    } else if (id == "grabbed") {
        blocks_left -= 1;
        dodges_left = 0;
        int zed_number = 0;
        for( auto &dest : g->m.points_in_radius( pos(), 1, 0 ) ){
            if (g->mon_at(dest) != -1){
                zed_number ++;
            }
        }
        if (zed_number > 0){
            add_effect("grabbed", 2, bp_torso, false, (intense + zed_number) / 2); //If intensity isn't pass the cap, average it with # of zeds
        }
    } else if (id == "impaled") {
        blocks_left -= 2;
        dodges_left = 0;
        // Set ourselves up for removal
        it.set_duration(0);
    } else if (id == "bite") {
        bool recovered = false;
        /* Recovery chances, use binomial distributions if balancing here. Healing in the bite
         * stage provides additional benefits, so both the bite stage chance of healing and
         * the cumulative chances for spontaneous healing are both given.
         * Cumulative heal chances for the bite + infection stages:
         * -200 health - 38.6%
         *    0 health - 46.8%
         *  200 health - 53.7%
         *
         * Heal chances in the bite stage:
         * -200 health - 23.4%
         *    0 health - 28.3%
         *  200 health - 32.9%
         *
         * Cumulative heal chances the bite + infection stages with the resistant mutation:
         * -200 health - 82.6%
         *    0 health - 84.5%
         *  200 health - 86.1%
         *
         * Heal chances in the bite stage with the resistant mutation:
         * -200 health - 60.7%
         *    0 health - 63.2%
         *  200 health - 65.6%
         */
        if (dur % 10 == 0)  {
            int recover_factor = 100;
            if (has_effect("recover")) {
                recover_factor -= get_effect_dur("recover") / 600;
            }
            if (has_trait("INFRESIST")) {
                recover_factor += 200;
            }
            recover_factor += get_healthy() / 10;

            if (x_in_y(recover_factor, 108000)) {
                //~ %s is bodypart name.
                add_msg_if_player(m_good, _("Your %s wound begins to feel better."),
                                     body_part_name(bp).c_str());
                // Set ourselves up for removal
                it.set_duration(0);
                recovered = true;
            }
        }
        if (!recovered) {
            // Move up to infection
            if (dur > 3600) {
                add_effect("infected", 1, bp, true);
                // Set ourselves up for removal
                it.set_duration(0);
            } else {
                it.mod_duration(1);
            }
        }
    } else if (id == "infected") {
        bool recovered = false;
        // Recovery chance, use binomial distributions if balancing here.
        // See "bite" for balancing notes on this.
        if (dur % 10 == 0)  {
            int recover_factor = 100;
            if (has_effect("recover")) {
                recover_factor -= get_effect_dur("recover") / 600;
            }
            if (has_trait("INFRESIST")) {
                recover_factor += 200;
            }
            recover_factor += get_healthy() / 10;

            if (x_in_y(recover_factor, 864000)) {
                //~ %s is bodypart name.
                add_msg_if_player(m_good, _("Your %s wound begins to feel better."),
                                     body_part_name(bp).c_str());
                add_effect("recover", 4 * dur);
                // Set ourselves up for removal
                it.set_duration(0);
                recovered = true;
            }
        }
        if (!recovered) {
            // Death happens
            if (dur > 14400) {
                add_msg_if_player(m_bad, _("You succumb to the infection."));
                add_memorial_log(pgettext("memorial_male", "Succumbed to the infection."),
                                      pgettext("memorial_female", "Succumbed to the infection."));
                hurtall(500, nullptr);
            }
            it.mod_duration(1);
        }
    } else if (id == "lying_down") {
        set_moves(0);
        if (can_sleep()) {
            add_msg_if_player(_("You fall asleep."));
            // Communicate to the player that he is using items on the floor
            std::string item_name = is_snuggling();
            if (item_name == "many") {
                if (one_in(15) ) {
                    add_msg_if_player(_("You nestle your pile of clothes for warmth."));
                } else {
                    add_msg_if_player(_("You use your pile of clothes for warmth."));
                }
            } else if (item_name != "nothing") {
                if (one_in(15)) {
                    add_msg_if_player(_("You snuggle your %s to keep warm."), item_name.c_str());
                } else {
                    add_msg_if_player(_("You use your %s to keep warm."), item_name.c_str());
                }
            }
            if( has_active_mutation("HIBERNATE") && get_hunger() < -60 ) {
                add_memorial_log(pgettext("memorial_male", "Entered hibernation."),
                                   pgettext("memorial_female", "Entered hibernation."));
                // 10 days' worth of round-the-clock Snooze.  Cata seasons default to 14 days.
                fall_asleep(144000);
                // If you're not fatigued enough for 10 days, you won't sleep the whole thing.
                // In practice, the fatigue from filling the tank from (no msg) to Time For Bed
                // will last about 8 days.
            }

            fall_asleep(6000); //10 hours, default max sleep time.
            // Set ourselves up for removal
            it.set_duration(0);
        }
        if (dur == 1 && !sleeping) {
            add_msg_if_player(_("You try to sleep, but can't..."));
        }
    } else if (id == "sleep") {
        set_moves(0);
        #ifdef TILES
        if( is_player() && calendar::once_every(MINUTES(10)) ) {
            SDL_PumpEvents();
        }
        #endif // TILES

        if( intense < 24 ) {
            it.mod_intensity(1);
        } else if( intense < 1 ) {
            it.set_intensity(1);
        }

        if( fatigue < -25 && it.get_duration() > 30 ) {
            it.set_duration(dice(3, 10));
        }

        if( fatigue <= 0 && fatigue > -20 ) {
            fatigue = -25;
            add_msg_if_player(m_good, _("You feel well rested."));
            it.set_duration(dice(3, 100));
        }

        // TODO: Move this to update_needs when NPCs can mutate
        if( calendar::once_every(MINUTES(10)) && has_trait("CHLOROMORPH") &&
            g->is_in_sunlight(pos()) ) {
            // Hunger and thirst fall before your Chloromorphic physiology!
            if (get_hunger() >= -30) {
                mod_hunger(-5);
            }
            if (thirst >= -30) {
                thirst -= 5;
            }
        }

        // Check mutation category strengths to see if we're mutated enough to get a dream
        std::string highcat = get_highest_category();
        int highest = mutation_category_level[highcat];

        // Determine the strength of effects or dreams based upon category strength
        int strength = 0; // Category too weak for any effect or dream
        if (crossed_threshold()) {
            strength = 4; // Post-human.
        } else if (highest >= 20 && highest < 35) {
            strength = 1; // Low strength
        } else if (highest >= 35 && highest < 50) {
            strength = 2; // Medium strength
        } else if (highest >= 50) {
            strength = 3; // High strength
        }

        // Get a dream if category strength is high enough.
        if (strength != 0) {
            //Once every 6 / 3 / 2 hours, with a bit of randomness
            if ((int(calendar::turn) % (3600 / strength) == 0) && one_in(3)) {
                // Select a dream
                std::string dream = get_category_dream(highcat, strength);
                if( !dream.empty() ) {
                    add_msg_if_player( "%s", dream.c_str() );
                }
                // Mycus folks upgrade in their sleep.
                if (has_trait("THRESH_MYCUS")) {
                    if (one_in(8)) {
                        mutate_category("MUTCAT_MYCUS");
                        mod_hunger(10);
                        fatigue += 5;
                        thirst += 10;
                    }
                }
            }
        }

        bool woke_up = false;
        int tirednessVal = rng(5, 200) + rng(0, abs(fatigue * 2 * 5));
        if (!has_effect("blind") && !worn_with_flag("BLIND") && !has_active_bionic("bio_blindfold")) {
            if (has_trait("HEAVYSLEEPER2") && !has_trait("HIBERNATE")) {
                // So you can too sleep through noon
                if ((tirednessVal * 1.25) < g->m.ambient_light_at(pos()) && (fatigue < 10 || one_in(fatigue / 2))) {
                    add_msg_if_player(_("It's too bright to sleep."));
                    // Set ourselves up for removal
                    it.set_duration(0);
                    woke_up = true;
                }
             // Ursine hibernators would likely do so indoors.  Plants, though, might be in the sun.
            } else if (has_trait("HIBERNATE")) {
                if ((tirednessVal * 5) < g->m.ambient_light_at(pos()) && (fatigue < 10 || one_in(fatigue / 2))) {
                    add_msg_if_player(_("It's too bright to sleep."));
                    // Set ourselves up for removal
                    it.set_duration(0);
                    woke_up = true;
                }
            } else if (tirednessVal < g->m.ambient_light_at(pos()) && (fatigue < 10 || one_in(fatigue / 2))) {
                add_msg_if_player(_("It's too bright to sleep."));
                // Set ourselves up for removal
                it.set_duration(0);
                woke_up = true;
            }
        }

        // Have we already woken up?
        if (!woke_up) {
            // Cold or heat may wake you up.
            // Player will sleep through cold or heat if fatigued enough
            for (int i = 0 ; i < num_bp ; i++) {
                if (temp_cur[i] < BODYTEMP_VERY_COLD - fatigue / 2) {
                    if (one_in(5000)) {
                        add_msg_if_player(_("You toss and turn trying to keep warm."));
                    }
                    if (temp_cur[i] < BODYTEMP_FREEZING - fatigue / 2 ||
                                        one_in(temp_cur[i] + 5000)) {
                        add_msg_if_player(m_bad, _("It's too cold to sleep."));
                        // Set ourselves up for removal
                        it.set_duration(0);
                        woke_up = true;
                        break;
                    }
                } else if (temp_cur[i] > BODYTEMP_VERY_HOT + fatigue / 2) {
                    if (one_in(5000)) {
                        add_msg_if_player(_("You toss and turn in the heat."));
                    }
                    if (temp_cur[i] > BODYTEMP_SCORCHING + fatigue / 2 ||
                                        one_in(15000 - temp_cur[i])) {
                        add_msg_if_player(m_bad, _("It's too hot to sleep."));
                        // Set ourselves up for removal
                        it.set_duration(0);
                        woke_up = true;
                        break;
                    }
                }
            }
        }

        // A bit of a hack: check if we are about to wake up for any reason,
        // including regular timing out of sleep
        if( (it.get_duration() == 1 || woke_up) && calendar::turn - start > HOURS(2) ) {
            print_health();
        }
    } else if (id == "alarm_clock") {
        if (has_effect("sleep")) {
            if (dur == 1) {
                if(has_bionic("bio_watch")) {
                    // Normal alarm is volume 12, tested against (2/3/6)d15 for
                    // normal/HEAVYSLEEPER/HEAVYSLEEPER2.
                    //
                    // It's much harder to ignore an alarm inside your own skull,
                    // so this uses an effective volume of 20.
                    const int volume = 20;
                    if ( (!(has_trait("HEAVYSLEEPER") || has_trait("HEAVYSLEEPER2")) &&
                          dice(2, 15) < volume) ||
                          (has_trait("HEAVYSLEEPER") && dice(3, 15) < volume) ||
                          (has_trait("HEAVYSLEEPER2") && dice(6, 15) < volume) ) {
                        wake_up();
                        add_msg_if_player(_("Your internal chronometer wakes you up."));
                    } else {
                        // 10 minute cyber-snooze
                        it.mod_duration(100);
                    }
                } else {
                    sounds::sound( pos(), 16, _("beep-beep-beep!"));
                    if( !can_hear( pos(), 16 ) ) {
                        // 10 minute automatic snooze
                        it.mod_duration(100);
                    } else {
                        add_msg_if_player(_("You turn off your alarm-clock."));
                    }
                }
            }
        }
    }
}

double player::vomit_mod()
{
    double mod = 1;
    if (has_effect("weed_high")) {
        mod *= .1;
    }
    if (has_trait("STRONGSTOMACH")) {
        mod *= .5;
    }
    if (has_trait("WEAKSTOMACH")) {
        mod *= 2;
    }
    if (has_trait("NAUSEA")) {
        mod *= 3;
    }
    if (has_trait("VOMITOUS")) {
        mod *= 3;
    }

    return mod;
}

void player::suffer()
{
    for (size_t i = 0; i < my_bionics.size(); i++) {
        if (my_bionics[i].powered) {
            process_bionic(i);
        }
    }

    for( auto &mut : my_mutations ) {
        auto &tdata = mut.second;
        if (!tdata.powered ) {
            continue;
        }
        const auto &mdata = mutation_branch::get( mut.first );
        if (tdata.powered && tdata.charge > 0) {
        // Already-on units just lose a bit of charge
        tdata.charge--;
        } else {
            // Not-on units, or those with zero charge, have to pay the power cost
            if (mdata.cooldown > 0) {
                tdata.powered = true;
                tdata.charge = mdata.cooldown - 1;
            }
            if (mdata.hunger){
                mod_hunger(mdata.cost);
                if (get_hunger() >= 700) { // Well into Famished
                    add_msg(m_warning, _("You're too famished to keep your %s going."), mdata.name.c_str());
                    tdata.powered = false;
                }
            }
            if (mdata.thirst){
                thirst += mdata.cost;
                if (thirst >= 260) { // Well into Dehydrated
                    add_msg(m_warning, _("You're too dehydrated to keep your %s going."), mdata.name.c_str());
                    tdata.powered = false;
                }
            }
            if (mdata.fatigue){
                fatigue += mdata.cost;
                if (fatigue >= EXHAUSTED) { // Exhausted
                    add_msg(m_warning, _("You're too exhausted to keep your %s going."), mdata.name.c_str());
                    tdata.powered = false;
                }
            }

            if (tdata.powered == false) {
                apply_mods(mut.first, false);
            }
        }
    }

    if (underwater) {
        if (!has_trait("GILLS") && !has_trait("GILLS_CEPH")) {
            oxygen--;
        }
        if (oxygen < 12 && worn_with_flag("REBREATHER")) {
                oxygen += 12;
            }
        if (oxygen <= 5) {
            if (has_bionic("bio_gills") && power_level >= 25) {
                oxygen += 5;
                charge_power(-25);
            } else {
                add_msg(m_bad, _("You're drowning!"));
                apply_damage( nullptr, bp_torso, rng( 1, 4 ) );
            }
        }
    }

    if(has_active_mutation("WINGS_INSECT")){
        //~Sound of buzzing Insect Wings
        sounds::sound( pos(), 10, _("BZZZZZ"));
    }

    double shoe_factor = footwear_factor();
    if( has_trait("ROOTS3") && g->m.has_flag("DIGGABLE", posx(), posy()) && !shoe_factor) {
        if (one_in(100)) {
            add_msg(m_good, _("This soil is delicious!"));
            if (get_hunger() > -20) {
                mod_hunger(-2);
            }
            if (thirst > -20) {
                thirst -= 2;
            }
            mod_healthy_mod(10, 50);
            // No losing oneself in the fertile embrace of rich
            // New England loam.  But it can be a near thing.
            ///\EFFECT_INT decreases chance of losing focus points while eating soil with ROOTS3
            if ( (one_in(int_cur)) && (focus_pool >= 25) ) {
                focus_pool--;
            }
        } else if (one_in(50)){
            if (get_hunger() > -20) {
                mod_hunger(-1);
            }
            if (thirst > -20) {
                thirst--;
            }
            mod_healthy_mod(5, 50);
        }
    }

    if (!in_sleep_state()) {
        if (weight_carried() > weight_capacity()) {
            // Starts at 1 in 25, goes down by 5 for every 50% more carried
            if (one_in(35 - 5 * weight_carried() / (weight_capacity() / 2))) {
                add_msg_if_player(m_bad, _("Your body strains under the weight!"));
                // 1 more pain for every 800 grams more (5 per extra STR needed)
                if ( ((weight_carried() - weight_capacity()) / 800 > pain && pain < 100)) {
                    mod_pain(1);
                }
            }
        }
        if (weight_carried() > 4 * weight_capacity()) {
            if (has_effect("downed")) {
                add_effect("downed", 1, num_bp, false, 0, true );
            } else {
                add_effect("downed", 2, num_bp, false, 0, true );
            }
        }
        int timer = -3600;
        if (has_trait("ADDICTIVE")) {
            timer = -4000;
        }
        if (has_trait("NONADDICTIVE")) {
            timer = -3200;
        }
        for (size_t i = 0; i < addictions.size(); i++) {
            if (addictions[i].sated <= 0 &&
                addictions[i].intensity >= MIN_ADDICTION_LEVEL) {
                addict_effect(*this, addictions[i], [&](char const *const msg) {
                    if (msg) {
                        g->cancel_activity_query(msg);
                    } else {
                        g->cancel_activity();
                    }
                });
            }
            addictions[i].sated--;
            if (!one_in(addictions[i].intensity - 2) && addictions[i].sated > 0) {
                addictions[i].sated -= 1;
            }
            if (addictions[i].sated < timer - (100 * addictions[i].intensity)) {
                if (addictions[i].intensity <= 2) {
                    addictions.erase(addictions.begin() + i);
                    i--;
                } else {
                    addictions[i].intensity = int(addictions[i].intensity / 2);
                    addictions[i].intensity--;
                    addictions[i].sated = 0;
                }
            }
        }
        if (has_trait("CHEMIMBALANCE")) {
            if (one_in(3600) && (!(has_trait("NOPAIN")))) {
                add_msg(m_bad, _("You suddenly feel sharp pain for no reason."));
                mod_pain( 3 * rng(1, 3) );
            }
            if (one_in(3600)) {
                int pkilladd = 5 * rng(-1, 2);
                if (pkilladd > 0) {
                    add_msg(m_bad, _("You suddenly feel numb."));
                } else if ((pkilladd < 0) && (!(has_trait("NOPAIN")))) {
                    add_msg(m_bad, _("You suddenly ache."));
                }
                pkill += pkilladd;
            }
            if (one_in(3600)) {
                add_msg(m_bad, _("You feel dizzy for a moment."));
                moves -= rng(10, 30);
            }
            if (one_in(3600)) {
                int hungadd = 5 * rng(-1, 3);
                if (hungadd > 0) {
                    add_msg(m_bad, _("You suddenly feel hungry."));
                } else {
                    add_msg(m_good, _("You suddenly feel a little full."));
                }
                mod_hunger(hungadd);
            }
            if (one_in(3600)) {
                add_msg(m_bad, _("You suddenly feel thirsty."));
                thirst += 5 * rng(1, 3);
            }
            if (one_in(3600)) {
                add_msg(m_good, _("You feel fatigued all of a sudden."));
                fatigue += 10 * rng(2, 4);
            }
            if (one_in(4800)) {
                if (one_in(3)) {
                    add_morale(MORALE_FEELING_GOOD, 20, 100);
                } else {
                    add_morale(MORALE_FEELING_BAD, -20, -100);
                }
            }
            if (one_in(3600)) {
                if (one_in(3)) {
                    add_msg(m_bad, _("You suddenly feel very cold."));
                    for (int i = 0 ; i < num_bp ; i++) {
                        temp_cur[i] = BODYTEMP_VERY_COLD;
                    }
                } else {
                    add_msg(m_bad, _("You suddenly feel cold."));
                    for (int i = 0 ; i < num_bp ; i++) {
                        temp_cur[i] = BODYTEMP_COLD;
                    }
                }
            }
            if (one_in(3600)) {
                if (one_in(3)) {
                    add_msg(m_bad, _("You suddenly feel very hot."));
                    for (int i = 0 ; i < num_bp ; i++) {
                        temp_cur[i] = BODYTEMP_VERY_HOT;
                    }
                } else {
                    add_msg(m_bad, _("You suddenly feel hot."));
                    for (int i = 0 ; i < num_bp ; i++) {
                        temp_cur[i] = BODYTEMP_HOT;
                    }
                }
            }
        }
        if ((has_trait("SCHIZOPHRENIC") || has_artifact_with(AEP_SCHIZO)) &&
            one_in(2400)) { // Every 4 hours or so
            monster phantasm;
            int i;
            switch(rng(0, 11)) {
                case 0:
                    add_effect("hallu", 3600);
                    break;
                case 1:
                    add_effect("visuals", rng(15, 60));
                    break;
                case 2:
                    add_msg(m_warning, _("From the south you hear glass breaking."));
                    break;
                case 3:
                    add_msg(m_warning, _("YOU SHOULD QUIT THE GAME IMMEDIATELY."));
                    add_morale(MORALE_FEELING_BAD, -50, -150);
                    break;
                case 4:
                    for (i = 0; i < 10; i++) {
                        add_msg("XXXXXXXXXXXXXXXXXXXXXXXXXXX");
                    }
                    break;
                case 5:
                    add_msg(m_bad, _("You suddenly feel so numb..."));
                    pkill += 25;
                    break;
                case 6:
                    add_msg(m_bad, _("You start to shake uncontrollably."));
                    add_effect("shakes", 10 * rng(2, 5));
                    break;
                case 7:
                    for (i = 0; i < 10; i++) {
                        g->spawn_hallucination();
                    }
                    break;
                case 8:
                    add_msg(m_bad, _("It's a good time to lie down and sleep."));
                    add_effect("lying_down", 200);
                    break;
                case 9:
                    add_msg(m_bad, _("You have the sudden urge to SCREAM!"));
                    shout(_("AHHHHHHH!"));
                    break;
                case 10:
                    add_msg(std::string(name + name + name + name + name + name + name +
                        name + name + name + name + name + name + name +
                        name + name + name + name + name + name).c_str());
                    break;
                case 11:
                    body_part bp = random_body_part(true);
                    add_effect("formication", 600, bp);
                    break;
            }
        }
        if (has_trait("JITTERY") && !has_effect("shakes")) {
            if (stim > 50 && one_in(300 - stim)) {
                add_effect("shakes", 300 + stim);
            } else if (get_hunger() > 80 && one_in(500 - get_hunger())) {
                add_effect("shakes", 400);
            }
        }

        if (has_trait("MOODSWINGS") && one_in(3600)) {
            if (rng(1, 20) > 9) { // 55% chance
                add_morale(MORALE_MOODSWING, -100, -500);
            } else {  // 45% chance
                add_morale(MORALE_MOODSWING, 100, 500);
            }
        }

        if (has_trait("VOMITOUS") && one_in(4200)) {
            vomit();
        }

        if (has_trait("SHOUT1") && one_in(3600)) {
            shout();
        }
        if (has_trait("SHOUT2") && one_in(2400)) {
            shout();
        }
        if (has_trait("SHOUT3") && one_in(1800)) {
            shout();
        }
        if (has_trait("M_SPORES") && one_in(2400)) {
            spores();
        }
        if (has_trait("M_BLOSSOMS") && one_in(1800)) {
            blossoms();
        }
    } // Done with while-awake-only effects

    if (has_trait("ASTHMA") && one_in(3600 - stim * 50)) {
        bool auto_use = has_charges("inhaler", 1);
        if (underwater) {
            oxygen = int(oxygen / 2);
            auto_use = false;
        }

        if (has_effect("sleep")) {
            add_msg_if_player(_("You have an asthma attack!"));
            wake_up();
            auto_use = false;
        } else {
            add_msg_if_player( m_bad, _( "You have an asthma attack!" ) );
        }

        if (auto_use) {
            use_charges("inhaler", 1);
            moves -= 40;
            const auto charges = charges_of( "inhaler" );
            if( charges == 0 ) {
                add_msg_if_player( m_bad, _( "You use your last inhaler charge." ) );
            } else {
                add_msg_if_player( m_info, ngettext( "You use your inhaler, only %d charge left.",
                                                     "You use your inhaler, only %d charges left.", charges ),
                                   charges );
            }
        } else {
            add_effect("asthma", 50 * rng(1, 4));
            if (!is_npc()) {
                g->cancel_activity_query(_("You have an asthma attack!"));
            }
        }
    }

    if (has_trait("LEAVES") && g->is_in_sunlight(pos()) && one_in(600)) {
        mod_hunger(-1);
    }

    if (pain > 0) {
        if (has_trait("PAINREC1") && one_in(600)) {
            pain--;
        }
        if (has_trait("PAINREC2") && one_in(300)) {
            pain--;
        }
        if (has_trait("PAINREC3") && one_in(150)) {
            pain--;
        }
    }

    if( (has_trait("ALBINO") || has_effect("datura")) &&
        g->is_in_sunlight(pos()) && one_in(10) ) {
        // Umbrellas and rain gear can also keep the sun off!
        // (No, really, I know someone who uses an umbrella when it's sunny out.)
        if (!((worn_with_flag("RAINPROOF")) || (weapon.has_flag("RAIN_PROTECT"))) ) {
            add_msg(m_bad, _("The sunlight is really irritating."));
            if (in_sleep_state()) {
                wake_up();
            }
            if (one_in(10)) {
                mod_pain(1);
            }
            else focus_pool --;
        }
    }

    if (has_trait("SUNBURN") && g->is_in_sunlight(pos()) && one_in(10)) {
        if (!((worn_with_flag("RAINPROOF")) || (weapon.has_flag("RAIN_PROTECT"))) ) {
        add_msg(m_bad, _("The sunlight burns your skin!"));
        if (in_sleep_state()) {
            wake_up();
        }
        mod_pain(1);
        hurtall(1, nullptr);
        }
    }

    if ((has_trait("TROGLO") || has_trait("TROGLO2")) &&
        g->is_in_sunlight(pos()) && g->weather == WEATHER_SUNNY) {
        mod_str_bonus(-1);
        mod_dex_bonus(-1);
        add_miss_reason(_("The sunlight distracts you."), 1);
        mod_int_bonus(-1);
        mod_per_bonus(-1);
    }
    if (has_trait("TROGLO2") && g->is_in_sunlight(pos())) {
        mod_str_bonus(-1);
        mod_dex_bonus(-1);
        add_miss_reason(_("The sunlight distracts you."), 1);
        mod_int_bonus(-1);
        mod_per_bonus(-1);
    }
    if (has_trait("TROGLO3") && g->is_in_sunlight(pos())) {
        mod_str_bonus(-4);
        mod_dex_bonus(-4);
        add_miss_reason(_("You can't stand the sunlight!"), 4);
        mod_int_bonus(-4);
        mod_per_bonus(-4);
    }

    if (has_trait("SORES")) {
        for (int i = bp_head; i < num_bp; i++) {
            int sores_pain = 5 + (int)(0.4 * abs( encumb( body_part( i ) ) ) );
            if ((pain < sores_pain) && (!(has_trait("NOPAIN")))) {
                pain = 0;
                mod_pain( sores_pain );
            }
        }
    }

    if (has_trait("SLIMY") && !in_vehicle) {
        g->m.add_field( pos(), fd_slime, 1, 0 );
    }
        //Web Weavers...weave web
    if (has_active_mutation("WEB_WEAVER") && !in_vehicle) {
      g->m.add_field( pos(), fd_web, 1, 0 ); //this adds density to if its not already there.

     }

    if (has_trait("VISCOUS") && !in_vehicle) {
        if (one_in(3)){
            g->m.add_field( pos(), fd_slime, 1, 0 );
        } else {
            g->m.add_field( pos(), fd_slime, 2, 0 );
        }
    }

    // Blind/Deaf for brief periods about once an hour,
    // and visuals about once every 30 min.
    if (has_trait("PER_SLIME")) {
        if (one_in(600) && !has_effect("deaf")) {
            add_msg(m_bad, _("Suddenly, you can't hear anything!"));
            add_effect("deaf", 100 * rng (2, 6)) ;
        }
        if (one_in(600) && !(has_effect("blind"))) {
            add_msg(m_bad, _("Suddenly, your eyes stop working!"));
            add_effect("blind", 10 * rng (2, 6)) ;
        }
        // Yes, you can be blind and hallucinate at the same time.
        // Your post-human biology is truly remarkable.
        if (one_in(300) && !(has_effect("visuals"))) {
            add_msg(m_bad, _("Your visual centers must be acting up..."));
            add_effect("visuals", 120 * rng (3, 6)) ;
        }
    }

    if (has_trait("WEB_SPINNER") && !in_vehicle && one_in(3)) {
        g->m.add_field( pos(), fd_web, 1, 0 ); //this adds density to if its not already there.
    }

    if( has_trait("RADIOGENIC") && int(calendar::turn) % MINUTES(30) == 0 && radiation > 0 ) {
        // At 100 irradiation, twice as fast as REGEN
        if( x_in_y( radiation, 100 ) ) {
            healall( 1 );
            radiation -= 5;
        }
    }

    int rad_mut = 0;
    if (has_trait("RADIOACTIVE3") ) {
        rad_mut = 3;
    } else if (has_trait("RADIOACTIVE2")) {
        rad_mut = 2;
    } else if (has_trait("RADIOACTIVE1")) {
        rad_mut = 1;
    }
    if( rad_mut > 0 ) {
        if( g->m.get_radiation( pos3() ) < rad_mut - 1 && one_in( 600 / rad_mut ) ) {
            g->m.adjust_radiation( pos3(), 1 );
        } else if( one_in( 300 / rad_mut ) ) {
            radiation++;
        }
    }

    if (has_trait("UNSTABLE") && one_in(28800)) { // Average once per 2 days
        mutate();
    }
    if (has_trait("CHAOTIC") && one_in(7200)) { // Should be once every 12 hours
        mutate();
    }
    if (has_artifact_with(AEP_MUTAGENIC) && one_in(28800)) {
        mutate();
    }
    if (has_artifact_with(AEP_FORCE_TELEPORT) && one_in(600)) {
        g->teleport(this);
    }

    // checking for radioactive items in inventory
    int selfRadiation = 0;
    selfRadiation = leak_level("RADIOACTIVE");

    int localRadiation = g->m.get_radiation( pos3() );

    if (localRadiation || selfRadiation) {
        bool has_helmet = false;

        bool power_armored = is_wearing_power_armor(&has_helmet);

        double rads;
        if ((power_armored && has_helmet) || worn_with_flag("RAD_PROOF")) {
            rads = 0; // Power armor protects completely from radiation
        } else if (power_armored || worn_with_flag("RAD_RESIST")) {
            rads = localRadiation / 200.0f + selfRadiation / 10.0f;
        } else {
            rads = localRadiation / 32.0f + selfRadiation / 3.0f;
        }
        int rads_max = 0;
        if( rads > 0 ) {
            rads_max = static_cast<int>( rads );
            if( x_in_y( rads - rads_max, 1 ) ) {
                rads_max++;
            }
            if( calendar::once_every(MINUTES(3)) && has_bionic("bio_geiger") && localRadiation > 0 ) {
                add_msg(m_warning, _("You feel anomalous sensation coming from your radiation sensors."));
            }
        }
        radiation += rng( 0, rads_max );

        // Apply rads to any radiation badges.
        for (item *const it : inv_dump()) {
            if (it->type->id != "rad_badge") {
                continue;
            }

            // Actual irradiation levels of badges and the player aren't precisely matched.
            // This is intentional.
            int const before = it->irridation;

            const int delta = rng( 0, rads_max );
            if (delta == 0) {
                continue;
            }

            it->irridation += static_cast<int>(delta);

            // If in inventory (not worn), don't print anything.
            if (inv.has_item(it)) {
                continue;
            }

            // If the color hasn't changed, don't print anything.
            const std::string &col_before = rad_badge_color(before);
            const std::string &col_after  = rad_badge_color(it->irridation);
            if (col_before == col_after) {
                continue;
            }

            add_msg_if_player(m_warning, _("Your radiation badge changes from %1$s to %2$s!"),
                col_before.c_str(), col_after.c_str() );
        }
    }

    if( calendar::once_every(MINUTES(15)) ) {
        if (radiation < 0) {
            radiation = 0;
        } else if (radiation > 2000) {
            radiation = 2000;
        }
        if (OPTIONS["RAD_MUTATION"] && rng(60, 2500) < radiation) {
            mutate();
            radiation /= 2;
            radiation -= 5;
        } else if (radiation > 100 && rng(1, 1500) < radiation) {
            vomit();
            radiation -= 50;
        }
    }

    if( radiation > 150 && ( int(calendar::turn) % MINUTES(10) == 0 ) ) {
        hurtall(radiation / 100, nullptr);
    }

    if (reactor_plut || tank_plut || slow_rad) {
        // Microreactor CBM and supporting bionics
        if (has_bionic("bio_reactor") || has_bionic("bio_advreactor")) {
            //first do the filtering of plutonium from storage to reactor
            int plut_trans;
            plut_trans = 0;
            if (tank_plut > 0) {
                if (has_active_bionic("bio_plut_filter")) {
                    plut_trans = (tank_plut * 0.025);
                } else {
                    plut_trans = (tank_plut * 0.005);
                }
                if (plut_trans < 1) {
                    plut_trans = 1;
                }
                tank_plut -= plut_trans;
                reactor_plut += plut_trans;
            }
            //leaking radiation, reactor is unshielded, but still better than a simple tank
            slow_rad += ((tank_plut * 0.1) + (reactor_plut * 0.01));
            //begin power generation
            if (reactor_plut > 0) {
                int power_gen;
                power_gen = 0;
                if (has_bionic("bio_advreactor")){
                    if ((reactor_plut * 0.05) > 2000){
                        power_gen = 2000;
                    } else {
                        power_gen = reactor_plut * 0.05;
                        if (power_gen < 1) {
                            power_gen = 1;
                        }
                    }
                    slow_rad += (power_gen * 3);
                    while (slow_rad >= 50) {
                        if (power_gen >= 1) {
                            slow_rad -= 50;
                            power_gen -= 1;
                            reactor_plut -= 1;
                        } else {
                        break;
                        }
                    }
                } else if (has_bionic("bio_reactor")) {
                    if ((reactor_plut * 0.025) > 500){
                        power_gen = 500;
                    } else {
                        power_gen = reactor_plut * 0.025;
                        if (power_gen < 1) {
                            power_gen = 1;
                        }
                    }
                    slow_rad += (power_gen * 3);
                }
                reactor_plut -= power_gen;
                while (power_gen >= 250) {
                    apply_damage( nullptr, bp_torso, 1);
                    mod_pain(1);
                    add_msg(m_bad, _("Your chest burns as your power systems overload!"));
                    charge_power(50);
                    power_gen -= 60; // ten units of power lost due to short-circuiting into you
                }
                charge_power(power_gen);
            }
        } else {
            slow_rad += (((reactor_plut * 0.4) + (tank_plut * 0.4)) * 100);
            //plutonium in body without any kind of container.  Not good at all.
            reactor_plut *= 0.6;
            tank_plut *= 0.6;
        }
        while (slow_rad >= 500) {
            radiation += 1;
            slow_rad -=500;
        }
    }

    // Negative bionics effects
    if (has_bionic("bio_dis_shock") && one_in(1200)) {
        add_msg(m_bad, _("You suffer a painful electrical discharge!"));
        mod_pain(1);
        moves -= 150;

        if (weapon.type->id == "e_handcuffs" && weapon.charges > 0) {
            weapon.charges -= rng(1, 3) * 50;
            if (weapon.charges < 1) weapon.charges = 1;
            add_msg(m_good, _("The %s seems to be affected by the discharge."), weapon.tname().c_str());
        }
    }
    if (has_bionic("bio_dis_acid") && one_in(1500)) {
        add_msg(m_bad, _("You suffer a burning acidic discharge!"));
        hurtall(1, nullptr);
    }
    if (has_bionic("bio_drain") && power_level > 24 && one_in(600)) {
        add_msg(m_bad, _("Your batteries discharge slightly."));
        charge_power(-25);
    }
    if (has_bionic("bio_noise") && one_in(500)) {
        if(!is_deaf()) {
            add_msg(m_bad, _("A bionic emits a crackle of noise!"));
        } else {
            add_msg(m_bad, _("A bionic shudders, but you hear nothing."));
        }
        sounds::sound( pos(), 60, "");
    }
    if (has_bionic("bio_power_weakness") && max_power_level > 0 &&
        power_level >= max_power_level * .75) {
        mod_str_bonus(-3);
    }
    if (has_bionic("bio_trip") && one_in(500) && !has_effect("visuals")) {
        add_msg(m_bad, _("Your vision pixelates!"));
        add_effect("visuals", 100);
    }
    if (has_bionic("bio_spasm") && one_in(3000) && !has_effect("downed")) {
        add_msg(m_bad, _("Your malfunctioning bionic causes you to spasm and fall to the floor!"));
        mod_pain(1);
        add_effect("stunned", 1);
        add_effect("downed", 1, num_bp, false, 0, true );
    }
    if (has_bionic("bio_shakes") && power_level > 24 && one_in(1200)) {
        add_msg(m_bad, _("Your bionics short-circuit, causing you to tremble and shiver."));
        charge_power(-25);
        add_effect("shakes", 50);
    }
    if (has_bionic("bio_leaky") && one_in(500)) {
        mod_healthy_mod(-50, -200);
    }
    if (has_bionic("bio_sleepy") && one_in(500) && !in_sleep_state()) {
        fatigue++;
    }
    if (has_bionic("bio_itchy") && one_in(500) && !has_effect("formication")) {
        add_msg(m_bad, _("Your malfunctioning bionic itches!"));
      body_part bp = random_body_part(true);
        add_effect("formication", 100, bp);
    }

    // Artifact effects
    if (has_artifact_with(AEP_ATTENTION)) {
        add_effect("attention", 3);
    }
}

void player::mend( int rate_multiplier )
{
    // Wearing splints can slowly mend a broken limb back to 1 hp.
    bool any_broken = false;
    for( int i = 0; i < num_hp_parts; i++ ) {
        if( hp_cur[i] <= 0 ) {
            any_broken = true;
            break;
        }
    }

    if( !any_broken ) {
        return;
    }

    const double mending_odds = 500.0; // ~50% to mend in a week
    double healing_factor = 1.0;
    // Studies have shown that alcohol and tobacco use delay fracture healing time
    if( has_effect("cig") || addiction_level(ADD_CIG) > 0 ) {
        healing_factor *= 0.5;
    }
    if( has_effect("drunk") || addiction_level(ADD_ALCOHOL) > 0 ) {
        healing_factor *= 0.5;
    }

    // Bed rest speeds up mending
    if( has_effect("sleep") ) {
        healing_factor *= 4.0;
    } else if( fatigue > DEAD_TIRED ) {
        // but being dead tired does not...
        healing_factor *= 0.75;
    }

    // Being healthy helps.
    if( get_healthy() > 0 ) {
        healing_factor *= 2.0;
    }

    // And being well fed...
    if( get_hunger() < 0 ) {
        healing_factor *= 2.0;
    }

    if(thirst < 0) {
        healing_factor *= 2.0;
    }

    // Mutagenic healing factor!
    if( has_trait("REGEN") ) {
        healing_factor *= 16.0;
    } else if( has_trait("FASTHEALER2") ) {
        healing_factor *= 4.0;
    } else if( has_trait("FASTHEALER") ) {
        healing_factor *= 2.0;
    } else if( has_trait("SLOWHEALER") ) {
        healing_factor *= 0.5;
    }

    if( has_trait("REGEN_LIZ") ) {
        healing_factor = 20.0;
    }

    for( int iter = 0; iter < rate_multiplier; iter++ ) {
        bool any_broken = false;
        for( int i = 0; i < num_hp_parts; i++ ) {
            const bool broken = (hp_cur[i] <= 0);
            if( !broken ) {
                continue;
            }

            any_broken = true;

            bool mended = false;
            body_part part;
            switch(i) {
                case hp_arm_r:
                    part = bp_arm_r;
                    mended = is_wearing_on_bp("arm_splint", bp_arm_r) && x_in_y(healing_factor, mending_odds);
                    break;
                case hp_arm_l:
                    part = bp_arm_l;
                    mended = is_wearing_on_bp("arm_splint", bp_arm_l) && x_in_y(healing_factor, mending_odds);
                    break;
                case hp_leg_r:
                    part = bp_leg_r;
                    mended = is_wearing_on_bp("leg_splint", bp_leg_r) && x_in_y(healing_factor, mending_odds);
                    break;
                case hp_leg_l:
                    part = bp_leg_l;
                    mended = is_wearing_on_bp("leg_splint", bp_leg_l) && x_in_y(healing_factor, mending_odds);
                    break;
                default:
                    // No mending for you!
                    continue;
            }
            if( mended == false && has_trait("REGEN_LIZ") ) {
                // Splints aren't *strictly* necessary for your anatomy
                mended = x_in_y(healing_factor * 0.2, mending_odds);
            }
            if( mended ) {
                hp_cur[i] = 1;
                //~ %s is bodypart
                add_memorial_log( pgettext("memorial_male", "Broken %s began to mend."),
                                  pgettext("memorial_female", "Broken %s began to mend."),
                                  body_part_name(part).c_str() );
                //~ %s is bodypart
                add_msg_if_player( m_good, _("Your %s has started to mend!"),
                    body_part_name(part).c_str());
            }
        }

        if( !any_broken ) {
            return;
        }
    }
}

void player::vomit()
{
    add_memorial_log(pgettext("memorial_male", "Threw up."),
                     pgettext("memorial_female", "Threw up."));

    if (get_stomach_food() != 0 || get_stomach_water() != 0) {
        add_msg_player_or_npc( m_bad, _("You throw up heavily!"), _("<npcname> throws up heavily!") );
    } else {
        add_msg_if_player(m_warning, _("You feel nauseous, but your stomach is empty."));
    }
    mod_hunger(get_stomach_food());
    thirst += get_stomach_water();
    set_stomach_food(0);
    set_stomach_water(0);
    moves -= 100;
    for( auto &elem : effects ) {
        for( auto &_effect_it : elem.second ) {
            auto &it = _effect_it.second;
            if (it.get_id() == "foodpoison") {
                it.mod_duration(-300);
            } else if (it.get_id() == "drunk" ) {
                it.mod_duration(rng(-100, -500));
            }
        }
    }
    remove_effect("pkill1");
    remove_effect("pkill2");
    remove_effect("pkill3");
    wake_up();
}

void player::drench( int saturation, int flags, bool ignore_waterproof )
{
    // OK, water gets in your AEP suit or whatever.  It wasn't built to keep you dry.
    if ( has_trait("DEBUG_NOTEMP") || has_active_mutation("SHELL2") ||
         ( !ignore_waterproof && is_waterproof(flags) ) ) {
        return;
    }

    // Make the body wet
    for( int i = bp_torso; i < num_bp; ++i ) {
        // Different body parts have different size, they can only store so much water
        int bp_wetness_max = 0;
        if( mfb(i) & flags ){
            bp_wetness_max = drench_capacity[i];
        }

        if( bp_wetness_max == 0 ){
            continue;
        }
        // Different sources will only make the bodypart wet to a limit
        int source_wet_max = saturation / 2;
        int wetness_increment = source_wet_max / 8;
        // Make sure increment is at least 1
        if( source_wet_max != 0 && wetness_increment == 0 ) {
            wetness_increment = 1;
        }
        // Respect maximums
        const int wetness_max = std::min( source_wet_max, bp_wetness_max );
        if( body_wetness[i] < wetness_max ){
            body_wetness[i] = std::min( wetness_max, body_wetness[i] + wetness_increment );
        }
    }
}

void player::drench_mut_calc()
{
    for( size_t i = 0; i < num_bp; i++ ) {
        body_part bp = static_cast<body_part>( i );
        int ignored = 0;
        int neutral = 0;
        int good = 0;

        for( const auto &iter : my_mutations ) {
            const auto &mdata = mutation_branch::get( iter.first );
            const auto wp_iter = mdata.protection.find( bp );
            if( wp_iter != mdata.protection.end() ) {
                ignored += wp_iter->second.x;
                neutral += wp_iter->second.y;
                good += wp_iter->second.z;
            }
        }

        mut_drench[i][WT_GOOD] = good;
        mut_drench[i][WT_NEUTRAL] = neutral;
        mut_drench[i][WT_IGNORED] = ignored;
    }
}

void player::apply_wetness_morale( int temperature )
{
    // First, a quick check if we have any wetness to calculate morale from
    // Faster than checking all worn items for friendliness
    if( !std::any_of( body_wetness.begin(), body_wetness.end(),
        []( const int w ) { return w != 0; } ) ) {
        return;
    }

    // Normalize temperature to [-1.0,1.0]
    temperature = std::max( 0, std::min( 100, temperature ) );
    const double global_temperature_mod = -1.0 + ( 2.0 * temperature / 100.0 );

    int total_morale = 0;
    const auto wet_friendliness = exclusive_flag_coverage( "WATER_FRIENDLY" );
    for( int i = bp_torso; i < num_bp; ++i ) {
        // Sum of body wetness can go up to 103
        const int part_drench = body_wetness[i];
        if( part_drench == 0 ) {
            continue;
        }

        const auto &part_arr = mut_drench[i];
        const int part_ignored = part_arr[WT_IGNORED];
        const int part_neutral = part_arr[WT_NEUTRAL];
        const int part_good    = part_arr[WT_GOOD];

        if( part_ignored >= part_drench ) {
            continue;
        }

        int bp_morale = 0;
        const bool is_friendly = wet_friendliness[i];
        const int effective_drench = part_drench - part_ignored;
        if( is_friendly ) {
            // Using entire bonus from mutations and then some "human" bonus
            bp_morale = std::min( part_good, effective_drench ) + effective_drench / 2;
        } else if( effective_drench < part_good ) {
            // Positive or 0
            // Won't go higher than part_good / 2
            // Wet slime/scale doesn't feel as good when covered by wet rags/fur/kevlar
            bp_morale = std::min( effective_drench, part_good - effective_drench );
        } else if( effective_drench > part_good + part_neutral ) {
            // This one will be negative
            bp_morale = part_good + part_neutral - effective_drench;
        }

        // Clamp to [COLD,HOT] and cast to double
        const double part_temperature =
            std::min( BODYTEMP_HOT, std::max( BODYTEMP_COLD, temp_cur[i] ) );
        // 0.0 at COLD, 1.0 at HOT
        const double part_mod = (part_temperature - BODYTEMP_COLD) /
                                (BODYTEMP_HOT - BODYTEMP_COLD);
        // Average of global and part temperature modifiers, each in range [-1.0, 1.0]
        double scaled_temperature = ( global_temperature_mod + part_mod ) / 2;

        if( bp_morale < 0 ) {
            // Damp, hot clothing on hot skin feels bad
            scaled_temperature = fabs( scaled_temperature );
        }

        // For an unmutated human swimming in deep water, this will add up to:
        // +51 when hot in 100% water friendly clothing
        // -103 when cold/hot in 100% unfriendly clothing
        total_morale += static_cast<int>( bp_morale * ( 1.0 + scaled_temperature ) / 2.0 );
    }

    if( total_morale == 0 ) {
        return;
    }

    int morale_effect = total_morale / 8;
    if( morale_effect == 0 ) {
        if( total_morale > 0 ) {
            morale_effect = 1;
        } else {
            morale_effect = -1;
        }
    }

    add_morale( MORALE_WET, morale_effect, total_morale, 5, 5, true );
}

void player::update_body_wetness( const w_point &weather )
{
    // Average number of turns to go from completely soaked to fully dry
    // assuming average temperature and humidity
    constexpr int average_drying = HOURS(1);

    // A modifier on drying time
    double delay = 1.0;
    // Weather slows down drying
    delay -= ( weather.temperature - 65 ) / 100.0;
    delay += ( weather.humidity - 66 ) / 100.0;
    delay = std::max( 0.1, delay );
    // Fur/slime retains moisture
    if( has_trait("LIGHTFUR") || has_trait("FUR") || has_trait("FELINE_FUR") ||
        has_trait("LUPINE_FUR") || has_trait("CHITIN_FUR") || has_trait("CHITIN_FUR2") ||
        has_trait("CHITIN_FUR3")) {
        delay = delay * 6 / 5;
    }
    if( has_trait( "URSINE_FUR" ) || has_trait( "SLIMY" ) ) {
        delay = delay * 3 / 2;
    }

    if( !one_in_improved( average_drying * delay / 100.0 ) ) {
        // No drying this turn
        return;
    }

    // Now per-body-part stuff
    // To make drying uniform, make just one roll and reuse it
    const int drying_roll = rng( 1, 100 );
    if( drying_roll > 40 ) {
        // Wouldn't affect anything
        return;
    }

    for( int i = 0; i < num_bp; ++i ) {
        if( body_wetness[i] == 0 ) {
            continue;
        }
        // This is to normalize drying times
        int drying_chance = drench_capacity[i];
        // Body temperature affects duration of wetness
        // Note: Using temp_conv rather than temp_cur, to better approximate environment
        if( temp_conv[i] >= BODYTEMP_SCORCHING ) {
            drying_chance *= 2;
        } else if( temp_conv[i] >=  BODYTEMP_VERY_HOT ) {
            drying_chance = drying_chance * 3 / 2;
        } else if( temp_conv[i] >= BODYTEMP_HOT ) {
            drying_chance = drying_chance * 4 / 3;
        } else if( temp_conv[i] > BODYTEMP_COLD ) {
            // Comfortable, doesn't need any changes
        } else {
            // Evaporation doesn't change that much at lower temp
            drying_chance = drying_chance * 3 / 4;
        }

        if( drying_chance < 1 ) {
            drying_chance = 1;
        }

        if( drying_chance >= drying_roll ) {
            body_wetness[i] -= 1;
            if( body_wetness[i] < 0 ) {
                body_wetness[i] = 0;
            }
        }
    }
    // TODO: Make clothing slow down drying
}

int player::net_morale(morale_point effect) const
{
    double bonus = effect.bonus;

    // If the effect is old enough to have started decaying,
    // reduce it appropriately.
    if (effect.age > effect.decay_start)
    {
        bonus *= logarithmic_range(effect.decay_start,
                                effect.duration, effect.age);
    }

    // Optimistic characters focus on the good things in life,
    // and downplay the bad things.
    if (has_trait("OPTIMISTIC"))
    {
        if (bonus >= 0)
        {
            bonus *= 1.25;
        }
        else
        {
            bonus *= 0.75;
        }
    }

     // Again, those grouchy Bad-Tempered folks always focus on the negative.
     // They can't handle positive things as well.  They're No Fun.  D:
    if (has_trait("BADTEMPER"))
    {
        if (bonus < 0)
        {
            bonus *= 1.25;
        }
        else
        {
            bonus *= 0.75;
        }
    }
    return bonus;
}

int player::morale_level() const
{
    // Add up all of the morale bonuses (and penalties).
    int ret = 0;
    for (auto &i : morale) {
        ret += net_morale(i);
    }

    // Prozac reduces negative morale by 75%.
    if (has_effect("took_prozac") && ret < 0) {
        ret = int(ret / 4);
    }

    return ret;
}

void player::add_morale(morale_type type, int bonus, int max_bonus,
                        int duration, int decay_start,
                        bool cap_existing, const itype* item_type)
{
    bool placed = false;

    // Search for a matching morale entry.
    for (auto &i : morale) {
        if (i.type == type && i.item_type == item_type) {
            // Found a match!
            placed = true;

            // Scale the morale bonus to its current level.
            if (i.age > i.decay_start) {
                i.bonus *= logarithmic_range(i.decay_start, i.duration, i.age);
            }

            // If we're capping the existing effect, we can use the new duration
            // and decay start.
            if (cap_existing) {
                i.duration = duration;
                i.decay_start = decay_start;
            } else {
                // Otherwise, we need to figure out whether the existing effect had
                // more remaining duration and decay-resistance than the new one does.
                // Only takes the new duration if new bonus and old are the same sign.
                if (i.duration - i.age <= duration &&
                   ((i.bonus > 0) == (max_bonus > 0)) ) {
                    i.duration = duration;
                } else {
                    // This will give a longer duration than above.
                    i.duration -= i.age;
                }

                if (i.decay_start - i.age <= decay_start &&
                   ((i.bonus > 0) == (max_bonus > 0)) ) {
                    i.decay_start = decay_start;
                } else {
                    // This will give a later decay start than above.
                    i.decay_start -= i.age;
                }
            }

            // Now that we've finished using it, reset the age to 0.
            i.age = 0;

            // Is the current morale level for this entry below its cap, if any?
            if (abs(i.bonus) < abs(max_bonus) || max_bonus == 0) {
                // Add the requested morale boost.
                i.bonus += bonus;

                // If we passed the cap, pull back to it.
                if (abs(i.bonus) > abs(max_bonus) && max_bonus != 0) {
                    i.bonus = max_bonus;
                }
            } else if (cap_existing) {
                // The existing bonus is above the new cap.  Reduce it.
                i.bonus = max_bonus;
            }
            //Found a match, so no need to check further
            break;
        }
    }

    // No matching entry, so add a new one
    if (!placed)
    {
        morale_point tmp(type, item_type, bonus, duration, decay_start, 0);
        morale.push_back(tmp);
    }
}

int player::has_morale( morale_type type ) const
{
    for( auto &elem : morale ) {
        if( elem.type == type ) {
            return elem.bonus;
        }
    }
    return 0;
}

void player::rem_morale(morale_type type, const itype* item_type)
{
    for( size_t i = 0; i < morale.size(); ++i ) {
        if (morale[i].type == type && morale[i].item_type == item_type) {
            morale.erase(morale.begin() + i);
            break;
        }
    }
}

void player::process_active_items()
{
    if( weapon.needs_processing() && weapon.process( this, pos3(), false ) ) {
        weapon = ret_null;
    }

    std::vector<item *> inv_active = inv.active_items();
    for( auto tmp_it : inv_active ) {

        if( tmp_it->process( this, pos3(), false ) ) {
            inv.remove_item(tmp_it);
        }
    }

    // worn items
    remove_worn_items_with( [this]( item &itm ) {
        return itm.needs_processing() && itm.process( this, pos3(), false );
    } );

    long ch_UPS = charges_of( "UPS" );
    item *cloak = nullptr;
    item *power_armor = nullptr;
    // Manual iteration because we only care about *worn* active items.
    for( auto &w : worn ) {
        if( !w.active ) {
            continue;
        }
        if( w.typeId() == OPTICAL_CLOAK_ITEM_ID ) {
            cloak = &w;
        }
        // Only the main power armor item can be active, the other ones (hauling frame, helmet) aren't.
        if( power_armor == nullptr && w.is_power_armor() ) {
            power_armor = &w;
        }
    }
    if( cloak != nullptr ) {
        if( ch_UPS >= 40 ) {
            use_charges( "UPS", 40 );
            if( ch_UPS < 200 && one_in( 3 ) ) {
                add_msg_if_player( m_warning, _( "Your optical cloak flickers for a moment!" ) );
            }
        } else if( ch_UPS > 0 ) {
            use_charges( "UPS", ch_UPS );
        } else {
            add_msg_if_player( m_warning, _( "Your optical cloak flickers for a moment as it becomes opaque." ) );
            // Bypass the "you deactivate the ..." message
            cloak->active = false;
        }
    }
    static const std::string BIO_POWER_ARMOR_INTERFACE( "bio_power_armor_interface" );
    static const std::string BIO_POWER_ARMOR_INTERFACE_MK_II( "bio_power_armor_interface_mkII" );
    const bool has_power_armor_interface = ( has_active_bionic( BIO_POWER_ARMOR_INTERFACE ) ||
                                             has_active_bionic( BIO_POWER_ARMOR_INTERFACE_MK_II ) ) &&
                                           power_level > 0;
    // For power armor that is powered via the armor interface bionic, energy is consumed by that bionic
    // (it's an active bionic consuming power on its own). The bionic is preferred over UPS usage.
    // Only if the character does not have that bionic it falls back to the UPS.
    if( power_armor != nullptr && !has_power_armor_interface ) {
        if( ch_UPS > 0 ) {
            use_charges( "UPS", 4 );
        } else {
            add_msg_if_player( m_warning, _( "Your power armor disengages." ) );
            // Bypass the "you deactivate the ..." message
            power_armor->active = false;
        }
    }
    // Load all items that use the UPS to their minimal functional charge,
    // The tool is not really useful if its charges are below charges_to_use
    ch_UPS = charges_of( "UPS" ); // might have been changed by cloak
    long ch_UPS_used = 0;
    for( size_t i = 0; i < inv.size() && ch_UPS_used < ch_UPS; i++ ) {
        item &it = inv.find_item(i);
        if( !it.has_flag( "USE_UPS" ) ) {
            continue;
        }
        if( it.charges < it.type->maximum_charges() ) {
            ch_UPS_used++;
            it.charges++;
        }
    }
    if( weapon.has_flag( "USE_UPS" ) &&  ch_UPS_used < ch_UPS &&
        weapon.charges < weapon.type->maximum_charges() ) {
        ch_UPS_used++;
        weapon.charges++;
    }

    for( item& worn_item : worn ) {
        if( ch_UPS_used >= ch_UPS ) {
            break;
        }
        if( !worn_item.has_flag( "USE_UPS" ) ) {
            continue;
        }
        if( worn_item.charges < worn_item.type->maximum_charges() ) {
            ch_UPS_used++;
            worn_item.charges++;
        }
    }
    if( ch_UPS_used > 0 ) {
        use_charges( "UPS", ch_UPS_used );
    }
}

item player::reduce_charges( int position, long quantity )
{
    item &it = i_at( position );
    if( it.is_null() ) {
        debugmsg( "invalid item position %d for reduce_charges", position );
        return ret_null;
    }
    if( it.reduce_charges( quantity ) ) {
        return i_rem( position );
    }
    item tmp( it );
    tmp.charges = quantity;
    return tmp;
}

item player::reduce_charges( item *it, long quantity )
{
    if( !has_item( it ) ) {
        debugmsg( "invalid item (name %s) for reduce_charges", it->tname().c_str() );
        return ret_null;
    }
    if( const_cast<item *>( it )->reduce_charges( quantity ) ) {
        return i_rem( it );
    }
    item result( *it );
    result.charges = quantity;
    return result;
}

int player::invlet_to_position( const long linvlet ) const
{
    // Invlets may come from curses, which may also return any kind of key codes, those being
    // of type long and they can become valid, but different characters when casted to char.
    // Example: KEY_NPAGE (returned when the player presses the page-down key) is 0x152,
    // casted to char would yield 0x52, which happesn to be 'R', a valid invlet.
    if( linvlet > std::numeric_limits<char>::max() || linvlet < std::numeric_limits<char>::min() ) {
        return INT_MIN;
    }
    const char invlet = static_cast<char>( linvlet );
    if( is_npc() ) {
        DebugLog( D_WARNING,  D_GAME ) << "Why do you need to call player::invlet_to_position on npc " << name;
    }
    if( weapon.invlet == invlet ) {
        return -1;
    }
    auto iter = worn.begin();
    for( size_t i = 0; i < worn.size(); i++, iter++ ) {
        if( iter->invlet == invlet ) {
            return worn_position_to_index( i );
        }
    }
    return inv.invlet_to_position( invlet );
}

const martialart &player::get_combat_style() const
{
    return style_selected.obj();
}

std::vector<item *> player::inv_dump()
{
    std::vector<item *> ret;
    if (!weapon.is_null() && !weapon.has_flag("NO_UNWIELD")) {
        ret.push_back(&weapon);
    }
    for (auto &i : worn) {
        ret.push_back(&i);
    }
    inv.dump(ret);
    return ret;
}

std::list<item> player::use_amount(itype_id it, int _quantity)
{
    std::list<item> ret;
    long quantity = _quantity; // Don't wanny change the function signature right now
    if (weapon.use_amount(it, quantity, ret)) {
        remove_weapon();
    }
    for( auto a = worn.begin(); a != worn.end() && quantity > 0; ) {
        if( a->use_amount( it, quantity, ret ) ) {
            a = worn.erase( a );
        } else {
            ++a;
        }
    }
    if (quantity <= 0) {
        return ret;
    }
    std::list<item> tmp = inv.use_amount(it, quantity);
    ret.splice(ret.end(), tmp);
    return ret;
}

bool player::use_charges_if_avail(itype_id it, long quantity)
{
    if (has_charges(it, quantity))
    {
        use_charges(it, quantity);
        return true;
    }
    return false;
}

bool player::has_fire(const int quantity) const
{
// TODO: Replace this with a "tool produces fire" flag.

    if( g->m.has_nearby_fire( pos3() ) ) {
        return true;
    } else if (has_charges("torch_lit", 1)) {
        return true;
    } else if (has_charges("battletorch_lit", quantity)) {
        return true;
    } else if (has_charges("handflare_lit", 1)) {
        return true;
    } else if (has_charges("candle_lit", 1)) {
        return true;
    } else if (has_active_bionic("bio_tools")) {
        return true;
    } else if (has_bionic("bio_lighter")) {
        return true;
    } else if (has_bionic("bio_laser")) {
        return true;
    } else if (has_charges("ref_lighter", quantity)) {
        return true;
    } else if (has_charges("matches", quantity)) {
        return true;
    } else if (has_charges("lighter", quantity)) {
        return true;
    } else if (has_charges("flamethrower", quantity)) {
        return true;
    } else if (has_charges("flamethrower_simple", quantity)) {
        return true;
    } else if (has_charges("hotplate", quantity)) {
        return true;
    } else if (has_charges("welder", quantity)) {
        return true;
    } else if (has_charges("welder_crude", quantity)) {
        return true;
    } else if (has_charges("shishkebab_on", quantity)) {
        return true;
    } else if (has_charges("firemachete_on", quantity)) {
        return true;
    } else if (has_charges("broadfire_on", quantity)) {
        return true;
    } else if (has_charges("firekatana_on", quantity)) {
        return true;
    } else if (has_charges("zweifire_on", quantity)) {
        return true;
    }
    return false;
}

void player::use_fire(const int quantity)
{
//Ok, so checks for nearby fires first,
//then held lit torch or candle, bio tool/lighter/laser
//tries to use 1 charge of lighters, matches, flame throwers
// (home made, military), hotplate, welder in that order.
// bio_lighter, bio_laser, bio_tools, has_active_bionic("bio_tools"

    if( g->m.has_nearby_fire( pos3() ) ) {
        return;
    } else if (has_charges("torch_lit", 1)) {
        return;
    } else if (has_charges("battletorch_lit", 1)) {
        return;
    } else if (has_charges("handflare_lit", 1)) {
        return;
    } else if (has_charges("candle_lit", 1)) {
        return;
    } else if (has_charges("shishkebab_on", quantity)) {
        return;
    } else if (has_charges("firemachete_on", quantity)) {
        return;
    } else if (has_charges("broadfire_on", quantity)) {
        return;
    } else if (has_charges("firekatana_on", quantity)) {
        return;
    } else if (has_charges("zweifire_on", quantity)) {
        return;
    } else if (has_active_bionic("bio_tools")) {
        return;
    } else if (has_bionic("bio_lighter")) {
        return;
    } else if (has_bionic("bio_laser")) {
        return;
    } else if (has_charges("ref_lighter", quantity)) {
        use_charges("ref_lighter", quantity);
        return;
    } else if (has_charges("matches", quantity)) {
        use_charges("matches", quantity);
        return;
    } else if (has_charges("lighter", quantity)) {
        use_charges("lighter", quantity);
        return;
    } else if (has_charges("flamethrower", quantity)) {
        use_charges("flamethrower", quantity);
        return;
    } else if (has_charges("flamethrower_simple", quantity)) {
        use_charges("flamethrower_simple", quantity);
        return;
    } else if (has_charges("hotplate", quantity)) {
        use_charges("hotplate", quantity);
        return;
    } else if (has_charges("welder", quantity)) {
        use_charges("welder", quantity);
        return;
    } else if (has_charges("welder_crude", quantity)) {
        use_charges("welder_crude", quantity);
        return;
    } else if (has_charges("shishkebab_off", quantity)) {
        use_charges("shishkebab_off", quantity);
        return;
    } else if (has_charges("firemachete_off", quantity)) {
        use_charges("firemachete_off", quantity);
        return;
    } else if (has_charges("broadfire_off", quantity)) {
        use_charges("broadfire_off", quantity);
        return;
    } else if (has_charges("firekatana_off", quantity)) {
        use_charges("firekatana_off", quantity);
        return;
    } else if (has_charges("zweifire_off", quantity)) {
        use_charges("zweifire_off", quantity);
        return;
    }
}

// does NOT return anything if the item is integrated toolset or fire!
std::list<item> player::use_charges(itype_id it, long quantity)
{
    std::list<item> ret;
    // the first two cases *probably* don't need to be tracked for now...
    if (it == "toolset") {
        charge_power(-quantity);
        return ret;
    }
    if (it == "fire") {
        use_fire(quantity);
        return ret;
    }
    if ( it == "UPS" ) {
        const long charges_off = std::min( quantity, charges_of( "UPS_off" ) );
        if ( charges_off > 0 ) {
            std::list<item> tmp = use_charges( "UPS_off", charges_off );
            ret.splice(ret.end(), tmp);
            quantity -= charges_off;
            if (quantity <= 0) {
                return ret;
            }
        }
        return ret;
    }
    if (weapon.use_charges(it, quantity, ret)) {
        remove_weapon();
    }
    for( auto a = worn.begin(); a != worn.end() && quantity > 0; ) {
        if( a->use_charges( it, quantity, ret ) ) {
            a = worn.erase( a );
        } else {
            ++a;
        }
    }
    if (quantity <= 0) {
        return ret;
    }
    std::list<item> tmp = inv.use_charges(it, quantity);
    ret.splice(ret.end(), tmp);
    if (quantity <= 0) {
        return ret;
    }
    // Threat requests for UPS charges as request for adv. UPS charges
    // and as request for bionic UPS charges, both with their own modificators
    // If we reach this, the regular UPS could not provide all the requested
    // charges, so we *have* to remove as many as charges as we can (but not
    // more than requested) to not let any charges un-consumed.
    if ( it == "UPS_off" ) {
        // Request for 8 UPS charges:
        // 8 UPS = 8 * 6 / 10 == 48/10 == 4.8 adv. UPS
        // consume 5 adv. UPS charges, see player::charges_of, if the adv. UPS
        // had only 4 charges, it would report as floor(4/0.6)==6 normal UPS
        // charges
        long quantity_adv = ceil(quantity * 0.6);
        long avail_adv = charges_of("adv_UPS_off");
        long adv_charges_to_use = std::min(avail_adv, quantity_adv);
        if (adv_charges_to_use > 0) {
            std::list<item> tmp = use_charges("adv_UPS_off", adv_charges_to_use);
            ret.splice(ret.end(), tmp);
            quantity -= static_cast<long>(adv_charges_to_use / 0.6);
            if (quantity <= 0) {
                return ret;
            }
        }
    }
    if ( power_level > 0 && it == "UPS_off" && has_bionic( "bio_ups" ) ) {
        // Need always at least 1 power unit, to prevent exploits
        // and make sure power_level does not get negative
        long ch = std::max(1l, quantity / 10);
        ch = std::min<long>(power_level, ch);
        charge_power(-ch);
        quantity -= ch * 10;
        // TODO: add some(pseudo?) item to resulting list?
    }
    return ret;
}

int player::butcher_factor() const
{
    int result = INT_MIN;
    if( has_bionic( "bio_tools" ) ) {
        item tmp( "toolset", 0 );
        result = std::max( result, tmp.butcher_factor() );
    }
    if( has_bionic( "bio_razor" ) ) {
        result = std::max( result, 8 );
    }
    result = std::max( result, inv.butcher_factor() );
    result = std::max( result, weapon.butcher_factor() );
    for( const auto &elem : worn ) {
        result = std::max( result, elem.butcher_factor() );
    }
    return result;
}

item* player::pick_usb()
{
    std::vector<std::pair<item*, int> > drives = inv.all_items_by_type("usb_drive");

    if (drives.empty()) {
        return NULL; // None available!
    }

    std::vector<std::string> selections;
    for (size_t i = 0; i < drives.size() && i < 9; i++) {
        selections.push_back( drives[i].first->tname() );
    }

    int select = menu_vec(false, _("Choose drive:"), selections);

    return drives[ select - 1 ].first;
}

bool player::covered_with_flag( const std::string &flag, const std::bitset<num_bp> &parts ) const
{
    std::bitset<num_bp> covered = 0;

    for (auto armorPiece = worn.rbegin(); armorPiece != worn.rend(); ++armorPiece) {
        std::bitset<num_bp> cover = armorPiece->get_covered_body_parts() & parts;

        if (cover.none()) {
            continue; // For our purposes, this piece covers nothing.
        }
        if ((cover & covered).any()) {
            continue; // the body part(s) is already covered.
        }

        bool hasFlag = armorPiece->has_flag(flag);
        if (!hasFlag) {
            return false; // The item is the top layer on a relevant body part, and isn't tagged, so we fail.
        } else {
            covered |= cover; // The item is the top layer on a relevant body part, and is tagged.
        }
    }

    return (covered == parts);
}

std::bitset<num_bp> player::exclusive_flag_coverage( const std::string &flag ) const
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

bool player::is_waterproof( const std::bitset<num_bp> &parts ) const
{
    return covered_with_flag("WATERPROOF", parts);
}

bool player::has_amount(const itype_id &it, int quantity) const
{
    if (it == "toolset")
    {
        return has_active_bionic("bio_tools");
    }
    return (amount_of(it) >= quantity);
}

int player::amount_of(const itype_id &it) const
{
    if (it == "toolset" && has_active_bionic("bio_tools")) {
        return 1;
    }
    if (it == "apparatus") {
        return ( has_items_with_quality("SMOKE_PIPE", 1, 1) ? 1 : 0 );
    }
    int quantity = weapon.amount_of(it, true);
    for( const auto &elem : worn ) {
        quantity += elem.amount_of( it, true );
    }
    quantity += inv.amount_of(it);
    return quantity;
}

int player::amount_worn(const itype_id &id) const
{
    int amount = 0;
    for(auto &elem : worn) {
        if(elem.typeId() == id) {
            ++amount;
        }
    }
    return amount;
}

bool player::has_charges(const itype_id &it, long quantity) const
{
    if (it == "fire" || it == "apparatus") {
        return has_fire(quantity);
    }
    return (charges_of(it) >= quantity);
}

long player::charges_of(const itype_id &it) const
{
    if (it == "toolset") {
        if (has_active_bionic("bio_tools")) {
            return power_level;
        } else {
            return 0;
        }
    }
    // Handle requests for UPS charges as request for adv. UPS charges
    // and as request for bionic UPS charges, both with their own multiplier
    if ( it == "UPS" ) {
        // This includes the UPS bionic (regardless of active state)
        return charges_of( "UPS_off" );
    }
    // Now regular charges from all items (weapone,worn,inventory)
    long quantity = weapon.charges_of(it);
    for( const auto &armor : worn ) {
        quantity += armor.charges_of(it);
    }
    quantity += inv.charges_of(it);
    // Now include charges from advanced UPS if the request was UPS
    if ( it == "UPS_off" ) {
        // Round charges from adv. UPS down, if this reports there are N
        // charges available, we must be able to remove at least N charges.
        quantity += static_cast<long>( floor( charges_of( "adv_UPS_off" ) / 0.6 ) );
    }
    if ( power_level > 0 ) {
        if ( it == "UPS_off" && has_bionic( "bio_ups" ) ) {
            quantity += power_level * 10;
        }
    }
    return quantity;
}

int  player::leak_level( std::string flag ) const
{
    int leak_level = 0;
    leak_level = inv.leak_level(flag);
    return leak_level;
}

bool player::has_item(int position) {
    return !i_at(position).is_null();
}

bool player::has_item( const item *it ) const
{
    return has_item_with( [&it]( const item & i ) {
        return &i == it;
    } );
}

bool player::has_mission_item(int mission_id) const
{
    return mission_id != -1 && has_item_with( has_mission_item_filter{ mission_id } );
}

hint_rating player::rate_action_eat( const item &it ) const
{
    //TODO more cases, for HINT_IFFY
    if( it.is_food_container() || it.is_food() ) {
        return HINT_GOOD;
    }

    return HINT_CANT;
}

//Returns the amount of charges that were consumed by the player
int player::drink_from_hands(item& water) {
    int charges_consumed = 0;
    if( query_yn( _("Drink %s from your hands?"), water.type_name().c_str() ) )
    {
        // Create a dose of water no greater than the amount of water remaining.
        item water_temp( water );
        // If player is slaked water might not get consumed.
        consume_item( water_temp );
        charges_consumed = water.charges - water_temp.charges;
        if( charges_consumed > 0 )
        {
            moves -= 350;
        }
    }

    return charges_consumed;
}


bool player::consume_item( item &target )
{
    if( target.is_null() ) {
        add_msg_if_player( m_info, _("You do not have that item."));
        return false;
    }
    if (is_underwater()) {
        add_msg_if_player( m_info, _("You can't do that while underwater."));
        return false;
    }
    item *to_eat = nullptr;
    if( target.is_food_container( this ) ) {
        to_eat = &target.contents[0];
    } else if( target.is_food( this ) ) {
        to_eat = &target;
    } else {
        add_msg_if_player(m_info, _("You can't eat your %s."), target.tname().c_str());
        if(is_npc()) {
            debugmsg("%s tried to eat a %s", name.c_str(), target.tname().c_str());
        }
        return false;
    }
    const auto comest = dynamic_cast<const it_comest*>( to_eat->type );

    int amount_used = 1;
    if (comest != NULL) {
        if (comest->comesttype == "FOOD" || comest->comesttype == "DRINK") {
            if( !eat(to_eat, comest) ) {
                return false;
            }
        } else if (comest->comesttype == "MED") {
            if (comest->tool != "null") {
                // Check tools
                bool has = has_amount(comest->tool, 1);
                // Tools with charges need to have charges, not just be present.
                if( item::count_by_charges( comest->tool ) ) {
                    has = has_charges(comest->tool, 1);
                }
                if (!has) {
                    add_msg_if_player(m_info, _("You need a %s to consume that!"),
                                         item::nname( comest->tool ).c_str());
                    return false;
                }
                use_charges(comest->tool, 1); // Tools like lighters get used
            }
            if (comest->has_use()) {
                //Check special use
                amount_used = comest->invoke( this, to_eat, pos3() );
                if( amount_used <= 0 ) {
                    return false;
                }
            }
            consume_effects(to_eat, comest);
            moves -= 250;
        } else {
            debugmsg("Unknown comestible type of item: %s\n", to_eat->tname().c_str());
        }
    } else {
 // Consume other type of items.
        // For when bionics let you eat fuel
        if (to_eat->is_ammo() && has_active_bionic("bio_batteries") &&
            to_eat->ammo_type() == "battery") {
            const int factor = 1;
            int max_change = max_power_level - power_level;
            if (max_change == 0) {
                add_msg_if_player(m_info, _("Your internal power storage is fully powered."));
            }
            charge_power(to_eat->charges / factor);
            to_eat->charges -= max_change * factor; //negative charges seem to be okay
            to_eat->charges++; //there's a flat subtraction later
        } else if (to_eat->is_ammo() &&  ( has_active_bionic("bio_reactor") || has_active_bionic("bio_advreactor") ) && ( to_eat->ammo_type() == "reactor_slurry" || to_eat->ammo_type() == "plutonium")) {
            if (to_eat->type->id == "plut_cell" && query_yn(_("Thats a LOT of plutonium.  Are you sure you want that much?"))) {
                tank_plut += 5000;
            } else if (to_eat->type->id == "plut_slurry_dense") {
                tank_plut += 500;
            } else if (to_eat->type->id == "plut_slurry") {
                tank_plut += 250;
            }
            add_msg_player_or_npc( _("You add your %s to your reactor's tank."), _("<npcname> pours %s into their reactor's tank."),
            to_eat->tname().c_str());
        } else if (!to_eat->is_food() && !to_eat->is_food_container(this)) {
            if (to_eat->is_book()) {
                if (to_eat->type->book->skill && !query_yn(_("Really eat %s?"), to_eat->tname().c_str())) {
                    return false;
                }
            }
            int charge = (to_eat->volume() + to_eat->weight()) / 9;
            if (to_eat->made_of("leather")) {
                charge /= 4;
            }
            if (to_eat->made_of("wood")) {
                charge /= 2;
            }
            charge_power(charge);
            to_eat->charges = 0;
            add_msg_player_or_npc( _("You eat your %s."), _("<npcname> eats a %s."),
                                     to_eat->tname().c_str());
        }
        moves -= 250;
    }

    to_eat->charges -= amount_used;
    return to_eat->charges <= 0;
}

bool player::consume(int target_position)
{
    auto &target = i_at( target_position );
    const bool was_in_container = target.is_food_container( this );
    if( consume_item( target ) ) {
        if( was_in_container ) {
            i_rem( &target.contents[0] );
        } else {
            i_rem( &target );
        }
        if( was_in_container && target_position == -1 ) {
            add_msg_if_player(_("You are now wielding an empty %s."), weapon.tname().c_str());
        } else if( was_in_container && target_position < -1 ) {
            add_msg_if_player(_("You are now wearing an empty %s."), target.tname().c_str());
        } else if( was_in_container && !is_npc() ) {
            bool drop_it = false;
            if (OPTIONS["DROP_EMPTY"] == "no") {
                drop_it = false;
            } else if (OPTIONS["DROP_EMPTY"] == "watertight") {
                drop_it = !target.is_watertight_container();
            } else if (OPTIONS["DROP_EMPTY"] == "all") {
                drop_it = true;
            }
            if (drop_it) {
                add_msg(_("You drop the empty %s."), target.tname().c_str());
                g->m.add_item_or_charges( pos(), inv.remove_item(&target) );
            } else {
                add_msg(m_info, _("%c - an empty %s"), (target.invlet ? target.invlet : ' '), target.tname().c_str());
            }
        }
    }

    if( target_position >= 0 ) {
        // Always restack and resort the inventory when items in it have been changed.
        inv.restack( this );
        inv.unsort();
    }

    return true;
}

bool player::eat(item *eaten, const it_comest *comest)
{
    int to_eat = 1;
    if (comest == NULL) {
        debugmsg("player::eat(%s); comest is NULL!", eaten->tname().c_str());
        return false;
    }
    if (comest->tool != "null") {
        bool has = has_amount(comest->tool, 1);
        if( item::count_by_charges( comest->tool ) ) {
            has = has_charges(comest->tool, 1);
        }
        if (!has) {
            add_msg_if_player(m_info, _("You need a %s to consume that!"),
                       item::nname( comest->tool ).c_str());
            return false;
        }
    }
    if (is_underwater()) {
        add_msg_if_player(m_info, _("You can't do that while underwater."));
        return false;
    }
    // For all those folks who loved eating marloss berries.  D:< mwuhahaha
    if (has_trait("M_DEPENDENT") && (eaten->type->id != "mycus_fruit")) {
        add_msg_if_player(m_info, _("We can't eat that.  It's not right for us."));
        return false;
    }
    // Here's why PROBOSCIS is such a negative trait.
    if ( (has_trait("PROBOSCIS")) && (comest->comesttype == "FOOD" ||
        eaten->has_flag("USE_EAT_VERB")) ) {
        add_msg_if_player(m_info,  _("Ugh, you can't drink that!"));
        return false;
    }
    bool overeating = (!has_trait("GOURMAND") && get_hunger() < 0 &&
                       nutrition_for(comest) >= 5);
    bool hiberfood = (has_active_mutation("HIBERNATE") && (get_hunger() > -60 && thirst > -60 ));
    eaten->calc_rot( global_square_location() ); // check if it's rotten before eating!
    bool spoiled = eaten->rotten();

    last_item = itype_id(eaten->type->id);

    if (overeating && !has_trait("HIBERNATE") && !has_trait("EATHEALTH") && !is_npc() &&
        !has_trait("SLIMESPAWNER") && !query_yn(_("You're full.  Force yourself to eat?"))) {
        return false;
    } else if (has_trait("GOURMAND") && get_hunger() < 0 && nutrition_for(comest) >= 5) {
        if (!query_yn(_("You're fed.  Try to pack more in anyway?"))) {
            return false;
        }
    }

    int temp_nutr = nutrition_for(comest);
    int temp_quench = comest->quench;
    if (hiberfood && !is_npc() && (((get_hunger() - temp_nutr) < -60) || ((thirst - temp_quench) < -60)) && has_active_mutation("HIBERNATE")){
       if (!query_yn(_("You're adequately fueled. Prepare for hibernation?"))) {
        return false;
       }
       else
       if(!is_npc()) {add_memorial_log(pgettext("memorial_male", "Began preparing for hibernation."),
                                       pgettext("memorial_female", "Began preparing for hibernation."));
                      add_msg(_("You've begun stockpiling calories and liquid for hibernation.  You get the feeling that you should prepare for bed, just in case, but...you're hungry again, and you could eat a whole week's worth of food RIGHT NOW."));
      }
    }
    if (has_trait("CARNIVORE") && (eaten->made_of("veggy") || eaten->made_of("fruit") || eaten->made_of("wheat")) &&
      !(eaten->made_of("flesh") ||eaten->made_of("hflesh") || eaten->made_of("iflesh") || eaten->made_of("milk") ||
      eaten->made_of("egg")) && nutrition_for(comest) > 0) {
        add_msg_if_player(m_info, _("Eww.  Inedible plant stuff!"));
        return false;
    }
    if ((!has_trait("SAPIOVORE") && !has_trait("CANNIBAL") && !has_trait("PSYCHOPATH")) && eaten->made_of("hflesh")&&
        !is_npc() && !query_yn(_("The thought of eating that makes you feel sick.  Really do it?"))) {
        return false;
    }
    if ((!has_trait("SAPIOVORE") && has_trait("CANNIBAL") && !has_trait("PSYCHOPATH") && !has_trait("SPIRITUAL")) && eaten->made_of("hflesh")&& !is_npc() &&
        !query_yn(_("The thought of eating that makes you feel both guilty and excited.  Really do it?"))) {
        return false;
    }

    if ((!has_trait("SAPIOVORE") && has_trait("CANNIBAL") && !has_trait("PSYCHOPATH") && has_trait("SPIRITUAL")) &&
         eaten->made_of("hflesh")&& !is_npc() &&
        !query_yn(_("Cannibalism is such a universal taboo.  Will you break it?"))) {
        return false;
    }

    if (has_trait("VEGETARIAN") && eaten->made_of("flesh") && !is_npc() &&
        !query_yn(_("Really eat that %s?  Your stomach won't be happy."), eaten->tname().c_str())) {
        return false;
    }
    if (has_trait("MEATARIAN") && eaten->made_of("veggy") && !is_npc() &&
        !query_yn(_("Really eat that %s?  Your stomach won't be happy."), eaten->tname().c_str())) {
        return false;
    }
    if (has_trait("LACTOSE") && eaten->made_of("milk") && (!has_bionic("bio_digestion")) && !is_npc() &&
        !query_yn(_("Really eat that %s?  Your stomach won't be happy."), eaten->tname().c_str())) {
        return false;
    }
    if (has_trait("ANTIFRUIT") && eaten->made_of("fruit") && !is_npc() &&
        !query_yn(_("Really eat that %s?  Your stomach won't be happy."), eaten->tname().c_str())) {
        return false;
    }
    if (has_trait("ANTIJUNK") && eaten->made_of("junk") && (!has_bionic("bio_digestion")) && !is_npc() &&
        !query_yn(_("Really eat that %s?  Your stomach won't be happy."), eaten->tname().c_str())) {
        return false;
    }
    if (has_trait("ANTIWHEAT") && eaten->made_of("wheat") &&
        (!has_bionic("bio_digestion")) && !is_npc() &&
        !query_yn(_("Really eat that %s?  Your stomach won't be happy."), eaten->tname().c_str())) {
        return false;
    }
    if (has_trait("CARNIVORE") && ((eaten->made_of("junk")) && !(eaten->made_of("flesh") ||
      eaten->made_of("hflesh") || eaten->made_of("iflesh") || eaten->made_of("milk") ||
      eaten->made_of("egg")) ) && (!has_bionic("bio_digestion")) && !is_npc() &&
        !query_yn(_("Really eat that %s?  It smells completely unappealing."), eaten->tname().c_str()) ) {
        return false;
    }
    // Check for eating/Food is so water and other basic liquids that do not rot don't cause problems.
    // I'm OK with letting plants drink coffee. (Whether it would count as cannibalism is another story.)
    if ((has_trait("SAPROPHAGE") && (!spoiled) && (!has_bionic("bio_digestion")) && !is_npc() &&
      (eaten->has_flag("USE_EAT_VERB") || comest->comesttype == "FOOD") &&
      !query_yn(_("Really eat that %s?  Your stomach won't be happy."), eaten->tname().c_str()))) {
        //~ No, we don't eat "rotten" food. We eat properly aged food, like a normal person.
        //~ Semantic difference, but greatly facilitates people being proud of their character.
        add_msg_if_player(m_info,  _("It's too fresh, let it age a little first.  "));
        return false;
    }

    if (spoiled) {
        if (is_npc()) {
            return false;
        }
        if ((!(has_trait("SAPROVORE") || has_trait("SAPROPHAGE"))) &&
            !query_yn(_("This %s smells awful!  Eat it?"), eaten->tname().c_str())) {
            return false;
        }
    }

    //not working directly in the equation... can't imagine why
    int temp_hunger = get_hunger() - nutrition_for(comest);
    int temp_thirst = thirst - comest->quench;
    int capacity = has_trait("GOURMAND") ? -60 : -20;
    if( has_active_mutation("HIBERNATE") && !is_npc() &&
        // If BOTH hunger and thirst are above the capacity...
        ( get_hunger() > capacity && thirst > capacity ) &&
        // ...and EITHER of them crosses under the capacity...
        ( temp_hunger < capacity || temp_thirst < capacity ) ) {
        // Prompt to make sure player wants to gorge for hibernation...
        if( query_yn(_("Start gorging in preparation for hibernation?")) ) {
            // ...and explain what that means.
            add_msg(m_info, _("As you force yourself to eat, you have the feeling that you'll just be able to keep eating and then sleep for a long time."));
        } else {
            return false;
        }
    }

    if ( has_active_mutation("HIBERNATE") ) {
        capacity = -620;
    }
    if ( has_trait("GIZZARD") ) {
        capacity = 0;
    }

    if( has_trait("SLIMESPAWNER") && !is_npc() ) {
        capacity -= 40;
        if ( (temp_hunger < capacity && temp_thirst <= (capacity + 10) ) ||
        (temp_thirst < capacity && temp_hunger <= (capacity + 10) ) ) {
            add_msg(m_mixed, _("You feel as though you're going to split open!  In a good way??"));
            mod_pain(5);
            std::vector<tripoint> valid;
            for (int x = posx() - 1; x <= posx() + 1; x++) {
                for (int y = posy() - 1; y <= posy() + 1; y++) {
                    tripoint dest(x, y, posz());
                    if (g->is_empty( dest )) {
                        valid.push_back( dest );
                    }
                }
            }
            int numslime = 1;
            for (int i = 0; i < numslime && !valid.empty(); i++) {
                const tripoint target = random_entry_removed( valid );
                if (g->summon_mon(mon_player_blob, target)) {
                    monster *slime = g->monster_at( target );
                    slime->friendly = -1;
                }
            }
            mod_hunger(40);
            thirst += 40;
            //~slimespawns have *small voices* which may be the Nice equivalent
            //~of the Rat King's ALL CAPS invective.  Probably shared-brain telepathy.
            add_msg(m_good, _("hey, you look like me! let's work together!"));
        }
    }

    if( ( nutrition_for(comest) > 0 && temp_hunger < capacity ) ||
        ( comest->quench > 0 && temp_thirst < capacity ) ) {
        if ((spoiled) && !(has_trait("SAPROPHAGE")) ){//rotten get random nutrification
            if (!query_yn(_("You can hardly finish it all.  Consume it?"))) {
                return false;
            }
        } else {
            if ( (( nutrition_for(comest) > 0 && temp_hunger < capacity ) ||
              ( comest->quench > 0 && temp_thirst < capacity )) &&
              ( (!(has_trait("EATHEALTH"))) || (!(has_trait("SLIMESPAWNER"))) ) ) {
                if (!query_yn(_("You will not be able to finish it all.  Consume it?"))) {
                return false;
                }
            }
        }
    }

    if (comest->has_use()) {
        to_eat = comest->invoke( this, eaten, pos3() );
        if( to_eat <= 0 ) {
            return false;
        }
    }

    if ( (spoiled) && !(has_trait("SAPROPHAGE")) ) {
        add_msg(m_bad, _("Ick, this %s doesn't taste so good..."), eaten->tname().c_str());
        if (!has_trait("SAPROVORE") && !has_trait("EATDEAD") &&
       (!has_bionic("bio_digestion") || one_in(3))) {
            add_effect("foodpoison", rng(60, (nutrition_for(comest) + 1) * 60));
        }
        consume_effects(eaten, comest, spoiled);
    } else if ((spoiled) && has_trait("SAPROPHAGE")) {
        add_msg(m_good, _("Mmm, this %s tastes delicious..."), eaten->tname().c_str());
        consume_effects(eaten, comest, spoiled);
    } else {
        consume_effects(eaten, comest);
        if (!(has_trait("GOURMAND") || has_active_mutation("HIBERNATE") || has_trait("EATHEALTH"))) {
            if ((overeating && rng(-200, 0) > get_hunger())) {
                vomit();
            }
        }
    }
    // At this point, we've definitely eaten the item, so use up some turns.
    int mealtime = 250;
      if (has_trait("MOUTH_TENTACLES")  || has_trait ("MANDIBLES")) {
        mealtime /= 2;
    } if (has_trait("GOURMAND")) {
        mealtime -= 100;
    } if ((has_trait("BEAK_HUM")) &&
      (comest->comesttype == "FOOD" || eaten->has_flag("USE_EAT_VERB")) ) {
        mealtime += 200; // Much better than PROBOSCIS but still optimized for fluids
    } if (has_trait("SABER_TEETH")) {
        mealtime += 250; // They get In The Way
    } if (has_trait("AMORPHOUS")) {
        mealtime *= 1.1; // Minor speed penalty for having to flow around it
                          // rather than just grab & munch
    }
        moves -= (mealtime);

    // If it's poisonous... poison us.  TODO: More several poison effects
    if (eaten->poison > 0) {
        if (!has_trait("EATPOISON") && !has_trait("EATDEAD")) {
            if (eaten->poison >= rng(2, 4)) {
                add_effect("poison", eaten->poison * 100);
            }
            add_effect("foodpoison", eaten->poison * 300);
        }
    }


    if (has_trait("AMORPHOUS")) {
        add_msg_player_or_npc(_("You assimilate your %s."), _("<npcname> assimilates a %s."),
                                  eaten->tname().c_str());
    } else if (comest->comesttype == "DRINK" && !eaten->has_flag("USE_EAT_VERB")) {
        add_msg_player_or_npc( _("You drink your %s."), _("<npcname> drinks a %s."),
                                  eaten->tname().c_str());
    } else if (comest->comesttype == "FOOD" || eaten->has_flag("USE_EAT_VERB")) {
        add_msg_player_or_npc( _("You eat your %s."), _("<npcname> eats a %s."),
                                  eaten->tname().c_str());
    }

    // Moved this later in the process, so you actually eat it before converting to HP
    if ( (has_trait("EATHEALTH")) && ( nutrition_for(comest) > 0 && temp_hunger < capacity ) ) {
        int room = (capacity - temp_hunger);
        int excess_food = ((nutrition_for(comest)) - room);
        add_msg_player_or_npc( _("You feel the %s filling you out."),
                                 _("<npcname> looks better after eating the %s."),
                                  eaten->tname().c_str());
        // Guaranteed 1 HP healing, no matter what.  You're welcome.  ;-)
        if (excess_food <= 5) {
            healall(1);
        }
        // Straight conversion, except it's divided amongst all your body parts.
        else healall(excess_food /= 5);
    }

    if (item::find_type( comest->tool )->is_tool() ) {
        use_charges(comest->tool, 1); // Tools like lighters get used
    }

    if( has_bionic("bio_ethanol") && comest->can_use( "ALCOHOL" ) ) {
        charge_power(rng(50, 200));
    }
    if( has_bionic("bio_ethanol") && comest->can_use( "ALCOHOL_WEAK" ) ) {
        charge_power(rng(25, 100));
    }
    if( has_bionic("bio_ethanol") && comest->can_use( "ALCOHOL_STRONG" ) ) {
        charge_power(rng(75, 300));
    }
    //eating plant fertilizer stops here
    if (has_trait("THRESH_PLANT") && comest->can_use( "PLANTBLECH" )){
        return true;
    }
    if (eaten->made_of("hflesh") && !has_trait("SAPIOVORE")) {
    // Sapiovores don't recognize humans as the same species.
    // It's not cannibalism if you're not eating your own kind.
      if (has_trait("CANNIBAL") && has_trait("PSYCHOPATH") && has_trait("SPIRITUAL")) {
          add_msg_if_player(m_good, _("You feast upon the human flesh, and in doing so, devour their spirit."));
          add_morale(MORALE_CANNIBAL, 25, 300); // You're not really consuming anything special; you just think you are.
      } else if (has_trait("CANNIBAL") && has_trait("PSYCHOPATH")) {
          add_msg_if_player(m_good, _("You feast upon the human flesh."));
          add_morale(MORALE_CANNIBAL, 15, 200);
      } else if (has_trait("PSYCHOPATH") && !has_trait("CANNIBAL") && has_trait("SPIRITUAL")) {
          add_msg_if_player( _("You greedily devour the taboo meat."));
          add_morale(MORALE_CANNIBAL, 5, 50); // Small bonus for violating a taboo.
      } else if (has_trait("PSYCHOPATH") && !has_trait("CANNIBAL")) {
          add_msg_if_player( _("Meh. You've eaten worse."));
      } else if (!has_trait("PSYCHOPATH") && has_trait("CANNIBAL") && has_trait("SPIRITUAL")) {
          add_msg_if_player(m_good, _("You consume the sacred human flesh."));
          add_morale(MORALE_CANNIBAL, 15, 200); // Boosted because you understand the philosophical implications of your actions, and YOU LIKE THEM.
      } else if (!has_trait("PSYCHOPATH") && has_trait("CANNIBAL")) {
          add_msg_if_player(m_good, _("You indulge your shameful hunger."));
          add_morale(MORALE_CANNIBAL, 10, 50);
      } else if (!has_trait("PSYCHOPATH") && has_trait("SPIRITUAL")) {
          add_msg_if_player(m_bad, _("This is probably going to count against you if there's still an afterlife."));
          add_morale(MORALE_CANNIBAL, -60, -400, 600, 300);
      } else {
          add_msg_if_player(m_bad, _("You feel horrible for eating a person."));
          add_morale(MORALE_CANNIBAL, -60, -400, 600, 300);
      }
    }
    if (has_trait("VEGETARIAN") && (eaten->made_of("flesh") || eaten->made_of("hflesh") || eaten->made_of("iflesh"))) {
        add_msg_if_player(m_bad, _("Yuck! How can anybody eat this stuff?"));
        add_morale(MORALE_VEGETARIAN, -75, -400, 300, 240);
    }
    if (has_trait("MEATARIAN") && eaten->made_of("veggy")) {
        add_msg_if_player(m_bad, _("Yuck! How can anybody eat this stuff?"));
        add_morale(MORALE_MEATARIAN, -75, -400, 300, 240);
    }
    if (has_trait("LACTOSE") && eaten->made_of("milk")) {
        add_msg_if_player(m_bad, _("Your stomach begins gurgling and you feel bloated and ill."));
        add_morale(MORALE_LACTOSE, -75, -400, 300, 240);
    }
    if (has_trait("ANTIFRUIT") && eaten->made_of("fruit")) {
        add_msg_if_player(m_bad, _("Yuck! How can anybody eat this stuff?"));
        add_morale(MORALE_ANTIFRUIT, -75, -400, 300, 240);
    }
    if (has_trait("ANTIJUNK") && eaten->made_of("junk")) {
        add_msg_if_player(m_bad, _("Yuck! How can anybody eat this stuff?"));
        add_morale(MORALE_ANTIJUNK, -75, -400, 300, 240);
    }
    if (has_trait("ANTIWHEAT") && eaten->made_of("wheat")) {
        add_msg_if_player(m_bad, _("Your stomach begins gurgling and you feel bloated and ill."));
        add_morale(MORALE_ANTIWHEAT, -75, -400, 300, 240);
    }
    // Carnivores CAN eat junk food, but they won't like it much.
    // Pizza-scraping happens in consume_effects.
    if (has_trait("CARNIVORE") && ((eaten->made_of("junk")) && !(eaten->made_of("flesh") ||
    eaten->made_of("hflesh") || eaten->made_of("iflesh") || eaten->made_of("milk") ||
    eaten->made_of("egg")) ) ) {
        add_msg_if_player(m_bad, _("Your stomach begins gurgling and you feel bloated and ill."));
        add_morale(MORALE_NO_DIGEST, -25, -125, 300, 240);
    }
    if (has_trait("SAPROPHAGE") && !(spoiled) && (eaten->has_flag("USE_EAT_VERB") ||
    comest->comesttype == "FOOD")) {
    // It's OK to *drink* things that haven't rotted.  Alternative is to ban water.  D:
        add_msg_if_player(m_bad, _("Your stomach begins gurgling and you feel bloated and ill."));
        add_morale(MORALE_NO_DIGEST, -75, -400, 300, 240);
    }
    if ((!crossed_threshold() || has_trait("THRESH_URSINE")) && mutation_category_level["MUTCAT_URSINE"] > 40
        && eaten->made_of("honey")) {
        //Need at least 5 bear muts for effect to show, to filter out mutations in common with other mutcats
        int honey_fun = has_trait("THRESH_URSINE") ?
            std::min(mutation_category_level["MUTCAT_URSINE"]/8, 20) :
            mutation_category_level["MUTCAT_URSINE"]/12;
        if (honey_fun < 10)
            add_msg_if_player(m_good, _("You find the sweet taste of honey surprisingly palatable."));
        else
            add_msg_if_player(m_good, _("You feast upon the sweet honey."));
        add_morale(MORALE_HONEY, honey_fun, 100);
    }
    if( (has_trait("HERBIVORE") || has_trait("RUMINANT")) &&
        (eaten->made_of("flesh") || eaten->made_of("egg")) ) {
        add_msg_if_player(m_bad, _("Your stomach immediately revolts, you can't keep this disgusting stuff down."));
        if( !one_in(3) ) {
            vomit();
        }
    }
    return true;
}

int player::nutrition_for(const it_comest *comest)
{
    /* thresholds:
    **  100 : 1x
    **  300 : 2x
    ** 1400 : 4x
    ** 2800 : 6x
    ** 6000 : 10x
    */

    float nutr;

    if (get_hunger() < 100) {
        nutr = comest->nutr;
    } else if (get_hunger() <= 300) {
        nutr = ((float)get_hunger()/300) * 2 * comest->nutr;
    } else if (get_hunger() <= 1400) {
        nutr = ((float)get_hunger()/1400) * 4 * comest->nutr;
    } else if (get_hunger() <= 2800) {
        nutr = ((float)get_hunger()/2800) * 6 * comest->nutr;
    } else {
        nutr = ((float)get_hunger()/6000)* 10 * comest->nutr;
    }

    return (int)nutr;
}

void player::consume_effects(item *eaten, const it_comest *comest, bool rotten)
{
    if (has_trait("THRESH_PLANT") && comest->can_use( "PLANTBLECH" )) {
        return;
    }
    if( (has_trait("HERBIVORE") || has_trait("RUMINANT")) &&
        (eaten->made_of("flesh") || eaten->made_of("egg")) ) {
        // No good can come of this.
        return;
    }
    float factor = 1.0;
    float hunger_factor = 1.0;
    bool unhealthy_allowed = true;

    if (has_trait("GIZZARD")) {
        factor *= .6;
    }
    if (has_trait("CARNIVORE")) {
        // At least partially edible
        if(eaten->made_of("flesh") || eaten->made_of("hflesh") || eaten->made_of("iflesh") ||
              eaten->made_of("milk") || eaten->made_of("egg")) {
            // Other things are in it, we only get partial benefits
            if ((eaten->made_of("veggy") || eaten->made_of("fruit") || eaten->made_of("wheat"))) {
                factor *= .5;
            } else {
                // Carnivores don't get unhealthy off pure meat diets
                unhealthy_allowed = false;
            }
        }
    }
    // Saprophages get full nutrition from rotting food
    if (rotten && !has_trait("SAPROPHAGE")) {
        // everyone else only gets a portion of the nutrition
        hunger_factor *= rng_float(0, 1);
        // and takes a health penalty if they aren't adapted
        if (!has_trait("SAPROVORE") && !has_bionic("bio_digestion")) {
            mod_healthy_mod(-30, -200);
        }
    }

    // Bio-digestion gives extra nutrition
    if (has_bionic("bio_digestion")) {
        hunger_factor += rng_float(0, 1);
    }

    mod_hunger(-nutrition_for(comest) * factor * hunger_factor);
    thirst -= comest->quench * factor;
    mod_stomach_food(nutrition_for(comest) * factor * hunger_factor);
    mod_stomach_water(comest->quench * factor);
    if( unhealthy_allowed || comest->healthy > 0 ) {
        // Effectively no upper cap on healthy food, moderate cap on unhealthy food.
        mod_healthy_mod( comest->healthy, (comest->healthy >= 0) ? 200 : -50 );
    }

    if (comest->stim != 0) {
        if (abs(stim) < (abs(comest->stim) * 3)) {
            if (comest->stim < 0) {
                stim = std::max(comest->stim * 3, stim + comest->stim);
            } else {
                stim = std::min(comest->stim * 3, stim + comest->stim);
            }
        }
    }
    add_addiction(comest->add, comest->addict);
    if (addiction_craving(comest->add) != MORALE_NULL) {
        rem_morale(addiction_craving(comest->add));
    }
    if (eaten->has_flag("HOT") && eaten->has_flag("EATEN_HOT")) {
        add_morale(MORALE_FOOD_HOT, 5, 10);
    }
    auto fun = comest->fun;
    if (eaten->has_flag("COLD") && eaten->has_flag("EATEN_COLD") && fun > 0) {
            add_morale(MORALE_FOOD_GOOD, fun * 3, fun * 3, 60, 30, false, comest);
    }
    if (eaten->has_flag("COLD") && eaten->has_flag("EATEN_COLD") && fun <= 0) {
            fun = 1;
    }
    if (has_trait("GOURMAND")) {
        if (fun < -2) {
            add_morale(MORALE_FOOD_BAD, fun * 0.5, fun, 60, 30, false, comest);
        } else if (fun > 0) {
            add_morale(MORALE_FOOD_GOOD, fun * 3, fun * 6, 60, 30, false, comest);
        }
        if (has_trait("GOURMAND") && !(has_active_mutation("HIBERNATE"))) {
            if ((nutrition_for(comest) > 0 && get_hunger() < -60) || (comest->quench > 0 && thirst < -60)) {
                add_msg_if_player(_("You can't finish it all!"));
            }
            if (get_hunger() < -60) {
                set_hunger(-60);
            }
            if (thirst < -60) {
                thirst = -60;
            }
        }
    } if (has_active_mutation("HIBERNATE")) {
         if ((nutrition_for(comest) > 0 && get_hunger() < -60) || (comest->quench > 0 && thirst < -60)) { //Tell the player what's going on
            add_msg_if_player(_("You gorge yourself, preparing to hibernate."));
            if (one_in(2)) {
                (fatigue += (nutrition_for(comest))); //50% chance of the food tiring you
            }
        }
        if ((nutrition_for(comest) > 0 && get_hunger() < -200) || (comest->quench > 0 && thirst < -200)) { //Hibernation should cut burn to 60/day
            add_msg_if_player(_("You feel stocked for a day or two. Got your bed all ready and secured?"));
            if (one_in(2)) {
                (fatigue += (nutrition_for(comest))); //And another 50%, intended cumulative
            }
        }

        if ((nutrition_for(comest) > 0 && get_hunger() < -400) || (comest->quench > 0 && thirst < -400)) {
            add_msg_if_player(_("Mmm.  You can still fit some more in...but maybe you should get comfortable and sleep."));
             if (!(one_in(3))) {
                (fatigue += (nutrition_for(comest))); //Third check, this one at 66%
            }
        }
        if ((nutrition_for(comest) > 0 && get_hunger() < -600) || (comest->quench > 0 && thirst < -600)) {
            add_msg_if_player(_("That filled a hole!  Time for bed..."));
            fatigue += (nutrition_for(comest)); //At this point, you're done.  Schlaf gut.
        }
        if ((nutrition_for(comest) > 0 && get_hunger() < -620) || (comest->quench > 0 && thirst < -620)) {
            add_msg_if_player(_("You can't finish it all!"));
        }
        if (get_hunger() < -620) {
            set_hunger(-620);
        }
        if (thirst < -620) {
            thirst = -620;
        }
    } else {
        if (fun < 0) {
            add_morale(MORALE_FOOD_BAD, fun, fun * 6, 60, 30, false, comest);
        } else if (fun > 0) {
            add_morale(MORALE_FOOD_GOOD, fun, fun * 4, 60, 30, false, comest);
        }
        if ((nutrition_for(comest) > 0 && get_hunger() < -20) || (comest->quench > 0 && thirst < -20)) {
            add_msg_if_player(_("You can't finish it all!"));
        }
        if (get_hunger() < -20) {
            set_hunger(-20);
        }
        if (thirst < -20) {
            thirst = -20;
        }
    }
}

void player::rooted_message() const
{
    if( (has_trait("ROOTS2") || has_trait("ROOTS3") ) &&
        g->m.has_flag("DIGGABLE", posx(), posy()) &&
        !footwear_factor() ) {
        add_msg(m_info, _("You sink your roots into the soil."));
    }
}

void player::rooted()
// Should average a point every two minutes or so; ground isn't uniformly fertile
// Overfiling triggered hibernation checks, so capping.
{
    double shoe_factor = footwear_factor();
    if( (has_trait("ROOTS2") || has_trait("ROOTS3")) &&
        g->m.has_flag("DIGGABLE", posx(), posy()) && shoe_factor != 1.0 ) {
        if( one_in(20.0 / (1.0 - shoe_factor)) ) {
            if (get_hunger() > -20) {
                mod_hunger(-1);
            }
            if (thirst > -20) {
                thirst--;
            }
            mod_healthy_mod(5, 50);
        }
    }
}

bool player::can_wield( const item &it, bool interactive ) const
{
    if( it.is_two_handed(*this) && !has_two_arms() ) {
        if( it.has_flag("ALWAYS_TWOHAND") ) {
            if( interactive ) {
                add_msg( m_info, _("The %s can't be wielded with only one arm."), it.tname().c_str() );
            }
        } else {
            if( interactive ) {
                add_msg( m_info, _("You are too weak to wield %s with only one arm."),
                         it.tname().c_str() );
            }
        }
        return false;
    }
    return true;
}

bool player::wield(item* it, bool autodrop)
{
    if (weapon.has_flag("NO_UNWIELD")) {
        add_msg(m_info, _("You cannot unwield your %s!  Withdraw them with 'p'."),
                weapon.tname().c_str());
        return false;
    }
    if (it == NULL || it->is_null()) {
        if(weapon.is_null()) {
            return false;
        }
        if (autodrop || volume_carried() + weapon.volume() <= volume_capacity()) {
            moves -= item_handling_cost(weapon);
            inv.add_item_keep_invlet(remove_weapon());
            inv.unsort();
            recoil = MIN_RECOIL;
            return true;
        } else if (query_yn(_("No space in inventory for your %s.  Drop it?"),
                   weapon.tname().c_str())) {
            g->m.add_item_or_charges(posx(), posy(), remove_weapon());
            recoil = MIN_RECOIL;
            return true;
        } else {
            return false;
        }
    }
    if (&weapon == it) {
        add_msg(m_info, _("You're already wielding that!"));
        return false;
    } else if (it == NULL || it->is_null()) {
        add_msg(m_info, _("You don't have that item."));
        return false;
    }

    if( !can_wield( *it ) ) {
        return false;
    }

    int mv = 0;

    if( is_armed() ) {
        if( volume_carried() + weapon.volume() - it->volume() < volume_capacity() ) {
            mv += item_handling_cost(weapon);
            inv.add_item_keep_invlet( remove_weapon() );
        } else if( query_yn(_("No space in inventory for your %s.  Drop it?"),
                            weapon.tname().c_str() ) ) {
            g->m.add_item_or_charges( posx(), posy(), remove_weapon() );
        } else {
            return false;
        }
        inv.unsort();
    }

    // Wielding from inventory is relatively slow and does not improve with increasing weapon skill.
    // Worn items (including guns with shoulder straps) are faster but still slower
    // than a skilled player with a holster.
    // There is an additional penalty when wielding items from the inventory whilst currently grabbed.

    mv += item_handling_cost(*it);

    if( is_worn( *it ) ) {
        it->on_takeoff( *this );
    } else {
        mv *= 2;
    }

    moves -= mv;

    weapon = i_rem( it );
    last_item = itype_id( weapon.type->id );

    weapon.on_wield( *this, mv );

    return true;
}

// ids of martial art styles that are available with the bio_cqb bionic.
static const std::array<matype_id, 4> bio_cqb_styles {{
    matype_id{ "style_karate" }, matype_id{ "style_judo" }, matype_id{ "style_muay_thai" }, matype_id{ "style_biojutsu" }
}};

class ma_style_callback : public uimenu_callback
{
public:
    virtual bool key(int key, int entnum, uimenu *menu) override {
        if( key != '?' ) {
            return false;
        }
        matype_id style_selected;
        const size_t index = entnum;
        if( g->u.has_active_bionic( "bio_cqb" ) && index < menu->entries.size() ) {
            const size_t id = menu->entries[index].retval - 2;
            if( id < bio_cqb_styles.size() ) {
                style_selected = bio_cqb_styles[id];
            }
        } else if( index >= 3 && index - 3 < g->u.ma_styles.size() ) {
            style_selected = g->u.ma_styles[index - 3];
        }
        if( !style_selected.str().empty() ) {
            const martialart &ma = style_selected.obj();
            std::ostringstream buffer;
            buffer << ma.name << "\n\n \n\n";
            if( !ma.techniques.empty() ) {
                buffer << ngettext( "Technique:", "Techniques:", ma.techniques.size() ) << " ";
                for( auto technique = ma.techniques.cbegin();
                     technique != ma.techniques.cend(); ++technique ) {
                    buffer << technique->obj().name;
                    if( ma.techniques.size() > 1 && technique == ----ma.techniques.cend() ) {
                        //~ Seperators that comes before last element of a list.
                        buffer << _(" and ");
                    } else if( technique != --ma.techniques.cend() ) {
                        //~ Seperator for a list of items.
                        buffer << _(", ");
                    }
                }
            }
            if( !ma.weapons.empty() ) {
                buffer << "\n\n \n\n";
                buffer << ngettext( "Weapon:", "Weapons:", ma.weapons.size() ) << " ";
                for( auto weapon = ma.weapons.cbegin(); weapon != ma.weapons.cend(); ++weapon ) {
                    buffer << item::nname( *weapon );
                    if( ma.weapons.size() > 1 && weapon == ----ma.weapons.cend() ) {
                        //~ Seperators that comes before last element of a list.
                        buffer << _(" and ");
                    } else if( weapon != --ma.weapons.cend() ) {
                        //~ Seperator for a list of items.
                        buffer << _(", ");
                    }
                }
            }
            popup(buffer.str(), PF_NONE);
            menu->redraw();
        }
        return true;
    }
    virtual ~ma_style_callback() { }
};

bool player::pick_style() // Style selection menu
{
    //Create menu
    // Entries:
    // 0: Cancel
    // 1: No style
    // x: dynamic list of selectable styles

    //If there are style already, cursor starts there
    // if no selected styles, cursor starts from no-style

    // Any other keys quit the menu

    uimenu kmenu;
    kmenu.text = _("Select a style (press ? for more info)");
    std::unique_ptr<ma_style_callback> ma_style_info(new ma_style_callback());
    kmenu.callback = ma_style_info.get();
    kmenu.desc_enabled = true;
    kmenu.addentry( 0, true, 'c', _("Cancel") );
    if (keep_hands_free) {
      kmenu.addentry_desc( 1, true, 'h', _("Keep hands free (on)"), _("When this is enabled, player won't wield things unless explicitly told to."));
    }
    else {
      kmenu.addentry_desc( 1, true, 'h', _("Keep hands free (off)"), _("When this is enabled, player won't wield things unless explicitly told to."));
    }

    if (has_active_bionic("bio_cqb")) {
        for(size_t i = 0; i < bio_cqb_styles.size(); i++) {
            if( bio_cqb_styles[i].is_valid() ) {
                auto &style = bio_cqb_styles[i].obj();
                kmenu.addentry_desc( i + 2, true, -1, style.name, style.description );
            }
        }

        kmenu.query();
        const size_t selection = kmenu.ret;
        if ( selection >= 2 && selection - 2 < bio_cqb_styles.size() ) {
            style_selected = bio_cqb_styles[selection - 2];
        }
        else if ( selection == 1 ) {
            keep_hands_free = !keep_hands_free;
        } else {
            return false;
        }
    }
    else {
        kmenu.addentry( 2, true, 'n', _("No style") );
        kmenu.selected = 2;
        kmenu.return_invalid = true; //cancel with any other keys

        for (size_t i = 0; i < ma_styles.size(); i++) {
            auto &style = ma_styles[i].obj();
                //Check if this style is currently selected
                if( ma_styles[i] == style_selected ) {
                    kmenu.selected =i+3; //+3 because there are "cancel", "keep hands free" and "no style" first in the list
                }
                kmenu.addentry_desc( i+3, true, -1, style.name , style.description );
        }

        kmenu.query();
        int selection = kmenu.ret;

        //debugmsg("selected %d",choice);
        if (selection >= 3)
            style_selected = ma_styles[selection - 3];
        else if (selection == 2)
            style_selected = matype_id( "style_none" );
        else if (selection == 1)
            keep_hands_free = !keep_hands_free;
        else
            return false;
    }

    return true;
}

hint_rating player::rate_action_wear( const item &it ) const
{
    //TODO flag already-worn items as HINT_IFFY

    if (!it.is_armor()) {
        return HINT_CANT;
    }

    // are we trying to put on power armor? If so, make sure we don't have any other gear on.
    if (it.is_power_armor() && worn.size()) {
        if (it.covers(bp_torso)) {
            return HINT_IFFY;
        } else if (it.covers(bp_head) && !worn.front().is_power_armor()) {
            return HINT_IFFY;
        }
    }
    // are we trying to wear something over power armor? We can't have that, unless it's a backpack, or similar.
    if (worn.size() && worn.front().is_power_armor() && !(it.covers(bp_head))) {
        if (!(it.covers(bp_torso) && it.color() == c_green)) {
            return HINT_IFFY;
        }
    }

    // Make sure we're not wearing 2 of the item already
    int count = 0;
    for (auto &i : worn) {
        if (i.type->id == it.type->id) {
            count++;
        }
    }
    if (count == 2) {
        return HINT_IFFY;
    }
    if (has_trait("WOOLALLERGY") && (it.made_of("wool") || it.item_tags.count("wooled") > 0)) {
        return HINT_IFFY; //should this be HINT_CANT? I kinda think not, because HINT_CANT is more for things that can NEVER happen
    }
    if (it.covers(bp_head) && encumb(bp_head) != 0) {
        return HINT_IFFY;
    }
    if ((it.covers(bp_hand_l) || it.covers(bp_hand_r)) && has_trait("WEBBED")) {
        return HINT_IFFY;
    }
    if ((it.covers(bp_hand_l) || it.covers(bp_hand_r)) && has_trait("TALONS")) {
        return HINT_IFFY;
    }
    if ((it.covers(bp_hand_l) || it.covers(bp_hand_r)) && (has_trait("ARM_TENTACLES") ||
            has_trait("ARM_TENTACLES_4") || has_trait("ARM_TENTACLES_8")) ) {
        return HINT_IFFY;
    }
    if (it.covers(bp_mouth) && (has_trait("BEAK") ||
            has_trait("BEAK_PECK") || has_trait("BEAK_HUM")) ) {
        return HINT_IFFY;
    }
    if ((it.covers(bp_foot_l) || it.covers(bp_foot_r)) && has_trait("HOOVES")) {
        return HINT_IFFY;
    }
    if ((it.covers(bp_foot_l) || it.covers(bp_foot_r)) && has_trait("LEG_TENTACLES")) {
        return HINT_IFFY;
     }
    if (it.covers(bp_head) && has_trait("HORNS_CURLED")) {
        return HINT_IFFY;
    }
    if (it.covers(bp_torso) && (has_trait("SHELL") || has_trait("SHELL2")))  {
        return HINT_IFFY;
    }
    if (it.covers(bp_head) && !it.made_of("wool") &&
          !it.made_of("cotton") && !it.made_of("leather") && !it.made_of("nomex") &&
          (has_trait("HORNS_POINTED") || has_trait("ANTENNAE") || has_trait("ANTLERS"))) {
        return HINT_IFFY;
    }
    // Checks to see if the player is wearing shoes
    if (((it.covers(bp_foot_l) && is_wearing_shoes("left")) ||
          (it.covers(bp_foot_r) && is_wearing_shoes("right"))) &&
          !it.has_flag("BELTED") && !it.has_flag("SKINTIGHT")) {
    return HINT_IFFY;
    }

    return HINT_GOOD;
}


hint_rating player::rate_action_change_side( const item &it ) const {
   if (!is_worn(it)) {
      return HINT_IFFY;
   }

    if (!it.is_sided()) {
        return HINT_CANT;
    }

    return HINT_GOOD;
}

int player::item_handling_cost( const item& it, bool effects, int factor ) const {
    int mv = std::max( 1, it.volume() * factor );
    if( effects && has_effect( "grabbed" ) ) {
        mv *= 2;
    }
    return std::min(mv, MAX_HANDLING_COST);
}

bool player::wear(int inventory_position, bool interactive)
{
    item* to_wear = NULL;
    if (inventory_position == -1)
    {
        to_wear = &weapon;
    }
    else if( inventory_position < -1 ) {
        if( interactive ) {
            add_msg( m_info, _( "You are already wearing that." ) );
        }
        return false;
    }
    else
    {
        to_wear = &inv.find_item(inventory_position);
    }

    if (to_wear->is_null())
    {
        if(interactive)
        {
            add_msg(m_info, _("You don't have that item."));
        }

        return false;
    }

    if (!wear_item(*to_wear, interactive))
    {
        return false;
    }

    if (inventory_position == -1)
    {
        weapon = ret_null;
    }
    else
    {
        // it has been copied into worn vector, but assigned an invlet,
        // in case it's a stack, reset the invlet to avoid duplicates
        to_wear->invlet = 0;
        inv.remove_item(to_wear);
        inv.restack(this);
    }

    return true;
}

bool player::wear_item( const item &to_wear, bool interactive )
{
    if( !to_wear.is_armor() ) {
        add_msg_if_player(m_info, _("Putting on a %s would be tricky."), to_wear.tname().c_str());
        return false;
    }

    // are we trying to put on power armor? If so, make sure we don't have any other gear on.
    if (to_wear.is_power_armor())
    {
        for( auto &elem : worn ) {
            if( ( elem.get_covered_body_parts() & to_wear.get_covered_body_parts() ).any() ) {
                if(interactive)
                {
                    add_msg_if_player(m_info, _("You can't wear power armor over other gear!"));
                }
                return false;
            }
        }

        if (!(to_wear.covers(bp_torso)))
        {
            bool power_armor = false;

            if (worn.size())
            {
                for( auto &elem : worn ) {
                    if( elem.is_power_armor() ) {
                        power_armor = true;
                        break;
                    }
                }
            }

            if (!power_armor)
            {
                if(interactive)
                {
                    add_msg_if_player(m_info, _("You can only wear power armor components with power armor!"));
                }
                return false;
            }
        }

        for (auto &i : worn)
        {
            if (i.is_power_armor() && i.typeId() == to_wear.typeId() )
            {
                if(interactive)
                {
                    add_msg_if_player(m_info, _("You cannot wear more than one %s!"), to_wear.tname().c_str());
                }
                return false;
            }
        }
    }
    else
    {
        // Only headgear can be worn with power armor, except other power armor components
        if(!to_wear.covers(bp_head) && !to_wear.covers(bp_eyes) &&
             !to_wear.covers(bp_head)) {
            for (auto &i : worn)
            {
                if( i.is_power_armor() )
                {
                    if(interactive)
                    {
                        add_msg_if_player(m_info, _("You can't wear %s with power armor!"), to_wear.tname().c_str());
                    }
                    return false;
                }
            }
        }
    }

    // Make sure we're not wearing 2 of the item already
    int count = amount_worn(to_wear.typeId());
    if (count == MAX_WORN_PER_TYPE) {
        if(interactive) {
            add_msg_if_player(m_info, _("You can't wear more than two %s at once."),
                    to_wear.tname(count).c_str());
        }
        return false;
    }

    if (!to_wear.has_flag("OVERSIZE")) {
        if (has_trait("WOOLALLERGY") && (to_wear.made_of("wool") || to_wear.item_tags.count("wooled"))) {
            if(interactive) {
                add_msg_if_player(m_info, _("You can't wear that, it's made of wool!"));
            }
            return false;
        }

        // this simply checked if it was zero, I've updated this for the new encumb system
        if (to_wear.covers(bp_head) && (encumb(bp_head) > 10) && (!(to_wear.get_encumber() < 9))) {
            if(interactive) {
                add_msg_if_player(m_info, wearing_something_on(bp_head) ?
                                _("You can't wear another helmet!") : _("You can't wear a helmet!"));
            }
            return false;
        }

        if ((to_wear.covers(bp_hand_l) || to_wear.covers(bp_hand_r) ||
              to_wear.covers(bp_arm_l) || to_wear.covers(bp_arm_r) ||
              to_wear.covers(bp_leg_l) || to_wear.covers(bp_leg_r) ||
              to_wear.covers(bp_foot_l) || to_wear.covers(bp_foot_r) ||
              to_wear.covers(bp_torso) || to_wear.covers(bp_head)) &&
            (has_trait("HUGE") || has_trait("HUGE_OK"))) {
            if(interactive) {
                add_msg_if_player(m_info, _("The %s is much too small to fit your huge body!"),
                        to_wear.type_name().c_str());
            }
            return false;
        }

        if( (to_wear.covers(bp_hand_l) || to_wear.covers(bp_hand_r)) && has_trait("WEBBED") ) {
            if( interactive ) {
                add_msg_if_player(m_info, _("You cannot put %s over your webbed hands."), to_wear.type_name().c_str());
            }
            return false;
        }

        if( (to_wear.covers(bp_hand_l) || to_wear.covers(bp_hand_r)) &&
            (has_trait("ARM_TENTACLES") || has_trait("ARM_TENTACLES_4") ||
             has_trait("ARM_TENTACLES_8")) ) {
            if( interactive ) {
                add_msg_if_player(m_info, _("You cannot put %s over your tentacles."), to_wear.type_name().c_str());
            }
            return false;
        }

        if( (to_wear.covers(bp_hand_l) || to_wear.covers(bp_hand_r)) && has_trait("TALONS") ) {
            if( interactive ) {
                add_msg_if_player(m_info, _("You cannot put %s over your talons."), to_wear.type_name().c_str());
            }
            return false;
        }

        if( (to_wear.covers(bp_hand_l) || to_wear.covers(bp_hand_r)) &&
            (has_trait("PAWS") || has_trait("PAWS_LARGE")) ) {
            if(interactive) {
                add_msg_if_player(m_info, _("You cannot get %s to stay on your paws."), to_wear.type_name().c_str());
            }
            return false;
        }

        if( to_wear.covers(bp_mouth) && (has_trait("BEAK") || has_trait("BEAK_PECK") ||
                                          has_trait("BEAK_HUM")) ) {
            if( interactive ) {
                add_msg_if_player(m_info, _("You cannot put a %s over your beak."), to_wear.type_name().c_str());
            }
            return false;
        }

        if( to_wear.covers(bp_mouth) &&
            (has_trait("MUZZLE") || has_trait("MUZZLE_BEAR") || has_trait("MUZZLE_LONG") ||
             has_trait("MUZZLE_RAT")) ) {
            if( interactive ) {
                add_msg_if_player(m_info, _("You cannot fit the %s over your muzzle."), to_wear.type_name().c_str());
            }
            return false;
        }

        if( to_wear.covers(bp_mouth) && has_trait("MINOTAUR") ) {
            if( interactive ) {
                add_msg_if_player(m_info, _("You cannot fit the %s over your snout."), to_wear.type_name().c_str());
            }
            return false;
        }

        if( to_wear.covers(bp_mouth) && has_trait("SABER_TEETH") ) {
            if( interactive ) {
                add_msg_if_player(m_info, _("Your saber teeth are simply too large for %s to fit."), to_wear.type_name().c_str());
            }
            return false;
        }

        if( to_wear.covers(bp_mouth) && has_trait("MANDIBLES") ) {
            if( interactive ) {
                add_msg_if_player(_("Your mandibles are simply too large for %s to fit."), to_wear.type_name().c_str());
            }
            return false;
        }

        if( to_wear.covers(bp_mouth) && has_trait("PROBOSCIS") ) {
            if( interactive ) {
                add_msg_if_player(m_info, _("Your proboscis is simply too large for %s to fit."), to_wear.type_name().c_str());
            }
            return false;
        }

        if( (to_wear.covers(bp_foot_l) || to_wear.covers(bp_foot_r)) && has_trait("HOOVES") ) {
            if( interactive ) {
                add_msg_if_player(m_info, _("You cannot wear footwear on your hooves."));
            }
            return false;
        }

        if( (to_wear.covers(bp_foot_l) || to_wear.covers(bp_foot_r)) && has_trait("LEG_TENTACLES") ) {
            if( interactive ) {
                add_msg_if_player(m_info, _("You cannot wear footwear on your tentacles."));
            }
            return false;
        }

        if( (to_wear.covers(bp_foot_l) || to_wear.covers(bp_foot_r)) && has_trait("RAP_TALONS")) {
            if( interactive ) {
                add_msg_if_player(m_info, _("Your talons are much too large for footgear."));
            }
            return false;
        }

        if( to_wear.covers(bp_head) && has_trait("HORNS_CURLED") ) {
            if( interactive ) {
                add_msg_if_player(m_info, _("You cannot wear headgear over your horns."));
            }
            return false;
        }

        if( to_wear.covers(bp_torso) && (has_trait("SHELL") || has_trait("SHELL2")) ) {
            if( interactive ) {
                add_msg_if_player(m_info, _("You cannot fit that over your shell."));
            }
            return false;
        }

        if( to_wear.covers(bp_torso) &&
            ((has_trait("INSECT_ARMS")) || (has_trait("ARACHNID_ARMS"))) ) {
            if( interactive ) {
                add_msg_if_player(m_info, _("Your new limbs are too wriggly to fit under that."));
            }
            return false;
        }

        if( to_wear.covers(bp_head) &&
            !to_wear.made_of("wool") && !to_wear.made_of("cotton") &&
            !to_wear.made_of("nomex") && !to_wear.made_of("leather") &&
            (has_trait("HORNS_POINTED") || has_trait("ANTENNAE") || has_trait("ANTLERS"))) {
            if( interactive ) {
                add_msg_if_player(m_info, _("You cannot wear a helmet over your %s."),
                           (has_trait("HORNS_POINTED") ? _("horns") :
                            (has_trait("ANTENNAE") ? _("antennae") : _("antlers"))));
            }
            return false;
        }

        if (((to_wear.covers(bp_foot_l) && is_wearing_shoes("left")) ||
              (to_wear.covers(bp_foot_r) && is_wearing_shoes("right"))) &&
              !to_wear.has_flag("BELTED") && !to_wear.has_flag("SKINTIGHT")) {
            // Checks to see if the player is wearing shoes
            if(interactive){
                add_msg_if_player(m_info, _("You're already wearing footwear!"));
            }
            return false;
        }
    }

    const bool was_deaf = is_deaf();
    last_item = itype_id(to_wear.type->id);
    worn.push_back(to_wear);

    if( interactive ) {
        add_msg( _("You put on your %s."), to_wear.tname().c_str() );
        moves -= 350; // TODO: Make this variable?

        worn.back().on_wear( *this );

        for (body_part i = bp_head; i < num_bp; i = body_part(i + 1))
        {
            if (to_wear.covers(i) && encumb(i) >= 40)
            {
                add_msg_if_player(m_warning,
                    i == bp_eyes ?
                    _("Your %s are very encumbered! %s"):_("Your %s is very encumbered! %s"),
                    body_part_name(body_part(i)).c_str(), encumb_text(body_part(i)).c_str());
            }
        }
        if( !was_deaf && is_deaf() ) {
            add_msg_if_player( m_info, _( "You're deafened!" ) );
        }
    } else {
        add_msg_if_npc( _("<npcname> puts on their %s."), to_wear.tname().c_str() );
    }

    item &new_item = worn.back();
    if( new_item.invlet == 0 ) {
        inv.assign_empty_invlet( new_item, false );
    }

    recalc_sight_limits();

    return true;
}

bool player::change_side (item *target, bool interactive)
{
    return change_side(get_item_position(target), interactive);
}

bool player::change_side (int pos, bool interactive) {
    int idx = worn_position_to_index(pos);
    if (static_cast<size_t>(idx) >= worn.size()) {
        if (interactive) {
            add_msg_if_player(m_info, _("You are not wearing that item."));
        }
        return false;
    }

    auto it = worn.begin();
    std::advance(it, idx);

    if (!it->is_sided()) {
        if (interactive) {
            add_msg_if_player(m_info, _("You cannot swap the side on which your %s is worn."), it->tname().c_str());
        }
        return false;
    }

    it->set_side(it->get_side() == LEFT ? RIGHT : LEFT);

    if (interactive) {
        add_msg_if_player(m_info, _("You swap the side on which your %s is worn."), it->tname().c_str());
        moves -= 250;
    }

    return true;
}

hint_rating player::rate_action_takeoff( const item &it ) const
{
    if (!it.is_armor()) {
        return HINT_CANT;
    }

    if (is_worn(it)) {
      return HINT_GOOD;
    }

    return HINT_IFFY;
}

bool player::takeoff( item *target, bool autodrop, std::vector<item> *items)
{
    return takeoff( get_item_position( target ), autodrop, items );
}

bool player::takeoff(int inventory_position, bool autodrop, std::vector<item> *items)
{
    if (inventory_position == -1) {
        return wield( nullptr, autodrop );
    }

    int worn_index = worn_position_to_index( inventory_position );
    if( static_cast<size_t>( worn_index ) >= worn.size() ) {
        add_msg_if_player( m_info, _("You are not wearing that item.") );
        return false;
    }
    bool taken_off = false;

    auto first_iter = worn.begin();
    std::advance( first_iter, worn_index );
    item &w = *first_iter;

    // Handle power armor.
    if (w.is_power_armor() && w.covers(bp_torso)) {
        // We're trying to take off power armor, but cannot do that if we have a power armor component on!
        for( auto iter = worn.begin(); iter != worn.end(); ) {
            item& other_armor = *iter;

            if( &other_armor == &w || !other_armor.is_power_armor() ) {
                ++iter;
                continue;
            }
            if( !autodrop && items == nullptr ) {
                add_msg_if_player( m_info, _("You can't take off power armor while wearing other power armor components.") );
                return false;
            }

            other_armor.on_takeoff(*this);

            if( items != nullptr ) {
                items->push_back( other_armor );
            } else {
                g->m.add_item_or_charges( pos(), other_armor );
            }
            add_msg_player_or_npc( _("You take off your %s."),
                                   _("<npcname> takes off their %s."),
                                   other_armor.tname().c_str() );
            iter = worn.erase( iter );
            taken_off = true;
        }
    }

    if( items != nullptr ) {
        w.on_takeoff(*this);
        items->push_back( w );
        taken_off = true;
    } else if (autodrop || volume_capacity() - w.get_storage() >= volume_carried() + w.volume()) {
        w.on_takeoff(*this);
        inv.add_item_keep_invlet(w);
        taken_off = true;
    } else if (query_yn(_("No room in inventory for your %s.  Drop it?"), w.tname().c_str())) {
        w.on_takeoff(*this);
        g->m.add_item_or_charges( pos(), w );
        taken_off = true;
    } else {
        taken_off = false;
    }
    if( taken_off ) {
        moves -= 250;    // TODO: Make this variable
        add_msg_player_or_npc( _("You take off your %s."),
                               _("<npcname> takes off their %s."),
                               w.tname().c_str() );
        worn.erase( first_iter );
    }

    recalc_sight_limits();

    return taken_off;
}

void player::use_wielded() {
    use(-1);
}

hint_rating player::rate_action_reload( const item &it ) const
{
    if( it.ammo_capacity() <= 0 || it.ammo_type() == "NULL" ) {
        return HINT_CANT;
    }

    if( it.has_flag("NO_RELOAD") || it.has_flag("RELOAD_AND_SHOOT") ) {
        return HINT_CANT;
    }

    if ( it.ammo_remaining() < it.ammo_capacity() ) {
        return HINT_GOOD;
    }

    if( it.is_gun() ) {
        for( const auto& mod : it.contents ) {
            // @todo deprecate spare magazine
            if( mod.typeId() == "spare_mag" && mod.charges < it.ammo_capacity() ) {
                return HINT_GOOD;
            }
            if (mod.is_auxiliary_gunmod() && mod.ammo_remaining() < mod.ammo_capacity() ) {
                return HINT_GOOD;
            }
        }
    }

   return HINT_IFFY;
}

hint_rating player::rate_action_unload( const item &it ) const
{
    if( ( it.is_container() || it.is_gun() ) && !it.contents.empty() ) {
        // gunmods can also be unloaded
        return HINT_GOOD;
    }

    if( it.ammo_type() == "NULL" || it.has_flag("NO_UNLOAD") ) {
        return HINT_CANT;
    }

    if( it.ammo_remaining() > 0 ) {
        return HINT_GOOD;
    }

    if ( it.ammo_capacity() > 0 ) {
        return HINT_IFFY;
    }

    return HINT_CANT;
}

hint_rating player::rate_action_disassemble( const item &it )
{
    if( can_disassemble( it, crafting_inventory(), false ) ) {
        return HINT_GOOD;
    }

    return HINT_CANT;
}

hint_rating player::rate_action_use( const item &it ) const
{
    if( it.is_tool() ) {
        const auto tool = dynamic_cast<const it_tool*>(it.type);
        if (tool->charges_per_use != 0 && it.charges < tool->charges_per_use) {
            return HINT_IFFY;
        } else {
            return HINT_GOOD;
        }
    } else if (it.is_gunmod()) {
        ///\EFFECT_GUN >0 allows rating estimates for gun modifications
        if (get_skill_level( skill_gun ) == 0) {
            return HINT_IFFY;
        } else {
            return HINT_GOOD;
        }
    } else if (it.is_bionic()) {
        return HINT_GOOD;
    } else if (it.is_food() || it.is_food_container() || it.is_book() || it.is_armor()) {
        return HINT_IFFY; //the rating is subjective, could be argued as HINT_CANT or HINT_GOOD as well
    } else if (it.is_gun()) {
        if (!it.contents.empty()) {
            return HINT_GOOD;
        } else {
            return HINT_IFFY;
        }
    } else if( it.type->has_use() ) {
        return HINT_GOOD;
    }

    return HINT_CANT;
}

bool player::has_enough_charges( const item &it, bool show_msg ) const
{
    const it_tool *tool = dynamic_cast<const it_tool *>( it.type );
    if( tool == NULL || tool->charges_per_use <= 0 ) {
        // If the item is not a tool, it can always be invoked as it does not consume charges.
        return true;
    }
    if( it.has_flag( "USE_UPS" ) ) {
        if( has_charges( "UPS", tool->charges_per_use ) ) {
            return true;
        }
        if( show_msg ) {
            add_msg_if_player( m_info,
                    ngettext( "Your %s needs %d charge from some UPS.",
                              "Your %s needs %d charges from some UPS.",
                              tool->charges_per_use ),
                    it.tname().c_str(), tool->charges_per_use );
        }
        return false;
    } else if( it.charges < tool->charges_per_use ) {
        if( show_msg ) {
            add_msg_if_player( m_info,
                    ngettext( "Your %s has %d charge but needs %d.",
                              "Your %s has %d charges but needs %d.",
                              it.charges ),
                    it.tname().c_str(), it.charges, tool->charges_per_use );
        }
        return false;
    }
    return true;
}

bool player::consume_charges(item *used, long charges_used)
{
    // Non-tools can use charges too - when they're comestibles
    const auto tool = dynamic_cast<const it_tool*>(used->type);
    const auto comest = dynamic_cast<const it_comest*>(used->type);
    if( charges_used <= 0 || (tool == nullptr && comest == nullptr) ) {
        return false;
    }

    if( tool != nullptr && tool->charges_per_use <= 0 ) {
        // An item that doesn't normally expend charges is destroyed instead.
        /* We can't be certain the item is still in the same position,
         * as other items may have been consumed as well, so remove
         * the item directly instead of by its position. */
        i_rem( used );
        return true;
    }

    if( used->has_flag( "USE_UPS" ) ) {
        use_charges( "UPS", charges_used );
        //Replace 1 with charges it needs to use.
        if( used->active && used->charges <= 1 && !has_charges( "UPS", 1 ) ) {
            add_msg_if_player( m_info, _( "You need an UPS of some kind for this %s to work continuously." ), used->tname().c_str() );
        }
    } else {
        used->charges -= std::min( used->charges, charges_used );
    }

    if( comest != nullptr && used->charges <= 0 ) {
        i_rem( used );
        return true;
    }

    // We may have fiddled with the state of the item in the iuse method,
    // so restack to sort things out.
    inv.restack();
    return false;
}

void player::use(int inventory_position)
{
    item* used = &i_at(inventory_position);
    item copy;

    if (used->is_null()) {
        add_msg(m_info, _("You do not have that item."));
        return;
    }

    last_item = itype_id(used->type->id);

    if (used->is_tool()) {
        if( !used->type->has_use() ) {
            add_msg_if_player( _( "You can't do anything interesting with your %s." ), used->tname().c_str() );
            return;
        }

        invoke_item( used );
    } else if (used->is_gunmod()) {
        const auto mod = used->type->gunmod.get();
        ///\EFFECT_GUN allows installation of more difficult gun mods
        if (!(get_skill_level( skill_gun ) >= mod->req_skill)) {
            add_msg(m_info, _("You need to be at least level %d in the marksmanship skill before you \
can install this mod."), mod->req_skill);
            return;
        }
        int gunpos = g->inv(_("Select gun to modify:"));
        item* gun = &(i_at(gunpos));
        if (gun->is_null()) {
            add_msg(m_info, _("You do not have that item."));
            return;
        } else if (!gun->is_gun()) {
            add_msg(m_info, _("That %s is not a weapon."), gun->tname().c_str());
            return;
        } else if( gun->is_gunmod() ) {
            add_msg(m_info, _("That %s is a gunmod, it can not be modded."), gun->tname().c_str());
            return;
        }
        islot_gun* guntype = gun->type->gun.get();
        if (guntype->skill_used == skill_id("pistol") && !mod->used_on_pistol) {
            add_msg(m_info, _("That %s cannot be attached to a handgun."),
                       used->tname().c_str());
            return;
        } else if (guntype->skill_used == skill_id("shotgun") && !mod->used_on_shotgun) {
            add_msg(m_info, _("That %s cannot be attached to a shotgun."),
                       used->tname().c_str());
            return;
        } else if (guntype->skill_used == skill_id("smg") && !mod->used_on_smg) {
            add_msg(m_info, _("That %s cannot be attached to a submachine gun."),
                       used->tname().c_str());
            return;
        } else if (guntype->skill_used == skill_id("rifle") && !mod->used_on_rifle) {
            add_msg(m_info, _("That %s cannot be attached to a rifle."),
                       used->tname().c_str());
            return;
        } else if (guntype->skill_used == skill_id("archery") && !mod->used_on_bow && guntype->ammo == "arrow") {
            add_msg(m_info, _("That %s cannot be attached to a bow."),
                       used->tname().c_str());
            return;
        } else if (guntype->skill_used == skill_id("archery") && !mod->used_on_crossbow && (guntype->ammo == "bolt" || gun->typeId() == "bullet_crossbow")) {
            add_msg(m_info, _("That %s cannot be attached to a crossbow."),
                       used->tname().c_str());
            return;
        } else if (guntype->skill_used == skill_id("launcher") && !mod->used_on_launcher) {
            add_msg(m_info, _("That %s cannot be attached to a launcher."),
                       used->tname().c_str());
            return;
        } else if ( !mod->acceptable_ammo_types.empty() &&
                    mod->acceptable_ammo_types.count(guntype->ammo) == 0 ) {
                add_msg(m_info, _("That %1$s cannot be used on a %2$s."), used->tname().c_str(),
                       ammo_name(guntype->ammo).c_str());
                return;
        } else if (guntype->valid_mod_locations.count(mod->location) == 0) {
            add_msg(m_info, _("Your %s doesn't have a slot for this mod."), gun->tname().c_str());
            return;
        } else if (gun->get_free_mod_locations(mod->location) <= 0) {
            add_msg(m_info, _("Your %1$s doesn't have enough room for another %2$s mod. To remove the mods, \
activate your weapon."), gun->tname().c_str(), _(mod->location.c_str()));
            return;
        }
        if (used->typeId() == "spare_mag" && gun->has_flag("RELOAD_ONE")) {
            add_msg(m_info, _("You can not use a spare magazine in your %s."),
                       gun->tname().c_str());
            return;
        }
        if (mod->location == "magazine" &&
            gun->clip_size() <= 2) {
            add_msg(m_info, _("You can not extend the ammo capacity of your %s."),
                       gun->tname().c_str());
            return;
        }
        if (used->typeId() == "waterproof_gunmod" && gun->has_flag("WATERPROOF_GUN")) {
            add_msg(m_info, _("Your %s is already waterproof."),
                       gun->tname().c_str());
            return;
        }
        if (used->typeId() == "tuned_mechanism" && gun->has_flag("NEVER_JAMS")) {
            add_msg(m_info, _("This %s is eminently reliable. You can't improve upon it this way."),
                       gun->tname().c_str());
            return;
        }
        if (gun->typeId() == "hand_crossbow" && !mod->used_on_pistol) {
          add_msg(m_info, _("Your %s isn't big enough to use that mod.'"), gun->tname().c_str(),
          used->tname().c_str());
          return;
        }
        if (used->typeId() == "brass_catcher" && gun->has_flag("RELOAD_EJECT")) {
            add_msg(m_info, _("You cannot attach a brass catcher to your %s."),
                       gun->tname().c_str());
            return;
        }
        for (auto &i : gun->contents) {
            if (i.type->id == used->type->id) {
                add_msg(m_info, _("Your %1$s already has a %2$s."), gun->tname().c_str(),
                           used->tname().c_str());
                return;
            } else if ((used->typeId() == "clip" || used->typeId() == "clip2") &&
                       (i.type->id == "clip" || i.type->id == "clip2")) {
                add_msg(m_info, _("Your %s already has an extended magazine."),
                           gun->tname().c_str());
                return;
            }
        }
        add_msg(_("You attach the %1$s to your %2$s."), used->tname().c_str(),
                   gun->tname().c_str());
        gun->contents.push_back(i_rem(used));
        return;

    } else if (used->is_bionic()) {
        if( install_bionics( *used->type ) ) {
            i_rem(inventory_position);
        }
        return;
    } else if (used->is_food() || used->is_food_container()) {
        consume(inventory_position);
        return;
    } else if (used->is_book()) {
        read(inventory_position);
        return;
    } else if (used->is_gun()) {
        std::vector<item> &mods = used->contents;
        unsigned imodcount = 0;
        for( auto &gm : mods ){
            if( gm.has_flag("IRREMOVABLE") ){
                imodcount++;
            }
        }
        // Get weapon mod names.
        if (mods.empty() || mods.size() == imodcount ) {
            add_msg(m_info, _("Your %s doesn't appear to be modded."), used->tname().c_str());
            return;
        }
        if( inventory_position < -1 ) {
            // Prevent removal of shoulder straps and thereby making the gun un-wearable again.
            add_msg( _( "You can not modify your %s while it's worn." ), used->tname().c_str() );
            return;
        }
        // Create menu.
        int choice = -1;

        uimenu kmenu;
        kmenu.selected = 0;
        kmenu.text = _("Remove which modification?");
        for (size_t i = 0; i < mods.size(); i++) {
            if( !mods[i].has_flag("IRREMOVABLE") ){
                kmenu.addentry( i, true, -1, mods[i].tname() );
            }
        }
        kmenu.addentry( mods.size(), true, 'r', _("Remove all") );
        kmenu.addentry( mods.size() + 1, true, 'q', _("Cancel") );
        kmenu.query();
        choice = kmenu.ret;

        if (choice < int(mods.size())) {
            const std::string mod = used->contents[choice].tname();
            remove_gunmod(used, unsigned(choice));
            add_msg(_("You remove your %1$s from your %2$s."), mod.c_str(), used->tname().c_str());
        } else if (choice == int(mods.size())) {
            for (int i = used->contents.size() - 1; i >= 0; i--) {
                if( !used->contents[i].has_flag("IRREMOVABLE") ){
                    remove_gunmod(used, i);
                }
            }
            add_msg(_("You remove all the modifications from your %s."), used->tname().c_str());
        } else {
            add_msg(_("Never mind."));
            return;
        }
        // Removing stuff from a gun takes time.
        moves -= int(used->reload_time(*this) / 2);
        return;
    } else if ( used->type->has_use() ) {
        invoke_item( used );
        return;
    } else {
        add_msg(m_info, _("You can't do anything interesting with your %s."),
                used->tname().c_str());
        return;
    }
}

bool player::invoke_item( item* used )
{
    return invoke_item( used, pos() );
}

bool player::invoke_item( item* used, const tripoint &pt )
{
    if( !has_enough_charges( *used, true ) ) {
        return false;
    }

    if( used->type->use_methods.size() < 2 ) {
        const long charges_used = used->type->invoke( this, used, pt );
        return consume_charges( used, charges_used );
    }

    // Food can't be invoked here - it is already invoked as a part of consumption
    if( used->is_food() || used->is_food_container() ) {
        return consume_item( *used );
    }

    uimenu umenu;
    umenu.text = string_format( _("What to do with your %s?"), used->tname().c_str() );
    int num_total = 0;
    for( const auto &um : used->type->use_methods ) {
        bool usable = um.can_call( this, used, false, pt );
        const std::string &aname = um.get_name();
        umenu.addentry( num_total, usable, MENU_AUTOASSIGN, aname );
        num_total++;
    }

    umenu.addentry( num_total, true, 'q', _("Cancel") );
    umenu.query();
    int choice = umenu.ret;
    if( choice < 0 || choice >= num_total ) {
        return false;
    }

    const std::string &method = used->type->use_methods[choice].get_type_name();
    long charges_used = used->type->invoke( this, used, pt, method );
    return consume_charges( used, charges_used );
}

bool player::invoke_item( item* used, const std::string &method )
{
    return invoke_item( used, method, pos() );
}

bool player::invoke_item( item* used, const std::string &method, const tripoint &pt )
{
    if( !has_enough_charges( *used, true ) ) {
        return false;
    }

    item *actually_used = used->get_usable_item( method );
    if( actually_used == nullptr ) {
        debugmsg( "Tried to invoke a method %s on item %s, which doesn't have this method",
                  method.c_str(), used->tname().c_str() );
    }

    // Food can't be invoked here - it is already invoked as a part of consumption
    if( used->is_food() || used->is_food_container() ) {
        return consume_item( *used );
    }

    long charges_used = actually_used->type->invoke( this, actually_used, pt, method );
    return consume_charges( actually_used, charges_used );
}

void player::remove_gunmod(item *weapon, unsigned id)
{
    if (id >= weapon->contents.size()) {
        return;
    }
    item *gunmod = &weapon->contents[id];
    item ammo;
    if (gunmod->charges > 0) {
        if( gunmod->has_curammo() ) {
            ammo = item( gunmod->get_curammo_id(), calendar::turn );
        } else {
            ammo = item(default_ammo(weapon->ammo_type()), calendar::turn);
        }
        ammo.charges = gunmod->charges;
        if (ammo.made_of(LIQUID)) {
            while(!g->handle_liquid(ammo, false, false)) {
                // handled only part of it, retry
            }
        } else {
            i_add_or_drop(ammo);
        }
        gunmod->unset_curammo();
        gunmod->charges = 0;
    }
    if( gunmod->is_in_auxiliary_mode() ) {
        weapon->next_mode();
    }
    i_add_or_drop(*gunmod);
    weapon->contents.erase(weapon->contents.begin()+id);
    // gunmod removal decreased the gun's clip_size, move ammo to inventory
    if ( weapon->clip_size() < weapon->charges ) {
        ammo = item( weapon->get_curammo_id(), calendar::turn );
        ammo.charges = weapon->charges - weapon->clip_size();
        weapon->charges = weapon->clip_size();
        i_add_or_drop(ammo);
    }
}

hint_rating player::rate_action_read( const item &it ) const
{
    if (!it.is_book()) {
        return HINT_CANT;
    }

    //Check for NPCs to read for you, negates Illiterate and Far Sighted
    //The NPC gets a slight boost to int requirement since they wouldn't need to
    //understand what they are reading necessarily
    int assistants = 0;
    for( auto &elem : g->active_npc ) {
        if (rl_dist( elem->pos(), g->u.pos() ) < PICKUP_RANGE && elem->is_friend()) {
            ///\EFFECT_INT_NPC allows NPCs to read harder books for you
            if ((elem->int_cur+1) >= it.type->book->intel) {
                assistants++;
            }
       }
    }

    if (g && g->m.ambient_light_at(pos()) < 8 && LL_LIT > g->m.light_at(pos())) {
        return HINT_IFFY;
    } else if (morale_level() < MIN_MORALE_READ && it.type->book->fun <= 0) {
        return HINT_IFFY; //won't read non-fun books when sad
    } else if (it.type->book->intel > 0 && has_trait("ILLITERATE") && assistants == 0) {
        return HINT_IFFY;
    } else if (has_trait("HYPEROPIC") && !is_wearing("glasses_reading") &&
               !is_wearing("glasses_bifocal") && !has_effect("contacts") && assistants == 0) {
        return HINT_IFFY;
    }

    return HINT_GOOD;
}

bool player::read(int inventory_position)
{
    // Find the object
    item* it = &i_at(inventory_position);

    if (it == NULL || it->is_null()) {
        add_msg(m_info, _("You do not have that item."));
        return false;
    }

    if (!it->is_book()) {
        add_msg(m_info, _("Your %s is not good reading material."),
        it->tname().c_str());
        return false;
    }

    //Check for NPCs to read for you, negates Illiterate and Far Sighted
    int assistants = 0;
    for( auto &elem : g->active_npc ) {
        if (rl_dist( elem->pos(), g->u.pos() ) < PICKUP_RANGE && elem->is_friend()){
            ///\EFFECT_INT_NPC allows NPCs to read harder books for you
            if ((elem->int_cur+1) >= it->type->book->intel)
                assistants++;
        }
    }

    vehicle *veh = g->m.veh_at( pos() );
    if( veh != nullptr && veh->player_in_control(*this) ) {
        add_msg(m_info, _("It's a bad idea to read while driving!"));
        return false;
    }

    // Check if reading is okay
    // check for light level
    if (fine_detail_vision_mod() > 4) {
        add_msg(m_info, _("You can't see to read!"));
        return false;
    }

    // check for traits
    if (has_trait("HYPEROPIC") && !is_wearing("glasses_reading") &&
        !is_wearing("glasses_bifocal") && !has_effect("contacts")) {
        add_msg(m_info, _("Your eyes won't focus without reading glasses."));
        if (assistants != 0){
            add_msg(m_info, _("A fellow survivor reads aloud to you..."));
        } else {
            return false;
        }
    }

    auto tmp = it->type->book.get();
    int time; //Declare this here so that we can change the time depending on whats needed
    // activity.get_value(0) == 1: see below at player_activity(ACT_READ)
    const bool continuous = (activity.get_value(0) == 1);
    bool study = continuous;
    if (tmp->intel > 0 && has_trait("ILLITERATE")) {
        add_msg(m_info, _("You're illiterate!"));
        if (assistants != 0){
            add_msg(m_info, _("A fellow survivor reads aloud to you..."));
        } else {
            return false;
        }
    }

    // Now we've established that the player CAN read.

    // If the player hasn't read this book before, skim it to get an idea of what's in it.
    if( !has_identified( it->type->id ) ) {
        // Base read_speed() is 1000 move points (1 minute per tmp->time)
        time = tmp->time * read_speed() * (fine_detail_vision_mod());
        if (tmp->intel > int_cur) {
            // Lower int characters can read, at a speed penalty
            ///\EFFECT_INT increases reading speed of difficult books
            time += (tmp->time * (tmp->intel - int_cur) * 100);
        }
        // We're just skimming, so it's 10x faster.
        time /= 10;

        assign_activity( ACT_READ, time - moves, -1, inventory_position );
        // Never trigger studying when skimming the book.
        activity.values.push_back(0);
        moves = 0;
        return true;
    }


    const skill_id &skill = tmp->skill;
    const auto &skill_level = get_skill_level( skill );
    if( !skill ) {
        // special guidebook effect: print a misc. hint when read
        if (it->typeId() == "guidebook") {
            add_msg(m_info, get_hint().c_str());
            moves -= 100;
            return false;
        }
        // otherwise do nothing as there's no associated skill
    } else if (morale_level() < MIN_MORALE_READ && tmp->fun <= 0) { // See morale.h
        add_msg(m_info, _("What's the point of studying?  (Your morale is too low!)"));
        return false;
    } else if( skill_level < tmp->req ) {
        add_msg(_("The %s-related jargon flies over your head!"),
                   skill.obj().name().c_str());
        if (tmp->recipes.empty()) {
            return false;
        } else {
            add_msg(m_info, _("But you might be able to learn a recipe or two."));
        }
    } else if( skill_level >= tmp->level && !can_study_recipe(*it->type) &&
               !query_yn(tmp->fun > 0 ?
                         _("It would be fun, but your %s skill won't be improved.  Read anyway?") :
                         _("Your %s skill won't be improved.  Read anyway?"),
                         skill.obj().name().c_str())) {
        return false;
    } else if( !continuous &&
                 ( ( skill_level.can_train() && skill_level < tmp->level ) ||
                     can_study_recipe(*it->type) ) &&
                 !query_yn( skill_level.can_train() && skill_level < tmp->level ?
                 _("Study %s until you learn something? (gain a level)") :
                 _("Study the book until you learn all recipes?"),
                 skill.obj().name().c_str()) ) {
        study = false;
    } else {
        //If we just started studying, tell the player how to stop
        if(!continuous) {
            add_msg(m_info, _("Now studying %s, %s to stop early."),
                    it->tname().c_str(), press_x(ACTION_PAUSE).c_str());
            if ( (has_trait("ROOTS2") || (has_trait("ROOTS3"))) &&
                 g->m.has_flag("DIGGABLE", posx(), posy()) &&
                 (!(footwear_factor())) ) {
                add_msg(m_info, _("You sink your roots into the soil."));
            }
        }
        study = true;
    }

    if (!tmp->recipes.empty() && !continuous) {
        if( can_study_recipe( *it->type ) ) {
            add_msg(m_info, _("This book has more recipes for you to learn."));
        } else if( studied_all_recipes( *it->type ) ) {
            add_msg(m_info, _("You know all the recipes this book has to offer."));
        } else {
            add_msg(m_info, _("This book has more recipes, but you don't have the skill to learn them yet."));
        }
    }

    // Base read_speed() is 1000 move points (1 minute per tmp->time)
    time = tmp->time * read_speed() * (fine_detail_vision_mod());
    if (fine_detail_vision_mod() > 1.0) {
        add_msg(m_warning, _("It's difficult to see fine details right now. Reading will take longer than usual."));
    }

    if (tmp->intel > get_int() && !continuous) {
        add_msg(m_warning, _("This book is too complex for you to easily understand. It will take longer to read."));
        // Lower int characters can read, at a speed penalty
        ///\EFFECT_INT increases reading speed of difficult books
        time += (tmp->time * (tmp->intel - get_int()) * 100);
    }

    assign_activity( ACT_READ, time, -1, inventory_position );
    // activity.get_value(0) == 1 means continuous studing until
    // the player gained the next skill level, this ensured by this:
    activity.values.push_back(study ? 1 : 0);
    moves = 0;

    // Reinforce any existing morale bonus/penalty, so it doesn't decay
    // away while you read more.
    int minutes = time / 1000;
    // If you don't have a problem with eating humans, To Serve Man becomes rewarding
    if ((has_trait("CANNIBAL") || has_trait("PSYCHOPATH") || has_trait("SAPIOVORE")) &&
        it->typeId() == "cookbook_human") {
        add_morale(MORALE_BOOK, 0, 75, minutes + 30, minutes, false, it->type);
    } else if ( has_trait("SPIRITUAL") && it->has_flag("INSPIRATIONAL") ) {
        add_morale(MORALE_BOOK, 15, 90, minutes + 60, minutes, false, it->type);
    } else {
        add_morale(MORALE_BOOK, 0, tmp->fun * 15, minutes + 30, minutes, false, it->type);
    }

    return true;
}

void player::do_read( item *book )
{
    auto reading = book->type->book.get();
    const skill_id &skill = reading->skill;

    if( !has_identified( book->type->id ) ) {
        // Note that we've read the book.
        items_identified.insert( book->type->id );

        add_msg(_("You skim %s to find out what's in it."), book->type_name().c_str());
        if( skill && get_skill_level( skill ).can_train() ) {
            add_msg(m_info, _("Can bring your %s skill to %d."),
                    skill.obj().name().c_str(), reading->level);
            if( reading->req != 0 ){
                add_msg(m_info, _("Requires %s level %d to understand."),
                        skill.obj().name().c_str(), reading->req);
            }
        }

        add_msg(m_info, _("Requires intelligence of %d to easily read."), reading->intel);
        if (reading->fun != 0) {
            add_msg(m_info, _("Reading this book affects your morale by %d"), reading->fun);
        }
        add_msg(m_info, ngettext("A chapter of this book takes %d minute to read.",
                         "A chapter of this book takes %d minutes to read.", reading->time),
                reading->time );

        std::vector<std::string> recipe_list;
        for( auto const & elem : reading->recipes ) {
            // If the player knows it, they recognize it even if it's not clearly stated.
            if( elem.is_hidden() && !knows_recipe( elem.recipe ) ) {
                continue;
            }
            recipe_list.push_back( elem.name );
        }
        if( !recipe_list.empty() ) {
            std::string recipes = "";
            size_t index = 1;
            for( auto iter = recipe_list.begin();
                 iter != recipe_list.end(); ++iter, ++index ) {
                recipes += *iter;
                if(index == recipe_list.size() - 1) {
                    recipes += _(" and "); // Who gives a fuck about an oxford comma?
                } else if(index != recipe_list.size()) {
                    recipes += _(", ");
                }
            }
            std::string recipe_line = string_format(
                ngettext("This book contains %1$d crafting recipe: %2$s",
                         "This book contains %1$d crafting recipes: %2$s", recipe_list.size()),
                recipe_list.size(), recipes.c_str());
            add_msg(m_info, "%s", recipe_line.c_str());
        }
        if( recipe_list.size() != reading->recipes.size() ) {
            add_msg( m_info, _( "It might help you figuring out some more recipes." ) );
        }
        activity.type = ACT_NULL;
        return;
    }

    if( reading->fun != 0 ) {
        int fun_bonus = 0;
        const int chapters = book->get_chapters();
        const int remain = book->get_remaining_chapters( *this );
        if( chapters > 0 && remain == 0 ) {
            //Book is out of chapters -> re-reading old book, less fun
            add_msg(_("The %s isn't as much fun now that you've finished it."), book->tname().c_str());
            if( one_in(6) ) { //Don't nag incessantly, just once in a while
                add_msg(m_info, _("Maybe you should find something new to read..."));
            }
            //50% penalty
            fun_bonus = (reading->fun * 5) / 2;
        } else {
            fun_bonus = reading->fun * 5;
        }
        // If you don't have a problem with eating humans, To Serve Man becomes rewarding
        if( (has_trait("CANNIBAL") || has_trait("PSYCHOPATH") || has_trait("SAPIOVORE")) &&
            book->typeId() == "cookbook_human" ) {
            fun_bonus = 25;
            add_morale(MORALE_BOOK, fun_bonus, fun_bonus * 3, 60, 30, true, book->type);
        } else if ( has_trait("SPIRITUAL") && book->has_flag("INSPIRATIONAL") ) {
            fun_bonus = 15;
            add_morale(MORALE_BOOK, fun_bonus, fun_bonus * 5, 90, 90, true, book->type);
        } else {
            add_morale(MORALE_BOOK, fun_bonus, reading->fun * 15, 60, 30, true, book->type);
        }
    }

    book->mark_chapter_as_read( *this );

    bool no_recipes = true;
    if( !reading->recipes.empty() ) {
        bool recipe_learned = try_study_recipe( *book->type );
        if( !studied_all_recipes( *book->type ) ) {
            no_recipes = false;
        }

        // for books that the player cannot yet read due to skill level or have no skill component,
        // but contain lower level recipes, break out once recipe has been studied
        if( !skill || (get_skill_level(skill) < reading->req) ) {
            if( recipe_learned ) {
                add_msg(m_info, _("The rest of the book is currently still beyond your understanding."));
            }

            activity.type = ACT_NULL;
            return;
        }
    }

    if( skill && get_skill_level( skill ) < reading->level &&
        get_skill_level( skill ).can_train() ) {
        auto &skill_level = skillLevel( skill );
        int originalSkillLevel = skill_level;
        ///\EFFECT_INT increases reading speed
        int min_ex = reading->time / 10 + int_cur / 4;
        int max_ex = reading->time /  5 + int_cur / 2 - originalSkillLevel;
        if (min_ex < 1) {
            min_ex = 1;
        }
        if (max_ex < 2) {
            max_ex = 2;
        }
        if (max_ex > 10) {
            max_ex = 10;
        }
        if (max_ex < min_ex) {
            max_ex = min_ex;
        }

        min_ex *= originalSkillLevel + 1;
        max_ex *= originalSkillLevel + 1;

        skill_level.readBook( min_ex, max_ex, reading->level );

        add_msg(_("You learn a little about %s! (%d%%)"), skill.obj().name().c_str(),
                skill_level.exercise());

        if( skill_level == originalSkillLevel && activity.get_value(0) == 1 ) {
            // continuously read until player gains a new skill level
            activity.type = ACT_NULL;
            read(activity.position);
            // Rooters root (based on time spent reading)
            int root_factor = (reading->time / 20);
            double foot_factor = footwear_factor();
            if( (has_trait("ROOTS2") || has_trait("ROOTS3")) &&
                g->m.has_flag("DIGGABLE", pos()) &&
                !foot_factor ) {
                if (get_hunger() > -20) {
                    mod_hunger(-root_factor * foot_factor);
                }
                if (thirst > -20) {
                    thirst -= root_factor * foot_factor;
                }
                mod_healthy_mod(root_factor * foot_factor, 50);
            }
            if (activity.type != ACT_NULL) {
                return;
            }
        }

        int new_skill_level = skill_level;
        if (new_skill_level > originalSkillLevel) {
            add_msg(m_good, _("You increase %s to level %d."),
                    skill.obj().name().c_str(),
                    new_skill_level);

            if(new_skill_level % 4 == 0) {
                //~ %s is skill name. %d is skill level
                add_memorial_log(pgettext("memorial_male", "Reached skill level %1$d in %2$s."),
                                   pgettext("memorial_female", "Reached skill level %1$d in %2$s."),
                                   new_skill_level, skill.obj().name().c_str());
            }
        }

        if( skill_level == reading->level || !skill_level.can_train() ) {
            if( no_recipes ) {
                add_msg(m_info, _("You can no longer learn from %s."), book->type_name().c_str());
            } else {
                add_msg(m_info, _("Your skill level won't improve, but %s has more recipes for you."),
                        book->type_name().c_str());
            }
        }
    } else if( can_study_recipe(*book->type) && activity.get_value(0) == 1 ) {
        // continuously read until player gains a new recipe
        activity.type = ACT_NULL;
        read(activity.position);
        // Rooters root (based on time spent reading)
        int root_factor = (reading->time / 20);
        double foot_factor = footwear_factor();
        if( (has_trait("ROOTS2") || has_trait("ROOTS3")) &&
            g->m.has_flag("DIGGABLE", pos()) &&
            !foot_factor ) {
            if (get_hunger() > -20) {
                mod_hunger(-root_factor * foot_factor);
            }
            if (thirst > -20) {
                thirst -= root_factor * foot_factor;
            }
            mod_healthy_mod(root_factor * foot_factor, 50);
        }
        if (activity.type != ACT_NULL) {
            return;
        }
    } else if ( !reading->recipes.empty() && no_recipes ) {
        add_msg(m_info, _("You can no longer learn from %s."), book->type_name().c_str());
    }

    for( auto &m : reading->use_methods ) {
        m.call( this, book, false, pos3() );
    }

    activity.type = ACT_NULL;
}

bool player::has_identified( std::string item_id ) const
{
    return items_identified.count( item_id ) > 0;
}

bool player::can_study_recipe(const itype &book) const
{
    if( !book.book ) {
        return false;
    }
    for( auto const &elem : book.book->recipes ) {
        auto const r = elem.recipe;
        if( !knows_recipe( r ) &&
            ( !r->skill_used ||
              get_skill_level( r->skill_used ) >= elem.skill_level ) ) {
            return true;
        }
    }
    return false;
}

bool player::studied_all_recipes(const itype &book) const
{
    if( !book.book ) {
        return true;
    }
    for( auto &elem : book.book->recipes ) {
        if( !knows_recipe( elem.recipe ) ) {
            return false;
        }
    }
    return true;
}

bool player::try_study_recipe( const itype &book )
{
    if( !book.book ) {
        return false;
    }
    for( auto const & elem : book.book->recipes ) {
        auto const r = elem.recipe;
        if( knows_recipe( r ) ) {
            continue;
        }
        if( !r->skill_used || get_skill_level( r->skill_used ) >= elem.skill_level ) {
            if( !r->skill_used ||
                rng(0, 4) <= (get_skill_level(r->skill_used) - elem.skill_level) / 2) {
                learn_recipe( r );
                add_msg(m_good, _("Learned a recipe for %1$s from the %2$s."),
                                item::nname( r->result ).c_str(), book.nname(1).c_str());
                return true;
            } else {
                add_msg(_("Failed to learn a recipe from the %s."), book.nname(1).c_str());
                return false;
            }
        }
    }
    return true; // _("false") seems to mean _("attempted and failed")
}

void player::try_to_sleep()
{
    int vpart = -1;
    vehicle *veh = g->m.veh_at( pos(), vpart );
    const trap &trap_at_pos = g->m.tr_at(pos());
    const ter_id ter_at_pos = g->m.ter(pos());
    const furn_id furn_at_pos = g->m.furn(pos());
    bool plantsleep = false;
    bool websleep = false;
    bool webforce = false;
    bool websleeping = false;
    bool in_shell = false;
    if (has_trait("CHLOROMORPH")) {
        plantsleep = true;
        if( (ter_at_pos == t_dirt || ter_at_pos == t_pit ||
              ter_at_pos == t_dirtmound || ter_at_pos == t_pit_shallow ||
              ter_at_pos == t_grass) && !veh &&
              furn_at_pos == f_null ) {
            add_msg(m_good, _("You relax as your roots embrace the soil."));
        } else if (veh) {
            add_msg(m_bad, _("It's impossible to sleep in this wheeled pot!"));
        } else if (furn_at_pos != f_null) {
            add_msg(m_bad, _("The humans' furniture blocks your roots. You can't get comfortable."));
        } else { // Floor problems
            add_msg(m_bad, _("Your roots scrabble ineffectively at the unyielding surface."));
        }
    }
    if (has_trait("WEB_WALKER")) {
        websleep = true;
    }
    // Not sure how one would get Arachnid w/o web-making, but Just In Case
    if (has_trait("THRESH_SPIDER") && (has_trait("WEB_SPINNER") || (has_trait("WEB_WEAVER"))) ) {
        webforce = true;
    }
    if (websleep || webforce) {
        int web = g->m.get_field_strength( pos(), fd_web );
            if (!webforce) {
            // At this point, it's kinda weird, but surprisingly comfy...
            if (web >= 3) {
                add_msg(m_good, _("These thick webs support your weight, and are strangely comfortable..."));
                websleeping = true;
            }
            else if (web > 0) {
                add_msg(m_info, _("You try to sleep, but the webs get in the way.  You brush them aside."));
                g->m.remove_field( pos(), fd_web );
            }
        } else {
            // Here, you're just not comfortable outside a nice thick web.
            if (web >= 3) {
                add_msg(m_good, _("You relax into your web."));
                websleeping = true;
            }
            else {
                add_msg(m_bad, _("You try to sleep, but you feel exposed and your spinnerets keep twitching."));
                add_msg(m_info, _("Maybe a nice thick web would help you sleep."));
            }
        }
    }
    if (has_active_mutation("SHELL2")) {
        // Your shell's interior is a comfortable place to sleep.
        in_shell = true;
    }
    if(!plantsleep && (furn_at_pos == f_bed || furn_at_pos == f_makeshift_bed ||
         trap_at_pos.loadid == tr_cot || trap_at_pos.loadid == tr_rollmat ||
         trap_at_pos.loadid == tr_fur_rollmat || furn_at_pos == f_armchair ||
         furn_at_pos == f_sofa || furn_at_pos == f_hay || furn_at_pos == f_straw_bed ||
         ter_at_pos == t_improvised_shelter || (in_shell) || (websleeping) ||
         (veh && veh->part_with_feature (vpart, "SEAT") >= 0) ||
         (veh && veh->part_with_feature (vpart, "BED") >= 0)) ) {
        add_msg(m_good, _("This is a comfortable place to sleep."));
    } else if (ter_at_pos != t_floor && !plantsleep) {
        add_msg( ter_at_pos.obj().movecost <= 2 ?
                 _("It's a little hard to get to sleep on this %s.") :
                 _("It's hard to get to sleep on this %s."),
                 ter_at_pos.obj().name.c_str() );
    }
    add_effect("lying_down", 300);
}

int player::sleep_spot( const tripoint &p ) const
{
    int sleepy = 0;
    bool plantsleep = false;
    bool websleep = false;
    bool webforce = false;
    bool in_shell = false;
    if (has_addiction(ADD_SLEEP)) {
        sleepy -= 4;
    }
    if (has_trait("INSOMNIA")) {
        // 12.5 points is the difference between "tired" and "dead tired"
        sleepy -= 12;
    }
    if (has_trait("EASYSLEEPER")) {
        // Low fatigue (being rested) has a much stronger effect than high fatigue
        // so it's OK for the value to be that much higher
        sleepy += 24;
    }
    if (has_trait("CHLOROMORPH")) {
        plantsleep = true;
    }
    if (has_trait("WEB_WALKER")) {
        websleep = true;
    }
    // Not sure how one would get Arachnid w/o web-making, but Just In Case
    if (has_trait("THRESH_SPIDER") && (has_trait("WEB_SPINNER") || (has_trait("WEB_WEAVER"))) ) {
        webforce = true;
    }
    if (has_active_mutation("SHELL2")) {
        // Your shell's interior is a comfortable place to sleep.
        in_shell = true;
    }
    int vpart = -1;
    vehicle *veh = g->m.veh_at(p, vpart);
    const maptile tile = g->m.maptile_at( p );
    const trap &trap_at_pos = tile.get_trap_t();
    const ter_id ter_at_pos = tile.get_ter();
    const furn_id furn_at_pos = tile.get_furn();
    int web = g->m.get_field_strength( p, fd_web );
    // Plant sleepers use a different method to determine how comfortable something is
    // Web-spinning Arachnids do too
    if (!plantsleep && !webforce) {
        // Shells are comfortable and get used anywhere
        if (in_shell) {
            sleepy += 4;
        // Else use the vehicle tile if we are in one
        } else if (veh) {
            if (veh->part_with_feature (vpart, "BED") >= 0) {
                sleepy += 4;
            } else if (veh->part_with_feature (vpart, "SEAT") >= 0) {
                sleepy += 3;
            } else {
                // Sleeping elsewhere is uncomfortable
                sleepy -= g->m.move_cost(p);
            }
        // Not in a vehicle, start checking furniture/terrain/traps at this point in decreasing order
        } else if (furn_at_pos == f_bed) {
            sleepy += 5;
        } else if (furn_at_pos == f_makeshift_bed || trap_at_pos.loadid == tr_cot ||
                    furn_at_pos == f_sofa) {
            sleepy += 4;
        } else if (websleep && web >= 3) {
            sleepy += 4;
        } else if (trap_at_pos.loadid == tr_rollmat || trap_at_pos.loadid == tr_fur_rollmat ||
              furn_at_pos == f_armchair || ter_at_pos == t_improvised_shelter) {
            sleepy += 3;
        } else if (furn_at_pos == f_straw_bed || furn_at_pos == f_hay || furn_at_pos == f_tatami) {
            sleepy += 2;
        } else if (ter_at_pos == t_floor || ter_at_pos == t_floor_waxed ||
                    ter_at_pos == t_carpet_red || ter_at_pos == t_carpet_yellow ||
                    ter_at_pos == t_carpet_green || ter_at_pos == t_carpet_purple) {
            sleepy += 1;
        } else {
            // Not a comfortable sleeping spot
            sleepy -= g->m.move_cost(p);
        }
    // Has plantsleep
    } else if (plantsleep) {
        if (veh || furn_at_pos != f_null) {
            // Sleep ain't happening in a vehicle or on furniture
            sleepy -= 999;
        } else {
            // It's very easy for Chloromorphs to get to sleep on soil!
            if (ter_at_pos == t_dirt || ter_at_pos == t_pit || ter_at_pos == t_dirtmound ||
                  ter_at_pos == t_pit_shallow) {
                sleepy += 10;
            // Not as much if you have to dig through stuff first
            } else if (ter_at_pos == t_grass) {
                sleepy += 5;
            // Sleep ain't happening
            } else {
                sleepy -= 999;
            }
        }
    // Has webforce
    } else {
        if (web >= 3) {
            // Thick Web and you're good to go
            sleepy += 10;
        }
        else {
            sleepy -= 999;
        }
    }
    if (fatigue < TIRED + 1) {
        sleepy -= int( (TIRED + 1 - fatigue) / 4);
    } else {
        sleepy += int((fatigue - TIRED + 1) / 16);
    }

    if( stim > 0 || !has_trait("INSOMNIA") ) {
        sleepy -= 2 * stim;
    } else {
        // Make it harder for insomniac to get around the trait
        sleepy -= stim;
    }

    return sleepy;
}

bool player::can_sleep()
{
    if( has_effect( "meth" ) ) {
        // Sleep ain't happening until that meth wears off completely.
        return false;
    }
    int sleepy = sleep_spot( pos3() );
    sleepy += rng( -8, 8 );
    if( sleepy > 0 ) {
        return true;
    }
    return false;
}

void player::fall_asleep(int duration)
{
    add_effect("sleep", duration);
}

void player::wake_up()
{
    if (has_effect("sleep")) {
        if(calendar::turn - get_effect("sleep").get_start_turn() > HOURS(2) ) {
            print_health();
        }
    }

    remove_effect("sleep");
    remove_effect("lying_down");
}

std::string player::is_snuggling() const
{
    auto begin = g->m.i_at( pos() ).begin();
    auto end = g->m.i_at( pos() ).end();

    if( in_vehicle ) {
        int vpart;
        vehicle *veh = g->m.veh_at( pos(), vpart );
        if( veh != nullptr ) {
            int cargo = veh->part_with_feature( vpart, VPFLAG_CARGO, false );
            if( cargo >= 0 ) {
                if( !veh->get_items(cargo).empty() ) {
                    begin = veh->get_items(cargo).begin();
                    end = veh->get_items(cargo).end();
                }
            }
        }
    }
    const item* floor_armor = NULL;
    int ticker = 0;

    // If there are no items on the floor, return nothing
    if( begin == end ) {
        return "nothing";
    }

    for( auto candidate = begin; candidate != end; ++candidate ) {
        if( !candidate->is_armor() ) {
            continue;
        } else if( candidate->volume() > 1 &&
                   ( candidate->covers( bp_torso ) || candidate->covers( bp_leg_l ) ||
                     candidate->covers( bp_leg_r ) ) ) {
            floor_armor = &*candidate;
            ticker++;
        }
    }

    if ( ticker == 0 ) {
        return "nothing";
    }
    else if ( ticker == 1 ) {
        return floor_armor->type_name();
    }
    else if ( ticker > 1 ) {
        return "many";
    }

    return "nothing";
}

// Returned values range from 1.0 (unimpeded vision) to 5.0 (totally blind).
// LIGHT_AMBIENT DIM is enough light for detail work, but held items get a boost.
float player::fine_detail_vision_mod()
{
    // PER_SLIME_OK implies you can get enough eyes around the bile
    // that you can generaly see.  There'll still be the haze, but
    // it's annoying rather than limiting.
    if( has_effect("blind") || worn_with_flag("BLIND") || has_active_bionic("bio_blindfold") ||
        (( has_effect("boomered") || has_effect("darkness") ) && !has_trait("PER_SLIME_OK")) ) {
        return 5.0;
    }
    // Scale linearly as light level approaches LIGHT_AMBIENT_LIT.
    // If we're actually a source of light, assume we can direct it where we need it.
    // Therefore give a hefty bonus relative to ambient light.
    float own_light = std::max( 1.0, LIGHT_AMBIENT_LIT - active_light() - 2 );

    // Same calculation as above, but with a result 3 lower.
    float ambient_light = std::max( 1.0, LIGHT_AMBIENT_LIT - g->m.ambient_light_at( pos() ) + 1.0 );

    return std::min( own_light, ambient_light );
}

int player::get_wind_resistance(body_part bp) const
{
    int coverage = 0;
    float totalExposed = 1.0;
    int totalCoverage = 0;
    int penalty = 100;

    for( auto &i : worn ) {
        if( i.covers(bp) ) {
            if( i.made_of("leather") || i.made_of("plastic") || i.made_of("bone") ||
                i.made_of("chitin") || i.made_of("nomex") ) {
                penalty = 10; // 90% effective
            } else if( i.made_of("cotton") ) {
                penalty = 30;
            } else if( i.made_of("wool") ) {
                penalty = 40;
            } else {
                penalty = 1; // 99% effective
            }

            coverage = std::max(0, i.get_coverage() - penalty);
            totalExposed *= (1.0 - coverage/100.0); // Coverage is between 0 and 1?
        }
    }

    // Your shell provides complete wind protection if you're inside it
    if (has_active_mutation("SHELL2")) {
        totalCoverage = 100;
        return totalCoverage;
    }

    totalCoverage = 100 - totalExposed*100;

    return totalCoverage;
}

int player::warmth(body_part bp) const
{
    int ret = 0, warmth = 0;

    for (auto &i : worn) {
        if( i.covers( bp ) ) {
            warmth = i.get_warmth();
            // Wool items do not lose their warmth due to being wet.
            // Warmth is reduced by 0 - 66% based on wetness.
            if (!i.made_of("wool"))
            {
                warmth *= 1.0 - 0.66 * body_wetness[bp] / drench_capacity[bp];
            }
            ret += warmth;
        }
    }
    return ret;
}

int bestwarmth( const std::list< item > &its, const std::string &flag )
{
    int best = 0;
    for( auto &w : its ) {
        if( w.has_flag( flag ) && w.get_warmth() > best ) {
            best = w.get_warmth();
        }
    }
    return best;
}

int player::bonus_warmth(body_part bp) const
{
    int ret = 0;

    // If the player is not wielding anything big, check if hands can be put in pockets
    if( ( bp == bp_hand_l || bp == bp_hand_r ) && weapon.volume() < 2 ) {
        ret += bestwarmth( worn, "POCKETS" );
    }

    // If the player's head is not encumbered, check if hood can be put up
    if( bp == bp_head && encumb( bp_head ) < 10 ) {
        ret += bestwarmth( worn, "HOOD" );
    }

    // If the player's mouth is not encumbered, check if collar can be put up
    if( bp == bp_mouth && encumb( bp_mouth ) < 10 ) {
        ret += bestwarmth( worn, "COLLAR" );
    }

    return ret;
}

template <typename arr>
void layer_item( body_part bp, arr &layer, int &armorenc, const item &it, bool power_armor )
{
    if( !it.covers( bp ) ) {
        return;
    }

    std::pair<int, int> &this_layer = layer[it.get_layer()];
    int encumber_val = it.get_encumber();
    // For the purposes of layering penalty, set a min of 2 and a max of 10 per item.
    int layering_encumbrance = std::min( 10, std::max( 2, encumber_val ) );

    this_layer.first += layering_encumbrance;
    this_layer.second = std::max(this_layer.second, layering_encumbrance);

    if( it.is_power_armor() && power_armor ) {
        armorenc += std::max( 0, encumber_val - 40 );
    } else {
        armorenc += encumber_val;
    }
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
int player::item_encumb( body_part bp, double &layers, int &armorenc, const item &new_item ) const
{
    int ret = 0;
    // First is the total of encumbrance on the given layer,
    // and second is the highest encumbrance of any one item on the layer.
    std::array<std::pair<int, int>, MAX_CLOTHING_LAYER> layer = {{{0,0},{0,0},{0,0},{0,0},{0,0}}};

    const bool power_armored = is_wearing_active_power_armor();
    for( auto& w : worn ) {
        layer_item( bp, layer, armorenc, w, power_armored );
    }

    if( !new_item.is_null() ) {
        layer_item( bp, layer, armorenc, new_item, power_armored );
    }

    armorenc = std::max(0, armorenc);
    ret += armorenc;

    // The stacking penalty applies by doubling the encumbrance of
    // each item except the highest encumbrance one.
    // So we add them together and then subtract out the highest.
    for( const auto &elem : layer ) {
        layers += std::max(0, elem.first - elem.second);
    }

    ret += layers;
    return std::max( 0, ret );
}

int player::encumb( body_part bp ) const
{
    int iArmorEnc = 0;
    double iLayers = 0;
    return encumb( bp, iLayers, iArmorEnc, ret_null );
}

int player::encumb( body_part bp, const item &new_item ) const {
    int iArmorEnc = 0;
    double iLayers = 0;
    return encumb( bp, iLayers, iArmorEnc, new_item );
}

int player::encumb( body_part bp, double &layers, int &armorenc ) const
{
    return encumb( bp, layers, armorenc, ret_null );
}

int player::encumb( body_part bp, double &layers, int &armorenc, const item &new_item ) const
{
    int ret = 0;
    // Armor
    ret += item_encumb( bp, layers, armorenc, new_item );

    // Bionics and mutation
    ret += mut_cbm_encumb( bp );
    return std::max( 0, ret );
}

int player::mut_cbm_encumb( body_part bp ) const
{
    int ret = 0;
    if( bp != bp_head && bp != bp_mouth && bp != bp_eyes && has_bionic("bio_stiff") ) {
        ret += 10;
    }
    if( bp != bp_eyes && bp != bp_mouth && (has_trait("CHITIN3") || has_trait("CHITIN_FUR3") ) ) {
        ret += 10;
    }
    if( bp == bp_mouth && has_trait("SLIT_NOSTRILS") ) {
        ret += 10;
    }
    if( (bp == bp_arm_l || bp == bp_arm_r) && has_trait("ARM_FEATHERS") ) {
        ret += 20;
    }
    if( (bp == bp_arm_l || bp == bp_arm_r) && has_trait("INSECT_ARMS") ) {
        ret += 30;
    }
    if( (bp == bp_arm_l || bp == bp_arm_r) && has_trait("ARACHNID_ARMS")) {
        ret += 40;
    }
    if( (bp == bp_hand_l || bp == bp_hand_r) && has_trait("PAWS") ) {
        ret += 10;
    }
    if( (bp == bp_hand_l || bp == bp_hand_r) && has_trait("PAWS_LARGE") ) {
        ret += 20;
    }
    if( (bp == bp_arm_l || bp == bp_arm_r || bp == bp_torso ) && has_trait("LARGE") ) {
        ret += 10;
    }
    if( bp == bp_torso && has_trait("WINGS_BUTTERFLY") ) {
        ret += 10;
    }
    if( bp == bp_torso && has_trait("SHELL2") ) {
        ret += 10;
    }
    if( (bp == bp_hand_l || bp == bp_hand_r) &&
        (has_trait("ARM_TENTACLES") || has_trait("ARM_TENTACLES_4") ||
         has_trait("ARM_TENTACLES_8")) ) {
        ret += 30;
    }
    if( (bp == bp_hand_l || bp == bp_hand_r) &&
        (has_trait("CLAWS_TENTACLE") )) {
        ret += 20;
    }
    if( bp == bp_mouth &&
        ( has_bionic("bio_nostril") ) ) {
        ret += 10;
    }
    if( (bp == bp_hand_l || bp == bp_hand_r) &&
        ( has_bionic("bio_thumbs") ) ) {
        ret += 20;
    }
    if( bp == bp_eyes &&
        ( has_bionic("bio_pokedeye") ) ) {
        ret += 10;
    }

    return ret;
}

int player::get_armor_bash(body_part bp) const
{
    return get_armor_bash_base(bp) + armor_bash_bonus;
}

int player::get_armor_cut(body_part bp) const
{
    return get_armor_cut_base(bp) + armor_cut_bonus;
}

int player::get_armor_bash_base(body_part bp) const
{
    int ret = 0;
    for (auto &i : worn) {
        if (i.covers(bp)) {
            ret += i.bash_resist();
        }
    }
    if (has_bionic("bio_carbon")) {
        ret += 2;
    }
    if (bp == bp_head && has_bionic("bio_armor_head")) {
        ret += 3;
    }
    if ((bp == bp_arm_l || bp == bp_arm_r) && has_bionic("bio_armor_arms")) {
        ret += 3;
    }
    if (bp == bp_torso && has_bionic("bio_armor_torso")) {
        ret += 3;
    }
    if ((bp == bp_leg_l || bp == bp_leg_r) && has_bionic("bio_armor_legs")) {
        ret += 3;
    }
    if (bp == bp_eyes && has_bionic("bio_armor_eyes")) {
        ret += 3;
    }
    if (has_trait("FUR") || has_trait("LUPINE_FUR") || has_trait("URSINE_FUR")) {
        ret++;
    }
    if (bp == bp_head && has_trait("LYNX_FUR")) {
        ret++;
    }
    if (has_trait("FAT")) {
        ret ++;
    }
    if (has_trait("M_SKIN")) {
        ret += 2;
    }
    if (has_trait("M_SKIN2")) {
        ret += 3;
    }
    if (has_trait("CHITIN")) {
        ret += 2;
    }
    if (has_trait("SHELL") && bp == bp_torso) {
        ret += 6;
    }
    if (has_trait("SHELL2") && !has_active_mutation("SHELL2") && bp == bp_torso) {
        ret += 9;
    }
    if (has_active_mutation("SHELL2")) {
        // Limbs & head are safe inside the shell! :D
        ret += 9;
    }
    return ret;
}

int player::get_armor_cut_base(body_part bp) const
{
    int ret = 0;
    for (auto &i : worn) {
        if (i.covers(bp)) {
            ret += i.cut_resist();
        }
    }
    if (has_bionic("bio_carbon")) {
        ret += 4;
    }
    if (bp == bp_head && has_bionic("bio_armor_head")) {
        ret += 3;
    } else if ((bp == bp_arm_l || bp == bp_arm_r) && has_bionic("bio_armor_arms")) {
        ret += 3;
    } else if (bp == bp_torso && has_bionic("bio_armor_torso")) {
        ret += 3;
    } else if ((bp == bp_leg_l || bp == bp_leg_r) && has_bionic("bio_armor_legs")) {
        ret += 3;
    } else if (bp == bp_eyes && has_bionic("bio_armor_eyes")) {
        ret += 3;
    }
    if (has_trait("THICKSKIN")) {
        ret++;
    }
    if (has_trait("THINSKIN")) {
        ret--;
    }
    if (has_trait("M_SKIN")) {
        ret ++;
    }
    if (has_trait("M_SKIN2")) {
        ret += 3;
    }
    if (has_trait("SCALES")) {
        ret += 2;
    }
    if (has_trait("THICK_SCALES")) {
        ret += 4;
    }
    if (has_trait("SLEEK_SCALES")) {
        ret += 1;
    }
    if (has_trait("CHITIN") || has_trait("CHITIN_FUR")) {
        ret += 2;
    }
    if (has_trait("CHITIN2") || has_trait("CHITIN_FUR2")) {
        ret += 4;
    }
    if (has_trait("CHITIN3") || has_trait("CHITIN_FUR3")) {
        ret += 8;
    }
    if (has_trait("SHELL") && bp == bp_torso) {
        ret += 14;
    }
    if (has_trait("SHELL2") && !has_active_mutation("SHELL2") && bp == bp_torso) {
        ret += 17;
    }
    if (has_active_mutation("SHELL2")) {
        // Limbs & head are safe inside the shell! :D
        ret += 17;
    }
    return ret;
}

bool player::armor_absorb(damage_unit& du, item& armor) {
    if( rng( 1, 100 ) > armor.get_coverage() ) {
        return false;
    }

    // TODO: add some check for power armor

    const auto res = resistances(armor);
    const float effective_resist = res.get_effective_resist(du);
    // Amount of damage mitigated
    const float mitigation = std::min(effective_resist, du.amount);
    du.amount -= mitigation; // mitigate the damage first

    if( mitigation <= 0 ) {
        // If it doesn't protect from it at all, it's some weird type,
        // like electricity (which should not damage clothing)
        // or fire (for which resistance isn't implemented yet so it would slice power armor)
        return false;
    }

    // Scale chance of article taking damage based on the number of parts it covers.
    // This represents large articles being able to take more punishment
    // before becoming inneffective or being destroyed.
    const int num_parts_covered = armor.get_covered_body_parts().count();
    if( !one_in( num_parts_covered ) ) {
        return false;
    }

    // Don't damage armor as much when bypassed by armor piercing
    // Most armor piercing damage comes from bypassing armor, not forcing through
    const int raw_dmg = du.amount;
    const int raw_armor = res.type_resist( du.type );
    if( (raw_dmg > raw_armor && !one_in(du.amount) && one_in(2)) ||
        // or if it isn't, but 1/50 chance
        (raw_dmg <= raw_armor && !armor.has_flag("STURDY") &&
         !armor.is_power_armor() && one_in(200)) ) {

        auto &material = armor.get_random_material();
        std::string damage_verb = ( du.type == DT_BASH ) ?
            material.bash_dmg_verb() : material.cut_dmg_verb();

        const std::string pre_damage_name = armor.tname();
        const std::string pre_damage_adj = armor.get_base_material().
            dmg_adj(armor.damage);

        // add "further" if the damage adjective and verb are the same
        std::string format_string = ( pre_damage_adj == damage_verb ) ?
            _("Your %1$s is %2$s further!") : _("Your %1$s is %2$s!");
        add_msg_if_player( m_bad, format_string.c_str(), pre_damage_name.c_str(),
                           damage_verb.c_str());
        //item is damaged
        if( is_player() ) {
            SCT.add(posx(), posy(), NORTH, remove_color_tags( pre_damage_name ),
                    m_neutral, damage_verb, m_info);
        }

        if (armor.has_flag("FRAGILE")) {
            armor.damage += rng(2,3);
        } else {
            armor.damage++;
        }

        if( armor.damage >= 5 ) {
            //~ %s is armor name
            add_memorial_log( pgettext("memorial_male", "Worn %s was completely destroyed."),
                              pgettext("memorial_female", "Worn %s was completely destroyed."),
                              pre_damage_name.c_str() );
            add_msg_player_or_npc( m_bad, _("Your %s is completely destroyed!"),
                                   _("<npcname>'s %s is completely destroyed!"),
                                   pre_damage_name.c_str() );
            return true;
        }
    }
    return false;
}

void player::absorb_hit(body_part bp, damage_instance &dam) {
    std::list<item> worn_remains;
    bool armor_destroyed = false;

    for( auto &elem : dam.damage_units ) {
        if( elem.amount < 0 ) {
            // Prevents 0 damage hits (like from hallucinations) from ripping armor
            elem.amount = 0;
            continue;
        }

        // CBMs absorb damage first before hitting armor
        if( has_active_bionic("bio_ads") ) {
            if( elem.amount > 0 && power_level > 24 ) {
                if( elem.type == DT_BASH ) {
                    elem.amount -= rng( 1, 8 );
                } else if( elem.type == DT_CUT ) {
                    elem.amount -= rng( 1, 4 );
                } else if( elem.type == DT_STAB ) {
                    elem.amount -= rng( 1, 2 );
                }
                charge_power(-25);
            }
            if( elem.amount < 0 ) {
                elem.amount = 0;
            }
        }

        // The worn vector has the innermost item first, so
        // iterate reverse to damage the outermost (last in worn vector) first.
        for( auto iter = worn.rbegin(); iter != worn.rend(); ) {
            item& armor = *iter;

            if( !armor.covers( bp ) ) {
                ++iter;
                continue;
            }

            if( armor_absorb( elem, armor ) ) {
                armor_destroyed = true;
                worn_remains.insert( worn_remains.end(), armor.contents.begin(), armor.contents.end() );
                // decltype is the typename of the iterator, ote that reverse_iterator::base returns the
                // iterator to the next element, not the one the revers_iterator points to.
                // http://stackoverflow.com/questions/1830158/how-to-call-erase-with-a-reverse-iterator
                iter = decltype(iter)( worn.erase( --iter.base() ) );
            } else {
                ++iter;
            }
        }

        // Next, apply reductions from bionics and traits.
        if( has_bionic("bio_carbon") ) {
            switch (elem.type) {
            case DT_BASH:
                elem.amount -= 2;
                break;
            case DT_CUT:
                elem.amount -= 4;
                break;
            case DT_STAB:
                elem.amount -= 3.2;
                break;
            default:
                break;
            }
        }
        if( bp == bp_head && has_bionic("bio_armor_head") ) {
            switch (elem.type) {
            case DT_BASH:
            case DT_CUT:
                elem.amount -= 3;
                break;
            case DT_STAB:
                elem.amount -= 2.4;
                break;
            default:
                break;
            }
        } else if( (bp == bp_arm_l || bp == bp_arm_r) && has_bionic("bio_armor_arms") ) {
            switch (elem.type) {
            case DT_BASH:
            case DT_CUT:
                elem.amount -= 3;
                break;
            case DT_STAB:
                elem.amount -= 2.4;
                break;
            default:
                break;
            }
        } else if( bp == bp_torso && has_bionic("bio_armor_torso") ) {
            switch (elem.type) {
            case DT_BASH:
            case DT_CUT:
                elem.amount -= 3;
                break;
            case DT_STAB:
                elem.amount -= 2.4;
                break;
            default:
                break;
            }
        } else if( (bp == bp_leg_l || bp == bp_leg_r) && has_bionic("bio_armor_legs") ) {
            switch (elem.type) {
            case DT_BASH:
            case DT_CUT:
                elem.amount -= 3;
                break;
            case DT_STAB:
                elem.amount -= 2.4;
                break;
            default:
                break;
            }
        } else if( bp == bp_eyes && has_bionic("bio_armor_eyes") ) {
            switch (elem.type) {
            case DT_BASH:
            case DT_CUT:
                elem.amount -= 3;
                break;
            case DT_STAB:
                elem.amount -= 2.4;
                break;
            default:
                break;
            }
        }
        if( elem.type == DT_CUT ) {
            if( has_trait("THICKSKIN") ) {
                elem.amount -= 1;
            }
            if( elem.amount > 0 && has_trait("THINSKIN") ) {
                elem.amount += 1;
            }
            if (has_trait("SCALES")) {
                elem.amount -= 2;
            }
            if (has_trait("THICK_SCALES")) {
                elem.amount -= 4;
            }
            if (has_trait("SLEEK_SCALES")) {
                elem.amount -= 1;
            }
            if (has_trait("FAT")) {
                elem.amount --;
            }
            if (has_trait("CHITIN") || has_trait("CHITIN_FUR") || has_trait("CHITIN_FUR2")) {
                elem.amount -= 2;
            }
            if ((bp == bp_foot_l || bp == bp_foot_r) && has_trait("HOOVES")) {
                elem.amount--;
            }
            if (has_trait("CHITIN2")) {
                elem.amount -= 4;
            }
            if (has_trait("CHITIN3") || has_trait("CHITIN_FUR3")) {
                elem.amount -= 8;
            }
            elem.amount -= mabuff_arm_cut_bonus();
        }
        if( elem.type == DT_BASH ) {
            if (has_trait("FEATHERS")) {
                elem.amount--;
            }
            if (has_trait("AMORPHOUS")) {
                elem.amount--;
                if (!(has_trait("INT_SLIME"))) {
                    elem.amount -= 3;
                }
            }
            if ((bp == bp_arm_l || bp == bp_arm_r) && has_trait("ARM_FEATHERS")) {
                elem.amount--;
            }
            if (has_trait("FUR") || has_trait("LUPINE_FUR") || has_trait("URSINE_FUR")) {
                elem.amount--;
            }
            if (bp == bp_head && has_trait("LYNX_FUR")) {
                elem.amount--;
            }
            if (has_trait("CHITIN2")) {
                elem.amount--;
            }
            if (has_trait("CHITIN3") || has_trait("CHITIN_FUR3")) {
                elem.amount -= 2;
            }
            if (has_trait("PLANTSKIN")) {
                elem.amount--;
            }
            if (has_trait("BARK")) {
                elem.amount -= 2;
            }
            if (has_trait("LIGHT_BONES")) {
                elem.amount *= 1.4;
            }
            if (has_trait("HOLLOW_BONES")) {
                elem.amount *= 1.8;
            }
            elem.amount -= mabuff_arm_bash_bonus();
        }

        if( elem.amount < 0 ) {
            elem.amount = 0;
        }
    }
    for( item& remain : worn_remains ) {
        g->m.add_item_or_charges( pos(), remain );
    }
    if( armor_destroyed ) {
        drop_inventory_overflow();
    }
}

int player::get_env_resist(body_part bp) const
{
    int ret = 0;
    for (auto &i : worn) {
        // Head protection works on eyes too (e.g. baseball cap)
        if (i.covers(bp) || (bp == bp_eyes && i.covers(bp_head))) {
            ret += i.get_env_resist();
        }
    }

    if (bp == bp_mouth && has_bionic("bio_purifier") && ret < 5) {
        ret += 2;
        if (ret > 5) {
            ret = 5;
        }
    }

    if (bp == bp_eyes && has_bionic("bio_armor_eyes") && ret < 5) {
        ret += 2;
        if (ret > 5) {
            ret = 5;
        }
    }
    return ret;
}

bool player::wearing_something_on(body_part bp) const
{
    for (auto &i : worn) {
        if (i.covers(bp))
            return true;
    }
    return false;
}


bool player::natural_attack_restricted_on( body_part bp ) const
{
    for( auto &i : worn ) {
        if( i.covers( bp ) && !i.has_flag( "ALLOWS_NATURAL_ATTACKS" ) ) {
            return true;
        }
    }
    return false;
}

bool player::is_wearing_shoes(std::string side) const
{
    bool left = true;
    bool right = true;
    if (side == "left" || side == "both") {
        left = false;
        for (auto &i : worn) {
            const item *worn_item = &i;
            if (i.covers(bp_foot_l) &&
                !worn_item->has_flag("BELTED") &&
                !worn_item->has_flag("SKINTIGHT")) {
                left = true;
                break;
            }
        }
    }
    if (side == "right" || side == "both") {
        right = false;
        for (auto &i : worn) {
            const item *worn_item = &i;
            if (i.covers(bp_foot_r) &&
                !worn_item->has_flag("BELTED") &&
                !worn_item->has_flag("SKINTIGHT")) {
                right = true;
                break;
            }
        }
    }
    return (left && right);
}

double player::footwear_factor() const
{
    double ret = 0;
    if (wearing_something_on(bp_foot_l)) {
        ret += .5;
    }
    if (wearing_something_on(bp_foot_r)) {
        ret += .5;
    }
    return ret;
}

int player::shoe_type_count(const itype_id & it) const
{
    int ret = 0;
    if (is_wearing_on_bp(it, bp_foot_l)) {
        ret++;
    }
    if (is_wearing_on_bp(it, bp_foot_r)) {
        ret++;
    }
    return ret;
}

bool player::is_wearing_power_armor(bool *hasHelmet) const {
    bool result = false;
    for( auto &elem : worn ) {
        if( !elem.is_power_armor() ) {
            continue;
        }
        if (hasHelmet == NULL) {
            // found power armor, helmet not requested, cancel loop
            return true;
        }
        // found power armor, continue search for helmet
        result = true;
        if( elem.covers( bp_head ) ) {
            *hasHelmet = true;
            return true;
        }
    }
    return result;
}

bool player::is_wearing_active_power_armor() const
{
    for( const auto &w : worn ) {
        if( w.is_power_armor() && w.active ) {
            return true;
        }
    }
    return false;
}

int player::adjust_for_focus(int amount) const
{
    int effective_focus = focus_pool;
    if (has_trait("FASTLEARNER"))
    {
        effective_focus += 15;
    }
    if (has_trait("SLOWLEARNER"))
    {
        effective_focus -= 15;
    }
    double tmp = amount * (effective_focus / 100.0);
    int ret = int(tmp);
    if (rng(0, 100) < 100 * (tmp - ret))
    {
        ret++;
    }
    return ret;
}

void player::practice( const Skill* s, int amount, int cap )
{
    SkillLevel& level = skillLevel(s);
    // Double amount, but only if level.exercise isn't a small negative number?
    if (level.exercise() < 0) {
        if (amount >= -level.exercise()) {
            amount -= level.exercise();
        } else {
            amount += amount;
        }
    }

    if( !level.can_train() ) {
        // If leveling is disabled, don't train, don't drain focus, don't print anything
        // Leaving as a skill method rather than global for possible future skill cap setting
        return;
    }

    bool isSavant = has_trait("SAVANT");

    const Skill* savantSkill = NULL;
    SkillLevel savantSkillLevel = SkillLevel();

    if (isSavant) {
        for( auto const &skill : Skill::skills ) {
            if( skillLevel( skill ) > savantSkillLevel ) {
                savantSkill = &skill;
                savantSkillLevel = skillLevel( skill );
            }
        }
    }

    amount = adjust_for_focus(amount);

    if (has_trait("PACIFIST") && s->is_combat_skill()) {
        if(!one_in(3)) {
          amount = 0;
        }
    }
    if (has_trait("PRED2") && s->is_combat_skill()) {
        if(one_in(3)) {
          amount *= 2;
        }
    }
    if (has_trait("PRED3") && s->is_combat_skill()) {
        amount *= 2;
    }

    if (has_trait("PRED4") && s->is_combat_skill()) {
        amount *= 3;
    }

    if (isSavant && s != savantSkill) {
        amount /= 2;
    }



    if (get_skill_level(s) > cap) { //blunt grinding cap implementation for crafting
        amount = 0;
        int curLevel = get_skill_level(s);
        if(is_player() && one_in(5)) {//remind the player intermittently that no skill gain takes place
            add_msg(m_info, _("This task is too simple to train your %s beyond %d."),
                    s->name().c_str(), curLevel);
        }
    }

    if (amount > 0 && level.isTraining()) {
        int oldLevel = get_skill_level(s);
        skillLevel(s).train(amount);
        int newLevel = get_skill_level(s);
        if (is_player() && newLevel > oldLevel) {
            add_msg(m_good, _("Your skill in %s has increased to %d!"), s->name().c_str(), newLevel);
        }
        if(is_player() && newLevel > cap) {
            //inform player immediately that the current recipe can't be used to train further
            add_msg(m_info, _("You feel that %s tasks of this level are becoming trivial."),
                    s->name().c_str());
        }


        int chance_to_drop = focus_pool;
        focus_pool -= chance_to_drop / 100;
        // Apex Predators don't think about much other than killing.
        // They don't lose Focus when practicing combat skills.
        if ((rng(1, 100) <= (chance_to_drop % 100)) && (!(has_trait("PRED4") &&
                                                          s->is_combat_skill()))) {
            focus_pool--;
        }
    }

    skillLevel(s).practice();
}

void player::practice( const skill_id &s, int amount, int cap )
{
    practice( &s.obj(), amount, cap );
}

bool player::has_recipe_requirements( const recipe *rec ) const
{
    bool meets_requirements = false;
    if( !rec->skill_used || get_skill_level( rec->skill_used) >= rec->difficulty ) {
        meets_requirements = true;
        for( const auto &required_skill : rec->required_skills ) {
            if( get_skill_level( required_skill.first ) < required_skill.second ) {
                meets_requirements = false;
            }
        }
    }

    return meets_requirements;
}

bool player::knows_recipe(const recipe *rec) const
{
    // do we know the recipe by virtue of it being autolearned?
    if( rec->autolearn && has_recipe_requirements( rec ) ) {
        return true;
    }

    if( learned_recipes.find( rec->ident ) != learned_recipes.end() ) {
        return true;
    }

    return false;
}

int player::has_recipe( const recipe *r, const inventory &crafting_inv ) const
{
    // Iterate over the nearby items and see if there's a book that has the recipe.
    const_invslice slice = crafting_inv.const_slice();
    int difficulty = -1;
    for( auto stack = slice.cbegin(); stack != slice.cend(); ++stack ) {
        // We are only checking qualities, so we only care about the first item in the stack.
        const item &candidate = (*stack)->front();
        if( candidate.is_book() && items_identified.count(candidate.type->id) ) {
            for( auto const & elem : candidate.type->book->recipes ) {
                // Does it have the recipe, and do we meet it's requirements?
                if( elem.recipe != r ) {
                    continue;
                }
                if( ( !r->skill_used ||
                      get_skill_level(r->skill_used) >= r->difficulty ) &&
                    ( difficulty == -1 || r->difficulty < difficulty ) ) {
                    difficulty = r->difficulty;
                }
            }
        } else {
            if (candidate.has_flag("HAS_RECIPE")){
                if (candidate.get_var("RECIPE") == r->ident){
                    if (difficulty == -1) difficulty = r->difficulty;
                }
            }
        }
    }
    return difficulty;
}

void player::learn_recipe( const recipe * const rec )
{
    learned_recipes[rec->ident] = rec;
}

void player::assign_activity(activity_type type, int moves, int index, int pos, std::string name)
{
    if( !backlog.empty() && backlog.front().type == type && backlog.front().index == index &&
        backlog.front().position == pos && backlog.front().name == name &&
        !backlog.front().auto_resume) {
        add_msg_if_player( _("You resume your task."));
        activity = backlog.front();
        backlog.pop_front();
    } else {
        if( activity.type != ACT_NULL ) {
            backlog.push_front( activity );
        }
        activity = player_activity(type, moves, index, pos, name);
    }
    if( this->moves <= activity.moves_left ) {
        activity.moves_left -= this->moves;
        this->moves = 0;
    } else {
        this->moves -= activity.moves_left;
        activity.moves_left = 0;
    }
    activity.warned_of_proximity = false;
}

bool player::has_activity(const activity_type type) const
{
    if (activity.type == type) {
        return true;
    }

    return false;
}

void player::cancel_activity()
{
    // Clear any backlog items that aren't auto-resume.
    for( auto backlog_item = backlog.begin(); backlog_item != backlog.end(); ) {
        if( backlog_item->auto_resume ) {
            backlog_item++;
        } else {
            backlog_item = backlog.erase( backlog_item );
        }
    }
    if( activity.is_suspendable() ) {
        backlog.push_front( activity );
    }
    activity = player_activity();
}

bool player::has_gun_for_ammo( const ammotype &at ) const
{
    return has_item_with( [at]( const item & it ) {
        // item::ammo_type considers the active gunmod.
        return it.is_gun() && it.ammo_type() == at;
    } );
}

std::string player::weapname(bool charges) const
{
    if (!(weapon.is_tool() && dynamic_cast<const it_tool*>(weapon.type)->max_charges <= 0) &&
          weapon.charges >= 0 && charges) {
        std::stringstream dump;
        int spare_mag = weapon.has_gunmod("spare_mag");
        // For guns, just print the unadorned name.
        dump << weapon.type_name(1).c_str();
        if (!(weapon.has_flag("NO_AMMO") || weapon.has_flag("RELOAD_AND_SHOOT"))) {
            dump << " (" << weapon.charges;
            if( -1 != spare_mag ) {
                dump << "+" << weapon.contents[spare_mag].charges;
            }
            for (auto &i : weapon.contents) {
                if( i.is_auxiliary_gunmod() ) {
                    dump << "+" << i.charges;
                }
            }
            dump << ")";
        }
        return dump.str();
    } else if (weapon.is_container()) {
        std::stringstream dump;
        dump << weapon.tname().c_str();
        if(weapon.contents.size() == 1) {
            dump << " (" << weapon.contents[0].charges << ")";
        }
        return dump.str();
    } else if (weapon.is_null()) {
        return _("fists");
    } else {
        return weapon.tname();
    }
}

bool player::wield_contents(item *container, int pos, int factor)
{
    // if index not specified and container has multiple items then ask the player to choose one
    if( pos < 0 ) {
        std::vector<std::string> opts;
        std::transform( container->contents.begin(), container->contents.end(),
                        std::back_inserter( opts ), []( const item& elem ) {
                            return elem.display_name();
                        } );
        if( opts.size() > 1 ) {
            pos = ( uimenu( false, _("Wield what?"), opts ) ) - 1;
        } else {
            pos = 0;
        }
    }

    if( pos >= static_cast<int>(container->contents.size() ) ) {
        debugmsg( "Tried to wield non-existent item from container (player::wield_contents)" );
        return false;
    }

    if( !can_wield( container->contents[pos] ) ) {
        return false;
    }

    int mv = 0;

    if( is_armed() ) {
        if( volume_carried() + weapon.volume() - container->contents[pos].volume() <
            volume_capacity() ) {
            mv += item_handling_cost(weapon);
            inv.add_item_keep_invlet( remove_weapon() );
        } else if( query_yn( _("No space in inventory for your %s.  Drop it?"),
                             weapon.tname().c_str() ) ) {
            g->m.add_item_or_charges( posx(), posy(), remove_weapon() );
        } else {
            return false;
        }
        inv.unsort();
    }

    weapon = container->contents[pos];
    inv.assign_empty_invlet( weapon, true );
    last_item = itype_id( weapon.type->id );
    container->contents.erase( container->contents.begin() + pos );

    // TODO Doxygen comment covering all possible gun and weapon skills
    // documenting decrease in time spent wielding from a container
    int lvl = get_skill_level( weapon.is_gun() ? weapon.gun_skill() : weapon.weap_skill() );

    mv += (weapon.volume() * factor) / std::max( lvl, 1 );
    moves -= mv;

    weapon.on_wield( *this, mv );

    return true;
}

void player::store(item* container, item* put, const skill_id &skill_used, int volume_factor)
{
    const int lvl = get_skill_level(skill_used);
    moves -= (lvl == 0) ? ((volume_factor + 1) * put->volume()) : (volume_factor * put->volume()) / lvl;
    container->put_in(i_rem(put));
}

nc_color encumb_color(int level)
{
 if (level < 0)
  return c_green;
 if (level < 10)
  return c_ltgray;
 if (level < 40)
  return c_yellow;
 if (level < 70)
  return c_ltred;
 return c_red;
}

void player::set_skill_level(const Skill* _skill, int level)
{
    skillLevel(_skill).level(level);
}

void player::set_skill_level(Skill const &_skill, int level)
{
    set_skill_level(&_skill, level);
}

void player::set_skill_level(const skill_id &ident, int level)
{
    skillLevel(ident).level(level);
}

void player::boost_skill_level(const Skill* _skill, int level)
{
    skillLevel(_skill).level(level+skillLevel(_skill));
}

void player::boost_skill_level(const skill_id &ident, int level)
{
    skillLevel(ident).level(level+skillLevel(ident));
}

int player::get_melee() const
{
    return get_skill_level( skill_id( "melee" ) );
}

void player::setID (int i)
{
    if( id >= 0 ) {
        debugmsg( "tried to set id of a npc/player, but has already a id: %d", id );
    } else if( i < -1 ) {
        debugmsg( "tried to set invalid id of a npc/player: %d", i );
    } else {
        id = i;
    }
}

int player::getID () const
{
    return this->id;
}

bool player::uncanny_dodge()
{
    bool is_u = this == &g->u;
    bool seen = g->u.sees( *this );
    if( this->power_level < 74 || !this->has_active_bionic("bio_uncanny_dodge") ) { return false; }
    tripoint adjacent = adjacent_tile();
    charge_power(-75);
    if( adjacent.x != posx() || adjacent.y != posy()) {
        position.x = adjacent.x;
        position.y = adjacent.y;
        if( is_u ) {
            add_msg( _("Time seems to slow down and you instinctively dodge!") );
        } else if( seen ) {
            add_msg( _("%s dodges... so fast!"), this->disp_name().c_str() );

        }
        return true;
    }
    if( is_u ) {
        add_msg( _("You try to dodge but there's no room!") );
    } else if( seen ) {
        add_msg( _("%s tries to dodge but there's no room!"), this->disp_name().c_str() );
    }
    return false;
}

// adjacent_tile() returns a safe, unoccupied adjacent tile. If there are no such tiles, returns player position instead.
tripoint player::adjacent_tile() const
{
    std::vector<tripoint> ret;
    int dangerous_fields;
    for( const tripoint &p : g->m.points_in_radius( pos(), 1 ) ) {
        if( p == pos() ) {
            // Don't consider player position
            continue;
        }
        const trap &curtrap = g->m.tr_at( p );
        if( g->mon_at( p ) == -1 && g->npc_at( p ) == -1 && g->m.move_cost( p ) > 0 &&
            (curtrap.is_null() || curtrap.is_benign()) ) {
            // Only consider tile if unoccupied, passable and has no traps
            dangerous_fields = 0;
            auto &tmpfld = g->m.field_at( p );
            for( auto &fld : tmpfld ) {
                const field_entry &cur = fld.second;
                if( cur.is_dangerous() ) {
                    dangerous_fields++;
                }
            }

            if( dangerous_fields == 0 ) {
                ret.push_back( p );
            }
        }
    }

    return random_entry( ret, pos() ); // player position if no valid adjacent tiles
}

int player::climbing_cost( const tripoint &from, const tripoint &to ) const
{
    if( !g->m.valid_move( from, to, false, true ) ) {
        return 0;
    }

    const int diff = g->m.climb_difficulty( from );

    if( diff > 5 ) {
        return 0;
    }

    return 50 + diff * 100;
    // TODO: All sorts of mutations, equipment weight etc.
}

void player::environmental_revert_effect()
{
    addictions.clear();
    morale.clear();

    for (int part = 0; part < num_hp_parts; part++) {
        hp_cur[part] = hp_max[part];
    }
    set_hunger(0);
    thirst = 0;
    fatigue = 0;
    set_healthy(0);
    set_healthy_mod(0);
    stim = 0;
    pain = 0;
    pkill = 0;
    radiation = 0;

    recalc_sight_limits();
}

bool player::is_invisible() const
{
    static const std::string str_bio_cloak("bio_cloak"); // This function used in monster::plan_moves
    static const std::string str_bio_night("bio_night");
    return (
        has_active_bionic(str_bio_cloak) ||
        has_active_bionic(str_bio_night) ||
        has_active_optcloak() ||
        has_trait("DEBUG_CLOAK") ||
        has_artifact_with(AEP_INVISIBLE)
    );
}

int player::visibility( bool, int ) const
{
    // 0-100 %
    if ( is_invisible() ) {
        return 0;
    }
    // todo:
    // if ( dark_clothing() && light check ...
    return 100;
}

void player::set_destination(const std::vector<tripoint> &route)
{
    auto_move_route = route;
}

void player::clear_destination()
{
    auto_move_route.clear();
    next_expected_position = tripoint_min;
}

bool player::has_destination() const
{
    return !auto_move_route.empty();
}

std::vector<tripoint> &player::get_auto_move_route()
{
    return auto_move_route;
}

action_id player::get_next_auto_move_direction()
{
    if (!has_destination()) {
        return ACTION_NULL;
    }

    if (next_expected_position != tripoint_min ) {
        if( pos3() != next_expected_position ) {
            // We're off course, possibly stumbling or stuck, cancel auto move
            return ACTION_NULL;
        }
    }

    next_expected_position = auto_move_route.front();
    auto_move_route.erase(auto_move_route.begin());

    tripoint dp = next_expected_position - pos3();

    // Make sure the direction is just one step and that
    // all diagonal moves have 0 z component
    if( abs( dp.x ) > 1 || abs( dp.y ) > 1 || abs( dp.z ) > 1 ||
        ( abs( dp.z ) != 0 && ( abs( dp.x ) != 0 || abs( dp.y ) != 0 ) ) ) {
        // Should never happen, but check just in case
        return ACTION_NULL;
    }

    return get_movement_direction_from_delta( dp.x, dp.y, dp.z );
}

void player::shift_destination(int shiftx, int shifty)
{
    if( next_expected_position != tripoint_min ) {
        next_expected_position.x += shiftx;
        next_expected_position.y += shifty;
    }

    for( auto &elem : auto_move_route ) {
        elem.x += shiftx;
        elem.y += shifty;
    }
}

bool player::has_weapon() const
{
    return !unarmed_attack();
}

m_size player::get_size() const
{
    return MS_MEDIUM;
}

int player::get_hp( hp_part bp ) const
{
    if( bp < num_hp_parts ) {
        return hp_cur[bp];
    }
    int hp_total = 0;
    for( int i = 0; i < num_hp_parts; ++i ) {
        hp_total += hp_cur[i];
    }
    return hp_total;
}

int player::get_hp_max( hp_part bp ) const
{
    if( bp < num_hp_parts ) {
        return hp_max[bp];
    }
    int hp_total = 0;
    for( int i = 0; i < num_hp_parts; ++i ) {
        hp_total += hp_max[i];
    }
    return hp_total;
}

int player::get_stamina_max() const
{
    if (has_trait("BADCARDIO"))
        return 750;
    if (has_trait("GOODCARDIO"))
        return 1250;
    return 1000;
}

void player::burn_move_stamina( int moves )
{
    // Regain 10 stamina / turn
    // 7/turn walking
    // 20/turn running
    int burn_ratio = 7;
    if( move_mode == "run" ) {
        burn_ratio = 20;
    }
    mod_stat( "stamina", -((moves * burn_ratio) / 100) );
}

field_id player::playerBloodType() const
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

Creature::Attitude player::attitude_to( const Creature &other ) const
{
    const auto m = dynamic_cast<const monster *>( &other );
    const auto p = dynamic_cast<const npc *>( &other );
    if( m != nullptr ) {
        if( m->friendly != 0 ) {
            return A_FRIENDLY;
        }
        switch( m->attitude( const_cast<player *>( this ) ) ) {
                // player probably does not want to harm them, but doesn't care much at all.
            case MATT_FOLLOW:
            case MATT_FPASSIVE:
            case MATT_IGNORE:
            case MATT_FLEE:
                return A_NEUTRAL;
                // player does not want to harm those.
            case MATT_FRIEND:
            case MATT_ZLAVE: // Don't want to harm your zlave!
                return A_FRIENDLY;
            case MATT_ATTACK:
                return A_HOSTILE;
            case MATT_NULL:
            case NUM_MONSTER_ATTITUDES:
                break;
        }
    } else if( p != nullptr ) {
        if( p->attitude == NPCATT_KILL ) {
            return A_HOSTILE;
        } else if( p->is_friend() ) {
            return A_FRIENDLY;
        } else {
            return A_NEUTRAL;
        }
    }
    return A_NEUTRAL;
}

bool player::sees( const tripoint &t, bool ) const
{
    static const std::string str_bio_night("bio_night");
    const int wanted_range = rl_dist( pos3(), t );
    bool can_see = is_player() ? g->m.pl_sees( t, wanted_range ) :
        Creature::sees( t );;
    // Only check if we need to override if we already came to the opposite conclusion.
    if( can_see && wanted_range < 15 && wanted_range > sight_range(1) &&
        has_active_bionic(str_bio_night) ) {
        can_see = false;
    }
    if( can_see && wanted_range > unimpaired_range() ) {
        can_see = false;
    }
    // Clairvoyance is a really expensive check,
    // so try to avoid making it if at all possible.
    if( !can_see && wanted_range < MAX_CLAIRVOYANCE && wanted_range < clairvoyance() ) {
        return true;
    }
    return can_see;
}

bool player::sees( const Creature &critter ) const
{
    // This handles only the player/npc specific stuff (monsters don't have traits or bionics).
    const int dist = rl_dist( pos3(), critter.pos3() );
    if (dist <= 3 && has_trait("ANTENNAE")) {
        return true;
    }
    if( critter.digging() && has_active_bionic( "bio_ground_sonar" ) ) {
        // Bypass the check below, the bionic sonar also bypasses the sees(point) check because
        // walls don't block sonar which is transmitted in the ground, not the air.
        // TODO: this might need checks whether the player is in the air, or otherwise not connected
        // to the ground. It also might need a range check.
        return true;
    }
    return Creature::sees( critter );
}

bool player::has_container_for( const item &newit ) const
{
    if( !newit.made_of( LIQUID ) ) {
        // Currently only liquids need a container
        return true;
    }
    unsigned charges = newit.charges;
    charges -= weapon.get_remaining_capacity_for_liquid( newit );
    for( auto& w : worn ) {
        charges -= w.get_remaining_capacity_for_liquid( newit );
    }
    for( size_t i = 0; i < inv.size() && charges > 0; i++ ) {
        const std::list<item>&items = inv.const_stack( i );
        // Assume that each item in the stack has the same remaining capacity
        charges -= items.front().get_remaining_capacity_for_liquid( newit ) * items.size();
    }
    return charges <= 0;
}




nc_color player::bodytemp_color(int bp) const
{
  nc_color color =  c_ltgray; // default
    if (bp == bp_eyes) {
        color = c_ltgray;    // Eyes don't count towards warmth
    } else if (temp_conv[bp] >  BODYTEMP_SCORCHING) {
        color = c_red;
    } else if (temp_conv[bp] >  BODYTEMP_VERY_HOT) {
        color = c_ltred;
    } else if (temp_conv[bp] >  BODYTEMP_HOT) {
        color = c_yellow;
    } else if (temp_conv[bp] >  BODYTEMP_COLD) {
        color = c_green;
    } else if (temp_conv[bp] >  BODYTEMP_VERY_COLD) {
        color = c_ltblue;
    } else if (temp_conv[bp] >  BODYTEMP_FREEZING) {
        color = c_cyan;
    } else if (temp_conv[bp] <= BODYTEMP_FREEZING) {
        color = c_blue;
    }
    return color;
}

//message related stuff
void player::add_msg_if_player(const char* msg, ...) const
{
    va_list ap;
    va_start(ap, msg);
    Messages::vadd_msg(msg, ap);
    va_end(ap);
}
void player::add_msg_player_or_npc(const char* player_str, const char* npc_str, ...) const
{
    va_list ap;
    va_start(ap, npc_str);
    Messages::vadd_msg(player_str, ap);
    va_end(ap);
}
void player::add_msg_if_player(game_message_type type, const char* msg, ...) const
{
    va_list ap;
    va_start(ap, msg);
    Messages::vadd_msg(type, msg, ap);
    va_end(ap);
}
void player::add_msg_player_or_npc(game_message_type type, const char* player_str, const char* npc_str, ...) const
{
    va_list ap;
    va_start(ap, npc_str);
    Messages::vadd_msg(type, player_str, ap);
    va_end(ap);
}

bool player::knows_trap( const tripoint &pos ) const
{
    const tripoint p = g->m.getabs( pos );
    return known_traps.count( p ) > 0;
}

void player::add_known_trap( const tripoint &pos, const trap &t)
{
    const tripoint p = g->m.getabs( pos );
    if( t.is_null() ) {
        known_traps.erase( p );
    } else {
        // TODO: known_traps should map to a trap_str_id
        known_traps[p] = t.id.str();
    }
}

bool player::is_deaf() const
{
    return get_effect_int( "deaf" ) > 2 || worn_with_flag("DEAF") ||
           (has_active_bionic("bio_earplugs") && !has_active_bionic("bio_ears"));
}

bool player::can_hear( const tripoint &source, const int volume ) const
{
    if( is_deaf() ) {
        return false;
    }

    // source is in-ear and at our square, we can hear it
    if ( source == pos3() && volume == 0 ) {
        return true;
    }

    // TODO: sound attenuation due to weather

    const int dist = rl_dist( source, pos3() );
    const float volume_multiplier = hearing_ability();
    return volume * volume_multiplier >= dist;
}

float player::hearing_ability() const
{
    float volume_multiplier = 1.0;

    // Mutation/Bionic volume modifiers
    if( has_active_bionic("bio_ears") && !has_active_bionic("bio_earplugs") ) {
        volume_multiplier *= 3.5;
    }
    if( has_trait("PER_SLIME") ) {
        // Random hearing :-/
        // (when it's working at all, see player.cpp)
        // changed from 0.5 to fix Mac compiling error
        volume_multiplier *= (rng(1, 2));
    }
    if( has_trait("BADHEARING") ) {
        volume_multiplier *= .5;
    }
    if( has_trait("GOODHEARING") ) {
        volume_multiplier *= 1.25;
    }
    if( has_trait("CANINE_EARS") ) {
        volume_multiplier *= 1.5;
    }
    if( has_trait("URSINE_EARS") || has_trait("FELINE_EARS") ) {
        volume_multiplier *= 1.25;
    }
    if( has_trait("LUPINE_EARS") ) {
        volume_multiplier *= 1.75;
    }

    if( has_effect( "deaf" ) ) {
        // Scale linearly up to 300
        volume_multiplier *= ( 300.0 - get_effect_dur( "deaf" ) ) / 300.0;
    }

    if( has_effect( "earphones" ) ) {
        volume_multiplier *= .25;
    }

    return volume_multiplier;
}

int player::print_info(WINDOW* w, int vStart, int, int column) const
{
    mvwprintw( w, vStart++, column, _( "You (%s)" ), name.c_str() );
    return vStart;
}

std::vector<Creature *> get_creatures_if( std::function<bool (const Creature &)>pred )
{
    std::vector<Creature *> result;
    for( size_t i = 0; i < g->num_zombies(); i++ ) {
        auto &critter = g->zombie( i );
        if( !critter.is_dead() && pred( critter ) ) {
            result.push_back( &critter );
        }
    }
    for( auto & n : g->active_npc ) {
        if( pred( *n ) ) {
            result.push_back( n );
        }
    }
    if( pred( g->u ) ) {
        result.push_back( &g->u );
    }
    return result;
}

bool player::is_visible_in_range( const Creature &critter, const int range ) const
{
    return sees( critter ) && rl_dist( pos(), critter.pos() ) <= range;
}

std::vector<Creature *> player::get_visible_creatures( const int range ) const
{
    return get_creatures_if( [this, range]( const Creature &critter ) -> bool {
        return this != &critter && this->sees(critter) &&
          rl_dist( this->pos(), critter.pos() ) <= range;
    } );
}

std::vector<Creature *> player::get_targetable_creatures( const int range ) const
{
    return get_creatures_if( [this, range]( const Creature &critter ) -> bool {
        return this != &critter && ( this->sees(critter) || this->sees_with_infrared(critter) ) &&
          rl_dist( this->pos(), critter.pos() ) <= range;
    } );
}

std::vector<Creature *> player::get_hostile_creatures() const
{
    return get_creatures_if( [this] ( const Creature &critter ) -> bool {
        return this != &critter && this->sees(critter) && critter.attitude_to(*this) == A_HOSTILE;
    } );
}


void player::place_corpse()
{
    std::vector<item *> tmp = inv_dump();
    item body;
    body.make_corpse( NULL_ID, calendar::turn, name );
    for( auto itm : tmp ) {
        g->m.add_item_or_charges( pos(), *itm );
    }
    for( auto & bio : my_bionics ) {
        if( item::type_is_defined( bio.id ) ) {
            body.put_in( item( bio.id, calendar::turn ) );
        }
    }

    // Restore amount of installed pseudo-modules of Power Storage Units
    std::pair<int, int> storage_modules = amount_of_storage_bionics();
    for (int i = 0; i < storage_modules.first; ++i) {
        body.contents.push_back( item( "bio_power_storage", calendar::turn ) );
    }
    for (int i = 0; i < storage_modules.second; ++i) {
        body.contents.push_back( item( "bio_power_storage_mkII", calendar::turn ) );
    }
    g->m.add_item_or_charges( pos(), body );
}

bool player::sees_with_infrared( const Creature &critter ) const
{
    const bool has_ir = has_active_bionic( "bio_infrared" ) ||
                        has_trait( "INFRARED" ) ||
                        has_trait( "LIZ_IR" ) ||
                        worn_with_flag( "IR_EFFECT" );
    if( !has_ir || !critter.is_warm() ) {
        return false;
    }
    return g->m.sees(pos(), critter.pos(), sight_range(DAYLIGHT_LEVEL) );
}

std::vector<std::string> player::get_overlay_ids() const {
    std::vector<std::string> rval;

    // first get mutations
    for( auto & mutation : get_mutations() ) {
        rval.push_back("mutation_"+mutation);
    }

    // next clothing
    // TODO: worry about correct order of clothing overlays
    for(const item& worn_item : worn) {
        rval.push_back("worn_"+worn_item.typeId());
    }

    // last weapon
    // TODO: might there be clothing that covers the weapon?
    if(!weapon.is_null()) {
        rval.push_back("wielded_"+weapon.typeId());
    }
    return rval;
}

void player::spores()
{
    sounds::sound( pos(), 10, _("Pouf!")); //~spore-release sound
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (i == 0 && j == 0) {
                continue;
            }

            tripoint sporep( posx() + i, posy() + j, posz() );
            g->m.fungalize( sporep, this, 0.25 );
        }
    }
}

void player::blossoms()
{
    // Player blossoms are shorter-ranged, but you can fire much more frequently if you like.
    sounds::sound( pos(), 10, _("Pouf!"));
    tripoint tmp = pos();
    int &i = tmp.x;
    int &j = tmp.y;
    for ( i = posx() - 2; i <= posx() + 2; i++) {
        for ( j = posy() - 2; j <= posy() + 2; j++) {
            g->m.add_field( tmp, fd_fungal_haze, rng(1, 2), 0 );
        }
    }
}

int player::add_ammo_to_worn_quiver( item &ammo )
{
    std::vector<item *>quivers;
    for( auto & worn_item : worn) {
        if( worn_item.type->can_use( "QUIVER")) {
            quivers.push_back( &worn_item);
        }
    }

    // sort quivers by contents, such that empty quivers go last
    std::sort( quivers.begin(), quivers.end(), item_ptr_compare_by_charges);

    int quivered_sum = 0;
    int move_cost_per_arrow = 10;
    for( std::vector<item *>::iterator it = quivers.begin(); it != quivers.end(); it++) {
        item *quiver = *it;
        int stored = quiver->quiver_store_arrow( ammo);
        if( stored > 0) {
            add_msg_if_player( ngettext( "You store %1$d %2$s in your %3$s.", "You store %1$d %2$s in your %3$s.", stored),
                               stored, quiver->contents[0].type_name(stored).c_str(), quiver->type_name().c_str());
        }
        moves -= std::min( 100, stored * move_cost_per_arrow);
        quivered_sum += stored;
    }

    return quivered_sum;
}

float player::power_rating() const
{
    int ret = 2;
    // Small guns can be easily hidden from view
    if( weapon.volume() <= 1 ) {
        ret = 2;
    } else if( weapon.is_gun() ) {
        ret = 4;
    } else if( weapon.damage_bash() + weapon.damage_cut() > 20 ) {
        ret = 3; // Melee weapon or weapon-y tool
    }
    if( has_trait("HUGE") || has_trait("HUGE_OK") ) {
        ret += 1;
    }
    if( is_wearing_power_armor( nullptr ) ) {
        ret = 5; // No mercy!
    }
    return ret;
}

std::vector<const item *> player::all_items_with_flag( const std::string flag ) const
{
    return items_with( [&flag]( const item & it ) {
        return it.has_flag( flag );
    } );
}

bool player::has_item_with_flag( std::string flag ) const
{
    return has_item_with( [&flag]( const item & it ) {
        return it.has_flag( flag );
    } );
}

bool player::has_items_with_quality( const std::string &quality_id, int level, int amount ) const
{
    return has_item_with( [&quality_id, level, &amount]( const item &it ) {
        if( it.has_quality( quality_id, level ) ) {
            // Each suitable item decreases the require count until it reaches 0, where the requirement is fulfilled.
            amount--;
        }
        return amount <= 0;
    } );
}

void player::on_mission_assignment( mission &new_mission )
{
    active_missions.push_back( &new_mission );
    set_active_mission( new_mission );
}

void player::on_mission_finished( mission &mission )
{
    if( mission.has_failed() ) {
        failed_missions.push_back( &mission );
    } else {
        completed_missions.push_back( &mission );
    }
    const auto iter = std::find( active_missions.begin(), active_missions.end(), &mission );
    if( iter == active_missions.end() ) {
        debugmsg( "completed mission %d was not in the active_missions list", mission.get_id() );
    } else {
        active_missions.erase( iter );
    }
    if( &mission == active_mission ) {
        if( active_missions.empty() ) {
            active_mission = nullptr;
        } else {
            active_mission = active_missions.front();
        }
    }
}

void player::set_active_mission( mission &mission )
{
    const auto iter = std::find( active_missions.begin(), active_missions.end(), &mission );
    if( iter == active_missions.end() ) {
        debugmsg( "new active mission %d is not in the active_missions list", mission.get_id() );
    } else {
        active_mission = &mission;
    }
}

mission *player::get_active_mission() const
{
    return active_mission;
}

tripoint player::get_active_mission_target() const
{
    if( active_mission == nullptr ) {
        return overmap::invalid_tripoint;
    }
    return active_mission->get_target();
}

std::vector<mission*> player::get_active_missions() const
{
    return active_missions;
}

std::vector<mission*> player::get_completed_missions() const
{
    return completed_missions;
}

std::vector<mission*> player::get_failed_missions() const
{
    return failed_missions;
}

void player::print_encumbrance(WINDOW *win, int min, int max, int line) const
{
    // initialize these once, and only once
    static std::string asText[] = {_("Torso"), _("Head"), _("Eyes"), _("Mouth"), _("L. Arm"), _("R. Arm"),
                             _("L. Hand"), _("R. Hand"), _("L. Leg"), _("R. Leg"), _("L. Foot"),
                             _("R. Foot")};
    static body_part aBodyPart[] = {bp_torso, bp_head, bp_eyes, bp_mouth, bp_arm_l, bp_arm_r, bp_hand_l,
                             bp_hand_r, bp_leg_l, bp_leg_r, bp_foot_l, bp_foot_r};
    int iEnc, iArmorEnc, iBodyTempInt;
    double iLayers;
    std::string out;
    /*** I chose to instead only display X+Y instead of X+Y=Z. More room was needed ***
     *** for displaying triple digit encumbrance, due to new encumbrance system.    ***
     *** If the player wants to see the total without having to do them maths, the  ***
     *** armor layers ui shows everything they want :-) -Davek                      ***/
    for (int i = min; i < max; ++i) {
        out.clear();
        iLayers = iArmorEnc = 0;
        iBodyTempInt = (temp_conv[i] / 100.0) * 2 - 100; // Scale of -100 to +100
        iEnc = encumb(aBodyPart[i], iLayers, iArmorEnc);
        // limb, and possible color highlighting
        out = string_format("%-7s", asText[i].c_str());
        mvwprintz(win, i + 1 - min, 1, (line == i) ? h_ltgray : c_ltgray, out.c_str());
        // take into account the new encumbrance system for layers
        out = string_format("(%1d) ", static_cast<int>(iLayers / 10.0));
        wprintz(win, c_ltgray, out.c_str());
        // accumulated encumbrance from clothing, plus extra encumbrance from layering
        wprintz(win, encumb_color(iEnc), string_format("%3d", iArmorEnc).c_str());
        // seperator in low toned color
        wprintz(win, c_ltgray, "+");
        wprintz(win, encumb_color(iEnc), string_format("%-3d", iEnc - iArmorEnc).c_str());
        // print warmth, tethered to right hand side of the window
        out = string_format("(% 3d)", iBodyTempInt);
        mvwprintz(win, i + 1 - min, getmaxx(win) - 6, bodytemp_color(i), out.c_str());
    }
}

