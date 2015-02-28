#include "player.h"
#include "mutation.h"
#include "game.h"
#include "translations.h"
#include "messages.h"
#include "monster.h"
#include "monstergenerator.h"
#include "overmapbuffer.h"
#include "sounds.h"

#include <math.h>    //sqrt
#include <algorithm> //std::min
#include <sstream>

mutation* Character::mutation_by_invlet(char ch) {
    for( auto &elem : my_mutations ) {
        if( elem.get_key() == ch ) {
            return &elem;
        }
    }
    return nullptr;
}

bool Character::has_mut(const muttype_id &flag) const
{
    return my_mutations.find(flag) != my_mutations.end();
}

bool Character::is_trait(const muttype_id &flag) const
{
    // Returns 1 or 0 depending on the existence of the trait
    return my_traits.count(flag);
}

bool Character::add_mutation(const muttype_id &flag, bool message)
{
    // Only add the mutation if we don't already have it
    if (!has_mut(flag)) {
        char new_key = ' ';
        // Find a free letter
        for( const auto &letter : inv_chars ) {
            bool found = false;
            for( auto &m : my_mutations ) {
                if( letter == m.second.get_key() ) {
                    found = true;
                    break;
                }
            }
            if( !found ) {
                new_key = letter;
                break;
            }
        }
        
        // Add the new mutation and set its key
        my_mutations[flag] = new mutation();
        my_mutations[flag].set_key(new_key);
        
        // Handle mutation gain effects
        mutation_effect(flag);
        recalc_sight_limits();
        
        // Print message if needed
        if (message && is_player()) {
            game_message_type rating;
            // Rating = rating for mutation addition
            mut_rating rate = mut_types[flag].get_rating();
            if (rate == mut_good) {
                rating = m_good;
            } else if (rate == mut_bad) {
                rating = m_bad;
            } else if (rate == mut_mixed) {
                rating = m_mixed;
            } else {
                rating = m_neutral;
            }
            if (is_trait(flag)) {
                add_msg(rating, _("Your innate trait %s returns!"),
                                mut_types[flag].get_name().c_str());
            } else {
                add_msg(rating, _("You gain a mutation called %s!"),
                                mut_types[flag].get_name().c_str());
            }
        }

        set_cat_levels();
        // Once drench is moved to Character we can remove the cast.
        player p = dynamic_cast<player> this;
        if (p != NULL) {
            drench_mut_calc();
        }
        return true;
    }
    return false;
}

void Character::add_trait(const muttype_id &flag, bool message)
{
    if (add_mutation(flag), message) {
        // If we successfully added the mutation make it a trait
        my_traits.insert(flag);
    }
    // Mutation add effects are handled in add_mutation().
}

void Character::remove_mutation(const muttype_id &flag, bool message)
{
    if (!has_mut(flag)) {
        my_mutations.erase(flag);
        // Handle mutation loss effects
        mutation_loss_effect(flag);
        recalc_sight_limits();
            
        // Print message if needed
        if (message && is_player()) {
            game_message_type rating;
            // Rating = !rating for mutation addition
            mut_rating rate = mut_types[flag].get_rating();
            if (rate == mut_good) {
                rating = m_bad;
            } else if (rate == mut_bad) {
                rating = m_good;
            } else if (rate == mut_mixed) {
                rating = m_mixed;
            } else {
                rating = m_neutral;
            }
            if (is_trait(flag)) {
                add_msg(rating, _("You lose your innate %s trait."),
                            mut_types[flag].get_name().c_str());
            } else {
                add_msg(rating, _("You lose your %s mutation."),
                            mut_types[flag].get_name().c_str());
            }
        }
        
        // Check to see if any traits are no longer suppressed
        for (auto &i : mut_types[flag].get_cancels()) {
            if (!has_conflicting_mut(flag)) {
                add_mutation(i);
            }
        }

        set_cat_levels();
        // Once drench is moved to Character we can remove the cast.
        player p = dynamic_cast<player> this;
        if (p != NULL) {
            drench_mut_calc();
        }
    }
}

void Character::replace_mutation(const muttype_id &flag1, const muttype_id &flag2)
{
    add_mutation(flag1, false);
    remove_mutation(falg2, false);
    if (is_player()) {
        game_message_type rating;
        // Rating = !rating for mutation addition
        mut_rating rate = mut_types[flag].get_rating();
        if (rate == mut_good) {
            rating = m_bad;
        } else if (rate == mut_bad) {
            rating = m_good;
        } else if (rate == mut_mixed) {
            rating = m_mixed;
        } else {
            rating = m_neutral;
        }
        if (is_trait(flag1)) {
            add_msg(rating, _("Your %1$s mutation turns into %2$s!"),
                    mut_types[flag1].get_name().c_str(),
                    mut_types[flag2].get_name().c_str());
        } else {
            add_msg(rating, _("Your innate %1$s trait turns into %2$s!"),
                    mut_types[flag1].get_name().c_str(),
                    mut_types[flag2].get_name().c_str());
        }
    }
}

std::vector<muttype_id> Character::get_dependant_muts(const muttype_id &flag) const
{
    std::vector<muttype_id> dependants;
    // Go through all of our mutations
    for (auto &mut : my_mutations) {
        bool pushed = false;
        // Push back those who replace the given mutation.
        for (auto &i : mut.second.get_replaces()) {
            if (i == flag) {
                dependants.push_back(mut.first);
                pushed = true;
                break;
            }
        }
        if (pushed) {
            // Short circuit if needed.
            continue;
        }
        // Push back those who require the given mutation.
        for (auto &i : mut.second.get_prereqs()) {
            if (i == flag) {
                dependants.push_back(mut.first);
                pushed = true;
                break;
            }
        }
        if (pushed) {
            // Short circuit if needed.
            continue;
        }
        // Push back those who secondary require the given mutation.
        for (auto &i : mut.second.get_prereqs2()) {
            if (i == flag) {
                dependants.push_back(mut.first);
                pushed = true;
                break;
            }
        }
    }
    return dependants;
}

void Character::downgrade_mutation(const muttype_id &flag, bool trait_force)
{
    if (!has_mut(flag)) {
        // We don't have the mutation, abort!
        return;
    }
    
    if (!trait_force && is_trait(flag)) {
        // You can't downgrade traits unless you are forced to!
        return;
    }
    
    // Check if we have mutations that depend on this one that would need to be
    // removed first.
    std::vector<muttype_id> deps = get_dependant_muts(flag);
    if (!deps.empty()) {
        downgrade_mutation(deps[rng(0, deps.size() - 1)], trait_force);
        return;
    }
    
    // First check to see if we can downgrade to anything.
    if (mut_types[flag].get_replaces().empty()) {
        // If we can't find anything to downgrade to just remove the mutation.
        remove_mutation(flag);
    } else {
        // We found something to downgrade to, figure out which of the downgrades
        // are valid things to downgrade to.
        std::vector<muttype_id> valid_downgrades;
        for (auto &i : mut_types[flag].get_replaces()) {
            // First check if we have an opposite mutation
            if (has_conflicting_mut(i)) {
                // Not a valid thing to downgrade to, continue
                continue;
            }
            
            // Then check if we have all the prereqs for a downgrade
            bool valid = true;
            for (auto &check : i.get_prereqs()) {
                if (!has_mut(check)) {
                    // Missing a prereq!
                    valid = false;
                    break;
                }
            }
            // Check again for prereqs2 if needed
            if (valid) {
                for (auto &check : i.get_prereqs2()) {
                    if (!has_mut(check)) {
                        // Missing a prereq2!
                        valid = false;
                        break;
                    }
                }
            }
            
            // If it's still valid, add it to the list
            if (valid) {
                valid_downgrades.push_back(i);
            }
        }
        
        // Hopefully at this point we have a valid downgrade
        muttype_id replacement;
        if (!valid_downgrades.epmty()) {
            // Pick a random valid downgrade
            replacement = valid_downgrades[rng(0, valid_downgrades.size() - 1)]
        } else {
            // If we don't have any valid downgrades, force a random downgrade
            std::vector<muttype_id> choices = mut_types[flag].get_replaces();
            replacement = choices[rng(0, choices.size() - 1);
        }
        // Replace our old mutation with the new one.
        replace_mutation(flag, replacement);
    }
}

int mutation_type::get_mod(std::string arg, bool active) const
{
    int ret = 0;
    auto found = mods.find(std::make_pair(active, arg));
    if (found != mods.end()) {
        ret += found->second;
    }
    return ret;
}

void Character::apply_mods(const muttype_id &mut_id, bool add_remove, bool active)
{
    auto mut = &mut_types[mut];
    int sign = add_remove ? 1 : -1;
    int str_change = mut.get_mod("STR", active);
    str_max += sign * str_change;
    per_max += sign * mut.get_mod("PER", active);
    dex_max += sign * mut.get_mod("DEX", active);
    int_max += sign * mut.get_mod("INT", active);

    if( str_change != 0 ) {
        recalc_hp();
    }
}

void Character::mutation_effect(const muttype_id mut)
{
    bool is_u = is_player();
    bool destroy = false;
    std::vector<body_part> bps;

    if (mut == "TOUGH" || mut == "TOUGH2" || mut == "TOUGH3" || mut == "GLASSJAW" ||
        mut == "FLIMSY" || mut == "FLIMSY2" || mut == "FLIMSY3" ||
        mut == "MUT_TOUGH" || mut == "MUT_TOUGH2" || mut == "MUT_TOUGH3") {
        recalc_hp();

    } else if (mut == "WEBBED" || mut == "PAWS" || mut == "PAWS_LARGE" || mut == "ARM_TENTACLES" ||
               mut == "ARM_TENTACLES_4" || mut == "ARM_TENTACLES_8") {
        // Push off gloves
        bps.push_back(bp_hand_l);
        bps.push_back(bp_hand_r);

    } else if (mut == "TALONS") {
        // Destroy gloves
        destroy = true;
        bps.push_back(bp_hand_l);
        bps.push_back(bp_hand_r);

    } else if (mut == "BEAK" || mut == "BEAK_PECK" || mut == "BEAK_HUM" || mut == "MANDIBLES" ||
               mut == "SABER_TEETH") {
        // Destroy mouthwear
        destroy = true;
        bps.push_back(bp_mouth);

    } else if (mut == "MINOTAUR" || mut == "MUZZLE" || mut == "MUZZLE_BEAR" || mut == "MUZZLE_LONG" ||
               mut == "PROBOSCIS" || mut == "MUZZLE_RAT") {
        // Push off mouthwear
        bps.push_back(bp_mouth);

    } else if (mut == "HOOVES" || mut == "RAP_TALONS") {
        // Destroy footwear
        destroy = true;
        bps.push_back(bp_foot_l);
        bps.push_back(bp_foot_r);

    } else if (mut == "SHELL") {
        // Destroy torsowear
        destroy = true;
        bps.push_back(bp_torso);

    } else if ( (mut == "INSECT_ARMS") || (mut == "ARACHNID_ARMS") || (mut == "WINGS_BUTTERFLY") ) {
        // Push off torsowear
        bps.push_back(bp_torso);

    } else if (mut == "HORNS_CURLED" || mut == "CHITIN3") {
        // Push off all helmets
        bps.push_back(bp_head);

    } else if (mut == "HORNS_POINTED" || mut == "ANTENNAE" || mut == "ANTLERS") {
        // Push off non-cloth helmets
        bps.push_back(bp_head);

    } else if (mut == "HUGE") {
        // And there goes your clothing; by now you shouldn't need it anymore
        add_msg(m_bad, _("You rip out of your clothing!"));
        destroy = true;
        bps.push_back(bp_torso);
        bps.push_back(bp_leg_l);
        bps.push_back(bp_leg_r);
        bps.push_back(bp_arm_l);
        bps.push_back(bp_arm_r);
        bps.push_back(bp_hand_l);
        bps.push_back(bp_hand_r);
        bps.push_back(bp_head);
        bps.push_back(bp_foot_l);
        bps.push_back(bp_foot_r);

    } else if (mut == "STR_ALPHA") {
        if (str_max <= 6) {
            str_max = 8;
        } else if (str_max <= 7) {
            str_max = 11;
        } else if (str_max <= 14) {
            str_max = 15;
        } else {
            str_max = 18;
        }
        recalc_hp();
    } else if (mut == "DEX_ALPHA") {
        if (dex_max <= 6) {
            dex_max = 8;
        } else if (dex_max <= 7) {
            dex_max = 11;
        } else if (dex_max <= 14) {
            dex_max = 15;
        } else {
            dex_max = 18;
        }
    } else if (mut == "INT_ALPHA") {
        if (int_max <= 6) {
            int_max = 8;
        } else if (int_max <= 7) {
            int_max = 11;
        } else if (int_max <= 14) {
            int_max = 15;
        } else {
            int_max = 18;
        }
    } else if (mut == "INT_SLIME") {
        int_max *= 2; // Now, can you keep it? :-)

    } else if (mut == "PER_ALPHA") {
        if (per_max <= 6) {
            per_max = 8;
        } else if (per_max <= 7) {
            per_max = 11;
        } else if (per_max <= 14) {
            per_max = 15;
        } else {
            per_max = 18;
        }
    } else {
        apply_mods(mut, true, false);
    }

    std::string mutation_safe = "OVERSIZE";
    for (size_t i = 0; i < worn.size(); i++) {
        for( auto &bp : bps ) {
            if( ( worn[i].covers( bp ) ) && ( !( worn[i].has_flag( mutation_safe ) ) ) ) {
                if (destroy) {
                    if (is_u) {
                        add_msg(m_bad, _("Your %s is destroyed!"), worn[i].tname().c_str());
                    }

                    worn.erase(worn.begin() + i);

                } else {
                    if (is_u) {
                        add_msg(m_bad, _("Your %s is pushed off."), worn[i].tname().c_str());
                    }

                    int pos = player::worn_position_to_index(i);
                    g->m.add_item_or_charges(posx(), posy(), worn[i]);
                    i_rem(pos);
                }
                // Reset to the start of the vector
                i = 0;
            }
        }
    }
}

void Character::mutation_loss_effect(const muttype_id mut)
{
    if (mut == "TOUGH" || mut == "TOUGH2" || mut == "TOUGH3" || mut == "GLASSJAW" ||
        mut == "FLIMSY" || mut == "FLIMSY2" || mut == "FLIMSY3" ||
        mut == "MUT_TOUGH" || mut == "MUT_TOUGH2" || mut == "MUT_TOUGH3") {
        recalc_hp();

    } else if (mut == "STR_ALPHA") {
        if (str_max == 18) {
            str_max = 15;
        } else if (str_max == 15) {
            str_max = 8;
        } else if (str_max == 11) {
            str_max = 7;
        } else {
            str_max = 4;
        }
        recalc_hp();
    } else if (mut == "DEX_ALPHA") {
        if (dex_max == 18) {
            dex_max = 15;
        } else if (dex_max == 15) {
            dex_max = 8;
        } else if (dex_max == 11) {
            dex_max = 7;
        } else {
            dex_max = 4;
        }
    } else if (mut == "INT_ALPHA") {
        if (int_max == 18) {
            int_max = 15;
        } else if (int_max == 15) {
            int_max = 8;
        } else if (int_max == 11) {
            int_max = 7;
        } else {
            int_max = 4;
        }
    } else if (mut == "INT_SLIME") {
        int_max /= 2; // In case you have a freak accident with the debug menu ;-)

    } else if (mut == "PER_ALPHA") {
        if (per_max == 18) {
            per_max = 15;
        } else if (per_max == 15) {
            per_max = 8;
        } else if (per_max == 11) {
            per_max = 7;
        } else {
            per_max = 4;
        }
    } else {
        apply_mods(mut, false, false);
    }
}

bool Character::has_active_mutation(const muttype_id &flag) const
{
    // Constant-time check first
    if (mut_types[flag].activatable == false) {
        return false;
    }
    
    // Then do the more expensive checks
    if (has_mut(flag)) {
        return my_mutations[flag].is_active();
    }
    return false;
}

void Character::activate_mutation(const muttype_id &flag)
{
    if (!has_mut(flag)) {
        // We don't have the desired mutation!
        debugmsg("Tried to activate a nonexistent mutation!");
        return;
    }
    if (!my_mutations[flag].is_activatable()) {
        // It's not an activatable mutation!
        debugmsg("Tried to activate a non-activatable mutation!");
        return;
    }
    if (my_mutations[flag].is_active()) {
        // It's already active, abort
        return;
    }
    
    // Check cost limits
    std::unordered_map<std::string, int> act_costs;
    if ((act_costs["hunger"] > 0 && hunger >= 2800) ||
          (act_costs["thirst"] > 0 && thirst >= 520) ||
          (act_costs["fatigue"] > 0 && fatigue >= 575)) {
        // Do we want to activate regardless of the danger?
        if (!query_yn( _("You feel like you could hurt yourself using your %s! Activate anyways?",
                my_mutations[flag].get_name().c_str()))) {
            return;
        }
    }
    
    // We've decided to activate here!
    // Activate the mutation
    if (my_mutations[flag].activate(*this)) {
        if (is_player()) {
            add_msg(m_neutral, _("You activate your %s."), my_mutations[flag].get_name().c_str());
        }
        // We successfully activated, so pay the costs
        hunger += act_costs["hunger"];
        thirst += act_costs["thirst"];
        fatigue += act_costs["fatigue"];
    }
}

void mutation::get_costs(std::unordered_map<std::string, int> &cost)
{
    cost["hunger"] = mut_type->costs["hunger"];
    cost["thirst"] = mut_type->costs["thirst"];
    cost["fatigue"] = mut_type->costs["fatigue"];
}

int mutation::get_cost(std::string arg)
{
    return mut_type->costs[arg];
}

bool mutation::activate(Character &ch)
{
    bool activated = false;
    muttype_id id = mut_type->id;
    
    // Handle legacy mutation effects
    if (ch.is_player()) {
        const auto *p = dynamic_cast< player* >( &ch );
        if (p != nullptr) {
            activated = p->legacy_mut_effects(id);
        }
    }
    
    // Iff activated == false it means we broke out in the legacy check.
    if (activated) {
        activated = activation_effects(id, ch);
    }
    
    if (!activated) {
        // We canceled activation, return false
        return false;
    }
    
    // We didn't stop the deactivation.
    // Set charges to be equal to the duration of the activation.
    // No "this turn already used" reduction on purpose.
    charge = mut_types[id].duration;
    return true;
}

bool mutation::activation_effects(const muttype_id id, Character &ch)
{
    /* NEW ON-ACTIVATION MUTATION EFFECTS SHOULD GO HERE.
     * Cast ch to player if you really need to access player class variables;
     * DO NOT add new effects to player::legacy_mut_effects(). */
    
    // Mutation was not canceled in activation_effects(), return true.
    return true;
}
    
bool player::legacy_mut_effects(const muttype_id id)
{
    /* DO NOT ADD NEW MUTATIONS EFFECTS HERE.
     * New mutation activation effects should go in mutation::activation_effects().
     * This is for old mutation effects which could not be easily converted over
     * into the new system. As more things are moved from player to Character,
     * these should be able to be moved. */
    if (id == "BURROW"){
        if (is_underwater()) {
            add_msg_if_player(m_info, _("You can't do that while underwater."));
            return false;
        }
        int dirx, diry;
        if (!choose_adjacent(_("Burrow where?"), dirx, diry)) {
            return false;
        }

        if (dirx == g->u.posx() && diry == g->u.posy()) {
            add_msg_if_player(_("You've got places to go and critters to beat."));
            add_msg_if_player(_("Let the lesser folks eat their hearts out."));
            return false;
        }
        int turns;
        if (g->m.is_bashable(dirx, diry) && g->m.has_flag("SUPPORTS_ROOF", dirx, diry) &&
            g->m.ter(dirx, diry) != t_tree) {
            // Takes about 100 minutes (not quite two hours) base time.
            // Being better-adapted to the task means that skillful Survivors can do it almost twice as fast.
            turns = (100000 - 5000 * g->u.skillLevel("carpentry"));
        } else if (g->m.move_cost(dirx, diry) == 2 && g->levz == 0 &&
                   g->m.ter(dirx, diry) != t_dirt && g->m.ter(dirx, diry) != t_grass) {
            turns = 18000;
        } else {
            add_msg_if_player(m_info, _("You can't burrow there."));
            return false;
        }
        g->u.assign_activity(ACT_BURROW, turns, -1, 0);
        g->u.activity.placement = point(dirx, diry);
        add_msg_if_player(_("You tear into the %s with your teeth and claws."),
                          g->m.tername(dirx, diry).c_str());
        return true; // handled when the activity finishes
    } else if( id == "WEB_WEAVER" ) {
        g->m.add_field(posx(), posy(), fd_web, 1);
        add_msg(_("You start spinning web with your spinnerets!"));
        return true;
    } else if (id == "SLIMESPAWNER") {
        std::vector<point> valid;
        for (int x = posx() - 1; x <= posx() + 1; x++) {
            for (int y = posy() - 1; y <= posy() + 1; y++) {
                if (g->is_empty(x, y)) {
                    valid.push_back( point(x, y) );
                }
            }
        }
        // Oops, no room to divide!
        if (valid.size() == 0) {
            add_msg(m_bad, _("You focus, but are too hemmed in to birth a new slimespring!"));
            return false;
        }
        add_msg(m_good, _("You focus, and with a pleasant splitting feeling, birth a new slimespring!"));
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
        //~ Usual enthusiastic slimespring small voices! :D
        if (one_in(3)) {
            add_msg(m_good, _("wow! you look just like me! we should look out for each other!"));
        } else if (one_in(2)) {
            add_msg(m_good, _("come on, big me, let's go!"));
        } else {
            add_msg(m_good, _("we're a team, we've got this!"));
        }
        return true;
    } else if (id == "SHOUT1") {
        sounds::sound(posx(), posy(), 10 + 2 * str_cur, _("You shout loudly!"));
        return true;
    } else if (id == "SHOUT2"){
        sounds::sound(posx(), posy(), 15 + 3 * str_cur, _("You scream loudly!"));
        return true;;
    } else if (id == "SHOUT3"){
        sounds::sound(posx(), posy(), 20 + 4 * str_cur, _("You let out a piercing howl!"));
        return true;
    } else if ((id == "NAUSEA") || (id == "VOMITOUS") ){
        vomit();
        return true;
    } else if (id == "M_FERTILE"){
        spores();
        return true;
    } else if (id == "M_BLOOM"){
        blossoms();
        return true;
    } else if (id == "VINES3"){
        item newit("vine_30", calendar::turn, false);
        if (!can_pickVolume(newit.volume())) { //Accounts for result_mult
            add_msg(_("You detach a vine but don't have room to carry it, so you drop it."));
            g->m.add_item_or_charges(posx(), posy(), newit);
        } else if (!can_pickWeight(newit.weight(), !OPTIONS["DANGEROUS_PICKUPS"])) {
            add_msg(_("Your freshly-detached vine is too heavy to carry, so you drop it."));
            g->m.add_item_or_charges(posx(), posy(), newit);
        } else {
            inv.assign_empty_invlet(newit);
            newit = i_add(newit);
            add_msg(m_info, "%c - %s", newit.invlet == 0 ? ' ' : newit.invlet, newit.tname().c_str());
        }
        return true;
    }
    
    // Mutation activation was not canceled in legacy mutation effects, so return true.
    return true;
}

void Character::deactivate_mutation(const muttype_id &flag)
{
    if (!has_mut(flag)) {
        // We don't have the desired mutation!
        debugmsg("Tried to deactivate a nonexistent mutation!");
        return;
    }
    if (!my_mutations[flag].is_activatable()) {
        // It's not an activatable mutation!
        debugmsg("Tried to deactivate a non-activatable mutation!");
        return;
    }
    if (!my_mutations[flag].is_active()) {
        // It's already deactivated, abort
        return;
    }
    if (is_player()) {
        add_msg(m_neutral, _("You stop using your %s."), my_mutations[flag].get_name().c_str());
    }
    
    // We've decided to deactivate here!
    // Deactivate the mutation
    my_mutations[flag].deactivate(*this)
}

void mutation::deactivate(Character &ch)
{
    muttype_id id = mut_type->id;
    if (deactivation_effects(id, ch)) {
        // We didn't stop the deactivation
        charge = 0;
    }
}

void mutation::deactivation_effects(const muttype_id id, Character &ch)
{
    /* NEW ON-DEACTIVATION MUTATION EFFECTS SHOULD GO HERE.
     * Cast ch to player if you really need to access player class variables. */
}

void Character::process_mutations()
{
    // Bitset is <hunger, thirst, fatigue>
    std::bitset<3> stops;
    if (hunger >= 2800) {
        stops.set(0);
    }
    if (thirst >= 520) {
        stops.set(1);
    }
    if (fatigue >= 575) {
        stops.set(2);
    }
    std::unordered_map<std::string, int> pro_costs;
    for (auto &mut : my_mutations) {
        if (!mut.second.process(pro_costs, stops) && is_player()) {
            add_msg(m_info, _("You can't keep your %s active anymore!"), mut.get_name().c_str());
        }
    }
    // Pay any reactivation costs
    hunger += pro_costs["hunger"];
    thirst += pro_costs["thirst"];
    fatigue += pro_costs["fatigue"];
}

bool mutation::process(std::unordered_map<std::string, int> &cost, std::bitset<3> stop)
{
    if (charge == 0) {
        // It's either not on or only charges a cost for activation;
        // do nothing.
        return true;
    }
    
    if (charge == 1 && mut_type->repeating) {
        // Check each stop type to see if we can reactivate
        bool stopped = false;
        if (stops[0] && mut_type->costs["hunger"] > 0) {
            stopped = true;
        } else if (!stops[1] && mut_type->costs["thirst"] > 0) {
            stopped = true;
        } else if (!stops[2] && mut_type->costs["fatigue"] > 0) {
            stopped = true;
        }
        
        if (!stopped) {
            // We reactivated our mutation, add the cost and set the charge
            cost["hunger"] += mut_type->costs["hunger"];
            cost["thirst"] += mut_type->costs["thirst"];
            cost["fatigue"] += mut_type->costs["fatigue"];
            charge = mut_type->duration;
            return true;
        } else {
            // We failed our reactivation, reduce charge and return false
            charge--;
            return false;
        }
    }
    
    // We didn't need to reactivate, reduce charge by 1.
    charge--;
    return true;
}

void show_mutations_titlebar(WINDOW *window, player *p, std::string menu_mode)
{
    werase(window);

    std::string caption = _("MUTATIONS -");
    int cap_offset = utf8_width(caption.c_str()) + 1;
    mvwprintz(window, 0,  0, c_blue, "%s", caption.c_str());

    std::stringstream pwr;
    pwr << string_format(_("Power: %d/%d"), int(p->power_level), int(p->max_power_level));
    int pwr_length = utf8_width(pwr.str().c_str()) + 1;

    std::string desc;
    int desc_length = getmaxx(window) - cap_offset - pwr_length;

    if(menu_mode == "reassigning") {
        desc = _("Reassigning.\nSelect a mutation to reassign or press SPACE to cancel.");
    } else if(menu_mode == "activating") {
        desc = _("<color_green>Activating</color>  <color_yellow>!</color> to examine, <color_yellow>=</color> to reassign.");
    } else if(menu_mode == "examining") {
        desc = _("<color_ltblue>Examining</color>  <color_yellow>!</color> to activate, <color_yellow>=</color> to reassign.");
    }
    fold_and_print(window, 0, cap_offset, desc_length, c_white, desc);
    fold_and_print(window, 1, 0, desc_length, c_white, _("Might need to use ? to assign the keys."));

    wrefresh(window);
}

void player::mutations_window()
{
    std::vector <muttype_id> passive;
    std::vector <muttype_id> active;
    for ( auto &mut : my_mutations ) {
        if (mut.second.is_activatable()) {
            active.push_back(mut.first);
        } else {
            passive.push_back(mut.first);
        }
    }
    
    // maximal number of rows in both columns
    const int mutations_count = std::max(passive.size(), active.size());

    int TITLE_HEIGHT = 2;
    int DESCRIPTION_HEIGHT = 5;

    // Main window
    /** Total required height is:
    * top frame line:                                         + 1
    * height of title window:                                 + TITLE_HEIGHT
    * line after the title:                                   + 1
    * line with active/passive mutation captions:               + 1
    * height of the biggest list of active/passive mutations:   + mutations_count
    * line before mutation description:                         + 1
    * height of description window:                           + DESCRIPTION_HEIGHT
    * bottom frame line:                                      + 1
    * TOTAL: TITLE_HEIGHT + mutations_count + DESCRIPTION_HEIGHT + 5
    */
    int HEIGHT = std::min(TERMY, std::max(FULL_SCREEN_HEIGHT,
                                          TITLE_HEIGHT + mutations_count + DESCRIPTION_HEIGHT + 5));
    int WIDTH = FULL_SCREEN_WIDTH + (TERMX - FULL_SCREEN_WIDTH) / 2;
    int START_X = (TERMX - WIDTH) / 2;
    int START_Y = (TERMY - HEIGHT) / 2;
    WINDOW *wBio = newwin(HEIGHT, WIDTH, START_Y, START_X);

    // Description window @ the bottom of the bio window
    int DESCRIPTION_START_Y = START_Y + HEIGHT - DESCRIPTION_HEIGHT - 1;
    int DESCRIPTION_LINE_Y = DESCRIPTION_START_Y - START_Y - 1;
    WINDOW *w_description = newwin(DESCRIPTION_HEIGHT, WIDTH - 2,
                                   DESCRIPTION_START_Y, START_X + 1);

    // Title window
    int TITLE_START_Y = START_Y + 1;
    int HEADER_LINE_Y = TITLE_HEIGHT + 1; // + lines with text in titlebar, local
    WINDOW *w_title = newwin(TITLE_HEIGHT, WIDTH - 2, TITLE_START_Y, START_X + 1);

    int scroll_position = 0;
    int second_column = 32 + (TERMX - FULL_SCREEN_WIDTH) /
                        4; // X-coordinate of the list of active mutations

    input_context ctxt("MUTATIONS");
    ctxt.register_updown();
    ctxt.register_action("ANY_INPUT");
    ctxt.register_action("TOGGLE_EXAMINE");
    ctxt.register_action("REASSIGN");
    ctxt.register_action("HELP_KEYBINDINGS");

    bool redraw = true;
    std::string menu_mode = "activating";

    while(true) {
        // offset for display: mutation with index i is drawn at y=list_start_y+i
        // drawing the mutation starts with mutation[scroll_position]
        const int list_start_y = HEADER_LINE_Y + 2 - scroll_position;
        int max_scroll_position = HEADER_LINE_Y + 2 + mutations_count -
                                  ((menu_mode == "examining") ? DESCRIPTION_LINE_Y : (HEIGHT - 1));
        if(redraw) {
            redraw = false;

            werase(wBio);
            draw_border(wBio);
            // Draw line under title
            mvwhline(wBio, HEADER_LINE_Y, 1, LINE_OXOX, WIDTH - 2);
            // Draw symbols to connect additional lines to border
            mvwputch(wBio, HEADER_LINE_Y, 0, BORDER_COLOR, LINE_XXXO); // |-
            mvwputch(wBio, HEADER_LINE_Y, WIDTH - 1, BORDER_COLOR, LINE_XOXX); // -|

            // Captions
            mvwprintz(wBio, HEADER_LINE_Y + 1, 2, c_ltblue, _("Passive:"));
            mvwprintz(wBio, HEADER_LINE_Y + 1, second_column, c_ltblue, _("Active:"));

            draw_exam_window(wBio, DESCRIPTION_LINE_Y, menu_mode == "examining");
            nc_color type;
            if (passive.empty()) {
                mvwprintz(wBio, list_start_y, 2, c_ltgray, _("None"));
            } else {
                for (size_t i = scroll_position; i < passive.size(); i++) {
                    mutation &mut = my_mutations[passive[i]];
                    if (list_start_y + static_cast<int>(i) ==
                        (menu_mode == "examining" ? DESCRIPTION_LINE_Y : HEIGHT - 1)) {
                        break;
                    }
                    type = c_cyan;
                    mvwprintz(wBio, list_start_y + i, 2, type, "%c %s",
                              mut.key, mut.get_name().c_str());
                }
            }

            if (active.empty()) {
                mvwprintz(wBio, list_start_y, second_column, c_ltgray, _("None"));
            } else {
                for (size_t i = scroll_position; i < active.size(); i++) {
                    mutation &mut = my_mutations[active[i]];
                    if (list_start_y + static_cast<int>(i) ==
                        (menu_mode == "examining" ? DESCRIPTION_LINE_Y : HEIGHT - 1)) {
                        break;
                    }
                    if (!mut.is_active()) {
                        type = c_red;
                    } else if (mut.is_active()) {
                        type = c_ltgreen;
                    } else {
                        type = c_ltred;
                    }
                    mvwputch( wBio, list_start_y + i, second_column, type, mut.get_key() );
                    
                    std::stringstream mut_desc;
                    mut_desc << mut.get_name() << string_format( _(" - "));
                    bool has_cost = false;
                    // Print resource cost or "Free "
                    if (mut.get_cost("hunger") > 0) {
                        mut_desc << string_format( _("%d Hu "), mut.get_cost("hunger"));
                        has_cost = true;
                    }
                    if (mut.get_cost("thirst") > 0) {
                        mut_desc << string_format( _("%d Th "), mut.get_cost("thirst"));
                        has_cost = true;
                    }
                    if (mut.get_cost("fatigue") > 0) {
                        mut_desc << string_format( _("%d Fa "), mut.get_cost("fatigue"));
                        has_cost = true;
                    }
                    if (!has_cost) {
                        mut_des << string_format( _("Free "));
                    }
                    
                    if (!mut.get_repeating()) {
                        if (mut.get_duration() <= 0) {
                            // One shot activation (instant effects)
                            mut_des << string_format( _("to activate"));
                        } else {
                            // One shot activation (effects for X turns)
                            mut_des << string_format( _("for %d turns"), mut.get_duration());
                        }
                    } else {
                        if (mut.get_duration() > 0 && has_cost) {
                            // Cost X amount every Y turns
                            mut_des << string_format( _("per %d turns"), mut.get_duration());
                        } else {
                            // Free ON/OFF toggling
                            mut_des << string_format( _("to activate"));
                        }
                    }
                    
                    // Show if the mutation is active through text as well 
                    // (at least until we get better colorblind option).
                    if (mut.is_active()) {
                        mut_desc << _(" - A");
                    }
                    mvwprintz( wBio, list_start_y + i, second_column + 2, type,
                               mut_desc.str().c_str() );
                }
            }

            // Scrollbar
            if(scroll_position > 0) {
                mvwputch(wBio, HEADER_LINE_Y + 2, 0, c_ltgreen, '^');
            }
            if(scroll_position < max_scroll_position && max_scroll_position > 0) {
                mvwputch(wBio, (menu_mode == "examining" ? DESCRIPTION_LINE_Y : HEIGHT - 1) - 1,
                         0, c_ltgreen, 'v');
            }
        }
        wrefresh(wBio);
        show_mutations_titlebar(w_title, this, menu_mode);
        const std::string action = ctxt.handle_input();
        const long ch = ctxt.get_raw_input().get_first_input();
        if (menu_mode == "reassigning") {
            menu_mode = "activating";
            mutation *tmp = mutation_by_invlet(ch);
            if(tmp == nullptr)
                // Selected an non-existing mutation (or escape, or ...)
                continue;
            }
            redraw = true;
            const char newch = popup_getkey(_("%s; enter new letter."),
                                            my_mutations[mut_id].get_name().c_str());
            wrefresh(wBio);
            if(newch == ch || newch == ' ' || newch == KEY_ESCAPE) {
                continue;
            }
            mutation *otmp = mutation_by_invlet(newch)
            if (otmp == nullptr && inv_chars.find(newch) == std::string::npos ) {
                // TODO separate list of letters for mutations
                popup(_("%c is not a valid inventory letter."), newch);
                continue;
            }
            if( otmp != 0 ) {
                muttype_id tmp_key = tmp.get_key();
                tmp.set_key(otmp.get_key());
                otmp.set_key(tmp_key);
            } else {
                tmp.set_key(newch);
            }
            // TODO: show a message like when reassigning a key to an item?
        } else if (action == "DOWN") {
            if(scroll_position < max_scroll_position) {
                scroll_position++;
                redraw = true;
            }
        } else if (action == "UP") {
            if(scroll_position > 0) {
                scroll_position--;
                redraw = true;
            }
        } else if (action == "REASSIGN") {
            menu_mode = "reassigning";
        } else if (action == "TOGGLE_EXAMINE") { // switches between activation and examination
            menu_mode = menu_mode == "activating" ? "examining" : "activating";
            werase(w_description);
            draw_exam_window(wBio, DESCRIPTION_LINE_Y, false);
            redraw = true;
        }else if (action == "HELP_KEYBINDINGS") {
            redraw = true;
        } else {
            mutation *tmp = mutation_by_invlet(ch);
            if (tmp == nullptr) {
                // entered a key that is not mapped to any mutation,
                // -> leave screen
                break;
            }
            if (menu_mode == "activating") {
                if (tmp.is_activatable()) {
                    if (tmp.is_active()) {
                        // Message is handled inside deactivate_mutation()
                        deactivate_mutation(mut_id);
                        
                        // Action done, leave screen
                        delwin(w_title);
                        delwin(w_description);
                        delwin(wBio);
                        break;
                    } else {
                        // this will clear the mutations menu for targeting purposes
                        werase(wBio);
                        wrefresh(wBio);
                        delwin(w_title);
                        delwin(w_description);
                        delwin(wBio);
                        g->draw();
                        // Message is handled inside activate_mutation()
                        activate_mutation( tmp.get_id() );
                        // Action done, leave screen
                        break;
                    }
                } else {
                    popup(_("You cannot activate %s!  To read a description of \
%s, press '!', then '%c'."), tmp.get_name().c_str(), tmp.get_name().c_str(),
                          tmp.get_key() );
                    redraw = true;
                }
            }
            if (menu_mode == "examining") { // Describing mutations, not activating them!
                draw_exam_window(wBio, DESCRIPTION_LINE_Y, true);
                // Clear the lines first
                werase(w_description);
                fold_and_print(w_description, 0, 0, WIDTH - 2, c_ltblue, tmp.get_desc());
                wrefresh(w_description);
            }
        }
    }
    //if we activated a mutation, already killed the windows
    if(!(menu_mode == "activating")) {
        werase(wBio);
        wrefresh(wBio);
        delwin(w_title);
        delwin(w_description);
        delwin(wBio);
    }
}

bool Character::mutation_ok(const muttype_id &flag, bool force_good, bool force_bad) const
{
    if (has_mut(flag) || has_higher_mut(flag)) {
        // We already have this mutation or something that replaces it.
        return false;
    }

    if (force_bad && mut_types[flag].get_rating() == m_good) {
        // This is a good mutation, and we're due for a bad one.
        return false;
    }

    if (force_good && mut_types[flag].get_rating() == m_bad) {
        // This is a bad mutation, and we're due for a good one.
        return false;
    }

    return true;
}

mut_rating mutation_type::get_rating()
{
    return rating;
}

bool Character::has_conflicting_mut(const muttype_id &flag) const
{
    // First check all of the mutations our goal cancels
    for (auto &mut : mut_types[flag].get_cancels()) {
        if (has_mut(mut)) {
            return true;
        }
    }
    
    // Then check what each of our mutations cancel
    for (auto &mut : my_mutations) {
        for (auto &cans : i.second.get_cancels()) {
            // And see if any of those are our search flag.
            if (j == flag) {
                return true;
            }
        }
    }
    return false;
}

bool Character::has_lower_mut(const muttype_id &flag) const
{
    std::vector<muttype_id> replace = mut_types[flag].get_replaces();
    for (auto &i : replace) {
        if (has_mut(i) || has_lower_mut(i)) {
            return true;
        }
    }
    return false;
}

bool Character::has_higher_mut(const muttype_id &flag) const
{
    std::vector<muttype_id> replaced = mut_types[flag].get_replacements();
    for (auto &i : replaced) {
        if (has_mut(i) || has_higher_mut(i)) {
            return true;
        }
    }
    return false;
}

bool Character::crossed_threshold() const
{
    // Check if any of our mutations are thresholds
    for (auto &mut : my_mutations) {
        if (mut.second.get_threshold()) {
            return true;
        }
    }
    return false;
}

void Character::set_cat_levels()
{
    mutation_category_level.clear();

    // Loop through our mutations
    for( auto &mut : my_mutations ) {
        set_cat_level_rec( mut.first );
    }
}

void Character::set_cat_level_rec(const muttype_id &flag)
{
    // Base traits don't count
    if (!is_trait(flag)) {
        // Each mutation counts for 8 points in its categories
        for (auto &cat : mut_types[flag].get_category()) {
            mutation_category_level[cat] += 8;
        }
        // as well as triggering points for each prereq recursively
        for (auto &i : mut_types[flag].get_replaces()) {
            set_cat_level_rec(i);
        }
        for (auto &i : mut_types[flag].get_prereqs()) {
            set_cat_level_rec(i);
        }
        for (auto &i : mut_types[flag].get_prereqs2()) {
            set_cat_level_rec(i);
        }
    }
}

std::string Character::get_highest_category() const // Returns the mutation category with the highest strength
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

std::string Character::get_category_dream(const std::string &cat, int strength) const
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

void Character::mutate()
{
    // 1 in 3 chance of forcing a bad mutation
    bool force_bad = one_in(3);
    bool force_good = false;
    // Unless you have Robust Genetics, which turns it into a 1 in 3 chance
    // of getting a good mutation instead.
    if (force_bad && has_mut("ROBUST")) {
        force_bad = false;
        force_good = true;
    }

    // Determine the highest mutation category
    std::string cat = get_highest_category();

    // Store valid upgrades and downgrades for our existing mutations.
    std::vector<std::string> upgrades;
    std::vector<std::string> downgrades;

    // For each mutation that we have...
    for (auto &mut : my_mutations) {
        muttype_id base_mut = mut.first;
        mut_type &mut_dat = mut_types[base_mut];
        
        // Store the mutations that replace it.
        for (auto i : mut_dat.get_replacements()) {
            // Check for validity
            if (mut_dat.get_valid() && mutation_ok(i, force_good, force_bad)) {
                upgrades.push_back(i);
            }
        }
        
        // Store the valid mutations that it adds to.
        for (auto i : mut_dat.get_additions()) {
            // Check for validity
            if (mut_dat.get_valid() && mutation_ok(i, force_good, force_bad)) {
                upgrades.push_back(i);
            }
        }
        
        // Traits can't be downgraded
        if (!is_trait(mut.first)) {
            // Check if it's in our current highest category
            bool in_cat = false;
            for(auto &cats : mut_dat.get_category()) {
                if (cats = cat) {
                    // It's in our desired category, break out
                    in_cat = true;
                    break;
                }
            }
            
            // Mark for removal if the mutation is not in the highest category,
            // not a profession trait, and not a threshold mutation.
            if (!in_cat && !mut_dat.get_profession() && !mut_dat.get_threshold()) {
                // Non-purifiable stuff is pretty tenacious, only being successfully
                // targeted by category enforced downgrades 25% of the time.
                if (mut_dat.get_purifiable() || one_in(4)) {
                    downgrades.push_back(base_mut);
                }
            }
        }
    }

    // Try to upgrade or remove an existing mutation first(50/50 each).
    // Return if the upgrade/remove was successful.
    if(one_in(2)) {
        if (!upgrades.empty()) {
            // (upgrades count) chances to upgrade, 5 chances to not.
            // +4 here because size() returns max index + 1
            size_t roll = rng(0, upgrades.size() + 4);
            if (roll < upgrades.size()) {
                // We got a valid upgrade index, so use it and return.
                mutate_towards(upgrades[roll]);
                return;
            }
        }
    } else {
        // Remove existing mutations that don't fit into our category
        if (!downgrades.empty() && cat != "") {
            // (downgrades count) chances to downgrade, 5 chances to not.
            // +4 here because size() returns max index + 1
            size_t roll = rng(0, downgrades.size() + 4);
            if (roll < downgrades.size()) {
                // We got a valid downgrade index, so use it and return.
                downgrade_mutation(downgrades[roll]);
                return;
            }
        }
    }


    // We didn't upgrade or downgrade an existing mutation. Therefore we get
    // to choose a new random valid mutation.
    bool first_pass = true;
    std::vector<std::string> valid;
    do {
        // First try with the current highest category if cat != "".
        // If we've already tried that then try again with a null category (cat = "")
        if (!first_pass) {
            cat = "";
        }

        // Go through all the mutations, pushing back the valid ones.
        for (auto &i : mut_types) {
            if (!i.second.get_valid()) {
                // Not a valid mutation, go to the next one.
                continue;
            }
            
            // If category is not null only check those with a matching category.
            if (cat != "") {
                bool valid_cat = false;
                for (auto &check : i.second.get_category()) {
                    if (check == cat) {
                        valid_cat = true;
                        break;
                    }
                }
                if (!valid_cat) {
                    // Our category wasn't valid, go to the next mutation.
                    continue;
                }
            }
            
            // Our mutation is valid and either our category is null or we had
            // the proper category to get here.
            if (mutation_ok(i.first, force_good, force_bad)) {
                // If it's passed all the checks and is ok, push it back.
                valid.push_back(i.first);
            }
        }

        if (valid.empty()) {
            // Change first_pass so we won't repeat endlessly.
            first_pass = false;
        }
    } while (valid.empty() && cat != "");

    if (valid.empty()) {
        // Couldn't find anything at all!
        return;
    }

    // Pick a random valid mutation and mutate towards it.
    std::string selection = valid[ rng(0, valid.size() - 1) ];
    mutate_towards(selection);
}

void Character::purify()
{
    // First try to add a trait back in.
    std::vector<muttype_id> un_traits;
    for (auto &trait : my_traits) {
        // If it's valid, purifiable, we don't have it, and we don't have any
        // conflicting mutations, then push it back.
        if(mut_types[trait].get_valid() && mut_types[trait].get_purifiable() &&
              !has_mut(trait) && !has_conflicting_mut(trait)) {
            un_traits.push_back(trait);
        }
    }
    if (!un_traits.empty()) {
        // Pick a random trait to move towards.
        mutate_towards(un_traits[rng(0, un_traits.size() - 1)]);
        return;
    }
    
    // If we didn't bring a trait back then try to downgrade another mutation.
    std::vector<muttype_id> un_muts;
    for (auto &mut : my_mutations) {
        // If it's not a trait and it's purifiable it's fair game for downgrading.
        if (mut_types[mut.first].get_purifiable() && !is_trait(mut.first)) {
            un_muts.push_back(mut.first);
        }
    }
    if (!un_muts.empty()) {
        // Pick a random mutation to downgrade.
        downgrade_mutation(un_muts[rng(0, un_muts.size() - 1)]);
        return;
    }
}

void Character::mutate_category(const std::string &cat)
{
    // 1 in 3 chance of forcing a bad mutation
    bool force_bad = one_in(3);
    bool force_good = false;
    // Unless you have Robust Genetics, which turns it into a 1 in 3 chance
    // of getting a good mutation instead.
    if (force_bad && has_mut("ROBUST")) {
        force_bad = false;
        force_good = true;
    }
    
    // Go through all the mutations, pushing back the valid ones.
    std::vector<std::string> valid;
    for (auto &i : mut_types) {
        if (!i.second.get_valid()) {
            // Not a valid mutation, go to the next one.
            continue;
        }
        
        // If category is not null only check those with a matching category.
        if (cat != "") {
            bool valid_cat = false;
            for (auto &check : i.second.get_category()) {
                if (check == cat) {
                    valid_cat = true;
                    break;
                }
            }
            if (!valid_cat) {
                // Our category wasn't valid, go to the next mutation.
                continue;
            }
        }
        
        // Our mutation is valid and either our category is null or we had
        // the proper category to get here.
        if (mutation_ok(i.first, force_good, force_bad)) {
            // If it's passed all the checks and is ok, push it back.
            valid.push_back(i.first);
        }
    }

    // If we can't mutate in the category do nothing
    if (valid.empty()) {
        return;
    }

    // Else pick a random valid mutation and mutate.
    std::string selection = valid[ rng(0, valid.size() - 1) ];
    mutate_towards(selection);
}

bool Character::mut_vect_check(std::vector<muttype_id> &current_vector) const
{
    // First check if the vector is empty
    if (!current_vector.empty()) {
        // Then check if we have a matching mutation
        bool current_check = false;
        for (auto &check : current_vector) {
            if (has_mut(check)) {
                // We found a matching mutation, break
                current_check = true;
                break;
            }
        }
        
        // If we didn't find a matching mutation return true
        if (!current_check) {
            return true;
        }
    }
    // Either the vector was empty or we found a matching mutation, return false.
    return false;
}

void Character::mutate_towards(muttype_id &flag)
{
    mutation_type &goal = mut_types[flag];
    
    // Profession mutations just fail silently
    if (goal.get_profession()) {
        return;
    }
    
    // Build lists of all mutations the new one will cancel
    std::vector<std::string> valid_cancels;
    std::vector<std::string> trait_cancels;
    for (auto &i : goal.get_cancels()) {
        if (has_mut(i)) {
            if (is_trait(i)) {
                // Store canceled traits separately since they can't
                // be canceled, only replaced
                trait_cancels.push_back(i);
            } else {
                valid_cancels.push_back(i);
            }
        }
    }
    
    // First try to downgrade a canceled mutation.
    if (!valid_cancels.empty()) {
        downgrade_mutation(valid_cancels[rng(0, valid_cancels.size() - 1)]);
        return;
    }
    
    // Then try to add a mutation that the goal replaces
    std::vector<muttype_id> v_check = goal.get_replaces();
    if (mut_vect_check(v_check) {
        mutate_towards(v_check[rng(0, v_check.size() - 1)]);
        return;
    }
    
    // Then try to add a mutation that the goal requires
    v_check = goal.get_prereqs();
    if (mut_vect_check(v_check) {
        mutate_towards(v_check[rng(0, v_check.size() - 1)]);
        return;
    }
    
    // Then try to add a secondary mutation that the goal requires
    v_check = goal.get_prereqs2();
    if (mut_vect_check(v_check) {
        mutate_towards(v_check[rng(0, v_check.size() - 1)]);
        return;
    }
    
    // Check if we need a threshold. Thresholds require special mutagen to
    // breach, so just print a message here without actually adding any mutation.
    v_check = goal.get_threshreq();
    if (mut_vect_check(v_check) {
        add_msg(_("You feel something straining deep inside you, yearning to be free..."));
        return;
    }
    
    // Finally try to remove canceled traits
    if (!trait_cancels.empty()) {
        downgrade_mutation(valid_cancels[rng(0, trait_cancels.size() - 1)], true);
        return;
    }
    
    // At this point we are actually adding the new mutation.
    // Check if we are going to be replacing something
    if (!goal.get_replaces().empty()) {
        // Get a list of all of our current mutations we could replace
        std::vector<std::string> reps;
        for (auto &i : goal.get_replaces()) {
            if (has_mut(i)) {
                reps.push_back(i);
            }
        }
        if (!reps.is_empty()) {
            // Pick a mutation to replace and replace it.
            replace_mutation(flag, reps[rng(0, reps.size() - 1)]);
            return;
        } else {
            // This should never happen, but just in case.
            debugmsg("Attempted to replace a nonexistent mutation!");
            return;
        }
    } else {
        // Add our new mutation.
        add_mutation(flag);
    }
}
