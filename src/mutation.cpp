#include "mutation.h"
#include "player.h"
#include "action.h"
#include "game.h"
#include "map.h"
#include "translations.h"
#include "messages.h"
#include "monster.h"
#include "overmapbuffer.h"
#include "sounds.h"
#include "options.h"
#include "mapdata.h"
#include "debug.h"
#include "field.h"
#include "vitamin.h"

bool Character::has_trait(const std::string &b) const
{
    return my_mutations.count( b ) > 0;
}

bool Character::has_trait_flag( const std::string &b ) const
{
    // UGLY, SLOW, should be cached as my_mutation_flags or something
    for( const auto &mut : my_mutations ) {
        auto &mut_data = mutation_branch::get( mut.first );
        if( mut_data.flags.count( b ) > 0 ) {
            return true;
        }
    }

    return false;
}

bool Character::has_base_trait(const std::string &b) const
{
    // Look only at base traits
    return my_traits.find( b ) != my_traits.end();
}

void Character::toggle_trait(const std::string &flag)
{
    const auto titer = my_traits.find( flag );
    if( titer == my_traits.end() ) {
        my_traits.insert( flag );
    } else {
        my_traits.erase( titer );
    }
    const auto miter = my_mutations.find( flag );
    if( miter == my_mutations.end() ) {
        my_mutations[flag]; // Creates a new entry with default values
        mutation_effect(flag);
    } else {
        my_mutations.erase( miter );
        mutation_loss_effect(flag);
    }
    recalc_sight_limits();
    reset_encumbrance();
}

void Character::set_mutation(const std::string &flag)
{
    const auto iter = my_mutations.find( flag );
    if( iter == my_mutations.end() ) {
        my_mutations[flag]; // Creates a new entry with default values
    } else {
        debugmsg("Trying to set %s mutation, but the character already has it.", flag.c_str());
    }
    recalc_sight_limits();
    reset_encumbrance();
}

void Character::unset_mutation(const std::string &flag)
{
    const auto iter = my_mutations.find( flag );
    if( iter == my_mutations.end() ) {
        debugmsg("Trying to unset %s mutation, but the character does not have it.", flag.c_str());
    } else {
        my_mutations.erase( iter );
    }
    recalc_sight_limits();
    reset_encumbrance();
}

int Character::get_mod(std::string mut, std::string arg) const
{
    auto &mod_data = mutation_branch::get( mut ).mods;
    int ret = 0;
    auto found = mod_data.find(std::make_pair(false, arg));
    if (found != mod_data.end()) {
        ret += found->second;
    }
    /* Deactivated due to inability to store active mutation state
    if (has_active_mutation(mut)) {
        found = mod_data.find(std::make_pair(true, arg));
        if (found != mod_data.end()) {
            ret += found->second;
        }
    } */
    return ret;
}

void Character::apply_mods(const std::string &mut, bool add_remove)
{
    int sign = add_remove ? 1 : -1;
    int str_change = get_mod(mut, "STR");
    str_max += sign * str_change;
    per_max += sign * get_mod(mut, "PER");
    dex_max += sign * get_mod(mut, "DEX");
    int_max += sign * get_mod(mut, "INT");

    if( str_change != 0 ) {
        recalc_hp();
    }
}

bool mutation_branch::conflicts_with_item( const item &it ) const
{
    if( allow_soft_gear && it.is_soft() ) {
        return false;
    }

    for( body_part bp : restricts_gear ) {
        if( it.covers( bp ) ) {
            return true;
        }
    }

    return false;
}

const resistances &mutation_branch::damage_resistance( body_part bp ) const
{
    const auto iter = armor.find( bp );
    if( iter == armor.end() ) {
        static const resistances nulres;
        return nulres;
    }

    return iter->second;
}

void Character::mutation_effect(std::string mut)
{
    if (mut == "TOUGH" || mut == "TOUGH2" || mut == "TOUGH3" || mut == "GLASSJAW" ||
        mut == "FLIMSY" || mut == "FLIMSY2" || mut == "FLIMSY3" ||
        mut == "MUT_TOUGH" || mut == "MUT_TOUGH2" || mut == "MUT_TOUGH3") {
        recalc_hp();

    } else if (mut == "STR_ALPHA") {
        ///\EFFECT_STR_MAX determines bonus from STR mutation
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
        ///\EFFECT_DEX_MAX determines bonus from DEX mutation
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
        ///\EFFECT_INT_MAX determines bonus from INT mutation
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
        ///\EFFECT_PER_MAX determines bonus from PER mutation
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
        apply_mods(mut, true);
    }

    const auto &branch = mutation_branch::get( mut );

    remove_worn_items_with( [&]( item& armor ) {
        static const std::string mutation_safe = "OVERSIZE";
        if( armor.has_flag( mutation_safe ) ) {
            return false;
        }
        if( !branch.conflicts_with_item( armor ) ) {
            return false;
        }
        if( branch.destroys_gear ) {
            add_msg_player_or_npc( m_bad,
                _("Your %s is destroyed!"),
                _("<npcname>'s %s is destroyed!"),
                armor.tname().c_str() );
            for( item& remain : armor.contents ) {
                g->m.add_item_or_charges( pos(), remain );
            }
        } else {
            add_msg_player_or_npc( m_bad,
                _("Your %s is pushed off!"),
                _("<npcname>'s %s is pushed off!"),
                armor.tname().c_str() );
            g->m.add_item_or_charges( pos(), armor );
        }
        return true;
    } );

    if( branch.starts_active ) {
        my_mutations[mut].powered = true;
    }

    on_mutation_gain( mut );
}

void Character::mutation_loss_effect(std::string mut)
{
    if (mut == "TOUGH" || mut == "TOUGH2" || mut == "TOUGH3" || mut == "GLASSJAW" ||
        mut == "FLIMSY" || mut == "FLIMSY2" || mut == "FLIMSY3" ||
        mut == "MUT_TOUGH" || mut == "MUT_TOUGH2" || mut == "MUT_TOUGH3") {
        recalc_hp();

    } else if (mut == "STR_ALPHA") {
        ///\EFFECT_STR_MAX determines penalty from STR mutation loss
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
        ///\EFFECT_DEX_MAX determines penalty from DEX mutation loss
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
        ///\EFFECT_INT_MAX determines penalty from INT mutation loss
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
        ///\EFFECT_PER_MAX determines penalty from PER mutation loss
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
        apply_mods(mut, false);
    }

    on_mutation_loss( mut );
}

bool Character::has_active_mutation(const std::string & b) const
{
    const auto iter = my_mutations.find( b );
    return iter != my_mutations.end() && iter->second.powered;
}

void player::activate_mutation( const std::string &mut )
{
    const auto &mdata = mutation_branch::get( mut );
    auto &tdata = my_mutations[mut];
    int cost = mdata.cost;
    // You can take yourself halfway to Near Death levels of hunger/thirst.
    // Fatigue can go to Exhausted.
    if ((mdata.hunger && get_hunger() >= 700) || (mdata.thirst && get_thirst() >= 260) ||
      (mdata.fatigue && get_fatigue() >= EXHAUSTED)) {
      // Insufficient Foo to *maintain* operation is handled in player::suffer
        add_msg_if_player(m_warning, _("You feel like using your %s would kill you!"), mdata.name.c_str());
        return;
    }
    if (tdata.powered && tdata.charge > 0) {
        // Already-on units just lose a bit of charge
        tdata.charge--;
    } else {
        // Not-on units, or those with zero charge, have to pay the power cost
        if (mdata.cooldown > 0) {
            tdata.charge = mdata.cooldown - 1;
        }
        if (mdata.hunger){
            mod_hunger(cost);
        }
        if (mdata.thirst){
            mod_thirst(cost);
        }
        if (mdata.fatigue){
            mod_fatigue(cost);
        }
        tdata.powered = true;

        // Handle stat changes from activation
        apply_mods(mut, true);
        recalc_sight_limits();
    }

    if( mut == "WEB_WEAVER" ) {
        g->m.add_field(pos(), fd_web, 1, 0);
        add_msg_if_player(_("You start spinning web with your spinnerets!"));
    } else if( mut == "WEB_ROPE" ) {
        add_msg_if_player(_("You spin a rope from your silk."));
        item rope( "rope_30" );
        i_add_or_drop( rope );
        tdata.powered = false;
    } else if (mut == "BURROW"){
        if( is_underwater() ) {
            add_msg_if_player(m_info, _("You can't do that while underwater."));
            tdata.powered = false;
            return;
        }
        tripoint dirp;
        if (!choose_adjacent(_("Burrow where?"), dirp)) {
            tdata.powered = false;
            return;
        }

        if( dirp == pos() ) {
            add_msg_if_player(_("You've got places to go and critters to beat."));
            add_msg_if_player(_("Let the lesser folks eat their hearts out."));
            tdata.powered = false;
            return;
        }
        int turns;
        if (g->m.is_bashable(dirp) && g->m.has_flag("SUPPORTS_ROOF", dirp) &&
            g->m.ter(dirp) != t_tree) {
            // Takes about 100 minutes (not quite two hours) base time.
            // Being better-adapted to the task means that skillful Survivors can do it almost twice as fast.
            ///\EFFECT_CARPENTRY speeds up burrowing
            turns = (100000 - 5000 * get_skill_level( skill_id( "carpentry" ) ));
        } else if (g->m.move_cost(dirp) == 2 && g->get_levz() == 0 &&
                   g->m.ter(dirp) != t_dirt && g->m.ter(dirp) != t_grass) {
            turns = 18000;
        } else {
            add_msg_if_player(m_info, _("You can't burrow there."));
            tdata.powered = false;
            return;
        }
        assign_activity( activity_id( "ACT_BURROW" ), turns, -1, 0 );
        activity.placement = dirp;
        add_msg_if_player(_("You tear into the %s with your teeth and claws."),
                          g->m.tername(dirp).c_str());
        tdata.powered = false;
        return; // handled when the activity finishes
    } else if (mut == "SLIMESPAWNER") {
        std::vector<tripoint> valid;
        for (int x = posx() - 1; x <= posx() + 1; x++) {
            for (int y = posy() - 1; y <= posy() + 1; y++) {
                tripoint dest(x, y, posz());
                if (g->is_empty(dest)) {
                    valid.push_back( dest );
                }
            }
        }
        // Oops, no room to divide!
        if (valid.size() == 0) {
            add_msg_if_player(m_bad, _("You focus, but are too hemmed in to birth a new slimespring!"));
            tdata.powered = false;
            return;
        }
        add_msg_if_player(m_good, _("You focus, and with a pleasant splitting feeling, birth a new slimespring!"));
        int numslime = 1;
        for (int i = 0; i < numslime && !valid.empty(); i++) {
            const tripoint target = random_entry_removed( valid );
            if (g->summon_mon(mtype_id( "mon_player_blob" ), target)) {
                monster *slime = g->monster_at( target );
                slime->friendly = -1;
            }
        }
        if (one_in(3)) {
            //~ Usual enthusiastic slimespring small voices! :D
            add_msg_if_player(m_good, _("wow! you look just like me! we should look out for each other!"));
        } else if (one_in(2)) {
            //~ Usual enthusiastic slimespring small voices! :D
            add_msg_if_player(m_good, _("come on, big me, let's go!"));
        } else {
            //~ Usual enthusiastic slimespring small voices! :D
            add_msg_if_player(m_good, _("we're a team, we've got this!"));
        }
        tdata.powered = false;
        return;
    } else if ((mut == "NAUSEA") || (mut == "VOMITOUS") ){
        vomit();
        tdata.powered = false;
        return;
    } else if (mut == "M_FERTILE"){
        spores();
        tdata.powered = false;
        return;
    } else if (mut == "M_BLOOM"){
        blossoms();
        tdata.powered = false;
        return;
    } else if (mut == "VINES3"){
        item newit( "vine_30", calendar::turn );
        if ( !can_pickVolume( newit ) ) { //Accounts for result_mult
            add_msg_if_player(_("You detach a vine but don't have room to carry it, so you drop it."));
            g->m.add_item_or_charges(pos(), newit);
        } else if ( !can_pickWeight( newit, !get_option<bool>( "DANGEROUS_PICKUPS" ) ) ) {
            add_msg_if_player(_("Your freshly-detached vine is too heavy to carry, so you drop it."));
            g->m.add_item_or_charges(pos(), newit);
        } else {
            inv.assign_empty_invlet(newit);
            newit = i_add(newit);
            add_msg_if_player(m_info, "%c - %s", newit.invlet == 0 ? ' ' : newit.invlet, newit.tname().c_str());
        }
        tdata.powered = false;
        return;
    } else if( mut == "SELFAWARE" ) {
        print_health();
        tdata.powered = false;
        return;
    }
}

void player::deactivate_mutation( const std::string &mut )
{
    my_mutations[mut].powered = false;

    // Handle stat changes from deactivation
    apply_mods(mut, false);
    recalc_sight_limits();
}

std::string Character::trait_by_invlet( const long ch ) const
{
    for( auto &mut : my_mutations ) {
        if( mut.second.key == ch ) {
            return mut.first;
        }
    }
    return std::string();
}

bool player::mutation_ok( const std::string &mutation, bool force_good, bool force_bad ) const
{
    if (has_trait(mutation) || has_child_flag(mutation)) {
        // We already have this mutation or something that replaces it.
        return false;
    }
    const auto &mdata = mutation_branch::get( mutation );
    if (force_bad && mdata.points > 0) {
        // This is a good mutation, and we're due for a bad one.
        return false;
    }

    if (force_good && mdata.points < 0) {
        // This is a bad mutation, and we're due for a good one.
        return false;
    }

    return true;
}

void player::mutate()
{
    bool force_bad = one_in(3);
    bool force_good = false;
    if (has_trait("ROBUST") && force_bad) {
        // Robust Genetics gives you a 33% chance for a good mutation,
        // instead of the 33% chance of a bad one.
        force_bad = false;
        force_good = true;
    }

    // Determine the highest mutation category
    std::string cat = get_highest_category();

    // See if we should upgrade/extend an existing mutation...
    std::vector<std::string> upgrades;

    // ... or remove one that is not in our highest category
    std::vector<std::string> downgrades;

    // For each mutation...
    for( auto &traits_iter : mutation_branch::get_all() ) {
        const auto &base_mutation = traits_iter.first;
        const auto &base_mdata = traits_iter.second;
        bool thresh_save = base_mdata.threshold;
        bool prof_save = base_mdata.profession;
        bool purify_save = base_mdata.purifiable;

        // ...that we have...
        if (has_trait(base_mutation)) {
            // ...consider the mutations that replace it.
            for( auto &mutation : base_mdata.replacements ) {
                bool valid_ok = mutation_branch::get( mutation ).valid;

                if ( (mutation_ok(mutation, force_good, force_bad)) &&
                     (valid_ok) ) {
                    upgrades.push_back(mutation);
                }
            }

            // ...consider the mutations that add to it.
            for( auto &mutation : base_mdata.additions ) {
                bool valid_ok = mutation_branch::get( mutation ).valid;

                if ( (mutation_ok(mutation, force_good, force_bad)) &&
                     (valid_ok) ) {
                    upgrades.push_back(mutation);
                }
            }

            // ...consider whether its in our highest category
            if( has_trait(base_mutation) && !has_base_trait(base_mutation) ) {
                // Starting traits don't count toward categories
                std::vector<std::string> group = mutations_category[cat];
                bool in_cat = false;
                for( auto &elem : group ) {
                    if( elem == base_mutation ) {
                        in_cat = true;
                        break;
                    }
                }

                // mark for removal
                // no removing Thresholds/Professions this way!
                if(!in_cat && !thresh_save && !prof_save) {
                    // non-purifiable stuff should be pretty tenacious
                    // category-enforcement only targets it 25% of the time
                    // (purify_save defaults true, = false for non-purifiable)
                    if((purify_save) || ((one_in(4)) && (!(purify_save))) ) {
                        downgrades.push_back(base_mutation);
                    }
                }
            }
        }
    }

    // Preliminary round to either upgrade or remove existing mutations
    if(one_in(2)) {
        if (!upgrades.empty()) {
            // (upgrade count) chances to pick an upgrade, 4 chances to pick something else.
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
            size_t roll = rng(0, downgrades.size() + 4);
            if (roll < downgrades.size()) {
                remove_mutation(downgrades[roll]);
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
            for( auto &traits_iter : mutation_branch::get_all() ) {
                if( traits_iter.second.valid ) {
                    valid.push_back( traits_iter.first );
                }
            }
        } else {
            // Pull the category's list
            valid = mutations_category[cat];
        }

        // Remove anything we already have, that we have a child of, or that
        // goes against our intention of a good/bad mutation
        for (size_t i = 0; i < valid.size(); i++) {
            if ( (!mutation_ok(valid[i], force_good, force_bad)) ||
                 (!(mutation_branch::get( valid[i] ).valid)) ) {
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

    if (mutate_towards(random_entry(valid))) {
        return;
    } else {
        // if mutation failed (errors, post-threshold pick), try again once.
        mutate_towards(random_entry(valid));
    }
}

void player::mutate_category( const std::string &cat )
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
    for (size_t i = 0; i < valid.size(); i++) {
        if (!mutation_ok(valid[i], force_good, force_bad)) {
            valid.erase(valid.begin() + i);
            i--;
        }
    }

    // if we can't mutate in the category do nothing
    if (valid.empty()) {
        return;
    }

    if (mutate_towards(random_entry(valid))) {
        return;
    } else {
        // if mutation failed (errors, post-threshold pick), try again once.
        mutate_towards(random_entry(valid));
    }
}

bool player::mutate_towards( const std::string &mut )
{
    if (has_child_flag(mut)) {
        remove_child_flag(mut);
        return true;
    }
    const auto &mdata = mutation_branch::get( mut );

    bool has_prereqs = false;
    bool prereq1 = false;
    bool prereq2 = false;
    std::vector<std::string> canceltrait;
    std::vector<std::string> prereq = mdata.prereqs;
    std::vector<std::string> prereqs2 = mdata.prereqs2;
    std::vector<std::string> cancel = mdata.cancels;

    for (size_t i = 0; i < cancel.size(); i++) {
        if (!has_trait( cancel[i] )) {
            cancel.erase(cancel.begin() + i);
            i--;
        } else if (has_base_trait( cancel[i] )) {
            //If we have the trait, but it's a base trait, don't allow it to be removed normally
            canceltrait.push_back( cancel[i]);
            cancel.erase(cancel.begin() + i);
            i--;
        }
    }

    for (size_t i = 0; i < cancel.size(); i++) {
        if (!cancel.empty()) {
            std::string removed = cancel[i];
            remove_mutation(removed);
            cancel.erase(cancel.begin() + i);
            i--;
            // This checks for cases where one trait knocks out several others
            // Probably a better way, but gets it Fixed Now--KA101
            return mutate_towards(mut);
        }
    }

    for (size_t i = 0; (!prereq1) && i < prereq.size(); i++) {
        if (has_trait(prereq[i])) {
            prereq1 = true;
        }
    }

    for (size_t i = 0; (!prereq2) && i < prereqs2.size(); i++) {
        if (has_trait(prereqs2[i])) {
            prereq2 = true;
        }
    }

    if (prereq1 && prereq2) {
        has_prereqs = true;
    }

    if (!has_prereqs && (!prereq.empty() || !prereqs2.empty())) {
        if (!prereq1 && !prereq.empty()) {
            return mutate_towards( random_entry( prereq ) );
        } else if (!prereq2 && !prereqs2.empty()) {
            return mutate_towards( random_entry( prereqs2 ) );
        }
    }

    // Check for threshhold mutation, if needed
    bool threshold = mdata.threshold;
    bool profession = mdata.profession;
    bool has_threshreq = false;
    std::vector<std::string> threshreq = mdata.threshreq;

    // It shouldn't pick a Threshold anyway--they're supposed to be non-Valid
    // and aren't categorized. This can happen if someone makes a threshold mut. into a prereq.
    if (threshold) {
        add_msg_if_player(_("You feel something straining deep inside you, yearning to be free..."));
        return false;
    }
    if (profession) {
        // Profession picks fail silently
        return false;
    }

    for (size_t i = 0; !has_threshreq && i < threshreq.size(); i++) {
        if (has_trait(threshreq[i])) {
            has_threshreq = true;
        }
    }

    // No crossing The Threshold by simply not having it
    if (!has_threshreq && !threshreq.empty()) {
        add_msg_if_player(_("You feel something straining deep inside you, yearning to be free..."));
        return false;
    }

    // Check if one of the prereqs that we have TURNS INTO this one
    std::string replacing = "";
    prereq = mdata.prereqs; // Reset it
    for( auto &elem : prereq ) {
        if( has_trait( elem ) ) {
            std::string pre = elem;
            const auto &p = mutation_branch::get( pre );
            for (size_t j = 0; replacing == "" && j < p.replacements.size(); j++) {
                if (p.replacements[j] == mut) {
                    replacing = pre;
                }
            }
        }
    }

    // Loop through again for prereqs2
    std::string replacing2 = "";
    prereq = mdata.prereqs2; // Reset it
    for( auto &elem : prereq ) {
        if( has_trait( elem ) ) {
            std::string pre2 = elem;
            const auto &p = mutation_branch::get( pre2 );
            for (size_t j = 0; replacing2 == "" && j < p.replacements.size(); j++) {
                if (p.replacements[j] == mut) {
                    replacing2 = pre2;
                }
            }
        }
    }

    set_mutation(mut);

    bool mutation_replaced = false;

    game_message_type rating;

    if (replacing != "") {
        const auto &replace_mdata = mutation_branch::get( replacing );
        if(mdata.mixed_effect || replace_mdata.mixed_effect) {
            rating = m_mixed;
        } else if(replace_mdata.points - mdata.points < 0) {
            rating = m_good;
        } else if(mdata.points - replace_mdata.points < 0) {
            rating = m_bad;
        } else {
            rating = m_neutral;
        }
        // TODO: Limit this to visible mutations
        // TODO: In case invisible mutation turns into visible or vice versa
        //  print only the visible mutation appearing/disappearing
        add_msg_player_or_npc(rating,
            _("Your %1$s mutation turns into %2$s!"),
            _("<npcname>'s %1$s mutation turns into %2$s!"),
            replace_mdata.name.c_str(), mdata.name.c_str() );
        add_memorial_log(pgettext("memorial_male", "'%s' mutation turned into '%s'"),
                         pgettext("memorial_female", "'%s' mutation turned into '%s'"),
                         replace_mdata.name.c_str(), mdata.name.c_str());
        unset_mutation(replacing);
        mutation_loss_effect(replacing);
        mutation_effect(mut);
        mutation_replaced = true;
    }
    if (replacing2 != "") {
        const auto &replace_mdata = mutation_branch::get( replacing2 );
        if(mdata.mixed_effect || replace_mdata.mixed_effect) {
            rating = m_mixed;
        } else if(replace_mdata.points - mdata.points < 0) {
            rating = m_good;
        } else if(mdata.points - replace_mdata.points < 0) {
            rating = m_bad;
        } else {
            rating = m_neutral;
        }
        add_msg_player_or_npc(rating,
            _("Your %1$s mutation turns into %2$s!"),
            _("<npcname>'s %1$s mutation turns into %2$s!"),
            replace_mdata.name.c_str(), mdata.name.c_str() );
        add_memorial_log(pgettext("memorial_male", "'%s' mutation turned into '%s'"),
                         pgettext("memorial_female", "'%s' mutation turned into '%s'"),
                         replace_mdata.name.c_str(), mdata.name.c_str());
        unset_mutation(replacing2);
        mutation_loss_effect(replacing2);
        mutation_effect(mut);
        mutation_replaced = true;
    }
    for (size_t i = 0; i < canceltrait.size(); i++) {
        const auto &cancel_mdata = mutation_branch::get( canceltrait[i] );
        if(mdata.mixed_effect || cancel_mdata.mixed_effect) {
            rating = m_mixed;
        } else if(mdata.points < cancel_mdata.points) {
            rating = m_bad;
        } else if(mdata.points > cancel_mdata.points) {
            rating = m_good;
        } else if(mdata.points == cancel_mdata.points) {
            rating = m_neutral;
        } else {
            rating = m_mixed;
        }
        // If this new mutation cancels a base trait, remove it and add the mutation at the same time
        add_msg_player_or_npc( rating,
            _("Your innate %1$s trait turns into %2$s!"),
            _("<npcname>'s innate %1$s trait turns into %2$s!"),
            cancel_mdata.name.c_str(), mdata.name.c_str() );
        add_memorial_log(pgettext("memorial_male", "'%s' mutation turned into '%s'"),
                        pgettext("memorial_female", "'%s' mutation turned into '%s'"),
                        cancel_mdata.name.c_str(), mdata.name.c_str());
        unset_mutation(canceltrait[i]);
        mutation_loss_effect(canceltrait[i]);
        mutation_effect(mut);
        mutation_replaced = true;
    }
    if (!mutation_replaced) {
        if(mdata.mixed_effect) {
            rating = m_mixed;
        } else if(mdata.points > 0) {
            rating = m_good;
        } else if(mdata.points < 0) {
            rating = m_bad;
        } else {
            rating = m_neutral;
        }
        // TODO: Limit to visible mutations
        add_msg_player_or_npc( rating,
            _("You gain a mutation called %s!"),
            _("<npcname> gains a mutation called %s!"),
            mdata.name.c_str() );
        add_memorial_log(pgettext("memorial_male", "Gained the mutation '%s'."),
                         pgettext("memorial_female", "Gained the mutation '%s'."),
                         mdata.name.c_str());
        mutation_effect(mut);
    }

    set_highest_cat_level();
    drench_mut_calc();
    return true;
}

void player::remove_mutation( const std::string &mut )
{
    const auto &mdata = mutation_branch::get( mut );
    // Check if there's a prereq we should shrink back into
    std::string replacing = "";
    std::vector<std::string> originals = mdata.prereqs;
    for (size_t i = 0; replacing == "" && i < originals.size(); i++) {
        std::string pre = originals[i];
        const auto &p = mutation_branch::get( pre );
        for (size_t j = 0; replacing == "" && j < p.replacements.size(); j++) {
            if (p.replacements[j] == mut) {
                replacing = pre;
            }
        }
    }

    std::string replacing2 = "";
    std::vector<std::string> originals2 = mdata.prereqs2;
    for (size_t i = 0; replacing2 == "" && i < originals2.size(); i++) {
        std::string pre2 = originals2[i];
        const auto &p = mutation_branch::get( pre2 );
        for (size_t j = 0; replacing2 == "" && j < p.replacements.size(); j++) {
            if (p.replacements[j] == mut) {
                replacing2 = pre2;
            }
        }
    }

    // See if this mutation is cancelled by a base trait
    //Only if there's no prereq to shrink to, thus we're at the bottom of the trait line
    if (replacing == "") {
        //Check each mutation until we reach the end or find a trait to revert to
        for( auto &iter : mutation_branch::get_all() ) {
            //See if it's in our list of base traits but not active
            if (has_base_trait(iter.first) && !has_trait(iter.first)) {
                //See if that base trait cancels the mutation we are using
                std::vector<std::string> traitcheck = iter.second.cancels;
                if (!traitcheck.empty()) {
                    for (size_t j = 0; replacing == "" && j < traitcheck.size(); j++) {
                        if (traitcheck[j] == mut) {
                            replacing = (iter.first);
                        }
                    }
                }
            }
            if( !replacing.empty() ) {
                break;
            }
        }
    }

    // Duplicated for prereq2
    if (replacing2 == "") {
        //Check each mutation until we reach the end or find a trait to revert to
        for( auto &iter : mutation_branch::get_all() ) {
            //See if it's in our list of base traits but not active
            if (has_base_trait(iter.first) && !has_trait(iter.first)) {
                //See if that base trait cancels the mutation we are using
                std::vector<std::string> traitcheck = iter.second.cancels;
                if (!traitcheck.empty()) {
                    for (size_t j = 0; replacing2 == "" && j < traitcheck.size(); j++) {
                        if (traitcheck[j] == mut && (iter.first) != replacing) {
                            replacing2 = (iter.first);
                        }
                    }
                }
            }
            if( !replacing2.empty() ) {
                break;
            }
        }
    }

    // make sure we don't toggle a mutation or trait twice, or it will cancel itself out.
    if(replacing == replacing2) {
        replacing2 = "";
    }

    // This should revert back to a removed base trait rather than simply removing the mutation
    unset_mutation(mut);

    bool mutation_replaced = false;

    game_message_type rating;

    if (replacing != "") {
        const auto &replace_mdata = mutation_branch::get( replacing );
        if(mdata.mixed_effect || replace_mdata.mixed_effect) {
            rating = m_mixed;
        } else if(replace_mdata.points - mdata.points > 0) {
            rating = m_good;
        } else if(mdata.points - replace_mdata.points > 0) {
            rating = m_bad;
        } else {
            rating = m_neutral;
        }
        add_msg_player_or_npc( rating,
            _("Your %1$s mutation turns into %2$s."),
            _("<npcname>'s %1$s mutation turns into %2$s."),
            mdata.name.c_str(), replace_mdata.name.c_str() );
        set_mutation(replacing);
        mutation_loss_effect(mut);
        mutation_effect(replacing);
        mutation_replaced = true;
    }
    if (replacing2 != "") {
        const auto &replace_mdata = mutation_branch::get( replacing2 );
        if(mdata.mixed_effect || replace_mdata.mixed_effect) {
            rating = m_mixed;
        } else if(replace_mdata.points - mdata.points > 0) {
            rating = m_good;
        } else if(mdata.points - replace_mdata.points > 0) {
            rating = m_bad;
        } else {
            rating = m_neutral;
        }
        add_msg_player_or_npc( rating,
            _("Your %1$s mutation turns into %2$s."),
            _("<npcname>'s %1$s mutation turns into %2$s."),
            mdata.name.c_str(), replace_mdata.name.c_str() );
        set_mutation(replacing2);
        mutation_loss_effect(mut);
        mutation_effect(replacing2);
        mutation_replaced = true;
    }
    if(!mutation_replaced) {
        if(mdata.mixed_effect) {
            rating = m_mixed;
        } else if(mdata.points > 0) {
            rating = m_bad;
        } else if(mdata.points < 0) {
            rating = m_good;
        } else {
            rating = m_neutral;
        }
        add_msg_player_or_npc( rating,
            _("You lose your %s mutation."),
            _("<npcname> loses their %s mutation."),
            mdata.name.c_str() );
        mutation_loss_effect(mut);
    }

    set_highest_cat_level();
    drench_mut_calc();
}

bool player::has_child_flag( const std::string &flag ) const
{
    for( auto &elem : mutation_branch::get( flag ).replacements ) {
        std::string tmp = elem;
        if (has_trait(tmp) || has_child_flag(tmp)) {
            return true;
        }
    }
    return false;
}

void player::remove_child_flag( const std::string &flag )
{
    for( auto &elem : mutation_branch::get( flag ).replacements ) {
        std::string tmp = elem;
        if (has_trait(tmp)) {
            remove_mutation(tmp);
            return;
        } else if (has_child_flag(tmp)) {
            remove_child_flag(tmp);
            return;
        }
    }
}
