#include "player.h"
#include "mutation.h"
#include "game.h"

// mutation_effect handles things like destruction of armor, etc.
void mutation_effect(game *g, player &p, pl_flag mut);
// mutation_loss_effect handles what happens when you lose a mutation
void mutation_loss_effect(game *g, player &p, pl_flag mut);

bool player::mutation_ok(game *g, pl_flag mutation, bool force_good, bool force_bad)
{
    if (has_trait(mutation) || has_child_flag(g, mutation))
    {
        // We already have this mutation or something that replaces it.
        return false;
    }

    if (force_bad && traits[mutation].points > 0)
    {
        // This is a good mutation, and we're due for a bad one.
        return false;
    }

    if (force_good && traits[mutation].points < 0)
    {
        // This is a bad mutation, and we're due for a good one.
        return false;
    }

    return true;
}

void player::mutate(game *g)
{
    bool force_bad = one_in(3);
    bool force_good = false;
    if (has_trait(PF_ROBUST) && force_bad)
    {
        // Robust Genetics gives you a 33% chance for a good mutation,
        // instead of the 33% chance of a bad one.
        force_bad = false;
        force_good = true;
    }

    // First, see if we should ugrade/extend an existing mutation
    std::vector<pl_flag> upgrades;
    // For each mutation...
    for (int base_mutation_index = 1; base_mutation_index < PF_MAX2; base_mutation_index++)
    {
        pl_flag base_mutation = (pl_flag) base_mutation_index;

        // ...that we have...
        if (has_trait(base_mutation))
        {
            // ...consider the mutations that replace it.
            for (int i = 0; i < g->mutation_data[base_mutation].replacements.size(); i++)
            {
                pl_flag mutation = g->mutation_data[base_mutation].replacements[i];

                if (mutation_ok(g, mutation, force_good, force_bad))
                {
                    upgrades.push_back(mutation);
                }
            }

            // ...consider the mutations that add to it.
            for (int i = 0; i < g->mutation_data[base_mutation].additions.size(); i++)
            {
                pl_flag mutation = g->mutation_data[base_mutation].additions[i];

                if (mutation_ok(g, mutation, force_good, force_bad))
                {
                    upgrades.push_back(mutation);
                }
            }
        }
    }

    // If we have upgrades, we prefer them.
    if (upgrades.size() > 0)
    {
        // (upgrade count) chances to pick an upgrade, 4 chances to pick something else.
        int roll = rng(0, upgrades.size() + 4);
        if (roll < upgrades.size())
        {
            // We got a valid upgrade index, so use it and return.
            mutate_towards(g, upgrades[roll]);
            return;
        }
    }

    // Next, see if we should mutate within a given category
    mutation_category cat = MUTCAT_NULL;

    // Count up the number of mutations in categories and find
    // the category with the highest single count.
    int total = 0, highest = 0;
    for (int i = 0; i < NUM_MUTATION_CATEGORIES; i++)
    {
        total += mutation_category_level[i];
        if (mutation_category_level[i] > highest)
        {
            cat = mutation_category(i);
            highest = mutation_category_level[i];
        }
    }

    // Pick one of the mutations out of a hat.  If it's in the highest-count
    // category, we pick from that category.  If not, we mutate randomly.
    if (rng(0, total) > highest)
    {
        cat = MUTCAT_NULL;
    }

    std::vector<pl_flag> valid; // Valid mutations
    bool first_pass = true;

    do
    {
        // If we tried once with a non-NULL category, and couldn't find anything valid
        // there, try again with MUTCAT_NULL
        if (!first_pass)
        {
            cat = MUTCAT_NULL;
        }

        if (cat == MUTCAT_NULL)
        {
            // Pull the full list
            for (int i = 1; i < PF_MAX2; i++)
            {
                if (g->mutation_data[i].valid)
                {
                    valid.push_back( pl_flag(i) );
                }
            }
        }
        else
        {
            // Pull the category's list
            valid = mutations_from_category(cat);
        }

        // Remove anything we already have, that we have a child of, or that
        // goes against our intention of a good/bad mutation
        for (int i = 0; i < valid.size(); i++)
        {
            if (!mutation_ok(g, valid[i], force_good, force_bad))
            {
                valid.erase(valid.begin() + i);
                i--;
            }
        }

        if (valid.empty())
        {
            // So we won't repeat endlessly
            first_pass = false;
        }

    }
    while (valid.empty() && cat != MUTCAT_NULL);


    if (valid.empty())
    {
        // Couldn't find anything at all!
        return;
    }

    pl_flag selection = valid[ rng(0, valid.size() - 1) ]; // Pick one!

    mutate_towards(g, selection);
}

void player::mutate_towards(game *g, pl_flag mut)
{
 if (has_child_flag(g, mut)) {
  remove_child_flag(g, mut);
  return;
 }
 bool has_prereqs = false;
 std::vector<pl_flag> prereq = g->mutation_data[mut].prereqs;
 std::vector<pl_flag> cancel = g->mutation_data[mut].cancels;

 for (int i = 0; i < cancel.size(); i++) {
  if (!has_trait( cancel[i] )) {
   cancel.erase(cancel.begin() + i);
   i--;
  }
 }

 if (!cancel.empty()) {
  pl_flag removed = cancel[ rng(0, cancel.size() - 1) ];
  remove_mutation(g, removed);
  return;
 }

 for (int i = 0; !has_prereqs && i < prereq.size(); i++) {
  if (has_trait(prereq[i]))
   has_prereqs = true;
 }
 if (!has_prereqs && !prereq.empty()) {
  pl_flag devel = prereq[ rng(0, prereq.size() - 1) ];
  mutate_towards(g, devel);
  return;
 }

// Check if one of the prereqs that we have TURNS INTO this one
 pl_flag replacing = PF_NULL;
 prereq = g->mutation_data[mut].prereqs; // Reset it
 for (int i = 0; i < prereq.size(); i++) {
  if (has_trait(prereq[i])) {
   pl_flag pre = prereq[i];
   for (int j = 0; replacing == PF_NULL &&
                   j < g->mutation_data[pre].replacements.size(); j++) {
    if (g->mutation_data[pre].replacements[j] == mut)
     replacing = pre;
   }
  }
 }

 toggle_trait(mut);
 if (replacing != PF_NULL)
    {
        g->add_msg("Your %s mutation turns into %s!", traits[replacing].name.c_str(),
                   traits[mut].name.c_str());
        toggle_trait(replacing);
        mutation_loss_effect(g, *this, replacing);
        mutation_effect(g, *this, mut);
    }
  else
    {
        g->add_msg("You gain a mutation called %s!", traits[mut].name.c_str());
        mutation_effect(g, *this, mut);
    }

// Weight us towards any categories that include this mutation
 for (int i = 0; i < NUM_MUTATION_CATEGORIES; i++) {
  std::vector<pl_flag> group = mutations_from_category(mutation_category(i));
  bool found = false;
  for (int j = 0; !found && j < group.size(); j++) {
   if (group[j] == mut)
    found = true;
  }
  if (found)
   mutation_category_level[i] += 8;
  else if (mutation_category_level[i] > 0 &&
           !one_in(mutation_category_level[i]))
   mutation_category_level[i]--;
 }

}

void player::remove_mutation(game *g, pl_flag mut)
{
// Check if there's a prereq we should shrink back into
 pl_flag replacing = PF_NULL;
 std::vector<pl_flag> originals = g->mutation_data[mut].prereqs;
 for (int i = 0; replacing == PF_NULL && i < originals.size(); i++) {
  pl_flag pre = originals[i];
  for (int j = 0; replacing == PF_NULL &&
                  j < g->mutation_data[pre].replacements.size(); j++) {
   if (g->mutation_data[pre].replacements[j] == mut)
    replacing = pre;
  }
 }

 toggle_trait(mut);
 if (replacing != PF_NULL) {
  g->add_msg("Your %s mutation turns into %s.", traits[mut].name.c_str(),
             traits[replacing].name.c_str());
  toggle_trait(replacing);
  mutation_loss_effect(g, *this, mut);
  mutation_effect(g, *this, replacing);
 } else {
  g->add_msg("You lose your %s mutation.", traits[mut].name.c_str());
  mutation_loss_effect(g, *this, mut);
 }

}

bool player::has_child_flag(game *g, pl_flag flag)
{
 for (int i = 0; i < g->mutation_data[flag].replacements.size(); i++) {
  pl_flag tmp = g->mutation_data[flag].replacements[i];
  if (has_trait(tmp) || has_child_flag(g, tmp))
   return true;
 }
 return false;
}

void player::remove_child_flag(game *g, pl_flag flag)
{
 for (int i = 0; i < g->mutation_data[flag].replacements.size(); i++) {
  pl_flag tmp = g->mutation_data[flag].replacements[i];
  if (has_trait(tmp)) {
   remove_mutation(g, tmp);
   return;
  } else if (has_child_flag(g, tmp)) {
   remove_child_flag(g, tmp);
   return;
  }
 }
}

void mutation_effect(game *g, player &p, pl_flag mut)
{
 bool is_u = (&p == &(g->u));
 bool destroy = false;
 std::vector<body_part> bps;

 switch (mut) {
// Push off gloves
  case PF_WEBBED:
  case PF_ARM_TENTACLES:
  case PF_ARM_TENTACLES_4:
  case PF_ARM_TENTACLES_8:
   bps.push_back(bp_hands);
   break;
// Destroy gloves
  case PF_TALONS:
   destroy = true;
   bps.push_back(bp_hands);
   break;
// Destroy mouthwear
  case PF_BEAK:
  case PF_MANDIBLES:
   destroy = true;
   bps.push_back(bp_mouth);
   break;
// Destroy footwear
  case PF_HOOVES:
   destroy = true;
   bps.push_back(bp_feet);
   break;
// Destroy torsowear
  case PF_SHELL:
   destroy = true;
   bps.push_back(bp_torso);
   break;
// Push off all helmets
  case PF_HORNS_CURLED:
  case PF_CHITIN3:
   bps.push_back(bp_head);
   break;
// Push off non-cloth helmets
  case PF_HORNS_POINTED:
  case PF_ANTENNAE:
  case PF_ANTLERS:
   bps.push_back(bp_head);
   break;

  case PF_STR_UP:
   p.str_max ++;
   break;
  case PF_STR_UP_2:
   p.str_max += 2;
   break;
  case PF_STR_UP_3:
   p.str_max += 4;
   break;
  case PF_STR_UP_4:
   p.str_max += 7;
   break;

  case PF_DEX_UP:
   p.dex_max ++;
   break;
  case PF_DEX_UP_2:
   p.dex_max += 2;
   break;
  case PF_DEX_UP_3:
   p.dex_max += 4;
   break;
  case PF_DEX_UP_4:
   p.dex_max += 7;
   break;

  case PF_INT_UP:
   p.int_max ++;
   break;
  case PF_INT_UP_2:
   p.int_max += 2;
   break;
  case PF_INT_UP_3:
   p.int_max += 4;
   break;
  case PF_INT_UP_4:
   p.int_max += 7;
   break;

  case PF_PER_UP:
   p.per_max ++;
   break;
  case PF_PER_UP_2:
   p.per_max += 2;
   break;
  case PF_PER_UP_3:
   p.per_max += 4;
   break;
  case PF_PER_UP_4:
   p.per_max += 7;
   break;
 }

 for (int i = 0; i < p.worn.size(); i++) {
  for (int j = 0; j < bps.size(); j++) {
   if ((dynamic_cast<it_armor*>(p.worn[i].type))->covers & mfb(bps[j])) {
    if (destroy) {
     if (is_u)
      g->add_msg("Your %s is destroyed!", p.worn[i].tname().c_str());
    } else {
     if (is_u)
      g->add_msg("Your %s is pushed off.", p.worn[i].tname().c_str());
     g->m.add_item(p.posx, p.posy, p.worn[i]);
    }
    p.worn.erase(p.worn.begin() + i);
    i--;
   }
  }
 }
}

void mutation_loss_effect(game *g, player &p, pl_flag mut)
{
 switch (mut) {
  case PF_STR_UP:
   p.str_max--;
   break;
  case PF_STR_UP_2:
   p.str_max -= 2;
   break;
  case PF_STR_UP_3:
   p.str_max -= 4;
   break;
  case PF_STR_UP_4:
   p.str_max -= 7;
   break;

  case PF_DEX_UP:
   p.dex_max--;
   break;
  case PF_DEX_UP_2:
   p.dex_max -= 2;
   break;
  case PF_DEX_UP_3:
   p.dex_max -= 4;
   break;
  case PF_DEX_UP_4:
   p.dex_max -= 7;
   break;

  case PF_INT_UP:
   p.int_max--;
   break;
  case PF_INT_UP_2:
   p.int_max -= 2;
   break;
  case PF_INT_UP_3:
   p.int_max -= 4;
   break;
  case PF_INT_UP_4:
   p.int_max -= 7;
   break;

  case PF_PER_UP:
   p.per_max--;
   break;
  case PF_PER_UP_2:
   p.per_max -= 2;
   break;
  case PF_PER_UP_3:
   p.per_max -= 4;
   break;
  case PF_PER_UP_4:
   p.per_max -= 7;
   break;
 }
}
