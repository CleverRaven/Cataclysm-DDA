#include "player.h"
#include "profession.h"
#include "bionics.h"
#include "mission.h"
#include "game.h"
#include "disease.h"
#include "addiction.h"
#include "moraledata.h"
#include "inventory.h"
#include "options.h"
#include <sstream>
#include <stdlib.h>
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

#include <fstream>

std::map<std::string, trait> traits;
extern std::map<std::string, martialart> ma_styles;
std::vector<std::string> vStartingTraits[2];

std::string morale_data[NUM_MORALE_TYPES];

stats player_stats;

static const itype_id OPTICAL_CLOAK_ITEM_ID( "optical_cloak" );

void game::init_morale()
{
    std::string tmp_morale_data[NUM_MORALE_TYPES] = {
    "This is a bug (moraledata.h:moraledata)",
    _("Enjoyed %i"),
    _("Enjoyed a hot meal"),
    _("Music"),
    _("Enjoyed honey"),
    _("Played Video Game"),
    _("Marloss Bliss"),
    _("Mutagenic Anticipation"),
    _("Good Feeling"),
    _("Supported"),
    _("Looked at photos"),

    _("Nicotine Craving"),
    _("Caffeine Craving"),
    _("Alcohol Craving"),
    _("Opiate Craving"),
    _("Speed Craving"),
    _("Cocaine Craving"),
    _("Crack Cocaine Craving"),
    _("Mutagen Craving"),
    _("Diazepam Craving"),
    _("Marloss Craving"),

    _("Disliked %i"),
    _("Ate Human Flesh"),
    _("Ate Meat"),
    _("Ate Vegetables"),
    _("Ate Fruit"),
    _("Lactose Intolerance"),
    _("Ate Junk Food"),
    _("Wheat Allergy"),
    _("Ate Indigestible Food"),
    _("Wet"),
    _("Dried Off"),
    _("Cold"),
    _("Hot"),
    _("Bad Feeling"),
    _("Killed Innocent"),
    _("Killed Friend"),
    _("Guilty about Killing"),
    _("Guilty about Mutilating Corpse"),
    _("Chimerical Mutation"),
    _("Fey Mutation"),

    _("Moodswing"),
    _("Read %i"),
    _("Got comfy"),

    _("Heard Disturbing Scream"),

    _("Masochism"),
    _("Hoarder"),
    _("Stylish"),
    _("Optimist"),
    _("Bad Tempered"),
    //~ You really don't like wearing the Uncomfy Gear
    _("Uncomfy Gear"),
    _("Found kitten <3")
    };
    for (int i = 0; i < NUM_MORALE_TYPES; ++i) {
        morale_data[i]=tmp_morale_data[i];
    }
}


std::string morale_point::name() const
{
    // Start with the morale type's description.
    std::string ret = morale_data[type];

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

player::player() : Character(), name("")
{
 posx = 0;
 posy = 0;
 id = -1; // -1 is invalid
 view_offset_x = 0;
 view_offset_y = 0;
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
 hunger = 0;
 thirst = 0;
 stomach_food = 0;
 stomach_water = 0;
 fatigue = 0;
 stim = 0;
 pain = 0;
 pkill = 0;
 radiation = 0;
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
 active_mission = -1;
 in_vehicle = false;
 controlling_vehicle = false;
 grab_point.x = 0;
 grab_point.y = 0;
 grab_type = OBJECT_NONE;
 style_selected = "style_none";
 keep_hands_free = false;
 focus_pool = 100;
 last_item = itype_id("null");
 sight_max = 9999;
 sight_boost = 0;
 sight_boost_cap = 0;
 last_batch = 0;
 lastconsumed = itype_id("null");
 next_expected_position.x = -1;
 next_expected_position.y = -1;

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

 mDrenchEffect[bp_eyes] = 1;
 mDrenchEffect[bp_mouth] = 1;
 mDrenchEffect[bp_head] = 7;
 mDrenchEffect[bp_leg_l] = 11;
 mDrenchEffect[bp_leg_r] = 11;
 mDrenchEffect[bp_foot_l] = 3;
 mDrenchEffect[bp_foot_r] = 3;
 mDrenchEffect[bp_arm_l] = 10;
 mDrenchEffect[bp_arm_r] = 10;
 mDrenchEffect[bp_hand_l] = 3;
 mDrenchEffect[bp_hand_r] = 3;
 mDrenchEffect[bp_torso] = 40;

 recalc_sight_limits();
}

player::~player()
{
}

void player::normalize()
{
    Creature::normalize();

    ret_null = item("null", 0);
    weapon   = item("null", 0);
    style_selected = "style_none";

    recalc_hp();

    for (int i = 0 ; i < num_bp; i++) {
        temp_conv[i] = BODYTEMP_NORM;
    }
}

void player::pick_name()
{
    name = Name::generate(male);
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

// just a shim for now since actual player death is handled in game::is_game_over
void player::die(Creature* nkiller)
{
    if( nkiller != NULL && !nkiller->is_fake() ) {
        killer = nkiller;
    }
    set_turn_died(int(calendar::turn));
}

void player::reset_stats()
{
    clear_miss_reasons();

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
        add_miss_reason(_("Your thick scales get in the way."), 2);
    }
    if (has_trait("CHITIN2") || has_trait("CHITIN3") || has_trait("CHITIN_FUR3")) {
        mod_dex_bonus(-1);
        add_miss_reason(_("Your chitin gets in the way."), 1);
    }
    if (has_trait("COMPOUND_EYES") && !wearing_something_on(bp_eyes)) {
        mod_per_bonus(1);
    }
    if (has_trait("BIRD_EYE")) {
        mod_per_bonus(4);
    }
    if (has_trait("INSECT_ARMS")) {
        mod_dex_bonus(-2);
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
        mod_dex_bonus(-1);
        add_miss_reason(_("Your webbed hands get in the way."), 1);
    }
    if (has_trait("ARACHNID_ARMS")) {
        mod_dex_bonus(-4);
        add_miss_reason(_("Your arachnid limbs get in the way."), 4);
    }
    if (has_trait("ARACHNID_ARMS_OK")) {
        if (!wearing_something_on(bp_torso)) {
            mod_dex_bonus(2);
        }
        else {
            mod_dex_bonus(-2);
            add_miss_reason(_("Your clothing constricts your arachnid limbs."), 2);
        }
    }
    if (has_trait("ARM_TENTACLES") || has_trait("ARM_TENTACLES_4") ||
            has_trait("ARM_TENTACLES_8")) {
        mod_dex_bonus(1);
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
    mod_dodge_bonus( mabuff_dodge_bonus() - (encumb(bp_leg_l) + encumb(bp_leg_r))/2 - encumb(bp_torso) );
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
    if (has_trait("WINGS_BAT")) {
        mod_dodge_bonus(-3);
    }
    if (has_trait("WINGS_BUTTERFLY")) {
        mod_dodge_bonus(-4);
    }

    if (str_max >= 16) {mod_dodge_bonus(-1);} // Penalty if we're huge
    else if (str_max <= 5) {mod_dodge_bonus(1);} // Bonus if we're small

    // Hit-related effects
    mod_hit_bonus( mabuff_tohit_bonus() + weapon.type->m_to_hit - encumb(bp_torso) );

    // Apply static martial arts buffs
    ma_static_effects();

    if (int(calendar::turn) % 10 == 0) {
        update_mental_focus();
    }
    nv_cached = false;
    pda_cached = false;

    recalc_sight_limits();
    recalc_speed_bonus();

    Creature::reset_stats();

}

void player::process_turn()
{
    Creature::process_turn();

    // Didn't just pick something up
    last_item = itype_id("null");

    if (has_active_bionic("bio_metabolics") && power_level + 25 <= max_power_level &&
            hunger < 100 && (int(calendar::turn) % 5 == 0)) {
        hunger += 2;
        power_level += 25;
    }

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
    // Or maybe you're debugging and would rather not be smelled.
    if (has_trait("CHLOROMORPH") || has_trait("DEBUG_NOSCENT")) {
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
        int pen = int((volume_capacity()-volume_carried()) / 2);
        if (pen > 70)
        {
            pen = 70;
        }
        if (pen <= 0)
        {
            pen = 0;
        }
        if (has_effect("took_xanax"))
        {
            pen = int(pen / 7);
        }
        else if (has_effect("took_prozac"))
        {
            pen = int(pen / 2);
        }
        add_morale(MORALE_PERM_HOARDER, -pen, -pen, 5, 5, true);
    }

    // The stylish get a morale bonus for each body part covered in an item
    // with the FANCY or SUPER_FANCY tag.
    if (has_trait("STYLISH"))
    {
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
    if (fatigue >= 383 && focus_pool > 60) {
        focus_pool = 60;
    }
}

// written mostly by FunnyMan3595 in Github issue #613 (DarklingWolf's repo),
// with some small edits/corrections by Soron
int player::calc_focus_equilibrium()
{
    // Factor in pain, since it's harder to rest your mind while your body hurts.
    int eff_morale = morale_level() - pain;
    // Cenobites don't mind, though
    if (has_trait("CENOBITE")) {
        eff_morale = eff_morale + pain;
    }
    int focus_gain_rate = 100;

    if (activity.type == ACT_READ) {
        item &book = i_at(activity.position);
        if( book.is_book() ) {
            auto &bt = *book.type->book;
            // apply a penalty when we're actually learning something
            if( skillLevel( bt.skill ) < bt.level ) {
                focus_gain_rate -= 50;
            }
        } else {
            activity.type = ACT_NULL;
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
    w_point weather = g->weatherGen.get_weather( pos(), calendar::turn );
    int vpart = -1;
    vehicle *veh = g->m.veh_at( posx, posy, vpart );
    int vehwindspeed = 0;
    if( veh ) {
        vehwindspeed = abs(veh->velocity / 100); // vehicle velocity in mph
    }
    const oter_id &cur_om_ter = overmap_buffer.ter(g->om_global_location());
    std::string omtername = otermap[cur_om_ter].name;
    bool sheltered = g->is_sheltered(posx, posy);
    int total_windpower = get_local_windpower(weather.windpower + vehwindspeed, omtername, sheltered);
    // Temperature norms
    // Ambient normal temperature is lower while asleep
    int ambient_norm = (has_effect("sleep") ? 3100 : 1900);
    // This gets incremented in the for loop and used in the morale calculation
    int morale_pen = 0;
    const trap_id trap_at_pos = g->m.tr_at(posx, posy);
    const ter_id ter_at_pos = g->m.ter(posx, posy);
    const furn_id furn_at_pos = g->m.furn(posx, posy);
    // When the player is sleeping, he will use floor items for warmth
    int floor_item_warmth = 0;
    // When the player is sleeping, he will use floor bedding for warmth
    int floor_bedding_warmth = 0;
    // If the PC has fur, etc, that'll apply too
    int floor_mut_warmth = 0;
    if( in_sleep_state() ) {
        // Search the floor for items
        auto floor_item = g->m.i_at(posx, posy);

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

        // Search the floor for bedding
        if( furn_at_pos == f_bed ) {
            floor_bedding_warmth += 1000;
        } else if( furn_at_pos == f_makeshift_bed || furn_at_pos == f_armchair ||
                   furn_at_pos == f_sofa ) {
            floor_bedding_warmth += 500;
        } else if( furn_at_pos == f_straw_bed ) {
            floor_bedding_warmth += 200;
        } else if( trap_at_pos == tr_cot || ter_at_pos == t_improvised_shelter ||
                   furn_at_pos == f_tatami ) {
            floor_bedding_warmth -= 500;
        } else if( trap_at_pos == tr_rollmat ) {
            floor_bedding_warmth -= 1000;
        } else if( trap_at_pos == tr_fur_rollmat || furn_at_pos == f_hay ) {
            floor_bedding_warmth += 0;
        } else if( veh && veh->part_with_feature (vpart, "SEAT") >= 0 ) {
            floor_bedding_warmth += 200;
        } else if( veh && veh->part_with_feature (vpart, "BED") >= 0 ) {
            floor_bedding_warmth += 300;
        } else {
            floor_bedding_warmth -= 2000;
        }
        // Fur, etc effects for sleeping here.
        // Full-power fur is about as effective as a makeshift bed
        if (has_trait("FUR") || has_trait("LUPINE_FUR") || has_trait("URSINE_FUR")) {
            floor_mut_warmth += 500;
        }
        // Feline fur, not quite as warm.  Cats do better in warmer spots.
        if (has_trait("FELINE_FUR")) {
            floor_mut_warmth += 300;
        }
        // Light fur's better than nothing!
        if (has_trait("LIGHTFUR")) {
            floor_mut_warmth += 100;
        }
        // Spider hair really isn't meant for this sort of thing
        if (has_trait("CHITIN_FUR")) {
            floor_mut_warmth += 50;
        }
        if (has_trait("CHITIN_FUR2") || has_trait("CHITIN_FUR3")) {
            floor_mut_warmth += 75;
        }
        // Down helps too
        if (has_trait("DOWN")) {
            floor_mut_warmth += 250;
        }
        // Curl up in your shell to conserve heat & stay warm
        if (has_active_mutation("SHELL2")) {
            floor_mut_warmth += 200;
        }
        // DOWN doesn't provide floor insulation, though.
        // Better-than-light fur or being in one's shell does.
        if ( (!(has_trait("DOWN"))) && (floor_mut_warmth >= 200)) {
            if (floor_bedding_warmth < 0) {
                floor_bedding_warmth = 0;
            }
        }
    }
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
        double scaled_temperature = logistic_range( BODYTEMP_VERY_COLD, BODYTEMP_VERY_HOT,
                                                    temp_cur[i] );
        // Produces a smooth curve between 30.0 and 60.0.
        float homeostasis_adjustement = 30.0 * (1.0 + scaled_temperature);
        int clothing_warmth_adjustement = homeostasis_adjustement * warmth(body_part(i));
        // WINDCHILL

        bp_windpower = (float)bp_windpower * (1 - get_wind_resistance(body_part(i)) / 100.0);
        // Calculate windchill
        int windchill = get_local_windchill( g->get_temperature(),
                                             get_local_humidity(weather.humidity, g->weather,
                                                     sheltered),
                                             bp_windpower );
        // If you're standing in water, air temperature is replaced by water temperature. No wind.
        // Convert to C.
        int water_temperature = 100 * (g->weatherGen.get_water_temperature() - 32) * 5 / 9;
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

        // Convergeant temperature is affected by ambient temperature,
        // clothing warmth, and body wetness.
        temp_conv[i] = BODYTEMP_NORM + adjusted_temp + windchill * 100 + clothing_warmth_adjustement;
        // HUNGER
        temp_conv[i] -= hunger / 6 + 100;
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
                int heat_intensity = 0;

                int ffire = g->m.get_field_strength( point(posx + j, posy + k), fd_fire );
                if(ffire > 0) {
                    heat_intensity = ffire;
                } else if (g->m.tr_at(posx + j, posy + k) == tr_lava ) {
                    heat_intensity = 3;
                }
                int t;
                if( heat_intensity > 0 && g->m.sees( posx, posy, posx + j, posy + k, -1, t ) ) {
                    // Ensure fire_dist >= 1 to avoid divide-by-zero errors.
                    int fire_dist = std::max(1, std::max( std::abs( j ), std::abs( k ) ) );
                    if (frostbite_timer[i] > 0) {
                        frostbite_timer[i] -= heat_intensity - fire_dist / 2;
                    }
                    temp_conv[i] +=  300 * heat_intensity * heat_intensity / (fire_dist * fire_dist);
                    blister_count += heat_intensity / (fire_dist * fire_dist);
                    if( fire_dist <= 1 ) {
                        // Extend limbs/lean over a single adjacent fire to warm up
                        best_fire = std::max( best_fire, heat_intensity );
                    }
                }
            }
        }
        // TILES
        int tile_strength = 0;
        // Being on fire increases very intensely the convergent temperature.
        if (has_effect("onfire")) {
            temp_conv[i] += 15000;
        }
        // Same with standing on fire.
        tile_strength = g->m.get_field_strength(point(posx, posy), fd_fire);
        if (tile_strength > 2 || trap_at_pos == tr_lava) {
            temp_conv[i] += 15000;
        }
        // Standing in the hot air of a fire is nice.
        tile_strength = g->m.get_field_strength(point(posx, posy), fd_hot_air1);
        switch (tile_strength) {
        case 3:
            temp_conv[i] +=  500;
            break;
        case 2:
            temp_conv[i] +=  300;
            break;
        case 1:
            temp_conv[i] +=  100;
            break;
        default:
            break;
        }
        tile_strength = g->m.get_field_strength(point(posx, posy), fd_hot_air2);
        switch (tile_strength) {
        case 3:
            temp_conv[i] += 1000;
            break;
        case 2:
            temp_conv[i] +=  800;
            break;
        case 1:
            temp_conv[i] +=  300;
            break;
        default:
            break;
        }
        tile_strength = g->m.get_field_strength(point(posx, posy), fd_hot_air3);
        switch (tile_strength) {
        case 3:
            temp_conv[i] += 3500;
            break;
        case 2:
            temp_conv[i] += 2000;
            break;
        case 1:
            temp_conv[i] +=  800;
            break;
        default:
            break;
        }
        tile_strength = g->m.get_field_strength(point(posx, posy), fd_hot_air4);
        switch (tile_strength) {
        case 3:
            temp_conv[i] += 8000;
            break;
        case 2:
            temp_conv[i] += 5000;
            break;
        case 1:
            temp_conv[i] += 3500;
            break;
        default:
            break;
        }
        // WEATHER
        if( g->weather == WEATHER_SUNNY && g->is_in_sunlight(posx, posy) ) {
            temp_conv[i] += 1000;
        }
        if( g->weather == WEATHER_CLEAR && g->is_in_sunlight(posx, posy) ) {
            temp_conv[i] += 500;
        }
        // DISEASES
        if( has_effect("flu") && i == bp_head ) {
            temp_conv[i] += 1500;
        }
        if( has_effect("common_cold") ) {
            temp_conv[i] -= 750;
        }
        // BIONICS
        // Bionic "Internal Climate Control" says it eases the effects of high and low ambient temps
        const int variation = BODYTEMP_NORM * 0.5;
        if( in_climate_control() && temp_conv[i] < BODYTEMP_SCORCHING + variation &&
            temp_conv[i] > BODYTEMP_FREEZING - variation ) {
            if( temp_conv[i] > BODYTEMP_SCORCHING ) {
                temp_conv[i] = BODYTEMP_VERY_HOT;
            } else if( temp_conv[i] > BODYTEMP_VERY_HOT ) {
                temp_conv[i] = BODYTEMP_HOT;
            } else if( temp_conv[i] > BODYTEMP_HOT ) {
                temp_conv[i] = BODYTEMP_NORM;
            } else if( temp_conv[i] < BODYTEMP_FREEZING ) {
                temp_conv[i] = BODYTEMP_VERY_COLD;
            } else if( temp_conv[i] < BODYTEMP_VERY_COLD) {
                temp_conv[i] = BODYTEMP_COLD;
            } else if( temp_conv[i] < BODYTEMP_COLD ) {
                temp_conv[i] = BODYTEMP_NORM;
            }
        }
        // Bionic "Thermal Dissipation" says it prevents fire damage up to 2000F.
        // 500 is picked at random...
        if( has_bionic("bio_heatsink") || is_wearing("rm13_armor_on")) {
            blister_count -= 500;
        }
        // BLISTERS : Skin gets blisters from intense heat exposure.
        if( blister_count - 10 * get_env_resist(body_part(i)) > 20 ) {
            add_effect("blisters", 1, (body_part)i);
        }
        // BLOOD LOSS : Loss of blood results in loss of body heat
        int blood_loss = 0;
        if( i == bp_leg_l || i == bp_leg_r ) {
            blood_loss = (100 - 100 * (hp_cur[hp_leg_l] + hp_cur[hp_leg_r]) /
                          (hp_max[hp_leg_l] + hp_max[hp_leg_r]));
        } else if( i == bp_arm_l || i == bp_arm_r ) {
            blood_loss = (100 - 100 * (hp_cur[hp_arm_l] + hp_cur[hp_arm_r]) /
                          (hp_max[hp_arm_l] + hp_max[hp_arm_r]));
        } else if( i == bp_torso ) {
            blood_loss = (100 - 100 * hp_cur[hp_torso] / hp_max[hp_torso]);
        } else if( i == bp_head ) {
            blood_loss = (100 - 100 * hp_cur[hp_head] / hp_max[hp_head]);
        }
        temp_conv[i] -= blood_loss * temp_conv[i] / 200; // 1% bodyheat lost per 2% hp lost
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
        // MUTATIONS and TRAITS
        // Lightly furred
        if( has_trait("LIGHTFUR") ) {
            temp_conv[i] += (temp_cur[i] > BODYTEMP_NORM ? 250 : 500);
        }
        // Furry or Lupine/Ursine Fur
        if( has_trait("FUR") || has_trait("LUPINE_FUR") || has_trait("URSINE_FUR") ) {
            temp_conv[i] += (temp_cur[i] > BODYTEMP_NORM ? 750 : 1500);
        }
        // Feline fur
        if( has_trait("FELINE_FUR") ) {
            temp_conv[i] += (temp_cur[i] > BODYTEMP_NORM ? 500 : 1000);
        }
        // Feathers: minor means minor.
        if( has_trait("FEATHERS") ) {
            temp_conv[i] += (temp_cur[i] > BODYTEMP_NORM ? 50 : 100);
        }
        if( has_trait("CHITIN_FUR") ) {
            temp_conv[i] += (temp_cur[i] > BODYTEMP_NORM ? 100 : 150);
        }
        if( has_trait("CHITIN_FUR2") || has_trait ("CHITIN_FUR3") ) {
            temp_conv[i] += (temp_cur[i] > BODYTEMP_NORM ? 150 : 250);
        }
        // Down; lets heat out more easily if needed but not as Warm
        // as full-blown fur.  So less miserable in Summer.
        if( has_trait("DOWN") ) {
            temp_conv[i] += (temp_cur[i] > BODYTEMP_NORM ? 300 : 800);
        }
        // Fat deposits don't hold in much heat, but don't shift for temp
        if( has_trait("FAT") ) {
            temp_conv[i] += (temp_cur[i] > BODYTEMP_NORM ? 200 : 200);
        }
        // Being in the shell holds in heat, but lets out less in summer :-/
        if( has_active_mutation("SHELL2") ) {
            temp_conv[i] += (temp_cur[i] > BODYTEMP_NORM ? 500 : 750);
        }
        // Disintegration
        if (has_trait("ROT1")) {
            temp_conv[i] -= 250;
        } else if (has_trait("ROT2")) {
            temp_conv[i] -= 750;
        } else if (has_trait("ROT3")) {
            temp_conv[i] -= 1500;
        }
        // Radioactive
        if (has_trait("RADIOACTIVE1")) {
            temp_conv[i] += 250;
        } else if (has_trait("RADIOACTIVE2")) {
            temp_conv[i] += 750;
        } else if (has_trait("RADIOACTIVE3")) {
            temp_conv[i] += 1500;
        }
        // Chemical Imbalance
        // Added line in player::suffer()
        // FINAL CALCULATION : Increments current body temperature towards convergent.
        int bonus_warmth = 0;
        if ( in_sleep_state() ) {
            bonus_warmth = floor_bedding_warmth + floor_item_warmth + floor_mut_warmth;
        } else if ( best_fire > 0 ) {
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
                disease_intensity( "cold", false, (body_part)num_bp ) == 0 &&
                disease_intensity( "hot", false, (body_part)num_bp ) == 0 &&
                temp_cur[i] > BODYTEMP_COLD && temp_cur[i] <= BODYTEMP_NORM ) {
                add_morale( MORALE_COMFY, 1, 5, 20, 10, true );
            }
        }

        int temp_before = temp_cur[i];
        int temp_difference = temp_cur[i] - temp_conv[i]; // Negative if the player is warming up.
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
            int wetness_percentage = 100 * body_wetness[i] / mDrenchEffect.at(i); // 0 - 100
            // Warmth gives a slight buff to temperature resistance
            // Wetness gives a heavy nerf to tempearture resistance
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
    if( morale_pen < 0 && int(calendar::turn) % 10 == 0 ) {
        add_morale(MORALE_COLD, -2, -abs(morale_pen), 10, 5, true);
    }
    if( morale_pen > 0 && int(calendar::turn) % 10 == 0 ) {
        add_morale(MORALE_HOT,  -2, -abs(morale_pen), 10, 5, true);
    }
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
    if (hunger > 100) {
        mod_speed_bonus(-int((hunger - 100) / 10));
    }

    mod_speed_bonus(stim > 40 ? 40 : stim);

    for (auto maps : effects) {
        for (auto i : maps.second) {
            bool reduced = has_trait(i.second.get_resist_trait()) ||
                            has_effect(i.second.get_resist_effect());
            mod_speed_bonus(i.second.get_mod("SPEED", reduced));
        }
    }

    // add martial arts speed bonus
    mod_speed_bonus(mabuff_speed_bonus());

    // Not sure why Sunlight Dependent is here, but OK
    // Ectothermic/COLDBLOOD4 is intended to buff folks in the Summer
    // Threshold-crossing has its charms ;-)
    if (g != NULL) {
        if (has_trait("SUNLIGHT_DEPENDENT") && !g->is_in_sunlight(posx, posy)) {
            mod_speed_bonus(-(g->light_level() >= 12 ? 5 : 10));
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

int player::run_cost(int base_cost, bool diag)
{
    float movecost = float(base_cost);
    if (diag)
        movecost *= 0.7071f; // because everything here assumes 100 is base
    bool flatground = movecost < 105;
    const ter_id ter_at_pos = g->m.ter(posx, posy);
    // If your floor is hard, flat, and otherwise skateable, list it here
    // The "FLAT" tag includes soft surfaces, so not a good fit.
    bool offroading = ( flatground && (!((ter_at_pos == t_rock_floor) ||
      (ter_at_pos == t_pit_covered) || (ter_at_pos == t_metal_floor) ||
      (ter_at_pos == t_pit_spiked_covered) || (ter_at_pos == t_pavement) ||
      (ter_at_pos == t_pavement_y) || (ter_at_pos == t_sidewalk) ||
      (ter_at_pos == t_concrete) || (ter_at_pos == t_floor) ||
      (ter_at_pos == t_door_glass_o) || (ter_at_pos == t_utility_light) ||
      (ter_at_pos == t_door_o) || (ter_at_pos == t_rdoor_o) ||
      (ter_at_pos == t_door_frame) || (ter_at_pos == t_mdoor_frame) ||
      (ter_at_pos == t_fencegate_o) || (ter_at_pos == t_chaingate_o) ||
      (ter_at_pos == t_door_metal_o) || (ter_at_pos == t_door_bar_o) ||
      (ter_at_pos == t_pit_glass_covered) || (ter_at_pos == t_sidewalk_bg_dp) ||
      (ter_at_pos == t_pavement_bg_dp) || (ter_at_pos == t_pavement_y_bg_dp) ||
      (ter_at_pos == t_linoleum_white) || (ter_at_pos == t_linoleum_gray))) );

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
    }
    else if (hp_cur[hp_leg_l] < hp_max[hp_leg_l] * .40) {
        movecost += 25;
    }
    if (hp_cur[hp_leg_r] == 0) {
        movecost += 50;
    }
    else if (hp_cur[hp_leg_r] < hp_max[hp_leg_r] * .40) {
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
    if (is_wearing("swim_fins")) {
            movecost *= 1.0f + (0.25f * shoe_type_count("swim_fins"));
    }
    if ( (is_wearing("roller_blades")) && !(is_on_ground())) {
        if (offroading) {
            movecost *= 1.0f + (0.25f * shoe_type_count("roller_blades"));
        } else if (flatground) {
            movecost *= 1.0f - (0.25f * shoe_type_count("roller_blades"));
        } else {
            movecost *= 1.5f;
        }
    }
    // Quad skates might be more stable than inlines,
    // but that also translates into a slower speed when on good surfaces.
    if ( (is_wearing("rollerskates")) && !(is_on_ground())) {
        if (offroading) {
            movecost *= 1.0f + (0.15f * shoe_type_count("rollerskates"));
        } else if (flatground) {
            movecost *= 1.0f - (0.15f * shoe_type_count("rollerskates"));
        } else {
            movecost *= 1.3f;
        }
    }

    movecost += encumb(bp_mouth) * 5 + (encumb(bp_foot_l) + encumb(bp_foot_r)) * 2.5 + (encumb(bp_leg_l) + encumb(bp_leg_r)) * 1.5;

    // ROOTS3 does slow you down as your roots are probing around for nutrients,
    // whether you want them to or not.  ROOTS1 is just too squiggly without shoes
    // to give you some stability.  Plants are a bit of a slow-mover.  Deal.
    if (!is_wearing_shoes("left") && !has_trait("PADDED_FEET") && !has_trait("HOOVES") &&
        !has_trait("TOUGH_FEET") && !has_trait("ROOTS2") ) {
        movecost += 8;
    }
    if (!is_wearing_shoes("right") && !has_trait("PADDED_FEET") && !has_trait("HOOVES") &&
        !has_trait("TOUGH_FEET") && !has_trait("ROOTS2") ) {
        movecost += 8;
    }

    if( !footwear_factor() && has_trait("ROOTS3") &&
        g->m.has_flag("DIGGABLE", posx, posy) ) {
        movecost += 10 * footwear_factor();
    }

    if (diag) {
        movecost *= 1.4142;
    }

    return int(movecost);
}

int player::swim_speed()
{
    int ret = 440 + weight_carried() / 60 - 50 * skillLevel("swimming");
    if (has_trait("PAWS")) {
        ret -= 20 + str_cur * 3;
    }
    if (has_trait("PAWS_LARGE")) {
        ret -= 20 + str_cur * 4;
    }
    if (is_wearing("swim_fins")) {
        ret -= (15 * str_cur) / (3 - shoe_type_count("swim_fins"));
    }
    if (has_trait("WEBBED")) {
        ret -= 60 + str_cur * 5;
    }
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
    ret += (50 - skillLevel("swimming") * 2) * (encumb(bp_leg_l) + encumb(bp_leg_r));
    ret += (80 - skillLevel("swimming") * 3) * encumb(bp_torso);
    if (skillLevel("swimming") < 10) {
        for (auto &i : worn) {
            ret += (i.volume() * (10 - skillLevel("swimming"))) / 2;
        }
    }
    ret -= str_cur * 6 + dex_cur * 4;
    if( worn_with_flag("FLOATATION") ) {
        ret = std::max(ret, 400);
        ret = std::min(ret, 200);
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

bool player::digging() const {
    return false;
}

bool player::is_on_ground() const
{
    bool on_ground = false;
    if(has_effect("downed") || hp_cur[hp_leg_l] == 0 || hp_cur[hp_leg_r] == 0 ){
        on_ground = true;
    }
    return  on_ground;
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

    char check = dump.peek();
    if ( check == ' ' ) {
        // sigh..
        check = data[1];
    }
    if ( check == '{' ) {
        JsonIn jsin(dump);
        try {
            deserialize(jsin);
        } catch (std::string jsonerr) {
            debugmsg("Bad player json\n%s", jsonerr.c_str() );
        }
        return;
    } else { // old save
        load_legacy(dump);
    }
}

std::string player::save_info()
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
    const oter_id &cur_ter = overmap_buffer.ter(g->om_global_location());
    point cur_loc = g->om_location();
    std::string tername = otermap[cur_ter].name;

    //Were they in a town, or out in the wilderness?
    int city_index = g->cur_om->closest_city(cur_loc);
    std::string kill_place;
    if(city_index < 0) {
        //~ First parameter is a pronoun (“He”/“She”), second parameter is a terrain name.
        kill_place = string_format(_("%s was killed in a %s in the middle of nowhere."),
                     pronoun.c_str(), tername.c_str());
    } else {
        city nearest_city = g->cur_om->cities[city_index];
        //Give slightly different messages based on how far we are from the middle
        int distance_from_city = abs(g->cur_om->dist_from_city(cur_loc));
        if(distance_from_city > nearest_city.s + 4) {
            //~ First parameter is a pronoun (“He”/“She”), second parameter is a terrain name.
            kill_place = string_format(_("%s was killed in a %s in the wilderness."),
                         pronoun.c_str(), tername.c_str());

        } else if(distance_from_city >= nearest_city.s) {
            //~ First parameter is a pronoun (“He”/“She”), second parameter is a terrain name, third parameter is a city name.
            kill_place = string_format(_("%s was killed in a %s on the outskirts of %s."),
                         pronoun.c_str(), tername.c_str(), nearest_city.name.c_str());
        } else {
            //~ First parameter is a pronoun (“He”/“She”), second parameter is a terrain name, third parameter is a city name.
            kill_place = string_format(_("%s was killed in a %s in %s."),
                         pronoun.c_str(), tername.c_str(), nearest_city.name.c_str());
        }
    }

    //Header
    std::string version = string_format("%s", getVersionString());
    memorial_file << string_format(_("Cataclysm - Dark Days Ahead version %s memorial file"), version.c_str()) << "\n";
    memorial_file << "\n";
    memorial_file << string_format(_("In memory of: %s"), name.c_str()) << "\n";
    if(epitaph.length() > 0) { //Don't record empty epitaphs
        //~ The “%s” will be replaced by an epitaph as displyed in the memorial files. Replace the quotation marks as appropriate for your language.
        memorial_file << string_format(pgettext("epitaph","\"%s\""), epitaph.c_str()) << "\n\n";
    }
    //~ First parameter: Pronoun, second parameter: a profession name (with article)
    memorial_file << string_format("%s was %s when the apocalypse began.",
                                   pronoun.c_str(), profession_name.c_str()) << "\n";
    memorial_file << string_format("%s died on %s of year %d, day %d, at %s.",
                     pronoun.c_str(), season_name_uc[calendar::turn.get_season()].c_str(), (calendar::turn.years() + 1),
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

    const std::map<std::string, mtype*> monids = MonsterGenerator::generator().get_all_mtypes();
    for( const auto &monid : monids ) {
        if( g->kill_count( monid.first ) > 0 ) {
            memorial_file << "  " << monid.second->sym << " - "
                          << string_format( "%4d", g->kill_count( monid.first ) ) << " "
                          << monid.second->nname( g->kill_count( monid.first ) ) << "\n";
            total_kills += g->kill_count( monid.first );
        }
    }
    if(total_kills == 0) {
      memorial_file << indent << _("No monsters were killed.") << "\n";
    } else {
      memorial_file << string_format(_("Total kills: %d"), total_kills) << "\n";
    }
    memorial_file << "\n";

    //Skills
    memorial_file << _("Skills:") << "\n";
    for( auto &skill : Skill::skills ) {
        SkillLevel next_skill_level = skillLevel( skill );
        memorial_file << indent << ( skill )->name() << ": " << next_skill_level.level() << " ("
                      << next_skill_level.exercise() << "%)\n";
    }
    memorial_file << "\n";

    //Traits
    memorial_file << _("Traits:") << "\n";
    bool had_trait = false;
    for (std::map<std::string, trait>::iterator iter = traits.begin(); iter != traits.end(); ++iter) {
      if(has_trait(iter->first)) {
        had_trait = true;
        memorial_file << indent << traits[iter->first].name << "\n";
      }
    }
    if(!had_trait) {
      memorial_file << indent << _("(None)") << "\n";
    }
    memorial_file << "\n";

    //Effects (illnesses)
    memorial_file << _("Ongoing Effects:") << "\n";
    bool had_effect = false;
    for (auto &next_illness : illness) {
      if(dis_name(next_illness).size() > 0) {
        had_effect = true;
        memorial_file << indent << dis_name(next_illness) << "\n";
      }
    }
    //Various effects not covered by the illness list - from player.cpp
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
      bionic_id next_bionic_id = my_bionics[i].id;
      memorial_file << indent << (i+1) << ": " << bionics[next_bionic_id]->name << "\n";
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
    memorial_file << indent << weapon.invlet << " - " << weapon.tname() << "\n";
    memorial_file << "\n";

    memorial_file << _("Equipment:") << "\n";
    for( auto &elem : worn ) {
        item next_item = elem;
      memorial_file << indent << next_item.invlet << " - " << next_item.tname();
      if(next_item.charges > 0) {
        memorial_file << " (" << next_item.charges << ")";
      } else if (next_item.contents.size() == 1
              && next_item.contents[0].charges > 0) {
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
      memorial_file << indent << next_item.invlet << " - " << next_item.tname();
      if( elem->size() > 1 ) {
          memorial_file << " [" << elem->size() << "]";
      }
      if(next_item.charges > 0) {
        memorial_file << " (" << next_item.charges << ")";
      } else if (next_item.contents.size() == 1
              && next_item.contents[0].charges > 0) {
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
                               season_name_uc[calendar::turn.get_season()].c_str(),
                               calendar::turn.days() + 1, calendar::turn.print_time().c_str()
                               );

    const oter_id &cur_ter = overmap_buffer.ter(g->om_global_location());
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
std::string player::dump_memorial()
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

void player::mod_stat( std::string stat, int modifier )
{
    if( stat == "hunger" ) {
        hunger += modifier;
    } else if( stat == "thirst" ) {
        thirst += modifier;
    } else if( stat == "fatigue" ) {
        fatigue += modifier;
    } else if( stat == "oxygen" ) {
        oxygen += modifier;
    } else {
        // Fall through to the creature method.
        Creature::mod_stat( stat, modifier );
    }
}

inline bool skill_display_sort(const std::pair<const Skill*, int> &a, const std::pair<const Skill*, int> &b)
{
    int levelA = a.second;
    int levelB = b.second;
    return levelA > levelB || (levelA == levelB && a.first->name() < b.first->name());
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
    for (auto &next_illness : illness) {
        if (dis_name(next_illness).size() > 0) {
            effect_name.push_back(dis_name(next_illness));
            effect_text.push_back(dis_description(next_illness));
        }
    }
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
        stim_text << _("Speed") << " " << stim << "   " << _("Intelligence") << " " << intpen <<
            "   " << _("Perception") << " " << perpen << "   " << _("Dexterity") << " " << dexpen;
        effect_text.push_back(stim_text.str());
    }

    if ((has_trait("TROGLO") && g->is_in_sunlight(posx, posy) &&
         g->weather == WEATHER_SUNNY) ||
        (has_trait("TROGLO2") && g->is_in_sunlight(posx, posy) &&
         g->weather != WEATHER_SUNNY)) {
        effect_name.push_back(_("In Sunlight"));
        effect_text.push_back(_("The sunlight irritates you.\n\
Strength - 1;    Dexterity - 1;    Intelligence - 1;    Perception - 1"));
    } else if (has_trait("TROGLO2") && g->is_in_sunlight(posx, posy)) {
        effect_name.push_back(_("In Sunlight"));
        effect_text.push_back(_("The sunlight irritates you badly.\n\
Strength - 2;    Dexterity - 2;    Intelligence - 2;    Perception - 2"));
    } else if (has_trait("TROGLO3") && g->is_in_sunlight(posx, posy)) {
        effect_name.push_back(_("In Sunlight"));
        effect_text.push_back(_("The sunlight irritates you terribly.\n\
Strength - 4;    Dexterity - 4;    Intelligence - 4;    Perception - 4"));
    }

    for( auto &elem : addictions ) {
        if( elem.sated < 0 && elem.intensity >= MIN_ADDICTION_LEVEL ) {
            effect_name.push_back( addiction_name( elem ) );
            effect_text.push_back( addiction_text( elem ) );
        }
    }

    unsigned maxy = unsigned(TERMY);

    unsigned infooffsetytop = 11;
    unsigned infooffsetybottom = 15;
    std::vector<std::string> traitslist;

    for( const auto &elem : my_mutations ) {
        traitslist.push_back( elem );
    }

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
        std::vector<std::string> traitslist;
        std::string race;
        for( auto &mut : my_mutations ) {
            traitslist.push_back( mut );
            for( auto &elem : traitslist ) {
                if( mutation_data[elem].threshold ) {
                    race = traits[elem].name;
                    break;
                }
            }
            if( !race.empty() ) {
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
    mvwprintz(w_tip, 0, FULL_SCREEN_WIDTH - utf8_width(help_msg.c_str()), c_ltred, help_msg.c_str());
    help_msg.clear();
    wrefresh(w_tip);

    // First!  Default STATS screen.
    const char* title_STATS = _("STATS");
    mvwprintz(w_stats, 0, 13 - utf8_width(title_STATS)/2, c_ltgray, title_STATS);
    mvwprintz(w_stats, 2, 1, c_ltgray, "                     ");
    mvwprintz(w_stats, 2, 1, c_ltgray, _("Strength:"));
    mvwprintz(w_stats, 2, 20, c_ltgray, str_max>9?"(%d)":" (%d)", str_max);
    mvwprintz(w_stats, 3, 1, c_ltgray, "                     ");
    mvwprintz(w_stats, 3, 1, c_ltgray, _("Dexterity:"));
    mvwprintz(w_stats, 3, 20, c_ltgray, dex_max>9?"(%d)":" (%d)", dex_max);
    mvwprintz(w_stats, 4, 1, c_ltgray, "                     ");
    mvwprintz(w_stats, 4, 1, c_ltgray, _("Intelligence:"));
    mvwprintz(w_stats, 4, 20, c_ltgray, int_max>9?"(%d)":" (%d)", int_max);
    mvwprintz(w_stats, 5, 1, c_ltgray, "                     ");
    mvwprintz(w_stats, 5, 1, c_ltgray, _("Perception:"));
    mvwprintz(w_stats, 5, 20, c_ltgray, per_max>9?"(%d)":" (%d)", per_max);

    nc_color status = c_white;

    int stat_tmp = get_str();
    if (stat_tmp <= 0)
        status = c_dkgray;
    else if (stat_tmp < str_max / 2)
        status = c_red;
    else if (stat_tmp < str_max)
        status = c_ltred;
    else if (stat_tmp == str_max)
        status = c_white;
    else if (stat_tmp < str_max * 1.5)
        status = c_ltgreen;
    else
        status = c_green;
    mvwprintz(w_stats,  2, (stat_tmp < 10 ? 17 : 16), status, "%d", stat_tmp);

    stat_tmp = get_dex();
    if (stat_tmp <= 0)
        status = c_dkgray;
    else if (stat_tmp < dex_max / 2)
        status = c_red;
    else if (stat_tmp < dex_max)
        status = c_ltred;
    else if (stat_tmp == dex_max)
        status = c_white;
    else if (stat_tmp < dex_max * 1.5)
        status = c_ltgreen;
    else
        status = c_green;
    mvwprintz(w_stats,  3, (stat_tmp < 10 ? 17 : 16), status, "%d", stat_tmp);

    stat_tmp = get_int();
    if (stat_tmp <= 0)
        status = c_dkgray;
    else if (stat_tmp < int_max / 2)
        status = c_red;
    else if (stat_tmp < int_max)
        status = c_ltred;
    else if (stat_tmp == int_max)
        status = c_white;
    else if (stat_tmp < int_max * 1.5)
        status = c_ltgreen;
    else
        status = c_green;
    mvwprintz(w_stats,  4, (stat_tmp < 10 ? 17 : 16), status, "%d", stat_tmp);

    stat_tmp = get_per();
    if (stat_tmp <= 0)
        status = c_dkgray;
    else if (stat_tmp < per_max / 2)
        status = c_red;
    else if (stat_tmp < per_max)
        status = c_ltred;
    else if (stat_tmp == per_max)
        status = c_white;
    else if (stat_tmp < per_max * 1.5)
        status = c_ltgreen;
    else
        status = c_green;
    mvwprintz(w_stats,  5, (stat_tmp < 10 ? 17 : 16), status, "%d", stat_tmp);

    wrefresh(w_stats);

    // Next, draw encumberment.
    std::string asText[] = {_("Torso"), _("Head"), _("Eyes"), _("Mouth"), _("L. Arm"), _("R. Arm"),
                             _("L. Hand"), _("R. Hand"), _("L. Leg"), _("R. Leg"), _("L. Foot"),
                             _("R. Foot")};
    body_part aBodyPart[] = {bp_torso, bp_head, bp_eyes, bp_mouth, bp_arm_l, bp_arm_r, bp_hand_l,
                             bp_hand_r, bp_leg_l, bp_leg_r, bp_foot_l, bp_foot_r};
    int iEnc, iArmorEnc, iBodyTempInt;
    double iLayers;

    const char *title_ENCUMB = _("ENCUMBRANCE AND WARMTH");
    mvwprintz(w_encumb, 0, 13 - utf8_width(title_ENCUMB) / 2, c_ltgray, title_ENCUMB);
    for (int i = 0; i < 8; i++) {
        iLayers = iArmorEnc = 0;
        iBodyTempInt = (temp_conv[i] / 100.0) * 2 - 100; // Scale of -100 to +100
        iEnc = encumb(aBodyPart[i], iLayers, iArmorEnc);
        mvwprintz(w_encumb, i + 1, 1, c_ltgray, "%s", asText[i].c_str());
        mvwprintz(w_encumb, i + 1, 8, c_ltgray, "(%d)", static_cast<int>( iLayers ) );
        mvwprintz(w_encumb, i + 1, 11, c_ltgray, "%*s%d%s%d=", (iArmorEnc < 0 || iArmorEnc > 9 ? 1 : 2),
                  " ", iArmorEnc, "+", iEnc - iArmorEnc);
        wprintz(w_encumb, encumb_color(iEnc), "%s%d", (iEnc < 0 || iEnc > 9 ? "" : " ") , iEnc);
        wprintz(w_encumb, bodytemp_color(i), " (%3d)", iBodyTempInt);
    }
    wrefresh(w_encumb);

    // Next, draw traits.
    const char *title_TRAITS = _("TRAITS");
    mvwprintz(w_traits, 0, 13 - utf8_width(title_TRAITS)/2, c_ltgray, title_TRAITS);
    std::sort(traitslist.begin(), traitslist.end(), trait_display_sort);
    for (size_t i = 0; i < traitslist.size() && i < trait_win_size_y; i++) {
        if ( (mutation_data[traitslist[i]].threshold == true) ||
            (mutation_data[traitslist[i]].profession == true) ) {
            status = c_white;
        }
        else if (traits[traitslist[i]].mixed_effect == true) {
            status = c_pink;
        }
        else if (traits[traitslist[i]].points > 0) {
            status = c_ltgreen;
        }
        else if (traits[traitslist[i]].points < 0) {
            status = c_ltred;
        }
        else {
            status = c_yellow;
        }
        mvwprintz(w_traits, i+1, 1, status, traits[traitslist[i]].name.c_str());
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
    std::vector<const Skill*> skillslist;
    const char *title_SKILLS = _("SKILLS");
    mvwprintz(w_skills, 0, 13 - utf8_width(title_SKILLS)/2, c_ltgray, title_SKILLS);

    std::vector<std::pair<const Skill*, int> > sorted;
    int num_skills = Skill::skills.size();
    for (int i = 0; i < num_skills; i++) {
        const Skill* s = Skill::skills[i];
        SkillLevel &sl = skillLevel(s);
        sorted.push_back(std::pair<const Skill*, int>(s, sl.level() * 100 + sl.exercise()));
    }
    std::sort(sorted.begin(), sorted.end(), skill_display_sort);
    for( auto &elem : sorted ) {
        skillslist.push_back( ( elem ).first );
    }

    for( auto &elem : skillslist ) {
        SkillLevel level = skillLevel( elem );

        // Default to not training and not rusting
        nc_color text_color = c_blue;
        bool training = level.isTraining();
        bool rusting = level.isRusting();

        if(training && rusting) {
            text_color = c_ltred;
        } else if (training) {
            text_color = c_ltblue;
        } else if (rusting) {
            text_color = c_red;
        }

        int level_num = (int)level;
        int exercise = level.exercise();

        if( has_active_bionic( "bio_cqb" ) &&
            ( ( elem )->ident() == "melee" || ( elem )->ident() == "unarmed" ||
              ( elem )->ident() == "cutting" || ( elem )->ident() == "bashing" ||
              ( elem )->ident() == "stabbing" ) ) {
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
        pen = stim;
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
    if (hunger > 100) {
        pen = int((hunger - 100) / 10);
        mvwprintz(w_speed, line, 1, c_red, _("Hunger              -%s%d%%"),
                  (pen < 10 ? " " : ""), pen);
        line++;
    }
    if (has_trait("SUNLIGHT_DEPENDENT") && !g->is_in_sunlight(posx, posy)) {
        pen = (g->light_level() >= 12 ? 5 : 10);
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
            bool reduced = has_trait(it.get_resist_trait()) || has_effect(it.get_resist_effect());
            int move_adjust = it.get_mod("SPEED", reduced);
            if (move_adjust != 0) {
                dis_text = it.get_speed_name();
                speed_effects[dis_text] += move_adjust;
            }
        }
    }

    for( auto &speed_effect : speed_effects ) {
        nc_color col = ( speed_effect.second > 0 ? c_green : c_red );
        mvwprintz( w_speed, line, 1, col, "%s", speed_effect.first.c_str() );
        mvwprintz( w_speed, line, 21, col, ( speed_effect.second > 0 ? "+" : "-" ) );
        mvwprintz( w_speed, line, ( abs( speed_effect.second ) >= 10 ? 22 : 23 ), col, "%d%%",
                   abs( speed_effect.second ) );
        line++;
    }

    if (has_trait("QUICK")) {
        pen = int(newmoves * .1);
        mvwprintz(w_speed, line, 1, c_green, _("Quick               +%s%d%%"),
                  (pen < 10 ? " " : ""), pen);
    }
    if (has_bionic("bio_speed")) {
        pen = int(newmoves * .1);
        mvwprintz(w_speed, line, 1, c_green, _("Bionic Speed        +%s%d%%"),
                  (pen < 10 ? " " : ""), pen);
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
                mvwprintz(w_stats, 0, 0, h_ltgray, _("                          "));
                mvwprintz(w_stats, 0, 13 - utf8_width(title_STATS)/2, h_ltgray, title_STATS);
                if (line == 0) {
                    // display player current STR effects
                    mvwprintz(w_stats, 2, 1, h_ltgray, _("Strength:"));
                    mvwprintz(w_stats, 6, 1, c_magenta, _("Base HP: %d              "), hp_max[1]);
                    mvwprintz(w_stats, 7, 1, c_magenta, _("Carry weight: %.1f %s     "),
                              convert_weight(weight_capacity()),
                              OPTIONS["USE_METRIC_WEIGHTS"] == "kg"?_("kg"):_("lbs"));
                    mvwprintz(w_stats, 8, 1, c_magenta, _("Melee damage: %d         "),
                              base_damage(false));

                    fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta, _("\
Strength affects your melee damage, the amount of weight you can carry, your total HP, \
your resistance to many diseases, and the effectiveness of actions which require brute force."));
                } else if (line == 1) {
                    // display player current DEX effects
                    mvwprintz(w_stats, 3, 1, h_ltgray, _("Dexterity:"));
                    mvwprintz(w_stats, 6, 1, c_magenta, _("Melee to-hit bonus: +%d                      "),
                              base_to_hit(false));
                    mvwprintz(w_stats, 7, 1, c_magenta, "                                            ");
                    mvwprintz(w_stats, 7, 1, c_magenta, _("Ranged penalty: -%d"),
                              abs(ranged_dex_mod()));
                    mvwprintz(w_stats, 8, 1, c_magenta, "                                            ");
                    if (throw_dex_mod(false) <= 0) {
                        mvwprintz(w_stats, 8, 1, c_magenta, _("Throwing bonus: +%d"),
                        abs(throw_dex_mod(false)));
                    } else {
                        mvwprintz(w_stats, 8, 1, c_magenta, _("Throwing penalty: -%d"),
                        abs(throw_dex_mod(false)));
                    }
                    fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta, _("\
Dexterity affects your chance to hit in melee combat, helps you steady your \
gun for ranged combat, and enhances many actions that require finesse."));
                } else if (line == 2) {
                    // display player current INT effects
                    mvwprintz(w_stats, 4, 1, h_ltgray, _("Intelligence:"));
                    mvwprintz(w_stats, 6, 1, c_magenta, _("Read times: %d%%           "), read_speed(false));
                    mvwprintz(w_stats, 7, 1, c_magenta, _("Skill rust: %d%%           "), rust_rate(false));
                    mvwprintz(w_stats, 8, 1, c_magenta, _("Crafting Bonus: %d          "), get_int());

                    fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta, _("\
Intelligence is less important in most situations, but it is vital for more complex tasks like \
electronics crafting. It also affects how much skill you can pick up from reading a book."));
                } else if (line == 3) {
                    // display player current PER effects
                    mvwprintz(w_stats, 5, 1, h_ltgray, _("Perception:"));
                    mvwprintz(w_stats, 6, 1,  c_magenta, _("Ranged penalty: -%d"),
                              abs(ranged_per_mod()),"          ");
                    mvwprintz(w_stats, 7, 1, c_magenta, _("Trap detection level: %d       "), get_per());
                    mvwprintz(w_stats, 8, 1, c_magenta, "                             ");
                    fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta, _("\
Perception is the most important stat for ranged combat. It's also used for \
detecting traps and other things of interest."));
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
                    mvwprintz(w_stats, 0, 0, c_ltgray, _("                          "));
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
            mvwprintz(w_encumb, 0, 0, h_ltgray,  _("                          "));
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

            for (unsigned i = min; i < max; i++) {
                iLayers = iArmorEnc = 0;
                iBodyTempInt = (temp_conv[i] / 100.0) * 2 - 100; // Scale of -100 to +100
                iEnc = encumb(aBodyPart[i], iLayers, iArmorEnc);
                if (line == i) {
                    mvwprintz(w_encumb, i + 1 - min, 1, h_ltgray, "%s", asText[i].c_str());
                } else {
                    mvwprintz(w_encumb, i + 1 - min, 1, c_ltgray, "%s", asText[i].c_str());
                }
                mvwprintz(w_encumb, i + 1 - min, 8, c_ltgray, "(%d)", static_cast<int>( iLayers ) );
                mvwprintz(w_encumb, i + 1 - min, 11, c_ltgray, "%*s%d%s%d=", (iArmorEnc < 0 || iArmorEnc > 9 ? 1 : 2),
                          " ", iArmorEnc, "+", iEnc - iArmorEnc);
                wprintz(w_encumb, encumb_color(iEnc), "%s%d", (iEnc < 0 || iEnc > 9 ? "" : " ") , iEnc);
                wprintz(w_encumb, bodytemp_color(i), " (%3d)", iBodyTempInt);
            }
            draw_scrollbar(w_encumb, line, encumb_win_size_y, 12, 1);

            werase(w_info);
            std::string s;
            if (line == 0) {
                s += string_format( _("Melee skill %+d; "), -encumb( bp_torso ) );
                s += dodge_skill_text( -encumb( bp_torso ) );
                s += swim_cost_text( encumb( bp_torso ) * ( 80 - skillLevel( "swimming" ) * 3 ) );
                s += melee_cost_text( encumb( bp_torso ) * 20 );
            } else if (line == 1) { //Torso
                s += _("Head encumbrance has no effect; it simply limits how much you can put on.");
            } else if (line == 2) { //Head
                s += string_format( _("\
Perception %+d when checking traps or firing ranged weapons;\n\
Perception %+.1f when throwing items."),
                               -encumb(bp_eyes),
                               double(double(-encumb(bp_eyes)) / 2));
            } else if (line == 3) { //Eyes
                s += run_cost_text( encumb( bp_mouth ) * 5 );
            } else if (line == 4) { //Left Arm
                s += _("Arm encumbrance affects your accuracy with ranged weapons.");
            } else if (line == 5) { //Right Arm
                s += _("Arm encumbrance affects your accuracy with ranged weapons.");
            } else if (line == 6) { //Left Hand
                s += reload_cost_text( encumb( bp_hand_l ) * 15 );
                s += string_format( _("Dexterity %+d when throwing items."), -encumb( bp_hand_l ) );
            } else if (line == 7) { //Right Hand
                s += reload_cost_text( encumb( bp_hand_r ) * 15 );
                s += string_format( _("Dexterity %+d when throwing items."), -encumb( bp_hand_r ) );
            } else if (line == 8) { //Left Leg
                s += run_cost_text( encumb( bp_leg_l ) * 1.5 );
                s += swim_cost_text( encumb( bp_leg_l ) * ( 50 - skillLevel( "swimming" ) * 2 ) / 2 );
                s += dodge_skill_text( -encumb( bp_leg_l ) / 4.0 );
            } else if (line == 9) { //Right Leg
                s += run_cost_text( encumb( bp_leg_r ) * 1.5 );
                s += swim_cost_text( encumb( bp_leg_r ) * ( 50 - skillLevel( "swimming" ) * 2 ) / 2 );
                s += dodge_skill_text( -encumb( bp_leg_r ) / 4.0 );
            } else if (line == 10) { //Left Foot
                s += run_cost_text( encumb( bp_foot_l ) * 2.5 );
            } else if (line == 11) { //Right Foot
                s += run_cost_text( encumb( bp_foot_r ) * 2.5 );
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
                mvwprintz(w_encumb, 0, 0, c_ltgray,  _("                          "));
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
            mvwprintz(w_traits, 0, 0, h_ltgray,  _("                          "));
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
                mvwprintz(w_traits, 1 + i - min, 1, c_ltgray, "                         ");
                if (i > traits.size())
                    status = c_ltblue;
                else if ( (mutation_data[traitslist[i]].threshold == true) ||
                        (mutation_data[traitslist[i]].profession == true) ) {
                    status = c_white;
                }
                else if (traits[traitslist[i]].mixed_effect == true) {
                    status = c_pink;
                }
                else if (traits[traitslist[i]].points > 0) {
                    status = c_ltgreen;
                }
                else if (traits[traitslist[i]].points < 0) {
                    status = c_ltred;
                }
                else {
                    status = c_yellow;
                }
                if (i == line) {
                    mvwprintz(w_traits, 1 + i - min, 1, hilite(status), "%s",
                              traits[traitslist[i]].name.c_str());
                } else {
                    mvwprintz(w_traits, 1 + i - min, 1, status, "%s",
                              traits[traitslist[i]].name.c_str());
                }
            }
            if (line < traitslist.size()) {
                fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH-2, c_magenta,
                               traits[traitslist[line]].description);
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
                mvwprintz(w_traits, 0, 0, c_ltgray,  _("                          "));
                mvwprintz(w_traits, 0, 13 - utf8_width(title_TRAITS)/2, c_ltgray, title_TRAITS);
                for (size_t i = 0; i < traitslist.size() && i < trait_win_size_y; i++) {
                    mvwprintz(w_traits, i + 1, 1, c_black, "                         ");
                    if ((mutation_data[traitslist[i]].threshold == true) ||
                        (mutation_data[traitslist[i]].profession == true)) {
                        status = c_white;
                    }
                    else if (traits[traitslist[i]].mixed_effect == true) {
                        status = c_pink;
                    }
                    else if (traits[traitslist[i]].points > 0) {
                        status = c_ltgreen;
                    }
                    else if (traits[traitslist[i]].points < 0) {
                        status = c_ltred;
                    }
                    else {
                        status = c_yellow;
                    }
                    mvwprintz(w_traits, i + 1, 1, status, "%s", traits[traitslist[i]].name.c_str());
                }
                wrefresh(w_traits);
                line = 0;
                curtab++;
            } else if (action == "QUIT") {
                done = true;
            }
            break;

        case 5: // Effects tab
            mvwprintz(w_effects, 0, 0, h_ltgray,  _("                          "));
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
                mvwprintz(w_effects, 0, 0, c_ltgray,  _("                          "));
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
            mvwprintz(w_skills, 0, 0, h_ltgray,  _("                          "));
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
                SkillLevel level = skillLevel(aSkill);

                bool isLearning = level.isTraining();
                bool rusting = level.isRusting();
                int exercise = level.exercise();

                if (i == line) {
                    selectedSkill = aSkill;
                    if (exercise >= 100)
                        status = isLearning ? h_pink : h_magenta;
                    else if (rusting)
                        status = isLearning ? h_ltred : h_red;
                    else
                        status = isLearning ? h_ltblue : h_blue;
                } else {
                    if (rusting)
                        status = isLearning ? c_ltred : c_red;
                    else
                        status = isLearning ? c_ltblue : c_blue;
                }
                mvwprintz(w_skills, 1 + i - min, 1, c_ltgray, "                         ");
                mvwprintz(w_skills, 1 + i - min, 1, status, "%s:", aSkill->name().c_str());
                mvwprintz(w_skills, 1 + i - min,19, status, "%-2d(%2d%%)", (int)level, (exercise <  0 ? 0 : exercise));
            }

            //Draw Scrollbar
            draw_scrollbar(w_skills, line, skill_win_size_y, skillslist.size(), 1);

            werase(w_info);

            if (line < skillslist.size()) {
                fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH-2, c_magenta, selectedSkill->description());
            }
            wrefresh(w_skills);
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
                mvwprintz(w_skills, 0, 0, c_ltgray,  _("                          "));
                mvwprintz(w_skills, 0, 13 - utf8_width(title_SKILLS)/2, c_ltgray, title_SKILLS);
                for (size_t i = 0; i < skillslist.size() && i < size_t(skill_win_size_y); i++) {
                    const Skill* thisSkill = skillslist[i];
                    SkillLevel level = skillLevel(thisSkill);
                    bool isLearning = level.isTraining();
                    bool rusting = level.isRusting();

                    if (rusting)
                        status = isLearning ? c_ltred : c_red;
                    else
                        status = isLearning ? c_ltblue : c_blue;

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
        int length = i.name().length();
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

        // Trim the name if need be.
        if (name.length() > size_t(name_column_width)){
            name = name.erase(name_column_width-3, std::string::npos) + "...";
        }

        // Print out the name.
        mvwprintz(w, i + 3,  1, (bonus < 0 ? c_red : c_green), name.c_str());

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
    const int confidence_width = window_width - utf8_width( confidence_label.c_str() );
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
    const int steadiness_width = window_width - utf8_width( steadiness_label.c_str() );
    const std::string steadiness_meter = std::string( steadiness_width * steadiness, '*' );
    mvwprintw(w, line_number++, 1, "%s%s",
              steadiness_label.c_str(), steadiness_meter.c_str() );
    return line_number;
}

void player::print_gun_mode( WINDOW *w, nc_color c )
{
    // Print current weapon, or attachment if active.
    item* gunmod = weapon.active_gunmod();
    std::stringstream attachment;
    if (gunmod != NULL) {
        attachment << gunmod->type_name().c_str();
        for( auto &mod : weapon.contents ) {
            if( mod.is_auxiliary_gunmod() ) {
                attachment << " (" << mod.charges << ")";
            }
        }
        wprintz(w, c, _("%s (Mod)"), attachment.str().c_str());
    } else {
        if (weapon.get_gun_mode() == "MODE_BURST") {
            wprintz(w, c, _("%s (Burst)"), weapname().c_str());
        } else {
            wprintz(w, c, _("%s"), weapname().c_str());
        }
    }
}

void player::print_recoil( WINDOW *w ) const
{
    if (weapon.is_gun()) {
        const int adj_recoil = recoil + driving_recoil;
        if( adj_recoil > 150 ) {
            // 150 is the minimum when not actively aiming
            nc_color c = c_ltgray;
            if( adj_recoil >= 690 ) {
                c = c_red;
            } else if( adj_recoil >= 450 ) {
                c = c_ltred;
            } else if( adj_recoil >= 210 ) {
                c = c_yellow;
            }
            wprintz(w, c, _("Recoil"));
        }
    }
}

void player::disp_status(WINDOW *w, WINDOW *w2)
{
    bool sideStyle = use_narrow_sidebar();
    WINDOW *weapwin = sideStyle ? w2 : w;

    {
        const int y = sideStyle ? 1 : 0;
        wmove( weapwin, y, 0 );
        print_gun_mode( weapwin, c_ltgray );
    }

    // Print currently used style or weapon mode.
    std::string style = "";
    if (is_armed()) {
        // Show normal if no martial style is selected,
        // or if the currently selected style does nothing for your weapon.
        if (style_selected == "style_none" ||
            (!can_melee() && !martialarts[style_selected].has_weapon(weapon.type->id))) {
            style = _("Normal");
        } else {
            style = martialarts[style_selected].name;
        }

        int x = sideStyle ? (getmaxx(weapwin) - 13) : 0;
        mvwprintz(weapwin, 1, x, c_red, style.c_str());
    } else {
        if (style_selected == "style_none") {
            style = _("No Style");
        } else {
            style = martialarts[style_selected].name;
        }
        if (style != "") {
            int x = sideStyle ? (getmaxx(weapwin) - 13) : 0;
            mvwprintz(weapwin, 1, x, c_blue, style.c_str());
        }
    }

    wmove(w, sideStyle ? 1 : 2, 0);
    if (hunger > 2800)
        wprintz(w, c_red,    _("Starving!"));
    else if (hunger > 1400)
        wprintz(w, c_ltred,  _("Near starving"));
    else if (hunger > 300)
        wprintz(w, c_ltred,  _("Famished"));
    else if (hunger > 100)
        wprintz(w, c_yellow, _("Very hungry"));
    else if (hunger > 40)
        wprintz(w, c_yellow, _("Hungry"));
    else if (hunger < 0)
        wprintz(w, c_green,  _("Full"));
    else if (hunger < -20)
        wprintz(w, c_green,  _("Sated"));
    else if (hunger < -60)
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

    int x = sideStyle ? 37 : 32;
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
    if (fatigue > 575)
        wprintz(w, c_red,    _("Exhausted"));
    else if (fatigue > 383)
        wprintz(w, c_ltred,  _("Dead tired"));
    else if (fatigue > 191)
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
    else if (morale_cur >= 10)  morale_str = ":)";
    else if (morale_cur > -10)  morale_str = ":|";
    else if (morale_cur > -100) morale_str = "):";
    else if (morale_cur > -200) morale_str = "D:";
    else                        morale_str = "D8";
    mvwprintz(w, sideStyle ? 0 : 3, sideStyle ? 11 : 10, col_morale, morale_str);

    vehicle *veh = g->remoteveh();
    if( veh == nullptr && in_vehicle ) {
        veh = g->m.veh_at (posx, posy);
    }
    if( veh ) {
  veh->print_fuel_indicator(w, sideStyle ? 2 : 3, sideStyle ? getmaxx(w) - 5 : 49);
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
  int velx    = metric ?  5 : 4; // strlen(units) + 1
  int cruisex = metric ? 10 : 9; // strlen(units) + 6
  float conv  = metric ? 0.0161f : 0.01f;

  if (0 == sideStyle) {
    if (!veh->cruise_on) speedox += 2;
    if (!metric)         speedox++;
  }

  const char *speedo = veh->cruise_on ? "{%s....>....}" : "{%s....}";
  mvwprintz(w, speedoy, speedox,        col_indf1, speedo, units);
  mvwprintz(w, speedoy, speedox + velx, col_vel,   "%4d", int(veh->velocity * conv));
  if (veh->cruise_on)
    mvwprintz(w, speedoy, speedox + cruisex, c_ltgreen, "%4d", int(veh->cruise_velocity * conv));


  if (veh->velocity != 0) {
   nc_color col_indc = veh->skidding? c_red : c_green;
   int dfm = veh->face.dir() - veh->move.dir();
   wmove(w, sideStyle ? 5 : 3, getmaxx(w) - 3);
   wprintz(w, col_indc, dfm <  0 ? "L" : ".");
   wprintz(w, col_indc, dfm == 0 ? "0" : ".");
   wprintz(w, col_indc, dfm >  0 ? "R" : ".");
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

    int x  = sideStyle ? 18 : 13;
    int y  = sideStyle ?  0 :  3;
    int dx = sideStyle ?  0 :  7;
    int dy = sideStyle ?  1 :  0;
    mvwprintz(w, y + dy * 0, x + dx * 0, col_str, _("Str %2d"), get_str());
    mvwprintz(w, y + dy * 1, x + dx * 1, col_dex, _("Dex %2d"), get_dex());
    mvwprintz(w, y + dy * 2, x + dx * 2, col_int, _("Int %2d"), get_int());
    mvwprintz(w, y + dy * 3, x + dx * 3, col_per, _("Per %2d"), get_per());

    int spdx = sideStyle ?  0 : x + dx * 4;
    int spdy = sideStyle ?  5 : y + dy * 4;
    mvwprintz(w, spdy, spdx, col_spd, _("Spd %2d"), get_speed());
    if (this->weight_carried() > this->weight_capacity()) {
        col_time = h_black;
    }
    if (this->volume_carried() > this->volume_capacity() - 2) {
        if (this->weight_carried() > this->weight_capacity()) {
            col_time = c_dkgray_magenta;
        } else {
            col_time = c_dkgray_red;
        }
    }
    wprintz(w, col_time, "  %d", movecounter);
 }
}

bool player::has_trait(const std::string &b) const
{
    // Look for active mutations and traits
    return my_mutations.find( b ) != my_mutations.end();
}

bool player::has_base_trait(const std::string &b) const
{
    // Look only at base traits
    return my_traits.find( b ) != my_traits.end();
}

bool player::has_conflicting_trait(const std::string &flag) const
{
    return (has_opposite_trait(flag) || has_lower_trait(flag) || has_higher_trait(flag));
}

bool player::has_opposite_trait(const std::string &flag) const
{
    if (!mutation_data[flag].cancels.empty()) {
        std::vector<std::string> cancels = mutation_data[flag].cancels;
        for (auto &i : cancels) {
            if (has_trait(i)) {
                return true;
            }
        }
    }
    return false;
}

bool player::has_lower_trait(const std::string &flag) const
{
    if (!mutation_data[flag].prereqs.empty()) {
        std::vector<std::string> prereqs = mutation_data[flag].prereqs;
        for (auto &i : prereqs) {
            if (has_trait(i) || has_lower_trait(i)) {
                return true;
            }
        }
    }
    return false;
}

bool player::has_higher_trait(const std::string &flag) const
{
    if (!mutation_data[flag].replacements.empty()) {
        std::vector<std::string> replacements = mutation_data[flag].replacements;
        for (auto &i : replacements) {
            if (has_trait(i) || has_higher_trait(i)) {
                return true;
            }
        }
    }
    return false;
}

bool player::crossed_threshold()
{
    std::vector<std::string> traitslist;
    for( auto &mut : my_mutations ) {
        traitslist.push_back( mut );
    }
    for( auto &i : traitslist ) {
        if (mutation_data[i].threshold == true) {
            return true;
        }
    }
    return false;
}

bool player::purifiable(const std::string &flag) const
{
    if(mutation_data[flag].purifiable) {
        return true;
    }
    return false;
}

void player::toggle_str_set( std::unordered_set< std::string > &set, const std::string &str )
{
    auto toggled_element = std::find( set.begin(), set.end(), str );
    if( toggled_element == set.end() ) {
        char new_key = ' ';
        // Find a letter in inv_chars that isn't in trait_keys.
        for( const auto &letter : inv_chars ) {
            bool found = false;
            for( const auto &key : trait_keys ) {
                if( letter == key.second ) {
                    found = true;
                    break;
                }
            }
            if( !found ) {
                new_key = letter;
                break;
            }
        }
        set.insert( str );
        trait_keys[str] = new_key;
    } else {
        set.erase( toggled_element );
        trait_keys.erase(str);
    }
}

void mutation_effect(player &p, std::string mut);
void mutation_loss_effect(player &p, std::string mut);
void player::toggle_trait(const std::string &flag)
{
    toggle_str_set(my_traits, flag); //Toggles a base trait on the player
    toggle_str_set(my_mutations, flag); //Toggles corresponding trait in mutations list as well.
    if( has_trait( flag ) ) {
        mutation_effect( *this, flag );
    } else {
        mutation_loss_effect( *this, flag );
    }
    recalc_sight_limits();
}

void player::toggle_mutation(const std::string &flag)
{
    toggle_str_set(my_mutations, flag); //Toggles a mutation on the player
    recalc_sight_limits();
}

void player::set_cat_level_rec(const std::string &sMut)
{
    if (!has_base_trait(sMut)) { //Skip base traits
        for( auto &elem : mutation_data[sMut].category ) {
            mutation_category_level[elem] += 8;
        }

        for (auto &i : mutation_data[sMut].prereqs) {
            set_cat_level_rec(i);
        }

        for (auto &i : mutation_data[sMut].prereqs2) {
            set_cat_level_rec(i);
        }
    }
}

void player::set_highest_cat_level()
{
    mutation_category_level.clear();

    // Loop through our mutations
    for( auto &mut : my_mutations ) {
        set_cat_level_rec( mut );
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
    dream selected_dream;
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
    int index = rng(0, valid_dreams.size() - 1); // Randomly select a dream from the valid list
    selected_dream = valid_dreams[index];
    index = rng(0, selected_dream.messages.size() - 1); // Randomly selected a message from the chosen dream
    message = selected_dream.messages[index];
    return message;
}

bool player::in_climate_control()
{
    bool regulated_area=false;
    // Check
    if(has_active_bionic("bio_climate")) { return true; }
    for( auto &w : worn ) {
        if( w.typeId() == "rm13_armor_on" ) {
            return true;
        }
        if( w.active && w.is_power_armor() ) {
            return true;
        }
    }
    if(int(calendar::turn) >= next_climate_control_check)
    {
        next_climate_control_check=int(calendar::turn)+20;  // save cpu and similate acclimation.
        int vpart = -1;
        vehicle *veh = g->m.veh_at(posx, posy, vpart);
        if(veh)
        {
            regulated_area=(
                veh->is_inside(vpart) &&    // Already checks for opened doors
                veh->total_power(true) > 0  // Out of gas? No AC for you!
            );  // TODO: (?) Force player to scrounge together an AC unit
        }
        // TODO: AC check for when building power is implemented
        last_climate_control_ret=regulated_area;
        if(!regulated_area) { next_climate_control_check+=40; }  // Takes longer to cool down / warm up with AC, than it does to step outside and feel cruddy.
    }
    else
    {
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
        if (stack_iter->active && stack_iter->has_flag("RADIO_ACTIVATION")) {
            rc_items.push_back( stack_iter );
        }
    }

    for( auto &elem : worn ) {
        if( elem.active && elem.has_flag( "RADIO_ACTIVATION" ) ) {
            rc_items.push_back( &elem );
        }
    }

    if (!weapon.is_null()) {
        if ( weapon.active  && weapon.has_flag("RADIO_ACTIVATION")) {
            rc_items.push_back( &weapon );
        }
    }
    return rc_items;
}



bool player::has_bionic(const bionic_id & b) const
{
    for (auto &i : my_bionics) {
        if (i.id == b) {
            return true;
        }
    }
    return false;
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

bool player::has_active_bionic(const bionic_id & b) const
{
    for (auto &i : my_bionics) {
        if (i.id == b) {
            return (i.powered);
        }
    }
    return false;
}
bool player::has_active_mutation(const std::string & b) const
{
    const auto &mut_iter = my_mutations.find( b );
    if( mut_iter == my_mutations.end() ) {
        return false;
    }
    return traits[*mut_iter].powered;
}

void player::add_bionic( bionic_id b )
{
    if( has_bionic( b ) ) {
        debugmsg( "Tried to install bionic %s that is already installed!", b.c_str() );
        return;
    }
    char newinv = ' ';
    for( auto &inv_char : inv_chars ) {
        if( bionic_by_invlet( inv_char ) == nullptr ) {
            newinv = inv_char;
            break;
        }
    }
    my_bionics.push_back( bionic( b, newinv ) );
    recalc_sight_limits();
}

void player::remove_bionic(bionic_id b) {
    std::vector<bionic> new_my_bionics;
    for(auto &i : my_bionics) {
        if (!(i.id == b)) {
            new_my_bionics.push_back(bionic(i.id, i.invlet));
        }
    }
    my_bionics = new_my_bionics;
    recalc_sight_limits();
}

int player::num_bionics() const
{
    return my_bionics.size();
}

bionic& player::bionic_at_index(int i)
{
    return my_bionics[i];
}

bionic* player::bionic_by_invlet(char ch) {
    for( auto &elem : my_bionics ) {
        if( elem.invlet == ch ) {
            return &elem;
        }
    }
    return 0;
}

// Returns true if a bionic was removed.
bool player::remove_random_bionic() {
    const int numb = num_bionics();
    if (numb) {
        int rem = rng(0, num_bionics() - 1);
        my_bionics.erase(my_bionics.begin() + rem);
        recalc_sight_limits();
    }
    return numb;
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
float player::active_light()
{
    float lumination = 0;

    int maxlum = 0;
    const invslice & stacks = inv.slice();
    for( auto &stack : stacks ) {
        item &itemit = stack->front();
        item * stack_iter = &itemit;
        if (stack_iter->active && stack_iter->charges > 0) {
            int lumit = stack_iter->getlight_emit(true);
            if ( maxlum < lumit ) {
                maxlum = lumit;
            }
        }
    }

    for( auto &elem : worn ) {
        if( elem.active && elem.charges > 0 ) {
            int lumit = elem.getlight_emit( true );
            if ( maxlum < lumit ) {
                maxlum = lumit;
            }
        }
    }

    if (!weapon.is_null()) {
        if ( weapon.active  && weapon.charges > 0) {
            int lumit = weapon.getlight_emit(true);
            if ( maxlum < lumit ) {
                maxlum = lumit;
            }
        }
    }

    lumination = (float)maxlum;

    if ( lumination < 60 && has_active_bionic("bio_flashlight") ) {
        lumination = 60;
    } else if ( lumination < 25 && has_artifact_with(AEP_GLOW) ) {
        lumination = 25;
    }

    return lumination;
}

point player::pos() const
{
    return point(posx, posy);
}

int player::sight_range(int light_level) const
{
    // Apply the sight boost (night vision).
    if (light_level < sight_boost_cap) {
        light_level = std::min(light_level + sight_boost, sight_boost_cap);
    }

    // Clamp to sight_max.
    return std::min(light_level, sight_max);
}

// This must be called when any of the following change:
// - diseases
// - bionics
// - traits
// - underwater
// - clothes
// With the exception of clothes, all changes to these player attributes must
// occur through a function in this class which calls this function. Clothes are
// typically added/removed with wear() and takeoff(), but direct access to the
// 'wears' vector is still allowed due to refactor exhaustion.
void player::recalc_sight_limits()
{
    sight_max = 9999;
    sight_boost = 0;
    sight_boost_cap = 0;

    // Set sight_max.
    if (has_effect("blind")) {
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

    // Set sight_boost and sight_boost_cap, based on night vision.
    // (A player will never have more than one night vision trait.)
    sight_boost_cap = 12;
    // Debug-only NV, by vache's request
    if (has_trait("DEBUG_NIGHTVISION")) {
        sight_boost = 59;
        sight_boost_cap = 59;
    } else if (has_nv() || has_trait("NIGHTVISION3") || has_trait("ELFA_FNV") || is_wearing("rm13_armor_on") ||
      (has_trait("CEPH_VISION")) ) {
        // Yes, I'm breaking the cap. I doubt the reality bubble shrinks at night.
        // BIRD_EYE represents excellent fine-detail vision so I think it works.
        if (has_trait("BIRD_EYE")) {
            sight_boost = 13;
        }
        else {
        sight_boost = sight_boost_cap;
        }
    } else if (has_trait("ELFA_NV")) {
        sight_boost = 6; // Elf-a and Bird eyes shouldn't coexist
    } else if (has_trait("NIGHTVISION2") || has_trait("FEL_NV") || has_trait("URSINE_EYE")) {
        if (has_trait("BIRD_EYE")) {
            sight_boost = 5;
        }
         else {
            sight_boost = 4;
         }
    } else if (has_trait("NIGHTVISION")) {
        if (has_trait("BIRD_EYE")) {
            sight_boost = 2;
        }
        else {
            sight_boost = 1;
        }
    }
}

int player::unimpaired_range()
{
 int ret = DAYLIGHT_LEVEL;
 if (has_trait("PER_SLIME")) {
    ret = 6;
 }
 if (has_active_mutation("SHELL2")) {
    ret = 2;
 }
 if (has_effect("in_pit")) {
    ret = 1;
  }
 if (has_effect("blind")) {
    ret = 0;
  }
 return ret;
}

bool player::overmap_los(int omtx, int omty, int sight_points)
{
    const tripoint ompos = g->om_global_location();
    if (omtx < ompos.x - sight_points || omtx > ompos.x + sight_points ||
        omty < ompos.y - sight_points || omty > ompos.y + sight_points) {
        // Outside maximum sight range
        return false;
    }

    const std::vector<point> line = line_to(ompos.x, ompos.y, omtx, omty, 0);
    for (size_t i = 0; i < line.size() && sight_points >= 0; i++) {
        const oter_id &ter = overmap_buffer.ter(line[i].x, line[i].y, ompos.z);
        const int cost = otermap[ter].see_cost;
        sight_points -= cost;
        if (sight_points < 0)
            return false;
    }
    return true;
}

int player::overmap_sight_range(int light_level)
{
    int sight = sight_range(light_level);
    if( sight < SEEX ) {
        return 0;
    }
    if( sight <= SEEX * 4) {
        return (sight / (SEEX / 2) );
    }
    if ((has_amount("binoculars", 1) || has_amount("rifle_scope", 1) ||
        -1 != weapon.has_gunmod("rifle_scope") ) && !has_trait("EAGLEEYED"))  {
        if (has_trait("BIRD_EYE")) {
            return 25;
        }
        return 20;
    }
    else if (!(has_amount("binoculars", 1) || has_amount("rifle_scope", 1) ||
        -1 != weapon.has_gunmod("rifle_scope") ) && has_trait("EAGLEEYED"))  {
        if (has_trait("BIRD_EYE")) {
            return 25;
        }
        return 20;
    }
    else if ((has_amount("binoculars", 1) || has_amount("rifle_scope", 1) ||
        -1 != weapon.has_gunmod("rifle_scope") ) && has_trait("EAGLEEYED"))  {
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

bool player::sight_impaired()
{
 return ((has_effect("boomered") && (!(has_trait("PER_SLIME_OK")))) ||
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

bool player::avoid_trap(trap* tr, int x, int y)
{
  int myroll = dice(3, int(dex_cur + skillLevel("dodge") * 1.5));
 int traproll;
    if (tr->can_see(*this, x, y)) {
        traproll = dice(3, tr->get_avoidance());
    } else {
        traproll = dice(6, tr->get_avoidance());
    }
 if (has_trait("LIGHTSTEP"))
  myroll += dice(2, 6);
 if (has_trait("CLUMSY"))
  myroll -= dice(2, 6);
 if (myroll >= traproll)
  return true;
 return false;
}

bool player::has_nv()
{
    static bool nv = false;

    if( !nv_cached ) {
        nv_cached = true;
        nv = (worn_with_flag("GNV_EFFECT") ||
              has_active_bionic("bio_night_vision"));
    }

    return nv;
}

bool player::has_pda()
{
    static bool pda = false;
    if ( !pda_cached ) {
      pda_cached = true;
      pda = has_amount("pda", 1);
    }

    return pda;
}

void player::pause()
{
    moves = 0;
    recoil -= str_cur + 2 * skillLevel("gun");
    recoil = std::max(MIN_RECOIL * 2, recoil);
    recoil = int(recoil / 2);

    // Train swimming if underwater
    if (underwater) {
        practice( "swimming", 1 );
        if (g->temperature <= 50) {
            drench(100, mfb(bp_leg_l)|mfb(bp_leg_r)|mfb(bp_torso)|mfb(bp_arm_l)|mfb(bp_arm_r)|
                        mfb(bp_head)| mfb(bp_eyes)|mfb(bp_mouth)|mfb(bp_foot_l)|mfb(bp_foot_r)|
                        mfb(bp_hand_l)|mfb(bp_hand_r));
        } else {
            drench(100, mfb(bp_leg_l)|mfb(bp_leg_r)|mfb(bp_torso)|mfb(bp_arm_l)|mfb(bp_arm_r)|
                        mfb(bp_head)| mfb(bp_eyes)|mfb(bp_mouth));
        }
    }

    VehicleList vehs = g->m.get_vehicles();
    vehicle* veh = NULL;
    for (auto &v : vehs) {
        veh = v.v;
        if (veh && veh->velocity != 0 && veh->player_in_control(this)) {
            if (one_in(10)) {
                practice( "driving", 1 );
            }
            break;
        }
    }

    search_surroundings();
}

void player::search_surroundings()
{
    if (controlling_vehicle) {
        return;
    }
    // Search for traps in a larger area than before because this is the only
    // way we can "find" traps that aren't marked as visible.
    // Detection formula takes care of likelihood of seeing within this range.
    for (size_t i = 0; i < 121; i++) {
        const int x = posx + i / 11 - 5;
        const int y = posy + i % 11 - 5;
        const trap_id trid = g->m.tr_at(x, y);
        if (trid == tr_null || (x == posx && y == posy)) {
            continue;
        }
        if( !sees( x, y ) ) {
            continue;
        }
        const trap *tr = traplist[trid];
        if (tr->name.empty() || tr->can_see(*this, x, y)) {
            // Already seen, or has no name -> can never be seen
            continue;
        }
        // Chance to detect traps we haven't yet seen.
        if (tr->detect_trap(*this, x, y)) {
            if( tr->get_visibility() > 0 ) {
                // Only bug player about traps that aren't trivial to spot.
                const std::string direction = direction_name(direction_from(posx, posy, x, y));
                add_msg_if_player(_("You've spotted a %s to the %s!"),
                                  tr->name.c_str(), direction.c_str());
            }
            add_known_trap(x, y, tr->id);
        }
    }
}

int player::throw_range(int pos)
{
 item tmp;
 if (pos == -1)
  tmp = weapon;
 else if (pos == INT_MIN)
  return -1;
 else
  tmp = inv.find_item(pos);

 if (tmp.count_by_charges() && tmp.charges > 1)
  tmp.charges = 1;

 if ((tmp.weight() / 113) > int(str_cur * 15))
  return 0;
 // Increases as weight decreases until 150 g, then decreases again
 int ret = (str_cur * 8) / (tmp.weight() >= 150 ? tmp.weight() / 113 : 10 - int(tmp.weight() / 15));
 ret -= int(tmp.volume() / 4);
 if (has_active_bionic("bio_railgun") && (tmp.made_of("iron") || tmp.made_of("steel")))
    ret *= 2;
 if (ret < 1)
  return 1;
// Cap at double our strength + skill
 if (ret > str_cur * 1.5 + skillLevel("throw"))
   return str_cur * 1.5 + skillLevel("throw");
 return ret;
}

int player::ranged_dex_mod() const
{
    const int dex = get_dex();

    if (dex >= 12) { return 0; }
    return (12 - dex) * 15;
}

int player::ranged_per_mod() const
{
    const int per = get_per();

    if (per >= 12) { return 0; }
    return (12 - per) * 15;
}

int player::throw_dex_mod(bool return_stat_effect) const
{
  // Stat window shows stat effects on based on current stat
 int dex = get_dex();
 if (dex == 8 || dex == 9)
  return 0;
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

int player::read_speed(bool return_stat_effect)
{
  // Stat window shows stat effects on based on current stat
 int intel = (return_stat_effect ? get_int() : get_int());
 int ret = 1000 - 50 * (intel - 8);
 if (has_trait("FASTREADER"))
  ret *= .8;
 if (has_trait("SLOWREADER"))
  ret *= 1.3;
 if (ret < 100)
  ret = 100;
 // return_stat_effect actually matters here
 return (return_stat_effect ? ret : ret / 10);
}

int player::rust_rate(bool return_stat_effect)
{
    if (OPTIONS["SKILL_RUST"] == "off") {
        return 0;
    }

    // Stat window shows stat effects on based on current stat
    int intel = (return_stat_effect ? get_int() : get_int());
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

int player::talk_skill()
{
    int ret = get_int() + get_per() + skillLevel("speech") * 3;
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

int player::intimidation()
{
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

bool player::is_dead_state() const {
    return hp_cur[hp_head] <= 0 || hp_cur[hp_torso] <= 0;
}

void player::on_gethit(Creature *source, body_part bp_hit, damage_instance &) {
    bool u_see = g->u.sees(*this);
    if (source != NULL) {
        if (has_active_bionic("bio_ods")) {
            if (is_player()) {
                add_msg(m_good, _("Your offensive defense system shocks %s in mid-attack!"),
                                source->disp_name().c_str());
            } else if (u_see) {
                add_msg(_("%s's offensive defense system shocks %s in mid-attack!"),
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
                    add_msg(_("%1$s's %2$s puncture %s in mid-attack!"), name.c_str(),
                                (has_trait("QUILLS") ? _("quills") : _("spines")),
                                source->disp_name().c_str());
                }
            } else {
                add_msg(m_good, _("Your %s puncture %s in mid-attack!"),
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
                    add_msg(_("%1$s's %2$s scrape %s in mid-attack!"), name.c_str(),
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
                    add_msg(_("%1$s gets a load of %2$s's %s stuck in!"), source->disp_name().c_str(),
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
}

dealt_damage_instance player::deal_damage(Creature* source, body_part bp, const damage_instance& d)
{
    dealt_damage_instance dealt_dams = Creature::deal_damage(source, bp, d); //damage applied here
    int dam = dealt_dams.total_damage(); //block reduction should be by applied this point

    if (in_sleep_state()) {
        wake_up();
    }

    if (is_player()) {
        if (source != nullptr) {
            g->cancel_activity_query(_("You were attacked by %s!"), source->disp_name().c_str());
        } else {
            // TODO: Find the name of what the player is hurt by
            g->cancel_activity_query(_("You were hurt!"));
        }
    }

    // TODO: Pre or post blit hit tile onto "this"'s location here
    if(g->u.sees( pos() )) {
        g->draw_hit_player(this, dam);

        if (dam > 0 && is_player() && source) {
            //monster hits player melee
            nc_color color;
            std::string health_bar = "";
            get_HP_Bar(dam, this->get_hp_max(bodypart_to_hp_part(bp)), color, health_bar);

            SCT.add(this->xpos(),
                    this->ypos(),
                    direction_from(0, 0, this->xpos() - source->xpos(), this->ypos() - source->ypos()),
                    health_bar.c_str(), m_bad,
                    body_part_name(bp), m_neutral);
        }
    }

    // handle snake artifacts
    if (has_artifact_with(AEP_SNAKES) && dam >= 6) {
        int snakes = int(dam / 6);
        std::vector<point> valid;
        for (int x = posx - 1; x <= posx + 1; x++) {
            for (int y = posy - 1; y <= posy + 1; y++) {
                if (g->is_empty(x, y)) {
                    valid.push_back( point(x, y) );
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
        monster snake(GetMType("mon_shadow_snake"));
        for (int i = 0; i < snakes; i++) {
            int index = rng(0, valid.size() - 1);
            point sp = valid[index];
            valid.erase(valid.begin() + index);
            snake.spawn(sp.x, sp.y);
            snake.friendly = -1;
            g->add_zombie(snake);
        }
    }

    // And slimespawners too
    if ((has_trait("SLIMESPAWNER")) && (dam >= 10) && one_in(20 - dam)) {
        std::vector<point> valid;
        for (int x = posx - 1; x <= posx + 1; x++) {
            for (int y = posy - 1; y <= posy + 1; y++) {
                if (g->is_empty(x, y)) {
                    valid.push_back( point(x, y) );
                }
            }
        }
        add_msg(m_warning, _("Slime is torn from you, and moves on its own!"));
        int numslime = 1;
        monster slime(GetMType("mon_player_blob"));
        for (int i = 0; i < numslime; i++) {
            int index = rng(0, valid.size() - 1);
            point sp = valid[index];
            valid.erase(valid.begin() + index);
            slime.spawn(sp.x, sp.y);
            slime.friendly = -1;
            g->add_zombie(slime);
        }
    }

    if (has_trait("ADRENALINE") && !has_effect("adrenaline") &&
        (hp_cur[hp_head] < 25 || hp_cur[hp_torso] < 15)) {
        add_effect("adrenaline", 200);
    }

    int cut_dam = dealt_dams.type_damage(DT_CUT);
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

    /*
        It almost looks like damage may be getting applied twice in some cases.
     */
    case bp_mouth: // Fall through to head damage
    case bp_head:
        hp_cur[hp_head] -= dam; //this looks like an extra damage hit, as is applied in apply_damage from player::apply_damage()
        if (hp_cur[hp_head] < 0) {
            lifetime_stats()->damage_taken+=hp_cur[hp_head];
            hp_cur[hp_head] = 0;
        }
        break;
    case bp_torso:
        // getting hit throws off our shooting
        recoil += int(dam * 3);
        break;
    case bp_hand_l: // Fall through to arms
    case bp_arm_l:
        if (weapon.is_two_handed(this)) {
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

        if ( source->has_flag(MF_GRABS)) {
            add_msg_player_or_npc(m_bad, _("%s grabs you!"), _("%s grabs <npcname>!"),
                                  source->disp_name().c_str());
            if (has_grab_break_tec() && get_grab_resist() > 0 && get_dex() > get_str() ?
                dice(get_dex(), 10) : dice(get_str(), 10) > dice(source->get_dex(), 10)) {
                add_msg_player_or_npc(m_good, _("You break the grab!"),
                                      _("<npcname> breaks the grab!"));
            } else {
                add_effect("grabbed", 1);
            }
        }
    }

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
    if (!is_npc() && ((npain >= 1) && (rng(0, pain) >= 10))) {
        g->cancel_activity_query(_("Ouch, you were hurt!"));
        if (in_sleep_state()) {
            wake_up();
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
    if( is_dead_state() ) {
        // don't do any more damage if we're already dead
        return;
    }
    hp_part hurtpart;
    switch (hurt) {
        case bp_eyes: // Fall through to head damage
        case bp_mouth: // Fall through to head damage
        case bp_head:
            hurtpart = hp_head;
            break;
        case bp_torso:
            hurtpart = hp_torso;
            break;
        case bp_hand_l: // fall through to arms
        case bp_arm_l:
            hurtpart = hp_arm_l;
            break;
        case bp_hand_r: // but fall through to arms
        case bp_arm_r:
            hurtpart = hp_arm_r;
            break;
        case bp_foot_l: // but fall through to legs
        case bp_leg_l:
            hurtpart = hp_leg_l;
            break;
        case bp_foot_r: // but fall through to legs
        case bp_leg_r:
            hurtpart = hp_leg_r;
            break;
        default:
            debugmsg("Wacky body part hurt!");
            hurtpart = hp_torso;
    }
    if (in_sleep_state()) {
        wake_up();
    }

    if (dam <= 0) {
        return;
    }

    if (!is_npc()) {
        if (source != nullptr) {
            g->cancel_activity_query(_("You were attacked by %s!"), source->disp_name().c_str());
        } else {
            // TODO: Find the name of what the player is hurt by
            g->cancel_activity_query(_("You were hurt!"));
        }
    }

    mod_pain( dam /2 );

    if (has_trait("ADRENALINE") && !has_effect("adrenaline") &&
        (hp_cur[hp_head] < 25 || hp_cur[hp_torso] < 15)) {
        add_effect("adrenaline", 200);
    }
    hp_cur[hurtpart] -= dam;
    if (hp_cur[hurtpart] < 0) {
        lifetime_stats()->damage_taken += hp_cur[hurtpart];
        hp_cur[hurtpart] = 0;
    }
    lifetime_stats()->damage_taken += dam;
    if( is_dead_state() ) {
        die( source );
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

void player::hurtall(int dam)
{
    for (int i = 0; i < num_hp_parts; i++) {
        hp_cur[i] -= dam;
        if (hp_cur[i] < 0) {
            lifetime_stats()->damage_taken += hp_cur[i];
            hp_cur[i] = 0;
        }
        mod_pain( dam / 2 );
        lifetime_stats()->damage_taken += dam;
    }
}

void player::hitall(int dam, int vary)
{
    if (in_sleep_state()) {
        wake_up();
    }

    for (int i = 0; i < num_hp_parts; i++) {
        int ddam = vary? dam * rng (100 - vary, 100) / 100 : dam;
        int cut = 0;
        absorb((body_part) i, ddam, cut);
        hp_cur[i] -= ddam;
        if (hp_cur[i] < 0) {
            lifetime_stats()->damage_taken += hp_cur[i];
            hp_cur[i] = 0;
        }

        // Average of pre and post armor damage levels, divided by 8.
        mod_pain( (dam + ddam) / 16 );
        lifetime_stats()->damage_taken += ddam;
    }
}

void player::knock_back_from(int x, int y)
{
 if (x == posx && y == posy)
  return; // No effect
 point to(posx, posy);
 if (x < posx)
  to.x++;
 if (x > posx)
  to.x--;
 if (y < posy)
  to.y++;
 if (y > posy)
  to.y--;

// First, see if we hit a monster
 int mondex = g->mon_at(to.x, to.y);
 if (mondex != -1) {
  monster *critter = &(g->zombie(mondex));
  deal_damage( critter, bp_torso, damage_instance( DT_BASH, critter->type->size ) );
  add_effect("stunned", 1);
  if ((str_max - 6) / 4 > critter->type->size) {
   critter->knock_back_from(posx, posy); // Chain reaction!
   critter->apply_damage( this, bp_torso, (str_max - 6) / 4);
   critter->add_effect("stunned", 1);
  } else if ((str_max - 6) / 4 == critter->type->size) {
   critter->apply_damage( this, bp_torso, (str_max - 6) / 4);
   critter->add_effect("stunned", 1);
  }

  add_msg_player_or_npc(_("You bounce off a %s!"), _("<npcname> bounces off a %s!"),
                            critter->name().c_str() );

  return;
 }

 int npcdex = g->npc_at(to.x, to.y);
 if (npcdex != -1) {
  npc *p = g->active_npc[npcdex];
  deal_damage( p, bp_torso, damage_instance( DT_BASH, p->get_size() ) );
  add_effect("stunned", 1);
  p->deal_damage( this, bp_torso, damage_instance( DT_BASH, 3 ) );
  add_msg_player_or_npc( _("You bounce off %s!"), _("<npcname> bounces off %s!"), p->name.c_str() );
  return;
 }

// If we're still in the function at this point, we're actually moving a tile!
 if (g->m.has_flag("LIQUID", to.x, to.y) && g->m.has_flag(TFLAG_DEEP_WATER, to.x, to.y)) {
  if (!is_npc()) {
   g->plswim(to.x, to.y);
  }
// TODO: NPCs can't swim!
 } else if (g->m.move_cost(to.x, to.y) == 0) { // Wait, it's a wall (or water)

  // It's some kind of wall.
  apply_damage( nullptr, bp_torso, 3 ); // TODO: who knocked us back? Maybe that creature should be the source of the damage?
  add_effect("stunned", 2);
  add_msg_player_or_npc( _("You bounce off a %s!"), _("<npcname> bounces off a %s!"),
                             g->m.tername(to.x, to.y).c_str() );

 } else { // It's no wall
  posx = to.x;
  posy = to.y;
 }
}

void player::bp_convert(hp_part &hpart, body_part bp)
{
    hpart =  num_hp_parts;
    switch(bp) {
        case bp_head:
            hpart = hp_head;
            break;
        case bp_torso:
            hpart = hp_torso;
            break;
        case bp_arm_l:
            hpart = hp_arm_l;
            break;
        case bp_arm_r:
            hpart = hp_arm_r;
            break;
        case bp_leg_l:
            hpart = hp_leg_l;
            break;
        case bp_leg_r:
            hpart = hp_leg_r;
            break;
        default:
            //Silence warnings
            break;
    }
}

void player::hp_convert(hp_part hpart, body_part &bp)
{
    bp =  num_bp;
    switch(hpart) {
        case hp_head:
            bp = bp_head;
            break;
        case hp_torso:
            bp = bp_torso;
            break;
        case hp_arm_l:
            bp = bp_arm_l;
            break;
        case hp_arm_r:
            bp = bp_arm_r;
            break;
        case hp_leg_l:
            bp = bp_leg_l;
            break;
        case hp_leg_r:
            bp = bp_leg_r;
            break;
        default:
            // Silence warnings
            break;
    }
}

int player::hp_percentage()
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

void player::recalc_hp()
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

void player::get_sick()
{
    if (has_trait("DISIMMUNE")) {
        return;
    }

    if (!has_effect("flu") && !has_effect("common_cold") &&
        one_in(900 + get_healthy() + (has_trait("DISRESISTANT") ? 300 : 0))) {
        if (one_in(6) && !has_effect("flushot")) {
            add_env_effect("flu", bp_mouth, 3, rng(40000, 80000));
        } else {
            add_env_effect("common_cold", bp_mouth, 3, rng(20000, 60000));
        }
    }
}

void player::update_health(int base_threshold)
{
    if (has_artifact_with(AEP_SICK)) {
        base_threshold += 50;
    }
    Creature::update_health(base_threshold);
}

bool player::infect(dis_type type, body_part vector, int strength,
                     int duration, bool permanent, int intensity,
                     int max_intensity, int decay, int additive, bool targeted,
                     bool main_parts_only)
{
    if (strength <= 0) {
        return false;
    }

    if (dice(strength, 3) > dice(get_env_resist(vector), 3)) {
        if (targeted) {
            add_disease(type, duration, permanent, intensity, max_intensity, decay,
                          additive, vector, main_parts_only);
        } else {
            add_disease(type, duration, permanent, intensity, max_intensity, decay, additive);
        }
        return true;
    }

    return false;
}

void player::add_disease(dis_type type, int duration, bool permanent,
                         int intensity, int max_intensity, int decay,
                         int additive, body_part part, bool main_parts_only)
{
    if (duration <= 0) {
        return;
    }

    if (part != num_bp && hp_cur[bodypart_to_hp_part(part)] == 0) {
        return;
    }

    if (main_parts_only) {
        part = mutate_to_main_part(part);
    }

    bool found = false;
    for (auto &i : illness) {
        if (i.type == type) {
            if ((part == num_bp) ^ (i.bp == num_bp)) {
                debugmsg("Bodypart missmatch when applying disease %s",
                         type.c_str());
                return;
            }
            if (i.bp == part) {
                if (additive > 0) {
                    i.duration += duration;
                } else if (additive < 0) {
                    i.duration -= duration;
                    if (i.duration <= 0) {
                        i.duration = 1;
                    }
                }
                i.intensity += intensity;
                if (max_intensity != -1 && i.intensity > max_intensity) {
                    i.intensity = max_intensity;
                }
                if (permanent) {
                    i.permanent = true;
                }
                i.decay = decay;
                found = true;
                // Found it, so no need to keep checking
                break;
            }
        }
    }
    if (!found) {
        disease tmp(type, duration, intensity, part, permanent, decay);
        illness.push_back(tmp);
    }

    recalc_sight_limits();
}

void player::rem_disease(dis_type type, body_part part)
{
    for (auto &next_illness : illness) {
        if (next_illness.type == type && ( part == num_bp || next_illness.bp == part )) {
            next_illness.duration = -1;
        }
    }

    recalc_sight_limits();
}

bool player::has_disease(dis_type type, body_part part) const
{
    for (auto &i : illness) {
        if (i.duration > 0 && i.type == type && ( part == num_bp || i.bp == part )) {
            return true;
        }
    }
    return false;
}

bool player::pause_disease(dis_type type, body_part part)
{
    for (auto &i : illness) {
        if (i.type == type && ( part == num_bp || i.bp == part )) {
                i.permanent = true;
                return true;
        }
    }
    return false;
}

bool player::unpause_disease(dis_type type, body_part part)
{
    for (auto &i : illness) {
        if (i.type == type && ( part == num_bp || i.bp == part )) {
                i.permanent = false;
                return true;
        }
    }
    return false;
}

int player::disease_duration(dis_type type, bool all, body_part part) const
{
    int tmp = 0;
    for (auto &i : illness) {
        if (i.type == type && (part ==  num_bp || i.bp == part)) {
            if (all == false) {
                return i.duration;
            } else {
                tmp += i.duration;
            }
        }
    }
    return tmp;
}

int player::disease_intensity(dis_type type, bool all, body_part part) const
{
    int tmp = 0;
    for (auto &i : illness) {
        if (i.type == type && (part ==  num_bp || i.bp == part)) {
            if (all == false) {
                return i.intensity;
            } else {
                tmp += i.intensity;
            }
        }
    }
    return tmp;
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

int player::addiction_level(add_type type)
{
    for (auto &i : addictions) {
        if (i.type == type) {
            return i.intensity;
        }
    }
    return 0;
}

bool player::siphon(vehicle *veh, ammotype desired_liquid)
{
    int liquid_amount = veh->drain( desired_liquid, veh->fuel_capacity(desired_liquid) );
    item used_item( default_ammo(desired_liquid), calendar::turn );
    used_item.charges = liquid_amount;
    int extra = g->move_liquid( used_item );
    if( extra == -1 ) {
        // Failed somehow, put the liquid back and bail out.
        veh->refill( desired_liquid, used_item.charges );
        return false;
    }
    int siphoned = liquid_amount - extra;
    veh->refill( desired_liquid, extra );
    if( siphoned > 0 ) {
        add_msg(ngettext("Siphoned %d unit of %s from the %s.",
                            "Siphoned %d units of %s from the %s.",
                            siphoned),
                   siphoned, used_item.tname().c_str(), veh->name.c_str());
        //Don't consume turns if we decided not to siphon
        return true;
    } else {
        return false;
    }
}

void player::cough(bool harmful, int loudness) {
    if (!is_npc()) {
        add_msg(m_bad, _("You cough heavily."));
        g->sound(posx, posy, loudness, "");
    } else {
        g->sound(posx, posy, loudness, _("a hacking cough."));
    }
    moves -= 80;
    if (harmful && !one_in(4)) {
        apply_damage( nullptr, bp_torso, 1 );
    }
    if (has_effect("sleep") && ((harmful && one_in(3)) || one_in(10)) ) {
        wake_up();
    }
}

void player::add_pain_msg(int val, body_part bp)
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

static int bound_mod_to_vals(int val, int mod, int max, int min)
{
    if (val + mod > max && max != 0) {
        mod = std::max(max - val, 0);
    }
    if (val + mod < min && min != 0) {
        mod = std::min(min - val, 0);
    }
    return mod;
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
        mod_healthy_mod(bound_mod_to_vals(get_healthy_mod(), e.get_amount("H_MOD", reduced),
                        e.get_max_val("H_MOD", reduced), e.get_min_val("H_MOD", reduced)));
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
        hunger += bound_mod_to_vals(hunger, e.get_amount("HUNGER", reduced),
                        e.get_max_val("HUNGER", reduced), e.get_min_val("HUNGER", reduced));
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
    if (has_effect("darkness") && g->is_in_sunlight(posx, posy)) {
        remove_effect("darkness");
    }
    if (has_trait("M_IMMUNE") && has_effect("fungus")) {
        vomit();
        remove_effect("fungus");
        add_msg_if_player(m_bad,  _("We have mistakenly colonized a local guide!  Purging now."));
    }
    if (has_trait("PARAIMMUNE") && (has_effect("dermatik") || has_effect("tapeworm") ||
          has_effect("bloodworms") || has_effect("brainworm") || has_effect("paincysts")) ) {
        remove_effect("dermatik");
        remove_effect("tapeworm");
        remove_effect("bloodworms");
        remove_effect("brainworm");
        remove_effect("paincysts");
        add_msg_if_player(m_good, _("Something writhes and inside of you as it dies."));
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
            bool reduced = has_trait(it.get_resist_trait()) || has_effect(it.get_resist_effect());
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
                    mod_healthy_mod(bound_mod_to_vals(get_healthy_mod(), val,
                                it.get_max_val("H_MOD", reduced), it.get_min_val("H_MOD", reduced)));
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
                    hunger += bound_mod_to_vals(hunger, val, it.get_max_val("HUNGER", reduced),
                                                it.get_min_val("HUNGER", reduced));
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
            if (val != 0) {
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

            // Handle stat changes
            mod_str_bonus(it.get_mod("STR", reduced));
            mod_dex_bonus(it.get_mod("DEX", reduced));
            mod_per_bonus(it.get_mod("PER", reduced));
            mod_int_bonus(it.get_mod("INT", reduced));
            // Speed is already added in recalc_speed_bonus

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
        }
    }

    Creature::process_effects();
}

void player::hardcoded_effects(effect &it)
{
    std::string id = it.get_id();
    int dur = it.get_duration();
    int intense = it.get_intensity();
    body_part bp = it.get_bp();
    bool sleeping = has_effect("sleep");
    bool msg_trig = one_in(400);
    if (id == "onfire") {
        // TODO: this should be determined by material properties
        if (!has_trait("M_SKIN2")) {
            hurtall(3);
        }
        for (size_t i = 0; i < worn.size(); i++) {
            item tmp = worn[i];
            bool burnVeggy = (tmp.made_of("veggy") || tmp.made_of("paper"));
            bool burnFabric = ((tmp.made_of("cotton") || tmp.made_of("wool")) && one_in(10));
            bool burnPlastic = ((tmp.made_of("plastic")) && one_in(50));
            if (burnVeggy || burnFabric || burnPlastic) {
                worn.erase(worn.begin() + i);
                if (i != 0) {
                    i--;
                }
            }
        }
    } else if (id == "spores") {
        // Equivalent to X in 150000 + health * 100
        if ((!has_trait("M_IMMUNE")) && (one_in(100) && x_in_y(intense, 150 + get_healthy() / 10)) ) {
            add_effect("fungus", 1, num_bp, true);
        }
    } else if (id == "fungus") {
        int bonus = get_healthy() / 10 + (has_trait(it.get_resist_trait()) ? 100 : 0);
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
                hunger += awfulness;
                thirst += awfulness;
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
                int sporex, sporey;
                monster spore(GetMType("mon_spore"));
                for (int i = -1; i <= 1; i++) {
                    for (int j = -1; j <= 1; j++) {
                        if (i == 0 && j == 0) {
                            continue;
                        }
                        sporex = posx + i;
                        sporey = posy + j;
                        if (g->m.move_cost(sporex, sporey) > 0) {
                            const int zid = g->mon_at(sporex, sporey);
                            if (zid >= 0) {  // Spores hit a monster
                                if (g->u.sees(sporex, sporey) &&
                                      !g->zombie(zid).type->in_species("FUNGUS")) {
                                    add_msg(_("The %s is covered in tiny spores!"),
                                               g->zombie(zid).name().c_str());
                                }
                                monster &critter = g->zombie( zid );
                                if( !critter.make_fungus() ) {
                                    critter.die( this ); // Counts as kill by player
                                }
                            } else if (one_in(4) && g->num_zombies() <= 1000){
                                spore.spawn(sporex, sporey);
                                g->add_zombie(spore);
                            }
                        }
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
            g->m.add_field(posx, posy, playerBloodType(), 1);
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
                hunger -= 2;
                if (one_in(6)) {
                    vomit();
                    if (one_in(2)) {
                        // we've vomited enough for now
                        puked = true;
                    }
                }
            }
            if (is_npc() && one_in(200)) {
                std::string npcText;
                switch(rng(1,4)) {
                    case 1:
                        npcText = "\"I think it's starting to kick in.\"";
                        break;
                    case 2:
                        npcText = "\"Oh God, what's happening?\"";
                        break;
                    case 3:
                        npcText = "\"Of course... it's all fractals!\"";
                        break;
                    default:
                        npcText = "\"Huh?  What was that?\"";
                        break;

                }
                int loudness = 20 + str_cur - int_cur;
                loudness = (loudness > 5 ? loudness : 5);
                loudness = (loudness < 30 ? loudness : 30);
                g->sound(posx, posy, loudness, _(npcText.c_str()));
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
            int num_insects = rng(1, std::min(3, str_max / 3));
            apply_damage( nullptr, bp, rng( 2, 4 ) * num_insects );
            // Figure out where they may be placed
            add_msg_player_or_npc( m_bad, _("Your flesh crawls; insects tear through the flesh and begin to emerge!"),
                _("Insects begin to emerge from <npcname>'s skin!") );
            monster grub(GetMType("mon_dermatik_larva"));
            for (int i = posx - 1; i <= posx + 1; i++) {
                for (int j = posy - 1; j <= posy + 1; j++) {
                    if (num_insects == 0) {
                        break;
                    } else if (i == 0 && j == 0) {
                        continue;
                    }
                    if (g->mon_at(i, j) == -1) {
                        grub.spawn(i, j);
                        if (one_in(3)) {
                            grub.friendly = -1;
                        } else {
                            grub.friendly = 0;
                        }
                        g->add_zombie(grub);
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
            MonsterGroupResult spawn_details = MonsterGroupManager::GetResultFromGroup("GROUP_NETHER");
            monster beast(GetMType(spawn_details.name));
            int x, y;
            int tries = 0;
            do {
                x = posx + rng(-4, 4);
                y = posy + rng(-4, 4);
                tries++;
            } while (((x == posx && y == posy) || g->mon_at(x, y) != -1) && tries < 10);
            if (tries < 10) {
                if (g->m.move_cost(x, y) == 0) {
                    g->m.make_rubble(x, y, f_rubble_rock, true);
                }
                beast.spawn(x, y);
                g->add_zombie(beast);
                if (g->u.sees(x, y)) {
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
                add_msg_if_player(m_bad, _("You feel paranoid. They're watching you."));
                mod_pain(1);
                fatigue += dice(1,6);
            } else if (one_in(500)) {
                add_msg_if_player(m_bad, _("You feel like you need less teeth. You pull one out, and it is rotten to the core."));
                mod_pain(1);
            } else if (one_in(500)) {
                add_msg_if_player(m_bad, _("You notice a large abscess. You pick at it."));
                body_part itch = random_body_part(true);
                add_effect("formication", 600, itch);
                mod_pain(1);
            } else if (one_in(500)) {
                add_msg_if_player(m_bad, _("You feel so sick, like you've been poisoned, but you need more. So much more."));
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
                MonsterGroupResult spawn_details = MonsterGroupManager::GetResultFromGroup("GROUP_NETHER");
                monster beast(GetMType(spawn_details.name));
                int x, y;
                int tries = 0;
                do {
                    x = posx + rng(-4, 4);
                    y = posy + rng(-4, 4);
                    tries++;
                    if (tries >= 10) {
                        break;
                    }
                } while (((x == posx && y == posy) || g->mon_at(x, y) != -1));
                if (tries < 10) {
                    if (g->m.move_cost(x, y) == 0) {
                        g->m.make_rubble(x, y, f_rubble_rock, true);
                    }
                    beast.spawn(x, y);
                    g->add_zombie(beast);
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
            hurtall(500);
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
    } else if (id == "brainworm") {
        if (one_in(256)) {
            add_msg_if_player(m_bad, _("Your head aches faintly."));
        }
        if(one_in(1024)) {
            mod_healthy_mod(-10);
            apply_damage( nullptr, bp_head, rng( 0, 1 ) );
            if (!has_effect("visuals")) {
                add_msg_if_player(m_bad, _("Your vision is getting fuzzy."));
                add_effect("visuals", rng(10, 600));
            }
        }
        if(one_in(4096)) {
            mod_healthy_mod(-10);
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
                add_effect("downed",rng(1,4));
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
                add_effect("downed",rng(1,4));
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
        // Set ourselves up for removal
        it.set_duration(0);
    } else if (id == "bite") {
        bool recovered = false;
        // Recovery chance
        if (dur % 10 == 0)  {
            int recover_factor = 100;
            if (has_effect("recover")) {
                recover_factor -= get_effect_dur("recover") / 600;
            }
            if (has_trait("INFRESIST")) {
                recover_factor += 1000;
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
        // Recovery chance
        if (dur % 10 == 0)  {
            int recover_factor = 100;
            if (has_effect("recover")) {
                recover_factor -= get_effect_dur("recover") / 600;
            }
            if (has_trait("INFRESIST")) {
                recover_factor += 1000;
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
                add_msg(m_bad, _("You succumb to the infection."));
                add_memorial_log(pgettext("memorial_male", "Succumbed to the infection."),
                                      pgettext("memorial_female", "Succumbed to the infection."));
                hurtall(500);
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
            if (has_trait("HIBERNATE") && (hunger < -60)) {
                add_memorial_log(pgettext("memorial_male", "Entered hibernation."),
                                   pgettext("memorial_female", "Entered hibernation."));
                // 10 days' worth of round-the-clock Snooze.  Cata seasons default to 14 days.
                fall_asleep(144000);
            }
            // If you're not fatigued enough for 10 days, you won't sleep the whole thing.
            // In practice, the fatigue from filling the tank from (no msg) to Time For Bed
            // will last about 8 days.
            if (hunger >= -60) {
                fall_asleep(6000); //10 hours, default max sleep time.
            }
            // Set ourselves up for removal
            it.set_duration(0);
        }
        if (dur == 1 && !sleeping) {
            add_msg_if_player(_("You try to sleep, but can't..."));
        }
    } else if (id == "sleep") {
        set_moves(0);
        // Hibernating only kicks in whilst Engorged; separate tracking for hunger/thirst here
        // as a safety catch.  One test subject managed to get two Colds during hibernation;
        // since those add fatigue and dry out the character, the subject went for the full 10 days plus
        // a little, and came out of it well into Parched.  Hibernating shouldn't endanger your
        // life like that--but since there's much less fluid reserve than food reserve,
        // simply using the same numbers won't work.
        if((int(calendar::turn) % 350 == 0) && has_trait("HIBERNATE") && !(hunger > -60) && !(thirst >= 80)) {
            int recovery_chance;
            // Hibernators' metabolism slows down: you heal and recover Fatigue much more slowly.
            // Accelerated recovery capped to 2x over 2 hours...well, it was ;-P
            // After 16 hours of activity, equal to 7.25 hours of rest
            if (intense < 24) {
                it.mod_intensity(1);
            } else if (intense < 1) {
                it.set_intensity(1);
            }
            recovery_chance = 24 - intense + 1;
            if (fatigue > 0) {
                fatigue -= 1 + one_in(recovery_chance);
            }
            int heal_chance = get_healthy() / 4;
            if ((has_trait("FLIMSY") && x_in_y(3, 4)) || (has_trait("FLIMSY2") && one_in(2)) ||
                  (has_trait("FLIMSY3") && one_in(4)) ||
                  (!has_trait("FLIMSY") && !has_trait("FLIMSY2") && !has_trait("FLIMSY3"))) {
                if (has_trait("FASTHEALER") || has_trait("MET_RAT")) {
                    heal_chance += 100;
                } else if (has_trait("FASTHEALER2")) {
                    heal_chance += 150;
                } else if (has_trait("REGEN")) {
                    heal_chance += 200;
                } else if (has_trait("SLOWHEALER")) {
                    heal_chance += 13;
                } else {
                    heal_chance += 25;
                }
                healall(heal_chance / 100);
                heal_chance %= 100;
                if (x_in_y(heal_chance, 100)) {
                    healall(1);
                }
            }

            if (fatigue <= 0 && fatigue > -20) {
                fatigue = -25;
                add_msg_if_player(m_good, _("You feel well rested."));
                it.set_duration(dice(3, 100));
                add_memorial_log(pgettext("memorial_male", "Awoke from hibernation."),
                                   pgettext("memorial_female", "Awoke from hibernation."));
            }

        // If you hit Very Thirsty, you kick up into regular Sleep as a safety precaution.
        // See above.  No log note for you. :-/
        } else if(int(calendar::turn) % 50 == 0) {
            // Accelerated recovery capped to 2x over 2 hours
            // After 16 hours of activity, equal to 7.25 hours of rest
            if (intense < 24) {
                it.mod_intensity(1);
            } else if (intense < 1) {
                it.set_intensity(1);
            }

            auto const recovery_chance = 24 - intense + 1;

            if (fatigue > 0) {
                auto delta = 1.0 + (one_in(recovery_chance) ? 1.0 : 0.0);

                // You fatigue & recover faster with Sleepy
                // Very Sleepy, you just fatigue faster
                if (has_trait("SLEEPY") || has_trait("MET_RAT")) {
                    auto const roll = (one_in(recovery_chance) ? 1.0 : 0.0);
                    delta += (1.0 + roll) / 2.0;
                }

                // Tireless folks recover fatigue really fast
                // as well as gaining it really slowly
                // (Doesn't speed healing any, though...)
                if (has_trait("WAKEFUL3")) {
                    auto const roll = (one_in(recovery_chance) ? 1.0 : 0.0);
                    delta += (2.0 + roll) / 2.0;
                }

                fatigue -= static_cast<int>(std::round(delta));
            }

            int heal_chance = get_healthy() / 4;
            if ((has_trait("FLIMSY") && x_in_y(3, 4)) || (has_trait("FLIMSY2") && one_in(2)) ||
                  (has_trait("FLIMSY3") && one_in(4)) ||
                  (!has_trait("FLIMSY") && !has_trait("FLIMSY2") && !has_trait("FLIMSY3"))) {
                if (has_trait("FASTHEALER") || has_trait("MET_RAT")) {
                    heal_chance += 100;
                } else if (has_trait("FASTHEALER2")) {
                    heal_chance += 150;
                } else if (has_trait("REGEN")) {
                    heal_chance += 200;
                } else if (has_trait("SLOWHEALER")) {
                    heal_chance += 13;
                } else {
                    heal_chance += 25;
                }
                healall(heal_chance / 100);
                heal_chance %= 100;
                if (x_in_y(heal_chance, 100)) {
                    healall(1);
                }
            }

            if (fatigue <= 0 && fatigue > -20) {
                fatigue = -25;
                add_msg_if_player(m_good, _("You feel well rested."));
                it.set_duration(dice(3, 100));
            }
        }

        if (int(calendar::turn) % 100 == 0 && !has_bionic("bio_recycler") && !(hunger < -60)) {
            // Hunger and thirst advance more slowly while we sleep. This is the standard rate.
            hunger--;
            thirst--;
        }

        // Hunger and thirst advance *much* more slowly whilst we hibernate.
        // (int (calendar::turn) % 50 would be zero burn.)
        // Very Thirsty catch deliberately NOT applied here, to fend off Dehydration debuffs
        // until the char wakes.  This was time-trial'd quite thoroughly,so kindly don't "rebalance"
        // without a good explanation and taking a night to make sure it works
        // with the extended sleep duration, OK?
        if (int(calendar::turn) % 70 == 0 && !has_bionic("bio_recycler") && (hunger < -60)) {
            hunger--;
            thirst--;
        }

        if (int(calendar::turn) % 100 == 0 && has_trait("CHLOROMORPH") &&
        g->is_in_sunlight(xpos(), ypos()) ) {
            // Hunger and thirst fall before your Chloromorphic physiology!
            if (hunger >= -30) {
                hunger -= 5;
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
                        hunger += 10;
                        fatigue += 5;
                        thirst += 10;
                    }
                }
            }
        }

        bool woke_up = false;
        int tirednessVal = rng(5, 200) + rng(0, abs(fatigue * 2 * 5));
        if (has_trait("HEAVYSLEEPER2") && !has_trait("HIBERNATE")) {
            // So you can too sleep through noon
            if ((tirednessVal * 1.25) < g->light_level() && (fatigue < 10 || one_in(fatigue / 2))) {
                add_msg_if_player(_("It's too bright to sleep."));
                // Set ourselves up for removal
                it.set_duration(0);
                woke_up = true;
            }
         // Ursine hibernators would likely do so indoors.  Plants, though, might be in the sun.
        } else if (has_trait("HIBERNATE")) {
            if ((tirednessVal * 5) < g->light_level() && (fatigue < 10 || one_in(fatigue / 2))) {
                add_msg_if_player(_("It's too bright to sleep."));
                // Set ourselves up for removal
                it.set_duration(0);
                woke_up = true;
            }
        } else if (tirednessVal < g->light_level() && (fatigue < 10 || one_in(fatigue / 2))) {
            add_msg(_("It's too bright to sleep."));
            // Set ourselves up for removal
            it.set_duration(0);
            woke_up = true;
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
                    if(!g->sound(xpos(), ypos(), 12, _("beep-beep-beep!"))) {
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

    for (auto mut : my_mutations) {
        if (!traits[mut].powered ) {
            continue;
        }
        if (traits[mut].powered && traits[mut].charge > 0) {
        // Already-on units just lose a bit of charge
        traits[mut].charge--;
        } else {
            // Not-on units, or those with zero charge, have to pay the power cost
            if (traits[mut].cooldown > 0) {
                traits[mut].powered = true;
                traits[mut].charge = traits[mut].cooldown - 1;
            }
            if (traits[mut].hunger){
                hunger += traits[mut].cost;
                if (hunger >= 700) { // Well into Famished
                    add_msg(m_warning, _("You're too famished to keep your %s going."), traits[mut].name.c_str());
                    traits[mut].powered = false;
                    traits[mut].cooldown = traits[mut].cost;
                }
            }
            if (traits[mut].thirst){
                thirst += traits[mut].cost;
                if (thirst >= 260) { // Well into Dehydrated
                    add_msg(m_warning, _("You're too dehydrated to keep your %s going."), traits[mut].name.c_str());
                    traits[mut].powered = false;
                    traits[mut].cooldown = traits[mut].cost;
                }
            }
            if (traits[mut].fatigue){
                fatigue += traits[mut].cost;
                if (fatigue >= 575) { // Exhausted
                    add_msg(m_warning, _("You're too exhausted to keep your %s going."), traits[mut].name.c_str());
                    traits[mut].powered = false;
                    traits[mut].cooldown = traits[mut].cost;
                }
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
        if (oxygen < 0) {
            if (has_bionic("bio_gills") && power_level >= 25) {
                oxygen += 5;
                power_level -= 25;
            } else {
                add_msg(m_bad, _("You're drowning!"));
                apply_damage( nullptr, bp_torso, rng( 1, 4 ) );
            }
        }
    }

    if(has_active_mutation("WINGS_INSECT")){
        //~Sound of buzzing Insect Wings
        g->sound(posx, posy, 10, "BZZZZZ");
    }

    double shoe_factor = footwear_factor();
    if( has_trait("ROOTS3") && g->m.has_flag("DIGGABLE", posx, posy) && !shoe_factor) {
        if (one_in(100)) {
            add_msg(m_good, _("This soil is delicious!"));
            if (hunger > -20) {
                hunger -= 2;
            }
            if (thirst > -20) {
                thirst -= 2;
            }
            mod_healthy_mod(10);
            // No losing oneself in the fertile embrace of rich
            // New England loam.  But it can be a near thing.
            if ( (one_in(int_cur)) && (focus_pool >= 25) ) {
                focus_pool--;
            }
        } else if (one_in(50)){
            if (hunger > -20) {
                hunger--;
            }
            if (thirst > -20) {
                thirst--;
            }
            mod_healthy_mod(5);
        }
    }

    for( auto &elem : illness ) {
        if( elem.duration <= 0 ) {
            continue;
        }
        dis_effect( *this, elem );
    }

    // Diseases may remove themselves as part of applying (MA buffs do) so do a
    // separate loop through the remaining ones for duration, decay, etc..
    for( auto it = illness.begin(); it != illness.end(); ) {
        auto &next_illness = *it;
        if( next_illness.duration < 0 ) {
            it = illness.erase( it );
            continue;
        }
        if (!next_illness.permanent) {
            next_illness.duration--;
        }
        if (next_illness.decay > 0 && one_in(next_illness.decay)) {
            next_illness.intensity--;
        }
        if (next_illness.duration <= 0 || next_illness.intensity == 0) {
            it = illness.erase( it );
        } else {
            it++;
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
            if (has_trait("LEG_TENT_BRACE")){
                add_msg_if_player(m_bad, _("Your tentacles buckle under the weight!"));
            }
            if (has_effect("downed")) {
                add_effect("downed", 1);
            } else {
                add_effect("downed", 2);
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
                addict_effect(addictions[i]);
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
                hunger += hungadd;
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
                    g->sound(posx, posy, 10 + 2 * str_cur, "AHHHHHHH!");
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
            } else if (hunger > 80 && one_in(500 - hunger)) {
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
            g->sound(posx, posy, 10 + 2 * str_cur, _("You shout loudly!"));
        }
        if (has_trait("SHOUT2") && one_in(2400)) {
            g->sound(posx, posy, 15 + 3 * str_cur, _("You scream loudly!"));
        }
        if (has_trait("SHOUT3") && one_in(1800)) {
            g->sound(posx, posy, 20 + 4 * str_cur, _("You let out a piercing howl!"));
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

    if (has_trait("LEAVES") && g->is_in_sunlight(posx, posy) && one_in(600)) {
        hunger--;
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

    if ((has_trait("ALBINO") || has_effect("datura")) && g->is_in_sunlight(posx, posy) && one_in(10)) {
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

    if (has_trait("SUNBURN") && g->is_in_sunlight(posx, posy) && one_in(10)) {
        if (!((worn_with_flag("RAINPROOF")) || (weapon.has_flag("RAIN_PROTECT"))) ) {
        add_msg(m_bad, _("The sunlight burns your skin!"));
        if (in_sleep_state()) {
            wake_up();
        }
        mod_pain(1);
        hurtall(1);
        }
    }

    if ((has_trait("TROGLO") || has_trait("TROGLO2")) &&
        g->is_in_sunlight(posx, posy) && g->weather == WEATHER_SUNNY) {
        mod_str_bonus(-1);
        mod_dex_bonus(-1);
        add_miss_reason(_("The sunlight distracts you."), 1);
        mod_int_bonus(-1);
        mod_per_bonus(-1);
    }
    if (has_trait("TROGLO2") && g->is_in_sunlight(posx, posy)) {
        mod_str_bonus(-1);
        mod_dex_bonus(-1);
        add_miss_reason(_("The sunlight distracts you."), 1);
        mod_int_bonus(-1);
        mod_per_bonus(-1);
    }
    if (has_trait("TROGLO3") && g->is_in_sunlight(posx, posy)) {
        mod_str_bonus(-4);
        mod_dex_bonus(-4);
        add_miss_reason(_("You can't stand the sunlight!"), 4);
        mod_int_bonus(-4);
        mod_per_bonus(-4);
    }

    if (has_trait("SORES")) {
        for (int i = bp_head; i < num_bp; i++) {
            if ((pain < 5 + 4 * abs(encumb(body_part(i)))) && (!(has_trait("NOPAIN")))) {
                pain = 0;
                mod_pain( 5 + 4 * abs(encumb(body_part(i))) );
            }
        }
    }

    if (has_trait("SLIMY") && !in_vehicle) {
        g->m.add_field(posx, posy, fd_slime, 1);
    }
        //Web Weavers...weave web
    if (has_active_mutation("WEB_WEAVER") && !in_vehicle) {
      g->m.add_field(posx, posy, fd_web, 1); //this adds density to if its not already there.

     }

    if (has_trait("VISCOUS") && !in_vehicle) {
        if (one_in(3)){
            g->m.add_field(posx, posy, fd_slime, 1);
        }
        else {
            g->m.add_field(posx, posy, fd_slime, 2);
        }
    }

    // Blind/Deaf for brief periods about once an hour,
    // and visuals about once every 30 min.
    if (has_trait("PER_SLIME")) {
        if (one_in(600) && !has_effect("deaf")) {
            add_msg(m_bad, _("Suddenly, you can't hear anything!"));
            add_effect("deaf", 20 * rng (2, 6)) ;
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
        g->m.add_field(posx, posy, fd_web, 1); //this adds density to if its not already there.
    }

    if (has_trait("RADIOGENIC") && int(calendar::turn) % 50 == 0 && radiation > 0) {
        if (radiation > 10) {
            radiation -= 10;
            healall(1);
        } else {
            if(x_in_y(radiation, 10)) {
                healall(1);
            }
            radiation = 0;
        }
    }

    if (has_trait("RADIOACTIVE1")) {
        if (g->m.get_radiation(posx, posy) < 10 && one_in(50)) {
            g->m.adjust_radiation(posx, posy, 1);
        }
    }
    if (has_trait("RADIOACTIVE2")) {
        if (g->m.get_radiation(posx, posy) < 20 && one_in(25)) {
            g->m.adjust_radiation(posx, posy, 1);
        }
    }
    if (has_trait("RADIOACTIVE3")) {
        if (g->m.get_radiation(posx, posy) < 30 && one_in(10)) {
            g->m.adjust_radiation(posx, posy, 1);
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

    int localRadiation = g->m.get_radiation(posx, posy);

    if (localRadiation || selfRadiation) {
        bool has_helmet = false;

        bool power_armored = is_wearing_power_armor(&has_helmet);

        if ((power_armored && has_helmet) || worn_with_flag("RAD_PROOF")) {
            radiation += 0; // Power armor protects completely from radiation
        } else if (power_armored || worn_with_flag("RAD_RESIST")) {
            radiation += rng(0, localRadiation / 40) + rng(0, selfRadiation / 5);
        } else {
            radiation += rng(0, localRadiation / 16) + rng(0, selfRadiation);
        }

        // Apply rads to any radiation badges.
        for (item *const it : inv_dump()) {
            if (it->type->id != "rad_badge") {
                continue;
            }

            // Actual irradiation levels of badges and the player aren't precisely matched.
            // This is intentional.
            int const before = it->irridation;
            it->irridation += rng(0, localRadiation / 16);

            // If in inventory (not worn), don't print anything.
            if (inv.has_item(it)) {
                continue;
            }

            // If the color hasn't changed, don't print anything.
            auto const &col_before = rad_badge_color(before);
            auto const &col_after  = rad_badge_color(it->irridation);
            if (before == it->irridation || col_before == col_after) {
                continue;
            }

            add_msg_if_player(m_warning, _("Your radiation badge changes from %s to %s!"),
                _(col_before.c_str()), _(col_after.c_str()) );
        }
    }

    if( int(calendar::turn) % 150 == 0 ) {
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

    if( radiation > 150 && !(int(calendar::turn) % 90) ) {
        hurtall(radiation / 100);
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
        hurtall(1);
    }
    if (has_bionic("bio_drain") && power_level > 24 && one_in(600)) {
        add_msg(m_bad, _("Your batteries discharge slightly."));
        power_level -= 25;
    }
    if (has_bionic("bio_noise") && one_in(500)) {
        if(!is_deaf())
            add_msg(m_bad, _("A bionic emits a crackle of noise!"));
        else
            add_msg(m_bad, _("A bionic shudders, but you hear nothing."));
        g->sound(posx, posy, 60, "");
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
        add_effect("downed", 1);
    }
    if (has_bionic("bio_shakes") && power_level > 24 && one_in(1200)) {
        add_msg(m_bad, _("Your bionics short-circuit, causing you to tremble and shiver."));
        power_level -= 25;
        add_effect("shakes", 50);
    }
    if (has_bionic("bio_leaky") && one_in(500)) {
        mod_healthy_mod(-50);
    }
    if (has_bionic("bio_sleepy") && one_in(500)) {
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

    // check for limb mending every 1000 turns (~1.6 hours)
    if(calendar::turn.get_turn() % 1000 == 0) {
        mend();
    }
}

void player::mend()
{
    // Wearing splints can slowly mend a broken limb back to 1 hp.
    // 2 weeks is faster than a fracture would heal IRL,
    // but 3 weeks average (a generous estimate) was tedious and no fun.
    for(int i = 0; i < num_hp_parts; i++) {
        int broken = (hp_cur[i] <= 0);
        if(broken) {
            double mending_odds = 200.0; // 2 weeks, on average. (~20160 minutes / 100 minutes)
            double healing_factor = 1.0;
            if (has_trait("REGEN_LIZ")) {
                healing_factor = 20.0;
            }
            // Studies have shown that alcohol and tobacco use delay fracture healing time
            if(has_effect("cig") || addiction_level(ADD_CIG)) {
                healing_factor *= 0.5;
            }
            if(has_effect("drunk") || addiction_level(ADD_ALCOHOL)) {
                healing_factor *= 0.5;
            }

            // Bed rest speeds up mending
            if(has_effect("sleep")) {
                healing_factor *= 4.0;
            } else if(fatigue > 383) {
            // but being dead tired does not...
                healing_factor *= 0.75;
            }

            // Being healthy helps.
            if(get_healthy() > 0) {
                healing_factor *= 2.0;
            }

            // And being well fed...
            if(hunger < 0) {
                healing_factor *= 2.0;
            }

            if(thirst < 0) {
                healing_factor *= 2.0;
            }

            // Mutagenic healing factor!
            if(has_trait("REGEN")) {
                healing_factor *= 16.0;
            } else if (has_trait("FASTHEALER2")) {
                healing_factor *= 4.0;
            } else if (has_trait("FASTHEALER")) {
                healing_factor *= 2.0;
            } else if (has_trait("SLOWHEALER")) {
                healing_factor *= 0.5;
            }

            bool mended = false;
            body_part part;
            switch(i) {
                case hp_arm_r:
                    part = bp_arm_r;
                    mended = is_wearing_on_bp("arm_splint", bp_arm_r) && x_in_y(healing_factor, mending_odds);
                    if (mended == false && has_trait("REGEN_LIZ")) {
                        healing_factor *= 0.2; // Splints aren't *strictly* necessary for your anatomy
                        mended = x_in_y(healing_factor, mending_odds);
                    }
                    break;
                case hp_arm_l:
                    part = bp_arm_l;
                    mended = is_wearing_on_bp("arm_splint", bp_arm_l) && x_in_y(healing_factor, mending_odds);
                    if (mended == false && has_trait("REGEN_LIZ")) {
                        healing_factor *= 0.2; // But without them, you're looking at a much longer recovery.
                        mended = x_in_y(healing_factor, mending_odds);
                    }
                    break;
                case hp_leg_r:
                    part = bp_leg_r;
                    mended = is_wearing_on_bp("leg_splint", bp_leg_r) && x_in_y(healing_factor, mending_odds);
                    if (mended == false && has_trait("REGEN_LIZ")) {
                        healing_factor *= 0.2;
                        mended = x_in_y(healing_factor, mending_odds);
                    }
                    break;
                case hp_leg_l:
                    part = bp_leg_l;
                    mended = is_wearing_on_bp("leg_splint", bp_leg_l) && x_in_y(healing_factor, mending_odds);
                    if (mended == false && has_trait("REGEN_LIZ")) {
                        healing_factor *= 0.2;
                        mended = x_in_y(healing_factor, mending_odds);
                    }
                    break;
                default:
                    // No mending for you!
                    break;
            }
            if(mended) {
                hp_cur[i] = 1;
                //~ %s is bodypart
                add_memorial_log(pgettext("memorial_male", "Broken %s began to mend."),
                                  pgettext("memorial_female", "Broken %s began to mend."),
                                  body_part_name(part).c_str());
                //~ %s is bodypart
                add_msg(m_good, _("Your %s has started to mend!"),
                body_part_name(part).c_str());
            }
        }
    }
}

void player::vomit()
{
    add_memorial_log(pgettext("memorial_male", "Threw up."),
                     pgettext("memorial_female", "Threw up."));

    if (stomach_food != 0 || stomach_water != 0) {
        add_msg(m_bad, _("You throw up heavily!"));
    } else {
        add_msg(m_warning, _("You feel nauseous, but your stomach is empty."));
    }
    int nut_loss = stomach_food;
    int quench_loss = stomach_water;
    stomach_food = 0;
    stomach_water = 0;
    hunger += nut_loss;
    thirst += quench_loss;
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

void player::drench(int saturation, int flags)
{
    // OK, water gets in your AEP suit or whatever.  It wasn't built to keep you dry.
    if ( (has_trait("DEBUG_NOTEMP")) || (has_active_mutation("SHELL2")) ||
      ((is_waterproof(flags)) && (!(g->m.has_flag(TFLAG_DEEP_WATER, posx, posy)))) ) {
        return;
    }

    // Make the body wet
    for (int i = 0; i < num_bp; ++i) {
        // Different body parts have different size, they can only store so much water
        int bp_wetness_max = 0;
        if (mfb(i) & flags){
            bp_wetness_max = mDrenchEffect[i];
        }
        if (bp_wetness_max == 0){
            continue;
        }
        // Different sources will only make the bodypart wet to a limit
        int source_wet_max = saturation / 2;
        int wetness_increment = source_wet_max / 8;
        // Make sure increment is at least 1
        if (source_wet_max != 0 && wetness_increment == 0) {
            wetness_increment = 1;
        }
        // Respect maximums
        if (body_wetness[i] < source_wet_max && body_wetness[i] < bp_wetness_max){
            body_wetness[i] += wetness_increment;
        }
    }

    // Apply morale results from getting wet
    int effected = 0;
    int tot_ignored = 0; //Always ignored
    int tot_neut = 0; //Ignored for good wet bonus
    int tot_good = 0; //Increase good wet bonus

    for (std::map<int, int>::iterator iter = mDrenchEffect.begin(); iter != mDrenchEffect.end(); ++iter) {
        if (mfb(iter->first) & flags) {
            effected += iter->second;
            tot_ignored += mMutDrench[iter->first]["ignored"];
            tot_neut += mMutDrench[iter->first]["neutral"];
            tot_good += mMutDrench[iter->first]["good"];
        }
    }

    if (effected == 0) {
        return;
    }

    bool wants_drench = false;
    // If not protected by mutations then check clothing
    if (tot_good + tot_neut + tot_ignored < effected) {
        wants_drench = is_water_friendly(flags);
    } else {
        wants_drench = true;
    }

    int morale_cap;
    if (wants_drench) {
        morale_cap = g->get_temperature() - std::min(65, 65 + (tot_ignored - tot_good) / 2) * saturation / 100;
    } else {
        morale_cap = -(saturation / 2);
    }

    // Good increases pos and cancels neg, neut cancels neg, ignored cancels both
    if (morale_cap > 0) {
        morale_cap = morale_cap * (effected - tot_ignored + tot_good) / effected;
    } else if (morale_cap < 0) {
        morale_cap = morale_cap * (effected - tot_ignored - tot_neut - tot_good) / effected;
    }

    if (morale_cap == 0) {
        return;
    }

    int morale_effect = morale_cap / 8;
    if (morale_effect == 0) {
        if (morale_cap > 0) {
            morale_effect = 1;
        } else {
            morale_effect = -1;
        }
    }

    int dur = 60;
    int d_start = 30;
    if (morale_cap < 0) {
        if (has_trait("LIGHTFUR") || has_trait("FUR") || has_trait("FELINE_FUR") ||
          has_trait("LUPINE_FUR") || has_trait("CHITIN_FUR") || has_trait("CHITIN_FUR2") ||
          has_trait("CHITIN_FUR3")) {
            dur /= 5;
            d_start /= 5;
        }
        // Shaggy fur holds water longer.  :-/
        if (has_trait("URSINE_FUR")) {
            dur /= 3;
            d_start /= 3;
        }
    } else {
        if (has_trait("SLIMY")) {
            dur *= 1.2;
            d_start *= 1.2;
        }
    }

    add_morale(MORALE_WET, morale_effect, morale_cap, dur, d_start);
}

void player::drench_mut_calc()
{
    mMutDrench.clear();
    int ignored, neutral, good;

    for( auto &elem : mDrenchEffect ) {
        ignored = 0;
        neutral = 0;
        good = 0;

        for( const auto &_iter : my_mutations ) {
            for( auto &_wp_iter : mutation_data[_iter].protection ) {
                if( body_parts[_wp_iter.first] == elem.first ) {
                    ignored += _wp_iter.second.second.x;
                    neutral += _wp_iter.second.second.y;
                    good += _wp_iter.second.second.z;
                }
            }
        }

        mMutDrench[elem.first]["good"] = good;
        mMutDrench[elem.first]["neutral"] = neutral;
        mMutDrench[elem.first]["ignored"] = ignored;
    }
}

void player::update_body_wetness()
{
    /**
    * Issue : morale and wetness still aren't linked ... they are two seperate things that mostly coincide
    **/

    /*
    * Mutations and weather can affect the duration of the player being wet.
    */
    int delay = 10;
    if( has_trait("LIGHTFUR") || has_trait("FUR") || has_trait("FELINE_FUR") ||
        has_trait("LUPINE_FUR") || has_trait("CHITIN_FUR") || has_trait("CHITIN_FUR2") ||
        has_trait("CHITIN_FUR3")) {
        delay += 2;
    }
    if (has_trait("URSINE_FUR")) {
        delay += 5;
    }
    if (has_trait("SLIMY")) {
        delay -= 5;
    }
    if (g->weather == WEATHER_SUNNY) {
        delay -= 2;
    }

    if (calendar::turn % delay != 0) {
        return;
    }

    for (int i = 0; i < num_bp; ++i) {
        if (body_wetness[i] == 0) {
            continue;
        }
        body_wetness[i] -= 1;
        if (body_wetness[i] < 0) {
            body_wetness[i] = 0;
        }
    }
}

int player::weight_carried() const
{
    int ret = 0;
    ret += weapon.weight();
    for (auto &i : worn) {
        ret += i.weight();
    }
    ret += inv.weight();
    return ret;
}

int player::volume_carried() const
{
    return inv.volume();
}

int player::weight_capacity() const
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

int player::volume_capacity() const
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

double player::convert_weight(int weight)
{
    double ret;
    ret = double(weight);
    if (OPTIONS["USE_METRIC_WEIGHTS"] == "kg") {
        ret /= 1000;
    } else {
        ret /= 453.6;
    }
    return ret;
}

bool player::can_pickVolume(int volume) const
{
    return (volume_carried() + volume <= volume_capacity());
}
bool player::can_pickWeight(int weight, bool safe) const
{
    if (!safe)
    {
        //Player can carry up to four times their maximum weight
        return (weight_carried() + weight <= weight_capacity() * 4);
    }
    else
    {
        return (weight_carried() + weight <= weight_capacity());
    }
}

int player::net_morale(morale_point effect)
{
    double bonus = effect.bonus;

    // If the effect is old enough to have started decaying,
    // reduce it appropriately.
    if (effect.age > effect.decay_start)
    {
        bonus *= logistic_range(effect.decay_start,
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

int player::morale_level()
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
                        bool cap_existing, itype* item_type)
{
    bool placed = false;

    // Search for a matching morale entry.
    for (auto &i : morale) {
        if (i.type == type && i.item_type == item_type) {
            // Found a match!
            placed = true;

            // Scale the morale bonus to its current level.
            if (i.age > i.decay_start) {
                i.bonus *= logistic_range(i.decay_start, i.duration, i.age);
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

void player::rem_morale(morale_type type, itype* item_type)
{
    for( size_t i = 0; i < morale.size(); ++i ) {
        if (morale[i].type == type && morale[i].item_type == item_type) {
            morale.erase(morale.begin() + i);
            break;
        }
    }
}

item& player::i_add(item it)
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

bool player::has_active_item(const itype_id & id) const
{
    return has_item_with( [id]( const item & it ) {
        return it.active && it.typeId() == id;
    } );
}

void player::process_active_items()
{
    if( weapon.needs_processing() && weapon.process( this, pos(), false ) ) {
        weapon = ret_null;
    }

    std::vector<item *> inv_active = inv.active_items();
    for( auto tmp_it : inv_active ) {

        if( tmp_it->process( this, pos(), false ) ) {
            inv.remove_item(tmp_it);
        }
    }

    // worn items
    for (size_t i = 0; i < worn.size(); i++) {
        if( worn[i].needs_processing() && worn[i].process( this, pos(), false ) ) {
            worn.erase(worn.begin() + i);
            i--;
        }
    }

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

    for( size_t i = 0; i < worn.size() && ch_UPS_used < ch_UPS; ++i ) {
        item& worn_item = worn[i];

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

item player::remove_weapon()
{
    if( weapon.active ) {
        weapon.deactivate_charger_gun();
    }
 item tmp = weapon;
 weapon = ret_null;
 return tmp;
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

item player::i_rem(int pos)
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

item player::i_rem(const item *it)
{
    auto tmp = remove_items_with( [&it] (const item &i) { return &i == it; } );
    if( tmp.empty() ) {
        debugmsg( "did not found item %s to remove it!", it->tname().c_str() );
        return ret_null;
    }
    return tmp.front();
}

// Negative positions indicate weapon/clothing, 0 & positive indicate inventory
item& player::i_at(int position)
{
    if (position == -1) {
        return weapon;
    }
    if (position < -1) {
        int worn_index = worn_position_to_index(position);
        if (size_t(worn_index) < worn.size()) {
            return worn[worn_index];
        }
    }
    return inv.find_item(position);
}

int player::invlet_to_position( char invlet ) const
{
    if( is_npc() ) {
        DebugLog( D_WARNING,  D_GAME ) << "Why do you need to call player::invlet_to_position on npc " << name;
    }
    if( weapon.invlet == invlet ) {
        return -1;
    }
    for( size_t i = 0; i < worn.size(); i++ ) {
        if( worn[i].invlet == invlet ) {
            return worn_position_to_index( i );
        }
    }
    return inv.invlet_to_position( invlet );
}

int player::get_item_position(const item* it) {
    if (&weapon == it) {
        return -1;
    }
    for (size_t i = 0; i < worn.size(); i++) {
        if (&worn[i] == it) {
            return worn_position_to_index(i);
        }
    }
    return inv.position_by_item(it);
}


const martialart &player::get_combat_style() const
{
    auto it = martialarts.find( style_selected );
    if( it != martialarts.end() ) {
        return it->second;
    }
    debugmsg( "unknown martial art style %s selected", style_selected.c_str() );
    return martialarts["style_none"];
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

std::list<item> player::use_amount(itype_id it, int quantity, bool use_container)
{
    std::list<item> ret;
    if (weapon.use_amount(it, quantity, use_container, ret)) {
        remove_weapon();
    }
    for( auto a = worn.begin(); a != worn.end() && quantity > 0; ) {
        if( a->use_amount( it, quantity, use_container, ret ) ) {
            a = worn.erase( a );
        } else {
            ++a;
        }
    }
    if (quantity <= 0) {
        return ret;
    }
    std::list<item> tmp = inv.use_amount(it, quantity, use_container);
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

    if (g->m.has_nearby_fire(posx, posy)) {
        return true;
    } else if (has_charges("torch_lit", 1)) {
        return true;
    } else if (has_charges("battletorch_lit", quantity)) {
        return true;
    } else if (has_charges("handflare_lit", 1)) {
        return true;
    } else if (has_charges("candle_lit", 1)) {
        return true;
    } else if (has_bionic("bio_tools")) {
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
// bio_lighter, bio_laser, bio_tools, has_bionic("bio_tools"

    if (g->m.has_nearby_fire(posx, posy)) {
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
    } else if (has_bionic("bio_tools")) {
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
        power_level -= quantity;
        if (power_level < 0) {
            power_level = 0;
        }
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
        power_level -= ch;
        quantity -= ch * 10;
        // TODO: add some(pseudo?) item to resulting list?
    }
    return ret;
}

int player::butcher_factor() const
{
    int result = INT_MIN;
    if (has_bionic("bio_tools")) {
        item tmp( "toolset", 0 );
        result = tmp.butcher_factor();
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

bool player::is_wearing(const itype_id & it) const
{
    for (auto &i : worn) {
        if (i.type->id == it) {
            return true;
        }
    }
    return false;
}

bool player::is_wearing_on_bp(const itype_id & it, body_part bp) const
{
    for (auto &i : worn) {
        if (i.type->id == it && i.covers(bp)) {
            return true;
        }
    }
    return false;
}

bool player::worn_with_flag( std::string flag ) const
{
    for (auto &i : worn) {
        if (i.has_flag( flag )) {
            return true;
        }
    }
    return false;
}

bool player::covered_with_flag(const std::string flag, std::bitset<num_bp> parts) const
{
    std::bitset<num_bp> covered = 0;

    for (std::vector<item>::const_reverse_iterator armorPiece = worn.rbegin(); armorPiece != worn.rend(); ++armorPiece) {
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

bool player::covered_with_flag_exclusively(const std::string flag, std::bitset<num_bp> parts) const
{
    for( const auto &elem : worn ) {
        if( ( elem.get_covered_body_parts() & parts ).any() && !elem.has_flag( flag ) ) {
            return false;
        }
    }
    return true;
}

bool player::is_water_friendly(std::bitset<num_bp> parts) const
{
    return covered_with_flag_exclusively("WATER_FRIENDLY", parts);
}

bool player::is_waterproof(std::bitset<num_bp> parts) const
{
    return covered_with_flag("WATERPROOF", parts);
}

bool player::has_artifact_with(const art_effect_passive effect) const
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

bool player::has_amount(const itype_id &it, int quantity) const
{
    if (it == "toolset")
    {
        return has_bionic("bio_tools");
    }
    return (amount_of(it) >= quantity);
}

int player::amount_of(const itype_id &it) const
{
    if (it == "toolset" && has_bionic("bio_tools")) {
        return 1;
    }
    if (it == "apparatus") {
        if (has_amount("crackpipe", 1) ||
            has_amount("can_drink", 1) ||
            has_amount("pipe_glass", 1) ||
            has_amount("pipe_tobacco", 1)) {
            return 1;
        }
    }
    int quantity = weapon.amount_of(it, true);
    for( const auto &elem : worn ) {
        quantity += elem.amount_of( it, true );
    }
    quantity += inv.amount_of(it);
    return quantity;
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
        if (has_bionic("bio_tools")) {
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

std::set<char> player::allocated_invlets() const {
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

bool player::has_item(int position) {
    return !i_at(position).is_null();
}

bool player::has_item( const item *it ) const
{
    return has_item_with( [&it]( const item & i ) {
        return &i == it;
    } );
}

struct has_mission_item_filter {
    int mission_id;
    bool operator()(const item &it) {
        return it.mission_id == mission_id;
    }
};

bool player::has_mission_item(int mission_id) const
{
    return mission_id != -1 && has_item_with( has_mission_item_filter{ mission_id } );
}

void player::remove_mission_items( int mission_id )
{
    if( mission_id == -1 ) {
        return;
    }
    remove_items_with( has_mission_item_filter { mission_id } );
}

bool player::i_add_or_drop(item& it, int qty) {
    bool retval = true;
    bool drop = false;
    inv.assign_empty_invlet(it);
    for (int i = 0; i < qty; ++i) {
        if (!drop && (!can_pickWeight(it.weight(), !OPTIONS["DANGEROUS_PICKUPS"])
                      || !can_pickVolume(it.volume()))) {
            drop = true;
        }
        if (drop) {
            retval &= g->m.add_item_or_charges(posx, posy, it);
        } else {
            i_add(it);
        }
    }
    return retval;
}

hint_rating player::rate_action_eat(item *it)
{
 //TODO more cases, for HINT_IFFY
 if (it->is_food_container() || it->is_food()) {
  return HINT_GOOD;
 }
 return HINT_CANT;
}

//Returns the amount of charges that were consumed byt he player
int player::drink_from_hands(item& water) {
    int charges_consumed = 0;
    if (query_yn(_("Drink from your hands?")))
        {
            // Create a dose of water no greater than the amount of water remaining.
            item water_temp(water);
            inv.push_back(water_temp);
            // If player is slaked water might not get consumed.
            if (consume(inv.position_by_type(water_temp.typeId())))
            {
                moves -= 350;

                charges_consumed = 1;// for some reason water_temp doesn't seem to have charges consumed, jsut set this to 1
            }
            inv.remove_item(inv.position_by_type(water_temp.typeId()));
        }
    return charges_consumed;
}


bool player::consume(int target_position)
{
    auto &target = i_at( target_position );
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
    it_comest *comest = dynamic_cast<it_comest*>( to_eat->type );

    int amount_used = 1;
    bool was_consumed = false;
    if (comest != NULL) {
        if (comest->comesttype == "FOOD" || comest->comesttype == "DRINK") {
            was_consumed = eat(to_eat, comest);
            if (!was_consumed) {
                return was_consumed;
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
                amount_used = comest->invoke(this, to_eat, false, pos());
                if( amount_used <= 0 ) {
                    return false;
                }
            }
            consume_effects(to_eat, comest);
            moves -= 250;
            was_consumed = true;
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
        } else if (!to_eat->is_food() && !to_eat->is_food_container(this)) {
            if (to_eat->is_book()) {
                if (to_eat->type->book->skill != NULL && !query_yn(_("Really eat %s?"), to_eat->tname().c_str())) {
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
        was_consumed = true;
    }

    if (!was_consumed) {
        return false;
    }

    // Actions after consume
    to_eat->charges -= amount_used;
    if (to_eat->charges <= 0) {
        const bool was_in_container = &target != to_eat;
        i_rem( to_eat );
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
                g->m.add_item_or_charges(posx, posy, inv.remove_item(&target));
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

bool player::eat(item *eaten, it_comest *comest)
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
    bool overeating = (!has_trait("GOURMAND") && hunger < 0 &&
                       comest->nutr >= 5);
    bool hiberfood = (has_active_mutation("HIBERNATE") && (hunger > -60 && thirst > -60 ));
    eaten->calc_rot(pos()); // check if it's rotten before eating!
    bool spoiled = eaten->rotten();

    last_item = itype_id(eaten->type->id);

    if (overeating && !has_trait("HIBERNATE") && !has_trait("EATHEALTH") && !is_npc() &&
        !has_trait("SLIMESPAWNER") && !query_yn(_("You're full.  Force yourself to eat?"))) {
        return false;
    } else if (has_trait("GOURMAND") && hunger < 0 && comest->nutr >= 5) {
        if (!query_yn(_("You're fed.  Try to pack more in anyway?"))) {
            return false;
        }
    }

    int temp_nutr = comest->nutr;
    int temp_quench = comest->quench;
    if (hiberfood && !is_npc() && (((hunger - temp_nutr) < -60) || ((thirst - temp_quench) < -60)) && has_active_mutation("HIBERNATE")){
       if (!query_yn(_("You're adequately fueled. Prepare for hibernation?"))) {
        return false;
       }
       else
       if(!is_npc()) {add_memorial_log(pgettext("memorial_male", "Began preparing for hibernation."),
                                       pgettext("memorial_female", "Began preparing for hibernation."));
                      add_msg(_("You've begun stockpiling calories and liquid for hibernation. You get the feeling that you should prepare for bed, just in case, but...you're hungry again, and you could eat a whole week's worth of food RIGHT NOW."));
      }
    }
    if (has_trait("CARNIVORE") && (eaten->made_of("veggy") || eaten->made_of("fruit") || eaten->made_of("wheat")) &&
      !(eaten->made_of("flesh") ||eaten->made_of("hflesh") || eaten->made_of("iflesh") || eaten->made_of("milk") ||
      eaten->made_of("egg")) && comest->nutr > 0) {
        add_msg_if_player(m_info, _("Eww.  Inedible plant stuff!"));
        return false;
    }
    if ((!has_trait("SAPIOVORE") && !has_trait("CANNIBAL") && !has_trait("PSYCHOPATH")) && eaten->made_of("hflesh")&&
        !is_npc() && !query_yn(_("The thought of eating that makes you feel sick. Really do it?"))) {
        return false;
    }
    if ((!has_trait("SAPIOVORE") && has_trait("CANNIBAL") && !has_trait("PSYCHOPATH") && !has_trait("SPIRITUAL")) && eaten->made_of("hflesh")&& !is_npc() &&
        !query_yn(_("The thought of eating that makes you feel both guilty and excited. Really do it?"))) {
        return false;
    }

    if ((!has_trait("SAPIOVORE") && has_trait("CANNIBAL") && !has_trait("PSYCHOPATH") && has_trait("SPIRITUAL")) &&
         eaten->made_of("hflesh")&& !is_npc() &&
        !query_yn(_("Cannibalism is such a universal taboo.  Will you break it?"))) {
        return false;
    }

    if (has_trait("VEGETARIAN") && eaten->made_of("flesh") && !is_npc() &&
        !query_yn(_("Really eat that %s? Your stomach won't be happy."), eaten->tname().c_str())) {
        return false;
    }
    if (has_trait("MEATARIAN") && eaten->made_of("veggy") && !is_npc() &&
        !query_yn(_("Really eat that %s? Your stomach won't be happy."), eaten->tname().c_str())) {
        return false;
    }
    if (has_trait("LACTOSE") && eaten->made_of("milk") && (!has_bionic("bio_digestion")) && !is_npc() &&
        !query_yn(_("Really eat that %s? Your stomach won't be happy."), eaten->tname().c_str())) {
        return false;
    }
    if (has_trait("ANTIFRUIT") && eaten->made_of("fruit") && !is_npc() &&
        !query_yn(_("Really eat that %s? Your stomach won't be happy."), eaten->tname().c_str())) {
        return false;
    }
    if (has_trait("ANTIJUNK") && eaten->made_of("junk") && (!has_bionic("bio_digestion")) && !is_npc() &&
        !query_yn(_("Really eat that %s? Your stomach won't be happy."), eaten->tname().c_str())) {
        return false;
    }
    if (has_trait("ANTIWHEAT") && eaten->made_of("wheat") &&
        (!has_bionic("bio_digestion")) && !is_npc() &&
        !query_yn(_("Really eat that %s? Your stomach won't be happy."), eaten->tname().c_str())) {
        return false;
    }
    if (has_trait("CARNIVORE") && ((eaten->made_of("junk")) && !(eaten->made_of("flesh") ||
      eaten->made_of("hflesh") || eaten->made_of("iflesh") || eaten->made_of("milk") ||
      eaten->made_of("egg")) ) && (!has_bionic("bio_digestion")) && !is_npc() &&
        !query_yn(_("Really eat that %s? It smells completely unappealing."), eaten->tname().c_str()) ) {
        return false;
    }
    // Check for eating/Food is so water and other basic liquids that do not rot don't cause problems.
    // I'm OK with letting plants drink coffee. (Whether it would count as cannibalism is another story.)
    if ((has_trait("SAPROPHAGE") && (!spoiled) && (!has_bionic("bio_digestion")) && !is_npc() &&
      (eaten->has_flag("USE_EAT_VERB") || comest->comesttype == "FOOD") &&
      !query_yn(_("Really eat that %s? Your stomach won't be happy."), eaten->tname().c_str()))) {
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
    int temp_hunger = hunger - comest->nutr;
    int temp_thirst = thirst - comest->quench;
    int capacity = has_trait("GOURMAND") ? -60 : -20;
    if( has_active_mutation("HIBERNATE") && !is_npc() &&
        // If BOTH hunger and thirst are above the capacity...
        ( hunger > capacity && thirst > capacity ) &&
        // ...and EITHER of them crosses under the capacity...
        ( temp_hunger < capacity || temp_thirst < capacity ) ) {
        // Prompt to make sure player wants to gorge for hibernation...
        if( query_yn(_("Start gorging in preperation for hibernation?")) ) {
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
            add_msg(m_mixed, _("You feel as though you're going to split open! In a good way??"));
            mod_pain(5);
            std::vector<point> valid;
            for (int x = posx - 1; x <= posx + 1; x++) {
                for (int y = posy - 1; y <= posy + 1; y++) {
                    if (g->is_empty(x, y)) {
                        valid.push_back( point(x, y) );
                    }
                }
            }
            monster slime(GetMType("mon_player_blob"));
            int numslime = 1;
            for (int i = 0; i < numslime; i++) {
                int index = rng(0, valid.size() - 1);
                point sp = valid[index];
                valid.erase(valid.begin() + index);
                slime.spawn(sp.x, sp.y);
                slime.friendly = -1;
                g->add_zombie(slime);
            }
            hunger += 40;
            thirst += 40;
            //~slimespawns have *small voices* which may be the Nice equivalent
            //~of the Rat King's ALL CAPS invective.  Probably shared-brain telepathy.
            add_msg(m_good, _("hey, you look like me! let's work together!"));
        }
    }

    if( ( comest->nutr > 0 && temp_hunger < capacity ) ||
        ( comest->quench > 0 && temp_thirst < capacity ) ) {
        if ((spoiled) && !(has_trait("SAPROPHAGE")) ){//rotten get random nutrification
            if (!query_yn(_("You can hardly finish it all. Consume it?"))) {
                return false;
            }
        } else {
            if ( (( comest->nutr > 0 && temp_hunger < capacity ) ||
              ( comest->quench > 0 && temp_thirst < capacity )) &&
              ( (!(has_trait("EATHEALTH"))) || (!(has_trait("SLIMESPAWNER"))) ) ) {
                if (!query_yn(_("You will not be able to finish it all. Consume it?"))) {
                return false;
                }
            }
        }
    }

    if (comest->has_use()) {
        to_eat = comest->invoke(this, eaten, false, pos());
        if( to_eat <= 0 ) {
            return false;
        }
    }

    if ( (spoiled) && !(has_trait("SAPROPHAGE")) ) {
        add_msg(m_bad, _("Ick, this %s doesn't taste so good..."), eaten->tname().c_str());
        if (!has_trait("SAPROVORE") && !has_trait("EATDEAD") &&
       (!has_bionic("bio_digestion") || one_in(3))) {
            add_effect("foodpoison", rng(60, (comest->nutr + 1) * 60));
        }
        consume_effects(eaten, comest, spoiled);
    } else if ((spoiled) && has_trait("SAPROPHAGE")) {
        add_msg(m_good, _("Mmm, this %s tastes delicious..."), eaten->tname().c_str());
        consume_effects(eaten, comest, spoiled);
    } else {
        consume_effects(eaten, comest);
        if (!(has_trait("GOURMAND") || has_active_mutation("HIBERNATE") || has_trait("EATHEALTH"))) {
            if ((overeating && rng(-200, 0) > hunger)) {
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
    if ( (has_trait("EATHEALTH")) && ( comest->nutr > 0 && temp_hunger < capacity ) ) {
        int room = (capacity - temp_hunger);
        int excess_food = ((comest->nutr) - room);
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
        if( !one_in(3) && (stomach_food || stomach_water) ) {
            vomit();
        }
    }
    return true;
}

void player::consume_effects(item *eaten, it_comest *comest, bool rotten)
{
    if (has_trait("THRESH_PLANT") && comest->can_use( "PLANTBLECH" )) {
        return;
    }
    if( (has_trait("HERBIVORE") || has_trait("RUMINANT")) &&
        (eaten->made_of("flesh") || eaten->made_of("egg")) ) {
        // No good can come of this.
        return;
    }
    if ( !(has_trait("GIZZARD")) && (rotten) && !(has_trait("SAPROPHAGE")) ) {
        hunger -= rng(0, comest->nutr);
        thirst -= comest->quench;
        if (!has_trait("SAPROVORE") && !has_bionic("bio_digestion")) {
            mod_healthy_mod(-30);
        }
    } else if (has_trait("GIZZARD")) {
        // Carrion-eating Birds might have Saprovore; Saprophage is unlikely,
        // but best to code defensively.
        // Thanks for the warning, i2amroy.
        if ((rotten) && !(has_trait("SAPROPHAGE")) ) {
            int nut = (rng(0, comest->nutr) * 0.66 );
            int que = (comest->quench) * 0.66;
            hunger -= nut;
            thirst -= que;
            stomach_food += nut;
            stomach_water += que;
            if (!has_trait("SAPROVORE") && !has_bionic("bio_digestion")) {
                mod_healthy_mod(-30);
            }
        } else {
        // Shorter GI tract, so less nutrients captured.
            int giz_nutr = (comest->nutr) * 0.66;
            int giz_quench = (comest->quench) * 0.66;
            int giz_healthy = (comest->healthy) * 0.66;
            hunger -= (giz_nutr);
            thirst -= (giz_quench);
            mod_healthy_mod(giz_healthy);
            stomach_food += (giz_nutr);
            stomach_water += (giz_quench);
        }
    } else if (has_trait("CARNIVORE") && (eaten->made_of("veggy") || eaten->made_of("fruit") || eaten->made_of("wheat")) &&
      (eaten->made_of("flesh") || eaten->made_of("hflesh") || eaten->made_of("iflesh") || eaten->made_of("milk") ||
      eaten->made_of("egg")) ) {
          // Carnivore is stripping the good stuff out of that plant crap it's mixed up with.
          // They WILL eat the Meat Pizza.
          int carn_nutr = (comest->nutr) * 0.5;
          int carn_quench = (comest->quench) * 0.5;
          int carn_healthy = (comest->healthy) * 0.5;
          hunger -= (carn_nutr);
          thirst -= (carn_quench);
          mod_healthy_mod(carn_healthy);
          stomach_food += (carn_nutr);
          stomach_water += (carn_quench);
          add_msg_if_player(m_good, _("You eat the good parts and leave that indigestible plant stuff behind."));
    } else if (has_trait("CARNIVORE") && ((eaten->made_of("flesh") || eaten->made_of("hflesh") ||
      eaten->made_of("iflesh") || eaten->made_of("egg"))) ) {
          // Carnivores, being specialized to consume meat, get more nutrients from a wholly-meat or egg meal.
          if (comest->healthy < 1) {
              int carn_healthy = (comest->healthy) + 1;
              mod_healthy_mod(carn_healthy);
          }
          hunger -= comest->nutr;
          thirst -= comest->quench;
          mod_healthy_mod(comest->healthy);
          stomach_food += comest->nutr;
          stomach_water += comest->quench;
    } else {
    // Saprophages get the same boost from rotten food that others get from fresh.
        hunger -= comest->nutr;
        thirst -= comest->quench;
        mod_healthy_mod(comest->healthy);
        stomach_food += comest->nutr;
        stomach_water += comest->quench;
    }

    if (has_bionic("bio_digestion")) {
        hunger -= rng(0, comest->nutr);
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
        if ((comest->nutr > 0 && hunger < -60) || (comest->quench > 0 && thirst < -60)) {
            add_msg_if_player(_("You can't finish it all!"));
        }
        if (hunger < -60) {
            hunger = -60;
        }
        if (thirst < -60) {
            thirst = -60;
        }
    }
    } if (has_active_mutation("HIBERNATE")) {
         if ((comest->nutr > 0 && hunger < -60) || (comest->quench > 0 && thirst < -60)) { //Tell the player what's going on
            add_msg_if_player(_("You gorge yourself, preparing to hibernate."));
            if (one_in(2)) {
                (fatigue += (comest->nutr)); //50% chance of the food tiring you
            }
        }
        if ((comest->nutr > 0 && hunger < -200) || (comest->quench > 0 && thirst < -200)) { //Hibernation should cut burn to 60/day
            add_msg_if_player(_("You feel stocked for a day or two. Got your bed all ready and secured?"));
            if (one_in(2)) {
                (fatigue += (comest->nutr)); //And another 50%, intended cumulative
            }
        }
        if ((comest->nutr > 0 && hunger < -400) || (comest->quench > 0 && thirst < -400)) {
            add_msg_if_player(_("Mmm.  You can stil fit some more in...but maybe you should get comfortable and sleep."));
             if (!(one_in(3))) {
                (fatigue += (comest->nutr)); //Third check, this one at 66%
            }
        }
        if ((comest->nutr > 0 && hunger < -600) || (comest->quench > 0 && thirst < -600)) {
            add_msg_if_player(_("That filled a hole!  Time for bed..."));
            fatigue += (comest->nutr); //At this point, you're done.  Schlaf gut.
        }
        if ((comest->nutr > 0 && hunger < -620) || (comest->quench > 0 && thirst < -620)) {
            add_msg_if_player(_("You can't finish it all!"));
        }
        if (hunger < -620) {
            hunger = -620;
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
        if ((comest->nutr > 0 && hunger < -20) || (comest->quench > 0 && thirst < -20)) {
            add_msg_if_player(_("You can't finish it all!"));
        }
        if (hunger < -20) {
            hunger = -20;
        }
        if (thirst < -20) {
            thirst = -20;
        }
    }
}

void player::rooted_message() const
{
    if( (has_trait("ROOTS2") || has_trait("ROOTS3") ) &&
        g->m.has_flag("DIGGABLE", posx, posy) &&
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
        g->m.has_flag("DIGGABLE", posx, posy) &&
        !shoe_factor ) {
        if( one_in(20 / shoe_factor) ) {
            if (hunger > -20) {
                hunger--;
            }
            if (thirst > -20) {
                thirst--;
            }
            mod_healthy_mod(5);
        }
    }
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
  if (autodrop || volume_carried() + weapon.volume() < volume_capacity()) {
   inv.add_item_keep_invlet(remove_weapon());
   inv.unsort();
   moves -= 20;
   recoil = MIN_RECOIL;
   return true;
  } else if (query_yn(_("No space in inventory for your %s.  Drop it?"),
                      weapon.tname().c_str())) {
   g->m.add_item_or_charges(posx, posy, remove_weapon());
   recoil = MIN_RECOIL;
   return true;
  } else
   return false;
 }
 if (&weapon == it) {
  add_msg(m_info, _("You're already wielding that!"));
  return false;
 } else if (it == NULL || it->is_null()) {
  add_msg(m_info, _("You don't have that item."));
  return false;
 }

 if (it->is_two_handed(this) && !has_two_arms()) {
  add_msg(m_info, _("You cannot wield a %s with only one arm."),
             it->tname().c_str());
  return false;
 }
 if (!is_armed()) {
  weapon = i_rem(it);
  moves -= 30;
  weapon.on_wield( *this );
  last_item = itype_id(weapon.type->id);
  return true;
 } else if (volume_carried() + weapon.volume() - it->volume() <
            volume_capacity()) {
  item tmpweap = remove_weapon();
  weapon = i_rem(it);
  inv.add_item_keep_invlet(tmpweap);
  inv.unsort();
  moves -= 45;
  weapon.on_wield( *this );
  last_item = itype_id(weapon.type->id);
  return true;
 } else if (query_yn(_("No space in inventory for your %s.  Drop it?"),
                     weapon.tname().c_str())) {
  g->m.add_item_or_charges(posx, posy, remove_weapon());
  weapon = i_rem(it);
  inv.unsort();
  moves -= 30;
  weapon.on_wield( *this );
  last_item = itype_id(weapon.type->id);
  return true;
 }

 return false;

}

// ids of martial art styles that are available with the bio_cqb bionic.
static const std::array<std::string, 4> bio_cqb_styles {{
"style_karate", "style_judo", "style_muay_thai", "style_biojutsu"
}};

class ma_style_callback : public uimenu_callback
{
public:
    virtual bool key(int key, int entnum, uimenu *menu) {
        if( key != '?' ) {
            return false;
        }
        std::string style_selected;
        const size_t index = entnum;
        if( g->u.has_active_bionic( "bio_cqb" ) && index < menu->entries.size() ) {
            const size_t id = menu->entries[index].retval - 2;
            if( id < bio_cqb_styles.size() ) {
                style_selected = bio_cqb_styles[id];
            }
        } else if( index >= 3 && index - 3 < g->u.ma_styles.size() ) {
            style_selected = g->u.ma_styles[index - 3];
        }
        if( !style_selected.empty() ) {
            const martialart &ma = martialarts[style_selected];
            std::ostringstream buffer;
            buffer << ma.name << "\n\n";
            buffer << ma.description << "\n\n";
            if( !ma.techniques.empty() ) {
                buffer << ngettext( "Technique:", "Techniques:", ma.techniques.size() ) << " ";
                for( auto technique = ma.techniques.cbegin();
                     technique != ma.techniques.cend(); ++technique ) {
                    buffer << ma_techniques[*technique].name;
                    if( ma.techniques.size() > 1 && technique == ----ma.techniques.cend() ) {
                        //~ Seperators that comes before last element of a list.
                        buffer << _(" and ");
                    } else if( technique != --ma.techniques.cend() ) {
                        //~ Seperator for a list of items.
                        buffer << _(", ");
                    }
                }
                buffer << "\n";
            }
            if( !ma.weapons.empty() ) {
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
        else if( index == 1 ) { // description of the "keep hands free" option
            std::ostringstream buffer;
            buffer << _("Keep hands free") << "\n\n";
            buffer << _("When this is enabled, player won't wield things unless explicitly told to.");
            popup(buffer.str(), PF_NONE);
            menu->redraw();
        }
        return true;
    }
    virtual ~ma_style_callback() { }
};

void player::pick_style() // Style selection menu
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
    kmenu.text = _("Select a style (press ? for style info)");
    std::auto_ptr<ma_style_callback> ma_style_info(new ma_style_callback());
    kmenu.callback = ma_style_info.get();
    kmenu.addentry( 0, true, 'c', _("Cancel") );
    if (keep_hands_free) {
      kmenu.addentry( 1, true, 'h', _("Keep hands free (on)") );
    }
    else {
      kmenu.addentry( 1, true, 'h', _("Keep hands free (off)") );
    }

    if (has_active_bionic("bio_cqb")) {
        for(size_t i = 0; i < bio_cqb_styles.size(); i++) {
            if (martialarts.find(bio_cqb_styles[i]) != martialarts.end()) {
                kmenu.addentry( i + 2, true, -1, martialarts[bio_cqb_styles[i]].name );
            }
        }

        kmenu.query();
        const size_t selection = kmenu.ret;
        if ( selection >= 2 && selection - 2 < bio_cqb_styles.size() ) {
            style_selected = bio_cqb_styles[selection - 2];
        }
        else if ( selection == 1 ) {
            keep_hands_free = !keep_hands_free;
        }
    }
    else {
        kmenu.addentry( 2, true, 'n', _("No style") );
        kmenu.selected = 2;
        kmenu.return_invalid = true; //cancel with any other keys

        for (size_t i = 0; i < ma_styles.size(); i++) {
            if(martialarts.find(ma_styles[i]) == martialarts.end()) {
                debugmsg ("Bad hand to hand style: %s",ma_styles[i].c_str());
            } else {
                //Check if this style is currently selected
                if( ma_styles[i] == style_selected ) {
                    kmenu.selected =i+3; //+3 because there are "cancel", "keep hands free" and "no style" first in the list
                }
                kmenu.addentry( i+3, true, -1, martialarts[ma_styles[i]].name );
            }
        }

        kmenu.query();
        int selection = kmenu.ret;

        //debugmsg("selected %d",choice);
        if (selection >= 3)
            style_selected = ma_styles[selection - 3];
        else if (selection == 2)
            style_selected = "style_none";
        else if (selection == 1)
            keep_hands_free = !keep_hands_free;

        //else
        //all other means -> don't change, keep current.
    }
}

hint_rating player::rate_action_wear(item *it)
{
    //TODO flag already-worn items as HINT_IFFY

    if (!it->is_armor()) {
        return HINT_CANT;
    }

    // are we trying to put on power armor? If so, make sure we don't have any other gear on.
    if (it->is_power_armor() && worn.size()) {
        if (it->covers(bp_torso)) {
            return HINT_IFFY;
        } else if (it->covers(bp_head) && !worn[0].is_power_armor()) {
            return HINT_IFFY;
        }
    }
    // are we trying to wear something over power armor? We can't have that, unless it's a backpack, or similar.
    if (worn.size() && worn[0].is_power_armor() && !(it->covers(bp_head))) {
        if (!(it->covers(bp_torso) && it->color() == c_green)) {
            return HINT_IFFY;
        }
    }

    // Make sure we're not wearing 2 of the item already
    int count = 0;
    for (auto &i : worn) {
        if (i.type->id == it->type->id) {
            count++;
        }
    }
    if (count == 2) {
        return HINT_IFFY;
    }
    if (has_trait("WOOLALLERGY") && it->made_of("wool")) {
        return HINT_IFFY; //should this be HINT_CANT? I kinda think not, because HINT_CANT is more for things that can NEVER happen
    }
    if (it->covers(bp_head) && encumb(bp_head) != 0) {
        return HINT_IFFY;
    }
    if ((it->covers(bp_hand_l) || it->covers(bp_hand_r)) && has_trait("WEBBED")) {
        return HINT_IFFY;
    }
    if ((it->covers(bp_hand_l) || it->covers(bp_hand_r)) && has_trait("TALONS")) {
        return HINT_IFFY;
    }
    if ((it->covers(bp_hand_l) || it->covers(bp_hand_r)) && (has_trait("ARM_TENTACLES") ||
            has_trait("ARM_TENTACLES_4") || has_trait("ARM_TENTACLES_8")) ) {
        return HINT_IFFY;
    }
    if (it->covers(bp_mouth) && (has_trait("BEAK") ||
            has_trait("BEAK_PECK") || has_trait("BEAK_HUM")) ) {
        return HINT_IFFY;
    }
    if ((it->covers(bp_foot_l) || it->covers(bp_foot_r)) && has_trait("HOOVES")) {
        return HINT_IFFY;
    }
    if ((it->covers(bp_foot_l) || it->covers(bp_foot_r)) && has_trait("LEG_TENTACLES")) {
        return HINT_IFFY;
     }
    if (it->covers(bp_head) && has_trait("HORNS_CURLED")) {
        return HINT_IFFY;
    }
    if (it->covers(bp_torso) && (has_trait("SHELL") || has_trait("SHELL2")))  {
        return HINT_IFFY;
    }
    if (it->covers(bp_head) && !it->made_of("wool") &&
          !it->made_of("cotton") && !it->made_of("leather") && !it->made_of("nomex") &&
          (has_trait("HORNS_POINTED") || has_trait("ANTENNAE") || has_trait("ANTLERS"))) {
        return HINT_IFFY;
    }
    // Checks to see if the player is wearing shoes
    if (((it->covers(bp_foot_l) && is_wearing_shoes("left")) ||
          (it->covers(bp_foot_r) && is_wearing_shoes("right"))) &&
          !it->has_flag("BELTED") && !it->has_flag("SKINTIGHT")) {
    return HINT_IFFY;
    }

    return HINT_GOOD;
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

    if (!wear_item(to_wear, interactive))
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

bool player::wear_item(item *to_wear, bool interactive)
{
    if( !to_wear->is_armor() ) {
        add_msg(m_info, _("Putting on a %s would be tricky."), to_wear->tname().c_str());
        return false;
    }

    // are we trying to put on power armor? If so, make sure we don't have any other gear on.
    if (to_wear->is_power_armor())
    {
        for( auto &elem : worn ) {
            if( ( elem.get_covered_body_parts() & to_wear->get_covered_body_parts() ).any() ) {
                if(interactive)
                {
                    add_msg(m_info, _("You can't wear power armor over other gear!"));
                }
                return false;
            }
        }

        if (!(to_wear->covers(bp_torso)))
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
                    add_msg(m_info, _("You can only wear power armor components with power armor!"));
                }
                return false;
            }
        }

        for (auto &i : worn)
        {
            if (i.is_power_armor() && i.typeId() == to_wear->typeId() )
            {
                if(interactive)
                {
                    add_msg(m_info, _("You cannot wear more than one %s!"), to_wear->tname().c_str());
                }
                return false;
            }
        }
    }
    else
    {
        // Only headgear can be worn with power armor, except other power armor components
        if(!to_wear->covers(bp_head) && !to_wear->covers(bp_eyes) &&
             !to_wear->covers(bp_head)) {
            for (auto &i : worn)
            {
                if( i.is_power_armor() )
                {
                    if(interactive)
                    {
                        add_msg(m_info, _("You can't wear %s with power armor!"), to_wear->tname().c_str());
                    }
                    return false;
                }
            }
        }
    }

    // Make sure we're not wearing 2 of the item already
    int count = 0;

    for (auto &i : worn) {
        if (i.type->id == to_wear->type->id) {
            count++;
        }
    }

    if (count == 2) {
        if(interactive) {
            add_msg(m_info, _("You can't wear more than two %s at once."),
                    to_wear->tname(count).c_str());
        }
        return false;
    }

    if (!to_wear->has_flag("OVERSIZE")) {
        if (has_trait("WOOLALLERGY") && to_wear->made_of("wool")) {
            if(interactive) {
                add_msg(m_info, _("You can't wear that, it's made of wool!"));
            }
            return false;
        }

        if (to_wear->covers(bp_head) && encumb(bp_head) != 0 && to_wear->get_encumber() > 0) {
            if(interactive) {
                add_msg(m_info, wearing_something_on(bp_head) ?
                                _("You can't wear another helmet!") : _("You can't wear a helmet!"));
            }
            return false;
        }

        if ((to_wear->covers(bp_hand_l) || to_wear->covers(bp_hand_r) ||
              to_wear->covers(bp_arm_l) || to_wear->covers(bp_arm_r) ||
              to_wear->covers(bp_leg_l) || to_wear->covers(bp_leg_r) ||
              to_wear->covers(bp_foot_l) || to_wear->covers(bp_foot_r) ||
              to_wear->covers(bp_torso) || to_wear->covers(bp_head)) &&
            (has_trait("HUGE") || has_trait("HUGE_OK"))) {
            if(interactive) {
                add_msg(m_info, _("The %s is much too small to fit your huge body!"),
                        to_wear->type_name().c_str());
            }
            return false;
        }

        if ((to_wear->covers(bp_hand_l) || to_wear->covers(bp_hand_r)) && has_trait("WEBBED"))
        {
            if(interactive)
            {
                add_msg(m_info, _("You cannot put %s over your webbed hands."), to_wear->type_name().c_str());
            }
            return false;
        }

        if ( (to_wear->covers(bp_hand_l) || to_wear->covers(bp_hand_r)) &&
             (has_trait("ARM_TENTACLES") || has_trait("ARM_TENTACLES_4") ||
              has_trait("ARM_TENTACLES_8")) )
        {
            if(interactive)
            {
                add_msg(m_info, _("You cannot put %s over your tentacles."), to_wear->type_name().c_str());
            }
            return false;
        }

        if ((to_wear->covers(bp_hand_l) || to_wear->covers(bp_hand_r)) && has_trait("TALONS"))
        {
            if(interactive)
            {
                add_msg(m_info, _("You cannot put %s over your talons."), to_wear->type_name().c_str());
            }
            return false;
        }

        if ((to_wear->covers(bp_hand_l) || to_wear->covers(bp_hand_r)) && (has_trait("PAWS") || has_trait("PAWS_LARGE")) )
        {
            if(interactive)
            {
                add_msg(m_info, _("You cannot get %s to stay on your paws."), to_wear->type_name().c_str());
            }
            return false;
        }

        if (to_wear->covers(bp_mouth) && (has_trait("BEAK") || has_trait("BEAK_PECK") ||
        has_trait("BEAK_HUM")) )
        {
            if(interactive)
            {
                add_msg(m_info, _("You cannot put a %s over your beak."), to_wear->type_name().c_str());
            }
            return false;
        }

        if (to_wear->covers(bp_mouth) &&
            (has_trait("MUZZLE") || has_trait("MUZZLE_BEAR") || has_trait("MUZZLE_LONG") ||
            has_trait("MUZZLE_RAT")))
        {
            if(interactive)
            {
                add_msg(m_info, _("You cannot fit the %s over your muzzle."), to_wear->type_name().c_str());
            }
            return false;
        }

        if (to_wear->covers(bp_mouth) && has_trait("MINOTAUR"))
        {
            if(interactive)
            {
                add_msg(m_info, _("You cannot fit the %s over your snout."), to_wear->type_name().c_str());
            }
            return false;
        }

        if (to_wear->covers(bp_mouth) && has_trait("SABER_TEETH"))
        {
            if(interactive)
            {
                add_msg(m_info, _("Your saber teeth are simply too large for %s to fit."), to_wear->type_name().c_str());
            }
            return false;
        }

        if (to_wear->covers(bp_mouth) && has_trait("MANDIBLES"))
        {
            if(interactive)
            {
                add_msg(_("Your mandibles are simply too large for %s to fit."), to_wear->type_name().c_str());
            }
            return false;
        }

        if (to_wear->covers(bp_mouth) && has_trait("PROBOSCIS"))
        {
            if(interactive)
            {
                add_msg(m_info, _("Your proboscis is simply too large for %s to fit."), to_wear->type_name().c_str());
            }
            return false;
        }

        if ((to_wear->covers(bp_foot_l) || to_wear->covers(bp_foot_r)) && has_trait("HOOVES"))
        {
            if(interactive)
            {
                add_msg(m_info, _("You cannot wear footwear on your hooves."));
            }
            return false;
        }

        if ((to_wear->covers(bp_foot_l) || to_wear->covers(bp_foot_r)) && has_trait("LEG_TENTACLES"))
        {
            if(interactive)
            {
                add_msg(m_info, _("You cannot wear footwear on your tentacles."));
            }
            return false;
        }

        if ((to_wear->covers(bp_foot_l) || to_wear->covers(bp_foot_r)) && has_trait("RAP_TALONS"))
        {
            if(interactive)
            {
                add_msg(m_info, _("Your talons are much too large for footgear."));
            }
            return false;
        }

        if (to_wear->covers(bp_head) && has_trait("HORNS_CURLED"))
        {
            if(interactive)
            {
                add_msg(m_info, _("You cannot wear headgear over your horns."));
            }
            return false;
        }

        if (to_wear->covers(bp_torso) && (has_trait("SHELL") || has_trait("SHELL2")) )
        {
            if(interactive)
            {
                add_msg(m_info, _("You cannot fit that over your shell."));
            }
            return false;
        }

        if (to_wear->covers(bp_torso) && ((has_trait("INSECT_ARMS")) || (has_trait("ARACHNID_ARMS"))) )
        {
            if(interactive)
            {
                add_msg(m_info, _("Your new limbs are too wriggly to fit under that."));
            }
            return false;
        }

        if (to_wear->covers(bp_head) &&
            !to_wear->made_of("wool") && !to_wear->made_of("cotton") &&
            !to_wear->made_of("nomex") && !to_wear->made_of("leather") &&
            (has_trait("HORNS_POINTED") || has_trait("ANTENNAE") || has_trait("ANTLERS")))
        {
            if(interactive)
            {
                add_msg(m_info, _("You cannot wear a helmet over your %s."),
                           (has_trait("HORNS_POINTED") ? _("horns") :
                            (has_trait("ANTENNAE") ? _("antennae") : _("antlers"))));
            }
            return false;
        }

        if (((to_wear->covers(bp_foot_l) && is_wearing_shoes("left")) ||
              (to_wear->covers(bp_foot_r) && is_wearing_shoes("right"))) &&
              !to_wear->has_flag("BELTED") && !to_wear->has_flag("SKINTIGHT")) {
            // Checks to see if the player is wearing shoes
            if(interactive){
                add_msg(m_info, _("You're already wearing footwear!"));
            }
            return false;
        }
    }

    if (to_wear->invlet == 0) {
        inv.assign_empty_invlet( *to_wear, false );
    }

    const bool was_deaf = is_deaf();
    last_item = itype_id(to_wear->type->id);
    worn.push_back(*to_wear);

    if(interactive)
    {
        add_msg(_("You put on your %s."), to_wear->tname().c_str());
        moves -= 350; // TODO: Make this variable?

        worn.back().on_wear( *this );

        for (body_part i = bp_head; i < num_bp; i = body_part(i + 1))
        {
            if (to_wear->covers(i) && encumb(i) >= 4)
            {
                add_msg(m_warning,
                    i == bp_eyes ?
                    _("Your %s are very encumbered! %s"):_("Your %s is very encumbered! %s"),
                    body_part_name(body_part(i)).c_str(), encumb_text(body_part(i)).c_str());
            }
        }
        if( !was_deaf && is_deaf() ) {
            add_msg( m_info, _( "You're deafened!" ) );
        }
    }

    recalc_sight_limits();

    return true;
}

hint_rating player::rate_action_takeoff(item *it) {
    if (!it->is_armor()) {
        return HINT_CANT;
    }

    for (auto &i : worn) {
        if (i.invlet == it->invlet) { //surely there must be an easier way to do this?
            return HINT_GOOD;
        }
    }

    return HINT_IFFY;
}

bool player::takeoff( item *target, bool autodrop, std::vector<item> *items)
{
    return takeoff( get_item_position( target ), autodrop, items );
}

bool player::takeoff(int inventory_position, bool autodrop, std::vector<item> *items)
{
    bool taken_off = false;
    if (inventory_position == -1) {
        taken_off = wield(NULL, autodrop);
    } else {
        int worn_index = worn_position_to_index(inventory_position);
        if (worn_index >= 0 && size_t(worn_index) < worn.size()) {
            item &w = worn[worn_index];

            // Handle power armor.
            if (w.is_power_armor() && w.covers(bp_torso)) {
                // We're trying to take off power armor, but cannot do that if we have a power armor component on!
                for (int j = worn.size() - 1; j >= 0; j--) {
                    if (worn[j].is_power_armor() &&
                            j != worn_index) {
                        if( autodrop || items != nullptr ) {
                            if( items != nullptr ) {
                                items->push_back( worn[j] );
                            } else {
                                g->m.add_item_or_charges( posx, posy, worn[j] );
                            }
                            add_msg(_("You take off your %s."), worn[j].tname().c_str());
                            worn.erase(worn.begin() + j);
                            // If we are before worn_index, erasing this element shifted its position by 1.
                            if (worn_index > j) {
                                worn_index -= 1;
                                w = worn[worn_index];
                            }
                            taken_off = true;
                        } else {
                            add_msg(m_info, _("You can't take off power armor while wearing other power armor components."));
                            return false;
                        }
                    }
                }
            }

            if( items != nullptr ) {
                items->push_back( w );
                taken_off = true;
            } else if (autodrop || volume_capacity() - w.get_storage() > volume_carried() + w.volume()) {
                inv.add_item_keep_invlet(w);
                taken_off = true;
            } else if (query_yn(_("No room in inventory for your %s.  Drop it?"),
                    w.tname().c_str())) {
                g->m.add_item_or_charges(posx, posy, w);
                taken_off = true;
            } else {
                taken_off = false;
            }
            if( taken_off ) {
                add_msg(_("You take off your %s."), w.tname().c_str());
                worn.erase(worn.begin() + worn_index);
            }
        } else {
            add_msg(m_info, _("You are not wearing that item."));
        }
    }

    recalc_sight_limits();

    return taken_off;
}

void player::use_wielded() {
  use(-1);
}

hint_rating player::rate_action_reload(item *it) {
    if (it->has_flag("NO_RELOAD")) {
        return HINT_CANT;
    }
    if (it->is_gun()) {
        if (it->has_flag("RELOAD_AND_SHOOT") || it->ammo_type() == "NULL") {
            return HINT_CANT;
        }
        if (it->charges == it->clip_size()) {
            bool alternate_magazine = false;
            for (auto &i : it->contents) {
                if ((i.is_gunmod() && (i.typeId() == "spare_mag" &&
                      i.charges < it->type->gun->clip)) ||
                      (i.is_auxiliary_gunmod() && i.charges < i.clip_size())) {
                    alternate_magazine = true;
                }
            }
            if(alternate_magazine == false) {
                return HINT_IFFY;
            }
        }
        return HINT_GOOD;
    } else if (it->is_tool()) {
        it_tool* tool = dynamic_cast<it_tool*>(it->type);
        if (tool->ammo == "NULL") {
            return HINT_CANT;
        }
        return HINT_GOOD;
    }
    return HINT_CANT;
}

hint_rating player::rate_action_unload( const item &it ) const
{
    if( ( it.is_container() || it.is_gun() ) && !it.contents.empty() ) {
        return HINT_GOOD;
    }
    if( !it.is_gun() &&
        !it.is_auxiliary_gunmod() &&
        !( it.typeId() == "spare_mag" ) &&
        ( !it.is_tool() || it.ammo_type() == "NULL" ) ) {
        return HINT_CANT;
    }
    for( auto &gunmod : it.contents ) {
        if( gunmod.is_auxiliary_gunmod() && gunmod.charges > 0 ) {
            return HINT_GOOD;
        } else if( gunmod.typeId() == "spare_mag" && gunmod.charges > 0 ) {
            return HINT_GOOD;
        }
    }
    if( it.charges <= 0 ) {
        return HINT_CANT;
    }
    return HINT_GOOD;
}

//TODO refactor stuff so we don't need to have this code mirroring game::disassemble
hint_rating player::rate_action_disassemble(item *it) {
    for( auto &recipes_cat_iter : recipes ) {
        for( auto cur_recipe : recipes_cat_iter.second ) {

            if (it->type->id == cur_recipe->result && cur_recipe->reversible) {
                /* ok, a valid recipe exists for the item, and it is reversible
                   assign the activity
                   check tools are available
                   loop over the tools and see what's required...again */
                const inventory &crafting_inv = crafting_inventory();
                for (const auto &j : cur_recipe->requirements.tools) {
                    bool have_tool = false;
                    if (j.empty()) { // no tools required, may change this
                        have_tool = true;
                    } else {
                        for (const auto &k : j) {
                            itype_id type = k.type;
                            int req = k.count; // -1 => 1

                            // if crafting recipe required a welder, disassembly requires a hacksaw or super toolkit
                            if (type == "welder") {
                                if( crafting_inv.has_items_with_quality( "SAW_M_FINE", 1, 1 ) ) {
                                    have_tool = true;
                                } else {
                                    have_tool = false;
                                }
                            } else if ((req <= 0 && crafting_inv.has_amount (type, 1)) ||
                                (req > 0 && crafting_inv.has_charges(type, req))) {
                                have_tool = true;
                                break;
                            }
                        }
                    }
                    if (!have_tool) {
                       return HINT_IFFY;
                    }
                }
                // all tools present
                return HINT_GOOD;
            }
        }
    }
    if(it->is_book()) {
        return HINT_GOOD;
    }
    // no recipe exists, or the item cannot be disassembled
    return HINT_CANT;
}

hint_rating player::rate_action_use(const item *it) const
{
    if (it->is_tool()) {
        it_tool *tool = dynamic_cast<it_tool*>(it->type);
        if (tool->charges_per_use != 0 && it->charges < tool->charges_per_use) {
            return HINT_IFFY;
        } else {
            return HINT_GOOD;
        }
    } else if (it->is_gunmod()) {
        if (get_skill_level("gun") == 0) {
            return HINT_IFFY;
        } else {
            return HINT_GOOD;
        }
    } else if (it->is_bionic()) {
        return HINT_GOOD;
    } else if (it->is_food() || it->is_food_container() || it->is_book() || it->is_armor()) {
        return HINT_IFFY; //the rating is subjective, could be argued as HINT_CANT or HINT_GOOD as well
    } else if (it->is_gun()) {
        if (!it->contents.empty()) {
            return HINT_GOOD;
        } else {
            return HINT_IFFY;
        }
    } else if( it->type->has_use() ) {
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
        it_tool *tool = dynamic_cast<it_tool*>(used->type);
        if (!has_enough_charges(*used, true)) {
            return;
        }
        const long charges_used = tool->invoke( this, used, false, pos() );
        if (charges_used <= 0) {
            // Canceled or not used up or whatever
            return;
        }
        if( tool->charges_per_use <= 0 ) {
            // An item that doesn't normally expend charges is destroyed instead.
            /* We can't be certain the item is still in the same position,
             * as other items may have been consumed as well, so remove
             * the item directly instead of by its position. */
            i_rem(used);
            return;
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
        // We may have fiddled with the state of the item in the iuse method,
        // so restack to sort things out.
        inv.restack();
    } else if (used->is_gunmod()) {
        const auto mod = used->type->gunmod.get();
        if (!(skillLevel("gun") >= mod->req_skill)) {
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
        if (guntype->skill_used == Skill::skill("pistol") && !mod->used_on_pistol) {
            add_msg(m_info, _("That %s cannot be attached to a handgun."),
                       used->tname().c_str());
            return;
        } else if (guntype->skill_used == Skill::skill("shotgun") && !mod->used_on_shotgun) {
            add_msg(m_info, _("That %s cannot be attached to a shotgun."),
                       used->tname().c_str());
            return;
        } else if (guntype->skill_used == Skill::skill("smg") && !mod->used_on_smg) {
            add_msg(m_info, _("That %s cannot be attached to a submachine gun."),
                       used->tname().c_str());
            return;
        } else if (guntype->skill_used == Skill::skill("rifle") && !mod->used_on_rifle) {
            add_msg(m_info, _("That %s cannot be attached to a rifle."),
                       used->tname().c_str());
            return;
        } else if (guntype->skill_used == Skill::skill("archery") && !mod->used_on_bow && guntype->ammo == "arrow") {
            add_msg(m_info, _("That %s cannot be attached to a bow."),
                       used->tname().c_str());
            return;
        } else if (guntype->skill_used == Skill::skill("archery") && !mod->used_on_crossbow && guntype->ammo == "bolt") {
            add_msg(m_info, _("That %s cannot be attached to a crossbow."),
                       used->tname().c_str());
            return;
        } else if (guntype->skill_used == Skill::skill("launcher") && !mod->used_on_launcher) {
            add_msg(m_info, _("That %s cannot be attached to a launcher."),
                       used->tname().c_str());
            return;
        } else if ( !mod->acceptible_ammo_types.empty() &&
                    mod->acceptible_ammo_types.count(guntype->ammo) == 0 ) {
                add_msg(m_info, _("That %s cannot be used on a %s."), used->tname().c_str(),
                       ammo_name(guntype->ammo).c_str());
                return;
        } else if (guntype->valid_mod_locations.count(mod->location) == 0) {
            add_msg(m_info, _("Your %s doesn't have a slot for this mod."), gun->tname().c_str());
            return;
        } else if (gun->get_free_mod_locations(mod->location) <= 0) {
            add_msg(m_info, _("Your %s doesn't have enough room for another %s mod. To remove the mods, \
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
                add_msg(m_info, _("Your %s already has a %s."), gun->tname().c_str(),
                           used->tname().c_str());
                return;
            } else if ((used->typeId() == "clip" || used->typeId() == "clip2") &&
                       (i.type->id == "clip" || i.type->id == "clip2")) {
                add_msg(m_info, _("Your %s already has an extended magazine."),
                           gun->tname().c_str());
                return;
            }
        }
        add_msg(_("You attach the %s to your %s."), used->tname().c_str(),
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
        // Get weapon mod names.
        if (mods.empty()) {
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
            kmenu.addentry( i, true, -1, mods[i].tname() );
        }
        kmenu.addentry( mods.size(), true, 'r', _("Remove all") );
        kmenu.addentry( mods.size() + 1, true, 'q', _("Cancel") );
        kmenu.query();
        choice = kmenu.ret;

        if (choice < int(mods.size())) {
            const std::string mod = used->contents[choice].tname();
            remove_gunmod(used, unsigned(choice));
            add_msg(_("You remove your %s from your %s."), mod.c_str(), used->tname().c_str());
        } else if (choice == int(mods.size())) {
            for (int i = used->contents.size() - 1; i >= 0; i--) {
                remove_gunmod(used, i);
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
        used->type->invoke(this, used, false, pos());
        return;
    } else {
        add_msg(m_info, _("You can't do anything interesting with your %s."),
                used->tname().c_str());
        return;
    }
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
}

hint_rating player::rate_action_read(item *it)
{
 if (!it->is_book()) {
  return HINT_CANT;
 }

 if (g && g->light_level() < 8 && LL_LIT > g->m.light_at(posx, posy)) {
  return HINT_IFFY;
 } else if (morale_level() < MIN_MORALE_READ && it->type->book->fun <= 0) {
  return HINT_IFFY; //won't read non-fun books when sad
 } else if (it->type->book->intel > 0 && has_trait("ILLITERATE")) {
  return HINT_IFFY;
 } else if (has_trait("HYPEROPIC") && !is_wearing("glasses_reading")
            && !is_wearing("glasses_bifocal") && !has_effect("contacts")) {
  return HINT_IFFY;
 }

 return HINT_GOOD;
}

void player::read(int inventory_position)
{
    vehicle *veh = g->m.veh_at (posx, posy);
    if (veh && veh->player_in_control (this)) {
        add_msg(m_info, _("It's a bad idea to read while driving!"));
        return;
    }

    // Check if reading is okay
    // check for light level
    if (fine_detail_vision_mod() > 4) { //minimum LL_LOW or LL_DARK + (ELFA_NV or atomic_light)
        add_msg(m_info, _("You can't see to read!"));
        return;
    }

    // check for traits
    if (has_trait("HYPEROPIC") && !is_wearing("glasses_reading") &&
        !is_wearing("glasses_bifocal") && !has_effect("contacts")) {
        add_msg(m_info, _("Your eyes won't focus without reading glasses."));
        return;
    }

    // Find the object
    item* it = &i_at(inventory_position);

    if (it == NULL || it->is_null()) {
        add_msg(m_info, _("You do not have that item."));
        return;
    }

    if (!it->is_book()) {
        add_msg(m_info, _("Your %s is not good reading material."),
        it->tname().c_str());
        return;
    }

    auto tmp = it->type->book.get();
    int time; //Declare this here so that we can change the time depending on whats needed
    // activity.get_value(0) == 1: see below at player_activity(ACT_READ)
    const bool continuous = (activity.get_value(0) == 1);
    bool study = continuous;
    if (tmp->intel > 0 && has_trait("ILLITERATE")) {
        add_msg(m_info, _("You're illiterate!"));
        return;
    }

    // Now we've established that the player CAN read.

    // If the player hasn't read this book before, skim it to get an idea of what's in it.
    if( !has_identified( it->type->id ) ) {
        // Base read_speed() is 1000 move points (1 minute per tmp->time)
        time = tmp->time * read_speed() * (fine_detail_vision_mod());
        if (tmp->intel > int_cur) {
            // Lower int characters can read, at a speed penalty
            time += (tmp->time * (tmp->intel - int_cur) * 100);
        }
        // We're just skimming, so it's 10x faster.
        time /= 10;

        activity = player_activity(ACT_READ, time - moves, -1, inventory_position, "");
        // Never trigger studying when skimming the book.
        activity.values.push_back(0);
        moves = 0;
        return;
    }


    const auto skill = tmp->skill;
    if( skill == nullptr ) {
        // special guidebook effect: print a misc. hint when read
        if (it->typeId() == "guidebook") {
            add_msg(m_info, get_hint().c_str());
            moves -= 100;
            return;
        }
        // otherwise do nothing as there's no associated skill
    } else if (morale_level() < MIN_MORALE_READ && tmp->fun <= 0) { // See morale.h
        add_msg(m_info, _("What's the point of studying?  (Your morale is too low!)"));
        return;
    } else if( skillLevel( skill ) < tmp->req ) {
        add_msg(_("The %s-related jargon flies over your head!"),
                   skill->name().c_str());
        if (tmp->recipes.empty()) {
            return;
        } else {
            add_msg(m_info, _("But you might be able to learn a recipe or two."));
        }
    } else if (skillLevel(skill) >= tmp->level && !can_study_recipe(*it->type) &&
               !query_yn(tmp->fun > 0 ?
                         _("It would be fun, but your %s skill won't be improved.  Read anyway?") :
                         _("Your %s skill won't be improved.  Read anyway?"),
                         skill->name().c_str())) {
        return;
    } else if( !continuous && ( skillLevel(skill) < tmp->level || can_study_recipe(*it->type) ) &&
                         !query_yn( skillLevel(skill) < tmp->level ?
                         _("Study %s until you learn something? (gain a level)") :
                         _("Study the book until you learn all recipes?"),
                         skill->name().c_str()) ) {
        study = false;
    } else {
        //If we just started studying, tell the player how to stop
        if(!continuous) {
            add_msg(m_info, _("Now studying %s, %s to stop early."),
                    it->tname().c_str(), press_x(ACTION_PAUSE).c_str());
            if ( (has_trait("ROOTS2") || (has_trait("ROOTS3"))) &&
                 g->m.has_flag("DIGGABLE", posx, posy) &&
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

    if (tmp->intel > int_cur && !continuous) {
        add_msg(m_warning, _("This book is too complex for you to easily understand. It will take longer to read."));
        // Lower int characters can read, at a speed penalty
        time += (tmp->time * (tmp->intel - int_cur) * 100);
    }

    activity = player_activity(ACT_READ, time, -1, inventory_position, "");
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
    } else if (has_trait("SPIRITUAL") && it->has_flag("INSPIRATIONAL")) {
        add_morale(MORALE_BOOK, 50, 150, minutes + 60, minutes, false, it->type);
    } else {
        add_morale(MORALE_BOOK, 0, tmp->fun * 15, minutes + 30, minutes, false, it->type);
    }
}

void player::do_read( item *book )
{
    auto reading = book->type->book.get();
    const auto skill = reading->skill;

    if( !has_identified( book->type->id ) ) {
        // Note that we've read the book.
        items_identified.insert( book->type->id );

        add_msg(_("You skim %s to find out what's in it."), book->type_name().c_str());
        if( skill != nullptr ) {
            add_msg(m_info, _("Can bring your %s skill to %d."),
                    skill->name().c_str(), reading->level);
            if( reading->req != 0 ){
                add_msg(m_info, _("Requires %s level %d to understand."),
                        skill->name().c_str(), reading->req);
            }
        }

        add_msg(m_info, _("Requires intelligence of %d to easily read."), reading->intel);
        if (reading->fun != 0) {
            add_msg(m_info, _("Reading this book affects your morale by %d"), reading->fun);
        }
        add_msg(m_info, ngettext("This book takes %d minute to read.",
                         "This book takes %d minutes to read.", reading->time),
                reading->time );

        if (!(reading->recipes.empty())) {
            std::string recipes = "";
            size_t index = 1;
            for( auto iter = reading->recipes.begin();
                 iter != reading->recipes.end(); ++iter, ++index ) {
                recipes += item::nname( iter->first->result );
                if(index == reading->recipes.size() - 1) {
                    recipes += _(" and "); // Who gives a fuck about an oxford comma?
                } else if(index != reading->recipes.size()) {
                    recipes += _(", ");
                }
            }
            std::string recipe_line = string_format(
                ngettext("This book contains %1$d crafting recipe: %2$s",
                         "This book contains %1$d crafting recipes: %2$s", reading->recipes.size()),
                reading->recipes.size(), recipes.c_str());
            add_msg(m_info, "%s", recipe_line.c_str());
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
        if( skill == nullptr || (skillLevel(skill) < reading->req) ) {
            if( recipe_learned ) {
                add_msg(m_info, _("The rest of the book is currently still beyond your understanding."));
            }

            activity.type = ACT_NULL;
            return;
        }
    }

    if( skill != nullptr && skillLevel(skill) < reading->level ) {
        int originalSkillLevel = skillLevel( skill );
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

        skillLevel(skill).readBook( min_ex, max_ex, reading->level );

        add_msg(_("You learn a little about %s! (%d%%)"), skill->name().c_str(),
                skillLevel(skill).exercise());

        if (skillLevel(skill) == originalSkillLevel && activity.get_value(0) == 1) {
            // continuously read until player gains a new skill level
            activity.type = ACT_NULL;
            read(activity.position);
            // Rooters root (based on time spent reading)
            int root_factor = (reading->time / 20);
            double foot_factor = footwear_factor();
            if( (has_trait("ROOTS2") || has_trait("ROOTS3")) &&
                g->m.has_flag("DIGGABLE", posx, posy) &&
                !foot_factor ) {
                if (hunger > -20) {
                    hunger -= root_factor * foot_factor;
                }
                if (thirst > -20) {
                    thirst -= root_factor * foot_factor;
                }
                mod_healthy_mod(root_factor * foot_factor);
            }
            if (activity.type != ACT_NULL) {
                return;
            }
        }

        int new_skill_level = skillLevel(skill);
        if (new_skill_level > originalSkillLevel) {
            add_msg(m_good, _("You increase %s to level %d."),
                    skill->name().c_str(),
                    new_skill_level);

            if(new_skill_level % 4 == 0) {
                //~ %s is skill name. %d is skill level
                add_memorial_log(pgettext("memorial_male", "Reached skill level %1$d in %2$s."),
                                   pgettext("memorial_female", "Reached skill level %1$d in %2$s."),
                                   new_skill_level, skill->name().c_str());
            }
        }

        if( skillLevel(skill) == reading->level ) {
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
            g->m.has_flag("DIGGABLE", posx, posy) &&
            !foot_factor ) {
            if (hunger > -20) {
                hunger -= root_factor * foot_factor;
            }
            if (thirst > -20) {
                thirst -= root_factor * foot_factor;
            }
            mod_healthy_mod(root_factor * foot_factor);
        }
        if (activity.type != ACT_NULL) {
            return;
        }
    } else if ( !reading->recipes.empty() && no_recipes ) {
        add_msg(m_info, _("You can no longer learn from %s."), book->type_name().c_str());
    }

    for( auto &m : reading->use_methods ) {
        m.call( this, book, false, pos() );
    }

    activity.type = ACT_NULL;
}

bool player::has_identified( std::string item_id ) const
{
    return items_identified.count( item_id ) > 0;
}

bool player::can_study_recipe(const itype &book)
{
    if( !book.book ) {
        return false;
    }
    for( auto &elem : book.book->recipes ) {
        if( !knows_recipe( elem.first ) &&
            ( elem.first->skill_used == NULL ||
              skillLevel( elem.first->skill_used ) >= elem.second ) ) {
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
        if( !knows_recipe( elem.first ) ) {
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
    for( auto iter = book.book->recipes.begin(); iter != book.book->recipes.end(); ++iter ) {
        if (!knows_recipe(iter->first) &&
            (iter->first->skill_used == NULL ||
             skillLevel(iter->first->skill_used) >= iter->second)) {
            if (iter->first->skill_used == NULL ||
                rng(0, 4) <= (skillLevel(iter->first->skill_used) - iter->second) / 2) {
                learn_recipe((recipe *)iter->first);
                add_msg(m_good, _("Learned a recipe for %s from the %s."),
                                item::nname( iter->first->result ).c_str(), book.nname(1).c_str());
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
    vehicle *veh = g->m.veh_at (posx, posy, vpart);
    const trap_id trap_at_pos = g->m.tr_at(posx, posy);
    const ter_id ter_at_pos = g->m.ter(posx, posy);
    const furn_id furn_at_pos = g->m.furn(posx, posy);
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
        int web = g->m.get_field_strength( point(posx, posy), fd_web );
            if (!webforce) {
            // At this point, it's kinda weird, but surprisingly comfy...
            if (web >= 3) {
                add_msg(m_good, _("These thick webs support your weight, and are strangely comfortable..."));
                websleeping = true;
            }
            else if (web > 0) {
                add_msg(m_info, _("You try to sleep, but the webs get in the way.  You brush them aside."));
                g->m.remove_field( posx, posy, fd_web );
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
         trap_at_pos == tr_cot || trap_at_pos == tr_rollmat ||
         trap_at_pos == tr_fur_rollmat || furn_at_pos == f_armchair ||
         furn_at_pos == f_sofa || furn_at_pos == f_hay || furn_at_pos == f_straw_bed ||
         ter_at_pos == t_improvised_shelter || (in_shell) || (websleeping) ||
         (veh && veh->part_with_feature (vpart, "SEAT") >= 0) ||
         (veh && veh->part_with_feature (vpart, "BED") >= 0)) ) {
        add_msg(m_good, _("This is a comfortable place to sleep."));
    } else if (ter_at_pos != t_floor && !plantsleep) {
        add_msg( terlist[ter_at_pos].movecost <= 2 ?
                 _("It's a little hard to get to sleep on this %s.") :
                 _("It's hard to get to sleep on this %s."),
                 terlist[ter_at_pos].name.c_str() );
    }
    add_effect("lying_down", 300);
}

bool player::can_sleep()
{
    int sleepy = 0;
    bool plantsleep = false;
    bool websleep = false;
    bool webforce = false;
    bool in_shell = false;
    if (has_addiction(ADD_SLEEP)) {
        sleepy -= 3;
    }
    if (has_trait("INSOMNIA")) {
        sleepy -= 8;
    }
    if (has_trait("EASYSLEEPER")) {
        sleepy += 8;
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
    vehicle *veh = g->m.veh_at(posx, posy, vpart);
    const trap_id trap_at_pos = g->m.tr_at(posx, posy);
    const ter_id ter_at_pos = g->m.ter(posx, posy);
    const furn_id furn_at_pos = g->m.furn(posx, posy);
    int web = g->m.get_field_strength( point(posx, posy), fd_web );
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
                sleepy -= g->m.move_cost(posx, posy);
            }
        // Not in a vehicle, start checking furniture/terrain/traps at this point in decreasing order
        } else if (furn_at_pos == f_bed) {
            sleepy += 5;
        } else if (furn_at_pos == f_makeshift_bed || trap_at_pos == tr_cot ||
                    furn_at_pos == f_sofa) {
            sleepy += 4;
        } else if (websleep && web >= 3) {
            sleepy += 4;
        } else if (trap_at_pos == tr_rollmat || trap_at_pos == tr_fur_rollmat ||
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
            sleepy -= g->m.move_cost(posx, posy);
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
    if (fatigue < 192) {
        sleepy -= int( (192 - fatigue) / 4);
    } else {
        sleepy += int((fatigue - 192) / 16);
    }
    sleepy += rng(-8, 8);
    sleepy -= 2 * stim;
    if (sleepy > 0) {
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
    remove_effect("sleep");
    remove_effect("lying_down");
}

std::string player::is_snuggling()
{
    auto begin = g->m.i_at( posx, posy ).begin();
    auto end = g->m.i_at( posx, posy ).end();

    if( in_vehicle ) {
        int vpart;
        vehicle *veh = g->m.veh_at( posx, posy, vpart );
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
// 2.5 is enough light for detail work.
float player::fine_detail_vision_mod()
{
    // PER_SLIME_OK implies you can get enough eyes around the bile
    // that you can generaly see.  There'll still be the haze, but
    // it's annoying rather than limiting.
    if (has_effect("blind") || ((has_effect("boomered")) &&
    !(has_trait("PER_SLIME_OK"))))
    {
        return 5;
    }
    if ( has_nv() )
    {
        return 1.5;
    }
    // flashlight is handled by the light level check below
    if (has_active_item("lightstrip"))
    {
        return 1;
    }
    if (LL_LIT <= g->m.light_at(posx, posy))
    {
        return 1;
    }

    float vision_ii = 0;
    if (g->m.light_at(posx, posy) == LL_LOW) { vision_ii = 4; }
    else if (g->m.light_at(posx, posy) == LL_DARK) { vision_ii = 5; }

    if (has_item_with_flag("LIGHT_2")){
        vision_ii -= 2;
    } else if (has_item_with_flag("LIGHT_1")){
        vision_ii -= 1;
    }

    if (has_trait("NIGHTVISION")) { vision_ii -= .5; }
    else if (has_trait("ELFA_NV")) { vision_ii -= 1; }
    else if (has_trait("NIGHTVISION2") || has_trait("FEL_NV")) { vision_ii -= 2; }
    else if (has_trait("NIGHTVISION3") || has_trait("ELFA_FNV") || is_wearing("rm13_armor_on") ||
      has_trait("CEPH_VISION")) {
        vision_ii -= 3;
    }

    if (vision_ii < 1) { vision_ii = 1; }
    return vision_ii;
}

int player::get_wind_resistance(body_part bp) const
{
    int coverage = 0;
    float totalExposed = 1.0;
    int totalCoverage = 0;
    int penalty = 100;

    for (auto &i : worn)
    {
        if (i.covers(bp))
        {
            if (i.made_of("leather") || i.made_of("plastic") || i.made_of("bone") || i.made_of("chitin") || i.made_of("nomex"))
            {
                penalty = 10; // 90% effective
            }
            else if (i.made_of("cotton"))
            {
                penalty = 30;
            }
            else if (i.made_of("wool"))
            {
                penalty = 40;
            }
            else
            {
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

int bestwarmth( const std::vector< item > &its, const std::string &flag )
{
    int best = 0;
    for( auto &w : its ) {
        if( w.has_flag( flag ) && w.get_warmth() > best ) {
            best = w.get_warmth();
        }
    }
    return best;
}

int player::warmth(body_part bp) const
{
    int ret = 0, warmth = 0;

    // If the player is not wielding anything big, check if hands can be put in pockets
    if( ( bp == bp_hand_l || bp == bp_hand_r ) && weapon.volume() < 2 && 
        ( temp_conv[bp] <= BODYTEMP_NORM || temp_cur[bp] <= BODYTEMP_COLD ) ) {
        ret += bestwarmth( worn, "POCKETS" );
    }

    // If the player's head is not encumbered, check if hood can be put up
    if( bp == bp_head && encumb( bp_head ) < 1 &&
        ( temp_conv[bp] <= BODYTEMP_NORM || temp_cur[bp] <= BODYTEMP_COLD ) ) {
        ret += bestwarmth( worn, "HOOD" );
    }

    // If the player's mouth is not encumbered, check if collar can be put up
    if( bp == bp_mouth && encumb( bp_mouth ) < 1 &&
        ( temp_conv[bp] <= BODYTEMP_NORM || temp_cur[bp] <= BODYTEMP_COLD ) ) {
        ret += bestwarmth( worn, "COLLAR" );
    }

    for (auto &i : worn) {
        if( i.covers( bp ) ) {
            warmth = i.get_warmth();
            // Wool items do not lose their warmth due to being wet.
            // Warmth is reduced by 0 - 66% based on wetness.
            if (!i.made_of("wool"))
            {
                warmth *= 1.0 - 0.66 * body_wetness[bp] / mDrenchEffect.at(bp);
            }
            ret += warmth;
        }
    }
    return ret;
}

int player::encumb( body_part bp ) const
{
    int iArmorEnc = 0;
    double iLayers = 0;
    return encumb(bp, iLayers, iArmorEnc);
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
 * reduce the encumbrance penalty by one, or if that is already 0, they reduce the layering effect.
 *
 * Use cases:
 * What would typically be considered normal "street clothes" should not be considered encumbering.
 * Tshirt, shirt, jacket on torso/arms, underwear and pants on legs, socks and shoes on feet.
 * This is currently handled by each of these articles of clothing
 * being on a different layer and/or body part, therefore accumulating no encumbrance.
 */
int player::encumb(body_part bp, double &layers, int &armorenc) const
{
    int ret = 0;
    double layer[MAX_CLOTHING_LAYER] = { };
    int level = 0;
    bool is_wearing_active_power_armor = false;
    for( auto &w : worn ) {
        if( w.active && w.is_power_armor() ) {
            is_wearing_active_power_armor = true;
            break;
        }
    }

    for (size_t i = 0; i < worn.size(); ++i) {
        if( worn[i].covers(bp) ) {
            if( worn[i].has_flag( "SKINTIGHT" ) ) {
                level = UNDERWEAR;
            } else if ( worn[i].has_flag( "WAIST" ) ) {
                level = WAIST_LAYER;
            } else if ( worn[i].has_flag( "OUTER" ) ) {
                level = OUTER_LAYER;
            } else if ( worn[i].has_flag( "BELTED") ) {
                level = BELTED_LAYER;
            } else {
                level = REGULAR_LAYER;
            }

            layer[level]++;
            if( worn[i].is_power_armor() && is_wearing_active_power_armor ) {
                armorenc += std::max( 0, worn[i].get_encumber() - 4);
            } else {
                armorenc += worn[i].get_encumber();
                // Fitted clothes will reduce either encumbrance or layering.
                if( worn[i].has_flag( "FIT" ) ) {
                    if( worn[i].get_encumber() > 0 && armorenc > 0 ) {
                        armorenc--;
                    } else if (layer[level] > 0) {
                        layer[level] -= .5;
                    }
                }
            }
        }
    }
    if (armorenc < 0) {
        armorenc = 0;
    }
    ret += armorenc;

    for( auto &elem : layer ) {
        layers += std::max( 0.0, elem - 1.0 );
    }

    if (layers > 0.0) {
        ret += layers;
    }

    if (volume_carried() > volume_capacity() - 2 && bp != bp_head) {
        ret += 3;
    }

    // Bionics and mutation
    if ( has_bionic("bio_stiff") && bp != bp_head && bp != bp_mouth && bp != bp_eyes ) {
        ret += 1;
    }
    if ( (has_trait("CHITIN3") || has_trait("CHITIN_FUR3") ) &&
      bp != bp_eyes && bp != bp_mouth ) {
        ret += 1;
    }
    if ( has_trait("SLIT_NOSTRILS") && bp == bp_mouth ) {
        ret += 1;
    }
    if ( has_trait("ARM_FEATHERS") && (bp == bp_arm_l || bp == bp_arm_r) ) {
        ret += 2;
    }
    if ( has_trait("INSECT_ARMS") && (bp == bp_arm_l || bp == bp_arm_r) ) {
        ret += 3;
    }
    if ( has_trait("ARACHNID_ARMS") && (bp == bp_arm_l || bp == bp_arm_r) ) {
        ret += 4;
    }
    if ( has_trait("PAWS") && (bp == bp_hand_l || bp == bp_hand_r) ) {
        ret += 1;
    }
    if ( has_trait("PAWS_LARGE") && (bp == bp_hand_l || bp == bp_hand_r) ) {
        ret += 2;
    }
    if ( has_trait("LARGE") && (bp == bp_arm_l || bp == bp_arm_r || bp == bp_torso )) {
        ret += 1;
    }
    if ( has_trait("WINGS_BUTTERFLY") && (bp == bp_torso )) {
        ret += 1;
    }
    if ( has_trait("SHELL2") && (bp == bp_torso )) {
        ret += 1;
    }
    if ((bp == bp_hand_l || bp == bp_hand_r) &&
        (has_trait("ARM_TENTACLES") || has_trait("ARM_TENTACLES_4") ||
         has_trait("ARM_TENTACLES_8")) ) {
        ret += 3;
    }
    if ((bp == bp_hand_l || bp == bp_hand_r) &&
        (has_trait("CLAWS_TENTACLE") )) {
        ret += 2;
    }
    if (bp == bp_mouth &&
        ( has_bionic("bio_nostril") ) ) {
        ret += 1;
    }
    if ((bp == bp_hand_l || bp == bp_hand_r) &&
        ( has_bionic("bio_thumbs") ) ) {
        ret += 2;
    }
    if (bp == bp_eyes &&
        ( has_bionic("bio_pokedeye") ) ) {
        ret += 1;
    }
    if ( ret < 0 ) {
        ret = 0;
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

void get_armor_on(player* p, body_part bp, std::vector<int>& armor_indices) {
    for (size_t i = 0; i < p->worn.size(); i++) {
        if (p->worn[i].covers(bp)) {
            armor_indices.push_back(i);
        }
    }
}

// mutates du, returns true if armor was damaged
bool player::armor_absorb(damage_unit& du, item& armor) {
    float mitigation = 0; // total amount of damage mitigated
    float effective_resist = resistances(armor).get_effective_resist(du);
    bool armor_damaged = false;

    std::string pre_damage_name = armor.tname();
    std::string pre_damage_adj = armor.get_base_material().dmg_adj(armor.damage);

    if (rng(0,100) <= armor.get_coverage()) {
        if (armor.is_power_armor()) { // TODO: add some check for power armor
        }

        mitigation = std::min(effective_resist, du.amount);
        du.amount -= mitigation; // mitigate the damage first

        // if the post-mitigation amount is greater than the amount
        if ((du.amount > effective_resist && !one_in(du.amount) && one_in(2)) ||
                // or if it isn't, but 1/50 chance
                (du.amount <= effective_resist && !armor.has_flag("STURDY")
                && !armor.is_power_armor() && one_in(200))) {
            armor_damaged = true;
            armor.damage++;
            auto &material = armor.get_random_material();
            std::string damage_verb = du.type == DT_BASH
                ? material.bash_dmg_verb()
                : material.cut_dmg_verb();

            // add "further" if the damage adjective and verb are the same
            std::string format_string = pre_damage_adj == damage_verb
                ? _("Your %s is %s further!")
                : _("Your %s is %s!");
            add_msg_if_player( m_bad, format_string.c_str(), pre_damage_name.c_str(),
                                      damage_verb.c_str());
            //item is damaged
            if( is_player() ) {
                SCT.add(xpos(), ypos(),
                    NORTH,
                    pre_damage_name, m_neutral,
                    damage_verb, m_info);
            }
        }
    }
    return armor_damaged;
}
void player::absorb_hit(body_part bp, damage_instance &dam) {
    for( auto &elem : dam.damage_units ) {

        // Recompute the armor indices for every damage unit because we may have
        // destroyed armor earlier in the loop.
        std::vector<int> armor_indices;

        get_armor_on(this,bp,armor_indices);

        // CBMs absorb damage first before hitting armor
        if (has_active_bionic("bio_ads")) {
            if( elem.amount > 0 && power_level > 24 ) {
                if( elem.type == DT_BASH )
                    elem.amount -= rng( 1, 8 );
                else if( elem.type == DT_CUT )
                    elem.amount -= rng( 1, 4 );
                else if( elem.type == DT_STAB )
                    elem.amount -= rng( 1, 2 );
                power_level -= 25;
            }
            if( elem.amount < 0 )
                elem.amount = 0;
        }

        // The worn vector has the innermost item first, so
        // iterate reverse to damage the outermost (last in worn vector) first.
        for (std::vector<int>::reverse_iterator armor_it = armor_indices.rbegin();
                armor_it != armor_indices.rend(); ++armor_it) {

            const int index = *armor_it;

            armor_absorb( elem, worn[index] );

            // now check if armor was completely destroyed and display relevant messages
            // TODO: use something less janky than the old code for this check
            if (worn[index].damage >= 5) {
                //~ %s is armor name
                add_memorial_log(pgettext("memorial_male", "Worn %s was completely destroyed."),
                                 pgettext("memorial_female", "Worn %s was completely destroyed."),
                                 worn[index].tname().c_str());
                add_msg_player_or_npc( m_bad, _("Your %s is completely destroyed!"),
                                              _("<npcname>'s %s is completely destroyed!"),
                                              worn[index].tname().c_str() );
                worn.erase(worn.begin() + index);
            }
        }
    }
}

// TODO: move the ONE caller of this in player::hitall to call absorb_hit() instead and
// get rid of this.
void player::absorb(body_part bp, int &dam, int &cut)
{
    int arm_bash = 0, arm_cut = 0;
    bool cut_through = true;      // to determine if cutting damage penetrates multiple layers of armor
    int bash_absorb = 0;      // to determine if lower layers of armor get damaged

    // CBMS absorb damage first before hitting armor
    if (has_active_bionic("bio_ads")) {
        if (dam > 0 && power_level > 24) {
            dam -= rng(1, 8);
            power_level -= 25;
        }
        if (cut > 0 && power_level > 24) {
            cut -= rng(0, 4);
            power_level -= 25;
        }
        if (dam < 0) {
            dam = 0;
        }
        if (cut < 0) {
            cut = 0;
        }
    }

    // determines how much damage is absorbed by armor
    // zero if damage misses a covered part
    int bash_reduction = 0;
    int cut_reduction = 0;

    // See, we do it backwards, iterating inwards
    for (int i = worn.size() - 1; i >= 0; i--) {
        if (worn[i].covers(bp)) {
            // first determine if damage is at a covered part of the body
            // probability given by coverage
            if (rng(0, 100) <= worn[i].get_coverage()) {
                // hit a covered part of the body, so now determine if armor is damaged
                arm_bash = worn[i].bash_resist();
                arm_cut  = worn[i].cut_resist();
                // also determine how much damage is absorbed by armor
                // factor of 3 to normalise for material hardness values
                bash_reduction = arm_bash / 3;
                cut_reduction = arm_cut / 3;

                // power armor first  - to depreciate eventually
                if (worn[i].is_power_armor()) {
                    if (cut > arm_cut * 2 || dam > arm_bash * 2) {
                        add_msg_if_player(m_bad, _("Your %s is damaged!"), worn[i].tname().c_str());
                        worn[i].damage++;
                    }
                } else { // normal armor
                    // determine how much the damage exceeds the armor absorption
                    // bash damage takes into account preceding layers
                    int diff_bash = (dam - arm_bash - bash_absorb < 0) ? -1 : (dam - arm_bash);
                    int diff_cut  = (cut - arm_cut  < 0) ? -1 : (cut - arm_cut);
                    bool armor_damaged = false;
                    std::string pre_damage_name = worn[i].tname();

                    // armor damage occurs only if damage exceeds armor absorption
                    // plus a luck factor, even if damage is below armor absorption (2% chance)
                    if ((dam > arm_bash && !one_in(diff_bash)) ||
                        (!worn[i].has_flag ("STURDY") && diff_bash == -1 && one_in(50))) {
                        armor_damaged = true;
                        worn[i].damage++;
                    }
                    bash_absorb += arm_bash;

                    // cut damage falls through to inner layers only if preceding layer was damaged
                    if (cut_through) {
                        if ((cut > arm_cut && !one_in(diff_cut)) ||
                            (!worn[i].has_flag ("STURDY") && diff_cut == -1 && one_in(50))) {
                            armor_damaged = true;
                            worn[i].damage++;
                        } else {
                            // layer of clothing was not damaged,
                            // so stop cutting damage from penetrating
                            cut_through = false;
                        }
                    }

                    // now check if armor was completely destroyed and display relevant messages
                    if (worn[i].damage >= 5) {
                      //~ %s is armor name
                      add_memorial_log(pgettext("memorial_male", "Worn %s was completely destroyed."),
                                       pgettext("memorial_female", "Worn %s was completely destroyed."),
                                       worn[i].tname().c_str());
                        add_msg_player_or_npc(m_bad, _("Your %s is completely destroyed!"),
                                                     _("<npcname>'s %s is completely destroyed!"),
                                                     worn[i].tname().c_str() );
                        worn.erase(worn.begin() + i);
                    } else if (armor_damaged) {
                        auto &material = worn[i].get_random_material();
                        std::string damage_verb = diff_bash > diff_cut ? material.bash_dmg_verb() :
                                                                         material.cut_dmg_verb();
                        add_msg_if_player( m_bad, _("Your %s is %s!"), pre_damage_name.c_str(),
                                                  damage_verb.c_str());
                    }
                } // end of armor damage code
            }
        }
        // reduce damage accordingly
        dam -= bash_reduction;
        cut -= cut_reduction;
    }
    // now account for CBMs and mutations
    if (has_bionic("bio_carbon")) {
        dam -= 2;
        cut -= 4;
    }
    if (bp == bp_head && has_bionic("bio_armor_head")) {
        dam -= 3;
        cut -= 3;
    } else if ((bp == bp_arm_l || bp == bp_arm_r) && has_bionic("bio_armor_arms")) {
        dam -= 3;
        cut -= 3;
    } else if (bp == bp_torso && has_bionic("bio_armor_torso")) {
        dam -= 3;
        cut -= 3;
    } else if ((bp == bp_leg_l || bp == bp_leg_r) && has_bionic("bio_armor_legs")) {
        dam -= 3;
        cut -= 3;
    } else if (bp == bp_eyes && has_bionic("bio_armor_eyes")) {
        dam -= 3;
        cut -= 3;
    }
    if (has_trait("THICKSKIN")) {
        cut--;
    }
    if (has_trait("THINSKIN")) {
        cut++;
    }
    if (has_trait("SCALES")) {
        cut -= 2;
    }
    if (has_trait("THICK_SCALES")) {
        cut -= 4;
    }
    if (has_trait("SLEEK_SCALES")) {
        cut -= 1;
    }
    if (has_trait("FEATHERS")) {
        dam--;
    }
    if (has_trait("AMORPHOUS")) {
        dam--;
        if (!(has_trait("INT_SLIME"))) {
            dam -= 3;
        }
    }
    if ((bp == bp_arm_l || bp == bp_arm_r) && has_trait("ARM_FEATHERS")) {
        dam--;
    }
    if (has_trait("FUR") || has_trait("LUPINE_FUR") || has_trait("URSINE_FUR")) {
        dam--;
    }
    if (bp == bp_head && has_trait("LYNX_FUR")) {
        dam--;
    }
    if (has_trait("FAT")) {
        cut --;
    }
    if (has_trait("CHITIN") || has_trait("CHITIN_FUR") || has_trait("CHITIN_FUR2")) {
        cut -= 2;
    }
    if (has_trait("CHITIN2")) {
        dam--;
        cut -= 4;
    }
    if (has_trait("CHITIN3") || has_trait("CHITIN_FUR3")) {
        dam -= 2;
        cut -= 8;
    }
    if (has_trait("PLANTSKIN")) {
        dam--;
    }
    if (has_trait("BARK")) {
        dam -= 2;
    }
    if ((bp == bp_foot_l || bp == bp_foot_r) && has_trait("HOOVES")) {
        cut--;
    }
    if (has_trait("LIGHT_BONES")) {
        dam *= 1.4;
    }
    if (has_trait("HOLLOW_BONES")) {
        dam *= 1.8;
    }

    // apply martial arts armor buffs
    dam -= mabuff_arm_bash_bonus();
    cut -= mabuff_arm_cut_bonus();

    if (dam < 0) {
        dam = 0;
    }
    if (cut < 0) {
        cut = 0;
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

int player::adjust_for_focus(int amount)
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

    bool isSavant = has_trait("SAVANT");

    const Skill* savantSkill = NULL;
    SkillLevel savantSkillLevel = SkillLevel();

    if (isSavant) {
        for( auto &skill : Skill::skills ) {
            if( skillLevel( skill ) > savantSkillLevel ) {
                savantSkill = skill;
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

    if (skillLevel(s) > cap) { //blunt grinding cap implementation for crafting
        amount = 0;
        int curLevel = skillLevel(s);
        if(is_player() && one_in(5)) {//remind the player intermittently that no skill gain takes place
            add_msg(m_info, _("This task is too simple to train your %s beyond %d."),
                    s->name().c_str(), curLevel);
        }
    }

    if (amount > 0 && level.isTraining()) {
        int oldLevel = skillLevel(s);
        skillLevel(s).train(amount);
        int newLevel = skillLevel(s);
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

void player::practice( std::string s, int amount, int cap )
{
    const Skill* aSkill = Skill::skill(s);
    practice( aSkill, amount, cap );
}

bool player::knows_recipe(const recipe *rec) const
{
    // do we know the recipe by virtue of it being autolearned?
    if( rec->autolearn ) {
        // Can the skill being trained can handle the difficulty of the task
        bool meets_requirements = false;
        if(rec->skill_used == NULL || get_skill_level(rec->skill_used) >= rec->difficulty){
            meets_requirements = true;
            //If there are required skills, insure their requirements are met, or we can't craft
            if(!rec->required_skills.empty()){
                for( auto iter = rec->required_skills.cbegin();
                     iter != rec->required_skills.cend(); ++iter ){
                    if( get_skill_level(iter->first) < iter->second ){
                        meets_requirements = false;
                    }
                }
            }
        }
        if(meets_requirements){
            return true;
        }
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
            const auto &recipes = candidate.type->book->recipes;
            for( auto book_recipe = recipes.cbegin();
                 book_recipe != recipes.cend(); ++book_recipe ) {
                // Does it have the recipe, and do we meet it's requirements?
                if( book_recipe->first->ident == r->ident &&
                    ( book_recipe->first->skill_used == NULL ||
                      get_skill_level(book_recipe->first->skill_used) >= book_recipe->second ) &&
                    ( difficulty == -1 || book_recipe->second < difficulty ) ) {
                    difficulty = book_recipe->second;
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

void player::learn_recipe(recipe *rec)
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

std::vector<item*> player::has_ammo(ammotype at)
{
    std::vector<item*> result = inv.all_ammo(at);
    if (weapon.is_of_ammo_type_or_contains_it(at)) {
        result.push_back(&weapon);
    }
    for( auto &elem : worn ) {
        if( elem.is_of_ammo_type_or_contains_it( at ) ) {
            result.push_back( &elem );
        }
    }
    return result;
}

std::vector<item *> player::has_exact_ammo( const ammotype &at, const itype_id &id )
{
    auto result = has_ammo( at );
    for( auto it = result.begin(); it != result.end(); ) {
        if( ( *it )->is_of_type_or_contains_it( id ) ) {
            ++it;
        } else {
            it = result.erase( it );
        }
    }
    return result;
}

bool player::has_gun_for_ammo( const ammotype &at ) const
{
    return has_item_with( [at]( const item & it ) {
        // item::ammo_type considers the active gunmod.
        return it.is_gun() && it.ammo_type() == at;
    } );
}

std::string player::weapname(bool charges)
{
    if (!(weapon.is_tool() && dynamic_cast<it_tool*>(weapon.type)->max_charges <= 0) &&
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

void player::wield_contents(item *container, bool force_invlet,
                            std::string /*skill_used*/, int /*volume_factor*/)
{
    if(!(container->contents.empty())) {
        item& weap = container->contents[0];
        inv.assign_empty_invlet(weap, force_invlet);
        wield(&(i_add(weap)));
        container->contents.erase(container->contents.begin());
    } else {
        debugmsg("Tried to wield contents of empty container (player::wield_contents)");
    }
}

void player::store(item* container, item* put, std::string skill_used, int volume_factor)
{
    int lvl = skillLevel(skill_used);
    moves -= (lvl == 0) ? ((volume_factor + 1) * put->volume()) : (volume_factor * put->volume()) / lvl;
    container->put_in(i_rem(put));
}

nc_color encumb_color(int level)
{
 if (level < 0)
  return c_green;
 if (level == 0)
  return c_ltgray;
 if (level < 4)
  return c_yellow;
 if (level < 7)
  return c_ltred;
 return c_red;
}

SkillLevel& player::skillLevel(std::string ident)
{
    return _skills[Skill::skill(ident)];
}

SkillLevel& player::skillLevel(const Skill* _skill)
{
    return _skills[_skill];
}

SkillLevel player::get_skill_level(const Skill* _skill) const
{
    for( const auto &elem : _skills ) {
        if( elem.first == _skill ) {
            return elem.second;
        }
    }
    return SkillLevel();
}

SkillLevel player::get_skill_level(const std::string &ident) const
{
    const Skill* sk = Skill::skill(ident);
    return get_skill_level(sk);
}

void player::copy_skill_levels(const player *rhs)
{
    _skills = rhs->_skills;
}

void player::set_skill_level(const Skill* _skill, int level)
{
    skillLevel(_skill).level(level);
}

void player::set_skill_level(std::string ident, int level)
{
    skillLevel(ident).level(level);
}

void player::boost_skill_level(const Skill* _skill, int level)
{
    skillLevel(_skill).level(level+skillLevel(_skill));
}

void player::boost_skill_level(std::string ident, int level)
{
    skillLevel(ident).level(level+skillLevel(ident));
}

int player::get_melee() const
{
    return get_skill_level("melee");
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
    point adjacent = adjacent_tile();
    power_level -= 75;
    if (adjacent.x != posx || adjacent.y != posy)
    {
        posx = adjacent.x;
        posy = adjacent.y;
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
point player::adjacent_tile()
{
    std::vector<point> ret;
    trap_id curtrap;
    int dangerous_fields;
    for( int i = posx - 1; i <= posx + 1; i++ ) {
        for( int j = posy - 1; j <= posy + 1; j++ ) {
            if( i == posx && j == posy ) {
                // don't consider player position
                continue;
            }
            curtrap = g->m.tr_at(i, j);
            if( g->mon_at(i, j) == -1 && g->npc_at(i, j) == -1 && g->m.move_cost(i, j) > 0 &&
                (curtrap == tr_null || traplist[curtrap]->is_benign()) ) {
                // only consider tile if unoccupied, passable and has no traps
                dangerous_fields = 0;
                auto &tmpfld = g->m.field_at(i, j);
                for( auto &fld : tmpfld ) {
                    const field_entry &cur = fld.second;
                    if( cur.is_dangerous() ) {
                        dangerous_fields++;
                    }
                }
                if (dangerous_fields == 0) {
                    ret.push_back(point(i, j));
                }
            }
        }
    }
    if( ret.size() ) {
        return ret[ rng( 0, ret.size() - 1 ) ];   // return a random valid adjacent tile
    }
    return point(posx, posy);           // or return player position if no valid adjacent tiles
}

// --- Library functions ---
// This stuff could be moved elsewhere, but there
// doesn't seem to be a good place to put it right now.

// Basic logistic function.
double player::logistic(double t)
{
    return 1 / (1 + exp(-t));
}

// Logistic curve [-6,6], flipped and scaled to
// range from 1 to 0 as pos goes from min to max.
double player::logistic_range(int min, int max, int pos)
{
    const double LOGI_CUTOFF = 4;
    const double LOGI_MIN = logistic(-LOGI_CUTOFF);
    const double LOGI_MAX = logistic(+LOGI_CUTOFF);
    const double LOGI_RANGE = LOGI_MAX - LOGI_MIN;
    // Anything beyond [min,max] gets clamped.
    if (pos < min)
    {
        return 1.0;
    }
    else if (pos > max)
    {
        return 0.0;
    }

    // Normalize the pos to [0,1]
    double range = max - min;
    double unit_pos = (pos - min) / range;

    // Scale and flip it to [+LOGI_CUTOFF,-LOGI_CUTOFF]
    double scaled_pos = LOGI_CUTOFF - 2 * LOGI_CUTOFF * unit_pos;

    // Get the raw logistic value.
    double raw_logistic = logistic(scaled_pos);

    // Scale the output to [0,1]
    return (raw_logistic - LOGI_MIN) / LOGI_RANGE;
}

// Calculates portions favoring x, then y, then z
void player::calculate_portions(int &x, int &y, int &z, int maximum)
{
    z = std::min(z, std::max(maximum - x - y, 0));
    y = std::min(y, std::max(maximum - x , 0));
    x = std::min(x, std::max(maximum, 0));
}

void player::environmental_revert_effect()
{
    illness.clear();
    addictions.clear();
    morale.clear();

    for (int part = 0; part < num_hp_parts; part++) {
        hp_cur[part] = hp_max[part];
    }
    hunger = 0;
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

bool player::is_invisible() const {
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

int player::visibility( bool, int ) const { // 0-100 %
    if ( is_invisible() ) {
        return 0;
    }
    // todo:
    // if ( dark_clothing() && light check ...
    return 100;
}

void player::set_destination(const std::vector<point> &route)
{
    auto_move_route = route;
}

void player::clear_destination()
{
    auto_move_route.clear();
    next_expected_position.x = -1;
    next_expected_position.y = -1;
}

bool player::has_destination() const
{
    return !auto_move_route.empty();
}

std::vector<point> &player::get_auto_move_route()
{
    return auto_move_route;
}

action_id player::get_next_auto_move_direction()
{
    if (!has_destination()) {
        return ACTION_NULL;
    }

    if (next_expected_position.x != -1) {
        if (posx != next_expected_position.x || posy != next_expected_position.y) {
            // We're off course, possibly stumbling or stuck, cancel auto move
            return ACTION_NULL;
        }
    }

    next_expected_position = auto_move_route.front();
    auto_move_route.erase(auto_move_route.begin());

    int dx = next_expected_position.x - posx;
    int dy = next_expected_position.y - posy;

    if (abs(dx) > 1 || abs(dy) > 1) {
        // Should never happen, but check just in case
        return ACTION_NULL;
    }

    return get_movement_direction_from_delta(dx, dy);
}

void player::shift_destination(int shiftx, int shifty)
{
    if (next_expected_position.x != -1) {
        next_expected_position.x += shiftx;
        next_expected_position.y += shifty;
    }

    for( auto &elem : auto_move_route ) {
        elem.x += shiftx;
        elem.y += shifty;
    }
}

bool player::has_weapon() const {
    return !unarmed_attack();
}

m_size player::get_size() const {
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

field_id player::playerBloodType() const {
    if (player::has_trait("THRESH_PLANT"))
        return fd_blood_veggy;
    if (player::has_trait("THRESH_INSECT") || player::has_trait("THRESH_SPIDER"))
        return fd_blood_insect;
    if (player::has_trait("THRESH_CEPHALOPOD"))
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

bool player::sees( const point t, int &bresenham_slope ) const
{
    static const std::string str_bio_night("bio_night");
    const int wanted_range = rl_dist( pos(), t );
    bool can_see = Creature::sees( t, bresenham_slope );
    // Only check if we need to override if we already came to the opposite conclusion.
    if( can_see && wanted_range < 15 && wanted_range > sight_range(1) &&
        has_active_bionic(str_bio_night) ) {
        can_see = false;
    }
    // Clairvoyance is a really expensive check,
    // so try to avoid making it if at all possible.
    if( !can_see && wanted_range < MAX_CLAIRVOYANCE && wanted_range < clairvoyance() ) {
        return true;
    }
    return can_see;
}

bool player::sees( const Creature &critter, int &bresenham_slope ) const
{
    // This handles only the player/npc specific stuff (monsters don't have traits or bionics).
    const int dist = rl_dist( pos(), critter.pos() );
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
    return Creature::sees( critter, bresenham_slope );
}

bool player::can_pickup(bool print_msg) const
{
    if (weapon.has_flag("NO_PICKUP")) {
        if( print_msg && is_player() ) {
            add_msg(m_info, _("You cannot pick up items with your %s!"), weapon.tname().c_str());
        }
        return false;
    }
    return true;
}

bool player::has_container_for(const item &newit)
{
    if (!newit.made_of(LIQUID)) {
        // Currently only liquids need a container
        return true;
    }
    unsigned charges = newit.charges;
    charges -= weapon.get_remaining_capacity_for_liquid(newit);
    for (size_t i = 0; i < worn.size() && charges > 0; i++) {
        charges -= worn[i].get_remaining_capacity_for_liquid(newit);
    }
    for (size_t i = 0; i < inv.size() && charges > 0; i++) {
        const std::list<item>&items = inv.const_stack(i);
        // Assume that each item in the stack has the same remaining capacity
        charges -= items.front().get_remaining_capacity_for_liquid(newit) * items.size();
    }
    return charges <= 0;
}




nc_color player::bodytemp_color(int bp)
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
};
void player::add_msg_player_or_npc(const char* player_str, const char* npc_str, ...) const
{
    va_list ap;
    va_start(ap, npc_str);
    Messages::vadd_msg(player_str, ap);
    va_end(ap);
};
void player::add_msg_if_player(game_message_type type, const char* msg, ...) const
{
    va_list ap;
    va_start(ap, msg);
    Messages::vadd_msg(type, msg, ap);
    va_end(ap);
};
void player::add_msg_player_or_npc(game_message_type type, const char* player_str, const char* npc_str, ...) const
{
    va_list ap;
    va_start(ap, npc_str);
    Messages::vadd_msg(type, player_str, ap);
    va_end(ap);
};

bool player::knows_trap(int x, int y) const
{
    const point a = g->m.getabs( x, y );
    const tripoint p( a.x, a.y, g->get_abs_levz() );
    return known_traps.count(p) > 0;
}

void player::add_known_trap(int x, int y, const std::string &t)
{
    const point a = g->m.getabs( x, y );
    const tripoint p( a.x, a.y, g->get_abs_levz() );
    if (t == "tr_null") {
        known_traps.erase(p);
    } else {
        known_traps[p] = t;
    }
}

bool player::is_deaf() const
{
    return has_effect("deaf") || worn_with_flag("DEAF");
}

int player::print_info(WINDOW* w, int vStart, int, int column) const
{
    mvwprintw( w, vStart++, column, _( "You (%s)" ), name.c_str() );
    return vStart;
}

bool player::is_visible_in_range( const Creature &critter, const int range ) const
{
    return sees( critter ) && rl_dist( pos(), critter.pos() ) <= range;
}

std::vector<Creature *> player::get_visible_creatures( const int range ) const
{
    std::vector<Creature *> result;
    for( size_t i = 0; i < g->num_zombies(); i++ ) {
        auto &critter = g->zombie( i );
        if( !critter.is_dead() && is_visible_in_range( critter, range ) ) {
            result.push_back( &critter );
        }
    }
    for( auto & n : g->active_npc ) {
        if( n != this && is_visible_in_range( *n, range ) ) {
            result.push_back( n );
        }
    }
    if( this != &g->u && is_visible_in_range( g->u, range ) ) {
        result.push_back( &g->u );
    }
    return result;
}

void player::place_corpse()
{
    std::vector<item *> tmp = inv_dump();
    item body;
    body.make_corpse( "corpse", GetMType( "mon_null" ), calendar::turn, name );
    for( auto itm : tmp ) {
        g->m.add_item_or_charges( posx, posy, *itm );
    }
    for( auto & bio : my_bionics ) {
        if( item::type_is_defined( bio.id ) ) {
            body.put_in( item( bio.id, calendar::turn ) );
        }
    }
    int pow = max_power_level;
    while( pow >= 100 ) {
        if( pow >= 250 ) {
            pow -= 250;
            body.contents.push_back( item( "bio_power_storage_mkII", calendar::turn ) );
        } else {
            pow -= 100;
            body.contents.push_back( item( "bio_power_storage", calendar::turn ) );
        }
    }
    g->m.add_item_or_charges( posx, posy, body );
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
    const auto range = sight_range( DAYLIGHT_LEVEL );
    if( is_player() ) {
        return g->m.pl_sees(critter.xpos(), critter.ypos(), range );
    } else {
        int bresenham_slope;
        return g->m.sees(critter.xpos(), critter.ypos(), range, bresenham_slope );
    }
}

std::vector<std::string> player::get_overlay_ids() const {
    std::vector<std::string> rval;

    // first get mutations
    for(const std::string& mutation : my_mutations) {
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
    g->sound(posx, posy, 10, _("Pouf!")); //~spore-release sound
            monster spore(GetMType("mon_spore"));
            int sporex, sporey;
            int mondex;
            for (int i = -1; i <= 1; i++) {
                for (int j = -1; j <= 1; j++) {
                    if (i == 0 && j == 0) {
                        continue;
                    }
                    sporex = posx + i;
                    sporey = posy + j;
                    mondex = g->mon_at(sporex, sporey);
                    if (g->m.move_cost(sporex, sporey) > 0) {
                        if (mondex != -1) { // Spores hit a monster
                            if (g->u.sees(sporex, sporey) &&
                                !g->zombie(mondex).type->in_species("FUNGUS")) {
                                add_msg(_("The %s is covered in tiny spores!"),
                                        g->zombie(mondex).name().c_str());
                            }
                            monster &critter = g->zombie( mondex );
                            if( !critter.make_fungus() ) {
                                critter.die( this );
                            }
                        } else if (one_in(3) && g->num_zombies() <= 1000) { // Spawn a spore
                        spore.spawn(sporex, sporey);
                        spore.friendly = -1;
                        g->add_zombie(spore);
                        }
                    }
                }
            }
}

void player::blossoms()
{
    // Player blossoms are shorter-ranged, but you can fire much more frequently if you like.
     g->sound(posx, posy, 10, _("Pouf!"));
     for (int i = posx - 2; i <= posx + 2; i++) {
        for (int j = posy - 2; j <= posy + 2; j++) {
                g->m.add_field( i, j, fd_fungal_haze, rng(1, 2));
        }
    }
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
