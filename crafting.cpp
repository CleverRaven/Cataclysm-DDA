#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include "catajson.h"
#include "input.h"
#include "game.h"
#include "options.h"
#include "output.h"
#include "crafting.h"
#include "inventory.h"
#include "item_factory.h"

//apparently we can't declare this in crafting.h? Complained about multiple definition.
std::vector<craft_cat> craft_cat_list;

void draw_recipe_tabs(WINDOW *w, craft_cat tab,bool filtered=false);

// This function just defines the recipes used throughout the game.
void game::init_recipes()
{
    int id = -1;
    int tl, cl;
    recipe* last_rec = NULL;

    catajson recipeRaw("data/raw/recipes.json");

    catajson craftCats = recipeRaw.get("categories");
    for (craftCats.set_begin(); craftCats.has_curr(); craftCats.next())
    {
        craft_cat_list.push_back(craftCats.curr().as_string());
    }

    catajson recipeList = recipeRaw.get("recipes");
    std::vector<std::string> recipeNames;
    for (recipeList.set_begin(); recipeList.has_curr(); recipeList.next())
    {
        catajson curr = recipeList.curr();
        // required fields
        std::string result = curr.get("result").as_string();
        std::string category = curr.get("category").as_string();
        int difficulty = curr.get("difficulty").as_int();
        int time = curr.get("time").as_int();
        bool autolearn = curr.get("autolearn").as_bool();
        // optional fields
        bool reversible = curr.has("reversible") ? curr.get("reversible").as_bool() : false;
        const char *skill1 = curr.has("skill_pri") ? curr.get("skill_pri").as_string().c_str() : NULL;
        const char *skill2 = curr.has("skill_sec") ? curr.get("skill_sec").as_string().c_str() : NULL;
        std::string id_suffix = curr.has("id_suffix") ? curr.get("id_suffix").as_string() : "";
        int learn_by_disassembly = curr.has("decomp_learn") ? curr.get("decomp_learn").as_int() : -1;

        tl = -1;
        cl = -1;
        ++id;

        std::string rec_name = result + id_suffix;

        last_rec = new recipe(rec_name, id, result, category, skill1, skill2,
                              difficulty, time, reversible, autolearn,
                              learn_by_disassembly);

        for (std::vector<std::string>::iterator name_iter = recipeNames.begin();
             name_iter != recipeNames.end();
             ++name_iter)
        {
            if ((*name_iter) == rec_name)
            {
                debugmsg("Recipe name collision: %s", rec_name.c_str());
            }
        }

        recipeNames.push_back(rec_name);

        catajson compList = curr.get("components");
        for (compList.set_begin(); compList.has_curr(); compList.next())
        {
            ++cl;
            std::vector<component> component_choices;
            catajson comp = compList.curr();
            // interchangable components
            for (comp.set_begin(); comp.has_curr(); comp.next())
            {
                std::string name = comp.curr().get(0).as_string();
                int quant = comp.curr().get(1).as_int();
                component_choices.push_back(component(name, quant));
            }
            last_rec->components.push_back(component_choices);
        }

        if (curr.has("tools"))
        {
            catajson toolList = curr.get("tools");
            for (toolList.set_begin(); toolList.has_curr(); toolList.next())
            {
                ++tl;
                std::vector<component> tool_choices;
                catajson tool = toolList.curr();
                // interchangable tools
                for (tool.set_begin(); tool.has_curr(); tool.next())
                {
                    std::string name = tool.curr().get(0).as_string();
                    int quant = tool.curr().get(1).as_int();
                    tool_choices.push_back(component(name, quant));
                }
                last_rec->tools.push_back(tool_choices);
            }
        }

        if (curr.has("book_learn"))
        {
            catajson book_list = curr.get("book_learn");
            for (book_list.set_begin(); book_list.has_curr(); book_list.next())
            {
                catajson book = book_list.curr();
                std::string book_name = book.get(0).as_string();
                int book_level = book.get(1).as_int();

                if (item_controller->find_template(book_name)->is_book())
                {
                    it_book *book_def = dynamic_cast<it_book*>(item_controller->find_template(book_name));
                    book_def->recipes[last_rec] = book_level;
                }
            }
        }

        recipes[category].push_back(last_rec);
    }
}

bool game::crafting_allowed()
{
    if (u.morale_level() < MIN_MORALE_CRAFT)
    {	// See morale.h
        add_msg("Your morale is too low to craft...");
        return false;
    }

    return true;
}

void game::recraft()
{
 if(u.lastrecipe == NULL)
 {
  popup("Craft something first");
 }
 else if (making_would_work(u.lastrecipe))
 {
  make_craft(u.lastrecipe);
 }
}

//TODO clean up this function to give better status messages (e.g., "no fire available")
bool game::making_would_work(recipe *making)
{
    if (!crafting_allowed())
    {
    	return false;
    }

    if(can_make(making))
    {
        if (item_controller->find_template((making->result))->phase == LIQUID)
        {
            if (u.has_watertight_container() || u.has_matching_liquid(item_controller->find_template(making->result)->id))
            {
                return true;
            }
            else
            {
                popup("You don't have anything to store that liquid in!");
            }
        }
        else
        {
            return true;
        }
    }
    else
    {
        popup("You can no longer make that craft!");
    }

    return false;
}
bool game::can_make(recipe *r)
{
    inventory crafting_inv = crafting_inventory();
    if(!u.knows_recipe(r))
    {
        return false;
    }
    // under the assumption that all comp and tool's array contains all the required stuffs at the start of the array

    // check all tools
    for(int i = 0 ; i < r->tools.size() ; i++)
    {
        // if current tool is null(size 0), assume that there is no more after it.
        if(r->tools[i].size()==0)
        {
            break;
        }
        bool has_tool = false;
        for(int j = 0 ; j < r->tools[i].size() ; j++)
        {
            itype_id type = r->tools[i][j].type;
            int req = r->tools[i][j].count;
            if((req<= 0 && crafting_inv.has_amount(type,1)) || (req > 0 && crafting_inv.has_charges(type,req)))
            {
                has_tool = true;
                break;
            }
        }
        if(!has_tool)
        {
            return false;
        }
    }
    // check all components
    for(int i = 0 ; i < r->components.size() ; i++)
    {
        if(r->components[i].size() == 0)
        {
            break;
        }
        bool has_comp = false;
        for(int j = 0 ; j < r->components[i].size() ; j++)
        {
            itype_id type = r->components[i][j].type;
            int req = r->components[i][j].count;
            if (item_controller->find_template(type)->count_by_charges() && req > 0)
            {
                if (crafting_inv.has_charges(type, req))
                {
                    has_comp = true;
                    break;
                }
            }
            else if (crafting_inv.has_amount(type, abs(req)))
            {
                has_comp = true;
                break;
            }
        }
        if(!has_comp)
        {
            return false;
        }
    }
    return true;
}

void game::craft()
{
    if (!crafting_allowed())
    {
    	return;
    }

    recipe *rec = select_crafting_recipe();
    if (rec)
    {
        make_craft(rec);
    }
}

void game::long_craft()
{
    if (!crafting_allowed())
    {
    	return;
    }

    recipe *rec = select_crafting_recipe();
    if (rec)
    {
        make_all_craft(rec);
    }
}

craft_cat game::next_craft_cat(craft_cat cat)
{
    for (std::vector<craft_cat>::iterator iter = craft_cat_list.begin();
         iter != craft_cat_list.end();
         ++iter)
    {
        if ((*iter) == cat)
        {
            return *(++iter);
        }
    }
    return NULL;
}

craft_cat game::prev_craft_cat(craft_cat cat)
{
    for (std::vector<craft_cat>::iterator iter = craft_cat_list.begin();
         iter != craft_cat_list.end();
         ++iter)
    {
        if ((*iter) == cat)
        {
            return *(--iter);
        }
    }
    return NULL;
}

recipe* game::select_crafting_recipe()
{
	WINDOW *w_head = newwin( 3, 80, (TERMY > 25) ? (TERMY-25)/2 : 0, (TERMX > 80) ? (TERMX -80)/2 : 0);
    WINDOW *w_data = newwin(22, 80, 3 + ((TERMY > 25) ? (TERMY-25)/2 : 0), (TERMX  > 80) ? (TERMX -80)/2 : 0);
    craft_cat tab = "CC_WEAPON";
    std::vector<recipe*> current;
    std::vector<bool> available;
    item tmp;
    int line = 0, xpos, ypos;
    bool redraw = true;
    bool done = false;
    recipe *chosen = NULL;
    InputEvent input;

    inventory crafting_inv = crafting_inventory();
    std::string filterstring = "";
    do {
        if (redraw)
        { // When we switch tabs, redraw the header
            redraw = false;
            line = 0;
            draw_recipe_tabs(w_head, tab, (filterstring == "")?false:true);
            current.clear();
            available.clear();
            // Set current to all recipes in the current tab; available are possible to make
            pick_recipes(current, available, tab,filterstring);
        }

        // Clear the screen of recipe data, and draw it anew
        werase(w_data);
        if(filterstring != "")
        {
            mvwprintz(w_data, 19, 5, c_white, "?: Describe, [F]ind , [R]eset");
        }
        else
        {
            mvwprintz(w_data, 19, 5, c_white, "?: Describe, [F]ind");
        }
        mvwprintz(w_data, 20, 5, c_white, "Press <ENTER> to attempt to craft object.");
        for (int i = 0; i < 80; i++)
        {
            mvwputch(w_data, 21, i, c_ltgray, LINE_OXOX);
            if (i < 21)
            {
                mvwputch(w_data, i, 0, c_ltgray, LINE_XOXO);
                mvwputch(w_data, i, 79, c_ltgray, LINE_XOXO);
            }
        }

        mvwputch(w_data, 21,  0, c_ltgray, LINE_XXOO); // _|
        mvwputch(w_data, 21, 79, c_ltgray, LINE_XOOX); // |_
        wrefresh(w_data);

        int recmin = 0, recmax = current.size();
        if(recmax > MAX_DISPLAYED_RECIPES)
        {
            if (line <= recmin + 9)
            {
                for (int i = recmin; i < recmin + MAX_DISPLAYED_RECIPES; i++)
                {
                    mvwprintz(w_data, i - recmin, 2, c_dkgray, "");	// Clear the line
                    if (i == line)
                    {
                        mvwprintz(w_data, i - recmin, 2, (available[i] ? h_white : h_dkgray),
                        item_controller->find_template(current[i]->result)->name.c_str());
                    }
                    else
                    {
                        mvwprintz(w_data, i - recmin, 2, (available[i] ? c_white : c_dkgray),
                        item_controller->find_template(current[i]->result)->name.c_str());
                    }
                }
            }
            else if (line >= recmax - 9)
            {
                for (int i = recmax - MAX_DISPLAYED_RECIPES; i < recmax; i++)
                {
                    mvwprintz(w_data, 18 + i - recmax, 2, c_ltgray, "");	// Clear the line
                    if (i == line)
                    {
                        mvwprintz(w_data, 18 + i - recmax, 2, (available[i] ? h_white : h_dkgray),
                        item_controller->find_template(current[i]->result)->name.c_str());
                    }
                    else
                    {
                        mvwprintz(w_data, 18 + i - recmax, 2, (available[i] ? c_white : c_dkgray),
                        item_controller->find_template(current[i]->result)->name.c_str());
                    }
                }
            }
            else
            {
                for (int i = line - 9; i < line + 9; i++)
                {
                    mvwprintz(w_data, 9 + i - line, 2, c_ltgray, "");	// Clear the line
                    if (i == line)
                    {
                        mvwprintz(w_data, 9 + i - line, 2, (available[i] ? h_white : h_dkgray),
                        item_controller->find_template(current[i]->result)->name.c_str());
                    }
                    else
                    {
                        mvwprintz(w_data, 9 + i - line, 2, (available[i] ? c_white : c_dkgray),
                        item_controller->find_template(current[i]->result)->name.c_str());
                    }
                }
            }
        }
        else
        {
            for (int i = 0; i < current.size() && i < 23; i++)
            {
                if (i == line)
                {
                    mvwprintz(w_data, i, 2, (available[i] ? h_white : h_dkgray),
                    item_controller->find_template(current[i]->result)->name.c_str());
                }
                else
                {
                    mvwprintz(w_data, i, 2, (available[i] ? c_white : c_dkgray),
                    item_controller->find_template(current[i]->result)->name.c_str());
                }
            }
        }
        if (current.size() > 0)
        {
            nc_color col = (available[line] ? c_white : c_dkgray);
            mvwprintz(w_data, 0, 30, col, "Primary skill: %s",
            (current[line]->sk_primary == NULL ? "N/A" :
            current[line]->sk_primary->name().c_str()));
            mvwprintz(w_data, 1, 30, col, "Secondary skill: %s",
            (current[line]->sk_secondary == NULL ? "N/A" :
            current[line]->sk_secondary->name().c_str()));
            mvwprintz(w_data, 2, 30, col, "Difficulty: %d", current[line]->difficulty);
            if (current[line]->sk_primary == NULL)
            {
                mvwprintz(w_data, 3, 30, col, "Your skill level: N/A");
            }
            else
            {
                mvwprintz(w_data, 3, 30, col, "Your skill level: %d",
                // Macs don't seem to like passing this as a class, so force it to int
                (int)u.skillLevel(current[line]->sk_primary));
            }
            if (current[line]->time >= 1000)
            {
                mvwprintz(w_data, 4, 30, col, "Time to complete: %d minutes",
                int(current[line]->time / 1000));
            }
            else
            {
                mvwprintz(w_data, 4, 30, col, "Time to complete: %d turns",
                int(current[line]->time / 100));
            }
            mvwprintz(w_data, 5, 30, col, "Tools required:");
            if (current[line]->tools.size() == 0)
            {
                mvwputch(w_data, 6, 30, col, '>');
                mvwprintz(w_data, 6, 32, c_green, "NONE");
                ypos = 6;
            }
            else
            {
                ypos = 5;
                // Loop to print the required tools
                for (int i = 0; i < current[line]->tools.size() && current[line]->tools[i].size() > 0; i++)
                {
                    ypos++;
                    xpos = 32;
                    mvwputch(w_data, ypos, 30, col, '>');
                    for (int j = 0; j < current[line]->tools[i].size(); j++)
                    {
                        itype_id type = current[line]->tools[i][j].type;
                        int charges = current[line]->tools[i][j].count;
                        nc_color toolcol = c_red;

                        if (charges < 0 && crafting_inv.has_amount(type, 1))
                        {
                            toolcol = c_green;
                        }
                        else if (charges > 0 && crafting_inv.has_charges(type, charges))
                        {
                            toolcol = c_green;
                        }

                        std::stringstream toolinfo;
                        toolinfo << item_controller->find_template(type)->name << " ";

                        if (charges > 0)
                        {
                            toolinfo << "(" << charges << " charges) ";
                        }
                        std::string toolname = toolinfo.str();
                        if (xpos + toolname.length() >= 80)
                        {
                            xpos = 32;
                            ypos++;
                        }
                        mvwprintz(w_data, ypos, xpos, toolcol, toolname.c_str());
                        xpos += toolname.length();
                        if (j < current[line]->tools[i].size() - 1)
                        {
                            if (xpos >= 77)
                            {
                            xpos = 32;
                            ypos++;
                            }
                            mvwprintz(w_data, ypos, xpos, c_white, "OR ");
                            xpos += 3;
                        }
                    }
                }
            }
        // Loop to print the required components
            ypos++;
            mvwprintz(w_data, ypos, 30, col, "Components required:");
            for (int i = 0; i < current[line]->components.size(); i++)
            {
                if (current[line]->components[i].size() > 0)
                {
                    ypos++;
                    mvwputch(w_data, ypos, 30, col, '>');
                }
                xpos = 32;
                for (int j = 0; j < current[line]->components[i].size(); j++)
                {
                    int count = current[line]->components[i][j].count;
                    itype_id type = current[line]->components[i][j].type;
                    nc_color compcol = c_red;
                    if (item_controller->find_template(type)->count_by_charges() && count > 0)
                    {
                        if (crafting_inv.has_charges(type, count))
                        {
                            compcol = c_green;
                        }
                    }
                    else if (crafting_inv.has_amount(type, abs(count)))
                    {
                        compcol = c_green;
                    }
                    std::stringstream dump;
                    dump << abs(count) << "x " << item_controller->find_template(type)->name << " ";
                    std::string compname = dump.str();
                    if (xpos + compname.length() >= 80)
                    {
                        ypos++;
                        xpos = 32;
                    }
                    mvwprintz(w_data, ypos, xpos, compcol, compname.c_str());
                    xpos += compname.length();
                    if (j < current[line]->components[i].size() - 1)
                    {
                        if (xpos >= 77)
                        {
                            ypos++;
                            xpos = 32;
                        }
                        mvwprintz(w_data, ypos, xpos, c_white, "OR ");
                        xpos += 3;
                    }
                }
            }
        }

        wrefresh(w_data);
        input = get_input();
        switch (input)
        {
            case DirectionW:
            case DirectionUp:
                if (tab == "CC_WEAPON")
                {
                    tab = "CC_MISC";
                }
                else
                {
                    tab = prev_craft_cat(tab);
                }
                redraw = true;
                break;
            case DirectionE:
            case DirectionDown:
                if (tab == "CC_MISC")
                {
                    tab = "CC_WEAPON";
                }
                else
                {
                    tab = next_craft_cat(tab);
                }
                redraw = true;
                break;
            case DirectionS:
                line++;
                break;
            case DirectionN:
                line--;
                break;
            case Confirm:
                if (!available[line])
                {
                    popup("You can't do that!");
                }
                else
                {// is player making a liquid? Then need to check for valid container
                    if (item_controller->find_template(current[line]->result)->phase == LIQUID)
                    {
                        if (u.has_watertight_container() || u.has_matching_liquid(item_controller->find_template(current[line]->result)->id))
                        {
                            chosen = current[line];
                            done = true;
                            break;
                        }
                        else
                        {
                            popup("You don't have anything to store that liquid in!");
                        }
                    }
                    else
                    {
                        chosen = current[line];
                        done = true;
                    }
                }
                break;
            case Help:
                tmp = item(item_controller->find_template(current[line]->result), 0);
                full_screen_popup(tmp.info(true).c_str());
                redraw = true;
                break;
            case Filter:
                filterstring = string_input_popup("Search :",55,filterstring);
                redraw = true;
                break;
            case Reset:
                filterstring = "";
                redraw = true;
                break;

        }
        if (line < 0)
        {
            line = current.size() - 1;
        }
        else if (line >= current.size())
        {
            line = 0;
        }
    } while (input != Cancel && !done);

    werase(w_head);
    werase(w_data);
    delwin(w_head);
    delwin(w_data);
    refresh_all();

    return chosen;
}

void draw_recipe_tabs(WINDOW *w, craft_cat tab,bool filtered)
{
    werase(w);
    for (int i = 0; i < 80; i++)
    {
        mvwputch(w, 2, i, c_ltgray, LINE_OXOX);
    }

    mvwputch(w, 2,  0, c_ltgray, LINE_OXXO); // |^
    mvwputch(w, 2, 79, c_ltgray, LINE_OOXX); // ^|
    if(!filtered)
    {
        draw_tab(w,  2, "WEAPONS", (tab == "CC_WEAPON") ? true : false);
        draw_tab(w, 13, "AMMO",    (tab == "CC_AMMO")   ? true : false);
        draw_tab(w, 21, "FOOD",    (tab == "CC_FOOD")   ? true : false);
        draw_tab(w, 29, "DRINKS",  (tab == "CC_DRINK")  ? true : false);
        draw_tab(w, 39, "CHEMS",   (tab == "CC_CHEM")   ? true : false);
        draw_tab(w, 48, "ELECTRONICS", (tab == "CC_ELECTRONIC") ? true : false);
        draw_tab(w, 63, "ARMOR",   (tab == "CC_ARMOR")  ? true : false);
        draw_tab(w, 72, "MISC",    (tab == "CC_MISC")   ? true : false);
    }
    else
    {
        draw_tab(w,  2, "Searched", true);
    }

    wrefresh(w);
}

inventory game::crafting_inventory(){
 inventory crafting_inv;
 crafting_inv.form_from_map(this, point(u.posx, u.posy), PICKUP_RANGE);
 crafting_inv += u.inv;
 crafting_inv += u.weapon;
 if (u.has_bionic("bio_tools")) {
  item tools(item_controller->find_template("toolset"), turn);
  tools.charges = u.power_level;
  crafting_inv += tools;
 }
 return crafting_inv;
}

void game::pick_recipes(std::vector<recipe*> &current,
                        std::vector<bool> &available, craft_cat tab,std::string filter)
{
    current.clear();
    available.clear();

    if (filter == "")
    {
        add_known_recipes(current, recipes[tab]);
    }
    else
    {
        for (recipe_map::iterator iter = recipes.begin(); iter != recipes.end(); ++iter)
        {
            add_known_recipes(current, iter->second, filter);
        }
    }

    for (int i = 0; i < current.size(); i++)
    {
        //Check if we have the requisite tools and components
        if(can_make(current[i]))
        {
            available.push_back(true);
        }
        else
        {
            available.push_back(false);
        }
    }
}

void game::add_known_recipes(std::vector<recipe*> &current, recipe_list source, std::string filter)
{
    for (recipe_list::iterator iter = source.begin(); iter != source.end(); ++iter)
    {
        if (u.knows_recipe(*iter))
        {
            if ((*iter)->difficulty >= 0 )
            {
                if (filter == "" || item_controller->find_template((*iter)->result)->name.find(filter) != std::string::npos)
                {
                    current.push_back(*iter);
                }
            }
        }
    }
}

void game::make_craft(recipe *making)
{
 u.assign_activity(this, ACT_CRAFT, making->time, making->id);
 u.moves = 0;
 u.lastrecipe = making;
}


void game::make_all_craft(recipe *making)
{
 u.assign_activity(this, ACT_LONGCRAFT, making->time, making->id);
 u.moves = 0;
 u.lastrecipe = making;
}

void game::complete_craft()
{
 recipe* making = recipe_by_index(u.activity.index); // Which recipe is it?

// # of dice is 75% primary skill, 25% secondary (unless secondary is null)
 int skill_dice = u.skillLevel(making->sk_primary) * 3;
 if (making->sk_secondary == NULL)
   skill_dice += u.skillLevel(making->sk_primary);
 else
   skill_dice += u.skillLevel(making->sk_secondary);

// farsightedness can impose a penalty on electronics and tailoring success
// it's equivalent to a 2-rank electronics penalty, 1-rank tailoring
 if (u.has_trait(PF_HYPEROPIC) && !u.is_wearing("glasses_reading")) {
  int main_rank_penalty = 0;
  if (making->sk_primary == Skill::skill("electronics")) {
   main_rank_penalty = 2;
  } else if (making->sk_primary == Skill::skill("tailoring")) {
   main_rank_penalty = 1;
  }
  skill_dice -= main_rank_penalty * 3;

  if (making->sk_secondary == NULL) {
   skill_dice -= main_rank_penalty;
  } else {
   int second_rank_penalty = 0;
   if (making->sk_secondary == Skill::skill("electronics")) {
    second_rank_penalty = 2;
   } else if (making->sk_secondary == Skill::skill("tailoring")) {
    second_rank_penalty = 1;
   }
   skill_dice -= second_rank_penalty;
  }
 }

// Sides on dice is 16 plus your current intelligence
 int skill_sides = 16 + u.int_cur;

 int diff_dice = making->difficulty * 4; // Since skill level is * 4 also
 int diff_sides = 24;	// 16 + 8 (default intelligence)

 int skill_roll = dice(skill_dice, skill_sides);
 int diff_roll  = dice(diff_dice,  diff_sides);

 if (making->sk_primary)
  u.practice(turn, making->sk_primary, making->difficulty * 5 + 20);
 if (making->sk_secondary)
  u.practice(turn, making->sk_secondary, 5);

// Messed up badly; waste some components.
 if (making->difficulty != 0 && diff_roll > skill_roll * (1 + 0.1 * rng(1, 5))) {
  add_msg("You fail to make the %s, and waste some materials.",
          item_controller->find_template(making->result)->name.c_str());
    for (int i = 0; i < making->components.size(); i++) 
    {
        if (making->components[i].size() > 0)
        consume_items(making->components[i]);
    }  
  
  for (int i = 0; i < making->tools.size(); i++) {
   if (making->tools[i].size() > 0)
    consume_tools(making->tools[i]);
  }
  u.activity.type = ACT_NULL;
  return;
  // Messed up slightly; no components wasted.
 } else if (diff_roll > skill_roll) {
  add_msg("You fail to make the %s, but don't waste any materials.",
          item_controller->find_template(making->result)->name.c_str());
  //this method would only have been called from a place that nulls u.activity.type,
  //so it appears that it's safe to NOT null that variable here.
  //rationale: this allows certain contexts (e.g. ACT_LONGCRAFT) to distinguish major and minor failures
  return;
 }
// If we're here, the craft was a success!
// Use up the components and tools
 for (int i = 0; i < making->components.size(); i++) {
  if (making->components[i].size() > 0)
   consume_items(making->components[i]);
 }
 for (int i = 0; i < making->tools.size(); i++) {
  if (making->tools[i].size() > 0)
   consume_tools(making->tools[i]);
 }

  // Set up the new item, and pick an inventory letter
 int iter = 0;
 item newit(item_controller->find_template(making->result), turn, nextinv);

    if (newit.is_armor() && newit.has_flag(IF_VARSIZE))
    {
        newit.item_flags |= mfb(IF_FIT);
    }
 // for food items
 if (newit.is_food())
  {
    int bday_tmp = turn % 3600;		// fuzzy birthday for stacking reasons
    newit.bday = int(turn) + 3600 - bday_tmp;

		if (newit.has_flag(IF_EATEN_HOT)) {	// hot foods generated
			newit.item_flags |= mfb(IF_HOT);
			newit.active = true;
			newit.item_counter = 600;
		}
  }
 if (!newit.craft_has_charges())
  newit.charges = 0;
 do {
  newit.invlet = nextinv;
  advance_nextinv();
  iter++;
 } while (u.has_item(newit.invlet) && iter < inv_chars.size());
 //newit = newit.in_its_container(&itypes);
 if (newit.made_of(LIQUID))
  handle_liquid(newit, false, false);
 else {
// We might not have space for the item
  if (iter == inv_chars.size() || u.volume_carried()+newit.volume() > u.volume_capacity()) {
   add_msg("There's no room in your inventory for the %s, so you drop it.",
             newit.tname().c_str());
   m.add_item(u.posx, u.posy, newit);
  } else if (u.weight_carried() + newit.volume() > u.weight_capacity()) {
   add_msg("The %s is too heavy to carry, so you drop it.",
           newit.tname().c_str());
   m.add_item(u.posx, u.posy, newit);
  } else {
   u.i_add(newit);
   add_msg("%c - %s", newit.invlet, newit.tname().c_str());
  }
 }
}

void game::consume_items(std::vector<component> components)
{
// For each set of components in the recipe, fill you_have with the list of all
// matching ingredients the player has.
 std::vector<component> player_has;
 std::vector<component> map_has;
 std::vector<component> mixed;
 std::vector<component> player_use;
 std::vector<component> map_use;
 std::vector<component> mixed_use;
 inventory map_inv;
 map_inv.form_from_map(this, point(u.posx, u.posy), PICKUP_RANGE);

 for (int i = 0; i < components.size(); i++) {
  itype_id type = components[i].type;
  int count = abs(components[i].count);
  bool pl = false, mp = false;


  if (item_controller->find_template(type)->count_by_charges() && count > 0)
  {
   if (u.has_charges(type, count)) {
    player_has.push_back(components[i]);
    pl = true;
   }
   if (map_inv.has_charges(type, count)) {
    map_has.push_back(components[i]);
    mp = true;
   }
   if (!pl && !mp && u.charges_of(type) + map_inv.charges_of(type) >= count)
    mixed.push_back(components[i]);
  }

  else { // Counting by units, not charges

   if (u.has_amount(type, count)) {
    player_has.push_back(components[i]);
    pl = true;
   }
   if (map_inv.has_amount(type, count)) {
    map_has.push_back(components[i]);
    mp = true;
   }
   if (!pl && !mp && u.amount_of(type) + map_inv.amount_of(type) >= count)
    mixed.push_back(components[i]);

  }
 }

 if (player_has.size() + map_has.size() + mixed.size() == 1) { // Only 1 choice
  if (player_has.size() == 1)
   player_use.push_back(player_has[0]);
  else if (map_has.size() == 1)
   map_use.push_back(map_has[0]);
  else
   mixed_use.push_back(mixed[0]);

 } else { // Let the player pick which component they want to use
  std::vector<std::string> options; // List for the menu_vec below
// Populate options with the names of the items
  for (int i = 0; i < map_has.size(); i++) {
   std::string tmpStr = item_controller->find_template(map_has[i].type)->name + " (nearby)";
   options.push_back(tmpStr);
  }
  for (int i = 0; i < player_has.size(); i++)
   options.push_back(item_controller->find_template(player_has[i].type)->name);
  for (int i = 0; i < mixed.size(); i++) {
   std::string tmpStr = item_controller->find_template(mixed[i].type)->name +" (on person & nearby)";
   options.push_back(tmpStr);
  }

  if (options.size() == 0) // This SHOULD only happen if cooking with a fire,
   return;                 // and the fire goes out.

// Get the selection via a menu popup
  int selection = menu_vec(false, "Use which component?", options) - 1;
  if (selection < map_has.size())
   map_use.push_back(map_has[selection]);
  else if (selection < map_has.size() + player_has.size()) {
   selection -= map_has.size();
   player_use.push_back(player_has[selection]);
  } else {
   selection -= map_has.size() + player_has.size();
   mixed_use.push_back(mixed[selection]);
  }
 }

 for (int i = 0; i < player_use.size(); i++) {
  if (item_controller->find_template(player_use[i].type)->count_by_charges() &&
      player_use[i].count > 0)
   u.use_charges(player_use[i].type, player_use[i].count);
  else
   u.use_amount(player_use[i].type, abs(player_use[i].count),
                   (player_use[i].count < 0));
 }
 for (int i = 0; i < map_use.size(); i++) {
  if (item_controller->find_template(map_use[i].type)->count_by_charges() &&
      map_use[i].count > 0)
   m.use_charges(point(u.posx, u.posy), PICKUP_RANGE,
                    map_use[i].type, map_use[i].count);
  else
   m.use_amount(point(u.posx, u.posy), PICKUP_RANGE,
                   map_use[i].type, abs(map_use[i].count),
                   (map_use[i].count < 0));
 }
 for (int i = 0; i < mixed_use.size(); i++) {
  if (item_controller->find_template(mixed_use[i].type)->count_by_charges() &&
      mixed_use[i].count > 0) {
   int from_map = mixed_use[i].count - u.charges_of(mixed_use[i].type);
   u.use_charges(mixed_use[i].type, u.charges_of(mixed_use[i].type));
   m.use_charges(point(u.posx, u.posy), PICKUP_RANGE,
                    mixed_use[i].type, from_map);
  } else {
   bool in_container = (mixed_use[i].count < 0);
   int from_map = abs(mixed_use[i].count) - u.amount_of(mixed_use[i].type);
   u.use_amount(mixed_use[i].type, u.amount_of(mixed_use[i].type),
                   in_container);
   m.use_amount(point(u.posx, u.posy), PICKUP_RANGE,
                   mixed_use[i].type, from_map, in_container);
  }
 }
}

void game::consume_tools(std::vector<component> tools)
{
 bool found_nocharge = false;
 inventory map_inv;
 map_inv.form_from_map(this, point(u.posx, u.posy), PICKUP_RANGE);
 std::vector<component> player_has;
 std::vector<component> map_has;
// Use charges of any tools that require charges used
 for (int i = 0; i < tools.size() && !found_nocharge; i++) {
  itype_id type = tools[i].type;
  if (tools[i].count > 0) {
   int count = tools[i].count;
   if (u.has_charges(type, count))
    player_has.push_back(tools[i]);
   if (map_inv.has_charges(type, count))
    map_has.push_back(tools[i]);
  } else if (u.has_amount(type, 1) || map_inv.has_amount(type, 1))
   found_nocharge = true;
 }
 if (found_nocharge)
  return; // Default to using a tool that doesn't require charges

 if (player_has.size() == 1 && map_has.size() == 0)
  u.use_charges(player_has[0].type, player_has[0].count);
 else if (map_has.size() == 1 && player_has.size() == 0)
  m.use_charges(point(u.posx, u.posy), PICKUP_RANGE,
                   map_has[0].type, map_has[0].count);
 else { // Variety of options, list them and pick one
// Populate the list
  std::vector<std::string> options;
  for (int i = 0; i < map_has.size(); i++) {
   std::string tmpStr = item_controller->find_template(map_has[i].type)->name + " (nearby)";
   options.push_back(tmpStr);
  }
  for (int i = 0; i < player_has.size(); i++)
   options.push_back(item_controller->find_template(player_has[i].type)->name);

  if (options.size() == 0) // This SHOULD only happen if cooking with a fire,
   return;                 // and the fire goes out.

// Get selection via a popup menu
  int selection = menu_vec(false, "Use which tool?", options) - 1;
  if (selection < map_has.size())
   m.use_charges(point(u.posx, u.posy), PICKUP_RANGE,
                    map_has[selection].type, map_has[selection].count);
  else {
   selection -= map_has.size();
   u.use_charges(player_has[selection].type, player_has[selection].count);
  }
 }
}

void game::disassemble(char ch)
{
    if (!ch)
    {
        ch = inv("Disassemble item:");
    }
    if (ch == 27)
    {
        add_msg("Never mind.");
        return;
    }
    if (!u.has_item(ch))
    {
        add_msg("You don't have item '%c'!", ch);
        return;
    }

    item* dis_item = &u.i_at(ch);


    for (recipe_map::iterator cat_iter = recipes.begin(); cat_iter != recipes.end(); ++cat_iter)
    {
        for (recipe_list::iterator list_iter = cat_iter->second.begin();
             list_iter != cat_iter->second.end();
             ++list_iter)
        {
            recipe* cur_recipe = *list_iter;
            if (dis_item->type == item_controller->find_template(cur_recipe->result) && cur_recipe->reversible)
            // ok, a valid recipe exists for the item, and it is reversible
            // assign the activity
            {
                // check tools are available
                // loop over the tools and see what's required...again
                inventory crafting_inv = crafting_inventory();
                bool have_all_tools = true;
                for (int j = 0; j < cur_recipe->tools.size(); j++)
                {
                    bool have_this_tool = false;
                    if (cur_recipe->tools[j].size() == 0) // no tools required, may change this
                    {
                        have_this_tool = true;
                    }
                    else
                    {
                        for (int k = 0; k < cur_recipe->tools[j].size(); k++)
                        {
                            itype_id type = cur_recipe->tools[j][k].type;
                            int req = cur_recipe->tools[j][k].count;	// -1 => 1

                            if ((req <= 0 && crafting_inv.has_amount (type, 1)) ||
                                (req >  0 && crafting_inv.has_charges(type, req)))
                            {
                                have_this_tool = true;
                                k = cur_recipe->tools[j].size();
                            }
                            // if crafting recipe required a welder, disassembly requires a hacksaw or super toolkit
                            if (type == "welder")
                            {
                                if (crafting_inv.has_amount("hacksaw", 1) ||
                                    crafting_inv.has_amount("toolset", 1))
                                {
                                    have_this_tool = true;
                                }
                                else
                                {
                                    have_this_tool = false;
                                }
                            }
                        }

                        if (!have_this_tool)
                        {
                            have_all_tools = false;
                            int req = cur_recipe->tools[j][0].count;
                            if (cur_recipe->tools[j][0].type == "welder")
                            {
                                add_msg("You need a hack saw to disassemble this.");
                            }
                            else
                            {
                                if (req <= 0)
                                {
                                    add_msg("You need a %s to disassemble this.",
                                    item_controller->find_template(cur_recipe->tools[j][0].type)->name.c_str());
                                }
                                else
                                {
                                    add_msg("You need a %s with %d charges to disassemble this.",
                                    item_controller->find_template(cur_recipe->tools[j][0].type)->name.c_str(), req);
                                }
                            }
                        }
                    }
                }
                // all tools present, so assign the activity
                if (have_all_tools)
                {
                 
                  if (OPTIONS[OPT_QUERY_DISASSEMBLE] && !(query_yn("Really disassemble your %s?", dis_item->tname(this).c_str())))
                  {
                   return;
                  }
                    u.assign_activity(this, ACT_DISASSEMBLE, cur_recipe->time, cur_recipe->id);
                    u.moves = 0;
                    std::vector<int> dis_items;
                    dis_items.push_back(ch);
                    u.activity.values = dis_items;
                }
                return; // recipe exists, but no tools, so do not start disassembly
            }
        }
    }
    // no recipe exists, or the item cannot be disassembled
    add_msg("This item cannot be disassembled!");
}

void game::complete_disassemble()
{
  // which recipe was it?
  recipe* dis = recipe_by_index(u.activity.index); // Which recipe is it?
  item* dis_item = &u.i_at(u.activity.values[0]);

  add_msg("You disassemble the item into its components.");
  // remove any batteries or ammo first
    if (dis_item->is_gun() && dis_item->curammo != NULL && dis_item->ammo_type() != AT_NULL)
    {
      item ammodrop;
      ammodrop = item(dis_item->curammo, turn);
      ammodrop.charges = dis_item->charges;
      if (ammodrop.made_of(LIQUID))
        handle_liquid(ammodrop, false, false);
      else
        m.add_item(u.posx, u.posy, ammodrop);
    }
    if (dis_item->is_tool() && dis_item->charges > 0 && dis_item->ammo_type() != AT_NULL)
    {
      item ammodrop;
      ammodrop = item(item_controller->find_template(default_ammo(dis_item->ammo_type())), turn);
      ammodrop.charges = dis_item->charges;
      if (ammodrop.made_of(LIQUID))
        handle_liquid(ammodrop, false, false);
      else
        m.add_item(u.posx, u.posy, ammodrop);
    }
    u.i_rem(u.activity.values[0]);  // remove the item

  // consume tool charges
  for (int j = 0; j < dis->tools.size(); j++)
  {
    if (dis->tools[j].size() > 0)
    consume_tools(dis->tools[j]);
  }

  // add the components to the map
  // Player skills should determine how many components are returned

  // adapting original crafting formula to check if disassembly was successful
  // # of dice is 75% primary skill, 25% secondary (unless secondary is null)
  int skill_dice = 2 + u.skillLevel(dis->sk_primary) * 3;
   if (dis->sk_secondary == NULL)
     skill_dice += u.skillLevel(dis->sk_primary);
   else
     skill_dice += u.skillLevel(dis->sk_secondary);
  // Sides on dice is 16 plus your current intelligence
   int skill_sides = 16 + u.int_cur;

   int diff_dice = dis->difficulty;
   int diff_sides = 24;	// 16 + 8 (default intelligence)

   // disassembly only nets a bit of practice
   if (dis->sk_primary)
    u.practice(turn, dis->sk_primary, (dis->difficulty) * 2);
   if (dis->sk_secondary)
    u.practice(turn, dis->sk_secondary, 2);

  for (int j = 0; j < dis->components.size(); j++)
  {
    if (dis->components[j].size() != 0)
    {
      int compcount = dis->components[j][0].count;
      bool comp_success = (dice(skill_dice, skill_sides) > dice(diff_dice,  diff_sides));
      do
      {
        item newit(item_controller->find_template(dis->components[j][0].type), turn);
        // skip item addition if component is a consumable like superglue
        if (dis->components[j][0].type == "superglue" || dis->components[j][0].type == "duct_tape")
          compcount--;
        else
        {
          if (newit.count_by_charges())
          {
            if (dis->difficulty == 0 || comp_success)
              m.spawn_item(u.posx, u.posy, item_controller->find_template(dis->components[j][0].type), 0, 0, compcount);
            else
              add_msg("You fail to recover a component.");
            compcount = 0;
          } else
          {
            if (dis->difficulty == 0 || comp_success)
              m.add_item(u.posx, u.posy, newit);
            else
              add_msg("You fail to recover a component.");
            compcount--;
          }
        }
      } while (compcount > 0);
    }
  }

  if (dis->learn_by_disassembly >= 0 && !u.knows_recipe(dis))
  {
    if (dis->sk_primary == NULL || dis->learn_by_disassembly <= u.skillLevel(dis->sk_primary))
    {
      if (rng(0,3) == 0)
      {
        u.learn_recipe(dis);
        add_msg("You learned a recipe from this disassembly!");
      }
      else
      {
        add_msg("You think you could learn a recipe from this item. Maybe you'll try again.");
      }
    }
    else
    {
      add_msg("With some more skill, you might learn a recipe from this.");
    }
  }
}

recipe* game::recipe_by_index(int index)
{
    for (recipe_map::iterator map_iter = recipes.begin(); map_iter != recipes.end(); ++map_iter)
    {
        for (recipe_list::iterator list_iter = map_iter->second.begin(); list_iter != map_iter->second.end(); ++list_iter)
        {
            if ((*list_iter)->id == index)
            {
                return *list_iter;
            }
        }
    }
    return NULL;
}

recipe* game::recipe_by_name(std::string name)
{
    for (recipe_map::iterator map_iter = recipes.begin(); map_iter != recipes.end(); ++map_iter)
    {
        for (recipe_list::iterator list_iter = map_iter->second.begin(); list_iter != map_iter->second.end(); ++list_iter)
        {
            if ((*list_iter)->ident == name)
            {
                return *list_iter;
            }
        }
    }
    return NULL;
}
