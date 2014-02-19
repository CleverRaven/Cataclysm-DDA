#include "gamemode.h"
#include "game.h"
#include "setvector.h"
#include "keypress.h"
#include "itype.h"
#include "mtype.h"
#include "overmapbuffer.h"
#include "crafting.h"
#include "monstergenerator.h"
#include "construction.h"
#include <string>
#include <vector>
#include <sstream>

#define SPECIAL_WAVE_CHANCE 5 // One in X chance of single-flavor wave
#define SPECIAL_WAVE_MIN 5 // Don't use a special wave with < X monsters

#define SELCOL(n) (selection == (n) ? c_yellow : c_blue)
#define TOGCOL(n, b) (selection == (n) ? (b ? c_ltgreen : c_yellow) :\
                      (b ? c_green : c_dkgray))
#define NUMALIGN(n) ((n) >= 10000 ? 20 : ((n) >= 1000 ? 21 :\
                     ((n) >= 100 ? 22 : ((n) >= 10 ? 23 : 24))))

std::string caravan_category_name(caravan_category cat);
std::vector<itype_id> caravan_items(caravan_category cat);
std::set<m_flag> monflags_to_add;

std::map<std::string, defense_game_monchanges> montype_changes;
std::map<std::string, std::vector<int> > original_itype_values;
std::vector<int> original_construction_values;
std::vector<int> original_recipe_values;


int caravan_price(player &u, int price);

void draw_caravan_borders(WINDOW *w, int current_window);
void draw_caravan_categories(WINDOW *w, int category_selected, int total_price,
                             unsigned long cash);
void draw_caravan_items(WINDOW *w, std::vector<itype_id> *items,
                        std::vector<int> *counts, int offset,
                        int item_selected);

std::string defense_style_name(defense_style style);
std::string defense_style_description(defense_style style);
std::string defense_location_name(defense_location location);
std::string defense_location_description(defense_location location);

defense_game::defense_game()
{
    current_wave = 0;
    hunger = false;
    thirst = false;
    sleep  = false;
    zombies = false;
    specials = false;
    spiders = false;
    triffids = false;
    robots = false;
    subspace = false;
    mercenaries = false;
    init_to_style(DEFENSE_EASY);
}

bool defense_game::init()
{
    g->turn = HOURS(12); // Start at noon
    g->temperature = 65;
    if (!g->u.create(PLTYPE_CUSTOM)) {
        return false;
    }
    g->u.str_cur = g->u.str_max;
    g->u.per_cur = g->u.per_max;
    g->u.int_cur = g->u.int_max;
    g->u.dex_cur = g->u.dex_max;
    init_itypes();
    init_mtypes();
    init_constructions();
    init_recipes();
    current_wave = 0;
    hunger = false;
    thirst = false;
    sleep  = false;
    zombies = false;
    specials = false;
    spiders = false;
    triffids = false;
    robots = false;
    subspace = false;
    mercenaries = false;
    init_to_style(DEFENSE_EASY);
    setup();
    g->u.cash = initial_cash;
    popup_nowait(_("Please wait as the map generates [ 0%]"));
    // TODO: support multiple defence games? clean up old defence game
    g->cur_om = &overmap_buffer.get(0, 0);
    init_map();
    caravan();
    return true;
}

void defense_game::per_turn()
{
    if (!thirst) {
        g->u.thirst = 0;
    }
    if (!hunger) {
        g->u.hunger = 0;
    }
    if (!sleep) {
        g->u.fatigue = 0;
    }
    if (int(g->turn) % (time_between_waves * 10) == 0) {
        current_wave++;
        if (current_wave > 1 && current_wave % waves_between_caravans == 0) {
            popup(_("A caravan approaches!  Press spacebar..."));
            caravan();
        }
        spawn_wave();
    }
}

void defense_game::pre_action(action_id &act)
{
    if (act == ACTION_SLEEP && !sleep) {
        g->add_msg(_("You don't need to sleep!"));
        act = ACTION_NULL;
    }
    if (act == ACTION_SAVE || act == ACTION_QUICKSAVE) {
        g->add_msg(_("You cannot save in defense mode!"));
        act = ACTION_NULL;
    }

    // Big ugly block for movement
    if ((act == ACTION_MOVE_N && g->u.posy == SEEX * int(MAPSIZE / 2) &&
         g->levy <= 93) ||
        (act == ACTION_MOVE_NE && ((g->u.posy == SEEY * int(MAPSIZE / 2) &&
                                    g->levy <=  93) ||
                                   (g->u.posx == SEEX * (1 + int(MAPSIZE / 2)) - 1 &&
                                    g->levx >= 98))) ||
        (act == ACTION_MOVE_E && g->u.posx == SEEX * (1 + int(MAPSIZE / 2)) - 1 &&
         g->levx >= 98) ||
        (act == ACTION_MOVE_SE && ((g->u.posy == SEEY * (1 + int(MAPSIZE / 2)) - 1 &&
                                    g->levy >= 98) ||
                                   (g->u.posx == SEEX * (1 + int(MAPSIZE / 2)) - 1 &&
                                    g->levx >= 98))) ||
        (act == ACTION_MOVE_S && g->u.posy == SEEY * (1 + int(MAPSIZE / 2)) - 1 &&
         g->levy >= 98) ||
        (act == ACTION_MOVE_SW && ((g->u.posy == SEEY * (1 + int(MAPSIZE / 2)) - 1 &&
                                    g->levy >= 98) ||
                                   (g->u.posx == SEEX * int(MAPSIZE / 2) &&
                                    g->levx <=  93))) ||
        (act == ACTION_MOVE_W && g->u.posx == SEEX * int(MAPSIZE / 2) &&
         g->levx <= 93) ||
        (act == ACTION_MOVE_NW && ((g->u.posy == SEEY * int(MAPSIZE / 2) &&
                                    g->levy <=  93) ||
                                   (g->u.posx == SEEX * int(MAPSIZE / 2) &&
                                    g->levx <=  93)))) {
        g->add_msg(_("You cannot leave the %s behind!"),
                   defense_location_name(location).c_str());
        act = ACTION_NULL;
    }
}

void defense_game::post_action(action_id act)
{
    (void)g;
    (void)act;
}

void defense_game::game_over()
{
    (void)g; //unused
    popup(_("You managed to survive through wave %d!"), current_wave);
    // Reset changed information
    reset_mtypes();
    reset_itypes();
    reset_constructions();
    reset_recipes();
    // Clear the changes from memory so that they are not added to multiple times in successive Defense games in one sitting.
    montype_changes.clear();
    original_construction_values.clear();
    original_itype_values.clear();
    original_recipe_values.clear();
}

void defense_game::reset_mtypes()
{
    // Reset mtype changes
    std::map<std::string, mtype*> montemplates = MonsterGenerator::generator().get_all_mtypes();
    for (std::map<std::string, mtype*>::iterator it = montemplates.begin(); it != montemplates.end(); ++it){
        defense_game_monchanges change = montype_changes[it->first];

        it->second->difficulty = change.original_difficulty;
        for (std::set<m_flag>::iterator fit = change.added_flags.begin(); fit != change.added_flags.end(); ++fit){
            it->second->flags.erase(*fit);
        }
    }
}

void defense_game::reset_itypes()
{
    itypes["2x4"]->volume = original_itype_values["2x4"][0];
    itypes["2x4"]->weight = original_itype_values["2x4"][1];
    itypes["landmine"]->price = original_itype_values["landmine"][0];
    itypes["bot_turret"]->price = original_itype_values["bot_turret"][0];
}

void defense_game::reset_constructions()
{
    for (unsigned i = 0; i < constructions.size(); i++) {
        constructions[i]->time = original_construction_values[i];
    }
}

void defense_game::reset_recipes()
{
    std::vector<int>::iterator it = original_recipe_values.begin();
    for (recipe_map::iterator map_iter = recipes.begin(); map_iter != recipes.end(); ++map_iter) {
        for (recipe_list::iterator list_iter = map_iter->second.begin();
             list_iter != map_iter->second.end(); ++list_iter, ++it) {
            (*list_iter)->time = *it; // Things take turns, not minutes
        }
    }
}

void defense_game::init_itypes()
{
    std::vector<int> change_2x4, change_landmine, change_bot;
    change_2x4.push_back(itypes["2x4"]->volume);
    change_2x4.push_back(itypes["2x4"]->weight);
    change_landmine.push_back(itypes["landmine"]->price);
    change_bot.push_back(itypes["bot_turret"]->price);

    original_itype_values["2x4"] = change_2x4;
    original_itype_values["landmine"] = change_landmine;
    original_itype_values["bot_turret"] = change_bot;

    itypes["2x4"]->volume = 0;
    itypes["2x4"]->weight = 0;
    itypes["landmine"]->price = 300;
    itypes["bot_turret"]->price = 6000;
}

void defense_game::init_mtypes()
{
    m_flag flags[] = {MF_BASHES, MF_SMELLS, MF_HEARS, MF_SEES};
    monflags_to_add.insert(flags, flags + 4);

    std::map<std::string, mtype *> montemplates = MonsterGenerator::generator().get_all_mtypes();
    std::pair<std::set<m_flag>::iterator, bool> ret;

    for (std::map<std::string, mtype*>::iterator it = montemplates.begin(); it != montemplates.end(); ++it){
        defense_game_monchanges change;
        change.original_difficulty = it->second->difficulty;

        it->second->difficulty *= 1.5;
        it->second->difficulty += int(it->second->difficulty / 5);
        for (std::set<m_flag>::iterator fit = monflags_to_add.begin(); fit != monflags_to_add.end(); ++fit){
            ret = it->second->flags.insert(*fit);
            if (ret.second) {
                change.added_flags.insert(*fit);
            }
        }

        montype_changes[it->first] = change;
    }
}

void defense_game::init_constructions()
{
    for (unsigned i = 0; i < constructions.size(); i++) {
        original_construction_values.push_back(constructions[i]->time);
        constructions[i]->time = 1; // Everything takes 1 minute
    }
}

void defense_game::init_recipes()
{
    for (recipe_map::iterator map_iter = recipes.begin(); map_iter != recipes.end(); ++map_iter) {
        for (recipe_list::iterator list_iter = map_iter->second.begin();
             list_iter != map_iter->second.end(); ++list_iter) {
            original_recipe_values.push_back((*list_iter)->time);
            (*list_iter)->time /= 10; // Things take turns, not minutes
        }
    }
}

void defense_game::init_map()
{
    for (int x = 0; x < OMAPX; x++) {
        for (int y = 0; y < OMAPY; y++) {
            g->cur_om->ter(x, y, 0) = "field";
            g->cur_om->seen(x, y, 0) = true;
        }
    }

    g->cur_om->save();
    g->levx = 100;
    g->levy = 100;
    g->levz = 0;
    g->u.posx = SEEX;
    g->u.posy = SEEY;

    switch (location) {

    case DEFLOC_HOSPITAL:
        for (int x = 49; x <= 51; x++) {
            for (int y = 49; y <= 51; y++) {
                g->cur_om->ter(x, y, 0) = "hospital";
            }
        }
        g->cur_om->ter(50, 49, 0) = "hospital_entrance";
        break;

    case DEFLOC_WORKS:
        for (int x = 49; x <= 50; x++) {
            for (int y = 49; y <= 50; y++) {
                g->cur_om->ter(x, y, 0) = "public_works";
            }
        }
        g->cur_om->ter(50, 49, 0) = "public_works_entrance";
        break;

    case DEFLOC_MALL:
        for (int x = 49; x <= 51; x++) {
            for (int y = 49; y <= 51; y++) {
                g->cur_om->ter(x, y, 0) = "megastore";
            }
        }
        g->cur_om->ter(50, 49, 0) = "megastore_entrance";
        break;

    case DEFLOC_BAR:
        g->cur_om->ter(50, 50, 0) = "bar_north";
        break;

    case DEFLOC_MANSION:
        for (int x = 49; x <= 51; x++) {
            for (int y = 49; y <= 51; y++) {
                g->cur_om->ter(x, y, 0) = "mansion";
            }
        }
        g->cur_om->ter(50, 49, 0) = "mansion_entrance";
        break;
    }

    // Init the map
    int old_percent = 0;
    for (int i = 0; i <= MAPSIZE * 2; i += 2) {
        for (int j = 0; j <= MAPSIZE * 2; j += 2) {
            int mx = g->levx - MAPSIZE + i, my = g->levy - MAPSIZE + j;
            int percent = 100 * ((j / 2 + MAPSIZE * (i / 2))) /
                          ((MAPSIZE) * (MAPSIZE + 1));
            if (percent >= old_percent + 1) {
                popup_nowait(_("Please wait as the map generates [%2d%]"), percent);
                old_percent = percent;
            }
            // Round down to the nearest even number
            mx -= mx % 2;
            my -= my % 2;
            tinymap tm(&g->traps);
            tm.generate(g->cur_om, mx, my, 0, int(g->turn));
            tm.clear_spawns();
            tm.clear_traps();
            tm.save(g->cur_om, int(g->turn), mx, my, 0);
        }
    }

    g->m.load(g->levx, g->levy, g->levz, true);

    g->update_map(g->u.posx, g->u.posy);
    monster generator(GetMType("mon_generator"), g->u.posx + 1, g->u.posy + 1);
    // Find a valid spot to spawn the generator
    std::vector<point> valid;
    for (int x = g->u.posx - 1; x <= g->u.posx + 1; x++) {
        for (int y = g->u.posy - 1; y <= g->u.posy + 1; y++) {
            if (generator.can_move_to(x, y) && g->is_empty(x, y)) {
                valid.push_back( point(x, y) );
            }
        }
    }
    if (!valid.empty()) {
        point p = valid[rng(0, valid.size() - 1)];
        generator.spawn(p.x, p.y);
    }
    generator.friendly = -1;
    g->add_zombie(generator);
}

void defense_game::init_to_style(defense_style new_style)
{
    style = new_style;
    hunger = false;
    thirst = false;
    sleep  = false;
    zombies = false;
    specials = false;
    spiders = false;
    triffids = false;
    robots = false;
    subspace = false;
    mercenaries = false;

    switch (new_style) {
    case DEFENSE_EASY:
        location = DEFLOC_HOSPITAL;
        initial_difficulty = 15;
        wave_difficulty = 10;
        time_between_waves = 30;
        waves_between_caravans = 3;
        initial_cash = 1000000;
        cash_per_wave = 100000;
        cash_increase = 30000;
        specials = true;
        spiders = true;
        triffids = true;
        mercenaries = true;
        break;

    case DEFENSE_MEDIUM:
        location = DEFLOC_MALL;
        initial_difficulty = 30;
        wave_difficulty = 15;
        time_between_waves = 20;
        waves_between_caravans = 4;
        initial_cash = 600000;
        cash_per_wave = 80000;
        cash_increase = 20000;
        specials = true;
        spiders = true;
        triffids = true;
        robots = true;
        hunger = true;
        mercenaries = true;
        break;

    case DEFENSE_HARD:
        location = DEFLOC_BAR;
        initial_difficulty = 50;
        wave_difficulty = 20;
        time_between_waves = 10;
        waves_between_caravans = 5;
        initial_cash = 200000;
        cash_per_wave = 60000;
        cash_increase = 10000;
        specials = true;
        spiders = true;
        triffids = true;
        robots = true;
        subspace = true;
        hunger = true;
        thirst = true;
        break;

        break;

    case DEFENSE_SHAUN:
        location = DEFLOC_BAR;
        initial_difficulty = 30;
        wave_difficulty = 15;
        time_between_waves = 5;
        waves_between_caravans = 6;
        initial_cash = 500000;
        cash_per_wave = 50000;
        cash_increase = 10000;
        zombies = true;
        break;

    case DEFENSE_DAWN:
        location = DEFLOC_MALL;
        initial_difficulty = 60;
        wave_difficulty = 20;
        time_between_waves = 30;
        waves_between_caravans = 4;
        initial_cash = 800000;
        cash_per_wave = 50000;
        cash_increase = 0;
        zombies = true;
        hunger = true;
        thirst = true;
        mercenaries = true;
        break;

    case DEFENSE_SPIDERS:
        location = DEFLOC_MALL;
        initial_difficulty = 60;
        wave_difficulty = 10;
        time_between_waves = 10;
        waves_between_caravans = 4;
        initial_cash = 600000;
        cash_per_wave = 50000;
        cash_increase = 10000;
        spiders = true;
        break;

    case DEFENSE_TRIFFIDS:
        location = DEFLOC_MANSION;
        initial_difficulty = 60;
        wave_difficulty = 20;
        time_between_waves = 30;
        waves_between_caravans = 2;
        initial_cash = 1000000;
        cash_per_wave = 60000;
        cash_increase = 10000;
        triffids = true;
        hunger = true;
        thirst = true;
        sleep = true;
        mercenaries = true;
        break;

    case DEFENSE_SKYNET:
        location = DEFLOC_HOSPITAL;
        initial_difficulty = 20;
        wave_difficulty = 20;
        time_between_waves = 20;
        waves_between_caravans = 6;
        initial_cash = 1200000;
        cash_per_wave = 100000;
        cash_increase = 20000;
        robots = true;
        hunger = true;
        thirst = true;
        mercenaries = true;
        break;

    case DEFENSE_LOVECRAFT:
        location = DEFLOC_MANSION;
        initial_difficulty = 20;
        wave_difficulty = 20;
        time_between_waves = 120;
        waves_between_caravans = 8;
        initial_cash = 400000;
        cash_per_wave = 100000;
        cash_increase = 10000;
        subspace = true;
        hunger = true;
        thirst = true;
        sleep = true;
    }
}

void defense_game::setup()
{
    WINDOW *w = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH, 0, 0);
    int selection = 1;
    refresh_setup(w, selection);

    while (true) {
        char ch = input();

        if (ch == 'S') {
            if (!zombies && !specials && !spiders && !triffids && !robots && !subspace) {
                popup(_("You must choose at least one monster group!"));
                refresh_setup(w, selection);
            } else {
                return;
            }
        } else if (ch == '+' || ch == '>' || ch == 'j') {
            if (selection == 19) {
                selection = 1;
            } else {
                selection++;
            }
            refresh_setup(w, selection);
        } else if (ch == '-' || ch == '<' || ch == 'k') {
            if (selection == 1) {
                selection = 19;
            } else {
                selection--;
            }
            refresh_setup(w, selection);
        } else if (ch == '!') {
            std::string name = string_input_popup(_("Template Name:"), 20); //TODO: this is NON FUNCTIONAL!!!
            refresh_setup(w, selection);
        } else {
            switch (selection) {
            case 1: // Scenario selection
                if (ch == 'l') {
                    if (style == defense_style(NUM_DEFENSE_STYLES - 1)) {
                        style = defense_style(1);
                    } else {
                        style = defense_style(style + 1);
                    }
                }
                if (ch == 'h') {
                    if (style == defense_style(1)) {
                        style = defense_style(NUM_DEFENSE_STYLES - 1);
                    } else {
                        style = defense_style(style - 1);
                    }
                }
                init_to_style(style);
                break;

            case 2: // Location selection
                if (ch == 'l') {
                    if (location == defense_location(NUM_DEFENSE_LOCATIONS - 1)) {
                        location = defense_location(1);
                    } else {
                        location = defense_location(location + 1);
                    }
                }
                if (ch == 'h') {
                    if (location == defense_location(1)) {
                        location = defense_location(NUM_DEFENSE_LOCATIONS - 1);
                    } else {
                        location = defense_location(location - 1);
                    }
                }
                mvwprintz(w, 5, 2, c_black, "\
 xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
                mvwprintz(w, 5, 2, c_yellow, defense_location_name(location).c_str());
                mvwprintz(w,  5, 28, c_ltgray,
                          defense_location_description(location).c_str());
                break;

            case 3: // Difficulty of the first wave
                if (ch == 'h' && initial_difficulty > 10) {
                    initial_difficulty -= 5;
                }
                if (ch == 'l' && initial_difficulty < 995) {
                    initial_difficulty += 5;
                }
                mvwprintz(w, 7, 22, c_black, "xxx");
                mvwprintz(w, 7, NUMALIGN(initial_difficulty), c_yellow, "%d",
                          initial_difficulty);
                break;

            case 4: // Wave Difficulty
                if (ch == 'h' && wave_difficulty > 10) {
                    wave_difficulty -= 5;
                }
                if (ch == 'l' && wave_difficulty < 995) {
                    wave_difficulty += 5;
                }
                mvwprintz(w, 8, 22, c_black, "xxx");
                mvwprintz(w, 8, NUMALIGN(wave_difficulty), c_yellow, "%d",
                          wave_difficulty);
                break;

            case 5:
                if (ch == 'h' && time_between_waves > 5) {
                    time_between_waves -= 5;
                }
                if (ch == 'l' && time_between_waves < 995) {
                    time_between_waves += 5;
                }
                mvwprintz(w, 10, 22, c_black, "xxx");
                mvwprintz(w, 10, NUMALIGN(time_between_waves), c_yellow, "%d",
                          time_between_waves);
                break;

            case 6:
                if (ch == 'h' && waves_between_caravans > 1) {
                    waves_between_caravans -= 1;
                }
                if (ch == 'l' && waves_between_caravans < 50) {
                    waves_between_caravans += 1;
                }
                mvwprintz(w, 11, 22, c_black, "xxx");
                mvwprintz(w, 11, NUMALIGN(waves_between_caravans), c_yellow, "%d",
                          waves_between_caravans);
                break;

            case 7:
                if (ch == 'h' && initial_cash > 0) {
                    initial_cash -= 100;
                }
                if (ch == 'l' && initial_cash < 99900) {
                    initial_cash += 100;
                }
                mvwprintz(w, 13, 20, c_black, "xxxxx");
                mvwprintz(w, 13, NUMALIGN(initial_cash), c_yellow, "%d", initial_cash);
                break;

            case 8:
                if (ch == 'h' && cash_per_wave > 0) {
                    cash_per_wave -= 100;
                }
                if (ch == 'l' && cash_per_wave < 9900) {
                    cash_per_wave += 100;
                }
                mvwprintz(w, 14, 21, c_black, "xxxx");
                mvwprintz(w, 14, NUMALIGN(cash_per_wave), c_yellow, "%d", cash_per_wave);
                break;

            case 9:
                if (ch == 'h' && cash_increase > 0) {
                    cash_increase -= 50;
                }
                if (ch == 'l' && cash_increase < 9950) {
                    cash_increase += 50;
                }
                mvwprintz(w, 15, 21, c_black, "xxxx");
                mvwprintz(w, 15, NUMALIGN(cash_increase), c_yellow, "%d", cash_increase);
                break;

            case 10:
                if (ch == ' ' || ch == '\n') {
                    zombies = !zombies;
                    specials = false;
                }
                mvwprintz(w, 18, 2, (zombies ? c_ltgreen : c_yellow), "Zombies");
                mvwprintz(w, 18, 14, c_yellow, _("Special Zombies"));
                break;

            case 11:
                if (ch == ' ' || ch == '\n') {
                    specials = !specials;
                    zombies = false;
                }
                mvwprintz(w, 18, 2, c_yellow, _("Zombies"));
                mvwprintz(w, 18, 14, (specials ? c_ltgreen : c_yellow), _("Special Zombies"));
                break;

            case 12:
                if (ch == ' ' || ch == '\n') {
                    spiders = !spiders;
                }
                mvwprintz(w, 18, 34, (spiders ? c_ltgreen : c_yellow), _("Spiders"));
                break;

            case 13:
                if (ch == ' ' || ch == '\n') {
                    triffids = !triffids;
                }
                mvwprintz(w, 18, 46, (triffids ? c_ltgreen : c_yellow), _("Triffids"));
                break;

            case 14:
                if (ch == ' ' || ch == '\n') {
                    robots = !robots;
                }
                mvwprintz(w, 18, 59, (robots ? c_ltgreen : c_yellow), _("Robots"));
                break;

            case 15:
                if (ch == ' ' || ch == '\n') {
                    subspace = !subspace;
                }
                mvwprintz(w, 18, 70, (subspace ? c_ltgreen : c_yellow), _("Subspace"));
                break;

            case 16:
                if (ch == ' ' || ch == '\n') {
                    hunger = !hunger;
                }
                mvwprintz(w, 21, 2, (hunger ? c_ltgreen : c_yellow), _("Food"));
                break;

            case 17:
                if (ch == ' ' || ch == '\n') {
                    thirst = !thirst;
                }
                mvwprintz(w, 21, 16, (thirst ? c_ltgreen : c_yellow), _("Water"));
                break;

            case 18:
                if (ch == ' ' || ch == '\n') {
                    sleep = !sleep;
                }
                mvwprintz(w, 21, 31, (sleep ? c_ltgreen : c_yellow), _("Sleep"));
                break;

            case 19:
                if (ch == ' ' || ch == '\n') {
                    mercenaries = !mercenaries;
                }
                mvwprintz(w, 21, 46, (mercenaries ? c_ltgreen : c_yellow), _("Mercenaries"));
                break;
            }
        }
        if (ch == 'h' || ch == 'l' || ch == ' ' || ch == '\n') {
            refresh_setup(w, selection);
        }
    }
}

void defense_game::refresh_setup(WINDOW *w, int selection)
{
    werase(w);
    mvwprintz(w,  0,  1, c_ltred, _("DEFENSE MODE"));
    mvwprintz(w,  0, 28, c_ltred, _("Press +/- or >/< to cycle, spacebar to toggle"));
    mvwprintz(w,  1, 28, c_ltred, _("Press S to start, ! to save as a template"));
    mvwprintz(w,  2,  2, c_ltgray, _("Scenario:"));
    mvwprintz(w,  3,  2, SELCOL(1), defense_style_name(style).c_str());
    mvwprintz(w,  3, 28, c_ltgray, defense_style_description(style).c_str());
    mvwprintz(w,  4,  2, c_ltgray, _("Location:"));
    mvwprintz(w,  5,  2, SELCOL(2), defense_location_name(location).c_str());
    mvwprintz(w,  5, 28, c_ltgray, defense_location_description(location).c_str());

    mvwprintz(w,  7,  2, c_ltgray, _("Initial Difficulty:"));
    mvwprintz(w,  7, NUMALIGN(initial_difficulty), SELCOL(3), "%d",
              initial_difficulty);
    mvwprintz(w,  7, 28, c_ltgray, _("The difficulty of the first wave."));
    mvwprintz(w,  8,  2, c_ltgray, _("Wave Difficulty:"));
    mvwprintz(w,  8, NUMALIGN(wave_difficulty), SELCOL(4), "%d", wave_difficulty);
    mvwprintz(w,  8, 28, c_ltgray, _("The increase of difficulty with each wave."));

    mvwprintz(w, 10,  2, c_ltgray, _("Time b/w Waves:"));
    mvwprintz(w, 10, NUMALIGN(time_between_waves), SELCOL(5), "%d",
              time_between_waves);
    mvwprintz(w, 10, 28, c_ltgray, _("The time, in minutes, between waves."));
    mvwprintz(w, 11,  2, c_ltgray, _("Waves b/w Caravans:"));
    mvwprintz(w, 11, NUMALIGN(waves_between_caravans), SELCOL(6), "%d",
              waves_between_caravans);
    mvwprintz(w, 11, 28, c_ltgray, _("The number of waves in between caravans."));

    mvwprintz(w, 13,  2, c_ltgray, _("Initial Cash:"));
    mvwprintz(w, 13, NUMALIGN(initial_cash), SELCOL(7), "%d", initial_cash);
    mvwprintz(w, 13, 28, c_ltgray, _("The amount of money the player starts with."));
    mvwprintz(w, 14,  2, c_ltgray, _("Cash for 1st Wave:"));
    mvwprintz(w, 14, NUMALIGN(cash_per_wave), SELCOL(8), "%d", cash_per_wave);
    mvwprintz(w, 14, 28, c_ltgray, _("The cash awarded for the first wave."));
    mvwprintz(w, 15,  2, c_ltgray, _("Cash Increase:"));
    mvwprintz(w, 15, NUMALIGN(cash_increase), SELCOL(9), "%d", cash_increase);
    mvwprintz(w, 15, 28, c_ltgray, _("The increase in the award each wave."));

    mvwprintz(w, 17,  2, c_ltgray, _("Enemy Selection:"));
    mvwprintz(w, 18,  2, TOGCOL(10, zombies), _("Zombies"));
    mvwprintz(w, 18, 14, TOGCOL(11, specials), _("Special Zombies"));
    mvwprintz(w, 18, 34, TOGCOL(12, spiders), _("Spiders"));
    mvwprintz(w, 18, 46, TOGCOL(13, triffids), _("Triffids"));
    mvwprintz(w, 18, 59, TOGCOL(14, robots), _("Robots"));
    mvwprintz(w, 18, 70, TOGCOL(15, subspace), _("Subspace"));

    mvwprintz(w, 20,  2, c_ltgray, _("Needs:"));
    mvwprintz(w, 21,  2, TOGCOL(16, hunger), _("Food"));
    mvwprintz(w, 21, 16, TOGCOL(17, thirst), _("Water"));
    mvwprintz(w, 21, 31, TOGCOL(18, sleep), _("Sleep"));
    mvwprintz(w, 21, 46, TOGCOL(19, mercenaries), _("Mercenaries"));
    wrefresh(w);
}

std::string defense_style_name(defense_style style)
{
    // 24 Characters Max!
    switch (style) {
    case DEFENSE_CUSTOM:
        return _("Custom");
    case DEFENSE_EASY:
        return _("Easy");
    case DEFENSE_MEDIUM:
        return _("Medium");
    case DEFENSE_HARD:
        return _("Hard");
    case DEFENSE_SHAUN:
        return _("Shaun of the Dead");
    case DEFENSE_DAWN:
        return _("Dawn of the Dead");
    case DEFENSE_SPIDERS:
        return _("Eight-Legged Freaks");
    case DEFENSE_TRIFFIDS:
        return _("Day of the Triffids");
    case DEFENSE_SKYNET:
        return _("Skynet");
    case DEFENSE_LOVECRAFT:
        return _("The Call of Cthulhu");
    default:
        return "Bug! (bug in defense.cpp:defense_style_name)";
    }
}

std::string defense_style_description(defense_style style)
{
    // 51 Characters Max!
    switch (style) {
    case DEFENSE_CUSTOM:
        return _("A custom game.");
    case DEFENSE_EASY:
        return _("Easy monsters and lots of money.");
    case DEFENSE_MEDIUM:
        return _("Harder monsters.  You have to eat.");
    case DEFENSE_HARD:
        return _("All monsters.  You have to eat and drink.");
    case DEFENSE_SHAUN:
        return _("Defend a bar against classic zombies.  Easy and fun.");
    case DEFENSE_DAWN:
        return _("Classic zombies.  Slower and more realistic.");
    case DEFENSE_SPIDERS:
        return _("Fast-paced spider-fighting fun!");
    case DEFENSE_TRIFFIDS:
        return _("Defend your mansion against the triffids.");
    case DEFENSE_SKYNET:
        return _("The robots have decided that humans are the enemy!");
    case DEFENSE_LOVECRAFT:
        return _("Ward off legions of eldritch horrors.");
    default:
        return "What the heck is this I don't even know. (defense.cpp:defense_style_description)";
    }
}

std::string defense_location_name(defense_location location)
{
    switch (location) {
    case DEFLOC_NULL:
        return "Nowhere?! (bug in defense.cpp:defense_location_name)";
    case DEFLOC_HOSPITAL:
        return _("Hospital");
    case DEFLOC_WORKS:
        return _("Public Works");
    case DEFLOC_MALL:
        return _("Megastore");
    case DEFLOC_BAR:
        return _("Bar");
    case DEFLOC_MANSION:
        return _("Mansion");
    default:
        return "a ghost's house (bug in defense.cpp:defense_location_name)";
    }
}

std::string defense_location_description(defense_location location)
{
    switch (location) {
    case DEFLOC_NULL:
        return "NULL Bug. (defense.cpp:defense_location_description)";
    case DEFLOC_HOSPITAL:
        return                 _("One entrance and many rooms.  Some medical supplies.");
    case DEFLOC_WORKS:
        return                 _("An easy fortifiable building with lots of useful tools inside.");
    case DEFLOC_MALL:
        return                 _("A large building with various supplies.");
    case DEFLOC_BAR:
        return                 _("A small building with plenty of alcohol.");
    case DEFLOC_MANSION:
        return                 _("A large house with many rooms and.");
    default:
        return "Unknown data bug. (defense.cpp:defense_location_description)";
    }
}

void defense_game::caravan()
{
    std::vector<itype_id> items[NUM_CARAVAN_CATEGORIES];
    std::vector<int> item_count[NUM_CARAVAN_CATEGORIES];

    // Init the items for each category
    for (int i = 0; i < NUM_CARAVAN_CATEGORIES; i++) {
        items[i] = caravan_items( caravan_category(i) );
        for (int j = 0; j < items[i].size(); j++) {
            if (current_wave == 0 || !one_in(4)) {
                item_count[i].push_back(0);    // Init counts to 0 for each item
            } else { // Remove the item
                items[i].erase( items[i].begin() + j);
                j--;
            }
        }
    }

    int total_price = 0;

    WINDOW *w = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH, 0, 0);

    int offset = 0, item_selected = 0, category_selected = 0;

    int current_window = 0;

    draw_caravan_borders(w, current_window);
    draw_caravan_categories(w, category_selected, total_price, g->u.cash);

    bool done = false;
    bool cancel = false;
    while (!done) {

        char ch = input();
        switch (ch) {
        case '?':
            popup_top(_("\
CARAVAN:\n\
Start by selecting a category using your favorite up/down keys.\n\
Switch between category selection and item selecting by pressing Tab.\n\
Pick an item with the up/down keys, press + to buy 1 more, - to buy 1 less.\n\
Press Enter to buy everything in your cart, Esc to buy nothing."));
            draw_caravan_categories(w, category_selected, total_price, g->u.cash);
            draw_caravan_items(w, &(items[category_selected]),
                               &(item_count[category_selected]), offset, item_selected);
            draw_caravan_borders(w, current_window);
            break;

        case 'j':
            if (current_window == 0) { // Categories
                category_selected++;
                if (category_selected == NUM_CARAVAN_CATEGORIES) {
                    category_selected = CARAVAN_CART;
                }
                draw_caravan_categories(w, category_selected, total_price, g->u.cash);
                offset = 0;
                item_selected = 0;
                draw_caravan_items(w, &(items[category_selected]),
                                   &(item_count[category_selected]), offset,
                                   item_selected);
                draw_caravan_borders(w, current_window);
            } else if (items[category_selected].size() > 0) { // Items
                if (item_selected < items[category_selected].size() - 1) {
                    item_selected++;
                } else {
                    item_selected = 0;
                    offset = 0;
                }
                if (item_selected > offset + 22) {
                    offset++;
                }
                draw_caravan_items(w, &(items[category_selected]),
                                   &(item_count[category_selected]), offset,
                                   item_selected);
                draw_caravan_borders(w, current_window);
            }
            break;

        case 'k':
            if (current_window == 0) { // Categories
                if (category_selected == 0) {
                    category_selected = NUM_CARAVAN_CATEGORIES - 1;
                } else {
                    category_selected--;
                }
                if (category_selected == NUM_CARAVAN_CATEGORIES) {
                    category_selected = CARAVAN_CART;
                }
                draw_caravan_categories(w, category_selected, total_price, g->u.cash);
                offset = 0;
                item_selected = 0;
                draw_caravan_items(w, &(items[category_selected]),
                                   &(item_count[category_selected]), offset,
                                   item_selected);
                draw_caravan_borders(w, current_window);
            } else if (items[category_selected].size() > 0) { // Items
                if (item_selected > 0) {
                    item_selected--;
                } else {
                    item_selected = items[category_selected].size() - 1;
                    offset = item_selected - 22;
                    if (offset < 0) {
                        offset = 0;
                    }
                }
                if (item_selected < offset) {
                    offset--;
                }
                draw_caravan_items(w, &(items[category_selected]),
                                   &(item_count[category_selected]), offset,
                                   item_selected);
                draw_caravan_borders(w, current_window);
            }
            break;

        case '+':
        case 'l':
            if (current_window == 1 && items[category_selected].size() > 0) {
                item_count[category_selected][item_selected]++;
                itype_id tmp_itm = items[category_selected][item_selected];
                total_price += caravan_price(g->u, itypes[tmp_itm]->price);
                if (category_selected == CARAVAN_CART) { // Find the item in its category
                    for (int i = 1; i < NUM_CARAVAN_CATEGORIES; i++) {
                        for (unsigned j = 0; j < items[i].size(); j++) {
                            if (items[i][j] == tmp_itm) {
                                item_count[i][j]++;
                            }
                        }
                    }
                } else { // Add / increase the item in the shopping cart
                    bool found_item = false;
                    for (unsigned i = 0; i < items[0].size() && !found_item; i++) {
                        if (items[0][i] == tmp_itm) {
                            found_item = true;
                            item_count[0][i]++;
                        }
                    }
                    if (!found_item) {
                        items[0].push_back(items[category_selected][item_selected]);
                        item_count[0].push_back(1);
                    }
                }
                draw_caravan_categories(w, category_selected, total_price, g->u.cash);
                draw_caravan_items(w, &(items[category_selected]),
                                   &(item_count[category_selected]), offset, item_selected);
                draw_caravan_borders(w, current_window);
            }
            break;

        case '-':
        case 'h':
            if (current_window == 1 && items[category_selected].size() > 0 &&
                item_count[category_selected][item_selected] > 0) {
                item_count[category_selected][item_selected]--;
                itype_id tmp_itm = items[category_selected][item_selected];
                total_price -= caravan_price(g->u, itypes[tmp_itm]->price);
                if (category_selected == CARAVAN_CART) { // Find the item in its category
                    for (int i = 1; i < NUM_CARAVAN_CATEGORIES; i++) {
                        for (unsigned j = 0; j < items[i].size(); j++) {
                            if (items[i][j] == tmp_itm) {
                                item_count[i][j]--;
                            }
                        }
                    }
                } else { // Decrease / remove the item in the shopping cart
                    bool found_item = false;
                    for (unsigned i = 0; i < items[0].size() && !found_item; i++) {
                        if (items[0][i] == tmp_itm) {
                            found_item = true;
                            item_count[0][i]--;
                            if (item_count[0][i] == 0) {
                                item_count[0].erase(item_count[0].begin() + i);
                                items[0].erase(items[0].begin() + i);
                            }
                        }
                    }
                }
                draw_caravan_categories(w, category_selected, total_price, g->u.cash);
                draw_caravan_items(w, &(items[category_selected]),
                                   &(item_count[category_selected]), offset, item_selected);
                draw_caravan_borders(w, current_window);
            }
            break;

        case '\t':
            current_window = (current_window + 1) % 2;
            draw_caravan_borders(w, current_window);
            break;

        case KEY_ESCAPE:
            if (query_yn(_("Really buy nothing?"))) {
                cancel = true;
                done = true;
            } else {
                draw_caravan_categories(w, category_selected, total_price, g->u.cash);
                draw_caravan_items(w, &(items[category_selected]),
                                   &(item_count[category_selected]), offset, item_selected);
                draw_caravan_borders(w, current_window);
            }
            break;

        case '\n':
            if (total_price > g->u.cash) {
                popup(_("You can't afford those items!"));
            } else if ((items[0].empty() && query_yn(_("Really buy nothing?"))) ||
                       (!items[0].empty() &&
                        query_yn(_("Buy %d items, leaving you with $%d?"), items[0].size(),
                                 g->u.cash - total_price))) {
                done = true;
            }
            if (!done) { // We canceled, so redraw everything
                draw_caravan_categories(w, category_selected, total_price, g->u.cash);
                draw_caravan_items(w, &(items[category_selected]),
                                   &(item_count[category_selected]), offset, item_selected);
                draw_caravan_borders(w, current_window);
            }
            break;
        } // switch (ch)

    } // while (!done)

    if (!cancel) {
        g->u.cash -= total_price;
        bool dropped_some = false;
        for (unsigned i = 0; i < items[0].size(); i++) {
            item tmp(itypes[ items[0][i] ], g->turn);
            tmp = tmp.in_its_container(&(itypes));
            for (int j = 0; j < item_count[0][i]; j++) {
                if (g->u.can_pickVolume(tmp.volume()) && g->u.can_pickWeight(tmp.weight()) &&
                    g->u.inv.size() < inv_chars.size()) {
                    g->u.i_add(tmp);
                } else { // Could fit it in the inventory!
                    dropped_some = true;
                    g->m.add_item_or_charges(g->u.posx, g->u.posy, tmp);
                }
            }
        }
        if (dropped_some) {
            g->add_msg(_("You drop some items."));
        }
    }
}

std::string caravan_category_name(caravan_category cat)
{
    switch (cat) {
    case CARAVAN_CART:
        return _("Shopping Cart");
    case CARAVAN_MELEE:
        return _("Melee Weapons");
    case CARAVAN_GUNS:
        return _("Firearms & Ammo");
    case CARAVAN_COMPONENTS:
        return _("Crafting & Construction Components");
    case CARAVAN_FOOD:
        return _("Food & Drugs");
    case CARAVAN_CLOTHES:
        return _("Clothing & Armor");
    case CARAVAN_TOOLS:
        return _("Tools, Traps & Grenades");
    }
    return "BUG (defense.cpp:caravan_category_name)";
}

std::vector<itype_id> caravan_items(caravan_category cat)
{
    std::vector<itype_id> ret;
    switch (cat) {
    case CARAVAN_CART:
        return ret;

    case CARAVAN_MELEE:
        setvector(&ret,
                  "hammer", "bat", "mace", "morningstar", "hammer_sledge", "hatchet",
                  "knife_combat", "rapier", "machete", "katana", "spear_knife",
                  "pike", "chainsaw_off", NULL);
        break;

    case CARAVAN_GUNS:
        setvector(&ret,
                  "crossbow", "bolt_steel", "compbow", "arrow_cf", "marlin_9a",
                  "22_lr", "hk_mp5", "9mm", "taurus_38", "38_special", "deagle_44",
                  "44magnum", "m1911", "hk_ump45", "45_acp", "fn_p90", "57mm",
                  "remington_870", "shot_00", "shot_slug", "browning_blr", "3006",
                  "ak47", "762_m87", "m4a1", "556", "savage_111f", "hk_g3",
                  "762_51", "hk_g80", "12mm", "plasma_rifle", "plasma", NULL);
        break;

    case CARAVAN_COMPONENTS:
        setvector(&ret,
                  "rag", "fur", "leather", "superglue", "string_36", "chain",
                  "processor", "RAM", "power_supply", "motor", "hose", "pot",
                  "2x4", "battery", "nail", "gasoline", NULL);
        break;

    case CARAVAN_FOOD:
        setvector(&ret,
                  "1st_aid", "water", "energy_drink", "whiskey", "can_beans",
                  "mre_beef", "flour", "inhaler", "codeine", "oxycodone", "adderall",
                  "cig", "meth", "royal_jelly", "mutagen", "purifier", NULL);
        break;

    case CARAVAN_CLOTHES:
        setvector(&ret,
                  "backpack", "vest", "trenchcoat", "jacket_leather", "kevlar",
                  "gloves_fingerless", "mask_filter", "mask_gas", "glasses_eye",
                  "glasses_safety", "goggles_ski", "goggles_nv", "helmet_ball",
                  "helmet_riot", NULL);
        break;

    case CARAVAN_TOOLS:
        setvector(&ret,
                  "screwdriver", "wrench", "saw", "hacksaw", "lighter", "sewing_kit",
                  "scissors", "extinguisher", "flashlight", "hotplate",
                  "soldering_iron", "shovel", "jackhammer", "landmine", "teleporter",
                  "grenade", "flashbang", "EMPbomb", "smokebomb", "bot_manhack",
                  "bot_turret", "UPS_off", "mininuke", NULL);
        break;
    }

    return ret;
}

void draw_caravan_borders(WINDOW *w, int current_window)
{
    // First, do the borders for the category window
    nc_color col = c_ltgray;
    if (current_window == 0) {
        col = c_yellow;
    }

    mvwputch(w, 0, 0, col, LINE_OXXO);
    for (int i = 1; i <= 38; i++) {
        mvwputch(w,  0, i, col, LINE_OXOX);
        mvwputch(w, 11, i, col, LINE_OXOX);
    }
    for (int i = 1; i <= 10; i++) {
        mvwputch(w, i,  0, col, LINE_XOXO);
        mvwputch(w, i, 39, c_yellow, LINE_XOXO); // Shared border, always yellow
    }
    mvwputch(w, 11,  0, col, LINE_XXXO);

    // These are shared with the items window, and so are always "on"
    mvwputch(w,  0, 39, c_yellow, LINE_OXXX);
    mvwputch(w, 11, 39, c_yellow, LINE_XOXX);

    col = (current_window == 1 ? c_yellow : c_ltgray);
    // Next, draw the borders for the item description window--always "off" & gray
    for (int i = 12; i <= 23; i++) {
        mvwputch(w, i,  0, c_ltgray, LINE_XOXO);
        mvwputch(w, i, 39, col,      LINE_XOXO);
    }
    for (int i = 1; i <= 38; i++) {
        mvwputch(w, FULL_SCREEN_HEIGHT - 1, i, c_ltgray, LINE_OXOX);
    }

    mvwputch(w, FULL_SCREEN_HEIGHT - 1,  0, c_ltgray, LINE_XXOO);
    mvwputch(w, FULL_SCREEN_HEIGHT - 1, 39, c_ltgray, LINE_XXOX);

    // Finally, draw the item section borders
    for (int i = 40; i <= FULL_SCREEN_WIDTH - 2; i++) {
        mvwputch(w,  0, i, col, LINE_OXOX);
        mvwputch(w, FULL_SCREEN_HEIGHT - 1, i, col, LINE_OXOX);
    }
    for (int i = 1; i <= FULL_SCREEN_HEIGHT - 2; i++) {
        mvwputch(w, i, FULL_SCREEN_WIDTH - 1, col, LINE_XOXO);
    }

    mvwputch(w, FULL_SCREEN_HEIGHT - 1, 39, col, LINE_XXOX);
    mvwputch(w,  0, FULL_SCREEN_WIDTH - 1, col, LINE_OOXX);
    mvwputch(w, FULL_SCREEN_HEIGHT - 1, FULL_SCREEN_WIDTH - 1, col, LINE_XOOX);

    // Quick reminded about help.
    mvwprintz(w, FULL_SCREEN_HEIGHT - 1, 2, c_red, _("Press ? for help."));
    wrefresh(w);
}

void draw_caravan_categories(WINDOW *w, int category_selected, int total_price,
                             unsigned long cash)
{
    // Clear the window
    for (int i = 1; i <= 10; i++) {
        mvwprintz(w, i, 1, c_black, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
    }
    mvwprintz(w, 1, 1, c_white, _("Your Cash:%6d"), cash);
    wprintz(w, c_ltgray, " -> ");
    wprintz(w, (total_price > cash ? c_red : c_green), "%d", cash - total_price);

    for (int i = 0; i < NUM_CARAVAN_CATEGORIES; i++)
        mvwprintz(w, i + 3, 1, (i == category_selected ? h_white : c_white),
                  caravan_category_name( caravan_category(i) ).c_str());
    wrefresh(w);
}

void draw_caravan_items(WINDOW *w, std::vector<itype_id> *items,
                        std::vector<int> *counts, int offset,
                        int item_selected)
{
    // Print the item info first.  This is important, because it contains \n which
    // will corrupt the item list.

    // Actually, clear the item info first.
    for (int i = 12; i <= FULL_SCREEN_HEIGHT - 2; i++) {
        mvwprintz(w, i, 1, c_black, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
    }
    // THEN print it--if item_selected is valid
    if (item_selected < items->size()) {
        item tmp(itypes[ (*items)[item_selected] ], 0); // Dummy item to get info
        fold_and_print(w, 12, 1, 38, c_white, tmp.info().c_str());
    }
    // Next, clear the item list on the right
    for (int i = 1; i <= FULL_SCREEN_HEIGHT - 2; i++) {
        mvwprintz(w, i, 40, c_black, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
    }
    // Finally, print the item list on the right
    for (int i = offset; i <= offset + FULL_SCREEN_HEIGHT - 2 && i < items->size(); i++) {
        mvwprintz(w, i - offset + 1, 40, (item_selected == i ? h_white : c_white),
                  itypes[ (*items)[i] ]->name.c_str());
        wprintz(w, c_white, " x %2d", (*counts)[i]);
        if ((*counts)[i] > 0) {
            int price = caravan_price(g->u, itypes[(*items)[i]]->price * (*counts)[i]);
            wprintz(w, (price > g->u.cash ? c_red : c_green), "($%6d)", price);
        }
    }
    wrefresh(w);
}

int caravan_price(player &u, int price)
{
    if (u.skillLevel("barter") > 10) {
        return int( double(price) * .5);
    }
    return int( double(price) * (1.0 - double(u.skillLevel("barter")) * .05));
}

void defense_game::spawn_wave()
{
    g->add_msg("********");
    int diff = initial_difficulty + current_wave * wave_difficulty;
    bool themed_wave = one_in(SPECIAL_WAVE_CHANCE); // All a single monster type
    g->u.cash += cash_per_wave + (current_wave - 1) * cash_increase;
    std::vector<std::string> valid;
    valid = pick_monster_wave();
    while (diff > 0) {
        // Clear out any monsters that exceed our remaining difficulty
        for (int i = 0; i < valid.size(); i++) {
            if (GetMType(valid[i])->difficulty > diff) {
                valid.erase(valid.begin() + i);
                i--;
            }
        }
        if (valid.empty()) {
            g->add_msg(_("Welcome to Wave %d!"), current_wave);
            g->add_msg("********");
            return;
        }
        int rn = rng(0, valid.size() - 1);
        mtype *type = GetMType(valid[rn]);
        if (themed_wave) {
            int num = diff / type->difficulty;
            if (num >= SPECIAL_WAVE_MIN) {
                // TODO: Do we want a special message here?
                for (int i = 0; i < num; i++) {
                    spawn_wave_monster(type);
                }
                g->add_msg( special_wave_message(type->name).c_str() );
                g->add_msg("********");
                return;
            } else {
                themed_wave = false;    // No partially-themed waves
            }
        }
        diff -= type->difficulty;
        spawn_wave_monster(type);
    }
    g->add_msg(_("Welcome to Wave %d!"), current_wave);
    g->add_msg("********");
}

std::vector<std::string> defense_game::pick_monster_wave()
{
    (void)g; //unused
    std::vector<std::string> valid;
    std::vector<std::string> ret;

    if (zombies || specials) {
        if (specials) {
            valid.push_back("GROUP_ZOMBIE");
        } else {
            valid.push_back("GROUP_VANILLA");
        }
    }
    if (spiders) {
        valid.push_back("GROUP_SPIDER");
    }
    if (triffids) {
        valid.push_back("GROUP_TRIFFID");
    }
    if (robots) {
        valid.push_back("GROUP_ROBOT");
    }
    if (subspace) {
        valid.push_back("GROUP_NETHER");
    }

    if (valid.empty()) {
        debugmsg("Couldn't find a valid monster group for defense!");
    } else {
        ret = MonsterGroupManager::GetMonstersFromGroup(valid[rng(0, valid.size() - 1)]);
    }

    return ret;
}

void defense_game::spawn_wave_monster(mtype *type)
{
    monster tmp(type);
    if (location == DEFLOC_HOSPITAL || location == DEFLOC_MALL) {
        // Always spawn to the north!
        tmp.setpos(rng(SEEX * (MAPSIZE / 2), SEEX * (1 + MAPSIZE / 2)), SEEY);
    } else if (one_in(2)) {
        tmp.spawn(rng(SEEX * (MAPSIZE / 2), SEEX * (1 + MAPSIZE / 2)), rng(1, SEEY));
        if (one_in(2)) {
            tmp.setpos(tmp.posx(), SEEY * MAPSIZE - 1 - tmp.posy());
        }
    } else {
        tmp.spawn(rng(1, SEEX), rng(SEEY * (MAPSIZE / 2), SEEY * (1 + MAPSIZE / 2)));
        if (one_in(2)) {
            tmp.setpos(SEEX * MAPSIZE - 1 - tmp.posx(), tmp.posy());
        }
    }
    tmp.wandx = g->u.posx;
    tmp.wandy = g->u.posy;
    tmp.wandf = 150;
    // We wanna kill!
    tmp.anger = 100;
    tmp.morale = 100;
    g->add_zombie(tmp);
}

std::string defense_game::special_wave_message(std::string name)
{
    std::stringstream ret;
    ret << string_format(_("Wave %d: "), current_wave);

    // Capitalize
    capitalize_letter(name);
    for (unsigned i = 2; i < name.size(); i++) {
        if (name[i - 1] == ' ') {
            capitalize_letter(name, i);
        }
    }

    switch (rng(1, 6)) {
    case 1:
        ret << string_format(_("%s Invasion!"), name.c_str());
        break;
    case 2:
        ret << string_format(_("Attack of the %ss!"), name.c_str());
        break;
    case 3:
        ret << string_format(_("%s Attack!"), name.c_str());
        break;
    case 4:
        ret << string_format(_("%s from Hell!"), name.c_str());
        break;
    case 5:
        ret << string_format(_("Beware! %s!"), name.c_str());
        break;
    case 6:
        ret << string_format(_("The Day of the %s!"), name.c_str());
        break;
    case 7:
        ret << string_format(_("%s Party!"), name.c_str());
        break;
    case 8:
        ret << string_format(_("Revenge of the %ss!"), name.c_str());
        break;
    case 9:
        ret << string_format(_("Rise of the %ss!"), name.c_str());
        break;
    }

    return ret.str();
}
