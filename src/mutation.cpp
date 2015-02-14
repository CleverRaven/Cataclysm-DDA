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

bool Character::has_trait(const std::string &b) const
{
    return my_mutations.count( b ) > 0;
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
}

void Character::toggle_mutation(const std::string &flag)
{
    const auto iter = my_mutations.find( flag );
    if( iter == my_mutations.end() ) {
        my_mutations[flag]; // Creates a new entry with default values
    } else {
        my_mutations.erase( iter );
    }
    // TODO: why does this not call mutation_loss_effect / mutation_effect?
    recalc_sight_limits();
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

void Character::mutation_effect(std::string mut)
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
        apply_mods(mut, true);
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

void Character::mutation_loss_effect(std::string mut)
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
        apply_mods(mut, false);
    }
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
    if ((mdata.hunger && hunger >= 700) || (mdata.thirst && thirst >= 260) ||
      (mdata.fatigue && fatigue >= 575)) {
      // Insufficient Foo to *maintain* operation is handled in player::suffer
        add_msg(m_warning, _("You feel like using your %s would kill you!"), mdata.name.c_str());
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
            hunger += cost;
        }
        if (mdata.thirst){
            thirst += cost;
        }
        if (mdata.fatigue){
            fatigue += cost;
        }
        tdata.powered = true;
        
        // Handle stat changes from activation
        apply_mods(mut, true);
    }

    if( mut == "WEB_WEAVER" ) {
        g->m.add_field(posx(), posy(), fd_web, 1);
        add_msg(_("You start spinning web with your spinnerets!"));
    } else if (mut == "BURROW"){
        if (g->u.is_underwater()) {
            add_msg_if_player(m_info, _("You can't do that while underwater."));
            tdata.powered = false;
            return;
        }
        int dirx, diry;
        if (!choose_adjacent(_("Burrow where?"), dirx, diry)) {
            tdata.powered = false;
            return;
        }

        if (dirx == g->u.posx() && diry == g->u.posy()) {
            add_msg_if_player(_("You've got places to go and critters to beat."));
            add_msg_if_player(_("Let the lesser folks eat their hearts out."));
            tdata.powered = false;
            return;
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
            tdata.powered = false;
            return;
        }
        g->u.assign_activity(ACT_BURROW, turns, -1, 0);
        g->u.activity.placement = point(dirx, diry);
        add_msg_if_player(_("You tear into the %s with your teeth and claws."),
                          g->m.tername(dirx, diry).c_str());
        tdata.powered = false;
        return; // handled when the activity finishes
    } else if (mut == "SLIMESPAWNER") {
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
            tdata.powered = false;
            return;
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
        tdata.powered = false;
        return;
    } else if (mut == "SHOUT1") {
        sounds::sound(posx(), posy(), 10 + 2 * str_cur, _("You shout loudly!"));
        tdata.powered = false;
        return;
    } else if (mut == "SHOUT2"){
        sounds::sound(posx(), posy(), 15 + 3 * str_cur, _("You scream loudly!"));
        tdata.powered = false;
        return;
    } else if (mut == "SHOUT3"){
        sounds::sound(posx(), posy(), 20 + 4 * str_cur, _("You let out a piercing howl!"));
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
        tdata.powered = false;
        return;
    }
}

void player::deactivate_mutation( const std::string &mut )
{
    my_mutations[mut].powered = false;
    
    // Handle stat changes from deactivation
    apply_mods(mut, false);
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

std::string Character::trait_by_invlet( const char ch ) const
{
    for( auto &mut : my_mutations ) {
        if( mut.second.key == ch ) {
            return mut.first;
        }
    }
    return std::string();
}

void player::power_mutations()
{
    std::vector <std::string> passive;
    std::vector <std::string> active;
    for( auto &mut : my_mutations ) {
        if (!mutation_branch::get( mut.first ).activated) {
            passive.push_back(mut.first);
        } else {
            active.push_back(mut.first);
        }
        // New mutations are initialized with no key at all, so we have to do this here.
        if( mut.second.key == ' ' ) {
            for( const auto &letter : inv_chars ) {
                if( trait_by_invlet( letter ).empty() ) {
                    mut.second.key = letter;
                    break;
                }
            }
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
                    const auto &md = mutation_branch::get( passive[i] );
                    const auto &td = my_mutations[passive[i]];
                    if (list_start_y + static_cast<int>(i) ==
                        (menu_mode == "examining" ? DESCRIPTION_LINE_Y : HEIGHT - 1)) {
                        break;
                    }
                    type = c_cyan;
                    mvwprintz(wBio, list_start_y + i, 2, type, "%c %s", td.key, md.name.c_str());
                }
            }

            if (active.empty()) {
                mvwprintz(wBio, list_start_y, second_column, c_ltgray, _("None"));
            } else {
                for (size_t i = scroll_position; i < active.size(); i++) {
                    const auto &md = mutation_branch::get( active[i] );
                    const auto &td = my_mutations[active[i]];
                    if (list_start_y + static_cast<int>(i) ==
                        (menu_mode == "examining" ? DESCRIPTION_LINE_Y : HEIGHT - 1)) {
                        break;
                    }
                    if (!td.powered) {
                        type = c_red;
                    }else if (td.powered) {
                        type = c_ltgreen;
                    } else {
                        type = c_ltred;
                    }
                    // TODO: track resource(s) used and specify
                    mvwputch( wBio, list_start_y + i, second_column, type, td.key );
                    std::stringstream mut_desc;
                    mut_desc << md.name;
                    if ( md.cost > 0 && md.cooldown > 0 ) {
                        mut_desc << string_format( _(" - %d RU / %d turns"),
                                      md.cost, md.cooldown );
                    } else if ( md.cost > 0 ) {
                        mut_desc << string_format( _(" - %d RU"), md.cost );
                    } else if ( md.cooldown > 0 ) {
                        mut_desc << string_format( _(" - %d turns"), md.cooldown );
                    }
                    if ( td.powered ) {
                        mut_desc << _(" - Active");
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
            const auto mut_id = trait_by_invlet( ch );
            if( mut_id.empty() ) {
                // Selected an non-existing mutation (or escape, or ...)
                continue;
            }
            redraw = true;
            const char newch = popup_getkey(_("%s; enter new letter."),
                                            mutation_branch::get_name( mut_id ).c_str());
            wrefresh(wBio);
            if(newch == ch || newch == ' ' || newch == KEY_ESCAPE) {
                continue;
            }
            const auto other_mut_id = trait_by_invlet( newch );
            // if there is already a mutation with the new key, the key
            // is considered valid.
            if( other_mut_id.empty() && inv_chars.find(newch) == std::string::npos ) {
                // TODO separate list of letters for mutations
                popup(_("%c is not a valid inventory letter."), newch);
                continue;
            }
            if( !other_mut_id.empty() ) {
                std::swap(my_mutations[mut_id].key, my_mutations[other_mut_id].key);
            } else {
                my_mutations[mut_id].key = newch;
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
            const auto mut_id = trait_by_invlet( ch );
            if( mut_id.empty() ) {
                // entered a key that is not mapped to any mutation,
                // -> leave screen
                break;
            }
            const auto &mut_data = mutation_branch::get( mut_id );
            if (menu_mode == "activating") {
                if (mut_data.activated) {
                    if (my_mutations[mut_id].powered) {
                        add_msg(m_neutral, _("You stop using your %s."), mut_data.name.c_str());

                        deactivate_mutation( mut_id );
                        delwin(w_title);
                        delwin(w_description);
                        delwin(wBio);
                        // Action done, leave screen
                        break;
                    } else if( (!mut_data.hunger || hunger <= 400) &&
                               (!mut_data.thirst || thirst <= 400) &&
                               (!mut_data.fatigue || fatigue <= 400) ) {

                        // this will clear the mutations menu for targeting purposes
                        werase(wBio);
                        wrefresh(wBio);
                        delwin(w_title);
                        delwin(w_description);
                        delwin(wBio);
                        g->draw();
                        add_msg( m_neutral, _("You activate your %s."), mut_data.name.c_str() );
                        activate_mutation( mut_id );
                        // Action done, leave screen
                        break;
                    } else {
                        popup( _( "You don't have enough in you to activate your %s!" ), mut_data.name.c_str() );
                        redraw = true;
                        continue;
                    }
                } else {
                    popup(_("\
You cannot activate %s!  To read a description of \
%s, press '!', then '%c'."), mut_data.name.c_str(), mut_data.name.c_str(),
                          my_mutations[mut_id].key );
                    redraw = true;
                }
            }
            if (menu_mode == "examining") { // Describing mutations, not activating them!
                draw_exam_window(wBio, DESCRIPTION_LINE_Y, true);
                // Clear the lines first
                werase(w_description);
                fold_and_print(w_description, 0, 0, WIDTH - 2, c_ltblue, mut_data.description);
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

    std::string selection = valid[ rng(0, valid.size() - 1) ]; // Pick one!
    mutate_towards(selection);
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

    std::string selection = valid[ rng(0, valid.size() - 1) ]; // Pick one!
    mutate_towards(selection);

    return;
}

void player::mutate_towards( const std::string &mut )
{
    if (has_child_flag(mut)) {
        remove_child_flag(mut);
        return;
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
            mutate_towards(mut);
            return;
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
            std::string devel = prereq[ rng(0, prereq.size() - 1) ];
            mutate_towards(devel);
            return;
        } else if (!prereq2 && !prereqs2.empty()) {
            std::string devel = prereqs2[ rng(0, prereqs2.size() - 1) ];
            mutate_towards(devel);
            return;
        }
    }

    // Check for threshhold mutation, if needed
    bool threshold = mdata.threshold;
    bool profession = mdata.profession;
    bool has_threshreq = false;
    std::vector<std::string> threshreq = mdata.threshreq;

    // It shouldn't pick a Threshold anyway--they're supposed to be non-Valid
    // and aren't categorized--but if it does, just reroll
    if (threshold) {
        add_msg(_("You feel something straining deep inside you, yearning to be free..."));
        mutate();
        return;
    }
    if (profession) {
        // Profession picks fail silently
        mutate();
        return;
    }

    for (size_t i = 0; !has_threshreq && i < threshreq.size(); i++) {
        if (has_trait(threshreq[i])) {
            has_threshreq = true;
        }
    }

    // No crossing The Threshold by simply not having it
    // Rerolling proved more trouble than it was worth, so deleted
    if (!has_threshreq && !threshreq.empty()) {
        add_msg(_("You feel something straining deep inside you, yearning to be free..."));
        return;
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

    toggle_mutation(mut);

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
        add_msg(rating, _("Your %1$s mutation turns into %2$s!"),
                replace_mdata.name.c_str(), mdata.name.c_str());
        add_memorial_log(pgettext("memorial_male", "'%s' mutation turned into '%s'"),
                         pgettext("memorial_female", "'%s' mutation turned into '%s'"),
                         replace_mdata.name.c_str(), mdata.name.c_str());
        toggle_mutation(replacing);
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
        add_msg(rating, _("Your %1$s mutation turns into %2$s!"),
                replace_mdata.name.c_str(), mdata.name.c_str());
        add_memorial_log(pgettext("memorial_male", "'%s' mutation turned into '%s'"),
                         pgettext("memorial_female", "'%s' mutation turned into '%s'"),
                         replace_mdata.name.c_str(), mdata.name.c_str());
        toggle_mutation(replacing2);
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
        add_msg(rating, _("Your innate %1$s trait turns into %2$s!"),
                cancel_mdata.name.c_str(), mdata.name.c_str());
        add_memorial_log(pgettext("memorial_male", "'%s' mutation turned into '%s'"),
                        pgettext("memorial_female", "'%s' mutation turned into '%s'"),
                        cancel_mdata.name.c_str(), mdata.name.c_str());
        toggle_mutation(canceltrait[i]);
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
        add_msg(rating, _("You gain a mutation called %s!"), mdata.name.c_str());
        add_memorial_log(pgettext("memorial_male", "Gained the mutation '%s'."),
                         pgettext("memorial_female", "Gained the mutation '%s'."),
                         mdata.name.c_str());
        mutation_effect(mut);
    }

    set_highest_cat_level();
    drench_mut_calc();
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
                        if (traitcheck[j] == mut) {
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

    // This should revert back to a removed base trait rather than simply removing the mutation
    toggle_mutation(mut);

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
        add_msg(rating, _("Your %1$s mutation turns into %2$s."), mdata.name.c_str(),
                replace_mdata.name.c_str());
        toggle_mutation(replacing);
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
        add_msg(rating, _("Your %1$s mutation turns into %2$s."), mdata.name.c_str(),
                replace_mdata.name.c_str());
        toggle_mutation(replacing2);
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
        add_msg(rating, _("You lose your %s mutation."), mdata.name.c_str());
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
