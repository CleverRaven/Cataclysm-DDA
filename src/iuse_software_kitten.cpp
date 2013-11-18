#include <string>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iterator>
#include <map>
#include <vector>

#include "keypress.h"
#include "output.h"
#include "catacharset.h"
#include "options.h"
#include "debug.h"
#include "posix_time.h"
#include "iuse_software_kitten.h"

#define EMPTY -1
#define ROBOT 0
#define KITTEN 1
#define MAXMESSAGES 1200
std::string robot_finds_kitten::getmessage(int idx) {
char rfimessages[MAXMESSAGES][81] =
{
    "I pity the fool who mistakes me for kitten!\", sez Mr. T.",
    "That's just an old tin can.",
    "It's an altar to the horse god.",
    "A box of dancing mechanical pencils. They dance! They sing!",
    "It's an old Duke Ellington record.",
    "A box of fumigation pellets.",
    "A digital clock. It's stuck at 2:17 PM.",
    "That's just a charred human corpse.",
    "I don't know what that is, but it's not kitten.",
    "An empty shopping bag. Paper or plastic?",
    "Could it be... a big ugly bowling trophy?",
    "A coat hanger hovers in thin air. Odd.",
    "Not kitten, just a packet of Kool-Aid(tm).",
    "A freshly-baked pumpkin pie.",
    "A lone, forgotten comma, sits here, sobbing.",
    "ONE HUNDRED THOUSAND CARPET FIBERS!!!!!",
    "It's Richard Nixon's nose!",
    "It's Lucy Ricardo. \"Aaaah, Ricky!\", she says.",
    "You stumble upon Bill Gates' stand-up act.",
    "Just an autographed copy of the Kama Sutra.",
    "It's the Will Rogers Highway. Who was Will Rogers, anyway?",
    "It's another robot, more advanced in design than you but strangely immobile.",
    "Leonard Richardson is here, asking people to lick him.",
    "It's a stupid mask, fashioned after a beagle.",
    "Your State Farm Insurance(tm) representative!",
    "It's the local draft board.",
    "Seven 1/4\" screws and a piece of plastic.",
    "An 80286 machine.",
    "One of those stupid \"Homes of the Stars\" maps.",
    "A signpost saying \"TO KITTEN\". It points in no particular direction.",
    "A hammock stretched between a tree and a volleyball pole.",
    "A Texas Instruments of Destruction calculator.",
    "It's a dark, amphorous blob of matter.",
    "Just a pincushion.",
    "It's a mighty zombie talking about some love and prosperity.",
    "\"Dear robot, you may have already won our 10 MILLION DOLLAR prize...\"",
    "It's just an object.",
    "A mere collection of pixels.",
    "A badly dented high-hat cymbal lies on its side here.",
    "A marijuana brownie.",
    "A plush Chewbacca.",
    "Daily hunger conditioner from Australasia",
    "Just some stuff.",
    "Why are you touching this when you should be finding kitten?",
    "A glorious fan of peacock feathers.",
    "It's some compromising photos of Babar the Elephant.",
    "A copy of the Weekly World News. Watch out for the chambered nautilus!",
    "It's the proverbial wet blanket.",
    "A \"Get Out of Jail Free\" card.",
    "An incredibly expensive \"Mad About You\" collector plate.",
    "Paul Moyer's necktie.",
    "A haircut and a real job. Now you know where to get one!",
    "An automated robot-hater. It frowns disapprovingly at you.",
    "An automated robot-liker. It smiles at you.",
    "It's a black hole. Don't fall in!",
    "Just a big brick wall.",
    "You found kitten! No, just kidding.",
    "Heart of Darkness brand pistachio nuts.",
    "A smoking branding iron shaped like a 24-pin connector.",
    "It's a Java applet.",
    "An abandoned used-car lot.",
    "A shameless plug for Crummy: http://www.crummy.com/",
    "A shameless plug for the UCLA Linux Users Group: http://linux.ucla.edu/",
    "A can of Spam Lite.",
    "This is another fine mess you've gotten us into, Stanley.",
    "It's scenery for \"Waiting for Godot\".",
    "This grain elevator towers high above you.",
    "A Mentos wrapper.",
    "It's the constellation Pisces.",
    "It's a fly on the wall. Hi, fly!",
    "This kind of looks like kitten, but it's not.",
    "It's a banana! Oh, joy!",
    "A helicopter has crashed here.",
    "Carlos Tarango stands here, doing his best impression of Pat Smear.",
    "A patch of mushrooms grows here.",
    "A patch of grape jelly grows here.",
    "A spindle, and a grindle, and a bucka-wacka-woom!",
    "A geyser sprays water high into the air.",
    "A toenail? What good is a toenail?",
    "You've found the fish! Not that it does you much good in this game.",
    "A Buttertonsils bar.",
    "One of the few remaining discoes.",
    "Ah, the uniform of a Revolutionary-era minuteman.",
    "A punch bowl, filled with punch and lemon slices.",
    "It's nothing but a G-thang, baby.",
    "IT'S ALIVE! AH HA HA HA HA!",
    "This was no boating accident!",
    "Wait! This isn't the poker chip! You've been tricked! DAMN YOU, MENDEZ!",
    "A livery stable! Get your livery!",
    "It's a perpetual immobility machine.",
    "\"On this spot in 1962, Henry Winkler was sick.\"",
    "There's nothing here; it's just an optical illusion.",
    "The World's Biggest Motzah Ball!",
    "A tribe of cannibals lives here. They eat Malt-O-Meal for breakfast, you know.",
    "This appears to be a rather large stack of trashy romance novels.",
    "Look out! Exclamation points!",
    "A herd of wild coffee mugs slumbers here.",
    "It's a limbo bar! How low can you go?",
    "It's the horizon. Now THAT'S weird.",
    "A vase full of artificial flowers is stuck to the floor here.",
    "A large snake bars your way.",
    "A pair of saloon-style doors swing slowly back and forth here.",
    "It's an ordinary bust of Beethoven... but why is it painted green?",
    "It's TV's lovable wisecracking Crow! \"Bite me!\", he says.",
    "Hey, look, it's war. What is it good for? Absolutely nothing. Say it again.",
    "It's the amazing self-referential thing that's not kitten.",
    "A flamboyant feather boa. Now you can dress up like Carol Channing!",
    "\"Sure hope we get some rain soon,\" says Farmer Joe.",
    "\"How in heck can I wash my neck if it ain't gonna rain no more?\" asks Farmer Al.",
    "\"Topsoil's all gone, ma,\" weeps Lil' Greg.",
    "This is a large brown bear. Oddly enough, it's currently peeing in the woods.",
    "A team of arctic explorers is camped here.",
    "This object here appears to be Louis Farrakhan's bow tie.",
    "This is the world-famous Chain of Jockstraps.",
    "A trash compactor, compacting away.",
    "This toaster strudel is riddled with bullet holes!",
    "It's a hologram of a crashed helicopter.",
    "This is a television. On screen you see a robot strangely similar to yourself.",
    "This balogna has a first name, it's R-A-N-C-I-D.",
    "A salmon hatchery? Look again. It's merely a single salmon.",
    "It's a rim shot. Ba-da-boom!",
    "It's creepy and it's kooky, mysterious and spooky. It's also somewhat ooky.",
    "This is an anagram.",
    "This object is like an analogy.",
    "It's a symbol. You see in it a model for all symbols everywhere.",
    "The object pushes back at you.",
    "A traffic signal. It appears to have been recently vandalized.",
    "\"There is no kitten!\" cackles the old crone. You are shocked by her blasphemy.",
    "This is a Lagrange point. Don't come too close now.",
    "The dirty old tramp bemoans the loss of his harmonica.",
    "Look, it's Fanny the Irishman!",
    "What in blazes is this?",
    "It's the instruction manual for a previous version of this game.",
    "A brain cell. Oddly enough, it seems to be functioning.",
    "Tea and/or crumpets.",
    "This jukebox has nothing but Cliff Richards albums in it.",
    "It's a Quaker Oatmeal tube, converted into a drum.",
    "This is a remote control. Being a robot, you keep a wide berth.",
    "It's a roll of industrial-strength copper wire.",
    "Oh boy! Grub! Er, grubs.",
    "A puddle of mud, where the mudskippers play.",
    "Plenty of nothing.",
    "Look at that, it's the Crudmobile.",
    "Just Walter Mattheau and Jack Lemmon.",
    "Two crepes, two crepes in a box.",
    "An autographed copy of \"Primary Colors\", by Anonymous.",
    "Another rabbit? That's three today!",
    "It's a segmentation fault. Core dumped, by the way.",
    "A historical marker showing the actual location of /dev/null.",
    "Thar's Mobius Dick, the convoluted whale. Arrr!",
    "It's a charcoal briquette, smoking away.",
    "A pizza, melting in the sun.",
    "It's a \"HOME ALONE 2: Lost in New York\" novelty cup.",
    "A stack of 7 inch floppies wobbles precariously.",
    "It's nothing but a corrupted floppy. Coaster anyone?",
    "A section of glowing phosphor cells sings a song of radiation to you.",
    "This TRS-80 III is eerily silent.",
    "A toilet bowl occupies this space.",
    "This peg-leg is stuck in a knothole!",
    "It's a solitary vacuum tube.",
    "This corroded robot is clutching a mitten.",
    "\"Hi, I'm Anson Williams, TV's 'Potsy'.\"",
    "This subwoofer was blown out in 1974.",
    "Three half-pennies and a wooden nickel.",
    "It's the missing chapter to \"A Clockwork Orange\".",
    "It's a burrito stand flyer. \"Taqueria El Ranchito\".",
    "This smiling family is happy because they eat LARD.",
    "Roger Avery, persona un famoso de los Estados Unidos.",
    "Ne'er but a potted plant.",
    "A parrot, kipping on its back.",
    "A forgotten telephone switchboard.",
    "A forgotten telephone switchboard operator.",
    "It's an automated robot-disdainer. It pretends you're not there.",
    "It's a portable hole. A sign reads: \"Closed for the winter\".",
    "Just a moldy loaf of bread.",
    "A little glass tub of Carmex. ($.89) Too bad you have no lips.",
    "A Swiss-Army knife. All of its appendages are out. (toothpick lost)",
    "It's a zen simulation, trapped within an ASCII character.",
    "It's a copy of \"The Rubaiyat of Spike Schudy\".",
    "It's \"War and Peace\" (unabridged, very small print).",
    "A willing, ripe tomato bemoans your inability to digest fruit.",
    "A robot comedian. You feel amused.",
    "It's KITT, the talking car.",
    "Here's Pete Peterson. His batteries seem to have long gone dead.",
    "\"Blup, blup, blup\", says the mud pot.",
    "More grist for the mill.",
    "Grind 'em up, spit 'em out, they're twigs.",
    "The boom box cranks out an old Ethel Merman tune.",
    "It's \"Finding kitten\", published by O'Reilly and Associates.",
    "Pumpkin pie spice.",
    "It's the Bass-Matic '76! Mmm, that's good bass!",
    "\"Lend us a fiver 'til Thursday\", pleas Andy Capp.",
    "It's a tape of '70s rock. All original hits! All original artists!",
    "You've found the fabled America Online disk graveyard!",
    "Empty jewelboxes litter the landscape.",
    "It's the astounding meta-object.",
    "Ed McMahon stands here, lost in thought. Seeing you, he bellows, \"YES SIR!\"",
    "...thingy???",
    "It's 1000 secrets the government doesn't want you to know!",
    "The letters O and R.",
    "A magical... magic thing.",
  };
  if (idx < 0 || idx >= nummessages) {
     return std::string("It is SOFTWARE BUG.");
  } else {
   return std::string(rfimessages[idx]);
  }
};

robot_finds_kitten::robot_finds_kitten(WINDOW *w)
{
    ret = false;
    char ktile[83] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!#&()*+./:;=?![]{|}y";
    int used_messages[MAXMESSAGES];

    rfkLINES = 20;
    rfkCOLS = 60;

    const int numbogus = 20;
    const int maxcolor = 15;
    nummessages = 201;
    empty.x = -1;
    empty.y = -1;
    empty.color = (nc_color)0;
    empty.character = ' ';
    for (int c = 0; c < rfkCOLS; c++) {
        for (int c2 = 0; c2 < rfkLINES; c2++) {
            rfkscreen[c][c2] = EMPTY;
        }
    }
    /* Create an array to ensure we don't get duplicate messages. */
    for (int c = 0; c < nummessages; c++) {
        used_messages[c] = 0;
        bogus_messages[c] = 0;
        bogus[c] = empty;
    }
    /* Now we initialize the various game OBJECTs.
       * Assign a position to the player. */
    robot.x = rand() % rfkCOLS;
    robot.y = rand() % (rfkLINES - 3) + 3;
    robot.character = '#';
    robot.color = int_to_color(1);
    rfkscreen[robot.x][robot.y] = ROBOT;

    /* Assign the kitten a unique position. */
    do {
        kitten.x = rand() % rfkCOLS;
        kitten.y = rand() % (rfkLINES - 3) + 3;
    } while (rfkscreen[kitten.x][kitten.y] != EMPTY);

    /* Assign the kitten a character and a color. */
    do {
        kitten.character = ktile[rand() % 82];
    } while (kitten.character == '#' || kitten.character == ' ');
    kitten.color = int_to_color( ( rand() % (maxcolor - 2) ) + 2);
    rfkscreen[kitten.x][kitten.y] = KITTEN;

    /* Now, initialize non-kitten OBJECTs. */
    for (int c = 0; c < numbogus; c++) {
        /* Assign a unique position. */
        do {
            bogus[c].x = rand() % rfkCOLS;
            bogus[c].y = (rand() % (rfkLINES - 3)) + 3;
        } while (rfkscreen[bogus[c].x][bogus[c].y] != EMPTY);
        rfkscreen[bogus[c].x][bogus[c].y] = c + 2;

        /* Assign a character. */
        do {
            bogus[c].character = ktile[rand() % 82];
        } while (bogus[c].character == '#' || bogus[c].character == ' ');
        bogus[c].color = int_to_color((rand() % (maxcolor - 2)) + 2);

        /* Assign a unique message. */
        int index = 0;
        do {
            index = rand() % nummessages;
        } while (used_messages[index] != 0);
        bogus_messages[c] = index;
        used_messages[index] = 1;
    }

    instructions(w);

    werase(w);
    mvwprintz (w, 0, 0, c_white, "robotfindskitten v22July2008");
    for (int c = 0; c < rfkCOLS; c++) {
        mvwputch (w, 2, c, c_white, '_');
    }
    wmove(w, kitten.y, kitten.x);
    draw_kitten(w);

    for (int c = 0; c < numbogus; c++) {
        mvwputch(w, bogus[c].y, bogus[c].x, bogus[c].color, bogus[c].character);
    }

    wmove(w, robot.y, robot.x);
    draw_robot(w);
    int old_x = robot.x;
    int old_y = robot.y;

    wrefresh(w);
    /* Now the fun begins. */
    int input = '.';
    input = getch();

    while (input != 'q') {
        process_input(input, w);
        if(ret == true) {
            break;
        }
        /* Redraw robot, where avaliable */
        if (!(old_x == robot.x && old_y == robot.y)) {
            wmove(w, old_y, old_x);
            wputch(w, c_white, ' ');
            wmove(w, robot.y, robot.x);
            draw_robot(w);
            rfkscreen[old_x][old_y] = EMPTY;
            rfkscreen[robot.x][robot.y] = ROBOT;
            old_x = robot.x;
            old_y = robot.y;
            wrefresh(w);
        }
        input = getch();
    }
}

void robot_finds_kitten::instructions(WINDOW *w)
{
    mvwprintw (w,0,0,"robotfindskitten v22July2008\n"
    "Originally by the illustrious Leonard Richardson\n"
    "ReWritten in PDCurses by Joseph Larson,\n"
    "Ported to CDDA gaming system by a nutcase.\n"
    "In this game, you are robot (");
    draw_robot(w);
    wprintw (w,"). Your job is to find kitten.\n"
    "This task is complicated by the existance of various\n"
    "things which are not kitten. Robot must touch items\n"
    "to determine if they are kitten or not. The game\n"
    "ends when robotfindskitten. Alternatively, you\n"
    "may end the game by hitting 'q'\n"
    "   Press any key to start.\n");
    wrefresh(w);
    getch();
}

void robot_finds_kitten::process_input(int input, WINDOW *w)

{
    timespec ts;
    ts.tv_sec = 1;
    ts.tv_nsec = 0;

    int check_x = robot.x;
    int check_y = robot.y;

    switch (input) {
        case KEY_UP: /* up */
            check_y--;
            break;
        case KEY_DOWN: /* down */
            check_y++;
            break;
        case KEY_LEFT: /* left */
            check_x--;
            break;
        case KEY_RIGHT: /* right */
            check_x++;
            break;
        case 0:
            break;
        default: { /* invalid command */
            for (int c = 0; c < rfkCOLS; c++) {
                mvwputch (w, 0, c, c_white, ' ');
                mvwputch (w, 1, c, c_white, ' ');
            }
            mvwprintz (w, 0, 0, c_white, "Invalid command: Use direction keys or Press 'q'.");
            return;
        }
    }

    if (check_y < 3 || check_y > rfkLINES - 1 || check_x < 0 || check_x > rfkCOLS - 1) {
        return;
    }

    if (rfkscreen[check_x][check_y] != EMPTY) {
        switch (rfkscreen[check_x][check_y]) {
            case ROBOT:
                /* We didn't move. */
                break;
            case KITTEN: {/* Found it! */
                for (int c = 0; c < rfkCOLS; c++) {
                    mvwputch (w, 0, c, c_white, ' ');
                }

                /* The grand cinema scene. */
                for (int c = 0; c <= 3; c++) {

                    wmove(w, 1, (rfkCOLS / 2) - 5 + c);
                    wputch(w, c_white, ' ');
                    wmove(w, 1, (rfkCOLS / 2) + 4 - c);
                    wputch(w, c_white, ' ');
                    wmove(w, 1, (rfkCOLS / 2) - 4 + c);
                    if (input == KEY_LEFT || input == KEY_UP) {
                        draw_kitten(w);
                    } else {
                        draw_robot(w);
                    }
                    wmove(w, 1, (rfkCOLS / 2) + 3 - c);
                    if (input == KEY_LEFT || input == KEY_UP) {
                        draw_robot(w);
                    } else {
                        draw_kitten(w);
                    }
                    wrefresh (w);
                    nanosleep(&ts, NULL);
                }

                /* They're in love! */
                mvwprintz(w, 0, ((rfkCOLS - 6) / 2) - 1, c_ltred, "<3<3<3");
                wrefresh(w);
                nanosleep(&ts, NULL);
                for (int c = 0; c < rfkCOLS; c++) {
                    mvwputch (w, 0, c, c_white, ' ');
                    mvwputch (w, 1, c, c_white, ' ');
                }
                mvwprintz (w, 0, 0, c_white, "You found kitten! Way to go, robot!");
                wrefresh(w);
                ret = true;
                int ech=input;
                do {
                    ech=getch ();
                } while ( ech == input );
            }
            break;

            default: {
                for (int c = 0; c < rfkCOLS; c++) {
                    mvwputch (w, 0, c, c_white, ' ');
                    mvwputch (w, 1, c, c_white, ' ');
                }
                std::vector<std::string> bogusvstr = foldstring( getmessage(bogus_messages[rfkscreen[check_x][check_y] - 2]), rfkCOLS);
                for (int c = 0; c < bogusvstr.size(); c++) {
                    mvwprintw (w, 0, 0, "%s", bogusvstr[c].c_str());
                }
                wrefresh(w);
            }
            break;
        }
        wmove(w, 2, 0);
        return;
    }
    /* Otherwise, move the robot. */
    robot.x = check_x;
    robot.y = check_y;
}
/////////////////////////////////
void robot_finds_kitten::draw_robot(WINDOW *w)  /* Draws robot at current position */
{
    wputch(w, robot.color, robot.character);
}

void robot_finds_kitten::draw_kitten(WINDOW *w)  /* Draws kitten at current position */
{
    wputch(w, kitten.color, kitten.character);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
