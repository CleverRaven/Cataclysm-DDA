#include "help.h"

#include "action.h"
#include "auto_pickup.h"
#include "catacharset.h"
#include "keypress.h"
#include "options.h"
#include "output.h"
#include "rng.h"
#include "translations.h"
#include <cmath>  // max

std::vector<std::string> hints;

//ready, checked
void help_main(WINDOW *win)
{
    werase(win);
    int y = fold_and_print(win, 1, 1, getmaxx(win) - 2, c_white, _("\
Please press one of the following for help on that topic:\n\
Press q or ESC to return to the game.")) + 2;

    std::vector<std::string> headers;
    headers.push_back(_("a: Introduction"));
    headers.push_back(_("b: Movement"));
    headers.push_back(_("c: Viewing"));
    headers.push_back(_("d: Hunger, Thirst, and Sleep"));
    headers.push_back(_("e: Pain and Stimulants"));
    headers.push_back(_("f: Addiction"));
    headers.push_back(_("g: Morale and Learning"));
    headers.push_back(_("h: Radioactivity and Mutation"));
    headers.push_back(_("i: Bionics"));
    headers.push_back(_("j: Crafting"));
    headers.push_back(_("k: Traps"));
    headers.push_back(_("l: Items overview"));
    headers.push_back(_("m: Combat"));
    headers.push_back(_("n: Unarmed Styles"));
    headers.push_back(_("o: Survival tips"));
    headers.push_back(_("p: Driving"));

    int half_size = headers.size() / 2;
    int second_column = getmaxx(win) / 2;
    for (size_t i = 0; i < headers.size(); i++) {
        if (i < half_size) {
            second_column = std::max(second_column, utf8_width(headers[i].c_str()) + 4);
        }
        mvwprintz(win, y + i % half_size, (i < half_size ? 1 : second_column),
                  c_white, headers[i].c_str());
    }

    headers.clear();
    headers.push_back(_("1: List of all commands (you can change key commands here)"));
    headers.push_back(_("2: List of all options  (you can change options here)"));
    headers.push_back(_("3: Auto pickup manager  (you can change pickup rules here)"));
    headers.push_back(_("4: List of item types and data"));
    headers.push_back(_("5: Description of map symbols"));
    headers.push_back(_("6: Description of gun types"));
    headers.push_back(_("7: Frequently Asked Questions (Some spoilers!)"));
    headers.push_back(_(" "));
    headers.push_back(_("q: Return to game"));

    y += half_size + 1;
    for (size_t i = 0; i < headers.size(); i++) {
        y += fold_and_print(win, y, 1, getmaxx(win) - 2, c_white, headers[i].c_str());
    }
    wrefresh(win);
}

//fix-it
void help_movement(WINDOW *win)
{
    werase(win);
    action_id movearray[] = {ACTION_MOVE_NW, ACTION_MOVE_N, ACTION_MOVE_NE,
                             ACTION_MOVE_W,  ACTION_PAUSE,  ACTION_MOVE_E,
                             ACTION_MOVE_SW, ACTION_MOVE_S, ACTION_MOVE_SE};
    std::vector<std::string> text;
    int pos_y = fold_and_print(win, 0, 1, getmaxx(win) - 2, c_white,
                               _("Movement is performed using the numpad, or vikeys.")) + 1;

    mvwprintz(win, pos_y + 1, 0, c_white, _("\
  \\ | /     \\ | /\n\
   \\|/       \\|/ \n\
  -- --     -- --  \n\
   /|\\       /|\\ \n\
  / | \\     / | \\"));
    for(int acty = 0; acty < 3; acty++) {
        for(int actx = 0; actx < 3; actx++) {
            std::vector<char> keys = keys_bound_to( movearray[acty * 3 + actx] );
            if (!keys.empty()) {
                mvwputch(win, acty * 3 + pos_y, actx * 3 + 1, c_ltblue, keys[0]);
                if (keys.size() > 0) {
                    mvwputch(win, acty * 3 + pos_y, actx * 3 + 11, c_ltblue, keys[1]);
                }
            }
        }
    }

    text.push_back(string_format(_("\
Each step will take 100 movement points (or more, depending on the terrain); \
you will then replenish a variable amount of movement points, depending on many factors \
(%s to see the exact amount)."), press_x(ACTION_PL_INFO, _("press "), _("'View Player Info'")).c_str()));

    text.push_back(_("To attempt to hit a monster with your weapon, simply move into it."));

    text.push_back(string_format(_("You may find doors, ('+'); these may be opened %s or closed %s. \
Some doors are locked. Locked doors, windows, and some other obstacles can be destroyed by smashing \
them (%sthen choose a direction). Smashing down obstacles is much easier with a good weapon or a \
strong character."), press_x(ACTION_OPEN, _("with "), "").c_str(),
                     press_x(ACTION_CLOSE, _("with "), "").c_str(),
                     press_x(ACTION_SMASH, "", ", ", "").c_str()));

    text.push_back(string_format(_("There may be times when you want to move more quickly by \
holding down a movement key. However, fast movement in this fashion may lead to the player \
getting into a dangerous situation or even killed before they have a chance to react. %s will \
toggle \"Run Mode\". While this is on, any movement will be ignored if new monsters enter the \
player's view."), press_x(ACTION_TOGGLE_SAFEMODE, _("Pressing "), _("'Toggle Safemode'") ).c_str()));

    int fig_last_line = pos_y + 8;
    for (size_t i = 0; i < text.size(); i++) {
        int pos_x = (pos_y < fig_last_line) ? 20 : 1;
        pos_y += fold_and_print(win, pos_y, pos_x, getmaxx(win) - pos_x - 2, c_white,
                                text[i].c_str()) + 1;
    }
    wrefresh(win);
    refresh();
    getch();
}

//fix-it
void help_driving(WINDOW *win)
{
    action_id movearray[] = {ACTION_MOVE_NW, ACTION_MOVE_N, ACTION_MOVE_NE,
                             ACTION_MOVE_W,  ACTION_PAUSE,  ACTION_MOVE_E,
                             ACTION_MOVE_SW, ACTION_MOVE_S, ACTION_MOVE_SE};
    werase(win);
    mvwprintz(win, 0, 0, c_white, _("\
You control vehicles using the numpad, or vikeys. However, you control their\n\
                    controls, rather than the vehicle directly. In order to\n\
                    assume control of the vehicle, get to a location with\n\
 \\ | /     \\ | /    \"vehicle controls\" and %s. Once you are in control,\n\
  \\|/       \\|/     %s accelerates, %s slows or reverses, &\n\
 -- --     -- --      %s & %s turn left or right.\n\
  /|\\       /|\\     Diagonals adjust course and speed. You default to\n\
 / | \\     / | \\    cruise control, so the gas/brake adjust the speed\n\
                    which the vehicle will attempt to maintain. 10-30 MPH,\n\
                    or 16-48 KPH, is a good speed for beginning drivers,\n\
who tend to fumble the controls. As your Driving skill improves, you will\n\
fumble less and less. To simply maintain course and speed, hit %s.\n\
\n\
It's a good idea to pull the handbrake-\"s\"-when parking, just to be safe.\n\
If you want to get out, hit the lights, toggle cruise control, turn the\n\
engine on or off, or otherwise use the vehicle controls, %s to bring up\n\
the \"Vehicle Controls\" menu, which has options for things you'd do\n\
from the driver's seat."),
            press_x(ACTION_CONTROL_VEHICLE, _("press "), "'Vehicle Controls'").c_str(),
            press_x(ACTION_MOVE_N, _(" "), "'move_n'").c_str(),
            press_x(ACTION_MOVE_S, _(" "), "'move_s'").c_str(),
            press_x(ACTION_MOVE_W, _(" "), "'move_w'").c_str(),
            press_x(ACTION_MOVE_E, _(" "), "'move_e'").c_str(),
            press_x(ACTION_PAUSE, _(" "), "'pause'").c_str(),
            press_x(ACTION_CONTROL_VEHICLE, _("press "), "'Vehicle Controls'").c_str());
            for(int acty = 0; acty < 3; acty++) {
                for(int actx = 0; actx < 3; actx++) {
                    std::vector<char> keys = keys_bound_to( movearray[acty*3+actx] );
                    if (!keys.empty()) {
                        mvwputch(win, (acty * 3 + 2), (actx * 3), c_ltblue, keys[0]);
                        if (keys.size() > 0) {
                            mvwputch(win, (acty * 3 + 2), (actx * 3 + 10), c_ltblue, keys[1]);
                        }
                    }
                }
            }
    wrefresh(win);
    refresh();
    getch();
}

//ready, checked
std::vector<std::string> text_introduction()
{
    std::vector<std::string> text;

    text.push_back(_("\
Cataclysm is a roguelike with a monster apocalypse setting. \
You have survived the original onslaught, and are ready to set out in search of safety."));

    text.push_back(_("\
Cataclysm differs from most roguelikes in several ways. Rather than exploring \
an underground dungeon, with a limited area on each level, you are exploring \
a truly infinite world, stretching in all four cardinal directions. As in most roguelikes, \
you will have to find food; you also need to keep yourself hydrated, and sleep periodically."));

    text.push_back(_("\
While Cataclysm has more challenges than many roguelikes, the near-future \
setting makes some tasks easier. Firearms, medications, and a wide variety \
of tools are all available to help you survive."));

    return text;
}

//ready, checked
std::vector<std::string> text_viewing()
{
    std::vector<std::string> text;

    text.push_back(string_format(_("\
The player can often see more than can be displayed on the screen at a time. Pressing %s enters \
look around mode, which allows you to scroll around using the movement keys and view items on \
the map. Pressing %s provides a list of nearby visible items, though items shut away in crates, \
cupboards, refrigerators and the like will not be displayed. Pressing Shift+vikeys will scroll \
the view persistently, allowing you to keep an eye on things as you move around."),
                                 press_x(ACTION_LOOK, "", "").c_str(),
                                 press_x(ACTION_LIST_ITEMS, "", "").c_str()));
    return text;
}

//fix-it
std::vector<std::string> text_hunger()
{
    std::vector<std::string> text;

    text.push_back(string_format(_("\
As time passes, you will begin to feel hunger and thirst. A status warning at the bottom of the \
screen will appear. As hunger and thirst reach critical levels, you will begin to suffer movement \
penalties. Thirst is more dangerous than hunger. Finding food in a city is usually easy; outside \
of a city, you may have to hunt an animal, then stand over its corpse and %s it into small chunks \
of meat. Likewise, outside of a city you may have to drink water from a river or other natural \
source; stand in shallow water and press %s to pick it up. You'll need a watertight container. \
Be forewarned that some sources of water aren't trustworthy and may produce diseased water. \
To be sure it's healthy, run all water you collect through a water filter before drinking."),
                                 press_x(ACTION_BUTCHER, _("butcher")).c_str(),
                                 from_sentence_case(press_x(ACTION_PICKUP, "", "")).c_str()));

    text.push_back(string_format(_("\
Every 14 to 20 hours, you'll find yourself growing sleepy. If you do not sleep by pressing %s, \
you'll start suffering stat and movement penalties. You may not always fall asleep right away. \
Sleeping indoors, especially on a bed, will help; or you can always use sleeping pills. \
While sleeping, you'll slowly replenish lost hit points. You'll also be vulnerable to attack, \
so try to find a safe place, or set traps for unwary intruders."),
                                 press_x(ACTION_SLEEP, "", "").c_str()));

    return text;
}

//ready, checked
std::vector<std::string> text_pain()
{
    std::vector<std::string> text;

    text.push_back(_("\
When you take damage from almost any source, you'll start to feel pain. Pain slows you down and \
reduces your stats, and finding a way to manage pain is an early imperative. The most common is \
drugs; aspirin, codeine, tramadol, oxycodone, and more are all great options. Be aware that while \
under the influence of a lot of painkillers, the physiological effects may slow you or \
reduce your stats."));

    text.push_back(_("\
Note that most painkillers take a little while to kick in. If you take some oxycodone, and don't \
notice the effects right away, don't start taking more; you may overdose and die!"));

    text.push_back(_("\
Pain will also disappear with time, so if drugs aren't available and you're in a lot of pain, it \
may be wise to find a safe spot and simply rest for an extended period of time."));

    text.push_back(_("\
Another common class of drugs is stimulants. Stimulants provide you with a temporary rush of \
energy, increasing your movement speed and many stats, most notably intelligence, making them \
useful study aids. There are two drawbacks to stimulants; they make it more difficult to sleep, \
and, more importantly, most are highly addictive. Stimulants range from the caffeine rush of \
cola to the more intense high of Adderall and methamphetamine."));

    return text;
}

//ready, checked
std::vector<std::string> text_addiction()
{
    std::vector<std::string> text;

    text.push_back(_("\
Many drugs have a potential for addiction. Each time you consume such a drug there is a chance \
that you will grow dependent on it. Consuming more of that drug will increase your dependance. \
Effects vary greatly between drugs, but all addictions have only one cure; going cold turkey. \
The process may last days, and will leave you very weak, so try to do it in a safe area."));

    text.push_back(_("\
If you are suffering from drug withdrawal, taking more of the drug will cause \
the effects to cease immediately, but may deepen your dependance."));
    
    return text;
}

//ready, checked
std::vector<std::string> text_morale()
{
    std::vector<std::string> text;

    text.push_back(_("\
Your character has a morale level, which affects you in many ways. The depressing post-apocalypse \
world is tough to deal with, and your mood will naturally decrease very slowly."));

    text.push_back(_("\
There are lots of options for increasing morale; reading an entertaining book, eating \
delicious food, and taking recreational drugs are but a few options. Most morale-boosting \
activities can only take you to a certain level before they grow boring."));

    text.push_back(_("\
There are also lots of ways for your morale to decrease, beyond its natural \
decay. Eating disgusting food, reading a boring technical book, or going \
through drug withdrawal are some prominent examples."));

    text.push_back(_("\
Low morale will make you sluggish and unmotivated. It will also reduce your stats, particularly \
intelligence. If your morale drops very low, you may even commit suicide. Very high morale fills \
you with gusto and energy, and you will find yourself moving faster. At extremely high levels, \
you will receive stat bonuses."));

    text.push_back(_("\
Morale is also responsible for ensuring you can learn effectively, via a mechanic referred \
to as 'focus'. Your focus level is a measure of how effectively you can learn. The natural \
level is 100, which indicates normal learning potential. Higher or lower focus levels make \
it easier or harder to learn from practical experience."));

    text.push_back(_("\
Your focus level has a natural set point that it converges towards. \
When your focus is much lower - or higher - than this set point, it will change faster \
than when it is near the set point. Having high morale will raise the set point, \
and having low morale will lower the set point. Pain is also factored \
into the set point calculation - it's harder to learn when you're in pain."));

    text.push_back(_("\
Your focus is also lowered by certain activities. Training your skills through real-world practice \
lowers your focus gradually, by an amount that depends on your current level of focus (higher \
focus means larger decreases, as well as improved learning). Training your skills by reading books \
decreases your focus rapidly, by giving a significant penalty to the set point of your focus."));

    return text;
}

//ready, checked
std::vector<std::string> text_mutation()
{
    std::vector<std::string> text;

    text.push_back(_("\
Though it is relatively rare, certain areas of the world may be contaminated \
with radiation. It will gradually accumulate in your body, weakening you \
more and more. While in radiation-free areas, your radiation level will \
slowly decrease. Taking iodine tablets will help speed the process."));

    text.push_back(_("\
If you become very irradiated, you may develop mutations. Most of the time, these \
mutations will be negative; however, many are beneficial, and others have both positive \
and negative effects. Your mutations may change your play style considerably. It is possible \
to find substances that will remove mutations, though these are extremely rare."));
    
    return text;
}

//ready, checked
std::vector<std::string> text_bionics()
{
    std::vector<std::string> text;

    text.push_back(_("\
Bionics are biomechanical upgrades to your body. \
While many are simply 'built-in' versions of items you would otherwise have to carry, \
some bionics have unique effects that are otherwise unobtainable."));

    text.push_back(_("\
Most bionics require a source of power, and you will need an internal battery \
to store energy for them. Your current amount of energy is displayed below \
your health. Replenishing energy can be done in a variety of ways, but all \
require the installation of a special bionic just for fuel consumption."));

    text.push_back(_("\
Bionics come in ready-to-install canisters. Installation of a bionic is best left to a trained \
professional. However, you may attempt to perform a self-installation. Performing such a task \
requires high levels of intelligence, first aid, mechanics, and/or electronics, and failure may \
cripple you! Many bionics canisters are difficult to find, but may be purchased from certain \
wandering vagabonds for a very high price."));
    
    return text;
}

//ready, checked
std::vector<std::string> text_crafting()
{
    std::vector<std::string> text;

    text.push_back(_("\
Many important items can be very hard to find, or will cost a great deal of \
money to trade for. Fortunately, it is possible to craft a wide variety of \
goods with the proper tools, materials, and training."));

    text.push_back(_("\
Some recipes require a set of tools. These are not used up when crafting, so you can keep your \
tool set. All recipes require one or more ingredients. These ARE used up in crafting."));

    text.push_back(string_format(_("\
To craft items, press %s. There are five categories: \
Weapons, Food, Electronics, Armor, and Miscellaneous. While a few items require \
no skill to create, the majority require you to have some knowledge:\n"),
                                 press_x(ACTION_CRAFT, "", "").c_str()));

    text.push_back(_("\
-> Mechanic skill is used for weapons, traps, and a few tools.\n\
-> Cooking skill, at low levels, is used for making tasty recipes; at higher levels, you have \
an understanding of chemistry and can make chemical weapons and beneficial elixirs.\n\
-> Tailoring skill is used to create basic clothing, and later tough armor.\n\
-> Electronics skill lets you make a wide variety of tools with intricate uses."));

    text.push_back(_("\
In addition to the primary crafting skills, other skills may be necessary to create certain \
items. Traps skill, Marksmanship skill, and First Aid skill are all required for certain items."));

    return text;
}

//ready, checked
std::vector<std::string> text_traps()
{
    std::vector<std::string> text;

    text.push_back(_("\
While sleeping in dangerous territory, it may be wise to set traps to ward off \
unwanted intrusions. There are a few traps to be found in the world, most notably \
bubble wrap (which will make a loud noise if stepped on, helping to wake you up) \
and bear traps (which make noise AND damage and trap anything that steps on them). \
Others need to be crafted; this requires the Traps skill and possibly Mechanics."));

    text.push_back(string_format(_("\
To set a trap, simply use the item (%s) and choose a direction; the trap will be \
placed on an adjacent tile. Some traps may require additional tools, like a shovel, \
to be set. Once set, a trap will remain in place until stepped on or disarmed."),
                                 press_x(ACTION_USE, "", "").c_str()));

    text.push_back(string_format(_("\
To disarm a trap, examine (%s) the space it is on. Your success depends \
upon your Traps skill and Dexterity. If you succeed, the trap is removed \
and replaced by some or all of its constituent parts; however, if you fail, \
there is a chance that you will set off the trap, suffering the consequences."),
                                 press_x(ACTION_EXAMINE, "", "").c_str()));

    text.push_back(_("\
Many traps are fully or partially hidden. Your ability to detect traps is \
entirely dependent upon your Perception. Should you stumble into a trap, \
you may have a chance to avoid it, depending on your Dodge skill."));

    return text;
}

//fix-it
std::vector<std::string> text_items()
{
    std::vector<std::string> text;

    text.push_back(string_format(_("\
There are a wide variety of items available for your use. You may find them \
lying on the ground; if so, simply press %s to pick up items on the \
same square. Some items are found inside a container, drawn as a { with a \
blue background. Pressing %s, then a direction, will allow you to examine \
these containers and loot their contents."),
                    from_sentence_case(press_x(ACTION_PICKUP, "", "")).c_str(),
                    press_x(ACTION_EXAMINE, "", "").c_str()));

    text.push_back(string_format(_("\
Pressing %s opens a comparison menu, where you can see two items \
side-by-side with their attributes highlighted to indicate which is superior. \
You can also access the item comparison menu by pressing 'C' when the %s \
nearby items menu is open and an item is selected."),
            press_x(ACTION_COMPARE, "", "").c_str(),
            press_x(ACTION_LIST_ITEMS, _("view")).c_str())); // F*ck U, coder!

    text.push_back(string_format(_("\
All items may be used as a melee weapon, though some are better than others. You can check the \
melee attributes of an item you're carrying by hitting %s to enter your inventory, then pressing \
the letter of the item. There are 3 melee values, bashing, cutting, and to-hit bonus (or \
penalty). Bashing damage is universally effective, but is capped by low strength. Cutting damage \
is a guaranteed increase in damage, but it may be reduced by a monster's natural armor."),
                                 press_x(ACTION_INVENTORY, "", "").c_str()));

    text.push_back(string_format(_("\
To wield an item as a weapon, %s then the proper letter. Pressing '-' in lieu of a letter will \
make you wield nothing. A wielded weapon will not contribute to your volume carried, so holding \
a large item in your hands may be a good option for travel. When unwielding your weapon, it will \
go back in your inventory, or will be dropped on the ground if there is no space."),
                                 from_sentence_case(press_x(ACTION_WIELD)).c_str()));

    text.push_back(string_format(_("\
To wear a piece of clothing, %s then the proper letter. Armor reduces damage and helps you \
resist things like smoke. To take off an item, %s then the proper letter."),
                                 from_sentence_case(press_x(ACTION_WEAR)).c_str(),
                                 from_sentence_case(press_x(ACTION_TAKE_OFF)).c_str()));

    text.push_back(string_format(_("\
Also available in the %s nearby items menu is the ability to filter or prioritize items. \
You can enter item names, or various advanced filter strings: {<token>:<search param>}"),
                                 press_x(ACTION_LIST_ITEMS, _("view")).c_str()));

    text.push_back(_("\
Currently Available tokens:\n\
\t c = category (books, food, etc)    | {c:books}\n\
\t m = material (cotton, kevlar, etc) | {m:iron}\n\
\t dgt = damage greater than (0-5)    | {dgt:2}\n\
\t dlt = damage less than (0-5)       | {dlt:1}"));

    return text;
}

//fix-it
std::vector<std::string> text_combat()
{
    std::vector<std::string> text;

    text.push_back(_("\
After 30 minutes of warmup time, monsters will begin to appear. They are \
represented by letters on your screen; a list of monster names, and their \
positions relative to you, is displayed on the right side of the screen."));

    text.push_back(_("\
To attack a monster with a melee weapon, simply move into them. The time it takes to attack \
depends on the size and weight of your weapon. Small, light weapons are the fastest; unarmed \
attacks increase in speed with your Unarmed Combat skill, and will eventually be VERY fast. \
A successful hit with a bashing weapon may stun the monster temporarily, while cutting weapons \
may get stuck, possibly pulling the weapon from your hands-- but a monster with a weapon stuck \
in it will move much more slowly. A miss may make you stumble and lose movement points. If a \
monster hits you, your clothing may absorb some damage, but you will absorb the excess."));

    text.push_back(string_format(_("\
Swarms of monsters may call for firearms. If you find one, wield it first, then reload%s. \
If you wish to change ammo, you must unload the weapon%s, then reload again. \
To fire%s, move the cursor to the relevant space, then hit '.' or 'f'. \
Some guns have alternate firing modes, such as burst fire; to cycle modes%s. \
Firing continuously, especially in bursts, will severely reduce accuracy."),
            press_x(ACTION_RELOAD, _(" by pressing "), "").c_str(),
            press_x(ACTION_UNLOAD, _(" by pressing "), "").c_str(),
            press_x(ACTION_FIRE, _(", press "), "").c_str(),
            press_x(ACTION_SELECT_FIRE_MODE, _(", press "),
                    _(" 'Toggle attack mode of Wielded Item'")).c_str()));

    text.push_back(_("\
Unlike most roguelikes, fleeing will often be your best option, especially when \
overwhelmed by a swarm of zombies. Try to avoid getting cornered inside a building. \
Ducking down into the subways or sewers is often an excellent escape tactic."));

    return text;
}

//ready, checked
std::vector<std::string> text_styles()
{
    std::vector<std::string> text;

    text.push_back(_("\
For the unarmed fighter, a variety of fighting styles are available. \
You can start with your choice of a single, commonly-taught style by starting with \
the Martial Arts Training trait. Many, many more can be taught by wandering masters."));

    text.push_back(string_format(_("\
To select a fighting style, press %s. If you are already unarmed this will make you start using the style. \
Otherwise, it will be locked in as your default unarmed style; to start using it, press %s then '-'."),
                                 press_x(ACTION_PICK_STYLE, "", "").c_str(),
                                 press_x(ACTION_WIELD, "", "").c_str()));

    text.push_back(string_format(_("\
Most styles have a variety of special moves associated with them. Most have a skill requirement, \
and will be unavailable until you reach a level of unarmed skill. You can check the moves by \
examining your style via the inventory screen (%s key)."),
                                 press_x(ACTION_INVENTORY, "", "").c_str()));

    text.push_back(_("\
Many styles also have special effects unlocked under certain conditions. \
These are varied and unique to each style, and range from special combo moves to bonuses \
depending on the situation you are in. You can check these by examining your style."));

    return text;
}

//ready, checked
std::vector<std::string> text_tips()
{
    std::vector<std::string> text;

    text.push_back(_("\
The first thing to do is to check your home for useful items. Your initial storage is \
limited, and a backpack, trenchcoat, or other storage medium will let you carry a lot \
more. Finding a weapon is important; frying pans, butcher knives, and more are common \
in your home; hardware stores may carry others. Unless you plan on concentrating on melee \
combat, seek out gun stores as soon as possible and load up on more than one type."));

    text.push_back(_("\
It's also important to carry a few medications; painkillers are a must-have, and drugs such \
as cigarettes will make life easier (but beware addiction). Leave cities as soon as you \
have a good stockpile of equipment. Their high concentration of zombies makes them a \
deathtrap--but a necessary resource for food and ammunition."));

    text.push_back(_("\
Combat is much easier if you can fight just one monster at once. Use doorways as a choke point, \
or stand behind a window and strike as the zombies slowly climb through. Never be afraid to just \
run if you can outpace your enemies. Irregular terrain, like forests, may help you lose monsters."));

    text.push_back(_("\
Firearms are the easiest way to kill an enemy, but the sound will attract \
unwanted attention. Save the guns for emergencies, and melee when you can."));

    text.push_back(_("\
Try to keep your inventory as full as possible without being overloaded. You never know when you \
might need an item, most are good to sell, and you can easily drop unwanted items on the floor."));

    text.push_back(_("\
Keep an eye on the weather. At night, sleeping might be difficult if \
you don't have a warm place to rest your head. Be sure to protect your \
extremities from frostbite and to keep your distance from large fires."));

    return text;
}

//ready, checked
std::vector<std::string> text_types()
{
    std::vector<std::string> text;

    text.push_back(string_format(_("\
~       Liquid\n\
%%%%       Food\n\
!       Medication\n\
These are all consumed by using %s. They provide a certain amount of nutrition, quench your thirst, may \
be a stimulant or a depressant, and may provide morale. There may also be more subtle effects."),
                                 press_x(ACTION_EAT, "", "").c_str()));

    text.push_back(string_format(_("\
/       Large weapon\n\
;       Small weapon or tool\n\
,       Tiny item\n\
These are all generic items, useful only to be wielded as a weapon. However, some have special \
uses; they will show up under the TOOLS section in your inventory. Press %s to use these."),
                                 press_x(ACTION_USE, "", "").c_str()));

    text.push_back(string_format(_("\
)       Container\n\
These items may hold other items. Some are passable weapons. Many will be listed with their \
contents, e.g. \"plastic bottle of water\". Those containing comestibles may be eaten with %s; \
this may leave an empty container."),
                                 press_x(ACTION_EAT, "", "").c_str()));

    text.push_back(string_format(_("\
[       Clothing\n\
This may be worn with the %s key or removed with the %s key. It may cover one or more body parts; \
you can wear multiple articles of clothing on any given body part, but this will encumber you \
severely. Each article of clothing may provide storage space, warmth, an encumberment, and a \
resistance to bashing and/or cutting attacks. Some may protect against environmental effects."),
                                press_x(ACTION_WEAR, "", "").c_str(),
                                press_x(ACTION_TAKE_OFF, "","").c_str()));

    text.push_back(string_format(_("\
(       Firearm\n\
This weapon may be loaded with ammunition with %s, unloaded with %s, and fired with %s. \
Some have automatic fire, which may be used with %s at a penalty to accuracy. \
The color refers to the type; handguns are gray, shotguns are red, submachine guns are cyan, \
rifles are brown, assault rifles are blue, and heavy machine guns are light red. Each has \
a dispersion rating, a bonus to damage, a rate of fire, and a maximum load. Note that most \
firearms load fully in one action, while shotguns must be loaded one shell at a time."),
                                press_x(ACTION_RELOAD, "", "").c_str(),
                                press_x(ACTION_UNLOAD, "", "").c_str(),
                                press_x(ACTION_FIRE, "", "").c_str(),
                                press_x(ACTION_SELECT_FIRE_MODE, "", "").c_str()));

    text.push_back(_("\
=       Ammunition\n\
Ammunition is worthless without a gun to load it into. Generally, \
there are several variants for any particular calibre. Ammunition has \
damage, dispersion, and range ratings, and an armor-piercing quality."));

    text.push_back(string_format(_("\
*       Thrown weapon; simple projectile or grenade\n\
These items are suited for throwing, and many are only useful when thrown, \
such as grenades, molotov cocktails, or tear gas. Once activated be certain \
to throw these items by pressing %s, then the letter of the item to throw."),
                                press_x(ACTION_THROW, "", "").c_str()));

    text.push_back(string_format(_("\
?       Book or magazine\n\
This can be read for training or entertainment by pressing %s. Most require a basic \
level of intelligence; some require some base knowledge in the relevant subject."),
                                press_x(ACTION_READ, "", "").c_str()));

    return text;
}

//pristine
void help_map(WINDOW *win)
{
    werase(win);
    mvwprintz(win, 0, 0, c_ltgray,  _("MAP SYMBOLS:"));
    mvwprintz(win, 1, 0, c_brown,   _("\
.           Field - Empty grassland, occasional wild fruit."));
    mvwprintz(win, 2, 0, c_green,   _("\
F           Forest - May be dense or sparse. Slow moving; foragable food."));
    mvwputch(win,  3,  0, c_dkgray, LINE_XOXO);
    mvwputch(win,  3,  1, c_dkgray, LINE_OXOX);
    mvwputch(win,  3,  2, c_dkgray, LINE_XXOO);
    mvwputch(win,  3,  3, c_dkgray, LINE_OXXO);
    mvwputch(win,  3,  4, c_dkgray, LINE_OOXX);
    mvwputch(win,  3,  5, c_dkgray, LINE_XOOX);
    mvwputch(win,  3,  6, c_dkgray, LINE_XXXO);
    mvwputch(win,  3,  7, c_dkgray, LINE_XXOX);
    mvwputch(win,  3,  8, c_dkgray, LINE_XOXX);
    mvwputch(win,  3,  9, c_dkgray, LINE_OXXX);
    mvwputch(win,  3, 10, c_dkgray, LINE_XXXX);

    mvwprintz(win,  3, 12, c_dkgray,  _("\
Road - Safe from burrowing animals."));
    mvwprintz(win,  4, 0, c_dkgray,  _("\
H=          Highway - Like roads, but lined with guard rails."));
    mvwprintz(win,  5, 0, c_dkgray,  _("\
|-          Bridge - Helps you cross rivers."));
    mvwprintz(win,  6, 0, c_blue,    _("\
R           River - Most creatures can not swim across them, but you may."));
    mvwprintz(win,  7, 0, c_dkgray,  _("\
O           Parking lot - Empty lot, few items. Mostly useless."));
    mvwprintz(win,  8, 0, c_ltgreen, _("\
^>v<        House - Filled with a variety of items. Good place to sleep."));
    mvwprintz(win,  9, 0, c_ltblue,  _("\
^>v<        Gas station - Good place to collect gasoline. Risk of explosion."));
    mvwprintz(win, 10, 0, c_ltred,   _("\
^>v<        Pharmacy - The best source for vital medications."));
    mvwprintz(win, 11, 0, c_green,   _("\
^>v<        Grocery store - Good source of canned food and other supplies."));
    mvwprintz(win, 12, 0, c_cyan,    _("\
^>v<        Hardware store - Home to tools, melee weapons and crafting goods."));
    mvwprintz(win, 13, 0, c_ltcyan,  _("\
^>v<        Sporting Goods store - Several survival tools and melee weapons."));
    mvwprintz(win, 14, 0, c_magenta, _("\
^>v<        Liquor store - Alcohol is good for crafting molotov cocktails."));
    mvwprintz(win, 15, 0, c_red,     _("\
^>v<        Gun store - Firearms and ammunition are very valuable."));
    mvwprintz(win, 16, 0, c_blue,    _("\
^>v<        Clothing store - High-capacity clothing, some light armor."));
    mvwprintz(win, 17, 0, c_brown,   _("\
^>v<        Library - Home to books, both entertaining and informative."));
    mvwprintz(win, 18, 0, c_white, _("\
^>v< are always man-made buildings. The pointed side indicates the front door."));

    mvwprintz(win, 22, 0, c_ltgray, _("There are many others out there... search for them!"));
    wrefresh(win);
    refresh();
    getch();
}

//ready, checked
std::vector<std::string> text_guns()
{
    std::vector<std::string> text;

    text.push_back(_("<color_ltgray>( Handguns</color>\n\
Handguns are small weapons held in one or both hands. They are much more difficult \
to aim and control than larger firearms, and this is reflected in their poor accuracy. \
However, their small size makes them appropriate for short-range combat, where larger guns \
fare poorly. They are also relatively quick to reload, and use a very wide selection of \
ammunition. Their small size and low weight make it possible to carry several loaded handguns, \
switching from one to the next once their ammo is spent."));

    text.push_back(_("<color_green>( Crossbows</color>\n\
The best feature of crossbows is their silence. The bolts they fire are only rarely destroyed; \
if you pick up the bolts after firing them, your ammunition will last much longer. \
Crossbows suffer from a short range and a very long reload time (modified by your strength); \
plus, most only hold a single round. \
\n\
For this reason, it is advisable to carry a few loaded crossbows. \
Crossbows can be very difficult to find; however, it is possible to craft one given enough \
Mechanics skill. Likewise, it is possible to make wooden bolts from any number of wooden objects, \
though these are much less effective than steel bolts. Crossbows use the handgun skill."));

    text.push_back(_("<color_red>( Shotguns</color>\n\
Shotguns are one of the most powerful weapons in the game, capable of taking \
out almost any enemy with a single hit. Birdshot and 00 shot spread, making \
it very easy to hit nearby monsters. However, they are very ineffective \
against armor, and some armored monsters might shrug off 00 shot completely. \
Shotgun slugs are the answer to this problem; they also offer much better range than shot.\
\n\
The biggest drawback to shotguns is their noisiness. They are very loud, and impossible to \
silence. A shot that kills one zombie might attract three more! Because of this, shotguns \
are best reserved for emergencies."));

    text.push_back(_("<color_cyan>( Submachine Guns</color>\n\
Submachine guns are small weapons (some are barely larger than a handgun), designed \
for relatively close combat and the ability to spray large amounts of bullets. \
However, they are more effective when firing single shots, so use discretion. \
They mainly use the 9mm and .45 ammunition; however, other SMGs exist. They reload \
moderately quickly, and are suitable for close or medium-long range combat."));

    text.push_back(_("<color_brown>( Hunting Rifles</color>\n\
Hunting rifles are popular for their superior range and accuracy. What's more, their scopes or \
sights make shots fired at targets more than 10 tiles away as accurate as those with a shorter \
range. However, they are very poor at hitting targets 4 squares or less from the player. \
Unlike assault rifles, hunting rifles have no automatic fire. They are also slow to reload and \
fire, so when facing a large group of nearby enemies, they are not the best pick."));

    text.push_back(_("<color_blue>( Assault Rifles</color>\n\
Assault rifles are similar to hunting rifles in many ways; they are also suited for long range \
combat, with similar bonuses and penalties. Unlike hunting rifles, assault rifles are capable \
of automatic fire. Assault rifles are less accurate than hunting rifles, and this is worsened \
under automatic fire, so save it for when you're highly skilled. \
\n\
Assault rifles are an excellent choice for medium or long range combat, or \
even close-range bursts again a large number of enemies. They are difficult \
to use, and are best saved for skilled riflemen."));

    text.push_back(_("<color_ltred>( Machine Guns</color>\n\
Machine guns are one of the most powerful firearms available. They are even \
larger than assault rifles, and make poor melee weapons; however, they are \
capable of holding 100 or more rounds of highly-damaging ammunition. They \
are not built for accuracy, and firing single rounds is not very effective. \
However, they also possess a very high rate of fire and somewhat low recoil, \
making them very good at clearing out large numbers of enemies."));

    text.push_back(_("<color_magenta>( Energy Weapons</color>\n\
Energy weapons is an umbrella term used to describe a variety of rifles and handguns \
which fire lasers, plasma, or energy attacks. They started to appear in military use \
just prior to the start of the apocalypse, and as such are very difficult to find.\
\n\
Energy weapons have no recoil at all; they are nearly silent, have a long range, and are \
fairly damaging. The biggest drawback to energy weapons is scarcity of ammunition; it is \
wise to reserve the precious ammo for when you really need it."));

    return text;
}

//fix-it
std::vector<std::string> text_faq()
{
    std::vector<std::string> text;

    text.push_back(_("\
Q: What is Safe Mode, and why does it prevent me from moving?\n\
A: Safe Mode is a way to guarantee that you won't die by holding a movement key down. \
When a monster comes into view, your movement will be ignored until Safe Mode is turned off \
with the ! key. This ensures that the sudden appearence of a monster won't catch you off guard."));

    text.push_back(_("\
Q: It seems like everything I eat makes me sick! What's wrong?\n\
A: Lots of the food found in towns is perishable, and will only last a few days after \
the start of a new game (July 12). Fruit, milk, and others are the first to go. After \
the first couple of days, you should switch to canned food, jerky, and hunting."));

    text.push_back(_("\
Q: How can I remove boards from boarded-up windows and doors?\n\
A: Use a hammer and choose the direction of the boarded-up window or door to remove the boards."));

    text.push_back(_("\
Q: The game just told me to quit, and other weird stuff is happening.\n\
A: You have the Schizophrenic trait, which might make the game seem buggy."));

    text.push_back(_("\
Q: How can I prevent monsters from attacking while I sleep?\n\
A: Find a safe place to sleep, in a building far from the front door. \
Set traps if you have them, or build a fire."));

    text.push_back(_("\
Q: Why do I always sink when I try to swim?\n\
A: Your swimming ability is reduced greatly by the weight you are carrying, and is also adversely \
affected by the clothing you wear. Until you reach a high level in the swimming skill, you'll \
need to drop your equipment and remove your clothing to swim, making it a last-ditch escape plan."));

    text.push_back(_("\
Q: How can I cure a fungal infection?\n\
A: At present time, there is only one cure, royal jelly. \
You can find royal jelly in the bee hives which dot forests."));

    text.push_back(string_format(_("\
Q: How do I get into science labs?\n\
A: You can enter the front door if you have an ID card by %s the keypad. If you are skilled \
in computers and have an electrohack, it is possible to hack the keypad. An EMP blast has a \
chance to force the doors open, but it's more likely to break them. You can also sneak in \
through the sewers sometimes, or try to smash through the walls with explosions."),
    press_x(ACTION_EXAMINE, _("examining")).c_str())); //Damn you, coder!

    text.push_back(_("\
Q: Why does my crafting fail so often?\n\
A: Check the difficulty of the recipe, and the primary skill used; your skill level should \
be around one and a half times the difficulty to be confident that it will succeed."));

    text.push_back(_("\
Q: Why can't I carry anything?\n\
A: At the start of the game you only have the space in your pockets. \
A good first goal of many survivors is to find a backpack or pouch to store things in. \
(The shelter basement is a good place to check first!)"));

    text.push_back(_("\
Q: Shotguns bring in more zombies than they kill!  What's the point?\n\
A: Shotguns are intended for emergency use. If you are cornered, use \
your shotgun to escape, then just run from the zombies it attracts."));

    text.push_back(_("\
Q: Help! I started a fire and now my house is burning down!\n\
A: Fires will spread to nearby flammable tiles if they are able. Lighting a \
stop fire in a set-up brazier, wood stove, stone fireplace, or pit will \
it from spreading. Fire extinguishers can put out fires that get out of control."));

    text.push_back(_("\
Q: I have a question that's not addressed here. How can I get an answer?\n\
A: Ask the helpful people on the forum at smf.cataclysmdda.com or email your question to \
TheDarklingWolf@Gmail.com."));

    return text;
}

void display_help()
{
    WINDOW *w_help_border = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                   (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0,
                                   (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0);
    WINDOW *w_help = newwin(FULL_SCREEN_HEIGHT - 2, FULL_SCREEN_WIDTH - 2,
                            1 + (int)((TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0),
                            1 + (int)((TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0));
    char ch;
    do {
        draw_border(w_help_border);
        center_print(w_help_border, 0, c_ltred, _(" HELP "));
        wrefresh(w_help_border);
        help_main(w_help);
        refresh();
        ch = getch();
        switch (ch)
        {
        case 'a':
        case 'A':
            multipage(w_help, text_introduction());
            break;

        case 'b':
        case 'B':
            help_movement(w_help);
            break;

        case 'c':
        case 'C':
            multipage(w_help, text_viewing());
            break;

        case 'd':
        case 'D':
            multipage(w_help, text_hunger());
            break;

        case 'e':
        case 'E':
            multipage(w_help, text_pain());
            break;

        case 'f':
        case 'F':
            multipage(w_help, text_addiction());
            break;

        case 'g':
        case 'G':
            multipage(w_help, text_morale());
            break;

        case 'h':
        case 'H':
            multipage(w_help, text_mutation());
            break;

        case 'i':
        case 'I':
            multipage(w_help, text_bionics());
            break;

        case 'j':
        case 'J':
            multipage(w_help, text_crafting());
            break;

        case 'k':
        case 'K':
            multipage(w_help, text_traps());
            break;

        case 'l':
        case 'L':
            multipage(w_help, text_items());
            break;
        case 'm':
        case 'M':
            multipage(w_help, text_combat());
            break;

        case 'n':
        case 'N':
            multipage(w_help, text_styles());
            break;

        case 'o':
        case 'O':
            multipage(w_help, text_tips());
            break;

        case 'p':
        case 'P':
            help_driving(w_help);
            break;

        // Keybindings
        case '1': {

            // Remember what the keybindings were originally so we can restore them if player cancels.
            std::map<char, action_id> keymap_old = keymap;

            werase(w_help);
            int offset = 1;
            char remapch = ' ';
            bool changed_keymap = false;
            bool needs_refresh = true;
            do {
                if (needs_refresh) {
                    werase(w_help);
                    mvwprintz(w_help, 0, 50, c_white, _("Use the arrow keys"));
                    mvwprintz(w_help, 1, 50, c_white, _("(or movement keys)"));
                    mvwprintz(w_help, 2, 50, c_white, _("to scroll."));

                    mvwprintz(w_help, 4, 50, c_white, _("Press ESC or q to return."));

                    mvwprintz(w_help, 6, 50, c_white, _("Press - to remove all"));
                    mvwprintz(w_help, 7, 50, c_white, _("keybindings from an action."));

                    mvwprintz(w_help, 9, 50, c_white, _("Press + to add the"));
                    mvwprintz(w_help, 10, 50, c_white, _("keybinding for an action."));
                    needs_refresh = false;
                }
                // Clear the lines
                for (int i = 0; i < FULL_SCREEN_HEIGHT - 2; i++)
                    mvwprintz(w_help, i, 0, c_black, "                                                ");

                //Draw Scrollbar
                draw_scrollbar(w_help_border, offset - 1, FULL_SCREEN_HEIGHT - 2, NUM_ACTIONS - 20, 1);

                for (int i = 0; i < FULL_SCREEN_HEIGHT - 2 && offset + i < NUM_ACTIONS; i++) {
                    std::vector<char> keys = keys_bound_to( action_id(offset + i) );
                    nc_color col = (keys.empty() ? c_ltred : c_white);
                    mvwprintz(w_help, i, 3, col, "%s: ", action_name( action_id(offset + i) ).c_str());
                    if (keys.empty()) {
                        wprintz(w_help,c_red, _("Unbound!"));
                    } else {
                        for (int j = 0; j < keys.size(); j++) {
                            wprintz(w_help,c_yellow, "%c", keys[j]);
                            if (j < keys.size() - 1) {
                                wprintz(w_help,c_white, _(" or "));
                            }
                        }
                    }
                }
                wrefresh(w_help);
                refresh();
                remapch = input();
                int sx = 0, sy = 0;
                get_direction(sx, sy, remapch);
                if (sy == -1 && offset > 1) {
                    offset--;
                }
                if (sy == 1 && offset + 20 < NUM_ACTIONS) {
                    offset++;
                }
                if (remapch == '-' || remapch == '+') {
                    needs_refresh = true;
                    for (int i = 0; i < FULL_SCREEN_HEIGHT - 2 && i + offset < NUM_ACTIONS; i++) {
                        mvwprintz(w_help, i, 0, c_ltblue, "%c", 'a' + i);
                        mvwprintz(w_help, i, 1, c_white, ":");
                    }
                        wrefresh(w_help);
                        refresh();
                        char actch = getch();
                    if (actch >= 'a' && actch <= 'a' + 24 && actch - 'a' + offset < NUM_ACTIONS) {
                        action_id act = action_id(actch - 'a' + offset);
                        if (remapch == '-' && query_yn(_("Clear keys for %s?"),
                            action_name(act).c_str())){
                            clear_bindings(act);
                            changed_keymap = true;
                        } else if (remapch == '+') {
                            char newbind = popup_getkey(_("New key for %s:"), action_name(act).c_str());
                            if (keymap.find(newbind) == keymap.end()) { // It's not in use!  Good.
                                keymap[ newbind ] = act;
                                changed_keymap = true;
                            } else {
                                popup(_("%c is used for %s."), newbind,
                                      action_name( keymap[newbind] ).c_str());
                            }
                        }
                    }
                }
            } while (remapch != 'q' && remapch != 'Q' && remapch != KEY_ESCAPE);


            if (changed_keymap) {
               if(query_yn(_("Save changes?"))) {
                   save_keymap();
               } else {
                   // Player wants to keep the old keybindings. Revert!
                   keymap = keymap_old;
               }
            }

            werase(w_help);
            } break;

        case '2':
            show_options(true);
            werase(w_help);
            break;

        case '3':
            show_auto_pickup();
            werase(w_help);
            break;

        case '4':
            multipage(w_help, text_types(), _("Item types:"));
            break;

        case '5':
            help_map(w_help);
            break;

        case '6':
            multipage(w_help, text_guns(), _("Gun types:"));
            break;

        case '7':
            multipage(w_help, text_faq());
            break;
        }
    } while (ch != 'q' && ch != KEY_ESCAPE);
    delwin(w_help);
    delwin(w_help_border);
}

void load_hint(JsonObject &jsobj)
{
    hints.push_back(_(jsobj.get_string("text").c_str()));
}

void clear_hints()
{
    hints.clear();
}

std::string get_hint()
{
    if (hints.empty()) {
        return "???";
    } else {
        return hints[rng(0, hints.size()-1)];
    }
}
