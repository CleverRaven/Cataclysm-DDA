#include "help.h"

#include "action.h"
#include "catacharset.h"
#include "input.h"
#include "output.h"
#include "rng.h"
#include "cursesdef.h"
#include "string_formatter.h"
#include "translations.h"
#include "text_snippets.h"
#include <cmath>  // max in help_main
#include <vector>

void help_draw_dir( const catacurses::window &win, int line_y )
{
    std::array<action_id, 9> movearray = {{
            ACTION_MOVE_NW, ACTION_MOVE_N, ACTION_MOVE_NE,
            ACTION_MOVE_W,  ACTION_PAUSE,  ACTION_MOVE_E,
            ACTION_MOVE_SW, ACTION_MOVE_S, ACTION_MOVE_SE
        }
    };
    mvwprintz( win, line_y + 1, 0, c_white, _( "\
  \\ | /     \\ | /\n\
   \\|/       \\|/ \n\
  -- --     -- --  \n\
   /|\\       /|\\ \n\
  / | \\     / | \\" ) );
    for( int acty = 0; acty < 3; acty++ ) {
        for( int actx = 0; actx < 3; actx++ ) {
            std::vector<char> keys = keys_bound_to( movearray[acty * 3 + actx] );
            if( !keys.empty() ) {
                mvwputch( win, acty * 3 + line_y, actx * 3 + 1, c_light_blue, keys[0] );
                if( keys.size() > 1 ) {
                    mvwputch( win, acty * 3 + line_y, actx * 3 + 11, c_light_blue, keys[1] );
                }
            }
        }
    }
}

void help_main( const catacurses::window &win )
{
    werase( win );
    int y = fold_and_print( win, 0, 1, getmaxx( win ) - 2, c_white, _( "\
Please press one of the following for help on that topic:\n\
Press q or ESC to return to the game." ) ) + 1;

    std::vector<std::string> headers;
    headers.push_back( _( "a: Introduction" ) );
    headers.push_back( _( "b: Movement" ) );
    headers.push_back( _( "c: Viewing" ) );
    headers.push_back( _( "d: Hunger, thirst, and sleep" ) );
    headers.push_back( _( "e: Pain and stimulants" ) );
    headers.push_back( _( "f: Addiction" ) );
    headers.push_back( _( "g: Morale and learning" ) );
    headers.push_back( _( "h: Radioactivity and mutation" ) );
    headers.push_back( _( "i: Bionics" ) );
    headers.push_back( _( "j: Crafting" ) );
    headers.push_back( _( "k: Traps" ) );
    headers.push_back( _( "l: Items overview" ) );
    headers.push_back( _( "m: Combat" ) );
    headers.push_back( _( "n: Unarmed styles" ) );
    headers.push_back( _( "o: Survival tips" ) );
    headers.push_back( _( "p: Driving" ) );

    size_t half_size = headers.size() / 2;
    int second_column = getmaxx( win ) / 2;
    for( size_t i = 0; i < headers.size(); i++ ) {
        if( i < half_size ) {
            second_column = std::max( second_column, utf8_width( headers[i] ) + 4 );
        }
        mvwprintz( win, y + i % half_size, ( i < half_size ? 1 : second_column ),
                   c_white, headers[i].c_str() );
    }

    headers.clear();
    headers.push_back( _( "1: List of item types and data" ) );
    headers.push_back( _( "2: Description of map symbols" ) );
    headers.push_back( _( "3: Description of gun types" ) );
    headers.push_back( _( "4: Frequently Asked Questions (some spoilers!)" ) );
    headers.push_back( _( " " ) );
    headers.push_back( _( "q: Return to game" ) );

    y += half_size + 1;
    for( auto &header : headers ) {
        y += fold_and_print( win, y, 1, getmaxx( win ) - 2, c_white, header );
    }
    wrefresh( win );
}

void help_movement( const catacurses::window &win )
{
    werase( win );
    std::vector<std::string> text;
    int pos_y = fold_and_print( win, 0, 1, getmaxx( win ) - 2, c_white,
                                _( "Movement is performed using the numpad, or vikeys." ) ) + 1;
    help_draw_dir( win, pos_y );
    text.push_back( string_format( _( "\
Each step will take 100 movement points (or more, depending on the terrain); \
you will then replenish a variable amount of movement points, \
depending on many factors (press %s to see the exact amount)." ),
                                   press_x( ACTION_PL_INFO, "", "" ).c_str() ) );

    text.push_back( _( "To attempt to hit a monster with your weapon, simply move into it." ) );

    text.push_back( string_format( _( "You may find doors, ('+'); these may be opened with %s \
or closed with %s. Some doors are locked. Locked doors, windows, and some other obstacles can be \
destroyed by smashing them (%s, then choose a direction). Smashing down obstacles is much easier \
with a good weapon or a strong character." ),
                                   press_x( ACTION_OPEN, "", "" ).c_str(),
                                   press_x( ACTION_CLOSE, "", "" ).c_str(),
                                   press_x( ACTION_SMASH, "", "" ).c_str() ) );

    text.push_back( string_format( _( "There may be times when you want to move more quickly \
by holding down a movement key. However, fast movement in this fashion may lead to the player \
getting into a dangerous situation or even killed before they have a chance to react. \
Pressing %s will toggle \"Safe Mode\". While this is on, any movement will be ignored if new \
monsters enter the player's view." ),
                                   press_x( ACTION_TOGGLE_SAFEMODE, "", "" ).c_str() ) );

    int fig_last_line = pos_y + 8;
    // TODO: do it better!
    std::vector<std::string> remained_text;
    for( auto &elem : text ) {
        if( pos_y < fig_last_line ) {
            pos_y += fold_and_print( win, pos_y, 20, getmaxx( win ) - 22, c_white, elem ) + 1;
        } else {
            remained_text.push_back( elem.c_str() );
        }
    }
    multipage( win, remained_text, "", pos_y );
}

void help_driving( const catacurses::window &win )
{
    std::vector<std::string> text;
    werase( win );
    int pos_y = fold_and_print( win, 0, 1, getmaxx( win ) - 2, c_white, _( "\
You control vehicles using the numpad, or vikeys. \
However, you control their controls, rather than the vehicle directly." ) ) + 1;

    help_draw_dir( win, pos_y );
    text.push_back( string_format( _( "\
In order to assume control of the vehicle, get to a location with \"vehicle controls\" \
and press %s. Once you are in control, %s accelerates, %s slows or reverses, & %s & %s \
turn left or right. Diagonals adjust course and speed. You default to cruise control, so \
the gas/brake adjust the speed which the vehicle will attempt to maintain." ),
                                   press_x( ACTION_CONTROL_VEHICLE, "", "" ).c_str(),
                                   press_x( ACTION_MOVE_N, "", "" ).c_str(),
                                   press_x( ACTION_MOVE_S, "", "" ).c_str(),
                                   press_x( ACTION_MOVE_W, "", "" ).c_str(),
                                   press_x( ACTION_MOVE_E, "", "" ).c_str() ) );

    text.push_back( string_format( _( "\
10-30 MPH, or 16-48 KPH, is a good speed for beginning drivers, \
who tend to fumble the controls. As your Driving skill improves, \
you will fumble less and less. To simply maintain course and speed, hit %s." ),
                                   press_x( ACTION_PAUSE, "", "" ).c_str() ) );

    text.push_back( string_format( _( "\
It's a good idea to pull the handbrake - \"s\" - when parking, just to be safe. \
If you want to get out, hit the lights, toggle cruise control, turn the engine on or off, \
or otherwise use the vehicle controls, press %s to bring up the \"Vehicle Controls\" menu, \
which has options for things you'd do from the driver's seat." ),
                                   press_x( ACTION_CONTROL_VEHICLE, "", "" ).c_str() ) );

    text.push_back( string_format( _( "Vehicle menu presents the most important parameters of \
your car, such as safe and maximum speed, current mass, total capacity and used volume of \
available cargo space, various fuel level indicators, and so on." ) ) );

    text.push_back( string_format( _( "Becoming a skilled mechanic, you may want \
to tune your car up.  The coefficients of aerodynamics, friction and mass efficiency \
play significant roles it this process.  Named coefficients are measured in the range \
from 0%% (which means terrible inefficiency) to 100%% (ideal conditions)." ) ) );

    int fig_last_line = pos_y + 8;
    std::vector<std::string> remained_text;
    for( auto &elem : text ) {
        if( pos_y < fig_last_line ) {
            pos_y += fold_and_print( win, pos_y, 20, getmaxx( win ) - 22, c_white, elem ) + 1;
        } else {
            remained_text.push_back( elem.c_str() );
        }
    }
    multipage( win, remained_text, "", pos_y );
}

std::vector<std::string> text_introduction()
{
    std::vector<std::string> text;

    text.push_back( _( "\
Cataclysm is a survival roguelike with a monster apocalypse setting. \
You have survived the original onslaught, but the future looks pretty grim." ) );

    text.push_back( _( "\
You must prepare to face the many hardships to come including dwindling supplies, \
hostile creatures, and harmful weather. Even among fellow survivors you must stay alert, since \
someone may be plotting behind your back to take your hard-earned loot." ) );

    text.push_back( _( "\
Cataclysm differs from the traditional roguelikes in several ways. Rather than exploring \
an underground dungeon, with a limited area on each level, you are exploring \
a truly infinite world, stretching in all four cardinal directions. In this survival roguelike, \
you will have to find food; you also need to keep yourself hydrated and sleep periodically." ) );

    text.push_back( _( "\
While Cataclysm has more tasks to keep track of than many other roguelikes, \
the near-future setting of the game makes some tasks easier. Firearms, medications, \
and a wide variety of tools are all available to help you survive." ) );

    return text;
}

std::vector<std::string> text_viewing()
{
    std::vector<std::string> text;

    text.push_back( string_format( _( "\
The player can often see more than can be displayed on the screen at a time. Pressing %s enters \
\"look around mode\", which allows you to scroll around using the movement keys and view items on \
the map as well as monsters and their stance toward the character. Pressing %s provides a list of \
nearby visible items, though items shut away in crates, cupboards, refrigerators and the like \
won't be displayed until you are close enough. Pressing Shift+vikeys (h,j,k,l,y,u,b,n) will scroll \
the view persistently, allowing you to keep an eye on things as you move around." ),
                                   press_x( ACTION_LOOK, "", "" ).c_str(),
                                   press_x( ACTION_LIST_ITEMS, "", "" ).c_str() ) );
    return text;
}

std::vector<std::string> text_hunger()
{
    std::vector<std::string> text;

    text.push_back( _( "\
As time passes, you will begin to feel hunger and thirst. When this happens, a status warning at the sidebar \
will appear. As hunger and thirst reach critical levels, you will begin to suffer movement \
penalties." ) );

    text.push_back( _( "\
Thirst is more dangerous than hunger but you can develop various vitamin deficiencies \
if you eat poorly. These deficiencies come in stages, so for example you won't go from perfectly good \
health into a full-blown scurvy in an instant. Any developing and on-going deficiencies will be reported \
in the character sheet. Deficiencies will inflict various penalties, but luckily they are always \
reversible, and multivitamin pills can help you to correct any deficiencies. You can also ingest too much \
vitamins, and that too can create problems. Be sure to have a balanced diet, or at least not a completely \
atrocious one. You can and should examine food items to view their nutritional facts." ) );

    text.push_back( string_format( _( "\
Finding food in a city is usually easy; outside of a city, you may have to hunt. After killing \
an animal, stand over the animal's corpse and butcher it into small chunks of meat by pressing %s. You \
might also be able to forage for edible fruit or vegetables; to do it, find a promising plant and \
examine it. Likewise, you may have to drink water from a river or another natural source. To collect it, \
stand in shallow water and press %s. You'll need a watertight container to store it. Be forewarned \
that some sources of water aren't trustworthy and may produce diseased water. To make sure it's healthy, \
purify the water by boiling it or using water purifier before drinking." ),
                                   press_x( ACTION_BUTCHER, "", "" ).c_str(),
                                   press_x( ACTION_PICKUP, "", "" ).c_str() ) );

    text.push_back( string_format( _( "\
Every 14 to 20 hours, you'll find yourself growing sleepy. If you do not sleep by pressing %s, \
you'll start suffering stat and movement penalties. You may not always fall asleep right away. \
Sleeping indoors, especially on a bed, will help. If that's not enough, sleeping pills may be of use. \
While sleeping, you'll slowly replenish lost hit points. You'll also be vulnerable to attack, \
so try to find a safe place to sleep, or set traps for unwary intruders." ),
                                   press_x( ACTION_SLEEP, "", "" ).c_str() ) );

    return text;
}

std::vector<std::string> text_pain()
{
    std::vector<std::string> text;

    text.push_back( _( "\
When you take damage from almost any source, you'll start to feel pain. Pain slows you down and \
reduces your stats, and finding a way to manage pain is an early imperative. The most common is \
drugs: aspirin, codeine, tramadol, oxycodone, and more are all great options. Be aware that while \
under the influence of many painkillers, the physiological side effects may slow you or \
reduce your stats." ) );

    text.push_back( _( "\
Note that most painkillers take a little while to kick in. If you take some oxycodone and don't \
notice the effects right away, don't start taking more -  you may overdose and die!" ) );

    text.push_back( _( "\
Pain will also disappear with time, so if drugs aren't available and you're in a lot of pain, it \
may be wise to find a safe spot and simply rest for an extended period of time." ) );

    text.push_back( _( "\
Another common class of drugs is stimulants. Stimulants provide you with a temporary rush of \
energy, increasing your movement speed and many stats (most notably intelligence), making them \
useful study aids. There are two drawbacks to stimulants: they make it more difficult to sleep \
and, more importantly, most are highly addictive. Stimulants range from the caffeine rush of \
cola to the more intense high of Adderall and methamphetamine." ) );

    return text;
}

std::vector<std::string> text_addiction()
{
    std::vector<std::string> text;

    text.push_back( _( "\
Many drugs have a potential for addiction. Each time you consume such a drug there is a chance \
that you will grow dependent on it. Consuming more of that drug will increase your dependence. \
Effects vary greatly between drugs, but all addictions have only one cure; going cold turkey. \
The process may last for days and will leave you very weak, so try to do it in a safe area." ) );

    text.push_back( _( "\
If you are suffering from drug withdrawal, taking more of the drug will cause \
the effects to cease immediately, but may deepen your dependence." ) );

    return text;
}

std::vector<std::string> text_morale()
{
    std::vector<std::string> text;

    text.push_back( _( "\
Your character has a morale level, which affects you in many ways. The depressing post-apocalypse \
world is tough to deal with, and your mood will naturally decrease very slowly." ) );

    text.push_back( _( "\
There are lots of options for increasing morale; reading an entertaining book, eating \
delicious food, and taking recreational drugs are but a few options. Most morale-boosting \
activities can only take you to a certain level before they grow boring." ) );

    text.push_back( _( "\
There are also lots of ways for your morale to decrease, beyond its natural \
decay. Eating disgusting food, reading a boring technical book, killing a friendly NPC, \
or going through drug withdrawal are some prominent examples." ) );

    text.push_back( _( "\
Low morale will make you sluggish and unmotivated. If your morale drops very low, you won't be able to perform \
crafting at all. If you become sufficiently depressed, you will suffer stat penalties. \
Very high morale fills you with gusto and energy, and you will find yourself moving faster. At extremely high levels, \
you will receive stat bonuses." ) );

    text.push_back( _( "\
Morale is also responsible for ensuring you can learn effectively, via a mechanic referred \
to as 'focus'. Your focus level is a measure of how effectively you can learn. The natural \
level is 100, which indicates normal learning potential. Higher or lower focus levels make \
it easier or harder to learn from practical experience." ) );

    text.push_back( _( "\
Your focus level has a natural set point that it converges towards. \
When your focus is much lower - or higher - than this set point, it will change faster \
than when it is near the set point. Having high morale will raise the set point, \
and having low morale will lower the set point. Pain is also factored \
into the set point calculation - it's harder to learn when you're in pain." ) );

    text.push_back( _( "\
Your focus is also lowered by certain activities. Training your skills through real-world practice \
lowers your focus gradually, by an amount that depends on your current level of focus (higher \
focus means larger decreases, as well as improved learning). Training your skills by reading books \
decreases your focus rapidly, by giving a significant penalty to the set point of your focus." ) );

    return text;
}

std::vector<std::string> text_mutation()
{
    std::vector<std::string> text;

    text.push_back( _( "\
Though it is relatively rare, certain areas of the world may be contaminated \
with radiation. It will gradually accumulate in your body, weakening you \
more and more. While in radiation-free areas, your radiation level will \
slowly decrease. Taking Prussian blue tablets will help speed the process." ) );

    text.push_back( _( "\
If you become very irradiated, you may develop mutations. Most of the time, these \
mutations will be negative; however, many are beneficial, and others have both positive \
and negative effects. Your mutations may change your play style considerably. It is possible \
to find substances that will remove mutations, though these are extremely rare." ) );

    text.push_back( _( "\
There are various mutagenic substances, and consuming (or injecting) them will grant mutations. \
However, the process taxes your body significantly, and can be addictive. With enough mutations \
and certain conditions being met, you will permanently transcend your humanity into a wholly \
different life-form." ) );
    return text;
}

std::vector<std::string> text_bionics()
{
    std::vector<std::string> text;

    text.push_back( _( "\
Bionics are biomechanical upgrades to your body. \
While many are simply 'built-in' versions of items you would otherwise have to carry, \
some bionics have unique effects that are otherwise unobtainable. Some bionics are \
constantly working, whereas others need to be toggled on and off as needed." ) );

    text.push_back( _( "\
Most bionics require a source of power, and you will need an internal battery \
to store energy for them. Your current amount of energy is displayed in the sidebar right below \
your health. Replenishing energy can be done in a variety of ways, but all \
require the installation of a special bionic just for fuel consumption." ) );

    text.push_back( _( "\
Bionics come in ready-to-install canisters. Installation of a bionic is best left to a trained \
professional or to specialized medical apparatus. Using machinery to manipulate bionics requires \
high levels of Intelligence, first aid, mechanics, and electronics. Beware, though, a failure \
may cripple you!  Many bionic canisters are difficult to find, but may be purchased from certain \
wandering vagabonds for a very high price." ) );

    text.push_back( _( "\
As you may note, all of your body parts have only limited space for containing bionics, \
so you should choose bionics for installation wisely. Of course, any bionic can be removed \
from your body but it's not any easier than its installation; and as well as installation, \
this non-trivial surgical procedure requires anesthesia." ) );

    return text;
}

std::vector<std::string> text_crafting()
{
    std::vector<std::string> text;

    text.push_back( _( "\
Many important items can be very hard to find or will cost a great deal of \
money to trade for. Fortunately, it is possible to craft a wide variety of \
goods (as best you can) with the proper tools, materials, and training." ) );

    text.push_back( _( "\
Some recipes require a set of tools. These are not used up when crafting, so you can keep your \
tool set. All recipes require one or more ingredients. These ARE used up in crafting." ) );

    text.push_back( string_format( _( "\
To craft items, press %s. There are seven categories: \
Weapons, Ammo, Food, Chemicals, Electronics, Armor, and Other. In each major category there \
are several smaller sub-categories. While a few items require no particular skill \
to create, the majority require you to have some knowledge. Sometimes a skilled \
survivor will work out a given recipe from her or his knowledge of the skill, but \
more often you will need reference material, commonly a book of some sort. Reading \
such references gives a chance to memorize recipes outright, and you can also craft \
while referring to the book: just have it handy when crafting. Different knowledge is \
useful for different applications:\n" ),
                                   press_x( ACTION_CRAFT, "", "" ).c_str() ) );

    text.push_back( _( "\
-> Fabrication is the generic artisan skill, used for a wide variety of gear.\n\
-> Cooking, at low levels, is used for making tasty recipes; at higher levels, you have \
an understanding of chemistry and can make chemical weapons and beneficial elixirs.\n\
-> Tailoring is used to create basic clothing, and armor later on.\n\
-> Electronics lets you make a wide variety of tools with intricate uses." ) );

    text.push_back( _( "\
In addition to the primary crafting skills, other skills may be necessary to create certain \
items. Traps, Marksmanship, and First Aid are all required for certain items." ) );

    text.push_back( _( "\
Crafting an item with high difficulty may fail and possibly waste some materials. You should prepare spare material, \
just in case." ) );

    return text;
}

std::vector<std::string> text_traps()
{
    std::vector<std::string> text;

    text.push_back( _( "\
Before sleeping in a dangerous territory, it may be wise to set traps to ward off \
unwanted intrusions. There are a few traps to be found in the world, most notably \
bubble wrap (which will make a loud noise if stepped on, helping to wake you up) \
and bear traps (which make noise AND damage, and trap anything that steps on them). \
Others need to be crafted; this requires the Traps skill and possibly Mechanics." ) );

    text.push_back( string_format( _( "\
To set a trap, simply use the item (press %s) and choose a direction; the trap will be \
placed on an adjacent tile. Some traps may require additional tools, like a shovel, \
to be set. Once set, a trap will remain in place until stepped on or disarmed." ),
                                   press_x( ACTION_USE, "", "" ).c_str() ) );

    text.push_back( string_format( _( "\
To disarm a trap, examine (press %s) the space it is on. Your success depends \
upon your Traps skill and Dexterity. If you succeed, the trap is removed \
and replaced by some or all of its constituent parts; however, if you fail, \
there is a chance that you will set off the trap, suffering the consequences." ),
                                   press_x( ACTION_EXAMINE, "", "" ).c_str() ) );

    text.push_back( _( "\
Many traps are fully or partially hidden. Your ability to detect traps is \
entirely dependent upon your Perception. Should you stumble into a trap, \
you may have a chance to avoid it, depending on your Dodge skill." ) );

    return text;
}

std::vector<std::string> text_items()
{
    std::vector<std::string> text;

    text.push_back( string_format( _( "\
There is a wide variety of items available for your use. You may find them \
lying on the ground; if so, simply press %s to pick up items on the \
same square. Some items are found inside a container, drawn as a { with a \
blue background. Pressing %s, then a direction key, will allow you to examine \
these containers and loot their contents." ),
                                   press_x( ACTION_PICKUP, "", "" ).c_str(),
                                   press_x( ACTION_EXAMINE, "", "" ).c_str() ) );

    text.push_back( string_format( _( "\
Pressing %s opens a comparison menu, where you can see two items \
side-by-side with their attributes highlighted to indicate which is superior. \
You can also access the item comparison menu by pressing C after %s to view \
nearby items menu is open and an item is selected." ),
                                   press_x( ACTION_COMPARE, "", "" ).c_str(),
                                   press_x( ACTION_LIST_ITEMS, "", "" ).c_str() ) );

    text.push_back( string_format( _( "\
All items may be used as a melee weapon, though some are better than others. You can check the \
melee attributes of an item you're carrying by hitting %s to enter your inventory, then pressing \
the letter of the item. There are 3 melee values: bashing, cutting, and to-hit bonus (or \
penalty). Bashing damage is universally effective, but is capped by low strength. Cutting damage \
is a guaranteed increase in damage, but it may be reduced by a monster's natural armor." ),
                                   press_x( ACTION_INVENTORY, "", "" ).c_str() ) );

    text.push_back( string_format( _( "\
To wield an item as a weapon, press %s then the proper letter. wielding the item you are currently wielding \
will unwield it, leaving your hands empty. A wielded weapon will not contribute to your volume carried, so \
holding a large item in your hands may be a good option for travel. When unwielding your weapon, \
it will go back in your inventory, or will be dropped on the ground if there is no space." ),
                                   press_x( ACTION_WIELD, "", "" ).c_str() ) );

    text.push_back( string_format( _( "\
To wear a piece of clothing, press %s then the proper letter. Armor reduces damage and helps you \
resist things like smoke. To take off an item, press %s then the proper letter." ),
                                   press_x( ACTION_WEAR, "", "" ).c_str(),
                                   press_x( ACTION_TAKE_OFF, "", "" ).c_str() ) );

    text.push_back( string_format( _( "\
Also available in the view nearby items menu (press %s to open) is the ability to filter or prioritize \
items. You can enter item names, or various advanced filter strings: {<token>:<search param>}" ),
                                   press_x( ACTION_LIST_ITEMS, "", "" ).c_str() ) );

    text.push_back( _( "\
Currently Available tokens:\n\
\t c = category (books, food, etc)    | {c:books}\n\
\t m = material (cotton, kevlar, etc) | {m:iron}\n\
\t dgt = damage greater than (0-5)    | {dgt:2}\n\
\t dlt = damage less than (0-5)       | {dlt:1}" ) );

    return text;
}

std::vector<std::string> text_combat()
{
    std::vector<std::string> text;

    text.push_back( _( "\
With the default Static spawn option, zombies in cities will spawn at the start of the game. \
All monsters are represented by letters on your screen; a list of monster names, and their \
positions relative to you, is displayed on the right side of the screen. \
If the world is set to use the Dynamic spawn option, you will have 90 minutes to prepare yourself \
before the spawns start; after that, monsters everywhere will be spawned based on the noise \
you make." ) );

    text.push_back( _( "\
To attack a monster with a melee weapon, simply move into them. The time it takes to attack \
depends on the size and weight of your weapon. Small, light weapons are the fastest; unarmed \
attacks increase in speed with your Unarmed Combat skill, and will eventually be VERY fast. \
A successful hit with a bashing weapon may stun the monster temporarily. A miss may make you \
stumble and lose movement points. If a monster hits you, your clothing may absorb some damage, \
but you will absorb the excess." ) );

    text.push_back( _( "\
Swarms of monsters may call for firearms. Most firearms in the game require compatible magazines \
to hold the ammunition. Compatible magazines are listed in a given firearm's description. \
Fortunately, a firearm often spawns with one such magazine in it." ) );

    text.push_back( string_format( _( "\
You can eject a magazine from a firearm by pressing %s and load it with compatible ammunition \
separately, or if you have a firearm with a partially filled magazine in it, and some matching \
loose ammo in the inventory, you can simply order a reload by pressing %s, so you will \
automatically eject the magazine, fill it with as much ammo as possible, and then put \
the magazine back in. You don't have to worry about chambering a round though. \
Of course all this takes some time, so try not to do it if there are monsters nearby." ),
                                   press_x( ACTION_UNLOAD, "", "" ).c_str(),
                                   press_x( ACTION_RELOAD, "", "" ).c_str() ) );

    text.push_back( _( "\
While magazines are often firearm-specific, on some occasions a magazine is compatible with \
several other firearms. The firearms in the game often reflect real-world prototypes in terms \
of caliber and compatibility. Below are some examples of interchangeable ammo:\n\
.308 = 7.62x51mm,\n\
.223 = 5.56 NATO,\n\
.270 = .30-06,\n\
.40 S&W = 10mm." ) );

    text.push_back( _( "Magazine descriptions also list the compatible ammo." ) );

    text.push_back( _( "\
Note that while several ammo types exist for a given caliber and magazine type, \
you can't mix and match these types into a single magazine. You can't \
for example load 9x19mm JHP and 9x19 FMJ ammo into the same magazine, \
since a magazine always requires identical rounds to be loaded in it." ) );

    text.push_back( _( "\
Magazines can be stored inside several worn accessories for quicker access, such as \
chest rigs and ammo pouches. All these compatible storage items are listed in a given \
magazine's description. At the moment, you can only store one magazine per clothing item. \
To store a magazine into a clothing item, 'a'ctivate the appropriate clothing item, \
at which point you'll get to choose which magazine to store." ) );

    text.push_back( string_format( _( "\
To fire, press %s, move the cursor to the relevant space, then hit '.' or 'f'. \
Some guns have alternate firing modes, such as burst fire; to cycle modes, press %s. \
Firing continuously, especially in bursts, will severely reduce accuracy." ),
                                   press_x( ACTION_FIRE, "", "" ).c_str(),
                                   press_x( ACTION_SELECT_FIRE_MODE, "", "" ).c_str() ) );

    text.push_back( _( "\
Fleeing will often be a solid tactic, especially when \
overwhelmed by a swarm of zombies. Try to avoid getting cornered inside a building. \
Ducking down into the subways or sewers is often an excellent escape tactic." ) );

    return text;
}

std::vector<std::string> text_styles()
{
    std::vector<std::string> text;

    text.push_back( _( "\
For the unarmed fighter, a variety of fighting styles are available. \
You can start with your choice of a single, commonly-taught style by starting with \
the Martial Arts Training trait. Many, many more can be found in their own traits, \
learned from manuals or by taking lessons from wandering masters." ) );

    text.push_back( string_format( _( "\
To select a fighting style, press %s. If you are already unarmed, this will make you start using the selected style. \
Otherwise, it will be locked in as your default unarmed style. To start using it, wield a relevant weapon \
or empty your hands, by pressing %s, then the key for the item you are currently wielding. If you wish to \
prevent yourself from accidentally wielding weapons taken from the ground, enable \"Keep hands free\" found \
in the styles menu." ),
                                   press_x( ACTION_PICK_STYLE, "", "" ).c_str(),
                                   press_x( ACTION_WIELD, "", "" ).c_str() ) );

    text.push_back( string_format( _( "\
Most styles have a variety of special moves associated with them. Most have a skill requirement, \
and will be unavailable until you reach a level of unarmed skill. You can check the moves by \
pressing '?' in the pick style menu." ) ) );

    text.push_back( _( "\
Many styles also have special effects unlocked under certain conditions. \
These are varied and unique to each style, and range from special combo moves to bonuses \
depending on the situation you are in. You can check these by examining your style." ) );

    return text;
}

std::vector<std::string> text_tips()
{
    std::vector<std::string> text;

    text.push_back( _( "\
The first thing to do is checking your starting location for useful items. Your initial storage is \
limited, and a backpack, trenchcoat, or other storage medium will let you carry a lot \
more. Finding a weapon is important; frying pans, butcher knives, and more are common \
in houses; hardware stores may carry others. Unless you plan on concentrating on melee \
combat, seek out gun stores as soon as possible and load up on more than one type." ) );

    text.push_back( _( "\
It's also important to carry a few medications; painkillers are a must-have, and drugs such \
as cigarettes will make your life easier (but beware of addiction). Leave the city as soon as you \
have a good stockpile of equipment. The high concentration of zombies makes them a \
deathtrap -- but a necessary resource of food and ammunition." ) );

    text.push_back( _( "\
Combat is much easier if you can fight just one monster at once. Use doorways as a choke point \
or stand behind a window and strike as the zombies slowly climb through. Never be afraid to just \
run if you can outpace your enemies. Irregular terrain, like forests, may help you lose monsters." ) );

    text.push_back( _( "\
Using firearms is the easiest way to kill an enemy, but the sound will attract \
unwanted attention. Save the guns for emergencies, and melee when you can." ) );

    text.push_back( _( "\
If you need to protect yourself from acid, clothing made of cloth < leather < \
kevlar < plastic. So while leather and kevlar will protect you from active \
enemies, a hazmat suit and rubber boots will make you nigh-immune to acid damage. \
Items made of glass, ceramics, diamond or precious metals will be totally immune to acid." ) );

    text.push_back( _( "\
Try to keep your inventory as full as possible without being overloaded. You never know when you \
might need an item, most are good to sell, and you can easily drop unwanted items on the floor." ) );

    text.push_back( _( "\
Keep an eye on the weather. Wind and humidity will exacerbate dire conditions, so seek shelter if \
you're unable to withstand them. Staying dry is important, especially if conditions are so low \
that they would cause frostbite. If you're having trouble staying warm over night, make a pile \
of clothing on the floor to sleep on." ) );

    text.push_back( _( "\
Your clothing can sit in one of five layers on your body: next-to-skin, standard, waist, over, \
and belted. You can wear one item from each layer on a body part without incurring an \
encumbrance penalty for too many worn items. Any items beyond the first on each layer add the \
encumbrance of the additional article(s) of clothing to the body part's encumbrance. The \
layering penalty applies a minimum of 2 and a maximum of 10 encumbrance per article of clothing." ) );

    text.push_back( _( "\
For example, on her torso, a character might wear a leather corset (next-to-skin), a leather touring \
suit (standard), a trenchcoat (over), and a survivor's runner pack (belted). Her encumbrance penalty \
is 0.  If she put on a tank top it would conflict with the leather touring suit and add the minimum \
encumbrance penalty of 2." ) );

    return text;
}

std::vector<std::string> text_types()
{
    std::vector<std::string> text;

    text.push_back( string_format( _( "\
~       Liquid\n\
%%       Food\n\
!       Medication\n\
These are all consumed by using %s. They provide a certain amount of nutrition, quench your thirst, may \
be a stimulant or a depressant, and may provide morale. There may also be more subtle effects." ),
                                   press_x( ACTION_EAT, "", "" ).c_str() ) );

    text.push_back( string_format( _( "\
/       Large weapon\n\
;       Small weapon or tool\n\
,       Tiny item\n\
These are all generic items, useful only to be wielded as a weapon. However, some have special \
uses; they will show up under the TOOLS section in your inventory. Press %s to use these." ),
                                   press_x( ACTION_USE, "", "" ).c_str() ) );

    text.push_back( string_format( _( "\
)       Container\n\
These items may hold other items. Some are passable weapons. Many will be listed with their \
contents, e.g. \"plastic bottle of water\". Those containing comestibles may be eaten with %s; \
this may leave an empty container." ),
                                   press_x( ACTION_EAT, "", "" ).c_str() ) );

    text.push_back( string_format( _( "\
[       Clothing\n\
This may be worn with the %s key or removed with the %s key. It may cover one or more body parts; \
you can wear multiple articles of clothing on any given body part, but this will encumber you \
severely. Each article of clothing may provide storage space, warmth, encumbrance, and a \
resistance to bashing and/or cutting attacks. Some may protect against environmental effects." ),
                                   press_x( ACTION_WEAR, "", "" ).c_str(),
                                   press_x( ACTION_TAKE_OFF, "", "" ).c_str() ) );

    text.push_back( string_format( _( "\
(       Firearm\n\
This weapon may be loaded with ammunition with %s, unloaded with %s, and fired with %s. \
Some have automatic fire, which may be used with %s at a penalty to accuracy. \
The color refers to the type; handguns are gray, shotguns are red, submachine guns are cyan, \
rifles are brown, assault rifles are blue, and heavy machine guns are light red. Each has \
a dispersion rating, a bonus to damage, a rate of fire, and a maximum load. Note that most \
firearms load fully in one action, while shotguns must be loaded one shell at a time." ),
                                   press_x( ACTION_RELOAD, "", "" ).c_str(),
                                   press_x( ACTION_UNLOAD, "", "" ).c_str(),
                                   press_x( ACTION_FIRE, "", "" ).c_str(),
                                   press_x( ACTION_SELECT_FIRE_MODE, "", "" ).c_str() ) );

    text.push_back( _( "\
=       Ammunition\n\
Ammunition is worthless without a gun to load it into. Generally, \
there are several variants for any particular caliber. Ammunition has \
damage, dispersion, and range ratings, and an armor-piercing quality." ) );

    text.push_back( string_format( _( "\
*       Thrown weapon; simple projectile or grenade\n\
These items are suited for throwing, and many are only useful when thrown, \
such as grenades, Molotov cocktails, or tear gas. Once activated be certain \
to throw these items by pressing %s, then the letter of the item to throw." ),
                                   press_x( ACTION_THROW, "", "" ).c_str() ) );

    text.push_back( string_format( _( "\
?       Book or magazine\n\
This can be read for training or entertainment by pressing %s. Most require a basic \
level of intelligence; some require some base knowledge in the relevant subject. Some books may \
contain useful crafting recipes." ),
                                   press_x( ACTION_READ, "", "" ).c_str() ) );

    return text;
}

void help_map( const catacurses::window &win )
{
    werase( win );
    mvwprintz( win, 0, 0, c_light_gray,  _( "MAP SYMBOLS:" ) );
    mvwprintz( win, 1, 0, c_brown,   _( "\
.           Field - Empty grassland, occasional wild fruit." ) );
    mvwprintz( win, 2, 0, c_green,   _( "\
F           Forest - May be dense or sparse. Slow moving; foragable food." ) );
    mvwputch( win,  3,  0, c_dark_gray, LINE_XOXO );
    mvwputch( win,  3,  1, c_dark_gray, LINE_OXOX );
    mvwputch( win,  3,  2, c_dark_gray, LINE_XXOO );
    mvwputch( win,  3,  3, c_dark_gray, LINE_OXXO );
    mvwputch( win,  3,  4, c_dark_gray, LINE_OOXX );
    mvwputch( win,  3,  5, c_dark_gray, LINE_XOOX );
    mvwputch( win,  3,  6, c_dark_gray, LINE_XXXO );
    mvwputch( win,  3,  7, c_dark_gray, LINE_XXOX );
    mvwputch( win,  3,  8, c_dark_gray, LINE_XOXX );
    mvwputch( win,  3,  9, c_dark_gray, LINE_OXXX );
    mvwputch( win,  3, 10, c_dark_gray, LINE_XXXX );

    mvwprintz( win,  3, 12, c_dark_gray,  _( "\
Road - Safe from burrowing animals." ) );
    mvwprintz( win,  4, 0, c_dark_gray,  _( "\
H=          Highway - Like roads, but lined with guard rails." ) );
    mvwprintz( win,  5, 0, c_dark_gray,  _( "\
|-          Bridge - Helps you cross rivers." ) );
    mvwprintz( win,  6, 0, c_blue,    _( "\
R           River - Most creatures can not swim across them, but you may." ) );
    mvwprintz( win,  7, 0, c_dark_gray,  _( "\
O           Parking lot - Empty lot, few items. Mostly useless." ) );
    mvwprintz( win,  8, 0, c_light_green, _( "\
^>v<        House - Filled with a variety of items. Good place to sleep." ) );
    mvwprintz( win,  9, 0, c_light_blue,  _( "\
^>v<        Gas station - A good place to collect gasoline. Risk of explosion." ) );
    mvwprintz( win, 10, 0, c_light_red,   _( "\
^>v<        Pharmacy - The best source for vital medications." ) );
    mvwprintz( win, 11, 0, c_green,   _( "\
^>v<        Grocery store - A good source of canned food and other supplies." ) );
    mvwprintz( win, 12, 0, c_cyan,    _( "\
^>v<        Hardware store - Home to tools, melee weapons and crafting goods." ) );
    mvwprintz( win, 13, 0, c_light_cyan,  _( "\
^>v<        Sporting Goods store - Several survival tools and melee weapons." ) );
    mvwprintz( win, 14, 0, c_magenta, _( "\
^>v<        Liquor store - Alcohol is good for crafting Molotov cocktails." ) );
    mvwprintz( win, 15, 0, c_red,     _( "\
^>v<        Gun store - Firearms and ammunition are very valuable." ) );
    mvwprintz( win, 16, 0, c_blue,    _( "\
^>v<        Clothing store - High-capacity clothing, some light armor." ) );
    mvwprintz( win, 17, 0, c_brown,   _( "\
^>v<        Library - Home to books, both entertaining and informative." ) );
    mvwprintz( win, 18, 0, c_white, _( "\
^>v<        Man-made buildings - The pointed side indicates the front door." ) );
    mvwprintz( win, 19, 0, c_light_gray, _( "\
            There are many others out there... search for them!" ) );
    mvwprintz( win, 20, 0,  c_light_gray,  _( "Note colors: " ) );
    int row = 20;
    int column = utf8_width( _( "Note colors: " ) );
    for( auto color_pair : get_note_color_names() ) {
        // The color index is not translatable, but the name is.
        std::string color_description = string_format( "%s:%s, ", color_pair.first.c_str(),
                                        _( color_pair.second.c_str() ) );
        int pair_width = utf8_width( color_description );

        if( column + pair_width > getmaxx( win ) ) {
            column = 0;
            row++;
        }
        mvwprintz( win, row, column, get_note_color( color_pair.first ),
                   color_description.c_str() );
        column += pair_width;
    }
    wrefresh( win );
    catacurses::refresh();
    inp_mngr.wait_for_any_key();
}

std::vector<std::string> text_guns()
{
    std::vector<std::string> text;

    text.push_back( _( "<color_light_gray>( Handguns</color>\n\
Handguns are small weapons held in one or both hands. They are much more difficult \
to aim and control than larger firearms, and this is reflected in their poor accuracy. \
However, their small size makes them appropriate for short-range combat, where larger guns \
fare poorly. They are also relatively quick to reload and use a very wide selection of \
ammunition. Their small size and low weight make it possible to carry several loaded handguns, \
switching from one to the next once their ammo is spent." ) );

    text.push_back( _( "<color_green>( Crossbows</color>\n\
The best feature of crossbows is their silence. The bolts they fire are only rarely destroyed; \
if you pick up the bolts after firing them, your ammunition supply will last much longer. \
Crossbows suffer from a short range and a very long reload time (modified by your strength); \
plus, most only hold a single round. \
\n\
For this reason, it is advisable to carry a few loaded crossbows. \
Crossbows can be very difficult to find; however, it is possible to craft one given enough \
Mechanics skill. Likewise, it is possible to make wooden bolts from any number of wooden objects, \
though these are much less effective than steel bolts. Crossbows use the handgun skill." ) );

    text.push_back( _( "<color_yellow>( Bows</color>\n\
Silent, deadly, easy to make, and the arrows are reusable. Bows have been used forever. Bows are two handed \
and require the user to have a certain strength level. If you do not have enough strength to draw \
the string back, you will fire at a reduced effectiveness. This reduces the reload and fire speeds \
and decreases the range of the arrows fired. \
\n\
Most normal bows require strength of at least 8, others require an above average strength \
of 10, or a significant strength of 12. Bows use the archery skill." ) );

    text.push_back( _( "<color_red>( Shotguns</color>\n\
Shotguns are some of the most powerful weapons in the game, capable of taking \
out almost any enemy with a single hit. Birdshot and 00 shot spread, making \
it very easy to hit nearby monsters. However, they are very ineffective \
against armor, and some armored monsters might shrug off 00 shot completely. \
Shotgun slugs are the answer to this problem; they also offer much better range than shot.\
\n\
The biggest drawback to shotguns is their noisiness. They are very loud, and impossible to \
silence. A shot that kills one zombie might attract three more! Because of this, shotguns \
are best reserved for emergencies." ) );

    text.push_back( _( "<color_cyan>( Submachine Guns</color>\n\
Submachine guns are small weapons (some are barely larger than a handgun), designed \
for relatively close combat and the ability to spray large amounts of bullets. \
However, they are more effective when firing single shots, so use discretion. \
They mainly use the 9mm and .45 ammunition; however, other SMGs exist. They reload \
moderately quickly, and are suitable for close or medium-long range combat." ) );

    text.push_back( _( "<color_brown>( Hunting Rifles</color>\n\
Hunting rifles are popular for their superior range and accuracy. What's more, their scopes or \
sights make shots fired at targets more than 10 tiles away as accurate as those with a shorter \
range. However, they are very poor at hitting targets 4 squares or less from the player. \
Unlike assault rifles, hunting rifles have no automatic fire. They are also slow to reload and \
fire, so when facing a large group of nearby enemies, they are not the best pick." ) );

    text.push_back( _( "<color_blue>( Assault Rifles</color>\n\
Assault rifles are similar to hunting rifles in many ways; they are also suited for long range \
combat, with similar bonuses and penalties. Unlike hunting rifles, assault rifles are capable \
of automatic fire. Assault rifles are less accurate than hunting rifles, and this is worsened \
under automatic fire, so save it for when you're highly skilled. \
\n\
Assault rifles are an excellent choice for medium or long range combat, or \
even close-range bursts again a large number of enemies. They are difficult \
to use, and are best saved for skilled riflemen." ) );

    text.push_back( _( "<color_light_red>( Machine Guns</color>\n\
Machine guns are one of the most powerful firearms available. They are even \
larger than assault rifles, and make poor melee weapons; however, they are \
capable of holding 100 or more rounds of highly-damaging ammunition. They \
are not built for accuracy, and firing single rounds is not very effective. \
However, they also possess a very high rate of fire and somewhat low recoil, \
making them very good at clearing out large numbers of enemies." ) );

    text.push_back( _( "<color_magenta>( Energy Weapons</color>\n\
Energy weapons is an umbrella term used to describe a variety of rifles and handguns \
which fire lasers, plasma, or energy attacks. They started to appear in military use \
just prior to the start of the apocalypse, and as such are very difficult to find.\
\n\
Energy weapons have no recoil at all; they are nearly silent, have a long range, and are \
fairly damaging. The biggest drawback to energy weapons is scarcity of ammunition; it is \
wise to reserve the precious ammo for when you really need it." ) );

    return text;
}

std::vector<std::string> text_faq()
{
    std::vector<std::string> text;

    text.push_back( _( "\
Q: What is Safe Mode, and why does it prevent me from moving?\n\
A: Safe Mode is a way to guarantee that you won't die by holding a movement key down. \
When a monster comes into view, your movement will be ignored until Safe Mode is turned off \
with the ! key. This ensures that the sudden appearance of a monster won't catch you off guard." ) );

    text.push_back( _( "\
Q: It seems like everything I eat makes me sick! What's wrong?\n\
A: Lots of the food found in towns is perishable and will only last a few days after \
the start of a new game. The electricity went out several days ago \
so fruit, milk and others are the first to go bad. \
After the first couple of days, you should switch to canned food, jerky, and hunting. \
Also, you should make sure to cook any food and purify any water you hunt up \
as it may contain parasites or otherwise be unsafe." ) );

    text.push_back( _( "\
Q: How can I remove boards from boarded-up windows and doors?\n\
A: Use a hammer and choose the direction of the boarded-up window or door to remove the boards." ) );

    text.push_back( _( "\
Q: The game just told me to quit, and other weird stuff is happening.\n\
A: You have the Schizophrenic trait, which might make the game seem buggy." ) );

    text.push_back( _( "\
Q: How can I prevent monsters from attacking while I sleep?\n\
A: Find a safe place to sleep, for example in a cleared building far from the front door. \
Set traps if you have them, or build a fire." ) );

    text.push_back( _( "\
Q: Why do I always sink when I try to swim?\n\
A: Your swimming ability is reduced greatly by the weight you are carrying, and is also adversely \
affected by the clothing you wear. Until you reach a high level of the swimming skill, you'll \
need to drop your equipment and remove your clothing to swim, making it a last-ditch escape plan." ) );

    text.push_back( _( "\
Q: How can I cure a fungal infection?\n\
A: Royal jelly, the Blood Filter bionic, and some antifungal chemicals can cure fungal infection. \
You can find royal jelly in the bee hives which dot forests. Antifungal chemicals to cure the fungal \
infection can either be found as random loot or made from other ingredients." ) );

    text.push_back( string_format( _( "\
Q: How do I get into science labs?\n\
A: You can enter the front door if you have an ID card by examining (%s) the keypad. If you are skilled \
in computers and have an electrohack, it is possible to hack the keypad. An EMP blast has a \
chance to force the doors open, but it's more likely to break them. You can also sneak in \
through the sewers sometimes, or try to smash through the walls with explosions." ),
                                   press_x( ACTION_EXAMINE, "", "" ).c_str() ) );

    text.push_back( _( "\
Q: Why does my crafting fail so often?\n\
A: Check the difficulty of the recipe, and the primary skill used; your skill level should \
be around one and a half times the difficulty to be confident that it will succeed." ) );

    text.push_back( _( "\
Q: Why can't I carry anything?\n\
A: At the start of the game you only have the space in your pockets. \
A good first goal of many survivors is to find a backpack or pouch to store things in. \
(The shelter basement is a good place to check first!)" ) );

    text.push_back( _( "\
Q: Shotguns bring in more zombies than they kill!  What's the point?\n\
A: Shotguns are intended for emergency use. If you are cornered, use \
your shotgun to escape, then just run from the zombies it attracts." ) );

    text.push_back( _( "\
Q: Help! I started a fire and now my house is burning down!\n\
A: Fires will spread to nearby flammable tiles if they are able. Lighting a \
fire in a set-up brazier, stove, wood stove, stone fireplace, or pit will \
stop it from spreading. Fire extinguishers can put out fires that get out of control." ) );

    text.push_back( _( "\
Q: I'm cold and can't sleep at night!\n\
A: Gather some clothes and put them in the place you use to sleep in. Being hungry, thirsty, wet, \
or injured can also make you feel the cold more, so try to avoid these effects before you go to sleep." ) );

    text.push_back( _( "\
Q: I have a question that's not addressed here. How can I get an answer?\n\
A: Ask the helpful people on the forum at discourse.cataclysmdda.org or at the IRC channel #CataclysmDDA on freenode." ) );

    return text;
}

void display_help()
{
    catacurses::window w_help_border = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                       ( TERMY > FULL_SCREEN_HEIGHT ) ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0,
                                       ( TERMX > FULL_SCREEN_WIDTH ) ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0 );
    catacurses::window w_help = catacurses::newwin( FULL_SCREEN_HEIGHT - 2, FULL_SCREEN_WIDTH - 2,
                                1 + ( int )( ( TERMY > FULL_SCREEN_HEIGHT ) ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0 ),
                                1 + ( int )( ( TERMX > FULL_SCREEN_WIDTH ) ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0 ) );
    char ch;
    bool needs_refresh = true;
    do {
        if( needs_refresh ) {
            draw_border( w_help_border, BORDER_COLOR, _( " HELP " ) );
            wrefresh( w_help_border );
            help_main( w_help );
            catacurses::refresh();
            needs_refresh = false;
        };
        // TODO: use input context
        ch = inp_mngr.get_input_event().get_first_input();
        switch( ch ) {
            case 'a':
            case 'A':
                multipage( w_help, text_introduction() );
                break;

            case 'b':
            case 'B':
                help_movement( w_help );
                break;

            case 'c':
            case 'C':
                multipage( w_help, text_viewing() );
                break;

            case 'd':
            case 'D':
                multipage( w_help, text_hunger() );
                break;

            case 'e':
            case 'E':
                multipage( w_help, text_pain() );
                break;

            case 'f':
            case 'F':
                multipage( w_help, text_addiction() );
                break;

            case 'g':
            case 'G':
                multipage( w_help, text_morale() );
                break;

            case 'h':
            case 'H':
                multipage( w_help, text_mutation() );
                break;

            case 'i':
            case 'I':
                multipage( w_help, text_bionics() );
                break;

            case 'j':
            case 'J':
                multipage( w_help, text_crafting() );
                break;

            case 'k':
            case 'K':
                multipage( w_help, text_traps() );
                break;

            case 'l':
            case 'L':
                multipage( w_help, text_items() );
                break;
            case 'm':
            case 'M':
                multipage( w_help, text_combat() );
                break;

            case 'n':
            case 'N':
                multipage( w_help, text_styles() );
                break;

            case 'o':
            case 'O':
                multipage( w_help, text_tips() );
                break;

            case 'p':
            case 'P':
                help_driving( w_help );
                break;

            case '1':
                multipage( w_help, text_types(), _( "Item types:" ) );
                break;

            case '2':
                help_map( w_help );
                break;

            case '3':
                multipage( w_help, text_guns(), _( "Gun types:" ) );
                break;

            case '4':
                multipage( w_help, text_faq() );
                break;

            default:
                continue;
        };
        needs_refresh = true;
    } while( ch != 'q' && ch != KEY_ESCAPE );
}

std::string get_hint()
{
    return SNIPPET.get( SNIPPET.assign( "hint" ) );
}
