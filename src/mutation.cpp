#include "player.h"
#include "mutation.h"
#include "game.h"
#include "translations.h"

// mutation_effect handles things like destruction of armor, etc.
void mutation_effect(game *g, player &p, std::string mut);
// mutation_loss_effect handles what happens when you lose a mutation
void mutation_loss_effect(player &p, std::string mut);

bool player::mutation_ok(game *g, std::string mutation, bool force_good, bool force_bad)
{
    if (has_trait(mutation) || has_child_flag(g, mutation)) {
        // We already have this mutation or something that replaces it.
        return false;
    }

    if (force_bad && traits[mutation].points > 0) {
        // This is a good mutation, and we're due for a bad one.
        return false;
    }

    if (force_good && traits[mutation].points < 0) {
        // This is a bad mutation, and we're due for a good one.
        return false;
    }

    return true;
}

void player::mutate(game *g)
{
    bool force_bad = one_in(3);
    bool force_good = false;
    if (has_trait("ROBUST") && force_bad) {
        // Robust Genetics gives you a 33% chance for a good mutation,
        // instead of the 33% chance of a bad one.
        force_bad = false;
        force_good = true;
    }

    // Determine the highest mutation categorie
    std::string cat = get_highest_category();

    // See if we should ugrade/extend an existing mutation...
    std::vector<std::string> upgrades;

    // ... or remove one that is not in our highest category
    std::vector<std::string> downgrades;

    // For each mutation...
    for (std::map<std::string, trait>::iterator iter = traits.begin(); iter != traits.end(); ++iter) {
        std::string base_mutation = iter->first;

        // ...that we have...
        if (has_trait(base_mutation)) {
            // ...consider the mutations that replace it.
            for (int i = 0; i < mutation_data[base_mutation].replacements.size(); i++) {
                std::string mutation = mutation_data[base_mutation].replacements[i];

                if (mutation_ok(g, mutation, force_good, force_bad)) {
                    upgrades.push_back(mutation);
                }
            }

            // ...consider the mutations that add to it.
            for (int i = 0; i < mutation_data[base_mutation].additions.size(); i++) {
                std::string mutation = mutation_data[base_mutation].additions[i];

                if (mutation_ok(g, mutation, force_good, force_bad)) {
                    upgrades.push_back(mutation);
                }
            }

            // ...consider whether its in our highest category
            if( has_trait(base_mutation) && !has_base_trait(base_mutation) ) { // Starting traits don't count toward categories
                std::vector<std::string> group = mutations_category[cat];
                bool in_cat = false;
                for (int j = 0; j < group.size(); j++) {
                    if (group[j] == base_mutation) {
                        in_cat = true;
                        break;
                    }
                }

                // mark for removal
                if(!in_cat) {
                    downgrades.push_back(base_mutation);
                }
            }
        }
    }

    // Preliminary round to either upgrade or remove existing mutations
    if(one_in(2)) {
        if (upgrades.size() > 0) {
            // (upgrade count) chances to pick an upgrade, 4 chances to pick something else.
            int roll = rng(0, upgrades.size() + 4);
            if (roll < upgrades.size()) {
                // We got a valid upgrade index, so use it and return.
                mutate_towards(g, upgrades[roll]);
                return;
            }
        }
    } else {
      // Remove existing mutations that don't fit into our category
      if (downgrades.size() > 0 && cat != "") {
          int roll = rng(0, downgrades.size() + 4);
          if (roll < downgrades.size()) {
              remove_mutation(g, downgrades[roll]);
              return;
          }
      }
    }

    std::vector<std::string> valid; // Valid mutations
    bool first_pass = true;

    do {
        // If we tried once with a non-NULL category, and couldn't find anything valid
        // there, try again with MUTCAT_NULL
        if (!first_pass) {
            cat = "";
        }

        if (cat == "") {
            // Pull the full list
            for (std::map<std::string, trait>::iterator iter = traits.begin(); iter != traits.end(); ++iter) {
                if (mutation_data[iter->first].valid) {
                    valid.push_back( iter->first );
                }
            }
        } else {
            // Pull the category's list
            valid = mutations_category[cat];
        }

        // Remove anything we already have, that we have a child of, or that
        // goes against our intention of a good/bad mutation
        for (int i = 0; i < valid.size(); i++) {
            if (!mutation_ok(g, valid[i], force_good, force_bad)) {
                valid.erase(valid.begin() + i);
                i--;
            }
        }

        if (valid.empty()) {
            // So we won't repeat endlessly
            first_pass = false;
        }
    } while (valid.empty() && cat != "");

    if (valid.empty()) {
        // Couldn't find anything at all!
        return;
    }

    std::string selection = valid[ rng(0, valid.size() - 1) ]; // Pick one!
    mutate_towards(g, selection);
}

void player::mutate_category(game *g, std::string cat)
{
    bool force_bad = one_in(3);
    bool force_good = false;
    if (has_trait("ROBUST") && force_bad) {
        // Robust Genetics gives you a 33% chance for a good mutation,
        // instead of the 33% chance of a bad one.
        force_bad = false;
        force_good = true;
    }

    // Pull the category's list for valid mutations
    std::vector<std::string> valid;
    valid = mutations_category[cat];

    // Remove anything we already have, that we have a child of, or that
    // goes against our intention of a good/bad mutation
    for (int i = 0; i < valid.size(); i++) {
        if (!mutation_ok(g, valid[i], force_good, force_bad)) {
            valid.erase(valid.begin() + i);
            i--;
        }
    }

    // if we can't mutate in the category do nothing
    if (valid.empty()) {
        return;
    }

    std::string selection = valid[ rng(0, valid.size() - 1) ]; // Pick one!
    mutate_towards(g, selection);

    return;
}

void player::mutate_towards(game *g, std::string mut)
{
    if (has_child_flag(g, mut)) {
        remove_child_flag(g, mut);
        return;
    }

    bool has_prereqs = false;
    std::string canceltrait = "";
    std::vector<std::string> prereq = mutation_data[mut].prereqs;
    std::vector<std::string> cancel = mutation_data[mut].cancels;

    for (int i = 0; i < cancel.size(); i++) {
        if (!has_trait( cancel[i] )) {
            cancel.erase(cancel.begin() + i);
            i--;
        } else if (has_base_trait( cancel[i] )) {
            //If we have the trait, but it's a base trait, don't allow it to be removed normally
            canceltrait = cancel[i];
            cancel.erase(cancel.begin() + i);
            i--;
        }
    }

    if (!cancel.empty()) {
        std::string removed = cancel[ rng(0, cancel.size() - 1) ];
        remove_mutation(g, removed);
        return;
    }

    for (int i = 0; !has_prereqs && i < prereq.size(); i++) {
        if (has_trait(prereq[i])) {
            has_prereqs = true;
        }
    }

    if (!has_prereqs && !prereq.empty()) {
        std::string devel = prereq[ rng(0, prereq.size() - 1) ];
        mutate_towards(g, devel);
        return;
    }

    // Check if one of the prereqs that we have TURNS INTO this one
    std::string replacing = "";
    prereq = mutation_data[mut].prereqs; // Reset it
    for (int i = 0; i < prereq.size(); i++) {
        if (has_trait(prereq[i])) {
            std::string pre = prereq[i];
            for (int j = 0; replacing == "" && j < mutation_data[pre].replacements.size(); j++) {
                if (mutation_data[pre].replacements[j] == mut) {
                    replacing = pre;
                }
            }
        }
    }

    toggle_mutation(mut);
    if (replacing != "") {
        g->add_msg(_("Your %1$s mutation turns into %2$s!"), traits[replacing].name.c_str(), traits[mut].name.c_str());
        g->u.add_memorial_log(_("'%s' mutation turned into '%s'"), traits[replacing].name.c_str(), traits[mut].name.c_str());
        toggle_mutation(replacing);
        mutation_loss_effect(*this, replacing);
        mutation_effect(g, *this, mut);

    } else if (canceltrait != "") {
        // If this new mutation cancels a base trait, remove it and add the mutation at the same time
        g->add_msg(_("Your innate %1$s trait turns into %2$s!"), traits[canceltrait].name.c_str(), traits[mut].name.c_str());
        g->u.add_memorial_log(_("'%s' trait turned into '%s'"), traits[canceltrait].name.c_str(), traits[mut].name.c_str());
        toggle_mutation(canceltrait);
        mutation_loss_effect(*this, canceltrait);
        mutation_effect(g, *this, mut);
    } else {
        g->add_msg(_("You gain a mutation called %s!"), traits[mut].name.c_str());
        g->u.add_memorial_log(_("Gained the mutation '%s'."), traits[mut].name.c_str());
        mutation_effect(g, *this, mut);
    }

    set_highest_cat_level();
    drench_mut_calc();
}

void player::remove_mutation(game *g, std::string mut)
{
    // Check for dependant mutations first
    std::vector<std::string> dependant;

    for (std::map<std::string, trait>::iterator iter = traits.begin(); iter != traits.end(); ++iter) {
        for (int i = 0; i < mutation_data[iter->first].prereqs.size(); i++) {
            if (mutation_data[iter->first].prereqs[i] == iter->first) {
                dependant.push_back(iter->first);
                break;
            }
        }
    }

    if (dependant.size() > 0) {
        remove_mutation(g, dependant[rng(0, dependant.size()-1)]);
        return;
    }

    // Check if there's a prereq we should shrink back into
    std::string replacing = "";
    std::vector<std::string> originals = mutation_data[mut].prereqs;
    for (int i = 0; replacing == "" && i < originals.size(); i++) {
        std::string pre = originals[i];
        for (int j = 0; replacing == "" && j < mutation_data[pre].replacements.size(); j++) {
            if (mutation_data[pre].replacements[j] == mut) {
                replacing = pre;
            }
        }
    }

    // See if this mutation is cancelled by a base trait
    //Only if there's no prereq to shrink to, thus we're at the bottom of the trait line
    if (replacing == "") {
        //Check each mutation until we reach the end or find a trait to revert to
        for (std::map<std::string, trait>::iterator iter = traits.begin(); replacing == "" && iter != traits.end(); ++iter) {
            //See if it's in our list of base traits but not active
            if (has_base_trait(iter->first) && !has_trait(iter->first)) {
                //See if that base trait cancels the mutation we are using
                std::vector<std::string> traitcheck = mutation_data[iter->first].cancels;
                if (!traitcheck.empty()) {
                    for (int j = 0; replacing == "" && j < traitcheck.size(); j++) {
                        if (traitcheck[j] == mut) {
                            replacing = (iter->first);
                        }
                    }
                }
            }
        }
    }

    // This should revert back to a removed base trait rather than simply removing the mutation
    toggle_mutation(mut);

    if (replacing != "") {
        g->add_msg(_("Your %1$s mutation turns into %2$s."), traits[mut].name.c_str(), traits[replacing].name.c_str());
        toggle_mutation(replacing);
        mutation_loss_effect(*this, mut);
        mutation_effect(g, *this, replacing);
    } else {
        g->add_msg(_("You lose your %s mutation."), traits[mut].name.c_str());
        mutation_loss_effect(*this, mut);
    }

    set_highest_cat_level();
    drench_mut_calc();
}

bool player::has_child_flag(game *g, std::string flag)
{
    for (int i = 0; i < mutation_data[flag].replacements.size(); i++) {
        std::string tmp = mutation_data[flag].replacements[i];
        if (has_trait(tmp) || has_child_flag(g, tmp)) {
            return true;
        }
    }
    return false;
}

void player::remove_child_flag(game *g, std::string flag)
{
    for (int i = 0; i < mutation_data[flag].replacements.size(); i++) {
        std::string tmp = mutation_data[flag].replacements[i];
        if (has_trait(tmp)) {
            remove_mutation(g, tmp);
            return;
        } else if (has_child_flag(g, tmp)) {
            remove_child_flag(g, tmp);
            return;
        }
    }
}

void mutation_effect(game *g, player &p, std::string mut)
{
    bool is_u = (&p == &(g->u));
    bool destroy = false;
    std::vector<body_part> bps;

    if (mut == "TOUGH" || mut == "GLASSJAW" || mut == "FRAIL") {
        p.recalc_hp();

    } else if (mut == "WEBBED" || mut == "ARM_TENTACLES" || mut == "ARM_TENTACLES_4" || mut == "ARM_TENTACLES_8") {
        // Push off gloves
        bps.push_back(bp_hands);

    } else if (mut == "TALONS") {
        // Destroy gloves
        destroy = true;
        bps.push_back(bp_hands);

    } else if (mut == "BEAK" || mut == "MANDIBLES") {
        // Destroy mouthwear
        destroy = true;
        bps.push_back(bp_mouth);

    } else if (mut == "MINOTAUR" || mut == "MUZZLE" || mut == "LONG_MUZZLE") {
        // Push off mouthwear
        bps.push_back(bp_mouth);

    } else if (mut == "HOOVES" || mut == "RAP_TALONS") {
        // Destroy footwear
        destroy = true;
        bps.push_back(bp_feet);

    } else if (mut == "SHELL") {
        // Destroy torsowear
        destroy = true;
        bps.push_back(bp_torso);

    } else if (mut == "HORNS_CURLED" || mut == "CHITIN3") {
        // Push off all helmets
        bps.push_back(bp_head);

    } else if (mut == "HORNS_POINTED" || mut == "ANTENNAE" || mut == "ANTLERS") {
        // Push off non-cloth helmets
        bps.push_back(bp_head);

    } else if (mut == "STR_UP") {
        p.str_max ++;
        p.recalc_hp();

    } else if (mut == "STR_UP_2") {
        p.str_max += 2;
        p.recalc_hp();

    } else if (mut == "STR_UP_3") {
        p.str_max += 4;
        p.recalc_hp();

    } else if (mut == "STR_UP_4") {
        p.str_max += 7;
        p.recalc_hp();

    } else if (mut == "DEX_UP") {
        p.dex_max ++;

    } else if (mut == "DEX_UP_2") {
        p.dex_max += 2;

    } else if (mut == "DEX_UP_3") {
        p.dex_max += 4;

    } else if (mut == "DEX_UP_4") {
        p.dex_max += 7;

    } else if (mut == "INT_UP") {
        p.int_max ++;

    } else if (mut == "INT_UP_2") {
        p.int_max += 2;

    } else if (mut == "INT_UP_3") {
        p.int_max += 4;

    } else if (mut == "INT_UP_4") {
        p.int_max += 7;

    } else if (mut == "PER_UP") {
        p.per_max ++;

    } else if (mut == "PER_UP_2") {
        p.per_max += 2;

    } else if (mut == "PER_UP_3") {
        p.per_max += 4;

    } else if (mut == "PER_UP_4") {
        p.per_max += 7;
    }

    for (int i = 0; i < p.worn.size(); i++) {
        for (int j = 0; j < bps.size(); j++) {
            if ((dynamic_cast<it_armor*>(p.worn[i].type))->covers & mfb(bps[j])) {
                if (destroy) {
                    if (is_u) {
                        g->add_msg(_("Your %s is destroyed!"), p.worn[i].tname().c_str());
                    }

                    p.worn.erase(p.worn.begin() + i);

                } else {
                    if (is_u) {
                        g->add_msg(_("Your %s is pushed off."), p.worn[i].tname().c_str());
                    }

                    char tmp_invlet = p.worn[i].invlet;
                    g->m.add_item_or_charges(p.posx, p.posy, p.worn[i]);
                    p.takeoff( g, p.worn[i].invlet, true );
                    p.i_rem( tmp_invlet );
                }
                // Reset to the start of the vector
                i = 0;
            }
        }
    }
}

void mutation_loss_effect(player &p, std::string mut)
{
    if (mut == "TOUGH" || mut == "GLASSJAW" || mut == "FRAIL") {
        p.recalc_hp();

    } else if (mut == "STR_UP") {
        p.str_max --;
        p.recalc_hp();

    } else if (mut == "STR_UP_2") {
        p.str_max -= 2;
        p.recalc_hp();

    } else if (mut == "STR_UP_3") {
        p.str_max -= 4;
        p.recalc_hp();

    } else if (mut == "STR_UP_4") {
        p.str_max -= 7;
        p.recalc_hp();

    } else if (mut == "DEX_UP") {
        p.dex_max --;

    } else if (mut == "DEX_UP_2") {
        p.dex_max -= 2;

    } else if (mut == "DEX_UP_3") {
        p.dex_max -= 4;

    } else if (mut == "DEX_UP_4") {
        p.dex_max -= 7;

    } else if (mut == "INT_UP") {
        p.int_max --;

    } else if (mut == "INT_UP_2") {
        p.int_max -= 2;

    } else if (mut == "INT_UP_3") {
        p.int_max -= 4;

    } else if (mut == "INT_UP_4") {
        p.int_max -= 7;

    } else if (mut == "PER_UP") {
        p.per_max --;

    } else if (mut == "PER_UP_2") {
        p.per_max -= 2;

    } else if (mut == "PER_UP_3") {
        p.per_max -= 4;

    } else if (mut == "PER_UP_4") {
        p.per_max -= 7;
    }
}
