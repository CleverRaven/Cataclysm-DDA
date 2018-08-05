#include "iuse_software_kitten.h"

#include "output.h"
#include "translations.h"
#include "posix_time.h"
#include "cursesdef.h"
#include "input.h"

#include <cstdlib>  // Needed for rand()
#include <iostream>

#define EMPTY -1
#define ROBOT 0
#define KITTEN 1
std::string robot_finds_kitten::getmessage( int idx )
{
    std::string rfimessages[MAXMESSAGES] = {
        _( "\"I pity the fool who mistakes me for kitten!\", sez Mr. T." ),
        _( "That's just an old tin can." ),
        _( "It's an altar to the horse god." ),
        _( "A box of dancing mechanical pencils. They dance! They sing!" ),
        _( "It's an old Duke Ellington record." ),
        _( "A box of fumigation pellets." ),
        _( "A digital clock. It's stuck at 2:17 PM." ),
        _( "That's just a charred human corpse." ),
        _( "I don't know what that is, but it's not kitten." ),
        _( "An empty shopping bag. Paper or plastic?" ),
        _( "Could it be... a big ugly bowling trophy?" ),
        _( "A coat hanger hovers in thin air. Odd." ),
        _( "Not kitten, just a packet of Kool-Aid(tm)." ),
        _( "A freshly-baked pumpkin pie." ),
        _( "A lone, forgotten comma, sits here, sobbing." ),
        _( "ONE HUNDRED THOUSAND CARPET FIBERS!" ),
        _( "It's Richard Nixon's nose!" ),
        _( "It's Lucy Ricardo. \"Aaaah, Ricky!\", she says." ),
        _( "You stumble upon Bill Gates' stand-up act." ),
        _( "Just an autographed copy of the Kama Sutra." ),
        _( "It's the Will Rogers Highway. Who was Will Rogers, anyway?" ),
        _( "It's another robot, more advanced in design than you but strangely immobile." ),
        _( "Leonard Richardson is here, asking people to lick him." ),
        _( "It's a stupid mask, fashioned after a beagle." ),
        _( "Your State Farm Insurance(tm) representative!" ),
        _( "It's the local draft board." ),
        _( "Seven 1/4\" screws and a piece of plastic." ),
        _( "An 80286 machine." ),
        _( "One of those stupid \"Homes of the Stars\" maps." ),
        _( "A signpost saying \"TO KITTEN\". It points in no particular direction." ),
        _( "A hammock stretched between a tree and a volleyball pole." ),
        _( "A Texas Instruments of Destruction calculator." ),
        _( "It's a dark, amorphous blob of matter." ),
        _( "Just a pincushion." ),
        _( "It's a mighty zombie talking about some love and prosperity." ),
        _( "\"Dear robot, you may have already won our 10 MILLION DOLLAR prize...\"" ),
        _( "It's just an object." ),
        _( "A mere collection of pixels." ),
        _( "A badly dented high-hat cymbal lies on its side here." ),
        _( "A marijuana brownie." ),
        _( "A plush Chewbacca." ),
        _( "Daily hunger conditioner from Australasia" ),
        _( "Just some stuff." ),
        _( "Why are you touching this when you should be finding kitten?" ),
        _( "A glorious fan of peacock feathers." ),
        _( "It's some compromising photos of Babar the Elephant." ),
        _( "A copy of the Weekly World News. Watch out for the chambered nautilus!" ),
        _( "It's the proverbial wet blanket." ),
        _( "A \"Get Out of Jail Free\" card." ),
        _( "An incredibly expensive \"Mad About You\" collector plate." ),
        _( "Paul Moyer's necktie." ),
        _( "A haircut and a real job. Now you know where to get one!" ),
        _( "An automated robot-hater. It frowns disapprovingly at you." ),
        _( "An automated robot-liker. It smiles at you." ),
        _( "It's a black hole. Don't fall in!" ),
        _( "Just a big brick wall." ),
        _( "You found kitten! No, just kidding." ),
        _( "Heart of Darkness brand pistachio nuts." ),
        _( "A smoking branding iron shaped like a 24-pin connector." ),
        _( "It's a Java applet." ),
        _( "An abandoned used-car lot." ),
        _( "A shameless plug for Crummy: http://www.crummy.com/" ),
        _( "A shameless plug for the UCLA Linux Users Group: http://linux.ucla.edu/" ),
        _( "A can of Spam Lite." ),
        _( "This is another fine mess you've gotten us into, Stanley." ),
        _( "It's scenery for \"Waiting for Godot\"." ),
        _( "This grain elevator towers high above you." ),
        _( "A Mentos wrapper." ),
        _( "It's the constellation Pisces." ),
        _( "It's a fly on the wall. Hi, fly!" ),
        _( "This kind of looks like kitten, but it's not." ),
        _( "It's a banana! Oh, joy!" ),
        _( "A helicopter has crashed here." ),
        _( "Carlos Tarango stands here, doing his best impression of Pat Smear." ),
        _( "A patch of mushrooms grows here." ),
        _( "A patch of grape jelly grows here." ),
        _( "A spindle, and a grindle, and a bucka-wacka-woom!" ),
        _( "A geyser sprays water high into the air." ),
        _( "A toenail? What good is a toenail?" ),
        _( "You've found the fish! Not that it does you much good in this game." ),
        _( "A Buttertonsils bar." ),
        _( "One of the few remaining discoes." ),
        _( "Ah, the uniform of a Revolutionary-era minuteman." ),
        _( "A punch bowl, filled with punch and lemon slices." ),
        _( "It's nothing but a G-thang, baby." ),
        _( "IT'S ALIVE! AH HA HA HA HA!" ),
        _( "This was no boating accident!" ),
        _( "Wait! This isn't the poker chip! You've been tricked! DAMN YOU, MENDEZ!" ),
        _( "A livery stable! Get your livery!" ),
        _( "It's a perpetual immobility machine." ),
        _( "\"On this spot in 1962, Henry Winkler was sick.\"" ),
        _( "There's nothing here; it's just an optical illusion." ),
        _( "The World's Biggest Motzah Ball!" ),
        _( "A tribe of cannibals lives here. They eat Malt-O-Meal for breakfast, you know." ),
        _( "This appears to be a rather large stack of trashy romance novels." ),
        _( "Look out! Exclamation points!" ),
        _( "A herd of wild coffee mugs slumbers here." ),
        _( "It's a limbo bar! How low can you go?" ),
        _( "It's the horizon. Now THAT'S weird." ),
        _( "A vase full of artificial flowers is stuck to the floor here." ),
        _( "A large snake bars your way." ),
        _( "A pair of saloon-style doors swing slowly back and forth here." ),
        _( "It's an ordinary bust of Beethoven... but why is it painted green?" ),
        _( "It's TV's lovable wisecracking Crow! \"Bite me!\", he says." ),
        _( "Hey, look, it's war. What is it good for? Absolutely nothing. Say it again." ),
        _( "It's the amazing self-referential thing that's not kitten." ),
        _( "A flamboyant feather boa. Now you can dress up like Carol Channing!" ),
        _( "\"Sure hope we get some rain soon,\" says Farmer Joe." ),
        _( "\"How in heck can I wash my neck if it ain't gonna rain no more?\" asks Farmer Al." ),
        _( "\"Topsoil's all gone, ma,\" weeps Lil' Greg." ),
        _( "This is a large brown bear. Oddly enough, it's currently peeing in the woods." ),
        _( "A team of arctic explorers is camped here." ),
        _( "This object here appears to be Louis Farrakhan's bow tie." ),
        _( "This is the world-famous Chain of Jockstraps." ),
        _( "A trash compactor, compacting away." ),
        _( "This toaster strudel is riddled with bullet holes!" ),
        _( "It's a hologram of a crashed helicopter." ),
        _( "This is a television. On screen you see a robot strangely similar to yourself." ),
        _( "This balogna has a first name, it's R-A-N-C-I-D." ),
        _( "A salmon hatchery? Look again. It's merely a single salmon." ),
        _( "It's a rim shot. Ba-da-boom!" ),
        _( "It's creepy and it's kooky, mysterious and spooky. It's also somewhat ooky." ),
        _( "This is an anagram." ),
        _( "This object is like an analogy." ),
        _( "It's a symbol. You see in it a model for all symbols everywhere." ),
        _( "The object pushes back at you." ),
        _( "A traffic signal. It appears to have been recently vandalized." ),
        _( "\"There is no kitten!\" cackles the old crone. You are shocked by her blasphemy." ),
        _( "This is a Lagrange point. Don't come too close now." ),
        _( "The dirty old tramp bemoans the loss of his harmonica." ),
        _( "Look, it's Fanny the Irishman!" ),
        _( "What in blazes is this?" ),
        _( "It's the instruction manual for a previous version of this game." ),
        _( "A brain cell. Oddly enough, it seems to be functioning." ),
        _( "Tea and/or crumpets." ),
        _( "This jukebox has nothing but Cliff Richards albums in it." ),
        _( "It's a Quaker Oatmeal tube, converted into a drum." ),
        _( "This is a remote control. Being a robot, you keep a wide berth." ),
        _( "It's a roll of industrial-strength copper wire." ),
        _( "Oh boy! Grub! Er, grubs." ),
        _( "A puddle of mud, where the mudskippers play." ),
        _( "Plenty of nothing." ),
        _( "Look at that, it's the Crudmobile." ),
        _( "Just Walter Mattheau and Jack Lemmon." ),
        _( "Two crepes, two crepes in a box." ),
        _( "An autographed copy of \"Primary Colors\", by Anonymous." ),
        _( "Another rabbit? That's three today!" ),
        _( "It's a segmentation fault. Core dumped, by the way." ),
        _( "A historical marker showing the actual location of /dev/null." ),
        _( "Thar's Mobius Dick, the convoluted whale. Arrr!" ),
        _( "It's a charcoal briquette, smoking away." ),
        _( "A pizza, melting in the sun." ),
        _( "It's a \"HOME ALONE 2: Lost in New York\" novelty cup." ),
        _( "A stack of 7 inch floppies wobbles precariously." ),
        _( "It's nothing but a corrupted floppy. Coaster anyone?" ),
        _( "A section of glowing phosphor cells sings a song of radiation to you." ),
        _( "This TRS-80 III is eerily silent." ),
        _( "A toilet bowl occupies this space." ),
        _( "This peg-leg is stuck in a knothole!" ),
        _( "It's a solitary vacuum tube." ),
        _( "This corroded robot is clutching a mitten." ),
        _( "\"Hi, I'm Anson Williams, TV's 'Potsy'.\"" ),
        _( "This subwoofer was blown out in 1974." ),
        _( "Three half-pennies and a wooden nickel." ),
        _( "It's the missing chapter to \"A Clockwork Orange\"." ),
        _( "It's a burrito stand flyer. \"Taqueria El Ranchito\"." ),
        _( "This smiling family is happy because they eat LARD." ),
        _( "Roger Avery, persona un famoso de los Estados Unidos." ),
        _( "Ne'er but a potted plant." ),
        _( "A parrot, kipping on its back." ),
        _( "A forgotten telephone switchboard." ),
        _( "A forgotten telephone switchboard operator." ),
        _( "It's an automated robot-disdainer. It pretends you're not there." ),
        _( "It's a portable hole. A sign reads: \"Closed for the winter\"." ),
        _( "Just a moldy loaf of bread." ),
        _( "A little glass tub of Carmex. ($.89) Too bad you have no lips." ),
        _( "A Swiss-Army knife. All of its appendages are out. (toothpick lost)" ),
        _( "It's a zen simulation, trapped within an ASCII character." ),
        _( "It's a copy of \"The Rubaiyat of Spike Schudy\"." ),
        _( "It's \"War and Peace\" (unabridged, very small print)." ),
        _( "A willing, ripe tomato bemoans your inability to digest fruit." ),
        _( "A robot comedian. You feel amused." ),
        _( "It's KITT, the talking car." ),
        _( "Here's Pete Peterson. His batteries seem to have long gone dead." ),
        _( "\"Blup, blup, blup\", says the mud pot." ),
        _( "More grist for the mill." ),
        _( "Grind 'em up, spit 'em out, they're twigs." ),
        _( "The boom box cranks out an old Ethel Merman tune." ),
        _( "It's \"Finding kitten\", published by O'Reilly and Associates." ),
        _( "Pumpkin pie spice." ),
        _( "It's the Bass-Matic '76! Mmm, that's good bass!" ),
        _( "\"Lend us a fiver 'til Thursday\", pleas Andy Capp." ),
        _( "It's a tape of '70s rock. All original hits! All original artists!" ),
        _( "You've found the fabled America Online disk graveyard!" ),
        _( "Empty jewelboxes litter the landscape." ),
        _( "It's the astounding meta-object." ),
        _( "Ed McMahon stands here, lost in thought. Seeing you, he bellows, \"YES SIR!\"" ),
        _( "...thingy???" ),
        _( "It's 1000 secrets the government doesn't want you to know!" ),
        _( "The letters O and R." ),
        _( "A magical... magic thing." ),
    };
    if( idx < 0 || idx >= nummessages ) {
        return std::string( _( "It is SOFTWARE BUG." ) );
    } else {
        return std::string( rfimessages[idx] );
    }
}

robot_finds_kitten::robot_finds_kitten( const catacurses::window &w )
{
    ret = false;
    char ktile[83] =
        "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!#&()*+./:;=?![]{|}y";
    int used_messages[MAXMESSAGES];

    rfkLINES = 20;
    rfkCOLS = 60;

    const int numbogus = 20;
    nummessages = 201;
    empty.x = -1;
    empty.y = -1;
    empty.color = nc_color();
    empty.character = ' ';
    for( int c = 0; c < rfkCOLS; c++ ) {
        for( int c2 = 0; c2 < rfkLINES; c2++ ) {
            rfkscreen[c][c2] = EMPTY;
        }
    }
    /* Create an array to ensure we don't get duplicate messages. */
    for( int c = 0; c < nummessages; c++ ) {
        used_messages[c] = 0;
        bogus_messages[c] = 0;
        bogus[c] = empty;
    }
    /* Now we initialize the various game OBJECTs.
       * Assign a position to the player. */
    robot.x = rand() % rfkCOLS;
    robot.y = rand() % ( rfkLINES - 3 ) + 3;
    robot.character = '#';
    robot.color = c_white;
    rfkscreen[robot.x][robot.y] = ROBOT;

    /* Assign the kitten a unique position. */
    do {
        kitten.x = rand() % rfkCOLS;
        kitten.y = rand() % ( rfkLINES - 3 ) + 3;
    } while( rfkscreen[kitten.x][kitten.y] != EMPTY );

    /* Assign the kitten a character and a color. */
    do {
        kitten.character = ktile[rand() % 82];
    } while( kitten.character == '#' || kitten.character == ' ' );

    do {
        kitten.color = all_colors.get_random();
    } while( kitten.color == c_black );

    rfkscreen[kitten.x][kitten.y] = KITTEN;

    /* Now, initialize non-kitten OBJECTs. */
    for( int c = 0; c < numbogus; c++ ) {
        /* Assign a unique position. */
        do {
            bogus[c].x = rand() % rfkCOLS;
            bogus[c].y = ( rand() % ( rfkLINES - 3 ) ) + 3;
        } while( rfkscreen[bogus[c].x][bogus[c].y] != EMPTY );
        rfkscreen[bogus[c].x][bogus[c].y] = c + 2;

        /* Assign a character. */
        do {
            bogus[c].character = ktile[rand() % 82];
        } while( bogus[c].character == '#' || bogus[c].character == ' ' );

        do {
            bogus[c].color = all_colors.get_random();
        } while( bogus[c].color == c_black );

        /* Assign a unique message. */
        int index = 0;
        do {
            index = rand() % nummessages;
        } while( used_messages[index] != 0 );
        bogus_messages[c] = index;
        used_messages[index] = 1;
    }

    instructions( w );

    werase( w );
    mvwprintz( w, 0, 0, c_white, _( "robotfindskitten v22July2008 - press q to quit." ) );
    for( int c = 0; c < rfkCOLS; c++ ) {
        mvwputch( w, 2, c, BORDER_COLOR, '_' );
    }
    wmove( w, kitten.y, kitten.x );
    draw_kitten( w );

    for( int c = 0; c < numbogus; c++ ) {
        mvwputch( w, bogus[c].y, bogus[c].x, bogus[c].color, bogus[c].character );
    }

    wmove( w, robot.y, robot.x );
    draw_robot( w );
    int old_x = robot.x;
    int old_y = robot.y;

    wrefresh( w );
    /* Now the fun begins. */
    int input = inp_mngr.get_input_event().get_first_input(); // @todo: use input context

    while( input != 'q' && input != 'Q' && input != KEY_ESCAPE ) {
        process_input( input, w );
        if( ret ) {
            break;
        }
        /* Redraw robot, where available */
        if( !( old_x == robot.x && old_y == robot.y ) ) {
            wmove( w, old_y, old_x );
            wputch( w, c_white, ' ' );
            wmove( w, robot.y, robot.x );
            draw_robot( w );
            rfkscreen[old_x][old_y] = EMPTY;
            rfkscreen[robot.x][robot.y] = ROBOT;
            old_x = robot.x;
            old_y = robot.y;
        }
        wrefresh( w );
        // TODO: use input context / rewrite loop so this is only called at one place
        input = inp_mngr.get_input_event().get_first_input();
    }
}

void robot_finds_kitten::instructions( const catacurses::window &w )
{
    int pos = 1;
    pos += fold_and_print( w, 0, 1, getmaxx( w ) - 4, c_light_gray,
                           _( "robotfindskitten v22July2008" ) );
    pos += 1 + fold_and_print( w, pos, 1, getmaxx( w ) - 4, c_light_gray, _( "\
Originally by the illustrious Leonard Richardson, \
rewritten in PDCurses by Joseph Larson, \
ported to CDDA gaming system by a nutcase." ) );

    pos += 1 + fold_and_print( w, pos, 1, getmaxx( w ) - 4, c_light_gray,
                               _( "In this game, you are robot (" ) );
    draw_robot( w );
    wprintz( w, c_light_gray, _( ")." ) );
    pos += 1 + fold_and_print( w, pos, 1, getmaxx( w ) - 4, c_light_gray, _( "\
Your job is to find kitten. This task is complicated by the existence of various things \
which are not kitten. Robot must touch items to determine if they are kitten or not. \
The game ends when robot finds kitten. Alternatively, you may end the game by hitting \
'q', 'Q' or the Escape key." ) );
    fold_and_print( w, pos, 1, getmaxx( w ) - 4, c_light_gray, _( "Press any key to start." ) );
    wrefresh( w );
    inp_mngr.wait_for_any_key();
}

void robot_finds_kitten::process_input( int input, const catacurses::window &w )
{
    timespec ts;
    ts.tv_sec = 1;
    ts.tv_nsec = 0;

    int check_x = robot.x;
    int check_y = robot.y;

    switch( input ) {
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
            for( int c = 0; c < rfkCOLS; c++ ) {
                mvwputch( w, 0, c, c_white, ' ' );
                mvwputch( w, 1, c, c_white, ' ' );
            }
            mvwprintz( w, 0, 0, c_white, _( "Invalid command: Use direction keys or press 'q'." ) );
            return;
        }
    }

    if( check_y < 3 || check_y > rfkLINES - 1 || check_x < 0 || check_x > rfkCOLS - 1 ) {
        return;
    }

    if( rfkscreen[check_x][check_y] != EMPTY ) {
        switch( rfkscreen[check_x][check_y] ) {
            case ROBOT:
                /* We didn't move. */
                break;
            case KITTEN: {/* Found it! */
                for( int c = 0; c < rfkCOLS; c++ ) {
                    mvwputch( w, 0, c, c_white, ' ' );
                }

                /* The grand cinema scene. */
                for( int c = 0; c <= 3; c++ ) {

                    wmove( w, 1, ( rfkCOLS / 2 ) - 5 + c );
                    wputch( w, c_white, ' ' );
                    wmove( w, 1, ( rfkCOLS / 2 ) + 4 - c );
                    wputch( w, c_white, ' ' );
                    wmove( w, 1, ( rfkCOLS / 2 ) - 4 + c );
                    if( input == KEY_LEFT || input == KEY_UP ) {
                        draw_kitten( w );
                    } else {
                        draw_robot( w );
                    }
                    wmove( w, 1, ( rfkCOLS / 2 ) + 3 - c );
                    if( input == KEY_LEFT || input == KEY_UP ) {
                        draw_robot( w );
                    } else {
                        draw_kitten( w );
                    }
                    wrefresh( w );
                    nanosleep( &ts, NULL );
                }

                /* They're in love! */
                mvwprintz( w, 0, ( ( rfkCOLS - 6 ) / 2 ) - 1, c_light_red, "<3<3<3" );
                wrefresh( w );
                nanosleep( &ts, NULL );
                for( int c = 0; c < rfkCOLS; c++ ) {
                    mvwputch( w, 0, c, c_white, ' ' );
                    mvwputch( w, 1, c, c_white, ' ' );
                }
                mvwprintz( w, 0, 0, c_white, _( "You found kitten! Way to go, robot!" ) );
                wrefresh( w );
                ret = true;
                int ech = input;
                do {
                    // TODO: use input context
                    ech = inp_mngr.get_input_event().get_first_input();
                } while( ech == input );
            }
            break;

            default: {
                for( int c = 0; c < rfkCOLS; c++ ) {
                    mvwputch( w, 0, c, c_white, ' ' );
                    mvwputch( w, 1, c, c_white, ' ' );
                }
                std::vector<std::string> bogusvstr = foldstring( getmessage(
                        bogus_messages[rfkscreen[check_x][check_y] - 2] ), rfkCOLS );
                for( size_t c = 0; c < bogusvstr.size(); c++ ) {
                    mvwprintw( w, c, 0, bogusvstr[c] );
                }
                wrefresh( w );
            }
            break;
        }
        wmove( w, 2, 0 );
        return;
    }
    /* Otherwise, move the robot. */
    robot.x = check_x;
    robot.y = check_y;
}

void robot_finds_kitten::draw_robot( const catacurses::window
                                     &w )  /* Draws robot at current position */
{
    wputch( w, robot.color, robot.character );
}

void robot_finds_kitten::draw_kitten( const catacurses::window
                                      &w )  /* Draws kitten at current position */
{
    wputch( w, kitten.color, kitten.character );
}
